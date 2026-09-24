// asyncify setjmp-longjmp runtime (-enable-emscripten-sjlj).

#include <stdint.h>
#include <setjmp.h>

#ifdef CWASM_THREADS
    _Thread_local uintptr_t __THREW__;
    _Thread_local int __threwValue;
    static _Thread_local int temp_ret0;
#else
    uintptr_t __THREW__;
    int __threwValue;
    static int temp_ret0;
#endif

void setThrew( uintptr_t threw, int value ) {
    if( __THREW__ == 0 ) {
        __THREW__ = threw;
        __threwValue = value;
    }
}

void setTempRet0( int value ) {
    temp_ret0 = value;
}

int getTempRet0( void ) {
    return temp_ret0;
}

void __wasm_setjmp( void* env, uint32_t label, void* func_invocation_id ) {
    struct __jmp_buf_tag* jb = env;
    jb->func_invocation_id = func_invocation_id;
    jb->label = label;
}

uint32_t __wasm_setjmp_test( void* env, void* func_invocation_id ) {
    struct __jmp_buf_tag* jb = env;
    if( jb->func_invocation_id == func_invocation_id ) { return jb->label; }
    return 0;
}

__attribute__( ( import_module( "env" ), import_name( "_emscripten_throw_longjmp" ) ) )
__attribute__( ( __noreturn__ ) )
void _emscripten_throw_longjmp( void );

__attribute__( ( __noreturn__ ) ) void emscripten_longjmp( uintptr_t env, int val ) {
    if( val == 0 ) { val = 1; }
    setThrew( env, val );
    _emscripten_throw_longjmp();
}
