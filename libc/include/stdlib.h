#ifndef __CWASM_STDLIB_H__
#define __CWASM_STDLIB_H__
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <locale.h>
#include <string.h>
#include <stdnoreturn.h>

#ifdef __cplusplus
    #ifndef restrict
        #define restrict __restrict
    #endif
#endif

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define RAND_MAX 0x7FFFFFFF

extern int __cwasm_mb_cur_max;
int __cwasm_mb_cur_max_get( void );
#define MB_CUR_MAX (__cwasm_mb_cur_max_get())

typedef struct { int quot; int rem; } div_t;
typedef struct { long quot; long rem; } ldiv_t;
#if __STDC_VERSION__ >= 199901L || defined(__cplusplus)
    typedef struct { long long quot; long long rem; } lldiv_t;
#endif

#ifdef __cplusplus
    extern "C" {
#endif

void* malloc( size_t size );
void* calloc( size_t nmemb, size_t size );
void* realloc( void* ptr, size_t size );
void free( void* ptr );

#if __STDC_VERSION__ >= 201112L || defined(__cplusplus)
    void* aligned_alloc( size_t alignment, size_t size );
#endif

#if defined(__cplusplus)
    [[noreturn]] void abort( void );
    [[noreturn]] void exit( int status );
    [[noreturn]] void _Exit( int status );
    [[noreturn]] void quick_exit( int status );
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    _Noreturn void abort( void );
    _Noreturn void exit( int status );
    _Noreturn void _Exit( int status );
    _Noreturn void quick_exit( int status );
#else
    __attribute__( ( __noreturn__ ) ) void abort( void );
    __attribute__( ( __noreturn__ ) ) void exit( int status );
    __attribute__( ( __noreturn__ ) ) void _Exit( int status );
    __attribute__( ( __noreturn__ ) ) void quick_exit( int status );
#endif
int atexit( void ( *func )( void ) );
int at_quick_exit( void ( *func )( void ) );

char* realpath( char const* path, char* resolved_path );

unsigned long long strtoull( char const* restrict nptr, char** restrict endptr, int base );
unsigned long strtoul( char const* restrict nptr, char** restrict endptr, int base );
long long strtoll( char const* restrict nptr, char** restrict endptr, int base );
long strtol( char const* restrict nptr, char** restrict endptr, int base );
int atoi( char const* nptr );
long atol( char const* nptr );
#if __STDC_VERSION__ >= 199901L || defined(__cplusplus)
    long long atoll( char const* nptr );
#endif

double strtod( char const* restrict nptr, char** restrict endptr );
float strtof( char const* restrict nptr, char** restrict endptr );
long double strtold( char const* restrict nptr, char** restrict endptr );
double atof( char const* nptr );

int rand( void );
void srand( unsigned int seed );
int rand_r( unsigned int* seed );

void* bsearch( void const* key, void const* base, size_t nmemb, size_t size, int ( *compar )( void const*, void const* ) );
void qsort( void* base, size_t nmemb, size_t size, int ( *compar )( void const*, void const* ) );

#undef abs
int abs( int j );
long labs( long j );
#if __STDC_VERSION__ >= 199901L || defined(__cplusplus)
    long long llabs( long long j );
#endif

div_t div( int numer, int denom );
ldiv_t ldiv( long numer, long denom );
#if __STDC_VERSION__ >= 199901L || defined(__cplusplus)
    lldiv_t lldiv( long long numer, long long denom );
#endif

char* getenv( char const* name );
int system( char const* command );

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_STDLIB_H__
