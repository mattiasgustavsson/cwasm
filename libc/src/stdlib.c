#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <float.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "cwasm_fs.h"
#include "cmdline.h"
#include "cwasm_lock.h"

#ifndef CWASM_THREADS
    int errno;
#endif

extern char __cwasm_locale_radix_char( void );

__attribute__( ( import_module( "env" ), import_name( "__exit" ) ) ) void __exit( int status );

#include <cwasm.h>
#include <sys/random.h>

CWASM_JS_LIB( RAND, int32_t, js_getentropy, ( uint32_t buf, uint32_t len ), {
  if (typeof crypto === 'undefined' || !crypto.getRandomValues)
    return -1;
  // getRandomValues writes into the view, but rejects shared views - fill a plain buffer then copy in
  var tmp = new Uint8Array(len);
  crypto.getRandomValues(tmp);
  mem.bytes_write(buf, tmp);
  return 0;
})

int getentropy( void* buffer, size_t length ) {
    if( !buffer || length == 0 ) {
        errno = EINVAL;
        return -1;
    }
    unsigned char* p = (unsigned char*)buffer;
    while( length > 0 ) {
        size_t chunk = length > 256 ? 256 : length;
        if( js_getentropy( (uint32_t)(uintptr_t)p, (uint32_t)chunk ) != 0 ) {
            errno = EIO;
            return -1;
        }
        p += chunk;
        length -= chunk;
    }
    return 0;
}

struct cwasm_rand_state { uint32_t state[ 17 ]; };
struct cwasm_rand_state cwasm_rand_state;
static int cwasm_rand_seeded;

int rand( void ) {
    if( !cwasm_rand_seeded ) { srand( 0u ); }

    uint32_t idx = cwasm_rand_state.state[ 16 ] & 15u;
    uint32_t a = cwasm_rand_state.state[ idx ];
    uint32_t c = cwasm_rand_state.state[( idx + 13u ) & 15u ];
    uint32_t b = a ^ c ^ ( a << 16 ) ^ ( c << 15 );
    c = cwasm_rand_state.state[( idx + 9u ) & 15u ];
    c ^= ( c >> 11 );
    a = cwasm_rand_state.state[ idx ] = b ^ c;
    uint32_t d = a ^ ( ( a << 5 ) & 0xda442d24u );
    cwasm_rand_state.state[ 16 ] = ( idx + 15u ) & 15u;
    a = cwasm_rand_state.state[ cwasm_rand_state.state[ 16 ] ];
    cwasm_rand_state.state[ cwasm_rand_state.state[ 16 ] ] =
    a ^ b ^ d ^ ( a << 2 ) ^ ( b << 18 ) ^ ( c << 28 );
    return (int)( cwasm_rand_state.state[ cwasm_rand_state.state[ 16 ] ] & (uint32_t)RAND_MAX );
}

static uint32_t cwasm_mix32( uint32_t h ) {
    h ^= h >> 16;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;
    return h;
}

void srand( unsigned int seed ) {
    cwasm_rand_seeded = 1;
    uint32_t h = cwasm_mix32( ( seed << 1u ) | 1u );
    cwasm_rand_state.state[ 16 ] = 0u;
    cwasm_rand_state.state[ 0 ] = h ^ 0xf68a9fc1u;
    for( uint32_t i = 1; i < 16u; ++i ) {
        uint32_t x = cwasm_rand_state.state[ i - 1u ];
        cwasm_rand_state.state[ i ] = 0x6c078965u * ( x ^ ( x >> 30 ) ) + i;
    }
}

int rand_r( unsigned int* seed ) {
    if( !seed ) {
        errno = EINVAL;
        return 0;
    }
    *seed += 0x9e3779b9u;
    return (int)( cwasm_mix32( *seed ) & (unsigned)RAND_MAX );
}

char* realpath( char const* path, char* resolved_path ) {
    char canon[ CWASM_FS_PATH_MAX ];
    char* out;
    size_t len;

    if( !path ) {
        errno = EINVAL;
        return NULL;
    }
    if( cwasm_fs_resolve( path, canon, sizeof( canon ) ) != 0 ) { return NULL; }
    len = strlen( canon );
    if( !resolved_path ) {
        out = (char*)malloc( len + 1u );
        if( !out ) {
            errno = ENOMEM;
            return NULL;
        }
    } else {
        if( len + 1u > CWASM_FS_PATH_MAX ) {
            errno = ENAMETOOLONG;
            return NULL;
        }
        out = resolved_path;
    }
    memcpy( out, canon, len + 1u );
    return out;
}

static char const* cwasm_skip_ws( char const* s ) {
    while( *s && ( *s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' || *s == '\f' || *s == '\v' ) ) {
        ++s;
    }
    return s;
}

static int cwasm_digit_val( int c ) {
    if( c >= '0' && c <= '9' ) { return c - '0'; }
    if( c >= 'a' && c <= 'z' ) { return c - 'a' + 10; }
    if( c >= 'A' && c <= 'Z' ) { return c - 'A' + 10; }
    return -1;
}

static int cwasm_parse_sign( char const** ps ) {
    char const* s = *ps;
    int neg = ( *s == '-' );
    if( *s == '+' || neg ) { ++s; }
    *ps = s;
    return neg;
}

static unsigned long cwasm_strtoul_core( char const* nptr, char** endptr, int base, int* perr ) {
    char const* s = cwasm_skip_ws( nptr );
    int neg = cwasm_parse_sign( &s );

    if( base && ( base < 2 || base > 36 ) ) {
        if( endptr ) { *endptr = (char*)nptr; }
        if( perr ) { *perr = EINVAL; }
        return 0;
    }

    if( base == 0 ) {
        if( s[ 0 ] == '0' && ( s[ 1 ] == 'x' || s[ 1 ] == 'X' ) ) { base = 16; }
        else if( s[ 0 ] == '0' ) { base = 8; }
        else { base = 10; }
    }

    if( base == 16 && s[ 0 ] == '0' && ( s[ 1 ] == 'x' || s[ 1 ] == 'X' ) ) {
        int pd = cwasm_digit_val( (unsigned char)s[ 2 ] );
        if( pd >= 0 && pd < 16 ) { s += 2; }
    }

    unsigned long acc = 0;
    int any = 0;
    unsigned long cutoff = ULONG_MAX / (unsigned long)base;
    unsigned long cutlim = ULONG_MAX % (unsigned long)base;
    while( *s ) {
        int d = cwasm_digit_val( (unsigned char)*s );
        if( d < 0 || d >= base ) { break; }
        any = 1;
        if( acc > cutoff || ( acc == cutoff && (unsigned long)d > cutlim ) ) {
            if( perr ) { *perr = ERANGE; }
            acc = ULONG_MAX;
            ++s;
            while( *s ) {
                d = cwasm_digit_val( (unsigned char)*s );
                if( d < 0 || d >= base ) { break; }
                ++s;
            }
            break;
        }
        acc = acc * (unsigned long)base + (unsigned long)d;
        ++s;
    }

    if( !any ) {
        if( endptr ) { *endptr = (char*)nptr; }
        return 0;
    }
    if( endptr ) { *endptr = (char*)s; }

    if( neg && !( perr && *perr == ERANGE ) ) { acc = (unsigned long)( 0ul - acc ); }
    return acc;
}

static unsigned long long cwasm_strtoull_core( char const* nptr, char** endptr, int base, int* perr ) {
    char const* s = cwasm_skip_ws( nptr );
    int neg = cwasm_parse_sign( &s );

    if( base && ( base < 2 || base > 36 ) ) {
        if( endptr ) { *endptr = (char*)nptr; }
        if( perr ) { *perr = EINVAL; }
        return 0;
    }

    if( base == 0 ) {
        if( s[ 0 ] == '0' && ( s[ 1 ] == 'x' || s[ 1 ] == 'X' ) ) { base = 16; }
        else if( s[ 0 ] == '0' ) { base = 8; }
        else { base = 10; }
    }

    if( base == 16 && s[ 0 ] == '0' && ( s[ 1 ] == 'x' || s[ 1 ] == 'X' ) ) {
        int pd = cwasm_digit_val( (unsigned char)s[ 2 ] );
        if( pd >= 0 && pd < 16 ) { s += 2; }
    }

    unsigned long long acc = 0;
    int any = 0;
    unsigned long long cutoff = ULLONG_MAX / (unsigned long long)base;
    unsigned long long cutlim = ULLONG_MAX % (unsigned long long)base;
    while( *s ) {
        int d = cwasm_digit_val( (unsigned char)*s );
        if( d < 0 || d >= base ) { break; }
        any = 1;
        if( acc > cutoff || ( acc == cutoff && (unsigned long long)d > cutlim ) ) {
            if( perr ) { *perr = ERANGE; }
            acc = ULLONG_MAX;
            ++s;
            while( *s ) {
                d = cwasm_digit_val( (unsigned char)*s );
                if( d < 0 || d >= base ) { break; }
                ++s;
            }
            break;
        }
        acc = acc * (unsigned long long)base + (unsigned long long)d;
        ++s;
    }

    if( !any ) {
        if( endptr ) { *endptr = (char*)nptr; }
        return 0;
    }
    if( endptr ) { *endptr = (char*)s; }

    if( neg && !( perr && *perr == ERANGE ) ) { acc = (unsigned long long)( 0ull - acc ); }
    return acc;
}

unsigned long long strtoull( char const* restrict nptr, char** restrict endptr, int base ) {
    int e = 0;
    unsigned long long r = cwasm_strtoull_core( nptr, endptr, base, &e );
    if( e ) { errno = e; }
    return r;
}

unsigned long strtoul( char const* restrict nptr, char** restrict endptr, int base ) {
    int e = 0;
    unsigned long r = cwasm_strtoul_core( nptr, endptr, base, &e );
    if( e ) { errno = e; }
    return r;
}

long long strtoll( char const* restrict nptr, char** restrict endptr, int base ) {
    int e = 0;
    unsigned long long ur = cwasm_strtoull_core( nptr, endptr, base, &e );
    char const* s = cwasm_skip_ws( nptr );
    int neg = ( *s == '-' );

    if( endptr && *endptr == (char*)nptr ) {
        return 0;
    }

    if( e == ERANGE ) { errno = ERANGE; return neg ? LLONG_MIN : LLONG_MAX; }

    if( neg ) {
        char* ep2 = NULL;
        s = cwasm_skip_ws( nptr );
        (void)cwasm_parse_sign( &s );
        unsigned long long mag = 0;
        int e2 = 0;
        mag = cwasm_strtoull_core( s, &ep2, base, &e2 );
        if( endptr ) { *endptr = ep2; }
        if( e2 == ERANGE ) { errno = ERANGE; return LLONG_MIN; }
        if( mag > (unsigned long long)LLONG_MAX + 1ull ) { errno = ERANGE; return LLONG_MIN; }
        if( mag == (unsigned long long)LLONG_MAX + 1ull ) { return LLONG_MIN; }
        return -(long long)mag;
    } else {
        if( ur > (unsigned long long)LLONG_MAX ) { errno = ERANGE; return LLONG_MAX; }
        if( e ) { errno = e; }
        return (long long)ur;
    }
}

long strtol( char const* restrict nptr, char** restrict endptr, int base ) {
    int e = 0;
    char const* s = cwasm_skip_ws( nptr );
    int neg = ( *s == '-' );

    unsigned long ur = cwasm_strtoul_core( nptr, endptr, base, &e );

    if( endptr && *endptr == (char*)nptr ) {
        return 0;
    }

    if( e == ERANGE ) {
        errno = ERANGE;
        return neg ? LONG_MIN : LONG_MAX;
    }

    if( neg ) {
        char* ep2 = NULL;
        s = cwasm_skip_ws( nptr );
        (void)cwasm_parse_sign( &s );
        int e2 = 0;
        unsigned long mag = cwasm_strtoul_core( s, &ep2, base, &e2 );
        if( endptr ) { *endptr = ep2; }
        if( e2 == ERANGE ) { errno = ERANGE; return LONG_MIN; }
        if( mag > (unsigned long)LONG_MAX + 1ul ) { errno = ERANGE; return LONG_MIN; }
        if( mag == (unsigned long)LONG_MAX + 1ul ) { return LONG_MIN; }
        return -(long)mag;
    }
    if( ur > (unsigned long)LONG_MAX ) { errno = ERANGE; return LONG_MAX; }
    return (long)ur;
}

int atoi( char const* nptr ) { return (int)strtol( nptr, NULL, 10 ); }
long atol( char const* nptr ) { return strtol( nptr, NULL, 10 ); }
#if __STDC_VERSION__ >= 199901L
    long long atoll( char const* nptr ) { return strtoll( nptr, NULL, 10 ); }
#endif

static int cwasm_match_ci( char const* s, char const* lit ) {
    while( *lit ) {
        char a = *s++;
        char b = *lit++;
        if( a >= 'A' && a <= 'Z' ) { a = (char)( a - 'A' + 'a' ); }
        if( b >= 'A' && b <= 'Z' ) { b = (char)( b - 'A' + 'a' ); }
        if( a != b ) { return 0; }
    }
    return 1;
}

typedef struct {
    unsigned long ul;
    unsigned long long ull;
    int wide;
} cwasm_decimal_acc;

static void cwasm_decimal_acc_init( cwasm_decimal_acc* a ) {
    a->ul = 0;
    a->ull = 0;
    a->wide = 0;
}

static void cwasm_decimal_acc_digit( cwasm_decimal_acc* a, int digit ) {
    unsigned long d = (unsigned long)digit;
    if( !a->wide ) {
        if( a->ul > ULONG_MAX / 10ul || ( a->ul == ULONG_MAX / 10ul && d > ULONG_MAX % 10ul ) ) {
            a->wide = 1;
            a->ull = (unsigned long long)a->ul * 10ull + (unsigned long long)d;
        } else {
            a->ul = a->ul * 10ul + d;
        }
    } else {
        a->ull = a->ull * 10ull + (unsigned long long)digit;
    }
}

static unsigned long long cwasm_decimal_acc_value( cwasm_decimal_acc const* a ) {
    return a->wide ? a->ull : (unsigned long long)a->ul;
}

static double const strtod_exact_pow10[ 23 ] = {
    1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10,
    1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22
};
static float const strtof_exact_pow10[ 23 ] = {
    1e0f, 1e1f, 1e2f, 1e3f, 1e4f, 1e5f, 1e6f, 1e7f, 1e8f, 1e9f, 1e10f,
    1e11f, 1e12f, 1e13f, 1e14f, 1e15f, 1e16f, 1e17f, 1e18f, 1e19f, 1e20f, 1e21f, 1e22f
};
static unsigned long long const strtod_uint_pow10[ 16 ] = {
    1ULL, 10ULL, 100ULL, 1000ULL, 10000ULL, 100000ULL, 1000000ULL, 10000000ULL,
    100000000ULL, 1000000000ULL, 10000000000ULL, 100000000000ULL,
    1000000000000ULL, 10000000000000ULL, 100000000000000ULL, 1000000000000000ULL
};

static unsigned long long const strtod_pow10_anchor_bits[ 15 ] = {
    0x3FF0000000000000ULL,
    0x4480F0CF064DD592ULL,
    0x4911EFC659CF7D4CULL,
    0x4DA2FDBB0E39FB47ULL,
    0x52341B8EBE2EF1C7ULL,
    0x56C54A3047C694FEULL,
    0x5B568A9C942F3BA3ULL,
    0x5FE7DDDF6B095FF1ULL,
    0x647945145230B378ULL,
    0x690AC1677AAD4AB1ULL,
    0x6D9C5416BB92E3E6ULL,
    0x722DFE729B9FF153ULL,
    0x76BFC1DF6A7A61BBULL,
    0x7B50CFEB353A97DBULL,
    0x7FE1CCF385EBC8A0ULL
};

#include "strtod_table.h"

static double strtod_eisel_lemire( unsigned long long sig, int e10, int neg ) {
    union { unsigned long long u; double d; } v;
    if( !sig ) { v.u = (unsigned long long)neg << 63; return v.d; }
    if( e10 > 308 ) {
        v.u = 0x7FF0000000000000ULL | ( (unsigned long long)neg << 63 );
        return v.d;
    }
    if( e10 < -342 ) { v.u = (unsigned long long)neg << 63; return v.d; }

    int idx = e10 + 342;
    unsigned long long hi = strtod_el[ idx ].hi;
    unsigned long long lo = strtod_el[ idx ].lo;
    int e2 = strtod_el[ idx ].e2;
    if( !hi ) { v.u = (unsigned long long)neg << 63; return v.d; }

    int lz = 0;
    unsigned long long t = sig;
    while( !( t & 0x8000000000000000ULL ) ) { t <<= 1; lz++; }
    unsigned long long m64 = sig << lz;
    unsigned long long ah = m64 >> 32, al = (unsigned)m64;
    unsigned long long bh = hi >> 32, bl = (unsigned)hi;
    unsigned long long hh = ah * bh;
    unsigned long long hl = ah * bl;
    unsigned long long lh = al * bh;
    unsigned long long ll = al * bl;
    unsigned long long mid = ( ll >> 32 ) + (unsigned long long)(unsigned)hl
    + (unsigned long long)(unsigned)lh;
    unsigned long long p_hi = hh + ( hl >> 32 ) + ( lh >> 32 ) + ( mid >> 32 );
    unsigned long long p_lo = m64 * hi;

    if( ( p_hi & 0x1FF ) == 0x1FF ) {
        bh = lo >> 32; bl = (unsigned)lo;
        hh = ah * bh; hl = ah * bl; lh = al * bh; ll = al * bl;
        mid = ( ll >> 32 ) + (unsigned long long)(unsigned)hl
        + (unsigned long long)(unsigned)lh;
        unsigned long long c_hi = hh + ( hl >> 32 ) + ( lh >> 32 ) + ( mid >> 32 );
        unsigned long long p_lo_new = p_lo + c_hi;
        if( p_lo_new < p_lo ) { ++p_hi; }
        if( ( p_hi & 0x1FF ) == 0x1FF && p_lo_new == 0xFFFFFFFFFFFFFFFFULL ) {
            return 0.0 / 0.0;
        }
    }

    int overflow = (int)( p_hi >> 63 );
    long long biased = (long long)1213 + overflow + e2 - lz;
    int shift = 10 + overflow;
    if( biased <= 0 ) { shift += (int)( 1 - biased ); biased = 0; }
    if( shift > 64 ) { v.u = (unsigned long long)neg << 63; return v.d; }

    unsigned long long m, guard, sticky;
    if( shift == 64 ) {
        m = 0;
        guard = p_hi >> 63;
        sticky = ( ( p_hi << 1 ) | p_lo ) != 0;
    } else {
        m = p_hi >> shift;
        guard = ( p_hi >> ( shift - 1 ) ) & 1;
        sticky = ( ( p_hi & ( ( 1ULL << ( shift - 1 ) ) - 1 ) ) | p_lo ) != 0;
    }
    if( guard & ( sticky | ( m & 1 ) ) ) { ++m; }

    if( biased == 0 ) { v.u = m | ( (unsigned long long)neg << 63 ); return v.d; }
    if( m >> 53 ) { m >>= 1; biased++; }
    if( biased >= 2047 ) {
        v.u = 0x7FF0000000000000ULL | ( (unsigned long long)neg << 63 );
        return v.d;
    }
    v.u = ( (unsigned long long)biased << 52 ) | ( m & ( ( 1ULL << 52 ) - 1 ) ) | ( (unsigned long long)neg << 63 );
    return v.d;
}

double strtod( char const* restrict nptr, char** restrict endptr ) {
    char const* s = cwasm_skip_ws( nptr );
    int neg = cwasm_parse_sign( &s );

    if( cwasm_match_ci( s, "inf" ) ) {
        char const* p = s + 3;
        if( cwasm_match_ci( p, "inity" ) ) { p += 5; }
        if( endptr ) { *endptr = (char*)p; }
        return neg ? -( 1.0 / 0.0 ) : ( 1.0 / 0.0 );
    }
    if( cwasm_match_ci( s, "nan" ) ) {
        char const* p = s + 3;
        if( *p == '(' ) {
            char const* q = p + 1;
            while( *q == '_' || (unsigned)( *q - '0' ) <= 9u ||
                (unsigned)( ( *q | 32 ) - 'a' ) <= 25u ) {
                ++q;
            }
            if( *q == ')' ) { p = q + 1; }
        }
        if( endptr ) { *endptr = (char*)p; }
        {
            union { uint64_t u; double d; } v;
            v.u = 0x7ff8000000000000ULL;
            return v.d;
        }
    }

    if( s[ 0 ] == '0' && ( s[ 1 ] == 'x' || s[ 1 ] == 'X' ) ) {
        char const* h = s + 2;
        unsigned long long hsig = 0;
        int hfrac_bits = 0;
        int h_past_dot = 0;
        int h_any = 0;
        while( 1 ) {
            if( !h_past_dot && *h == '.' ) { h_past_dot = 1; h++; continue; }
            unsigned hd;
            if( (unsigned)( *h - '0' ) <= 9u ) { hd = (unsigned)( *h - '0' ); }
            else if( (unsigned)( ( *h | 32 ) - 'a' ) <= 5u ) hd = (unsigned)( ( *h | 32 ) - 'a' ) + 10u;
            else { break; }
            h++; h_any = 1;
            if( hsig < ( 1ULL << 60 ) ) { hsig = hsig * 16u + hd; }
            if( h_past_dot ) { hfrac_bits += 4; }
        }
        int hbexp = 0;
        if( *h == 'p' || *h == 'P' ) {
            char const* hp = h + 1;
            int hp_neg = ( *hp == '-' );
            if( *hp == '+' || *hp == '-' ) { ++hp; }
            if( (unsigned)( *hp - '0' ) <= 9u ) {
                while( (unsigned)( *hp - '0' ) <= 9u ) { hbexp = hbexp * 10 + ( *hp++ ) - '0'; }
                if( hp_neg ) { hbexp = -hbexp; }
                h = hp;
            }
        }

        if( !h_any ) { if( endptr ) *endptr = (char*)s + 1; return neg ? -0.0 : 0.0; }
        if( endptr ) { *endptr = (char*)h; }
        if( !hsig ) { return neg ? -0.0 : 0.0; }

        int hshift = hbexp - hfrac_bits;
        int hmsb = 0;
        unsigned long long ht = hsig;
        while( ht > 1 ) { ht >>= 1; hmsb++; }
        int hres_exp = hmsb + hshift;
        int hbiased = hres_exp + 1023;
        int hextra = hmsb - 52;
        unsigned long long hm;
        if( hextra > 0 ) {
            unsigned long long hg = ( hsig >> ( hextra - 1 ) ) & 1u;
            unsigned long long hst = ( hsig & ( ( 1ULL << ( hextra - 1 ) ) - 1u ) ) != 0u;
            hm = hsig >> hextra;
            if( hg & ( hst | ( hm & 1u ) ) ) {
                ++hm;
                if( hm >> 53 ) { hm >>= 1; hbiased++; }
            }
        } else {
            hm = hsig << ( -hextra );
        }
        union { unsigned long long u; double d; } hv;
        if( hbiased >= 2047 ) {
            errno = ERANGE;
            hv.u = 0x7FF0000000000000ULL | ( (unsigned long long)neg << 63 );
        } else if( hbiased > 0 ) {
            hv.u = ( (unsigned long long)hbiased << 52 ) | ( hm & ( ( 1ULL << 52 ) - 1u ) )
            | ( (unsigned long long)neg << 63 );
        } else {
            int hsub = 1 - hbiased;
            if( hsub >= 64 ) {
                errno = ERANGE;
                hv.u = (unsigned long long)neg << 63;
            } else {
                unsigned long long hsg = hsub > 0 ? ( ( hm >> ( hsub - 1 ) ) & 1u ) : 0u;
                unsigned long long hss = hsub > 1 ? ( ( hm & ( ( 1ULL << ( hsub - 1 ) ) - 1u ) ) != 0u ) : 0u;
                unsigned long long hsm = hm >> hsub;
                if( hsg & ( hss | ( hsm & 1u ) ) ) {
                    ++hsm;
                    if( hsm >> 52 ) {
                        hv.u = ( 1ULL << 52 ) | ( (unsigned long long)neg << 63 );
                        return hv.d;
                    }
                }
                hv.u = hsm | ( (unsigned long long)neg << 63 );
            }
        }
        return hv.d;
    }

    cwasm_decimal_acc int_acc;
    cwasm_decimal_acc frac_acc;
    int int_digits = 0;
    int frac_cap = 0;
    unsigned long long el_sig = 0;
    int el_sig_n = 0;
    int el_dp_pos = -1;
    int el_first_sig = -1;
    int el_total = 0;

    cwasm_decimal_acc_init( &int_acc );
    cwasm_decimal_acc_init( &frac_acc );

    while( *s >= '0' && *s <= '9' ) {
        if( int_digits < 18 ) { cwasm_decimal_acc_digit( &int_acc, *s - '0' ); }
        ++int_digits;
        if( *s != '0' || el_first_sig >= 0 ) {
            if( el_first_sig < 0 ) { el_first_sig = el_total; }
            if( el_sig_n < 19 ) { el_sig = el_sig * 10 + (unsigned long long)( *s - '0' ); el_sig_n++; }
        }
        ++el_total;
        ++s;
    }
    if( *s == '.' || *s == __cwasm_locale_radix_char() ) {
        ++s;
        el_dp_pos = el_total;
        while( *s >= '0' && *s <= '9' ) {
            if( frac_cap < 18 ) { cwasm_decimal_acc_digit( &frac_acc, *s - '0' ); }
            ++frac_cap;
            if( *s != '0' || el_first_sig >= 0 ) {
                if( el_first_sig < 0 ) { el_first_sig = el_total; }
                if( el_sig_n < 19 ) { el_sig = el_sig * 10 + (unsigned long long)( *s - '0' ); el_sig_n++; }
            }
            ++el_total;
            ++s;
        }
    }
    if( el_dp_pos < 0 ) { el_dp_pos = el_total; }
    if( el_first_sig < 0 ) { el_first_sig = el_total; }

    int exp10 = 0;
    if( *s == 'e' || *s == 'E' ) {
        char const* es = s + 1;
        int exp_neg = cwasm_parse_sign( &es );
        if( *es >= '0' && *es <= '9' ) {
            s = es;
            while( *s >= '0' && *s <= '9' ) {
                if( exp10 < 10000 ) { exp10 = exp10 * 10 + ( *s - '0' ); }
                ++s;
            }
        }
        if( exp_neg ) { exp10 = -exp10; }
    }

    if( el_total == 0 ) {
        if( endptr ) { *endptr = (char*)nptr; }
        return 0.0;
    }

    if( endptr ) { *endptr = (char*)s; }

    int total_digits = int_digits + frac_cap;
    int adj_exp = exp10 - frac_cap;
    double val;
    if( int_digits <= 15 && total_digits <= 15 && adj_exp >= -22 && adj_exp <= 22 ) {
        unsigned long long intpart = cwasm_decimal_acc_value( &int_acc );
        unsigned long long frac_uint = cwasm_decimal_acc_value( &frac_acc );
        unsigned long long combined = intpart * strtod_uint_pow10[ frac_cap ] + frac_uint;
        double dcombined = (double)combined;
        if( adj_exp == 0 ) { val = dcombined; }
        else if( adj_exp > 0 ) { val = dcombined * strtod_exact_pow10[ adj_exp ]; }
        else { val = dcombined / strtod_exact_pow10[ -adj_exp ]; }
        if( neg ) { val = -val; }
        return val;
    }

    int el_adj_exp = exp10 + el_dp_pos - el_first_sig - el_sig_n;
    val = strtod_eisel_lemire( el_sig, el_adj_exp, neg );

    if( val != val ) {
        unsigned long long sig2 = el_sig;
        int ae2 = el_adj_exp;
        double v2 = (double)sig2;
        union { unsigned long long u; double d; } anchor;
        if( ae2 > 0 ) {
            int r = ae2 % 22; int q = ae2 / 22;
            if( r > 0 ) { v2 *= strtod_exact_pow10[ r ]; }
            if( q > 0 && q <= 14 ) {
                anchor.u = strtod_pow10_anchor_bits[ q ];
                v2 *= anchor.d;
            } else if( q > 14 ) { v2 = 1.0 / 0.0; }
        } else if( ae2 < 0 ) {
            int ae = -ae2; int r = ae % 22; int q = ae / 22;
            if( r > 0 ) { v2 /= strtod_exact_pow10[ r ]; }
            if( q > 0 && q <= 14 ) {
                anchor.u = strtod_pow10_anchor_bits[ q ];
                v2 /= anchor.d;
            } else if( q > 14 ) { v2 = 0.0; }
        }
        val = neg ? -v2 : v2;
    }

    if( isinf( val ) ) { errno = ERANGE; }
    else if( val == 0.0 && el_sig != 0 ) { errno = ERANGE; }

    return val;
}


static unsigned int const strtof_uint_pow10[ 10 ] = {
    1u, 10u, 100u, 1000u, 10000u, 100000u, 1000000u, 10000000u,
    100000000u, 1000000000u
};

typedef struct cwasm_strtof_scan {
    cwasm_decimal_acc intpart;
    int int_digits;
    cwasm_decimal_acc frac_uint;
    int frac_cap;
    uint32_t el_sig;
    int el_sig_n;
    int el_dp_pos;
    int el_first_sig;
    int el_total;
    int exp10;
} cwasm_strtof_scan;

static char const* cwasm_strtof_scan_decimal( char const* s, cwasm_strtof_scan* out ) {
    cwasm_decimal_acc_init( &out->intpart );
    out->int_digits = 0;
    cwasm_decimal_acc_init( &out->frac_uint );
    out->frac_cap = 0;
    out->el_sig = 0;
    out->el_sig_n = 0;
    out->el_dp_pos = -1;
    out->el_first_sig = -1;
    out->el_total = 0;
    out->exp10 = 0;

    while( *s >= '0' && *s <= '9' ) {
        if( out->int_digits < 9 ) { cwasm_decimal_acc_digit( &out->intpart, *s - '0' ); }
        ++out->int_digits;
        if( *s != '0' || out->el_first_sig >= 0 ) {
            if( out->el_first_sig < 0 ) { out->el_first_sig = out->el_total; }
            if( out->el_sig_n < 9 ) {
                out->el_sig = out->el_sig * 10u + (unsigned)( *s - '0' );
                ++out->el_sig_n;
            }
        }
        ++out->el_total;
        ++s;
    }
    if( *s == '.' || *s == __cwasm_locale_radix_char() ) {
        ++s;
        out->el_dp_pos = out->el_total;
        while( *s >= '0' && *s <= '9' ) {
            if( out->frac_cap < 9 ) {
                cwasm_decimal_acc_digit( &out->frac_uint, *s - '0' );
            }
            ++out->frac_cap;
            if( *s != '0' || out->el_first_sig >= 0 ) {
                if( out->el_first_sig < 0 ) { out->el_first_sig = out->el_total; }
                if( out->el_sig_n < 9 ) {
                    out->el_sig = out->el_sig * 10u + (unsigned)( *s - '0' );
                    ++out->el_sig_n;
                }
            }
            ++out->el_total;
            ++s;
        }
    }
    if( out->el_dp_pos < 0 ) { out->el_dp_pos = out->el_total; }
    if( out->el_first_sig < 0 ) { out->el_first_sig = out->el_total; }

    if( *s == 'e' || *s == 'E' ) {
        char const* es = s + 1;
        int exp_neg = cwasm_parse_sign( &es );
        if( *es >= '0' && *es <= '9' ) {
            s = es;
            while( *s >= '0' && *s <= '9' ) {
                if( out->exp10 < 10000 ) {
                    out->exp10 = out->exp10 * 10 + ( *s - '0' );
                }
                ++s;
            }
        }
        if( exp_neg ) { out->exp10 = -out->exp10; }
    }

    if( out->el_total == 0 ) { return NULL; }
    return s;
}

static uint32_t cwasm_strtof_eisel_fbits( unsigned long long sig, int e10, int neg ) {
    if( !sig ) { return (uint32_t)neg << 31; }
    if( e10 > 38 ) { return 0x7f800000u | ( (uint32_t)neg << 31 ); }
    if( e10 < -65 ) { return (uint32_t)neg << 31; }

    int idx = e10 + 342;
    unsigned long long hi = strtod_el[ idx ].hi;
    unsigned long long lo = strtod_el[ idx ].lo;
    int e2 = strtod_el[ idx ].e2;
    if( !hi ) { return (uint32_t)neg << 31; }

    int lz = 0;
    unsigned long long t = sig;
    while( !( t & 0x8000000000000000ULL ) ) { t <<= 1; lz++; }
    unsigned long long m64 = t;
    unsigned long long ah = m64 >> 32, al = (unsigned)m64;
    unsigned long long bh = hi >> 32, bl = (unsigned)hi;
    unsigned long long hh = ah * bh;
    unsigned long long hl = ah * bl;
    unsigned long long lh = al * bh;
    unsigned long long ll = al * bl;
    unsigned long long mid = ( ll >> 32 ) + (unsigned long long)(unsigned)hl
    + (unsigned long long)(unsigned)lh;
    unsigned long long p_hi = hh + ( hl >> 32 ) + ( lh >> 32 ) + ( mid >> 32 );
    unsigned long long p_lo = m64 * hi;

    if( ( p_hi & 0x1FF ) == 0x1FF ) {
        bh = lo >> 32; bl = (unsigned)lo;
        hh = ah * bh; hl = ah * bl; lh = al * bh; ll = al * bl;
        mid = ( ll >> 32 ) + (unsigned long long)(unsigned)hl
        + (unsigned long long)(unsigned)lh;
        unsigned long long c_hi = hh + ( hl >> 32 ) + ( lh >> 32 ) + ( mid >> 32 );
        unsigned long long p_lo_new = p_lo + c_hi;
        if( p_lo_new < p_lo ) { ++p_hi; }
        if( ( p_hi & 0x1FF ) == 0x1FF && p_lo_new == 0xFFFFFFFFFFFFFFFFULL ) {
            return 0x7fc00000u;
        }
    }

    int overflow = (int)( p_hi >> 63 );
    long long biased = (long long)317 + overflow + e2 - lz;
    int shift = 39 + overflow;
    if( biased <= 0 ) { shift += (int)( 1 - biased ); biased = 0; }
    if( shift > 64 ) { return (uint32_t)neg << 31; }

    unsigned long long m, guard, sticky;
    if( shift == 64 ) {
        m = 0;
        guard = p_hi >> 63;
        sticky = ( ( p_hi << 1 ) | p_lo ) != 0;
    } else {
        m = p_hi >> shift;
        guard = ( p_hi >> ( shift - 1 ) ) & 1;
        sticky = ( ( p_hi & ( ( 1ULL << ( shift - 1 ) ) - 1 ) ) | p_lo ) != 0;
    }
    if( guard & ( sticky | ( m & 1 ) ) ) { ++m; }

    if( biased == 0 ) { return (uint32_t)m | ( (uint32_t)neg << 31 ); }
    if( m >> 24 ) { m >>= 1; biased++; }
    if( biased >= 255 ) { return 0x7f800000u | ( (uint32_t)neg << 31 ); }
    return ( (uint32_t)biased << 23 ) | ( (uint32_t)m & 0x7fffffu ) | ( (uint32_t)neg << 31 );
}

static uint32_t cwasm_strtof_fallback_fbits( uint32_t el_sig, int el_adj_exp, int neg ) {
    union { float f; uint32_t u; } v;
    float v2 = (float)el_sig;
    int ae2 = el_adj_exp;

    if( ae2 > 0 ) {
        int r = ae2 % 22;
        int q = ae2 / 22;
        if( r > 0 ) { v2 *= strtof_exact_pow10[ r ]; }
        if( q > 14 ) { v2 = 1.0f / 0.0f; }
        else while( q-- > 0 )
        v2 *= strtof_exact_pow10[ 22 ];
    } else if( ae2 < 0 ) {
        int ae = -ae2;
        int r = ae % 22;
        int q = ae / 22;
        if( r > 0 ) { v2 /= strtof_exact_pow10[ r ]; }
        if( q > 14 ) { v2 = 0.0f; }
        else while( q-- > 0 )
        v2 /= strtof_exact_pow10[ 22 ];
    }

    v.f = neg ? -v2 : v2;
    return v.u;
}

static float cwasm_strtof_from_scan( cwasm_strtof_scan const* scan, int neg ) {
    int total_digits = scan->int_digits + scan->frac_cap;
    int adj_exp = scan->exp10 - scan->frac_cap;
    float val;

    if( scan->int_digits <= 7 && total_digits <= 7 && adj_exp >= -22 && adj_exp <= 22 ) {
        unsigned int intpart = (unsigned int)cwasm_decimal_acc_value( &scan->intpart );
        unsigned int frac_uint = (unsigned int)cwasm_decimal_acc_value( &scan->frac_uint );
        unsigned int combined = intpart * strtof_uint_pow10[ scan->frac_cap ] + frac_uint;
        float fcombined = (float)combined;
        if( adj_exp == 0 ) { val = fcombined; }
        else if( adj_exp > 0 ) { val = fcombined * strtof_exact_pow10[ adj_exp ]; }
        else { val = fcombined / strtof_exact_pow10[ -adj_exp ]; }
        if( neg ) { val = -val; }
        return val;
    }

    int el_adj_exp = scan->exp10 + scan->el_dp_pos - scan->el_first_sig - scan->el_sig_n;
    uint32_t fbits = cwasm_strtof_eisel_fbits( scan->el_sig, el_adj_exp, neg );
    if( fbits == 0x7fc00000u ) {
        union { float f; uint32_t u; } v;
        v.u = cwasm_strtof_fallback_fbits( scan->el_sig, el_adj_exp, neg );
        return v.f;
    }
    union { float f; uint32_t u; } r;
    r.u = fbits;
    return r.f;
}

static uint32_t cwasm_strtof_hex_bits( char const* s, char** endptr, int neg ) {
    char const* h = s + 2;
    uint32_t hsig = 0;
    int hfrac_bits = 0;
    int h_past_dot = 0;
    int h_any = 0;

    while( 1 ) {
        if( !h_past_dot && *h == '.' ) { h_past_dot = 1; h++; continue; }
        unsigned hd;
        if( (unsigned)( *h - '0' ) <= 9u ) { hd = (unsigned)( *h - '0' ); }
        else if( (unsigned)( ( *h | 32 ) - 'a' ) <= 5u ) hd = (unsigned)( ( *h | 32 ) - 'a' ) + 10u;
        else { break; }
        h++; h_any = 1;
        if( hsig < ( 1u << 24 ) ) { hsig = hsig * 16u + hd; }
        if( h_past_dot ) { hfrac_bits += 4; }
    }
    int hbexp = 0;
    if( *h == 'p' || *h == 'P' ) {
        ++h;
        int hp_neg = ( *h == '-' );
        if( *h == '+' || *h == '-' ) { ++h; }
        while( (unsigned)( *h - '0' ) <= 9u ) { hbexp = hbexp * 10 + ( *h++ ) - '0'; }
        if( hp_neg ) { hbexp = -hbexp; }
    }
    if( !h_any ) {
        // C: with no hex digits after 0x, the subject sequence is the initial 0,
        // so one character is converted and endptr lands on the 'x'.
        if( endptr ) { *endptr = (char*)s + 1; }
        return (uint32_t)neg << 31;
    }
    if( endptr ) { *endptr = (char*)h; }
    if( !hsig ) { return (uint32_t)neg << 31; }

    int hshift = hbexp - hfrac_bits;
    int hmsb = 0;
    uint32_t ht = hsig;
    while( ht > 1u ) { ht >>= 1; hmsb++; }
    int hres_exp = hmsb + hshift;
    int hbiased = hres_exp + 127;
    int hextra = hmsb - 23;
    uint32_t hm;
    if( hextra > 0 ) {
        uint32_t hg = ( hsig >> ( hextra - 1 ) ) & 1u;
        uint32_t hst = ( hsig & ( ( 1u << ( hextra - 1 ) ) - 1u ) ) != 0u;
        hm = hsig >> hextra;
        if( hg & ( hst | ( hm & 1u ) ) ) {
            ++hm;
            if( hm >> 24 ) { hm >>= 1; hbiased++; }
        }
    } else {
        hm = hsig << ( -hextra );
    }

    if( hbiased >= 255 ) {
        errno = ERANGE;
        return 0x7f800000u | ( (uint32_t)neg << 31 );
    }
    if( hbiased > 0 ) {
        return ( (uint32_t)hbiased << 23 ) | ( (uint32_t)hm & 0x007fffffu ) | ( (uint32_t)neg << 31 );
    }

    int hsub = 1 - hbiased;
    uint32_t hsg;
    uint32_t hss;
    uint32_t hsm;

    if( hsub >= 32 ) {
        errno = ERANGE;
        return (uint32_t)neg << 31;
    }
    hsg = hsub > 0 ? ( ( hm >> ( hsub - 1 ) ) & 1u ) : 0u;
    hss = hsub > 1 ? ( ( hm & ( ( 1u << ( hsub - 1 ) ) - 1u ) ) != 0u ) : 0u;
    hsm = hm >> hsub;
    if( hsg & ( hss | ( hsm & 1u ) ) ) {
        ++hsm;
        if( hsm >> 23 ) { return ( 1u << 23 ) | ( (uint32_t)neg << 31 ); }
    }
    if( hsm == 0 ) { errno = ERANGE; }
    return hsm | ( (uint32_t)neg << 31 );
}

float strtof( char const* restrict nptr, char** restrict endptr ) {
    char const* s = cwasm_skip_ws( nptr );
    int neg = cwasm_parse_sign( &s );
    union { uint32_t u; float f; } v;

    if( cwasm_match_ci( s, "inf" ) ) {
        char const* p = s + 3;
        if( cwasm_match_ci( p, "inity" ) ) { p += 5; }
        if( endptr ) { *endptr = (char*)p; }
        v.u = 0x7f800000u | ( (uint32_t)neg << 31 );
        return v.f;
    }
    if( cwasm_match_ci( s, "nan" ) ) {
        char const* p = s + 3;
        if( endptr ) { *endptr = (char*)p; }
        v.u = 0x7fc00000u;
        return v.f;
    }

    if( s[ 0 ] == '0' && ( s[ 1 ] == 'x' || s[ 1 ] == 'X' ) ) {
        v.u = cwasm_strtof_hex_bits( s, endptr, neg );
        return v.f;
    }

    cwasm_strtof_scan scan;
    char const* end = cwasm_strtof_scan_decimal( s, &scan );
    if( !end ) {
        if( endptr ) { *endptr = (char*)nptr; }
        return 0.0f;
    }
    if( endptr ) { *endptr = (char*)end; }
    {
        float r = cwasm_strtof_from_scan( &scan, neg );
        if( isinf( r ) ) { errno = ERANGE; }
        else if( r == 0.0f && scan.el_sig != 0 ) { errno = ERANGE; }
        return r;
    }
}

long double strtold( char const* restrict nptr, char** restrict endptr ) {
    char const* s = cwasm_skip_ws( nptr );
    int neg = cwasm_parse_sign( &s );
    long double val;
    int any = 0;
    int exp = 0;

    if( cwasm_match_ci( s, "inf" ) ) {
        char const* p = s + 3;
        if( cwasm_match_ci( p, "inity" ) ) { p += 5; }
        if( endptr ) { *endptr = (char*)p; }
        return neg ? -HUGE_VALL : HUGE_VALL;
    }
    if( cwasm_match_ci( s, "nan" ) ) {
        char const* p = s + 3;
        if( endptr ) { *endptr = (char*)p; }
        return (long double)__builtin_nan( "" );
    }

    if( s[ 0 ] == '0' && ( s[ 1 ] == 'x' || s[ 1 ] == 'X' ) ) {
        char const* p = s + 2;
        long double hval = 0.0L;
        int hex_any = 0;
        int hex_exp = 0;
        int hex_digits = 0;

        while( ( *p >= '0' && *p <= '9' ) ||
            ( *p >= 'a' && *p <= 'f' ) ||
            ( *p >= 'A' && *p <= 'F' ) ) {
            int d = ( *p <= '9' ) ? ( *p - '0' ) :
                ( ( *p | 32 ) - 'a' + 10 );
            hex_any = 1;
            if( hex_digits < 32 ) {
                hval = hval * 16.0L + (long double)d;
                ++hex_digits;
            } else {
                hex_exp += 4;
            }
            ++p;
        }
        if( *p == '.' ) {
            ++p;
            while( ( *p >= '0' && *p <= '9' ) ||
                ( *p >= 'a' && *p <= 'f' ) ||
                ( *p >= 'A' && *p <= 'F' ) ) {
                int d = ( *p <= '9' ) ? ( *p - '0' ) :
                    ( ( *p | 32 ) - 'a' + 10 );
                hex_any = 1;
                if( hex_digits < 32 ) {
                    hval = hval * 16.0L + (long double)d;
                    ++hex_digits;
                    hex_exp -= 4;
                }
                ++p;
            }
        }
        if( ( *p == 'p' || *p == 'P' ) && hex_any ) {
            char const* ep = p + 1;
            int exp_neg = 0;
            int eadd = 0;
            if( *ep == '+' || *ep == '-' ) {
                exp_neg = ( *ep == '-' );
                ++ep;
            }
            if( (unsigned)( *ep - '0' ) <= 9u ) {
                p = ep;
                while( (unsigned)( *p - '0' ) <= 9u ) {
                    if( eadd < 1000000 ) { eadd = eadd * 10 + ( *p - '0' ); }
                    ++p;
                }
                hex_exp += exp_neg ? -eadd : eadd;
            }
        }
        if( !hex_any ) {
            if( endptr ) { *endptr = (char*)nptr; }
            return 0.0L;
        }
        if( endptr ) { *endptr = (char*)p; }
        if( hex_exp ) { hval = ldexpl( hval, hex_exp ); }
        if( isinf( hval ) ) { errno = ERANGE; }
        return neg ? -hval : hval;
    }

    val = 0.0L;
    int pending_zeros = 0;
    int sig_digits = 0;
    int saw_nonzero = 0;

    while( (unsigned)( *s - '0' ) <= 9u ) {
        int d = *s++ - '0';
        any = 1;
        if( d == 0 ) {
            if( saw_nonzero ) { ++pending_zeros; }
            continue;
        }
        saw_nonzero = 1;
        while( pending_zeros > 0 ) {
            if( sig_digits < 36 ) {
                val *= 10.0L;
                ++sig_digits;
            } else {
                ++exp;
            }
            --pending_zeros;
        }
        if( sig_digits < 36 ) {
            val = val * 10.0L + (long double)d;
            ++sig_digits;
        } else {
            ++exp;
        }
    }
    if( saw_nonzero ) {
        exp += pending_zeros;
    }
    pending_zeros = 0;

    int frac = 0;
    if( *s == '.' ) {
        while( exp > 0 && sig_digits < 36 ) {
            val *= 10.0L;
            ++sig_digits;
            --exp;
        }
        ++s;
        while( (unsigned)( *s - '0' ) <= 9u ) {
            int d = *s++ - '0';
            any = 1;
            if( sig_digits < 36 ) {
                val = val * 10.0L + (long double)d;
                ++sig_digits;
                ++frac;
            }
        }
    }

    if( *s == 'e' || *s == 'E' ) {
        char const* es = s + 1;
        int exp_neg = 0;
        if( *es == '+' || *es == '-' ) {
            exp_neg = ( *es == '-' );
            ++es;
        }
        if( (unsigned)( *es - '0' ) <= 9u ) {
            int eadd = 0;
            s = es;
            while( (unsigned)( *s - '0' ) <= 9u ) {
                if( eadd < 1000000 ) { eadd = eadd * 10 + ( *s - '0' ); }
                ++s;
            }
            exp += exp_neg ? -eadd : eadd;
        }
    }

    if( !any ) {
        if( endptr ) { *endptr = (char*)nptr; }
        return 0.0L;
    }
    if( endptr ) { *endptr = (char*)s; }

    exp -= frac;
    if( exp != 0 ) {
        enum { CWASM_LD_POW10_MAX = 512 };
        static long double const cwasm_ld_pow10[] = {
            1e0L, 1e1L, 1e2L, 1e3L, 1e4L, 1e5L, 1e6L, 1e7L, 1e8L, 1e9L, 1e10L, 1e11L, 1e12L, 1e13L, 1e14L, 1e15L, 1e16L, 1e17L, 1e18L, 1e19L, 1e20L, 1e21L, 1e22L, 1e23L, 1e24L, 1e25L, 1e26L, 1e27L, 1e28L, 1e29L, 1e30L, 1e31L, 1e32L, 1e33L, 1e34L, 1e35L, 1e36L, 1e37L, 1e38L, 1e39L, 1e40L, 1e41L, 1e42L, 1e43L, 1e44L, 1e45L, 1e46L, 1e47L, 1e48L, 1e49L, 1e50L, 1e51L, 1e52L, 1e53L, 1e54L, 1e55L, 1e56L, 1e57L, 1e58L, 1e59L, 1e60L, 1e61L, 1e62L, 1e63L, 1e64L,
            1e65L, 1e66L, 1e67L, 1e68L, 1e69L, 1e70L, 1e71L, 1e72L, 1e73L, 1e74L, 1e75L, 1e76L, 1e77L, 1e78L, 1e79L, 1e80L, 1e81L, 1e82L, 1e83L, 1e84L, 1e85L, 1e86L, 1e87L, 1e88L, 1e89L, 1e90L, 1e91L, 1e92L, 1e93L, 1e94L, 1e95L, 1e96L, 1e97L, 1e98L, 1e99L, 1e100L, 1e101L, 1e102L, 1e103L, 1e104L, 1e105L, 1e106L, 1e107L, 1e108L, 1e109L, 1e110L, 1e111L, 1e112L, 1e113L, 1e114L, 1e115L, 1e116L, 1e117L, 1e118L, 1e119L, 1e120L, 1e121L, 1e122L, 1e123L, 1e124L, 1e125L, 1e126L, 1e127L, 1e128L,
            1e129L, 1e130L, 1e131L, 1e132L, 1e133L, 1e134L, 1e135L, 1e136L, 1e137L, 1e138L, 1e139L, 1e140L, 1e141L, 1e142L, 1e143L, 1e144L, 1e145L, 1e146L, 1e147L, 1e148L, 1e149L, 1e150L, 1e151L, 1e152L, 1e153L, 1e154L, 1e155L, 1e156L, 1e157L, 1e158L, 1e159L, 1e160L, 1e161L, 1e162L, 1e163L, 1e164L, 1e165L, 1e166L, 1e167L, 1e168L, 1e169L, 1e170L, 1e171L, 1e172L, 1e173L, 1e174L, 1e175L, 1e176L, 1e177L, 1e178L, 1e179L, 1e180L, 1e181L, 1e182L, 1e183L, 1e184L, 1e185L, 1e186L, 1e187L, 1e188L, 1e189L, 1e190L, 1e191L, 1e192L,
            1e193L, 1e194L, 1e195L, 1e196L, 1e197L, 1e198L, 1e199L, 1e200L, 1e201L, 1e202L, 1e203L, 1e204L, 1e205L, 1e206L, 1e207L, 1e208L, 1e209L, 1e210L, 1e211L, 1e212L, 1e213L, 1e214L, 1e215L, 1e216L, 1e217L, 1e218L, 1e219L, 1e220L, 1e221L, 1e222L, 1e223L, 1e224L, 1e225L, 1e226L, 1e227L, 1e228L, 1e229L, 1e230L, 1e231L, 1e232L, 1e233L, 1e234L, 1e235L, 1e236L, 1e237L, 1e238L, 1e239L, 1e240L, 1e241L, 1e242L, 1e243L, 1e244L, 1e245L, 1e246L, 1e247L, 1e248L, 1e249L, 1e250L, 1e251L, 1e252L, 1e253L, 1e254L, 1e255L, 1e256L,
            1e257L, 1e258L, 1e259L, 1e260L, 1e261L, 1e262L, 1e263L, 1e264L, 1e265L, 1e266L, 1e267L, 1e268L, 1e269L, 1e270L, 1e271L, 1e272L, 1e273L, 1e274L, 1e275L, 1e276L, 1e277L, 1e278L, 1e279L, 1e280L, 1e281L, 1e282L, 1e283L, 1e284L, 1e285L, 1e286L, 1e287L, 1e288L, 1e289L, 1e290L, 1e291L, 1e292L, 1e293L, 1e294L, 1e295L, 1e296L, 1e297L, 1e298L, 1e299L, 1e300L, 1e301L, 1e302L, 1e303L, 1e304L, 1e305L, 1e306L, 1e307L, 1e308L, 1e309L, 1e310L, 1e311L, 1e312L, 1e313L, 1e314L, 1e315L, 1e316L, 1e317L, 1e318L, 1e319L, 1e320L,
            1e321L, 1e322L, 1e323L, 1e324L, 1e325L, 1e326L, 1e327L, 1e328L, 1e329L, 1e330L, 1e331L, 1e332L, 1e333L, 1e334L, 1e335L, 1e336L, 1e337L, 1e338L, 1e339L, 1e340L, 1e341L, 1e342L, 1e343L, 1e344L, 1e345L, 1e346L, 1e347L, 1e348L, 1e349L, 1e350L, 1e351L, 1e352L, 1e353L, 1e354L, 1e355L, 1e356L, 1e357L, 1e358L, 1e359L, 1e360L, 1e361L, 1e362L, 1e363L, 1e364L, 1e365L, 1e366L, 1e367L, 1e368L, 1e369L, 1e370L, 1e371L, 1e372L, 1e373L, 1e374L, 1e375L, 1e376L, 1e377L, 1e378L, 1e379L, 1e380L, 1e381L, 1e382L, 1e383L, 1e384L,
            1e385L, 1e386L, 1e387L, 1e388L, 1e389L, 1e390L, 1e391L, 1e392L, 1e393L, 1e394L, 1e395L, 1e396L, 1e397L, 1e398L, 1e399L, 1e400L, 1e401L, 1e402L, 1e403L, 1e404L, 1e405L, 1e406L, 1e407L, 1e408L, 1e409L, 1e410L, 1e411L, 1e412L, 1e413L, 1e414L, 1e415L, 1e416L, 1e417L, 1e418L, 1e419L, 1e420L, 1e421L, 1e422L, 1e423L, 1e424L, 1e425L, 1e426L, 1e427L, 1e428L, 1e429L, 1e430L, 1e431L, 1e432L, 1e433L, 1e434L, 1e435L, 1e436L, 1e437L, 1e438L, 1e439L, 1e440L, 1e441L, 1e442L, 1e443L, 1e444L, 1e445L, 1e446L, 1e447L, 1e448L,
            1e449L, 1e450L, 1e451L, 1e452L, 1e453L, 1e454L, 1e455L, 1e456L, 1e457L, 1e458L, 1e459L, 1e460L, 1e461L, 1e462L, 1e463L, 1e464L, 1e465L, 1e466L, 1e467L, 1e468L, 1e469L, 1e470L, 1e471L, 1e472L, 1e473L, 1e474L, 1e475L, 1e476L, 1e477L, 1e478L, 1e479L, 1e480L, 1e481L, 1e482L, 1e483L, 1e484L, 1e485L, 1e486L, 1e487L, 1e488L, 1e489L, 1e490L, 1e491L, 1e492L, 1e493L, 1e494L, 1e495L, 1e496L, 1e497L, 1e498L, 1e499L, 1e500L, 1e501L, 1e502L, 1e503L, 1e504L, 1e505L, 1e506L, 1e507L, 1e508L, 1e509L, 1e510L, 1e511L, 1e512L
        };
        int scale_neg = exp < 0;
        unsigned n = scale_neg ? (unsigned)( -exp ) : (unsigned)exp;
        long double p;
        if( n <= (unsigned)CWASM_LD_POW10_MAX ) {
            p = cwasm_ld_pow10[ n ];
        } else {
            p = 1.0L;
            while( n > (unsigned)CWASM_LD_POW10_MAX ) {
                p *= cwasm_ld_pow10[ CWASM_LD_POW10_MAX ];
                n -= (unsigned)CWASM_LD_POW10_MAX;
                if( __builtin_isinf( p ) ) { break; }
            }
            if( !__builtin_isinf( p ) && n ) { p *= cwasm_ld_pow10[ n ]; }
        }
        if( scale_neg ) {
            if( __builtin_isinf( p ) ) {
                errno = ERANGE;
                return neg ? -0.0L : 0.0L;
            }
            val /= p;
        } else {
            val *= p;
        }
    }

    if( __builtin_isinf( val ) || val > LDBL_MAX ) {
        errno = ERANGE;
        return neg ? -HUGE_VALL : HUGE_VALL;
    }
    return neg ? -val : val;
}

double atof( char const* nptr ) {
    return strtod( nptr, NULL );
}

void* bsearch( void const* key, void const* base, size_t nmemb, size_t size,
    int ( *compar )( void const*, void const* ) ) {
    char const* b = (char const*)base;
    while( nmemb > 0 ) {
        size_t half = nmemb / 2;
        char const* mid = b + half * size;
        int cmp = compar( key, mid );
        if( cmp < 0 ) { nmemb = half; }
        else if( cmp > 0 ) {
            b = mid + size;
            nmemb -= half + 1;
        } else { return (void*)mid; }
    }
    return NULL;
}

static void cwasm_qsort_swap( void* va, void* vb, size_t size ) {
    if( va == vb || size == 0 ) { return; }
    unsigned char* a = (unsigned char*)va;
    unsigned char* b = (unsigned char*)vb;
    if( ( ( (uintptr_t)a | (uintptr_t)b | size ) & ( sizeof( size_t ) - 1 ) ) == 0 ) {
        size_t* wa = (size_t*)a;
        size_t* wb = (size_t*)b;
        size_t n = size / sizeof( size_t );
        while( n-- ) {
            size_t t = *wa;
            *wa++ = *wb;
            *wb++ = t;
        }
        return;
    }
    while( size-- ) {
        unsigned char t = *a;
        *a++ = *b;
        *b++ = t;
    }
}

// High quality quicksort implementation for C/C++
//
// Implements the algorithm from the paper "Engineering a Sort Function" by
// Jon L Bentley and M Douglas McIlroy (1993).
// http://cs.fit.edu/~pkc/classes/writing/samples/bentley93engineering.pdf
//
// The code has been written to be iterative (with a manual data stack) instead
// of recursive like the implementation in the paper. When run through a large
// set of tests with different initial data organization, it seems to be on about
// the same level as std::sort. I am not an expert in sorting though, so tests
// might be faulty. In any case, it works well for my use cases and has a tiny
// code footprint, so I'm happy with it.
//
//     /Mattias Gustavsson

void qsort( void* base, size_t nmemb, size_t size,
    int ( *compar )( void const*, void const* ) ) {
    char* arr = (char*)base;
    ptrdiff_t sz = (ptrdiff_t)size;
    size_t n = nmemb;
    if( n > 1 && sz > 0 ) {
        struct { size_t start; size_t count; } stack[ 64 ];
        ptrdiff_t top = 0;
        stack[ top ].start = 0;
        stack[ top ].count = n;
        while( top >= 0 ) {
            char* a = arr + stack[ top ].start * sz;
            size_t count = stack[ top-- ].count;
            for( ;; ) {
                if( count < 24 ) {
                    if( count > 1 ) {
                        char* end = a + count * sz;
                        char* minp = a;
                        for( char* p = a + sz; p < end; p += sz ) {
                            if( compar( p, minp ) < 0 ) { minp = p; }
                        }
                        cwasm_qsort_swap( a, minp, size );
                        for( char* cur = a + sz; cur < end; cur += sz ) {
                            char* ins = cur;
                            while( compar( ins - sz, ins ) > 0 ) {
                                cwasm_qsort_swap( ins, ins - sz, size );
                                ins -= sz;
                            }
                        }
                    }
                    break;
                }
                char* mid = a + ( count / 2 ) * sz;
                if( count > 40 ) {
                    char* lo = a;
                    char* hi = a + ( count - 1 ) * sz;
                    ptrdiff_t step = (ptrdiff_t)( count / 8 ) * sz;
                    char* x = lo, * y = lo + step, * z = lo + 2 * step;
                    lo = ( compar( x, y ) < 0 ? ( compar( y, z ) < 0 ? y : ( compar( x, z ) < 0 ? z : x ) )
                        : ( compar( y, z ) > 0 ? y : ( compar( x, z ) > 0 ? z : x ) ) );
                    x = mid - step; y = mid; z = mid + step;
                    mid = ( compar( x, y ) < 0 ? ( compar( y, z ) < 0 ? y : ( compar( x, z ) < 0 ? z : x ) )
                        : ( compar( y, z ) > 0 ? y : ( compar( x, z ) > 0 ? z : x ) ) );
                    x = hi - 2 * step; y = hi - step; z = hi;
                    hi = ( compar( x, y ) < 0 ? ( compar( y, z ) < 0 ? y : ( compar( x, z ) < 0 ? z : x ) )
                        : ( compar( y, z ) > 0 ? y : ( compar( x, z ) > 0 ? z : x ) ) );
                    x = lo; y = mid; z = hi;
                    mid = ( compar( x, y ) < 0 ? ( compar( y, z ) < 0 ? y : ( compar( x, z ) < 0 ? z : x ) )
                        : ( compar( y, z ) > 0 ? y : ( compar( x, z ) > 0 ? z : x ) ) );
                }
                char* pivot = a;
                cwasm_qsort_swap( pivot, mid, size );
                char* eq_lo_end = a;
                char* scan_lo = a;
                char* scan_hi = a + ( count - 1 ) * sz;
                char* eq_hi_start = scan_hi;
                for( ;; ) {
                    int r;
                    while( scan_lo <= scan_hi && ( r = compar( scan_lo, pivot ) ) <= 0 ) {
                        if( r == 0 ) { cwasm_qsort_swap( eq_lo_end, scan_lo, size ); eq_lo_end += sz; }
                        scan_lo += sz;
                    }
                    while( scan_hi >= scan_lo && ( r = compar( scan_hi, pivot ) ) >= 0 ) {
                        if( r == 0 ) { cwasm_qsort_swap( scan_hi, eq_hi_start, size ); eq_hi_start -= sz; }
                        scan_hi -= sz;
                    }
                    if( scan_lo > scan_hi ) { break; }
                    cwasm_qsort_swap( scan_lo, scan_hi, size );
                    scan_lo += sz; scan_hi -= sz;
                }
                char* part_end = a + count * sz;
                ptrdiff_t eq_lo_bytes = ( ( eq_lo_end - a ) < ( scan_lo - eq_lo_end ) ? ( eq_lo_end - a ) : ( scan_lo - eq_lo_end ) );
                ptrdiff_t cnt = eq_lo_bytes / sz; char* dst = a; char* src = scan_lo - eq_lo_bytes;
                while( cnt-- > 0 ) { cwasm_qsort_swap( dst, src, size ); dst += sz; src += sz; }
                ptrdiff_t eq_hi_bytes = ( ( eq_hi_start - scan_hi ) < ( part_end - eq_hi_start - sz ) ? ( eq_hi_start - scan_hi ) : ( part_end - eq_hi_start - sz ) );
                cnt = eq_hi_bytes / sz; dst = scan_lo; src = part_end - eq_hi_bytes;
                while( cnt-- > 0 ) { cwasm_qsort_swap( dst, src, size ); dst += sz; src += sz; }
                ptrdiff_t left = ( scan_lo - eq_lo_end ) / sz;
                ptrdiff_t right = ( eq_hi_start - scan_hi ) / sz;
                char* left_a = a;
                char* right_a = part_end - right * sz;
                if( left > 1 && right > 1 ) {
                    if( left > right ) {
                        ++top; stack[ top ].start = (size_t)( ( left_a - arr ) / sz );
                        stack[ top ].count = (size_t)left;
                        a = right_a; count = (size_t)right;
                    } else {
                        ++top; stack[ top ].start = (size_t)( ( right_a - arr ) / sz );
                        stack[ top ].count = (size_t)right;
                        a = left_a; count = (size_t)left;
                    }
                    continue;
                } else if( left > 1 ) { a = left_a; count = (size_t)left; continue; }
                else if( right > 1 ) { a = right_a; count = (size_t)right; continue; }
                else { break; }
            }
        }
    }
}

int abs( int j ) {
    if( j < 0 ) { return (int)( 0u - (unsigned)j ); }
    return j;
}

long labs( long j ) {
    if( j < 0 ) { return (long)( 0ul - (unsigned long)j ); }
    return j;
}

#if __STDC_VERSION__ >= 199901L
    long long llabs( long long j ) {
        if( j < 0 ) { return (long long)( 0ull - (unsigned long long)j ); }
        return j;
    }
#endif

div_t div( int numer, int denom ) {
    div_t r;
    if( denom == 0 ) {
        errno = EDOM;
        r.quot = 0;
        r.rem = numer;
        return r;
    }
    r.quot = numer / denom;
    r.rem = numer % denom;
    return r;
}

ldiv_t ldiv( long numer, long denom ) {
    ldiv_t r;
    if( denom == 0 ) {
        errno = EDOM;
        r.quot = 0;
        r.rem = numer;
        return r;
    }
    r.quot = numer / denom;
    r.rem = numer % denom;
    return r;
}

#if __STDC_VERSION__ >= 199901L
    lldiv_t lldiv( long long numer, long long denom ) {
        lldiv_t r;
        if( denom == 0 ) {
            errno = EDOM;
            r.quot = 0;
            r.rem = numer;
            return r;
        }
        r.quot = numer / denom;
        r.rem = numer % denom;
        return r;
    }
#endif

char* getenv( char const* name ) {
    return cwasm_getenv( name );
}

int system( char const* command ) {
    if( !command ) { return 0; }
    errno = ENOSYS;
    return -1;
}

typedef struct cwasm_atexit_node {
    void ( *func )( void );
    void ( *cxa_func )( void* );
    void* arg;
    struct cwasm_atexit_node* next;
} cwasm_atexit_node;

static cwasm_atexit_node* cwasm_atexit_head;
static cwasm_atexit_node* cwasm_at_quick_exit_head;
static cwasm_lock_t g_atexit_lock = CWASM_LOCK_INIT;

static int cwasm_atexit_push( cwasm_atexit_node** head, void ( *func )( void ),
    void ( *cxa_func )( void* ), void* arg ) {

    cwasm_atexit_node* n = (cwasm_atexit_node*)malloc( sizeof( *n ) );
    if( !n ) { return 1; }
    n->func = func;
    n->cxa_func = cxa_func;
    n->arg = arg;
    cwasm_lock( &g_atexit_lock );
    n->next = *head;
    *head = n;
    cwasm_unlock( &g_atexit_lock );
    return 0;
}


static void cwasm_atexit_run( cwasm_atexit_node** head ) {
    cwasm_atexit_node* n;

    cwasm_lock( &g_atexit_lock );
    n = *head;
    *head = NULL;
    cwasm_unlock( &g_atexit_lock );
    while( n ) {
        cwasm_atexit_node* next = n->next;
        if( n->cxa_func ) { n->cxa_func( n->arg ); } else { n->func(); }
        free( n );
        n = next;
    }
}

static noreturn void cwasm_halt( int status ) {
    __exit( status );
    __builtin_unreachable();
}

void _Exit( int status ) {
    cwasm_halt( status );
}

void abort( void ) {
    cwasm_halt( 134 );
}

void exit( int status ) {
    (void)fflush( NULL );
    cwasm_atexit_run( &cwasm_atexit_head );
    cwasm_halt( status );
}

int atexit( void ( *func )( void ) ) {
    if( !func ) { return 1; }
    return cwasm_atexit_push( &cwasm_atexit_head, func, NULL, NULL );
}

int at_quick_exit( void ( *func )( void ) ) {
    if( !func ) { return 1; }
    return cwasm_atexit_push( &cwasm_at_quick_exit_head, func, NULL, NULL );
}


int __cwasm_cxa_atexit( void ( *func )( void* ), void* arg ) {
    if( !func ) { return 1; }
    return cwasm_atexit_push( &cwasm_atexit_head, NULL, func, arg );
}


void quick_exit( int status ) {
    cwasm_atexit_run( &cwasm_at_quick_exit_head );
    cwasm_halt( status );
}
