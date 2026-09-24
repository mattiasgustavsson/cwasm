#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <cwasm.h>

#define __CWASM_MAX_THREADS 1024
#define __CWASM_DEFAULT_STACK_SIZE (1024 * 1024)
#define __CWASM_MAX_TLS_KEYS 128

_Static_assert( PTHREAD_KEYS_MAX == __CWASM_MAX_TLS_KEYS, "PTHREAD_KEYS_MAX must match __CWASM_MAX_TLS_KEYS" );

enum {
    __CWASM_T_FREE = 0,
    __CWASM_T_STARTING = 1,
    __CWASM_T_RUNNING = 2,
    __CWASM_T_EXITED = 3,
    __CWASM_T_REAPING = 4,
};

typedef struct cwasm_tls_slot_t {
    void* value;
    uint32_t gen;
} cwasm_tls_slot_t;

typedef struct cwasm_tcb_t {
    _Atomic uint32_t state;
    uint32_t tid;
    void* ( *start )( void* );
    void* arg;
    void* result;
    uint32_t stack_low;
    uint32_t stack_high;
    uint32_t stack_size;
    uint32_t tls_base;
    uint32_t tls_size;
    _Atomic uint32_t detached;
    int errno_val;
    unsigned int fenv_except;
    cwasm_tls_slot_t tls_keys[ __CWASM_MAX_TLS_KEYS ];
    _Atomic uint32_t host_futex;
} cwasm_tcb_t;

typedef struct cwasm_mutex_impl_t {
    _Atomic uint32_t locked;
    _Atomic uint32_t owner;
    _Atomic uint32_t depth;
    uint32_t type;
} cwasm_mutex_impl_t;

typedef struct cwasm_cond_impl_t {
    _Atomic uint32_t seq;
} cwasm_cond_impl_t;


cwasm_tcb_t cwasm_threads[ __CWASM_MAX_THREADS ];
_Atomic uint32_t cwasm_tls_key_used[ __CWASM_MAX_TLS_KEYS ];
_Atomic uint32_t cwasm_tls_key_gen[ __CWASM_MAX_TLS_KEYS ];
void ( *cwasm_tls_dtors[ __CWASM_MAX_TLS_KEYS ] )( void* );

static _Atomic uint32_t g_thread_alloc_lock;

CWASM_JS_LIB( THREAD, uint32_t, js_get_tls_size, ( void ), {
  var e = CWASM.asm;
  if (!e || e.__tls_size == null) return 0;
  return (e.__tls_size.value | 0);
})

CWASM_JS_LIB( THREAD, uint32_t, js_get_tls_align, ( void ), {
  var e = CWASM.asm;
  if (!e || e.__tls_align == null) return 16;
  var a = e.__tls_align.value | 0;
  return a < 16 ? 16 : a;
})

// Keep a live TLS symbol so wasm-ld emits __tls_size /__wasm_init_tls.
static _Thread_local volatile int cwasm_tls_anchor;

void __wasm_init_tls( void* p ) __attribute__( ( weak ) );

unsigned int __cwasm_fenv_get_except( void );
void __cwasm_fenv_set_except( unsigned int excepts );

CWASM_JS_LIB( THREAD, uint32_t, js_get_tid, ( void ), {
  return (CWASM.threadTid | 0);
})

static void* alloc_tls_image( uint32_t* out_size, int* oom ) {
    (void)cwasm_tls_anchor;
    uint32_t sz = js_get_tls_size();
    uint32_t al = js_get_tls_align();
    *oom = 0;
    if( sz == 0 ) {
        *out_size = 0;
        return NULL;
    }
    size_t bytes = ( (size_t)sz + (size_t)al - 1u ) & ~( (size_t)al - 1u );
    void* p = aligned_alloc( al, bytes );
    if( !p ) {
        *oom = 1;
        return NULL;
    }
    memset( p, 0, bytes );
    *out_size = (uint32_t)bytes;
    return p;
}

uint32_t cwasm_current_tid( void ) {
    return js_get_tid();
}

cwasm_tcb_t* cwasm_current_tcb( void ) {
    uint32_t tid = cwasm_current_tid();
    if( tid >= __CWASM_MAX_THREADS ) { return &cwasm_threads[ 0 ]; }
    return &cwasm_threads[ tid ];
}

int* __errno_location( void ) {
    return &cwasm_current_tcb()->errno_val;
}

void cwasm_atomic_wait32( _Atomic uint32_t* addr, uint32_t expected, int64_t timeout_ns ) {
    __builtin_wasm_memory_atomic_wait32( (int*)addr, (int)expected, timeout_ns );
}

void cwasm_atomic_notify( _Atomic uint32_t* addr, uint32_t count ) {
    __builtin_wasm_memory_atomic_notify( (int*)addr, count );
}

static void futex_lock( _Atomic uint32_t* lock ) {
    for( ;; ) {
        uint32_t expected = 0;
        if( atomic_compare_exchange_weak( lock, &expected, 1 ) ) { return; }
        cwasm_atomic_wait32( lock, 1, -1 );
    }
}

static void futex_unlock( _Atomic uint32_t* lock ) {
    atomic_store( lock, 0 );
    cwasm_atomic_notify( lock, 1 );
}

CWASM_JS_LIB( THREAD, int, js_thread_spawn, ( uint32_t tid, uint32_t stack_high, uint32_t tls_base ), {
  var addr = CWASM.asm.__cwasm_host_futex_ptr(tid) | 0;
  if (!addr) return -1;
  var i32 = new Int32Array(globalThis.MEM.buffer);
  var idx = addr >> 2;
  Atomics.store(i32, idx, 0);
  self.postMessage({ type: 'thread.spawn', tid: tid, stackHigh: stack_high, tlsBase: tls_base, futexAddr: addr });
  while (Atomics.load(i32, idx) === 0) {
    if (i32.buffer !== globalThis.MEM.buffer) i32 = new Int32Array(globalThis.MEM.buffer);
    Atomics.wait(i32, idx, 0);
  }
  var v = Atomics.load(i32, idx) | 0;
  Atomics.store(i32, idx, 0);
  return v === 1 ? 0 : -1;
})

// Must not return - parking in C after free would use the freed stack
CWASM_JS_LIB( THREAD, void, js_thread_exit_park, ( uint32_t tid ), {
  var exp = CWASM.asm;
  if (exp && exp.__cwasm_thread_mark_exited)
    exp.__cwasm_thread_mark_exited(tid | 0);
  if (CWASM.reapDetachedThread)
    CWASM.reapDetachedThread(tid | 0);
  self.postMessage({ type: 'thread.exited', tid: tid });
  var park = new Int32Array(new SharedArrayBuffer(4));
  for (;;) Atomics.wait(park, 0, 0);
})

CWASM_JS_LIB( THREAD, void, js_thread_yield, ( void ), {
  var f = CWASM.__yieldWord || (CWASM.__yieldWord = new Int32Array(new SharedArrayBuffer(4)));
  Atomics.wait(f, 0, 0, 1);
})

CWASM_JS_LIB_INIT( THREAD,
(
    var runThread = function (exports, tid, stackTop) {
        tid = tid | 0;
        CWASM.threadTid = tid;
        CWASM.exitCode = 0;
        CWASM.__processHalt = 0;
        if (stackTop != null && exports.__stack_pointer)
            exports.__stack_pointer.value = stackTop | 0;
        if (exports.__cwasm_thread_init)
            exports.__cwasm_thread_init(tid);
        var fp = exports.__cwasm_host_futex_ptr ? exports.__cwasm_host_futex_ptr(tid) | 0 : 0;
        if (fp) {
            var i = new Int32Array(globalThis.MEM.buffer);
            Atomics.store(i, fp >> 2, 1);
            Atomics.notify(i, fp >> 2);
        }
        CWASM.runtime.core.run(exports.__pthread_trampoline,
            function () { finishThreadReturn(exports); },
            function (e) { self.postMessage({ type: "thread.error", tid: CWASM.threadTid | 0, error: String(e && e.stack || e) }); });
    };
    CWASM.runtime.thread.onInstantiated = function (exports, msg) {
        var tid = CWASM.threadTid | 0;
        if (tid !== 0) { runThread(exports, tid, msg ? msg.stackTop : null); return; }
        if (msg && msg.stackTop != null && exports.__stack_pointer)
            exports.__stack_pointer.value = msg.stackTop | 0;
        if (exports.__cwasm_thread_init)
            exports.__cwasm_thread_init(0);
        self.postMessage({ type: "thread.started" });
        CWASM.runtime.core.run(exports._start,
            function () { finishThreadReturn(exports); },
            function (e) { self.postMessage({ type: "thread.error", tid: 0, error: String(e && e.stack || e) }); });
    };
    CWASM.runtime.thread.onThreadMsg = function (m) {
        if (m.type === "memory.grew") {
            if (CWASM.__setViews) CWASM.__setViews();
            return true;
        }
        if (m.type === "thread.rerun") {
            if (!CWASM.asm) throw new Error("thread.rerun before thread.init");
            runThread(CWASM.asm, m.tid, m.stackTop);
            return true;
        }
        return false;
    };
),
void, js_thread_host_ready, ( void ), {})

int sched_yield( void ) {
    js_thread_yield();
    return 0;
}

__attribute__( ( export_name( "__cwasm_host_futex_ptr" ) ) ) uint32_t __cwasm_host_futex_ptr( uint32_t tid ) {
    if( tid >= __CWASM_MAX_THREADS ) { return 0; }
    return (uint32_t)(uintptr_t)&cwasm_threads[ tid ].host_futex;
}

static cwasm_mutex_impl_t* mutex_impl( pthread_mutex_t* m ) {
    return (cwasm_mutex_impl_t*)(void*)m->_data;
}

static cwasm_cond_impl_t* cond_impl( pthread_cond_t* c ) {
    return (cwasm_cond_impl_t*)(void*)c->_data;
}

int pthread_mutex_init( pthread_mutex_t* mutex, pthread_mutexattr_t const* attr ) {
    if( !mutex ) { return EINVAL; }
    memset( mutex, 0, sizeof( *mutex ) );
    cwasm_mutex_impl_t* m = mutex_impl( mutex );
    m->type = attr ? (uint32_t)attr->_type : PTHREAD_MUTEX_NORMAL;
    return 0;
}

int pthread_mutexattr_init( pthread_mutexattr_t* attr ) {
    if( !attr ) { return EINVAL; }
    attr->_type = PTHREAD_MUTEX_NORMAL;
    return 0;
}

int pthread_mutexattr_destroy( pthread_mutexattr_t* attr ) {
    (void)attr;
    return 0;
}

int pthread_mutexattr_settype( pthread_mutexattr_t* attr, int type ) {
    if( !attr ) { return EINVAL; }
    if( type != PTHREAD_MUTEX_NORMAL && type != PTHREAD_MUTEX_RECURSIVE && type != PTHREAD_MUTEX_ERRORCHECK ) {
        return EINVAL;
    }
    attr->_type = type;
    return 0;
}

int pthread_mutex_destroy( pthread_mutex_t* mutex ) {
    (void)mutex;
    return 0;
}

int pthread_mutex_lock( pthread_mutex_t* mutex ) {
    cwasm_mutex_impl_t* m = mutex_impl( mutex );
    uint32_t self = cwasm_current_tid() + 1u;
    if( m->type == PTHREAD_MUTEX_RECURSIVE &&
        atomic_load( &m->owner ) == self && atomic_load( &m->depth ) ) {
        atomic_fetch_add( &m->depth, 1 );
        return 0;
    }
    if( m->type == PTHREAD_MUTEX_ERRORCHECK &&
        atomic_load( &m->owner ) == self && atomic_load( &m->depth ) ) {
        return EDEADLK;
    }
    for( ;; ) {
        uint32_t expected = 0;
        if( atomic_compare_exchange_weak( &m->locked, &expected, 1 ) ) {
            atomic_store( &m->owner, self );
            atomic_store( &m->depth, 1 );
            return 0;
        }
        cwasm_atomic_wait32( &m->locked, 1, -1 );
    }
}

int pthread_mutex_trylock( pthread_mutex_t* mutex ) {
    cwasm_mutex_impl_t* m = mutex_impl( mutex );
    uint32_t self = cwasm_current_tid() + 1u;
    if( m->type == PTHREAD_MUTEX_RECURSIVE &&
        atomic_load( &m->owner ) == self && atomic_load( &m->depth ) ) {
        atomic_fetch_add( &m->depth, 1 );
        return 0;
    }
    if( m->type == PTHREAD_MUTEX_ERRORCHECK &&
        atomic_load( &m->owner ) == self && atomic_load( &m->depth ) ) {
        return EBUSY;
    }
    uint32_t expected = 0;
    if( atomic_compare_exchange_strong( &m->locked, &expected, 1 ) ) {
        atomic_store( &m->owner, self );
        atomic_store( &m->depth, 1 );
        return 0;
    }
    return EBUSY;
}

int pthread_mutex_timedlock( pthread_mutex_t* mutex, struct timespec const* abstime ) {
    cwasm_mutex_impl_t* m = mutex_impl( mutex );
    uint32_t self = cwasm_current_tid() + 1u;
    int64_t abs_ns;

    if( !mutex || !abstime ) { return EINVAL; }
    if( abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000L ) { return EINVAL; }

    if( m->type == PTHREAD_MUTEX_RECURSIVE &&
        atomic_load( &m->owner ) == self && atomic_load( &m->depth ) ) {
        atomic_fetch_add( &m->depth, 1 );
        return 0;
    }
    if( m->type == PTHREAD_MUTEX_ERRORCHECK &&
        atomic_load( &m->owner ) == self && atomic_load( &m->depth ) ) {
        return EDEADLK;
    }

    abs_ns = (int64_t)abstime->tv_sec * 1000000000LL + abstime->tv_nsec;
    for( ;; ) {
        uint32_t expected = 0;
        struct timespec now;
        int64_t left;

        if( atomic_compare_exchange_weak( &m->locked, &expected, 1 ) ) {
            atomic_store( &m->owner, self );
            atomic_store( &m->depth, 1 );
            return 0;
        }
        if( clock_gettime( CLOCK_REALTIME, &now ) != 0 ) { return EINVAL; }
        left = abs_ns - ( (int64_t)now.tv_sec * 1000000000LL + now.tv_nsec );
        if( left <= 0 ) { return ETIMEDOUT; }
        if( __builtin_wasm_memory_atomic_wait32( (int*)&m->locked, 1, left ) == 2 ) {
            return ETIMEDOUT;
        }
    }
}

int pthread_mutex_unlock( pthread_mutex_t* mutex ) {
    cwasm_mutex_impl_t* m = mutex_impl( mutex );
    uint32_t self = cwasm_current_tid() + 1u;
    if( atomic_load( &m->owner ) != self ) { return EPERM; }
    if( m->type == PTHREAD_MUTEX_RECURSIVE && atomic_load( &m->depth ) > 1 ) {
        atomic_fetch_sub( &m->depth, 1 );
        return 0;
    }
    atomic_store( &m->depth, 0 );
    atomic_store( &m->owner, 0 );
    atomic_store( &m->locked, 0 );
    cwasm_atomic_notify( &m->locked, 1 );
    return 0;
}

int pthread_cond_init( pthread_cond_t* cond, pthread_condattr_t const* attr ) {
    (void)attr;
    if( !cond ) { return EINVAL; }
    memset( cond, 0, sizeof( *cond ) );
    return 0;
}

int pthread_cond_destroy( pthread_cond_t* cond ) {
    (void)cond;
    return 0;
}

int pthread_cond_signal( pthread_cond_t* cond ) {
    cwasm_cond_impl_t* c = cond_impl( cond );
    atomic_fetch_add( &c->seq, 1 );
    cwasm_atomic_notify( &c->seq, 1 );
    return 0;
}

int pthread_cond_broadcast( pthread_cond_t* cond ) {
    cwasm_cond_impl_t* c = cond_impl( cond );
    atomic_fetch_add( &c->seq, 1 );
    cwasm_atomic_notify( &c->seq, UINT32_MAX );
    return 0;
}

int pthread_cond_wait( pthread_cond_t* cond, pthread_mutex_t* mutex ) {
    cwasm_cond_impl_t* c = cond_impl( cond );
    uint32_t seq = atomic_load( &c->seq );
    pthread_mutex_unlock( mutex );
    cwasm_atomic_wait32( &c->seq, seq, -1 );
    pthread_mutex_lock( mutex );
    return 0;
}

int pthread_cond_timedwait( pthread_cond_t* cond, pthread_mutex_t* mutex,
    struct timespec const* abstime ) {
    cwasm_cond_impl_t* c = cond_impl( cond );
    uint32_t seq = atomic_load( &c->seq );
    struct timespec now;
    int64_t timeout_ns = -1;
    if( abstime ) {
        if( abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000L ) { return EINVAL; }
        if( clock_gettime( CLOCK_REALTIME, &now ) != 0 ) { return EINVAL; }
        int64_t abs_ns = (int64_t)abstime->tv_sec * 1000000000LL + abstime->tv_nsec;
        int64_t now_ns = (int64_t)now.tv_sec * 1000000000LL + now.tv_nsec;
        timeout_ns = abs_ns - now_ns;
        if( timeout_ns < 0 ) { timeout_ns = 0; }
    }
    pthread_mutex_unlock( mutex );
    int wr = __builtin_wasm_memory_atomic_wait32( (int*)&c->seq, (int)seq, timeout_ns );
    pthread_mutex_lock( mutex );
    if( wr == 2 ) { return ETIMEDOUT; }
    return 0;
}

int pthread_once( pthread_once_t* once_control, void ( *init_routine )( void ) ) {
    static pthread_mutex_t once_mu = PTHREAD_MUTEX_INITIALIZER;
    if( !once_control || !init_routine ) { return EINVAL; }
    if( atomic_load( ( _Atomic int* )&once_control->_once ) ) { return 0; }
    pthread_mutex_lock( &once_mu );
    if( !once_control->_once ) {
        init_routine();
        atomic_store( ( _Atomic int* )&once_control->_once, 1 );
    }
    pthread_mutex_unlock( &once_mu );
    return 0;
}

int pthread_key_create( pthread_key_t* key, void ( *destructor )( void* ) ) {
    if( !key ) { return EINVAL; }
    for( uint32_t i = 0; i < __CWASM_MAX_TLS_KEYS; ++i ) {
        uint32_t expected = 0;
        if( atomic_compare_exchange_strong( &cwasm_tls_key_used[ i ], &expected, 2 ) ) {
            uint32_t gen = atomic_load( &cwasm_tls_key_gen[ i ] );
            if( ( gen & 1u ) == 0u ) {
                atomic_store( &cwasm_tls_key_gen[ i ], gen + 1u );
            }
            cwasm_tls_dtors[ i ] = destructor;
            atomic_store( &cwasm_tls_key_used[ i ], 1 );
            *key = i;
            return 0;
        }
    }
    return EAGAIN;
}

int pthread_key_delete( pthread_key_t key ) {
    if( key >= __CWASM_MAX_TLS_KEYS ) { return EINVAL; }
    if( atomic_load( &cwasm_tls_key_used[ key ] ) != 1u ) { return EINVAL; }
    cwasm_tls_dtors[ key ] = NULL;
    atomic_fetch_add( &cwasm_tls_key_gen[ key ], 1u );
    atomic_store( &cwasm_tls_key_used[ key ], 0 );
    return 0;
}

int pthread_setspecific( pthread_key_t key, void const* value ) {
    if( key >= __CWASM_MAX_TLS_KEYS || atomic_load( &cwasm_tls_key_used[ key ] ) != 1u ) {
        return EINVAL;
    }
    cwasm_tls_slot_t* slot = &cwasm_current_tcb()->tls_keys[ key ];
    slot->value = (void*)(uintptr_t)value;
    slot->gen = atomic_load( &cwasm_tls_key_gen[ key ] );
    return 0;
}

void* pthread_getspecific( pthread_key_t key ) {
    if( key >= __CWASM_MAX_TLS_KEYS || atomic_load( &cwasm_tls_key_used[ key ] ) != 1u ) {
        return NULL;
    }
    cwasm_tls_slot_t* slot = &cwasm_current_tcb()->tls_keys[ key ];
    if( slot->gen != atomic_load( &cwasm_tls_key_gen[ key ] ) ) {
        return NULL;
    }
    return slot->value;
}

// pthread_t is 1-based so tid 0 (main) is not the null std::thread::id / default pthread_t value of 0
static pthread_t tid_to_pthread( uint32_t tid ) {
    return (pthread_t)( tid + 1u );
}

static uint32_t pthread_to_tid( pthread_t p ) {
    if( p == 0 || (uint32_t)p > __CWASM_MAX_THREADS ) { return UINT32_MAX; }
    return (uint32_t)p - 1u;
}

pthread_t pthread_self( void ) {
    return tid_to_pthread( cwasm_current_tid() );
}

int pthread_equal( pthread_t a, pthread_t b ) {
    return a == b;
}

int pthread_attr_init( pthread_attr_t* attr ) {
    if( !attr ) { return EINVAL; }
    attr->_flags = 0;
    attr->_stacksize = __CWASM_DEFAULT_STACK_SIZE;
    return 0;
}

int pthread_attr_destroy( pthread_attr_t* attr ) {
    (void)attr;
    return 0;
}

int pthread_attr_setstacksize( pthread_attr_t* attr, size_t stacksize ) {
    if( !attr || stacksize < 4096 || ( stacksize & 15u ) != 0 ) { return EINVAL; }
    attr->_stacksize = stacksize;
    return 0;
}

int pthread_attr_getstacksize( pthread_attr_t const* attr, size_t* stacksize ) {
    if( !attr || !stacksize ) { return EINVAL; }
    *stacksize = attr->_stacksize;
    return 0;
}

int pthread_attr_setdetachstate( pthread_attr_t* attr, int state ) {
    if( !attr ) { return EINVAL; }
    if( state == PTHREAD_CREATE_DETACHED ) {
        attr->_flags |= 1;
    } else if( state == PTHREAD_CREATE_JOINABLE ) {
        attr->_flags &= ~1u;
    } else {
        return EINVAL;
    }
    return 0;
}

int pthread_setschedparam( pthread_t thread, int policy, struct sched_param const* param ) {
    (void)thread;
    (void)policy;
    (void)param;
    return ENOTSUP;
}

static void free_thread_resources( cwasm_tcb_t* t ) {
    if( t->stack_low ) {
        free( (void*)(uintptr_t)t->stack_low );
        t->stack_low = 0;
    }
    if( t->tls_base ) {
        free( (void*)(uintptr_t)t->tls_base );
        t->tls_base = 0;
        t->tls_size = 0;
    }
}

static void release_thread( cwasm_tcb_t* t ) {
    uint32_t expected = __CWASM_T_EXITED;
    if( !atomic_compare_exchange_strong( &t->state, &expected, __CWASM_T_REAPING ) ) {
        return;
    }
    free_thread_resources( t );
    atomic_store( &t->state, __CWASM_T_FREE );
    cwasm_atomic_notify( &t->state, UINT32_MAX );
}

// Publish EXITED after the pthread body has returned to JS (stack idle).
__attribute__( ( export_name( "__cwasm_thread_mark_exited" ) ) ) void __cwasm_thread_mark_exited( uint32_t tid ) {
    if( tid >= __CWASM_MAX_THREADS ) { return; }
    cwasm_tcb_t* t = &cwasm_threads[ tid ];
    atomic_store( &t->state, __CWASM_T_EXITED );
    cwasm_atomic_notify( &t->state, UINT32_MAX );
}

// Emergency stack for reaping detached threads from the dying worker's Instance
// (SP still points at the pthread stack, free() must not run there)
static uint8_t g_reap_stack[ 8192 ];
static _Atomic uint32_t g_reap_lock;

__attribute__( ( export_name( "__cwasm_reap_stack_top" ) ) ) uint32_t __cwasm_reap_stack_top( void ) {
    return (uint32_t)(uintptr_t)( g_reap_stack + sizeof( g_reap_stack ) );
}

__attribute__( ( export_name( "__cwasm_reap_lock_ptr" ) ) ) uint32_t __cwasm_reap_lock_ptr( void ) {
    return (uint32_t)(uintptr_t)&g_reap_lock;
}

__attribute__( ( export_name( "__cwasm_thread_release_detached" ) ) ) void __cwasm_thread_release_detached( uint32_t tid ) {
    if( tid == 0 || tid >= __CWASM_MAX_THREADS ) { return; }
    cwasm_tcb_t* t = &cwasm_threads[ tid ];
    if( !atomic_load( &t->detached ) ) { return; }
    uint32_t expected = __CWASM_T_EXITED;
    if( !atomic_compare_exchange_strong( &t->state, &expected, __CWASM_T_REAPING ) ) {
        return;
    }
    free_thread_resources( t );
    atomic_store( &t->state, __CWASM_T_FREE );
    cwasm_atomic_notify( &t->state, UINT32_MAX );
}

static int alloc_tid( void ) {
    futex_lock( &g_thread_alloc_lock );
    for( uint32_t i = 1; i < __CWASM_MAX_THREADS; ++i ) {
        uint32_t st = atomic_load( &cwasm_threads[ i ].state );
        if( st == __CWASM_T_FREE ) {
            atomic_store( &cwasm_threads[ i ].state, __CWASM_T_STARTING );
            futex_unlock( &g_thread_alloc_lock );
            return (int)i;
        }
    }
    futex_unlock( &g_thread_alloc_lock );
    return -1;
}

int pthread_create( pthread_t* thread, pthread_attr_t const* attr,
    void* ( *start_routine )( void* ), void* arg ) {
    if( !thread || !start_routine ) { return EINVAL; }
    size_t stack_size = __CWASM_DEFAULT_STACK_SIZE;
    int detached = 0;
    if( attr ) {
        stack_size = attr->_stacksize ? attr->_stacksize : __CWASM_DEFAULT_STACK_SIZE;
        detached = !!( attr->_flags & 1 );
    }
    int tid = alloc_tid();
    if( tid < 0 ) {
        return EAGAIN;
    }

    void* stack = aligned_alloc( 16, stack_size );
    if( !stack ) {
        atomic_store( &cwasm_threads[ tid ].state, __CWASM_T_FREE );
        return EAGAIN;
    }

    uint32_t tls_sz = 0;
    int tls_oom = 0;
    void* tls = alloc_tls_image( &tls_sz, &tls_oom );
    if( tls_oom ) {
        free( stack );
        atomic_store( &cwasm_threads[ tid ].state, __CWASM_T_FREE );
        return EAGAIN;
    }

    cwasm_tcb_t* t = &cwasm_threads[ tid ];
    memset( t->tls_keys, 0, sizeof( t->tls_keys ) );
    t->tid = (uint32_t)tid;
    t->start = start_routine;
    t->arg = arg;
    t->result = NULL;
    t->stack_low = (uint32_t)(uintptr_t)stack;
    t->stack_size = (uint32_t)stack_size;
    t->stack_high = t->stack_low + (uint32_t)stack_size;
    t->tls_base = tls ? (uint32_t)(uintptr_t)tls : 0;
    t->tls_size = tls_sz;
    atomic_store( &t->detached, detached ? 1u : 0u );
    t->errno_val = 0;
    t->fenv_except = __cwasm_fenv_get_except();
    atomic_store( &t->host_futex, 0 );

    int err = js_thread_spawn( (uint32_t)tid, t->stack_high, t->tls_base );
    if( err ) {
        free_thread_resources( t );
        atomic_store( &t->state, __CWASM_T_FREE );
        return EAGAIN;
    }

    *thread = tid_to_pthread( (uint32_t)tid );
    return 0;
}

static void run_tls_dtors( cwasm_tcb_t* self ) {
    for( int pass = 0; pass < PTHREAD_DESTRUCTOR_ITERATIONS; ++pass ) {
        int any = 0;
        for( uint32_t k = 0; k < __CWASM_MAX_TLS_KEYS; ++k ) {
            uint32_t gen = atomic_load( &cwasm_tls_key_gen[ k ] );
            if( atomic_load( &cwasm_tls_key_used[ k ] ) == 1u && cwasm_tls_dtors[ k ] &&
                self->tls_keys[ k ].value && self->tls_keys[ k ].gen == gen ) {
                void* v = self->tls_keys[ k ].value;
                self->tls_keys[ k ].value = NULL;
                cwasm_tls_dtors[ k ]( v );
                any = 1;
            }
        }
        if( !any ) { return; }
    }
}

int pthread_join( pthread_t thread, void** retval ) {
    uint32_t tid = pthread_to_tid( thread );
    if( tid == cwasm_current_tid() ) { return EDEADLK; }
    if( tid >= __CWASM_MAX_THREADS || tid == 0 ) { return ESRCH; }
    cwasm_tcb_t* t = &cwasm_threads[ tid ];
    for( ;; ) {
        uint32_t st = atomic_load( &t->state );
        if( st == __CWASM_T_EXITED ) { break; }
        if( st == __CWASM_T_FREE ) { return ESRCH; }
        cwasm_atomic_wait32( &t->state, st, -1 );
    }
    if( retval ) { *retval = t->result; }
    release_thread( t );
    return 0;
}

int pthread_detach( pthread_t thread ) {
    uint32_t tid = pthread_to_tid( thread );
    if( tid >= __CWASM_MAX_THREADS || tid == 0 ) { return ESRCH; }
    cwasm_tcb_t* t = &cwasm_threads[ tid ];
    atomic_store( &t->detached, 1 );
    if( atomic_load( &t->state ) == __CWASM_T_EXITED ) { release_thread( t ); }
    return 0;
}

__attribute__( ( export_name( "__pthread_trampoline" ) ) )
void __pthread_trampoline( void ) {
    cwasm_tcb_t* self = cwasm_current_tcb();
    atomic_store( &self->state, __CWASM_T_RUNNING );
    void* result = self->start( self->arg );
    self->result = result;
    run_tls_dtors( self );
}

void pthread_exit( void* retval ) {
    cwasm_tcb_t* self = cwasm_current_tcb();
    self->result = retval;
    run_tls_dtors( self );
    js_thread_exit_park( self->tid );
}

void cwasm_start_main( void ) {
    js_thread_host_ready(); // keep THREAD init lib from being gc-stripped
    cwasm_threads[ 0 ].tid = 0;
    atomic_store( &cwasm_threads[ 0 ].state, __CWASM_T_RUNNING );
}

__attribute__( ( export_name( "__cwasm_thread_init" ) ) ) void __cwasm_thread_init( uint32_t tid ) {
    if( tid >= __CWASM_MAX_THREADS ) { return; }
    cwasm_threads[ tid ].tid = tid;
    if( !cwasm_threads[ tid ].tls_base ) {
        uint32_t tls_sz = 0;
        int tls_oom = 0;
        void* tls = alloc_tls_image( &tls_sz, &tls_oom );
        (void)tls_oom;
        if( tls &&__wasm_init_tls ) {
            __wasm_init_tls( tls );
            cwasm_threads[ tid ].tls_base = (uint32_t)(uintptr_t)tls;
            cwasm_threads[ tid ].tls_size = tls_sz;
        }
    } else if( __wasm_init_tls ) {
        __wasm_init_tls( (void*)(uintptr_t)cwasm_threads[ tid ].tls_base );
    }
    if( tid != 0 ) { __cwasm_fenv_set_except( cwasm_threads[ tid ].fenv_except ); }
}
