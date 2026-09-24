#ifndef __CWASM_THREADS_H__
#define __CWASM_THREADS_H__
#if !defined(CWASM_THREADS) && !defined(__STDC_NO_THREADS__)
    #define __STDC_NO_THREADS__ 1
#endif

#if !defined(__cplusplus) && (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L)
    #ifndef thread_local
        #define thread_local _Thread_local
    #endif
#endif

#ifndef __STDC_NO_THREADS__

    #include <time.h>
    #include <pthread.h>

    #ifdef __cplusplus
        #ifndef restrict
            #define restrict __restrict
        #endif
    #endif

    #ifdef __cplusplus
        extern "C" {
    #endif

    typedef pthread_t thrd_t;
    typedef pthread_key_t tss_t;
    typedef pthread_mutex_t mtx_t;
    typedef pthread_cond_t cnd_t;
    typedef pthread_once_t once_flag;

    typedef int ( *thrd_start_t )( void* );
    typedef void ( *tss_dtor_t )( void* );

    #define ONCE_FLAG_INIT PTHREAD_ONCE_INIT
    #define TSS_DTOR_ITERATIONS PTHREAD_DESTRUCTOR_ITERATIONS

    enum {
        mtx_plain = 0,
        mtx_recursive = 1,
        mtx_timed = 2,
    };

    enum {
        thrd_success = 0,
        thrd_busy = 1,
        thrd_error = 2,
        thrd_nomem = 3,
        thrd_timedout = 4,
    };

    void call_once( once_flag* flag, void ( *func )( void ) );
    int cnd_init( cnd_t* cond );
    int cnd_signal( cnd_t* cond );
    int cnd_broadcast( cnd_t* cond );
    int cnd_wait( cnd_t* cond, mtx_t* mtx );
    int cnd_timedwait( cnd_t* restrict cond, mtx_t* restrict mtx,
        struct timespec const* restrict ts );
    void cnd_destroy( cnd_t* cond );

    int mtx_init( mtx_t* mtx, int type );
    int mtx_lock( mtx_t* mtx );
    int mtx_timedlock( mtx_t* restrict mtx, struct timespec const* restrict ts );
    int mtx_trylock( mtx_t* mtx );
    int mtx_unlock( mtx_t* mtx );
    void mtx_destroy( mtx_t* mtx );

    int thrd_create( thrd_t* thr, thrd_start_t func, void* arg );
    thrd_t thrd_current( void );
    int thrd_detach( thrd_t thr );
    int thrd_equal( thrd_t thr0, thrd_t thr1 );
    #if defined(__cplusplus)
        [[noreturn]] void thrd_exit( int res );
    #else
        _Noreturn void thrd_exit( int res );
    #endif
    int thrd_join( thrd_t thr, int* res );
    int thrd_sleep( struct timespec const* duration, struct timespec* remaining );
    void thrd_yield( void );

    int tss_create( tss_t* key, tss_dtor_t dtor );
    void* tss_get( tss_t key );
    int tss_set( tss_t key, void* val );
    void tss_delete( tss_t key );

    #ifdef __cplusplus
        }
    #endif

#endif // !__STDC_NO_THREADS__

#endif // __CWASM_THREADS_H__
