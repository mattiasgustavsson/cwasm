#ifndef __CWASM_PTHREAD_H__
#define __CWASM_PTHREAD_H__
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <sched.h>

#ifdef __cplusplus
    extern "C" {
#endif

typedef uint32_t pthread_t;
typedef uint32_t pthread_key_t;

typedef struct {
    uint32_t _flags;
    size_t _stacksize;
} pthread_attr_t;

typedef struct {
    _Alignas( 8 ) unsigned char _data[ 64 ];
} pthread_mutex_t;

typedef struct {
    int _type;
} pthread_mutexattr_t;

typedef struct {
    _Alignas( 8 ) unsigned char _data[ 48 ];
} pthread_cond_t;

typedef struct {
    int _unused;
} pthread_condattr_t;

typedef struct {
    int _once;
} pthread_once_t;

#define PTHREAD_ONCE_INIT { 0 }
#define PTHREAD_MUTEX_INITIALIZER {{0}}
#define PTHREAD_COND_INITIALIZER {{0}}

#define PTHREAD_CREATE_JOINABLE 0
#define PTHREAD_CREATE_DETACHED 1

int pthread_create( pthread_t* thread, pthread_attr_t const* attr, void* ( *start_routine )( void* ), void* arg );
int pthread_join( pthread_t thread, void** retval );
int pthread_detach( pthread_t thread );
void pthread_exit( void* retval );
pthread_t pthread_self( void );
int pthread_equal( pthread_t a, pthread_t b );

int pthread_attr_init( pthread_attr_t* attr );
int pthread_attr_destroy( pthread_attr_t* attr );
int pthread_attr_setstacksize( pthread_attr_t* attr, size_t stacksize );
int pthread_attr_getstacksize( pthread_attr_t const* attr, size_t* stacksize );
int pthread_attr_setdetachstate( pthread_attr_t* attr, int state );

int pthread_mutex_init( pthread_mutex_t* mutex, pthread_mutexattr_t const* attr );
int pthread_mutex_destroy( pthread_mutex_t* mutex );
int pthread_mutex_lock( pthread_mutex_t* mutex );
int pthread_mutex_trylock( pthread_mutex_t* mutex );
int pthread_mutex_timedlock( pthread_mutex_t* mutex, struct timespec const* abstime );
int pthread_mutex_unlock( pthread_mutex_t* mutex );

int pthread_cond_init( pthread_cond_t* cond, pthread_condattr_t const* attr );
int pthread_cond_destroy( pthread_cond_t* cond );
int pthread_cond_wait( pthread_cond_t* cond, pthread_mutex_t* mutex );
int pthread_cond_timedwait( pthread_cond_t* cond, pthread_mutex_t* mutex, struct timespec const* abstime );
int pthread_cond_signal( pthread_cond_t* cond );
int pthread_cond_broadcast( pthread_cond_t* cond );

int pthread_once( pthread_once_t* once_control, void ( *init_routine )( void ) );

int pthread_key_create( pthread_key_t* key, void ( *destructor )( void* ) );
int pthread_key_delete( pthread_key_t key );
int pthread_setspecific( pthread_key_t key, void const* value );
void* pthread_getspecific( pthread_key_t key );

int pthread_mutexattr_init( pthread_mutexattr_t* attr );
int pthread_mutexattr_destroy( pthread_mutexattr_t* attr );
int pthread_mutexattr_settype( pthread_mutexattr_t* attr, int type );

#define PTHREAD_MUTEX_NORMAL 0
#define PTHREAD_MUTEX_RECURSIVE 1
#define PTHREAD_MUTEX_ERRORCHECK 2

// POSIX requires at least 4; C11 exposes the same budget as TSS_DTOR_ITERATIONS in <threads.h>.
#define PTHREAD_DESTRUCTOR_ITERATIONS 4

#define PTHREAD_KEYS_MAX 128

int pthread_setschedparam( pthread_t thread, int policy, struct sched_param const* param );

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_PTHREAD_H__
