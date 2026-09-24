#ifndef __CWASM_ASSERT_H__
#define __CWASM_ASSERT_H__
#include <stdnoreturn.h>

#ifdef __cplusplus
    extern "C" {
#endif

#if defined(__cplusplus)
    [[noreturn]] void __cwasm_assert_fail( char const* expr, char const* file, int line );
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    _Noreturn void __cwasm_assert_fail( char const* expr, char const* file, int line );
#else
    __attribute__( ( __noreturn__ ) ) void __cwasm_assert_fail( char const* expr, char const* file, int line );
#endif

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_ASSERT_H__
#undef assert
#ifdef NDEBUG
    #define assert(x) ((void)0)
#else
    #define assert(x) ((x) ? (void)0 : __cwasm_assert_fail(#x, __FILE__, __LINE__))
#endif

#if !defined(__cplusplus) && (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L)
    #undef static_assert
    #define static_assert _Static_assert
#endif // __CWASM_ASSERT_H__
