
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef CWASM_THREADS
    #include <pthread.h>
#endif

#pragma redefine_extname __atomic_load_c __atomic_load
#pragma redefine_extname __atomic_store_c __atomic_store
#pragma redefine_extname __atomic_exchange_c __atomic_exchange
#pragma redefine_extname __atomic_compare_exchange_c __atomic_compare_exchange
#pragma redefine_extname __atomic_is_lock_free_c __atomic_is_lock_free

#ifndef CWASM_ATOMIC_LOCK_COUNT
    #define CWASM_ATOMIC_LOCK_COUNT 64
#endif

_Static_assert( ( CWASM_ATOMIC_LOCK_COUNT & ( CWASM_ATOMIC_LOCK_COUNT - 1 ) ) == 0,
    "CWASM_ATOMIC_LOCK_COUNT must be a power of two" );

#ifdef CWASM_THREADS
    typedef pthread_mutex_t atomic_lock_t;
    static atomic_lock_t g_locks[ CWASM_ATOMIC_LOCK_COUNT ];
    static pthread_once_t g_locks_once = PTHREAD_ONCE_INIT;

    static void init_locks( void ) {
        for( size_t i = 0; i < CWASM_ATOMIC_LOCK_COUNT; ++i ) {
            pthread_mutex_init( &g_locks[ i ], NULL );
        }
    }

    static atomic_lock_t* lock_for( void* ptr ) {
        pthread_once( &g_locks_once, init_locks );
        uintptr_t key = ( (uintptr_t)ptr >> 4 ) & ( CWASM_ATOMIC_LOCK_COUNT - 1 );
        return &g_locks[ key ];
    }

    static void lock_acq( atomic_lock_t* l ) { pthread_mutex_lock( l ); }
    static void lock_rel( atomic_lock_t* l ) { pthread_mutex_unlock( l ); }
#else
    typedef char atomic_lock_t;
    static atomic_lock_t g_dummy;
    static atomic_lock_t* lock_for( void* ptr ) {
        (void)ptr;
        return &g_dummy;
    }
    static void lock_acq( atomic_lock_t* l ) { (void)l; }
    static void lock_rel( atomic_lock_t* l ) { (void)l; }
#endif

static bool native_lock_free( size_t size, void* ptr ) {
    uintptr_t p = (uintptr_t)ptr;
    switch( size ) {
        case 1:
        return true;
        case 2:
        return ( p & 1 ) == 0;
        case 4:
        return ( p & 3 ) == 0;
        case 8:
        return ( p & 7 ) == 0;
        default:
        return false;
    }
}

bool __atomic_is_lock_free_c( size_t size, void* ptr ) {
    return native_lock_free( size, ptr );
}

void __atomic_load_c( size_t size, void* ptr, void* ret, int model ) {
    (void)model;
    atomic_lock_t* l = lock_for( ptr );
    lock_acq( l );
    memcpy( ret, ptr, size );
    lock_rel( l );
}

void __atomic_store_c( size_t size, void* ptr, void* val, int model ) {
    (void)model;
    atomic_lock_t* l = lock_for( ptr );
    lock_acq( l );
    memcpy( ptr, val, size );
    lock_rel( l );
}

void __atomic_exchange_c( size_t size, void* ptr, void* val, void* ret, int model ) {
    (void)model;
    atomic_lock_t* l = lock_for( ptr );
    lock_acq( l );
    memcpy( ret, ptr, size );
    memcpy( ptr, val, size );
    lock_rel( l );
}

bool __atomic_compare_exchange_c( size_t size, void* ptr, void* expected, void* desired,
    int success_model, int failure_model ) {
    (void)success_model;
    (void)failure_model;
    atomic_lock_t* l = lock_for( ptr );
    lock_acq( l );
    if( memcmp( ptr, expected, size ) == 0 ) {
        memcpy( ptr, desired, size );
        lock_rel( l );
        return true;
    }
    memcpy( expected, ptr, size );
    lock_rel( l );
    return false;
}
