
#include <threads.h>

#ifndef __STDC_NO_THREADS__

    #include <errno.h>
    #include <pthread.h>
    #include <sched.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <time.h>

    static int thrd_from_errno( int e ) {
        switch( e ) {
            case 0: return thrd_success;
            case EBUSY: return thrd_busy;
            case ETIMEDOUT: return thrd_timedout;
            case ENOMEM:
            case EAGAIN: return thrd_nomem;
            default: return thrd_error;
        }
    }


    void call_once( once_flag* flag, void ( *func )( void ) ) {
        pthread_once( flag, func );
    }


    int cnd_init( cnd_t* cond ) {
        return thrd_from_errno( pthread_cond_init( cond, NULL ) );
    }


    int cnd_signal( cnd_t* cond ) {
        return thrd_from_errno( pthread_cond_signal( cond ) );
    }


    int cnd_broadcast( cnd_t* cond ) {
        return thrd_from_errno( pthread_cond_broadcast( cond ) );
    }


    int cnd_wait( cnd_t* cond, mtx_t* mtx ) {
        return thrd_from_errno( pthread_cond_wait( cond, mtx ) );
    }


    int cnd_timedwait( cnd_t* restrict cond, mtx_t* restrict mtx,
        struct timespec const* restrict ts ) {

        // C11 specifies `ts` as an absolute time based on TIME_UTC, which is what
        // pthread_cond_timedwait wants (CLOCK_REALTIME). No conversion needed.
        return thrd_from_errno( pthread_cond_timedwait( cond, mtx, ts ) );
    }


    void cnd_destroy( cnd_t* cond ) {
        pthread_cond_destroy( cond );
    }


    int mtx_init( mtx_t* mtx, int type ) {
        pthread_mutexattr_t attr;
        int rc;

        if( type & ~( mtx_plain | mtx_recursive | mtx_timed ) ) { return thrd_error; }

        if( pthread_mutexattr_init( &attr ) != 0 ) { return thrd_error; }
        if( type & mtx_recursive ) {
            pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
        }
        rc = pthread_mutex_init( mtx, &attr );
        pthread_mutexattr_destroy( &attr );
        return thrd_from_errno( rc );
    }


    int mtx_lock( mtx_t* mtx ) {
        return thrd_from_errno( pthread_mutex_lock( mtx ) );
    }


    int mtx_timedlock( mtx_t* restrict mtx, struct timespec const* restrict ts ) {
        return thrd_from_errno( pthread_mutex_timedlock( mtx, ts ) );
    }


    int mtx_trylock( mtx_t* mtx ) {
        return thrd_from_errno( pthread_mutex_trylock( mtx ) );
    }


    int mtx_unlock( mtx_t* mtx ) {
        return thrd_from_errno( pthread_mutex_unlock( mtx ) );
    }


    void mtx_destroy( mtx_t* mtx ) {
        pthread_mutex_destroy( mtx );
    }


    struct thrd_pack {
        thrd_start_t func;
        void* arg;
    };


    static void* thrd_entry( void* raw ) {
        struct thrd_pack* pack = raw;
        thrd_start_t func = pack->func;
        void* arg = pack->arg;
        free( pack );
        return (void*)(intptr_t)func( arg );
    }


    int thrd_create( thrd_t* thr, thrd_start_t func, void* arg ) {
        struct thrd_pack* pack;
        int rc;

        if( !thr || !func ) { return thrd_error; }
        pack = malloc( sizeof( *pack ) );
        if( !pack ) { return thrd_nomem; }
        pack->func = func;
        pack->arg = arg;
        rc = pthread_create( thr, NULL, thrd_entry, pack );
        if( rc != 0 ) {
            free( pack );
            return thrd_from_errno( rc );
        }
        return thrd_success;
    }


    thrd_t thrd_current( void ) {
        return pthread_self();
    }


    int thrd_detach( thrd_t thr ) {
        return thrd_from_errno( pthread_detach( thr ) );
    }


    int thrd_equal( thrd_t thr0, thrd_t thr1 ) {
        return pthread_equal( thr0, thr1 );
    }


    void thrd_exit( int res ) {
        pthread_exit( (void*)(intptr_t)res );
        __builtin_unreachable();
    }


    int thrd_join( thrd_t thr, int* res ) {
        void* retval = NULL;
        int rc = pthread_join( thr, &retval );
        if( rc != 0 ) { return thrd_from_errno( rc ); }
        if( res ) { *res = (int)(intptr_t)retval; }
        return thrd_success;
    }


    int thrd_sleep( struct timespec const* duration, struct timespec* remaining ) {
        if( !duration ) { return -2; }
        if( nanosleep( duration, remaining ) != 0 ) { return errno == EINTR ? -1 : -2; }
        return 0;
    }


    void thrd_yield( void ) {
        sched_yield();
    }


    int tss_create( tss_t* key, tss_dtor_t dtor ) {
        if( !key ) { return thrd_error; }
        return thrd_from_errno( pthread_key_create( key, dtor ) );
    }


    void* tss_get( tss_t key ) {
        return pthread_getspecific( key );
    }


    int tss_set( tss_t key, void* val ) {
        return thrd_from_errno( pthread_setspecific( key, val ) );
    }


    void tss_delete( tss_t key ) {
        pthread_key_delete( key );
    }

#endif /* !__STDC_NO_THREADS__ */
