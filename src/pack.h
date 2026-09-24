#ifndef pack_h
#define pack_h

// WFS embed pack, deflate, base64, icons, HTML build and the --files index

static char const cwasm_default_icon_b64[] =
    "iVBORw0KGgoAAAANSUhEUgAAADAAAAAwBAMAAAClLOS0AAAAMFBMVEX7+v9XP+9dRe9hSvBjTPBl"
    "T/BqVfBzYPGDcfOSg/Wuo/fCuvnSzPvj3/z08/7////sqf1zAAABjElEQVQ4y+2TMUtDMRDH76Ju"
    "tsl7bg7v5b0WnESrmyDiFxARXNwc/QTdnAXB1Q/gpqC7g+AH0FadjdHd9lLdtIlp7csb7OgiGPgf"
    "ufy4C3fH4TaMzs6mN587hcuKC9DAOPgJbgfmdQwYvl2NAR8X/ot2cCfDDU55cqmDh9sw/jD4B38I"
    "OEVeMJRSVI6WyQ6NZDKwOkQ87K26qeN5N9esPjSbu1T+keaQG2EzblgU1cqIKsSdDHIQtt0YvX0D"
    "ThFJqL1IR56yhQAsxW8Cop6oVLP+/lsJqhqNfEazeCukPeJpWQfhjFDISE0Ia6EWAN4xNC2G0PWV"
    "JJTrAF5xq3KF62BehOrrmAJ46mWuhUtOM3jsa1augeKZddiw1w1Yma1PrJkCOMidFXfWl1Gv+0bc"
    "Fy1JKOW8nToeAaKANKSyelFtEHS47B9AD5ZPxCiiQnCTaHhEaQ/PzmGZwgQJul7qXbo4T3SPilTY"
    "7Rp88nIdAdOtaRGWMxbKz1Fh5sfnfHsCsDofSjEJMJjvL67zF2CyoqnUyRU2AAAAAElFTkSuQmCC"
    ;


typedef struct wfs_embed_t {
    cstr_t name;
    cstr_t path;
} wfs_embed_t;


// WFS pack format; the reader is libc/src/cwasm_fs.c
#define WFS_MAGIC_0 'W'
#define WFS_MAGIC_1 'F'
#define WFS_MAGIC_2 'S'
#define WFS_VERSION 0
#define WFS_NAME_MAX 255
#define WFS_SHIP_FILE_MODE 0444u
#define WFS_ENTRY_META_SIZE 20u


// names are absolute inside the pack
static void wfs_embed_push( array_param_t(wfs_embed_t)* evp, char const* name,
    char const* path ) {

    array_t(wfs_embed_t)* ev;
    array_from_param( ev, evp );
    wfs_embed_t e;
    e.name = name[ 0 ] != '/' ? cstr_format( "/%s", name ) : cstr( name );
    e.path = cstr( path );
    array_add( ev, e );
}


typedef struct wfs_entry_t {
    uint32_t size;
    uint32_t off;
    uint32_t mode;
    int32_t mtime_sec;
    int32_t atime_sec;
} wfs_entry_t;


static int wfs_pack_build( array_param_t(wfs_embed_t)* evp, buf_t* out ) {
    array_t(wfs_embed_t)* ev;
    array_from_param( ev, evp );
    array_t(wfs_entry_t)* entries;
    array_create( entries );

    #define CLEANUP_AND_RETURN( STATUS ) (\
            array_destroy( entries ), \
            (STATUS) )

    if( ev->count == 0 ) { return CLEANUP_AND_RETURN( 0 ); }
    uint32_t index_size = 4u; // count u32
    uint64_t total = 0;
    for( int i = 0; i < (int)ev->count; ++i ) {
        size_t name_len = cstr_size( ev->items[ i ].name );
        long sz = file_size( ev->items[ i ].path );
        struct stat st;
        if( name_len == 0 || name_len > WFS_NAME_MAX || sz < 0 || (uint64_t)sz > UINT32_MAX ||
            stat( ev->items[ i ].path, &st ) != 0 ) {
            return CLEANUP_AND_RETURN( 0 );
        }
        wfs_entry_t e;
        e.size = (uint32_t)sz;
        e.off = 0;
        e.mode = WFS_SHIP_FILE_MODE;
        e.mtime_sec = (int32_t)st.st_mtime;
        e.atime_sec = (int32_t)st.st_atime;
        array_add( entries, e );
        uint32_t entry_size = (uint32_t)( 1 + name_len + 8u + WFS_ENTRY_META_SIZE );
        if( index_size > UINT32_MAX - entry_size ) { return CLEANUP_AND_RETURN( 0 ); }
        index_size += entry_size;
        total += (uint64_t)sz;
    }

    uint64_t data_off = 4u + 4u + index_size; // magic + index_size + index region
    if( data_off + total > UINT32_MAX ) { return CLEANUP_AND_RETURN( 0 ); }
    for( int i = 0; i < (int)entries->count; ++i ) {
        entries->items[ i ].off = (uint32_t)data_off;
        data_off += entries->items[ i ].size;
    }

    uint8_t magic[ 4 ] = {
        WFS_MAGIC_0, WFS_MAGIC_1, WFS_MAGIC_2, (uint8_t)WFS_VERSION
    };
    if( !buf_init( out, (size_t)data_off ) ) { return CLEANUP_AND_RETURN( 0 ); }
    buf_put( out, magic, 4 );
    buf_u32( out, index_size );
    buf_u32( out, (uint32_t)entries->count );

    // nsec fields are always zero
    for( int i = 0; i < (int)entries->count; ++i ) {
        wfs_entry_t* e = &entries->items[ i ];
        cstr_t name = ev->items[ i ].name;
        buf_u8( out, (uint8_t)cstr_size( name ) );
        buf_put( out, name, cstr_size( name ) );
        buf_u32( out, e->off );
        buf_u32( out, e->size );
        buf_u32( out, e->mode );
        buf_i32( out, e->mtime_sec );
        buf_i32( out, 0 );
        buf_i32( out, e->atime_sec );
        buf_i32( out, 0 );
    }

    for( int i = 0; i < (int)entries->count; ++i ) {
        wfs_entry_t* e = &entries->items[ i ];
        if( !load_file_into( ev->items[ i ].path, out ) || out->n != e->off + e->size ) {
            return CLEANUP_AND_RETURN( 0 );
        }
    }
    return CLEANUP_AND_RETURN( !out->failed );
    #undef CLEANUP_AND_RETURN
}


static int deflate_to_buf( uint8_t const* in, size_t in_n, buf_t* out ) {
    if( in_n > INT_MAX || !buf_init( out, (size_t)sdefl_bound( (int)in_n ) ) ) { return 0; }
    int n = cwasm_deflate( out->p, in, (int)in_n );
    if( n <= 0 ) { return 0; }
    out->n = (size_t)n;
    out->p[ out->n ] = '\0';
    return 1;
}


static char const b64_alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


static int b64_encode( uint8_t const* data, size_t n, buf_t* out ) {
    if( !buf_init( out, 4 * ( ( n + 2 ) / 3 ) ) ) { return 0; }
    char* o = (char*)out->p;
    size_t i = 0;
    size_t k = 0;
    while( i + 3 <= n ) {
        uint32_t v = ( (uint32_t)data[ i ] << 16 ) | ( (uint32_t)data[ i + 1 ] << 8 ) |
            (uint32_t)data[ i + 2 ];
        o[ k++ ] = b64_alphabet[ ( v >> 18 ) & 63 ];
        o[ k++ ] = b64_alphabet[ ( v >> 12 ) & 63 ];
        o[ k++ ] = b64_alphabet[ ( v >> 6 ) & 63 ];
        o[ k++ ] = b64_alphabet[ ( v >> 0 ) & 63 ];
        i += 3;
    }
    size_t r = n - i;
    if( r == 1 ) {
        uint32_t v = ( (uint32_t)data[ i ] << 16 );
        o[ k++ ] = b64_alphabet[ ( v >> 18 ) & 63 ];
        o[ k++ ] = b64_alphabet[ ( v >> 12 ) & 63 ];
        o[ k++ ] = '=';
        o[ k++ ] = '=';
    } else if( r == 2 ) {
        uint32_t v = ( (uint32_t)data[ i ] << 16 ) | ( (uint32_t)data[ i + 1 ] << 8 );
        o[ k++ ] = b64_alphabet[ ( v >> 18 ) & 63 ];
        o[ k++ ] = b64_alphabet[ ( v >> 12 ) & 63 ];
        o[ k++ ] = b64_alphabet[ ( v >> 6 ) & 63 ];
        o[ k++ ] = '=';
    }
    o[ k ] = '\0';
    out->n = k;
    return 1;
}


static cstr_t b64_cstr( uint8_t const* data, size_t n ) {
    buf_t b = { 0 };
    cstr_t r = b64_encode( data, n, &b ) ? cstr( (char const*)b.p ) : NULL;
    buf_free( &b );
    return r;
}


static int cmp_strptr( char* const* a, char* const* b ) {
    return cstr_compare( *a, *b );
}


static int walk_content_files( char const* root, char const* rel,
    array_param_t(char*)* outp ) {

    array_t(char*)* out;
    array_from_param( out, outp );
    dir_t* d = dir_open( rel[ 0 ] ? cstr_format( "%s/%s", root, rel ) : root );
    if( !d ) { return 0; }

    #define CLEANUP_AND_RETURN( STATUS ) (\
            dir_close( d ), \
            (STATUS) )

    for( ;; ) {
        dir_entry_t* de = dir_read( d );
        if( !de ) { break; }
        char const* name = dir_name( de );
        if( !name || cstr_is_equal( name, "." ) || cstr_is_equal( name, ".." ) ) { continue; }
        cstr_t child = rel[ 0 ] ? cstr_format( "%s/%s", rel, name ) : cstr( name );

        if( dir_is_folder( de ) ) {
            if( !walk_content_files( root, child, outp ) ) {
                return CLEANUP_AND_RETURN( 0 );
            }
        } else if( dir_is_file( de ) ) {
            if( cstr_is_equal( name, "CWASM_FILES" ) ) { continue; }
            array_add( out, (char*)cstr_format( "/%s", child ) );
        }
    }

    return CLEANUP_AND_RETURN( 1 );
    #undef CLEANUP_AND_RETURN
}


static int generate_cwasm_files( char const* content_dir ) {
    if( !content_dir || !content_dir[ 0 ] ) { return 0; }

    array_t(char*)* paths;
    array_create( paths );
    int ok = walk_content_files( content_dir, "", array_to_param( paths ) );
    if( ok && paths->count > 1 ) {
        array_sort( paths, cmp_strptr );
    }
    if( ok ) {
        FILE* f = fopen( cstr_format( "%s/CWASM_FILES", content_dir ), "wb" );
        if( !f ) {
            ok = 0;
        } else {
            for( int i = 0; i < (int)paths->count; ++i ) {
                if( fprintf( f, "%s\n", paths->items[ i ] ) < 0 ) {
                    ok = 0;
                    break;
                }
            }
            fclose( f );
        }
    }
    array_destroy( paths );
    return ok;
}


static cstr_t html_escape( char const* s ) {
    cstr_t r = cstr_replace( cstr( s ), "&", "&amp;" );
    r = cstr_replace( r, "<", "&lt;" );
    r = cstr_replace( r, "\"", "&quot;" );
    return cstr_replace( r, "'", "&#39;" );
}


typedef struct icon_pair_t {
    cstr_t favicon;
    cstr_t iosicon;
} icon_pair_t;


// alpha blend over white, packed rgb multiply
static void icon_ios_blend( stbi_uc const* rgba, stbi_uc* ios ) {
    for( int i = 0; i < 48 * 48; ++i ) {
        uint32_t color1 = 0xffffffffu;
        uint32_t color2 = ( (uint32_t const*)rgba )[ i ];
        uint64_t c1 = (uint64_t)color1;
        uint64_t c2 = (uint64_t)color2;
        uint64_t a = (uint64_t)( color2 >> 24 );
        c1 = ( c1 | ( c1 << 24 ) ) & 0x00ff00ff00ffull;
        c2 = ( c2 | ( c2 << 24 ) ) & 0x00ff00ff00ffull;
        uint64_t o = ( ( ( ( c2 - c1 ) * a ) >> 8 ) + c1 ) & 0x00ff00ff00ffull;
        ( (uint32_t*)ios )[ i ] = 0xff000000u | (uint32_t)( o | ( o >> 24 ) );
    }
}


static int icon_resize_crop_48( stbi_uc const* src, int sw, int sh, stbi_uc* dst ) {
    double sx = 48.0 / (double)sw;
    double sy = 48.0 / (double)sh;
    double scale = sx > sy ? sx : sy;
    int tw = (int)( sw * scale + 0.5 );
    int th = (int)( sh * scale + 0.5 );
    if( tw < 48 ) { tw = 48; }
    if( th < 48 ) { th = 48; }
    buf_t tmp = { 0 };
    int ok = buf_init( &tmp, (size_t)tw * (size_t)th * 4 ) &&
        stbir_resize_uint8_linear( src, sw, sh, sw * 4, tmp.p, tw, th, tw * 4, STBIR_RGBA );
    if( ok ) {
        int ox = ( tw - 48 ) / 2;
        int oy = ( th - 48 ) / 2;
        for( int y = 0; y < 48; ++y ) {
            memcpy( dst + y * 48 * 4, tmp.p + ( ( oy + y ) * tw + ox ) * 4, 48 * 4 );
        }
    }
    buf_free( &tmp );
    return ok;
}


static cstr_t rgba48_to_b64( stbi_uc const* px ) {
    int png_len = 0;
    unsigned char* png = stbi_write_png_to_mem( px, 48 * 4, 48, 48, 4, &png_len );
    if( !png ) { return NULL; }
    cstr_t r = b64_cstr( png, (size_t)png_len );
    STBIW_FREE( png );
    return r;
}


static int icon_from_rgba48( stbi_uc const* px, uint8_t const* raw_png, size_t raw_png_n,
    icon_pair_t* icons ) {

    icons->favicon = raw_png ? b64_cstr( raw_png, raw_png_n ) : rgba48_to_b64( px );
    if( !icons->favicon ) { return 0; }
    int has_alpha = 0;
    for( int i = 0; i < 48 * 48; ++i ) {
        if( px[ i * 4 + 3 ] != 255 ) {
            has_alpha = 1;
            break;
        }
    }
    if( has_alpha ) {
        stbi_uc ios[ 48 * 48 * 4 ];
        icon_ios_blend( px, ios );
        icons->iosicon = rgba48_to_b64( ios );
        if( !icons->iosicon ) { return 0; }
    } else {
        icons->iosicon = icons->favicon;
    }
    return 1;
}


static int icon_from_path( char const* path, icon_pair_t* icons ) {
    buf_t file = { 0 };
    if( !load_file( path, &file ) ) {
        fprintf( stderr, "error: cannot read icon %s\n", path );
        return 0;
    }

    int w = 0;
    int h = 0;
    int comp = 0;
    if( !stbi_info_from_memory( file.p, (int)file.n, &w, &h, &comp ) ) {
        fprintf( stderr, "error: icon is not a supported image: %s\n", path );
        buf_free( &file );
        return 0;
    }

    stbi_uc* px = stbi_load_from_memory( file.p, (int)file.n, &w, &h, &comp, 4 );
    if( !px ) {
        fprintf( stderr, "error: icon failed to decode: %s\n", path );
        buf_free( &file );
        return 0;
    }

    int ok = 0;
    if( w == 48 && h == 48 ) {
        ok = icon_from_rgba48( px, file.p, file.n, icons );
    } else {
        stbi_uc out[ 48 * 48 * 4 ];
        if( !icon_resize_crop_48( px, w, h, out ) ) {
            fprintf( stderr, "error: icon resize failed: %s\n", path );
        } else {
            ok = icon_from_rgba48( out, NULL, 0, icons );
        }
    }

    STBI_FREE( px );
    buf_free( &file );
    return ok;
}


static int html_build( char const* tpl, size_t tpl_n, buf_t const* js, char const* path_out,
    char const* title_arg, char const* icon_path, buf_t* out ) {

    size_t js_count = count_sub( tpl, tpl_n, "{{{js}}}" );
    size_t title_count = count_sub( tpl, tpl_n, "{{{title}}}" );
    size_t fav_count = count_sub( tpl, tpl_n, "{{{favicon}}}" );
    size_t ios_count = count_sub( tpl, tpl_n, "{{{iosicon}}}" );
    if( !js_count ) {
        fprintf( stderr, "error: missing marker {{{js}}} in template\n" );
        return 0;
    }

    cstr_t title = cstr( "" );
    if( title_count ) {
        cstr_t raw = cstr( title_arg );
        if( !title_arg ) {
            raw = cstr_path_basename( path_out );
            if( cstr_ends( raw, ".html" ) ) { raw = cstr_left( raw, cstr_len( raw ) - 5 ); }
        }
        title = html_escape( raw );
    }

    icon_pair_t icons = { "", "" };
    if( fav_count || ios_count ) {
        if( icon_path ) {
            if( !icon_from_path( icon_path, &icons ) ) { return 0; }
        } else {
            icons.favicon = cstr( cwasm_default_icon_b64 );
            icons.iosicon = icons.favicon;
        }
    }

    size_t bound = tpl_n + js_count * js->n + title_count * cstr_size( title ) +
        fav_count * cstr_size( icons.favicon ) + ios_count * cstr_size( icons.iosicon );
    if( !buf_init( out, bound ) ) { return 0; }
    buf_put( out, tpl, tpl_n );
    return splice( out, "{{{title}}}", title, cstr_size( title ), SPLICE_RAW ) &&
        splice( out, "{{{favicon}}}", icons.favicon, cstr_size( icons.favicon ), SPLICE_RAW ) &&
        splice( out, "{{{iosicon}}}", icons.iosicon, cstr_size( icons.iosicon ), SPLICE_RAW ) &&
        splice( out, "{{{js}}}", (char const*)js->p, js->n, SPLICE_RAW ) && !out->failed;
}

#endif /* pack_h */
