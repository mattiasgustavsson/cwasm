#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/random.h>
#include <unistd.h>

#include <cwasm.h>
#include "cmdline.h"
#include "cwasm_fs.h"

int isatty( int fd ) {
    return cwasm_stdio_isatty( fd );
}

CWASM_JS_LIB( CMD, void, js_exec_navigate, ( char const* url ), {
  const u = mem.str_read(url);
  if (u) location.assign(u);
})

static int exec_is_unreserved( unsigned char c ) {
    return ( c >= 'A' && c <= 'Z' ) || ( c >= 'a' && c <= 'z' ) ||
        ( c >= '0' && c <= '9' ) || c == '-' || c == '_' || c == '.' || c == '~';
}

static int exec_append_char( char* dst, size_t dstsz, size_t* pos, char ch ) {
    if( *pos + 1 >= dstsz ) {
        errno = ENAMETOOLONG;
        return -1;
    }
    dst[( *pos )++ ] = ch;
    dst[ *pos ] = '\0';
    return 0;
}

static int exec_append_encoded( char* dst, size_t dstsz, size_t* pos, char const* s ) {
    static char const hex[] = "0123456789ABCDEF";

    for( ; *s; ++s ) {
        unsigned char c = (unsigned char)*s;

        if( exec_is_unreserved( c ) ) {
            if( exec_append_char( dst, dstsz, pos, (char)c ) != 0 ) { return -1; }
        } else if( c == ' ' ) {
            if( exec_append_char( dst, dstsz, pos, '+' ) != 0 ) { return -1; }
        } else {
            if( *pos + 3 >= dstsz ) {
                errno = ENAMETOOLONG;
                return -1;
            }
            dst[( *pos )++ ] = '%';
            dst[( *pos )++ ] = hex[ c >> 4 ];
            dst[( *pos )++ ] = hex[ c & 0x0fu ];
            dst[ *pos ] = '\0';
        }
    }
    return 0;
}

static int exec_build_page_url( char const* canonical, char* out, size_t outsz ) {
    char page[ CWASM_FS_PATH_MAX ];
    char* slash;

    if( !canonical || canonical[ 0 ] != '/' ) {
        errno = EINVAL;
        return -1;
    }
    if( strncmp( canonical, "/?/appdata/", 11 ) == 0 ) {
        errno = EACCES;
        return -1;
    }
    if( cwasm_fetch_cmdline_prefix( page, sizeof( page ) ) != 0 ) { return -1; }
    slash = strrchr( page, '/' );
    if( !slash ) {
        errno = ENOENT;
        return -1;
    }
    slash[ 1 ] = '\0';
    if( snprintf( out, outsz, "%s%s", page, canonical + 1 ) >= (int)outsz ) {
        errno = ENAMETOOLONG;
        return -1;
    }
    return 0;
}

static int exec_append_query( char* url, size_t urlsz, char* const argv[] ) {
    char query[ 2048 ];
    size_t qpos = 0;
    size_t url_len;
    int i;
    int first = 1;

    if( !argv ) { return 0; }
    for( i = 1; argv[ i ]; ++i ) {
        char const* arg = argv[ i ];

        if( !first ) {
            if( qpos + 1 >= sizeof( query ) ) {
                errno = ENAMETOOLONG;
                return -1;
            }
            query[ qpos++ ] = ' ';
        }
        first = 0;
        while( *arg ) {
            if( qpos + 1 >= sizeof( query ) ) {
                errno = ENAMETOOLONG;
                return -1;
            }
            query[ qpos++ ] = *arg++;
        }
    }
    if( first ) { return 0; }
    query[ qpos ] = '\0';

    url_len = strlen( url );
    if( url_len + 1 >= urlsz ) {
        errno = ENAMETOOLONG;
        return -1;
    }
    url[ url_len++ ] = '?';
    url[ url_len ] = '\0';
    return exec_append_encoded( url, urlsz, &url_len, query );
}

static int exec_append_fragment_raw( char* url, size_t urlsz, char const* frag ) {
    size_t url_len;

    if( !frag || !*frag ) { return 0; }
    url_len = strlen( url );
    if( url_len + 1 >= urlsz ) {
        errno = ENAMETOOLONG;
        return -1;
    }
    url[ url_len++ ] = '#';
    url[ url_len ] = '\0';
    return exec_append_encoded( url, urlsz, &url_len, frag );
}

static int exec_append_fragment( char* url, size_t urlsz, char* const envp[] ) {
    char frag[ 2048 ];
    size_t fpos = 0;
    int i;
    int first = 1;

    if( !envp ) { return 0; }
    for( i = 0; envp[ i ]; ++i ) {
        char const* e = envp[ i ];

        if( !e || !*e ) { continue; }
        if( !first ) {
            if( fpos + 1 >= sizeof( frag ) ) {
                errno = ENAMETOOLONG;
                return -1;
            }
            frag[ fpos++ ] = '&';
        }
        first = 0;
        while( *e ) {
            if( fpos + 1 >= sizeof( frag ) ) {
                errno = ENAMETOOLONG;
                return -1;
            }
            frag[ fpos++ ] = *e++;
        }
    }
    if( first ) { return 0; }
    frag[ fpos ] = '\0';
    return exec_append_fragment_raw( url, urlsz, frag );
}

static int exec_append_fragment_inherit( char* url, size_t urlsz ) {
    char frag[ 2048 ];

    if( cwasm_fetch_envstring( frag, sizeof( frag ) ) != 0 ) { return -1; }
    return exec_append_fragment_raw( url, urlsz, frag );
}

static int exec_common( char const* path, char* const argv[], char* const envp[] ) {
    char canonical[ CWASM_FS_PATH_MAX ];
    char url[ CWASM_FS_PATH_MAX * 2 ];
    char const* resolved;

    if( !path ) {
        errno = EINVAL;
        return -1;
    }
    resolved = cwasm_fs_resolve_buf( path );
    if( !resolved ) { return -1; }
    if( strlen( resolved ) + 1 > sizeof( canonical ) ) {
        errno = ENAMETOOLONG;
        return -1;
    }
    memcpy( canonical, resolved, strlen( resolved ) + 1u );
    if( exec_build_page_url( canonical, url, sizeof( url ) ) != 0 ) { return -1; }
    if( exec_append_query( url, sizeof( url ), argv ) != 0 ) { return -1; }
    if( envp ) {
        if( exec_append_fragment( url, sizeof( url ), envp ) != 0 ) { return -1; }
    } else if( exec_append_fragment_inherit( url, sizeof( url ) ) != 0 ) {
        return -1;
    }

    js_exec_navigate( url );
    __builtin_unreachable();
}

int execv( char const* path, char* const argv[] ) {
    return exec_common( path, argv, NULL );
}

int execve( char const* path, char* const argv[], char* const envp[] ) {
    return exec_common( path, argv, envp );
}

int execl( char const* path, char const* arg, ... ) {
    char* argv[ 256 ];
    va_list ap;
    int i = 0;

    if( !path || !arg ) {
        errno = EINVAL;
        return -1;
    }
    argv[ i++ ] = (char*)arg;
    va_start( ap, arg );
    while( i < (int)( sizeof( argv ) / sizeof( argv[ 0 ] ) ) ) {
        char* next = va_arg( ap, char* );

        argv[ i++ ] = next;
        if( !next ) { break; }
    }
    va_end( ap );
    if( i >= (int)( sizeof( argv ) / sizeof( argv[ 0 ] ) ) ) {
        errno = ENAMETOOLONG;
        return -1;
    }
    return execv( path, argv );
}

int execle( char const* path, char const* arg, ... ) {
    char* argv[ 256 ];
    char** envp;
    va_list ap;
    int i = 0;

    if( !path || !arg ) {
        errno = EINVAL;
        return -1;
    }
    argv[ i++ ] = (char*)arg;
    va_start( ap, arg );
    while( i < (int)( sizeof( argv ) / sizeof( argv[ 0 ] ) ) ) {
        char* next = va_arg( ap, char* );

        argv[ i++ ] = next;
        if( !next ) { break; }
    }
    if( i >= (int)( sizeof( argv ) / sizeof( argv[ 0 ] ) ) ) {
        va_end( ap );
        errno = ENAMETOOLONG;
        return -1;
    }
    envp = va_arg( ap, char** );
    va_end( ap );
    return execve( path, argv, envp );
}

int close( int fd ) {
    if( fd < 0 ) {
        errno = EBADF;
        return -1;
    }
    if( cwasm_dir_fd_is_dir( fd ) ) { return cwasm_dir_fd_close( fd ); }
    return cwasm_stdio_close_fd( fd );
}

ssize_t read( int fd, void* buf, size_t count ) {
    return cwasm_stdio_read( fd, buf, count );
}

ssize_t write( int fd, void const* buf, size_t count ) {
    return cwasm_stdio_write( fd, buf, count );
}

off_t lseek( int fd, off_t offset, int whence ) {
    return cwasm_stdio_lseek( fd, offset, whence );
}

int unlink( char const* path ) { return remove( path ); }

int unlinkat( int dirfd, char const* path, int flags ) {
    char resolved[ CWASM_FS_PATH_MAX ];

    if( !path ) {
        errno = EINVAL;
        return -1;
    }
    if( cwasm_fs_resolve_at( dirfd, path, resolved, sizeof( resolved ) ) != 0 ) {
        return -1;
    }
    if( flags & AT_REMOVEDIR ) { return rmdir( resolved ); }
    return unlink( resolved );
}

char* getcwd( char* buf, size_t size ) {
    if( !buf ) {
        errno = EINVAL;
        return NULL;
    }
    if( size == 0 ) {
        errno = EINVAL;
        return NULL;
    }
    if( size < 2 ) {
        errno = ERANGE;
        return NULL;
    }
    return cwasm_fs_getcwd( buf, size );
}

int chdir( char const* path ) {
    if( !path ) {
        errno = EINVAL;
        return -1;
    }
    return cwasm_fs_chdir( path );
}

int access( char const* path, int mode ) {
    char const* resolved;

    if( !path ) {
        errno = EINVAL;
        return -1;
    }
    resolved = cwasm_fs_resolve_buf( path );
    if( !resolved ) { return -1; }
    return cwasm_fs_access( resolved, mode );
}

ssize_t readlink( char const* path, char* buf, size_t bufsiz ) {
    char const* resolved;
    struct stat st;

    if( !path || !buf || !bufsiz ) {
        errno = EINVAL;
        return -1;
    }
    resolved = cwasm_fs_resolve_buf( path );
    if( !resolved ) { return -1; }
    if( cwasm_fs_stat( resolved, &st ) != 0 ) { return -1; }
    errno = ENOTSUP;
    return -1;
}

int symlink( char const* target, char const* path ) {
    (void)target;
    (void)path;
    errno = ENOTSUP;
    return -1;
}

int link( char const* oldpath, char const* newpath ) {
    (void)oldpath;
    (void)newpath;
    errno = ENOTSUP;
    return -1;
}

int truncate( char const* path, off_t length ) {
    int fd;
    int rc;

    if( !path || length < 0 ) {
        errno = EINVAL;
        return -1;
    }
    fd = open( path, O_WRONLY );
    if( fd < 0 ) { return -1; }
    rc = ftruncate( fd, length );
    close( fd );
    return rc;
}

int utimes( char const* path, struct timeval const times[ 2 ] ) {
    char const* resolved;

    if( !path ) {
        errno = EINVAL;
        return -1;
    }
    resolved = cwasm_fs_resolve_buf( path );
    if( !resolved ) { return -1; }
    return cwasm_fs_utimes( resolved, times );
}

int mkdir( char const* path, mode_t mode ) {
    char const* resolved;

    if( !path ) {
        errno = EINVAL;
        return -1;
    }
    resolved = cwasm_fs_resolve_buf( path );
    if( !resolved ) { return -1; }
    return cwasm_fs_mkdir( resolved, mode );
}

int rmdir( char const* path ) {
    char const* resolved;

    if( !path ) {
        errno = EINVAL;
        return -1;
    }
    resolved = cwasm_fs_resolve_buf( path );
    if( !resolved ) { return -1; }
    return cwasm_fs_rmdir( resolved );
}

int mkstemp( char* path_template ) {
    static char const chars[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    size_t n;
    char* suffix;
    int attempt;

    if( !path_template ) {
        errno = EINVAL;
        return -1;
    }
    n = strlen( path_template );
    if( n < 6 || memcmp( path_template + n - 6, "XXXXXX", 6 ) != 0 ) {
        errno = EINVAL;
        return -1;
    }
    suffix = path_template + n - 6;

    for( attempt = 0; attempt < 256; ++attempt ) {
        unsigned char rnd[ 6 ];
        int fd;
        size_t i;

        if( getentropy( rnd, sizeof( rnd ) ) != 0 ) { return -1; }
        for( i = 0; i < 6; ++i ) {
            suffix[ i ] = chars[ rnd[ i ] % ( sizeof( chars ) - 1u ) ];
        }

        fd = open( path_template, O_RDWR | O_CREAT | O_EXCL, 0600 );
        if( fd >= 0 ) { return fd; }
        if( errno != EEXIST ) { return -1; }
    }
    errno = EEXIST;
    return -1;
}

long pathconf( char const* path, int name ) {
    (void)path;
    switch( name ) {
    case _PC_PATH_MAX:
        return PATH_MAX;
    default:
        errno = EINVAL;
        return -1;
    }
}

CWASM_JS_LIB( CMD, double, js_hardware_concurrency, ( void ), {
  return navigator.hardwareConcurrency | 0 || 4;
})

long sysconf( int name ) {
    switch( name ) {
    case _SC_PAGE_SIZE:
        return 65536L; // wasm page
    case _SC_NPROCESSORS_ONLN: {
        long n = (long)js_hardware_concurrency();
        return n > 0 ? n : 4;
    }
    default:
        errno = EINVAL;
        return -1;
    }
}

int statvfs( char const* path, struct statvfs* buf ) {
    char const* resolved;

    if( !path || !buf ) {
        errno = EINVAL;
        return -1;
    }
    resolved = cwasm_fs_resolve_buf( path );
    if( !resolved ) { return -1; }
    return cwasm_fs_statvfs( resolved, buf );
}
