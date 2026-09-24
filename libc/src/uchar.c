#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <wchar.h>
#include <uchar.h>

int __cwasm_locale_ctype_utf8( void );
int __cwasm_locale_collate_utf8( void );
int __cwasm_wcscoll_utf8( wchar_t const* s1, wchar_t const* s2 );
size_t __cwasm_wcsxfrm_utf8( wchar_t* dest, wchar_t const* src, size_t n );

struct __cwasm_mbstate {
    uint32_t acc;
    uint32_t min_cp;
    uint32_t extra;
    uint8_t want;
    uint8_t have;
    uint16_t _pad;
};

_Static_assert( sizeof( struct __cwasm_mbstate ) <= sizeof( mbstate_t ),
    "mbstate_t too small" );

#define MS(ps) (__cwasm_mbstate(ps))

static struct __cwasm_mbstate* __cwasm_mbstate( mbstate_t* ps ) {
    return ( struct __cwasm_mbstate* )(void*)ps;
}

static struct __cwasm_mbstate const* __cwasm_mbstate_const( mbstate_t const* ps ) {
    return ( struct __cwasm_mbstate const* )(void const*)ps;
}

void __cwasm_mbstate_reset( mbstate_t* ps ) {
    if( !ps ) { return; }
    MS( ps )->acc = 0;
    MS( ps )->min_cp = 0;
    MS( ps )->extra = 0;
    MS( ps )->want = 0;
    MS( ps )->have = 0;
    MS( ps )->_pad = 0;
}

int __cwasm_mbstate_has_partial( mbstate_t const* ps ) {
    if( !ps ) { return 0; }
    return __cwasm_mbstate_const( ps )->want != 0;
}

#ifdef CWASM_THREADS
    #define CWASM_MB_TLS _Thread_local
#else
    #define CWASM_MB_TLS
#endif

static size_t cwasm_utf8_mbrtowc( wchar_t* restrict pwc,
    char const* restrict s,
    size_t n,
    mbstate_t* restrict ps ) {
    if( !ps ) {
        static CWASM_MB_TLS mbstate_t state;
        ps = &state;
    }
    if( n == 0 ) { return (size_t)-2; }

    size_t used = 0;

    if( MS( ps )->want == 0 ) {
        uint8_t b0 = (uint8_t)(unsigned char)s[ 0 ];
        if( b0 < 0x80u ) {
            if( pwc ) { *pwc = (wchar_t)b0; }
            return b0 == 0 ? 0 : 1;
        }

        uint8_t want;
        uint32_t acc;
        uint32_t min_cp;

        if( b0 >= 0xc2u && b0 <= 0xdfu ) {
            want = 2;
            acc = b0 & 0x1fu;
            min_cp = 0x80u;
        } else if( b0 >= 0xe0u && b0 <= 0xefu ) {
            want = 3;
            acc = b0 & 0x0fu;
            min_cp = 0x800u;
        } else if( b0 >= 0xf0u && b0 <= 0xf4u ) {
            want = 4;
            acc = b0 & 0x07u;
            min_cp = 0x10000u;
        } else {
            errno = EILSEQ;
            return (size_t)-1;
        }

        MS( ps )->want = want;
        MS( ps )->have = 1;
        MS( ps )->acc = acc;
        MS( ps )->min_cp = min_cp;
        ++s;
        --n;
        used = 1;
    }

    while( MS( ps )->have < MS( ps )->want ) {
        if( n == 0 ) { return (size_t)-2; }

        uint8_t bx = (uint8_t)(unsigned char)*s++;
        --n;
        ++used;

        if( ( bx & 0xc0u ) != 0x80u ) {
            __cwasm_mbstate_reset( ps );
            errno = EILSEQ;
            return (size_t)-1;
        }

        MS( ps )->acc = ( MS( ps )->acc << 6 ) | (uint32_t)( bx & 0x3fu );
        ++MS( ps )->have;
    }

    uint32_t cp = MS( ps )->acc;
    uint32_t min_cp = MS( ps )->min_cp;
    __cwasm_mbstate_reset( ps );

    if( cp < min_cp || cp > 0x10ffffu || ( cp >= 0xd800u && cp <= 0xdfffu ) ) {
        errno = EILSEQ;
        return (size_t)-1;
    }

    if( pwc ) { *pwc = (wchar_t)cp; }
    return cp == 0 ? 0 : used;
}

size_t mbrtowc( wchar_t* restrict pwc,
    char const* restrict s,
    size_t n,
    mbstate_t* restrict ps ) {
    if( !s ) {
        __cwasm_mbstate_reset( ps );
        return 0;
    }

    if( __cwasm_locale_ctype_utf8() ) { return cwasm_utf8_mbrtowc( pwc, s, n, ps ); }
    if( n == 0 ) { return (size_t)-2; }

    unsigned char b = (unsigned char)s[ 0 ];
    if( pwc ) { *pwc = (wchar_t)b; }
    return b == 0 ? 0 : 1;
}

static size_t cwasm_utf8_wcrtomb( char* restrict s, wchar_t wc ) {
    uint32_t cp = (uint32_t)wc;
    if( cp > 0x10ffffu || ( cp >= 0xd800u && cp <= 0xdfffu ) ) {
        errno = EILSEQ;
        return (size_t)-1;
    }

    if( cp < 0x80u ) {
        s[ 0 ] = (char)cp;
        return 1;
    }
    if( cp < 0x800u ) {
        s[ 0 ] = (char)( 0xc0u | ( cp >> 6 ) );
        s[ 1 ] = (char)( 0x80u | ( cp & 0x3fu ) );
        return 2;
    }
    if( cp < 0x10000u ) {
        s[ 0 ] = (char)( 0xe0u | ( cp >> 12 ) );
        s[ 1 ] = (char)( 0x80u | ( ( cp >> 6 ) & 0x3fu ) );
        s[ 2 ] = (char)( 0x80u | ( cp & 0x3fu ) );
        return 3;
    }

    s[ 0 ] = (char)( 0xf0u | ( cp >> 18 ) );
    s[ 1 ] = (char)( 0x80u | ( ( cp >> 12 ) & 0x3fu ) );
    s[ 2 ] = (char)( 0x80u | ( ( cp >> 6 ) & 0x3fu ) );
    s[ 3 ] = (char)( 0x80u | ( cp & 0x3fu ) );
    return 4;
}

size_t wcrtomb( char* restrict s, wchar_t wc, mbstate_t* restrict ps ) {
    if( !s ) {
        __cwasm_mbstate_reset( ps );
        return 1;
    }

    if( __cwasm_locale_ctype_utf8() ) { return cwasm_utf8_wcrtomb( s, wc ); }

    uint32_t cp = (uint32_t)wc;
    if( cp > 0xffu ) {
        errno = EILSEQ;
        return (size_t)-1;
    }
    s[ 0 ] = (char)(unsigned char)cp;
    return 1;
}

size_t mbsrtowcs( wchar_t* restrict dst,
    char const** restrict src,
    size_t len,
    mbstate_t* restrict ps ) {
    if( !src || !*src ) { return 0; }

    char const* s = *src;
    size_t out = 0;

    for( ;; ) {
        wchar_t wc = 0;
        size_t r = mbrtowc( &wc, s, (size_t)-1, ps );
        if( r == (size_t)-1 ) {
            *src = s;
            return (size_t)-1;
        }
        if( r == (size_t)-2 ) {
            *src = s;
            errno = EILSEQ;
            return (size_t)-1;
        }

        if( r == 0 ) {
            if( dst ) {
                if( out < len ) { dst[ out ] = 0; }
                *src = NULL;
            }
            return out;
        }

        if( dst ) {
            if( out >= len ) {
                *src = s;
                return out;
            }
            dst[ out ] = wc;
        }

        ++out;
        s += r;
    }
}

size_t wcsrtombs( char* restrict dst,
    wchar_t const** restrict src,
    size_t len,
    mbstate_t* restrict ps ) {
    if( !src || !*src ) { return 0; }

    wchar_t const* ws = *src;
    size_t out = 0;

    for( ;; ) {
        wchar_t wc = *ws;
        if( wc == 0 ) {
            if( dst ) {
                if( out < len ) { dst[ out ] = 0; }
                *src = NULL;
            }
            return out;
        }

        char buf[ 4 ];
        size_t nb = wcrtomb( buf, wc, ps );
        if( nb == (size_t)-1 ) {
            *src = ws;
            return (size_t)-1;
        }

        if( dst ) {
            if( out + nb > len ) {
                *src = ws;
                return out;
            }
            for( size_t i = 0; i < nb; ++i ) { dst[ out + i ] = buf[ i ]; }
        }

        out += nb;
        ++ws;
    }
}

static CWASM_MB_TLS mbstate_t cwasm_mbrtoc16_state;
static CWASM_MB_TLS mbstate_t cwasm_c16rtomb_state;

size_t mbrtoc32( char32_t* restrict pc32,
    char const* restrict s,
    size_t n,
    mbstate_t* restrict ps ) {
    if( !s ) {
        __cwasm_mbstate_reset( ps );
        return 0;
    }

    wchar_t wc;
    size_t r = mbrtowc( &wc, s, n, ps );
    if( r == (size_t)-1 || r == (size_t)-2 ) { return r; }
    if( pc32 ) { *pc32 = (char32_t)(uint32_t)wc; }
    return r;
}

size_t c32rtomb( char* restrict s, char32_t c32, mbstate_t* restrict ps ) {
    return wcrtomb( s, (wchar_t)(uint32_t)c32, ps );
}

size_t mbrtoc16( char16_t* restrict pc16,
    char const* restrict s,
    size_t n,
    mbstate_t* restrict ps ) {
    if( !ps ) { ps = &cwasm_mbrtoc16_state; }

    if( !s ) {
        __cwasm_mbstate_reset( ps );
        return 0;
    }

    if( MS( ps )->extra ) {
        uint32_t lo = MS( ps )->extra;
        MS( ps )->extra = 0;
        if( pc16 ) { *pc16 = (char16_t)lo; }
        return (size_t)-3;
    }

    wchar_t wc;
    size_t r = mbrtowc( &wc, s, n, ps );
    if( r == (size_t)-1 || r == (size_t)-2 ) { return r; }

    uint32_t cp = (uint32_t)wc;
    if( cp == 0 ) {
        if( pc16 ) { *pc16 = 0; }
        return 0;
    }

    if( cp <= 0xFFFFu ) {
        if( cp - 0xD800u < 0x400u || cp - 0xDC00u < 0x400u ) {
            errno = EILSEQ;
            return (size_t)-1;
        }
        if( pc16 ) { *pc16 = (char16_t)cp; }
        return r;
    }

    cp -= 0x10000u;
    MS( ps )->extra = 0xDC00u + ( cp & 0x3FFu );
    if( pc16 ) { *pc16 = (char16_t)( 0xD800u + ( cp >> 10 ) ); }
    return r;
}

size_t c16rtomb( char* restrict s, char16_t c16, mbstate_t* restrict ps ) {
    if( !ps ) { ps = &cwasm_c16rtomb_state; }

    if( !s ) {
        __cwasm_mbstate_reset( ps );
        return 1;
    }

    uint32_t u = (uint32_t)c16;

    if( MS( ps )->extra ) {
        uint32_t hi = MS( ps )->extra;
        MS( ps )->extra = 0;
        if( hi - 0xD800u >= 0x400u || u - 0xDC00u >= 0x400u ) {
            __cwasm_mbstate_reset( ps );
            errno = EILSEQ;
            return (size_t)-1;
        }
        return wcrtomb( s, (wchar_t)( 0x10000u + ( ( ( hi - 0xD800u ) << 10 ) | ( u - 0xDC00u ) ) ), ps );
    }

    if( u - 0xD800u < 0x400u ) {
        MS( ps )->extra = u;
        return 0;
    }
    if( u - 0xDC00u < 0x400u ) {
        errno = EILSEQ;
        return (size_t)-1;
    }

    return wcrtomb( s, (wchar_t)u, ps );
}

size_t mbrlen( char const* restrict s, size_t n, mbstate_t* restrict ps ) {
    return mbrtowc( NULL, s, n, ps );
}

int mbtowc( wchar_t* restrict pwc, char const* restrict s, size_t n ) {
    static CWASM_MB_TLS mbstate_t state;

    if( !s ) {
        __cwasm_mbstate_reset( &state );
        return 0;
    }
    size_t r = mbrtowc( pwc, s, n, &state );
    if( r == (size_t)-1 || r == (size_t)-2 ) { return -1; }
    return (int)r;
}

static CWASM_MB_TLS mbstate_t cwasm_mblen_state;
static CWASM_MB_TLS mbstate_t cwasm_wctomb_state;

int mblen( char const* s, size_t n ) {
    size_t r;

    if( !s ) {
        __cwasm_mbstate_reset( &cwasm_mblen_state );
        return 0;
    }
    if( n == 0 ) { return 0; }
    r = mbrtowc( NULL, s, n, &cwasm_mblen_state );
    if( r == (size_t)-1 ) { return -1; }
    if( r == (size_t)-2 ) { return -1; }
    return (int)r;
}

int wctomb( char* s, wchar_t wc ) {
    size_t r;

    if( !s ) {
        __cwasm_mbstate_reset( &cwasm_wctomb_state );
        return 0;
    }
    r = wcrtomb( s, wc, &cwasm_wctomb_state );
    if( r == (size_t)-1 ) { return -1; }
    return (int)r;
}

size_t mbstowcs( wchar_t* restrict pwcs, char const* restrict s, size_t n ) {
    mbstate_t st;
    char const* src = s;
    size_t len = pwcs ? n : (size_t)-1;

    __cwasm_mbstate_reset( &st );
    return mbsrtowcs( pwcs, &src, len, &st );
}

size_t wcstombs( char* restrict s, wchar_t const* restrict pwcs, size_t n ) {
    mbstate_t st;
    wchar_t const* src = pwcs;
    size_t len = s ? n : (size_t)-1;

    __cwasm_mbstate_reset( &st );
    return wcsrtombs( s, &src, len, &st );
}

size_t wcsnrtombs( char* restrict dst, wchar_t const** restrict src, size_t nwc,
    size_t len, mbstate_t* restrict ps ) {
    wchar_t const* ws;
    size_t out = 0;
    size_t i = 0;

    if( !src || !*src ) { return 0; }
    ws = *src;

    for( ;; ) {
        wchar_t wc;
        char buf[ 8 ];
        size_t n;

        if( i >= nwc ) {
            if( dst && src ) { *src = ws; }
            return out;
        }

        wc = *ws;
        if( wc == 0 ) {
            if( dst ) {
                if( out < len ) { dst[ out ] = 0; }
                if( src ) { *src = NULL; }
            }
            return out;
        }

        n = wcrtomb( buf, wc, ps );
        if( n == (size_t)-1 ) {
            if( dst && src ) { *src = ws; }
            return (size_t)-1;
        }

        if( dst ) {
            if( out + n > len ) {
                if( src ) { *src = ws; }
                return out;
            }
            for( size_t j = 0; j < n; ++j ) { dst[ out + j ] = buf[ j ]; }
        }

        out += n;
        ++ws;
        ++i;
    }
}

size_t mbsnrtowcs( wchar_t* restrict dst, char const** restrict src, size_t nms,
    size_t len, mbstate_t* restrict ps ) {
    char const* mb;
    size_t out = 0;

    if( !src || !*src ) { return 0; }
    mb = *src;

    while( nms > 0 ) {
        wchar_t wc;
        size_t n;

        if( *mb == '\0' ) {
            if( dst ) {
                if( out < len ) { dst[ out ] = 0; }
                if( src ) { *src = NULL; }
            }
            return out;
        }

        if( dst && out >= len ) {
            if( src ) { *src = mb; }
            return out;
        }

        n = mbrtowc( &wc, mb, nms, ps );
        if( n == (size_t)-1 ) {
            if( dst && src ) { *src = mb; }
            return (size_t)-1;
        }
        if( n == (size_t)-2 ) {
            if( dst && src ) { *src = mb; }
            return out;
        }

        if( dst ) { dst[ out ] = wc; }
        mb += n;
        nms -= n;
        ++out;
    }

    if( dst && src ) { *src = mb; }
    return out;
}

int wcscoll( wchar_t const* s1, wchar_t const* s2 ) {
    if( __cwasm_locale_collate_utf8() ) { return __cwasm_wcscoll_utf8( s1, s2 ); }
    return wcscmp( s1, s2 );
}

size_t wcsxfrm( wchar_t* restrict dest, wchar_t const* restrict src, size_t n ) {
    if( __cwasm_locale_collate_utf8() ) { return __cwasm_wcsxfrm_utf8( dest, src, n ); }
    if( !src ) { return 0; }
    if( n > 0 && dest ) { wcsncpy( dest, src, n ); }
    return wcslen( src );
}
