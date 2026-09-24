#ifndef __CWASM_STDDEF_H__
#define __CWASM_STDDEF_H__
typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;
#if !defined(__cplusplus)
    typedef __WCHAR_TYPE__ wchar_t;
#endif

#if __STDC_VERSION__ >= 201112L || defined(__cplusplus)
    typedef union { long long __ll; long double __ld; } max_align_t;
#endif

#ifndef NULL
    #if defined(__cplusplus)
        #define NULL nullptr
    #else
        #define NULL ((void*)0)
    #endif
#endif

#if defined(__cplusplus)
    #ifndef offsetof
        #define offsetof(type, member) __builtin_offsetof(type, member)
    #endif
#else
    #undef offsetof
    #define offsetof(type, member) __builtin_offsetof(type, member)
#endif

#endif // __CWASM_STDDEF_H__
