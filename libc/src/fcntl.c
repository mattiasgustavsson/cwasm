#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#include "cwasm_fs.h"

static unsigned cwasm_flags_from_open( int flags ) {
    unsigned f = 0;
    int acc = flags & O_ACCMODE;

    if( acc == O_RDONLY ) { f = CWASM_FS_OPEN_READ; }
    else if( acc == O_WRONLY ) { f = CWASM_FS_OPEN_WRITE; }
    else if( acc == O_RDWR ) { f = CWASM_FS_OPEN_READ | CWASM_FS_OPEN_WRITE; }

    if( flags & O_CREAT ) { f |= CWASM_FS_OPEN_CREATE; }
    if( flags & O_TRUNC ) { f |= CWASM_FS_OPEN_TRUNCATE; }
    if( flags & O_APPEND ) { f |= CWASM_FS_OPEN_APPEND; }
    if( flags & O_EXCL ) { f |= CWASM_FS_OPEN_EXCLUSIVE; }
    return f;
}

int open( char const* path, int flags, ... ) {
    char const* resolved;
    unsigned f;
    mode_t mode = 0666;
    int fd;
    va_list ap;

    if( !path ) {
        errno = EINVAL;
        return -1;
    }
    if( flags & O_CREAT ) {
        va_start( ap, flags );
        mode = va_arg( ap, mode_t );
        va_end( ap );
    }
    resolved = cwasm_fs_resolve_buf( path );
    if( !resolved ) { return -1; }
    f = cwasm_flags_from_open( flags );
    fd = cwasm_stdio_open( resolved, f, mode );
    if( fd < 0 ) { return -1; }
    return fd;
}

int openat( int dirfd, char const* path, int flags, ... ) {
    char resolved[ CWASM_FS_PATH_MAX ];
    mode_t mode = 0666;
    struct stat st;
    va_list ap;

    if( !path ) {
        errno = EINVAL;
        return -1;
    }
    if( cwasm_fs_resolve_at( dirfd, path, resolved, sizeof( resolved ) ) != 0 ) {
        return -1;
    }
    if( flags & O_DIRECTORY ) {
        if( stat( resolved, &st ) != 0 ) { return -1; }
        if( !S_ISDIR( st.st_mode ) ) {
            errno = ENOTDIR;
            return -1;
        }
        return cwasm_dir_fd_open( resolved );
    }
    if( flags & O_CREAT ) {
        va_start( ap, flags );
        mode = va_arg( ap, mode_t );
        va_end( ap );
    }
    flags &= ~( O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC );
    return open( resolved, flags, mode );
}
