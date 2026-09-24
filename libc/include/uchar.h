#ifndef __CWASM_UCHAR_H__
#define __CWASM_UCHAR_H__
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <wchar.h>

#ifdef __cplusplus
    #ifndef restrict
        #define restrict __restrict
    #endif
#endif

#ifndef __cplusplus
    typedef uint_least16_t char16_t;
    typedef uint_least32_t char32_t;
#endif


#ifdef __cplusplus
    extern "C" {
#endif

size_t mbrtoc32( char32_t* restrict pc32,
    char const* restrict s,
    size_t n,
    mbstate_t* restrict ps );

size_t c32rtomb( char* restrict s, char32_t c32, mbstate_t* restrict ps );

// mbrtoc16 may return (size_t)-3 when delivering a previously staged UTF-16 unit
size_t mbrtoc16( char16_t* restrict pc16, char const* restrict s, size_t n, mbstate_t* restrict ps );

size_t c16rtomb( char* restrict s, char16_t c16, mbstate_t* restrict ps );

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_UCHAR_H__
