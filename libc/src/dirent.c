#include <dirent.h>
#include <errno.h>

#include "cwasm_fs.h"

DIR* opendir( char const* name ) {
    char const* resolved;
    cwasm_fs_dir_t* dir;

    if( !name ) {
        errno = EINVAL;
        return NULL;
    }
    resolved = cwasm_fs_resolve_buf( name );
    if( !resolved ) { return NULL; }
    dir = cwasm_fs_opendir( resolved );
    if( !dir ) { return NULL; }
    return (DIR*)dir;
}

DIR* fdopendir( int fd ) {
    char path[ CWASM_FS_PATH_MAX ];
    cwasm_fs_dir_t* dir;

    if( cwasm_dir_fd_path( fd, path, sizeof( path ) ) != 0 ) { return NULL; }
    dir = cwasm_fs_opendir( path );
    if( !dir ) { return NULL; }
    cwasm_fs_dir_set_fd( dir, fd );
    return (DIR*)dir;
}

struct dirent* readdir( DIR* dirp ) {
    if( !dirp ) {
        errno = EBADF;
        return NULL;
    }
    return cwasm_fs_readdir( (cwasm_fs_dir_t*)dirp );
}

int closedir( DIR* dirp ) {
    cwasm_fs_dir_t* dir;
    int fd;

    if( !dirp ) {
        errno = EBADF;
        return -1;
    }
    dir = (cwasm_fs_dir_t*)dirp;
    fd = cwasm_fs_dir_take_fd( dir );
    if( cwasm_fs_closedir( dir ) != 0 ) { return -1; }
    if( fd >= 0 ) { return cwasm_dir_fd_close( fd ); }
    return 0;
}
