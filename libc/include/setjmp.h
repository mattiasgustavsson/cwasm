#ifndef __CWASM_SETJMP_H__
#define __CWASM_SETJMP_H__
#include <stdint.h>
#include <stdnoreturn.h>

struct __jmp_buf_tag {
    void* func_invocation_id;
    uint32_t label;
    struct {
        void* env;
        int val;
    } _cwasm_lj_arg;
};

typedef struct __jmp_buf_tag jmp_buf[ 1 ];

#ifdef __cplusplus
    extern "C" {
#endif

int setjmp( jmp_buf env );
#if defined(__cplusplus)
    [[noreturn]] void longjmp( jmp_buf env, int val );
#else
    _Noreturn void longjmp( jmp_buf env, int val );
#endif

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_SETJMP_H__
