#include <stddef.h>
#include <stdint.h>
#include <wctype.h>
#include <locale.h>
#include <wchar.h>

#include "unicode.h"

int __cwasm_locale_ctype_utf8( void );

static int __cwasm_isw_ascii( wint_t wc ) { return (uint32_t)wc < 128u; }

static int cwasm_uc_in_ranges( struct cwasm_uc_range const* r, unsigned n, uint32_t cp ) {
    unsigned lo = 0;
    unsigned hi = n;

    while( lo < hi ) {
        unsigned mid = ( lo + hi ) / 2u;
        if( cp > r[ mid ].hi ) { lo = mid + 1u; }
        else if( cp < r[ mid ].lo ) { hi = mid; }
        else { return 1; }
    }
    return 0;
}

static uint32_t cwasm_uc_case_map( struct cwasm_uc_case const* m, unsigned n, uint32_t cp ) {
    unsigned lo = 0;
    unsigned hi = n;

    while( lo < hi ) {
        unsigned mid = ( lo + hi ) / 2u;
        if( cp > m[ mid ].src ) { lo = mid + 1u; }
        else if( cp < m[ mid ].src ) { hi = mid; }
        else { return m[ mid ].dst; }
    }
    return cp;
}

static int cwasm_uc_test( struct cwasm_uc_range const* r, unsigned n, uint32_t cp ) {
    if( __cwasm_isw_ascii( (wint_t)cp ) ) { return 0; }
    if( !__cwasm_locale_ctype_utf8() ) { return 0; }
    return cwasm_uc_in_ranges( r, n, cp );
}

int iswalpha( wint_t wc ) {
    if( __cwasm_isw_ascii( wc ) ) {
        return ( (unsigned)wc - 'A' < 26u ) || ( (unsigned)wc - 'a' < 26u );
    }
    return cwasm_uc_test( cwasm_uc_alpha, cwasm_uc_alpha_n, (uint32_t)wc );
}

int iswdigit( wint_t wc ) {
    if( __cwasm_isw_ascii( wc ) ) { return (unsigned)wc - '0' < 10u; }
    return cwasm_uc_test( cwasm_uc_digit, cwasm_uc_digit_n, (uint32_t)wc );
}

int iswalnum( wint_t wc ) { return iswalpha( wc ) || iswdigit( wc ); }

int iswspace( wint_t wc ) {
    if( __cwasm_isw_ascii( wc ) ) {
        return wc == ' ' || wc == '\t' || wc == '\n' || wc == '\r' || wc == '\f' || wc == '\v';
    }
    return cwasm_uc_test( cwasm_uc_space, cwasm_uc_space_n, (uint32_t)wc );
}

int iswblank( wint_t wc ) {
    if( __cwasm_isw_ascii( wc ) ) { return wc == ' ' || wc == '\t'; }
    if( (uint32_t)wc == 0x00a0u ) { return __cwasm_locale_ctype_utf8(); }
    return cwasm_uc_test( cwasm_uc_blank, cwasm_uc_blank_n, (uint32_t)wc );
}

int iswlower( wint_t wc ) {
    if( __cwasm_isw_ascii( wc ) ) { return (unsigned)wc - 'a' < 26u; }
    return cwasm_uc_test( cwasm_uc_lower, cwasm_uc_lower_n, (uint32_t)wc );
}

int iswupper( wint_t wc ) {
    if( __cwasm_isw_ascii( wc ) ) { return (unsigned)wc - 'A' < 26u; }
    return cwasm_uc_test( cwasm_uc_upper, cwasm_uc_upper_n, (uint32_t)wc );
}

int iswxdigit( wint_t wc ) {
    uint32_t cp = (uint32_t)wc;

    if( __cwasm_isw_ascii( wc ) ) {
        return iswdigit( wc ) || (unsigned)wc - 'A' < 6u || (unsigned)wc - 'a' < 6u;
    }
    if( !__cwasm_locale_ctype_utf8() ) { return 0; }
    if( cp >= 0xff10u && cp <= 0xff19u ) { return 1; }
    if( cp >= 0xff21u && cp <= 0xff26u ) { return 1; }
    if( cp >= 0xff41u && cp <= 0xff46u ) { return 1; }
    return iswdigit( wc );
}

int iswcntrl( wint_t wc ) {
    if( __cwasm_isw_ascii( wc ) ) { return (unsigned)wc < 0x20u || wc == 0x7f; }
    return cwasm_uc_test( cwasm_uc_cntrl, cwasm_uc_cntrl_n, (uint32_t)wc );
}

int iswprint( wint_t wc ) {
    if( __cwasm_isw_ascii( wc ) ) { return (unsigned)wc - 0x20u < 0x5fu; }
    return cwasm_uc_test( cwasm_uc_print, cwasm_uc_print_n, (uint32_t)wc );
}

int iswgraph( wint_t wc ) {
    if( __cwasm_isw_ascii( wc ) ) { return iswprint( wc ) && !iswspace( wc ); }
    return cwasm_uc_test( cwasm_uc_graph, cwasm_uc_graph_n, (uint32_t)wc );
}

int iswpunct( wint_t wc ) {
    if( __cwasm_isw_ascii( wc ) ) { return iswgraph( wc ) && !iswalnum( wc ); }
    return cwasm_uc_test( cwasm_uc_punct, cwasm_uc_punct_n, (uint32_t)wc );
}

wint_t towlower( wint_t wc ) {
    if( __cwasm_isw_ascii( wc ) ) {
        return (unsigned)wc - 'A' < 26u ? (wint_t)( (unsigned)wc + ( 'a' - 'A' ) ) : wc;
    }
    if( !__cwasm_locale_ctype_utf8() ) { return wc; }
    return (wint_t)cwasm_uc_case_map( cwasm_uc_to_lower, cwasm_uc_to_lower_n, (uint32_t)wc );
}

wint_t towupper( wint_t wc ) {
    if( __cwasm_isw_ascii( wc ) ) {
        return (unsigned)wc - 'a' < 26u ? (wint_t)( (unsigned)wc - ( 'a' - 'A' ) ) : wc;
    }
    if( !__cwasm_locale_ctype_utf8() ) { return wc; }
    return (wint_t)cwasm_uc_case_map( cwasm_uc_to_upper, cwasm_uc_to_upper_n, (uint32_t)wc );
}

static int __cwasm_streq( char const* a, char const* b ) {
    while( *a && *b ) {
        if( *a != *b ) { return 0; }
        ++a;
        ++b;
    }
    return *a == *b;
}

wctype_t wctype( char const* property ) {
    if( !property ) { return 0; }
    if( __cwasm_streq( property, "alnum" ) ) { return 1; }
    if( __cwasm_streq( property, "alpha" ) ) { return 2; }
    if( __cwasm_streq( property, "blank" ) ) { return 3; }
    if( __cwasm_streq( property, "cntrl" ) ) { return 4; }
    if( __cwasm_streq( property, "digit" ) ) { return 5; }
    if( __cwasm_streq( property, "graph" ) ) { return 6; }
    if( __cwasm_streq( property, "lower" ) ) { return 7; }
    if( __cwasm_streq( property, "print" ) ) { return 8; }
    if( __cwasm_streq( property, "punct" ) ) { return 9; }
    if( __cwasm_streq( property, "space" ) ) { return 10; }
    if( __cwasm_streq( property, "upper" ) ) { return 11; }
    if( __cwasm_streq( property, "xdigit" ) ) { return 12; }
    return 0;
}

int iswctype( wint_t wc, wctype_t desc ) {
    switch( desc ) {
    case 1: return iswalnum( wc );
    case 2: return iswalpha( wc );
    case 3: return iswblank( wc );
    case 4: return iswcntrl( wc );
    case 5: return iswdigit( wc );
    case 6: return iswgraph( wc );
    case 7: return iswlower( wc );
    case 8: return iswprint( wc );
    case 9: return iswpunct( wc );
    case 10: return iswspace( wc );
    case 11: return iswupper( wc );
    case 12: return iswxdigit( wc );
    default: return 0;
    }
}

wctrans_t wctrans( char const* property ) {
    if( !property ) { return 0; }
    if( __cwasm_streq( property, "tolower" ) ) { return 1; }
    if( __cwasm_streq( property, "toupper" ) ) { return 2; }
    return 0;
}

wint_t towctrans( wint_t wc, wctrans_t desc ) {
    switch( desc ) {
    case 1: return towlower( wc );
    case 2: return towupper( wc );
    default: return wc;
    }
}

int wcwidth( wint_t wc ) {
    uint32_t cp = (uint32_t)wc;

    if( cp == 0 ) { return 0; }
    if( __cwasm_isw_ascii( wc ) ) {
        if( cp < 0x20u || cp == 0x7fu ) { return -1; }
        return 1;
    }
    if( !__cwasm_locale_ctype_utf8() ) { return ( cp <= 0x10ffffu ) ? 1 : -1; }
    if( cwasm_uc_test( cwasm_uc_cntrl, cwasm_uc_cntrl_n, cp ) ) { return -1; }
    if( cwasm_uc_test( cwasm_uc_combining, cwasm_uc_combining_n, cp ) ) { return 0; }
    if( !cwasm_uc_test( cwasm_uc_print, cwasm_uc_print_n, cp )
        && !cwasm_uc_test( cwasm_uc_space, cwasm_uc_space_n, cp ) ) {
        return -1;
    }
    if( cwasm_uc_test( cwasm_uc_wide, cwasm_uc_wide_n, cp ) ) { return 2; }
    return 1;
}

int wcswidth( wchar_t const* s, size_t n ) {
    int width = 0;

    if( !s ) { return 0; }
    while( n && *s ) {
        int cw = wcwidth( (wint_t)*s );
        if( cw < 0 ) { return -1; }
        width += cw;
        ++s;
        --n;
    }
    return width;
}

#define CWASM_COLLATE_STACK 12

static int cwasm_collate_is_combining( uint32_t cp ) {
    if( cp < 128u ) { return 0; }
    return cwasm_uc_in_ranges( cwasm_uc_combining, cwasm_uc_combining_n, cp );
}

static uint32_t cwasm_collate_fold( uint32_t cp ) {
    if( cp < 128u ) {
        if( cp >= 'A' && cp <= 'Z' ) { return cp + ( 'a' - 'A' ); }
        return cp;
    }
    return cwasm_uc_case_map( cwasm_uc_fold, cwasm_uc_fold_n, cp );
}

static struct cwasm_uc_decomp const* cwasm_collate_lookup_decomp( uint32_t cp ) {
    unsigned lo = 0;
    unsigned hi = cwasm_uc_decomp_n;

    while( lo < hi ) {
        unsigned mid = ( lo + hi ) / 2u;
        if( cp > cwasm_uc_decomp[ mid ].src ) { lo = mid + 1u; }
        else if( cp < cwasm_uc_decomp[ mid ].src ) { hi = mid; }
        else { return &cwasm_uc_decomp[ mid ]; }
    }
    return NULL;
}

static void cwasm_collate_hangul_parts( uint32_t cp, uint32_t* out, int* n ) {
    uint32_t s = cp - 0xac00u;
    uint32_t l = 0x1100u + s / ( 21u * 28u );
    uint32_t v = 0x1161u + ( s % ( 21u * 28u ) ) / 28u;
    uint32_t t = 0x11a7u + s % 28u;

    out[( *n )++ ] = l;
    out[( *n )++ ] = v;
    if( t != 0x11a7u ) { out[( *n )++ ] = t; }
}

static void cwasm_collate_nfd_expand( uint32_t cp, uint32_t* out, int* n ) {
    struct cwasm_uc_decomp const* d;

    if( cp >= 0xac00u && cp <= 0xd7a3u ) {
        cwasm_collate_hangul_parts( cp, out, n );
        return;
    }

    d = cwasm_collate_lookup_decomp( cp );
    if( !d ) {
        out[( *n )++ ] = cp;
        return;
    }

    cwasm_collate_nfd_expand( d->dst0, out, n );
    if( d->dst1 ) { cwasm_collate_nfd_expand( d->dst1, out, n ); }
}

static int cwasm_collate_decode_utf8( unsigned char const** ps, uint32_t* out ) {
    unsigned char const* s = *ps;
    uint8_t b0 = s[ 0 ];

    if( b0 < 0x80u ) {
        *out = b0;
        *ps = s + 1;
        return 1;
    }

    if( b0 >= 0xc2u && b0 <= 0xdfu ) {
        uint8_t b1 = s[ 1 ];
        if( !b1 || ( b1 & 0xc0u ) != 0x80u ) { goto raw; }
        *out = ( (uint32_t)( b0 & 0x1fu ) << 6 ) | (uint32_t)( b1 & 0x3fu );
        *ps = s + 2;
        return 1;
    }

    if( b0 >= 0xe0u && b0 <= 0xefu ) {
        uint8_t b1 = s[ 1 ];
        uint8_t b2;
        if( !b1 || ( b1 & 0xc0u ) != 0x80u ) { goto raw; }
        if( b0 == 0xe0u && b1 < 0xa0u ) { goto raw; }
        if( b0 == 0xedu && b1 >= 0xa0u ) { goto raw; }
        b2 = s[ 2 ];
        if( !b2 || ( b2 & 0xc0u ) != 0x80u ) { goto raw; }
        *out = ( (uint32_t)( b0 & 0x0fu ) << 12 ) |
            ( (uint32_t)( b1 & 0x3fu ) << 6 ) |
            (uint32_t)( b2 & 0x3fu );
        *ps = s + 3;
        return 1;
    }

    if( b0 >= 0xf0u && b0 <= 0xf4u ) {
        uint8_t b1 = s[ 1 ];
        uint8_t b2;
        uint8_t b3;
        if( !b1 || ( b1 & 0xc0u ) != 0x80u ) { goto raw; }
        if( b0 == 0xf0u && b1 < 0x90u ) { goto raw; }
        if( b0 == 0xf4u && b1 >= 0x90u ) { goto raw; }
        b2 = s[ 2 ];
        if( !b2 || ( b2 & 0xc0u ) != 0x80u ) { goto raw; }
        b3 = s[ 3 ];
        if( !b3 || ( b3 & 0xc0u ) != 0x80u ) { goto raw; }
        *out = ( (uint32_t)( b0 & 0x07u ) << 18 ) |
            ( (uint32_t)( b1 & 0x3fu ) << 12 ) |
            ( (uint32_t)( b2 & 0x3fu ) << 6 ) |
            (uint32_t)( b3 & 0x3fu );
        *ps = s + 4;
        return 1;
    }

raw:
    *out = b0;
    *ps = s + 1;
    return 1;
}

typedef struct {
    unsigned char const* p;
    uint32_t pending[ CWASM_COLLATE_STACK ];
    int pending_n;
    int eof;
} cwasm_collate_mb_iter;

typedef struct {
    wchar_t const* p;
    uint32_t pending[ CWASM_COLLATE_STACK ];
    int pending_n;
    int eof;
} cwasm_collate_wc_iter;

static void cwasm_collate_push_expanded( uint32_t cp, uint32_t* pending, int* pending_n ) {
    uint32_t expanded[ CWASM_COLLATE_STACK ];
    int n = 0;
    int i;

    cwasm_collate_nfd_expand( cp, expanded, &n );
    for( i = n - 1; i >= 0; --i ) { pending[( *pending_n )++ ] = expanded[ i ]; }
}

static void cwasm_collate_mb_fill( cwasm_collate_mb_iter* it ) {
    uint32_t cp;

    while( it->pending_n == 0 && !it->eof ) {
        if( !*it->p ) {
            it->eof = 1;
            break;
        }
        cwasm_collate_decode_utf8( &it->p, &cp );
        cwasm_collate_push_expanded( cp, it->pending, &it->pending_n );
    }
}

static void cwasm_collate_wc_fill( cwasm_collate_wc_iter* it ) {
    uint32_t cp;

    while( it->pending_n == 0 && !it->eof ) {
        if( !*it->p ) {
            it->eof = 1;
            break;
        }
        cp = (uint32_t) * it->p++;
        cwasm_collate_push_expanded( cp, it->pending, &it->pending_n );
    }
}

static int cwasm_collate_mb_next( cwasm_collate_mb_iter* it, uint32_t* out ) {
    for( ;; ) {
        cwasm_collate_mb_fill( it );
        if( it->pending_n == 0 ) { return 0; }
        uint32_t cp = it->pending[ --it->pending_n ];
        if( cwasm_collate_is_combining( cp ) ) { continue; }
        *out = cwasm_collate_fold( cp );
        return 1;
    }
}

static int cwasm_collate_wc_next( cwasm_collate_wc_iter* it, uint32_t* out ) {
    for( ;; ) {
        cwasm_collate_wc_fill( it );
        if( it->pending_n == 0 ) { return 0; }
        uint32_t cp = it->pending[ --it->pending_n ];
        if( cwasm_collate_is_combining( cp ) ) { continue; }
        *out = cwasm_collate_fold( cp );
        return 1;
    }
}

static int cwasm_collate_compare( uint32_t a, uint32_t b ) {
    if( a < b ) { return -1; }
    if( a > b ) { return 1; }
    return 0;
}

int __cwasm_strcoll_utf8( char const* s1, char const* s2 ) {
    cwasm_collate_mb_iter i1 = { (unsigned char const*)s1, {0}, 0, 0 };
    cwasm_collate_mb_iter i2 = { (unsigned char const*)s2, {0}, 0, 0 };
    uint32_t c1;
    uint32_t c2;
    int h1;
    int h2;
    int cmp;

    if( !s1 || !s2 ) { return s1 ? 1 : ( s2 ? -1 : 0 ); }

    for( ;; ) {
        h1 = cwasm_collate_mb_next( &i1, &c1 );
        h2 = cwasm_collate_mb_next( &i2, &c2 );
        if( !h1 && !h2 ) { return 0; }
        if( !h1 ) { return -1; }
        if( !h2 ) { return 1; }
        cmp = cwasm_collate_compare( c1, c2 );
        if( cmp ) { return cmp; }
    }
}

static size_t cwasm_collate_utf8_encode( char* dst, size_t n, uint32_t cp ) {
    if( cp < 0x80u ) {
        if( n > 0 && dst ) { dst[ 0 ] = (char)cp; }
        return 1;
    }
    if( cp < 0x800u ) {
        if( n > 0 && dst ) { dst[ 0 ] = (char)( 0xc0u | ( cp >> 6 ) ); }
        if( n > 1 && dst ) { dst[ 1 ] = (char)( 0x80u | ( cp & 0x3fu ) ); }
        return 2;
    }
    if( cp < 0x10000u ) {
        if( n > 0 && dst ) { dst[ 0 ] = (char)( 0xe0u | ( cp >> 12 ) ); }
        if( n > 1 && dst ) { dst[ 1 ] = (char)( 0x80u | ( ( cp >> 6 ) & 0x3fu ) ); }
        if( n > 2 && dst ) { dst[ 2 ] = (char)( 0x80u | ( cp & 0x3fu ) ); }
        return 3;
    }
    if( n > 0 && dst ) { dst[ 0 ] = (char)( 0xf0u | ( cp >> 18 ) ); }
    if( n > 1 && dst ) { dst[ 1 ] = (char)( 0x80u | ( ( cp >> 12 ) & 0x3fu ) ); }
    if( n > 2 && dst ) { dst[ 2 ] = (char)( 0x80u | ( ( cp >> 6 ) & 0x3fu ) ); }
    if( n > 3 && dst ) { dst[ 3 ] = (char)( 0x80u | ( cp & 0x3fu ) ); }
    return 4;
}

size_t __cwasm_strxfrm_utf8( char* dest, char const* src, size_t n ) {
    cwasm_collate_mb_iter it = { (unsigned char const*)src, {0}, 0, 0 };
    uint32_t cp;
    size_t total = 0;
    size_t written = 0;
    char scratch[ 4 ];

    if( !src ) { return 0; }

    while( cwasm_collate_mb_next( &it, &cp ) ) {
        size_t need = cwasm_collate_utf8_encode( scratch, sizeof( scratch ), cp );
        if( dest && n > written ) {
            size_t room = n - written;
            size_t copy = ( need < room ) ? need : room;
            size_t i;
            for( i = 0; i < copy; ++i ) { dest[ written + i ] = scratch[ i ]; }
            written += copy;
        }
        total += need;
    }

    if( dest && n > written ) { dest[ written ] = '\0'; }
    return total;
}

int __cwasm_wcscoll_utf8( wchar_t const* s1, wchar_t const* s2 ) {
    cwasm_collate_wc_iter i1 = { s1, {0}, 0, 0 };
    cwasm_collate_wc_iter i2 = { s2, {0}, 0, 0 };
    uint32_t c1;
    uint32_t c2;
    int h1;
    int h2;
    int cmp;

    if( !s1 || !s2 ) { return s1 ? 1 : ( s2 ? -1 : 0 ); }

    for( ;; ) {
        h1 = cwasm_collate_wc_next( &i1, &c1 );
        h2 = cwasm_collate_wc_next( &i2, &c2 );
        if( !h1 && !h2 ) { return 0; }
        if( !h1 ) { return -1; }
        if( !h2 ) { return 1; }
        cmp = cwasm_collate_compare( c1, c2 );
        if( cmp ) { return cmp; }
    }
}

size_t __cwasm_wcsxfrm_utf8( wchar_t* dest, wchar_t const* src, size_t n ) {
    cwasm_collate_wc_iter it = { src, {0}, 0, 0 };
    uint32_t cp;
    size_t total = 0;

    if( !src ) { return 0; }

    while( cwasm_collate_wc_next( &it, &cp ) ) {
        if( dest && n > 0 ) {
            dest[ total ] = (wchar_t)cp;
            --n;
        }
        ++total;
    }

    if( dest && n > 0 ) { dest[ total ] = L'\0'; }
    return total;
}
