#ifndef wasm_mod_h
#define wasm_mod_h

// wasm module reading and rewriting, import/export probing, CWASM_JS harvesting

static uint32_t rd_leb_u32( uint8_t const* p, size_t n, size_t* i ) {
    uint32_t r = 0;
    int s = 0;
    while( *i < n ) {
        uint8_t b = p[ ( *i )++ ];
        r |= (uint32_t)( b & 0x7F ) << s;
        if( !( b & 0x80 ) ) { break; }
        s += 7;
    }
    return r;
}


static void wr_leb_u32( buf_t* b, uint32_t v ) {
    for( ;; ) {
        uint8_t byte = (uint8_t)( v & 0x7F );
        v >>= 7;
        if( v ) {
            byte |= 0x80;
            buf_u8( b, byte );
        } else {
            buf_u8( b, byte );
            break;
        }
    }
}


static cstr_t rd_wasm_str( uint8_t const* p, size_t n, size_t* i, uint32_t* out_len ) {
    uint32_t len = rd_leb_u32( p, n, i );
    // truncates on malformed input
    if( *i > n ) { *i = n; }
    if( len > (uint32_t)( n - *i ) ) { len = (uint32_t)( n - *i ); }
    cstr_t s = cstr_n( (char const*)( p + *i ), len );
    *i += len;
    if( out_len ) { *out_len = len; }
    return s;
}


static void wr_wasm_str( buf_t* b, char const* s, uint32_t len ) {
    wr_leb_u32( b, len );
    buf_put( b, s, len );
}


static cstr_t strip_init_parens( cstr_t s ) {
    s = cstr_trim( s );
    if( s[ 0 ] == '(' ) { s = cstr_trim( cstr_mid( s, 1, cstr_len( s ) - 1 ) ); }
    size_t n = cstr_len( s );
    if( n && s[ n - 1 ] == ')' ) { s = cstr_trim( cstr_left( s, n - 1 ) ); }
    return s;
}


static cstr_t js_escape_ctrl( char const* in ) {
    size_t n = cstr_size( in );
    char* out = cstr_temp_buffer( n * 4 + 1 );
    size_t k = 0;
    for( size_t i = 0; i < n; ++i ) {
        unsigned char c = (unsigned char)in[ i ];
        if( c >= 32 ) {
            out[ k++ ] = (char)c;
            continue;
        }
        out[ k++ ] = '\\';
        if( c == 0 ) {
            out[ k++ ] = '0';
        } else if( c == '\t' ) {
            out[ k++ ] = 't';
        } else if( c == '\n' ) {
            out[ k++ ] = 'n';
        } else if( c == '\v' ) {
            out[ k++ ] = 'v';
        } else if( c == '\f' ) {
            out[ k++ ] = 'f';
        } else if( c == '\r' ) {
            out[ k++ ] = 'r';
        } else {
            static char const hx[] = "0123456789ABCDEF";
            out[ k++ ] = 'x';
            out[ k++ ] = hx[ ( c >> 4 ) & 15 ];
            out[ k++ ] = hx[ c & 15 ];
        }
    }
    out[ k ] = '\0';
    return cstr( out );
}


// "( int x, char const* s )" -> "x,s"
static cstr_t strip_c_types_to_js_args( char const* args_in ) {
    cstr_t s = cstr( args_in );
    int open = cstr_find( s, "(", 0 );
    if( open >= 0 ) { s = cstr_mid( s, (CSTR_SIZE_T)open + 1, 0 ); }
    int close = cstr_rfind( s, ")", 0 );
    if( close >= 0 ) { s = cstr_left( s, (CSTR_SIZE_T)close ); }
    s = cstr_trim( s );
    if( !s[ 0 ] || cstr_is_equal( s, "void" ) ) { return cstr( "" ); }

    cstr_t out = cstr( "" );
    cstr_tokenizer_t params = cstr_tokenizer( s );
    for( cstr_t param = cstr_tokenize( &params, "," ); param;
        param = cstr_tokenize( &params, "," ) ) {
        int cut = cstr_find( param, "=", 0 );
        if( cut >= 0 ) { param = cstr_left( param, (CSTR_SIZE_T)cut ); }
        cut = cstr_find( param, "[", 0 );
        if( cut >= 0 ) { param = cstr_left( param, (CSTR_SIZE_T)cut ); }
        param = cstr_trim( param );
        size_t k = cstr_size( param );
        while( k > 0 ) {
            unsigned char c = (unsigned char)param[ k - 1 ];
            if( !( ( c >= '0' && c <= '9' ) || ( c >= 'A' && c <= 'Z' ) ||
                ( c >= 'a' && c <= 'z' ) || c == '_' ) ) {
                break;
            }
            --k;
        }
        char const* name = param + k;
        if( name[ 0 ] ) { out = out[ 0 ] ? cstr_format( "%s,%s", out, name ) : cstr( name ); }
    }
    return out;
}


typedef struct wfunc_t {
    cstr_t lib;
    cstr_t name;
    cstr_t jsargs;
    cstr_t jscode;
    cstr_t jsinit;
    int opid; // JSMAIN only, shared by worker stub and page handler
} wfunc_t;


static cstr_t copy_seg_v( char const* start ) {
    if( !start ) { return cstr( "" ); }
    char const* end = start;
    while( *end && (unsigned char)*end != 0x0B ) { ++end; }
    return cstr_n( start, (CSTR_SIZE_T)( end - start ) );
}


static void rd_limits( uint8_t const* p, size_t n, size_t* i, uint32_t* minv, int* has_max,
    uint32_t* maxv, int* shared ) {

    uint32_t flags = rd_leb_u32( p, n, i );
    uint32_t mn = rd_leb_u32( p, n, i );
    uint32_t mx = ( flags & 1 ) ? rd_leb_u32( p, n, i ) : 0;
    if( minv ) { *minv = mn; }
    if( has_max ) { *has_max = flags & 1; }
    if( maxv ) { *maxv = mx; }
    if( shared ) { *shared = !!( flags & 2 ); }
}


typedef struct import_rec_t {
    cstr_t mod;
    cstr_t fld;
    uint8_t kind; // 0 func, 1 table, 2 memory, 3 global, 4 tag
    uint32_t a; // type index for func/tag, limits min for table/memory
    uint32_t b; // limits max for table/memory
    int has_b;
    int shared; // memory limits bit 1
    uint8_t t0; // table elemtype, global valtype or tag attribute
    uint8_t t1; // global mutability
} import_rec_t;


typedef struct cwasm_facts_t {
    int threads; // shared memory import, or env.__cwasm_threads
    int cpp_eh; // env.__cpp_exception tag import
    int asyncified; // asyncify_* exports
    int em_sjlj; // invoke_* or _emscripten_throw_longjmp imports
} cwasm_facts_t;


static void probe_import_section( uint8_t const* sec, size_t sec_n, cwasm_facts_t* f ) {
    size_t i = 0;
    uint32_t count = rd_leb_u32( sec, sec_n, &i );

    for( uint32_t k = 0; k < count && i < sec_n; ++k ) {
        cstr_t mod = rd_wasm_str( sec, sec_n, &i, NULL );
        cstr_t fld = rd_wasm_str( sec, sec_n, &i, NULL );
        uint8_t kind = sec[ i++ ];

        if( kind == 0 ) {
            (void)rd_leb_u32( sec, sec_n, &i );
            if( cstr_starts( fld, "invoke_" ) ||
                cstr_is_equal( fld, "_emscripten_throw_longjmp" ) ) {
                f->em_sjlj = 1;
            }
            if( cstr_is_equal( mod, "env" ) && cstr_is_equal( fld, "__cwasm_threads" ) ) {
                f->threads = 1;
            }
        } else if( kind == 1 ) {
            ++i;
            rd_limits( sec, sec_n, &i, NULL, NULL, NULL, NULL );
        } else if( kind == 2 ) {
            int shared;
            rd_limits( sec, sec_n, &i, NULL, NULL, NULL, &shared );
            if( shared ) { f->threads = 1; }
        } else if( kind == 3 ) {
            i += 2;
        } else if( kind == 4 ) {
            ++i;
            (void)rd_leb_u32( sec, sec_n, &i );
            if( cstr_is_equal( mod, "env" ) && cstr_is_equal( fld, "__cpp_exception" ) ) {
                f->cpp_eh = 1;
            }
        } else {
            return;
        }
    }
}


static void probe_export_section( uint8_t const* sec, size_t sec_n, cwasm_facts_t* f ) {
    size_t i = 0;
    uint32_t count = rd_leb_u32( sec, sec_n, &i );

    for( uint32_t k = 0; k < count && i < sec_n; ++k ) {
        cstr_t nm = rd_wasm_str( sec, sec_n, &i, NULL );
        ++i; // kind
        (void)rd_leb_u32( sec, sec_n, &i ); // index
        if( cstr_starts( nm, "asyncify_" ) ) { f->asyncified = 1; }
    }
}


static int probe_wasm( uint8_t const* in, size_t in_n, cwasm_facts_t* f ) {
    memset( f, 0, sizeof( *f ) );
    if( in_n < 8 ) { return 0; }
    size_t i = 8;
    while( i < in_n ) {
        uint8_t id = in[ i++ ];
        uint32_t sec_len = rd_leb_u32( in, in_n, &i );
        if( i + sec_len > in_n ) { return 0; }
        if( id == 2 ) {
            probe_import_section( in + i, sec_len, f );
        } else if( id == 7 ) {
            probe_export_section( in + i, sec_len, f );
        }
        i += sec_len;
    }
    return 1;
}


static int rebuild_import_section( uint8_t const* sec, size_t sec_n, buf_t* out,
    array_param_t(wfunc_t)* wfuncsp, array_param_t(wfunc_t)* mfuncsp,
    array_param_t(import_rec_t)* importsp ) {

    array_t(wfunc_t)* wfuncs;
    array_t(wfunc_t)* mfuncs;
    array_t(import_rec_t)* imports;
    array_from_param( wfuncs, wfuncsp );
    array_from_param( mfuncs, mfuncsp );
    array_from_param( imports, importsp );

    size_t i = 0;
    uint32_t count = rd_leb_u32( sec, sec_n, &i );
    wr_leb_u32( out, count );

    for( uint32_t k = 0; k < count && i < sec_n; ++k ) {
        size_t entry_start = i;

        uint32_t mlen = 0;
        cstr_t mod = rd_wasm_str( sec, sec_n, &i, &mlen );
        cstr_t fld = rd_wasm_str( sec, sec_n, &i, NULL );

        uint8_t kind = sec[ i++ ];

        import_rec_t rec = { 0 };
        rec.mod = mod;
        rec.fld = fld;
        rec.kind = kind;

        if( kind == 0 ) {
            rec.a = rd_leb_u32( sec, sec_n, &i );
        } else if( kind == 1 || kind == 2 ) {
            if( kind == 1 ) { rec.t0 = sec[ i++ ]; }
            rd_limits( sec, sec_n, &i, &rec.a, &rec.has_b, &rec.b, &rec.shared );
        } else if( kind == 3 ) {
            rec.t0 = sec[ i++ ];
            rec.t1 = sec[ i++ ];
        } else if( kind == 4 ) {
            rec.t0 = sec[ i++ ];
            rec.a = rd_leb_u32( sec, sec_n, &i );
        } else {
            return 0;
        }

        int is_main = kind == 0 && cstr_is_equal( mod, "JSMAIN" );
        int is_js = kind == 0 && cstr_is_equal( mod, "JS" );
        if( ( is_js || is_main ) && cstr_find( fld, "\v", 0 ) >= 0 ) {
            // field is name \v args \v code \v lib \v init
            char const* parts[ 5 ];
            for( int t = 0; t < 5; ++t ) { parts[ t ] = NULL; }
            parts[ 0 ] = fld;
            int idx = 1;
            for( char const* p = fld; *p && idx < 5; ++p ) {
                if( (unsigned char)*p == 0x0B ) { parts[ idx++ ] = p + 1; }
            }
            if( !parts[ 2 ] ) {
                fprintf( stderr, "error: cwasm JS import missing fields\n" );
                return 0;
            }

            cstr_t name = copy_seg_v( parts[ 0 ] );
            cstr_t args = copy_seg_v( parts[ 1 ] );
            cstr_t code = copy_seg_v( parts[ 2 ] );
            cstr_t lib = copy_seg_v( parts[ 3 ] );
            cstr_t init = copy_seg_v( parts[ 4 ] );

            cstr_t jsargs = strip_c_types_to_js_args( args );

            if( is_main ) {
                int nargs = jsargs[ 0 ] ? 1 : 0;
                for( char const* p = jsargs; *p; ++p ) {
                    if( *p == ',' ) { ++nargs; }
                }
                if( nargs > 16 ) {
                    fprintf( stderr, "error: CWASM_JS_MAIN '%s' has %d args (mailbox max 16)\n",
                        name, nargs );
                    return 0;
                }
            }

            init = strip_init_parens( init );
            code = js_escape_ctrl( code );
            init = js_escape_ctrl( init );

            wfunc_t wf = { 0 };
            wf.lib = lib;
            wf.name = name;
            wf.jsargs = jsargs;
            wf.jscode = code;
            wf.jsinit = init;
            if( is_main ) {
                wf.opid = (int)mfuncs->count;
                array_add( mfuncs, wf );
            } else {
                array_add( wfuncs, wf );
            }

            wr_wasm_str( out, mod, mlen );
            wr_wasm_str( out, name, (uint32_t)cstr_size( name ) );
            buf_u8( out, kind );
            wr_leb_u32( out, rec.a );

            rec.fld = name;
            array_add( imports, rec );
            continue;
        }

        buf_put( out, sec + entry_start, i - entry_start );
        array_add( imports, rec );
    }
    return !out->failed;
}


// the rewrite never grows a section, so out sized to in_n always fits
static int rewrite_wasm_and_collect( uint8_t const* in, size_t in_n, buf_t* out,
    array_param_t(wfunc_t)* wfuncsp, array_param_t(wfunc_t)* mfuncsp,
    array_param_t(import_rec_t)* importsp ) {

    buf_put( out, in, 8 );
    size_t i = 8;

    while( i < in_n ) {
        uint8_t id = in[ i++ ];
        uint32_t sec_len = rd_leb_u32( in, in_n, &i );
        if( i + sec_len > in_n ) { return 0; }
        uint8_t const* sec = in + i;
        size_t sec_n = sec_len;
        i += sec_n;

        buf_u8( out, id );

        if( id != 2 ) {
            wr_leb_u32( out, (uint32_t)sec_n );
            buf_put( out, sec, sec_n );
        } else {
            buf_t rebuilt = { 0 };
            int ok = buf_init( &rebuilt, sec_n ) &&
                rebuild_import_section( sec, sec_n, &rebuilt, wfuncsp, mfuncsp, importsp );
            if( ok ) {
                wr_leb_u32( out, (uint32_t)rebuilt.n );
                buf_put( out, rebuilt.p, rebuilt.n );
            }
            buf_free( &rebuilt );
            if( !ok ) { return 0; }
        }
    }
    return !out->failed;
}


static int imports_declare_threads( array_param_t(import_rec_t)* importsp ) {
    array_t(import_rec_t)* imports;
    array_from_param( imports, importsp );
    for( int i = 0; i < (int)imports->count; ++i ) {
        import_rec_t* r = &imports->items[ i ];
        if( r->kind == 2 && r->shared ) { return 1; }
        if( r->kind == 0 && cstr_is_equal( r->mod, "env" ) &&
            cstr_is_equal( r->fld, "__cwasm_threads" ) ) {
            return 1;
        }
    }
    return 0;
}

#endif /* wasm_mod_h */
