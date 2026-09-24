#ifndef __CWASM_STDNORETURN_H__
#define __CWASM_STDNORETURN_H__
// C11 noreturn keyword only. Don't #define noreturn in C++ - it breaks
// __attribute__((noreturn)) in libc++/libcxxabi (expands to _Noreturn)
#if !defined(__cplusplus) && !defined(noreturn)
    #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
        #define noreturn _Noreturn
    #endif
#endif

#endif // __CWASM_STDNORETURN_H__
