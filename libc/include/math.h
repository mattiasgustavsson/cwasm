// cwasm math (wasm32):
// - sin/cos/exp/log/pow/... : host Math.* via math.c CWASM_JS_LIB (JS import required)
// - fma/fmod/rint/nearbyint/ldexp/frexp : __builtin_* (wasm/IEEE, no JS)
// - long double : 16-byte fp128 (wasm32 ABI); *l libm uses double-precision ops
// - round() : JS Math.round (ties away from zero)
// - rint()/nearbyint() : IEEE round-to-nearest-even
#ifndef __CWASM_MATH_H__
#define __CWASM_MATH_H__
#include <stdint.h>
#include <stdbool.h>

#define FP_NAN 0
#define FP_INFINITE 1
#define FP_ZERO 2
#define FP_SUBNORMAL 3
#define FP_NORMAL 4

// clang defaults to -fno-math-errno on wasm32, so using MATH_ERREXCEPT instead of MATH_ERRNO
#define MATH_ERRNO 1
#define MATH_ERREXCEPT 2
#define math_errhandling MATH_ERREXCEPT

#define FP_ILOGB0 (-2147483647)
#define FP_ILOGBNAN 2147483647

static inline float __cwasm_nan_f( void ) {
    return __builtin_nanf( "" );
}

static inline float __cwasm_inf_f( void ) {
    return __builtin_inff();
}

static inline double __cwasm_nan_d( void ) {
    return __builtin_nan( "" );
}

static inline double __cwasm_inf_d( void ) {
    return __builtin_inf();
}

static inline int __cwasm_isnan_f( float x ) {
    return __builtin_isnan( x );
}

static inline int __cwasm_isnan_d( double x ) {
    return __builtin_isnan( x );
}

static inline int __cwasm_isinf_f( float x ) {
    return __builtin_isinf( x );
}

static inline int __cwasm_isinf_d( double x ) {
    return __builtin_isinf( x );
}

static inline int __cwasm_isfinite_f( float x ) {
    return __builtin_isfinite( x );
}

static inline int __cwasm_isfinite_d( double x ) {
    return __builtin_isfinite( x );
}

static inline int __cwasm_signbit_f( float x ) {
    return __builtin_signbit( x );
}

static inline int __cwasm_signbit_d( double x ) {
    return __builtin_signbit( x );
}

static inline int __cwasm_fpclassify_f( float x ) {
    uint32_t u;
    __builtin_memcpy( &u, &x, sizeof( u ) );
    uint32_t e = ( u >> 23 ) & 0xffu;
    uint32_t m = u & 0x007fffffu;
    if( e == 0xffu ) { return m ? FP_NAN : FP_INFINITE; }
    if( e == 0u ) { return m ? FP_SUBNORMAL : FP_ZERO; }
    return FP_NORMAL;
}

static inline int __cwasm_fpclassify_d( double x ) {
    uint64_t u;
    __builtin_memcpy( &u, &x, sizeof( u ) );
    uint64_t e = ( u >> 52 ) & 0x7ffull;
    if( e == 0x7ffull ) { return ( u & 0x000fffffffffffffull ) ? FP_NAN : FP_INFINITE; }
    if( e == 0ull ) { return ( u & 0x000fffffffffffffull ) ? FP_SUBNORMAL : FP_ZERO; }
    return FP_NORMAL;
}

static inline int __cwasm_isnan_l( long double x ) {
    return __builtin_isnan( x );
}

static inline int __cwasm_isinf_l( long double x ) {
    return __builtin_isinf( x );
}

static inline int __cwasm_isfinite_l( long double x ) {
    return __builtin_isfinite( x );
}

static inline int __cwasm_signbit_l( long double x ) {
    return __builtin_signbit( x );
}

static inline int __cwasm_fpclassify_l( long double x ) {
    return __builtin_fpclassify( FP_NAN, FP_INFINITE, FP_NORMAL, FP_SUBNORMAL, FP_ZERO, x );
}

#ifdef __cplusplus
    extern "C" {
#endif

#ifndef NAN
    #define NAN (__builtin_nanf(""))
#endif
#ifndef INFINITY
    #define INFINITY (__builtin_inff())
#endif

#ifndef HUGE_VALF
    #define HUGE_VALF (__builtin_inff())
#endif
#ifndef HUGE_VAL
    #define HUGE_VAL (__builtin_inf())
#endif
#ifndef HUGE_VALL
    #define HUGE_VALL (__builtin_infl())
#endif

double ldexp( double x, int exp );
float ldexpf( float x, int exp );
long double ldexpl( long double x, int exp );
double frexp( double x, int* exp );
float frexpf( float x, int* exp );
long double frexpl( long double x, int* exp );
double modf( double x, double* iptr );
float modff( float x, float* iptr );
long double modfl( long double x, long double* iptr );
double scalbn( double x, int n );
float scalbnf( float x, int n );
long double scalbnl( long double x, int n );
double scalbln( double x, long n );
float scalblnf( float x, long n );
long double scalblnl( long double x, long n );
int ilogb( double x );
int ilogbf( float x );
int ilogbl( long double x );
double logb( double x );
float logbf( float x );
long double logbl( long double x );
double nextafter( double x, double y );
float nextafterf( float x, float y );
long double nextafterl( long double x, long double y );
double nexttoward( double x, long double y );
float nexttowardf( float x, long double y );
long double nexttowardl( long double x, long double y );
double remainder( double x, double y );
float remainderf( float x, float y );
long double remainderl( long double x, long double y );
double remquo( double x, double y, int* quo );
float remquof( float x, float y, int* quo );
long double remquol( long double x, long double y, int* quo );

#ifndef M_E
    #define M_E 2.71828182845904523536
#endif
#ifndef M_LOG2E
    #define M_LOG2E 1.44269504088896340736
#endif
#ifndef M_LOG10E
    #define M_LOG10E 0.434294481903251827651
#endif
#ifndef M_LN2
    #define M_LN2 0.693147180559945309417
#endif
#ifndef M_LN10
    #define M_LN10 2.30258509299404568402
#endif
#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
    #define M_PI_2 1.57079632679489661923
#endif
#ifndef M_PI_4
    #define M_PI_4 0.785398163397448309616
#endif
#ifndef M_1_PI
    #define M_1_PI 0.318309886183790671538
#endif
#ifndef M_2_PI
    #define M_2_PI 0.636619772367581343076
#endif
#ifndef M_2_SQRTPI
    #define M_2_SQRTPI 1.12837916709551257390
#endif
#ifndef M_SQRT2
    #define M_SQRT2 1.41421356237309504880
#endif
#ifndef M_SQRT1_2
    #define M_SQRT1_2 0.707106781186547524401
#endif

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
    #ifndef isnan
        #define isnan(x) _Generic((x), \
            float:__cwasm_isnan_f, \
            long double:__cwasm_isnan_l, \
            default:__cwasm_isnan_d)(x)
    #endif
    #ifndef isinf
        #define isinf(x) _Generic((x), \
            float:__cwasm_isinf_f, \
            long double:__cwasm_isinf_l, \
            default:__cwasm_isinf_d)(x)
    #endif
    #ifndef isfinite
        #define isfinite(x) _Generic((x), \
            float:__cwasm_isfinite_f, \
            long double:__cwasm_isfinite_l, \
            default:__cwasm_isfinite_d)(x)
    #endif
    #ifndef signbit
        #define signbit(x) _Generic((x), \
            float:__cwasm_signbit_f, \
            long double:__cwasm_signbit_l, \
            default:__cwasm_signbit_d)(x)
    #endif
    #ifndef fpclassify
        #define fpclassify(x) _Generic((x), \
            float:__cwasm_fpclassify_f, \
            long double:__cwasm_fpclassify_l, \
            default:__cwasm_fpclassify_d)(x)
    #endif
    #ifndef isnormal
        #define isnormal(x) (fpclassify(x) == FP_NORMAL)
    #endif
#else
    #ifndef isnan
        #define isnan(x) __cwasm_isnan_d((double)(x))
    #endif
    #ifndef isinf
        #define isinf(x) __cwasm_isinf_d((double)(x))
    #endif
    #ifndef isfinite
        #define isfinite(x) __cwasm_isfinite_d((double)(x))
    #endif
    #ifndef signbit
        #define signbit(x) __cwasm_signbit_d((double)(x))
    #endif
    #ifndef fpclassify
        #define fpclassify(x) __cwasm_fpclassify_d((double)(x))
    #endif
    #ifndef isnormal
        #define isnormal(x) (fpclassify(x) == FP_NORMAL)
    #endif
#endif

#ifndef isunordered
    #define isunordered(x, y) __builtin_isunordered((x), (y))
#endif
#ifndef isgreater
    #define isgreater(x, y) __builtin_isgreater((x), (y))
#endif
#ifndef isgreaterequal
    #define isgreaterequal(x, y) __builtin_isgreaterequal((x), (y))
#endif
#ifndef isless
    #define isless(x, y) __builtin_isless((x), (y))
#endif
#ifndef islessequal
    #define islessequal(x, y) __builtin_islessequal((x), (y))
#endif
#ifndef islessgreater
    #define islessgreater(x, y) __builtin_islessgreater((x), (y))
#endif

float sinf( float x );
float cosf( float x );
float tanf( float x );
float asinf( float x );
float acosf( float x );
float atanf( float x );
float expf( float x );
float logf( float x );
float sqrtf( float x );
float fabsf( float x );
float fmodf( float x, float y );
float powf( float x, float y );
float atan2f( float y, float x );
float log10f( float x );
float log2f( float x );
float exp2f( float x );
float sinhf( float x );
float coshf( float x );
float tanhf( float x );
float hypotf( float x, float y );
float copysignf( float x, float y );
float fminf( float a, float b );
float fmaxf( float a, float b );
float truncf( float x );
float floorf( float x );
float ceilf( float x );
float roundf( float x );
float nearbyintf( float x );
float rintf( float x );
float fmaf( float x, float y, float z );

double fma( double x, double y, double z );

double sin( double x );
double cos( double x );
double tan( double x );
double asin( double x );
double acos( double x );
double atan( double x );
double atan2( double y, double x );
double exp( double x );
double log( double x );
double sqrt( double x );
double fabs( double x );
double fmod( double x, double y );
double pow( double x, double y );
double floor( double x );
double ceil( double x );
double trunc( double x );
double round( double x );
double nearbyint( double x );
double rint( double x );
double fmin( double a, double b );
double fmax( double a, double b );
double copysign( double x, double y );
long double fabsl( long double x );
long double fmaxl( long double a, long double b );
long double fminl( long double a, long double b );
long double copysignl( long double x, long double y );
long double sinl( long double x );
long double cosl( long double x );
long double tanl( long double x );
long double asinl( long double x );
long double acosl( long double x );
long double atanl( long double x );
long double atan2l( long double y, long double x );
long double expl( long double x );
long double logl( long double x );
long double log10l( long double x );
long double log2l( long double x );
long double exp2l( long double x );
long double sqrtl( long double x );
long double fmodl( long double x, long double y );
long double powl( long double x, long double y );
long double floorl( long double x );
long double ceill( long double x );
long double truncl( long double x );
long double roundl( long double x );
long double nearbyintl( long double x );
long double rintl( long double x );
long double sinhl( long double x );
long double coshl( long double x );
long double tanhl( long double x );
long double hypotl( long double x, long double y );
long double fmal( long double x, long double y, long double z );
double hypot( double x, double y );
double log10( double x );
double log2( double x );
double exp2( double x );
double sinh( double x );
double cosh( double x );
double tanh( double x );

float nanf( char const* tagp );
double nan( char const* tagp );
long double nanl( char const* tagp );

double acosh( double x );
float acoshf( float x );
long double acoshl( long double x );
double asinh( double x );
float asinhf( float x );
long double asinhl( long double x );
double atanh( double x );
float atanhf( float x );
long double atanhl( long double x );

double cbrt( double x );
float cbrtf( float x );
long double cbrtl( long double x );
double erf( double x );
float erff( float x );
long double erfl( long double x );
double erfc( double x );
float erfcf( float x );
long double erfcl( long double x );
double expm1( double x );
float expm1f( float x );
long double expm1l( long double x );
double fdim( double x, double y );
float fdimf( float x, float y );
long double fdiml( long double x, long double y );

double lgamma( double x );
float lgammaf( float x );
long double lgammal( long double x );
double tgamma( double x );
float tgammaf( float x );
long double tgammal( long double x );

double log1p( double x );
float log1pf( float x );
long double log1pl( long double x );

long lrint( double x );
long lrintf( float x );
long lrintl( long double x );
long long llrint( double x );
long long llrintf( float x );
long long llrintl( long double x );
long lround( double x );
long lroundf( float x );
long lroundl( long double x );
long long llround( double x );
long long llroundf( float x );
long long llroundl( long double x );

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_MATH_H__
