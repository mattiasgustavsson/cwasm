#ifndef __CWASM_STDALIGN_H__
#define __CWASM_STDALIGN_H__
#if !defined(__cplusplus) && (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L)
    #define alignas _Alignas
    #define alignof _Alignof
#endif
#define __alignas_is_defined 1
#define __alignof_is_defined 1

#endif // __CWASM_STDALIGN_H__
