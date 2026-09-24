#ifndef __CWASM_FS_H__
#define __CWASM_FS_H__

#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>

#define CWASM_FS_PATH_MAX 1024

#define CWASM_FS_OPEN_READ 0x01u
#define CWASM_FS_OPEN_WRITE 0x02u
#define CWASM_FS_OPEN_CREATE 0x04u
#define CWASM_FS_OPEN_TRUNCATE 0x08u
#define CWASM_FS_OPEN_APPEND 0x10u
#define CWASM_FS_OPEN_EXCLUSIVE 0x20u

typedef enum {
    CWASM_FS_HKIND_NONE = 0,
    CWASM_FS_HKIND_WFS,
    CWASM_FS_HKIND_HTTP,
    CWASM_FS_HKIND_IDB,
} cwasm_fs_hkind_t;

typedef struct {
    cwasm_fs_hkind_t kind;
    uint32_t wfs_off;
    uint32_t wfs_size;
    uint32_t pos;
    int js_file_id;
    uint8_t* idb_buf;
    uint32_t idb_size;
    uint32_t idb_cap;
    int idb_dirty;
    int append_mode;
    int unlink_on_close;
    char* path;
    mode_t file_mode;
    int64_t meta_atime_sec;
    long meta_atime_nsec;
    int64_t meta_mtime_sec;
    long meta_mtime_nsec;
} cwasm_fs_handle_t;

typedef struct cwasm_fs_dir cwasm_fs_dir_t;

void cwasm_fs_init( void );

int cwasm_fs_resolve( char const* in, char* out, size_t outsz );
int cwasm_fs_resolve_at( int dirfd, char const* path, char* out, size_t outsz );
char const* cwasm_fs_resolve_buf( char const* in );

int cwasm_dir_fd_is_dir( int fd );
int cwasm_dir_fd_is_dir_unlocked( int fd );
int cwasm_dir_fd_open( char const* canonical_path );
int cwasm_dir_fd_path( int fd, char* buf, size_t bufsz );
int cwasm_dir_fd_close( int fd );

char* cwasm_fs_getcwd( char* buf, size_t size );
int cwasm_fs_chdir( char const* path );

int cwasm_fs_open( char const* canonical, unsigned flags, mode_t mode, cwasm_fs_handle_t* hout );
int cwasm_fs_read( cwasm_fs_handle_t* h, void* buf, uint32_t n );
int cwasm_fs_write( cwasm_fs_handle_t* h, void const* buf, uint32_t n );
long cwasm_fs_seek( cwasm_fs_handle_t* h, long offset, int whence );
long cwasm_fs_tell( cwasm_fs_handle_t* h );
int cwasm_fs_flush( cwasm_fs_handle_t* h );
void cwasm_fs_close( cwasm_fs_handle_t* h );

#define CWASM_FS_DEV_NONE 0
#define CWASM_FS_DEV_RANDOM 1
#define CWASM_FS_DEV_NULL 2

int cwasm_fs_dev_kind( char const* canonical );
int cwasm_fs_dev_stat( char const* canonical, struct stat* st );

int cwasm_fs_stat( char const* canonical, struct stat* st );
int cwasm_fs_is_appdata_path( char const* canonical );
int cwasm_fs_fstat( cwasm_fs_handle_t* h, struct stat* st );
int cwasm_fs_ftruncate( cwasm_fs_handle_t* h, off_t length );

cwasm_fs_dir_t* cwasm_fs_opendir( char const* canonical );
void cwasm_fs_dir_set_fd( cwasm_fs_dir_t* dir, int fd );
int cwasm_fs_dir_take_fd( cwasm_fs_dir_t* dir );
struct dirent* cwasm_fs_readdir( cwasm_fs_dir_t* dir );
int cwasm_fs_closedir( cwasm_fs_dir_t* dir );

int cwasm_fs_mkdir( char const* canonical, mode_t mode );
int cwasm_fs_rmdir( char const* canonical );
int cwasm_fs_remove( char const* canonical );
int cwasm_fs_rename( char const* oldpath, char const* newpath );
int cwasm_fs_access( char const* canonical, int mode );
int cwasm_fs_chmod( char const* canonical, mode_t mode );
int cwasm_fs_utimes( char const* canonical, struct timeval const times[ 2 ] );
int cwasm_fs_statvfs( char const* canonical, struct statvfs* buf );

int cwasm_stdio_fstat( uint32_t fh, struct stat* st );
int cwasm_stdio_ftruncate( uint32_t fh, off_t length );
int cwasm_stdio_fflush( uint32_t fh );
int cwasm_stdio_fchmod( uint32_t fh, mode_t mode );
int cwasm_stdio_open( char const* resolved, unsigned flags, mode_t mode );
int cwasm_stdio_fh_is_open( uint32_t fh );
int cwasm_stdio_fh_is_open_unlocked( uint32_t fh );
void cwasm_stdio_table_lock( void );
void cwasm_stdio_table_unlock( void );
int cwasm_stdio_isatty( int fd );
ssize_t cwasm_stdio_read( int fd, void* buf, size_t count );
ssize_t cwasm_stdio_write( int fd, void const* buf, size_t count );
int cwasm_stdio_close_fd( int fd );
off_t cwasm_stdio_lseek( int fd, off_t offset, int whence );

void wfs_init( void );
uint32_t wfs_stdin_size( void );
uint32_t wfs_embed_read( uint32_t offset, void* dst, uint32_t len );

#endif // __CWASM_FS_H__
