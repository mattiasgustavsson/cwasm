#ifndef __CWASM_DIRENT_H__
#define __CWASM_DIRENT_H__
#include <stdint.h>

#define DT_UNKNOWN 0
#define DT_REG 8
#define DT_DIR 4

struct dirent {
    unsigned char d_type;
    char d_name[ 256 ];
};

typedef struct DIR DIR;


#ifdef __cplusplus
    extern "C" {
#endif

DIR* opendir( char const* name );
DIR* fdopendir( int fd );
struct dirent* readdir( DIR* dirp );
int closedir( DIR* dirp );

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_DIRENT_H__
