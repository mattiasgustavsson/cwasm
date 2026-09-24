#ifndef jsgen_h
#define jsgen_h

// JS generation, cwasm.js bundle slicing and {{{marker}}} splicing

static size_t js_body_size( char const* str, size_t n ) {
    size_t size = 0;
    for( size_t i = 0; i < n; ++i ) {
        unsigned char c = (unsigned char)str[ i ];
        size += c == '\\' || c == '\"' ? 2 : c < 32 || c == '<' ? 4 : 1;
    }
    return size;
}


static size_t js_body_write( char* dst, char const* str, size_t n ) {
    static char const hx[] = "0123456789ABCDEF";
    size_t k = 0;
    for( size_t i = 0; i < n; ++i ) {
        unsigned char c = (unsigned char)str[ i ];
        if( c == '\\' || c == '\"' ) {
            dst[ k++ ] = '\\';
            dst[ k++ ] = (char)c;
        } else if( c < 32 || c == '<' ) {
            dst[ k++ ] = '\\';
            dst[ k++ ] = 'x';
            dst[ k++ ] = hx[ ( c >> 4 ) & 15 ];
            dst[ k++ ] = hx[ c & 15 ];
        } else {
            dst[ k++ ] = (char)c;
        }
    }
    return k;
}


static int buf_js_string( buf_t* b, char const* str, size_t n ) {
    size_t body = js_body_size( str, n );
    if( b->failed || !b->p || body + 2 > b->cap - b->n ) {
        b->failed = 1;
        return 0;
    }
    b->p[ b->n++ ] = '\"';
    b->n += js_body_write( (char*)b->p + b->n, str, n );
    b->p[ b->n++ ] = '\"';
    b->p[ b->n ] = '\0';
    return 1;
}


static int is_js_ident( char const* s ) {
    unsigned char c = (unsigned char)s[ 0 ];
    if( !( ( c == '$' ) || ( c == '_' ) || ( c >= 'A' && c <= 'Z' ) ||
        ( c >= 'a' && c <= 'z' ) ) ) {
        return 0;
    }
    for( size_t i = 1; s[ i ]; ++i ) {
        c = (unsigned char)s[ i ];
        if( !( ( c == '$' ) || ( c == '_' ) || ( c >= 'A' && c <= 'Z' ) ||
            ( c >= 'a' && c <= 'z' ) || ( c >= '0' && c <= '9' ) ) ) {
            return 0;
        }
    }
    return 1;
}


static int buf_js_key( buf_t* b, char const* key ) {
    return is_js_ident( key ) ? buf_str( b, key ) : buf_js_string( b, key, cstr_size( key ) );
}


static char const* find_sub( char const* h, size_t hn, char const* nd ) {
    size_t nn = strlen( nd );
    if( !nn || hn < nn ) { return NULL; }
    for( size_t i = 0; i + nn <= hn; ++i ) {
        if( !memcmp( h + i, nd, nn ) ) { return h + i; }
    }
    return NULL;
}


static size_t count_sub( char const* h, size_t hn, char const* nd ) {
    size_t nn = strlen( nd );
    size_t count = 0;
    for( char const* p = find_sub( h, hn, nd ); p;
        p = find_sub( p + nn, hn - (size_t)( p + nn - h ), nd ) ) {
        ++count;
    }
    return count;
}


typedef enum splice_mode_t {
    SPLICE_RAW,
    SPLICE_BODY,
    SPLICE_QUOTED,
} splice_mode_t;


static int splice( buf_t* b, char const* marker, char const* ins, size_t ins_n,
    splice_mode_t mode ) {

    size_t ml = strlen( marker );
    size_t ins_size = mode == SPLICE_RAW ? ins_n
        : js_body_size( ins, ins_n ) + ( mode == SPLICE_QUOTED ? 2 : 0 );
    size_t at = 0;
    for( ;; ) {
        char const* p = find_sub( (char const*)b->p + at, b->n - at, marker );
        if( !p ) { break; }
        at = (size_t)( p - (char const*)b->p );
        if( b->failed || b->n + ins_size > b->cap + ml ) {
            b->failed = 1;
            fprintf( stderr, "error: replacing %s\n", marker );
            return 0;
        }
        memmove( b->p + at + ins_size, b->p + at + ml, b->n - at - ml );
        char* dst = (char*)b->p + at;
        if( mode == SPLICE_RAW ) {
            memcpy( dst, ins, ins_n );
        } else {
            if( mode == SPLICE_QUOTED ) { *dst++ = '\"'; }
            dst += js_body_write( dst, ins, ins_n );
            if( mode == SPLICE_QUOTED ) { *dst = '\"'; }
        }
        b->n = b->n - ml + ins_size;
        b->p[ b->n ] = '\0';
        at += ins_size;
    }
    return 1;
}


static int splice_slice( buf_t* b, char const* marker, char const* src, size_t src_n,
    int as_literal ) {

    if( !find_sub( (char const*)b->p, b->n, marker ) ) { return 1; }
    if( !as_literal ) { return splice( b, marker, src, src_n, SPLICE_RAW ); }
    buf_t min = { 0 };
    int ok = minify_file( "js", src ? src : "", src_n, &min ) &&
        splice( b, marker, (char const*)min.p, min.n, SPLICE_QUOTED );
    buf_free( &min );
    return ok;
}


typedef struct lib_group_t {
    cstr_t lib;
    array_t(wfunc_t*)* funcs;
    array_t(cstr_t)* inits;
} lib_group_t;


static lib_group_t* lib_find_or_add( array_param_t(lib_group_t)* lvp, cstr_t libname ) {
    array_t(lib_group_t)* lv;
    array_from_param( lv, lvp );
    for( int i = 0; i < (int)lv->count; ++i ) {
        if( cstr_is_equal( lv->items[ i ].lib, libname ) ) { return &lv->items[ i ]; }
    }
    lib_group_t g;
    g.lib = libname;
    array_create( g.funcs );
    array_create( g.inits );
    array_add( lv, g );
    return &lv->items[ lv->count - 1 ];
}


static void lib_group_all( array_param_t(lib_group_t)* libsp, array_param_t(wfunc_t)* funcsp ) {
    array_t(wfunc_t)* funcs;
    array_from_param( funcs, funcsp );
    for( int i = 0; i < (int)funcs->count; ++i ) {
        wfunc_t* f = &funcs->items[ i ];
        if( !f->lib ) { f->lib = cstr( "" ); }
        lib_group_t* g = lib_find_or_add( libsp, f->lib );
        array_add( g->funcs, f );
        if( f->jsinit && f->jsinit[ 0 ] ) { array_add( g->inits, f->jsinit ); }
    }
}


static void lib_array_free( array_param_t(lib_group_t)* libsp ) {
    array_t(lib_group_t)* libs;
    array_from_param( libs, libsp );
    for( int i = 0; i < (int)libs->count; ++i ) {
        array_destroy( libs->items[ i ].funcs );
        array_destroy( libs->items[ i ].inits );
    }
    array_destroy( libs );
}


static size_t wfuncs_bound( array_param_t(wfunc_t)* funcsp ) {
    array_t(wfunc_t)* funcs;
    array_from_param( funcs, funcsp );
    size_t bound = 0;
    for( int i = 0; i < (int)funcs->count; ++i ) {
        wfunc_t* f = &funcs->items[ i ];
        bound += 4 * cstr_size( f->name ) + cstr_size( f->jsargs ) + cstr_size( f->jscode ) +
            cstr_size( f->jsinit ) + 64;
    }
    return bound;
}


static size_t imports_bound( array_param_t(import_rec_t)* importsp ) {
    array_t(import_rec_t)* imports;
    array_from_param( imports, importsp );
    size_t bound = 0;
    for( int i = 0; i < (int)imports->count; ++i ) {
        import_rec_t* r = &imports->items[ i ];
        bound += 4 * ( cstr_size( r->mod ) + cstr_size( r->fld ) ) + 256;
    }
    return bound;
}


static cstr_t gen_module_opts( array_param_t(import_rec_t)* importsp, int jspi ) {
    array_t(import_rec_t)* imports;
    array_from_param( imports, importsp );
    int cpp_exceptions = 0;
    int threads = imports_declare_threads( importsp );
    uint32_t mem_initial = 0;
    uint32_t mem_maximum = 0;
    int mem_shared = 0;
    for( int i = 0; i < (int)imports->count; ++i ) {
        import_rec_t* r = &imports->items[ i ];
        if( r->kind == 4 && cstr_is_equal( r->mod, "env" ) &&
            cstr_is_equal( r->fld, "__cpp_exception" ) ) {
            cpp_exceptions = 1;
        }
        if( r->kind == 2 && r->shared ) {
            mem_shared = 1;
            mem_initial = r->a;
            mem_maximum = r->has_b ? r->b : r->a;
        }
    }

    if( mem_shared ) {
        return cstr_format(
            "{cpp_exceptions:%s,jspi:%s,threads:%s,memory:{initial:%u,maximum:%u,shared:true}}",
            cpp_exceptions ? "true" : "false", jspi ? "true" : "false",
            threads ? "true" : "false", mem_initial,
            mem_maximum ? mem_maximum : mem_initial );
    }
    return cstr_format( "{cpp_exceptions:%s,jspi:%s,threads:%s}",
        cpp_exceptions ? "true" : "false", jspi ? "true" : "false",
        threads ? "true" : "false" );
}


static cstr_t gen_imports_expr( array_param_t(wfunc_t)* wfuncsp, array_param_t(wfunc_t)* mfuncsp,
    array_param_t(import_rec_t)* importsp, cstr_t* out_factory ) {

    array_t(wfunc_t)* wfuncs;
    array_t(wfunc_t)* mfuncs;
    array_t(import_rec_t)* imports;
    array_from_param( wfuncs, wfuncsp );
    array_from_param( mfuncs, mfuncsp );
    array_from_param( imports, importsp );
    *out_factory = NULL;
    array_t(lib_group_t)* libs;
    array_create( libs );
    lib_group_all( array_to_param( libs ), wfuncsp );

    buf_t s = { 0 };
    buf_init( &s, 1024 + wfuncs_bound( wfuncsp ) + wfuncs_bound( mfuncsp ) +
        imports_bound( importsp ) );
    buf_str( &s, "function __cwasm_build_imports(){\n" );

    buf_str( &s, "var JS={" );
    int any_js_props = 0;

    for( int li = 0; li < (int)libs->count; ++li ) {
        lib_group_t* g = &libs->items[ li ];
        if( g->inits->count != 0 ) { continue; }
        for( int fi = 0; fi < (int)g->funcs->count; ++fi ) {
            wfunc_t* f = g->funcs->items[ fi ];
            if( !any_js_props ) {
                buf_str( &s, "\n" );
                any_js_props = 1;
            }
            buf_str( &s, "  " );
            buf_js_key( &s, f->name );
            buf_str( &s, ":(" );
            buf_str( &s, f->jsargs ? f->jsargs : "" );
            buf_str( &s, ")=>" );
            buf_str( &s, f->jscode ? f->jscode : "0" );
            buf_str( &s, ",\n" );
        }
    }

    for( int i = 0; i < (int)imports->count; ++i ) {
        import_rec_t* r = &imports->items[ i ];
        if( r->kind != 0 || !cstr_is_equal( r->mod, "JS" ) ) { continue; }
        int have = 0;
        for( int w = 0; w < (int)wfuncs->count; ++w ) {
            if( cstr_is_equal( wfuncs->items[ w ].name, r->fld ) ) {
                have = 1;
                break;
            }
        }
        if( have ) { continue; }

        if( !any_js_props ) {
            buf_str( &s, "\n" );
            any_js_props = 1;
        }
        buf_str( &s, "  " );
        buf_js_key( &s, r->fld );
        buf_str( &s, ":(()=>0),\n" );
    }

    buf_str( &s, "};\n" );

    if( mfuncs->count ) {
        buf_str( &s, "var JSMAIN={" );
        for( int i = 0; i < (int)mfuncs->count; ++i ) {
            wfunc_t* f = &mfuncs->items[ i ];
            buf_str( &s, "\n  " );
            buf_js_key( &s, f->name );
            buf_str( &s, cstr_format( ":((...a)=>__cwasm_main_call(%d,a)),", f->opid ) );
        }
        buf_str( &s, "\n};\n" );
    }

    buf_str( &s, "var imports={\n" );

    int has_js_mod = ( wfuncs->count != 0 );
    if( !has_js_mod ) {
        for( int i = 0; i < (int)imports->count; ++i ) {
            if( cstr_is_equal( imports->items[ i ].mod, "JS" ) ) {
                has_js_mod = 1;
                break;
            }
        }
    }
    if( has_js_mod ) { buf_str( &s, "  JS:JS,\n" ); }
    if( mfuncs->count ) { buf_str( &s, "  JSMAIN:JSMAIN,\n" ); }

    array_t(cstr_t)* mods;
    array_create( mods );
    for( int i = 0; i < (int)imports->count; ++i ) {
        import_rec_t* r = &imports->items[ i ];
        if( cstr_is_equal( r->mod, "JS" ) || cstr_is_equal( r->mod, "JSMAIN" ) ) { continue; }

        int seen = 0;
        for( int k = 0; k < (int)mods->count; ++k ) {
            if( cstr_is_equal( mods->items[ k ], r->mod ) ) {
                seen = 1;
                break;
            }
        }
        if( seen ) { continue; }
        array_add( mods, r->mod );
    }

    for( int mi = 0; mi < (int)mods->count; ++mi ) {
        char const* mod = mods->items[ mi ];
        buf_str( &s, "  " );
        buf_js_key( &s, mod );
        buf_str( &s, ":{\n" );

        for( int i = 0; i < (int)imports->count; ++i ) {
            import_rec_t* r = &imports->items[ i ];
            if( !cstr_is_equal( r->mod, mod ) ) { continue; }

            buf_str( &s, "    " );
            buf_js_key( &s, r->fld );
            buf_str( &s, ":" );

            if( r->kind == 0 ) {
                buf_str( &s, "(()=>0)" );
            } else if( r->kind == 2 ) {
                cstr_t limits;
                if( r->shared ) {
                    // a shared memory must declare a maximum
                    uint32_t mx = r->has_b ? r->b : ( r->a > 256 ? r->a : 256 );
                    limits = cstr_format( "initial:%u,maximum:%u,shared:true", r->a, mx );
                } else if( r->has_b ) {
                    limits = cstr_format( "initial:%u,maximum:%u", r->a, r->b );
                } else {
                    limits = cstr_format( "initial:%u", r->a );
                }
                buf_str( &s,
                    "(typeof CWASM_EXISTING_MEMORY!==\"undefined\"&&CWASM_EXISTING_MEMORY)"
                    "?CWASM_EXISTING_MEMORY:new WebAssembly.Memory({" );
                buf_str( &s, limits );
                buf_str( &s, "})" );
            } else if( r->kind == 1 ) {
                buf_str( &s, "new WebAssembly.Table({element:\"funcref\"," );
                buf_str( &s, r->has_b ? cstr_format( "initial:%u,maximum:%u})", r->a, r->b )
                    : cstr_format( "initial:%u})", r->a ) );
            } else if( r->kind == 3 ) {
                char const* vt = ( r->t0 == 0x7F ? "i32" : r->t0 == 0x7E ? "i64"
                    : r->t0 == 0x7D ? "f32" : "f64" );
                buf_str( &s, cstr_format( "new WebAssembly.Global({value:\"%s\",mutable:%s},0)",
                    vt, r->t1 ? "true" : "false" ) );
            } else if( r->kind == 4 && ( cstr_is_equal( r->fld, "__cpp_exception" ) ||
                cstr_is_equal( r->fld, "__c_longjmp" ) ) ) {
                buf_str( &s, "new WebAssembly.Tag({parameters:[\"i32\"]})" );
            } else {
                buf_str( &s, "0" );
            }

            buf_str( &s, ",\n" );
        }

        buf_str( &s, "  },\n" );
    }

    array_destroy( mods );

    buf_str( &s, "};\n" );

    for( int li = 0; li < (int)libs->count; ++li ) {
        lib_group_t* g = &libs->items[ li ];
        if( g->inits->count == 0 ) { continue; }

        buf_str( &s, "(function(){\n" );
        for( int ii = 0; ii < (int)g->inits->count; ++ii ) {
            buf_str( &s, "  " );
            buf_str( &s, g->inits->items[ ii ] );
            buf_str( &s, "\n" );
        }
        for( int fi = 0; fi < (int)g->funcs->count; ++fi ) {
            wfunc_t* f = g->funcs->items[ fi ];
            buf_str( &s, "  " );
            if( is_js_ident( f->name ) ) {
                buf_str( &s, "JS." );
                buf_str( &s, f->name );
            } else {
                buf_str( &s, "JS[" );
                buf_js_string( &s, f->name, cstr_size( f->name ) );
                buf_str( &s, "]" );
            }
            buf_str( &s, "=(" );
            buf_str( &s, f->jsargs ? f->jsargs : "" );
            buf_str( &s, ")=>" );
            buf_str( &s, f->jscode ? f->jscode : "0" );
            buf_str( &s, ";\n" );
        }
        buf_str( &s, "})();\n" );
    }

    buf_str( &s, "return imports;\n" );
    buf_str( &s, "}\n" );

    lib_array_free( array_to_param( libs ) );
    if( s.failed ) {
        buf_free( &s );
        return NULL;
    }

    *out_factory = cstr( (char const*)s.p );
    cstr_t r = cstr_format( "(() => {\n%svar imports=__cwasm_build_imports();\n"
        "if(typeof CWASM!=='undefined'&&CWASM)CWASM.__cwasm_build_imports=__cwasm_build_imports;\n"
        "return imports;\n})()", (char const*)s.p );
    buf_free( &s );
    return r;
}


static cstr_t gen_main_handlers( array_param_t(wfunc_t)* mfuncsp ) {
    array_t(wfunc_t)* mfuncs;
    array_from_param( mfuncs, mfuncsp );
    if( !mfuncs->count ) { return NULL; }

    array_t(lib_group_t)* libs;
    array_create( libs );
    lib_group_all( array_to_param( libs ), mfuncsp );

    buf_t s = { 0 };
    buf_init( &s, 256 + wfuncs_bound( mfuncsp ) );
    buf_str( &s, "function __cwasm_build_main_handlers(){\n" );
    buf_str( &s, "var H=[];\n" );

    for( int pass = 0; pass < 2; ++pass ) {
        for( int li = 0; li < (int)libs->count; ++li ) {
            lib_group_t* g = &libs->items[ li ];
            // pass 0: libs without inits. pass 1: libs with inits, one IIFE each
            if( ( g->inits->count != 0 ) != pass ) { continue; }
            char const* indent = pass ? "  " : "";
            if( pass ) {
                buf_str( &s, "(function(){\n" );
                for( int ii = 0; ii < (int)g->inits->count; ++ii ) {
                    buf_str( &s, "  " );
                    buf_str( &s, g->inits->items[ ii ] );
                    buf_str( &s, "\n" );
                }
            }
            for( int fi = 0; fi < (int)g->funcs->count; ++fi ) {
                wfunc_t* f = g->funcs->items[ fi ];
                buf_str( &s, cstr_format( "%sH[%d]=(", indent, f->opid ) );
                buf_str( &s, f->jsargs ? f->jsargs : "" );
                buf_str( &s, ")=>" );
                buf_str( &s, f->jscode ? f->jscode : "0" );
                buf_str( &s, ";\n" );
            }
            if( pass ) { buf_str( &s, "})();\n" ); }
        }
    }

    buf_str( &s, "return H;\n}\n" );

    lib_array_free( array_to_param( libs ) );
    cstr_t r = s.failed ? NULL : cstr( (char const*)s.p );
    buf_free( &s );
    return r;
}


typedef struct bundle_slice_t {
    cstr_t name;
    char const* data;
    size_t n;
} bundle_slice_t;


typedef struct bundle_t {
    buf_t raw; // owns the whole file; slices point into it
    array_t(bundle_slice_t)* v;
} bundle_t;


static void bundle_free( bundle_t* b ) {
    if( !b ) { return; }
    if( b->v ) { array_destroy( b->v ); }
    buf_free( &b->raw );
    memset( b, 0, sizeof( *b ) );
}


// banner: 80 columns, "-------- NAME ----..."
static int bundle_load( bundle_t* b, char const* path ) {
    if( !load_file( path, &b->raw ) ) { return 0; }
    char* raw = (char*)b->raw.p;
    size_t raw_n = b->raw.n;
    array_create( b->v );

    struct {
        size_t line_off;
        size_t content_off;
        cstr_t name;
    } bans[ 32 ];
    int nb = 0;

    size_t i = 0;
    while( i < raw_n ) {
        size_t line_start = i;
        while( i < raw_n && raw[ i ] != '\n' ) { ++i; }
        size_t line_len = i - line_start;
        char const* line = raw + line_start;

        int is_banner = line_len == 80 && memcmp( line, "-------- ", 9 ) == 0;
        size_t name_start = 9;
        size_t p = name_start;
        if( is_banner ) {
            while( p < line_len && line[ p ] != ' ' ) { ++p; }
            if( p >= line_len || p == name_start || p + 1 >= line_len ) {
                is_banner = 0;
            }
        }
        if( is_banner ) {
            for( size_t t = p + 1; t < line_len; ++t ) {
                if( line[ t ] != '-' ) {
                    is_banner = 0;
                    break;
                }
            }
        }
        if( is_banner ) {
            if( nb >= 32 ) {
                bundle_free( b );
                return 0;
            }
            bans[ nb ].line_off = line_start;
            bans[ nb ].name = cstr_n( line + name_start, p - name_start );
            size_t c = ( i < raw_n && raw[ i ] == '\n' ) ? i + 1 : i;
            if( c < raw_n && raw[ c ] == '\n' ) { ++c; }
            bans[ nb ].content_off = c;
            ++nb;
        }
        if( i < raw_n && raw[ i ] == '\n' ) { ++i; }
    }

    // slices are NUL-terminated in place, over the separator newline
    for( int k = 0; k < nb; ++k ) {
        size_t end;
        if( k + 1 < nb ) {
            // the '\n' before a banner is the separator
            end = bans[ k + 1 ].line_off;
            size_t t = end;
            while( t > bans[ k ].content_off && raw[ t - 1 ] == '\n' ) { --t; }
            if( end > t ) {
                end -= 1;
            } else {
                while( end > bans[ k ].content_off &&
                    ( raw[ end - 1 ] == '\n' || raw[ end - 1 ] == '\r' ) ) {
                    --end;
                }
            }
        } else {
            end = raw_n;
        }
        raw[ end ] = '\0';

        bundle_slice_t slice;
        slice.name = bans[ k ].name;
        slice.data = raw + bans[ k ].content_off;
        slice.n = end - bans[ k ].content_off;
        array_add( b->v, slice );
    }
    return 1;
}


static char const* bundle_slice( bundle_t const* b, char const* name, size_t* out_n ) {
    for( int i = 0; i < (int)b->v->count; ++i ) {
        bundle_slice_t const* s = &b->v->items[ i ];
        if( !cstr_is_equal( s->name, name ) ) { continue; }
        *out_n = s->n;
        return s->data;
    }
    return NULL;
}

#endif /* jsgen_h */
