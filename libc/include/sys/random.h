#ifndef __CWASM_SYS_RANDOM_H__
#define __CWASM_SYS_RANDOM_H__
#include <stddef.h>

#ifdef __cplusplus
    extern "C" {
#endif

int getentropy( void* buffer, size_t length );

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_SYS_RANDOM_H__
