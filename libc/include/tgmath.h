#ifndef __CWASM_TGMATH_H__
#define __CWASM_TGMATH_H__
#include <math.h>
#include <complex.h>

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)

static inline float __cwasm_tg_sin_f( float x ) { return sinf( x ); }
static inline double __cwasm_tg_sin_d( double x ) { return sin( x ); }
static inline float __cwasm_tg_cos_f( float x ) { return cosf( x ); }
static inline double __cwasm_tg_cos_d( double x ) { return cos( x ); }
static inline float __cwasm_tg_tan_f( float x ) { return tanf( x ); }
static inline double __cwasm_tg_tan_d( double x ) { return tan( x ); }
static inline float __cwasm_tg_asin_f( float x ) { return asinf( x ); }
static inline double __cwasm_tg_asin_d( double x ) { return asin( x ); }
static inline float __cwasm_tg_acos_f( float x ) { return acosf( x ); }
static inline double __cwasm_tg_acos_d( double x ) { return acos( x ); }
static inline float __cwasm_tg_atan_f( float x ) { return atanf( x ); }
static inline double __cwasm_tg_atan_d( double x ) { return atan( x ); }
static inline float __cwasm_tg_exp_f( float x ) { return expf( x ); }
static inline double __cwasm_tg_exp_d( double x ) { return exp( x ); }
static inline float __cwasm_tg_log_f( float x ) { return logf( x ); }
static inline double __cwasm_tg_log_d( double x ) { return log( x ); }
static inline float __cwasm_tg_log10_f( float x ) { return log10f( x ); }
static inline double __cwasm_tg_log10_d( double x ) { return log10( x ); }
static inline float __cwasm_tg_log2_f( float x ) { return log2f( x ); }
static inline double __cwasm_tg_log2_d( double x ) { return log2( x ); }
static inline float __cwasm_tg_exp2_f( float x ) { return exp2f( x ); }
static inline double __cwasm_tg_exp2_d( double x ) { return exp2( x ); }
static inline float __cwasm_tg_sqrt_f( float x ) { return sqrtf( x ); }
static inline double __cwasm_tg_sqrt_d( double x ) { return sqrt( x ); }
static inline float __cwasm_tg_fabs_f( float x ) { return fabsf( x ); }
static inline double __cwasm_tg_fabs_d( double x ) { return fabs( x ); }
static inline float __cwasm_tg_floor_f( float x ) { return floorf( x ); }
static inline double __cwasm_tg_floor_d( double x ) { return floor( x ); }
static inline float __cwasm_tg_ceil_f( float x ) { return ceilf( x ); }
static inline double __cwasm_tg_ceil_d( double x ) { return ceil( x ); }
static inline float __cwasm_tg_trunc_f( float x ) { return truncf( x ); }
static inline double __cwasm_tg_trunc_d( double x ) { return trunc( x ); }
static inline float __cwasm_tg_round_f( float x ) { return roundf( x ); }
static inline double __cwasm_tg_round_d( double x ) { return round( x ); }

static inline float __cwasm_tg_atan2_f( float y, float x ) { return atan2f( y, x ); }
static inline double __cwasm_tg_atan2_d( double y, double x ) { return atan2( y, x ); }
static inline float __cwasm_tg_pow_f( float x, float y ) { return powf( x, y ); }
static inline double __cwasm_tg_pow_d( double x, double y ) { return pow( x, y ); }
static inline float __cwasm_tg_fmod_f( float x, float y ) { return fmodf( x, y ); }
static inline double __cwasm_tg_fmod_d( double x, double y ) { return fmod( x, y ); }
static inline float __cwasm_tg_hypot_f( float x, float y ) { return hypotf( x, y ); }
static inline double __cwasm_tg_hypot_d( double x, double y ) { return hypot( x, y ); }
static inline float __cwasm_tg_copysign_f( float x, float y ) { return copysignf( x, y ); }
static inline double __cwasm_tg_copysign_d( double x, double y ) { return copysign( x, y ); }
static inline float __cwasm_tg_fmin_f( float x, float y ) { return fminf( x, y ); }
static inline double __cwasm_tg_fmin_d( double x, double y ) { return fmin( x, y ); }
static inline float __cwasm_tg_fmax_f( float x, float y ) { return fmaxf( x, y ); }
static inline double __cwasm_tg_fmax_d( double x, double y ) { return fmax( x, y ); }

static inline float __cwasm_tg_cosh_f( float x ) { return coshf( x ); }
static inline double __cwasm_tg_cosh_d( double x ) { return cosh( x ); }
static inline float __cwasm_tg_sinh_f( float x ) { return sinhf( x ); }
static inline double __cwasm_tg_sinh_d( double x ) { return sinh( x ); }
static inline float __cwasm_tg_tanh_f( float x ) { return tanhf( x ); }
static inline double __cwasm_tg_tanh_d( double x ) { return tanh( x ); }
static inline float __cwasm_tg_acosh_f( float x ) { return acoshf( x ); }
static inline double __cwasm_tg_acosh_d( double x ) { return acosh( x ); }
static inline float __cwasm_tg_asinh_f( float x ) { return asinhf( x ); }
static inline double __cwasm_tg_asinh_d( double x ) { return asinh( x ); }
static inline float __cwasm_tg_atanh_f( float x ) { return atanhf( x ); }
static inline double __cwasm_tg_atanh_d( double x ) { return atanh( x ); }
static inline float __cwasm_tg_cbrt_f( float x ) { return cbrtf( x ); }
static inline double __cwasm_tg_cbrt_d( double x ) { return cbrt( x ); }
static inline float __cwasm_tg_erf_f( float x ) { return erff( x ); }
static inline double __cwasm_tg_erf_d( double x ) { return erf( x ); }
static inline float __cwasm_tg_erfc_f( float x ) { return erfcf( x ); }
static inline double __cwasm_tg_erfc_d( double x ) { return erfc( x ); }
static inline float __cwasm_tg_expm1_f( float x ) { return expm1f( x ); }
static inline double __cwasm_tg_expm1_d( double x ) { return expm1( x ); }
static inline float __cwasm_tg_log1p_f( float x ) { return log1pf( x ); }
static inline double __cwasm_tg_log1p_d( double x ) { return log1p( x ); }
static inline float __cwasm_tg_logb_f( float x ) { return logbf( x ); }
static inline double __cwasm_tg_logb_d( double x ) { return logb( x ); }
static inline float __cwasm_tg_lgamma_f( float x ) { return lgammaf( x ); }
static inline double __cwasm_tg_lgamma_d( double x ) { return lgamma( x ); }
static inline float __cwasm_tg_tgamma_f( float x ) { return tgammaf( x ); }
static inline double __cwasm_tg_tgamma_d( double x ) { return tgamma( x ); }
static inline float __cwasm_tg_nearbyint_f( float x ) { return nearbyintf( x ); }
static inline double __cwasm_tg_nearbyint_d( double x ) { return nearbyint( x ); }
static inline float __cwasm_tg_rint_f( float x ) { return rintf( x ); }
static inline double __cwasm_tg_rint_d( double x ) { return rint( x ); }
static inline long __cwasm_tg_lrint_f( float x ) { return lrintf( x ); }
static inline long __cwasm_tg_lrint_d( double x ) { return lrint( x ); }
static inline long long __cwasm_tg_llrint_f( float x ) { return llrintf( x ); }
static inline long long __cwasm_tg_llrint_d( double x ) { return llrint( x ); }
static inline long __cwasm_tg_lround_f( float x ) { return lroundf( x ); }
static inline long __cwasm_tg_lround_d( double x ) { return lround( x ); }
static inline long long __cwasm_tg_llround_f( float x ) { return llroundf( x ); }
static inline long long __cwasm_tg_llround_d( double x ) { return llround( x ); }
static inline int __cwasm_tg_ilogb_f( float x ) { return ilogbf( x ); }
static inline int __cwasm_tg_ilogb_d( double x ) { return ilogb( x ); }
static inline float __cwasm_tg_fdim_f( float x, float y ) { return fdimf( x, y ); }
static inline double __cwasm_tg_fdim_d( double x, double y ) { return fdim( x, y ); }
static inline float __cwasm_tg_remainder_f( float x, float y ) { return remainderf( x, y ); }
static inline double __cwasm_tg_remainder_d( double x, double y ) { return remainder( x, y ); }
static inline float __cwasm_tg_nextafter_f( float x, float y ) { return nextafterf( x, y ); }
static inline double __cwasm_tg_nextafter_d( double x, double y ) { return nextafter( x, y ); }
static inline float __cwasm_tg_fma_f( float x, float y, float z ) { return fmaf( x, y, z ); }
static inline double __cwasm_tg_fma_d( double x, double y, double z ) { return fma( x, y, z ); }
static inline float __cwasm_tg_remquo_f( float x, float y, int* q ) { return remquof( x, y, q ); }
static inline double __cwasm_tg_remquo_d( double x, double y, int* q ) { return remquo( x, y, q ); }
static inline float __cwasm_tg_frexp_f( float x, int* e ) { return frexpf( x, e ); }
static inline double __cwasm_tg_frexp_d( double x, int* e ) { return frexp( x, e ); }
static inline float __cwasm_tg_ldexp_f( float x, int n ) { return ldexpf( x, n ); }
static inline double __cwasm_tg_ldexp_d( double x, int n ) { return ldexp( x, n ); }
static inline float __cwasm_tg_scalbn_f( float x, int n ) { return scalbnf( x, n ); }
static inline double __cwasm_tg_scalbn_d( double x, int n ) { return scalbn( x, n ); }
static inline float __cwasm_tg_scalbln_f( float x, long n ) { return scalblnf( x, n ); }
static inline double __cwasm_tg_scalbln_d( double x, long n ) { return scalbln( x, n ); }
static inline float __cwasm_tg_nexttoward_f( float x, long double y ) { return nexttowardf( x, y ); }
static inline double __cwasm_tg_nexttoward_d( double x, long double y ) { return nexttoward( x, y ); }

#define __CWASM_TGMATH_1R(f, d, l, x) \
    _Generic((x), float: (f), long double: (l), default: (d))(x)
#define __CWASM_TGMATH_2R(f, d, l, x, y) \
    _Generic(((x) + (y)), float: (f), long double: (l), default: (d))((x), (y))
#define __CWASM_TGMATH_1C(f, d, l, cf, cd, cl, x) \
    _Generic((x), \
        float: (f), long double: (l), \
        float complex: (cf), double complex: (cd), long double complex: (cl), \
        default: (d))(x)
#define __CWASM_TGMATH_2C(f, d, l, cf, cd, cl, x, y) \
    _Generic(((x) + (y)), \
        float: (f), long double: (l), \
        float complex: (cf), double complex: (cd), long double complex: (cl), \
        default: (d))((x), (y))

#define __CWASM_TGMATH_1R_A(f, d, l, x, a) \
    _Generic((x), float: (f), long double: (l), default: (d))((x), (a))
#define __CWASM_TGMATH_2R_A(f, d, l, x, y, a) \
    _Generic(((x) + (y)), float: (f), long double: (l), default: (d))((x), (y), (a))
#define __CWASM_TGMATH_3R(f, d, l, x, y, z) \
    _Generic(((x) + (y) + (z)), float: (f), long double: (l), default: (d))((x), (y), (z))

#define sin(x) __CWASM_TGMATH_1C(__cwasm_tg_sin_f, __cwasm_tg_sin_d, sinl, csinf, csin, csinl, (x))
#define cos(x) __CWASM_TGMATH_1C(__cwasm_tg_cos_f, __cwasm_tg_cos_d, cosl, ccosf, ccos, ccosl, (x))
#define tan(x) __CWASM_TGMATH_1C(__cwasm_tg_tan_f, __cwasm_tg_tan_d, tanl, ctanf, ctan, ctanl, (x))
#define asin(x) __CWASM_TGMATH_1C(__cwasm_tg_asin_f, __cwasm_tg_asin_d, asinl, casinf, casin, casinl, (x))
#define acos(x) __CWASM_TGMATH_1C(__cwasm_tg_acos_f, __cwasm_tg_acos_d, acosl, cacosf, cacos, cacosl, (x))
#define atan(x) __CWASM_TGMATH_1C(__cwasm_tg_atan_f, __cwasm_tg_atan_d, atanl, catanf, catan, catanl, (x))
#define exp(x) __CWASM_TGMATH_1C(__cwasm_tg_exp_f, __cwasm_tg_exp_d, expl, cexpf, cexp, cexpl, (x))
#define log(x) __CWASM_TGMATH_1C(__cwasm_tg_log_f, __cwasm_tg_log_d, logl, clogf, clog, clogl, (x))
#define sqrt(x) __CWASM_TGMATH_1C(__cwasm_tg_sqrt_f, __cwasm_tg_sqrt_d, sqrtl, csqrtf, csqrt, csqrtl, (x))
#define fabs(x) __CWASM_TGMATH_1C(__cwasm_tg_fabs_f, __cwasm_tg_fabs_d, fabsl, cabsf, cabs, cabsl, (x))

#define log10(x) __CWASM_TGMATH_1R(__cwasm_tg_log10_f, __cwasm_tg_log10_d, log10l, (x))
#define log2(x) __CWASM_TGMATH_1R(__cwasm_tg_log2_f, __cwasm_tg_log2_d, log2l, (x))
#define exp2(x) __CWASM_TGMATH_1R(__cwasm_tg_exp2_f, __cwasm_tg_exp2_d, exp2l, (x))
#define floor(x) __CWASM_TGMATH_1R(__cwasm_tg_floor_f, __cwasm_tg_floor_d, floorl, (x))
#define ceil(x) __CWASM_TGMATH_1R(__cwasm_tg_ceil_f, __cwasm_tg_ceil_d, ceill, (x))
#define trunc(x) __CWASM_TGMATH_1R(__cwasm_tg_trunc_f, __cwasm_tg_trunc_d, truncl, (x))
#define round(x) __CWASM_TGMATH_1R(__cwasm_tg_round_f, __cwasm_tg_round_d, roundl, (x))

#define pow(x, y) __CWASM_TGMATH_2C(__cwasm_tg_pow_f, __cwasm_tg_pow_d, powl, cpowf, cpow, cpowl, (x), (y))
#define atan2(x, y) __CWASM_TGMATH_2R(__cwasm_tg_atan2_f, __cwasm_tg_atan2_d, atan2l, (x), (y))
#define fmod(x, y) __CWASM_TGMATH_2R(__cwasm_tg_fmod_f, __cwasm_tg_fmod_d, fmodl, (x), (y))
#define hypot(x, y) __CWASM_TGMATH_2R(__cwasm_tg_hypot_f, __cwasm_tg_hypot_d, hypotl, (x), (y))
#define copysign(x, y) __CWASM_TGMATH_2R(__cwasm_tg_copysign_f, __cwasm_tg_copysign_d, copysignl, (x), (y))
#define fmin(x, y) __CWASM_TGMATH_2R(__cwasm_tg_fmin_f, __cwasm_tg_fmin_d, fminl, (x), (y))
#define fmax(x, y) __CWASM_TGMATH_2R(__cwasm_tg_fmax_f, __cwasm_tg_fmax_d, fmaxl, (x), (y))

#define cosh(x) __CWASM_TGMATH_1C(__cwasm_tg_cosh_f, __cwasm_tg_cosh_d, coshl, ccoshf, ccosh, ccoshl, (x))
#define sinh(x) __CWASM_TGMATH_1C(__cwasm_tg_sinh_f, __cwasm_tg_sinh_d, sinhl, csinhf, csinh, csinhl, (x))
#define tanh(x) __CWASM_TGMATH_1C(__cwasm_tg_tanh_f, __cwasm_tg_tanh_d, tanhl, ctanhf, ctanh, ctanhl, (x))
#define acosh(x) __CWASM_TGMATH_1C(__cwasm_tg_acosh_f, __cwasm_tg_acosh_d, acoshl, cacoshf, cacosh, cacoshl, (x))
#define asinh(x) __CWASM_TGMATH_1C(__cwasm_tg_asinh_f, __cwasm_tg_asinh_d, asinhl, casinhf, casinh, casinhl, (x))
#define atanh(x) __CWASM_TGMATH_1C(__cwasm_tg_atanh_f, __cwasm_tg_atanh_d, atanhl, catanhf, catanh, catanhl, (x))
#define cbrt(x) __CWASM_TGMATH_1R(__cwasm_tg_cbrt_f, __cwasm_tg_cbrt_d, cbrtl, (x))
#define erf(x) __CWASM_TGMATH_1R(__cwasm_tg_erf_f, __cwasm_tg_erf_d, erfl, (x))
#define erfc(x) __CWASM_TGMATH_1R(__cwasm_tg_erfc_f, __cwasm_tg_erfc_d, erfcl, (x))
#define expm1(x) __CWASM_TGMATH_1R(__cwasm_tg_expm1_f, __cwasm_tg_expm1_d, expm1l, (x))
#define log1p(x) __CWASM_TGMATH_1R(__cwasm_tg_log1p_f, __cwasm_tg_log1p_d, log1pl, (x))
#define logb(x) __CWASM_TGMATH_1R(__cwasm_tg_logb_f, __cwasm_tg_logb_d, logbl, (x))
#define lgamma(x) __CWASM_TGMATH_1R(__cwasm_tg_lgamma_f, __cwasm_tg_lgamma_d, lgammal, (x))
#define tgamma(x) __CWASM_TGMATH_1R(__cwasm_tg_tgamma_f, __cwasm_tg_tgamma_d, tgammal, (x))
#define nearbyint(x) __CWASM_TGMATH_1R(__cwasm_tg_nearbyint_f, __cwasm_tg_nearbyint_d, nearbyintl, (x))
#define rint(x) __CWASM_TGMATH_1R(__cwasm_tg_rint_f, __cwasm_tg_rint_d, rintl, (x))
#define lrint(x) __CWASM_TGMATH_1R(__cwasm_tg_lrint_f, __cwasm_tg_lrint_d, lrintl, (x))
#define llrint(x) __CWASM_TGMATH_1R(__cwasm_tg_llrint_f, __cwasm_tg_llrint_d, llrintl, (x))
#define lround(x) __CWASM_TGMATH_1R(__cwasm_tg_lround_f, __cwasm_tg_lround_d, lroundl, (x))
#define llround(x) __CWASM_TGMATH_1R(__cwasm_tg_llround_f, __cwasm_tg_llround_d, llroundl, (x))
#define ilogb(x) __CWASM_TGMATH_1R(__cwasm_tg_ilogb_f, __cwasm_tg_ilogb_d, ilogbl, (x))
#define fdim(x, y) __CWASM_TGMATH_2R(__cwasm_tg_fdim_f, __cwasm_tg_fdim_d, fdiml, (x), (y))
#define remainder(x, y) __CWASM_TGMATH_2R(__cwasm_tg_remainder_f, __cwasm_tg_remainder_d, remainderl, (x), (y))
#define nextafter(x, y) __CWASM_TGMATH_2R(__cwasm_tg_nextafter_f, __cwasm_tg_nextafter_d, nextafterl, (x), (y))
#define fma(x, y, z) __CWASM_TGMATH_3R(__cwasm_tg_fma_f, __cwasm_tg_fma_d, fmal, (x), (y), (z))
#define remquo(x, y, q) __CWASM_TGMATH_2R_A(__cwasm_tg_remquo_f, __cwasm_tg_remquo_d, remquol, (x), (y), (q))
#define frexp(x, e) __CWASM_TGMATH_1R_A(__cwasm_tg_frexp_f, __cwasm_tg_frexp_d, frexpl, (x), (e))
#define ldexp(x, n) __CWASM_TGMATH_1R_A(__cwasm_tg_ldexp_f, __cwasm_tg_ldexp_d, ldexpl, (x), (n))
#define scalbn(x, n) __CWASM_TGMATH_1R_A(__cwasm_tg_scalbn_f, __cwasm_tg_scalbn_d, scalbnl, (x), (n))
#define scalbln(x, n) __CWASM_TGMATH_1R_A(__cwasm_tg_scalbln_f, __cwasm_tg_scalbln_d, scalblnl, (x), (n))
#define nexttoward(x, y) __CWASM_TGMATH_1R_A(__cwasm_tg_nexttoward_f, __cwasm_tg_nexttoward_d, nexttowardl, (x), (y))

#endif

#endif // __CWASM_TGMATH_H__
