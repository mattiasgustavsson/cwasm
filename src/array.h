/*
------------------------------------------------------------------------------
          Licensing information can be found at the end of the file.
------------------------------------------------------------------------------

array.h - v0.2 - Dynamic array library for C/C++.

Do this:
    #define ARRAY_IMPLEMENTATION
before you include this file in *one* C/C++ file to create the implementation.
*/

#ifndef array_h
#define array_h

#include <stddef.h>

#ifndef ARRAY_SIZE_T
    #define ARRAY_SIZE_T size_t
#endif


#ifndef ARRAY_UINTPTR_T
    #include <stdint.h>
    #define ARRAY_UINTPTR_T uintptr_t
#endif


#ifdef __cplusplus
    #if defined(_MSC_VER)
        #pragma warning( push )        
        #pragma warning( disable: 4577 ) // 'noexcept' used with no exception handling mode specified
    #endif
    #include <type_traits>
    #include <initializer_list>
    #if defined(_MSC_VER)
        #pragma warning( pop )
    #endif
#endif

#if defined(__clang__) || defined(__GNUC__)
    #define array_t( type ) struct __attribute__((__may_alias__)) { type* const items; ARRAY_SIZE_T const count; ARRAY_SIZE_T const capacity; }
#elif defined(__TINYC__)
    #define array_t( type ) struct { type* items; ARRAY_SIZE_T const count; ARRAY_SIZE_T const capacity; }
#else
    #define array_t( type ) struct { type* const items; ARRAY_SIZE_T const count; ARRAY_SIZE_T const capacity; }
#endif

/*

// How array params are passed

void f( array_param_t(int)* arrp ) {
    array_t(int)* arr; array_from_param( arr, arrp );
    for( int i = 0; i < arr->count; ++i ) {
        arr->items[ i ]++;
    }
}

...

array_t(int)* arr; array_create(arr);
array_add( arr, 1 );
array_add( arr, 2 );
test( array_to_param( arr ) );

*/



#define array_param_t( type ) type* const

// Note: __typeof__ is present in MSVC since 2024

#ifndef __cplusplus
    #define array_to_param(array) ( (__typeof__(&((array)->items))) ( (ARRAY_UINTPTR_T)(array) + offsetof(struct internal_array_t, items) ) )
#else
    #define array_to_param(array) \
        ( reinterpret_cast< decltype(&((array)->items)) >( (ARRAY_UINTPTR_T)(array) + offsetof(struct internal_array_t, items) ) )
#endif


#ifndef __cplusplus
    #define array_from_param(array, param) ( \
        INTERNAL_ARRAY_TYPE_CHECK( ((array)->items), (*(param)) ), \
        (array) = (void*)( (ARRAY_UINTPTR_T)(param) - offsetof(struct internal_array_t, items) ) \
    )
#else
    #define array_from_param(array, param) ( \
        INTERNAL_ARRAY_TYPE_CHECK( ((array)->items), (*(param)) ), \
        (array) = reinterpret_cast< decltype(array) >( (ARRAY_UINTPTR_T)(param) - offsetof(struct internal_array_t, items) ) \
    )
#endif


#ifndef __cplusplus
    #define array_create( array ) ( \
        INTERNAL_ARRAY_COMPATIBLE_TYPE( *((array)->items) ), \
        (array) = (void*) internal_array_create( sizeof( *((array)->items) ), NULL ) \
    )
    #define array_create_memctx( array, memctx ) ( \
        INTERNAL_ARRAY_COMPATIBLE_TYPE( *((array)->items) ), \
        (array) = (void*) internal_array_create( sizeof( *((array)->items) ), (memctx) ) \
    )
#else
    #define array_create( array ) ( \
        INTERNAL_ARRAY_COMPATIBLE_TYPE( *((array)->items) ), \
        (array) = reinterpret_cast< decltype(array) >( \
            internal_array_create( sizeof( *((array)->items) ), NULL ) ) \
    )
    #define array_create_memctx( array, memctx ) ( \
        INTERNAL_ARRAY_COMPATIBLE_TYPE( *((array)->items) ), \
        (array) = reinterpret_cast< decltype(array) >( \
            internal_array_create( sizeof( *((array)->items) ), (memctx) ) ) \
    )
#endif

#define array_destroy( array ) internal_array_destroy( (struct internal_array_t*) (array) )

#define array_reserve( array, min_capacity ) internal_array_reserve( (struct internal_array_t*) (array), (min_capacity) )

#ifndef __cplusplus
    #define array_add( array, item ) ( \
        INTERNAL_ARRAY_TYPE_CHECK( *((array)->items), (item) ), \
        (void)( *( (__typeof__(*((array)->items))*) internal_array_add( (struct internal_array_t*) (array) ) ) = (item) ) \
    )
#else
    #define array_add( array, item ) ( \
        INTERNAL_ARRAY_TYPE_CHECK( *((array)->items), (item) ), \
        (void)( *static_cast< std::remove_reference<decltype(*((array)->items))>::type* >( \
            internal_array_add( (struct internal_array_t*)(array) ) ) = (item) ) \
    )
#endif

#define array_remove( array, index ) internal_array_remove( (struct internal_array_t*) (array), (index) )
#define array_remove_ordered( array, index ) internal_array_remove_ordered( (struct internal_array_t*) (array), (index) )

#ifndef __cplusplus
    #define array_get( array, index, oob_value ) ( \
        INTERNAL_ARRAY_TYPE_CHECK( *((array)->items), (oob_value) ), \
        *((__typeof__( *((array)->items) ) const*) internal_array_get( (struct internal_array_t*)(array), (index), \
            ( (__typeof__(*((array)->items))[]) { (oob_value) } ) ) \
        ) \
    )
#elif defined( _MSC_VER ) 
    #define array_get( array, index, oob_value ) ( \
        __pragma( warning( push ) ) \
        __pragma( warning( disable: 4577 ) ) \
        INTERNAL_ARRAY_TYPE_CHECK( *((array)->items), (oob_value) ), \
        *static_cast< std::remove_reference<decltype(*((array)->items))>::type const* >( \
            internal_array_get( (struct internal_array_t*)(array), (index), \
                (void const*) std::initializer_list< std::remove_reference<decltype(*((array)->items))>::type > { (oob_value) }.begin() ) \
        ) \
        __pragma( warning( pop ) ) \
    ) 
#else
    #define array_get( array, index, oob_value ) ( \
        INTERNAL_ARRAY_TYPE_CHECK( *((array)->items), (oob_value) ), \
        *static_cast< std::remove_reference<decltype(*((array)->items))>::type const* >( \
            internal_array_get( (struct internal_array_t*)(array), (index), \
                (void const*) std::initializer_list< std::remove_reference<decltype(*((array)->items))>::type > { (oob_value) }.begin() ) \
        ) \
    )
#endif


#ifndef __cplusplus
    #define array_set( array, index, item ) ( \
        INTERNAL_ARRAY_TYPE_CHECK( *((array)->items), (item) ), \
        (void)( *( (__typeof__(*((array)->items))*) internal_array_set( (struct internal_array_t*) (array), (index) ) ) = (item) ) \
    )
#else
    #define array_set( array, index, item ) ( \
        INTERNAL_ARRAY_TYPE_CHECK( *((array)->items), (item) ), \
        (void)( *static_cast< std::remove_reference<decltype(*((array)->items))>::type* >( \
            internal_array_set( (struct internal_array_t*)(array), (index) ) ) = (item) ) \
    )
#endif

// Note: casting pointer types `(int(*)(void const*,void const*))(compare)` is fine in practice, as otherwise GetProcAddress and dlsym would not work either

#ifndef __cplusplus
    #define array_sort( array, compare ) ( \
        ( (void) sizeof( (compare)( (__typeof__(*((array)->items)) const*) 0, (__typeof__(*((array)->items)) const*) 0 ) ) ), \
        internal_array_sort( (struct internal_array_t*) (array), (int(*)(void const*,void const*))(compare) ) \
    )
#else
    #define array_sort( array, compare ) ( \
        ( (void) sizeof( (compare)( \
            (std::remove_reference<decltype(*((array)->items))>::type const*) 0, \
            (std::remove_reference<decltype(*((array)->items))>::type const*) 0 ) ) ), \
        internal_array_sort( (struct internal_array_t*)(array), (int(*)(void const*,void const*))(compare) ) \
    )
#endif

#define ARRAY_NOT_FOUND ((ARRAY_SIZE_T)-1)

#ifndef __cplusplus
    #define array_bsearch( array, key, compare ) ( \
        ( (void) sizeof( (compare)( (__typeof__(key) const*) 0, (__typeof__(*((array)->items)) const*) 0 ) ) ), \
        internal_array_bsearch( (struct internal_array_t*) (array), (void const*) ( (__typeof__((key))[]) { (key) } ), \
            (int(*)(void const*,void const*))(compare) ) \
    )
#else
    #define array_bsearch( array, key, compare ) ( \
        ( (void) sizeof( (compare)( \
            (std::remove_reference<decltype((key))>::type const*) 0, \
            (std::remove_reference<decltype(*((array)->items))>::type const*) 0 ) ) ), \
        internal_array_bsearch( (struct internal_array_t*)(array), \
            (void const*) std::initializer_list< std::remove_reference<decltype((key))>::type > { (key) }.begin(), \
            (int(*)(void const*,void const*))(compare) ) \
    )
#endif

#ifndef __cplusplus
    #define array_find( array, key, compare ) ( \
        ( (void) sizeof( (compare)( (__typeof__(key) const*) 0, (__typeof__(*((array)->items)) const*) 0 ) ) ), \
        internal_array_find( (struct internal_array_t*) (array), (void const*) ( (__typeof__((key))[]) { (key) } ), \
            (int(*)(void const*,void const*))(compare) ) \
    )
#else
    #define array_find( array, key, compare ) ( \
        ( (void) sizeof( (compare)( \
            (std::remove_reference<decltype((key))>::type const*) 0, \
            (std::remove_reference<decltype(*((array)->items))>::type const*) 0 ) ) ), \
        internal_array_find( (struct internal_array_t*)(array), \
            (void const*) std::initializer_list< std::remove_reference<decltype((key))>::type > { (key) }.begin(), \
            (int(*)(void const*,void const*))(compare) ) \
    )
#endif


#if defined( __clang__ )
    #define INTERNAL_ARRAY_TYPE_CHECK(a, b) \
        ( _Pragma("clang diagnostic push") \
          _Pragma("clang diagnostic error \"-Wpointer-type-mismatch\"") \
          _Pragma("clang diagnostic error \"-Wincompatible-pointer-types\"") \
          _Pragma("clang diagnostic error \"-Wconditional-type-mismatch\"") \
          ( (void) sizeof( 0 ? (b) : (a) ) ) \
          _Pragma("clang diagnostic pop") )
#elif defined( __GNUC__ )
    #define INTERNAL_ARRAY_TYPE_CHECK(a, b) ( (void) sizeof( 0 ? (b) : (a) ) )
#elif defined( __TINYC__ )
    #define INTERNAL_ARRAY_TYPE_CHECK(a, b) ( (void)sizeof( *( (__typeof__(a)*) 0 ) = (b) ) )
#elif defined( _MSC_VER ) 
    #define INTERNAL_ARRAY_TYPE_CHECK(a, b) \
        ( __pragma( warning( push ) ) \
          __pragma( warning( error: 4047) ) \
          __pragma( warning( error: 4133 ) ) \
          ( (void) sizeof( 0 ? (a) : (b) ) ) \
          __pragma( warning( pop ) ) )
#else
    #error Unsupported compiler.
#endif


#ifndef __cplusplus
    #define INTERNAL_ARRAY_COMPATIBLE_TYPE( x ) ( (void) 0 )
#else
    template<typename T> struct internal_array_trivially_copyable_check { 
        static_assert(std::is_trivially_copyable<T>::value, "array element type must be trivially copyable");
        static_assert(alignof(T) <= alignof(max_align_t), "array element type alignment not compatible");
        enum { value = 1 };
    };
    #define INTERNAL_ARRAY_COMPATIBLE_TYPE(x) \
        ( (void)sizeof( internal_array_trivially_copyable_check< typename std::remove_cv< typename std::remove_reference<decltype(x)>::type >::type > ) )
#endif


#if defined(__clang__) || defined(__GNUC__)
    struct __attribute__((__may_alias__)) internal_array_t { void* items; ARRAY_SIZE_T count; ARRAY_SIZE_T capacity; ARRAY_SIZE_T item_size; void* memctx; };
#else
    struct internal_array_t { void* items; ARRAY_SIZE_T count; ARRAY_SIZE_T capacity; ARRAY_SIZE_T item_size; void* memctx; };
#endif

struct internal_array_t* internal_array_create( ARRAY_SIZE_T item_size, void* memctx );
void internal_array_destroy( struct internal_array_t* array );
void internal_array_reserve( struct internal_array_t* array, ARRAY_SIZE_T min_capacity );
void* internal_array_add( struct internal_array_t* array );
void internal_array_remove( struct internal_array_t* array, ARRAY_SIZE_T index );
void internal_array_remove_ordered( struct internal_array_t* array, ARRAY_SIZE_T index );
void const* internal_array_get(struct internal_array_t* array, ARRAY_SIZE_T index, void const* oob_value );
void* internal_array_set( struct internal_array_t* array, ARRAY_SIZE_T index );
void internal_array_sort( struct internal_array_t* array, int (*compare)( void const* a, void const* b ) );
ARRAY_SIZE_T internal_array_bsearch( struct internal_array_t* array, void const* key, int (*compare)( void const* key, void const* item ) );
ARRAY_SIZE_T internal_array_find( struct internal_array_t* array, void const* key, int (*compare)( void const* key, void const* item ) );


#endif /* array_h */

// If we are running tests on windows
#if defined( ARRAY_RUN_TESTS ) && defined( _WIN32 ) && !defined( __TINYC__ )
    // To get file names/line numbers with meory leak detection, we need to include crtdbg.h before all other files
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
#endif

/*
----------------------
    IMPLEMENTATION
----------------------
*/

#ifdef ARRAY_IMPLEMENTATION
#undef ARRAY_IMPLEMENTATION

#ifndef ARRAY_MALLOC
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <stdlib.h>
    #define ARRAY_MALLOC( ctx, size ) ( malloc( size ) )
    #define ARRAY_FREE( ctx, ptr ) ( free( ptr ) )
#endif

#ifndef ARRAY_MEMSET
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <string.h>
    #define ARRAY_MEMSET( dst, val, cnt ) ( memset( (dst), (val), (cnt) ) )
#endif

#ifndef ARRAY_MEMCPY
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <string.h>
    #define ARRAY_MEMCPY( dst, src, cnt ) ( memcpy( (dst), (src), (cnt) ) )
#endif

#ifndef ARRAY_MEMMOVE
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <string.h>
    #define ARRAY_MEMMOVE( dst, src, cnt ) ( memmove( (dst), (src), (cnt) ) )
#endif

#ifndef ARRAY_QSORT
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <stdlib.h>
    #define ARRAY_QSORT( base, num, size, cmp ) ( qsort( (base), (num), (size), (cmp) ) )
#endif

#ifndef ARRAY_BSEARCH
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <stdlib.h>
    #define ARRAY_BSEARCH( key, base, num, size, cmp ) ( bsearch( (key), (base), (num), (size), (cmp) ) )
#endif

#ifndef ARRAY_ON_OUT_OF_BOUNDS
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <assert.h>
    #define ARRAY_ON_OUT_OF_BOUNDS( message ) assert( !(message) )
#endif

#ifndef ARRAY_ON_OUT_OF_MEMORY
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <assert.h>
    #define ARRAY_ON_OUT_OF_MEMORY( message ) assert( !(message) )
#endif

#ifndef ARRAY_DUMMY_ALIGN
    #if defined(__cplusplus)
        #define ARRAY_DUMMY_ALIGN alignof( max_align_t )
    #elif defined( _MSC_VER )
        #pragma warning( push ) 
        #pragma warning( disable: 4324 ) // structure was padded due to alignment specifier
        typedef struct __declspec(align(16)) internal_array_max_align_t { long long ll; long double ld; void* p; } internal_array_max_align_t;
        #pragma warning( pop )
        #define ARRAY_DUMMY_ALIGN __alignof( internal_array_max_align_t )
    #elif defined( __STDC_VERSION__ ) && __STDC_VERSION__ >= 201112L
        #include <stddef.h>
        #define ARRAY_DUMMY_ALIGN _Alignof( max_align_t )
    #else
        #define ARRAY_DUMMY_ALIGN ( sizeof( void* ) * 2 )
    #endif
#endif


struct internal_array_t* internal_array_create( ARRAY_SIZE_T item_size, void* memctx ) {
    struct internal_array_t* array = (struct internal_array_t*) ARRAY_MALLOC( memctx, sizeof( struct internal_array_t ) + item_size + ( ARRAY_DUMMY_ALIGN - 1 ) );
    if( !array || item_size == 0 || (ARRAY_SIZE_T)16 > ((ARRAY_SIZE_T)-1) / item_size ) {
        ARRAY_ON_OUT_OF_MEMORY( "Allocation failed for array_create" );
        return NULL;
    }

    array->memctx = memctx;
    array->item_size = item_size;
    array->capacity = 16;
    array->count = 0;
    array->items = ARRAY_MALLOC( memctx, array->capacity * item_size );
    if( !array->items ) {
        ARRAY_ON_OUT_OF_MEMORY( "Allocation failed for array_create" );
        ARRAY_FREE( array->memctx, array );
        return NULL;
    }
    return array;
}


void internal_array_destroy( struct internal_array_t* array ) {
    if( array ) {
        ARRAY_FREE( array->memctx, array->items );
        ARRAY_FREE( array->memctx, array );
    }
}


void internal_array_reserve( struct internal_array_t* array, ARRAY_SIZE_T min_capacity ) {
    if( array->capacity < min_capacity ) {
        // round to next power-of-two
        --min_capacity;
        min_capacity |= min_capacity >> 1;
        min_capacity |= min_capacity >> 2;
        min_capacity |= min_capacity >> 4;
        min_capacity |= min_capacity >> 8;
        min_capacity |= min_capacity >> 16;
        #if defined(_MSC_VER)
            #pragma warning( push )        
            #pragma warning( disable: 4127 ) // conditional expression is constant
        #endif
        if( sizeof(ARRAY_SIZE_T) == 8 ) min_capacity |= min_capacity >> 32;
        #if defined(_MSC_VER)
            #pragma warning( pop )
        #endif
        ++min_capacity;

        if( min_capacity == 0 || min_capacity > ( (ARRAY_SIZE_T)-1 ) / array->item_size ) {
            ARRAY_ON_OUT_OF_MEMORY( "Allocation failed for array_reserve" );
            return;
        }

        void* items = array->items;
        array->items = ARRAY_MALLOC( array->memctx, min_capacity * array->item_size );
        if( !array->items ) {
            ARRAY_ON_OUT_OF_MEMORY( "Allocation failed for array_reserve" );
            array->items = items;
            return;
        }
        array->capacity = min_capacity;
        ARRAY_MEMCPY( array->items, items, array->count * array->item_size );
        ARRAY_FREE( array->memctx, items );
    }
}


void* internal_array_add( struct internal_array_t* array ) {
    if( array->count >= array->capacity ) {
        if( array->capacity > ( (ARRAY_SIZE_T)-1 ) / 2 || array->capacity * 2 > ( (ARRAY_SIZE_T)-1 ) / array->item_size  ) {
            ARRAY_ON_OUT_OF_MEMORY("Allocation failed for array_add");
            void* dummy = (void*)( (ARRAY_UINTPTR_T)( array + 1 ) +
                ( ( (ARRAY_UINTPTR_T) ARRAY_DUMMY_ALIGN - ( (ARRAY_UINTPTR_T)( array + 1 ) % (ARRAY_UINTPTR_T) ARRAY_DUMMY_ALIGN ) )
                  % (ARRAY_UINTPTR_T) ARRAY_DUMMY_ALIGN ) );
            return dummy;
        }
        ARRAY_SIZE_T capacity = array->capacity * 2;
        void* items = array->items;
        array->items = ARRAY_MALLOC( array->memctx, capacity * array->item_size );
        if( !array->items ) {
            ARRAY_ON_OUT_OF_MEMORY( "Allocation failed for array_add" );
            array->items = items;
            void* dummy = (void*)( (ARRAY_UINTPTR_T)( array + 1 ) +
                ( ( (ARRAY_UINTPTR_T) ARRAY_DUMMY_ALIGN - ( (ARRAY_UINTPTR_T)( array + 1 ) % (ARRAY_UINTPTR_T) ARRAY_DUMMY_ALIGN ) )
                  % (ARRAY_UINTPTR_T) ARRAY_DUMMY_ALIGN ) );
            return dummy;
        }
        array->capacity = capacity;
        ARRAY_MEMCPY( array->items, items, array->count * array->item_size );
        ARRAY_FREE( array->memctx, items );
    }
    return (void*)( ( (ARRAY_UINTPTR_T) array->items ) + ( array->count++ ) * array->item_size );
}


void internal_array_remove( struct internal_array_t* array, ARRAY_SIZE_T index ) {
    if( index < array->count ) {
        --array->count;
        ARRAY_MEMMOVE( (void*)( ( (ARRAY_UINTPTR_T) array->items ) + index * array->item_size ),
            (void*)( ( (ARRAY_UINTPTR_T) array->items ) + array->count  * array->item_size ), array->item_size );
    } else {
        ARRAY_ON_OUT_OF_BOUNDS( "Array index out of bounds for array_remove" );
    }
}


void internal_array_remove_ordered( struct internal_array_t* array, ARRAY_SIZE_T index ) {
    if( index < array->count ) {
        --array->count;
        ARRAY_MEMMOVE( (void*)( ( (ARRAY_UINTPTR_T) array->items ) + index * array->item_size ),
            (void*)( ( (ARRAY_UINTPTR_T) array->items ) + ( index + 1 ) * array->item_size ),
            array->item_size * ( array->count - index ) );
    } else {
        ARRAY_ON_OUT_OF_BOUNDS( "Array index out of bounds for array_remove_ordered" );
    }
}


void const* internal_array_get( struct internal_array_t* array, ARRAY_SIZE_T index, void const* oob_value ) {
    if( index < array->count ) {
        return (void const*)( (ARRAY_UINTPTR_T)array->items + index * array->item_size );
    } else {
        ARRAY_ON_OUT_OF_BOUNDS( "Array index out of bounds for array_get" );
        return oob_value;
    }
}


void* internal_array_set( struct internal_array_t* array, ARRAY_SIZE_T index ) {
    if( index < array->count ) {
        return (void*)( ( (ARRAY_UINTPTR_T) array->items ) + index * array->item_size );
    } else {
        ARRAY_ON_OUT_OF_BOUNDS( "Array index out of bounds for array_set" );
        void* dummy = (void*)( (ARRAY_UINTPTR_T)( array + 1 ) +
            ( ( (ARRAY_UINTPTR_T) ARRAY_DUMMY_ALIGN - ( (ARRAY_UINTPTR_T)( array + 1 ) % (ARRAY_UINTPTR_T) ARRAY_DUMMY_ALIGN ) )
              % (ARRAY_UINTPTR_T) ARRAY_DUMMY_ALIGN ) );
        return dummy;
    }
}


void internal_array_sort( struct internal_array_t* array, int (*compare)( void const*, void const* ) ) {
    ARRAY_QSORT( array->items, array->count, array->item_size, compare );
}


ARRAY_SIZE_T internal_array_bsearch( struct internal_array_t* array, void const* key, int (*compare)( void const* key, void const* item ) ) {
    void* item = ARRAY_BSEARCH( key, array->items, array->count, array->item_size, compare );
    if( item ) {
        return (ARRAY_SIZE_T)( ( ((ARRAY_UINTPTR_T)item) - ((ARRAY_UINTPTR_T)array->items) ) / array->item_size );
    } else {
        return ARRAY_NOT_FOUND;
    }
}


ARRAY_SIZE_T internal_array_find( struct internal_array_t* array, void const* key, int (*compare)( void const* key, void const* item ) ) {
    for( ARRAY_SIZE_T i = 0; i < array->count; ++i ) {
        void const* item = (void const*)( ( (ARRAY_UINTPTR_T) array->items ) + i * array->item_size );
        if( compare( key, item ) == 0 ) {
            return i;
        }
    }
    return ARRAY_NOT_FOUND;
}


#endif /* ARRAY_IMPLEMENTATION */


/*
-------------
 TESTS
-------------

To build and run the test suite, compile it like this:
	
	cl /Tc array.h /DARRAY_RUN_TESTS /DARRAY_IMPLEMENTATION

and then simply run this from the commandline: 

	array.exe

For more extensive tests, like memory leaks or access violations, you can use the 
testfw.h test framework by placing it in the same directory as array.h, and 
defining the ARRAY_USE_EXTERNAL_TESTFW when building

	cl /Tc array.h /DARRAY_RUN_TESTS /DARRAY_IMPLEMENTATION /DARRAY_USE_EXTERNAL_TESTFW /MTd

Note that to get correct memory leak reporting, you must specify /MTd to use the
debug memory allocation system in MSVC.

You can find testfw.h here:

	https://github.com/mattiasgustavsson/libs/blob/main/testfw.h

*/


#ifdef ARRAY_RUN_TESTS

#ifdef ARRAY_USE_EXTERNAL_TESTFW
	#include "testfw.h"
#else
	#pragma warning( push )
	#pragma warning( disable: 4710 ) // function not inlined
	#include <stdio.h>
	#pragma warning( pop )

	char const* g_testfw_desc = NULL; int g_testfw_line = 0, g_testfw_current_test_failed = 0, g_testfw_tests_count = 0, g_testfw_asserts_count = 0, g_testfw_tests_failed = 0, g_testfw_asserts_failed = 0;
	#define TESTFW_INIT() g_testfw_desc = NULL; g_testfw_line = 0; g_testfw_current_test_failed = 0; g_testfw_tests_count = 0; g_testfw_asserts_count = 0; g_testfw_tests_failed = 0; g_testfw_asserts_failed = 0;
	#define TESTFW_SUMMARY() \
		( ( g_testfw_tests_failed == 0 ) ? printf( "\n===============================================================================\nAll tests passed (%d assertions in %d test cases)\n", g_testfw_asserts_count, g_testfw_tests_count ) : \
		( printf( "\n===============================================================================\ntest cases: %4d | %4d passed | %4d failed\n",  g_testfw_tests_count, g_testfw_tests_count - g_testfw_tests_failed, g_testfw_tests_failed ),\
		printf( "assertions: %4d | %4d passed | %4d failed\n",  g_testfw_asserts_count, g_testfw_asserts_count - g_testfw_asserts_failed, g_testfw_asserts_failed ) ), g_testfw_tests_failed != 0 )
	#define TESTFW_TEST_BEGIN( desc ) { ++g_testfw_tests_count; g_testfw_desc = (desc); g_testfw_line = __LINE__; g_testfw_current_test_failed = 0;
	#define TESTFW_TEST_END() if( g_testfw_current_test_failed ) { ++g_testfw_tests_failed; } } g_testfw_desc = NULL; g_testfw_line = 0;
	#define TESTFW_EXPECTED( expression ) \
		++g_testfw_asserts_count; if( !(expression) ) { ++g_testfw_asserts_failed; g_testfw_current_test_failed = 1; if( g_testfw_desc )  {  printf( "\n-------------------------------------------------------------------------------\n" ); \
		printf( "%s\n", g_testfw_desc );  printf( "-------------------------------------------------------------------------------\n" ); printf( "%s(%d): %s\n", __FILE__, g_testfw_line, __func__ ); \
		printf( "...............................................................................\n" ); g_testfw_desc = NULL; }  printf( "\n%s(%d): FAILED:\n", __FILE__, __LINE__ );  printf( "\n  TESTFW_EXPECTED( %s )\n", #expression ); }  
#endif


static int is_pow2_size_t( ARRAY_SIZE_T x ) {
    return x != 0 && (x & (x - 1)) == 0;
}

static ARRAY_SIZE_T next_pow2_ge( ARRAY_SIZE_T x ) {
    if( x <= 1 ) return 1;
    ARRAY_SIZE_T p = 1;
    while( p < x ) p <<= 1;
    return p;
}

static int cmp_int_asc( int const* a, int const* b ) {
    return (*a > *b) - (*a < *b);
}

typedef struct { int key; int payload; } rec_t;
static int cmp_rec_by_key( rec_t const* a, rec_t const* b ) {
    return (a->key > b->key) - (a->key < b->key);
}
static int cmp_key_int_to_rec( int const* k, rec_t const* e ) {
    return (*k > e->key) - (*k < e->key);
}

#ifdef __cplusplus
    #define REC_INIT( k, p ) rec_t{ (k), (p) }
#else
    #define REC_INIT( k, p ) (rec_t){ (k), (p) }
#endif

static int g_sidefx;
static int sidefx_int( int v ) { ++g_sidefx; return v; }

static void bump_all( array_param_t(int)* ap ) {
    array_t(int)* a;
    array_from_param( a, ap );
    for( ARRAY_SIZE_T i = 0; i < a->count; ++i ) a->items[ i ] += 1;
}

static uint32_t g_rng = 0x12345678u;
static uint32_t rng_u32( void ) {
    uint32_t x = g_rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g_rng = x;
    return x;
}



int main( int argc, char** argv ) {
    (void) argc, (void) argv;

    TESTFW_INIT();

    TESTFW_TEST_BEGIN( "destroy(NULL) is safe" );
        array_t(int)* arr = NULL;
        array_destroy( arr );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "create basic invariants" );
        array_t(int)* a = NULL;
        array_create( a );

        TESTFW_EXPECTED( a != NULL );
        TESTFW_EXPECTED( a->items != NULL );
        TESTFW_EXPECTED( a->count == 0 );
        TESTFW_EXPECTED( a->capacity >= 16 );
        TESTFW_EXPECTED( ((struct internal_array_t*)a)->item_size == sizeof( int ) );

        array_destroy( a );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "create_memctx stores ctx" );
        array_t(int)* a = NULL;
        void* ctx = (void*)0x1234;

        array_create_memctx( a, ctx );

        TESTFW_EXPECTED( a != NULL );
        TESTFW_EXPECTED( ((struct internal_array_t*)a)->memctx == ctx );

        array_destroy( a );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "add stores values and increments count" );
        array_t(int)* a = NULL;
        array_create( a );

        array_add( a, 10 );
        array_add( a, 20 );
        array_add( a, 30 );

        TESTFW_EXPECTED( a->count == 3 );
        TESTFW_EXPECTED( a->items[ 0 ] == 10 );
        TESTFW_EXPECTED( a->items[ 1 ] == 20 );
        TESTFW_EXPECTED( a->items[ 2 ] == 30 );

        array_destroy( a );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "add evaluates item expression once" );
        array_t(int)* a = NULL;
        array_create( a );

        g_sidefx = 0;
        array_add( a, sidefx_int( 7 ) );
        TESTFW_EXPECTED( g_sidefx == 1 );
        TESTFW_EXPECTED( a->count == 1 );
        TESTFW_EXPECTED( a->items[ 0 ] == 7 );

        array_destroy( a );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "growth happens exactly when count == capacity; content preserved" );
        array_t(int)* a = NULL;
        array_create( a );

        ARRAY_SIZE_T cap0 = a->capacity;

        /* fill without growth */
        for( ARRAY_SIZE_T i = 0; i < cap0; ++i ) array_add( a, (int)i );
        TESTFW_EXPECTED( a->count == cap0 );
        TESTFW_EXPECTED( a->capacity == cap0 );

        /* trigger growth */
        array_add( a, 999 );
        TESTFW_EXPECTED( a->count == cap0 + 1 );
        TESTFW_EXPECTED( a->capacity >= cap0 * 2 );

        /* preserved */
        for( ARRAY_SIZE_T i = 0; i < cap0; ++i ) { TESTFW_EXPECTED( a->items[ i ] == (int)i ); }
        TESTFW_EXPECTED( a->items[ cap0 ] == 999 );

        array_destroy( a );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "reserve no-op when min_capacity <= capacity" );
        array_t(int)* a = NULL;
        array_create( a );

        ARRAY_SIZE_T cap_before = a->capacity;
        array_reserve( a, cap_before );
        TESTFW_EXPECTED( a->capacity == cap_before );

        array_destroy( a );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "reserve rounds up to next power of two + preserves content" );
        array_t(int)* a = NULL;
        array_create( a );

        for( int i = 0; i < 25; ++i ) array_add( a, i * 2 );

        ARRAY_SIZE_T want = a->capacity + 1;
        array_reserve( a, want );

        TESTFW_EXPECTED( a->capacity >= want );
        TESTFW_EXPECTED( is_pow2_size_t( a->capacity ) );
        TESTFW_EXPECTED( a->count == 25 );
        for( int i = 0; i < 25; ++i ) { TESTFW_EXPECTED( a->items[ i ] == i * 2 ); }

        array_destroy( a );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "reserve rounding edge values hammer" );
        {
            ARRAY_SIZE_T mins[] = { 0, 1, 2, 3, 15, 16, 17, 31, 32, 33, 63, 64, 65 };
            for( int mi = 0; mi < (int)(sizeof( mins )/sizeof( *mins )); ++mi ) {
                array_t(int)* a = NULL;
                array_create( a );

                ARRAY_SIZE_T cap0 = a->capacity;
                array_reserve( a, mins[ mi ] );

                TESTFW_EXPECTED( a->count == 0 );

                if( mins[ mi ] <= cap0 ) {
                    TESTFW_EXPECTED( a->capacity == cap0 );
                } else {
                    TESTFW_EXPECTED( a->capacity >= mins[ mi ] );
                    TESTFW_EXPECTED( is_pow2_size_t( a->capacity ) );
                    /* for fresh arrays, capacity should be exactly next pow2 of mins[mi] */
                    TESTFW_EXPECTED( a->capacity == next_pow2_ge( mins[ mi ] ) );
                }

                array_destroy( a );
            }
        }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "reserve random small values hammer (fresh arrays)" );
        g_rng = 0x12345678u;

        for( int it = 0; it < 200; ++it ) {
            ARRAY_SIZE_T m = (ARRAY_SIZE_T)(rng_u32() % 256u);

            array_t(int)* a = NULL;
            array_create( a );

            ARRAY_SIZE_T cap0 = a->capacity;
            array_reserve( a, m );

            if( m <= cap0 ) {
                TESTFW_EXPECTED( a->capacity == cap0 );
            } else {
                TESTFW_EXPECTED( a->capacity >= m );
                TESTFW_EXPECTED( is_pow2_size_t( a->capacity ) );
            }

            array_destroy( a );
        }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "get/set in-bounds correctness (index 0 + last)" );
        array_t(int)* a = NULL;
        array_create( a );

        array_add( a, 11 );
        array_add( a, 22 );
        array_add( a, 33 );

        TESTFW_EXPECTED( array_get( a, 0, -1 ) == 11 );
        TESTFW_EXPECTED( array_get( a, a->count - 1, -1 ) == 33 );

        array_set( a, 0, 111 );
        array_set( a, a->count - 1, 333 );

        TESTFW_EXPECTED( a->items[ 0 ] == 111 );
        TESTFW_EXPECTED( a->items[ 1 ] == 22 );
        TESTFW_EXPECTED( a->items[ 2 ] == 333 );

        array_destroy( a );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "remove unordered edges (index 0, count==1, repeated to empty)" );
        {
            array_t(int)* a = NULL;
            array_create( a );

            for( int i = 0; i < 6; ++i ) array_add( a, i ); /* 0 1 2 3 4 5 */
            array_remove( a, 0 );
            TESTFW_EXPECTED( a->count == 5 );
            /* swapped last into first */
            TESTFW_EXPECTED( a->items[ 0 ] == 5 );

            array_destroy( a );
        }
        {
            array_t(int)* a = NULL;
            array_create( a );
            array_add( a, 42 );
            array_remove( a, 0 );
            TESTFW_EXPECTED( a->count == 0 );
            array_destroy( a );
        }
        {
            array_t(int)* a = NULL;
            array_create( a );
            for( int i = 0; i < 10; ++i ) array_add( a, 100 + i );
            while( a->count > 0 ) array_remove( a, 0 );
            TESTFW_EXPECTED( a->count == 0 );
            array_destroy( a );
        }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "remove_ordered edges (index 0 and last) preserve order" );
        {
            array_t(int)* a = NULL;
            array_create( a );
            for( int i = 0; i < 5; ++i ) array_add( a, 10 + i ); /* 10 11 12 13 14 */

            array_remove_ordered( a, 0 ); /* 11 12 13 14 */
            TESTFW_EXPECTED( a->count == 4 );
            TESTFW_EXPECTED( a->items[ 0 ] == 11 );
            TESTFW_EXPECTED( a->items[ 3 ] == 14 );

            array_remove_ordered( a, a->count - 1 ); /* remove 14 -> 11 12 13 */
            TESTFW_EXPECTED( a->count == 3 );
            TESTFW_EXPECTED( a->items[ 0 ] == 11 );
            TESTFW_EXPECTED( a->items[ 2 ] == 13 );

            array_destroy( a );
        }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "array_sort cases (sorted, reverse, all equal, duplicates)" );
        {
            array_t(int)* a = NULL;
            array_create( a );
            for( int i = 0; i < 20; ++i ) array_add( a, i );
            array_sort( a, cmp_int_asc );
            for( ARRAY_SIZE_T i = 1; i < a->count; ++i ) { TESTFW_EXPECTED( a->items[ i-1 ] <= a->items[ i ] ); }
            array_destroy( a );
        }
        {
            array_t(int)* a = NULL;
            array_create( a );
            for( int i = 19; i >= 0; --i ) array_add( a, i );
            array_sort( a, cmp_int_asc );
            for( ARRAY_SIZE_T i = 1; i < a->count; ++i ) { TESTFW_EXPECTED( a->items[ i-1 ] <= a->items[ i ] ); }
            array_destroy( a );
        }
        {
            array_t(int)* a = NULL;
            array_create( a );
            for( int i = 0; i < 100; ++i ) array_add( a, 7 );
            array_sort( a, cmp_int_asc );
            for( ARRAY_SIZE_T i = 0; i < a->count; ++i ) { TESTFW_EXPECTED( a->items[ i ] == 7 ); }
            array_destroy( a );
        }
        {
            array_t(int)* a = NULL;
            array_create( a );
            int vals[] = { 5, 1, 9, 1, 3, 7, 3, 3, 2, 2 };
            for( int i = 0; i < (int)(sizeof( vals )/sizeof( *vals )); ++i ) array_add( a, vals[ i ] );
            array_sort( a, cmp_int_asc );
            for( ARRAY_SIZE_T i = 1; i < a->count; ++i ) { TESTFW_EXPECTED( a->items[ i-1 ] <= a->items[ i ] ); }
            array_destroy( a );
        }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "array_bsearch (first/middle/last, not found, duplicates-any)" );
        {
            array_t(int)* a = NULL;
            array_create( a );
            int vals[] = { 1, 2, 3, 4, 5, 6, 7 };
            for( int i = 0; i < (int)(sizeof( vals )/sizeof( *vals )); ++i ) array_add( a, vals[ i ] );

            int k1 = 1, k4 = 4, k7 = 7;
            TESTFW_EXPECTED( array_bsearch( a, k1, cmp_int_asc ) == 0 );
            TESTFW_EXPECTED( array_bsearch( a, k4, cmp_int_asc ) == 3 );
            TESTFW_EXPECTED( array_bsearch( a, k7, cmp_int_asc ) == 6 );

            int kx = 999;
            TESTFW_EXPECTED( array_bsearch( a, kx, cmp_int_asc ) == ARRAY_NOT_FOUND );

            array_destroy( a );
        }
        {
            array_t(int)* a = NULL;
            array_create( a );
            int vals[] = { 1, 2, 2, 2, 3, 4 };
            for( int i = 0; i < (int)(sizeof( vals )/sizeof( *vals )); ++i ) array_add( a, vals[ i ] );

            int k = 2;
            ARRAY_SIZE_T idx = array_bsearch( a, k, cmp_int_asc );
            TESTFW_EXPECTED( idx != ARRAY_NOT_FOUND );
            TESTFW_EXPECTED( a->items[ idx ] == 2 );

            array_destroy( a );
        }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "array_find (first match, not found, heterogeneous key comparator)" );
        {
            array_t(int)* a = NULL;
            array_create( a );

            array_add( a, 1 );
            array_add( a, 2 );
            array_add( a, 2 );
            array_add( a, 3 );

            int k = 2;
            TESTFW_EXPECTED( array_find( a, k, cmp_int_asc ) == 1 );

            int knf = 9;
            TESTFW_EXPECTED( array_find( a, knf, cmp_int_asc ) == ARRAY_NOT_FOUND );

            array_destroy( a );
        }
        {
            array_t(rec_t)* a = NULL;
            array_create( a );
            array_add( a, REC_INIT( 10, 1000 ) );
            array_add( a, REC_INIT( 20, 2000 ) );
            array_add( a, REC_INIT( 30, 3000 ) );

            int k = 30;
            ARRAY_SIZE_T idx = array_find( a, k, cmp_key_int_to_rec );
            TESTFW_EXPECTED( idx == 2 );
            TESTFW_EXPECTED( a->items[ idx ].payload == 3000 );

            array_destroy( a );
        }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "struct element type works (sort + find by struct key)" );
        array_t(rec_t)* a = NULL;
        array_create( a );

        array_add( a, REC_INIT( 30, 3000 ) );
        array_add( a, REC_INIT( 10, 1000 ) );
        array_add( a, REC_INIT( 20, 2000 ) );

        array_sort( a, cmp_rec_by_key );
        TESTFW_EXPECTED( a->items[ 0 ].key == 10 );
        TESTFW_EXPECTED( a->items[ 1 ].key == 20 );
        TESTFW_EXPECTED( a->items[ 2 ].key == 30 );

        rec_t k = REC_INIT( 20, 0 );
        ARRAY_SIZE_T idx = array_find( a, k, cmp_rec_by_key );
        TESTFW_EXPECTED( idx == 1 );
        TESTFW_EXPECTED( a->items[ idx ].payload == 2000 );

        array_destroy( a );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "array_to_param/array_from_param roundtrip works" );
        array_t(int)* a = NULL;
        array_create( a );

        array_add( a, 10 );
        array_add( a, 20 );

        bump_all( array_to_param( a ) );

        TESTFW_EXPECTED( a->items[ 0 ] == 11 );
        TESTFW_EXPECTED( a->items[ 1 ] == 21 );

        array_destroy( a );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "NOT_FOUND constant is all-ones" );
        TESTFW_EXPECTED( ARRAY_NOT_FOUND == (ARRAY_SIZE_T)-1 );
    TESTFW_TEST_END();

    return TESTFW_SUMMARY();
}


// pass-through so the program will build with either /SUBSYSTEM:WINDOWS or /SUBSYSTEM:CONSOLE
#if defined( _WIN32 ) && !defined( __TINYC__ )
    #ifdef __cplusplus
        extern "C" int __stdcall WinMain( struct HINSTANCE__*, struct HINSTANCE__*, char*, int ) {
            return main( __argc, __argv );
        }
    #else
        struct HINSTANCE__;
        int __stdcall WinMain( struct HINSTANCE__* a, struct HINSTANCE__* b, char* c, int d ) {
            (void) a, (void) b, (void) c, (void) d; return main( __argc, __argv );
        }
    #endif
#endif

#ifdef ARRAY_USE_EXTERNAL_TESTFW
	#define TESTFW_IMPLEMENTATION
	#ifdef _MSC_VER
		#pragma warning( push )
		#pragma warning( disable: 4710 ) // function not inlined
		#pragma warning( disable: 4820 ) // '4' bytes padding added after data member
	#endif
	#include "testfw.h"
	#ifdef _MSC_VER
		#pragma warning( pop )
	#endif
#endif /* ARRAY_USE_EXTERNAL_TESTFW */

#endif /* ARRAY_RUN_TESTS */


/*
revision history:
    0.2     better type safety
    0.1     first released version
*/

/*
------------------------------------------------------------------------------

This software is available under 2 licenses - you may choose the one you like.

------------------------------------------------------------------------------

ALTERNATIVE A - MIT License

Copyright (c) 2022 Mattias Gustavsson

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

------------------------------------------------------------------------------

ALTERNATIVE B - Public Domain (www.unlicense.org)

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

------------------------------------------------------------------------------
*/
