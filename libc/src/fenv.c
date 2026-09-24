#include <fenv.h>

// wasm32 float instructions always use IEEE-754 round-to-nearest-even. The exception
// flags below are tracked in software for API compatibility; nothing raises them.

#ifdef CWASM_THREADS
    static _Thread_local unsigned int cwasm_fe_except;
#else
    static unsigned int cwasm_fe_except;
#endif

fenv_t const __fe_dfl_env = { 0u };

static unsigned int cwasm_fe_mask( int excepts ) {
    return (unsigned int)excepts & FE_ALL_EXCEPT;
}

unsigned int __cwasm_fenv_get_except( void ) {
    return cwasm_fe_except;
}

void __cwasm_fenv_set_except( unsigned int excepts ) {
    cwasm_fe_except = excepts & FE_ALL_EXCEPT;
}

int feclearexcept( int excepts ) {
    cwasm_fe_except &= ~cwasm_fe_mask( excepts );
    return 0;
}

int fegetexceptflag( fexcept_t* flagp, int excepts ) {
    if( flagp ) { *flagp = cwasm_fe_except & cwasm_fe_mask( excepts ); }
    return 0;
}

int feraiseexcept( int excepts ) {
    cwasm_fe_except |= cwasm_fe_mask( excepts );
    return 0;
}

int fesetexceptflag( fexcept_t const* flagp, int excepts ) {
    unsigned int mask = cwasm_fe_mask( excepts );
    if( !flagp ) { return 0; }
    cwasm_fe_except = ( cwasm_fe_except & ~mask ) | ( *flagp & mask );
    return 0;
}

int fetestexcept( int excepts ) {
    return (int)( cwasm_fe_except & cwasm_fe_mask( excepts ) );
}

int fegetround( void ) {
    return FE_TONEAREST;
}

int fesetround( int mode ) {
    return mode == FE_TONEAREST ? 0 : 1;
}

int fegetenv( fenv_t* envp ) {
    if( envp ) {
        envp->__opaque = (unsigned int)FE_TONEAREST | ( cwasm_fe_except << 8 );
    }
    return 0;
}

int feholdexcept( fenv_t* envp ) {
    if( envp ) {
        envp->__opaque = (unsigned int)FE_TONEAREST | ( cwasm_fe_except << 8 );
    }
    cwasm_fe_except = 0;
    return 0;
}

int fesetenv( fenv_t const* envp ) {
    if( !envp ) { return 0; }
    cwasm_fe_except = envp->__opaque >> 8;
    return 0;
}

int feupdateenv( fenv_t const* envp ) {
    if( !envp ) { return 0; }
    cwasm_fe_except |= envp->__opaque >> 8;
    return 0;
}
