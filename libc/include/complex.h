#ifndef __CWASM_COMPLEX_H__
#define __CWASM_COMPLEX_H__
#include <math.h>

#define complex _Complex

#define _Complex_I ((float const _Complex)__builtin_complex(0.0f, 1.0f))
#define I _Complex_I

#define CMPLX(x, y) ((double complex){(double)(x), (double)(y)})
#define CMPLXF(x, y) ((float complex){(float)(x), (float)(y)})
#define CMPLXL(x, y) ((long double complex){(long double)(x), (long double)(y)})

#ifdef __cplusplus
    extern "C" {
#endif

float complex cacosf( float complex z );
double complex cacos( double complex z );
long double complex cacosl( long double complex z );

float complex cacoshf( float complex z );
double complex cacosh( double complex z );
long double complex cacoshl( long double complex z );

float complex casinf( float complex z );
double complex casin( double complex z );
long double complex casinl( long double complex z );

float complex casinhf( float complex z );
double complex casinh( double complex z );
long double complex casinhl( long double complex z );

float complex catanf( float complex z );
double complex catan( double complex z );
long double complex catanl( long double complex z );

float complex catanhf( float complex z );
double complex catanh( double complex z );
long double complex catanhl( long double complex z );

float complex ccosf( float complex z );
double complex ccos( double complex z );
long double complex ccosl( long double complex z );

float complex ccoshf( float complex z );
double complex ccosh( double complex z );
long double complex ccoshl( long double complex z );

float complex cexpf( float complex z );
double complex cexp( double complex z );
long double complex cexpl( long double complex z );

float complex clogf( float complex z );
double complex clog( double complex z );
long double complex clogl( long double complex z );

float complex cpowf( float complex x, float complex y );
double complex cpow( double complex x, double complex y );
long double complex cpowl( long double complex x, long double complex y );

float complex csinf( float complex z );
double complex csin( double complex z );
long double complex csinl( long double complex z );

float complex csinhf( float complex z );
double complex csinh( double complex z );
long double complex csinhl( long double complex z );

float complex csqrtf( float complex z );
double complex csqrt( double complex z );
long double complex csqrtl( long double complex z );

float complex ctanf( float complex z );
double complex ctan( double complex z );
long double complex ctanl( long double complex z );

float complex ctanhf( float complex z );
double complex ctanh( double complex z );
long double complex ctanhl( long double complex z );

float cabsf( float complex z );
double cabs( double complex z );
long double cabsl( long double complex z );

float cargf( float complex z );
double carg( double complex z );
long double cargl( long double complex z );

float complex conjf( float complex z );
double complex conj( double complex z );
long double complex conjl( long double complex z );

float complex cprojf( float complex z );
double complex cproj( double complex z );
long double complex cprojl( long double complex z );

float crealf( float complex z );
double creal( double complex z );
long double creall( long double complex z );

float cimagf( float complex z );
double cimag( double complex z );
long double cimagl( long double complex z );

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
    #define creal(z) \
    _Generic((z), \
        float complex: crealf, \
        double complex: creal, \
        long double complex: creall, \
        float: crealf, \
        long double: creall, \
        default: creal)(z)

    #define cimag(z) \
    _Generic((z), \
        float complex: cimagf, \
        double complex: cimag, \
        long double complex: cimagl, \
        float: cimagf, \
        long double: cimagl, \
        default: cimag)(z)

    #define conj(z) \
    _Generic((z), \
        float complex: conjf, \
        double complex: conj, \
        long double complex: conjl, \
        float: conjf, \
        long double: conjl, \
        default: conj)(z)

    #define cproj(z) \
    _Generic((z), \
        float complex: cprojf, \
        double complex: cproj, \
        long double complex: cprojl, \
        float: cprojf, \
        long double: cprojl, \
        default: cproj)(z)

    #define carg(z) \
    _Generic((z), \
        float complex: cargf, \
        double complex: carg, \
        long double complex: cargl, \
        float: cargf, \
        long double: cargl, \
        default: carg)(z)

    #define cabs(z) \
    _Generic((z), \
        float complex: cabsf, \
        double complex: cabs, \
        long double complex: cabsl, \
        float: cabsf, \
        long double: cabsl, \
        default: cabs)(z)
#endif

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_COMPLEX_H__
