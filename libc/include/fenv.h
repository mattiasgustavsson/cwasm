#ifndef __CWASM_FENV_H__
#define __CWASM_FENV_H__
#ifdef __cplusplus
    extern "C" {
#endif

typedef struct { unsigned int __opaque; } fenv_t;
typedef unsigned int fexcept_t;

#define FE_INVALID 0x01
#define FE_DIVBYZERO 0x02
#define FE_OVERFLOW 0x04
#define FE_UNDERFLOW 0x08
#define FE_INEXACT 0x10
#define FE_ALL_EXCEPT (FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW | FE_UNDERFLOW | FE_INEXACT)

// FE_DOWNWARD/FE_TOWARDZERO/FE_UPWARD are not supported by cwasm
#define FE_TONEAREST 0

extern fenv_t const __fe_dfl_env;
#define FE_DFL_ENV (&__fe_dfl_env)

int feclearexcept( int excepts );
int fegetexceptflag( fexcept_t* flagp, int excepts );
int feraiseexcept( int excepts );
int fesetexceptflag( fexcept_t const* flagp, int excepts );
int fetestexcept( int excepts );

int fegetround( void );
int fesetround( int mode );

int fegetenv( fenv_t* envp );
int feholdexcept( fenv_t* envp );
int fesetenv( fenv_t const* envp );
int feupdateenv( fenv_t const* envp );

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_FENV_H__
