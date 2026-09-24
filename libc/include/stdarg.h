#ifndef __CWASM_STDARG_H__
#define __CWASM_STDARG_H__
// wasm32 passes a pointer to the vararg block as an extra parameter.
// using compiler builtins so va_start reads that block, not &last+4.
typedef __builtin_va_list va_list;

#define va_start(ap, param) __builtin_va_start(ap, param)
#define va_end(ap) __builtin_va_end(ap)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_copy(dst, src) __builtin_va_copy(dst, src)

#endif // __CWASM_STDARG_H__
