#include "cwasm_fs.h"
#include "cmdline.h"
#include "cwasm_lock.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <cwasm.h>

__attribute__( ( import_module( "env" ), import_name( "__async_wait_signal" ) ) )
extern int cwasm_env_async_wait_signal( void );

#ifdef CWASM_THREADS
    #include <pthread.h>
    #include <stdatomic.h>
#endif

#define CWASM_FS_APPDATA "/?/appdata/"
#define CWASM_FS_DEFAULT_FILE_MODE 0644u
#define CWASM_FS_DEFAULT_DIR_MODE 0755u
#define CWASM_FS_SHIP_FILE_MODE 0444u

#define WDB_MAGIC_0 'W'
#define WDB_MAGIC_1 'D'
#define WDB_MAGIC_2 'B'
#define WDB_VERSION 0
#define WDB_HDR_SIZE 24u

#define WFS_MAGIC_0 'W'
#define WFS_MAGIC_1 'F'
#define WFS_MAGIC_2 'S'
#define WFS_VERSION 0
#define WFS_HEADER_SIZE 12
#define WFS_ENTRY_META_SIZE 20u

CWASM_JS_LIB( WFS, uint32_t, js_embed_size, ( void ), {
  return CWASM.embedRaw ? CWASM.embedRaw.length : 0;
})

CWASM_JS_LIB( WFS, uint32_t, js_embed_copy, ( uint32_t dst, uint32_t offset, uint32_t length ), {
  if (!CWASM.embedRaw || !dst) return 0;
  const n = Math.min(length, Math.max(0, CWASM.embedRaw.length - offset));
  if (n > 0) mem.bytes_write(dst, CWASM.embedRaw.subarray(offset, offset + n));
  return n;
})

typedef struct {
    uint32_t size;
    uint32_t mode;
    int64_t atime_sec;
    long atime_nsec;
    int64_t mtime_sec;
    long mtime_nsec;
} wfs_file_meta_t;

typedef struct {
    char* name;
    uint32_t off;
    uint32_t size;
    uint32_t mode;
    int32_t mtime_sec;
    int32_t mtime_nsec;
    int32_t atime_sec;
    int32_t atime_nsec;
} wfs_entry_t;

typedef int ( *wfs_readdir_fn )( char const* path, void* user );

static wfs_entry_t* g_wfs_entries;
static int g_wfs_count;
#ifdef CWASM_THREADS
    static _Atomic int g_wfs_inited;
#else
    static int g_wfs_inited;
#endif
static int g_wfs_has_pack;
static uint32_t g_stdin_size;
static cwasm_lock_t g_wfs_lock = CWASM_LOCK_INIT;

static void wfs_entries_clear( void ) {
    int i;

    if( !g_wfs_entries ) { return; }
    for( i = 0; i < g_wfs_count; ++i ) { free( g_wfs_entries[ i ].name ); }
    free( g_wfs_entries );
    g_wfs_entries = NULL;
    g_wfs_count = 0;
}

static int wfs_read_u32( uint8_t const* p, size_t n, size_t* off, uint32_t* out ) {
    if( *off + 4 > n ) { return 0; }
    *out = (uint32_t)p[ *off ] | ( (uint32_t)p[ *off + 1 ] << 8 ) |
    ( (uint32_t)p[ *off + 2 ] << 16 ) | ( (uint32_t)p[ *off + 3 ] << 24 );
    *off += 4;
    return 1;
}

static int wfs_read_i32( uint8_t const* p, size_t n, size_t* off, int32_t* out ) {
    uint32_t v;
    if( !wfs_read_u32( p, n, off, &v ) ) { return 0; }
    *out = (int32_t)v;
    return 1;
}

static void wfs_meta_from_entry( wfs_entry_t const* e, wfs_file_meta_t* meta ) {
    meta->size = e->size;
    meta->mode = e->mode;
    meta->atime_sec = e->atime_sec;
    meta->atime_nsec = e->atime_nsec;
    meta->mtime_sec = e->mtime_sec;
    meta->mtime_nsec = e->mtime_nsec;
}

static int wfs_parse_index( uint8_t const* blob, size_t n ) {
    size_t off = 0;
    uint32_t index_size;
    uint32_t count;
    size_t index_end;

    if( n < WFS_HEADER_SIZE ) { return 0; }
    if( blob[ 0 ] != WFS_MAGIC_0 || blob[ 1 ] != WFS_MAGIC_1 ||
        blob[ 2 ] != WFS_MAGIC_2 || blob[ 3 ] != (uint8_t)WFS_VERSION ) {
        return 0;
    }
    off = 4;
    if( !wfs_read_u32( blob, n, &off, &index_size ) ) { return 0; }
    if( !wfs_read_u32( blob, n, &off, &count ) ) { return 0; }
    if( index_size < 4u ) { return 0; }
    // index_size covers count u32 + entries, starting at byte 8 (after magic + index_size field).
    index_end = (size_t)8 + (size_t)index_size;
    if( index_end > n ) { return 0; }
    if( count == 0 ) { return 1; }

    g_wfs_entries = (wfs_entry_t*)malloc( (size_t)count * sizeof( wfs_entry_t ) );
    if( !g_wfs_entries ) { return 0; }
    memset( g_wfs_entries, 0, (size_t)count * sizeof( wfs_entry_t ) );

    for( uint32_t i = 0; i < count; ++i ) {
        uint8_t name_len;
        uint32_t off_data;
        uint32_t size;
        uint32_t mode;
        int32_t mtime_sec;
        int32_t mtime_nsec;
        int32_t atime_sec;
        int32_t atime_nsec;
        wfs_entry_t* e;

        if( off + 1 > index_end ) { goto fail; }
        name_len = blob[ off++ ];
        if( name_len == 0 || off + name_len + 8u + WFS_ENTRY_META_SIZE > index_end ) {
            goto fail;
        }
        e = &g_wfs_entries[ g_wfs_count ];
        e->name = (char*)malloc( (size_t)name_len + 1u );
        if( !e->name ) { goto fail; }
        memcpy( e->name, blob + off, name_len );
        e->name[ name_len ] = '\0';
        off += name_len;
        if( !wfs_read_u32( blob, n, &off, &off_data ) ) { goto fail; }
        if( !wfs_read_u32( blob, n, &off, &size ) ) { goto fail; }
        if( !wfs_read_u32( blob, n, &off, &mode ) ) { goto fail; }
        if( !wfs_read_i32( blob, n, &off, &mtime_sec ) ) { goto fail; }
        if( !wfs_read_i32( blob, n, &off, &mtime_nsec ) ) { goto fail; }
        if( !wfs_read_i32( blob, n, &off, &atime_sec ) ) { goto fail; }
        if( !wfs_read_i32( blob, n, &off, &atime_nsec ) ) { goto fail; }
        e->off = off_data;
        e->size = size;
        e->mode = mode;
        e->mtime_sec = mtime_sec;
        e->mtime_nsec = mtime_nsec;
        e->atime_sec = atime_sec;
        e->atime_nsec = atime_nsec;
        ++g_wfs_count;
    }
    if( off != index_end ) { goto fail; }
    return 1;

    fail:
    wfs_entries_clear();
    return 0;
}

uint32_t wfs_embed_read( uint32_t offset, void* dst, uint32_t len ) {
    if( !dst || !len ) { return 0; }
    return js_embed_copy( (uint32_t)(uintptr_t)dst, offset, len );
}

void wfs_init( void ) {
    uint32_t sz;
    uint8_t hdr[ WFS_HEADER_SIZE ];
    uint32_t index_size;
    size_t index_end;
    uint8_t* index_buf = NULL;
    size_t index_n = 0;

    #ifdef CWASM_THREADS
        if( atomic_load_explicit( &g_wfs_inited, memory_order_acquire ) ) { return; }
    #else
        if( g_wfs_inited ) { return; }
    #endif

    cwasm_lock( &g_wfs_lock );
    #ifdef CWASM_THREADS
        if( atomic_load_explicit( &g_wfs_inited, memory_order_relaxed ) ) {
            cwasm_unlock( &g_wfs_lock );
            return;
        }
    #else
        if( g_wfs_inited ) {
            cwasm_unlock( &g_wfs_lock );
            return;
        }
    #endif

    sz = js_embed_size();
    if( !sz ) {
        #ifdef CWASM_THREADS
            atomic_store_explicit( &g_wfs_inited, 1, memory_order_release );
        #else
            g_wfs_inited = 1;
        #endif
        cwasm_unlock( &g_wfs_lock );
        return;
    }

    if( wfs_embed_read( 0, hdr, WFS_HEADER_SIZE ) != WFS_HEADER_SIZE ) {
        #ifdef CWASM_THREADS
            atomic_store_explicit( &g_wfs_inited, 1, memory_order_release );
        #else
            g_wfs_inited = 1;
        #endif
        cwasm_unlock( &g_wfs_lock );
        return;
    }

    if( hdr[ 0 ] != WFS_MAGIC_0 || hdr[ 1 ] != WFS_MAGIC_1 ||
        hdr[ 2 ] != WFS_MAGIC_2 || hdr[ 3 ] != (uint8_t)WFS_VERSION ) {
        g_stdin_size = sz;
        #ifdef CWASM_THREADS
            atomic_store_explicit( &g_wfs_inited, 1, memory_order_release );
        #else
            g_wfs_inited = 1;
        #endif
        cwasm_unlock( &g_wfs_lock );
        return;
    }

    index_size = (uint32_t)hdr[ 4 ] | ( (uint32_t)hdr[ 5 ] << 8 ) |
    ( (uint32_t)hdr[ 6 ] << 16 ) | ( (uint32_t)hdr[ 7 ] << 24 );
    if( index_size < 4u ) {
        #ifdef CWASM_THREADS
            atomic_store_explicit( &g_wfs_inited, 1, memory_order_release );
        #else
            g_wfs_inited = 1;
        #endif
        cwasm_unlock( &g_wfs_lock );
        return;
    }

    index_end = (size_t)8 + (size_t)index_size;
    if( index_end > (size_t)sz ) {
        #ifdef CWASM_THREADS
            atomic_store_explicit( &g_wfs_inited, 1, memory_order_release );
        #else
            g_wfs_inited = 1;
        #endif
        cwasm_unlock( &g_wfs_lock );
        return;
    }

    index_n = index_end;
    index_buf = (uint8_t*)malloc( index_n );
    if( !index_buf ) {
        cwasm_unlock( &g_wfs_lock );
        return;
    }

    if( wfs_embed_read( 0, index_buf, (uint32_t)index_n ) != (uint32_t)index_n ) {
        free( index_buf );
        cwasm_unlock( &g_wfs_lock );
        return;
    }

    if( !wfs_parse_index( index_buf, index_n ) ) {
        free( index_buf );
        #ifdef CWASM_THREADS
            atomic_store_explicit( &g_wfs_inited, 1, memory_order_release );
        #else
            g_wfs_inited = 1;
        #endif
        cwasm_unlock( &g_wfs_lock );
        return;
    }
    g_wfs_has_pack = 1;
    #ifdef CWASM_THREADS
        atomic_store_explicit( &g_wfs_inited, 1, memory_order_release );
    #else
        g_wfs_inited = 1;
    #endif
    free( index_buf );
    cwasm_unlock( &g_wfs_lock );
}

static int wfs_has_embed( void ) {
    wfs_init();
    return g_wfs_has_pack;
}

uint32_t wfs_stdin_size( void ) {
    wfs_init();
    return g_stdin_size;
}

static int wfs_lookup( char const* path, uint32_t* off, uint32_t* size, wfs_file_meta_t* meta ) {
    int i;

    if( !path ) { return 0; }
    wfs_init();
    for( i = 0; i < g_wfs_count; ++i ) {
        if( !strcmp( path, g_wfs_entries[ i ].name ) ) {
            if( off ) { *off = g_wfs_entries[ i ].off; }
            if( size ) { *size = g_wfs_entries[ i ].size; }
            if( meta ) { wfs_meta_from_entry( &g_wfs_entries[ i ], meta ); }
            return 1;
        }
    }
    return 0;
}

static void wfs_readdir_collect( wfs_readdir_fn fn, void* user ) {
    int i;
    if( !fn ) { return; }
    wfs_init();
    for( i = 0; i < g_wfs_count; ++i ) { fn( g_wfs_entries[ i ].name, user ); }
}

typedef struct {
    mode_t mode;
    int64_t atime_sec;
    long atime_nsec;
    int64_t mtime_sec;
    long mtime_nsec;
} cwasm_fs_idb_meta_t;

CWASM_JS_LIB( FS, int, js_fs_open, ( char const* url ), {
  const u = mem.str_read(url);
  if (!u) return 0;
  if (!CWASM.fs_slots) { CWASM.fs_slots = []; CWASM.fs_url_index = {}; }
  let id = CWASM.fs_url_index[u] | 0;
  if (!id) {
    id = CWASM.fs_slots.length + 1;
    CWASM.fs_url_index[u] = id;
    CWASM.fs_slots.push({url: u, state: 0, data: null, headSize: 0, headMtime: 0});
  }
  const s = CWASM.fs_slots[id - 1];
  const EMPTY = 0, LOADING = 1, READY = 2, FAILED = 3;
  const start = () => {
    s.state = LOADING;
    s.data = null;
    s.headSize = 0;
    __async_signal_arm();
    fetch(u).then(r => {
      if (!r || !r.ok) throw 0;
      const lm = r.headers.get('Last-Modified');
      s.headMtime = lm ? ((Date.parse(lm) / 1000) | 0) : 0;
      return r.arrayBuffer();
    }).then(ab => {
      s.data = new Uint8Array(ab);
      s.state = READY;
    }).catch(() => {
      s.data = null;
      s.state = FAILED;
    }).finally(() => { __async_resume_signal(); });
  };
  if (s.state === EMPTY || s.state === FAILED || (s.state === READY && !s.data)) start();
  return id;
})

CWASM_JS_LIB( FS, int, js_fs_head, ( char const* url ), {
  const u = mem.str_read(url);
  if (!u) return 0;
  if (!CWASM.fs_slots) { CWASM.fs_slots = []; CWASM.fs_url_index = {}; }
  let id = CWASM.fs_url_index[u] | 0;
  if (!id) {
    id = CWASM.fs_slots.length + 1;
    CWASM.fs_url_index[u] = id;
    CWASM.fs_slots.push({url: u, state: 0, data: null, headSize: 0, headMtime: 0});
  }
  const s = CWASM.fs_slots[id - 1];
  const EMPTY = 0, LOADING = 1, READY = 2, FAILED = 3;
  const startHead = () => {
    s.state = LOADING;
    s.data = null;
    s.headSize = 0;
    __async_signal_arm();
    fetch(u, {method: 'HEAD'}).then(r => {
      if (!r || !r.ok) throw 0;
      const cl = r.headers.get('Content-Length');
      const lm = r.headers.get('Last-Modified');
      s.headSize = cl ? (parseInt(cl, 10) >>> 0) : 0;
      s.headMtime = lm ? ((Date.parse(lm) / 1000) | 0) : 0;
      s.state = READY;
    }).catch(() => {
      s.data = null;
      s.headSize = 0;
      s.state = FAILED;
    }).finally(() => { __async_resume_signal(); });
  };
  if (s.state === READY && s.data) return id;
  if (s.state === EMPTY || s.state === FAILED || (s.state === READY && !s.data && !s.headSize)) startHead();
  return id;
})

CWASM_JS_LIB( FS, int32_t, js_fs_mtime, ( int file_id ), {
  const id = file_id | 0;
  if (!CWASM.fs_slots || id <= 0 || id > CWASM.fs_slots.length) return 0;
  const s = CWASM.fs_slots[id - 1];
  return s.headMtime ? (s.headMtime | 0) : 0;
})

CWASM_JS_LIB( FS, int, js_fs_file_ready, ( int file_id ), {
  const id = file_id | 0;
  if (!CWASM.fs_slots || id <= 0 || id > CWASM.fs_slots.length) return -1;
  const s = CWASM.fs_slots[id - 1];
  if (s.state === 2) return 1;
  if (s.state === 3) return 0;
  if (s.state === 1) return 2;
  return -1;
})

CWASM_JS_LIB( FS, uint32_t, js_fs_size, ( int file_id ), {
  const id = file_id | 0;
  if (!CWASM.fs_slots || id <= 0 || id > CWASM.fs_slots.length) return 0;
  const s = CWASM.fs_slots[id - 1];
  if (s.data) return s.data.length >>> 0;
  return s.headSize ? (s.headSize >>> 0) : 0;
})

CWASM_JS_LIB( FS, int, js_fs_read, ( int file_id, uint32_t offset, uint32_t dst, uint32_t n ), {
  const id = file_id | 0;
  const off = offset >>> 0;
  const len = n >>> 0;
  if (!CWASM.fs_slots || id <= 0 || id > CWASM.fs_slots.length) return -1;
  const s = CWASM.fs_slots[id - 1];
  if (!s || s.state !== 2 || !s.data) return -1;
  if (!dst && len) return -1;
  const avail = s.data.length >>> 0;
  if (off >= avail) return 0;
  const take = Math.min(len, avail - off) >>> 0;
  if (take) mem.bytes_write(dst, s.data.subarray(off, off + take));
  return take | 0;
})

CWASM_JS_LIB( FS, void, js_fs_close, ( int file_id ), {
  const id = file_id | 0;
  if (!CWASM.fs_slots || id <= 0 || id > CWASM.fs_slots.length) return;
  const s = CWASM.fs_slots[id - 1];
  if (s) { s.data = null; s.state = 0; }
})

CWASM_JS_LIB( FS, int, js_fs_idb_run, ( uint32_t op, char const* key, uint32_t data, uint32_t len ), {
  const k = mem.str_read(key);
  const PUT = 1, DEL = 2, LIST = 3;
  if (!CWASM.fs_idb_slot) CWASM.fs_idb_slot = {state: 0, data: null};
  const slot = CWASM.fs_idb_slot;
  const done = (ok, bytes) => {
    if (slot.state !== 1) return;
    if (ok && bytes) slot.data = bytes;
    else if (ok) slot.data = new Uint8Array(0);
    else slot.data = null;
    slot.state = ok ? 2 : 3;
    __async_resume_signal();
  };
  const openDb = () => new Promise((res, rej) => {
    const r = indexedDB.open('cwasm-appdata', 1);
    r.onupgradeneeded = () => { r.result.createObjectStore('files'); };
    r.onsuccess = () => res(r.result);
    r.onerror = () => rej(r.error);
  });
  slot.state = 1;
  slot.data = null;
  __async_signal_arm();
  openDb().then(db => {
    const rw = op === PUT || op === DEL;
    const tx = db.transaction('files', rw ? 'readwrite' : 'readonly');
    const st = tx.objectStore('files');
    if (op === PUT) {
      const buf = len ? mem.bytes_read(data, len) : new Uint8Array(0);
      const req = st.put(buf, k);
      req.onsuccess = () => done(1, null);
      req.onerror = () => done(0, null);
    } else if (op === DEL) {
      const req = st.delete(k);
      req.onsuccess = () => done(1, null);
      req.onerror = () => done(0, null);
    } else if (op === LIST) {
      const req = st.getAllKeys();
      req.onsuccess = () => {
        const keys = req.result || [];
        const pref = k || "";
        const lines = keys.filter(x => !pref || String(x).startsWith(pref)).map(String).sort().join('\n');
        done(1, new TextEncoder().encode(lines));
      };
      req.onerror = () => done(0, null);
    } else {
      const req = st.get(k);
      req.onsuccess = () => {
        const v = req.result;
        if (v === undefined) done(0, null);
        else done(1, v instanceof Uint8Array ? v : new Uint8Array(v));
      };
      req.onerror = () => done(0, null);
    }
    tx.oncomplete = () => db.close();
    tx.onerror = () => { done(0, null); db.close(); };
  }).catch(() => done(0, null));
  return 0;
})

CWASM_JS_LIB( FS, int, js_fs_idb_ready, ( void ), {
  if (!CWASM.fs_idb_slot) return -1;
  if (CWASM.fs_idb_slot.state === 2) return 1;
  if (CWASM.fs_idb_slot.state === 3) return 0;
  if (CWASM.fs_idb_slot.state === 1) return 2;
  return -1;
})

CWASM_JS_LIB( FS, uint32_t, js_fs_idb_size, ( void ), {
  return CWASM.fs_idb_slot && CWASM.fs_idb_slot.data ? (CWASM.fs_idb_slot.data.length >>> 0) : 0;
})

CWASM_JS_LIB( FS, int, js_fs_idb_read, ( uint32_t offset, uint32_t dst, uint32_t n ), {
  const off = offset >>> 0;
  const len = n >>> 0;
  const s = CWASM.fs_idb_slot;
  if (!s || s.state !== 2 || !s.data) return -1;
  if (!dst && len) return -1;
  const avail = s.data.length >>> 0;
  if (off >= avail) return 0;
  const take = Math.min(len, avail - off) >>> 0;
  if (take) mem.bytes_write(dst, s.data.subarray(off, off + take));
  return take | 0;
})

CWASM_JS_LIB( FS, int, js_fs_storage_run, ( void ), {
  if (!CWASM.fs_storage_slot) CWASM.fs_storage_slot = {state: 0, quota: 0, usage: 0};
  const slot = CWASM.fs_storage_slot;
  const done = (ok, quota, usage) => {
    slot.state = ok ? 2 : 3;
    slot.quota = quota >>> 0;
    slot.usage = usage >>> 0;
    __async_resume_signal();
  };
  slot.state = 1;
  __async_signal_arm();
  if (navigator.storage && navigator.storage.estimate) {
    navigator.storage.estimate().then(e => {
      const q = e.quota || 0;
      const u = e.usage || 0;
      done(1, Math.ceil(q / 4096), Math.ceil(u / 4096));
    }).catch(() => done(0, 0, 0));
  } else {
    done(1, 0, 0);
  }
  return 0;
})

CWASM_JS_LIB( FS, int, js_fs_storage_ready, ( void ), {
  if (!CWASM.fs_storage_slot) return -1;
  if (CWASM.fs_storage_slot.state === 2) return 1;
  if (CWASM.fs_storage_slot.state === 3) return 0;
  if (CWASM.fs_storage_slot.state === 1) return 2;
  return -1;
})

CWASM_JS_LIB( FS, uint32_t, js_fs_storage_quota, ( void ), {
  return CWASM.fs_storage_slot ? (CWASM.fs_storage_slot.quota >>> 0) : 0;
})

CWASM_JS_LIB( FS, uint32_t, js_fs_storage_usage, ( void ), {
  return CWASM.fs_storage_slot ? (CWASM.fs_storage_slot.usage >>> 0) : 0;
})

static char g_cwd[ CWASM_FS_PATH_MAX ] = "/";
#ifndef CWASM_THREADS
    static char g_resolve_buf[ CWASM_FS_PATH_MAX ];
#endif
static char g_http_base[ CWASM_FS_PATH_MAX ];
#ifdef CWASM_THREADS
    static _Atomic int g_fs_inited;
#else
    static int g_fs_inited;
#endif
static cwasm_lock_t g_fs_lock = CWASM_LOCK_INIT;

static char** g_manifest;
static int g_manifest_n;
static int g_manifest_loaded;
static int g_manifest_loading;
#ifdef CWASM_THREADS
    static pthread_cond_t g_manifest_cv = PTHREAD_COND_INITIALIZER;
#endif

struct cwasm_fs_dir {
    char path[ CWASM_FS_PATH_MAX ];
    int is_appdata;
    int dir_fd;
    char** entries;
    int n_entries;
    int pos;
    int dotdot_added;
    struct dirent ent;
};

static char g_dir_fd_paths[ 256 ][ CWASM_FS_PATH_MAX ];
static uint8_t g_dir_fd_used[ 256 ];

int cwasm_dir_fd_is_dir( int fd ) {
    int used;
    if( fd < 0 || fd >= 256 ) { return 0; }
    // Takes the stdio table lock - do NOT call while already holding it.
    // Under the table lock use cwasm_dir_fd_is_dir_unlocked.
    cwasm_stdio_table_lock();
    used = g_dir_fd_used[ fd ] != 0;
    cwasm_stdio_table_unlock();
    return used;
}

int cwasm_dir_fd_is_dir_unlocked( int fd ) {
    if( fd < 0 || fd >= 256 ) { return 0; }
    return g_dir_fd_used[ fd ] != 0;
}

int cwasm_dir_fd_open( char const* canonical_path ) {
    int fd;

    if( !canonical_path ) {
        errno = EINVAL;
        return -1;
    }
    if( strlen( canonical_path ) + 1u > CWASM_FS_PATH_MAX ) {
        errno = ENAMETOOLONG;
        return -1;
    }
    cwasm_stdio_table_lock();
    for( fd = 3; fd < 256; ++fd ) {
        if( g_dir_fd_used[ fd ] || cwasm_stdio_fh_is_open_unlocked( (uint32_t)fd ) ) {
            continue;
        }
        strncpy( g_dir_fd_paths[ fd ], canonical_path, sizeof( g_dir_fd_paths[ fd ] ) - 1u );
        g_dir_fd_paths[ fd ][ sizeof( g_dir_fd_paths[ fd ] ) - 1u ] = '\0';
        g_dir_fd_used[ fd ] = 1;
        cwasm_stdio_table_unlock();
        return fd;
    }
    cwasm_stdio_table_unlock();
    errno = EMFILE;
    return -1;
}

int cwasm_dir_fd_path( int fd, char* buf, size_t bufsz ) {
    if( fd < 0 || fd >= 256 ) {
        errno = EBADF;
        return -1;
    }
    if( !buf || bufsz == 0 ) {
        errno = EINVAL;
        return -1;
    }
    cwasm_stdio_table_lock();
    if( !g_dir_fd_used[ fd ] ) {
        cwasm_stdio_table_unlock();
        errno = EBADF;
        return -1;
    }
    if( strlen( g_dir_fd_paths[ fd ] ) + 1u > bufsz ) {
        cwasm_stdio_table_unlock();
        errno = ERANGE;
        return -1;
    }
    strcpy( buf, g_dir_fd_paths[ fd ] );
    cwasm_stdio_table_unlock();
    return 0;
}

int cwasm_dir_fd_close( int fd ) {
    if( fd < 0 || fd >= 256 ) {
        errno = EBADF;
        return -1;
    }
    cwasm_stdio_table_lock();
    if( !g_dir_fd_used[ fd ] ) {
        cwasm_stdio_table_unlock();
        errno = EBADF;
        return -1;
    }
    g_dir_fd_paths[ fd ][ 0 ] = '\0';
    g_dir_fd_used[ fd ] = 0;
    cwasm_stdio_table_unlock();
    return 0;
}

enum { IDB_GET = 0, IDB_PUT = 1, IDB_DEL = 2, IDB_LIST = 3 };

static int cwasm_fs_normalize( char const* in, char* out, size_t outsz ) {
    char const* parts[ 64 ];
    char const* p = in;
    unsigned nparts = 0;
    size_t outlen = 1;

    if( !in || !out || outsz < 2 ) { return -1; }
    if( in[ 0 ] != '/' ) {
        errno = EINVAL;
        return -1;
    }

    ++p;
    while( *p ) {
        char const* start;
        size_t len;

        while( *p == '/' ) { ++p; }
        if( !*p ) { break; }

        start = p;
        while( *p && *p != '/' ) { ++p; }
        len = (size_t)( p - start );

        if( len == 1 && start[ 0 ] == '.' ) { continue; }
        if( len == 2 && start[ 0 ] == '.' && start[ 1 ] == '.' ) {
            if( nparts > 0 ) { --nparts; }
            continue;
        }
        if( nparts >= sizeof( parts ) / sizeof( parts[ 0 ] ) ) {
            errno = ENAMETOOLONG;
            return -1;
        }
        parts[ nparts++ ] = start;
    }

    out[ 0 ] = '/';
    out[ 1 ] = '\0';
    if( nparts == 0 ) { return 0; }

    for( unsigned i = 0; i < nparts; ++i ) {
        char const* seg = parts[ i ];
        size_t seglen = 0;

        while( seg[ seglen ] && seg[ seglen ] != '/' ) { ++seglen; }
        if( outlen + ( i > 0 ? 1u : 0u ) + seglen >= outsz ) {
            errno = ENAMETOOLONG;
            return -1;
        }
        if( i > 0 ) { out[ outlen++ ] = '/'; }
        memcpy( out + outlen, seg, seglen );
        outlen += seglen;
        out[ outlen ] = '\0';
    }
    return 0;
}

static int cwasm_fs_join( char const* dir, char const* rel, char* out, size_t outsz ) {
    char tmp[ CWASM_FS_PATH_MAX ];
    size_t dlen;
    size_t rlen;

    if( rel[ 0 ] == '/' ) { return cwasm_fs_normalize( rel, out, outsz ); }

    dlen = strlen( dir );
    rlen = strlen( rel );
    if( dlen + rlen + 2 >= sizeof( tmp ) ) {
        errno = ENAMETOOLONG;
        return -1;
    }

    memcpy( tmp, dir, dlen + 1 );
    if( rel[ 0 ] && dlen && tmp[ dlen - 1 ] != '/' ) { tmp[ dlen++ ] = '/'; }
    memcpy( tmp + dlen, rel, rlen + 1 );

    if( tmp[ 0 ] != '/' ) {
        errno = EINVAL;
        return -1;
    }
    return cwasm_fs_normalize( tmp, out, outsz );
}

int cwasm_fs_resolve( char const* in, char* out, size_t outsz ) {
    char cwd_copy[ CWASM_FS_PATH_MAX ];
    if( !in || !out || outsz < 2 ) {
        errno = EINVAL;
        return -1;
    }
    while( in[ 0 ] == '.' && in[ 1 ] == '/' ) { in += 2; }
    if( in[ 0 ] == '/' ) { return cwasm_fs_normalize( in, out, outsz ); }
    cwasm_lock( &g_fs_lock );
    memcpy( cwd_copy, g_cwd, sizeof( cwd_copy ) );
    cwasm_unlock( &g_fs_lock );
    return cwasm_fs_join( cwd_copy, in, out, outsz );
}

char const* cwasm_fs_resolve_buf( char const* in ) {
    #ifdef CWASM_THREADS
        static _Thread_local char tls_resolve_buf[ CWASM_FS_PATH_MAX ];
        if( cwasm_fs_resolve( in, tls_resolve_buf, sizeof( tls_resolve_buf ) ) != 0 ) {
            return NULL;
        }
        return tls_resolve_buf;
    #else
        if( cwasm_fs_resolve( in, g_resolve_buf, sizeof( g_resolve_buf ) ) != 0 ) {
            return NULL;
        }
        return g_resolve_buf;
    #endif
}

int cwasm_fs_dev_kind( char const* canonical ) {
    if( !canonical ) { return CWASM_FS_DEV_NONE; }
    if( strcmp( canonical, "/dev/urandom" ) == 0 || strcmp( canonical, "/dev/random" ) == 0 ) {
        return CWASM_FS_DEV_RANDOM;
    }
    if( strcmp( canonical, "/dev/null" ) == 0 ) { return CWASM_FS_DEV_NULL; }
    return CWASM_FS_DEV_NONE;
}

int cwasm_fs_dev_stat( char const* canonical, struct stat* st ) {
    int kind = cwasm_fs_dev_kind( canonical );

    if( !kind || !st ) {
        errno = kind ? EINVAL : ENOENT;
        return -1;
    }
    memset( st, 0, sizeof( *st ) );
    st->st_mode = (mode_t)( S_IFCHR | 0444u );
    if( kind == CWASM_FS_DEV_NULL ) { st->st_mode = (mode_t)( S_IFCHR | 0666u ); }
    st->st_nlink = 1;
    return 0;
}

int cwasm_fs_resolve_at( int dirfd, char const* path, char* out, size_t outsz ) {
    char base[ CWASM_FS_PATH_MAX ];

    cwasm_fs_init();
    if( !path || !out || outsz < 2 ) {
        errno = EINVAL;
        return -1;
    }
    if( dirfd == AT_FDCWD ) {
        if( !cwasm_fs_getcwd( base, sizeof( base ) ) ) { return -1; }
    } else {
        if( cwasm_dir_fd_path( dirfd, base, sizeof( base ) ) != 0 ) { return -1; }
    }
    if( path[ 0 ] == '/' ) { return cwasm_fs_normalize( path, out, outsz ); }
    return cwasm_fs_join( base, path, out, outsz );
}

static int cwasm_fs_is_appdata( char const* path ) {
    return cwasm_fs_is_appdata_path( path );
}

int cwasm_fs_is_appdata_path( char const* path ) {
    size_t plen;

    if( !path ) { return 0; }
    plen = strlen( CWASM_FS_APPDATA );
    if( strncmp( path, CWASM_FS_APPDATA, plen ) == 0 ) { return 1; }
    if( plen > 0 && CWASM_FS_APPDATA[ plen - 1 ] == '/' &&
        strncmp( path, CWASM_FS_APPDATA, plen - 1 ) == 0 && path[ plen - 1 ] == '\0' ) {
        return 1;
    }
    return 0;
}

static int cwasm_fs_appdata_key( char const* path, char* key, size_t keysz ) {
    size_t plen = strlen( CWASM_FS_APPDATA );
    size_t klen;

    if( !cwasm_fs_is_appdata( path ) ) {
        errno = EINVAL;
        return -1;
    }
    klen = strlen( path + plen );
    if( keysz < klen + 2u ) {
        errno = ENAMETOOLONG;
        return -1;
    }
    key[ 0 ] = '/';
    memcpy( key + 1, path + plen, klen + 1u );
    return 0;
}

static int cwasm_fs_appdata_vpath_from_key( char const* key, char* out, size_t outsz ) {
    size_t plen = strlen( CWASM_FS_APPDATA );
    size_t klen;

    if( !key || key[ 0 ] != '/' ) {
        errno = EINVAL;
        return -1;
    }
    klen = strlen( key + 1 );
    if( plen + klen + 1u > outsz ) {
        errno = ENAMETOOLONG;
        return -1;
    }
    memcpy( out, CWASM_FS_APPDATA, plen );
    memcpy( out + plen, key + 1, klen + 1u );
    return 0;
}

static void cwasm_fs_refresh_http_base( void ) {
    char page[ CWASM_FS_PATH_MAX ];
    char* slash;

    // Caller must hold g_fs_lock.
    g_http_base[ 0 ] = '\0';
    if( cwasm_fetch_cmdline_prefix( page, sizeof( page ) ) != 0 ) { return; }
    slash = strrchr( page, '/' );
    if( !slash ) { return; }
    slash[ 1 ] = '\0';
    strncpy( g_http_base, page, sizeof( g_http_base ) - 1u );
    g_http_base[ sizeof( g_http_base ) - 1u ] = '\0';
}

static int cwasm_fs_http_url( char const* canonical, char* url, size_t urlsz ) {
    char base[ sizeof( g_http_base ) ];

    if( canonical[ 0 ] != '/' ) {
        errno = EINVAL;
        return -1;
    }

    cwasm_lock( &g_fs_lock );
    if( !g_http_base[ 0 ] ) { cwasm_fs_refresh_http_base(); }
    if( !g_http_base[ 0 ] ) {
        cwasm_unlock( &g_fs_lock );
        errno = ENOENT;
        return -1;
    }
    memcpy( base, g_http_base, sizeof( base ) );
    cwasm_unlock( &g_fs_lock );

    if( snprintf( url, urlsz, "%s%s", base, canonical + 1 ) >= (int)urlsz ) {
        errno = ENAMETOOLONG;
        return -1;
    }
    return 0;
}

static int cwasm_fs_http_load( char const* url, int* out_id ) {
    int id;
    int st;

    id = js_fs_open( url );
    if( id <= 0 ) {
        errno = EIO;
        return -1;
    }
    cwasm_env_async_wait_signal();
    st = js_fs_file_ready( id );
    if( st != 1 ) {
        errno = ENOENT;
        return -1;
    }
    *out_id = id;
    return 0;
}

static int cwasm_fs_http_head_load( char const* url, uint32_t* out_size, int32_t* out_mtime ) {
    int id;
    int st;

    id = js_fs_head( url );
    if( id <= 0 ) {
        errno = EIO;
        return -1;
    }
    cwasm_env_async_wait_signal();
    st = js_fs_file_ready( id );
    if( st != 1 ) {
        errno = ENOENT;
        return -1;
    }
    *out_size = js_fs_size( id );
    if( out_mtime ) { *out_mtime = js_fs_mtime( id ); }
    return 0;
}

static int cwasm_fs_idb_op( uint32_t op, char const* key, void const* data, uint32_t len ) {
    int st;

    js_fs_idb_run( op, key, (uint32_t)(uintptr_t)data, len );
    cwasm_env_async_wait_signal();
    st = js_fs_idb_ready();
    if( st != 1 ) {
        if( op == IDB_GET ) { errno = ENOENT; }
        else if( op == IDB_PUT ) { errno = ENOSPC; }
        else { errno = EIO; }
        return -1;
    }
    return 0;
}

static int cwasm_fs_idb_hdr_valid( uint8_t const* buf, uint32_t len ) {
    if( len < WDB_HDR_SIZE ) { return 0; }
    return buf[ 0 ] == WDB_MAGIC_0 && buf[ 1 ] == WDB_MAGIC_1 &&
        buf[ 2 ] == WDB_MAGIC_2 && buf[ 3 ] == (uint8_t)WDB_VERSION;
}

static void cwasm_fs_idb_meta_default( cwasm_fs_idb_meta_t* meta, mode_t mode ) {
    meta->mode = mode & 0777u;
    meta->atime_sec = 0;
    meta->atime_nsec = 0;
    meta->mtime_sec = 0;
    meta->mtime_nsec = 0;
}

static void cwasm_fs_idb_meta_encode( cwasm_fs_idb_meta_t const* meta, uint8_t hdr[ WDB_HDR_SIZE ] ) {
    uint32_t mode = meta->mode & 0777u;
    int32_t ats = (int32_t)meta->atime_sec;
    int32_t atn = (int32_t)meta->atime_nsec;
    int32_t mts = (int32_t)meta->mtime_sec;
    int32_t mtn = (int32_t)meta->mtime_nsec;

    hdr[ 0 ] = WDB_MAGIC_0;
    hdr[ 1 ] = WDB_MAGIC_1;
    hdr[ 2 ] = WDB_MAGIC_2;
    hdr[ 3 ] = (uint8_t)WDB_VERSION;
    memcpy( hdr + 4, &mode, 4 );
    memcpy( hdr + 8, &ats, 4 );
    memcpy( hdr + 12, &atn, 4 );
    memcpy( hdr + 16, &mts, 4 );
    memcpy( hdr + 20, &mtn, 4 );
}

static int cwasm_fs_idb_meta_decode( uint8_t const* buf, uint32_t len, cwasm_fs_idb_meta_t* meta,
    uint32_t* data_off, uint32_t* data_len ) {
    uint32_t mode;
    int32_t ats;
    int32_t atn;
    int32_t mts;
    int32_t mtn;

    if( !cwasm_fs_idb_hdr_valid( buf, len ) ) { return EIO; }
    memcpy( &mode, buf + 4, 4 );
    memcpy( &ats, buf + 8, 4 );
    memcpy( &atn, buf + 12, 4 );
    memcpy( &mts, buf + 16, 4 );
    memcpy( &mtn, buf + 20, 4 );
    meta->mode = mode & 0777u;
    meta->atime_sec = ats;
    meta->atime_nsec = atn;
    meta->mtime_sec = mts;
    meta->mtime_nsec = mtn;
    *data_off = WDB_HDR_SIZE;
    *data_len = len >= WDB_HDR_SIZE ? len - WDB_HDR_SIZE : 0u;
    return 0;
}

static void cwasm_fs_handle_meta_get( cwasm_fs_handle_t const* h, cwasm_fs_idb_meta_t* meta ) {
    meta->mode = h->file_mode & 0777u;
    meta->atime_sec = h->meta_atime_sec;
    meta->atime_nsec = h->meta_atime_nsec;
    meta->mtime_sec = h->meta_mtime_sec;
    meta->mtime_nsec = h->meta_mtime_nsec;
}

static void cwasm_fs_handle_meta_set( cwasm_fs_handle_t* h, cwasm_fs_idb_meta_t const* meta ) {
    h->file_mode = ( h->file_mode & S_IFMT ) | ( meta->mode & 0777u );
    h->meta_atime_sec = meta->atime_sec;
    h->meta_atime_nsec = meta->atime_nsec;
    h->meta_mtime_sec = meta->mtime_sec;
    h->meta_mtime_nsec = meta->mtime_nsec;
}

static void cwasm_fs_handle_touch_mtime( cwasm_fs_handle_t* h ) {
    time_t now = time( NULL );
    h->meta_mtime_sec = now;
    h->meta_mtime_nsec = 0;
}

static int cwasm_fs_idb_unpack_raw( uint8_t const* raw, uint32_t raw_sz, uint8_t** payload,
    uint32_t* payload_sz, cwasm_fs_idb_meta_t* meta ) {
    uint32_t off;
    uint32_t len;
    int err;

    err = cwasm_fs_idb_meta_decode( raw, raw_sz, meta, &off, &len );
    if( err ) { return err; }
    *payload = (uint8_t*)malloc( len ? len : 1u );
    if( !*payload ) { return ENOMEM; }
    if( len ) { memcpy( *payload, raw + off, len ); }
    *payload_sz = len;
    return 0;
}

static int cwasm_fs_idb_pack_raw( uint8_t const* payload, uint32_t payload_sz,
    cwasm_fs_idb_meta_t const* meta, uint8_t** out, uint32_t* out_sz ) {
    uint8_t hdr[ WDB_HDR_SIZE ];

    cwasm_fs_idb_meta_encode( meta, hdr );
    *out_sz = WDB_HDR_SIZE + payload_sz;
    *out = (uint8_t*)malloc( *out_sz );
    if( !*out ) { return ENOMEM; }
    memcpy( *out, hdr, WDB_HDR_SIZE );
    if( payload_sz ) { memcpy( *out + WDB_HDR_SIZE, payload, payload_sz ); }
    return 0;
}

static int cwasm_fs_idb_load_slot( uint8_t** payload, uint32_t* payload_sz, cwasm_fs_idb_meta_t* meta ) {
    uint32_t raw_sz;
    uint8_t* raw;
    int err;

    raw_sz = js_fs_idb_size();
    raw = (uint8_t*)malloc( raw_sz ? raw_sz : 1u );
    if( !raw ) { return ENOMEM; }
    if( raw_sz ) { js_fs_idb_read( 0, (uint32_t)(uintptr_t)raw, raw_sz ); }
    err = cwasm_fs_idb_unpack_raw( raw, raw_sz, payload, payload_sz, meta );
    free( raw );
    return err;
}

static int cwasm_fs_idb_put_payload( char const* key, uint8_t const* payload, uint32_t payload_sz,
    cwasm_fs_idb_meta_t const* meta ) {
    uint8_t* packed;
    uint32_t packed_sz;
    int err;

    err = cwasm_fs_idb_pack_raw( payload, payload_sz, meta, &packed, &packed_sz );
    if( err ) { return err; }
    err = cwasm_fs_idb_op( IDB_PUT, key, packed, packed_sz ) == 0 ? 0 : EIO;
    free( packed );
    return err;
}

static int cwasm_fs_cmp_str( void const* a, void const* b ) {
    return strcmp( *(char const* const*)a, *(char const* const*)b );
}

static int cwasm_fs_dir_prefix( char const* dir, char* out, size_t outsz ) {
    size_t len = strlen( dir );
    if( len == 0 ) {
        out[ 0 ] = '/';
        out[ 1 ] = '\0';
        return 0;
    }
    if( len + 2 > outsz ) {
        errno = ENAMETOOLONG;
        return -1;
    }
    memcpy( out, dir, len + 1 );
    if( out[ len - 1 ] != '/' ) {
        out[ len ] = '/';
        out[ len + 1 ] = '\0';
    }
    return 0;
}

static int cwasm_fs_readdir_add( char*** list, int* n, int* cap, char const* name ) {
    int i;

    for( i = 0; i < *n; ++i ) {
        if( !strcmp( ( *list )[ i ], name ) ) { return 0; }
    }
    if( *n == *cap ) {
        int nc = *cap ? *cap * 2 : 8;
        char** nv = (char**)realloc( *list, (size_t)nc * sizeof( char* ) );
        if( !nv ) { return -1; }
        *list = nv;
        *cap = nc;
    }
    ( *list )[ *n ] = strdup( name );
    if( !( *list )[ *n ] ) { return -1; }
    ++( *n );
    return 0;
}

static void cwasm_fs_readdir_from_path( char const* dir, char const* path, char*** list, int* n, int* cap ) {
    char prefix[ CWASM_FS_PATH_MAX ];
    size_t plen;
    char const* rest;
    char const* slash;
    char seg[ 256 ];

    if( cwasm_fs_dir_prefix( dir, prefix, sizeof( prefix ) ) != 0 ) { return; }
    plen = strlen( prefix );
    if( strncmp( path, prefix, plen ) != 0 ) { return; }
    if( path[ plen ] == '\0' ) { return; }
    rest = path + plen;
    slash = strchr( rest, '/' );
    if( slash ) {
        size_t seglen = (size_t)( slash - rest );
        if( seglen >= sizeof( seg ) ) { return; }
        memcpy( seg, rest, seglen );
        seg[ seglen ] = '\0';
    } else {
        if( strlen( rest ) >= sizeof( seg ) ) { return; }
        strcpy( seg, rest );
    }
    cwasm_fs_readdir_add( list, n, cap, seg );
}

typedef struct {
    char const* dir;
    char** list;
    int* n;
    int* cap;
} readdir_ctx_t;

static int wfs_collect_cb( char const* path, void* user ) {
    readdir_ctx_t* ctx = (readdir_ctx_t*)user;
    cwasm_fs_readdir_from_path( ctx->dir, path, &ctx->list, ctx->n, ctx->cap );
    return 0;
}

static void cwasm_fs_manifest_free( void ) {
    int i;
    if( !g_manifest ) { return; }
    for( i = 0; i < g_manifest_n; ++i ) { free( g_manifest[ i ] ); }
    free( g_manifest );
    g_manifest = NULL;
    g_manifest_n = 0;
}

static int cwasm_fs_manifest_parse( char const* text, size_t len ) {
    size_t i = 0;
    int cap = 0;

    cwasm_fs_manifest_free();
    while( i < len ) {
        size_t start = i;
        while( i < len && text[ i ] != '\n' && text[ i ] != '\r' ) { ++i; }
        if( i > start ) {
            char* line = (char*)malloc( i - start + 1u );
            if( !line ) { return -1; }
            memcpy( line, text + start, i - start );
            line[ i - start ] = '\0';
            if( line[ 0 ] == '/' ) {
                if( g_manifest_n == cap ) {
                    int nc = cap ? cap * 2 : 16;
                    char** nv = (char**)realloc( g_manifest, (size_t)nc * sizeof( char* ) );
                    if( !nv ) {
                        free( line );
                        return -1;
                    }
                    g_manifest = nv;
                    cap = nc;
                }
                g_manifest[ g_manifest_n++ ] = line;
            } else {
                free( line );
            }
        }
        while( i < len && ( text[ i ] == '\n' || text[ i ] == '\r' ) ) { ++i; }
    }
    if( g_manifest_n > 1 ) {
        qsort( g_manifest, (size_t)g_manifest_n, sizeof( char* ), cwasm_fs_cmp_str );
    }
    return 0;
}

static int cwasm_fs_manifest_load( void ) {
    char url[ CWASM_FS_PATH_MAX * 2 ];
    char http_base[ sizeof( g_http_base ) ];
    int id;
    uint32_t sz;
    char* buf;

    cwasm_lock( &g_fs_lock );
    for( ;; ) {
        if( g_manifest_loaded ) {
            cwasm_unlock( &g_fs_lock );
            return 0;
        }
        if( !g_manifest_loading ) {
            g_manifest_loading = 1;
            break;
        }
        #ifdef CWASM_THREADS
            pthread_cond_wait( &g_manifest_cv, &g_fs_lock );
        #else
            cwasm_unlock( &g_fs_lock );
            cwasm_lock( &g_fs_lock );
        #endif
    }

    if( !g_http_base[ 0 ] ) { cwasm_fs_refresh_http_base(); }
    if( !g_http_base[ 0 ] ) {
        g_manifest_loaded = 1;
        g_manifest_loading = 0;
        #ifdef CWASM_THREADS
            pthread_cond_broadcast( &g_manifest_cv );
        #endif
        cwasm_unlock( &g_fs_lock );
        return 0;
    }
    if( snprintf( url, sizeof( url ), "%sCWASM_FILES", g_http_base ) >= (int)sizeof( url ) ) {
        g_manifest_loaded = 1;
        g_manifest_loading = 0;
        #ifdef CWASM_THREADS
            pthread_cond_broadcast( &g_manifest_cv );
        #endif
        cwasm_unlock( &g_fs_lock );
        return 0;
    }
    memcpy( http_base, g_http_base, sizeof( http_base ) );
    cwasm_unlock( &g_fs_lock );

    if( cwasm_fs_http_load( url, &id ) != 0 ) {
        if( snprintf( url, sizeof( url ), "%sCWASM_FILES_AUTO.php", http_base ) >= (int)sizeof( url ) ) {
            cwasm_lock( &g_fs_lock );
            g_manifest_loaded = 1;
            g_manifest_loading = 0;
            #ifdef CWASM_THREADS
                pthread_cond_broadcast( &g_manifest_cv );
            #endif
            cwasm_unlock( &g_fs_lock );
            return 0;
        }
        if( cwasm_fs_http_load( url, &id ) != 0 ) {
            cwasm_lock( &g_fs_lock );
            g_manifest_loaded = 1;
            g_manifest_loading = 0;
            #ifdef CWASM_THREADS
                pthread_cond_broadcast( &g_manifest_cv );
            #endif
            cwasm_unlock( &g_fs_lock );
            return 0;
        }
    }

    sz = js_fs_size( id );
    buf = (char*)malloc( sz ? sz : 1u );
    if( !buf ) {
        js_fs_close( id );
        cwasm_lock( &g_fs_lock );
        g_manifest_loading = 0;
        #ifdef CWASM_THREADS
            pthread_cond_broadcast( &g_manifest_cv );
        #endif
        cwasm_unlock( &g_fs_lock );
        return -1;
    }
    if( sz && js_fs_read( id, 0, (uint32_t)(uintptr_t)buf, sz ) != (int)sz ) {
        free( buf );
        js_fs_close( id );
        cwasm_lock( &g_fs_lock );
        g_manifest_loading = 0;
        #ifdef CWASM_THREADS
            pthread_cond_broadcast( &g_manifest_cv );
        #endif
        cwasm_unlock( &g_fs_lock );
        return -1;
    }
    cwasm_lock( &g_fs_lock );
    if( cwasm_fs_manifest_parse( buf, sz ) != 0 ) {
        cwasm_fs_manifest_free();
        g_manifest_loading = 0;
        #ifdef CWASM_THREADS
            pthread_cond_broadcast( &g_manifest_cv );
        #endif
        cwasm_unlock( &g_fs_lock );
        free( buf );
        js_fs_close( id );
        return -1;
    }
    g_manifest_loaded = 1;
    g_manifest_loading = 0;
    #ifdef CWASM_THREADS
        pthread_cond_broadcast( &g_manifest_cv );
    #endif
    cwasm_unlock( &g_fs_lock );
    free( buf );
    js_fs_close( id );
    return 0;
}

void cwasm_fs_init( void ) {
    // Idempotent: gating on g_fs_inited would permanently skip a failed WFS pack.
    wfs_init();

    cwasm_lock( &g_fs_lock );
    #ifdef CWASM_THREADS
        if( !atomic_load_explicit( &g_fs_inited, memory_order_relaxed ) ) {
            cwasm_fs_refresh_http_base();
            atomic_store_explicit( &g_fs_inited, 1, memory_order_release );
        }
    #else
        if( !g_fs_inited ) {
            cwasm_fs_refresh_http_base();
            g_fs_inited = 1;
        }
    #endif
    cwasm_unlock( &g_fs_lock );
}

char* cwasm_fs_getcwd( char* buf, size_t size ) {
    if( !buf ) {
        errno = EINVAL;
        return NULL;
    }
    if( size < 2 ) {
        errno = ERANGE;
        return NULL;
    }
    cwasm_lock( &g_fs_lock );
    if( strlen( g_cwd ) + 1 > size ) {
        cwasm_unlock( &g_fs_lock );
        errno = ERANGE;
        return NULL;
    }
    strcpy( buf, g_cwd );
    cwasm_unlock( &g_fs_lock );
    return buf;
}

static int cwasm_fs_is_virtual_dir( char const* canonical ) {
    size_t plen;

    if( !canonical ) { return 0; }
    if( !strcmp( canonical, "/?" ) ) { return 1; }
    if( !strcmp( canonical, CWASM_FS_APPDATA ) ) { return 1; }
    plen = strlen( CWASM_FS_APPDATA );
    if( plen > 0 && CWASM_FS_APPDATA[ plen - 1 ] == '/' &&
        strncmp( canonical, CWASM_FS_APPDATA, plen - 1 ) == 0 && canonical[ plen - 1 ] == '\0' ) {
        return 1;
    }
    return 0;
}

static int cwasm_fs_path_is_dir( char const* canonical, int is_appdata ) {
    char prefix[ CWASM_FS_PATH_MAX ];
    int i;
    int cap = 0;
    int n = 0;
    char** list = NULL;
    readdir_ctx_t ctx;
    int found = 0;

    if( !strcmp( canonical, "/" ) ) { return 1; }
    if( cwasm_fs_is_virtual_dir( canonical ) ) { return 1; }
    if( is_appdata && !strcmp( canonical, CWASM_FS_APPDATA ) ) { return 1; }

    if( cwasm_fs_dir_prefix( canonical, prefix, sizeof( prefix ) ) != 0 ) { return 0; }

    if( is_appdata ) {
        char key[ CWASM_FS_PATH_MAX ];
        char marker[ CWASM_FS_PATH_MAX ];
        if( cwasm_fs_appdata_key( canonical, key, sizeof( key ) ) != 0 ) { return 0; }
        snprintf( marker, sizeof( marker ), "%s/", key );
        if( cwasm_fs_idb_op( IDB_GET, marker, NULL, 0 ) == 0 ) {
            found = 1;
            goto out;
        }
        if( cwasm_fs_idb_op( IDB_LIST, marker, NULL, 0 ) != 0 ) { goto out; }
        {
            uint32_t sz = js_fs_idb_size();
            char* text = (char*)malloc( sz + 1u );
            char const* p;
    char const* end;
            size_t markerlen = strlen( marker );
            if( !text ) { goto out; }
            if( sz ) { js_fs_idb_read( 0, (uint32_t)(uintptr_t)text, sz ); }
            text[ sz ] = '\0';
            p = text;
            while( *p ) {
                end = p + strcspn( p, "\n" );
                if( end > p && (size_t)( end - p ) >= markerlen && memcmp( p, marker, markerlen ) == 0 ) {
                    found = 1;
                    break;
                }
                p = ( *end == '\n' ) ? end + 1 : end;
            }
            free( text );
        }
        goto out;
    }

    if( wfs_has_embed() ) {
        ctx.dir = canonical;
        ctx.list = list;
        ctx.n = &n;
        ctx.cap = &cap;
        wfs_readdir_collect( wfs_collect_cb, &ctx );
        list = ctx.list;
        found = n > 0;
        goto out;
    }

    if( cwasm_fs_manifest_load() == 0 ) {
        // Once loaded the table is immutable; only walk after a successful load
        // so a failed loader's retry cannot free the table under our feet
        cwasm_lock( &g_fs_lock );
        for( i = 0; i < g_manifest_n; ++i ) {
            cwasm_fs_readdir_from_path( canonical, g_manifest[ i ], &list, &n, &cap );
        }
        cwasm_unlock( &g_fs_lock );
        found = n > 0;
    }

out:
    for( i = 0; i < n; ++i ) { free( list[ i ] ); }
    free( list );
    return found;
}

int cwasm_fs_chdir( char const* path ) {
    char resolved[ CWASM_FS_PATH_MAX ];
    int is_app;

    cwasm_fs_init();
    if( !path ) {
        errno = EINVAL;
        return -1;
    }
    if( cwasm_fs_resolve( path, resolved, sizeof( resolved ) ) != 0 ) { return -1; }
    is_app = cwasm_fs_is_appdata( resolved );
    if( !cwasm_fs_path_is_dir( resolved, is_app ) ) {
        errno = ENOENT;
        return -1;
    }
    cwasm_lock( &g_fs_lock );
    strcpy( g_cwd, resolved );
    cwasm_unlock( &g_fs_lock );
    return 0;
}

static int cwasm_fs_wfs_open( char const* canonical, unsigned flags, cwasm_fs_handle_t* h ) {
    uint32_t off;
    uint32_t size;
    wfs_file_meta_t meta;

    if( flags & ( CWASM_FS_OPEN_WRITE | CWASM_FS_OPEN_CREATE | CWASM_FS_OPEN_TRUNCATE ) ) {
        return EACCES;
    }
    if( !wfs_lookup( canonical, &off, &size, &meta ) ) { return ENOENT; }
    memset( h, 0, sizeof( *h ) );
    h->kind = CWASM_FS_HKIND_WFS;
    h->path = strdup( canonical );
    if( !h->path ) { return ENOMEM; }
    h->wfs_off = off;
    h->wfs_size = size;
    h->append_mode = ( flags & CWASM_FS_OPEN_APPEND ) != 0;
    h->pos = h->append_mode ? size : 0u;
    h->file_mode = S_IFREG | ( meta.mode & 0777u );
    h->meta_atime_sec = meta.atime_sec;
    h->meta_atime_nsec = meta.atime_nsec;
    h->meta_mtime_sec = meta.mtime_sec;
    h->meta_mtime_nsec = meta.mtime_nsec;
    return 0;
}

static int cwasm_fs_http_open( char const* canonical, unsigned flags, cwasm_fs_handle_t* h ) {
    char url[ CWASM_FS_PATH_MAX * 2 ];
    int id;

    if( flags & ( CWASM_FS_OPEN_WRITE | CWASM_FS_OPEN_CREATE | CWASM_FS_OPEN_TRUNCATE ) ) {
        return EACCES;
    }
    if( cwasm_fs_path_is_dir( canonical, 0 ) ) { return ENOENT; }
    if( cwasm_fs_http_url( canonical, url, sizeof( url ) ) != 0 ) {
        return errno ? errno : ENOENT;
    }
    if( cwasm_fs_http_load( url, &id ) != 0 ) { return errno ? errno : ENOENT; }
    memset( h, 0, sizeof( *h ) );
    h->kind = CWASM_FS_HKIND_HTTP;
    h->path = strdup( canonical );
    if( !h->path ) {
        js_fs_close( id );
        return ENOMEM;
    }
    h->js_file_id = id;
    h->pos = 0;
    h->file_mode = S_IFREG | CWASM_FS_SHIP_FILE_MODE;
    h->meta_atime_sec = 0;
    h->meta_atime_nsec = 0;
    h->meta_mtime_sec = js_fs_mtime( id );
    h->meta_mtime_nsec = 0;
    return 0;
}

static int cwasm_fs_idb_parent_exists( char const* key ) {
    char parent[ CWASM_FS_PATH_MAX ];
    char const* slash;
    size_t plen;

    slash = strrchr( key, '/' );
    if( !slash || slash == key ) { return 1; }
    plen = (size_t)( slash - key );
    if( plen + 2u > sizeof( parent ) ) { return 0; }
    memcpy( parent, key, plen );
    parent[ plen ] = '/';
    parent[ plen + 1u ] = '\0';
    return cwasm_fs_idb_op( IDB_GET, parent, NULL, 0 ) == 0;
}

static int cwasm_fs_idb_open( char const* canonical, unsigned flags, mode_t mode, cwasm_fs_handle_t* h ) {
    char key[ CWASM_FS_PATH_MAX ];
    int read = ( flags & CWASM_FS_OPEN_READ ) != 0;
    int write = ( flags & ( CWASM_FS_OPEN_WRITE | CWASM_FS_OPEN_CREATE | CWASM_FS_OPEN_TRUNCATE | CWASM_FS_OPEN_APPEND ) ) != 0;

    if( cwasm_fs_appdata_key( canonical, key, sizeof( key ) ) != 0 ) { return EINVAL; }
    if( key[ strlen( key ) - 1 ] == '/' ) {
        if( write ) { return EISDIR; }
        return ENOENT;
    }

    memset( h, 0, sizeof( *h ) );
    h->kind = CWASM_FS_HKIND_IDB;
    h->path = strdup( canonical );
    if( !h->path ) { return ENOMEM; }

    if( read && !write ) {
        cwasm_fs_idb_meta_t meta;
        uint8_t* payload;
        uint32_t payload_sz;

        if( cwasm_fs_idb_op( IDB_GET, key, NULL, 0 ) != 0 ) {
            free( h->path );
            return ENOENT;
        }
        int err = cwasm_fs_idb_load_slot( &payload, &payload_sz, &meta );
        if( err ) {
            free( h->path );
            return err;
        }
        h->idb_buf = payload;
        h->idb_size = payload_sz;
        h->idb_cap = payload_sz ? payload_sz : 1u;
        h->append_mode = ( flags & CWASM_FS_OPEN_APPEND ) != 0;
        h->pos = h->append_mode ? payload_sz : 0u;
        h->file_mode = S_IFREG | meta.mode;
        cwasm_fs_handle_meta_set( h, &meta );
        return 0;
    }

    if( write ) {
        int load_existing = 0;

        if( ( flags & CWASM_FS_OPEN_EXCLUSIVE ) && ( flags & CWASM_FS_OPEN_CREATE ) &&
            cwasm_fs_idb_op( IDB_GET, key, NULL, 0 ) == 0 ) {
            free( h->path );
            return EEXIST;
        }
        if( !( flags & CWASM_FS_OPEN_CREATE ) && cwasm_fs_idb_op( IDB_GET, key, NULL, 0 ) != 0 ) {
            free( h->path );
            return ENOENT;
        }
        if( ( flags & CWASM_FS_OPEN_CREATE ) && cwasm_fs_idb_op( IDB_GET, key, NULL, 0 ) != 0 &&
            !cwasm_fs_idb_parent_exists( key ) ) {
            free( h->path );
            return ENOENT;
        }
        if( !( flags & CWASM_FS_OPEN_TRUNCATE ) ) {
            if( cwasm_fs_idb_op( IDB_GET, key, NULL, 0 ) == 0 ) { load_existing = 1; }
        }
        if( load_existing ) {
            cwasm_fs_idb_meta_t meta;
            uint8_t* payload;
            uint32_t payload_sz;

            int err = cwasm_fs_idb_load_slot( &payload, &payload_sz, &meta );
            if( err ) {
                free( h->path );
                return err;
            }
            h->idb_buf = payload;
            h->idb_size = payload_sz;
            h->idb_cap = payload_sz ? payload_sz : 1u;
            h->append_mode = ( flags & CWASM_FS_OPEN_APPEND ) != 0;
            h->pos = h->append_mode ? payload_sz : 0u;
            h->file_mode = S_IFREG | meta.mode;
            cwasm_fs_handle_meta_set( h, &meta );
        } else {
            time_t now = time( NULL );
            h->idb_buf = (uint8_t*)malloc( 1u );
            if( !h->idb_buf ) {
                free( h->path );
                return ENOMEM;
            }
            h->idb_cap = 1u;
            h->idb_size = 0u;
            h->append_mode = ( flags & CWASM_FS_OPEN_APPEND ) != 0;
            h->pos = 0u;
            h->file_mode = S_IFREG | ( mode & 0777u );
            h->meta_atime_sec = now;
            h->meta_atime_nsec = 0;
            h->meta_mtime_sec = now;
            h->meta_mtime_nsec = 0;
        }
        h->idb_dirty = 1;
        return 0;
    }

    free( h->path );
    return EINVAL;
}

int cwasm_fs_open( char const* canonical, unsigned flags, mode_t mode, cwasm_fs_handle_t* hout ) {
    int err;

    cwasm_fs_init();
    if( !canonical || !hout ) {
        errno = EINVAL;
        return -1;
    }
    if( cwasm_fs_is_appdata( canonical ) ) {
        err = cwasm_fs_idb_open( canonical, flags, mode, hout );
    } else if( wfs_has_embed() ) { err = cwasm_fs_wfs_open( canonical, flags, hout ); }
    else { err = cwasm_fs_http_open( canonical, flags, hout ); }

    if( err ) {
        errno = err;
        return -1;
    }
    return 0;
}

int cwasm_fs_read( cwasm_fs_handle_t* h, void* buf, uint32_t n ) {
    uint32_t got;
    int r;

    if( !h || !buf ) {
        errno = EINVAL;
        return -1;
    }
    if( h->kind == CWASM_FS_HKIND_WFS ) {
        uint32_t avail = h->pos < h->wfs_size ? h->wfs_size - h->pos : 0u;
        got = n < avail ? n : avail;
        if( got ) { got = wfs_embed_read( h->wfs_off + h->pos, buf, got ); }
        h->pos += got;
        return (int)got;
    }
    if( h->kind == CWASM_FS_HKIND_HTTP ) {
        r = js_fs_read( h->js_file_id, h->pos, (uint32_t)(uintptr_t)buf, n );
        if( r < 0 ) {
            errno = EIO;
            return -1;
        }
        h->pos += (uint32_t)r;
        return r;
    }
    if( h->kind == CWASM_FS_HKIND_IDB ) {
        uint32_t avail = h->pos < h->idb_size ? h->idb_size - h->pos : 0u;
        got = n < avail ? n : avail;
        if( got ) { memcpy( buf, h->idb_buf + h->pos, got ); }
        h->pos += got;
        return (int)got;
    }
    errno = EBADF;
    return -1;
}

static int cwasm_fs_idb_grow( cwasm_fs_handle_t* h, uint32_t need ) {
    if( need <= h->idb_cap ) { return 0; }
    {
        uint32_t nc = h->idb_cap ? h->idb_cap * 2u : 256u;
        uint8_t* nb;
        while( nc < need ) { nc *= 2u; }
        nb = (uint8_t*)realloc( h->idb_buf, nc );
        if( !nb ) { return -1; }
        h->idb_buf = nb;
        h->idb_cap = nc;
    }
    return 0;
}

int cwasm_fs_write( cwasm_fs_handle_t* h, void const* buf, uint32_t n ) {
    if( !h || !buf ) {
        errno = EINVAL;
        return -1;
    }
    if( h->kind == CWASM_FS_HKIND_WFS ) {
        errno = EACCES;
        return -1;
    }
    if( h->kind == CWASM_FS_HKIND_HTTP ) {
        errno = EACCES;
        return -1;
    }
    if( h->kind == CWASM_FS_HKIND_IDB ) {
        // POSIX O_APPEND: the write is placed at end-of-file whatever the current offset,
        // so an intervening lseek cannot redirect it
        if( h->append_mode ) { h->pos = h->idb_size; }
        if( h->pos + n < h->pos ) {
            errno = EINVAL;
            return -1;
        }
        if( cwasm_fs_idb_grow( h, h->pos + n ) != 0 ) {
            errno = ENOMEM;
            return -1;
        }
        memcpy( h->idb_buf + h->pos, buf, n );
        h->pos += n;
        if( h->pos > h->idb_size ) { h->idb_size = h->pos; }
        h->idb_dirty = 1;
        cwasm_fs_handle_touch_mtime( h );
        return (int)n;
    }
    errno = EBADF;
    return -1;
}

long cwasm_fs_seek( cwasm_fs_handle_t* h, long offset, int whence ) {
    int64_t base;
    int64_t np;

    if( !h ) {
        errno = EINVAL;
        return -1;
    }
    if( h->kind == CWASM_FS_HKIND_WFS ) { base = (int64_t)h->wfs_size; }
    else if( h->kind == CWASM_FS_HKIND_HTTP ) {
        base = (int64_t)js_fs_size( h->js_file_id );
    } else if( h->kind == CWASM_FS_HKIND_IDB ) { base = (int64_t)h->idb_size; }
    else {
        errno = EBADF;
        return -1;
    }

    if( whence == SEEK_SET ) { np = (int64_t)offset; }
    else if( whence == SEEK_CUR ) { np = (int64_t)h->pos + (int64_t)offset; }
    else if( whence == SEEK_END ) { np = base + (int64_t)offset; }
    else {
        errno = EINVAL;
        return -1;
    }
    if( np < 0 || np > (int64_t)UINT32_MAX ) {
        errno = EINVAL;
        return -1;
    }
    h->pos = (uint32_t)np;
    return (long)np;
}

long cwasm_fs_tell( cwasm_fs_handle_t* h ) {
    if( !h ) {
        errno = EINVAL;
        return -1;
    }
    return (long)h->pos;
}

void cwasm_fs_close( cwasm_fs_handle_t* h ) {
    char key[ CWASM_FS_PATH_MAX ];

    if( !h ) { return; }
    if( h->kind == CWASM_FS_HKIND_HTTP && h->js_file_id > 0 ) {
        js_fs_close( h->js_file_id );
        h->js_file_id = 0;
    }
    if( h->kind == CWASM_FS_HKIND_IDB ) {
        if( h->unlink_on_close ) {
            if( h->path && cwasm_fs_appdata_key( h->path, key, sizeof( key ) ) == 0 ) {
                cwasm_fs_idb_op( IDB_DEL, key, NULL, 0 );
            }
        } else { cwasm_fs_flush( h ); }
        free( h->idb_buf );
        free( h->path );
        h->idb_buf = NULL;
        h->path = NULL;
    } else {
        free( h->path );
        h->path = NULL;
    }
    memset( h, 0, sizeof( *h ) );
}

int cwasm_fs_flush( cwasm_fs_handle_t* h ) {
    char key[ CWASM_FS_PATH_MAX ];
    cwasm_fs_idb_meta_t meta;

    if( !h || h->kind != CWASM_FS_HKIND_IDB || !h->idb_dirty || !h->path ) { return 0; }
    if( cwasm_fs_appdata_key( h->path, key, sizeof( key ) ) != 0 ) { return -1; }
    cwasm_fs_handle_meta_get( h, &meta );
    if( cwasm_fs_idb_put_payload( key, h->idb_buf, h->idb_size, &meta ) != 0 ) {
        errno = EIO;
        return -1;
    }
    h->idb_dirty = 0;
    return 0;
}

int cwasm_fs_ftruncate( cwasm_fs_handle_t* h, off_t length ) {
    uint8_t* nb;
    size_t new_cap;

    if( !h ) {
        errno = EBADF;
        return -1;
    }
    if( length < 0 ) {
        errno = EINVAL;
        return -1;
    }
    if( h->kind != CWASM_FS_HKIND_IDB ) {
        errno = EACCES;
        return -1;
    }
    new_cap = (size_t)length;
    if( new_cap == 0 ) { new_cap = 1; }
    if( new_cap > h->idb_cap ) {
        nb = (uint8_t*)realloc( h->idb_buf, new_cap );
        if( !nb ) {
            errno = ENOMEM;
            return -1;
        }
        h->idb_buf = nb;
        h->idb_cap = (uint32_t)new_cap;
    }
    if( (uint32_t)length > h->idb_size ) {
        memset( h->idb_buf + h->idb_size, 0, (size_t)length - h->idb_size );
    }
    h->idb_size = (uint32_t)length;
    h->idb_dirty = 1;
    if( h->pos > h->idb_size ) { h->pos = h->idb_size; }
    cwasm_fs_handle_touch_mtime( h );
    return 0;
}

static unsigned long cwasm_fs_path_hash( char const* path ) {
    unsigned long h = 5381;
    unsigned char const* p;

    if( !path ) { return 1; }
    for( p = (unsigned char const*)path; *p; ++p ) { h = ( ( h << 5 ) + h ) + *p; }
    return h ? h : 1;
}

static void cwasm_fs_fill_stat( struct stat* st, int isdir, long size, mode_t perm, char const* canonical,
    int64_t atime_sec, long atime_nsec, int64_t mtime_sec, long mtime_nsec ) {
    memset( st, 0, sizeof( *st ) );
    st->st_mode = ( isdir ? S_IFDIR : S_IFREG ) | ( perm & 0777u );
    st->st_size = isdir ? 0 : size;
    st->st_dev = 1;
    st->st_ino = cwasm_fs_path_hash( canonical );
    st->st_nlink = 1;
    st->st_atim.tv_sec = atime_sec;
    st->st_atim.tv_nsec = atime_nsec;
    st->st_mtim.tv_sec = mtime_sec;
    st->st_mtim.tv_nsec = mtime_nsec;
}

int cwasm_fs_stat( char const* canonical, struct stat* st ) {
    char key[ CWASM_FS_PATH_MAX ];
    char marker[ CWASM_FS_PATH_MAX ];

    cwasm_fs_init();
    if( !canonical || !st ) {
        errno = EINVAL;
        return -1;
    }

    if( cwasm_fs_dev_kind( canonical ) ) { return cwasm_fs_dev_stat( canonical, st ); }

    if( cwasm_fs_is_virtual_dir( canonical ) ) {
        time_t now = time( NULL );
        cwasm_fs_fill_stat( st, 1, 0, CWASM_FS_DEFAULT_DIR_MODE, canonical, now, 0, now, 0 );
        return 0;
    }

    if( cwasm_fs_is_appdata( canonical ) ) {
        if( cwasm_fs_appdata_key( canonical, key, sizeof( key ) ) != 0 ) { return -1; }
        if( cwasm_fs_path_is_dir( canonical, 1 ) ) {
            mode_t dir_mode = CWASM_FS_DEFAULT_DIR_MODE;
            int64_t atime_sec = 0;
            int64_t mtime_sec = 0;
            long atime_nsec = 0;
            long mtime_nsec = 0;

            snprintf( marker, sizeof( marker ), "%s/", key );
            if( cwasm_fs_idb_op( IDB_GET, marker, NULL, 0 ) == 0 ) {
                cwasm_fs_idb_meta_t meta;
                uint8_t* payload;
                uint32_t payload_sz;
                int err = cwasm_fs_idb_load_slot( &payload, &payload_sz, &meta );

                if( err ) {
                    errno = err;
                    return -1;
                }
                free( payload );
                dir_mode = meta.mode;
                atime_sec = meta.atime_sec;
                atime_nsec = meta.atime_nsec;
                mtime_sec = meta.mtime_sec;
                mtime_nsec = meta.mtime_nsec;
            }
            cwasm_fs_fill_stat( st, 1, 0, dir_mode, canonical,
                atime_sec, atime_nsec, mtime_sec, mtime_nsec );
            return 0;
        }
        if( cwasm_fs_idb_op( IDB_GET, key, NULL, 0 ) != 0 ) {
            errno = ENOENT;
            return -1;
        }
        uint8_t* payload;
        uint32_t payload_sz;
        cwasm_fs_idb_meta_t meta;
        int err = cwasm_fs_idb_load_slot( &payload, &payload_sz, &meta );

        if( err ) {
            errno = err;
            return -1;
        }
        free( payload );
        cwasm_fs_fill_stat( st, 0, (long)payload_sz, meta.mode, canonical,
            meta.atime_sec, meta.atime_nsec, meta.mtime_sec, meta.mtime_nsec );
        return 0;
    }

    if( wfs_has_embed() ) {
        wfs_file_meta_t meta;

        if( wfs_lookup( canonical, NULL, NULL, &meta ) ) {
            cwasm_fs_fill_stat( st, 0, (long)meta.size, meta.mode, canonical,
                meta.atime_sec, meta.atime_nsec, meta.mtime_sec, meta.mtime_nsec );
            return 0;
        }
        if( cwasm_fs_path_is_dir( canonical, 0 ) ) {
            time_t now = time( NULL );
            cwasm_fs_fill_stat( st, 1, 0, CWASM_FS_DEFAULT_DIR_MODE, canonical, now, 0, now, 0 );
            return 0;
        }
        errno = ENOENT;
        return -1;
    }

    if( cwasm_fs_path_is_dir( canonical, 0 ) ) {
        time_t now = time( NULL );
        cwasm_fs_fill_stat( st, 1, 0, CWASM_FS_DEFAULT_DIR_MODE, canonical, now, 0, now, 0 );
        return 0;
    }
    char url[ CWASM_FS_PATH_MAX * 2 ];
    uint32_t size;
    int32_t mtime;

    if( cwasm_fs_http_url( canonical, url, sizeof( url ) ) != 0 ) { return -1; }
    if( cwasm_fs_http_head_load( url, &size, &mtime ) != 0 ) { return -1; }
    cwasm_fs_fill_stat( st, 0, (long)size, CWASM_FS_SHIP_FILE_MODE, canonical,
        0, 0, mtime, 0 );
    return 0;
}

int cwasm_fs_fstat( cwasm_fs_handle_t* h, struct stat* st ) {
    if( !h || !st ) {
        errno = EINVAL;
        return -1;
    }
    if( h->kind == CWASM_FS_HKIND_WFS || h->kind == CWASM_FS_HKIND_HTTP ||
        h->kind == CWASM_FS_HKIND_IDB ) {
        char const* canonical = h->path ? h->path : "/";
        int isdir = S_ISDIR( (int)h->file_mode );
        long size = h->kind == CWASM_FS_HKIND_HTTP
            ? (long)js_fs_size( h->js_file_id )
            : (long)( h->kind == CWASM_FS_HKIND_WFS ? h->wfs_size : h->idb_size );
        cwasm_fs_fill_stat( st, isdir, size, (mode_t)h->file_mode, canonical,
            h->meta_atime_sec, h->meta_atime_nsec,
            h->meta_mtime_sec, h->meta_mtime_nsec );
        return 0;
    }
    errno = EBADF;
    return -1;
}

static void cwasm_fs_dir_free( cwasm_fs_dir_t* dir ) {
    int i;
    if( !dir ) { return; }
    for( i = 0; i < dir->n_entries; ++i ) { free( dir->entries[ i ] ); }
    free( dir->entries );
    free( dir );
}

cwasm_fs_dir_t* cwasm_fs_opendir( char const* canonical ) {
    cwasm_fs_dir_t* dir;
    readdir_ctx_t ctx;
    int i;
    int cap = 0;
    char key[ CWASM_FS_PATH_MAX ];
    char* text = NULL;
    uint32_t sz;

    cwasm_fs_init();
    if( !canonical ) {
        errno = EINVAL;
        return NULL;
    }
    if( !cwasm_fs_path_is_dir( canonical, cwasm_fs_is_appdata( canonical ) ) ) {
        errno = ENOENT;
        return NULL;
    }

    dir = (cwasm_fs_dir_t*)malloc( sizeof( *dir ) );
    if( !dir ) {
        errno = ENOMEM;
        return NULL;
    }
    memset( dir, 0, sizeof( *dir ) );
    strncpy( dir->path, canonical, sizeof( dir->path ) - 1u );
    dir->is_appdata = cwasm_fs_is_appdata( canonical );

    if( dir->is_appdata ) {
        char list_prefix[ CWASM_FS_PATH_MAX ];
        size_t klen;

        if( cwasm_fs_appdata_key( canonical, key, sizeof( key ) ) != 0 ) {
            cwasm_fs_dir_free( dir );
            return NULL;
        }
        klen = strlen( key );
        if( klen + 2u >= sizeof( list_prefix ) ) {
            cwasm_fs_dir_free( dir );
            errno = ENAMETOOLONG;
            return NULL;
        }
        memcpy( list_prefix, key, klen );
        list_prefix[ klen ] = '/';
        list_prefix[ klen + 1 ] = '\0';
        if( cwasm_fs_idb_op( IDB_LIST, list_prefix, NULL, 0 ) != 0 ) {
            cwasm_fs_dir_free( dir );
            errno = ENOENT;
            return NULL;
        }
        sz = js_fs_idb_size();
        text = (char*)malloc( sz + 1u );
        if( !text ) {
            cwasm_fs_dir_free( dir );
            errno = ENOMEM;
            return NULL;
        }
        if( sz ) { js_fs_idb_read( 0, (uint32_t)(uintptr_t)text, sz ); }
        text[ sz ] = '\0';
        char* p = text;
        char line[ CWASM_FS_PATH_MAX ];
        char vpath[ CWASM_FS_PATH_MAX ];

        while( *p ) {
            char* e = p + strcspn( p, "\n" );
            size_t linelen = (size_t)( e - p );

            if( linelen > 0 && linelen < sizeof( line ) ) {
                memcpy( line, p, linelen );
                line[ linelen ] = '\0';
                if( cwasm_fs_appdata_vpath_from_key( line, vpath, sizeof( vpath ) ) == 0 ) {
                    cwasm_fs_readdir_from_path( canonical, vpath, &dir->entries, &dir->n_entries, &cap );
                }
            }
            p = ( *e == '\n' ) ? e + 1 : e;
        }
        free( text );
    } else if( wfs_has_embed() ) {
        ctx.dir = canonical;
        ctx.list = NULL;
        ctx.n = &dir->n_entries;
        ctx.cap = &cap;
        wfs_readdir_collect( wfs_collect_cb, &ctx );
        dir->entries = ctx.list;
    } else if( cwasm_fs_manifest_load() == 0 ) {
        cwasm_lock( &g_fs_lock );
        for( i = 0; i < g_manifest_n; ++i ) {
            cwasm_fs_readdir_from_path( canonical, g_manifest[ i ], &dir->entries, &dir->n_entries, &cap );
        }
        cwasm_unlock( &g_fs_lock );
    }

    if( dir->n_entries > 1 ) {
        qsort( dir->entries, (size_t)dir->n_entries, sizeof( char* ), cwasm_fs_cmp_str );
    }
    dir->pos = 0;
    return dir;
}

void cwasm_fs_dir_set_fd( cwasm_fs_dir_t* dir, int fd ) {
    if( dir ) { dir->dir_fd = fd; }
}

int cwasm_fs_dir_take_fd( cwasm_fs_dir_t* dir ) {
    int fd;

    if( !dir ) { return -1; }
    fd = dir->dir_fd;
    dir->dir_fd = -1;
    return fd;
}

static int cwasm_fs_readdir_child_is_dir( cwasm_fs_dir_t const* dir, char const* name ) {
    char child[ CWASM_FS_PATH_MAX ];
    size_t plen;

    if( !dir || !name ) { return 0; }
    plen = strlen( dir->path );
    if( plen == 1 && dir->path[ 0 ] == '/' ) {
        if( snprintf( child, sizeof( child ), "/%s", name ) >= (int)sizeof( child ) ) {
            return 0;
        }
    } else {
        if( snprintf( child, sizeof( child ), "%s/%s", dir->path, name ) >= (int)sizeof( child ) ) {
            return 0;
        }
    }
    return cwasm_fs_path_is_dir( child, dir->is_appdata );
}

struct dirent* cwasm_fs_readdir( cwasm_fs_dir_t* dir ) {
    if( !dir ) {
        errno = EBADF;
        return NULL;
    }
    if( dir->pos == 0 ) {
        dir->ent.d_type = DT_DIR;
        strcpy( dir->ent.d_name, "." );
        dir->pos = 1;
        return &dir->ent;
    }
    if( dir->pos == 1 ) {
        dir->ent.d_type = DT_DIR;
        strcpy( dir->ent.d_name, ".." );
        dir->pos = 2;
        return &dir->ent;
    }
    if( dir->pos - 2 < dir->n_entries ) {
        char const* name = dir->entries[ dir->pos - 2 ];
        dir->ent.d_type = cwasm_fs_readdir_child_is_dir( dir, name ) ? DT_DIR : DT_REG;
        strncpy( dir->ent.d_name, name, sizeof( dir->ent.d_name ) - 1u );
        dir->ent.d_name[ sizeof( dir->ent.d_name ) - 1u ] = '\0';
        ++dir->pos;
        return &dir->ent;
    }
    return NULL;
}

int cwasm_fs_closedir( cwasm_fs_dir_t* dir ) {
    if( !dir ) {
        errno = EBADF;
        return -1;
    }
    cwasm_fs_dir_free( dir );
    return 0;
}

int cwasm_fs_mkdir( char const* canonical, mode_t mode ) {
    char key[ CWASM_FS_PATH_MAX ];
    char marker[ CWASM_FS_PATH_MAX ];
    cwasm_fs_idb_meta_t meta;
    time_t now;

    cwasm_fs_init();
    if( !strcmp( canonical, "/?" ) ) {
        errno = EEXIST;
        return -1;
    }
    if( !cwasm_fs_is_appdata( canonical ) ) {
        errno = EACCES;
        return -1;
    }
    if( cwasm_fs_appdata_key( canonical, key, sizeof( key ) ) != 0 ) { return -1; }
    if( key[ 1 ] == '\0' ) {
        errno = EEXIST;
        return -1;
    }
    if( !cwasm_fs_idb_parent_exists( key ) ) {
        errno = ENOENT;
        return -1;
    }
    snprintf( marker, sizeof( marker ), "%s/", key );
    if( cwasm_fs_idb_op( IDB_GET, marker, NULL, 0 ) == 0 ) {
        errno = EEXIST;
        return -1;
    }
    now = time( NULL );
    cwasm_fs_idb_meta_default( &meta, mode & 0777u );
    meta.atime_sec = now;
    meta.mtime_sec = now;
    if( cwasm_fs_idb_put_payload( marker, NULL, 0, &meta ) != 0 ) {
        errno = EIO;
        return -1;
    }
    return 0;
}

static int cwasm_fs_idb_dir_has_children( char const* key, char const* marker ) {
    uint32_t sz;
    char* text;
    char const* p;
    size_t markerlen;

    if( cwasm_fs_idb_op( IDB_LIST, key, NULL, 0 ) != 0 ) { return -1; }
    sz = js_fs_idb_size();
    if( !sz ) { return 0; }
    text = (char*)malloc( sz + 1u );
    if( !text ) { return -1; }
    if( js_fs_idb_read( 0, (uint32_t)(uintptr_t)text, sz ) != (int)sz ) {
        free( text );
        return -1;
    }
    text[ sz ] = '\0';
    markerlen = strlen( marker );
    p = text;
    while( *p ) {
        char const* e = p + strcspn( p, "\n" );
        size_t len = (size_t)( e - p );

        if( len > 0 && !( len == markerlen && memcmp( p, marker, markerlen ) == 0 ) ) {
            free( text );
            return 1;
        }
        p = ( *e == '\n' ) ? e + 1 : e;
    }
    free( text );
    return 0;
}

int cwasm_fs_rmdir( char const* canonical ) {
    char key[ CWASM_FS_PATH_MAX ];
    char marker[ CWASM_FS_PATH_MAX ];

    cwasm_fs_init();
    if( !cwasm_fs_is_appdata( canonical ) ) {
        struct stat st;
        if( cwasm_fs_stat( canonical, &st ) != 0 ) {
            errno = ENOENT;
            return -1;
        }
        errno = EACCES;
        return -1;
    }
    if( cwasm_fs_appdata_key( canonical, key, sizeof( key ) ) != 0 ) { return -1; }
    snprintf( marker, sizeof( marker ), "%s/", key );
    if( cwasm_fs_idb_op( IDB_GET, marker, NULL, 0 ) != 0 ) {
        errno = ENOENT;
        return -1;
    }
    int children = cwasm_fs_idb_dir_has_children( key, marker );
    if( children < 0 ) { return -1; }
    if( children > 0 ) {
        errno = ENOTEMPTY;
        return -1;
    }
    if( cwasm_fs_idb_op( IDB_DEL, marker, NULL, 0 ) != 0 ) { return -1; }
    return 0;
}

int cwasm_fs_remove( char const* canonical ) {
    char key[ CWASM_FS_PATH_MAX ];
    char marker[ CWASM_FS_PATH_MAX ];

    cwasm_fs_init();
    if( !cwasm_fs_is_appdata( canonical ) ) {
        struct stat st;
        errno = 0;
        if( cwasm_fs_stat( canonical, &st ) != 0 ) {
            errno = ENOENT;
            return -1;
        }
        errno = EACCES;
        return -1;
    }
    if( cwasm_fs_appdata_key( canonical, key, sizeof( key ) ) != 0 ) { return -1; }
    snprintf( marker, sizeof( marker ), "%s/", key );
    if( cwasm_fs_idb_op( IDB_GET, marker, NULL, 0 ) == 0 ) {
        return cwasm_fs_rmdir( canonical );
    }
    if( cwasm_fs_idb_op( IDB_GET, key, NULL, 0 ) != 0 ) {
        errno = ENOENT;
        return -1;
    }
    if( cwasm_fs_idb_op( IDB_DEL, key, NULL, 0 ) != 0 ) { return -1; }
    return 0;
}

typedef struct {
    char* old_key;
    char* new_key;
    uint8_t* data;
    uint32_t len;
} cwasm_fs_idb_move_t;

static int cwasm_fs_idb_key_matches( char const* key, char const* prefix, size_t plen ) {
    return strncmp( key, prefix, plen ) == 0 &&
        ( key[ plen ] == '\0' || key[ plen ] == '/' );
}

static void cwasm_fs_idb_moves_free( cwasm_fs_idb_move_t* moves, int nmoves ) {
    int i;

    for( i = 0; i < nmoves; ++i ) {
        free( moves[ i ].old_key );
        free( moves[ i ].new_key );
        free( moves[ i ].data );
    }
    free( moves );
}

static int cwasm_fs_idb_moves_add( cwasm_fs_idb_move_t** moves, int* nmoves, int* cap,
    char const* old_k, char const* new_k ) {
    uint32_t ksz;
    cwasm_fs_idb_move_t* nv;

    if( *nmoves == *cap ) {
        int nc = *cap ? *cap * 2 : 8;
        nv = (cwasm_fs_idb_move_t*)realloc( *moves, (size_t)nc * sizeof( **moves ) );
        if( !nv ) { return -1; }
        *moves = nv;
        *cap = nc;
    }
    ( *moves )[ *nmoves ].old_key = strdup( old_k );
    ( *moves )[ *nmoves ].new_key = strdup( new_k );
    ( *moves )[ *nmoves ].data = NULL;
    ( *moves )[ *nmoves ].len = 0;
    if( !( *moves )[ *nmoves ].old_key || !( *moves )[ *nmoves ].new_key ) { goto fail; }
    if( cwasm_fs_idb_op( IDB_GET, old_k, NULL, 0 ) != 0 ) { goto fail; }
    ksz = js_fs_idb_size();
    ( *moves )[ *nmoves ].len = ksz;
    if( ksz ) {
        ( *moves )[ *nmoves ].data = (uint8_t*)malloc( ksz );
        if( !( *moves )[ *nmoves ].data ) { goto fail; }
        js_fs_idb_read( 0, (uint32_t)(uintptr_t)( *moves )[ *nmoves ].data, ksz );
    }
    ++( *nmoves );
    return 0;

fail:
    free( ( *moves )[ *nmoves ].old_key );
    free( ( *moves )[ *nmoves ].new_key );
    free( ( *moves )[ *nmoves ].data );
    ( *moves )[ *nmoves ].old_key = NULL;
    ( *moves )[ *nmoves ].new_key = NULL;
    ( *moves )[ *nmoves ].data = NULL;
    return -1;
}

static int cwasm_fs_idb_rename( char const* oldkey, char const* newkey ) {
    cwasm_fs_idb_move_t* moves = NULL;
    int nmoves = 0;
    int cap = 0;
    char listkey[ CWASM_FS_PATH_MAX ];
    uint32_t sz;
    char* text = NULL;
    char const* p;
    size_t oldlen = strlen( oldkey );
    size_t newlen = strlen( newkey );
    int i;
    int written = 0;
    int rc = 0;
    int replaced = 0;
    uint8_t* replaced_data = NULL;
    uint32_t replaced_len = 0;

    if( !oldkey[ 0 ] || !newkey[ 0 ] ) {
        errno = EINVAL;
        return -1;
    }

    if( strcmp( oldkey, newkey ) == 0 ) { return 0; }

    if( newlen > oldlen && strncmp( newkey, oldkey, oldlen ) == 0 &&
        ( newkey[ oldlen ] == '/' || oldkey[ oldlen - 1 ] == '/' ) ) {
        errno = EINVAL;
        return -1;
    }

    if( cwasm_fs_idb_op( IDB_GET, newkey, NULL, 0 ) == 0 ) {
        replaced_len = js_fs_idb_size();
        if( replaced_len ) {
            replaced_data = (uint8_t*)malloc( replaced_len );
            if( !replaced_data ) {
                errno = ENOMEM;
                return -1;
            }
            if( js_fs_idb_read( 0, (uint32_t)(uintptr_t)replaced_data, replaced_len ) !=
                (int)replaced_len ) {
                free( replaced_data );
                errno = EIO;
                return -1;
            }
        }
        if( cwasm_fs_idb_op( IDB_DEL, newkey, NULL, 0 ) != 0 ) {
            free( replaced_data );
            return -1;
        }
        replaced = 1;
    } else if( cwasm_fs_idb_op( IDB_LIST, newkey, NULL, 0 ) == 0 && js_fs_idb_size() > 0 ) {
        errno = EEXIST;
        return -1;
    }

    if( snprintf( listkey, sizeof( listkey ), "%s", oldkey ) >= (int)sizeof( listkey ) ) {
        errno = ENAMETOOLONG;
        rc = -1;
        goto out;
    }

    if( cwasm_fs_idb_op( IDB_LIST, listkey, NULL, 0 ) != 0 ) {
        if( cwasm_fs_idb_moves_add( &moves, &nmoves, &cap, oldkey, newkey ) != 0 ) {
            rc = -1;
            goto out;
        }
        goto commit;
    }

    sz = js_fs_idb_size();
    text = (char*)malloc( sz + 1u );
    if( !text ) {
        errno = ENOMEM;
        rc = -1;
        goto out;
    }
    if( sz && js_fs_idb_read( 0, (uint32_t)(uintptr_t)text, sz ) != (int)sz ) {
        errno = EIO;
        rc = -1;
        goto out;
    }
    text[ sz ] = '\0';

    p = text;
    while( *p ) {
        char const* e = p + strcspn( p, "\n" );
        char new_k[ CWASM_FS_PATH_MAX ];
        char old_k[ CWASM_FS_PATH_MAX ];
        char const* suffix;
        size_t old_klen;

        if( e > p && cwasm_fs_idb_key_matches( p, oldkey, oldlen ) ) {
            old_klen = (size_t)( e - p );
            if( old_klen >= sizeof( old_k ) ) {
                errno = ENAMETOOLONG;
                rc = -1;
                goto out;
            }
            memcpy( old_k, p, old_klen );
            old_k[ old_klen ] = '\0';
            suffix = old_k + oldlen;
            if( snprintf( new_k, sizeof( new_k ), "%s%s", newkey, suffix ) >= (int)sizeof( new_k ) ) {
                errno = ENAMETOOLONG;
                rc = -1;
                goto out;
            }
            if( cwasm_fs_idb_moves_add( &moves, &nmoves, &cap, old_k, new_k ) != 0 ) {
                rc = -1;
                goto out;
            }
        }
        p = ( *e == '\n' ) ? e + 1 : e;
    }

    if( nmoves == 0 ) {
        errno = ENOENT;
        rc = -1;
        goto out;
    }

commit:
    for( i = 0; i < nmoves; ++i ) {
        if( cwasm_fs_idb_op( IDB_PUT, moves[ i ].new_key, moves[ i ].data, moves[ i ].len ) != 0 ) {
            int j;
            for( j = 0; j < written; ++j ) {
                cwasm_fs_idb_op( IDB_DEL, moves[ j ].new_key, NULL, 0 );
            }
            rc = -1;
            goto out;
        }
        ++written;
    }
    // Destination is now the moved content (or intentionally absent); do not restore it.
    replaced = 0;
    for( i = 0; i < nmoves; ++i ) {
        if( cwasm_fs_idb_op( IDB_DEL, moves[ i ].old_key, NULL, 0 ) != 0 ) {
            rc = -1;
            goto out;
        }
    }

out:
    if( rc != 0 && replaced ) {
        cwasm_fs_idb_op( IDB_PUT, newkey, replaced_data, replaced_len );
    }
    free( replaced_data );
    free( text );
    cwasm_fs_idb_moves_free( moves, nmoves );
    return rc;
}

int cwasm_fs_rename( char const* oldpath, char const* newpath ) {
    char oldkey[ CWASM_FS_PATH_MAX ];
    char newkey[ CWASM_FS_PATH_MAX ];

    cwasm_fs_init();
    if( !cwasm_fs_is_appdata( oldpath ) || !cwasm_fs_is_appdata( newpath ) ) {
        errno = EACCES;
        return -1;
    }
    if( cwasm_fs_appdata_key( oldpath, oldkey, sizeof( oldkey ) ) != 0 ) { return -1; }
    if( cwasm_fs_appdata_key( newpath, newkey, sizeof( newkey ) ) != 0 ) { return -1; }
    return cwasm_fs_idb_rename( oldkey, newkey );
}

int cwasm_fs_access( char const* canonical, int mode ) {
    struct stat st;
    mode_t perm;

    cwasm_fs_init();
    if( !canonical ) {
        errno = EINVAL;
        return -1;
    }
    if( cwasm_fs_stat( canonical, &st ) != 0 ) { return -1; }

    perm = st.st_mode & 0777u;

    if( mode & R_OK ) {
        if( S_ISREG( st.st_mode ) || S_ISDIR( st.st_mode ) ) {
            if( !( perm & S_IRUSR ) ) {
                errno = EACCES;
                return -1;
            }
        }
    }
    if( mode & W_OK ) {
        if( !cwasm_fs_is_appdata( canonical ) ) {
            errno = EACCES;
            return -1;
        }
        if( S_ISREG( st.st_mode ) && !( perm & S_IWUSR ) ) {
            errno = EACCES;
            return -1;
        }
    }
    if( mode & X_OK ) {
        if( S_ISREG( st.st_mode ) ) {
            errno = ENOTSUP;
            return -1;
        }
        if( S_ISDIR( st.st_mode ) && !( perm & S_IXUSR ) ) {
            errno = EACCES;
            return -1;
        }
    }
    return 0;
}

static int cwasm_fs_idb_meta_update_key( char const* key,
    void ( *mutate )( cwasm_fs_idb_meta_t* meta, void const* arg ),
    void const* arg ) {
    uint8_t* payload;
    uint32_t payload_sz;
    cwasm_fs_idb_meta_t meta;
    int err;

    if( cwasm_fs_idb_op( IDB_GET, key, NULL, 0 ) != 0 ) {
        errno = ENOENT;
        return -1;
    }
    err = cwasm_fs_idb_load_slot( &payload, &payload_sz, &meta );
    if( err ) {
        errno = err;
        return -1;
    }
    mutate( &meta, arg );
    err = cwasm_fs_idb_put_payload( key, payload, payload_sz, &meta );
    free( payload );
    if( err ) {
        errno = EIO;
        return -1;
    }
    return 0;
}

typedef struct {
    mode_t mode;
} cwasm_fs_chmod_arg;

static void cwasm_fs_mutate_chmod( cwasm_fs_idb_meta_t* meta, void const* arg ) {
    cwasm_fs_chmod_arg const* a = ( cwasm_fs_chmod_arg const* )arg;

    meta->mode = a->mode & 0777u;
}

typedef struct {
    struct timeval const* times;
} cwasm_fs_utimes_arg;

static void cwasm_fs_mutate_utimes( cwasm_fs_idb_meta_t* meta, void const* arg ) {
    cwasm_fs_utimes_arg const* a = ( cwasm_fs_utimes_arg const* )arg;

    if( a->times ) {
        meta->atime_sec = a->times[ 0 ].tv_sec;
        meta->atime_nsec = (long)a->times[ 0 ].tv_usec * 1000L;
        meta->mtime_sec = a->times[ 1 ].tv_sec;
        meta->mtime_nsec = (long)a->times[ 1 ].tv_usec * 1000L;
    } else {
        time_t now = time( NULL );
        meta->atime_sec = now;
        meta->atime_nsec = 0;
        meta->mtime_sec = now;
        meta->mtime_nsec = 0;
    }
}

static int cwasm_fs_idb_update_path( char const* canonical, int dir,
    void ( *mutate )( cwasm_fs_idb_meta_t* meta, void const* arg ),
    void const* arg ) {
    char key[ CWASM_FS_PATH_MAX ];
    char marker[ CWASM_FS_PATH_MAX ];

    if( cwasm_fs_appdata_key( canonical, key, sizeof( key ) ) != 0 ) { return -1; }
    if( dir ) {
        snprintf( marker, sizeof( marker ), "%s/", key );
        return cwasm_fs_idb_meta_update_key( marker, mutate, arg );
    }
    return cwasm_fs_idb_meta_update_key( key, mutate, arg );
}

int cwasm_fs_chmod( char const* canonical, mode_t mode ) {
    cwasm_fs_chmod_arg arg;
    int is_dir;

    cwasm_fs_init();
    if( !canonical ) {
        errno = EINVAL;
        return -1;
    }
    if( !cwasm_fs_is_appdata( canonical ) ) {
        errno = ENOTSUP;
        return -1;
    }
    is_dir = cwasm_fs_path_is_dir( canonical, 1 );
    arg.mode = mode;
    return cwasm_fs_idb_update_path( canonical, is_dir, cwasm_fs_mutate_chmod, &arg );
}

int cwasm_fs_utimes( char const* canonical, struct timeval const times[ 2 ] ) {
    cwasm_fs_utimes_arg arg;
    int is_dir;

    cwasm_fs_init();
    if( !canonical ) {
        errno = EINVAL;
        return -1;
    }
    if( times &&
        ( times[ 0 ].tv_usec < 0 || times[ 0 ].tv_usec >= 1000000 ||
        times[ 1 ].tv_usec < 0 || times[ 1 ].tv_usec >= 1000000 ) ) {
        errno = EINVAL;
        return -1;
    }
    if( !cwasm_fs_is_appdata( canonical ) ) {
        errno = ENOTSUP;
        return -1;
    }
    is_dir = cwasm_fs_path_is_dir( canonical, 1 );
    arg.times = times;
    return cwasm_fs_idb_update_path( canonical, is_dir, cwasm_fs_mutate_utimes, &arg );
}

static void cwasm_fs_fill_statvfs_ship( struct statvfs* buf ) {
    memset( buf, 0, sizeof( *buf ) );
    buf->f_bsize = 4096;
    buf->f_frsize = 4096;
    buf->f_blocks = 0;
    buf->f_bfree = 0;
    buf->f_bavail = 0;
}

int cwasm_fs_statvfs( char const* canonical, struct statvfs* buf ) {
    int st;
    uint32_t quota;
    uint32_t usage;
    uint32_t free_blocks;

    cwasm_fs_init();
    if( !canonical || !buf ) {
        errno = EINVAL;
        return -1;
    }
    if( !cwasm_fs_is_appdata( canonical ) ) {
        cwasm_fs_fill_statvfs_ship( buf );
        return 0;
    }

    js_fs_storage_run();
    cwasm_env_async_wait_signal();
    st = js_fs_storage_ready();
    if( st != 1 ) {
        errno = EIO;
        return -1;
    }

    quota = js_fs_storage_quota();
    usage = js_fs_storage_usage();
    free_blocks = quota > usage ? quota - usage : 0;

    memset( buf, 0, sizeof( *buf ) );
    buf->f_bsize = 4096;
    buf->f_frsize = 4096;
    buf->f_blocks = quota;
    buf->f_bfree = free_blocks;
    buf->f_bavail = free_blocks;
    return 0;
}
