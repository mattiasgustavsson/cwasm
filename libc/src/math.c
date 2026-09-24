#include <math.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>
#include <cwasm.h>

CWASM_JS_LIB( MATH, double, js_sin, ( double x ), { return Math.sin(x); })
CWASM_JS_LIB( MATH, double, js_cos, ( double x ), { return Math.cos(x); })
CWASM_JS_LIB( MATH, double, js_tan, ( double x ), { return Math.tan(x); })
CWASM_JS_LIB( MATH, double, js_asin, ( double x ), { return Math.asin(x); })
CWASM_JS_LIB( MATH, double, js_acos, ( double x ), { return Math.acos(x); })
CWASM_JS_LIB( MATH, double, js_atan, ( double x ), { return Math.atan(x); })
CWASM_JS_LIB( MATH, double, js_atan2, ( double x, double y ), { return Math.atan2(x, y); })
CWASM_JS_LIB( MATH, double, js_exp, ( double x ), { return Math.exp(x); })
CWASM_JS_LIB( MATH, double, js_log, ( double x ), { return Math.log(x); })
CWASM_JS_LIB( MATH, double, js_log10, ( double x ), { return Math.log10(x); })
CWASM_JS_LIB( MATH, double, js_log2, ( double x ), { return Math.log2(x); })
CWASM_JS_LIB( MATH, double, js_exp2, ( double x ), { return Math.pow(2, x); })
CWASM_JS_LIB( MATH, double, js_fabs, ( double x ), { return Math.abs(x); })
CWASM_JS_LIB( MATH, double, js_pow, ( double x, double y ), { return Math.pow(x, y); })
CWASM_JS_LIB( MATH, double, js_sinh, ( double x ), { return Math.sinh(x); })
CWASM_JS_LIB( MATH, double, js_cosh, ( double x ), { return Math.cosh(x); })
CWASM_JS_LIB( MATH, double, js_tanh, ( double x ), { return Math.tanh(x); })
CWASM_JS_LIB( MATH, double, js_hypot, ( double x, double y ), { return Math.hypot(x, y); })
CWASM_JS_LIB( MATH, double, js_floor, ( double x ), { return Math.floor(x); })
CWASM_JS_LIB( MATH, double, js_ceil, ( double x ), { return Math.ceil(x); })
CWASM_JS_LIB( MATH, double, js_trunc, ( double x ), { return Math.trunc(x); })
CWASM_JS_LIB( MATH, double, js_expm1, ( double x ), { return Math.expm1(x); })
CWASM_JS_LIB( MATH, double, js_log1p, ( double x ), { return Math.log1p(x); })

#ifndef FP_ILOGB0
    #define FP_ILOGB0 (-2147483647)
#endif
#ifndef FP_ILOGBNAN
    #define FP_ILOGBNAN 2147483647
#endif

typedef union {
    float f;
    uint32_t u;
} cwasm_fbits;

typedef union {
    double f;
    uint64_t u;
} cwasm_dbits;

static int cwasm_isnan_f( float x ) { return __builtin_isnan( x ); }
static int cwasm_isnan_d( double x ) { return __builtin_isnan( x ); }
static int cwasm_isfinite_d( double x ) { return __builtin_isfinite( x ); }

static double cwasm_copysign_d( double x, double y ) {
    cwasm_dbits xb = {.f = x};
    cwasm_dbits yb = {.f = y};
    xb.u = ( xb.u & 0x7fffffffffffffffull ) | ( yb.u & 0x8000000000000000ull );
    return xb.f;
}

static double cwasm_fmin_d( double a, double b ) {
    if( cwasm_isnan_d( a ) ) { return b; }
    if( cwasm_isnan_d( b ) ) { return a; }
    if( a == 0.0 && b == 0.0 ) {
        cwasm_dbits ab = {.f = a};
        cwasm_dbits bb = {.f = b};
        return ( ab.u | bb.u ) & 0x8000000000000000ull ? -0.0 : 0.0;
    }
    return a < b ? a : b;
}

static double cwasm_fmax_d( double a, double b ) {
    if( cwasm_isnan_d( a ) ) { return b; }
    if( cwasm_isnan_d( b ) ) { return a; }
    if( a == 0.0 && b == 0.0 ) {
        cwasm_dbits ab = {.f = a};
        cwasm_dbits bb = {.f = b};
        return ( ab.u & bb.u ) & 0x8000000000000000ull ? -0.0 : 0.0;
    }
    return a > b ? a : b;
}

static double cwasm_frexp_d( double x, int* eptr ) {
    cwasm_dbits y = {.f = x};
    uint64_t sign = y.u & 0x8000000000000000ull;
    uint64_t ix = ( y.u >> 52 ) & 0x7ffu;
    int e = 0;

    if( !ix ) {
        if( x == 0.0 ) {
            if( eptr ) { *eptr = 0; }
            return x;
        }
        y.u &= 0x7fffffffffffffffull;
        while( !( y.u & 0x7ff0000000000000ull ) ) {
            y.u <<= 1;
            --e;
        }
        ix = ( y.u >> 52 ) & 0x7ffu;
    } else if( ix == 0x7ffu ) {
        if( eptr ) { *eptr = 0; }
        return x;
    }

    if( eptr ) { *eptr = (int)ix - 0x3fe + e; }
    y.u &= 0x000fffffffffffffull;
    y.u |= 0x3fe0000000000000ull | sign;
    return y.f;
}

static float cwasm_frexp_f( float x, int* eptr ) {
    cwasm_fbits y = {.f = x};
    uint32_t sign = y.u & 0x80000000u;
    uint32_t ix = ( y.u >> 23 ) & 0xffu;
    int e = 0;

    if( !ix ) {
        if( x == 0.0f ) {
            if( eptr ) { *eptr = 0; }
            return x;
        }
        y.u &= 0x7fffffffu;
        while( !( y.u & 0x7f800000u ) ) {
            y.u <<= 1;
            --e;
        }
        ix = ( y.u >> 23 ) & 0xffu;
    } else if( ix == 0xffu ) {
        if( eptr ) { *eptr = 0; }
        return x;
    }

    if( eptr ) { *eptr = (int)ix - 0x7e + e; }
    y.u &= 0x007fffffu;
    y.u |= 0x3f000000u | sign;
    return y.f;
}

static double cwasm_ldexp_d( double x, int n ) {
    cwasm_dbits u = {.f = x};
    uint64_t ix = u.u;
    uint64_t sign = ix & 0x8000000000000000ull;
    uint64_t abs = ix & 0x7fffffffffffffffull;
    uint64_t lx;
    int exp;

    if( !abs ) { return x; }
    if( abs >= 0x7ff0000000000000ull ) { return x; }

    lx = abs & 0x000fffffffffffffull;
    exp = (int)( ( abs >> 52 ) & 0x7ff );
    if( exp == 0 ) {
        while( !( lx & 0x0010000000000000ull ) ) {
            lx <<= 1;
            --exp;
        }
        lx &= 0x000fffffffffffffull;
        ++exp;
    } else {
        lx |= 0x0010000000000000ull;
    }

    exp += n;
    if( exp > 0x7fe ) {
        u.u = sign | 0x7ff0000000000000ull;
        return u.f;
    }
    if( exp <= 0 ) {
        if( exp < -52 ) {
            u.u = sign;
            return u.f;
        }
        lx >>= ( 1 - exp );
        u.u = sign | ( lx & 0x000fffffffffffffull );
        return u.f;
    }

    u.u = sign | ( (uint64_t)exp << 52 ) | ( lx & 0x000fffffffffffffull );
    return u.f;
}

static float cwasm_ldexp_f( float x, int n ) {
    cwasm_fbits u = {.f = x};
    uint32_t sign = u.u & 0x80000000u;
    uint32_t abs = u.u & 0x7fffffffu;
    int e;

    if( !abs || abs >= 0x7f800000u ) { return x; }

    e = (int)( ( abs >> 23 ) & 0xff ) + n;
    if( e >= 0xff ) {
        u.u = sign | 0x7f800000u;
        return u.f;
    }
    if( e <= 0 ) {
        if( e < -23 ) {
            u.u = sign;
            return u.f;
        }
        abs = ( abs & 0x007fffffu ) | ( 0x7fu << 23 );
        abs >>= (unsigned)( 1 - e );
        u.u = sign | ( abs & 0x007fffffu );
        return u.f;
    }
    u.u = sign | ( (uint32_t)e << 23 ) | ( u.u & 0x007fffffu );
    return u.f;
}

static double cwasm_fmod_core( double x, double y, uint32_t* qlow ) {
    cwasm_dbits ux = {.f = x};
    cwasm_dbits uy = {.f = y};
    uint64_t ax = ux.u & 0x7fffffffffffffffull;
    uint64_t ay = uy.u & 0x7fffffffffffffffull;
    uint64_t sx = ux.u & 0x8000000000000000ull;
    uint64_t mx;
    uint64_t my;
    uint64_t d;
    uint32_t q = 0;
    int ex;
    int ey;

    if( qlow ) { *qlow = 0; }

    if( ay == 0 || ax >= 0x7ff0000000000000ull || ay > 0x7ff0000000000000ull ) {
        return __cwasm_nan_d();
    }
    if( ax < ay ) { return x; }
    if( ax == ay ) {
        if( qlow ) { *qlow = 1; }
        return sx ? -0.0 : 0.0;
    }

    ex = (int)( ax >> 52 );
    mx = ax & 0x000fffffffffffffull;
    if( ex ) { mx |= 0x0010000000000000ull; }
    else
        for( ex = 1; !( mx & 0x0010000000000000ull ); mx <<= 1 ) { --ex; }
    ey = (int)( ay >> 52 );
    my = ay & 0x000fffffffffffffull;
    if( ey ) { my |= 0x0010000000000000ull; }
    else
        for( ey = 1; !( my & 0x0010000000000000ull ); my <<= 1 ) { --ey; }

    for( ; ex > ey; --ex ) {
        q <<= 1;
        d = mx - my;
        if( !( d & 0x8000000000000000ull ) ) {
            mx = d;
            q |= 1;
        }
        mx <<= 1;
    }
    q <<= 1;
    d = mx - my;
    if( !( d & 0x8000000000000000ull ) ) {
        mx = d;
        q |= 1;
    }

    if( qlow ) { *qlow = q; }
    if( mx == 0 ) { return sx ? -0.0 : 0.0; }

    for( ; !( mx & 0x0010000000000000ull ); mx <<= 1 ) { --ex; }
    if( ex > 0 ) {
        ux.u = sx | ( (uint64_t)ex << 52 ) | ( mx & 0x000fffffffffffffull );
    } else { ux.u = sx | ( mx >> ( 1 - ex ) ); }
    return ux.f;
}

static double cwasm_fmod_d( double x, double y ) {
    return cwasm_fmod_core( x, y, 0 );
}

static double cwasm_nextafter_d( double x, double y ) {
    cwasm_dbits ux = {.f = x};
    cwasm_dbits uy = {.f = y};
    uint64_t ax;
    uint64_t ay;

    if( cwasm_isnan_d( x ) || cwasm_isnan_d( y ) ) { return x + y; }
    if( ux.u == uy.u ) { return y; }
    if( x == 0.0 ) {
        ux.u = ( uy.u & 0x8000000000000000ull ) | 1ull;
        return ux.f;
    }
    ax = ux.u & 0x7fffffffffffffffull;
    ay = uy.u & 0x7fffffffffffffffull;
    if( ax > ay || ( ( ux.u ^ uy.u ) & 0x8000000000000000ull ) ) { --ux.u; }
    else { ++ux.u; }
    return ux.f;
}

static float cwasm_nextafter_f( float x, float y ) {
    cwasm_fbits ux = {.f = x};
    cwasm_fbits uy = {.f = y};
    uint32_t ax;
    uint32_t ay;

    if( cwasm_isnan_f( x ) || cwasm_isnan_f( y ) ) { return x + y; }
    if( ux.u == uy.u ) { return y; }
    if( x == 0.0f ) {
        ux.u = ( uy.u & 0x80000000u ) | 1u;
        return ux.f;
    }
    ax = ux.u & 0x7fffffffu;
    ay = uy.u & 0x7fffffffu;
    if( ax > ay || ( ( ux.u ^ uy.u ) & 0x80000000u ) ) {
        --ux.u;
    } else { ++ux.u; }
    return ux.f;
}

static int cwasm_ilogb_d( double x ) {
    cwasm_dbits u = {.f = x};
    int e;

    if( !u.u ) { return FP_ILOGB0; }
    if( ( u.u >> 52 ) == 0x7ffu ) { return FP_ILOGBNAN; }
    if( !( u.u & 0x7ff0000000000000ull ) ) {
        cwasm_frexp_d( x, &e );
        return e - 1;
    }
    return (int)( ( u.u >> 52 ) & 0x7ff ) - 1023;
}

static int cwasm_ilogb_f( float x ) {
    cwasm_fbits u = {.f = x};
    int e;

    if( !u.u ) { return FP_ILOGB0; }
    if( ( u.u >> 23 ) == 0xffu ) { return FP_ILOGBNAN; }
    if( !( u.u & 0x7f800000u ) ) {
        cwasm_frexp_f( x, &e );
        return e - 1;
    }
    return (int)( ( u.u >> 23 ) & 0xff ) - 127;
}

static double cwasm_remainder_d( double x, double y ) {
    double ay;
    double r;
    double d;
    uint32_t q;

    if( !cwasm_isfinite_d( x ) || cwasm_isnan_d( y ) || y == 0.0 ) {
        return __cwasm_nan_d();
    }
    if( !cwasm_isfinite_d( y ) ) {
        return x;
    }

    ay = js_fabs( y );
    r = cwasm_fmod_core( js_fabs( x ), ay, &q );

    // ay - r is exact whenever r is at least ay/2 (the only case where the
    // comparison is close), so this decides the halfway case reliably.
    d = ay - r;
    if( r > d || ( r == d && ( q & 1 ) ) ) { r = -d; }

    return __builtin_signbit( x ) ? -r : r;
}

static float cwasm_remainder_f( float x, float y ) {
    return (float)cwasm_remainder_d( (double)x, (double)y );
}

static double cwasm_remquo_d( double x, double y, int* quo ) {
    double ay;
    double r;
    double d;
    uint32_t q;

    if( !cwasm_isfinite_d( x ) || cwasm_isnan_d( y ) || y == 0.0 ) {
        if( quo ) { *quo = 0; }
        return __cwasm_nan_d();
    }
    if( !cwasm_isfinite_d( y ) ) {
        if( quo ) { *quo = 0; }
        return x;
    }

    ay = js_fabs( y );
    r = cwasm_fmod_core( js_fabs( x ), ay, &q );

    d = ay - r;
    if( r > d || ( r == d && ( q & 1 ) ) ) {
        r = -d;
        ++q;
    }

    if( quo ) {
        int m = (int)( q & 0x7fffffffu );
        *quo = ( __builtin_signbit( x ) != __builtin_signbit( y ) ) ? -m : m;
    }
    return __builtin_signbit( x ) ? -r : r;
}

static float cwasm_remquo_f( float x, float y, int* quo ) {
    return (float)cwasm_remquo_d( (double)x, (double)y, quo );
}

static double cwasm_scalbln_d( double x, long n ) {
    while( n > INT_MAX ) {
        x = cwasm_ldexp_d( x, INT_MAX );
        n -= INT_MAX;
    }
    while( n < INT_MIN ) {
        x = cwasm_ldexp_d( x, INT_MIN );
        n -= INT_MIN;
    }
    return cwasm_ldexp_d( x, (int)n );
}

static float cwasm_scalbln_f( float x, long n ) {
    return (float)cwasm_scalbln_d( (double)x, n );
}

double ldexp( double x, int exp ) { return cwasm_ldexp_d( x, exp ); }
float ldexpf( float x, int exp ) { return cwasm_ldexp_f( x, exp ); }
long double ldexpl( long double x, int exp ) { return (long double)ldexp( (double)x, exp ); }

double frexp( double x, int* exp ) { return cwasm_frexp_d( x, exp ); }
float frexpf( float x, int* exp ) { return cwasm_frexp_f( x, exp ); }
long double frexpl( long double x, int* exp ) { return (long double)frexp( (double)x, exp ); }

double modf( double x, double* iptr ) {
    double i;

    if( cwasm_isnan_d( x ) ) {
        if( iptr ) { *iptr = x; }
        return x;
    }
    if( !cwasm_isfinite_d( x ) ) {
        if( iptr ) { *iptr = x; }
        return cwasm_copysign_d( 0.0, x );
    }
    i = ( x < 0.0 ) ? js_ceil( x ) : js_floor( x );
    if( iptr ) { *iptr = i; }
    return x - i;
}

float modff( float x, float* iptr ) {
    float i;

    if( cwasm_isnan_f( x ) ) {
        if( iptr ) { *iptr = x; }
        return x;
    }
    if( !cwasm_isfinite_d( (double)x ) ) {
        if( iptr ) { *iptr = x; }
        return (float)cwasm_copysign_d( 0.0, (double)x );
    }
    i = ( x < 0.0f ) ? (float)js_ceil( (double)x ) : (float)js_floor( (double)x );
    if( iptr ) { *iptr = i; }
    return x - i;
}

long double modfl( long double x, long double* iptr ) {
    double ipart;
    double frac = modf( (double)x, &ipart );
    if( iptr ) { *iptr = (long double)ipart; }
    return (long double)frac;
}

double scalbn( double x, int n ) { return cwasm_ldexp_d( x, n ); }
float scalbnf( float x, int n ) { return cwasm_ldexp_f( x, n ); }
long double scalbnl( long double x, int n ) { return (long double)scalbn( (double)x, n ); }

double scalbln( double x, long n ) { return cwasm_scalbln_d( x, n ); }
float scalblnf( float x, long n ) { return cwasm_scalbln_f( x, n ); }
long double scalblnl( long double x, long n ) { return (long double)scalbln( (double)x, n ); }

int ilogb( double x ) { return cwasm_ilogb_d( x ); }
int ilogbf( float x ) { return cwasm_ilogb_f( x ); }
int ilogbl( long double x ) { return ilogb( (double)x ); }

double logb( double x ) {
    if( cwasm_isnan_d( x ) ) { return x; }
    if( x == 0.0 ) { return -__cwasm_inf_d(); }
    if( !cwasm_isfinite_d( x ) ) { return __cwasm_inf_d(); }
    return (double)cwasm_ilogb_d( x );
}
float logbf( float x ) { return (float)logb( (double)x ); }
long double logbl( long double x ) { return (long double)logb( (double)x ); }

double nextafter( double x, double y ) { return cwasm_nextafter_d( x, y ); }
float nextafterf( float x, float y ) { return cwasm_nextafter_f( x, y ); }
long double nextafterl( long double x, long double y ) {
    return (long double)nextafter( (double)x, (double)y );
}

double nexttoward( double x, long double y ) { return cwasm_nextafter_d( x, (double)y ); }
float nexttowardf( float x, long double y ) { return cwasm_nextafter_f( x, (double)y ); }
long double nexttowardl( long double x, long double y ) {
    return (long double)nexttoward( (double)x, y );
}

double remainder( double x, double y ) { if( y == 0.0 && !cwasm_isnan_d( x ) ) errno = EDOM; return cwasm_remainder_d( x, y ); }
float remainderf( float x, float y ) {
    if( y == 0.0f && !cwasm_isnan_f( x ) ) { errno = EDOM; }
    return cwasm_remainder_f( x, y );
}
long double remainderl( long double x, long double y ) {
    return (long double)remainder( (double)x, (double)y );
}

double remquo( double x, double y, int* quo ) { return cwasm_remquo_d( x, y, quo ); }
float remquof( float x, float y, int* quo ) { return cwasm_remquo_f( x, y, quo ); }
long double remquol( long double x, long double y, int* quo ) {
    return (long double)remquo( (double)x, (double)y, quo );
}

// math_errhandling is MATH_ERREXCEPT, so errno is not the declared channel,
// but libm sets it anyway wherever it runs

static double math_ovf_d( double r, double x ) {
    if( __builtin_isinf( r ) && cwasm_isfinite_d( x ) ) { errno = ERANGE; }
    return r;
}

static float math_ovf_f( float r, float x ) {
    if( __builtin_isinf( r ) && __builtin_isfinite( x ) ) { errno = ERANGE; }
    return r;
}

static double math_pow_err( double x, double y ) {
    if( cwasm_isfinite_d( x ) && x < 0.0 && cwasm_isfinite_d( y ) && y != js_trunc( y ) ) {
        errno = EDOM;
    } else if( x == 0.0 && y < 0.0 ) { errno = ERANGE; }
    return x;
}

float sinf( float x ) { return (float)js_sin( (double)x ); }
float cosf( float x ) { return (float)js_cos( (double)x ); }
float tanf( float x ) { return (float)js_tan( (double)x ); }
float asinf( float x ) { return (float)asin( (double)x ); }
float acosf( float x ) { return (float)acos( (double)x ); }
float atanf( float x ) { return (float)js_atan( (double)x ); }
float atan2f( float y, float x ) { return (float)js_atan2( (double)y, (double)x ); }
float expf( float x ) { return math_ovf_f( (float)exp( (double)x ), x ); }
float logf( float x ) { return (float)log( (double)x ); }
float log10f( float x ) { return (float)log10( (double)x ); }
float log2f( float x ) { return (float)log2( (double)x ); }
float exp2f( float x ) { return math_ovf_f( (float)exp2( (double)x ), x ); }
float sqrtf( float x ) { if( x < 0.0f ) errno = EDOM; return __builtin_sqrtf( x ); }
float fabsf( float x ) { return (float)js_fabs( (double)x ); }
// The exact remainder of two float values is itself a float value, and the
// double computation is exact, so rounding back is lossless
float fmodf( float x, float y ) { return (float)fmod( (double)x, (double)y ); }
float powf( float x, float y ) { return math_ovf_f( (float)pow( (double)x, (double)y ), x ); }
float sinhf( float x ) { return math_ovf_f( (float)sinh( (double)x ), x ); }
float coshf( float x ) { return math_ovf_f( (float)cosh( (double)x ), x ); }
float tanhf( float x ) { return (float)js_tanh( (double)x ); }
float hypotf( float x, float y ) { return (float)js_hypot( (double)x, (double)y ); }
// C round(): round half AWAY FROM ZERO (Math.round rounds half toward +inf, wrong for negatives
// frac = x - trunc(x) is exact for finite x, so there is no floor(x+0.5) rounding bug
static double cwasm_round_d( double x ) {
    if( !cwasm_isfinite_d( x ) ) { return x; }
    double t = js_trunc( x );
    double d = x - t;
    if( d >= 0.5 ) { return t + 1.0; }
    if( d <= -0.5 ) { return t - 1.0; }
    return t;
}

float floorf( float x ) { return (float)js_floor( (double)x ); }
float ceilf( float x ) { return (float)js_ceil( (double)x ); }
float truncf( float x ) { return (float)js_trunc( (double)x ); }
float roundf( float x ) { return (float)cwasm_round_d( (double)x ); }
float nearbyintf( float x ) { return __builtin_nearbyintf( x ); }
float rintf( float x ) { return __builtin_rintf( x ); }
float fminf( float a, float b ) { return (float)cwasm_fmin_d( (double)a, (double)b ); }
float fmaxf( float a, float b ) { return (float)cwasm_fmax_d( (double)a, (double)b ); }
// __builtin_fma[f] recurses under -fno-builtin. Wider type, one final rounding.
float fmaf( float x, float y, float z ) { return (float)( (double)x * (double)y + (double)z ); }
float copysignf( float x, float y ) { return (float)cwasm_copysign_d( (double)x, (double)y ); }


double sin( double x ) { return js_sin( x ); }
double cos( double x ) { return js_cos( x ); }
double tan( double x ) { return js_tan( x ); }
double asin( double x ) { if( js_fabs( x ) > 1.0 ) errno = EDOM; return js_asin( x ); }
double acos( double x ) { if( js_fabs( x ) > 1.0 ) errno = EDOM; return js_acos( x ); }
double atan( double x ) { return js_atan( x ); }
double atan2( double y, double x ) { return js_atan2( y, x ); }
double exp( double x ) { return math_ovf_d( js_exp( x ), x ); }
double log( double x ) { if( x < 0.0 ) errno = EDOM; else if( x == 0.0 ) errno = ERANGE; return js_log( x ); }
double log10( double x ) { if( x < 0.0 ) errno = EDOM; else if( x == 0.0 ) errno = ERANGE; return js_log10( x ); }
double log2( double x ) { if( x < 0.0 ) errno = EDOM; else if( x == 0.0 ) errno = ERANGE; return js_log2( x ); }
double exp2( double x ) { return math_ovf_d( js_exp2( x ), x ); }
// __builtin_sqrt lowers to f64.sqrt; a bare sqrt() would recurse under -fno-builtin.
double sqrt( double x ) { if( x < 0.0 ) errno = EDOM; return __builtin_sqrt( x ); }
double fabs( double x ) { return js_fabs( x ); }
double fmod( double x, double y ) { if( y == 0.0 && !cwasm_isnan_d( x ) ) errno = EDOM; return cwasm_fmod_d( x, y ); }
double pow( double x, double y ) { math_pow_err( x, y ); return math_ovf_d( js_pow( x, y ), x ); }
double sinh( double x ) { return math_ovf_d( js_sinh( x ), x ); }
double cosh( double x ) { return math_ovf_d( js_cosh( x ), x ); }
double tanh( double x ) { return js_tanh( x ); }
double hypot( double x, double y ) { return js_hypot( x, y ); }
double floor( double x ) { return js_floor( x ); }
double ceil( double x ) { return js_ceil( x ); }
double trunc( double x ) { return js_trunc( x ); }
double round( double x ) { return cwasm_round_d( x ); }
double nearbyint( double x ) { return __builtin_nearbyint( x ); }
double rint( double x ) { return __builtin_rint( x ); }
double fmin( double a, double b ) { return cwasm_fmin_d( a, b ); }
double fmax( double a, double b ) { return cwasm_fmax_d( a, b ); }
double fma( double x, double y, double z ) { return (double)( (long double)x * (long double)y + (long double)z ); }
double copysign( double x, double y ) { return cwasm_copysign_d( x, y ); }

long double fabsl( long double x ) { return (long double)fabs( (double)x ); }
long double fmaxl( long double a, long double b ) { return (long double)fmax( (double)a, (double)b ); }
long double fminl( long double a, long double b ) { return (long double)fmin( (double)a, (double)b ); }
long double copysignl( long double x, long double y ) { return (long double)copysign( (double)x, (double)y ); }

long double sinl( long double x ) { return (long double)sin( (double)x ); }
long double cosl( long double x ) { return (long double)cos( (double)x ); }
long double tanl( long double x ) { return (long double)tan( (double)x ); }
long double asinl( long double x ) { return (long double)asin( (double)x ); }
long double acosl( long double x ) { return (long double)acos( (double)x ); }
long double atanl( long double x ) { return (long double)atan( (double)x ); }
long double expl( long double x ) { return (long double)exp( (double)x ); }
long double logl( long double x ) { return (long double)log( (double)x ); }
long double log10l( long double x ) { return (long double)log10( (double)x ); }
long double log2l( long double x ) { return (long double)log2( (double)x ); }
long double exp2l( long double x ) { return (long double)exp2( (double)x ); }
long double sqrtl( long double x ) { return (long double)sqrt( (double)x ); }
long double floorl( long double x ) { return (long double)floor( (double)x ); }
long double ceill( long double x ) { return (long double)ceil( (double)x ); }
long double truncl( long double x ) { return (long double)trunc( (double)x ); }
long double roundl( long double x ) { return (long double)round( (double)x ); }
long double nearbyintl( long double x ) { return (long double)nearbyint( (double)x ); }
long double rintl( long double x ) { return (long double)rint( (double)x ); }
long double sinhl( long double x ) { return (long double)sinh( (double)x ); }
long double coshl( long double x ) { return (long double)cosh( (double)x ); }
long double tanhl( long double x ) { return (long double)tanh( (double)x ); }
long double atan2l( long double y, long double x ) { return (long double)atan2( (double)y, (double)x ); }
long double fmodl( long double x, long double y ) { return (long double)fmod( (double)x, (double)y ); }
long double powl( long double x, long double y ) { return (long double)pow( (double)x, (double)y ); }
long double hypotl( long double x, long double y ) { return (long double)hypot( (double)x, (double)y ); }
long double fmal( long double x, long double y, long double z ) { return (long double)fma( (double)x, (double)y, (double)z ); }

float nanf( char const* tagp ) { (void)tagp; return __builtin_nanf( "" ); }
double nan( char const* tagp ) { (void)tagp; return __builtin_nan( "" ); }
long double nanl( char const* tagp ) { (void)tagp; return (long double)__builtin_nan( "" ); }

static double cwasm_acosh_d( double x ) {
    if( x < 1.0 ) { return __builtin_nan( "" ); }
    if( x == 1.0 ) { return 0.0; }
    if( !cwasm_isfinite_d( x ) ) { return x; }
    if( x >= 0x1p26 ) { return js_log( x ) + 0.693147180559945309417; }
    if( x >= 2.0 ) { return js_log( 2.0 * x - 1.0 / ( x + __builtin_sqrt( x * x - 1.0 ) ) ); }
    double t = x - 1.0;
    return js_log1p( t + __builtin_sqrt( 2.0 * t + t * t ) );
}
static double cwasm_asinh_d( double x ) {
    double ax = js_fabs( x );
    if( !cwasm_isfinite_d( x ) || x == 0.0 ) { return x; }
    if( ax >= 0x1p26 ) { return cwasm_copysign_d( js_log( ax ) + 0.693147180559945309417, x ); }
    if( ax >= 2.0 ) { return cwasm_copysign_d( js_log( 2.0 * ax + 1.0 / ( ax + __builtin_sqrt( ax * ax + 1.0 ) ) ), x ); }
    return cwasm_copysign_d( js_log1p( ax + ax * ax / ( 1.0 + __builtin_sqrt( 1.0 + ax * ax ) ) ), x );
}
static double cwasm_atanh_d( double x ) {
    return 0.5 * js_log( ( 1.0 + x ) / ( 1.0 - x ) );
}

double acosh( double x ) { if( x < 1.0 ) errno = EDOM; return cwasm_acosh_d( x ); }
float acoshf( float x ) { return (float)acosh( (double)x ); }
long double acoshl( long double x ) { return (long double)acosh( (double)x ); }

double asinh( double x ) { return cwasm_asinh_d( x ); }
float asinhf( float x ) { return (float)asinh( (double)x ); }
long double asinhl( long double x ) { return (long double)asinh( (double)x ); }

double atanh( double x ) { double a = js_fabs( x ); if( a > 1.0 ) errno = EDOM; else if( a == 1.0 ) errno = ERANGE; return cwasm_atanh_d( x ); }
float atanhf( float x ) { return (float)atanh( (double)x ); }
long double atanhl( long double x ) { return (long double)atanh( (double)x ); }

double cbrt( double x ) {
    int e;
    int sign;
    double m;
    double y;
    double y3;

    if( x == 0.0 || !cwasm_isfinite_d( x ) ) { return x; }
    sign = 0;
    if( x < 0.0 ) {
        sign = 1;
        x = -x;
    }
    m = cwasm_frexp_d( x, &e );
    while( e % 3 ) {
        m *= 0.5;
        ++e;
    }
    y = js_pow( m, 1.0 / 3.0 );
    y = cwasm_ldexp_d( y, e / 3 );
    y = ( 2.0 * y + x / ( y * y ) ) / 3.0;
    y3 = y * y * y;
    y = y - ( y3 - x ) / ( 3.0 * y * y );
    return sign ? -y : y;
}
float cbrtf( float x ) { return (float)cbrt( (double)x ); }
long double cbrtl( long double x ) { return (long double)cbrt( (double)x ); }

double expm1( double x ) { return math_ovf_d( js_expm1( x ), x ); }
float expm1f( float x ) { return math_ovf_f( (float)expm1( (double)x ), x ); }
long double expm1l( long double x ) { return (long double)expm1( (double)x ); }

double fdim( double x, double y ) {
    if( cwasm_isnan_d( x ) || cwasm_isnan_d( y ) ) { return __cwasm_nan_d(); }
    return x > y ? x - y : 0.0;
}
float fdimf( float x, float y ) { return (float)fdim( (double)x, (double)y ); }
long double fdiml( long double x, long double y ) { return (long double)fdim( (double)x, (double)y ); }


#define ERF_SQRTPI 1.7724538509055160273
#define ERF_2_SQRTPI 1.1283791670955126232

static double erf_series( double ax ) {
    double x2 = ax * ax;
    double term = ax;
    double sum = ax;
    for( int n = 0; n < 400; ++n ) {
        term *= -x2 / (double)( n + 1 );
        double add = term / (double)( 2 * ( n + 1 ) + 1 );
        sum += add;
        if( js_fabs( add ) <= 1e-19 * js_fabs( sum ) ) { break; }
    }
    return ERF_2_SQRTPI * sum;
}

static double erfc_cf( double ax ) {
    double const tiny = 1e-300;
    double f = ax;
    double C = f;
    double D = 0.0;
    for( int i = 1; i < 400; ++i ) {
        double a = 0.5 * (double)i;
        D = ax + a * D; if( D == 0.0 ) D = tiny; D = 1.0 / D;
        C = ax + a / C; if( C == 0.0 ) C = tiny;
        double delta = C * D;
        f *= delta;
        if( js_fabs( delta - 1.0 ) <= 1e-17 ) { break; }
    }
    return js_exp( -ax * ax ) / ERF_SQRTPI / f;
}

static double cwasm_erf_d( double x ) {
    if( cwasm_isnan_d( x ) ) { return x; }
    if( __builtin_isinf( x ) ) {
        return x > 0.0 ? 1.0 : -1.0;
    }
    double ax = js_fabs( x );
    if( ax == 0.0 ) { return x; }
    double e = ax < 1.0 ? erf_series( ax ) : 1.0 - erfc_cf( ax );
    return x < 0.0 ? -e : e;
}

static double cwasm_erfc_d( double x ) {
    if( cwasm_isnan_d( x ) ) { return x; }
    if( __builtin_isinf( x ) ) {
        return x > 0.0 ? 0.0 : 2.0;
    }
    double ax = js_fabs( x );
    double ec = ax < 1.0 ? 1.0 - erf_series( ax ) : erfc_cf( ax );
    return x < 0.0 ? 2.0 - ec : ec;
}

double erf( double x ) { return cwasm_erf_d( x ); }
float erff( float x ) { return (float)cwasm_erf_d( (double)x ); }
long double erfl( long double x ) { return (long double)cwasm_erf_d( (double)x ); }
double erfc( double x ) { return cwasm_erfc_d( x ); }
float erfcf( float x ) { return (float)cwasm_erfc_d( (double)x ); }
long double erfcl( long double x ) { return (long double)cwasm_erfc_d( (double)x ); }

double log1p( double x ) { if( x < -1.0 ) errno = EDOM; else if( x == -1.0 ) errno = ERANGE; return js_log1p( x ); }
float log1pf( float x ) { return (float)log1p( (double)x ); }
long double log1pl( long double x ) { return (long double)log1p( (double)x ); }
#define GAMMA_LN_SQRT_2PI 0.91893853320467274178
#define GAMMA_PI 3.14159265358979323846

static double lgamma_stirling( double z ) {
    double w = 1.0 / z;
    double w2 = w * w;
    double series = w * ( 1.0 / 12.0
        + w2 * ( -1.0 / 360.0
        + w2 * ( 1.0 / 1260.0
        + w2 * ( -1.0 / 1680.0
        + w2 * ( 1.0 / 1188.0
        + w2 * ( -691.0 / 360360.0
        + w2 * ( 1.0 / 156.0 ) ) ) ) ) ) );
    return ( z - 0.5 ) * js_log( z ) - z + GAMMA_LN_SQRT_2PI + series;
}

static double lgamma_pos( double x ) {
    double scale = 0.0;
    double z = x;

    while( z < 16.0 ) {
        scale += js_log( z );
        z += 1.0;
    }
    return lgamma_stirling( z ) - scale;
}

static double cwasm_lgamma_d( double x ) {
    if( cwasm_isnan_d( x ) ) { return x; }
    if( !cwasm_isfinite_d( x ) ) { return __cwasm_inf_d(); }
    if( x == 0.0 ) { return __cwasm_inf_d(); }
    if( x < 0.0 && x == js_floor( x ) ) {
        return __cwasm_inf_d();
    }

    if( x < 0.5 ) {
        double s = js_fabs( js_sin( GAMMA_PI * x ) );
        return js_log( GAMMA_PI / s ) - lgamma_pos( 1.0 - x );
    }
    return lgamma_pos( x );
}

static double cwasm_tgamma_d( double x ) {
    if( cwasm_isnan_d( x ) ) { return x; }
    if( x == __cwasm_inf_d() ) { return x; }
    if( x == -__cwasm_inf_d() ) { return __cwasm_nan_d(); }
    if( x == 0.0 ) { return cwasm_copysign_d( __cwasm_inf_d(), x ); }
    if( x < 0.0 && x == js_floor( x ) ) {
        return __cwasm_nan_d();
    }

    if( x < 0.5 ) {
        double s = js_sin( GAMMA_PI * x );
        return GAMMA_PI / ( s * js_exp( lgamma_pos( 1.0 - x ) ) );
    }

    if( x == js_floor( x ) && x <= 23.0 ) {
        double r = 1.0;
        for( double k = 2.0; k < x; k += 1.0 ) { r *= k; }
        return r;
    }

    double g = js_exp( lgamma_pos( x ) );
    if( !cwasm_isfinite_d( g ) ) { errno = ERANGE; }
    return g;
}

double tgamma( double x ) { return cwasm_tgamma_d( x ); }

float tgammaf( float x ) { return (float)cwasm_tgamma_d( (double)x ); }
long double tgammal( long double x ) { return (long double)cwasm_tgamma_d( (double)x ); }

double lgamma( double x ) { return cwasm_lgamma_d( x ); }

float lgammaf( float x ) { return (float)lgamma( (double)x ); }
long double lgammal( long double x ) { return (long double)lgamma( (double)x ); }

long lrint( double x ) { return (long)__builtin_nearbyint( x ); }
long lrintf( float x ) { return (long)__builtin_nearbyintf( x ); }
long lrintl( long double x ) { return (long)__builtin_nearbyint( (double)x ); }

long long llrint( double x ) { return (long long)__builtin_nearbyint( x ); }
long long llrintf( float x ) { return (long long)__builtin_nearbyintf( x ); }
long long llrintl( long double x ) { return (long long)__builtin_nearbyint( (double)x ); }

long lround( double x ) { return (long)cwasm_round_d( x ); }
long lroundf( float x ) { return (long)cwasm_round_d( (double)x ); }
long lroundl( long double x ) { return (long)cwasm_round_d( (double)x ); }

long long llround( double x ) { return (long long)cwasm_round_d( x ); }
long long llroundf( float x ) { return (long long)cwasm_round_d( (double)x ); }
long long llroundl( long double x ) { return (long long)cwasm_round_d( (double)x ); }
