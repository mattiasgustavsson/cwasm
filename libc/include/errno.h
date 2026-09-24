#ifndef __CWASM_ERRNO_H__
#define __CWASM_ERRNO_H__
#define EPERM 1
#define ENOENT 2
#define ESRCH 3
#define EINTR 4
#define EIO 5
#define EBADF 9
#define EAGAIN 11
#define EWOULDBLOCK EAGAIN
#define ENOMEM 12
#define EACCES 13
#define EBUSY 16
#define EEXIST 17
#define EXDEV 18
#define ENODEV 19
#define ENOTDIR 20
#define EISDIR 21
#define EINVAL 22
#define EMFILE 24
#define ENOSPC 28
#define ESPIPE 29
#define EDOM 33
#define ERANGE 34
#define EDEADLK 35
#define ENAMETOOLONG 36
#define ENOSYS 38
#define ENOTEMPTY 39
#define EILSEQ 84
#define ENOTSUP 95
#define ETIMEDOUT 110

#define ELAST ETIMEDOUT

#ifdef __cplusplus
    extern "C" {
#endif

#ifdef CWASM_THREADS
    int* __errno_location( void );
    #define errno (*__errno_location())
#else
    extern int errno;
#endif

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_ERRNO_H__
