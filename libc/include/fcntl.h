#ifndef __CWASM_FCNTL_H__
#define __CWASM_FCNTL_H__
#include <stddef.h>

#ifndef O_RDONLY
    #define O_RDONLY 0x0000
#endif
#ifndef O_WRONLY
    #define O_WRONLY 0x0001
#endif
#ifndef O_RDWR
    #define O_RDWR 0x0002
#endif
#ifndef O_ACCMODE
    #define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)
#endif

#ifndef O_CREAT
    #define O_CREAT 0x0100
#endif
#ifndef O_TRUNC
    #define O_TRUNC 0x0200
#endif
#ifndef O_APPEND
    #define O_APPEND 0x0400
#endif
#ifndef O_EXCL
    #define O_EXCL 0x0800
#endif

#ifndef O_BINARY
    #define O_BINARY 0
#endif
#ifndef O_NONBLOCK
    #define O_NONBLOCK 0x1000
#endif
#ifndef O_CLOEXEC
    #define O_CLOEXEC 0
#endif
#ifndef O_DIRECTORY
    #define O_DIRECTORY 0x010000
#endif
#ifndef O_NOFOLLOW
    #define O_NOFOLLOW 0x020000
#endif

#ifndef AT_FDCWD
    #define AT_FDCWD (-100)
#endif
#ifndef AT_SYMLINK_NOFOLLOW
    #define AT_SYMLINK_NOFOLLOW 0x100
#endif
#ifndef AT_REMOVEDIR
    #define AT_REMOVEDIR 0x200
#endif

#include <stdarg.h>

#ifdef __cplusplus
    extern "C" {
#endif

int open( char const* path, int flags, ... );
int openat( int dirfd, char const* path, int flags, ... );

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_FCNTL_H__
