#ifndef __CWASM_H__
#define __CWASM_H__
#ifdef __cplusplus
    #define CWASM_EXTERN extern "C"
#else
    #define CWASM_EXTERN extern
#endif

#define CWASM_JS(ret, name, args, ...) \
  CWASM_EXTERN __attribute__( ( import_module( "JS" ), import_name( #name "\v" #args "\v" #__VA_ARGS__))) ret name args;

#define CWASM_JS_INIT(INIT, ret, name, args, ...) \
  CWASM_EXTERN __attribute__( ( import_module( "JS" ), import_name( #name "\v" #args "\v" #__VA_ARGS__ "\v\v" #INIT))) ret name args;

#define CWASM_JS_LIB(lib, ret, name, args, ...) \
  CWASM_EXTERN __attribute__( ( import_module( "JS" ), import_name( #name "\v" #args "\v" #__VA_ARGS__ "\v" #lib))) ret name args;

#define CWASM_JS_LIB_INIT(lib, INIT, ret, name, args, ...) \
  CWASM_EXTERN __attribute__( ( import_module( "JS" ), import_name( #name "\v" #args "\v" #__VA_ARGS__ "\v" #lib "\v" #INIT))) ret name args;

#define CWASM_JS_MAIN(ret, name, args, ...) \
  CWASM_EXTERN __attribute__( ( import_module( "JSMAIN" ), import_name( #name "\v" #args "\v" #__VA_ARGS__))) ret name args;

#define CWASM_JS_MAIN_INIT(INIT, ret, name, args, ...) \
  CWASM_EXTERN __attribute__( ( import_module( "JSMAIN" ), import_name( #name "\v" #args "\v" #__VA_ARGS__ "\v\v" #INIT))) ret name args;

#define CWASM_JS_MAIN_LIB(lib, ret, name, args, ...) \
  CWASM_EXTERN __attribute__( ( import_module( "JSMAIN" ), import_name( #name "\v" #args "\v" #__VA_ARGS__ "\v" #lib))) ret name args;

#define CWASM_JS_MAIN_LIB_INIT(lib, INIT, ret, name, args, ...) \
  CWASM_EXTERN __attribute__( ( import_module( "JSMAIN" ), import_name( #name "\v" #args "\v" #__VA_ARGS__ "\v" #lib "\v" #INIT))) ret name args;

#endif // __CWASM_H__
