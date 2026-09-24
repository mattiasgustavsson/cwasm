#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <wchar.h>
#include <uchar.h>
#include <wctype.h>

int __cwasm_locale_ctype_utf8( void );
extern char __cwasm_locale_radix_char( void );
extern void __cwasm_mbstate_reset( mbstate_t* ps );

#define CWASM_WIDE_BYTE_CHUNK 64u

extern mbstate_t __cwasm_wide_rd_state[ FOPEN_MAX ];
extern mbstate_t __cwasm_wide_wr_state[ FOPEN_MAX ];
extern wchar_t __cwasm_wide_unget_buf[ FOPEN_MAX ][ __CWASM_STDIO_UNGET ];
extern uint8_t __cwasm_wide_unget_len[ FOPEN_MAX ];
extern uint8_t __cwasm_wide_unget_nbytes[ FOPEN_MAX ];
extern uint32_t __cwasm_stdio_fh( FILE* stream );
extern void __cwasm_stdio_clear_eof( FILE* stream );
extern int __cwasm_mbstate_has_partial( mbstate_t const* ps );
extern int __cwasm_stdio_orient_wide( FILE* stream );
extern size_t __cwasm_stdio_fread_bytes( void* ptr, size_t size, size_t nmemb, FILE* stream );
extern size_t __cwasm_stdio_fwrite_bytes( void const* ptr, size_t size, size_t nmemb,
    FILE* stream );

static char __cwasm_wide_byte_buf[ FOPEN_MAX ][ CWASM_WIDE_BYTE_CHUNK ];
static uint8_t __cwasm_wide_byte_len[ FOPEN_MAX ];
static uint8_t __cwasm_wide_byte_pos[ FOPEN_MAX ];

void __cwasm_wide_bytebuf_reset( uint32_t h ) {
    if( h >= (uint32_t)FOPEN_MAX ) { return; }
    __cwasm_wide_byte_len[ h ] = 0;
    __cwasm_wide_byte_pos[ h ] = 0;
}

size_t __cwasm_wide_bytebuf_pending( uint32_t h ) {
    if( h >= (uint32_t)FOPEN_MAX ) { return 0; }
    return (size_t)__cwasm_wide_byte_len[ h ] - (size_t)__cwasm_wide_byte_pos[ h ];
}

static size_t cwasm_wide_char_nbytes( wchar_t wc ) {
    char tmp[ MB_LEN_MAX ];
    mbstate_t st;
    size_t n;

    __cwasm_mbstate_reset( &st );
    n = wcrtomb( tmp, wc, &st );
    return ( n == (size_t)-1 ) ? 1u : n;
}

static int cwasm_wide_refill( FILE* stream, uint32_t h ) {
    size_t keep = (size_t)__cwasm_wide_byte_len[ h ] - (size_t)__cwasm_wide_byte_pos[ h ];
    char* base = __cwasm_wide_byte_buf[ h ];
    size_t n;

    if( keep > 0 ) { memmove( base, base + __cwasm_wide_byte_pos[ h ], keep ); }
    n = __cwasm_stdio_fread_bytes( base + keep, 1, CWASM_WIDE_BYTE_CHUNK - keep, stream );
    if( n == 0 ) { return -1; }
    __cwasm_wide_byte_len[ h ] = (uint8_t)( keep + n );
    __cwasm_wide_byte_pos[ h ] = 0;
    return 0;
}

static wint_t cwasm_fgetwc_decoded( FILE* stream, uint32_t h ) {
    wchar_t wc;

    for( ;; ) {
        size_t avail = (size_t)__cwasm_wide_byte_len[ h ] - (size_t)__cwasm_wide_byte_pos[ h ];
        size_t r;

        if( avail == 0 ) {
            if( cwasm_wide_refill( stream, h ) != 0 ) {
                if( __cwasm_mbstate_has_partial( &__cwasm_wide_rd_state[ h ] ) ) {
                    errno = EILSEQ;
                }
                return WEOF;
            }
            avail = (size_t)__cwasm_wide_byte_len[ h ] - (size_t)__cwasm_wide_byte_pos[ h ];
            if( avail == 0 ) {
                if( __cwasm_mbstate_has_partial( &__cwasm_wide_rd_state[ h ] ) ) {
                    errno = EILSEQ;
                }
                return WEOF;
            }
        }

        r = mbrtowc( &wc,
            __cwasm_wide_byte_buf[ h ] + __cwasm_wide_byte_pos[ h ],
            avail,
            &__cwasm_wide_rd_state[ h ] );
        if( r == (size_t)-1 ) { return WEOF; }
        if( r == (size_t)-2 ) {
            // mbrtowc consumed these into the state, so pos must advance or they re-feed as EILSEQ.
            __cwasm_wide_byte_pos[ h ] += (uint8_t)avail;
            if( avail >= CWASM_WIDE_BYTE_CHUNK ) {
                errno = EILSEQ;
                return WEOF;
            }
            if( cwasm_wide_refill( stream, h ) != 0 ) {
                errno = EILSEQ;
                return WEOF;
            }
            continue;
        }

        __cwasm_wide_byte_pos[ h ] += (uint8_t)r;
        return (wint_t)wc;
    }
}

wint_t btowc( int c ) {
    if( c == EOF ) { return WEOF; }
    if( !__cwasm_locale_ctype_utf8() ) { return (wint_t)(unsigned char)c; }
    if( (unsigned char)c < 0x80u ) { return (wint_t)(unsigned char)c; }
    return WEOF;
}

int wctob( wint_t c ) {
    if( c == WEOF ) { return EOF; }
    if( !__cwasm_locale_ctype_utf8() ) {
        return (uint32_t)c > 0xffu ? EOF : (int)(unsigned char)c;
    }
    if( (uint32_t)c <= 0x7fu ) { return (int)(unsigned char)c; }
    return EOF;
}

size_t wcslen( wchar_t const* s ) {
    size_t n = 0;
    while( s && s[ n ] ) { ++n; }
    return n;
}

wchar_t* wcscpy( wchar_t* dst, wchar_t const* src ) {
    wchar_t* d = dst;
    while( ( *d++ = *src++ ) != 0 ) { ; }
    return dst;
}

wchar_t* wcsncpy( wchar_t* dst, wchar_t const* src, size_t n ) {
    wchar_t* d = dst;
    while( n && *src ) {
        *d++ = *src++;
        --n;
    }
    while( n ) {
        *d++ = 0;
        --n;
    }
    return dst;
}

int wcscmp( wchar_t const* a, wchar_t const* b ) {
    while( *a && *a == *b ) {
        ++a;
        ++b;
    }
    return *a < *b ? -1 : *a > *b ? 1 : 0;
}

int wcsncmp( wchar_t const* a, wchar_t const* b, size_t n ) {
    while( n && *a && *a == *b ) {
        ++a;
        ++b;
        --n;
    }
    return n == 0 ? 0 : *a < *b ? -1 : *a > *b ? 1 : 0;
}

wchar_t* wcschr( wchar_t const* s, wchar_t c ) {
    while( *s ) {
        if( *s == c ) { return (wchar_t*)s; }
        ++s;
    }
    return c == 0 ? (wchar_t*)s : NULL;
}

wint_t fgetwc( FILE* stream ) {
    uint32_t h;
    wint_t r = WEOF;

    if( !stream ) {
        errno = EINVAL;
        return WEOF;
    }
    flockfile( stream );
    if( __cwasm_stdio_orient_wide( stream ) != 0 ) { goto out; }
    h = __cwasm_stdio_fh( stream );
    if( h >= (uint32_t)FOPEN_MAX ) {
        errno = EINVAL;
        goto out;
    }

    if( __cwasm_wide_unget_len[ h ] > 0 ) {
        wchar_t wc = __cwasm_wide_unget_buf[ h ][ --__cwasm_wide_unget_len[ h ] ];
        size_t nb = cwasm_wide_char_nbytes( wc );
        if( __cwasm_wide_unget_nbytes[ h ] >= nb ) {
            __cwasm_wide_unget_nbytes[ h ] -= (uint8_t)nb;
        } else { __cwasm_wide_unget_nbytes[ h ] = 0; }
        r = (wint_t)wc;
        goto out;
    }

    r = cwasm_fgetwc_decoded( stream, h );
out:
    funlockfile( stream );
    return r;
}

wint_t getwchar( void ) { return fgetwc( stdin ); }

wint_t ungetwc( wint_t wc, FILE* stream ) {
    wint_t r = WEOF;
    uint32_t h;

    if( !stream ) {
        errno = EINVAL;
        return WEOF;
    }
    flockfile( stream );
    if( __cwasm_stdio_orient_wide( stream ) != 0 ) { goto out; }
    if( wc == WEOF ) { goto out; }

    h = __cwasm_stdio_fh( stream );
    if( h >= (uint32_t)FOPEN_MAX ) {
        errno = EINVAL;
        goto out;
    }
    if( __cwasm_wide_unget_len[ h ] >= __CWASM_STDIO_UNGET ) {
        errno = EINVAL;
        goto out;
    }
    if( __cwasm_mbstate_has_partial( &__cwasm_wide_rd_state[ h ] ) ) {
        errno = EINVAL;
        goto out;
    }

    __cwasm_wide_unget_buf[ h ][ __cwasm_wide_unget_len[ h ]++ ] = (wchar_t)wc;
    size_t nb = cwasm_wide_char_nbytes( (wchar_t)wc );
    if( __cwasm_wide_unget_nbytes[ h ] + nb <= 255u ) {
        __cwasm_wide_unget_nbytes[ h ] += (uint8_t)nb;
    } else { __cwasm_wide_unget_nbytes[ h ] = 255u; }
    __cwasm_stdio_clear_eof( stream );
    r = wc;
out:
    funlockfile( stream );
    return r;
}

wint_t fputwc( wchar_t wc, FILE* stream ) {
    uint32_t h;
    char buf[ 4 ];
    size_t nb;
    wint_t r = WEOF;

    if( !stream ) {
        errno = EINVAL;
        return WEOF;
    }
    flockfile( stream );
    if( __cwasm_stdio_orient_wide( stream ) != 0 ) { goto out; }
    h = __cwasm_stdio_fh( stream );
    if( h >= (uint32_t)FOPEN_MAX ) {
        errno = EINVAL;
        goto out;
    }

    nb = wcrtomb( buf, wc, &__cwasm_wide_wr_state[ h ] );
    if( nb == (size_t)-1 ) { goto out; }

    if( __cwasm_stdio_fwrite_bytes( buf, 1, nb, stream ) != nb ) { goto out; }
    r = (wint_t)wc;
out:
    funlockfile( stream );
    return r;
}

wint_t putwchar( wchar_t wc ) { return fputwc( wc, stdout ); }

wchar_t* fgetws( wchar_t* restrict s, int n, FILE* restrict stream ) {
    int i = 0;
    int err = 0;

    if( !s || n <= 0 ) {
        errno = EINVAL;
        return NULL;
    }

    flockfile( stream );
    while( i < n - 1 ) {
        wint_t wc = fgetwc( stream );
        if( wc == WEOF ) {
            err = ferror( stream ) != 0;
            break;
        }
        s[ i++ ] = (wchar_t)wc;
        if( wc == L'\n' ) { break; }
    }
    funlockfile( stream );

    if( err ) { return NULL; }
    if( i == 0 && n > 1 ) { return NULL; }
    s[ i ] = 0;
    return s;
}

int fputws( wchar_t const* restrict s, FILE* restrict stream ) {
    if( !stream ) {
        errno = EINVAL;
        return EOF;
    }
    if( !s ) {
        errno = EINVAL;
        return EOF;
    }
    flockfile( stream );
    if( __cwasm_stdio_orient_wide( stream ) != 0 ) {
        funlockfile( stream );
        return EOF;
    }
    while( *s ) {
        if( fputwc( *s++, stream ) == WEOF ) {
            funlockfile( stream );
            return EOF;
        }
    }
    funlockfile( stream );
    return 0;
}

wchar_t* wcscat( wchar_t* dst, wchar_t const* src ) {
    wchar_t* d = dst;
    while( *d ) { ++d; }
    while( ( *d++ = *src++ ) ) { ; }
    return dst;
}

wchar_t* wcsrchr( wchar_t const* s, wchar_t c ) {
    wchar_t const* last = NULL;
    do {
        if( *s == c ) { last = s; }
    } while( *s++ );
    return (wchar_t*)last;
}

wchar_t* wcsstr( wchar_t const* haystack, wchar_t const* needle ) {
    if( !*needle ) { return (wchar_t*)haystack; }
    for( ; *haystack; ++haystack ) {
        wchar_t const* h = haystack;
        wchar_t const* n = needle;
        while( *h && *n && *h == *n ) {
            ++h;
            ++n;
        }
        if( !*n ) { return (wchar_t*)haystack; }
    }
    return NULL;
}

wchar_t* wcspbrk( wchar_t const* s, wchar_t const* accept ) {
    wchar_t const* a;
    for( ; *s; ++s ) {
        for( a = accept; *a; ++a ) {
            if( *s == *a ) { return (wchar_t*)s; }
        }
    }
    return NULL;
}

size_t wcsspn( wchar_t const* s, wchar_t const* accept ) {
    size_t n = 0;
    for( ; *s; s++, n++ ) {
        wchar_t const* a;
        int found = 0;
        for( a = accept; *a; ++a ) {
            if( *s == *a ) {
                found = 1;
                break;
            }
        }
        if( !found ) { break; }
    }
    return n;
}

int wmemcmp( wchar_t const* a, wchar_t const* b, size_t n ) {
    for( ; n; n--, a++, b++ ) {
        if( *a != *b ) { return *a < *b ? -1 : 1; }
    }
    return 0;
}

wchar_t* wcsdup( wchar_t const* s ) {
    size_t len;
    wchar_t* p;

    if( !s ) { return NULL; }
    len = wcslen( s );
    p = (wchar_t*)malloc( ( len + 1u ) * sizeof( wchar_t ) );
    if( !p ) { return NULL; }
    return wcscpy( p, s );
}

wchar_t* wcsncat( wchar_t* dst, wchar_t const* src, size_t n ) {
    wchar_t* d = dst;

    while( *d ) { ++d; }
    while( n && *src ) { n--, * d++ = *src++; }
    *d = 0;
    return dst;
}

size_t wcsnlen( wchar_t const* s, size_t maxlen ) {
    size_t n = 0;
    while( n < maxlen && s[ n ] ) { ++n; }
    return n;
}

size_t wcscspn( wchar_t const* s, wchar_t const* reject ) {
    size_t n = 0;

    for( ; s[ n ]; ++n ) {
        wchar_t const* r;
        for( r = reject; *r; ++r ) {
            if( s[ n ] == *r ) { return n; }
        }
    }
    return n;
}

wchar_t* wcstok( wchar_t* restrict s, wchar_t const* restrict delim,
    wchar_t** restrict ptr ) {
    wchar_t* token;

    if( !delim || !ptr ) { return NULL; }
    if( !s ) { s = *ptr; }
    if( !s ) { return NULL; }
    s += wcsspn( s, delim );
    if( !*s ) {
        *ptr = NULL;
        return NULL;
    }
    token = s;
    s += wcscspn( s, delim );
    if( *s ) {
        *s++ = 0;
        *ptr = s;
    } else {
        *ptr = NULL;
    }
    return token;
}

wchar_t* wmemchr( wchar_t const* s, wchar_t c, size_t n ) {
    for( ; n; n--, s++ ) {
        if( *s == c ) { return (wchar_t*)s; }
    }
    return NULL;
}

wchar_t* wmemcpy( wchar_t* restrict dst, wchar_t const* restrict src, size_t n ) {
    wchar_t* d = dst;
    while( n-- ) { *d++ = *src++; }
    return dst;
}

wchar_t* wmemmove( wchar_t* dst, wchar_t const* src, size_t n ) {
    wchar_t const* s = src;
    wchar_t* d = dst;

    if( d == s || n == 0 ) { return dst; }
    if( d < s ) {
        while( n-- ) { *d++ = *s++; }
    } else {
        d += n;
        s += n;
        while( n-- ) { *--d = *--s; }
    }
    return dst;
}

wchar_t* wmemset( wchar_t* s, wchar_t c, size_t n ) {
    wchar_t* p = s;
    while( n-- ) { *p++ = c; }
    return s;
}

static wchar_t const* cwasm_wc_skip_ws( wchar_t const* s ) {
    while( *s && iswspace( (wint_t)*s ) ) { ++s; }
    return s;
}

static int cwasm_wc_digit_val( wchar_t c, int base ) {
    uint32_t wc = (uint32_t)c;

    if( wc >= L'0' && wc <= L'9' ) { return (int)( wc - L'0' ); }
    if( base <= 10 ) { return -1; }
    if( wc >= L'a' && wc <= L'z' ) { return (int)( wc - L'a' ) + 10; }
    if( wc >= L'A' && wc <= L'Z' ) { return (int)( wc - L'A' ) + 10; }
    return -1;
}

static int cwasm_wc_parse_sign( wchar_t const** ps ) {
    wchar_t const* s = *ps;
    int neg = ( *s == L'-' );

    if( *s == L'+' || neg ) { ++s; }
    *ps = s;
    return neg;
}

static unsigned long cwasm_wcstoul_core( wchar_t const* nptr, wchar_t** endptr,
    int base, int* perr ) {
    wchar_t const* s = cwasm_wc_skip_ws( nptr );
    int neg = cwasm_wc_parse_sign( &s );
    unsigned long acc = 0;
    int any = 0;
    unsigned long cutoff;
    unsigned long cutlim;

    if( base && ( base < 2 || base > 36 ) ) {
        if( endptr ) { *endptr = (wchar_t*)nptr; }
        if( perr ) { *perr = EINVAL; }
        return 0;
    }

    if( base == 0 ) {
        if( s[ 0 ] == L'0' && ( s[ 1 ] == L'x' || s[ 1 ] == L'X' ) ) { base = 16; }
        else if( s[ 0 ] == L'0' ) { base = 8; }
        else { base = 10; }
    }

    if( base == 16 && s[ 0 ] == L'0' && ( s[ 1 ] == L'x' || s[ 1 ] == L'X' ) ) {
        int pd = cwasm_wc_digit_val( s[ 2 ], 16 );
        if( pd >= 0 && pd < 16 ) { s += 2; }
    }

    cutoff = ULONG_MAX / (unsigned long)base;
    cutlim = ULONG_MAX % (unsigned long)base;

    while( *s ) {
        int d = cwasm_wc_digit_val( *s, base );
        if( d < 0 || d >= base ) { break; }
        any = 1;
        if( acc > cutoff || ( acc == cutoff && (unsigned long)d > cutlim ) ) {
            if( perr ) { *perr = ERANGE; }
            acc = ULONG_MAX;
            ++s;
            while( *s ) {
                d = cwasm_wc_digit_val( *s, base );
                if( d < 0 || d >= base ) { break; }
                ++s;
            }
            break;
        }
        acc = acc * (unsigned long)base + (unsigned long)d;
        ++s;
    }

    if( !any ) {
        if( endptr ) { *endptr = (wchar_t*)nptr; }
        return 0;
    }
    if( endptr ) { *endptr = (wchar_t*)s; }

    if( neg && !( perr && *perr == ERANGE ) ) { acc = (unsigned long)( 0ul - acc ); }
    return acc;
}

static unsigned long long cwasm_wcstoull_core( wchar_t const* nptr, wchar_t** endptr,
    int base, int* perr ) {
    wchar_t const* s = cwasm_wc_skip_ws( nptr );
    int neg = cwasm_wc_parse_sign( &s );
    unsigned long long acc = 0;
    int any = 0;
    unsigned long long cutoff;
    unsigned long long cutlim;

    if( base && ( base < 2 || base > 36 ) ) {
        if( endptr ) { *endptr = (wchar_t*)nptr; }
        if( perr ) { *perr = EINVAL; }
        return 0;
    }

    if( base == 0 ) {
        if( s[ 0 ] == L'0' && ( s[ 1 ] == L'x' || s[ 1 ] == L'X' ) ) { base = 16; }
        else if( s[ 0 ] == L'0' ) { base = 8; }
        else { base = 10; }
    }

    if( base == 16 && s[ 0 ] == L'0' && ( s[ 1 ] == L'x' || s[ 1 ] == L'X' ) ) {
        int pd = cwasm_wc_digit_val( s[ 2 ], 16 );
        if( pd >= 0 && pd < 16 ) { s += 2; }
    }

    cutoff = ULLONG_MAX / (unsigned long long)base;
    cutlim = ULLONG_MAX % (unsigned long long)base;

    while( *s ) {
        int d = cwasm_wc_digit_val( *s, base );
        if( d < 0 || d >= base ) { break; }
        any = 1;
        if( acc > cutoff || ( acc == cutoff && (unsigned long long)d > cutlim ) ) {
            if( perr ) { *perr = ERANGE; }
            acc = ULLONG_MAX;
            ++s;
            while( *s ) {
                d = cwasm_wc_digit_val( *s, base );
                if( d < 0 || d >= base ) { break; }
                ++s;
            }
            break;
        }
        acc = acc * (unsigned long long)base + (unsigned long long)d;
        ++s;
    }

    if( !any ) {
        if( endptr ) { *endptr = (wchar_t*)nptr; }
        return 0;
    }
    if( endptr ) { *endptr = (wchar_t*)s; }
    if( neg && !( perr && *perr == ERANGE ) ) {
        acc = (unsigned long long)( 0ull - acc );
    }
    return acc;
}

unsigned long wcstoul( wchar_t const* restrict nptr, wchar_t** restrict endptr, int base ) {
    int e = 0;
    unsigned long r = cwasm_wcstoul_core( nptr, endptr, base, &e );
    if( e ) { errno = e; }
    return r;
}

long wcstol( wchar_t const* restrict nptr, wchar_t** restrict endptr, int base ) {
    int e = 0;
    wchar_t const* s = cwasm_wc_skip_ws( nptr );
    int neg = ( *s == L'-' );
    unsigned long ur = cwasm_wcstoul_core( nptr, endptr, base, &e );

    if( endptr && *endptr == (wchar_t*)nptr ) {
        return 0;
    }

    if( e == ERANGE ) {
        errno = ERANGE;
        return neg ? LONG_MIN : LONG_MAX;
    }
    if( neg ) {
        wchar_t* ep2 = NULL;
        wchar_t const* mag_s = cwasm_wc_skip_ws( nptr );
        (void)cwasm_wc_parse_sign( &mag_s );
        int e2 = 0;
        unsigned long mag = cwasm_wcstoul_core( mag_s, &ep2, base, &e2 );
        if( endptr ) { *endptr = ep2; }
        if( e2 == ERANGE ) {
            errno = ERANGE;
            return LONG_MIN;
        }
        if( mag > (unsigned long)LONG_MAX + 1ul ) {
            errno = ERANGE;
            return LONG_MIN;
        }
        if( mag == (unsigned long)LONG_MAX + 1ul ) { return LONG_MIN; }
        return -(long)mag;
    }
    if( ur > (unsigned long)LONG_MAX ) {
        errno = ERANGE;
        return LONG_MAX;
    }
    if( e ) { errno = e; }
    return (long)ur;
}

unsigned long long wcstoull( wchar_t const* restrict nptr, wchar_t** restrict endptr, int base ) {
    int e = 0;
    unsigned long long r = cwasm_wcstoull_core( nptr, endptr, base, &e );
    if( e ) { errno = e; }
    return r;
}

long long wcstoll( wchar_t const* restrict nptr, wchar_t** restrict endptr, int base ) {
    int e = 0;
    wchar_t const* s = cwasm_wc_skip_ws( nptr );
    int neg = ( *s == L'-' );
    unsigned long long ur = cwasm_wcstoull_core( nptr, endptr, base, &e );

    if( endptr && *endptr == (wchar_t*)nptr ) {
        return 0;
    }

    if( e == ERANGE ) {
        errno = ERANGE;
        return neg ? LLONG_MIN : LLONG_MAX;
    }
    if( neg ) {
        wchar_t* ep2 = NULL;
        wchar_t const* mag_s = cwasm_wc_skip_ws( nptr );
        (void)cwasm_wc_parse_sign( &mag_s );
        int e2 = 0;
        unsigned long long mag = cwasm_wcstoull_core( mag_s, &ep2, base, &e2 );
        if( endptr ) { *endptr = ep2; }
        if( e2 == ERANGE ) {
            errno = ERANGE;
            return LLONG_MIN;
        }
        if( mag > (unsigned long long)LLONG_MAX + 1ull ) {
            errno = ERANGE;
            return LLONG_MIN;
        }
        if( mag == (unsigned long long)LLONG_MAX + 1ull ) { return LLONG_MIN; }
        return -(long long)mag;
    }
    if( ur > (unsigned long long)LLONG_MAX ) {
        errno = ERANGE;
        return LLONG_MAX;
    }
    if( e ) { errno = e; }
    return (long long)ur;
}

static wchar_t const* cwasm_wcs_mbs_prefix_end( wchar_t const* ws, size_t nbytes ) {
    mbstate_t st;
    wchar_t const* p = ws;
    size_t used = 0;

    __cwasm_mbstate_reset( &st );
    while( *p && used < nbytes ) {
        char mb[ 8 ];
        size_t n = wcrtomb( mb, *p, &st );
        if( n == (size_t)-1 ) { break; }
        if( used + n > nbytes ) { break; }
        used += n;
        ++p;
    }
    return p;
}

static void cwasm_apply_radix_to_mbs( char* buf, size_t n, char radix ) {
    size_t i;

    if( radix == '.' ) { return; }
    for( i = 0; i < n; ++i ) {
        if( buf[ i ] == '.' ) { buf[ i ] = radix; }
    }
}

#define CWASM_WCS_MBS_STACK 512

static char* cwasm_wcs_to_mbs( wchar_t const* restrict nptr, wchar_t** restrict endptr,
    char* stack_buf, char** heap_buf, wchar_t const** ws_start,
    size_t* need_out ) {
    wchar_t const* s = cwasm_wc_skip_ws( nptr );
    char* mbs;
    size_t wl;
    wchar_t const* p;
    mbstate_t st;
    size_t need;

    if( !s || !*s ) {
        if( endptr ) { *endptr = (wchar_t*)nptr; }
        return NULL;
    }

    wl = wcslen( s );
    p = s;
    __cwasm_mbstate_reset( &st );
    need = wcsrtombs( NULL, &p, wl, &st );
    if( need == (size_t)-1 ) {
        if( endptr ) { *endptr = (wchar_t*)nptr; }
        return NULL;
    }

    if( need + 1u <= CWASM_WCS_MBS_STACK ) { mbs = stack_buf; }
    else {
        *heap_buf = (char*)malloc( need + 1u );
        if( !*heap_buf ) {
            if( endptr ) { *endptr = (wchar_t*)nptr; }
            return NULL;
        }
        mbs = *heap_buf;
    }

    p = s;
    __cwasm_mbstate_reset( &st );
    if( wcsrtombs( mbs, &p, wl, &st ) == (size_t)-1 ) {
        free( *heap_buf );
        *heap_buf = NULL;
        if( endptr ) { *endptr = (wchar_t*)nptr; }
        return NULL;
    }
    mbs[ need ] = '\0';
    *ws_start = s;
    *need_out = need;
    return mbs;
}

static void cwasm_wcstoflt_endptr( wchar_t const* nptr, wchar_t const* ws_start, char* mbs,
    char* e, wchar_t** endptr ) {
    if( !endptr ) { return; }
    if( !e || e == mbs ) { *endptr = (wchar_t*)nptr; }
    else {
        *endptr = (wchar_t*)cwasm_wcs_mbs_prefix_end( ws_start, (size_t)( e - mbs ) );
    }
}

double wcstod( wchar_t const* restrict nptr, wchar_t** restrict endptr ) {
    char stack_buf[ CWASM_WCS_MBS_STACK ];
    char* heap_buf = NULL;
    wchar_t const* s;
    char* mbs;
    char* e = NULL;
    size_t need;
    char radix = __cwasm_locale_radix_char();

    mbs = cwasm_wcs_to_mbs( nptr, endptr, stack_buf, &heap_buf, &s, &need );
    if( !mbs ) { return 0.0; }

    cwasm_apply_radix_to_mbs( mbs, need, radix );
    double r = strtod( mbs, &e );
    cwasm_wcstoflt_endptr( nptr, s, mbs, e, endptr );
    free( heap_buf );
    return r;
}

float wcstof( wchar_t const* restrict nptr, wchar_t** restrict endptr ) {
    char stack_buf[ CWASM_WCS_MBS_STACK ];
    char* heap_buf = NULL;
    wchar_t const* s;
    char* mbs;
    char* e = NULL;
    size_t need;
    char radix = __cwasm_locale_radix_char();

    mbs = cwasm_wcs_to_mbs( nptr, endptr, stack_buf, &heap_buf, &s, &need );
    if( !mbs ) { return 0.0f; }

    cwasm_apply_radix_to_mbs( mbs, need, radix );
    float r = strtof( mbs, &e );
    cwasm_wcstoflt_endptr( nptr, s, mbs, e, endptr );
    free( heap_buf );
    return r;
}

long double wcstold( wchar_t const* restrict nptr, wchar_t** restrict endptr ) {
    char stack_buf[ CWASM_WCS_MBS_STACK ];
    char* heap_buf = NULL;
    wchar_t const* s;
    char* mbs;
    char* e = NULL;
    size_t need;
    char radix = __cwasm_locale_radix_char();

    mbs = cwasm_wcs_to_mbs( nptr, endptr, stack_buf, &heap_buf, &s, &need );
    if( !mbs ) { return 0.0L; }

    cwasm_apply_radix_to_mbs( mbs, need, radix );
    long double r = strtold( mbs, &e );
    cwasm_wcstoflt_endptr( nptr, s, mbs, e, endptr );
    free( heap_buf );
    return r;
}


size_t wcsftime( wchar_t* restrict wcs, size_t maxsize,
    wchar_t const* restrict format, struct tm const* restrict timeptr ) {
    char fmt_stack[ 256 ];
    char out_stack[ 512 ];
    char* fmt_heap = NULL;
    char* out_heap = NULL;
    char* fmt = fmt_stack;
    char* out = out_stack;
    size_t outcap = sizeof( out_stack );
    wchar_t const* wp;
    char const* cp;
    mbstate_t st;
    size_t need;
    size_t produced = 0;
    size_t result = 0;

    if( !wcs || maxsize == 0 || !format || !timeptr ) {
        errno = EINVAL;
        return 0;
    }

    wp = format;
    __cwasm_mbstate_reset( &st );
    need = wcsrtombs( NULL, &wp, 0, &st );
    if( need == (size_t)-1 ) {
        errno = EILSEQ;
        return 0;
    }
    if( need + 1u > sizeof( fmt_stack ) ) {
        fmt_heap = (char*)malloc( need + 1u );
        if( !fmt_heap ) { return 0; }
        fmt = fmt_heap;
    }
    wp = format;
    __cwasm_mbstate_reset( &st );
    if( wcsrtombs( fmt, &wp, need + 1u, &st ) == (size_t)-1 ) {
        free( fmt_heap );
        errno = EILSEQ;
        return 0;
    }

    if( maxsize < (size_t)-1 / (size_t)MB_LEN_MAX ) {
        size_t want = maxsize * (size_t)MB_LEN_MAX + 1u;
        if( want > outcap ) {
            out_heap = (char*)malloc( want );
            if( out_heap ) {
                out = out_heap;
                outcap = want;
            }
        }
    }

    if( fmt[ 0 ] == '\0' ) {
        wcs[ 0 ] = L'\0';
        free( fmt_heap );
        free( out_heap );
        return 0;
    }

    if( strftime( out, outcap, fmt, timeptr ) == 0 ) {
        free( fmt_heap );
        free( out_heap );
        return 0;
    }

    cp = out;
    __cwasm_mbstate_reset( &st );
    produced = mbsrtowcs( wcs, &cp, maxsize, &st );
    if( produced == (size_t)-1 ) {
        free( fmt_heap );
        free( out_heap );
        errno = EILSEQ;
        return 0;
    }
    if( produced >= maxsize ) {
        free( fmt_heap );
        free( out_heap );
        return 0;
    }
    wcs[ produced ] = L'\0';
    result = produced;

    free( fmt_heap );
    free( out_heap );
    return result;
}

int mbsinit( mbstate_t const* ps ) {
    if( !ps ) { return 1; }
    return !__cwasm_mbstate_has_partial( ps );
}
