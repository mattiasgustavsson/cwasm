// Lock order (stdio):
//   g_stdio_lock (table) -> per-fh recursive lock when acquiring both from scratch.
//   Never *block* on a per-fh lock while holding the table lock (trylock + drop table +
//   retry) - otherwise flockfile->fopen deadlocks with fclose/fflush(NULL)/freopen.
//   Taking the table while already holding a per-fh lock is OK (flockfile->fopen,
//   freopen table re-acquire around slot publish).
//   When taking two fh locks, acquire lower index first.
//
// FS locks (g_fs_lock / g_wfs_lock):
//   Independent of stdio. Do not take stdio table or fh locks while holding an FS lock.
//   Stdio freopen/fopen drop fh locks across slow cwasm_fs_open so fh -> FS nesting does
//   not occur on those paths. Path resolve may briefly take g_fs_lock with no fh held.
#ifndef __CWASM_LOCK_H__
#define __CWASM_LOCK_H__

#ifdef CWASM_THREADS

    #include <pthread.h>

    typedef pthread_mutex_t cwasm_lock_t;
    #define CWASM_LOCK_INIT PTHREAD_MUTEX_INITIALIZER
    static inline void cwasm_lock( cwasm_lock_t* l ) { pthread_mutex_lock( l ); }
    static inline void cwasm_unlock( cwasm_lock_t* l ) { pthread_mutex_unlock( l ); }

// Recursive mutex - for FILE/flockfile and nested stdio. Must be inited at runtime (PTHREAD_MUTEX_INITIALIZER is non-recursive)
    typedef pthread_mutex_t cwasm_rec_lock_t;
    static inline void cwasm_rec_lock_init( cwasm_rec_lock_t* l ) {
        pthread_mutexattr_t a;
        pthread_mutexattr_init( &a );
        pthread_mutexattr_settype( &a, PTHREAD_MUTEX_RECURSIVE );
        pthread_mutex_init( l, &a );
        pthread_mutexattr_destroy( &a );
    }
    static inline void cwasm_rec_lock( cwasm_rec_lock_t* l ) { pthread_mutex_lock( l ); }
    static inline void cwasm_rec_unlock( cwasm_rec_lock_t* l ) { pthread_mutex_unlock( l ); }
    static inline int cwasm_rec_trylock( cwasm_rec_lock_t* l ) {
        return pthread_mutex_trylock( l ) == 0 ? 0 : -1;
    }

#else

    typedef int cwasm_lock_t;
    #define CWASM_LOCK_INIT 0
    static inline void cwasm_lock( cwasm_lock_t* l ) { (void)l; }
    static inline void cwasm_unlock( cwasm_lock_t* l ) { (void)l; }

    typedef int cwasm_rec_lock_t;
    static inline void cwasm_rec_lock_init( cwasm_rec_lock_t* l ) { (void)l; }
    static inline void cwasm_rec_lock( cwasm_rec_lock_t* l ) { (void)l; }
    static inline void cwasm_rec_unlock( cwasm_rec_lock_t* l ) { (void)l; }
    static inline int cwasm_rec_trylock( cwasm_rec_lock_t* l ) {
        (void)l;
        return 0;
    }

#endif

#endif // __CWASM_LOCK_H__
