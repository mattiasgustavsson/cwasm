#ifndef __CWASM_WCHAR_H__
#define __CWASM_WCHAR_H__
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#ifdef __cplusplus
    #ifndef restrict
        #define restrict __restrict
    #endif
#endif

typedef char cwasm_wchar_must_be_32bit[( sizeof( wchar_t ) == 4 ) ? 1 : -1 ];

#ifdef __WINT_TYPE__
    typedef __WINT_TYPE__ wint_t;
#else
    typedef int wint_t;
#endif
#define WEOF ((wint_t)-1)

typedef struct { unsigned char __state[ 16 ]; } mbstate_t;

#ifdef __cplusplus
    extern "C" {
#endif

int mbsinit( mbstate_t const* ps );
size_t mbrtowc( wchar_t* restrict pwc, char const* restrict s, size_t n, mbstate_t* restrict ps );
size_t wcrtomb( char* restrict s, wchar_t wc, mbstate_t* restrict ps );
size_t mbsrtowcs( wchar_t* restrict dst, char const** restrict src, size_t len, mbstate_t* restrict ps );
size_t wcsrtombs( char* restrict dst, wchar_t const** restrict src, size_t len, mbstate_t* restrict ps );
size_t wcsnrtombs( char* restrict dst, wchar_t const** restrict src, size_t nwc, size_t len, mbstate_t* restrict ps );
size_t mbsnrtowcs( wchar_t* restrict dst, char const** restrict src, size_t nms, size_t len, mbstate_t* restrict ps );
size_t mbrlen( char const* restrict s, size_t n, mbstate_t* restrict ps );
int mbtowc( wchar_t* restrict pwc, char const* restrict s, size_t n );
int mblen( char const* s, size_t n );
int wctomb( char* s, wchar_t wc );
size_t mbstowcs( wchar_t* restrict pwcs, char const* restrict s, size_t n );
size_t wcstombs( char* restrict s, wchar_t const* restrict pwcs, size_t n );
int wcscoll( wchar_t const* s1, wchar_t const* s2 );
size_t wcsxfrm( wchar_t* restrict dest, wchar_t const* restrict src, size_t n );
size_t wcsftime( wchar_t* restrict wcs, size_t maxsize, wchar_t const* restrict format, struct tm const* restrict timeptr );

wint_t btowc( int c );
int wctob( wint_t c );

size_t wcslen( wchar_t const* s );
wchar_t* wcscpy( wchar_t* dst, wchar_t const* src );
wchar_t* wcsncpy( wchar_t* dst, wchar_t const* src, size_t n );
wchar_t* wcsdup( wchar_t const* s );
wchar_t* wcscat( wchar_t* dst, wchar_t const* src );
wchar_t* wcsncat( wchar_t* dst, wchar_t const* src, size_t n );
size_t wcsnlen( wchar_t const* s, size_t maxlen );
int wcscmp( wchar_t const* a, wchar_t const* b );
int wcsncmp( wchar_t const* a, wchar_t const* b, size_t n );
wchar_t* wcschr( wchar_t const* s, wchar_t c );
wchar_t* wcsrchr( wchar_t const* s, wchar_t c );
wchar_t* wcsstr( wchar_t const* haystack, wchar_t const* needle );
wchar_t* wcspbrk( wchar_t const* s, wchar_t const* accept );
wchar_t* wcstok( wchar_t* restrict s, wchar_t const* restrict delim, wchar_t** restrict ptr );
size_t wcsspn( wchar_t const* s, wchar_t const* accept );
size_t wcscspn( wchar_t const* s, wchar_t const* reject );
int wmemcmp( wchar_t const* a, wchar_t const* b, size_t n );
wchar_t* wmemchr( wchar_t const* s, wchar_t c, size_t n );
wchar_t* wmemcpy( wchar_t* restrict dst, wchar_t const* restrict src, size_t n );
wchar_t* wmemmove( wchar_t* dst, wchar_t const* src, size_t n );
wchar_t* wmemset( wchar_t* s, wchar_t c, size_t n );

long wcstol( wchar_t const* restrict nptr, wchar_t** restrict endptr, int base );
unsigned long wcstoul( wchar_t const* restrict nptr, wchar_t** restrict endptr, int base );
long long wcstoll( wchar_t const* restrict nptr, wchar_t** restrict endptr, int base );
unsigned long long wcstoull( wchar_t const* restrict nptr, wchar_t** restrict endptr, int base );
double wcstod( wchar_t const* restrict nptr, wchar_t** restrict endptr );
float wcstof( wchar_t const* restrict nptr, wchar_t** restrict endptr );
long double wcstold( wchar_t const* restrict nptr, wchar_t** restrict endptr );

int wcwidth( wint_t wc );
int wcswidth( wchar_t const* s, size_t n );

wint_t fgetwc( FILE* stream );
wint_t getwchar( void );
wint_t ungetwc( wint_t wc, FILE* stream );
wint_t fputwc( wchar_t wc, FILE* stream );
wint_t putwchar( wchar_t wc );

#define getwc(stream) fgetwc(stream)
#define putwc(wc, stream) fputwc((wc), (stream))
wchar_t* fgetws( wchar_t* restrict s, int n, FILE* restrict stream );
int fputws( wchar_t const* restrict s, FILE* restrict stream );

int fwide( FILE* stream, int mode );

int fwprintf( FILE* restrict stream, wchar_t const* restrict format, ... );
int swprintf( wchar_t* restrict s, size_t n, wchar_t const* restrict format, ... );
int wprintf( wchar_t const* restrict format, ... );
int vfwprintf( FILE* restrict stream, wchar_t const* restrict format, va_list ap );
int vswprintf( wchar_t* restrict s, size_t n, wchar_t const* restrict format, va_list ap );
int vwprintf( wchar_t const* restrict format, va_list ap );

int fwscanf( FILE* restrict stream, wchar_t const* restrict format, ... );
int swscanf( wchar_t const* restrict s, wchar_t const* restrict format, ... );
int wscanf( wchar_t const* restrict format, ... );
int vfwscanf( FILE* restrict stream, wchar_t const* restrict format, va_list ap );
int vswscanf( wchar_t const* restrict s, wchar_t const* restrict format, va_list ap );
int vwscanf( wchar_t const* restrict format, va_list ap );

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_WCHAR_H__
