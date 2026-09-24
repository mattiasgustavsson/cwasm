#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <wchar.h>
#include <wctype.h>
#include <string.h>

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/random.h>

#ifdef CWASM_THREADS
    #include <sched.h>
    #include <stdatomic.h>
#endif

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#include <cwasm.h>
#include "cwasm_fs.h"
#include "cwasm_lock.h"
#include "wscanset.h"

extern char __cwasm_locale_radix_char( void );

void __cwasm_stdio_set_numeric_separators( char thousands, char decimal ) {
    stbsp_set_separators( thousands, decimal );
}

struct FILE {
    uint32_t __fh;
};

extern void __cwasm_mbstate_reset( mbstate_t* ps );
extern void __cwasm_wide_bytebuf_reset( uint32_t h );
extern size_t __cwasm_wide_bytebuf_pending( uint32_t h );

#define CWASM_STDIO_N 1024u
#define CWASM_STDIO_BUFSIZ 4096u

// Table lock for fh alloc/free, redirects, cwasm_stdio_init.
// Lock order: g_stdio_lock -> g_fh_locks[i] when acquiring both from scratch.
// Never block on an fh lock while holding the table lock (see cwasm_table_await_fh).
// Taking the table while already holding a per-fh lock is OK (flockfile->fopen).
// freopen drops fh locks across slow cwasm_fs_open (see cwasm_stdio_reopen_busy).
static cwasm_lock_t g_stdio_lock = CWASM_LOCK_INIT;
static cwasm_rec_lock_t g_fh_locks[ CWASM_STDIO_N ];
static int g_fh_locks_inited;

// Std streams only: set while freopen has dropped the fh lock across FS open.
static uint8_t cwasm_stdio_reopen_busy[ 3 ];

#ifdef CWASM_THREADS
    // Per-thread flockfile ownership: unlock the same real fh that was locked, even if freopen changes redirects
    static _Thread_local uint8_t g_flock_held[ CWASM_STDIO_N ];
    static _Thread_local uint32_t g_flock_real[ CWASM_STDIO_N ];
    static _Thread_local uint32_t g_flock_depth[ CWASM_STDIO_N ];
#endif

static void cwasm_fh_locks_init( void ) {
    uint32_t i;
    if( g_fh_locks_inited ) { return; }
    for( i = 0; i < CWASM_STDIO_N; ++i ) { cwasm_rec_lock_init( &g_fh_locks[ i ] ); }
    g_fh_locks_inited = 1;
}

static void cwasm_fh_lock( uint32_t fh ) {
    if( fh < CWASM_STDIO_N ) { cwasm_rec_lock( &g_fh_locks[ fh ] ); }
}

static void cwasm_fh_unlock( uint32_t fh ) {
    if( fh < CWASM_STDIO_N ) { cwasm_rec_unlock( &g_fh_locks[ fh ] ); }
}

static int cwasm_fh_trylock( uint32_t fh ) {
    if( fh >= CWASM_STDIO_N ) { return -1; }
    return cwasm_rec_trylock( &g_fh_locks[ fh ] );
}

static void cwasm_fh_unlock2( uint32_t a, uint32_t b ) {
    if( a == b ) {
        cwasm_fh_unlock( a );
        return;
    }
    if( a < b ) {
        cwasm_fh_unlock( b );
        cwasm_fh_unlock( a );
    } else {
        cwasm_fh_unlock( a );
        cwasm_fh_unlock( b );
    }
}

static int cwasm_fh_trylock2( uint32_t a, uint32_t b ) {
    uint32_t lo;
    uint32_t hi;

    if( a == b ) { return cwasm_fh_trylock( a ); }
    lo = a < b ? a : b;
    hi = a < b ? b : a;
    if( cwasm_fh_trylock( lo ) != 0 ) { return -1; }
    if( cwasm_fh_trylock( hi ) != 0 ) {
        cwasm_fh_unlock( lo );
        return -1;
    }
    return 0;
}

// Caller holds table, fh trylock failed. Drop table, briefly take fh, drop it,
// re-acquire table so the caller can retry without holding table while blocked.
// On return the caller still holds g_stdio_lock - do not lock it again.
static void cwasm_table_await_fh( uint32_t fh ) {
    cwasm_unlock( &g_stdio_lock );
    cwasm_fh_lock( fh );
    cwasm_fh_unlock( fh );
    cwasm_lock( &g_stdio_lock );
}

// Same as cwasm_table_await_fh but for two fh locks (lower index first). Awaits the
// contended lock(s) - do not await only min(a,b), which livelocks when hi is held.
static void cwasm_table_await_fh2( uint32_t a, uint32_t b ) {
    uint32_t lo;
    uint32_t hi;

    if( a == b ) {
        cwasm_table_await_fh( a );
        return;
    }
    lo = a < b ? a : b;
    hi = a < b ? b : a;
    cwasm_unlock( &g_stdio_lock );
    cwasm_fh_lock( lo );
    cwasm_fh_lock( hi );
    cwasm_fh_unlock( hi );
    cwasm_fh_unlock( lo );
    cwasm_lock( &g_stdio_lock );
}

CWASM_JS_LIB( IO, uint32_t, js_stdio_write, ( uint32_t buf, uint32_t total ), {
  if (!CWASM.__stdio_buf) CWASM.__stdio_buf = "";
  CWASM.__stdio_buf += mem.str_read(buf, total);
  for (var nl; (nl = CWASM.__stdio_buf.indexOf('\n')) >= 0; ) {
    print(CWASM.__stdio_buf.slice(0, nl + 1));
    CWASM.__stdio_buf = CWASM.__stdio_buf.slice(nl + 1);
  }
  return 0;
})

CWASM_JS_LIB( IO, uint32_t, js_stdio_flush, ( void ), {
  if (CWASM.__stdio_buf) {
    print(CWASM.__stdio_buf);
    CWASM.__stdio_buf = "";
  }
  return 0;
})

mbstate_t __cwasm_wide_rd_state[ CWASM_STDIO_N ];
mbstate_t __cwasm_wide_wr_state[ CWASM_STDIO_N ];
wchar_t __cwasm_wide_unget_buf[ CWASM_STDIO_N ][ __CWASM_STDIO_UNGET ];
uint8_t __cwasm_wide_unget_len[ CWASM_STDIO_N ];
uint8_t __cwasm_wide_unget_nbytes[ CWASM_STDIO_N ];
FILE __cwasm_stdin = {0u};
FILE __cwasm_stdout = {1u};
FILE __cwasm_stderr = {2u};

#define CWASM_FH_ORIENT_NONE 0u
#define CWASM_FH_ORIENT_WIDE 1u
#define CWASM_FH_ORIENT_BYTE 2u

static uint8_t cwasm_fh_orient[ CWASM_STDIO_N ];

static void cwasm_fh_orient_reset( uint32_t h ) {
    if( h < CWASM_STDIO_N ) { cwasm_fh_orient[ h ] = CWASM_FH_ORIENT_NONE; }
}

static int cwasm_fh_orient_fwide_value( uint32_t fh ) {
    switch( cwasm_fh_orient[ fh ] ) {
    case CWASM_FH_ORIENT_WIDE:
        return 1;
    case CWASM_FH_ORIENT_BYTE:
        return -1;
    default:
        return 0;
    }
}

static void __cwasm_wide_reset_handle( uint32_t h ) {
    if( h >= CWASM_STDIO_N ) { return; }
    __cwasm_mbstate_reset( &__cwasm_wide_rd_state[ h ] );
    __cwasm_mbstate_reset( &__cwasm_wide_wr_state[ h ] );
    __cwasm_wide_unget_len[ h ] = 0;
    __cwasm_wide_unget_nbytes[ h ] = 0;
    __cwasm_wide_bytebuf_reset( h );
}

#ifdef CWASM_THREADS
    static _Thread_local char cwasm_resolve_buf[ CWASM_FS_PATH_MAX ];
#else
    static char cwasm_resolve_buf[ CWASM_FS_PATH_MAX ];
#endif

static cwasm_fs_handle_t cwasm_fs_h[ CWASM_STDIO_N ];
static uint8_t cwasm_file_valid[ CWASM_STDIO_N ];
static uint8_t cwasm_fh_wfs[ CWASM_STDIO_N ];
static uint8_t cwasm_fh_dev[ CWASM_STDIO_N ];
static char* cwasm_fh_path[ CWASM_STDIO_N ];

#define CWASM_FH_RDONLY 1u
#define CWASM_FH_WRONLY 2u
#define CWASM_FH_RDWR 3u

static uint8_t cwasm_fh_eof[ CWASM_STDIO_N ];
static uint8_t cwasm_fh_err[ CWASM_STDIO_N ];
static uint8_t cwasm_fh_access[ CWASM_STDIO_N ];
static uint8_t cwasm_fh_tty[ CWASM_STDIO_N ];
static uint8_t cwasm_fh_unget_buf[ CWASM_STDIO_N ][ __CWASM_STDIO_UNGET ];
static uint8_t cwasm_fh_unget_len[ CWASM_STDIO_N ];
static uint32_t cwasm_stdio_redirect[ 3 ];
static uint32_t cwasm_stdin_pos;

typedef struct {
    char* data;
    size_t cap;
    size_t len;
    size_t pos;
    uint8_t vmode;
    uint8_t owned;
    uint8_t active;
} cwasm_fh_io_t;

static cwasm_fh_io_t cwasm_fh_io[ CWASM_STDIO_N ];

// FILE-object liveness. Atomic under threads so cwasm_fh_from_stream can read
// without taking the table lock (callers often nest into table -> fh).
#ifdef CWASM_THREADS
    static _Atomic uint8_t cwasm_file_obj_used[ CWASM_STDIO_N ];
#else
    static uint8_t cwasm_file_obj_used[ CWASM_STDIO_N ];
#endif
static FILE cwasm_file_objs[ CWASM_STDIO_N ];
#ifdef CWASM_THREADS
    static _Atomic int cwasm_stdio_inited;
#else
    static int cwasm_stdio_inited;
#endif

static void cwasm_stdio_init_unlocked( void );
static int cwasm_fh_valid_unlocked( uint32_t fh );

static uint32_t cwasm_fsio_fclose( uint32_t fh );
static int cwasm_fh_from_stream( FILE* stream, uint32_t* out_fh );
static FILE* cwasm_stream_from_fh( uint32_t fh );
static uint32_t cwasm_real_fh( uint32_t fh );

static int cwasm_obj_used_get( uint32_t fh ) {
    if( fh >= CWASM_STDIO_N ) { return 0; }
    #ifdef CWASM_THREADS
        return atomic_load_explicit( &cwasm_file_obj_used[ fh ], memory_order_acquire ) != 0;
    #else
        return cwasm_file_obj_used[ fh ] != 0;
    #endif
}

static void cwasm_obj_used_set( uint32_t fh, int used ) {
    if( fh >= CWASM_STDIO_N ) { return; }
    #ifdef CWASM_THREADS
        atomic_store_explicit( &cwasm_file_obj_used[ fh ], used ? 1u : 0u, memory_order_release );
    #else
        cwasm_file_obj_used[ fh ] = used ? 1u : 0u;
    #endif
}

static uint8_t cwasm_access_from_fsflags( unsigned f ) {
    int rd = ( f & CWASM_FS_OPEN_READ ) != 0;
    int wr = ( f & ( CWASM_FS_OPEN_WRITE | CWASM_FS_OPEN_CREATE |
        CWASM_FS_OPEN_TRUNCATE | CWASM_FS_OPEN_APPEND ) ) != 0;
    if( rd && wr ) { return CWASM_FH_RDWR; }
    if( wr ) { return CWASM_FH_WRONLY; }
    return CWASM_FH_RDONLY;
}

static int cwasm_mode_to_access( char const* mode, uint8_t* acc ) {
    int plus = 0;

    if( !mode || !mode[ 0 ] ) {
        errno = EINVAL;
        return -1;
    }
    for( size_t i = 1; mode[ i ]; ++i ) {
        if( mode[ i ] == '+' ) { plus = 1; }
        else if( mode[ i ] != 'b' && mode[ i ] != 'x' ) {
            errno = EINVAL;
            return -1;
        }
    }
    switch( mode[ 0 ] ) {
    case 'r':
        *acc = plus ? CWASM_FH_RDWR : CWASM_FH_RDONLY;
        break;
    case 'w':
    case 'a':
        *acc = plus ? CWASM_FH_RDWR : CWASM_FH_WRONLY;
        break;
    default:
        errno = EINVAL;
        return -1;
    }
    return 0;
}

static int cwasm_access_allows_read( uint8_t acc ) {
    return acc == CWASM_FH_RDONLY || acc == CWASM_FH_RDWR;
}

static int cwasm_access_allows_write( uint8_t acc ) {
    return acc == CWASM_FH_WRONLY || acc == CWASM_FH_RDWR;
}

static int cwasm_mode_allowed( char const* mode, uint8_t acc ) {
    uint8_t req;

    if( cwasm_mode_to_access( mode, &req ) != 0 ) { return 0; }
    if( mode[ 0 ] == 'r' ) { return cwasm_access_allows_read( acc ); }
    return cwasm_access_allows_write( acc );
}

static void cwasm_fh_clear_unget( uint32_t fh ) {
    if( fh < CWASM_STDIO_N ) { cwasm_fh_unget_len[ fh ] = 0; }
}

static void cwasm_fh_io_free( uint32_t fh ) {
    cwasm_fh_io_t* io;

    if( fh >= CWASM_STDIO_N ) { return; }
    io = &cwasm_fh_io[ fh ];
    if( io->owned && io->data ) { free( io->data ); }
    io->data = NULL;
    io->cap = io->len = io->pos = 0;
    io->owned = io->active = 0;
    io->vmode = 0;
}

static void cwasm_fh_io_defaults( uint32_t fh ) {
    cwasm_fh_io_t* io;

    if( fh >= CWASM_STDIO_N ) { return; }
    io = &cwasm_fh_io[ fh ];
    if( io->vmode != 0 ) { return; }
    if( fh == 0u && !cwasm_stdio_redirect[ 0 ] && !wfs_stdin_size() ) {
        io->vmode = _IONBF;
    } else if( fh == 1u ) { io->vmode = _IOLBF; }
    else if( fh == 2u ) { io->vmode = _IONBF; }
    else { io->vmode = _IOFBF; }
}

static void cwasm_fh_io_ensure( uint32_t fh ) {
    cwasm_fh_io_t* io;

    if( fh >= CWASM_STDIO_N ) { return; }
    io = &cwasm_fh_io[ fh ];
    cwasm_fh_io_defaults( fh );
    if( io->vmode == _IONBF || io->data ) { return; }
    io->cap = CWASM_STDIO_BUFSIZ;
    io->data = (char*)malloc( io->cap );
    io->owned = io->data ? 1u : 0u;
    if( !io->data ) {
        io->cap = 0;
        io->vmode = _IONBF;
    }
}

static int cwasm_raw_read( uint32_t fh, void* buf, size_t n ) {
    if( fh < CWASM_STDIO_N && cwasm_fh_dev[ fh ] == CWASM_FS_DEV_NULL ) {
        cwasm_fh_eof[ fh ] = 1;
        return 0;
    }
    if( fh < CWASM_STDIO_N && cwasm_fh_dev[ fh ] == CWASM_FS_DEV_RANDOM ) {
        if( !buf && n ) {
            errno = EINVAL;
            return -1;
        }
        if( n && getentropy( buf, n ) != 0 ) {
            cwasm_fh_err[ fh ] = 1;
            return -1;
        }
        return (int)n;
    }
    if( fh < CWASM_STDIO_N && cwasm_fh_wfs[ fh ] ) {
        int nread = cwasm_fs_read( &cwasm_fs_h[ fh ], buf, n );
        if( nread < 0 ) {
            cwasm_fh_err[ fh ] = 1;
            return -1;
        }
        if( n > 0 && nread == 0 ) { cwasm_fh_eof[ fh ] = 1; }
        return nread;
    }
    if( fh == 0u && !cwasm_stdio_redirect[ 0 ] ) {
        uint32_t stdin_size = wfs_stdin_size();
        if( stdin_size ) {
            uint32_t avail = cwasm_stdin_pos < stdin_size ? stdin_size - cwasm_stdin_pos : 0u;
            uint32_t got = n < avail ? (uint32_t)n : avail;
            if( got && buf ) { got = wfs_embed_read( cwasm_stdin_pos, buf, got ); }
            cwasm_stdin_pos += got;
            if( n > 0 && got == 0 ) { cwasm_fh_eof[ fh ] = 1; }
            return (int)got;
        }
        cwasm_fh_eof[ fh ] = 1;
        return 0;
    }
    cwasm_fh_err[ fh ] = 1;
    errno = EBADF;
    return -1;
}

static int cwasm_raw_write( uint32_t fh, void const* buf, size_t n ) {
    if( fh < CWASM_STDIO_N && cwasm_fh_dev[ fh ] == CWASM_FS_DEV_NULL ) {
        (void)buf;
        return (int)n;
    }
    if( fh < CWASM_STDIO_N && cwasm_fh_dev[ fh ] == CWASM_FS_DEV_RANDOM ) {
        errno = EACCES;
        cwasm_fh_err[ fh ] = 1;
        return -1;
    }
    if( fh < CWASM_STDIO_N && cwasm_fh_wfs[ fh ] ) {
        int wgot = cwasm_fs_write( &cwasm_fs_h[ fh ], buf, n );
        if( wgot < 0 ) {
            cwasm_fh_err[ fh ] = 1;
            return -1;
        }
        return wgot;
    }
    if( fh == 1u || fh == 2u ) {
        if( js_stdio_write( (uint32_t)(uintptr_t)buf, (uint32_t)n ) != 0 ) {
            cwasm_fh_err[ fh ] = 1;
            errno = EIO;
            return -1;
        }
        return (int)n;
    }
    cwasm_fh_err[ fh ] = 1;
    errno = EBADF;
    return -1;
}

static int cwasm_fh_flush_out( uint32_t fh ) {
    cwasm_fh_io_t* io = &cwasm_fh_io[ fh ];
    size_t off = 0;

    if( fh >= CWASM_STDIO_N || io->active != 2u || io->len == 0 ) { return 0; }
    while( off < io->len ) {
        int w = cwasm_raw_write( fh, io->data + off, io->len - off );
        if( w <= 0 ) { return -1; }
        off += (size_t)w;
    }
    io->len = 0;
    return 0;
}

static int cwasm_fh_discard_in( uint32_t fh ) {
    cwasm_fh_io_t* io = &cwasm_fh_io[ fh ];

    if( fh >= CWASM_STDIO_N ) { return 0; }
    io->active = 0;
    io->len = io->pos = 0;
    return 0;
}

static int cwasm_fh_fill_in( uint32_t fh ) {
    cwasm_fh_io_t* io = &cwasm_fh_io[ fh ];
    int n;

    if( io->active == 2u ) {
        if( cwasm_fh_flush_out( fh ) != 0 ) { return -1; }
    }
    io->active = 1u;
    io->pos = io->len = 0;
    if( io->vmode == _IONBF || !io->data || io->cap == 0 ) { return 0; }
    n = cwasm_raw_read( fh, io->data, io->cap );
    if( n < 0 ) { return -1; }
    io->len = (size_t)n;
    return (int)io->len;
}

static int cwasm_fh_write_bytes( uint32_t fh, void const* buf, size_t n ) {
    cwasm_fh_io_t* io = &cwasm_fh_io[ fh ];
    uint8_t const* p = (uint8_t const*)buf;
    size_t i = 0;

    if( io->active == 1u ) { cwasm_fh_discard_in( fh ); }

    if( io->vmode == _IONBF || !io->data || io->cap == 0 ) {
        if( cwasm_raw_write( fh, buf, n ) < 0 ) { return -1; }
        return 0;
    }

    io->active = 2u;
    while( i < n ) {
        size_t chunk = n - i;
        size_t room = io->cap - io->len;

        if( room == 0 ) {
            if( cwasm_fh_flush_out( fh ) != 0 ) { return -1; }
            room = io->cap;
        }
        if( chunk > room ) { chunk = room; }
        memcpy( io->data + io->len, p + i, chunk );
        io->len += chunk;
        i += chunk;

        if( io->vmode == _IOLBF ) {
            for( ;; ) {
                char* nl = (char*)memchr( io->data, '\n', io->len );
                if( !nl ) { break; }
                size_t upto = (size_t)( nl - io->data ) + 1u;
                if( cwasm_raw_write( fh, io->data, upto ) < 0 ) { return -1; }
                io->len -= upto;
                if( io->len ) { memmove( io->data, io->data + upto, io->len ); }
            }
            continue;
        }

        if( io->vmode == _IOFBF && io->len >= io->cap ) {
            if( cwasm_fh_flush_out( fh ) != 0 ) { return -1; }
        }
    }
    return 0;
}

// Caller must hold g_stdio_lock
static int cwasm_fsio_alloc_fh( uint32_t* out ) {
    uint32_t h;

    for( h = 0; h < CWASM_STDIO_N; ++h ) {
        if( h == 3u ) { continue; }
        if( !cwasm_file_valid[ h ] && !cwasm_obj_used_get( h ) &&
            !cwasm_dir_fd_is_dir_unlocked( (int)h ) ) {
            *out = h;
            return 0;
        }
    }
    errno = EMFILE;
    return -1;
}

// Caller must hold g_stdio_lock
static uint32_t cwasm_fsio_fopen_flags( uint32_t path, uint32_t flags, mode_t mode ) {
    uint32_t h;
    char const* ps = (char const*)(uintptr_t)path;
    cwasm_fs_handle_t wh;
    int dev;
    size_t plen;

    cwasm_stdio_init_unlocked();
    if( !ps ) {
        errno = EINVAL;
        return (uint32_t)-1;
    }

    dev = cwasm_fs_dev_kind( ps );
    if( dev ) {
        if( dev == CWASM_FS_DEV_RANDOM ) {
            if( !( flags & CWASM_FS_OPEN_READ ) || ( flags & CWASM_FS_OPEN_WRITE ) ) {
                errno = EACCES;
                return (uint32_t)-1;
            }
        } else if( !( flags & ( CWASM_FS_OPEN_READ | CWASM_FS_OPEN_WRITE ) ) ) {
            errno = EINVAL;
            return (uint32_t)-1;
        }
        if( cwasm_fsio_alloc_fh( &h ) != 0 ) { return (uint32_t)-1; }
        plen = strlen( ps );
        cwasm_fh_wfs[ h ] = 0;
        cwasm_fh_dev[ h ] = (uint8_t)dev;
        cwasm_file_valid[ h ] = 1;
        cwasm_fh_access[ h ] = cwasm_access_from_fsflags( flags );
        cwasm_fh_tty[ h ] = 0;
        cwasm_fh_eof[ h ] = 0;
        cwasm_fh_err[ h ] = 0;
        cwasm_fh_io_defaults( h );
        free( cwasm_fh_path[ h ] );
        cwasm_fh_path[ h ] = malloc( plen + 1u );
        if( cwasm_fh_path[ h ] ) { memcpy( cwasm_fh_path[ h ], ps, plen + 1u ); }
        return h;
    }

    cwasm_unlock( &g_stdio_lock );
    if( cwasm_fs_open( ps, flags, mode, &wh ) != 0 ) {
        cwasm_lock( &g_stdio_lock );
        return (uint32_t)-1;
    }
    cwasm_lock( &g_stdio_lock );
    cwasm_stdio_init_unlocked();

    if( cwasm_fsio_alloc_fh( &h ) != 0 ) {
        cwasm_fs_close( &wh );
        return (uint32_t)-1;
    }
    plen = strlen( ps );
    cwasm_fs_h[ h ] = wh;
    cwasm_fh_wfs[ h ] = 1;
    cwasm_fh_dev[ h ] = CWASM_FS_DEV_NONE;
    cwasm_file_valid[ h ] = 1;
    cwasm_fh_access[ h ] = cwasm_access_from_fsflags( flags );
    cwasm_fh_tty[ h ] = 0;
    cwasm_fh_io_defaults( h );
    free( cwasm_fh_path[ h ] );
    cwasm_fh_path[ h ] = malloc( plen + 1u );
    if( cwasm_fh_path[ h ] ) { memcpy( cwasm_fh_path[ h ], ps, plen + 1u ); }
    return h;
}

static uint32_t cwasm_fsio_fopen( uint32_t path, uint32_t mode ) {
    unsigned f = 0;
    char const* m = (char const*)(uintptr_t)mode;
    int plus = 0;
    int excl = 0;

    if( !m || !m[ 0 ] ) {
        errno = EINVAL;
        return (uint32_t)-1;
    }
    for( size_t i = 1; m[ i ]; ++i ) {
        if( m[ i ] == '+' ) { plus = 1; }
        else if( m[ i ] == 'x' ) { excl = 1; }
        else if( m[ i ] != 'b' ) {
            errno = EINVAL;
            return (uint32_t)-1;
        }
    }
    if( m[ 0 ] == 'r' ) {
        f = CWASM_FS_OPEN_READ;
        if( plus ) { f |= CWASM_FS_OPEN_WRITE; }
    } else if( m[ 0 ] == 'w' ) {
        f = CWASM_FS_OPEN_WRITE | CWASM_FS_OPEN_CREATE | CWASM_FS_OPEN_TRUNCATE;
        if( plus ) { f |= CWASM_FS_OPEN_READ; }
        if( excl ) { f |= CWASM_FS_OPEN_EXCLUSIVE; }
    } else if( m[ 0 ] == 'a' ) {
        f = CWASM_FS_OPEN_WRITE | CWASM_FS_OPEN_CREATE | CWASM_FS_OPEN_APPEND;
        if( plus ) { f |= CWASM_FS_OPEN_READ; }
        if( excl ) { f |= CWASM_FS_OPEN_EXCLUSIVE; }
    } else {
        errno = EINVAL;
        return (uint32_t)-1;
    }
    return cwasm_fsio_fopen_flags( path, f, 0666u );
}

// Caller must hold g_stdio_lock
static uint32_t cwasm_fsio_fclose( uint32_t fh ) {
    if( fh <= 2u && cwasm_stdio_redirect[ fh ] != 0u ) {
        fh = cwasm_stdio_redirect[ fh ];
    }
    if( fh >= CWASM_STDIO_N ) { return (uint32_t)-1; }
    if( !cwasm_file_valid[ fh ] && fh > 2u ) { return (uint32_t)-1; }
    if( fh <= 2u && !cwasm_file_valid[ fh ] ) { return (uint32_t)-1; }
    if( fh <= 2u && !cwasm_fh_wfs[ fh ] && !cwasm_fh_dev[ fh ] ) {
        (void)cwasm_fh_flush_out( fh );
        if( fh == 0u ) { cwasm_stdin_pos = 0; }
        cwasm_fh_eof[ fh ] = 0;
        cwasm_fh_err[ fh ] = 0;
        cwasm_fh_clear_unget( fh );
        cwasm_fh_discard_in( fh );
        cwasm_fh_orient_reset( fh );
        return 0;
    }
    if( fh < CWASM_STDIO_N && cwasm_fh_wfs[ fh ] ) {
        (void)cwasm_fh_flush_out( fh );
        cwasm_fs_close( &cwasm_fs_h[ fh ] );
        cwasm_fh_wfs[ fh ] = 0;
        cwasm_fh_dev[ fh ] = CWASM_FS_DEV_NONE;
        cwasm_file_valid[ fh ] = 0;
        cwasm_obj_used_set( fh, 0 );
        cwasm_fh_eof[ fh ] = 0;
        cwasm_fh_err[ fh ] = 0;
        cwasm_fh_tty[ fh ] = 0;
        cwasm_fh_clear_unget( fh );
        cwasm_fh_io_free( fh );
        free( cwasm_fh_path[ fh ] );
        cwasm_fh_path[ fh ] = NULL;
        cwasm_fh_orient_reset( fh );
        return 0;
    }
    if( fh < CWASM_STDIO_N ) {
        cwasm_fh_dev[ fh ] = CWASM_FS_DEV_NONE;
        cwasm_file_valid[ fh ] = 0;
        cwasm_obj_used_set( fh, 0 );
        cwasm_fh_eof[ fh ] = 0;
        cwasm_fh_err[ fh ] = 0;
        cwasm_fh_tty[ fh ] = 0;
        cwasm_fh_clear_unget( fh );
        cwasm_fh_io_free( fh );
        free( cwasm_fh_path[ fh ] );
        cwasm_fh_path[ fh ] = NULL;
        cwasm_fh_orient_reset( fh );
    }
    return 0;
}

// Caller must hold g_stdio_lock (+ fh lock)
static uint32_t cwasm_fsio_fclose_keep_file( uint32_t fh ) {
    if( fh >= CWASM_STDIO_N || fh <= 2u ) { return (uint32_t)-1; }
    if( !cwasm_file_valid[ fh ] ) { return (uint32_t)-1; }
    if( cwasm_fh_wfs[ fh ] ) {
        (void)cwasm_fh_flush_out( fh );
        cwasm_fs_close( &cwasm_fs_h[ fh ] );
        cwasm_fh_wfs[ fh ] = 0;
    }
    cwasm_fh_dev[ fh ] = CWASM_FS_DEV_NONE;
    cwasm_file_valid[ fh ] = 0;
    // obj_used stays 1 - slot reserved for this FILE*.
    cwasm_fh_eof[ fh ] = 0;
    cwasm_fh_err[ fh ] = 0;
    cwasm_fh_tty[ fh ] = 0;
    cwasm_fh_clear_unget( fh );
    cwasm_fh_io_free( fh );
    free( cwasm_fh_path[ fh ] );
    cwasm_fh_path[ fh ] = NULL;
    cwasm_fh_orient_reset( fh );
    __cwasm_wide_reset_handle( fh );
    return 0;
}

// Caller holds g_stdio_lock
static void cwasm_fsio_rebind_fh( uint32_t from, uint32_t to ) {
    if( from >= CWASM_STDIO_N || to >= CWASM_STDIO_N || from == to ) { return; }

    cwasm_fs_h[ to ] = cwasm_fs_h[ from ];
    memset( &cwasm_fs_h[ from ], 0, sizeof( cwasm_fs_h[ from ] ) );
    cwasm_fh_wfs[ to ] = cwasm_fh_wfs[ from ];
    cwasm_fh_wfs[ from ] = 0;
    cwasm_fh_dev[ to ] = cwasm_fh_dev[ from ];
    cwasm_fh_dev[ from ] = CWASM_FS_DEV_NONE;
    cwasm_fh_access[ to ] = cwasm_fh_access[ from ];
    cwasm_fh_access[ from ] = 0;
    cwasm_fh_tty[ to ] = cwasm_fh_tty[ from ];
    cwasm_fh_tty[ from ] = 0;
    cwasm_fh_eof[ to ] = cwasm_fh_eof[ from ];
    cwasm_fh_eof[ from ] = 0;
    cwasm_fh_err[ to ] = cwasm_fh_err[ from ];
    cwasm_fh_err[ from ] = 0;
    cwasm_fh_io[ to ] = cwasm_fh_io[ from ];
    memset( &cwasm_fh_io[ from ], 0, sizeof( cwasm_fh_io[ from ] ) );
    cwasm_fh_path[ to ] = cwasm_fh_path[ from ];
    cwasm_fh_path[ from ] = NULL;
    memcpy( cwasm_fh_unget_buf[ to ], cwasm_fh_unget_buf[ from ], sizeof( cwasm_fh_unget_buf[ 0 ] ) );
    cwasm_fh_unget_len[ to ] = cwasm_fh_unget_len[ from ];
    cwasm_fh_unget_len[ from ] = 0;
    cwasm_fh_orient[ to ] = cwasm_fh_orient[ from ];
    cwasm_fh_orient_reset( from );
    __cwasm_wide_rd_state[ to ] = __cwasm_wide_rd_state[ from ];
    __cwasm_wide_wr_state[ to ] = __cwasm_wide_wr_state[ from ];
    memcpy( __cwasm_wide_unget_buf[ to ], __cwasm_wide_unget_buf[ from ],
        sizeof( __cwasm_wide_unget_buf[ 0 ] ) );
    __cwasm_wide_unget_len[ to ] = __cwasm_wide_unget_len[ from ];
    __cwasm_wide_unget_nbytes[ to ] = __cwasm_wide_unget_nbytes[ from ];
    __cwasm_wide_reset_handle( from );

    cwasm_file_valid[ to ] = 1;
    cwasm_file_valid[ from ] = 0;
    cwasm_obj_used_set( from, 0 );
    cwasm_file_objs[ to ].__fh = to;
}

static uint32_t cwasm_fsio_fread( uint32_t buf, uint32_t sz, uint32_t nm, uint32_t fh ) {
    cwasm_fh_io_t* io;
    uint8_t* out;
    uint32_t total;
    uint32_t done = 0;

    if( fh <= 2u && cwasm_stdio_redirect[ fh ] != 0u ) {
        fh = cwasm_stdio_redirect[ fh ];
    }
    if( sz == 0 || nm == 0 ) { return 0; }
    if( nm > 0xFFFFFFFFu / sz ) {
        errno = EINVAL;
        return 0;
    }
    if( fh >= CWASM_STDIO_N || !cwasm_access_allows_read( cwasm_fh_access[ fh ] ) ) {
        if( fh < CWASM_STDIO_N ) { cwasm_fh_err[ fh ] = 1; }
        errno = EBADF;
        return 0;
    }

    total = sz * nm;
    out = (uint8_t*)(uintptr_t)buf;
    io = &cwasm_fh_io[ fh ];
    cwasm_fh_io_ensure( fh );

    while( done < total ) {
        if( cwasm_fh_unget_len[ fh ] > 0 ) {
            out[ done++ ] = cwasm_fh_unget_buf[ fh ][ --cwasm_fh_unget_len[ fh ] ];
            continue;
        }

        if( io->active == 1u && io->pos < io->len ) {
            size_t n = io->len - io->pos;
            if( n > total - done ) { n = total - done; }
            if( out ) { memcpy( out + done, io->data + io->pos, n ); }
            io->pos += n;
            done += (uint32_t)n;
            continue;
        }

        if( io->vmode != _IONBF && io->data && io->cap > 0 ) {
            if( cwasm_fh_fill_in( fh ) < 0 ) { return done / sz; }
            if( io->len > 0 ) { continue; }
        }

        uint32_t want = total - done;
        int nread = cwasm_raw_read( fh, out ? out + done : NULL, want );
        if( nread < 0 ) { return done / sz; }
        if( nread == 0 ) { break; }
        done += (uint32_t)nread;
        if( (uint32_t)nread < want ) { cwasm_fh_eof[ fh ] = 1; }
        break;
    }

    return done / sz;
}

static uint32_t cwasm_fsio_fwrite( uint32_t buf, uint32_t sz, uint32_t nm, uint32_t fh ) {
    uint32_t total;

    if( fh <= 2u && cwasm_stdio_redirect[ fh ] != 0u ) {
        fh = cwasm_stdio_redirect[ fh ];
    }
    if( sz == 0 || nm == 0 ) { return 0; }
    if( nm > 0xFFFFFFFFu / sz ) {
        errno = EINVAL;
        return 0;
    }
    if( fh >= CWASM_STDIO_N || !cwasm_access_allows_write( cwasm_fh_access[ fh ] ) ) {
        if( fh < CWASM_STDIO_N ) { cwasm_fh_err[ fh ] = 1; }
        errno = EBADF;
        return 0;
    }

    total = sz * nm;
    cwasm_fh_io_defaults( fh );
    if( cwasm_fh_write_bytes( fh, (void const*)(uintptr_t)buf, total ) != 0 ) {
        return 0;
    }
    return nm;
}

// Caller must hold g_stdio_lock
static void cwasm_stdio_init_unlocked( void ) {
    #ifdef CWASM_THREADS
        if( atomic_load_explicit( &cwasm_stdio_inited, memory_order_relaxed ) ) { return; }
    #else
        if( cwasm_stdio_inited ) { return; }
    #endif
    cwasm_fh_locks_init();
    cwasm_stdio_redirect[ 0 ] = 0;
    cwasm_stdio_redirect[ 1 ] = 0;
    cwasm_stdio_redirect[ 2 ] = 0;
    cwasm_file_valid[ 0 ] = 1;
    cwasm_file_valid[ 1 ] = 1;
    cwasm_file_valid[ 2 ] = 1;
    cwasm_obj_used_set( 0, 1 );
    cwasm_obj_used_set( 1, 1 );
    cwasm_obj_used_set( 2, 1 );
    cwasm_fh_access[ 0 ] = CWASM_FH_RDONLY;
    cwasm_fh_access[ 1 ] = CWASM_FH_WRONLY;
    cwasm_fh_access[ 2 ] = CWASM_FH_WRONLY;
    // --embed-stdin is piped file (not a tty), otherwise empty tty
    wfs_init();
    cwasm_fh_tty[ 0 ] = wfs_stdin_size() ? 0 : 1;
    cwasm_fh_tty[ 1 ] = 1;
    cwasm_fh_tty[ 2 ] = 1;
    cwasm_fh_io_defaults( 0 );
    cwasm_fh_io_defaults( 1 );
    cwasm_fh_io_defaults( 2 );
    #ifdef CWASM_THREADS
        atomic_store_explicit( &cwasm_stdio_inited, 1, memory_order_release );
    #else
        cwasm_stdio_inited = 1;
    #endif
}

// Lock order: table -> fh try, then drop table. Never block on fh while holding table.
// cwasm_table_await_fh drops and re-acquires the table lock - do not lock again.
static int cwasm_stream_lock( FILE* stream, uint32_t* out_fh ) {
    uint32_t logical;
    uint32_t real;

    if( cwasm_fh_from_stream( stream, &logical ) != 0 ) { return -1; }
    cwasm_lock( &g_stdio_lock );
    for( ;; ) {
        cwasm_stdio_init_unlocked();
        if( logical <= 2u && cwasm_stdio_reopen_busy[ logical ] ) {
            cwasm_unlock( &g_stdio_lock );
            #ifdef CWASM_THREADS
                sched_yield();
            #endif
            cwasm_lock( &g_stdio_lock );
            continue;
        }
        real = cwasm_real_fh( logical );
        if( real >= CWASM_STDIO_N || !cwasm_file_valid[ real ] ) {
            cwasm_unlock( &g_stdio_lock );
            errno = EBADF;
            return -1;
        }
        if( cwasm_fh_trylock( real ) == 0 ) {
            cwasm_unlock( &g_stdio_lock );
            *out_fh = real;
            return 0;
        }
        cwasm_table_await_fh( real );
    }
}

static int cwasm_fd_lock( int fd, uint32_t* out_fh ) {
    uint32_t real;

    if( fd < 0 ) {
        errno = EBADF;
        return -1;
    }
    cwasm_lock( &g_stdio_lock );
    for( ;; ) {
        cwasm_stdio_init_unlocked();
        if( (uint32_t)fd <= 2u && cwasm_stdio_reopen_busy[(uint32_t)fd ] ) {
            cwasm_unlock( &g_stdio_lock );
            #ifdef CWASM_THREADS
                sched_yield();
            #endif
            cwasm_lock( &g_stdio_lock );
            continue;
        }
        real = cwasm_real_fh( (uint32_t)fd );
        if( real >= CWASM_STDIO_N || !cwasm_file_valid[ real ] ) {
            cwasm_unlock( &g_stdio_lock );
            errno = EBADF;
            return -1;
        }
        if( cwasm_fh_trylock( real ) == 0 ) {
            cwasm_unlock( &g_stdio_lock );
            *out_fh = real;
            return 0;
        }
        cwasm_table_await_fh( real );
    }
}

#ifdef CWASM_THREADS
    static void cwasm_flock_note( uint32_t logical, uint32_t real ) {
        if( logical >= CWASM_STDIO_N ) { return; }
        if( g_flock_held[ logical ] ) {
            ++g_flock_depth[ logical ];
            return;
        }
        g_flock_held[ logical ] = 1;
        g_flock_real[ logical ] = real;
        g_flock_depth[ logical ] = 1;
    }

    static int cwasm_flock_release( uint32_t logical ) {
        if( logical >= CWASM_STDIO_N || !g_flock_held[ logical ] || g_flock_depth[ logical ] == 0 ) {
            return -1;
        }
        cwasm_fh_unlock( g_flock_real[ logical ] );
        if( --g_flock_depth[ logical ] == 0 ) { g_flock_held[ logical ] = 0; }
        return 0;
    }

    static void cwasm_flock_retarget( uint32_t logical, uint32_t new_real ) {
        uint32_t old_real;
        uint32_t depth;
        uint32_t i;

        if( logical >= CWASM_STDIO_N || !g_flock_held[ logical ] || g_flock_depth[ logical ] == 0 ) {
            return;
        }
        if( new_real >= CWASM_STDIO_N ) { return; }
        old_real = g_flock_real[ logical ];
        if( old_real == new_real ) { return; }
        depth = g_flock_depth[ logical ];
        // Lock new first - peers that resolve to new_real after publish cannot sneak in.
        for( i = 0; i < depth; ++i ) { cwasm_fh_lock( new_real ); }
        for( i = 0; i < depth; ++i ) { cwasm_fh_unlock( old_real ); }
        g_flock_real[ logical ] = new_real;
    }

    // Drop flockfile ownership entirely (unlock depth times). Never blocks - safe
    // under the table lock. Caller may still hold a separate fh lock for teardown.
    static void cwasm_flock_abandon( uint32_t logical ) {
        uint32_t old_real;
        uint32_t depth;
        uint32_t i;

        if( logical >= CWASM_STDIO_N || !g_flock_held[ logical ] || g_flock_depth[ logical ] == 0 ) {
            return;
        }
        old_real = g_flock_real[ logical ];
        depth = g_flock_depth[ logical ];
        g_flock_held[ logical ] = 0;
        g_flock_depth[ logical ] = 0;
        for( i = 0; i < depth; ++i ) { cwasm_fh_unlock( old_real ); }
    }
#endif

void flockfile( FILE* stream ) {
    uint32_t logical;
    uint32_t real;

    if( cwasm_fh_from_stream( stream, &logical ) != 0 ) { return; }
    #ifdef CWASM_THREADS
        if( logical < CWASM_STDIO_N && g_flock_held[ logical ] ) {
            cwasm_fh_lock( g_flock_real[ logical ] );
            ++g_flock_depth[ logical ];
            return;
        }
    #endif
    if( cwasm_stream_lock( stream, &real ) != 0 ) { return; }
    #ifdef CWASM_THREADS
        cwasm_flock_note( logical, real );
    #else
        (void)logical;
        (void)real;
    #endif
}

void funlockfile( FILE* stream ) {
    uint32_t logical;

    if( cwasm_fh_from_stream( stream, &logical ) != 0 ) { return; }
    #ifdef CWASM_THREADS
        (void)cwasm_flock_release( logical );
    #else
        if( !cwasm_stdio_inited ) { return; }
        cwasm_fh_unlock( cwasm_real_fh( logical ) );
    #endif
}

int ftrylockfile( FILE* stream ) {
    uint32_t logical;
    uint32_t real;
    int rc;

    if( cwasm_fh_from_stream( stream, &logical ) != 0 ) { return -1; }
    #ifdef CWASM_THREADS
        if( logical < CWASM_STDIO_N && g_flock_held[ logical ] ) {
            if( cwasm_fh_trylock( g_flock_real[ logical ] ) != 0 ) { return -1; }
            ++g_flock_depth[ logical ];
            return 0;
        }
    #endif
    cwasm_lock( &g_stdio_lock );
    cwasm_stdio_init_unlocked();
    if( logical <= 2u && cwasm_stdio_reopen_busy[ logical ] ) {
        cwasm_unlock( &g_stdio_lock );
        return -1;
    }
    real = cwasm_real_fh( logical );
    if( real >= CWASM_STDIO_N || !cwasm_file_valid[ real ] ) {
        cwasm_unlock( &g_stdio_lock );
        return -1;
    }
    rc = cwasm_fh_trylock( real );
    cwasm_unlock( &g_stdio_lock );
    if( rc != 0 ) { return -1; }
    #ifdef CWASM_THREADS
        cwasm_flock_note( logical, real );
    #endif
    return 0;
}

static uint32_t cwasm_real_fh( uint32_t fh ) {
    if( fh <= 2u && cwasm_stdio_redirect[ fh ] != 0u ) {
        return cwasm_stdio_redirect[ fh ];
    }
    return fh;
}

// Caller holds g_stdio_lock (or single-threaded / already inited under fh lock)
static int cwasm_fh_valid_unlocked( uint32_t fh ) {
    if( fh >= CWASM_STDIO_N ) { return 0; }
    if( fh == 3u ) { return 0; }
    return cwasm_file_valid[ fh ] != 0;
}

static int cwasm_resolve_path_buf( char const* in, char** out ) {
    if( !in ) {
        errno = EINVAL;
        return -1;
    }
    if( cwasm_fs_resolve( in, cwasm_resolve_buf, sizeof( cwasm_resolve_buf ) ) != 0 ) {
        return -1;
    }
    *out = cwasm_resolve_buf;
    return 0;
}

// Caller must already hold the stream's fh lock (flockfile / stream_lock).
// Do not call cwasm_stdio_init here - that would take the table lock under an fh
// lock and invert the documented lock order.
static int cwasm_require_fh( uint32_t fh ) {
    if( !cwasm_fh_valid_unlocked( fh ) ) {
        errno = EBADF;
        return -1;
    }
    return 0;
}

int cwasm_stdio_fh_is_open( uint32_t fh ) {
    int open;
    if( fh >= CWASM_STDIO_N ) { return 0; }
    cwasm_lock( &g_stdio_lock );
    // Fd number taken by an open stdio slot (FILE optional - redirect targets
    // may be valid without a published FILE* to avoid dual-handle orient)
    open = cwasm_file_valid[ fh ] != 0;
    cwasm_unlock( &g_stdio_lock );
    return open;
}

// Caller holds g_stdio_lock
int cwasm_stdio_fh_is_open_unlocked( uint32_t fh ) {
    if( fh >= CWASM_STDIO_N ) { return 0; }
    return cwasm_file_valid[ fh ] != 0;
}

void cwasm_stdio_table_lock( void ) { cwasm_lock( &g_stdio_lock ); }
void cwasm_stdio_table_unlock( void ) { cwasm_unlock( &g_stdio_lock ); }

static int cwasm_fh_from_stream( FILE* stream, uint32_t* out_fh ) {
    uintptr_t p;
    uintptr_t base;
    uintptr_t end;
    uint32_t idx;

    if( !stream || !out_fh ) {
        errno = EINVAL;
        return -1;
    }
    if( stream == &__cwasm_stdin ) {
        *out_fh = 0u;
        return 0;
    }
    if( stream == &__cwasm_stdout ) {
        *out_fh = 1u;
        return 0;
    }
    if( stream == &__cwasm_stderr ) {
        *out_fh = 2u;
        return 0;
    }
    p = (uintptr_t)stream;
    base = (uintptr_t)&cwasm_file_objs[ 0 ];
    end = (uintptr_t)&cwasm_file_objs[ CWASM_STDIO_N ];
    if( p < base || p >= end || ( ( p - base ) % sizeof( FILE ) ) != 0u ) {
        errno = EBADF;
        return -1;
    }
    idx = (uint32_t)( ( p - base ) / sizeof( FILE ) );
    if( idx < 4u || idx >= CWASM_STDIO_N || !cwasm_obj_used_get( idx ) ) {
        errno = EBADF;
        return -1;
    }
    *out_fh = idx;
    return 0;
}

int fwide( FILE* stream, int mode ) {
    uint32_t logical;
    uint32_t real;
    int cur;
    int rc;

    if( cwasm_fh_from_stream( stream, &logical ) != 0 ) { return -1; }
    if( cwasm_stream_lock( stream, &real ) != 0 ) { return -1; }
    (void)real;
    cur = cwasm_fh_orient_fwide_value( logical );

    if( mode > 0 ) {
        if( cur < 0 ) { rc = cur; }
        else {
            if( cur == 0 ) { cwasm_fh_orient[ logical ] = CWASM_FH_ORIENT_WIDE; }
            rc = 1;
        }
    } else if( mode < 0 ) {
        if( cur > 0 ) { rc = cur; }
        else {
            if( cur == 0 ) { cwasm_fh_orient[ logical ] = CWASM_FH_ORIENT_BYTE; }
            rc = -1;
        }
    } else {
        rc = cur;
    }
    cwasm_fh_unlock( real );
    return rc;
}

// Caller must hold the stream's fh lock (or use public wrappers)
int __cwasm_stdio_orient_byte( FILE* stream ) {
    uint32_t fh;

    if( cwasm_fh_from_stream( stream, &fh ) != 0 ) {
        if( !errno ) { errno = EBADF; }
        return -1;
    }
    if( cwasm_require_fh( fh ) != 0 ) {
        if( !errno ) { errno = EBADF; }
        return -1;
    }
    if( cwasm_fh_orient[ fh ] == CWASM_FH_ORIENT_WIDE ) {
        errno = EBADF;
        return -1;
    }
    if( cwasm_fh_orient[ fh ] == CWASM_FH_ORIENT_NONE ) {
        cwasm_fh_orient[ fh ] = CWASM_FH_ORIENT_BYTE;
    }
    return 0;
}

int __cwasm_stdio_orient_wide( FILE* stream ) {
    uint32_t fh;

    if( cwasm_fh_from_stream( stream, &fh ) != 0 ) {
        if( !errno ) { errno = EBADF; }
        return -1;
    }
    if( cwasm_require_fh( fh ) != 0 ) {
        if( !errno ) { errno = EBADF; }
        return -1;
    }
    if( cwasm_fh_orient[ fh ] == CWASM_FH_ORIENT_BYTE ) {
        errno = EBADF;
        return -1;
    }
    if( cwasm_fh_orient[ fh ] == CWASM_FH_ORIENT_NONE ) {
        cwasm_fh_orient[ fh ] = CWASM_FH_ORIENT_WIDE;
    }
    return 0;
}

uint32_t __cwasm_stdio_fh( FILE* stream ) {
    uint32_t fh;

    if( cwasm_fh_from_stream( stream, &fh ) != 0 ) { return UINT32_MAX; }
    return fh;
}

void __cwasm_stdio_clear_eof( FILE* stream ) {
    uint32_t fh = 0;

    if( cwasm_fh_from_stream( stream, &fh ) != 0 ) { return; }
    fh = cwasm_real_fh( fh );
    if( fh < CWASM_STDIO_N ) { cwasm_fh_eof[ fh ] = 0; }
}

static FILE* cwasm_stream_from_fh( uint32_t fh ) {
    if( fh == 0u ) { return &__cwasm_stdin; }
    if( fh == 1u ) { return &__cwasm_stdout; }
    if( fh == 2u ) { return &__cwasm_stderr; }
    if( fh < 4u || fh >= CWASM_STDIO_N ) { return NULL; }
    if( !cwasm_obj_used_get( fh ) ) { return NULL; }
    return &cwasm_file_objs[ fh ];
}

FILE* fdopen( int fd, char const* mode ) {
    FILE* out;

    if( fd < 0 || fd >= (int)CWASM_STDIO_N ) {
        errno = EINVAL;
        return NULL;
    }
    if( !mode ) {
        errno = EINVAL;
        return NULL;
    }
    cwasm_lock( &g_stdio_lock );
    for( ;; ) {
        cwasm_stdio_init_unlocked();
        if( !cwasm_fh_valid_unlocked( (uint32_t)fd ) ) {
            cwasm_unlock( &g_stdio_lock );
            errno = EBADF;
            return NULL;
        }
        if( !cwasm_mode_allowed( mode, cwasm_fh_access[(uint32_t)fd ] ) ) {
            cwasm_unlock( &g_stdio_lock );
            errno = EBADF;
            return NULL;
        }
        if( cwasm_fh_trylock( (uint32_t)fd ) != 0 ) {
            cwasm_table_await_fh( (uint32_t)fd );
            continue;
        }
        if( !cwasm_obj_used_get( (uint32_t)fd ) ) {
            cwasm_obj_used_set( (uint32_t)fd, 1 );
            cwasm_file_objs[(uint32_t)fd ].__fh = (uint32_t)fd;
        }
        cwasm_fh_orient_reset( (uint32_t)fd );
        out = cwasm_stream_from_fh( (uint32_t)fd );
        cwasm_fh_unlock( (uint32_t)fd );
        cwasm_unlock( &g_stdio_lock );
        return out;
    }
}

int cwasm_stdio_fstat( uint32_t fh, struct stat* st ) {
    uint32_t real;
    int rc = -1;

    if( !st ) {
        errno = EINVAL;
        return -1;
    }
    if( cwasm_fd_lock( (int)fh, &real ) != 0 ) { return -1; }
    if( real < CWASM_STDIO_N && cwasm_fh_dev[ real ] ) {
        char const* p = cwasm_fh_path[ real ];
        if( p ) { rc = cwasm_fs_dev_stat( p, st ); }
        else {
            memset( st, 0, sizeof( *st ) );
            st->st_mode = (mode_t)( S_IFCHR | 0444u );
            st->st_nlink = 1;
            rc = 0;
        }
        goto out;
    }
    if( real < CWASM_STDIO_N && cwasm_fh_wfs[ real ] ) {
        rc = cwasm_fs_fstat( &cwasm_fs_h[ real ], st );
        goto out;
    }
    if( real == 0u && !cwasm_stdio_redirect[ 0 ] ) {
        uint32_t stdin_size = wfs_stdin_size();
        memset( st, 0, sizeof( *st ) );
        if( stdin_size ) {
            st->st_mode = S_IFREG | 0444u;
            st->st_size = (long)stdin_size;
        } else {
            st->st_mode = S_IFCHR | 0666u;
        }
        st->st_nlink = 1;
        rc = 0;
        goto out;
    }
    if( real <= 2u ) {
        memset( st, 0, sizeof( *st ) );
        st->st_mode = S_IFCHR | 0666u;
        st->st_nlink = 1;
        rc = 0;
        goto out;
    }
    errno = EBADF;
out:
    cwasm_fh_unlock( real );
    return rc;
}

int cwasm_stdio_ftruncate( uint32_t fh, off_t length ) {
    int rc;
    uint32_t real;

    if( cwasm_fd_lock( (int)fh, &real ) != 0 ) { return -1; }
    if( real <= 2u || real >= CWASM_STDIO_N || !cwasm_fh_wfs[ real ] ) {
        errno = EBADF;
        cwasm_fh_unlock( real );
        return -1;
    }
    rc = cwasm_fs_ftruncate( &cwasm_fs_h[ real ], length );
    cwasm_fh_unlock( real );
    return rc;
}

int cwasm_stdio_fflush( uint32_t fh ) {
    fh = cwasm_real_fh( fh );
    if( fh >= CWASM_STDIO_N ) { return -1; }
    if( cwasm_fh_flush_out( fh ) != 0 ) { return -1; }
    if( fh == 1u || fh == 2u ) { return js_stdio_flush() == 0 ? 0 : -1; }
    if( fh <= 2u ) { return 0; }
    if( cwasm_fh_dev[ fh ] ) { return 0; }
    if( !cwasm_fh_wfs[ fh ] ) { return -1; }
    return cwasm_fs_flush( &cwasm_fs_h[ fh ] );
}

int cwasm_stdio_isatty( int fd ) {
    int r = 0;

    if( fd < 0 || fd >= (int)CWASM_STDIO_N ) {
        errno = EBADF;
        return 0;
    }
    cwasm_lock( &g_stdio_lock );
    cwasm_stdio_init_unlocked();
    if( (uint32_t)fd <= 2u && cwasm_stdio_redirect[(uint32_t)fd ] != 0u ) { r = 0; }
    else if( (uint32_t)fd <= 2u ) { r = cwasm_fh_tty[(uint32_t)fd ] ? 1 : 0; }
    else { r = 0; }
    cwasm_unlock( &g_stdio_lock );
    return r;
}

ssize_t cwasm_stdio_read( int fd, void* buf, size_t count ) {
    uint32_t fh;
    uint32_t got;
    ssize_t rc;
    int had_err;

    if( !buf && count ) {
        errno = EINVAL;
        return -1;
    }
    if( cwasm_fd_lock( fd, &fh ) != 0 ) { return -1; }
    if( !cwasm_access_allows_read( cwasm_fh_access[ fh ] ) ) {
        errno = EBADF;
        cwasm_fh_unlock( fh );
        return -1;
    }

    had_err = cwasm_fh_err[ fh ];
    got = cwasm_fsio_fread( (uint32_t)(uintptr_t)buf, 1, (uint32_t)count, fh );
    if( got == 0 && count > 0 && cwasm_fh_err[ fh ] && !had_err ) { rc = -1; }
    else { rc = (ssize_t)got; }
    cwasm_fh_unlock( fh );
    return rc;
}

ssize_t cwasm_stdio_write( int fd, void const* buf, size_t count ) {
    uint32_t fh;
    uint32_t got;
    ssize_t rc;
    int had_err;

    if( !buf && count ) {
        errno = EINVAL;
        return -1;
    }
    if( cwasm_fd_lock( fd, &fh ) != 0 ) { return -1; }
    if( !cwasm_access_allows_write( cwasm_fh_access[ fh ] ) ) {
        errno = EBADF;
        cwasm_fh_unlock( fh );
        return -1;
    }
    had_err = cwasm_fh_err[ fh ];
    got = cwasm_fsio_fwrite( (uint32_t)(uintptr_t)buf, 1, (uint32_t)count, fh );
    if( got == 0 && count > 0 && cwasm_fh_err[ fh ] && !had_err ) { rc = -1; }
    else { rc = (ssize_t)got; }
    cwasm_fh_unlock( fh );
    return rc;
}

static void cwasm_stdio_restore_console_tty( uint32_t fh ) {
    if( fh > 2u ) { return; }
    cwasm_fh_tty[ fh ] = ( fh == 0u && wfs_stdin_size() ) ? 0 : 1;
}

// Caller holds g_stdio_lock
static void cwasm_stdio_clear_redirects_to( uint32_t real_fh ) {
    uint32_t i;

    if( real_fh >= CWASM_STDIO_N ) { return; }
    for( i = 0; i < 3u; ++i ) {
        if( cwasm_stdio_redirect[ i ] != real_fh ) { continue; }
        cwasm_stdio_redirect[ i ] = 0;
        cwasm_stdio_restore_console_tty( i );
        #ifdef CWASM_THREADS
            cwasm_flock_abandon( i );
        #endif
    }
}

int cwasm_stdio_close_fd( int fd ) {
    uint32_t fh;
    int rc;

    if( fd < 0 ) {
        errno = EINVAL;
        return -1;
    }
    cwasm_lock( &g_stdio_lock );
    for( ;; ) {
        cwasm_stdio_init_unlocked();
        if( !cwasm_fh_valid_unlocked( (uint32_t)fd ) ) {
            cwasm_unlock( &g_stdio_lock );
            errno = EBADF;
            return -1;
        }
        fh = cwasm_real_fh( (uint32_t)fd );
        if( cwasm_fh_trylock( fh ) != 0 ) {
            cwasm_table_await_fh( fh );
            continue;
        }
        rc = cwasm_fsio_fclose( (uint32_t)fd ) == 0 ? 0 : -1;
        if( rc == 0 ) {
            cwasm_stdio_clear_redirects_to( fh );
            if( (uint32_t)fd > 2u && (uint32_t)fd < CWASM_STDIO_N ) {
                cwasm_obj_used_set( (uint32_t)fd, 0 );
            }
        }
        #ifdef CWASM_THREADS
            cwasm_flock_abandon( (uint32_t)fd );
        #endif
        cwasm_fh_unlock( fh );
        cwasm_unlock( &g_stdio_lock );
        return rc;
    }
}

off_t cwasm_stdio_lseek( int fd, off_t offset, int whence ) {
    uint32_t fh;
    uint32_t logical = (uint32_t)fd;
    off_t rc = (off_t)-1;

    if( cwasm_fd_lock( fd, &fh ) != 0 ) { return (off_t)-1; }
    if( cwasm_fh_flush_out( fh ) != 0 ) { goto out; }
    if( fh < CWASM_STDIO_N && cwasm_fh_wfs[ fh ] ) {
        if( offset < LONG_MIN || offset > LONG_MAX ) {
            errno = EINVAL;
            goto out;
        }
        if( cwasm_fs_seek( &cwasm_fs_h[ fh ], (long)offset, whence ) < 0 ) { goto out; }
        cwasm_fh_eof[ fh ] = 0;
        cwasm_fh_clear_unget( fh );
        cwasm_fh_discard_in( fh );
        if( logical < CWASM_STDIO_N ) { __cwasm_wide_reset_handle( logical ); }
        rc = cwasm_fs_tell( &cwasm_fs_h[ fh ] );
        goto out;
    }
    if( fh == 0u && logical == 0u && !cwasm_stdio_redirect[ 0 ] && wfs_stdin_size() ) {
        uint32_t stdin_size = wfs_stdin_size();
        long pos;

        if( offset < LONG_MIN || offset > LONG_MAX ) {
            errno = EINVAL;
            goto out;
        }
        if( whence == SEEK_SET ) { pos = (long)offset; }
        else if( whence == SEEK_CUR ) { pos = (long)cwasm_stdin_pos + (long)offset; }
        else if( whence == SEEK_END ) { pos = (long)stdin_size + (long)offset; }
        else {
            errno = EINVAL;
            goto out;
        }
        if( pos < 0 || (uint32_t)pos > stdin_size ) {
            errno = EINVAL;
            goto out;
        }
        cwasm_stdin_pos = (uint32_t)pos;
        cwasm_fh_eof[ fh ] = 0;
        cwasm_fh_clear_unget( fh );
        cwasm_fh_discard_in( fh );
        __cwasm_wide_reset_handle( logical );
        rc = (off_t)pos;
        goto out;
    }
    errno = ESPIPE;
out:
    cwasm_fh_unlock( fh );
    return rc;
}

int cwasm_stdio_fchmod( uint32_t fh, mode_t mode ) {
    uint32_t real;
    int rc = -1;
    char const* path;

    if( cwasm_fd_lock( (int)fh, &real ) != 0 ) { return -1; }
    if( real <= 2u ) {
        errno = EBADF;
        goto out;
    }
    if( real >= CWASM_STDIO_N || !cwasm_fh_wfs[ real ] || !cwasm_fh_path[ real ] ) {
        errno = EBADF;
        goto out;
    }
    path = cwasm_fh_path[ real ];
    rc = cwasm_fs_chmod( path, mode );
out:
    cwasm_fh_unlock( real );
    return rc;
}

int cwasm_stdio_open( char const* resolved, unsigned flags, mode_t mode ) {
    uint32_t h;

    if( !resolved ) {
        errno = EINVAL;
        return -1;
    }
    cwasm_fs_init();
    cwasm_lock( &g_stdio_lock );
    cwasm_stdio_init_unlocked();
    h = cwasm_fsio_fopen_flags( (uint32_t)(uintptr_t)resolved, flags, mode );
    if( h == (uint32_t)-1 ) {
        cwasm_unlock( &g_stdio_lock );
        return -1;
    }
    cwasm_obj_used_set( h, 1 );
    cwasm_file_objs[ h ].__fh = h;
    __cwasm_wide_reset_handle( h );
    cwasm_fh_orient_reset( h );
    cwasm_unlock( &g_stdio_lock );
    return (int)h;
}

int remove( char const* filename ) {
    char const* path = cwasm_fs_resolve_buf( filename );
    if( !path ) { return -1; }
    return cwasm_fs_remove( path );
}

int rename( char const* old, char const* newname ) {
    char opath[ CWASM_FS_PATH_MAX ];
    char npath[ CWASM_FS_PATH_MAX ];

    if( !old || !newname ) {
        errno = EINVAL;
        return -1;
    }
    if( cwasm_fs_resolve( old, opath, sizeof( opath ) ) != 0 ) { return -1; }
    if( cwasm_fs_resolve( newname, npath, sizeof( npath ) ) != 0 ) { return -1; }
    return cwasm_fs_rename( opath, npath );
}

FILE* tmpfile( void ) {
    char path[ CWASM_FS_PATH_MAX ];
    unsigned char rnd[ 8 ];
    unsigned long long r = 0;
    FILE* f;
    int i;

    mkdir( "/?/appdata/.tmp", 0755 );

    for( i = 0; i < 100; ++i ) {
        uint32_t real;

        if( getentropy( rnd, sizeof( rnd ) ) != 0 ) { return NULL; }
        r = 0;
        for( size_t j = 0; j < sizeof( rnd ); ++j ) { r = ( r << 8 ) | rnd[ j ]; }
        snprintf( path, sizeof( path ), "/?/appdata/.tmp/t%016llx.tmp", r );
        f = fopen( path, "w+bx" );
        if( !f ) {
            if( errno != EEXIST ) { return NULL; }
            continue;
        }
        if( cwasm_stream_lock( f, &real ) != 0 ) { return f; }
        if( real < CWASM_STDIO_N && cwasm_fh_wfs[ real ] ) {
            cwasm_fs_h[ real ].unlink_on_close = 1;
        }
        cwasm_fh_unlock( real );
        return f;
    }
    errno = EEXIST;
    return NULL;
}

char* tmpnam( char* s ) {
    static char buf[ L_tmpnam ];
    unsigned char rnd[ 8 ];
    unsigned long long r = 0;
    int i;

    if( !s ) { s = buf; }

    mkdir( "/?/appdata/.tmp", 0755 );

    for( i = 0; i < 100; ++i ) {
        if( getentropy( rnd, sizeof( rnd ) ) != 0 ) { return NULL; }
        r = 0;
        for( size_t j = 0; j < sizeof( rnd ); ++j ) { r = ( r << 8 ) | rnd[ j ]; }
        snprintf( s, L_tmpnam, "/?/appdata/.tmp/t%016llx.tmp",
            (unsigned long long)r );
        if( access( s, F_OK ) != 0 ) {
            if( errno == ENOENT ) { return s; }
            return NULL;
        }
    }
    errno = EEXIST;
    return NULL;
}

FILE* fopen( char const* filename, char const* mode ) {
    char* path;
    uint32_t h;

    if( !filename || !mode ) {
        errno = EINVAL;
        return NULL;
    }
    // Warm FS/WFS before taking stdio locks - avoids fh/table -> FS once-init
    cwasm_fs_init();
    if( cwasm_resolve_path_buf( filename, &path ) != 0 ) { return NULL; }
    cwasm_lock( &g_stdio_lock );
    cwasm_stdio_init_unlocked();
    h = cwasm_fsio_fopen( (uint32_t)(uintptr_t)path, (uint32_t)(uintptr_t)mode );
    if( h == (uint32_t)-1 ) {
        cwasm_unlock( &g_stdio_lock );
        if( !errno ) { errno = ENOENT; }
        return NULL;
    }
    cwasm_obj_used_set( h, 1 );
    cwasm_file_objs[ h ].__fh = h;
    __cwasm_wide_reset_handle( h );
    cwasm_fh_orient_reset( h );
    cwasm_unlock( &g_stdio_lock );
    return &cwasm_file_objs[ h ];
}

FILE* freopen( char const* filename, char const* mode, FILE* stream ) {
    uint32_t fh = 0;
    uint32_t h = (uint32_t)-1;
    char* path = NULL;
    char path_local[ CWASM_FS_PATH_MAX ];

    if( !mode ) { errno = EINVAL; return NULL; }
    if( cwasm_fh_from_stream( stream, &fh ) != 0 ) { return NULL; }
    // Warm FS before fh locks - freopen drops fh across open, still avoid nesting
    // once-init under any residual stdio lock from the caller (flockfile)
    cwasm_fs_init();
    if( filename ) {
        if( cwasm_resolve_path_buf( filename, &path ) != 0 ) { return NULL; }
    }

    cwasm_lock( &g_stdio_lock );
    cwasm_stdio_init_unlocked();
    if( !filename && !cwasm_fh_valid_unlocked( fh ) ) {
        cwasm_unlock( &g_stdio_lock );
        return NULL;
    }

    if( fh <= 2u ) {
        for( ;; ) {
            uint32_t old_real = cwasm_real_fh( fh );

            if( cwasm_fh_trylock2( fh, old_real ) != 0 ) {
                cwasm_table_await_fh2( fh, old_real );
                cwasm_stdio_init_unlocked();
                continue;
            }
            if( cwasm_real_fh( fh ) != old_real ) {
                cwasm_fh_unlock2( fh, old_real );
                continue;
            }
            if( cwasm_stdio_reopen_busy[ fh ] ) {
                cwasm_fh_unlock2( fh, old_real );
                cwasm_unlock( &g_stdio_lock );
                #ifdef CWASM_THREADS
                    sched_yield();
                #endif
                cwasm_lock( &g_stdio_lock );
                cwasm_stdio_init_unlocked();
                continue;
            }

            if( !filename ) {
                char const* cur = NULL;

                if( cwasm_stdio_redirect[ fh ] && old_real < CWASM_STDIO_N ) {
                    cur = cwasm_fh_path[ old_real ];
                }
                if( !cur || !cur[ 0 ] ) {
                    cwasm_fh_unlock2( fh, old_real );
                    cwasm_unlock( &g_stdio_lock );
                    errno = ENODEV;
                    return NULL;
                }
                if( strlen( cur ) >= sizeof( path_local ) ) {
                    cwasm_fh_unlock2( fh, old_real );
                    cwasm_unlock( &g_stdio_lock );
                    errno = ENAMETOOLONG;
                    return NULL;
                }
                memcpy( path_local, cur, strlen( cur ) + 1u );
                path = path_local;
            }

            if( cwasm_stdio_redirect[ fh ] ) {
                (void)cwasm_fsio_fclose( cwasm_stdio_redirect[ fh ] );
                cwasm_stdio_redirect[ fh ] = 0;
            }

            #ifdef CWASM_THREADS
                cwasm_flock_retarget( fh, fh );
            #endif
            if( old_real != fh ) { cwasm_fh_unlock( old_real ); }

            cwasm_stdio_reopen_busy[ fh ] = 1;
            cwasm_fh_unlock( fh );

            h = cwasm_fsio_fopen( (uint32_t)(uintptr_t)path, (uint32_t)(uintptr_t)mode );
            if( h == (uint32_t)-1 ) {
                cwasm_stdio_reopen_busy[ fh ] = 0;
                cwasm_stdio_restore_console_tty( fh );
                cwasm_unlock( &g_stdio_lock );
                return NULL;
            }

            for( ;; ) {
                if( cwasm_fh_trylock2( fh, h ) == 0 ) { break; }
                cwasm_table_await_fh2( fh, h );
                cwasm_stdio_init_unlocked();
            }

            cwasm_stdio_redirect[ fh ] = h;
            cwasm_fh_tty[ fh ] = 0;
            __cwasm_wide_reset_handle( fh );
            cwasm_fh_orient_reset( fh );
            cwasm_stdio_reopen_busy[ fh ] = 0;

            #ifdef CWASM_THREADS
                cwasm_unlock( &g_stdio_lock );
                cwasm_flock_retarget( fh, h );
                cwasm_fh_unlock( fh );
                cwasm_fh_unlock( h );
            #else
                cwasm_fh_unlock( fh );
                cwasm_fh_unlock( h );
                cwasm_unlock( &g_stdio_lock );
            #endif
            return stream;
        }
    }

    for( ;; ) {
        if( cwasm_fh_trylock( fh ) != 0 ) {
            cwasm_table_await_fh( fh );
            cwasm_stdio_init_unlocked();
            continue;
        }

        if( !cwasm_file_valid[ fh ] ) {
            if( cwasm_obj_used_get( fh ) ) {
                cwasm_fh_unlock( fh );
                cwasm_unlock( &g_stdio_lock );
                errno = EBUSY;
                return NULL;
            }
            cwasm_fh_unlock( fh );
            cwasm_unlock( &g_stdio_lock );
            errno = EBADF;
            return NULL;
        }

        if( !filename ) {
            char const* cur = cwasm_fh_path[ fh ];

            if( !cur || !cur[ 0 ] ) {
                cwasm_fh_unlock( fh );
                cwasm_unlock( &g_stdio_lock );
                errno = ENODEV;
                return NULL;
            }
            if( strlen( cur ) >= sizeof( path_local ) ) {
                cwasm_fh_unlock( fh );
                cwasm_unlock( &g_stdio_lock );
                errno = ENAMETOOLONG;
                return NULL;
            }
            memcpy( path_local, cur, strlen( cur ) + 1u );
            path = path_local;
        }

        if( cwasm_fsio_fclose_keep_file( fh ) != 0 ) {
            cwasm_fh_unlock( fh );
            cwasm_unlock( &g_stdio_lock );
            return NULL;
        }
        cwasm_fh_unlock( fh );

        h = cwasm_fsio_fopen( (uint32_t)(uintptr_t)path, (uint32_t)(uintptr_t)mode );
        if( h == (uint32_t)-1 ) {
            cwasm_obj_used_set( fh, 0 );
            #ifdef CWASM_THREADS
                cwasm_flock_abandon( fh );
            #endif
            cwasm_unlock( &g_stdio_lock );
            return NULL;
        }

        for( ;; ) {
            if( cwasm_fh_trylock2( fh, h ) == 0 ) { break; }
            cwasm_table_await_fh2( fh, h );
            cwasm_stdio_init_unlocked();
        }
        if( h != fh ) { cwasm_fsio_rebind_fh( h, fh ); }
        else { cwasm_file_objs[ fh ].__fh = fh; }
        cwasm_obj_used_set( fh, 1 );
        __cwasm_wide_reset_handle( fh );
        cwasm_fh_orient_reset( fh );
        cwasm_fh_unlock2( fh, h );
        cwasm_unlock( &g_stdio_lock );
        return stream;
    }
}

int fclose( FILE* stream ) {
    uint32_t fh = 0;
    uint32_t rc;
    uint32_t lock_fh;

    if( cwasm_fh_from_stream( stream, &fh ) != 0 ) { return EOF; }

    cwasm_lock( &g_stdio_lock );
    for( ;; ) {
        cwasm_stdio_init_unlocked();
        if( fh <= 2u && cwasm_stdio_reopen_busy[ fh ] ) {
            cwasm_unlock( &g_stdio_lock );
            #ifdef CWASM_THREADS
                sched_yield();
            #endif
            cwasm_lock( &g_stdio_lock );
            continue;
        }
        if( !cwasm_fh_valid_unlocked( fh ) ) {
            cwasm_unlock( &g_stdio_lock );
            return EOF;
        }
        if( fh <= 2u && cwasm_stdio_redirect[ fh ] != 0u ) {
            uint32_t rh = cwasm_stdio_redirect[ fh ];
            if( cwasm_fh_trylock( rh ) != 0 ) {
                cwasm_table_await_fh( rh );
                continue;
            }
            rc = cwasm_fsio_fclose( rh );
            if( rc == 0 ) {
                cwasm_stdio_redirect[ fh ] = 0;
                cwasm_stdio_restore_console_tty( fh );
                if( rh < CWASM_STDIO_N ) { cwasm_obj_used_set( rh, 0 ); }
                __cwasm_wide_reset_handle( fh );
                #ifdef CWASM_THREADS
                    cwasm_flock_abandon( fh );
                #endif
                cwasm_fh_unlock( rh );
                cwasm_unlock( &g_stdio_lock );
                return 0;
            }
            cwasm_fh_unlock( rh );
            cwasm_unlock( &g_stdio_lock );
            return EOF;
        }
        lock_fh = cwasm_real_fh( fh );
        if( cwasm_fh_trylock( lock_fh ) != 0 ) {
            cwasm_table_await_fh( lock_fh );
            continue;
        }
        rc = cwasm_fsio_fclose( fh );
        if( rc == 0 && fh > 2u && fh < CWASM_STDIO_N ) { cwasm_obj_used_set( fh, 0 ); }
        if( rc == (uint32_t)-1 ) {
            cwasm_fh_unlock( lock_fh );
            cwasm_unlock( &g_stdio_lock );
            if( !errno ) { errno = EBADF; }
            return EOF;
        }
        #ifdef CWASM_THREADS
            cwasm_flock_abandon( fh );
        #endif
        __cwasm_wide_reset_handle( fh );
        cwasm_fh_unlock( lock_fh );
        cwasm_unlock( &g_stdio_lock );
        return (int)rc;
    }
}

int fflush( FILE* stream ) {
    uint32_t fh;
    int rc = 0;

    if( !stream ) {
        // Lock order stays table -> fh try, never hold table during I/O
        cwasm_lock( &g_stdio_lock );
        cwasm_stdio_init_unlocked();
        for( fh = 0; fh < CWASM_STDIO_N; ) {
            uint32_t real;
            int do_flush;

            if( fh > 2u && !cwasm_file_valid[ fh ] ) {
                ++fh;
                continue;
            }
            if( fh <= 2u && cwasm_stdio_redirect[ fh ] != 0u ) {
                ++fh;
                continue;
            }
            real = fh;
            if( cwasm_fh_trylock( real ) != 0 ) {
                cwasm_table_await_fh( real );
                cwasm_stdio_init_unlocked();
                continue;
            }
            do_flush = ( fh <= 2u || cwasm_file_valid[ fh ] );
            cwasm_unlock( &g_stdio_lock );
            if( do_flush && cwasm_stdio_fflush( real ) != 0 ) { rc = EOF; }
            cwasm_fh_unlock( real );
            cwasm_lock( &g_stdio_lock );
            cwasm_stdio_init_unlocked();
            ++fh;
        }
        cwasm_unlock( &g_stdio_lock );
        return rc;
    }

    if( cwasm_stream_lock( stream, &fh ) != 0 ) { return EOF; }
    rc = cwasm_stdio_fflush( fh ) == 0 ? 0 : EOF;
    cwasm_fh_unlock( fh );
    return rc;
}

int fileno( FILE* stream ) {
    uint32_t real;

    if( cwasm_stream_lock( stream, &real ) != 0 ) {
        errno = EBADF;
        return -1;
    }
    cwasm_fh_unlock( real );
    return (int)real;
}

void setbuf( FILE* stream, char* buf ) {
    if( buf ) { (void)setvbuf( stream, buf, _IOFBF, BUFSIZ ); }
    else { (void)setvbuf( stream, NULL, _IONBF, 0 ); }
}

int setvbuf( FILE* stream, char* buf, int mode, size_t size ) {
    uint32_t fh = 0;
    cwasm_fh_io_t* io;
    int rc = -1;

    if( mode != _IOFBF && mode != _IOLBF && mode != _IONBF ) {
        errno = EINVAL;
        return -1;
    }
    if( cwasm_stream_lock( stream, &fh ) != 0 ) { return -1; }
    if( fh >= CWASM_STDIO_N ) { goto out; }

    io = &cwasm_fh_io[ fh ];
    cwasm_fh_io_ensure( fh );
    if( cwasm_fh_flush_out( fh ) != 0 ) { goto out; }
    cwasm_fh_discard_in( fh );

    if( io->owned && io->data && io->data != buf ) { free( io->data ); }

    if( mode == _IONBF ) {
        io->data = NULL;
        io->cap = io->len = io->pos = 0;
        io->owned = 0;
        io->vmode = _IONBF;
        rc = 0;
        goto out;
    }

    if( buf ) {
        io->data = buf;
        io->cap = size;
        io->owned = 0;
    } else if( size > 0 ) {
        io->data = (char*)malloc( size );
        if( !io->data ) { goto out; }
        io->cap = size;
        io->owned = 1;
    } else {
        io->data = NULL;
        io->cap = 0;
        io->owned = 0;
    }
    io->len = io->pos = 0;
    io->active = 0;
    io->vmode = (uint8_t)mode;
    rc = 0;
out:
    cwasm_fh_unlock( fh );
    return rc;
}

size_t __cwasm_stdio_fread_bytes( void* ptr, size_t size, size_t nmemb, FILE* stream ) {
    uint32_t fh = 0;
    uint32_t n;

    if( cwasm_fh_from_stream( stream, &fh ) != 0 || cwasm_require_fh( fh ) != 0 ) {
        return 0;
    }
    n = cwasm_fsio_fread( (uint32_t)(uintptr_t)ptr, (uint32_t)size, (uint32_t)nmemb, fh );
    return (size_t)n;
}

size_t __cwasm_stdio_fwrite_bytes( void const* ptr, size_t size, size_t nmemb, FILE* stream ) {
    uint32_t fh = 0;
    uint32_t n;

    if( cwasm_fh_from_stream( stream, &fh ) != 0 || cwasm_require_fh( fh ) != 0 ) {
        return 0;
    }
    n = cwasm_fsio_fwrite( (uint32_t)(uintptr_t)ptr, (uint32_t)size, (uint32_t)nmemb, fh );
    return (size_t)n;
}

size_t fread( void* ptr, size_t size, size_t nmemb, FILE* stream ) {
    size_t n;
    flockfile( stream );
    if( __cwasm_stdio_orient_byte( stream ) != 0 ) {
        funlockfile( stream );
        return 0;
    }
    n = __cwasm_stdio_fread_bytes( ptr, size, nmemb, stream );
    funlockfile( stream );
    return n;
}

size_t fwrite( void const* ptr, size_t size, size_t nmemb, FILE* stream ) {
    size_t n;
    flockfile( stream );
    if( __cwasm_stdio_orient_byte( stream ) != 0 ) {
        funlockfile( stream );
        return 0;
    }
    n = __cwasm_stdio_fwrite_bytes( ptr, size, nmemb, stream );
    funlockfile( stream );
    return n;
}

// Caller holds the real fh recursive lock
static int cwasm_fseek_locked( uint32_t logical, uint32_t real, long offset, int whence ) {
    long adj = offset;

    if( cwasm_fh_flush_out( real ) != 0 ) { return -1; }
    if( real < CWASM_STDIO_N && cwasm_fh_wfs[ real ] ) {
        if( whence == SEEK_CUR ) {
            cwasm_fh_io_t* io = &cwasm_fh_io[ real ];
            if( io->active == 1u && io->pos < io->len ) {
                adj -= (long)( io->len - io->pos );
            }
            if( cwasm_fh_unget_len[ real ] > 0 ) {
                adj -= (long)cwasm_fh_unget_len[ real ];
            }
            if( logical < CWASM_STDIO_N &&__cwasm_wide_unget_nbytes[ logical ] > 0 ) {
                adj -= (long)__cwasm_wide_unget_nbytes[ logical ];
            }
        }
        if( cwasm_fs_seek( &cwasm_fs_h[ real ], adj, whence ) < 0 ) { return -1; }
        cwasm_fh_eof[ real ] = 0;
        cwasm_fh_clear_unget( real );
        cwasm_fh_discard_in( real );
        if( logical < CWASM_STDIO_N ) { __cwasm_wide_reset_handle( logical ); }
        return 0;
    }
    if( real == 0u && logical == 0u && !cwasm_stdio_redirect[ 0 ] && wfs_stdin_size() ) {
        uint32_t stdin_size = wfs_stdin_size();
        long pos;

        if( whence == SEEK_SET ) { pos = adj; }
        else if( whence == SEEK_CUR ) { pos = (long)cwasm_stdin_pos + adj; }
        else if( whence == SEEK_END ) { pos = (long)stdin_size + adj; }
        else {
            errno = EINVAL;
            return -1;
        }
        if( pos < 0 || (uint32_t)pos > stdin_size ) {
            errno = EINVAL;
            return -1;
        }
        cwasm_stdin_pos = (uint32_t)pos;
        cwasm_fh_eof[ real ] = 0;
        cwasm_fh_clear_unget( real );
        cwasm_fh_discard_in( real );
        __cwasm_wide_reset_handle( logical );
        return 0;
    }
    errno = ESPIPE;
    return -1;
}

static long cwasm_ftell_locked( uint32_t logical, uint32_t real ) {
    if( real < CWASM_STDIO_N && cwasm_fh_wfs[ real ] ) {
        cwasm_fh_io_t* io = &cwasm_fh_io[ real ];
        long pos;

        pos = cwasm_fs_tell( &cwasm_fs_h[ real ] );
        if( io->active == 1u && io->pos < io->len ) {
            pos -= (long)( io->len - io->pos );
        } else if( io->active == 2u ) {
            pos += (long)io->len;
        }
        if( cwasm_fh_unget_len[ real ] > 0 ) { pos -= (long)cwasm_fh_unget_len[ real ]; }
        if( logical < CWASM_STDIO_N &&__cwasm_wide_unget_nbytes[ logical ] > 0 ) {
            pos -= (long)__cwasm_wide_unget_nbytes[ logical ];
        }
        return pos;
    }
    if( real == 0u && logical == 0u && !cwasm_stdio_redirect[ 0 ] ) {
        return (long)cwasm_stdin_pos;
    }
    errno = EBADF;
    return -1L;
}

int fseek( FILE* stream, long offset, int whence ) {
    uint32_t logical = 0;
    uint32_t real = 0;
    int rc;

    if( cwasm_fh_from_stream( stream, &logical ) != 0 ) { return -1; }
    if( cwasm_stream_lock( stream, &real ) != 0 ) { return -1; }
    rc = cwasm_fseek_locked( logical, real, offset, whence );
    cwasm_fh_unlock( real );
    return rc;
}

long ftell( FILE* stream ) {
    uint32_t logical = 0;
    uint32_t real = 0;
    long pos;

    if( cwasm_fh_from_stream( stream, &logical ) != 0 ) { return -1L; }
    if( cwasm_stream_lock( stream, &real ) != 0 ) { return -1L; }
    pos = cwasm_ftell_locked( logical, real );
    cwasm_fh_unlock( real );
    return pos;
}

int fseeko( FILE* stream, off_t offset, int whence ) {
    if( offset < LONG_MIN || offset > LONG_MAX ) {
        errno = EINVAL;
        return -1;
    }
    return fseek( stream, (long)offset, whence );
}

off_t ftello( FILE* stream ) {
    long pos = ftell( stream );
    if( pos < 0 ) { return (off_t)-1; }
    return (off_t)pos;
}

void rewind( FILE* stream ) {
    uint32_t logical = 0;
    uint32_t real = 0;

    if( cwasm_fh_from_stream( stream, &logical ) != 0 ) { return; }
    if( cwasm_stream_lock( stream, &real ) != 0 ) { return; }
    if( cwasm_fseek_locked( logical, real, 0, SEEK_SET ) == 0 ) {
        cwasm_fh_eof[ real ] = 0;
        cwasm_fh_err[ real ] = 0;
    } else if( errno == ESPIPE ) {
        errno = EBADF;
    }
    cwasm_fh_unlock( real );
}

#ifdef CWASM_THREADS
    static uint32_t cwasm_flock_real_of( uint32_t logical ) {
        if( logical < CWASM_STDIO_N && g_flock_held[ logical ] ) {
            return g_flock_real[ logical ];
        }
        return cwasm_real_fh( logical );
    }
#else
    static uint32_t cwasm_flock_real_of( uint32_t logical ) {
        return cwasm_real_fh( logical );
    }
#endif

int fgetpos( FILE* stream, fpos_t* pos ) {
    uint32_t logical = 0;
    uint32_t real;
    off_t p;
    int rc = -1;

    if( !pos ) {
        errno = EINVAL;
        return -1;
    }
    flockfile( stream );
    if( cwasm_fh_from_stream( stream, &logical ) != 0 ) { goto out; }
    real = cwasm_flock_real_of( logical );
    if( real >= CWASM_STDIO_N || !cwasm_file_valid[ real ] ) {
        errno = EBADF;
        goto out;
    }
    p = (off_t)cwasm_ftell_locked( logical, real );
    if( p < 0 ) { goto out; }
    if( logical < CWASM_STDIO_N ) { p -= (off_t) __cwasm_wide_bytebuf_pending( logical ); }
    pos->__off = p;
    if( logical < CWASM_STDIO_N ) {
        __builtin_memcpy( pos->__mbstate_rd, &__cwasm_wide_rd_state[ logical ], sizeof( pos->__mbstate_rd ) );
        __builtin_memcpy( pos->__mbstate_wr, &__cwasm_wide_wr_state[ logical ], sizeof( pos->__mbstate_wr ) );
    } else {
        __builtin_memset( pos->__mbstate_rd, 0, sizeof( pos->__mbstate_rd ) );
        __builtin_memset( pos->__mbstate_wr, 0, sizeof( pos->__mbstate_wr ) );
    }
    rc = 0;
out:
    funlockfile( stream );
    return rc;
}

int fsetpos( FILE* stream, fpos_t const* pos ) {
    uint32_t logical = 0;
    uint32_t real;
    int rc = -1;

    if( !pos ) {
        errno = EINVAL;
        return -1;
    }
    flockfile( stream );
    if( cwasm_fh_from_stream( stream, &logical ) != 0 ) { goto out; }
    real = cwasm_flock_real_of( logical );
    if( real >= CWASM_STDIO_N || !cwasm_file_valid[ real ] ) {
        errno = EBADF;
        goto out;
    }
    if( logical < CWASM_STDIO_N ) { __cwasm_wide_bytebuf_reset( logical ); }
    if( pos->__off < LONG_MIN || pos->__off > LONG_MAX ) {
        errno = EINVAL;
        goto out;
    }
    if( cwasm_fseek_locked( logical, real, (long)pos->__off, SEEK_SET ) != 0 ) {
        goto out;
    }
    if( logical < CWASM_STDIO_N ) {
        __builtin_memcpy( &__cwasm_wide_rd_state[ logical ], pos->__mbstate_rd, sizeof( pos->__mbstate_rd ) );
        __builtin_memcpy( &__cwasm_wide_wr_state[ logical ], pos->__mbstate_wr, sizeof( pos->__mbstate_wr ) );
        __cwasm_wide_unget_len[ logical ] = 0;
        __cwasm_wide_unget_nbytes[ logical ] = 0;
    }

    cwasm_fh_clear_unget( real );
    cwasm_fh_eof[ real ] = 0;
    rc = 0;
out:
    funlockfile( stream );
    return rc;
}

void clearerr( FILE* stream ) {
    uint32_t fh = 0;

    if( cwasm_stream_lock( stream, &fh ) != 0 ) { return; }
    if( fh < CWASM_STDIO_N ) {
        cwasm_fh_eof[ fh ] = 0;
        cwasm_fh_err[ fh ] = 0;
    }
    cwasm_fh_unlock( fh );
}

int feof( FILE* stream ) {
    uint32_t fh = 0;
    int r;

    if( cwasm_stream_lock( stream, &fh ) != 0 ) {
        if( !errno ) { errno = EBADF; }
        return 0;
    }
    r = ( fh < CWASM_STDIO_N && cwasm_fh_eof[ fh ] ) ? 1 : 0;
    cwasm_fh_unlock( fh );
    return r;
}

int ferror( FILE* stream ) {
    uint32_t fh = 0;
    int r;

    if( cwasm_stream_lock( stream, &fh ) != 0 ) {
        if( !errno ) { errno = EBADF; }
        return 0;
    }
    r = ( fh < CWASM_STDIO_N && cwasm_fh_err[ fh ] ) ? 1 : 0;
    cwasm_fh_unlock( fh );
    return r;
}

int fgetc( FILE* stream ) {
    uint32_t fh = 0;
    uint8_t c = 0;
    int r = EOF;

    flockfile( stream );
    if( __cwasm_stdio_orient_byte( stream ) != 0 ) { goto out; }
    if( cwasm_fh_from_stream( stream, &fh ) != 0 || cwasm_require_fh( fh ) != 0 ) {
        if( !errno ) { errno = EBADF; }
        goto out;
    }
    fh = cwasm_flock_real_of( fh );
    if( cwasm_fsio_fread( (uint32_t)(uintptr_t)&c, 1, 1, fh ) != 1 ) { goto out; }
    r = (int)c;
out:
    funlockfile( stream );
    return r;
}

int ungetc( int c, FILE* stream ) {
    uint32_t fh = 0;
    int r = EOF;

    flockfile( stream );
    if( __cwasm_stdio_orient_byte( stream ) != 0 ) { goto out; }
    if( cwasm_fh_from_stream( stream, &fh ) != 0 || cwasm_require_fh( fh ) != 0 ) {
        if( !errno ) { errno = EBADF; }
        goto out;
    }
    fh = cwasm_flock_real_of( fh );
    if( fh >= CWASM_STDIO_N || !cwasm_access_allows_read( cwasm_fh_access[ fh ] ) ) {
        errno = EBADF;
        goto out;
    }
    if( c == EOF ) {
        errno = EINVAL;
        goto out;
    }
    if( cwasm_fh_unget_len[ fh ] >= __CWASM_STDIO_UNGET ) {
        errno = EINVAL;
        goto out;
    }
    cwasm_fh_unget_buf[ fh ][ cwasm_fh_unget_len[ fh ]++ ] = (uint8_t)( c & 0xFF );
    cwasm_fh_eof[ fh ] = 0;
    r = (int)(unsigned char)( c & 0xFF );
out:
    funlockfile( stream );
    return r;
}

int fputc( int c, FILE* stream ) {
    uint32_t fh = 0;
    uint8_t b = (uint8_t)( c & 0xFF );
    int r = EOF;

    flockfile( stream );
    if( __cwasm_stdio_orient_byte( stream ) != 0 ) { goto out; }
    if( cwasm_fh_from_stream( stream, &fh ) != 0 || cwasm_require_fh( fh ) != 0 ) {
        if( !errno ) { errno = EBADF; }
        goto out;
    }
    fh = cwasm_flock_real_of( fh );
    if( cwasm_fsio_fwrite( (uint32_t)(uintptr_t)&b, 1, 1, fh ) != 1 ) { goto out; }
    r = (int)b;
out:
    funlockfile( stream );
    return r;
}

int fputs( char const* s, FILE* stream ) {
    if( !s ) { errno = EINVAL; return EOF; }
    size_t n = 0;
    while( s[ n ] ) { ++n; }
    size_t wrote = fwrite( s, 1, n, stream );
    return ( wrote == n ) ? (int)wrote : EOF;
}

char* fgets( char* s, int n, FILE* stream ) {
    int i = 0;
    int err = 0;
    if( !s || n <= 0 ) { errno = EINVAL; return NULL; }
    flockfile( stream );
    while( i + 1 < n ) {
        int c = fgetc( stream );
        if( c == EOF ) { err = ferror( stream ) != 0; break; }
        s[ i++ ] = (char)c;
        if( c == '\n' ) { break; }
    }
    funlockfile( stream );

    if( err ) { return NULL; }
    if( i == 0 && n > 1 ) { return NULL; }
    s[ i ] = '\0';
    return s;
}

#undef getc
#undef putc

int getc( FILE* stream ) { return fgetc( stream ); }

int putc( int c, FILE* stream ) { return fputc( c, stream ); }

int getchar( void ) {
    return fgetc( stdin );
}

int putchar( int c ) {
    return fputc( c, stdout );
}

int puts( char const* s ) {
    int r;

    flockfile( stdout );
    if( fputs( s, stdout ) == EOF ) {
        funlockfile( stdout );
        return EOF;
    }
    r = fputc( '\n', stdout );
    funlockfile( stdout );
    return r;
}

// narrow %ls / %lc. stb_sprintf has no wide conversion: it reads 'l' as a length
// modifier and prints the wchar_t* as a char*, so "%ls" of L"wide" renders as "w".

typedef struct {
    char* buf;
    size_t cap;
    size_t len;
} cwasm_nout;

static void cwasm_nout_write( cwasm_nout* o, char const* s, size_t n ) {
    if( o->buf && o->len < o->cap ) {
        size_t room = o->cap - o->len;
        size_t take = n < room ? n : room;
        memcpy( o->buf + o->len, s, take );
    }
    o->len += n;
}

static void cwasm_nout_pad( cwasm_nout* o, char c, int n ) {
    char blk[ 32 ];
    if( n <= 0 ) { return; }
    memset( blk, c, sizeof( blk ) );
    while( n > 0 ) {
        int chunk = n < (int)sizeof( blk ) ? n : (int)sizeof( blk );
        cwasm_nout_write( o, blk, (size_t)chunk );
        n -= chunk;
    }
}

static int cwasm_fmt_has_wide( char const* fmt ) {
    char const* p = fmt;

    if( !p ) { return 0; }
    while( *p ) {
        int f_plus = 0;
        int f_space = 0;
        int f_zero = 0;
        int star_w = 0;
        int has_prec = 0;

        if( *p++ != '%' ) { continue; }
        if( *p == '%' ) {
            ++p;
            continue;
        }
        while( *p && strchr( "-+ #0'", *p ) ) {
            if( *p == '+' ) { f_plus = 1; }
            else if( *p == ' ' ) { f_space = 1; }
            else if( *p == '0' ) { f_zero = 1; }
            ++p;
        }
        if( *p == '*' ) { star_w = 1; }
        while( ( *p >= '0' && *p <= '9' ) || *p == '*' ) { ++p; }
        if( *p == '.' ) {
            has_prec = 1;
            ++p;
            while( ( *p >= '0' && *p <= '9' ) || *p == '*' ) { ++p; }
        }
        char const* c = p;
        while( *c == 'h' || *c == 'l' || *c == 'L' || *c == 'j' || *c == 'z' || *c == 't' ) {
            ++c;
        }
        if( *c ) {
            // A negative * width is the - flag with a positive width (p5); stb takes it
            // as a huge unsigned width. The wrapper already resolves it.
            if( star_w ) { return 1; }
            if( f_plus && f_space ) { return 1; }
            if( has_prec && strchr( "diouxX", *c ) ) { return 1; }
            if( *c == 'a' || *c == 'A' ) { return 1; }
            if( f_zero && strchr( "aAeEfFgG", *c ) ) { return 1; }
        }
        if( *p == 'l' ) {
            ++p;
            if( *p == 'l' ) { ++p; }
            else if( *p == 's' || *p == 'c' ) { return 1; }
        } else if( *p == 'h' ) {
            char const* q = p + 1;
            if( *q == 'h' ) { ++q; }
            if( *q && strchr( "diouxX", *q ) ) { return 1; }
        }
        if( *p ) { ++p; }
    }
    return 0;
}

static int cwasm_put_wcs( cwasm_nout* o, wchar_t const* wcs, int prec, int prec_set ) {
    char mb[ MB_LEN_MAX ];
    mbstate_t st;
    size_t total = 0;
    wchar_t const* p;

    if( !wcs ) { wcs = L"(null)"; }
    memset( &st, 0, sizeof( st ) );
    for( p = wcs; *p; ++p ) {
        size_t n = wcrtomb( mb, *p, &st );
        if( n == (size_t)-1 ) {
            errno = EILSEQ;
            return -1;
        }
        if( prec_set && total + n > (size_t)prec ) { break; }
        cwasm_nout_write( o, mb, n );
        total += n;
    }
    return 0;
}

static size_t cwasm_wcs_mblen( wchar_t const* wcs, int prec, int prec_set ) {
    char mb[ MB_LEN_MAX ];
    mbstate_t st;
    size_t total = 0;
    wchar_t const* p;

    if( !wcs ) { wcs = L"(null)"; }
    memset( &st, 0, sizeof( st ) );
    for( p = wcs; *p; ++p ) {
        size_t n = wcrtomb( mb, *p, &st );
        if( n == (size_t)-1 ) { return (size_t)-1; }
        if( prec_set && total + n > (size_t)prec ) { break; }
        total += n;
    }
    return total;
}

typedef struct {
    int left;
    int plus;
    int space;
    int zero;
    int alt;
    int width;
    int prec;
    int prec_set;
    int lmod;
    char conv;
} cwasm_spec;

static void cwasm_emit_padded( cwasm_nout* o, cwasm_spec const* sp, char sign, char const* body ) {
    size_t blen = strlen( body );
    int len = (int)blen + ( sign ? 1 : 0 );
    int pad = sp->width - len;

    if( !sp->left ) { cwasm_nout_pad( o, ' ', pad ); }
    if( sign ) { cwasm_nout_write( o, &sign, 1 ); }
    cwasm_nout_write( o, body, blen );
    if( sp->left ) { cwasm_nout_pad( o, ' ', pad ); }
}

// The caller's va_list must advance here: one passed to stb does not advance ours.
static void cwasm_emit_one( cwasm_nout* o, char const* spec, cwasm_spec const* sp,
    va_list* ap ) {
    char conv = sp->conv;
    int lmod = sp->lmod;
    union {
        long long lld;
        unsigned long long llu;
        double d;
        void* p;
        char* s;
    } v;
    enum { A_LLD, A_LLU, A_D, A_P, A_S } kind;
    char tmp[ 256 ];
    char* dyn = NULL;
    char* dst = tmp;
    size_t cap = sizeof( tmp );
    int n;

    switch( conv ) {
    case 'd': case 'i':
        kind = A_LLD;
        v.lld = ( lmod == 3 ) ? va_arg( *ap, long long )
            : ( lmod == 2 ) ? (long long)va_arg( *ap, long )
            : (long long)va_arg( *ap, int );
        break;
    case 'u': case 'o': case 'x': case 'X':
        kind = A_LLU;
        v.llu = ( lmod == 3 ) ? va_arg( *ap, unsigned long long )
            : ( lmod == 2 ) ? (unsigned long long)va_arg( *ap, unsigned long )
            : (unsigned long long)va_arg( *ap, unsigned int );
        break;
    case 'f': case 'F': case 'e': case 'E':
    case 'g': case 'G': case 'a': case 'A':
        kind = A_D;
        v.d = ( lmod == 4 ) ? (double)va_arg( *ap, long double ) : va_arg( *ap, double );
        break;
    case 'c':
        kind = A_LLD;
        v.lld = (long long)va_arg( *ap, int );
        break;
    case 's':
        kind = A_S;
        v.s = va_arg( *ap, char* );
        break;
    case 'p':
        kind = A_P;
        v.p = va_arg( *ap, void* );
        break;
    default:
        return;
    }

    for( ;; ) {
        // cases stb v1.10 gets wrong; handled here instead of reaching it

        if( sp->prec_set && sp->prec == 0 && strchr( "diouxX", conv ) &&
            ( ( kind == A_LLD && v.lld == 0 ) || ( kind == A_LLU && v.llu == 0 ) ) ) {
            char sign = 0;
            if( kind == A_LLD ) {
                if( sp->plus ) { sign = '+'; }
                else if( sp->space ) { sign = ' '; }
            }
            cwasm_emit_padded( o, sp, sign, ( sp->alt && conv == 'o' ) ? "0" : "" );
            free( dyn );
            return;
        }

        if( kind == A_D && !__builtin_isfinite( v.d ) ) {
            char sign = 0;
            int neg = __builtin_signbit( v.d ) != 0;
            if( neg ) { sign = '-'; }
            else if( sp->plus ) { sign = '+'; }
            else if( sp->space ) { sign = ' '; }

            if( conv == 'a' || conv == 'A' ) {
                int upper = ( conv == 'A' );
                char const* body = __builtin_isnan( v.d ) ? ( upper ? "NAN" : "nan" )
                                                        : ( upper ? "INF" : "inf" );
                cwasm_emit_padded( o, sp, sign, body );
                free( dyn );
                return;
            }

            if( sp->zero ) {
                int upper = ( conv == 'E' || conv == 'F' || conv == 'G' );
                char const* body = __builtin_isnan( v.d ) ? ( upper ? "NAN" : "nan" )
                                                        : ( upper ? "INF" : "inf" );
                cwasm_emit_padded( o, sp, sign, body );
                free( dyn );
                return;
            }
        }

        switch( kind ) {
            case A_LLD:
                n = ( lmod == 3 ) ? stbsp_snprintf( dst, (int)cap, spec, v.lld )
                    : ( lmod == 2 ) ? stbsp_snprintf( dst, (int)cap, spec, (long)v.lld )
                    : ( lmod == 5 ) ? stbsp_snprintf( dst, (int)cap, spec, (int)(signed char)v.lld )
                    : ( lmod == 1 ) ? stbsp_snprintf( dst, (int)cap, spec, (int)(short)v.lld )
                    : stbsp_snprintf( dst, (int)cap, spec, (int)v.lld );
                break;
            case A_LLU:
                n = ( lmod == 3 ) ? stbsp_snprintf( dst, (int)cap, spec, v.llu )
                    : ( lmod == 2 ) ? stbsp_snprintf( dst, (int)cap, spec, (unsigned long)v.llu )
                    : ( lmod == 5 )
                        ? stbsp_snprintf( dst, (int)cap, spec, (unsigned int)(unsigned char)v.llu )
                    : ( lmod == 1 )
                        ? stbsp_snprintf( dst, (int)cap, spec, (unsigned int)(unsigned short)v.llu )
                    : stbsp_snprintf( dst, (int)cap, spec, (unsigned int)v.llu );
                break;
            case A_D: n = stbsp_snprintf( dst, (int)cap, spec, v.d ); break;
            case A_S: n = stbsp_snprintf( dst, (int)cap, spec, v.s ); break;
            default: n = stbsp_snprintf( dst, (int)cap, spec, v.p ); break;
        }

        if( n < 0 ) { break; }
        if( (size_t)n < cap ) { break; }
        if( dyn ) { break; }
        cap = (size_t)n + 1u;
        dyn = (char*)malloc( cap );
        if( !dyn ) {
            n = (int)strlen( tmp );
            dst = tmp;
            break;
        }
        dst = dyn;
    }
    if( n > 0 ) { cwasm_nout_write( o, dst, (size_t)n ); }
    free( dyn );
}

static int cwasm_vsnprintf_wide( char* buf, size_t size, char const* fmt, va_list ap ) {
    cwasm_nout o;
    char const* p = fmt;

    o.buf = buf;
    o.cap = ( buf && size > 0 ) ? size - 1u : 0u;
    o.len = 0;

    while( *p ) {
        char const* spec_start = p;
        char spec[ 64 ];
        int left = 0;
        int width = 0;
        int prec = 0;
        int prec_set = 0;
        int lmod = 0;
        int plus = 0;
        int space = 0;
        int zero = 0;
        int alt = 0;
        char conv;

        if( *p != '%' ) {
            char const* lit = p;
            while( *p && *p != '%' ) { ++p; }
            cwasm_nout_write( &o, lit, (size_t)( p - lit ) );
            continue;
        }
        ++p;
        if( *p == '%' ) {
            cwasm_nout_write( &o, "%", 1 );
            ++p;
            continue;
        }

        while( *p && strchr( "-+ #0'", *p ) ) {
            if( *p == '-' ) { left = 1; }
            else if( *p == '+' ) { plus = 1; }
            else if( *p == ' ' ) { space = 1; }
            else if( *p == '#' ) { alt = 1; }
            else if( *p == '0' ) { zero = 1; }
            ++p;
        }

        if( plus ) { space = 0; }
        if( *p == '*' ) {
            width = va_arg( ap, int );
            if( width < 0 ) {
                left = 1;
                width = -width;
            }
            ++p;
        } else {
            while( *p >= '0' && *p <= '9' ) { width = width * 10 + ( *p++ - '0' ); }
        }
        if( *p == '.' ) {
            ++p;
            prec_set = 1;
            if( *p == '*' ) {
                prec = va_arg( ap, int );
                if( prec < 0 ) { prec_set = 0; }
                ++p;
            } else {
                while( *p >= '0' && *p <= '9' ) { prec = prec * 10 + ( *p++ - '0' ); }
            }
        }
        if( *p == 'h' ) {
            ++p;
            lmod = 1;
            if( *p == 'h' ) {
                ++p;
                lmod = 5;
            }
        } else if( *p == 'l' ) {
            ++p;
            lmod = 2;
            if( *p == 'l' ) {
                ++p;
                lmod = 3;
            }
        } else if( *p == 'L' ) {
            ++p;
            lmod = 4;
        } else if( *p == 'j' ) {
            ++p;
            // wasm32: long is 32-bit but intmax_t is 64-bit, so j must read a long long
            lmod = 3;
        } else if( *p == 'z' || *p == 't' ) {
            ++p;
            lmod = 2;
        }
        conv = *p;
        if( !conv ) { break; }
        ++p;

        if( lmod == 2 && conv == 's' ) {
            wchar_t const* wcs = va_arg( ap, wchar_t const* );
            size_t mblen = cwasm_wcs_mblen( wcs, prec, prec_set );
            int pad;

            if( mblen == (size_t)-1 ) {
                errno = EILSEQ;
                return -1;
            }
            pad = width - (int)mblen;
            if( !left ) { cwasm_nout_pad( &o, ' ', pad ); }
            if( cwasm_put_wcs( &o, wcs, prec, prec_set ) != 0 ) { return -1; }
            if( left ) { cwasm_nout_pad( &o, ' ', pad ); }
            continue;
        }
        if( lmod == 2 && conv == 'c' ) {
            wchar_t wc = (wchar_t)va_arg( ap, wint_t );
            char mb[ MB_LEN_MAX ];
            mbstate_t st;
            size_t n;

            memset( &st, 0, sizeof( st ) );
            n = wcrtomb( mb, wc, &st );
            if( n == (size_t)-1 ) {
                errno = EILSEQ;
                return -1;
            }
            if( !left ) { cwasm_nout_pad( &o, ' ', width - (int)n ); }
            cwasm_nout_write( &o, mb, n );
            if( left ) { cwasm_nout_pad( &o, ' ', width - (int)n ); }
            continue;
        }
        if( conv == 'n' ) {
            void* q = va_arg( ap, void* );
            if( q ) {
                if( lmod == 3 ) { *(long long*)q = (long long)o.len; }
                else if( lmod == 2 ) { *(long*)q = (long)o.len; }
                else { *(int*)q = (int)o.len; }
            }
            continue;
        }

        size_t spec_len = (size_t)( p - spec_start );
        if( spec_len >= sizeof( spec ) ) { continue; }
        char* w = spec;
        char const* q = spec_start;

        *w++ = '%';
        ++q;
        while( *q && strchr( "-+ #0'", *q ) ) { ++q; }
        if( left ) { *w++ = '-'; }
        if( plus ) { *w++ = '+'; }
        if( space ) { *w++ = ' '; }
        if( alt ) { *w++ = '#'; }
        if( zero && !( prec_set && strchr( "diouxX", conv ) ) ) { *w++ = '0'; }
        while( *q == '*' || ( *q >= '0' && *q <= '9' ) ) { ++q; }
        if( width > 0 ) {
            w += stbsp_snprintf( w, (int)( sizeof( spec ) - (size_t)( w - spec ) ), "%d", width );
        }
        if( *q == '.' ) {
            ++q;
            while( *q == '*' || ( *q >= '0' && *q <= '9' ) ) { ++q; }
            if( prec_set ) {
                w += stbsp_snprintf( w, (int)( sizeof( spec ) - (size_t)( w - spec ) ), ".%d", prec );
            }
        }
        while( q < p - 1 ) {
            if( ( lmod == 1 || lmod == 5 ) && *q == 'h' ) {
                ++q;
                continue;
            }
            *w++ = *q++;
        }
        *w++ = conv;
        *w = '\0';

        cwasm_spec sp;
        sp.left = left; sp.plus = plus; sp.space = space;
        sp.zero = zero; sp.alt = alt;
        sp.width = width; sp.prec = prec; sp.prec_set = prec_set;
        sp.lmod = lmod; sp.conv = conv;
        cwasm_emit_one( &o, spec, &sp, &ap );
    }

    if( buf && size > 0 ) { buf[ o.len < o.cap ? o.len : o.cap ] = '\0'; }
    return (int)o.len;
}

int vsnprintf( char* str, size_t size, char const* format, va_list ap ) {
    // stb counts only for (buf == NULL && count == 0); else it ends in buf[count - 1] = 0.
    int stb_size;

    if( size > (size_t)INT_MAX ) { size = (size_t)INT_MAX; }
    stb_size = (int)size;

    if( cwasm_fmt_has_wide( format ) ) {
        va_list ap2;
        int n;
        va_copy( ap2, ap );
        n = cwasm_vsnprintf_wide( str, ( !str || size == 0 ) ? 0u : size, format, ap2 );
        va_end( ap2 );
        return n;
    }

    if( !str || size == 0 ) {
        va_list ap2;
        va_copy( ap2, ap );
        int n = stbsp_vsnprintf( NULL, 0, format, ap2 );
        va_end( ap2 );
        return n;
    }
    va_list ap2;
    va_copy( ap2, ap );
    int n = stbsp_vsnprintf( str, stb_size, format, ap2 );
    va_end( ap2 );
    return n;
}

int vasprintf( char** strp, char const* format, va_list ap ) {
    va_list ap2;
    int n;

    if( !strp ) { return -1; }
    va_copy( ap2, ap );
    n = vsnprintf( NULL, 0, format, ap2 );
    va_end( ap2 );
    if( n < 0 ) { return -1; }

    char* buf = (char*)malloc( (size_t)n + 1u );
    if( !buf ) { return -1; }

    va_copy( ap2, ap );
    vsnprintf( buf, (size_t)n + 1u, format, ap2 );
    va_end( ap2 );
    *strp = buf;
    return n;
}

int vsprintf( char* str, char const* format, va_list ap ) {
    va_list ap2;
    int n;

    va_copy( ap2, ap );
    if( cwasm_fmt_has_wide( format ) ) {
        n = cwasm_vsnprintf_wide( str, (size_t)INT_MAX, format, ap2 );
    } else { n = stbsp_vsprintf( str, format, ap2 ); }
    va_end( ap2 );
    return n;
}

int snprintf( char* str, size_t size, char const* format, ... ) {
    va_list ap;
    va_start( ap, format );
    int n = vsnprintf( str, size, format, ap );
    va_end( ap );
    return n;
}

int sprintf( char* str, char const* format, ... ) {
    va_list ap;
    va_start( ap, format );
    int n = vsprintf( str, format, ap );
    va_end( ap );
    return n;
}

int vfprintf( FILE* stream, char const* format, va_list ap ) {
    char stackbuf[ 1024 ];
    va_list ap2;
    int n;
    int rc;
    size_t w;
    char* buf;
    size_t cap;

    flockfile( stream );
    if( __cwasm_stdio_orient_byte( stream ) != 0 ) {
        funlockfile( stream );
        return -1;
    }
    va_copy( ap2, ap );
    n = vsnprintf( stackbuf, sizeof( stackbuf ), format, ap2 );
    va_end( ap2 );
    if( n < 0 ) {
        funlockfile( stream );
        return n;
    }
    if( (size_t)n < sizeof( stackbuf ) ) {
        w = __cwasm_stdio_fwrite_bytes( stackbuf, 1, (size_t)n, stream );
        rc = ( w == (size_t)n ) ? n : -1;
        funlockfile( stream );
        return rc;
    }
    cap = (size_t)n + 1;
    buf = (char*)malloc( cap );
    if( !buf ) {
        errno = ENOMEM;
        funlockfile( stream );
        return -1;
    }
    va_copy( ap2, ap );
    (void)vsnprintf( buf, cap, format, ap2 );
    va_end( ap2 );
    w = __cwasm_stdio_fwrite_bytes( buf, 1, (size_t)n, stream );
    rc = ( w == (size_t)n ) ? n : -1;
    free( buf );
    funlockfile( stream );
    return rc;
}

int fprintf( FILE* stream, char const* format, ... ) {
    va_list ap;
    va_start( ap, format );
    int n = vfprintf( stream, format, ap );
    va_end( ap );
    return n;
}

int vprintf( char const* format, va_list ap ) {
    return vfprintf( stdout, format, ap );
}

int printf( char const* format, ... ) {
    va_list ap;
    va_start( ap, format );
    int n = vfprintf( stdout, format, ap );
    va_end( ap );
    return n;
}

void perror( char const* s ) {
    char const* msg = strerror( errno );

    if( s && *s ) { (void)fprintf( stderr, "%s: %s\n", s, msg ); }
    else { (void)fprintf( stderr, "%s\n", msg ); }
}

#ifndef CWASM_SCAN_PUSHBACK_INITIAL
    #define CWASM_SCAN_PUSHBACK_INITIAL 64
#endif

#ifndef CWASM_SCAN_TOKEN_INITIAL
    #define CWASM_SCAN_TOKEN_INITIAL 128
#endif

static inline int cwasm_is_space( int c ) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

static inline int cwasm_is_digit10( int c ) {
    return (unsigned)( c - '0' ) < 10u;
}

static inline int cwasm_is_radix( int c ) {
    return c == '.' || c == __cwasm_locale_radix_char();
}

static inline int cwasm_scan_digit_val( int c ) {
    if( c >= '0' && c <= '9' ) { return c - '0'; }
    if( c >= 'a' && c <= 'f' ) { return c - 'a' + 10; }
    if( c >= 'A' && c <= 'F' ) { return c - 'A' + 10; }
    return -1;
}

enum cwasm_scan_src {
    CWASM_SCAN_SRC_STR,
    CWASM_SCAN_SRC_FILE
};

typedef struct cwasm_scan_input {
    enum cwasm_scan_src src;
    void* ctx;
    size_t count;
    int small_pb[ CWASM_SCAN_PUSHBACK_INITIAL ];
    int* pb;
    int pb_count;
    int pb_cap;
    int pb_heap;
    int oom;
    int restore_error;
} cwasm_scan_input;

static void cwasm_scan_init( cwasm_scan_input* in, enum cwasm_scan_src src, void* ctx ) {
    in->src = src;
    in->ctx = ctx;
    in->count = 0;
    in->pb = in->small_pb;
    in->pb_count = 0;
    in->pb_cap = CWASM_SCAN_PUSHBACK_INITIAL;
    in->pb_heap = 0;
    in->oom = 0;
    in->restore_error = 0;
}

static int cwasm_scan_grow_pb( cwasm_scan_input* in ) {
    int new_cap = in->pb_cap * 2;
    int* new_pb;
    if( new_cap <= in->pb_cap ) { return 0; }
    if( in->pb_heap ) {
        new_pb = (int*)realloc( in->pb, (size_t)new_cap * sizeof( int ) );
    } else {
        new_pb = (int*)malloc( (size_t)new_cap * sizeof( int ) );
        if( new_pb ) { memcpy( new_pb, in->pb, (size_t)in->pb_count * sizeof( int ) ); }
    }
    if( !new_pb ) {
        in->oom = 1;
        errno = ENOMEM;
        return 0;
    }
    in->pb = new_pb;
    in->pb_cap = new_cap;
    in->pb_heap = 1;
    return 1;
}

static int cwasm_scan_get( cwasm_scan_input* in ) {
    int c;
    if( in->pb_count > 0 ) { c = in->pb[ --in->pb_count ]; }
    else if( in->src == CWASM_SCAN_SRC_STR ) {
        char const** ps = (char const**)in->ctx;
        unsigned char uc = (unsigned char)** ps;
        if( !uc ) { c = EOF; }
        else {
            ++( *ps );
            c = (int)uc;
        }
    } else { c = fgetc( (FILE*)in->ctx ); }
    if( c != EOF ) { ++in->count; }
    return c;
}

static int cwasm_scan_unget( cwasm_scan_input* in, int c ) {
    if( c == EOF ) { return EOF; }
    if( in->pb_count >= in->pb_cap && !cwasm_scan_grow_pb( in ) ) { return EOF; }
    in->pb[ in->pb_count++ ] = c & 0xff;
    if( in->count > 0 ) { --in->count; }
    return c & 0xff;
}

static int cwasm_scan_get_w( cwasm_scan_input* in, int* width ) {
    int c;
    if( *width == 0 ) { return EOF; }
    c = cwasm_scan_get( in );
    if( c != EOF && *width > 0 ) { --( *width ); }
    return c;
}

static void cwasm_scan_unget_w( cwasm_scan_input* in, int c, int* width ) {
    if( c == EOF ) { return; }
    if( cwasm_scan_unget( in, c ) != EOF && *width >= 0 ) { ++( *width ); }
}

static void cwasm_scan_unget_bytes( cwasm_scan_input* in, unsigned char const* bytes, int n ) {
    while( n > 0 ) { (void)cwasm_scan_unget( in, bytes[ --n ] ); }
}

static void cwasm_scan_finish( cwasm_scan_input* in ) {
    int i;

    if( in->src == CWASM_SCAN_SRC_STR ) {
        char const** ps = (char const**)in->ctx;

        for( i = 0; i < in->pb_count; ++i ) { --( *ps ); }
    } else if( in->pb_count > 0 ) {
        FILE* fp = (FILE*)in->ctx;
        int restored = 0;

        if( in->pb_count > (int)__CWASM_STDIO_UNGET ) {
            off_t pos = ftello( fp );

            if( pos >= (off_t)in->pb_count &&
                fseeko( fp, pos - (off_t)in->pb_count, SEEK_SET ) == 0 ) {
                restored = 1;
            }
        }
        if( !restored ) {
            for( i = 0; i < in->pb_count; ++i ) {
                if( ungetc( in->pb[ i ], fp ) == EOF ) {
                    in->restore_error = 1;
                    break;
                }
            }
        }
    }
    if( in->pb_heap ) { free( in->pb ); }
    in->pb = NULL;
    in->pb_count = 0;
    in->pb_cap = 0;
}

enum {
    CWASM_SCAN_OK = 1,
    CWASM_SCAN_MATCH_FAIL = 0,
    CWASM_SCAN_INPUT_FAIL = -1
};

enum cwasm_scan_len {
    CWASM_SCAN_LEN_NONE,
    CWASM_SCAN_LEN_HH,
    CWASM_SCAN_LEN_H,
    CWASM_SCAN_LEN_L,
    CWASM_SCAN_LEN_LL,
    CWASM_SCAN_LEN_J,
    CWASM_SCAN_LEN_Z,
    CWASM_SCAN_LEN_T,
    CWASM_SCAN_LEN_CAP_L
};

static int cwasm_scan_return( cwasm_scan_input* in, int assigned, int status ) {
    int rc;
    cwasm_scan_finish( in );
    if( in->oom || in->restore_error ) {
        status = CWASM_SCAN_INPUT_FAIL;
    }
    if( status == CWASM_SCAN_INPUT_FAIL && assigned == 0 ) { rc = EOF; }
    else { rc = assigned; }
    return rc;
}

static void cwasm_scan_skip_ws( cwasm_scan_input* in ) {
    int c;
    do { c = cwasm_scan_get( in ); } while( c != EOF && cwasm_is_space( c ) );
    if( c != EOF ) { (void)cwasm_scan_unget( in, c ); }
}

static int cwasm_scan_read_wchar( cwasm_scan_input* in, mbstate_t* st,
    wchar_t* out_wc, unsigned char bytes[ 4 ], int* out_nbytes ) {
    int nbytes = 0;
    wchar_t wc = 0;
    for( ;; ) {
        int c = cwasm_scan_get( in );
        char b;
        size_t r;
        if( c == EOF ) {
            if( nbytes > 0 ) { errno = EILSEQ; }
            *out_nbytes = nbytes;
            return CWASM_SCAN_INPUT_FAIL;
        }
        if( nbytes >= 4 ) {
            bytes[ nbytes > 0 ? nbytes - 1 : 0 ] = (unsigned char)c;
            errno = EILSEQ;
            *out_nbytes = nbytes;
            return CWASM_SCAN_INPUT_FAIL;
        }
        bytes[ nbytes++ ] = (unsigned char)c;
        b = (char)(unsigned char)c;
        r = mbrtowc( &wc, &b, 1, st );
        if( r == (size_t)-1 ) {
            *out_nbytes = nbytes;
            return CWASM_SCAN_INPUT_FAIL;
        }
        if( r == (size_t)-2 ) { continue; }
        *out_wc = wc;
        *out_nbytes = nbytes;
        return CWASM_SCAN_OK;
    }
}

static void cwasm_accum_digit( uintmax_t* acc, int* overflow, int base, int d ) {
    uintmax_t lim = (uintmax_t)~(uintmax_t)0;
    if( *overflow ) { return; }
    if( *acc > ( lim - (uintmax_t)d ) / (uintmax_t)base ) {
        *acc = lim;
        *overflow = 1;
    } else {
        *acc = ( *acc * (uintmax_t)base ) + (uintmax_t)d;
    }
}

typedef struct {
    int wide;
    union {
        unsigned long narrow;
        uintmax_t wide_val;
    } u;
} cwasm_scan_mag;

static int cwasm_scan_uint( cwasm_scan_input* in, int requested_base, int width,
    enum cwasm_scan_len len, cwasm_scan_mag* out_mag, int* out_neg ) {
    int rem = ( width > 0 ) ? width : INT_MAX;
    int c;
    int c2;
    int c3;
    int neg = 0;
    int base = requested_base;
    int digits = 0;
    int overflow = 0;
    int wide = ( len == CWASM_SCAN_LEN_LL || len == CWASM_SCAN_LEN_J );
    uintmax_t acc_wide = 0;
    unsigned long acc_narrow = 0;
    #define CWASM_SCAN_ACCUM_DIGIT(d_) \
        do { \
            if (wide) cwasm_accum_digit(&acc_wide, &overflow, base, (d_)); \
            else { \
                uintmax_t t = (uintmax_t)acc_narrow; \
                cwasm_accum_digit(&t, &overflow, base, (d_)); \
                if (!overflow && t > (uintmax_t)ULONG_MAX) \
                    overflow = 1; \
                acc_narrow = overflow ? ULONG_MAX : (unsigned long)t; \
            } \
        } while (0)

    c = cwasm_scan_get_w( in, &rem );
    if( c == EOF ) { return CWASM_SCAN_INPUT_FAIL; }
    if( c == '+' || c == '-' ) {
        neg = ( c == '-' );
        if( rem == 0 ) { return CWASM_SCAN_MATCH_FAIL; }
    } else {
        cwasm_scan_unget_w( in, c, &rem );
    }

    c = cwasm_scan_get_w( in, &rem );
    if( c == EOF ) { return CWASM_SCAN_MATCH_FAIL; }

    if( base == 0 ) {
        if( c == '0' ) {
            base = 8;
            CWASM_SCAN_ACCUM_DIGIT( 0 );
            digits = 1;
            if( rem > 0 ) {
                c2 = cwasm_scan_get_w( in, &rem );
                if( c2 == 'x' || c2 == 'X' ) {
                    base = 16;
                    if( rem > 0 ) {
                        c3 = cwasm_scan_get_w( in, &rem );
                        if( c3 != EOF && cwasm_scan_digit_val( c3 ) >= 0 && cwasm_scan_digit_val( c3 ) < 16 ) {
                            acc_wide = 0;
                            acc_narrow = 0;
                            overflow = 0;
                            CWASM_SCAN_ACCUM_DIGIT( cwasm_scan_digit_val( c3 ) );
                        } else {
                            if( c3 != EOF ) { cwasm_scan_unget_w( in, c3, &rem ); }
                        }
                    }
                } else if( c2 != EOF ) {
                    cwasm_scan_unget_w( in, c2, &rem );
                }
            }
        } else {
            int d = cwasm_scan_digit_val( c );
            base = 10;
            if( d < 0 || d >= 10 ) {
                cwasm_scan_unget_w( in, c, &rem );
                return CWASM_SCAN_MATCH_FAIL;
            }
            CWASM_SCAN_ACCUM_DIGIT( d );
            digits = 1;
        }
    } else if( base == 16 ) {
        if( c == '0' && rem > 0 ) {
            c2 = cwasm_scan_get_w( in, &rem );
            if( c2 == 'x' || c2 == 'X' ) {
                if( rem > 0 ) {
                    c3 = cwasm_scan_get_w( in, &rem );
                    if( c3 != EOF && cwasm_scan_digit_val( c3 ) >= 0 && cwasm_scan_digit_val( c3 ) < 16 ) {
                        CWASM_SCAN_ACCUM_DIGIT( cwasm_scan_digit_val( c3 ) );
                        digits = 1;
                    } else {
                        if( c3 != EOF ) { cwasm_scan_unget_w( in, c3, &rem ); }
                        CWASM_SCAN_ACCUM_DIGIT( 0 );
                        digits = 1;
                    }
                } else {
                    CWASM_SCAN_ACCUM_DIGIT( 0 );
                    digits = 1;
                }
            } else {
                if( c2 != EOF ) { cwasm_scan_unget_w( in, c2, &rem ); }
                CWASM_SCAN_ACCUM_DIGIT( 0 );
                digits = 1;
            }
        } else {
            int d = cwasm_scan_digit_val( c );
            if( d < 0 || d >= 16 ) {
                cwasm_scan_unget_w( in, c, &rem );
                return CWASM_SCAN_MATCH_FAIL;
            }
            CWASM_SCAN_ACCUM_DIGIT( d );
            digits = 1;
        }
    } else {
        int d = cwasm_scan_digit_val( c );
        if( d < 0 || d >= base ) {
            cwasm_scan_unget_w( in, c, &rem );
            return CWASM_SCAN_MATCH_FAIL;
        }
        CWASM_SCAN_ACCUM_DIGIT( d );
        digits = 1;
    }

    while( rem > 0 ) {
        int d;
        c = cwasm_scan_get_w( in, &rem );
        if( c == EOF ) { break; }
        d = cwasm_scan_digit_val( c );
        if( d < 0 || d >= base ) {
            cwasm_scan_unget_w( in, c, &rem );
            break;
        }
        CWASM_SCAN_ACCUM_DIGIT( d );
        ++digits;
    }

    if( digits == 0 ) { return CWASM_SCAN_MATCH_FAIL; }
    out_mag->wide = wide;
    if( wide ) { out_mag->u.wide_val = acc_wide; }
    else { out_mag->u.narrow = acc_narrow; }
    *out_neg = neg;
    #undef CWASM_SCAN_ACCUM_DIGIT
    return CWASM_SCAN_OK;
}

static unsigned long cwasm_scan_signed_posmax( enum cwasm_scan_len len ) {
    switch( len ) {
    case CWASM_SCAN_LEN_HH: return (unsigned long)SCHAR_MAX;
    case CWASM_SCAN_LEN_H: return (unsigned long)SHRT_MAX;
    case CWASM_SCAN_LEN_L: return (unsigned long)LONG_MAX;
    case CWASM_SCAN_LEN_Z:
    case CWASM_SCAN_LEN_T: return (unsigned long)PTRDIFF_MAX;
    default: return (unsigned long)INT_MAX;
    }
}

static long cwasm_scan_signed_negmin( enum cwasm_scan_len len ) {
    switch( len ) {
    case CWASM_SCAN_LEN_HH: return SCHAR_MIN;
    case CWASM_SCAN_LEN_H: return SHRT_MIN;
    case CWASM_SCAN_LEN_L: return LONG_MIN;
    case CWASM_SCAN_LEN_Z:
    case CWASM_SCAN_LEN_T: return (long)PTRDIFF_MIN;
    default: return INT_MIN;
    }
}

static void cwasm_store_signed( va_list* ap, enum cwasm_scan_len len, intmax_t v ) {
    switch( len ) {
    case CWASM_SCAN_LEN_HH: *va_arg( *ap, signed char* ) = (signed char)( (long)v < SCHAR_MIN ? SCHAR_MIN : ( (long)v > SCHAR_MAX ? SCHAR_MAX : (long)v ) ); break;
    case CWASM_SCAN_LEN_H: *va_arg( *ap, short* ) = (short)( (long)v < SHRT_MIN ? SHRT_MIN : ( (long)v > SHRT_MAX ? SHRT_MAX : (long)v ) ); break;
    case CWASM_SCAN_LEN_L: *va_arg( *ap, long* ) = ( (long)v < LONG_MIN ? LONG_MIN : ( (long)v > LONG_MAX ? LONG_MAX : (long)v ) ); break;
    case CWASM_SCAN_LEN_LL: *va_arg( *ap, long long* ) = (long long)( v < LLONG_MIN ? LLONG_MIN : ( v > LLONG_MAX ? LLONG_MAX : v ) ); break;
    case CWASM_SCAN_LEN_J: *va_arg( *ap, intmax_t* ) = v; break;
    case CWASM_SCAN_LEN_Z: *va_arg( *ap, ptrdiff_t* ) = (ptrdiff_t)( (long)v < (long)PTRDIFF_MIN ? (long)PTRDIFF_MIN : ( (long)v > (long)PTRDIFF_MAX ? (long)PTRDIFF_MAX : (long)v ) ); break;
    case CWASM_SCAN_LEN_T: *va_arg( *ap, ptrdiff_t* ) = (ptrdiff_t)( (long)v < (long)PTRDIFF_MIN ? (long)PTRDIFF_MIN : ( (long)v > (long)PTRDIFF_MAX ? (long)PTRDIFF_MAX : (long)v ) ); break;
    default: *va_arg( *ap, int* ) = (int)( (long)v < INT_MIN ? INT_MIN : ( (long)v > INT_MAX ? INT_MAX : (long)v ) ); break;
    }
}

static void cwasm_store_unsigned( va_list* ap, enum cwasm_scan_len len, uintmax_t v ) {
    switch( len ) {
    case CWASM_SCAN_LEN_HH: *va_arg( *ap, unsigned char* ) = (unsigned char)v; break;
    case CWASM_SCAN_LEN_H: *va_arg( *ap, unsigned short* ) = (unsigned short)v; break;
    case CWASM_SCAN_LEN_L: *va_arg( *ap, unsigned long* ) = (unsigned long)v; break;
    case CWASM_SCAN_LEN_LL: *va_arg( *ap, unsigned long long* ) = (unsigned long long)v; break;
    case CWASM_SCAN_LEN_J: *va_arg( *ap, uintmax_t* ) = v; break;
    case CWASM_SCAN_LEN_Z: *va_arg( *ap, size_t* ) = (size_t)v; break;
    case CWASM_SCAN_LEN_T: *va_arg( *ap, size_t* ) = (size_t)v; break;
    default: *va_arg( *ap, unsigned int* ) = (unsigned int)v; break;
    }
}

static int cwasm_buf_append( char** buf, size_t* len, size_t* cap,
    char* small, int c ) {
    if( *len + 1 >= *cap ) {
        size_t new_cap = ( *cap < 64 ) ? 128 : ( *cap * 2 );
        char* new_buf;
        if( new_cap <= *cap ) { return 0; }
        if( *buf == small ) {
            new_buf = (char*)malloc( new_cap );
            if( new_buf ) { memcpy( new_buf, *buf, *len ); }
        } else {
            new_buf = (char*)realloc( *buf, new_cap );
        }
        if( !new_buf ) { errno = ENOMEM; return 0; }
        *buf = new_buf;
        *cap = new_cap;
    }
    ( *buf )[( *len )++ ] = (char)c;
    return 1;
}

static int cwasm_float_get_append( cwasm_scan_input* in, int* rem,
    char** buf, size_t* len, size_t* cap,
    char* small ) {
    int c = cwasm_scan_get_w( in, rem );
    if( c == EOF ) { return EOF; }
    if( !cwasm_buf_append( buf, len, cap, small, c ) ) { return -2; }
    return c;
}

#define cwasm_float_peek(in, rem) ({ int _c = cwasm_scan_get_w((in), (rem)); if (_c != EOF) cwasm_scan_unget_w((in), _c, (rem)); _c; })

static int cwasm_scan_float_token( cwasm_scan_input* in, int width,
    char** out_buf, size_t* out_len,
    char* small, size_t small_cap,
    size_t* out_commit_len ) {
    int rem = ( width > 0 ) ? width : INT_MAX;
    char* buf = small;
    size_t len = 0;
    size_t cap = small_cap;
    size_t commit_len = 0;
    int c;
    int saw_digit = 0;

    c = cwasm_scan_get_w( in, &rem );
    if( c == EOF ) { return CWASM_SCAN_INPUT_FAIL; }

    if( c == '+' || c == '-' ) {
        if( !cwasm_buf_append( &buf, &len, &cap, small, c ) ) { goto oom; }
        if( rem == 0 ) { goto fail_consumed; }
        c = cwasm_scan_get_w( in, &rem );
        if( c == EOF ) { goto fail_consumed; }
    }

    if( ( ( c >= 'A' && c <= 'Z' ) ? ( c - 'A' + 'a' ) : c ) == 'i' ) {
        char const* inf = "inf";
        char const* inity = "inity";
        int i;
        if( !cwasm_buf_append( &buf, &len, &cap, small, c ) ) { goto oom; }
        for( i = 1; i < 3; ++i ) {
            if( rem == 0 ) { goto fail_consumed; }
            c = cwasm_scan_get_w( in, &rem );
            if( c == EOF || ( ( c >= 'A' && c <= 'Z' ) ? ( c - 'A' + 'a' ) : c ) != inf[ i ] ) {
                if( c != EOF ) { cwasm_scan_unget_w( in, c, &rem ); }
                goto fail_consumed;
            }
            if( !cwasm_buf_append( &buf, &len, &cap, small, c ) ) { goto oom; }
        }
        for( i = 0; i < 5; ++i ) {
            c = cwasm_float_peek( in, &rem );
            if( c == EOF || ( ( c >= 'A' && c <= 'Z' ) ? ( c - 'A' + 'a' ) : c ) != inity[ i ] ) { break; }
            c = cwasm_float_get_append( in, &rem, &buf, &len, &cap, small );
            if( c == -2 ) { goto oom; }
        }
        goto done;
    }

    if( ( ( c >= 'A' && c <= 'Z' ) ? ( c - 'A' + 'a' ) : c ) == 'n' ) {
        char const* nan = "nan";
        int i;
        if( !cwasm_buf_append( &buf, &len, &cap, small, c ) ) { goto oom; }
        for( i = 1; i < 3; ++i ) {
            if( rem == 0 ) { goto fail_consumed; }
            c = cwasm_scan_get_w( in, &rem );
            if( c == EOF || ( ( c >= 'A' && c <= 'Z' ) ? ( c - 'A' + 'a' ) : c ) != nan[ i ] ) {
                if( c != EOF ) { cwasm_scan_unget_w( in, c, &rem ); }
                goto fail_consumed;
            }
            if( !cwasm_buf_append( &buf, &len, &cap, small, c ) ) { goto oom; }
        }
        c = cwasm_float_peek( in, &rem );
        if( c == '(' ) {
            size_t mark = len;
            int paren_ok = 0;
            c = cwasm_float_get_append( in, &rem, &buf, &len, &cap, small );
            if( c == -2 ) { goto oom; }
            while( rem > 0 ) {
                c = cwasm_scan_get_w( in, &rem );
                if( c == EOF ) { break; }
                if( !cwasm_buf_append( &buf, &len, &cap, small, c ) ) { goto oom; }
                if( c == ')' ) { paren_ok = 1; break; }
            }
            if( !paren_ok ) {
                while( len > mark ) { (void)cwasm_scan_unget( in, (unsigned char)buf[ --len ] ); }
                buf[ len ] = '\0';
            }
        }
        goto done;
    }

    if( c == '0' ) {
        int p;
        if( !cwasm_buf_append( &buf, &len, &cap, small, c ) ) { goto oom; }
        saw_digit = 1;
        p = cwasm_float_peek( in, &rem );
        if( p == 'x' || p == 'X' ) {
            int hex_digits = 0;
            c = cwasm_float_get_append( in, &rem, &buf, &len, &cap, small );
            if( c == -2 ) { goto oom; }
            while( rem > 0 ) {
                p = cwasm_float_peek( in, &rem );
                if( cwasm_scan_digit_val( p ) < 0 || cwasm_scan_digit_val( p ) >= 16 ) { break; }
                c = cwasm_float_get_append( in, &rem, &buf, &len, &cap, small );
                if( c == -2 ) { goto oom; }
                ++hex_digits;
            }
            p = cwasm_float_peek( in, &rem );
            if( cwasm_is_radix( p ) ) {
                c = cwasm_float_get_append( in, &rem, &buf, &len, &cap, small );
                if( c == -2 ) { goto oom; }
                while( rem > 0 ) {
                    p = cwasm_float_peek( in, &rem );
                    if( cwasm_scan_digit_val( p ) < 0 || cwasm_scan_digit_val( p ) >= 16 ) { break; }
                    c = cwasm_float_get_append( in, &rem, &buf, &len, &cap, small );
                    if( c == -2 ) { goto oom; }
                    ++hex_digits;
                }
            }
            if( hex_digits == 0 ) { goto fail_consumed; }
            p = cwasm_float_peek( in, &rem );
            if( p == 'p' || p == 'P' ) {
                int exp_digits = 0;
                c = cwasm_float_get_append( in, &rem, &buf, &len, &cap, small );
                if( c == -2 ) { goto oom; }
                p = cwasm_float_peek( in, &rem );
                if( p == '+' || p == '-' ) {
                    c = cwasm_float_get_append( in, &rem, &buf, &len, &cap, small );
                    if( c == -2 ) { goto oom; }
                }
                while( rem > 0 ) {
                    p = cwasm_float_peek( in, &rem );
                    if( !cwasm_is_digit10( p ) ) { break; }
                    c = cwasm_float_get_append( in, &rem, &buf, &len, &cap, small );
                    if( c == -2 ) { goto oom; }
                    ++exp_digits;
                }
                if( exp_digits == 0 ) { commit_len = len; }
            }
            goto done;
        }
    } else if( cwasm_is_digit10( c ) ) {
        if( !cwasm_buf_append( &buf, &len, &cap, small, c ) ) { goto oom; }
        saw_digit = 1;
    } else if( cwasm_is_radix( c ) ) {
        if( !cwasm_buf_append( &buf, &len, &cap, small, c ) ) { goto oom; }
    } else {
        cwasm_scan_unget_w( in, c, &rem );
        goto fail_consumed;
    }

    while( rem > 0 ) {
        c = cwasm_float_peek( in, &rem );
        if( !cwasm_is_digit10( c ) ) { break; }
        c = cwasm_float_get_append( in, &rem, &buf, &len, &cap, small );
        if( c == -2 ) { goto oom; }
        saw_digit = 1;
    }
    c = cwasm_float_peek( in, &rem );
    if( cwasm_is_radix( c ) ) {
        c = cwasm_float_get_append( in, &rem, &buf, &len, &cap, small );
        if( c == -2 ) { goto oom; }
        while( rem > 0 ) {
            c = cwasm_float_peek( in, &rem );
            if( !cwasm_is_digit10( c ) ) { break; }
            c = cwasm_float_get_append( in, &rem, &buf, &len, &cap, small );
            if( c == -2 ) { goto oom; }
            saw_digit = 1;
        }
    }
    if( !saw_digit ) { goto fail_consumed; }
    c = cwasm_float_peek( in, &rem );
    if( c == 'e' || c == 'E' ) {
        int exp_digits = 0;
        c = cwasm_float_get_append( in, &rem, &buf, &len, &cap, small );
        if( c == -2 ) { goto oom; }
        c = cwasm_float_peek( in, &rem );
        if( c == '+' || c == '-' ) {
            c = cwasm_float_get_append( in, &rem, &buf, &len, &cap, small );
            if( c == -2 ) { goto oom; }
        }
        while( rem > 0 ) {
            c = cwasm_float_peek( in, &rem );
            if( !cwasm_is_digit10( c ) ) { break; }
            c = cwasm_float_get_append( in, &rem, &buf, &len, &cap, small );
            if( c == -2 ) { goto oom; }
            ++exp_digits;
        }
        if( exp_digits == 0 ) { commit_len = len; }
    }

done:
    if( len + 1 >= cap ) {
        size_t new_cap = cap * 2;
        char* new_buf;
        if( new_cap <= cap ) { goto oom; }
        if( buf == small ) {
            new_buf = (char*)malloc( new_cap );
            if( new_buf ) { memcpy( new_buf, buf, len ); }
        } else {
            new_buf = (char*)realloc( buf, new_cap );
        }
        if( !new_buf ) { goto oom; }
        buf = new_buf;
        cap = new_cap;
    }
    buf[ len ] = '\0';
    *out_buf = buf;
    *out_len = len;
    *out_commit_len = commit_len;
    return CWASM_SCAN_OK;

fail_consumed:
    if( buf != small ) { free( buf ); }
    return CWASM_SCAN_MATCH_FAIL;

oom:
    if( buf != small ) { free( buf ); }
    errno = ENOMEM;
    return CWASM_SCAN_INPUT_FAIL;
}

static int cwasm_scan_float( cwasm_scan_input* in, int width, enum cwasm_scan_len len,
    int suppress, va_list* ap ) {
    char small[ CWASM_SCAN_TOKEN_INITIAL ];
    char* buf = small;
    char* endp;
    size_t token_len = 0;
    size_t commit_len = 0;
    int st;

    st = cwasm_scan_float_token( in, width, &buf, &token_len, small, sizeof( small ), &commit_len );
    if( st != CWASM_SCAN_OK ) { return st; }

    errno = 0;
    if( len == CWASM_SCAN_LEN_NONE ) {
        float val = strtof( buf, &endp );
        if( endp == buf ) {
            if( buf != small ) { free( buf ); }
            return CWASM_SCAN_MATCH_FAIL;
        }
        size_t keep_len = (size_t)( endp - buf );
        if( keep_len < commit_len ) { keep_len = commit_len; }
        while( token_len > keep_len ) {
            (void)cwasm_scan_unget( in, (unsigned char)buf[ --token_len ] );
        }
        if( !suppress ) { *va_arg( *ap, float* ) = val; }
    } else if( len == CWASM_SCAN_LEN_CAP_L ) {
        long double val = strtold( buf, &endp );
        if( endp == buf ) {
            if( buf != small ) { free( buf ); }
            return CWASM_SCAN_MATCH_FAIL;
        }
        size_t keep_len = (size_t)( endp - buf );
        if( keep_len < commit_len ) { keep_len = commit_len; }
        while( token_len > keep_len ) {
            (void)cwasm_scan_unget( in, (unsigned char)buf[ --token_len ] );
        }
        if( !suppress ) { *va_arg( *ap, long double* ) = val; }
    } else {
        double val = strtod( buf, &endp );
        if( endp == buf ) {
            if( buf != small ) { free( buf ); }
            return CWASM_SCAN_MATCH_FAIL;
        }
        size_t keep_len = (size_t)( endp - buf );
        if( keep_len < commit_len ) { keep_len = commit_len; }
        while( token_len > keep_len ) {
            (void)cwasm_scan_unget( in, (unsigned char)buf[ --token_len ] );
        }
        if( !suppress ) { *va_arg( *ap, double* ) = val; }
    }

    if( buf != small ) { free( buf ); }
    return CWASM_SCAN_OK;
}

static int cwasm_decode_format_wc( char const** pf, wchar_t* out ) {
    mbstate_t st;
    wchar_t wc = 0;
    char const* p = *pf;
    size_t used = 0;
    __cwasm_mbstate_reset( &st );
    for( ;; ) {
        size_t r;
        if( p[ used ] == '\0' ) { return 0; }
        r = mbrtowc( &wc, p + used, 1, &st );
        ++used;
        if( r == (size_t)-1 ) { return 0; }
        if( r == (size_t)-2 ) { continue; }
        *pf = p + used;
        *out = wc;
        return 1;
    }
}

static void cwasm_scanset_add_range( unsigned char set[ 256 ], int a, int b ) {
    int i;
    if( a > b ) {
        int t = a;
        a = b;
        b = t;
    }
    for( i = a; i <= b; ++i ) { set[(unsigned char)i ] = 1; }
}

static char const* cwasm_build_scanset( char const* fmt, unsigned char set[ 256 ], int* ok ) {
    int invert = 0;
    int prev = -1;
    int i;
    unsigned char const* p = (unsigned char const*)fmt;

    for( i = 0; i < 256; ++i ) { set[ i ] = 0; }
    *ok = 0;

    if( *p == '^' ) { invert = 1; p++; }
    if( *p == ']' ) { set[ ']' ] = 1; prev = ']'; p++; }

    while( *p && *p != ']' ) {
        int c = *p++;
        if( c == '-' && prev >= 0 && *p && *p != ']' ) {
            int end = *p++;
            cwasm_scanset_add_range( set, prev, end );
            prev = end;
        } else {
            set[(unsigned char)c ] = 1;
            prev = c;
        }
    }
    if( *p != ']' ) { return fmt; }
    ++p;
    if( invert ) { for( i = 0; i < 256; ++i ) set[ i ] = (unsigned char)!set[ i ]; }
    *ok = 1;
    return (char const*)p;
}

static char const* cwasm_build_wscanset( char const* fmt, cwasm_wscanset* ws, int* ok ) {
    char const* p = fmt;
    wchar_t prev = 0;
    int have_prev = 0;
    *ok = 0;
    cwasm_wset_init( ws );

    if( *p == '^' ) { ws->invert = 1; p++; }
    if( *p == ']' ) {
        if( !cwasm_wset_add_range( ws, L']', L']' ) ) { goto fail; }
        prev = L']';
        have_prev = 1;
        ++p;
    }

    while( *p && *p != ']' ) {
        wchar_t wc;
        if( *p == '-' && have_prev && p[ 1 ] && p[ 1 ] != ']' ) {
            wchar_t end;
            ++p;
            if( !cwasm_decode_format_wc( &p, &end ) ) { goto fail; }
            if( !cwasm_wset_add_range( ws, prev, end ) ) { goto fail; }
            prev = end;
            have_prev = 1;
        } else {
            if( !cwasm_decode_format_wc( &p, &wc ) ) { goto fail; }
            if( !cwasm_wset_add_range( ws, wc, wc ) ) { goto fail; }
            prev = wc;
            have_prev = 1;
        }
    }
    if( *p != ']' ) { goto fail; }
    ++p;
    *ok = 1;
    return p;

fail:
    cwasm_wset_free( ws );
    return fmt;
}

static int cwasm_scan_chars( cwasm_scan_input* in, int width, enum cwasm_scan_len len,
    int suppress, va_list* ap ) {
    int n = ( width > 0 ) ? width : 1;
    int i;

    if( len == CWASM_SCAN_LEN_L ) {
        wchar_t stack_buf[ 32 ];
        wchar_t* tmp = stack_buf;
        wchar_t* dst = suppress ? NULL : va_arg( *ap, wchar_t* );
        mbstate_t st;
        __cwasm_mbstate_reset( &st );
        if( !suppress && n > (int)( sizeof( stack_buf ) / sizeof( stack_buf[ 0 ] ) ) ) {
            tmp = (wchar_t*)malloc( (size_t)n * sizeof( wchar_t ) );
            if( !tmp ) { errno = ENOMEM; return CWASM_SCAN_INPUT_FAIL; }
        }
        for( i = 0; i < n; ++i ) {
            unsigned char bytes[ 4 ];
            int nb = 0;
            wchar_t wc = 0;
            int stc = cwasm_scan_read_wchar( in, &st, &wc, bytes, &nb );
            (void)bytes;
            if( stc != CWASM_SCAN_OK ) {
                if( tmp != stack_buf ) { free( tmp ); }
                return stc;
            }
            if( !suppress ) { tmp[ i ] = wc; }
        }
        if( !suppress ) {
            for( i = 0; i < n; ++i ) { dst[ i ] = tmp[ i ]; }
            if( tmp != stack_buf ) { free( tmp ); }
        }
    } else {
        char stack_buf[ 128 ];
        char* tmp = stack_buf;
        char* dst = suppress ? NULL : va_arg( *ap, char* );
        if( !suppress && n > (int)sizeof( stack_buf ) ) {
            tmp = (char*)malloc( (size_t)n );
            if( !tmp ) { errno = ENOMEM; return CWASM_SCAN_INPUT_FAIL; }
        }
        for( i = 0; i < n; ++i ) {
            int c = cwasm_scan_get( in );
            if( c == EOF ) {
                if( tmp != stack_buf ) { free( tmp ); }
                return CWASM_SCAN_INPUT_FAIL;
            }
            if( !suppress ) { tmp[ i ] = (char)c; }
        }
        if( !suppress ) {
            for( i = 0; i < n; ++i ) { dst[ i ] = tmp[ i ]; }
            if( tmp != stack_buf ) { free( tmp ); }
        }
    }
    return CWASM_SCAN_OK;
}

static int cwasm_scan_run( cwasm_scan_input* in, int width, enum cwasm_scan_len len,
    int suppress, va_list* ap, int mode,
    unsigned char const set[ 256 ], cwasm_wscanset const* wset ) {
    int rem = ( width > 0 ) ? width : INT_MAX;
    int n = 0;
    int hit_eof = 0;

    if( mode == 1 ) { cwasm_scan_skip_ws( in ); }

    if( len == CWASM_SCAN_LEN_L ) {
        wchar_t* dst = suppress ? NULL : va_arg( *ap, wchar_t* );
        mbstate_t st;
        __cwasm_mbstate_reset( &st );
        while( rem > 0 ) {
            unsigned char bytes[ 4 ];
            int nb = 0;
            wchar_t wc = 0;
            int r = cwasm_scan_read_wchar( in, &st, &wc, bytes, &nb );
            if( r != CWASM_SCAN_OK ) {
                if( nb == 0 ) { hit_eof = 1; break; }
                return CWASM_SCAN_INPUT_FAIL;
            }
            if( mode == 1 ? iswspace( (wint_t)wc ) : !cwasm_wset_contains( wset, wc ) ) {
                cwasm_scan_unget_bytes( in, bytes, nb );
                break;
            }
            if( !suppress ) { dst[ n ] = wc; }
            n++; rem--;
        }
        if( n == 0 ) { return hit_eof ? CWASM_SCAN_INPUT_FAIL : CWASM_SCAN_MATCH_FAIL; }
        if( !suppress ) { dst[ n ] = 0; }
    } else {
        char* dst = suppress ? NULL : va_arg( *ap, char* );
        while( rem > 0 ) {
            int c = cwasm_scan_get_w( in, &rem );
            if( c == EOF ) { hit_eof = 1; break; }
            if( mode == 1 ? cwasm_is_space( c ) : !set[(unsigned char)c ] ) {
                cwasm_scan_unget_w( in, c, &rem );
                break;
            }
            if( !suppress ) { dst[ n ] = (char)c; }
            ++n;
        }
        if( n == 0 ) { return hit_eof ? CWASM_SCAN_INPUT_FAIL : CWASM_SCAN_MATCH_FAIL; }
        if( !suppress ) { dst[ n ] = '\0'; }
    }
    return CWASM_SCAN_OK;
}

static int cwasm_try_scan_nil( cwasm_scan_input* in, int width ) {
    char const* nil = "(nil)";
    int rem = ( width > 0 ) ? width : INT_MAX;
    int got[ 5 ];
    int i;
    for( i = 0; i < 5; ++i ) {
        int c;
        if( rem == 0 ) { break; }
        c = cwasm_scan_get_w( in, &rem );
        if( c == EOF || ( ( c >= 'A' && c <= 'Z' ) ? ( c - 'A' + 'a' ) : c ) != nil[ i ] ) {
            if( c != EOF ) { cwasm_scan_unget_w( in, c, &rem ); }
            while( i > 0 ) { (void)cwasm_scan_unget( in, got[ --i ] ); }
            return 0;
        }
        got[ i ] = c;
    }
    if( i == 5 ) { return 1; }
    while( i > 0 ) { (void)cwasm_scan_unget( in, got[ --i ] ); }
    return 0;
}

static int cwasm_vxscanf( enum cwasm_scan_src src, void* ctx, char const* fmt, va_list ap ) {
    cwasm_scan_input in;
    int assigned = 0;
    va_list args;

    va_copy( args, ap );
    cwasm_scan_init( &in, src, ctx );

    #define CWASM_SCAN_RETURN(_status) \
        do { \
            int cwasm_scan_rc = cwasm_scan_return(&in, assigned, (_status)); \
            va_end(args); \
            return cwasm_scan_rc; \
        } while (0)

    while( *fmt ) {
        int suppress = 0;
        int width = 0;
        enum cwasm_scan_len len = CWASM_SCAN_LEN_NONE;
        char conv;

        if( cwasm_is_space( (unsigned char)*fmt ) ) {
            while( cwasm_is_space( (unsigned char)*fmt ) ) { ++fmt; }
            cwasm_scan_skip_ws( &in );
            continue;
        }

        if( *fmt != '%' ) {
            int c = cwasm_scan_get( &in );
            if( c == EOF ) { CWASM_SCAN_RETURN( CWASM_SCAN_INPUT_FAIL ); }
            if( c != (unsigned char)*fmt ) {
                (void)cwasm_scan_unget( &in, c );
                CWASM_SCAN_RETURN( CWASM_SCAN_MATCH_FAIL );
            }
            ++fmt;
            continue;
        }

        ++fmt;
        if( *fmt == '%' ) {
            int c = cwasm_scan_get( &in );
            if( c == EOF ) { CWASM_SCAN_RETURN( CWASM_SCAN_INPUT_FAIL ); }
            if( c != '%' ) {
                (void)cwasm_scan_unget( &in, c );
                CWASM_SCAN_RETURN( CWASM_SCAN_MATCH_FAIL );
            }
            ++fmt;
            continue;
        }

        if( *fmt == '*' ) { suppress = 1; fmt++; }
        while( *fmt >= '0' && *fmt <= '9' ) {
            if( width <= ( INT_MAX - 9 ) / 10 ) { width = width * 10 + ( *fmt - '0' ); }
            else { width = INT_MAX; }
            ++fmt;
        }

        if( *fmt == 'h' ) {
            ++fmt;
            if( *fmt == 'h' ) { fmt++; len = CWASM_SCAN_LEN_HH; }
            else { len = CWASM_SCAN_LEN_H; }
        } else if( *fmt == 'l' ) {
            ++fmt;
            if( *fmt == 'l' ) { fmt++; len = CWASM_SCAN_LEN_LL; }
            else { len = CWASM_SCAN_LEN_L; }
        } else if( *fmt == 'j' ) {
            fmt++; len = CWASM_SCAN_LEN_J;
        } else if( *fmt == 'z' ) {
            fmt++; len = CWASM_SCAN_LEN_Z;
        } else if( *fmt == 't' ) {
            fmt++; len = CWASM_SCAN_LEN_T;
        } else if( *fmt == 'L' ) { fmt++; len = CWASM_SCAN_LEN_CAP_L; }

        conv = *fmt++;
        if( conv == 0 ) { CWASM_SCAN_RETURN( CWASM_SCAN_MATCH_FAIL ); }

        if( conv == 'c' ) {
            int st = cwasm_scan_chars( &in, width, len, suppress, &args );
            if( st != CWASM_SCAN_OK ) { CWASM_SCAN_RETURN( st ); }
            if( !suppress ) { ++assigned; }
            continue;
        }

        if( conv == 's' ) {
            int st = cwasm_scan_run( &in, width, len, suppress, &args, 1, (unsigned char const*)0, NULL );
            if( st != CWASM_SCAN_OK ) { CWASM_SCAN_RETURN( st ); }
            if( !suppress ) { ++assigned; }
            continue;
        }

        if( conv == '[' ) {
            int ok = 0;
            int st;
            unsigned char set[ 256 ];
            cwasm_wscanset wset;
            if( len == CWASM_SCAN_LEN_L ) {
                fmt = cwasm_build_wscanset( fmt, &wset, &ok );
                if( !ok ) { CWASM_SCAN_RETURN( CWASM_SCAN_MATCH_FAIL ); }
                st = cwasm_scan_run( &in, width, len, suppress, &args, 0, set, &wset );
                cwasm_wset_free( &wset );
            } else {
                fmt = cwasm_build_scanset( fmt, set, &ok );
                if( !ok ) { CWASM_SCAN_RETURN( CWASM_SCAN_MATCH_FAIL ); }
                st = cwasm_scan_run( &in, width, len, suppress, &args, 0, set, NULL );
            }
            if( st != CWASM_SCAN_OK ) { CWASM_SCAN_RETURN( st ); }
            if( !suppress ) { ++assigned; }
            continue;
        }

        if( conv == 'n' ) {
            if( !suppress ) {
                long ncount = (long)in.count;
                cwasm_store_signed( &args, len, ( len == CWASM_SCAN_LEN_LL || len == CWASM_SCAN_LEN_J ) ? (intmax_t)ncount : ncount );
            }
            continue;
        }

        if( conv == 'p' ) {
            cwasm_scan_mag mag;
            int neg = 0;
            int st;
            if( len != CWASM_SCAN_LEN_NONE ) {
                CWASM_SCAN_RETURN( CWASM_SCAN_MATCH_FAIL );
            }
            cwasm_scan_skip_ws( &in );
            if( cwasm_try_scan_nil( &in, width ) ) {
                if( !suppress ) {
                    void** pp = va_arg( args, void** );
                    *pp = NULL;
                    ++assigned;
                }
                continue;
            }
            st = cwasm_scan_uint( &in, 16, width, CWASM_SCAN_LEN_NONE, &mag, &neg );
            if( st != CWASM_SCAN_OK ) { CWASM_SCAN_RETURN( st ); }
            if( !suppress ) {
                void** pp = va_arg( args, void** );
                (void)neg;
                *pp = (void*)(uintptr_t)mag.u.narrow;
                ++assigned;
            }
            continue;
        }

        if( conv == 'd' || conv == 'i' || conv == 'o' || conv == 'u' ||
            conv == 'x' || conv == 'X' ) {
            cwasm_scan_mag mag;
            int neg = 0;
            int base;
            int st;
            int signed_conv = ( conv == 'd' || conv == 'i' );
            if( len == CWASM_SCAN_LEN_CAP_L ) {
                CWASM_SCAN_RETURN( CWASM_SCAN_MATCH_FAIL );
            }
            if( conv == 'i' ) { base = 0; }
            else if( conv == 'o' ) { base = 8; }
            else if( conv == 'x' || conv == 'X' ) { base = 16; }
            else { base = 10; }
            cwasm_scan_skip_ws( &in );
            st = cwasm_scan_uint( &in, base, width, len, &mag, &neg );
            if( st != CWASM_SCAN_OK ) { CWASM_SCAN_RETURN( st ); }
            if( !suppress ) {
                intmax_t v;
                if( signed_conv ) {
                    if( mag.wide ) {
                        uintmax_t m = mag.u.wide_val;
                        uintmax_t posmax = (uintmax_t)INTMAX_MAX;
                        if( !neg ) { v = ( m > posmax ) ? INTMAX_MAX : (intmax_t)m; }
                        else if( m >= posmax + (uintmax_t)1 ) { v = INTMAX_MIN; }
                        else { v = -(intmax_t)m; }
                    } else {
                        unsigned long posmax = cwasm_scan_signed_posmax( len );
                        unsigned long m = mag.u.narrow;
                        if( !neg ) {
                            v = ( m > posmax ) ? (intmax_t)posmax : (intmax_t)m;
                        } else if( m > posmax + 1ul ) {
                            v = cwasm_scan_signed_negmin( len );
                        } else { v = -(intmax_t)m; }
                    }
                    cwasm_store_signed( &args, len, v );
                } else if( mag.wide ) {
                    uintmax_t m = mag.u.wide_val;
                    cwasm_store_unsigned( &args, len, neg ? ( (uintmax_t)0 - m ) : m );
                } else {
                    unsigned long m = mag.u.narrow;
                    cwasm_store_unsigned( &args, len, neg ? ( 0ul - m ) : m );
                }
                ++assigned;
            }
            continue;
        }

        if( conv == 'f' || conv == 'F' || conv == 'e' || conv == 'E' ||
            conv == 'g' || conv == 'G' || conv == 'a' || conv == 'A' ) {
            int st;
            if( len != CWASM_SCAN_LEN_NONE && len != CWASM_SCAN_LEN_L && len != CWASM_SCAN_LEN_CAP_L ) {
                CWASM_SCAN_RETURN( CWASM_SCAN_MATCH_FAIL );
            }
            cwasm_scan_skip_ws( &in );
            st = cwasm_scan_float( &in, width, len, suppress, &args );
            if( st != CWASM_SCAN_OK ) { CWASM_SCAN_RETURN( st ); }
            if( !suppress ) { ++assigned; }
            continue;
        }

        CWASM_SCAN_RETURN( CWASM_SCAN_MATCH_FAIL );
}

    CWASM_SCAN_RETURN( CWASM_SCAN_OK );
    #undef CWASM_SCAN_RETURN
}

int vsscanf( char const* s, char const* format, va_list ap ) {
    char const* p = s ? s : "";
    return cwasm_vxscanf( CWASM_SCAN_SRC_STR, (void*)&p, format, ap );
}

int sscanf( char const* s, char const* format, ... ) {
    va_list ap;
    va_start( ap, format );
    int n = vsscanf( s, format, ap );
    va_end( ap );
    return n;
}

int vfscanf( FILE* stream, char const* format, va_list ap ) {
    int n;
    flockfile( stream );
    if( __cwasm_stdio_orient_byte( stream ) != 0 ) {
        funlockfile( stream );
        return EOF;
    }
    n = cwasm_vxscanf( CWASM_SCAN_SRC_FILE, stream, format, ap );
    funlockfile( stream );
    return n;
}

int fscanf( FILE* stream, char const* format, ... ) {
    va_list ap;
    va_start( ap, format );
    int n = vfscanf( stream, format, ap );
    va_end( ap );
    return n;
}

int vscanf( char const* format, va_list ap ) {
    return vfscanf( stdin, format, ap );
}

int scanf( char const* format, ... ) {
    va_list ap;
    va_start( ap, format );
    int n = vfscanf( stdin, format, ap );
    va_end( ap );
    return n;
}

#if __STDC_VERSION__ < 201112L
    #define CWASM_GETS_MAX 8192

    char* gets( char* s ) {
        size_t n;

        if( !s ) {
            errno = EINVAL;
            return NULL;
        }
        if( !fgets( s, CWASM_GETS_MAX, stdin ) ) { return NULL; }
        n = strlen( s );
        if( n > 0 && s[ n - 1 ] == '\n' ) {
            s[ n - 1 ] = '\0';
            return s;
        }
        if( n == (size_t)CWASM_GETS_MAX - 1u ) {
            int c;
            while( ( c = fgetc( stdin ) ) != EOF && c != '\n' ) { ; }
            errno = ERANGE;
            return NULL;
        }
        return s;
    }
#endif
