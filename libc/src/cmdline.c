#include "cmdline.h"
#include "cwasm.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

CWASM_JS_LIB( CMD, uint32_t, js_cmdline, ( uint32_t dst, uint32_t len ), {
  return mem.str_write(CWASM.cmdline || "", dst, len);
})

CWASM_JS_LIB( CMD, uint32_t, js_envstring, ( uint32_t dst, uint32_t len ), {
  return mem.str_write(CWASM.envstring || "", dst, len);
})

static char* fetch_string( uint32_t ( *fn )( uint32_t, uint32_t ) ) {
    uint32_t need = fn( 0, 0 );
    uint32_t got;
    char* buf;

    if( !need ) { return strdup( "" ); }
    buf = (char*)malloc( need );
    if( !buf ) { return NULL; }
    got = fn( (uint32_t)(uintptr_t)buf, need );
    if( got != need ) {
        free( buf );
        return NULL;
    }
    return buf;
}

static int split_args( char* s, char** out, int max_out ) {
    int n = 0;

    if( !s ) { return 0; }
    while( *s && n < max_out ) {
        while( *s == ' ' ) { ++s; }
        if( !*s ) { break; }
        out[ n++ ] = s;
        while( *s && *s != ' ' ) { ++s; }
        if( *s ) { *s++ = '\0'; }
    }
    return n;
}

static int env_hex( unsigned char c ) {
    if( c >= '0' && c <= '9' ) { return (int)( c - '0' ); }
    if( c >= 'A' && c <= 'F' ) { return (int)( c - 'A' + 10 ); }
    if( c >= 'a' && c <= 'f' ) { return (int)( c - 'a' + 10 ); }
    return -1;
}

static int env_decode( char const* in, char* out, size_t outsz ) {
    size_t o = 0;

    if( !in ) { in = ""; }
    for( ; *in; ++in ) {
        unsigned char c = (unsigned char)*in;

        if( c == '+' ) { c = ' '; }
        else if( c == '%' ) {
            int hi;
            int lo;

            if( !in[ 1 ] || !in[ 2 ] ) { return -1; }
            hi = env_hex( (unsigned char)in[ 1 ] );
            lo = env_hex( (unsigned char)in[ 2 ] );
            if( hi < 0 || lo < 0 ) { return -1; }
            c = (unsigned char)( ( hi << 4 ) | lo );
            in += 2;
        }
        if( o + 1 >= outsz ) { return -1; }
        out[ o++ ] = (char)c;
    }
    out[ o ] = '\0';
    return 0;
}

static int env_key_match( char const* pair, char const* name, char* valbuf, size_t valbufsz ) {
    char keybuf[ 1024 ];
    char const* eq;
    size_t klen;

    eq = strchr( pair, '=' );
    klen = eq ? (size_t)( eq - pair ) : strlen( pair );
    if( klen + 1 > sizeof( keybuf ) ) { return 0; }
    memcpy( keybuf, pair, klen );
    keybuf[ klen ] = '\0';
    if( env_decode( keybuf, keybuf, sizeof( keybuf ) ) != 0 ) { return 0; }
    if( strcmp( keybuf, name ) != 0 ) { return 0; }
    if( !eq ) {
        valbuf[ 0 ] = '\0';
        return 1;
    }
    if( env_decode( eq + 1, valbuf, valbufsz ) != 0 ) { return 0; }
    return 1;
}

char** cwasm_build_argv( int* argc_out ) {
    char* line = fetch_string( js_cmdline );
    char* line_copy = line ? strdup( line ) : NULL;
    char* tmp_argv[ 256 ];
    char** argv;
    int argc;
    int i;

    argc = line_copy ? split_args( line_copy, tmp_argv, 256 ) : 0;
    argv = (char**)malloc( ( (size_t)argc + 1 ) * sizeof( char* ) );
    if( !argv ) {
        free( line );
        free( line_copy );
        if( argc_out ) { *argc_out = 0; }
        return NULL;
    }
    memset( argv, 0, ( (size_t)argc + 1 ) * sizeof( char* ) );
    for( i = 0; i < argc; ++i ) {
        argv[ i ] = strdup( tmp_argv[ i ] );
        if( !argv[ i ] ) {
            while( i > 0 ) {
                --i;
                free( argv[ i ] );
            }
            free( argv );
            free( line );
            free( line_copy );
            if( argc_out ) { *argc_out = 0; }
            return NULL;
        }
    }
    free( line );
    free( line_copy );
    if( argc_out ) { *argc_out = argc; }
    return argv;
}

int cwasm_fetch_cmdline_prefix( char* out, size_t outsz ) {
    uint32_t need = js_cmdline( 0, 0 );
    char* tmp;
    size_t n;

    if( !out || outsz < 1 ) {
        errno = EINVAL;
        return -1;
    }
    if( !need ) {
        out[ 0 ] = '\0';
        return 0;
    }
    if( need > 65536u ) {
        errno = ENAMETOOLONG;
        return -1;
    }
    tmp = (char*)malloc( need );
    if( !tmp ) { return -1; }
    js_cmdline( (uint32_t)(uintptr_t)tmp, need );
    n = strcspn( tmp, " \t\r\n" );
    if( n + 1 > outsz ) {
        free( tmp );
        errno = ERANGE;
        return -1;
    }
    memcpy( out, tmp, n );
    out[ n ] = '\0';
    free( tmp );
    return 0;
}

int cwasm_fetch_envstring( char* out, size_t outsz ) {
    uint32_t need = js_envstring( 0, 0 );

    if( !out || outsz < 1 ) {
        errno = EINVAL;
        return -1;
    }
    if( !need ) {
        out[ 0 ] = '\0';
        return 0;
    }
    if( need > 65536u ) {
        errno = ENAMETOOLONG;
        return -1;
    }
    if( need > outsz ) {
        errno = ERANGE;
        return -1;
    }
    js_envstring( (uint32_t)(uintptr_t)out, (uint32_t)outsz );
    return 0;
}

char* cwasm_getenv( char const* name ) {
    static char buf[ 1024 ];
    char env[ 2048 ];
    char const* p;
    char const* end;

    if( !name ) {
        return cwasm_fetch_envstring( buf, sizeof( buf ) ) == 0 ? buf : NULL;
    }
    if( cwasm_fetch_envstring( env, sizeof( env ) ) != 0 ) { return NULL; }
    if( !env[ 0 ] ) { return NULL; }
    for( p = env; p; p = end ) {
        end = strchr( p, '&' );
        if( end ) {
            size_t len = (size_t)( end - p );

            if( len + 1 > sizeof( buf ) ) { return NULL; }
            memcpy( buf, p, len );
            buf[ len ] = '\0';
            if( env_key_match( buf, name, buf, sizeof( buf ) ) ) { return buf; }
            ++end;
        } else if( env_key_match( p, name, buf, sizeof( buf ) ) ) {
            return buf;
        }
    }
    return NULL;
}

void cwasm_free_argv( int argc, char** argv ) {
    int i;

    if( !argv ) { return; }
    for( i = 0; i < argc; ++i ) { free( argv[ i ] ); }
    free( argv );
}
