#ifndef __CWASM_UNISTD_H__
#define __CWASM_UNISTD_H__
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#ifndef STDIN_FILENO
    #define STDIN_FILENO 0
    #define STDOUT_FILENO 1
    #define STDERR_FILENO 2
#endif

typedef long ssize_t;
typedef long off_t;

#ifndef F_OK
    #define F_OK 0
#endif
#ifndef X_OK
    #define X_OK 1
#endif
#ifndef W_OK
    #define W_OK 2
#endif
#ifndef R_OK
    #define R_OK 4
#endif

#ifndef _PC_PATH_MAX
    #define _PC_PATH_MAX 4
#endif

#ifndef _SC_PAGE_SIZE
    #define _SC_PAGE_SIZE 8
#endif
#ifndef _SC_PAGESIZE
    #define _SC_PAGESIZE _SC_PAGE_SIZE
#endif
#ifndef _SC_NPROCESSORS_ONLN
    #define _SC_NPROCESSORS_ONLN 84
#endif

#ifndef _POSIX_TIMERS
    #define _POSIX_TIMERS 1
#endif

#ifdef __cplusplus
    extern "C" {
#endif

int close( int fd );
int isatty( int fd );
ssize_t read( int fd, void* buf, size_t count );
ssize_t write( int fd, void const* buf, size_t count );
off_t lseek( int fd, off_t offset, int whence );
int ftruncate( int fd, off_t length );
int truncate( char const* path, off_t length );
int unlink( char const* path );
int unlinkat( int dirfd, char const* path, int flags );
char* getcwd( char* buf, size_t size );
int chdir( char const* path );
int access( char const* path, int mode );
int symlink( char const* target, char const* path );
int link( char const* oldpath, char const* newpath );
ssize_t readlink( char const* path, char* buf, size_t bufsiz );
int mkdir( char const* path, mode_t mode );
int rmdir( char const* path );
int mkstemp( char* path_template );
int execv( char const* path, char* const argv[] );
int execve( char const* path, char* const argv[], char* const envp[] );
int execl( char const* path, char const* arg, ... );
int execle( char const* path, char const* arg, ... );
long pathconf( char const* path, int name );
long sysconf( int name );
void* sbrk( intptr_t increment );

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_UNISTD_H__
