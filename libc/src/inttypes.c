#include <inttypes.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <wchar.h>

intmax_t imaxabs( intmax_t j ) {
    if( j < 0 ) { return (intmax_t)( 0ull - (uintmax_t)j ); }
    return j;
}

imaxdiv_t imaxdiv( intmax_t numer, intmax_t denom ) {
    imaxdiv_t r;

    if( denom == 0 ) {
        errno = EDOM;
        r.quot = 0;
        r.rem = numer;
        return r;
    }
    r.quot = numer / denom;
    r.rem = numer - r.quot * denom;
    return r;
}

intmax_t strtoimax( char const* nptr, char** endptr, int base ) {
    return (intmax_t)strtoll( nptr, endptr, base );
}

uintmax_t strtoumax( char const* nptr, char** endptr, int base ) {
    return (uintmax_t)strtoull( nptr, endptr, base );
}

intmax_t wcstoimax( wchar_t const* nptr, wchar_t** endptr, int base ) {
    return (intmax_t)wcstoll( nptr, endptr, base );
}

uintmax_t wcstoumax( wchar_t const* nptr, wchar_t** endptr, int base ) {
    return (uintmax_t)wcstoull( nptr, endptr, base );
}
