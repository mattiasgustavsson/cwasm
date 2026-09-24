#include <complex.h>

#undef creal
#undef cimag
#undef conj
#undef cproj
#undef carg
#undef cabs


#define CWASM_CX(x, y) ((double complex){(x), (y)})

static int cwasm_cx_inf( double x ) {
    return isinf( x );
}

static double complex cwasm_clog_reim( double x, double y ) {
    return CWASM_CX( log( hypot( x, y ) ), atan2( y, x ) );
}

static void cwasm_csqrt_reim( double a, double b, double* out_re, double* out_im ) {
    double r;
    double u;
    double v;

    if( a == 0.0 && b == 0.0 ) {
        *out_re = 0.0;
        *out_im = b;
        return;
    }
    if( b == 0.0 ) {
        if( a >= 0.0 ) {
            *out_re = sqrt( a );
            *out_im = b;
        } else {
            *out_re = 0.0;
            *out_im = copysign( sqrt( -a ), b );
        }
        return;
    }
    r = hypot( a, b );
    if( a >= 0.0 ) {
        u = sqrt( 0.5 * ( r + a ) );
        v = 0.5 * b / u;
        *out_re = u;
        *out_im = v;
    } else {
        v = sqrt( 0.5 * ( r - a ) );
        *out_im = copysign( v, b );
        *out_re = fabs( b ) / ( 2.0 * v );
    }
}

static double complex cwasm_cexp_scaled( double x, double y ) {
    double t;
    double s;
    double c;
    int k = 0;

    if( x > 710.0 ) {
        k = (int)( x / M_LN2 + 0.5 );
        x -= (double)k * M_LN2;
        t = exp( x );
    } else if( x < -745.0 ) {
        k = (int)( x / M_LN2 - 0.5 );
        x -= (double)k * M_LN2;
        t = exp( x );
    } else {
        t = exp( x );
    }
    s = sin( y );
    c = cos( y );
    if( k != 0 ) { return CWASM_CX( ldexp( t * c, k ), ldexp( t * s, k ) ); }
    return CWASM_CX( t * c, t * s );
}

static float complex cwasm_cexp_scaled_f( float x, float y ) {
    double t;
    double s;
    double c;
    int k = 0;

    if( x > 88.0f ) {
        k = (int)( (double)x / M_LN2 + 0.5 );
        x -= (float)( (double)k * M_LN2 );
        t = expf( x );
    } else if( x < -103.0f ) {
        k = (int)( (double)x / M_LN2 - 0.5 );
        x -= (float)( (double)k * M_LN2 );
        t = expf( x );
    } else {
        t = expf( x );
    }
    s = sinf( y );
    c = cosf( y );
    if( k != 0 ) {
        return CMPLXF( (float)ldexp( t * c, k ), (float)ldexp( t * s, k ) );
    }
    return CMPLXF( (float)( t * c ), (float)( t * s ) );
}

static double cwasm_real_asinh( double x ) {
    double ax = fabs( x );

    if( isnan( x ) || isinf( x ) ) { return x; }
    if( ax >= 0x1p26 ) { return copysign( log( ax ) + M_LN2, x ); }
    return log( x + sqrt( x * x + 1.0 ) );
}

static double cwasm_real_atanh( double x ) {
    if( isnan( x ) ) { return x; }
    if( fabs( x ) >= 1.0 ) { return NAN; }
    return 0.5 * ( log( 1.0 + x ) - log( 1.0 - x ) );
}

static double complex cwasm_casinh_impl( double x, double y ) {
    double ax = fabs( x );
    double ay = fabs( y );
    double sr;
    double si;

    if( isnan( x ) || isnan( y ) ) { return CWASM_CX( x + y, x + y ); }
    if( isinf( x ) || isinf( y ) ) { return CWASM_CX( x, y ); }
    if( y == 0.0 ) { return CWASM_CX( cwasm_real_asinh( x ), y ); }

    if( ax > 0x1p260 || ay > 0x1p260 || ( ax > 1.0 && ay > 1.0 && ax * ay > 1e300 ) ) {
        double l = log( hypot( ax, ay ) ) + M_LN2;
        return CWASM_CX( copysign( l, x ), copysign( l, y ) );
    }

    cwasm_csqrt_reim( x * x - y * y + 1.0, 2.0 * x * y, &sr, &si );
    return cwasm_clog_reim( x + sr, y + si );
}

static double complex cwasm_catanh_impl( double x, double y ) {
    double ax = fabs( x );
    double ay = fabs( y );
    double num;
    double den;
    double rs;
    double is;

    if( isnan( x ) || isnan( y ) ) { return CWASM_CX( x + y, x + y ); }
    if( y == 0.0 && fabs( x ) <= 1.0 ) { return CWASM_CX( cwasm_real_atanh( x ), y ); }
    if( x == 0.0 ) { return CWASM_CX( x, atan( y ) ); }

    num = ( 1.0 + x ) * ( 1.0 + x ) + y * y;
    den = ( 1.0 - x ) * ( 1.0 - x ) + y * y;
    if( num <= 0.0 || den <= 0.0 ) { return CWASM_CX( NAN, NAN ); }

    if( ax > 1.0 && ay < 1e-150 ) { rs = log( num / den ) * 0.25; }
    else { rs = ( log( num ) - log( den ) ) * 0.25; }
    is = 0.5 * atan2( 2.0 * y, 1.0 - x * x - y * y );
    return CWASM_CX( rs, is );
}

double creal( double complex z ) { return __real__( z ); }
float crealf( float complex z ) { return __real__( z ); }
long double creall( long double complex z ) { return __real__( z ); }

double cimag( double complex z ) { return __imag__( z ); }
float cimagf( float complex z ) { return __imag__( z ); }
long double cimagl( long double complex z ) { return __imag__( z ); }

double complex conj( double complex z ) {
    return CWASM_CX( __real__( z ), -__imag__( z ) );
}

float complex conjf( float complex z ) {
    return CMPLXF( __real__( z ), -__imag__( z ) );
}

long double complex conjl( long double complex z ) {
    return CMPLXL( __real__( z ), -__imag__( z ) );
}

double cabs( double complex z ) {
    return hypot( __real__( z ), __imag__( z ) );
}

float cabsf( float complex z ) {
    return hypotf( __real__( z ), __imag__( z ) );
}

long double cabsl( long double complex z ) {
    return hypotl( __real__( z ), __imag__( z ) );
}

double carg( double complex z ) {
    return atan2( __imag__( z ), __real__( z ) );
}

float cargf( float complex z ) {
    return atan2f( __imag__( z ), __real__( z ) );
}

long double cargl( long double complex z ) {
    return atan2l( __imag__( z ), __real__( z ) );
}

double complex cproj( double complex z ) {
    double x = __real__( z );
    double y = __imag__( z );
    if( cwasm_cx_inf( x ) || cwasm_cx_inf( y ) ) {
        return CWASM_CX( INFINITY, copysign( 0.0, y ) );
    }
    return z;
}

float complex cprojf( float complex z ) {
    return ( float complex )cproj( ( double complex )z );
}

long double complex cprojl( long double complex z ) {
    return ( long double complex )cproj( ( double complex )z );
}

double complex cexp( double complex z ) {
    return cwasm_cexp_scaled( __real__( z ), __imag__( z ) );
}

float complex cexpf( float complex z ) {
    return cwasm_cexp_scaled_f( __real__( z ), __imag__( z ) );
}

long double complex cexpl( long double complex z ) {
    return ( long double complex )cexp( ( double complex )z );
}

double complex clog( double complex z ) {
    return CWASM_CX( log( cabs( z ) ), carg( z ) );
}

float complex clogf( float complex z ) {
    return CMPLXF( logf( cabsf( z ) ), cargf( z ) );
}

long double complex clogl( long double complex z ) {
    return ( long double complex )clog( ( double complex )z );
}

double complex csqrt( double complex z ) {
    double x = __real__( z );
    double y = __imag__( z );
    double r;
    double u;
    double v;

    if( x == 0.0 && y == 0.0 ) { return CWASM_CX( 0.0, y ); }
    r = hypot( x, y );
    if( x >= 0.0 ) {
        u = sqrt( 0.5 * ( r + x ) );
        if( y == 0.0 ) { v = y; }
        else { v = 0.5 * y / u; }
    } else {
        double t = sqrt( 0.5 * ( r - x ) );
        u = fabs( y ) / ( 2.0 * t );
        v = copysign( t, y );
    }
    return CWASM_CX( u, v );
}

float complex csqrtf( float complex z ) {
    float x = __real__( z );
    float y = __imag__( z );
    float r;
    float u;
    float v;

    if( x == 0.0f && y == 0.0f ) { return CMPLXF( 0.0f, y ); }
    r = hypotf( x, y );
    if( x >= 0.0f ) {
        u = sqrtf( 0.5f * ( r + x ) );
        if( y == 0.0f ) { v = y; }
        else { v = 0.5f * y / u; }
    } else {
        float t = sqrtf( 0.5f * ( r - x ) );
        u = fabsf( y ) / ( 2.0f * t );
        v = copysignf( t, y );
    }
    return CMPLXF( u, v );
}

long double complex csqrtl( long double complex z ) {
    return ( long double complex )csqrt( ( double complex )z );
}

double complex cpow( double complex x, double complex y ) {
    double xr = __real__( x );
    double xi = __imag__( x );

    if( xr == 0.0 && xi == 0.0 ) {
        if( __real__( y ) == 0.0 && __imag__( y ) == 0.0 ) {
            return CWASM_CX( NAN, NAN );
        }
        if( __real__( y ) > 0.0 && __imag__( y ) == 0.0 ) {
            return CWASM_CX( 0.0, 0.0 );
        }
    }
    return cexp( y * clog( x ) );
}

float complex cpowf( float complex x, float complex y ) {
    return ( float complex )cpow( ( double complex )x, ( double complex )y );
}

long double complex cpowl( long double complex x, long double complex y ) {
    return ( long double complex )cpow( ( double complex )x, ( double complex )y );
}

double complex csin( double complex z ) {
    double x = __real__( z );
    double y = __imag__( z );
    return CWASM_CX( sin( x ) * cosh( y ), cos( x ) * sinh( y ) );
}

float complex csinf( float complex z ) {
    float x = __real__( z );
    float y = __imag__( z );
    return CMPLXF( sinf( x ) * coshf( y ), cosf( x ) * sinhf( y ) );
}

long double complex csinl( long double complex z ) {
    return ( long double complex )csin( ( double complex )z );
}

double complex ccos( double complex z ) {
    double x = __real__( z );
    double y = __imag__( z );
    return CWASM_CX( cos( x ) * cosh( y ), -sin( x ) * sinh( y ) );
}

float complex ccosf( float complex z ) {
    float x = __real__( z );
    float y = __imag__( z );
    return CMPLXF( cosf( x ) * coshf( y ), -sinf( x ) * sinhf( y ) );
}

long double complex ccosl( long double complex z ) {
    return ( long double complex )ccos( ( double complex )z );
}

double complex ctan( double complex z ) {
    double x = __real__( z );
    double y = __imag__( z );
    double ay = fabs( y );

    if( ay > 20.0 ) {
        double e = exp( -2.0 * ay );
        return CWASM_CX( sin( 2.0 * x ) * e, copysign( 1.0, y ) );
    }

    double sin2x = sin( 2.0 * x );
    double cos2x = cos( 2.0 * x );
    double sinh2y = sinh( 2.0 * y );
    double cosh2y = cosh( 2.0 * y );
    double den = cos2x + cosh2y;

    if( den == 0.0 ) {
        return CWASM_CX( copysign( INFINITY, sin2x ), copysign( INFINITY, sinh2y ) );
    }
    return CWASM_CX( sin2x / den, sinh2y / den );
}

float complex ctanf( float complex z ) {
    float x = __real__( z );
    float y = __imag__( z );
    float ay = fabsf( y );

    if( ay > 20.0f ) {
        float e = expf( -2.0f * ay );
        return CMPLXF( sinf( 2.0f * x ) * e, copysignf( 1.0f, y ) );
    }

    float sin2x = sinf( 2.0f * x );
    float cos2x = cosf( 2.0f * x );
    float sinh2y = sinhf( 2.0f * y );
    float cosh2y = coshf( 2.0f * y );
    float den = cos2x + cosh2y;

    if( den == 0.0f ) {
        return CMPLXF( copysignf( INFINITY, sin2x ), copysignf( INFINITY, sinh2y ) );
    }
    return CMPLXF( sin2x / den, sinh2y / den );
}

long double complex ctanl( long double complex z ) {
    return ( long double complex )ctan( ( double complex )z );
}

double complex casin( double complex z ) {
    double x = __real__( z );
    double y = __imag__( z );
    double complex w;

    if( isnan( x ) || isnan( y ) ) { return CWASM_CX( x + y, x + y ); }
    if( y == 0.0 && fabs( x ) <= 1.0 ) { return CWASM_CX( asin( x ), y ); }

    w = cwasm_casinh_impl( -y, x );
    return CWASM_CX( cimag( w ), -creal( w ) );
}

float complex casinf( float complex z ) {
    return ( float complex )casin( ( double complex )z );
}

long double complex casinl( long double complex z ) {
    return ( long double complex )casin( ( double complex )z );
}

double complex cacos( double complex z ) {
    double complex s = casin( z );

    return CWASM_CX( M_PI_2 - creal( s ), -cimag( s ) );
}

float complex cacosf( float complex z ) {
    return ( float complex )cacos( ( double complex )z );
}

long double complex cacosl( long double complex z ) {
    return ( long double complex )cacos( ( double complex )z );
}

double complex catan( double complex z ) {
    double x = __real__( z );
    double y = __imag__( z );
    double x2 = x * x;
    double yp = y + 1.0;
    double ym = y - 1.0;

    return CWASM_CX( 0.5 * atan2( 2.0 * x, 1.0 - x2 - y * y ),
        0.25 * log( ( x2 + yp * yp ) / ( x2 + ym * ym ) ) );
}

float complex catanf( float complex z ) {
    return ( float complex )catan( ( double complex )z );
}

long double complex catanl( long double complex z ) {
    return ( long double complex )catan( ( double complex )z );
}

double complex csinh( double complex z ) {
    double x = __real__( z );
    double y = __imag__( z );
    return CWASM_CX( sinh( x ) * cos( y ), cosh( x ) * sin( y ) );
}

float complex csinhf( float complex z ) {
    float x = __real__( z );
    float y = __imag__( z );
    return CMPLXF( sinhf( x ) * cosf( y ), coshf( x ) * sinf( y ) );
}

long double complex csinhl( long double complex z ) {
    return ( long double complex )csinh( ( double complex )z );
}

double complex ccosh( double complex z ) {
    double x = __real__( z );
    double y = __imag__( z );
    return CWASM_CX( cosh( x ) * cos( y ), sinh( x ) * sin( y ) );
}

float complex ccoshf( float complex z ) {
    float x = __real__( z );
    float y = __imag__( z );
    return CMPLXF( coshf( x ) * cosf( y ), sinhf( x ) * sinf( y ) );
}

long double complex ccoshl( long double complex z ) {
    return ( long double complex )ccosh( ( double complex )z );
}

double complex ctanh( double complex z ) {
    double x = __real__( z );
    double y = __imag__( z );
    double ax = fabs( x );

    if( ax > 20.0 ) {
        double e = exp( -2.0 * ax );
        return CWASM_CX( copysign( 1.0, x ), sin( 2.0 * y ) * e );
    }

    double sinh2x = sinh( 2.0 * x );
    double cosh2x = cosh( 2.0 * x );
    double sin2y = sin( 2.0 * y );
    double cos2y = cos( 2.0 * y );
    double den = cosh2x + cos2y;

    if( den == 0.0 ) {
        return CWASM_CX( copysign( INFINITY, sinh2x ), copysign( INFINITY, sin2y ) );
    }
    return CWASM_CX( sinh2x / den, sin2y / den );
}

float complex ctanhf( float complex z ) {
    float x = __real__( z );
    float y = __imag__( z );
    float ax = fabsf( x );

    if( ax > 20.0f ) {
        float e = expf( -2.0f * ax );
        return CMPLXF( copysignf( 1.0f, x ), sinf( 2.0f * y ) * e );
    }

    float sinh2x = sinhf( 2.0f * x );
    float cosh2x = coshf( 2.0f * x );
    float sin2y = sinf( 2.0f * y );
    float cos2y = cosf( 2.0f * y );
    float den = cosh2x + cos2y;

    if( den == 0.0f ) {
        return CMPLXF( copysignf( INFINITY, sinh2x ), copysignf( INFINITY, sin2y ) );
    }
    return CMPLXF( sinh2x / den, sin2y / den );
}

long double complex ctanhl( long double complex z ) {
    return ( long double complex )ctanh( ( double complex )z );
}

double complex casinh( double complex z ) {
    return cwasm_casinh_impl( __real__( z ), __imag__( z ) );
}

float complex casinhf( float complex z ) {
    return ( float complex )casinh( ( double complex )z );
}

long double complex casinhl( long double complex z ) {
    return ( long double complex )casinh( ( double complex )z );
}

double complex cacosh( double complex z ) {
    double complex a = cacos( z );
    double complex w = CWASM_CX( -cimag( a ), creal( a ) );

    // Annex G: principal value has nonnegative real part
    if( creal( w ) < 0.0 ) { w = -w; }
    return w;
}

float complex cacoshf( float complex z ) {
    return ( float complex )cacosh( ( double complex )z );
}

long double complex cacoshl( long double complex z ) {
    return ( long double complex )cacosh( ( double complex )z );
}

double complex catanh( double complex z ) {
    return cwasm_catanh_impl( __real__( z ), __imag__( z ) );
}

float complex catanhf( float complex z ) {
    return ( float complex )catanh( ( double complex )z );
}

long double complex catanhl( long double complex z ) {
    return ( long double complex )catanh( ( double complex )z );
}
