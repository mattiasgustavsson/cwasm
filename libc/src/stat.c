#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>

#include "cwasm_fs.h"

#include <sys/stat.h>

int stat( char const* path, struct stat* st ) {
    char const* resolved;

    if( !path || !st ) {
        errno = EINVAL;
        return -1;
    }
    resolved = cwasm_fs_resolve_buf( path );
    if( !resolved ) { return -1; }
    return cwasm_fs_stat( resolved, st );
}

int lstat( char const* path, struct stat* st ) {
    return stat( path, st );
}

int fstat( int fd, struct stat* st ) {
    if( !st ) {
        errno = EINVAL;
        return -1;
    }
    if( fd < 0 ) {
        errno = EBADF;
        return -1;
    }
    return cwasm_stdio_fstat( (uint32_t)fd, st );
}

int ftruncate( int fd, off_t length ) {
    if( fd < 0 ) {
        errno = EBADF;
        return -1;
    }
    return cwasm_stdio_ftruncate( (uint32_t)fd, length );
}

int chmod( char const* path, mode_t mode ) {
    char const* resolved;

    if( !path ) {
        errno = EINVAL;
        return -1;
    }
    resolved = cwasm_fs_resolve_buf( path );
    if( !resolved ) { return -1; }
    return cwasm_fs_chmod( resolved, mode );
}

int fchmod( int fd, mode_t mode ) {
    if( fd < 0 ) {
        errno = EBADF;
        return -1;
    }
    if( fd <= 2 ) {
        errno = EBADF;
        return -1;
    }
    return cwasm_stdio_fchmod( (uint32_t)fd, mode );
}

int fchmodat( int dirfd, char const* path, mode_t mode, int flags ) {
    char resolved[ CWASM_FS_PATH_MAX ];

    if( flags & ~AT_SYMLINK_NOFOLLOW ) {
        errno = EINVAL;
        return -1;
    }
    if( flags & AT_SYMLINK_NOFOLLOW ) {
        errno = ENOTSUP;
        return -1;
    }
    if( !path ) {
        errno = EINVAL;
        return -1;
    }
    if( cwasm_fs_resolve_at( dirfd, path, resolved, sizeof( resolved ) ) != 0 ) {
        return -1;
    }
    return chmod( resolved, mode );
}
