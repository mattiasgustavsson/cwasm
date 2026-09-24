#ifndef __CWASM_SYS_STAT_H__
#define __CWASM_SYS_STAT_H__
#include <stdint.h>
#include <time.h>

typedef uint32_t mode_t;

struct stat {
    mode_t st_mode;
    long st_size;
    unsigned long st_dev;
    unsigned long st_ino;
    unsigned short st_nlink;
    struct timespec st_atim;
    struct timespec st_mtim;
};

#define st_atime st_atim.tv_sec
#define st_mtime st_mtim.tv_sec
#define st_ctime st_mtim.tv_sec

#ifndef S_IFMT
    #define S_IFMT 0170000
#endif
#ifndef S_IFREG
    #define S_IFREG 0100000
#endif
#ifndef S_IFDIR
    #define S_IFDIR 0040000
#endif
#ifndef S_IFLNK
    #define S_IFLNK 0120000
#endif
#ifndef S_IFBLK
    #define S_IFBLK 0060000
#endif
#ifndef S_IFCHR
    #define S_IFCHR 0020000
#endif
#ifndef S_IFIFO
    #define S_IFIFO 0010000
#endif
#ifndef S_IFSOCK
    #define S_IFSOCK 0140000
#endif

#ifndef S_ISREG
    #define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#ifndef S_ISDIR
    #define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISLNK
    #define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#endif
#ifndef S_ISBLK
    #define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#endif
#ifndef S_ISCHR
    #define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#endif
#ifndef S_ISFIFO
    #define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#endif
#ifndef S_ISSOCK
    #define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
#endif

#ifndef S_IRUSR
    #define S_IRUSR 0400
#endif
#ifndef S_IWUSR
    #define S_IWUSR 0200
#endif
#ifndef S_IXUSR
    #define S_IXUSR 0100
#endif
#ifndef S_IRWXU
    #define S_IRWXU (S_IRUSR | S_IWUSR | S_IXUSR)
#endif


#ifdef __cplusplus
    extern "C" {
#endif

int mkdir( char const* path, mode_t mode );
int stat( char const* path, struct stat* st );
int lstat( char const* path, struct stat* st );
int fstat( int fd, struct stat* st );
int chmod( char const* path, mode_t mode );
int fchmod( int fd, mode_t mode );
int fchmodat( int dirfd, char const* path, mode_t mode, int flags );

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_SYS_STAT_H__
