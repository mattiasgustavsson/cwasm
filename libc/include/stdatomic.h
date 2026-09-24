#ifndef __CWASM_STDATOMIC_H__
#define __CWASM_STDATOMIC_H__
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
    typedef bool __cwasm_atomic_bool_t;
#else
    typedef _Bool __cwasm_atomic_bool_t;
#endif

typedef enum memory_order {
    memory_order_relaxed = __ATOMIC_RELAXED,
    memory_order_consume = __ATOMIC_CONSUME,
    memory_order_acquire = __ATOMIC_ACQUIRE,
    memory_order_release = __ATOMIC_RELEASE,
    memory_order_acq_rel = __ATOMIC_ACQ_REL,
    memory_order_seq_cst = __ATOMIC_SEQ_CST
} memory_order;

// no-op, we never form consume-ordering dependency chains.
#define kill_dependency(y) (y)

#define ATOMIC_BOOL_LOCK_FREE __GCC_ATOMIC_BOOL_LOCK_FREE
#define ATOMIC_CHAR_LOCK_FREE __GCC_ATOMIC_CHAR_LOCK_FREE
#define ATOMIC_CHAR16_T_LOCK_FREE __GCC_ATOMIC_CHAR16_T_LOCK_FREE
#define ATOMIC_CHAR32_T_LOCK_FREE __GCC_ATOMIC_CHAR32_T_LOCK_FREE
#define ATOMIC_WCHAR_T_LOCK_FREE __GCC_ATOMIC_WCHAR_T_LOCK_FREE
#define ATOMIC_SHORT_LOCK_FREE __GCC_ATOMIC_SHORT_LOCK_FREE
#define ATOMIC_INT_LOCK_FREE __GCC_ATOMIC_INT_LOCK_FREE
#define ATOMIC_LONG_LOCK_FREE __GCC_ATOMIC_LONG_LOCK_FREE
#define ATOMIC_LLONG_LOCK_FREE __GCC_ATOMIC_LLONG_LOCK_FREE
#define ATOMIC_POINTER_LOCK_FREE __GCC_ATOMIC_POINTER_LOCK_FREE

typedef _Atomic( __cwasm_atomic_bool_t ) atomic_bool;
typedef _Atomic( char ) atomic_char;
typedef _Atomic( signed char ) atomic_schar;
typedef _Atomic( unsigned char ) atomic_uchar;
typedef _Atomic( short ) atomic_short;
typedef _Atomic( unsigned short ) atomic_ushort;
typedef _Atomic( int ) atomic_int;
typedef _Atomic( unsigned int ) atomic_uint;
typedef _Atomic( long ) atomic_long;
typedef _Atomic( unsigned long ) atomic_ulong;
typedef _Atomic( long long ) atomic_llong;
typedef _Atomic( unsigned long long ) atomic_ullong;
typedef _Atomic( void* ) atomic_address;
typedef _Atomic( size_t ) atomic_size_t;
typedef _Atomic( ptrdiff_t ) atomic_ptrdiff_t;
typedef _Atomic( intptr_t ) atomic_intptr_t;
typedef _Atomic( uintptr_t ) atomic_uintptr_t;

typedef _Atomic( __CHAR16_TYPE__ ) atomic_char16_t;
typedef _Atomic( __CHAR32_TYPE__ ) atomic_char32_t;
typedef _Atomic( __WCHAR_TYPE__ ) atomic_wchar_t;

typedef _Atomic( int_least8_t ) atomic_int_least8_t;
typedef _Atomic( uint_least8_t ) atomic_uint_least8_t;
typedef _Atomic( int_least16_t ) atomic_int_least16_t;
typedef _Atomic( uint_least16_t ) atomic_uint_least16_t;
typedef _Atomic( int_least32_t ) atomic_int_least32_t;
typedef _Atomic( uint_least32_t ) atomic_uint_least32_t;
typedef _Atomic( int_least64_t ) atomic_int_least64_t;
typedef _Atomic( uint_least64_t ) atomic_uint_least64_t;

typedef _Atomic( int_fast8_t ) atomic_int_fast8_t;
typedef _Atomic( uint_fast8_t ) atomic_uint_fast8_t;
typedef _Atomic( int_fast16_t ) atomic_int_fast16_t;
typedef _Atomic( uint_fast16_t ) atomic_uint_fast16_t;
typedef _Atomic( int_fast32_t ) atomic_int_fast32_t;
typedef _Atomic( uint_fast32_t ) atomic_uint_fast32_t;
typedef _Atomic( int_fast64_t ) atomic_int_fast64_t;
typedef _Atomic( uint_fast64_t ) atomic_uint_fast64_t;

typedef _Atomic( intmax_t ) atomic_intmax_t;
typedef _Atomic( uintmax_t ) atomic_uintmax_t;

typedef struct atomic_flag { atomic_bool _v; } atomic_flag;

#define ATOMIC_FLAG_INIT { 0 }
#define ATOMIC_VAR_INIT(value) (value)

#define atomic_init(obj, val) __c11_atomic_init(obj, val)

#define atomic_store_explicit(obj, val, order) __c11_atomic_store(obj, val, order)
#define atomic_store(obj, val) atomic_store_explicit(obj, val, memory_order_seq_cst)

#define atomic_load_explicit(obj, order) __c11_atomic_load(obj, order)
#define atomic_load(obj) atomic_load_explicit(obj, memory_order_seq_cst)

#define atomic_exchange_explicit(obj, val, order) __c11_atomic_exchange(obj, val, order)
#define atomic_exchange(obj, val) atomic_exchange_explicit(obj, val, memory_order_seq_cst)

#define atomic_compare_exchange_strong_explicit(obj, expected, desired, succ, fail) \
    __c11_atomic_compare_exchange_strong(obj, expected, desired, succ, fail)
#define atomic_compare_exchange_weak_explicit(obj, expected, desired, succ, fail) \
    __c11_atomic_compare_exchange_weak(obj, expected, desired, succ, fail)
#define atomic_compare_exchange_strong(obj, expected, desired) \
    atomic_compare_exchange_strong_explicit(obj, expected, desired, memory_order_seq_cst, memory_order_seq_cst)
#define atomic_compare_exchange_weak(obj, expected, desired) \
    atomic_compare_exchange_weak_explicit(obj, expected, desired, memory_order_seq_cst, memory_order_seq_cst)

#define atomic_fetch_add_explicit(obj, arg, order) __c11_atomic_fetch_add(obj, arg, order)
#define atomic_fetch_add(obj, arg) atomic_fetch_add_explicit(obj, arg, memory_order_seq_cst)
#define atomic_fetch_sub_explicit(obj, arg, order) __c11_atomic_fetch_sub(obj, arg, order)
#define atomic_fetch_sub(obj, arg) atomic_fetch_sub_explicit(obj, arg, memory_order_seq_cst)
#define atomic_fetch_or_explicit(obj, arg, order) __c11_atomic_fetch_or(obj, arg, order)
#define atomic_fetch_or(obj, arg) atomic_fetch_or_explicit(obj, arg, memory_order_seq_cst)
#define atomic_fetch_xor_explicit(obj, arg, order) __c11_atomic_fetch_xor(obj, arg, order)
#define atomic_fetch_xor(obj, arg) atomic_fetch_xor_explicit(obj, arg, memory_order_seq_cst)
#define atomic_fetch_and_explicit(obj, arg, order) __c11_atomic_fetch_and(obj, arg, order)
#define atomic_fetch_and(obj, arg) atomic_fetch_and_explicit(obj, arg, memory_order_seq_cst)

#define atomic_thread_fence(order) __c11_atomic_thread_fence(order)
#define atomic_signal_fence(order) __c11_atomic_signal_fence(order)
#define atomic_is_lock_free(obj) __c11_atomic_is_lock_free(sizeof(*(obj)))

#ifdef __cplusplus
extern "C" {
#endif

static inline bool atomic_flag_test_and_set_explicit( volatile atomic_flag* obj,
    memory_order order ) {
    return atomic_exchange_explicit( &obj->_v, true, order );
}


static inline bool atomic_flag_test_and_set( volatile atomic_flag* obj ) {
    return atomic_flag_test_and_set_explicit( obj, memory_order_seq_cst );
}


static inline void atomic_flag_clear_explicit( volatile atomic_flag* obj,
    memory_order order ) {
    atomic_store_explicit( &obj->_v, false, order );
}


static inline void atomic_flag_clear( volatile atomic_flag* obj ) {
    atomic_flag_clear_explicit( obj, memory_order_seq_cst );
}

#ifdef __cplusplus
}
#endif

#endif // __CWASM_STDATOMIC_H__
