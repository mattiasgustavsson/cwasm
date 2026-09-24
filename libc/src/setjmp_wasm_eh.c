// wasm-EH / JSPI setjmp-longjmp runtime (-fwasm-exceptions -wasm-enable-sjlj)

#include <stdint.h>
#include <setjmp.h>

#define C_LONGJMP 1

__attribute__( ( __noreturn__ ) ) void __wasm_longjmp( void* env, int val ) {
    struct __jmp_buf_tag* jb = env;

    if( val == 0 ) { val = 1; }
    jb->_cwasm_lj_arg.env = env;
    jb->_cwasm_lj_arg.val = val;
    __builtin_wasm_throw( C_LONGJMP, &jb->_cwasm_lj_arg );
}
