/*
------------------------------------------------------------------------------
          Licensing information can be found at the end of the file.
------------------------------------------------------------------------------

cstr.h - v1.1 - String interning and manipulation library for C/C++.

Do this:
    #define CSTR_IMPLEMENTATION
before you include this file in *one* C/C++ file to create the implementation.
*/

#ifndef cstr_h
#define cstr_h

#ifndef CSTR_U32
    #define CSTR_U32 unsigned int
#endif

#ifndef CSTR_SIZE_T
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <stddef.h>
    #define CSTR_SIZE_T size_t
#endif

#ifndef CSTR_BOOL_T
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <stdbool.h>
    #define CSTR_BOOL_T bool
#endif


#ifndef CSTR_VA_LIST_T
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <stdarg.h>
    #define CSTR_VA_LIST_T va_list
#endif


typedef struct cstr_restore_point_t cstr_restore_point_t;
typedef struct cstr_tokenizer_t { void* internal; } cstr_tokenizer_t;


#ifndef CSTR_NO_GLOBAL_API

void cstr_reset( void );

struct cstr_restore_point_t* cstr_restore_point( void );
void cstr_rollback( struct cstr_restore_point_t* restore_point );

char const* cstr( char const* str );
char const* cstr_n( char const* str, CSTR_SIZE_T n );

CSTR_BOOL_T cstr_is_interned( char const* str );

CSTR_SIZE_T cstr_len( char const* str );
CSTR_SIZE_T cstr_size( char const* str );

char const* cstr_cat( char const* a, char const* b );

char const* cstr_vformat( char const* format, CSTR_VA_LIST_T args );
char const* cstr_format( char const* format, ... );

char const* cstr_trim( char const* str );
char const* cstr_ltrim( char const* str );
char const* cstr_rtrim( char const* str );

char const* cstr_left( char const* str, CSTR_SIZE_T n );
char const* cstr_right( char const* str, CSTR_SIZE_T n );
char const* cstr_mid( char const* str, CSTR_SIZE_T start, CSTR_SIZE_T n );

char const* cstr_upper( char const* str );
char const* cstr_lower( char const* str );

char const* cstr_lpad( char const* str, char padding, CSTR_SIZE_T total_max_length );
char const* cstr_rpad( char const* str, char padding, CSTR_SIZE_T total_max_length );

char const* cstr_join( char const* a, char const* b, char const* separator );

char const* cstr_replace( char const* str, char const* find, char const* replacement );

char const* cstr_insert( char const* str, int position, char const* insertion );
char const* cstr_remove( char const* str, int start, int length );

char const* cstr_int( int i );
char const* cstr_float( float f );

CSTR_BOOL_T cstr_starts( char const* str, char const* start );
CSTR_BOOL_T cstr_ends( char const* str, char const* end );

CSTR_BOOL_T cstr_is_equal( char const* a, char const* b );
int cstr_compare( char const* a, char const* b );
int cstr_compare_nocase( char const* a, char const* b );

int cstr_find( char const* str, char const* find, int start );
int cstr_rfind( char const* str, char const* find, int end );

CSTR_U32 cstr_hash( char const* str );

struct cstr_tokenizer_t cstr_tokenizer( char const* str );
char const* cstr_tokenize( struct cstr_tokenizer_t* tokenizer, char const* separators );

char* cstr_temp_buffer( CSTR_SIZE_T capacity );
CSTR_SIZE_T cstr_temp_buffer_capacity( void );

#endif /* CSTR_NO_GLOBAL_API */


#ifdef CSTR_INSTANCE_API

typedef struct cstri_t cstri_t;
struct cstri_t* cstri_create( void* memctx );
void cstri_destroy( struct cstri_t* cstri );

void cstri_reset( struct cstri_t* cstri );

struct cstr_restore_point_t* cstri_restore_point( struct cstri_t* cstri );
void cstri_rollback( struct cstri_t* cstri, struct cstr_restore_point_t* restore_point );

char const* cstri( struct cstri_t* cstri, char const* str );
char const* cstri_n( struct cstri_t* cstri, char const* str, CSTR_SIZE_T n );

CSTR_BOOL_T cstri_is_interned( struct cstri_t* cstri, char const* str );

CSTR_SIZE_T cstri_len( struct cstri_t* cstri, char const* str );
CSTR_SIZE_T cstri_size( struct cstri_t* cstri, char const* str );

char const* cstri_cat( struct cstri_t* cstri, char const* a, char const* b );

char const* cstri_vformat( struct cstri_t* cstri, char const* format, CSTR_VA_LIST_T args );
char const* cstri_format( struct cstri_t* cstri, char const* format, ... );

char const* cstri_trim( struct cstri_t* cstri, char const* str );
char const* cstri_ltrim( struct cstri_t* cstri, char const* str );
char const* cstri_rtrim( struct cstri_t* cstri, char const* str );

char const* cstri_left( struct cstri_t* cstri, char const* str, CSTR_SIZE_T n );
char const* cstri_right( struct cstri_t* cstri, char const* str, CSTR_SIZE_T n );
char const* cstri_mid( struct cstri_t* cstri, char const* str, CSTR_SIZE_T start, CSTR_SIZE_T n );

char const* cstri_upper( struct cstri_t* cstri, char const* str );
char const* cstri_lower( struct cstri_t* cstri, char const* str );

char const* cstri_lpad( struct cstri_t* cstri, char const* str, char padding, CSTR_SIZE_T total_max_length );
char const* cstri_rpad( struct cstri_t* cstri, char const* str, char padding, CSTR_SIZE_T total_max_length );

char const* cstri_join( struct cstri_t* cstri, char const* a, char const* b, char const* separator );

char const* cstri_replace( struct cstri_t* cstri, char const* str, char const* find, char const* replacement );

char const* cstri_insert( struct cstri_t* cstri, char const* str, int position, char const* insertion );
char const* cstri_remove( struct cstri_t* cstri, char const* str, int start, int length );

char const* cstri_int( struct cstri_t* cstri, int i );
char const* cstri_float( struct cstri_t* cstri, float f );

CSTR_BOOL_T cstri_starts( struct cstri_t* cstri, char const* str, char const* start );
CSTR_BOOL_T cstri_ends( struct cstri_t* cstri, char const* str, char const* end );

CSTR_BOOL_T cstri_is_equal( struct cstri_t* cstri, char const* a, char const* b );
int cstri_compare( struct cstri_t* cstri, char const* a, char const* b );
int cstri_compare_nocase( struct cstri_t* cstri, char const* a, char const* b );

int cstri_find( struct cstri_t* cstri, char const* str, char const* find, int start );
int cstri_rfind( struct cstri_t* cstri, char const* str, char const* find, int end );

CSTR_U32 cstri_hash( struct cstri_t* cstri, char const* str );

struct cstr_tokenizer_t cstri_tokenizer( struct cstri_t* cstri, char const* str );
char const* cstri_tokenize( struct cstri_t* cstri, struct cstr_tokenizer_t* tokenizer, char const* separators );

char* cstri_temp_buffer( struct cstri_t* cstri, CSTR_SIZE_T capacity );
CSTR_SIZE_T cstri_temp_buffer_capacity( struct cstri_t* cstri );

#endif /* CSTR_INSTANCE_API */


#endif /* cstr_h */


/**

cstr.h
======

String interning and manipulation library for C/C++.



Example
-------




API Documentation
-----------------



### Customization


#### C standard string functions


#### Custom memory allocators

CSTR_GLOBAL_API_MEMCTX

#### Thread safety



Instanced API vs Global API
---------------------------

There are two separate, but mirrored, APIs for all of the cstr functions. There's a global API, where each function is
prefixed by `cstr_` and an instanced API, where each function is prefixed by `cstri_`. With the instanced API, you must
explicitly create a `struct cstri_t` instance by calling `cstri_create`, and pass this instance to every `cstri_`
function you call. This allows you to use multiple separate string interning pools if you need to, for example, if you
want to use it from multiple threads where each thread has its own independent string interning pools.

With the global API, a shared `struct cstri_t` instance is created and managed internally, and implicitly sent to all
`cstr_` function you call. This is for convenience, when you only want a single string interning pool. By default, it
is not safe to call these from multiple threads, but you can define a mutex for it to use, making it thread safe (see
the section on "Thread safety" under "Customization").


cstri_create
------------

    struct cstri_t* cstri_create( void* memctx );


    
    
cstri_destroy
-------------

    void cstri_destroy( struct cstri_t* cstri );

    

cstr_reset / cstri_reset
------------------------

    void cstr_reset( void );
    void cstri_reset( struct cstri_t* cstri );


cstr_restore_point / cstri_restore_point
----------------------------------------

    struct cstr_restore_point_t* cstr_restore_point( void );
    struct cstr_restore_point_t* cstri_restore_point( struct cstri_t* cstri );



cstr_rollback / cstri_rollback
------------------------------

    void cstr_rollback( struct cstr_restore_point_t* restore_point );
    void cstri_rollback( struct cstri_t* cstri, struct cstr_restore_point_t* restore_point );



cstr / cstri
------------

    char const* cstr( char const* str );
    char const* cstri( struct cstri_t* cstri, char const* str );



cstr_n / cstri_n
----------------

    char const* cstr_n( char const* str, CSTR_SIZE_T n );
    char const* cstri_n( struct cstri_t* cstri, char const* str, CSTR_SIZE_T n );



cstr_is_interned / cstri_is_interned
------------------------------------

    CSTR_BOOL_T cstr_is_interned( char const* str );
    CSTR_BOOL_T cstri_is_interned( struct cstri_t* cstri, char const* str );



cstr_len / cstri_len
--------------------

    CSTR_SIZE_T cstr_len( char const* str );
    CSTR_SIZE_T cstri_len( struct cstri_t* cstri, char const* str );



cstr_cat / cstri_cat
--------------------

    char const* cstr_cat( char const* a, char const* b );
    char const* cstri_cat( struct cstri_t* cstri, char const* a, char const* b );



cstr_vformat / cstri_vformat
----------------------------

    char const* cstr_vformat( char const* format, CSTR_VA_LIST_T args );
    char const* cstri_vformat( struct cstri_t* cstri, char const* format, CSTR_VA_LIST_T args );



cstr_format / cstri_format
--------------------------

    char const* cstr_format( char const* format, ... );
    char const* cstri_format( struct cstri_t* cstri, char const* format, ... );



cstr_trim / cstri_trim
----------------------

    char const* cstr_trim( char const* str );
    char const* cstri_trim( struct cstri_t* cstri, char const* str );



cstr_ltrim / cstri_ltrim
------------------------

    char const* cstr_ltrim( char const* str );
    char const* cstri_ltrim( struct cstri_t* cstri, char const* str );



cstr_rtrim / cstri_rtrim
------------------------

    char const* cstr_rtrim( char const* str );
    char const* cstri_rtrim( struct cstri_t* cstri, char const* str );




cstr_left / cstri_left
----------------------

    char const* cstr_left( char const* str, CSTR_SIZE_T n );
    char const* cstri_left( struct cstri_t* cstri, char const* str, CSTR_SIZE_T n );



cstr_right / cstri_right
------------------------

    char const* cstr_right( char const* str, CSTR_SIZE_T n );
    char const* cstri_right( struct cstri_t* cstri, char const* str, CSTR_SIZE_T n );



cstr_mid / cstri_mid
--------------------

    char const* cstr_mid( char const* str, CSTR_SIZE_T start, CSTR_SIZE_T n );
    char const* cstri_mid( struct cstri_t* cstri, char const* str, CSTR_SIZE_T start, CSTR_SIZE_T n );



cstr_upper / cstri_upper
------------------------

    char const* cstr_upper( char const* str );
    char const* cstri_upper( struct cstri_t* cstri, char const* str );


cstr_upper/cstr_lower use unconditional locale-neutral Unicode case mappings only. They do not apply locale-sensitive or context-sensitive mappings.


cstr_lower / cstri_lower
------------------------

    char const* cstr_lower( char const* str );
    char const* cstri_lower( struct cstri_t* cstri, char const* str );

cstr_upper/cstr_lower use unconditional locale-neutral Unicode case mappings only. They do not apply locale-sensitive or context-sensitive mappings.


cstr_lpad / cstri_lpad
----------------------

    char const* cstr_lpad( char const* str, char padding, CSTR_SIZE_T total_max_length );
    char const* cstri_lpad( struct cstri_t* cstri, char const* str, char padding, CSTR_SIZE_T total_max_length );



cstr_rpad / cstri_rpad
----------------------

    char const* cstr_rpad( char const* str, char padding, CSTR_SIZE_T total_max_length );
    char const* cstri_rpad( struct cstri_t* cstri, char const* str, char padding, CSTR_SIZE_T total_max_length );



cstr_join / cstri_join
----------------------

    char const* cstr_join( char const* a, char const* b, char const* separator );
    char const* cstri_join( struct cstri_t* cstri, char const* a, char const* b, char const* separator );



cstr_replace / cstri_replace
----------------------------

    char const* cstr_replace( char const* str, char const* find, char const* replacement );
    char const* cstri_replace( struct cstri_t* cstri, char const* str, char const* find, char const* replacement );



cstr_insert / cstri_insert
--------------------------

    char const* cstr_insert( char const* str, int position, char const* insertion );
    char const* cstri_insert( struct cstri_t* cstri, char const* str, int position, char const* insertion );



cstr_remove / cstri_remove
--------------------------

    char const* cstr_remove( char const* str, int start, int length );
    char const* cstri_remove( struct cstri_t* cstri, char const* str, int start, int length );




cstr_int / cstri_int
--------------------

    char const* cstr_int( int i );
    char const* cstri_int( struct cstri_t* cstri, int i );



cstr_float / cstri_float
------------------------

    char const* cstr_float( float f );
    char const* cstri_float( struct cstri_t* cstri, float f );



cstr_starts / cstri_starts
--------------------------

    CSTR_BOOL_T cstr_starts( char const* str, char const* start );
    CSTR_BOOL_T cstri_starts( struct cstri_t* cstri, char const* str, char const* start );



cstr_ends / cstri_ends
----------------------

    CSTR_BOOL_T cstr_ends( char const* str, char const* end );
    CSTR_BOOL_T cstri_ends( struct cstri_t* cstri, char const* str, char const* end );



cstr_is_equal / cstri_is_equal
------------------------------

    CSTR_BOOL_T cstr_is_equal( char const* a, char const* b );
    CSTR_BOOL_T cstri_is_equal( struct cstri_t* cstri, char const* a, char const* b );



cstr_compare / cstri_compare
----------------------------

    int cstr_compare( char const* a, char const* b );
    int cstri_compare( struct cstri_t* cstri, char const* a, char const* b );



cstr_compare_nocase / cstri_compare_nocase
------------------------------------------

    int cstr_compare_nocase( char const* a, char const* b );
    int cstri_compare_nocase( struct cstri_t* cstri, char const* a, char const* b );



cstr_find / cstri_find
----------------------

    int cstr_find( char const* str, char const* find, int start );
    int cstri_find( struct cstri_t* cstri, char const* str, char const* find, int start );



cstr_rfind / cstri_rfind
------------------------

    int cstr_rfind( char const* str, char const* find, int end );
    int cstri_rfind( struct cstri_t* cstri, char const* str, char const* find, int end );



cstr_hash / cstri_hash
----------------------

    CSTR_U32 cstr_hash( char const* str );
    CSTR_U32 cstri_hash( struct cstri_t* cstri, char const* str );



cstr_tokenizer / cstri_tokenizer
--------------------------------

    struct cstr_tokenizer_t cstr_tokenizer( char const* str );
    struct cstr_tokenizer_t cstri_tokenizer( struct cstri_t* cstri, char const* str );



cstr_tokenize / cstri_tokenize
------------------------------

    char const* cstr_tokenize( struct cstr_tokenizer_t* tokenizer, char const* separators );
    char const* cstri_tokenize( struct cstri_t* cstri, struct cstr_tokenizer_t* tokenizer, char const* separators );



cstr_temp_buffer / cstri_temp_buffer
------------------------------------

    char* cstr_temp_buffer( CSTR_SIZE_T capacity );
    char* cstri_temp_buffer( struct cstri_t* cstri, CSTR_SIZE_T capacity );


cstr_temp_buffer_capacity / cstri_temp_buffer_capacity
------------------------------------

    CSTR_SIZE_T cstr_temp_buffer_capacity( void );
    CSTR_SIZE_T cstri_temp_buffer_capacity( struct cstri_t* cstri );


*/



// If we are running tests on windows
#if defined( CSTR_RUN_TESTS ) && defined( _WIN32 ) && !defined( __TINYC__ )
    // To get file names/line numbers with meory leak detection, we need to include crtdbg.h before all other files
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
#endif


/*
----------------------
    IMPLEMENTATION
----------------------
*/

#ifdef CSTR_IMPLEMENTATION
#undef CSTR_IMPLEMENTATION

#ifndef CSTR_DEFAULT_BLOCK_SIZE
    #define CSTR_DEFAULT_BLOCK_SIZE 0x400000 /* 4 MB */
#endif

#ifndef CSTR_MUTEX_LOCK
    #define CSTR_MUTEX_LOCK()
    #define CSTR_MUTEX_UNLOCK()
#endif

#ifndef CSTR_VA_START
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <stdarg.h>
    #define CSTR_VA_START va_start
    #define CSTR_VA_END va_end
    #define CSTR_VA_COPY va_copy
#endif

#ifndef CSTR_ASSERT
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <assert.h>
    #define CSTR_ASSERT( expression, message ) assert( ( expression ) && ( message ) )
#endif

#ifndef CSTR_ISSPACE
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <ctype.h>
    #define CSTR_ISSPACE( c ) ( isspace( c ) )
#endif

#ifndef CSTR_TOUPPER
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <ctype.h>
    #define CSTR_TOUPPER( c ) ( toupper( c ) )
#endif

#ifndef CSTR_TOLOWER
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <ctype.h>
    #define CSTR_TOLOWER( c ) ( tolower( c ) )
#endif

#ifndef CSTR_MEMCPY
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <string.h>
    #define CSTR_MEMCPY( dst, src, cnt ) ( memcpy( (dst), (src), (cnt) ) )
#endif

#ifndef CSTR_MEMCMP
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <string.h>
    #define CSTR_MEMCMP( a, b, cnt ) ( memcmp( (a), (b), (cnt) ) )
#endif

#ifndef CSTR_MEMSET
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <string.h>
    #define CSTR_MEMSET( ptr, val, cnt ) ( memset( (ptr), (val), (cnt) ) )
#endif

#ifndef CSTR_STRLEN
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <string.h>
    #define CSTR_STRLEN( s ) ( strlen( s ) )
#endif

#ifndef CSTR_STRSTR
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
	#include <string.h>
    #define CSTR_STRSTR( s1, s2 ) ( strstr( (s1), (s2) ) )
#endif

#ifndef CSTR_STRNICMP
    #ifdef _WIN32
        #define _CRT_NONSTDC_NO_DEPRECATE
        #define _CRT_SECURE_NO_WARNINGS
        #include <string.h>
        #define CSTR_STRNICMP( s1, s2, len ) ( _strnicmp( (s1), (s2), (len) ) )
    #else
        #include <strings.h>
        #define CSTR_STRNICMP( s1, s2, len ) ( strncasecmp( (s1), (s2), (len) ) )
    #endif
#endif

#ifndef CSTR_VSNPRINTF
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <stdio.h>
    #define CSTR_VSNPRINTF( s, n, fmt, args ) ( vsnprintf( (s), (n), (fmt), (args) ) )
#endif

#ifndef CSTR_MALLOC
    #define _CRT_NONSTDC_NO_DEPRECATE
    #define _CRT_SECURE_NO_WARNINGS
    #include <stdlib.h>
    #define CSTR_MALLOC( ctx, size ) ( malloc( size ) )
    #define CSTR_FREE( ctx, ptr ) ( free( ptr ) )
#endif

#ifndef CSTR_GLOBAL_API_MEMCTX
    #define CSTR_GLOBAL_API_MEMCTX NULL
#endif

#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#include <stdarg.h>


enum { INTERNAL_CSTR_ITEM_HEADER_SIZE = (int)( sizeof( CSTR_SIZE_T ) + sizeof( CSTR_SIZE_T ) + sizeof( CSTR_U32 ) ) };


//// instance api

struct cstr_block_t {
    char* head;
    char* tail;
    char* end;
};


struct cstr_slot_t {
    CSTR_U32 hash;
    CSTR_SIZE_T byte_count;
    CSTR_SIZE_T codepoint_count;
    char const* string;
};


struct cstri_t {
    void* memctx;
    CSTR_SIZE_T blocks_count;
    CSTR_SIZE_T blocks_capacity;
    struct cstr_block_t* blocks;
    CSTR_SIZE_T hash_table_count;
    CSTR_SIZE_T hash_table_capacity;
    struct cstr_slot_t* hash_table;
    CSTR_SIZE_T temp_capacity;
    char* temp_buffer;
};


#ifndef CSTR_ASCII_ONLY

static struct internal_cstr_utf8_range_t { CSTR_U32 lo; CSTR_U32 hi; } const internal_cstr_utf8_whitespace[] = {
    { 0x000009u, 0x00000Du }, { 0x000020u, 0x000020u }, { 0x000085u, 0x000085u }, { 0x0000A0u, 0x0000A0u }, { 0x001680u, 0x001680u }, { 0x002000u, 0x00200Au }, { 0x002028u, 0x002028u }, { 0x002029u, 0x002029u }, { 0x00202Fu, 0x00202Fu },
    { 0x00205Fu, 0x00205Fu }, { 0x003000u, 0x003000u },
};


static struct { CSTR_U32 lo; CSTR_U32 hi; int delta; } const internal_cstr_utf8_to_upper_ranges[] = {
    { 0x000061u, 0x00007Au, -32 }, { 0x0000E0u, 0x0000F6u, -32 }, { 0x0000F8u, 0x0000FEu, -32 }, { 0x00023Fu, 0x000240u, 10815 }, { 0x000256u, 0x000257u, -205 }, { 0x00028Au, 0x00028Bu, -217 }, { 0x00037Bu, 0x00037Du, 130 },
    { 0x0003ADu, 0x0003AFu, -37 }, { 0x0003B1u, 0x0003C1u, -32 }, { 0x0003C3u, 0x0003CBu, -32 }, { 0x0003CDu, 0x0003CEu, -63 }, { 0x000430u, 0x00044Fu, -32 }, { 0x000450u, 0x00045Fu, -80 }, { 0x000561u, 0x000586u, -48 },
    { 0x0010D0u, 0x0010FAu, 3008 }, { 0x0010FDu, 0x0010FFu, 3008 }, { 0x0013F8u, 0x0013FDu, -8 }, { 0x001C83u, 0x001C84u, -6242 }, { 0x001F00u, 0x001F07u, 8 }, { 0x001F10u, 0x001F15u, 8 }, { 0x001F20u, 0x001F27u, 8 }, { 0x001F30u, 0x001F37u, 8 },
    { 0x001F40u, 0x001F45u, 8 }, { 0x001F60u, 0x001F67u, 8 }, { 0x001F70u, 0x001F71u, 74 }, { 0x001F72u, 0x001F75u, 86 }, { 0x001F76u, 0x001F77u, 100 }, { 0x001F78u, 0x001F79u, 128 }, { 0x001F7Au, 0x001F7Bu, 112 }, { 0x001F7Cu, 0x001F7Du, 126 },
    { 0x001F80u, 0x001F87u, 8 }, { 0x001F90u, 0x001F97u, 8 }, { 0x001FA0u, 0x001FA7u, 8 }, { 0x001FB0u, 0x001FB1u, 8 }, { 0x001FD0u, 0x001FD1u, 8 }, { 0x001FE0u, 0x001FE1u, 8 }, { 0x002170u, 0x00217Fu, -16 }, { 0x0024D0u, 0x0024E9u, -26 },
    { 0x002C30u, 0x002C5Fu, -48 }, { 0x002D00u, 0x002D25u, -7264 }, { 0x00AB70u, 0x00ABBFu, -38864 }, { 0x00FF41u, 0x00FF5Au, -32 }, { 0x010428u, 0x01044Fu, -40 }, { 0x0104D8u, 0x0104FBu, -40 }, { 0x010597u, 0x0105A1u, -39 },
    { 0x0105A3u, 0x0105B1u, -39 }, { 0x0105B3u, 0x0105B9u, -39 }, { 0x0105BBu, 0x0105BCu, -39 }, { 0x010CC0u, 0x010CF2u, -64 }, { 0x010D70u, 0x010D85u, -32 }, { 0x0118C0u, 0x0118DFu, -32 }, { 0x016E60u, 0x016E7Fu, -32 },
    { 0x016EBBu, 0x016ED3u, -27 }, { 0x01E922u, 0x01E943u, -34 },
};


static struct { CSTR_U32 cp; CSTR_U32 to; } const internal_cstr_utf8_to_upper_pairs[] = {
    { 0x0000B5u, 0x00039Cu }, { 0x0000FFu, 0x000178u }, { 0x000101u, 0x000100u }, { 0x000103u, 0x000102u }, { 0x000105u, 0x000104u }, { 0x000107u, 0x000106u }, { 0x000109u, 0x000108u }, { 0x00010Bu, 0x00010Au }, { 0x00010Du, 0x00010Cu },
    { 0x00010Fu, 0x00010Eu }, { 0x000111u, 0x000110u }, { 0x000113u, 0x000112u }, { 0x000115u, 0x000114u }, { 0x000117u, 0x000116u }, { 0x000119u, 0x000118u }, { 0x00011Bu, 0x00011Au }, { 0x00011Du, 0x00011Cu }, { 0x00011Fu, 0x00011Eu },
    { 0x000121u, 0x000120u }, { 0x000123u, 0x000122u }, { 0x000125u, 0x000124u }, { 0x000127u, 0x000126u }, { 0x000129u, 0x000128u }, { 0x00012Bu, 0x00012Au }, { 0x00012Du, 0x00012Cu }, { 0x00012Fu, 0x00012Eu }, { 0x000131u, 0x000049u },
    { 0x000133u, 0x000132u }, { 0x000135u, 0x000134u }, { 0x000137u, 0x000136u }, { 0x00013Au, 0x000139u }, { 0x00013Cu, 0x00013Bu }, { 0x00013Eu, 0x00013Du }, { 0x000140u, 0x00013Fu }, { 0x000142u, 0x000141u }, { 0x000144u, 0x000143u },
    { 0x000146u, 0x000145u }, { 0x000148u, 0x000147u }, { 0x00014Bu, 0x00014Au }, { 0x00014Du, 0x00014Cu }, { 0x00014Fu, 0x00014Eu }, { 0x000151u, 0x000150u }, { 0x000153u, 0x000152u }, { 0x000155u, 0x000154u }, { 0x000157u, 0x000156u },
    { 0x000159u, 0x000158u }, { 0x00015Bu, 0x00015Au }, { 0x00015Du, 0x00015Cu }, { 0x00015Fu, 0x00015Eu }, { 0x000161u, 0x000160u }, { 0x000163u, 0x000162u }, { 0x000165u, 0x000164u }, { 0x000167u, 0x000166u }, { 0x000169u, 0x000168u },
    { 0x00016Bu, 0x00016Au }, { 0x00016Du, 0x00016Cu }, { 0x00016Fu, 0x00016Eu }, { 0x000171u, 0x000170u }, { 0x000173u, 0x000172u }, { 0x000175u, 0x000174u }, { 0x000177u, 0x000176u }, { 0x00017Au, 0x000179u }, { 0x00017Cu, 0x00017Bu },
    { 0x00017Eu, 0x00017Du }, { 0x00017Fu, 0x000053u }, { 0x000180u, 0x000243u }, { 0x000183u, 0x000182u }, { 0x000185u, 0x000184u }, { 0x000188u, 0x000187u }, { 0x00018Cu, 0x00018Bu }, { 0x000192u, 0x000191u }, { 0x000195u, 0x0001F6u },
    { 0x000199u, 0x000198u }, { 0x00019Au, 0x00023Du }, { 0x00019Bu, 0x00A7DCu }, { 0x00019Eu, 0x000220u }, { 0x0001A1u, 0x0001A0u }, { 0x0001A3u, 0x0001A2u }, { 0x0001A5u, 0x0001A4u }, { 0x0001A8u, 0x0001A7u }, { 0x0001ADu, 0x0001ACu },
    { 0x0001B0u, 0x0001AFu }, { 0x0001B4u, 0x0001B3u }, { 0x0001B6u, 0x0001B5u }, { 0x0001B9u, 0x0001B8u }, { 0x0001BDu, 0x0001BCu }, { 0x0001BFu, 0x0001F7u }, { 0x0001C5u, 0x0001C4u }, { 0x0001C6u, 0x0001C4u }, { 0x0001C8u, 0x0001C7u },
    { 0x0001C9u, 0x0001C7u }, { 0x0001CBu, 0x0001CAu }, { 0x0001CCu, 0x0001CAu }, { 0x0001CEu, 0x0001CDu }, { 0x0001D0u, 0x0001CFu }, { 0x0001D2u, 0x0001D1u }, { 0x0001D4u, 0x0001D3u }, { 0x0001D6u, 0x0001D5u }, { 0x0001D8u, 0x0001D7u },
    { 0x0001DAu, 0x0001D9u }, { 0x0001DCu, 0x0001DBu }, { 0x0001DDu, 0x00018Eu }, { 0x0001DFu, 0x0001DEu }, { 0x0001E1u, 0x0001E0u }, { 0x0001E3u, 0x0001E2u }, { 0x0001E5u, 0x0001E4u }, { 0x0001E7u, 0x0001E6u }, { 0x0001E9u, 0x0001E8u },
    { 0x0001EBu, 0x0001EAu }, { 0x0001EDu, 0x0001ECu }, { 0x0001EFu, 0x0001EEu }, { 0x0001F2u, 0x0001F1u }, { 0x0001F3u, 0x0001F1u }, { 0x0001F5u, 0x0001F4u }, { 0x0001F9u, 0x0001F8u }, { 0x0001FBu, 0x0001FAu }, { 0x0001FDu, 0x0001FCu },
    { 0x0001FFu, 0x0001FEu }, { 0x000201u, 0x000200u }, { 0x000203u, 0x000202u }, { 0x000205u, 0x000204u }, { 0x000207u, 0x000206u }, { 0x000209u, 0x000208u }, { 0x00020Bu, 0x00020Au }, { 0x00020Du, 0x00020Cu }, { 0x00020Fu, 0x00020Eu },
    { 0x000211u, 0x000210u }, { 0x000213u, 0x000212u }, { 0x000215u, 0x000214u }, { 0x000217u, 0x000216u }, { 0x000219u, 0x000218u }, { 0x00021Bu, 0x00021Au }, { 0x00021Du, 0x00021Cu }, { 0x00021Fu, 0x00021Eu }, { 0x000223u, 0x000222u },
    { 0x000225u, 0x000224u }, { 0x000227u, 0x000226u }, { 0x000229u, 0x000228u }, { 0x00022Bu, 0x00022Au }, { 0x00022Du, 0x00022Cu }, { 0x00022Fu, 0x00022Eu }, { 0x000231u, 0x000230u }, { 0x000233u, 0x000232u }, { 0x00023Cu, 0x00023Bu },
    { 0x000242u, 0x000241u }, { 0x000247u, 0x000246u }, { 0x000249u, 0x000248u }, { 0x00024Bu, 0x00024Au }, { 0x00024Du, 0x00024Cu }, { 0x00024Fu, 0x00024Eu }, { 0x000250u, 0x002C6Fu }, { 0x000251u, 0x002C6Du }, { 0x000252u, 0x002C70u },
    { 0x000253u, 0x000181u }, { 0x000254u, 0x000186u }, { 0x000259u, 0x00018Fu }, { 0x00025Bu, 0x000190u }, { 0x00025Cu, 0x00A7ABu }, { 0x000260u, 0x000193u }, { 0x000261u, 0x00A7ACu }, { 0x000263u, 0x000194u }, { 0x000264u, 0x00A7CBu },
    { 0x000265u, 0x00A78Du }, { 0x000266u, 0x00A7AAu }, { 0x000268u, 0x000197u }, { 0x000269u, 0x000196u }, { 0x00026Au, 0x00A7AEu }, { 0x00026Bu, 0x002C62u }, { 0x00026Cu, 0x00A7ADu }, { 0x00026Fu, 0x00019Cu }, { 0x000271u, 0x002C6Eu },
    { 0x000272u, 0x00019Du }, { 0x000275u, 0x00019Fu }, { 0x00027Du, 0x002C64u }, { 0x000280u, 0x0001A6u }, { 0x000282u, 0x00A7C5u }, { 0x000283u, 0x0001A9u }, { 0x000287u, 0x00A7B1u }, { 0x000288u, 0x0001AEu }, { 0x000289u, 0x000244u },
    { 0x00028Cu, 0x000245u }, { 0x000292u, 0x0001B7u }, { 0x00029Du, 0x00A7B2u }, { 0x00029Eu, 0x00A7B0u }, { 0x000345u, 0x000399u }, { 0x000371u, 0x000370u }, { 0x000373u, 0x000372u }, { 0x000377u, 0x000376u }, { 0x0003ACu, 0x000386u },
    { 0x0003C2u, 0x0003A3u }, { 0x0003CCu, 0x00038Cu }, { 0x0003D0u, 0x000392u }, { 0x0003D1u, 0x000398u }, { 0x0003D5u, 0x0003A6u }, { 0x0003D6u, 0x0003A0u }, { 0x0003D7u, 0x0003CFu }, { 0x0003D9u, 0x0003D8u }, { 0x0003DBu, 0x0003DAu },
    { 0x0003DDu, 0x0003DCu }, { 0x0003DFu, 0x0003DEu }, { 0x0003E1u, 0x0003E0u }, { 0x0003E3u, 0x0003E2u }, { 0x0003E5u, 0x0003E4u }, { 0x0003E7u, 0x0003E6u }, { 0x0003E9u, 0x0003E8u }, { 0x0003EBu, 0x0003EAu }, { 0x0003EDu, 0x0003ECu },
    { 0x0003EFu, 0x0003EEu }, { 0x0003F0u, 0x00039Au }, { 0x0003F1u, 0x0003A1u }, { 0x0003F2u, 0x0003F9u }, { 0x0003F3u, 0x00037Fu }, { 0x0003F5u, 0x000395u }, { 0x0003F8u, 0x0003F7u }, { 0x0003FBu, 0x0003FAu }, { 0x000461u, 0x000460u },
    { 0x000463u, 0x000462u }, { 0x000465u, 0x000464u }, { 0x000467u, 0x000466u }, { 0x000469u, 0x000468u }, { 0x00046Bu, 0x00046Au }, { 0x00046Du, 0x00046Cu }, { 0x00046Fu, 0x00046Eu }, { 0x000471u, 0x000470u }, { 0x000473u, 0x000472u },
    { 0x000475u, 0x000474u }, { 0x000477u, 0x000476u }, { 0x000479u, 0x000478u }, { 0x00047Bu, 0x00047Au }, { 0x00047Du, 0x00047Cu }, { 0x00047Fu, 0x00047Eu }, { 0x000481u, 0x000480u }, { 0x00048Bu, 0x00048Au }, { 0x00048Du, 0x00048Cu },
    { 0x00048Fu, 0x00048Eu }, { 0x000491u, 0x000490u }, { 0x000493u, 0x000492u }, { 0x000495u, 0x000494u }, { 0x000497u, 0x000496u }, { 0x000499u, 0x000498u }, { 0x00049Bu, 0x00049Au }, { 0x00049Du, 0x00049Cu }, { 0x00049Fu, 0x00049Eu },
    { 0x0004A1u, 0x0004A0u }, { 0x0004A3u, 0x0004A2u }, { 0x0004A5u, 0x0004A4u }, { 0x0004A7u, 0x0004A6u }, { 0x0004A9u, 0x0004A8u }, { 0x0004ABu, 0x0004AAu }, { 0x0004ADu, 0x0004ACu }, { 0x0004AFu, 0x0004AEu }, { 0x0004B1u, 0x0004B0u },
    { 0x0004B3u, 0x0004B2u }, { 0x0004B5u, 0x0004B4u }, { 0x0004B7u, 0x0004B6u }, { 0x0004B9u, 0x0004B8u }, { 0x0004BBu, 0x0004BAu }, { 0x0004BDu, 0x0004BCu }, { 0x0004BFu, 0x0004BEu }, { 0x0004C2u, 0x0004C1u }, { 0x0004C4u, 0x0004C3u },
    { 0x0004C6u, 0x0004C5u }, { 0x0004C8u, 0x0004C7u }, { 0x0004CAu, 0x0004C9u }, { 0x0004CCu, 0x0004CBu }, { 0x0004CEu, 0x0004CDu }, { 0x0004CFu, 0x0004C0u }, { 0x0004D1u, 0x0004D0u }, { 0x0004D3u, 0x0004D2u }, { 0x0004D5u, 0x0004D4u },
    { 0x0004D7u, 0x0004D6u }, { 0x0004D9u, 0x0004D8u }, { 0x0004DBu, 0x0004DAu }, { 0x0004DDu, 0x0004DCu }, { 0x0004DFu, 0x0004DEu }, { 0x0004E1u, 0x0004E0u }, { 0x0004E3u, 0x0004E2u }, { 0x0004E5u, 0x0004E4u }, { 0x0004E7u, 0x0004E6u },
    { 0x0004E9u, 0x0004E8u }, { 0x0004EBu, 0x0004EAu }, { 0x0004EDu, 0x0004ECu }, { 0x0004EFu, 0x0004EEu }, { 0x0004F1u, 0x0004F0u }, { 0x0004F3u, 0x0004F2u }, { 0x0004F5u, 0x0004F4u }, { 0x0004F7u, 0x0004F6u }, { 0x0004F9u, 0x0004F8u },
    { 0x0004FBu, 0x0004FAu }, { 0x0004FDu, 0x0004FCu }, { 0x0004FFu, 0x0004FEu }, { 0x000501u, 0x000500u }, { 0x000503u, 0x000502u }, { 0x000505u, 0x000504u }, { 0x000507u, 0x000506u }, { 0x000509u, 0x000508u }, { 0x00050Bu, 0x00050Au },
    { 0x00050Du, 0x00050Cu }, { 0x00050Fu, 0x00050Eu }, { 0x000511u, 0x000510u }, { 0x000513u, 0x000512u }, { 0x000515u, 0x000514u }, { 0x000517u, 0x000516u }, { 0x000519u, 0x000518u }, { 0x00051Bu, 0x00051Au }, { 0x00051Du, 0x00051Cu },
    { 0x00051Fu, 0x00051Eu }, { 0x000521u, 0x000520u }, { 0x000523u, 0x000522u }, { 0x000525u, 0x000524u }, { 0x000527u, 0x000526u }, { 0x000529u, 0x000528u }, { 0x00052Bu, 0x00052Au }, { 0x00052Du, 0x00052Cu }, { 0x00052Fu, 0x00052Eu },
    { 0x001C80u, 0x000412u }, { 0x001C81u, 0x000414u }, { 0x001C82u, 0x00041Eu }, { 0x001C85u, 0x000422u }, { 0x001C86u, 0x00042Au }, { 0x001C87u, 0x000462u }, { 0x001C88u, 0x00A64Au }, { 0x001C8Au, 0x001C89u }, { 0x001D79u, 0x00A77Du },
    { 0x001D7Du, 0x002C63u }, { 0x001D8Eu, 0x00A7C6u }, { 0x001E01u, 0x001E00u }, { 0x001E03u, 0x001E02u }, { 0x001E05u, 0x001E04u }, { 0x001E07u, 0x001E06u }, { 0x001E09u, 0x001E08u }, { 0x001E0Bu, 0x001E0Au }, { 0x001E0Du, 0x001E0Cu },
    { 0x001E0Fu, 0x001E0Eu }, { 0x001E11u, 0x001E10u }, { 0x001E13u, 0x001E12u }, { 0x001E15u, 0x001E14u }, { 0x001E17u, 0x001E16u }, { 0x001E19u, 0x001E18u }, { 0x001E1Bu, 0x001E1Au }, { 0x001E1Du, 0x001E1Cu }, { 0x001E1Fu, 0x001E1Eu },
    { 0x001E21u, 0x001E20u }, { 0x001E23u, 0x001E22u }, { 0x001E25u, 0x001E24u }, { 0x001E27u, 0x001E26u }, { 0x001E29u, 0x001E28u }, { 0x001E2Bu, 0x001E2Au }, { 0x001E2Du, 0x001E2Cu }, { 0x001E2Fu, 0x001E2Eu }, { 0x001E31u, 0x001E30u },
    { 0x001E33u, 0x001E32u }, { 0x001E35u, 0x001E34u }, { 0x001E37u, 0x001E36u }, { 0x001E39u, 0x001E38u }, { 0x001E3Bu, 0x001E3Au }, { 0x001E3Du, 0x001E3Cu }, { 0x001E3Fu, 0x001E3Eu }, { 0x001E41u, 0x001E40u }, { 0x001E43u, 0x001E42u },
    { 0x001E45u, 0x001E44u }, { 0x001E47u, 0x001E46u }, { 0x001E49u, 0x001E48u }, { 0x001E4Bu, 0x001E4Au }, { 0x001E4Du, 0x001E4Cu }, { 0x001E4Fu, 0x001E4Eu }, { 0x001E51u, 0x001E50u }, { 0x001E53u, 0x001E52u }, { 0x001E55u, 0x001E54u },
    { 0x001E57u, 0x001E56u }, { 0x001E59u, 0x001E58u }, { 0x001E5Bu, 0x001E5Au }, { 0x001E5Du, 0x001E5Cu }, { 0x001E5Fu, 0x001E5Eu }, { 0x001E61u, 0x001E60u }, { 0x001E63u, 0x001E62u }, { 0x001E65u, 0x001E64u }, { 0x001E67u, 0x001E66u },
    { 0x001E69u, 0x001E68u }, { 0x001E6Bu, 0x001E6Au }, { 0x001E6Du, 0x001E6Cu }, { 0x001E6Fu, 0x001E6Eu }, { 0x001E71u, 0x001E70u }, { 0x001E73u, 0x001E72u }, { 0x001E75u, 0x001E74u }, { 0x001E77u, 0x001E76u }, { 0x001E79u, 0x001E78u },
    { 0x001E7Bu, 0x001E7Au }, { 0x001E7Du, 0x001E7Cu }, { 0x001E7Fu, 0x001E7Eu }, { 0x001E81u, 0x001E80u }, { 0x001E83u, 0x001E82u }, { 0x001E85u, 0x001E84u }, { 0x001E87u, 0x001E86u }, { 0x001E89u, 0x001E88u }, { 0x001E8Bu, 0x001E8Au },
    { 0x001E8Du, 0x001E8Cu }, { 0x001E8Fu, 0x001E8Eu }, { 0x001E91u, 0x001E90u }, { 0x001E93u, 0x001E92u }, { 0x001E95u, 0x001E94u }, { 0x001E9Bu, 0x001E60u }, { 0x001EA1u, 0x001EA0u }, { 0x001EA3u, 0x001EA2u }, { 0x001EA5u, 0x001EA4u },
    { 0x001EA7u, 0x001EA6u }, { 0x001EA9u, 0x001EA8u }, { 0x001EABu, 0x001EAAu }, { 0x001EADu, 0x001EACu }, { 0x001EAFu, 0x001EAEu }, { 0x001EB1u, 0x001EB0u }, { 0x001EB3u, 0x001EB2u }, { 0x001EB5u, 0x001EB4u }, { 0x001EB7u, 0x001EB6u },
    { 0x001EB9u, 0x001EB8u }, { 0x001EBBu, 0x001EBAu }, { 0x001EBDu, 0x001EBCu }, { 0x001EBFu, 0x001EBEu }, { 0x001EC1u, 0x001EC0u }, { 0x001EC3u, 0x001EC2u }, { 0x001EC5u, 0x001EC4u }, { 0x001EC7u, 0x001EC6u }, { 0x001EC9u, 0x001EC8u },
    { 0x001ECBu, 0x001ECAu }, { 0x001ECDu, 0x001ECCu }, { 0x001ECFu, 0x001ECEu }, { 0x001ED1u, 0x001ED0u }, { 0x001ED3u, 0x001ED2u }, { 0x001ED5u, 0x001ED4u }, { 0x001ED7u, 0x001ED6u }, { 0x001ED9u, 0x001ED8u }, { 0x001EDBu, 0x001EDAu },
    { 0x001EDDu, 0x001EDCu }, { 0x001EDFu, 0x001EDEu }, { 0x001EE1u, 0x001EE0u }, { 0x001EE3u, 0x001EE2u }, { 0x001EE5u, 0x001EE4u }, { 0x001EE7u, 0x001EE6u }, { 0x001EE9u, 0x001EE8u }, { 0x001EEBu, 0x001EEAu }, { 0x001EEDu, 0x001EECu },
    { 0x001EEFu, 0x001EEEu }, { 0x001EF1u, 0x001EF0u }, { 0x001EF3u, 0x001EF2u }, { 0x001EF5u, 0x001EF4u }, { 0x001EF7u, 0x001EF6u }, { 0x001EF9u, 0x001EF8u }, { 0x001EFBu, 0x001EFAu }, { 0x001EFDu, 0x001EFCu }, { 0x001EFFu, 0x001EFEu },
    { 0x001F51u, 0x001F59u }, { 0x001F53u, 0x001F5Bu }, { 0x001F55u, 0x001F5Du }, { 0x001F57u, 0x001F5Fu }, { 0x001FB3u, 0x001FBCu }, { 0x001FBEu, 0x000399u }, { 0x001FC3u, 0x001FCCu }, { 0x001FE5u, 0x001FECu }, { 0x001FF3u, 0x001FFCu },
    { 0x00214Eu, 0x002132u }, { 0x002184u, 0x002183u }, { 0x002C61u, 0x002C60u }, { 0x002C65u, 0x00023Au }, { 0x002C66u, 0x00023Eu }, { 0x002C68u, 0x002C67u }, { 0x002C6Au, 0x002C69u }, { 0x002C6Cu, 0x002C6Bu }, { 0x002C73u, 0x002C72u },
    { 0x002C76u, 0x002C75u }, { 0x002C81u, 0x002C80u }, { 0x002C83u, 0x002C82u }, { 0x002C85u, 0x002C84u }, { 0x002C87u, 0x002C86u }, { 0x002C89u, 0x002C88u }, { 0x002C8Bu, 0x002C8Au }, { 0x002C8Du, 0x002C8Cu }, { 0x002C8Fu, 0x002C8Eu },
    { 0x002C91u, 0x002C90u }, { 0x002C93u, 0x002C92u }, { 0x002C95u, 0x002C94u }, { 0x002C97u, 0x002C96u }, { 0x002C99u, 0x002C98u }, { 0x002C9Bu, 0x002C9Au }, { 0x002C9Du, 0x002C9Cu }, { 0x002C9Fu, 0x002C9Eu }, { 0x002CA1u, 0x002CA0u },
    { 0x002CA3u, 0x002CA2u }, { 0x002CA5u, 0x002CA4u }, { 0x002CA7u, 0x002CA6u }, { 0x002CA9u, 0x002CA8u }, { 0x002CABu, 0x002CAAu }, { 0x002CADu, 0x002CACu }, { 0x002CAFu, 0x002CAEu }, { 0x002CB1u, 0x002CB0u }, { 0x002CB3u, 0x002CB2u },
    { 0x002CB5u, 0x002CB4u }, { 0x002CB7u, 0x002CB6u }, { 0x002CB9u, 0x002CB8u }, { 0x002CBBu, 0x002CBAu }, { 0x002CBDu, 0x002CBCu }, { 0x002CBFu, 0x002CBEu }, { 0x002CC1u, 0x002CC0u }, { 0x002CC3u, 0x002CC2u }, { 0x002CC5u, 0x002CC4u },
    { 0x002CC7u, 0x002CC6u }, { 0x002CC9u, 0x002CC8u }, { 0x002CCBu, 0x002CCAu }, { 0x002CCDu, 0x002CCCu }, { 0x002CCFu, 0x002CCEu }, { 0x002CD1u, 0x002CD0u }, { 0x002CD3u, 0x002CD2u }, { 0x002CD5u, 0x002CD4u }, { 0x002CD7u, 0x002CD6u },
    { 0x002CD9u, 0x002CD8u }, { 0x002CDBu, 0x002CDAu }, { 0x002CDDu, 0x002CDCu }, { 0x002CDFu, 0x002CDEu }, { 0x002CE1u, 0x002CE0u }, { 0x002CE3u, 0x002CE2u }, { 0x002CECu, 0x002CEBu }, { 0x002CEEu, 0x002CEDu }, { 0x002CF3u, 0x002CF2u },
    { 0x002D27u, 0x0010C7u }, { 0x002D2Du, 0x0010CDu }, { 0x00A641u, 0x00A640u }, { 0x00A643u, 0x00A642u }, { 0x00A645u, 0x00A644u }, { 0x00A647u, 0x00A646u }, { 0x00A649u, 0x00A648u }, { 0x00A64Bu, 0x00A64Au }, { 0x00A64Du, 0x00A64Cu },
    { 0x00A64Fu, 0x00A64Eu }, { 0x00A651u, 0x00A650u }, { 0x00A653u, 0x00A652u }, { 0x00A655u, 0x00A654u }, { 0x00A657u, 0x00A656u }, { 0x00A659u, 0x00A658u }, { 0x00A65Bu, 0x00A65Au }, { 0x00A65Du, 0x00A65Cu }, { 0x00A65Fu, 0x00A65Eu },
    { 0x00A661u, 0x00A660u }, { 0x00A663u, 0x00A662u }, { 0x00A665u, 0x00A664u }, { 0x00A667u, 0x00A666u }, { 0x00A669u, 0x00A668u }, { 0x00A66Bu, 0x00A66Au }, { 0x00A66Du, 0x00A66Cu }, { 0x00A681u, 0x00A680u }, { 0x00A683u, 0x00A682u },
    { 0x00A685u, 0x00A684u }, { 0x00A687u, 0x00A686u }, { 0x00A689u, 0x00A688u }, { 0x00A68Bu, 0x00A68Au }, { 0x00A68Du, 0x00A68Cu }, { 0x00A68Fu, 0x00A68Eu }, { 0x00A691u, 0x00A690u }, { 0x00A693u, 0x00A692u }, { 0x00A695u, 0x00A694u },
    { 0x00A697u, 0x00A696u }, { 0x00A699u, 0x00A698u }, { 0x00A69Bu, 0x00A69Au }, { 0x00A723u, 0x00A722u }, { 0x00A725u, 0x00A724u }, { 0x00A727u, 0x00A726u }, { 0x00A729u, 0x00A728u }, { 0x00A72Bu, 0x00A72Au }, { 0x00A72Du, 0x00A72Cu },
    { 0x00A72Fu, 0x00A72Eu }, { 0x00A733u, 0x00A732u }, { 0x00A735u, 0x00A734u }, { 0x00A737u, 0x00A736u }, { 0x00A739u, 0x00A738u }, { 0x00A73Bu, 0x00A73Au }, { 0x00A73Du, 0x00A73Cu }, { 0x00A73Fu, 0x00A73Eu }, { 0x00A741u, 0x00A740u },
    { 0x00A743u, 0x00A742u }, { 0x00A745u, 0x00A744u }, { 0x00A747u, 0x00A746u }, { 0x00A749u, 0x00A748u }, { 0x00A74Bu, 0x00A74Au }, { 0x00A74Du, 0x00A74Cu }, { 0x00A74Fu, 0x00A74Eu }, { 0x00A751u, 0x00A750u }, { 0x00A753u, 0x00A752u },
    { 0x00A755u, 0x00A754u }, { 0x00A757u, 0x00A756u }, { 0x00A759u, 0x00A758u }, { 0x00A75Bu, 0x00A75Au }, { 0x00A75Du, 0x00A75Cu }, { 0x00A75Fu, 0x00A75Eu }, { 0x00A761u, 0x00A760u }, { 0x00A763u, 0x00A762u }, { 0x00A765u, 0x00A764u },
    { 0x00A767u, 0x00A766u }, { 0x00A769u, 0x00A768u }, { 0x00A76Bu, 0x00A76Au }, { 0x00A76Du, 0x00A76Cu }, { 0x00A76Fu, 0x00A76Eu }, { 0x00A77Au, 0x00A779u }, { 0x00A77Cu, 0x00A77Bu }, { 0x00A77Fu, 0x00A77Eu }, { 0x00A781u, 0x00A780u },
    { 0x00A783u, 0x00A782u }, { 0x00A785u, 0x00A784u }, { 0x00A787u, 0x00A786u }, { 0x00A78Cu, 0x00A78Bu }, { 0x00A791u, 0x00A790u }, { 0x00A793u, 0x00A792u }, { 0x00A794u, 0x00A7C4u }, { 0x00A797u, 0x00A796u }, { 0x00A799u, 0x00A798u },
    { 0x00A79Bu, 0x00A79Au }, { 0x00A79Du, 0x00A79Cu }, { 0x00A79Fu, 0x00A79Eu }, { 0x00A7A1u, 0x00A7A0u }, { 0x00A7A3u, 0x00A7A2u }, { 0x00A7A5u, 0x00A7A4u }, { 0x00A7A7u, 0x00A7A6u }, { 0x00A7A9u, 0x00A7A8u }, { 0x00A7B5u, 0x00A7B4u },
    { 0x00A7B7u, 0x00A7B6u }, { 0x00A7B9u, 0x00A7B8u }, { 0x00A7BBu, 0x00A7BAu }, { 0x00A7BDu, 0x00A7BCu }, { 0x00A7BFu, 0x00A7BEu }, { 0x00A7C1u, 0x00A7C0u }, { 0x00A7C3u, 0x00A7C2u }, { 0x00A7C8u, 0x00A7C7u }, { 0x00A7CAu, 0x00A7C9u },
    { 0x00A7CDu, 0x00A7CCu }, { 0x00A7CFu, 0x00A7CEu }, { 0x00A7D1u, 0x00A7D0u }, { 0x00A7D3u, 0x00A7D2u }, { 0x00A7D5u, 0x00A7D4u }, { 0x00A7D7u, 0x00A7D6u }, { 0x00A7D9u, 0x00A7D8u }, { 0x00A7DBu, 0x00A7DAu }, { 0x00A7F6u, 0x00A7F5u },
    { 0x00AB53u, 0x00A7B3u },
};


static struct { CSTR_U32 cp; CSTR_U32 offset; CSTR_U32 count; } const internal_cstr_utf8_to_upper_multi[] = {
    { 0x0000DFu, 0, 2 }, { 0x000149u, 30, 2 }, { 0x0001F0u, 38, 2 }, { 0x000390u, 32, 3 }, { 0x0003B0u, 35, 3 }, { 0x000587u, 18, 2 }, { 0x001E96u, 40, 2 }, { 0x001E97u, 42, 2 }, { 0x001E98u, 44, 2 }, { 0x001E99u, 46, 2 }, { 0x001E9Au, 48, 2 },
    { 0x001F50u, 50, 2 }, { 0x001F52u, 52, 3 }, { 0x001F54u, 55, 3 }, { 0x001F56u, 58, 3 }, { 0x001F80u, 91, 2 }, { 0x001F81u, 93, 2 }, { 0x001F82u, 95, 2 }, { 0x001F83u, 97, 2 }, { 0x001F84u, 99, 2 }, { 0x001F85u, 101, 2 }, { 0x001F86u, 103, 2 },
    { 0x001F87u, 105, 2 }, { 0x001F88u, 107, 2 }, { 0x001F89u, 109, 2 }, { 0x001F8Au, 111, 2 }, { 0x001F8Bu, 113, 2 }, { 0x001F8Cu, 115, 2 }, { 0x001F8Du, 117, 2 }, { 0x001F8Eu, 119, 2 }, { 0x001F8Fu, 121, 2 }, { 0x001F90u, 123, 2 },
    { 0x001F91u, 125, 2 }, { 0x001F92u, 127, 2 }, { 0x001F93u, 129, 2 }, { 0x001F94u, 131, 2 }, { 0x001F95u, 133, 2 }, { 0x001F96u, 135, 2 }, { 0x001F97u, 137, 2 }, { 0x001F98u, 139, 2 }, { 0x001F99u, 141, 2 }, { 0x001F9Au, 143, 2 },
    { 0x001F9Bu, 145, 2 }, { 0x001F9Cu, 147, 2 }, { 0x001F9Du, 149, 2 }, { 0x001F9Eu, 151, 2 }, { 0x001F9Fu, 153, 2 }, { 0x001FA0u, 155, 2 }, { 0x001FA1u, 157, 2 }, { 0x001FA2u, 159, 2 }, { 0x001FA3u, 161, 2 }, { 0x001FA4u, 163, 2 },
    { 0x001FA5u, 165, 2 }, { 0x001FA6u, 167, 2 }, { 0x001FA7u, 169, 2 }, { 0x001FA8u, 171, 2 }, { 0x001FA9u, 173, 2 }, { 0x001FAAu, 175, 2 }, { 0x001FABu, 177, 2 }, { 0x001FACu, 179, 2 }, { 0x001FADu, 181, 2 }, { 0x001FAEu, 183, 2 },
    { 0x001FAFu, 185, 2 }, { 0x001FB2u, 199, 2 }, { 0x001FB3u, 187, 2 }, { 0x001FB4u, 201, 2 }, { 0x001FB6u, 61, 2 }, { 0x001FB7u, 211, 3 }, { 0x001FBCu, 189, 2 }, { 0x001FC2u, 203, 2 }, { 0x001FC3u, 191, 2 }, { 0x001FC4u, 205, 2 },
    { 0x001FC6u, 63, 2 }, { 0x001FC7u, 214, 3 }, { 0x001FCCu, 193, 2 }, { 0x001FD2u, 65, 3 }, { 0x001FD3u, 68, 3 }, { 0x001FD6u, 71, 2 }, { 0x001FD7u, 73, 3 }, { 0x001FE2u, 76, 3 }, { 0x001FE3u, 79, 3 }, { 0x001FE4u, 82, 2 }, { 0x001FE6u, 84, 2 },
    { 0x001FE7u, 86, 3 }, { 0x001FF2u, 207, 2 }, { 0x001FF3u, 195, 2 }, { 0x001FF4u, 209, 2 }, { 0x001FF6u, 89, 2 }, { 0x001FF7u, 217, 3 }, { 0x001FFCu, 197, 2 }, { 0x00FB00u, 2, 2 }, { 0x00FB01u, 4, 2 }, { 0x00FB02u, 6, 2 }, { 0x00FB03u, 8, 3 },
    { 0x00FB04u, 11, 3 }, { 0x00FB05u, 14, 2 }, { 0x00FB06u, 16, 2 }, { 0x00FB13u, 20, 2 }, { 0x00FB14u, 22, 2 }, { 0x00FB15u, 24, 2 }, { 0x00FB16u, 26, 2 }, { 0x00FB17u, 28, 2 },
};


static CSTR_U32 const internal_cstr_utf8_to_upper_pool[] = {
    0x000053u, 0x000053u, 0x000046u, 0x000046u, 0x000046u, 0x000049u, 0x000046u, 0x00004Cu, 0x000046u, 0x000046u, 0x000049u, 0x000046u, 0x000046u, 0x00004Cu, 0x000053u, 0x000054u, 0x000053u, 0x000054u, 0x000535u, 0x000552u, 0x000544u, 0x000546u,
    0x000544u, 0x000535u, 0x000544u, 0x00053Bu, 0x00054Eu, 0x000546u, 0x000544u, 0x00053Du, 0x0002BCu, 0x00004Eu, 0x000399u, 0x000308u, 0x000301u, 0x0003A5u, 0x000308u, 0x000301u, 0x00004Au, 0x00030Cu, 0x000048u, 0x000331u, 0x000054u, 0x000308u,
    0x000057u, 0x00030Au, 0x000059u, 0x00030Au, 0x000041u, 0x0002BEu, 0x0003A5u, 0x000313u, 0x0003A5u, 0x000313u, 0x000300u, 0x0003A5u, 0x000313u, 0x000301u, 0x0003A5u, 0x000313u, 0x000342u, 0x000391u, 0x000342u, 0x000397u, 0x000342u, 0x000399u,
    0x000308u, 0x000300u, 0x000399u, 0x000308u, 0x000301u, 0x000399u, 0x000342u, 0x000399u, 0x000308u, 0x000342u, 0x0003A5u, 0x000308u, 0x000300u, 0x0003A5u, 0x000308u, 0x000301u, 0x0003A1u, 0x000313u, 0x0003A5u, 0x000342u, 0x0003A5u, 0x000308u,
    0x000342u, 0x0003A9u, 0x000342u, 0x001F08u, 0x000399u, 0x001F09u, 0x000399u, 0x001F0Au, 0x000399u, 0x001F0Bu, 0x000399u, 0x001F0Cu, 0x000399u, 0x001F0Du, 0x000399u, 0x001F0Eu, 0x000399u, 0x001F0Fu, 0x000399u, 0x001F08u, 0x000399u, 0x001F09u,
    0x000399u, 0x001F0Au, 0x000399u, 0x001F0Bu, 0x000399u, 0x001F0Cu, 0x000399u, 0x001F0Du, 0x000399u, 0x001F0Eu, 0x000399u, 0x001F0Fu, 0x000399u, 0x001F28u, 0x000399u, 0x001F29u, 0x000399u, 0x001F2Au, 0x000399u, 0x001F2Bu, 0x000399u, 0x001F2Cu,
    0x000399u, 0x001F2Du, 0x000399u, 0x001F2Eu, 0x000399u, 0x001F2Fu, 0x000399u, 0x001F28u, 0x000399u, 0x001F29u, 0x000399u, 0x001F2Au, 0x000399u, 0x001F2Bu, 0x000399u, 0x001F2Cu, 0x000399u, 0x001F2Du, 0x000399u, 0x001F2Eu, 0x000399u, 0x001F2Fu,
    0x000399u, 0x001F68u, 0x000399u, 0x001F69u, 0x000399u, 0x001F6Au, 0x000399u, 0x001F6Bu, 0x000399u, 0x001F6Cu, 0x000399u, 0x001F6Du, 0x000399u, 0x001F6Eu, 0x000399u, 0x001F6Fu, 0x000399u, 0x001F68u, 0x000399u, 0x001F69u, 0x000399u, 0x001F6Au,
    0x000399u, 0x001F6Bu, 0x000399u, 0x001F6Cu, 0x000399u, 0x001F6Du, 0x000399u, 0x001F6Eu, 0x000399u, 0x001F6Fu, 0x000399u, 0x000391u, 0x000399u, 0x000391u, 0x000399u, 0x000397u, 0x000399u, 0x000397u, 0x000399u, 0x0003A9u, 0x000399u, 0x0003A9u,
    0x000399u, 0x001FBAu, 0x000399u, 0x000386u, 0x000399u, 0x001FCAu, 0x000399u, 0x000389u, 0x000399u, 0x001FFAu, 0x000399u, 0x00038Fu, 0x000399u, 0x000391u, 0x000342u, 0x000399u, 0x000397u, 0x000342u, 0x000399u, 0x0003A9u, 0x000342u, 0x000399u,
};


static struct { CSTR_U32 lo; CSTR_U32 hi; int delta; } const internal_cstr_utf8_to_lower_ranges[] = {
    { 0x000041u, 0x00005Au, 32 }, { 0x0000C0u, 0x0000D6u, 32 }, { 0x0000D8u, 0x0000DEu, 32 }, { 0x000189u, 0x00018Au, 205 }, { 0x0001B1u, 0x0001B2u, 217 }, { 0x000388u, 0x00038Au, 37 }, { 0x00038Eu, 0x00038Fu, 63 }, { 0x000391u, 0x0003A1u, 32 },
    { 0x0003A3u, 0x0003ABu, 32 }, { 0x0003FDu, 0x0003FFu, -130 }, { 0x000400u, 0x00040Fu, 80 }, { 0x000410u, 0x00042Fu, 32 }, { 0x000531u, 0x000556u, 48 }, { 0x0010A0u, 0x0010C5u, 7264 }, { 0x0013A0u, 0x0013EFu, 38864 },
    { 0x0013F0u, 0x0013F5u, 8 }, { 0x001C90u, 0x001CBAu, -3008 }, { 0x001CBDu, 0x001CBFu, -3008 }, { 0x001F08u, 0x001F0Fu, -8 }, { 0x001F18u, 0x001F1Du, -8 }, { 0x001F28u, 0x001F2Fu, -8 }, { 0x001F38u, 0x001F3Fu, -8 }, { 0x001F48u, 0x001F4Du, -8 },
    { 0x001F68u, 0x001F6Fu, -8 }, { 0x001F88u, 0x001F8Fu, -8 }, { 0x001F98u, 0x001F9Fu, -8 }, { 0x001FA8u, 0x001FAFu, -8 }, { 0x001FB8u, 0x001FB9u, -8 }, { 0x001FBAu, 0x001FBBu, -74 }, { 0x001FC8u, 0x001FCBu, -86 }, { 0x001FD8u, 0x001FD9u, -8 },
    { 0x001FDAu, 0x001FDBu, -100 }, { 0x001FE8u, 0x001FE9u, -8 }, { 0x001FEAu, 0x001FEBu, -112 }, { 0x001FF8u, 0x001FF9u, -128 }, { 0x001FFAu, 0x001FFBu, -126 }, { 0x002160u, 0x00216Fu, 16 }, { 0x0024B6u, 0x0024CFu, 26 },
    { 0x002C00u, 0x002C2Fu, 48 }, { 0x002C7Eu, 0x002C7Fu, -10815 }, { 0x00FF21u, 0x00FF3Au, 32 }, { 0x010400u, 0x010427u, 40 }, { 0x0104B0u, 0x0104D3u, 40 }, { 0x010570u, 0x01057Au, 39 }, { 0x01057Cu, 0x01058Au, 39 }, { 0x01058Cu, 0x010592u, 39 },
    { 0x010594u, 0x010595u, 39 }, { 0x010C80u, 0x010CB2u, 64 }, { 0x010D50u, 0x010D65u, 32 }, { 0x0118A0u, 0x0118BFu, 32 }, { 0x016E40u, 0x016E5Fu, 32 }, { 0x016EA0u, 0x016EB8u, 27 }, { 0x01E900u, 0x01E921u, 34 },
};


static struct { CSTR_U32 cp; CSTR_U32 to; } const internal_cstr_utf8_to_lower_pairs[] = {
    { 0x000100u, 0x000101u }, { 0x000102u, 0x000103u }, { 0x000104u, 0x000105u }, { 0x000106u, 0x000107u }, { 0x000108u, 0x000109u }, { 0x00010Au, 0x00010Bu }, { 0x00010Cu, 0x00010Du }, { 0x00010Eu, 0x00010Fu }, { 0x000110u, 0x000111u },
    { 0x000112u, 0x000113u }, { 0x000114u, 0x000115u }, { 0x000116u, 0x000117u }, { 0x000118u, 0x000119u }, { 0x00011Au, 0x00011Bu }, { 0x00011Cu, 0x00011Du }, { 0x00011Eu, 0x00011Fu }, { 0x000120u, 0x000121u }, { 0x000122u, 0x000123u },
    { 0x000124u, 0x000125u }, { 0x000126u, 0x000127u }, { 0x000128u, 0x000129u }, { 0x00012Au, 0x00012Bu }, { 0x00012Cu, 0x00012Du }, { 0x00012Eu, 0x00012Fu }, { 0x000130u, 0x000069u }, { 0x000132u, 0x000133u }, { 0x000134u, 0x000135u },
    { 0x000136u, 0x000137u }, { 0x000139u, 0x00013Au }, { 0x00013Bu, 0x00013Cu }, { 0x00013Du, 0x00013Eu }, { 0x00013Fu, 0x000140u }, { 0x000141u, 0x000142u }, { 0x000143u, 0x000144u }, { 0x000145u, 0x000146u }, { 0x000147u, 0x000148u },
    { 0x00014Au, 0x00014Bu }, { 0x00014Cu, 0x00014Du }, { 0x00014Eu, 0x00014Fu }, { 0x000150u, 0x000151u }, { 0x000152u, 0x000153u }, { 0x000154u, 0x000155u }, { 0x000156u, 0x000157u }, { 0x000158u, 0x000159u }, { 0x00015Au, 0x00015Bu },
    { 0x00015Cu, 0x00015Du }, { 0x00015Eu, 0x00015Fu }, { 0x000160u, 0x000161u }, { 0x000162u, 0x000163u }, { 0x000164u, 0x000165u }, { 0x000166u, 0x000167u }, { 0x000168u, 0x000169u }, { 0x00016Au, 0x00016Bu }, { 0x00016Cu, 0x00016Du },
    { 0x00016Eu, 0x00016Fu }, { 0x000170u, 0x000171u }, { 0x000172u, 0x000173u }, { 0x000174u, 0x000175u }, { 0x000176u, 0x000177u }, { 0x000178u, 0x0000FFu }, { 0x000179u, 0x00017Au }, { 0x00017Bu, 0x00017Cu }, { 0x00017Du, 0x00017Eu },
    { 0x000181u, 0x000253u }, { 0x000182u, 0x000183u }, { 0x000184u, 0x000185u }, { 0x000186u, 0x000254u }, { 0x000187u, 0x000188u }, { 0x00018Bu, 0x00018Cu }, { 0x00018Eu, 0x0001DDu }, { 0x00018Fu, 0x000259u }, { 0x000190u, 0x00025Bu },
    { 0x000191u, 0x000192u }, { 0x000193u, 0x000260u }, { 0x000194u, 0x000263u }, { 0x000196u, 0x000269u }, { 0x000197u, 0x000268u }, { 0x000198u, 0x000199u }, { 0x00019Cu, 0x00026Fu }, { 0x00019Du, 0x000272u }, { 0x00019Fu, 0x000275u },
    { 0x0001A0u, 0x0001A1u }, { 0x0001A2u, 0x0001A3u }, { 0x0001A4u, 0x0001A5u }, { 0x0001A6u, 0x000280u }, { 0x0001A7u, 0x0001A8u }, { 0x0001A9u, 0x000283u }, { 0x0001ACu, 0x0001ADu }, { 0x0001AEu, 0x000288u }, { 0x0001AFu, 0x0001B0u },
    { 0x0001B3u, 0x0001B4u }, { 0x0001B5u, 0x0001B6u }, { 0x0001B7u, 0x000292u }, { 0x0001B8u, 0x0001B9u }, { 0x0001BCu, 0x0001BDu }, { 0x0001C4u, 0x0001C6u }, { 0x0001C5u, 0x0001C6u }, { 0x0001C7u, 0x0001C9u }, { 0x0001C8u, 0x0001C9u },
    { 0x0001CAu, 0x0001CCu }, { 0x0001CBu, 0x0001CCu }, { 0x0001CDu, 0x0001CEu }, { 0x0001CFu, 0x0001D0u }, { 0x0001D1u, 0x0001D2u }, { 0x0001D3u, 0x0001D4u }, { 0x0001D5u, 0x0001D6u }, { 0x0001D7u, 0x0001D8u }, { 0x0001D9u, 0x0001DAu },
    { 0x0001DBu, 0x0001DCu }, { 0x0001DEu, 0x0001DFu }, { 0x0001E0u, 0x0001E1u }, { 0x0001E2u, 0x0001E3u }, { 0x0001E4u, 0x0001E5u }, { 0x0001E6u, 0x0001E7u }, { 0x0001E8u, 0x0001E9u }, { 0x0001EAu, 0x0001EBu }, { 0x0001ECu, 0x0001EDu },
    { 0x0001EEu, 0x0001EFu }, { 0x0001F1u, 0x0001F3u }, { 0x0001F2u, 0x0001F3u }, { 0x0001F4u, 0x0001F5u }, { 0x0001F6u, 0x000195u }, { 0x0001F7u, 0x0001BFu }, { 0x0001F8u, 0x0001F9u }, { 0x0001FAu, 0x0001FBu }, { 0x0001FCu, 0x0001FDu },
    { 0x0001FEu, 0x0001FFu }, { 0x000200u, 0x000201u }, { 0x000202u, 0x000203u }, { 0x000204u, 0x000205u }, { 0x000206u, 0x000207u }, { 0x000208u, 0x000209u }, { 0x00020Au, 0x00020Bu }, { 0x00020Cu, 0x00020Du }, { 0x00020Eu, 0x00020Fu },
    { 0x000210u, 0x000211u }, { 0x000212u, 0x000213u }, { 0x000214u, 0x000215u }, { 0x000216u, 0x000217u }, { 0x000218u, 0x000219u }, { 0x00021Au, 0x00021Bu }, { 0x00021Cu, 0x00021Du }, { 0x00021Eu, 0x00021Fu }, { 0x000220u, 0x00019Eu },
    { 0x000222u, 0x000223u }, { 0x000224u, 0x000225u }, { 0x000226u, 0x000227u }, { 0x000228u, 0x000229u }, { 0x00022Au, 0x00022Bu }, { 0x00022Cu, 0x00022Du }, { 0x00022Eu, 0x00022Fu }, { 0x000230u, 0x000231u }, { 0x000232u, 0x000233u },
    { 0x00023Au, 0x002C65u }, { 0x00023Bu, 0x00023Cu }, { 0x00023Du, 0x00019Au }, { 0x00023Eu, 0x002C66u }, { 0x000241u, 0x000242u }, { 0x000243u, 0x000180u }, { 0x000244u, 0x000289u }, { 0x000245u, 0x00028Cu }, { 0x000246u, 0x000247u },
    { 0x000248u, 0x000249u }, { 0x00024Au, 0x00024Bu }, { 0x00024Cu, 0x00024Du }, { 0x00024Eu, 0x00024Fu }, { 0x000370u, 0x000371u }, { 0x000372u, 0x000373u }, { 0x000376u, 0x000377u }, { 0x00037Fu, 0x0003F3u }, { 0x000386u, 0x0003ACu },
    { 0x00038Cu, 0x0003CCu }, { 0x0003CFu, 0x0003D7u }, { 0x0003D8u, 0x0003D9u }, { 0x0003DAu, 0x0003DBu }, { 0x0003DCu, 0x0003DDu }, { 0x0003DEu, 0x0003DFu }, { 0x0003E0u, 0x0003E1u }, { 0x0003E2u, 0x0003E3u }, { 0x0003E4u, 0x0003E5u },
    { 0x0003E6u, 0x0003E7u }, { 0x0003E8u, 0x0003E9u }, { 0x0003EAu, 0x0003EBu }, { 0x0003ECu, 0x0003EDu }, { 0x0003EEu, 0x0003EFu }, { 0x0003F4u, 0x0003B8u }, { 0x0003F7u, 0x0003F8u }, { 0x0003F9u, 0x0003F2u }, { 0x0003FAu, 0x0003FBu },
    { 0x000460u, 0x000461u }, { 0x000462u, 0x000463u }, { 0x000464u, 0x000465u }, { 0x000466u, 0x000467u }, { 0x000468u, 0x000469u }, { 0x00046Au, 0x00046Bu }, { 0x00046Cu, 0x00046Du }, { 0x00046Eu, 0x00046Fu }, { 0x000470u, 0x000471u },
    { 0x000472u, 0x000473u }, { 0x000474u, 0x000475u }, { 0x000476u, 0x000477u }, { 0x000478u, 0x000479u }, { 0x00047Au, 0x00047Bu }, { 0x00047Cu, 0x00047Du }, { 0x00047Eu, 0x00047Fu }, { 0x000480u, 0x000481u }, { 0x00048Au, 0x00048Bu },
    { 0x00048Cu, 0x00048Du }, { 0x00048Eu, 0x00048Fu }, { 0x000490u, 0x000491u }, { 0x000492u, 0x000493u }, { 0x000494u, 0x000495u }, { 0x000496u, 0x000497u }, { 0x000498u, 0x000499u }, { 0x00049Au, 0x00049Bu }, { 0x00049Cu, 0x00049Du },
    { 0x00049Eu, 0x00049Fu }, { 0x0004A0u, 0x0004A1u }, { 0x0004A2u, 0x0004A3u }, { 0x0004A4u, 0x0004A5u }, { 0x0004A6u, 0x0004A7u }, { 0x0004A8u, 0x0004A9u }, { 0x0004AAu, 0x0004ABu }, { 0x0004ACu, 0x0004ADu }, { 0x0004AEu, 0x0004AFu },
    { 0x0004B0u, 0x0004B1u }, { 0x0004B2u, 0x0004B3u }, { 0x0004B4u, 0x0004B5u }, { 0x0004B6u, 0x0004B7u }, { 0x0004B8u, 0x0004B9u }, { 0x0004BAu, 0x0004BBu }, { 0x0004BCu, 0x0004BDu }, { 0x0004BEu, 0x0004BFu }, { 0x0004C0u, 0x0004CFu },
    { 0x0004C1u, 0x0004C2u }, { 0x0004C3u, 0x0004C4u }, { 0x0004C5u, 0x0004C6u }, { 0x0004C7u, 0x0004C8u }, { 0x0004C9u, 0x0004CAu }, { 0x0004CBu, 0x0004CCu }, { 0x0004CDu, 0x0004CEu }, { 0x0004D0u, 0x0004D1u }, { 0x0004D2u, 0x0004D3u },
    { 0x0004D4u, 0x0004D5u }, { 0x0004D6u, 0x0004D7u }, { 0x0004D8u, 0x0004D9u }, { 0x0004DAu, 0x0004DBu }, { 0x0004DCu, 0x0004DDu }, { 0x0004DEu, 0x0004DFu }, { 0x0004E0u, 0x0004E1u }, { 0x0004E2u, 0x0004E3u }, { 0x0004E4u, 0x0004E5u },
    { 0x0004E6u, 0x0004E7u }, { 0x0004E8u, 0x0004E9u }, { 0x0004EAu, 0x0004EBu }, { 0x0004ECu, 0x0004EDu }, { 0x0004EEu, 0x0004EFu }, { 0x0004F0u, 0x0004F1u }, { 0x0004F2u, 0x0004F3u }, { 0x0004F4u, 0x0004F5u }, { 0x0004F6u, 0x0004F7u },
    { 0x0004F8u, 0x0004F9u }, { 0x0004FAu, 0x0004FBu }, { 0x0004FCu, 0x0004FDu }, { 0x0004FEu, 0x0004FFu }, { 0x000500u, 0x000501u }, { 0x000502u, 0x000503u }, { 0x000504u, 0x000505u }, { 0x000506u, 0x000507u }, { 0x000508u, 0x000509u },
    { 0x00050Au, 0x00050Bu }, { 0x00050Cu, 0x00050Du }, { 0x00050Eu, 0x00050Fu }, { 0x000510u, 0x000511u }, { 0x000512u, 0x000513u }, { 0x000514u, 0x000515u }, { 0x000516u, 0x000517u }, { 0x000518u, 0x000519u }, { 0x00051Au, 0x00051Bu },
    { 0x00051Cu, 0x00051Du }, { 0x00051Eu, 0x00051Fu }, { 0x000520u, 0x000521u }, { 0x000522u, 0x000523u }, { 0x000524u, 0x000525u }, { 0x000526u, 0x000527u }, { 0x000528u, 0x000529u }, { 0x00052Au, 0x00052Bu }, { 0x00052Cu, 0x00052Du },
    { 0x00052Eu, 0x00052Fu }, { 0x0010C7u, 0x002D27u }, { 0x0010CDu, 0x002D2Du }, { 0x001C89u, 0x001C8Au }, { 0x001E00u, 0x001E01u }, { 0x001E02u, 0x001E03u }, { 0x001E04u, 0x001E05u }, { 0x001E06u, 0x001E07u }, { 0x001E08u, 0x001E09u },
    { 0x001E0Au, 0x001E0Bu }, { 0x001E0Cu, 0x001E0Du }, { 0x001E0Eu, 0x001E0Fu }, { 0x001E10u, 0x001E11u }, { 0x001E12u, 0x001E13u }, { 0x001E14u, 0x001E15u }, { 0x001E16u, 0x001E17u }, { 0x001E18u, 0x001E19u }, { 0x001E1Au, 0x001E1Bu },
    { 0x001E1Cu, 0x001E1Du }, { 0x001E1Eu, 0x001E1Fu }, { 0x001E20u, 0x001E21u }, { 0x001E22u, 0x001E23u }, { 0x001E24u, 0x001E25u }, { 0x001E26u, 0x001E27u }, { 0x001E28u, 0x001E29u }, { 0x001E2Au, 0x001E2Bu }, { 0x001E2Cu, 0x001E2Du },
    { 0x001E2Eu, 0x001E2Fu }, { 0x001E30u, 0x001E31u }, { 0x001E32u, 0x001E33u }, { 0x001E34u, 0x001E35u }, { 0x001E36u, 0x001E37u }, { 0x001E38u, 0x001E39u }, { 0x001E3Au, 0x001E3Bu }, { 0x001E3Cu, 0x001E3Du }, { 0x001E3Eu, 0x001E3Fu },
    { 0x001E40u, 0x001E41u }, { 0x001E42u, 0x001E43u }, { 0x001E44u, 0x001E45u }, { 0x001E46u, 0x001E47u }, { 0x001E48u, 0x001E49u }, { 0x001E4Au, 0x001E4Bu }, { 0x001E4Cu, 0x001E4Du }, { 0x001E4Eu, 0x001E4Fu }, { 0x001E50u, 0x001E51u },
    { 0x001E52u, 0x001E53u }, { 0x001E54u, 0x001E55u }, { 0x001E56u, 0x001E57u }, { 0x001E58u, 0x001E59u }, { 0x001E5Au, 0x001E5Bu }, { 0x001E5Cu, 0x001E5Du }, { 0x001E5Eu, 0x001E5Fu }, { 0x001E60u, 0x001E61u }, { 0x001E62u, 0x001E63u },
    { 0x001E64u, 0x001E65u }, { 0x001E66u, 0x001E67u }, { 0x001E68u, 0x001E69u }, { 0x001E6Au, 0x001E6Bu }, { 0x001E6Cu, 0x001E6Du }, { 0x001E6Eu, 0x001E6Fu }, { 0x001E70u, 0x001E71u }, { 0x001E72u, 0x001E73u }, { 0x001E74u, 0x001E75u },
    { 0x001E76u, 0x001E77u }, { 0x001E78u, 0x001E79u }, { 0x001E7Au, 0x001E7Bu }, { 0x001E7Cu, 0x001E7Du }, { 0x001E7Eu, 0x001E7Fu }, { 0x001E80u, 0x001E81u }, { 0x001E82u, 0x001E83u }, { 0x001E84u, 0x001E85u }, { 0x001E86u, 0x001E87u },
    { 0x001E88u, 0x001E89u }, { 0x001E8Au, 0x001E8Bu }, { 0x001E8Cu, 0x001E8Du }, { 0x001E8Eu, 0x001E8Fu }, { 0x001E90u, 0x001E91u }, { 0x001E92u, 0x001E93u }, { 0x001E94u, 0x001E95u }, { 0x001E9Eu, 0x0000DFu }, { 0x001EA0u, 0x001EA1u },
    { 0x001EA2u, 0x001EA3u }, { 0x001EA4u, 0x001EA5u }, { 0x001EA6u, 0x001EA7u }, { 0x001EA8u, 0x001EA9u }, { 0x001EAAu, 0x001EABu }, { 0x001EACu, 0x001EADu }, { 0x001EAEu, 0x001EAFu }, { 0x001EB0u, 0x001EB1u }, { 0x001EB2u, 0x001EB3u },
    { 0x001EB4u, 0x001EB5u }, { 0x001EB6u, 0x001EB7u }, { 0x001EB8u, 0x001EB9u }, { 0x001EBAu, 0x001EBBu }, { 0x001EBCu, 0x001EBDu }, { 0x001EBEu, 0x001EBFu }, { 0x001EC0u, 0x001EC1u }, { 0x001EC2u, 0x001EC3u }, { 0x001EC4u, 0x001EC5u },
    { 0x001EC6u, 0x001EC7u }, { 0x001EC8u, 0x001EC9u }, { 0x001ECAu, 0x001ECBu }, { 0x001ECCu, 0x001ECDu }, { 0x001ECEu, 0x001ECFu }, { 0x001ED0u, 0x001ED1u }, { 0x001ED2u, 0x001ED3u }, { 0x001ED4u, 0x001ED5u }, { 0x001ED6u, 0x001ED7u },
    { 0x001ED8u, 0x001ED9u }, { 0x001EDAu, 0x001EDBu }, { 0x001EDCu, 0x001EDDu }, { 0x001EDEu, 0x001EDFu }, { 0x001EE0u, 0x001EE1u }, { 0x001EE2u, 0x001EE3u }, { 0x001EE4u, 0x001EE5u }, { 0x001EE6u, 0x001EE7u }, { 0x001EE8u, 0x001EE9u },
    { 0x001EEAu, 0x001EEBu }, { 0x001EECu, 0x001EEDu }, { 0x001EEEu, 0x001EEFu }, { 0x001EF0u, 0x001EF1u }, { 0x001EF2u, 0x001EF3u }, { 0x001EF4u, 0x001EF5u }, { 0x001EF6u, 0x001EF7u }, { 0x001EF8u, 0x001EF9u }, { 0x001EFAu, 0x001EFBu },
    { 0x001EFCu, 0x001EFDu }, { 0x001EFEu, 0x001EFFu }, { 0x001F59u, 0x001F51u }, { 0x001F5Bu, 0x001F53u }, { 0x001F5Du, 0x001F55u }, { 0x001F5Fu, 0x001F57u }, { 0x001FBCu, 0x001FB3u }, { 0x001FCCu, 0x001FC3u }, { 0x001FECu, 0x001FE5u },
    { 0x001FFCu, 0x001FF3u }, { 0x002126u, 0x0003C9u }, { 0x00212Au, 0x00006Bu }, { 0x00212Bu, 0x0000E5u }, { 0x002132u, 0x00214Eu }, { 0x002183u, 0x002184u }, { 0x002C60u, 0x002C61u }, { 0x002C62u, 0x00026Bu }, { 0x002C63u, 0x001D7Du },
    { 0x002C64u, 0x00027Du }, { 0x002C67u, 0x002C68u }, { 0x002C69u, 0x002C6Au }, { 0x002C6Bu, 0x002C6Cu }, { 0x002C6Du, 0x000251u }, { 0x002C6Eu, 0x000271u }, { 0x002C6Fu, 0x000250u }, { 0x002C70u, 0x000252u }, { 0x002C72u, 0x002C73u },
    { 0x002C75u, 0x002C76u }, { 0x002C80u, 0x002C81u }, { 0x002C82u, 0x002C83u }, { 0x002C84u, 0x002C85u }, { 0x002C86u, 0x002C87u }, { 0x002C88u, 0x002C89u }, { 0x002C8Au, 0x002C8Bu }, { 0x002C8Cu, 0x002C8Du }, { 0x002C8Eu, 0x002C8Fu },
    { 0x002C90u, 0x002C91u }, { 0x002C92u, 0x002C93u }, { 0x002C94u, 0x002C95u }, { 0x002C96u, 0x002C97u }, { 0x002C98u, 0x002C99u }, { 0x002C9Au, 0x002C9Bu }, { 0x002C9Cu, 0x002C9Du }, { 0x002C9Eu, 0x002C9Fu }, { 0x002CA0u, 0x002CA1u },
    { 0x002CA2u, 0x002CA3u }, { 0x002CA4u, 0x002CA5u }, { 0x002CA6u, 0x002CA7u }, { 0x002CA8u, 0x002CA9u }, { 0x002CAAu, 0x002CABu }, { 0x002CACu, 0x002CADu }, { 0x002CAEu, 0x002CAFu }, { 0x002CB0u, 0x002CB1u }, { 0x002CB2u, 0x002CB3u },
    { 0x002CB4u, 0x002CB5u }, { 0x002CB6u, 0x002CB7u }, { 0x002CB8u, 0x002CB9u }, { 0x002CBAu, 0x002CBBu }, { 0x002CBCu, 0x002CBDu }, { 0x002CBEu, 0x002CBFu }, { 0x002CC0u, 0x002CC1u }, { 0x002CC2u, 0x002CC3u }, { 0x002CC4u, 0x002CC5u },
    { 0x002CC6u, 0x002CC7u }, { 0x002CC8u, 0x002CC9u }, { 0x002CCAu, 0x002CCBu }, { 0x002CCCu, 0x002CCDu }, { 0x002CCEu, 0x002CCFu }, { 0x002CD0u, 0x002CD1u }, { 0x002CD2u, 0x002CD3u }, { 0x002CD4u, 0x002CD5u }, { 0x002CD6u, 0x002CD7u },
    { 0x002CD8u, 0x002CD9u }, { 0x002CDAu, 0x002CDBu }, { 0x002CDCu, 0x002CDDu }, { 0x002CDEu, 0x002CDFu }, { 0x002CE0u, 0x002CE1u }, { 0x002CE2u, 0x002CE3u }, { 0x002CEBu, 0x002CECu }, { 0x002CEDu, 0x002CEEu }, { 0x002CF2u, 0x002CF3u },
    { 0x00A640u, 0x00A641u }, { 0x00A642u, 0x00A643u }, { 0x00A644u, 0x00A645u }, { 0x00A646u, 0x00A647u }, { 0x00A648u, 0x00A649u }, { 0x00A64Au, 0x00A64Bu }, { 0x00A64Cu, 0x00A64Du }, { 0x00A64Eu, 0x00A64Fu }, { 0x00A650u, 0x00A651u },
    { 0x00A652u, 0x00A653u }, { 0x00A654u, 0x00A655u }, { 0x00A656u, 0x00A657u }, { 0x00A658u, 0x00A659u }, { 0x00A65Au, 0x00A65Bu }, { 0x00A65Cu, 0x00A65Du }, { 0x00A65Eu, 0x00A65Fu }, { 0x00A660u, 0x00A661u }, { 0x00A662u, 0x00A663u },
    { 0x00A664u, 0x00A665u }, { 0x00A666u, 0x00A667u }, { 0x00A668u, 0x00A669u }, { 0x00A66Au, 0x00A66Bu }, { 0x00A66Cu, 0x00A66Du }, { 0x00A680u, 0x00A681u }, { 0x00A682u, 0x00A683u }, { 0x00A684u, 0x00A685u }, { 0x00A686u, 0x00A687u },
    { 0x00A688u, 0x00A689u }, { 0x00A68Au, 0x00A68Bu }, { 0x00A68Cu, 0x00A68Du }, { 0x00A68Eu, 0x00A68Fu }, { 0x00A690u, 0x00A691u }, { 0x00A692u, 0x00A693u }, { 0x00A694u, 0x00A695u }, { 0x00A696u, 0x00A697u }, { 0x00A698u, 0x00A699u },
    { 0x00A69Au, 0x00A69Bu }, { 0x00A722u, 0x00A723u }, { 0x00A724u, 0x00A725u }, { 0x00A726u, 0x00A727u }, { 0x00A728u, 0x00A729u }, { 0x00A72Au, 0x00A72Bu }, { 0x00A72Cu, 0x00A72Du }, { 0x00A72Eu, 0x00A72Fu }, { 0x00A732u, 0x00A733u },
    { 0x00A734u, 0x00A735u }, { 0x00A736u, 0x00A737u }, { 0x00A738u, 0x00A739u }, { 0x00A73Au, 0x00A73Bu }, { 0x00A73Cu, 0x00A73Du }, { 0x00A73Eu, 0x00A73Fu }, { 0x00A740u, 0x00A741u }, { 0x00A742u, 0x00A743u }, { 0x00A744u, 0x00A745u },
    { 0x00A746u, 0x00A747u }, { 0x00A748u, 0x00A749u }, { 0x00A74Au, 0x00A74Bu }, { 0x00A74Cu, 0x00A74Du }, { 0x00A74Eu, 0x00A74Fu }, { 0x00A750u, 0x00A751u }, { 0x00A752u, 0x00A753u }, { 0x00A754u, 0x00A755u }, { 0x00A756u, 0x00A757u },
    { 0x00A758u, 0x00A759u }, { 0x00A75Au, 0x00A75Bu }, { 0x00A75Cu, 0x00A75Du }, { 0x00A75Eu, 0x00A75Fu }, { 0x00A760u, 0x00A761u }, { 0x00A762u, 0x00A763u }, { 0x00A764u, 0x00A765u }, { 0x00A766u, 0x00A767u }, { 0x00A768u, 0x00A769u },
    { 0x00A76Au, 0x00A76Bu }, { 0x00A76Cu, 0x00A76Du }, { 0x00A76Eu, 0x00A76Fu }, { 0x00A779u, 0x00A77Au }, { 0x00A77Bu, 0x00A77Cu }, { 0x00A77Du, 0x001D79u }, { 0x00A77Eu, 0x00A77Fu }, { 0x00A780u, 0x00A781u }, { 0x00A782u, 0x00A783u },
    { 0x00A784u, 0x00A785u }, { 0x00A786u, 0x00A787u }, { 0x00A78Bu, 0x00A78Cu }, { 0x00A78Du, 0x000265u }, { 0x00A790u, 0x00A791u }, { 0x00A792u, 0x00A793u }, { 0x00A796u, 0x00A797u }, { 0x00A798u, 0x00A799u }, { 0x00A79Au, 0x00A79Bu },
    { 0x00A79Cu, 0x00A79Du }, { 0x00A79Eu, 0x00A79Fu }, { 0x00A7A0u, 0x00A7A1u }, { 0x00A7A2u, 0x00A7A3u }, { 0x00A7A4u, 0x00A7A5u }, { 0x00A7A6u, 0x00A7A7u }, { 0x00A7A8u, 0x00A7A9u }, { 0x00A7AAu, 0x000266u }, { 0x00A7ABu, 0x00025Cu },
    { 0x00A7ACu, 0x000261u }, { 0x00A7ADu, 0x00026Cu }, { 0x00A7AEu, 0x00026Au }, { 0x00A7B0u, 0x00029Eu }, { 0x00A7B1u, 0x000287u }, { 0x00A7B2u, 0x00029Du }, { 0x00A7B3u, 0x00AB53u }, { 0x00A7B4u, 0x00A7B5u }, { 0x00A7B6u, 0x00A7B7u },
    { 0x00A7B8u, 0x00A7B9u }, { 0x00A7BAu, 0x00A7BBu }, { 0x00A7BCu, 0x00A7BDu }, { 0x00A7BEu, 0x00A7BFu }, { 0x00A7C0u, 0x00A7C1u }, { 0x00A7C2u, 0x00A7C3u }, { 0x00A7C4u, 0x00A794u }, { 0x00A7C5u, 0x000282u }, { 0x00A7C6u, 0x001D8Eu },
    { 0x00A7C7u, 0x00A7C8u }, { 0x00A7C9u, 0x00A7CAu }, { 0x00A7CBu, 0x000264u }, { 0x00A7CCu, 0x00A7CDu }, { 0x00A7CEu, 0x00A7CFu }, { 0x00A7D0u, 0x00A7D1u }, { 0x00A7D2u, 0x00A7D3u }, { 0x00A7D4u, 0x00A7D5u }, { 0x00A7D6u, 0x00A7D7u },
    { 0x00A7D8u, 0x00A7D9u }, { 0x00A7DAu, 0x00A7DBu }, { 0x00A7DCu, 0x00019Bu }, { 0x00A7F5u, 0x00A7F6u },
};


static struct { CSTR_U32 cp; CSTR_U32 offset; CSTR_U32 count; } const internal_cstr_utf8_to_lower_multi[] = {
    { 0x000130u, 0, 2 },
};


static CSTR_U32 const internal_cstr_utf8_to_lower_pool[] = {
    0x000069u, 0x000307u,
};


static struct { CSTR_U32 lo; CSTR_U32 hi; int delta; } const internal_cstr_utf8_case_fold_ranges[] = {
    { 0x000041u, 0x00005Au, 32 }, { 0x0000C0u, 0x0000D6u, 32 }, { 0x0000D8u, 0x0000DEu, 32 }, { 0x000189u, 0x00018Au, 205 }, { 0x0001B1u, 0x0001B2u, 217 }, { 0x000388u, 0x00038Au, 37 }, { 0x00038Eu, 0x00038Fu, 63 }, { 0x000391u, 0x0003A1u, 32 },
    { 0x0003A3u, 0x0003ABu, 32 }, { 0x0003FDu, 0x0003FFu, -130 }, { 0x000400u, 0x00040Fu, 80 }, { 0x000410u, 0x00042Fu, 32 }, { 0x000531u, 0x000556u, 48 }, { 0x0010A0u, 0x0010C5u, 7264 }, { 0x0013F8u, 0x0013FDu, -8 },
    { 0x001C83u, 0x001C84u, -6210 }, { 0x001C90u, 0x001CBAu, -3008 }, { 0x001CBDu, 0x001CBFu, -3008 }, { 0x001F08u, 0x001F0Fu, -8 }, { 0x001F18u, 0x001F1Du, -8 }, { 0x001F28u, 0x001F2Fu, -8 }, { 0x001F38u, 0x001F3Fu, -8 },
    { 0x001F48u, 0x001F4Du, -8 }, { 0x001F68u, 0x001F6Fu, -8 }, { 0x001FB8u, 0x001FB9u, -8 }, { 0x001FBAu, 0x001FBBu, -74 }, { 0x001FC8u, 0x001FCBu, -86 }, { 0x001FD8u, 0x001FD9u, -8 }, { 0x001FDAu, 0x001FDBu, -100 }, { 0x001FE8u, 0x001FE9u, -8 },
    { 0x001FEAu, 0x001FEBu, -112 }, { 0x001FF8u, 0x001FF9u, -128 }, { 0x001FFAu, 0x001FFBu, -126 }, { 0x002160u, 0x00216Fu, 16 }, { 0x0024B6u, 0x0024CFu, 26 }, { 0x002C00u, 0x002C2Fu, 48 }, { 0x002C7Eu, 0x002C7Fu, -10815 },
    { 0x00AB70u, 0x00ABBFu, -38864 }, { 0x00FF21u, 0x00FF3Au, 32 }, { 0x010400u, 0x010427u, 40 }, { 0x0104B0u, 0x0104D3u, 40 }, { 0x010570u, 0x01057Au, 39 }, { 0x01057Cu, 0x01058Au, 39 }, { 0x01058Cu, 0x010592u, 39 }, { 0x010594u, 0x010595u, 39 },
    { 0x010C80u, 0x010CB2u, 64 }, { 0x010D50u, 0x010D65u, 32 }, { 0x0118A0u, 0x0118BFu, 32 }, { 0x016E40u, 0x016E5Fu, 32 }, { 0x016EA0u, 0x016EB8u, 27 }, { 0x01E900u, 0x01E921u, 34 },
};


static struct { CSTR_U32 cp; CSTR_U32 to; } const internal_cstr_utf8_case_fold_pairs[] = {
    { 0x0000B5u, 0x0003BCu }, { 0x000100u, 0x000101u }, { 0x000102u, 0x000103u }, { 0x000104u, 0x000105u }, { 0x000106u, 0x000107u }, { 0x000108u, 0x000109u }, { 0x00010Au, 0x00010Bu }, { 0x00010Cu, 0x00010Du }, { 0x00010Eu, 0x00010Fu },
    { 0x000110u, 0x000111u }, { 0x000112u, 0x000113u }, { 0x000114u, 0x000115u }, { 0x000116u, 0x000117u }, { 0x000118u, 0x000119u }, { 0x00011Au, 0x00011Bu }, { 0x00011Cu, 0x00011Du }, { 0x00011Eu, 0x00011Fu }, { 0x000120u, 0x000121u },
    { 0x000122u, 0x000123u }, { 0x000124u, 0x000125u }, { 0x000126u, 0x000127u }, { 0x000128u, 0x000129u }, { 0x00012Au, 0x00012Bu }, { 0x00012Cu, 0x00012Du }, { 0x00012Eu, 0x00012Fu }, { 0x000132u, 0x000133u }, { 0x000134u, 0x000135u },
    { 0x000136u, 0x000137u }, { 0x000139u, 0x00013Au }, { 0x00013Bu, 0x00013Cu }, { 0x00013Du, 0x00013Eu }, { 0x00013Fu, 0x000140u }, { 0x000141u, 0x000142u }, { 0x000143u, 0x000144u }, { 0x000145u, 0x000146u }, { 0x000147u, 0x000148u },
    { 0x00014Au, 0x00014Bu }, { 0x00014Cu, 0x00014Du }, { 0x00014Eu, 0x00014Fu }, { 0x000150u, 0x000151u }, { 0x000152u, 0x000153u }, { 0x000154u, 0x000155u }, { 0x000156u, 0x000157u }, { 0x000158u, 0x000159u }, { 0x00015Au, 0x00015Bu },
    { 0x00015Cu, 0x00015Du }, { 0x00015Eu, 0x00015Fu }, { 0x000160u, 0x000161u }, { 0x000162u, 0x000163u }, { 0x000164u, 0x000165u }, { 0x000166u, 0x000167u }, { 0x000168u, 0x000169u }, { 0x00016Au, 0x00016Bu }, { 0x00016Cu, 0x00016Du },
    { 0x00016Eu, 0x00016Fu }, { 0x000170u, 0x000171u }, { 0x000172u, 0x000173u }, { 0x000174u, 0x000175u }, { 0x000176u, 0x000177u }, { 0x000178u, 0x0000FFu }, { 0x000179u, 0x00017Au }, { 0x00017Bu, 0x00017Cu }, { 0x00017Du, 0x00017Eu },
    { 0x00017Fu, 0x000073u }, { 0x000181u, 0x000253u }, { 0x000182u, 0x000183u }, { 0x000184u, 0x000185u }, { 0x000186u, 0x000254u }, { 0x000187u, 0x000188u }, { 0x00018Bu, 0x00018Cu }, { 0x00018Eu, 0x0001DDu }, { 0x00018Fu, 0x000259u },
    { 0x000190u, 0x00025Bu }, { 0x000191u, 0x000192u }, { 0x000193u, 0x000260u }, { 0x000194u, 0x000263u }, { 0x000196u, 0x000269u }, { 0x000197u, 0x000268u }, { 0x000198u, 0x000199u }, { 0x00019Cu, 0x00026Fu }, { 0x00019Du, 0x000272u },
    { 0x00019Fu, 0x000275u }, { 0x0001A0u, 0x0001A1u }, { 0x0001A2u, 0x0001A3u }, { 0x0001A4u, 0x0001A5u }, { 0x0001A6u, 0x000280u }, { 0x0001A7u, 0x0001A8u }, { 0x0001A9u, 0x000283u }, { 0x0001ACu, 0x0001ADu }, { 0x0001AEu, 0x000288u },
    { 0x0001AFu, 0x0001B0u }, { 0x0001B3u, 0x0001B4u }, { 0x0001B5u, 0x0001B6u }, { 0x0001B7u, 0x000292u }, { 0x0001B8u, 0x0001B9u }, { 0x0001BCu, 0x0001BDu }, { 0x0001C4u, 0x0001C6u }, { 0x0001C5u, 0x0001C6u }, { 0x0001C7u, 0x0001C9u },
    { 0x0001C8u, 0x0001C9u }, { 0x0001CAu, 0x0001CCu }, { 0x0001CBu, 0x0001CCu }, { 0x0001CDu, 0x0001CEu }, { 0x0001CFu, 0x0001D0u }, { 0x0001D1u, 0x0001D2u }, { 0x0001D3u, 0x0001D4u }, { 0x0001D5u, 0x0001D6u }, { 0x0001D7u, 0x0001D8u },
    { 0x0001D9u, 0x0001DAu }, { 0x0001DBu, 0x0001DCu }, { 0x0001DEu, 0x0001DFu }, { 0x0001E0u, 0x0001E1u }, { 0x0001E2u, 0x0001E3u }, { 0x0001E4u, 0x0001E5u }, { 0x0001E6u, 0x0001E7u }, { 0x0001E8u, 0x0001E9u }, { 0x0001EAu, 0x0001EBu },
    { 0x0001ECu, 0x0001EDu }, { 0x0001EEu, 0x0001EFu }, { 0x0001F1u, 0x0001F3u }, { 0x0001F2u, 0x0001F3u }, { 0x0001F4u, 0x0001F5u }, { 0x0001F6u, 0x000195u }, { 0x0001F7u, 0x0001BFu }, { 0x0001F8u, 0x0001F9u }, { 0x0001FAu, 0x0001FBu },
    { 0x0001FCu, 0x0001FDu }, { 0x0001FEu, 0x0001FFu }, { 0x000200u, 0x000201u }, { 0x000202u, 0x000203u }, { 0x000204u, 0x000205u }, { 0x000206u, 0x000207u }, { 0x000208u, 0x000209u }, { 0x00020Au, 0x00020Bu }, { 0x00020Cu, 0x00020Du },
    { 0x00020Eu, 0x00020Fu }, { 0x000210u, 0x000211u }, { 0x000212u, 0x000213u }, { 0x000214u, 0x000215u }, { 0x000216u, 0x000217u }, { 0x000218u, 0x000219u }, { 0x00021Au, 0x00021Bu }, { 0x00021Cu, 0x00021Du }, { 0x00021Eu, 0x00021Fu },
    { 0x000220u, 0x00019Eu }, { 0x000222u, 0x000223u }, { 0x000224u, 0x000225u }, { 0x000226u, 0x000227u }, { 0x000228u, 0x000229u }, { 0x00022Au, 0x00022Bu }, { 0x00022Cu, 0x00022Du }, { 0x00022Eu, 0x00022Fu }, { 0x000230u, 0x000231u },
    { 0x000232u, 0x000233u }, { 0x00023Au, 0x002C65u }, { 0x00023Bu, 0x00023Cu }, { 0x00023Du, 0x00019Au }, { 0x00023Eu, 0x002C66u }, { 0x000241u, 0x000242u }, { 0x000243u, 0x000180u }, { 0x000244u, 0x000289u }, { 0x000245u, 0x00028Cu },
    { 0x000246u, 0x000247u }, { 0x000248u, 0x000249u }, { 0x00024Au, 0x00024Bu }, { 0x00024Cu, 0x00024Du }, { 0x00024Eu, 0x00024Fu }, { 0x000345u, 0x0003B9u }, { 0x000370u, 0x000371u }, { 0x000372u, 0x000373u }, { 0x000376u, 0x000377u },
    { 0x00037Fu, 0x0003F3u }, { 0x000386u, 0x0003ACu }, { 0x00038Cu, 0x0003CCu }, { 0x0003C2u, 0x0003C3u }, { 0x0003CFu, 0x0003D7u }, { 0x0003D0u, 0x0003B2u }, { 0x0003D1u, 0x0003B8u }, { 0x0003D5u, 0x0003C6u }, { 0x0003D6u, 0x0003C0u },
    { 0x0003D8u, 0x0003D9u }, { 0x0003DAu, 0x0003DBu }, { 0x0003DCu, 0x0003DDu }, { 0x0003DEu, 0x0003DFu }, { 0x0003E0u, 0x0003E1u }, { 0x0003E2u, 0x0003E3u }, { 0x0003E4u, 0x0003E5u }, { 0x0003E6u, 0x0003E7u }, { 0x0003E8u, 0x0003E9u },
    { 0x0003EAu, 0x0003EBu }, { 0x0003ECu, 0x0003EDu }, { 0x0003EEu, 0x0003EFu }, { 0x0003F0u, 0x0003BAu }, { 0x0003F1u, 0x0003C1u }, { 0x0003F4u, 0x0003B8u }, { 0x0003F5u, 0x0003B5u }, { 0x0003F7u, 0x0003F8u }, { 0x0003F9u, 0x0003F2u },
    { 0x0003FAu, 0x0003FBu }, { 0x000460u, 0x000461u }, { 0x000462u, 0x000463u }, { 0x000464u, 0x000465u }, { 0x000466u, 0x000467u }, { 0x000468u, 0x000469u }, { 0x00046Au, 0x00046Bu }, { 0x00046Cu, 0x00046Du }, { 0x00046Eu, 0x00046Fu },
    { 0x000470u, 0x000471u }, { 0x000472u, 0x000473u }, { 0x000474u, 0x000475u }, { 0x000476u, 0x000477u }, { 0x000478u, 0x000479u }, { 0x00047Au, 0x00047Bu }, { 0x00047Cu, 0x00047Du }, { 0x00047Eu, 0x00047Fu }, { 0x000480u, 0x000481u },
    { 0x00048Au, 0x00048Bu }, { 0x00048Cu, 0x00048Du }, { 0x00048Eu, 0x00048Fu }, { 0x000490u, 0x000491u }, { 0x000492u, 0x000493u }, { 0x000494u, 0x000495u }, { 0x000496u, 0x000497u }, { 0x000498u, 0x000499u }, { 0x00049Au, 0x00049Bu },
    { 0x00049Cu, 0x00049Du }, { 0x00049Eu, 0x00049Fu }, { 0x0004A0u, 0x0004A1u }, { 0x0004A2u, 0x0004A3u }, { 0x0004A4u, 0x0004A5u }, { 0x0004A6u, 0x0004A7u }, { 0x0004A8u, 0x0004A9u }, { 0x0004AAu, 0x0004ABu }, { 0x0004ACu, 0x0004ADu },
    { 0x0004AEu, 0x0004AFu }, { 0x0004B0u, 0x0004B1u }, { 0x0004B2u, 0x0004B3u }, { 0x0004B4u, 0x0004B5u }, { 0x0004B6u, 0x0004B7u }, { 0x0004B8u, 0x0004B9u }, { 0x0004BAu, 0x0004BBu }, { 0x0004BCu, 0x0004BDu }, { 0x0004BEu, 0x0004BFu },
    { 0x0004C0u, 0x0004CFu }, { 0x0004C1u, 0x0004C2u }, { 0x0004C3u, 0x0004C4u }, { 0x0004C5u, 0x0004C6u }, { 0x0004C7u, 0x0004C8u }, { 0x0004C9u, 0x0004CAu }, { 0x0004CBu, 0x0004CCu }, { 0x0004CDu, 0x0004CEu }, { 0x0004D0u, 0x0004D1u },
    { 0x0004D2u, 0x0004D3u }, { 0x0004D4u, 0x0004D5u }, { 0x0004D6u, 0x0004D7u }, { 0x0004D8u, 0x0004D9u }, { 0x0004DAu, 0x0004DBu }, { 0x0004DCu, 0x0004DDu }, { 0x0004DEu, 0x0004DFu }, { 0x0004E0u, 0x0004E1u }, { 0x0004E2u, 0x0004E3u },
    { 0x0004E4u, 0x0004E5u }, { 0x0004E6u, 0x0004E7u }, { 0x0004E8u, 0x0004E9u }, { 0x0004EAu, 0x0004EBu }, { 0x0004ECu, 0x0004EDu }, { 0x0004EEu, 0x0004EFu }, { 0x0004F0u, 0x0004F1u }, { 0x0004F2u, 0x0004F3u }, { 0x0004F4u, 0x0004F5u },
    { 0x0004F6u, 0x0004F7u }, { 0x0004F8u, 0x0004F9u }, { 0x0004FAu, 0x0004FBu }, { 0x0004FCu, 0x0004FDu }, { 0x0004FEu, 0x0004FFu }, { 0x000500u, 0x000501u }, { 0x000502u, 0x000503u }, { 0x000504u, 0x000505u }, { 0x000506u, 0x000507u },
    { 0x000508u, 0x000509u }, { 0x00050Au, 0x00050Bu }, { 0x00050Cu, 0x00050Du }, { 0x00050Eu, 0x00050Fu }, { 0x000510u, 0x000511u }, { 0x000512u, 0x000513u }, { 0x000514u, 0x000515u }, { 0x000516u, 0x000517u }, { 0x000518u, 0x000519u },
    { 0x00051Au, 0x00051Bu }, { 0x00051Cu, 0x00051Du }, { 0x00051Eu, 0x00051Fu }, { 0x000520u, 0x000521u }, { 0x000522u, 0x000523u }, { 0x000524u, 0x000525u }, { 0x000526u, 0x000527u }, { 0x000528u, 0x000529u }, { 0x00052Au, 0x00052Bu },
    { 0x00052Cu, 0x00052Du }, { 0x00052Eu, 0x00052Fu }, { 0x0010C7u, 0x002D27u }, { 0x0010CDu, 0x002D2Du }, { 0x001C80u, 0x000432u }, { 0x001C81u, 0x000434u }, { 0x001C82u, 0x00043Eu }, { 0x001C85u, 0x000442u }, { 0x001C86u, 0x00044Au },
    { 0x001C87u, 0x000463u }, { 0x001C88u, 0x00A64Bu }, { 0x001C89u, 0x001C8Au }, { 0x001E00u, 0x001E01u }, { 0x001E02u, 0x001E03u }, { 0x001E04u, 0x001E05u }, { 0x001E06u, 0x001E07u }, { 0x001E08u, 0x001E09u }, { 0x001E0Au, 0x001E0Bu },
    { 0x001E0Cu, 0x001E0Du }, { 0x001E0Eu, 0x001E0Fu }, { 0x001E10u, 0x001E11u }, { 0x001E12u, 0x001E13u }, { 0x001E14u, 0x001E15u }, { 0x001E16u, 0x001E17u }, { 0x001E18u, 0x001E19u }, { 0x001E1Au, 0x001E1Bu }, { 0x001E1Cu, 0x001E1Du },
    { 0x001E1Eu, 0x001E1Fu }, { 0x001E20u, 0x001E21u }, { 0x001E22u, 0x001E23u }, { 0x001E24u, 0x001E25u }, { 0x001E26u, 0x001E27u }, { 0x001E28u, 0x001E29u }, { 0x001E2Au, 0x001E2Bu }, { 0x001E2Cu, 0x001E2Du }, { 0x001E2Eu, 0x001E2Fu },
    { 0x001E30u, 0x001E31u }, { 0x001E32u, 0x001E33u }, { 0x001E34u, 0x001E35u }, { 0x001E36u, 0x001E37u }, { 0x001E38u, 0x001E39u }, { 0x001E3Au, 0x001E3Bu }, { 0x001E3Cu, 0x001E3Du }, { 0x001E3Eu, 0x001E3Fu }, { 0x001E40u, 0x001E41u },
    { 0x001E42u, 0x001E43u }, { 0x001E44u, 0x001E45u }, { 0x001E46u, 0x001E47u }, { 0x001E48u, 0x001E49u }, { 0x001E4Au, 0x001E4Bu }, { 0x001E4Cu, 0x001E4Du }, { 0x001E4Eu, 0x001E4Fu }, { 0x001E50u, 0x001E51u }, { 0x001E52u, 0x001E53u },
    { 0x001E54u, 0x001E55u }, { 0x001E56u, 0x001E57u }, { 0x001E58u, 0x001E59u }, { 0x001E5Au, 0x001E5Bu }, { 0x001E5Cu, 0x001E5Du }, { 0x001E5Eu, 0x001E5Fu }, { 0x001E60u, 0x001E61u }, { 0x001E62u, 0x001E63u }, { 0x001E64u, 0x001E65u },
    { 0x001E66u, 0x001E67u }, { 0x001E68u, 0x001E69u }, { 0x001E6Au, 0x001E6Bu }, { 0x001E6Cu, 0x001E6Du }, { 0x001E6Eu, 0x001E6Fu }, { 0x001E70u, 0x001E71u }, { 0x001E72u, 0x001E73u }, { 0x001E74u, 0x001E75u }, { 0x001E76u, 0x001E77u },
    { 0x001E78u, 0x001E79u }, { 0x001E7Au, 0x001E7Bu }, { 0x001E7Cu, 0x001E7Du }, { 0x001E7Eu, 0x001E7Fu }, { 0x001E80u, 0x001E81u }, { 0x001E82u, 0x001E83u }, { 0x001E84u, 0x001E85u }, { 0x001E86u, 0x001E87u }, { 0x001E88u, 0x001E89u },
    { 0x001E8Au, 0x001E8Bu }, { 0x001E8Cu, 0x001E8Du }, { 0x001E8Eu, 0x001E8Fu }, { 0x001E90u, 0x001E91u }, { 0x001E92u, 0x001E93u }, { 0x001E94u, 0x001E95u }, { 0x001E9Bu, 0x001E61u }, { 0x001EA0u, 0x001EA1u }, { 0x001EA2u, 0x001EA3u },
    { 0x001EA4u, 0x001EA5u }, { 0x001EA6u, 0x001EA7u }, { 0x001EA8u, 0x001EA9u }, { 0x001EAAu, 0x001EABu }, { 0x001EACu, 0x001EADu }, { 0x001EAEu, 0x001EAFu }, { 0x001EB0u, 0x001EB1u }, { 0x001EB2u, 0x001EB3u }, { 0x001EB4u, 0x001EB5u },
    { 0x001EB6u, 0x001EB7u }, { 0x001EB8u, 0x001EB9u }, { 0x001EBAu, 0x001EBBu }, { 0x001EBCu, 0x001EBDu }, { 0x001EBEu, 0x001EBFu }, { 0x001EC0u, 0x001EC1u }, { 0x001EC2u, 0x001EC3u }, { 0x001EC4u, 0x001EC5u }, { 0x001EC6u, 0x001EC7u },
    { 0x001EC8u, 0x001EC9u }, { 0x001ECAu, 0x001ECBu }, { 0x001ECCu, 0x001ECDu }, { 0x001ECEu, 0x001ECFu }, { 0x001ED0u, 0x001ED1u }, { 0x001ED2u, 0x001ED3u }, { 0x001ED4u, 0x001ED5u }, { 0x001ED6u, 0x001ED7u }, { 0x001ED8u, 0x001ED9u },
    { 0x001EDAu, 0x001EDBu }, { 0x001EDCu, 0x001EDDu }, { 0x001EDEu, 0x001EDFu }, { 0x001EE0u, 0x001EE1u }, { 0x001EE2u, 0x001EE3u }, { 0x001EE4u, 0x001EE5u }, { 0x001EE6u, 0x001EE7u }, { 0x001EE8u, 0x001EE9u }, { 0x001EEAu, 0x001EEBu },
    { 0x001EECu, 0x001EEDu }, { 0x001EEEu, 0x001EEFu }, { 0x001EF0u, 0x001EF1u }, { 0x001EF2u, 0x001EF3u }, { 0x001EF4u, 0x001EF5u }, { 0x001EF6u, 0x001EF7u }, { 0x001EF8u, 0x001EF9u }, { 0x001EFAu, 0x001EFBu }, { 0x001EFCu, 0x001EFDu },
    { 0x001EFEu, 0x001EFFu }, { 0x001F59u, 0x001F51u }, { 0x001F5Bu, 0x001F53u }, { 0x001F5Du, 0x001F55u }, { 0x001F5Fu, 0x001F57u }, { 0x001FBEu, 0x0003B9u }, { 0x001FECu, 0x001FE5u }, { 0x002126u, 0x0003C9u }, { 0x00212Au, 0x00006Bu },
    { 0x00212Bu, 0x0000E5u }, { 0x002132u, 0x00214Eu }, { 0x002183u, 0x002184u }, { 0x002C60u, 0x002C61u }, { 0x002C62u, 0x00026Bu }, { 0x002C63u, 0x001D7Du }, { 0x002C64u, 0x00027Du }, { 0x002C67u, 0x002C68u }, { 0x002C69u, 0x002C6Au },
    { 0x002C6Bu, 0x002C6Cu }, { 0x002C6Du, 0x000251u }, { 0x002C6Eu, 0x000271u }, { 0x002C6Fu, 0x000250u }, { 0x002C70u, 0x000252u }, { 0x002C72u, 0x002C73u }, { 0x002C75u, 0x002C76u }, { 0x002C80u, 0x002C81u }, { 0x002C82u, 0x002C83u },
    { 0x002C84u, 0x002C85u }, { 0x002C86u, 0x002C87u }, { 0x002C88u, 0x002C89u }, { 0x002C8Au, 0x002C8Bu }, { 0x002C8Cu, 0x002C8Du }, { 0x002C8Eu, 0x002C8Fu }, { 0x002C90u, 0x002C91u }, { 0x002C92u, 0x002C93u }, { 0x002C94u, 0x002C95u },
    { 0x002C96u, 0x002C97u }, { 0x002C98u, 0x002C99u }, { 0x002C9Au, 0x002C9Bu }, { 0x002C9Cu, 0x002C9Du }, { 0x002C9Eu, 0x002C9Fu }, { 0x002CA0u, 0x002CA1u }, { 0x002CA2u, 0x002CA3u }, { 0x002CA4u, 0x002CA5u }, { 0x002CA6u, 0x002CA7u },
    { 0x002CA8u, 0x002CA9u }, { 0x002CAAu, 0x002CABu }, { 0x002CACu, 0x002CADu }, { 0x002CAEu, 0x002CAFu }, { 0x002CB0u, 0x002CB1u }, { 0x002CB2u, 0x002CB3u }, { 0x002CB4u, 0x002CB5u }, { 0x002CB6u, 0x002CB7u }, { 0x002CB8u, 0x002CB9u },
    { 0x002CBAu, 0x002CBBu }, { 0x002CBCu, 0x002CBDu }, { 0x002CBEu, 0x002CBFu }, { 0x002CC0u, 0x002CC1u }, { 0x002CC2u, 0x002CC3u }, { 0x002CC4u, 0x002CC5u }, { 0x002CC6u, 0x002CC7u }, { 0x002CC8u, 0x002CC9u }, { 0x002CCAu, 0x002CCBu },
    { 0x002CCCu, 0x002CCDu }, { 0x002CCEu, 0x002CCFu }, { 0x002CD0u, 0x002CD1u }, { 0x002CD2u, 0x002CD3u }, { 0x002CD4u, 0x002CD5u }, { 0x002CD6u, 0x002CD7u }, { 0x002CD8u, 0x002CD9u }, { 0x002CDAu, 0x002CDBu }, { 0x002CDCu, 0x002CDDu },
    { 0x002CDEu, 0x002CDFu }, { 0x002CE0u, 0x002CE1u }, { 0x002CE2u, 0x002CE3u }, { 0x002CEBu, 0x002CECu }, { 0x002CEDu, 0x002CEEu }, { 0x002CF2u, 0x002CF3u }, { 0x00A640u, 0x00A641u }, { 0x00A642u, 0x00A643u }, { 0x00A644u, 0x00A645u },
    { 0x00A646u, 0x00A647u }, { 0x00A648u, 0x00A649u }, { 0x00A64Au, 0x00A64Bu }, { 0x00A64Cu, 0x00A64Du }, { 0x00A64Eu, 0x00A64Fu }, { 0x00A650u, 0x00A651u }, { 0x00A652u, 0x00A653u }, { 0x00A654u, 0x00A655u }, { 0x00A656u, 0x00A657u },
    { 0x00A658u, 0x00A659u }, { 0x00A65Au, 0x00A65Bu }, { 0x00A65Cu, 0x00A65Du }, { 0x00A65Eu, 0x00A65Fu }, { 0x00A660u, 0x00A661u }, { 0x00A662u, 0x00A663u }, { 0x00A664u, 0x00A665u }, { 0x00A666u, 0x00A667u }, { 0x00A668u, 0x00A669u },
    { 0x00A66Au, 0x00A66Bu }, { 0x00A66Cu, 0x00A66Du }, { 0x00A680u, 0x00A681u }, { 0x00A682u, 0x00A683u }, { 0x00A684u, 0x00A685u }, { 0x00A686u, 0x00A687u }, { 0x00A688u, 0x00A689u }, { 0x00A68Au, 0x00A68Bu }, { 0x00A68Cu, 0x00A68Du },
    { 0x00A68Eu, 0x00A68Fu }, { 0x00A690u, 0x00A691u }, { 0x00A692u, 0x00A693u }, { 0x00A694u, 0x00A695u }, { 0x00A696u, 0x00A697u }, { 0x00A698u, 0x00A699u }, { 0x00A69Au, 0x00A69Bu }, { 0x00A722u, 0x00A723u }, { 0x00A724u, 0x00A725u },
    { 0x00A726u, 0x00A727u }, { 0x00A728u, 0x00A729u }, { 0x00A72Au, 0x00A72Bu }, { 0x00A72Cu, 0x00A72Du }, { 0x00A72Eu, 0x00A72Fu }, { 0x00A732u, 0x00A733u }, { 0x00A734u, 0x00A735u }, { 0x00A736u, 0x00A737u }, { 0x00A738u, 0x00A739u },
    { 0x00A73Au, 0x00A73Bu }, { 0x00A73Cu, 0x00A73Du }, { 0x00A73Eu, 0x00A73Fu }, { 0x00A740u, 0x00A741u }, { 0x00A742u, 0x00A743u }, { 0x00A744u, 0x00A745u }, { 0x00A746u, 0x00A747u }, { 0x00A748u, 0x00A749u }, { 0x00A74Au, 0x00A74Bu },
    { 0x00A74Cu, 0x00A74Du }, { 0x00A74Eu, 0x00A74Fu }, { 0x00A750u, 0x00A751u }, { 0x00A752u, 0x00A753u }, { 0x00A754u, 0x00A755u }, { 0x00A756u, 0x00A757u }, { 0x00A758u, 0x00A759u }, { 0x00A75Au, 0x00A75Bu }, { 0x00A75Cu, 0x00A75Du },
    { 0x00A75Eu, 0x00A75Fu }, { 0x00A760u, 0x00A761u }, { 0x00A762u, 0x00A763u }, { 0x00A764u, 0x00A765u }, { 0x00A766u, 0x00A767u }, { 0x00A768u, 0x00A769u }, { 0x00A76Au, 0x00A76Bu }, { 0x00A76Cu, 0x00A76Du }, { 0x00A76Eu, 0x00A76Fu },
    { 0x00A779u, 0x00A77Au }, { 0x00A77Bu, 0x00A77Cu }, { 0x00A77Du, 0x001D79u }, { 0x00A77Eu, 0x00A77Fu }, { 0x00A780u, 0x00A781u }, { 0x00A782u, 0x00A783u }, { 0x00A784u, 0x00A785u }, { 0x00A786u, 0x00A787u }, { 0x00A78Bu, 0x00A78Cu },
    { 0x00A78Du, 0x000265u }, { 0x00A790u, 0x00A791u }, { 0x00A792u, 0x00A793u }, { 0x00A796u, 0x00A797u }, { 0x00A798u, 0x00A799u }, { 0x00A79Au, 0x00A79Bu }, { 0x00A79Cu, 0x00A79Du }, { 0x00A79Eu, 0x00A79Fu }, { 0x00A7A0u, 0x00A7A1u },
    { 0x00A7A2u, 0x00A7A3u }, { 0x00A7A4u, 0x00A7A5u }, { 0x00A7A6u, 0x00A7A7u }, { 0x00A7A8u, 0x00A7A9u }, { 0x00A7AAu, 0x000266u }, { 0x00A7ABu, 0x00025Cu }, { 0x00A7ACu, 0x000261u }, { 0x00A7ADu, 0x00026Cu }, { 0x00A7AEu, 0x00026Au },
    { 0x00A7B0u, 0x00029Eu }, { 0x00A7B1u, 0x000287u }, { 0x00A7B2u, 0x00029Du }, { 0x00A7B3u, 0x00AB53u }, { 0x00A7B4u, 0x00A7B5u }, { 0x00A7B6u, 0x00A7B7u }, { 0x00A7B8u, 0x00A7B9u }, { 0x00A7BAu, 0x00A7BBu }, { 0x00A7BCu, 0x00A7BDu },
    { 0x00A7BEu, 0x00A7BFu }, { 0x00A7C0u, 0x00A7C1u }, { 0x00A7C2u, 0x00A7C3u }, { 0x00A7C4u, 0x00A794u }, { 0x00A7C5u, 0x000282u }, { 0x00A7C6u, 0x001D8Eu }, { 0x00A7C7u, 0x00A7C8u }, { 0x00A7C9u, 0x00A7CAu }, { 0x00A7CBu, 0x000264u },
    { 0x00A7CCu, 0x00A7CDu }, { 0x00A7CEu, 0x00A7CFu }, { 0x00A7D0u, 0x00A7D1u }, { 0x00A7D2u, 0x00A7D3u }, { 0x00A7D4u, 0x00A7D5u }, { 0x00A7D6u, 0x00A7D7u }, { 0x00A7D8u, 0x00A7D9u }, { 0x00A7DAu, 0x00A7DBu }, { 0x00A7DCu, 0x00019Bu },
    { 0x00A7F5u, 0x00A7F6u },
};


static struct { CSTR_U32 cp; CSTR_U32 offset; CSTR_U32 count; } const internal_cstr_utf8_case_fold_multi[] = {
    { 0x0000DFu, 0, 2 }, { 0x000130u, 2, 2 }, { 0x000149u, 4, 2 }, { 0x0001F0u, 6, 2 }, { 0x000390u, 8, 3 }, { 0x0003B0u, 11, 3 }, { 0x000587u, 14, 2 }, { 0x001E96u, 16, 2 }, { 0x001E97u, 18, 2 }, { 0x001E98u, 20, 2 }, { 0x001E99u, 22, 2 },
    { 0x001E9Au, 24, 2 }, { 0x001E9Eu, 26, 2 }, { 0x001F50u, 28, 2 }, { 0x001F52u, 30, 3 }, { 0x001F54u, 33, 3 }, { 0x001F56u, 36, 3 }, { 0x001F80u, 39, 2 }, { 0x001F81u, 41, 2 }, { 0x001F82u, 43, 2 }, { 0x001F83u, 45, 2 }, { 0x001F84u, 47, 2 },
    { 0x001F85u, 49, 2 }, { 0x001F86u, 51, 2 }, { 0x001F87u, 53, 2 }, { 0x001F88u, 55, 2 }, { 0x001F89u, 57, 2 }, { 0x001F8Au, 59, 2 }, { 0x001F8Bu, 61, 2 }, { 0x001F8Cu, 63, 2 }, { 0x001F8Du, 65, 2 }, { 0x001F8Eu, 67, 2 }, { 0x001F8Fu, 69, 2 },
    { 0x001F90u, 71, 2 }, { 0x001F91u, 73, 2 }, { 0x001F92u, 75, 2 }, { 0x001F93u, 77, 2 }, { 0x001F94u, 79, 2 }, { 0x001F95u, 81, 2 }, { 0x001F96u, 83, 2 }, { 0x001F97u, 85, 2 }, { 0x001F98u, 87, 2 }, { 0x001F99u, 89, 2 }, { 0x001F9Au, 91, 2 },
    { 0x001F9Bu, 93, 2 }, { 0x001F9Cu, 95, 2 }, { 0x001F9Du, 97, 2 }, { 0x001F9Eu, 99, 2 }, { 0x001F9Fu, 101, 2 }, { 0x001FA0u, 103, 2 }, { 0x001FA1u, 105, 2 }, { 0x001FA2u, 107, 2 }, { 0x001FA3u, 109, 2 }, { 0x001FA4u, 111, 2 },
    { 0x001FA5u, 113, 2 }, { 0x001FA6u, 115, 2 }, { 0x001FA7u, 117, 2 }, { 0x001FA8u, 119, 2 }, { 0x001FA9u, 121, 2 }, { 0x001FAAu, 123, 2 }, { 0x001FABu, 125, 2 }, { 0x001FACu, 127, 2 }, { 0x001FADu, 129, 2 }, { 0x001FAEu, 131, 2 },
    { 0x001FAFu, 133, 2 }, { 0x001FB2u, 135, 2 }, { 0x001FB3u, 137, 2 }, { 0x001FB4u, 139, 2 }, { 0x001FB6u, 141, 2 }, { 0x001FB7u, 143, 3 }, { 0x001FBCu, 146, 2 }, { 0x001FC2u, 148, 2 }, { 0x001FC3u, 150, 2 }, { 0x001FC4u, 152, 2 },
    { 0x001FC6u, 154, 2 }, { 0x001FC7u, 156, 3 }, { 0x001FCCu, 159, 2 }, { 0x001FD2u, 161, 3 }, { 0x001FD3u, 164, 3 }, { 0x001FD6u, 167, 2 }, { 0x001FD7u, 169, 3 }, { 0x001FE2u, 172, 3 }, { 0x001FE3u, 175, 3 }, { 0x001FE4u, 178, 2 },
    { 0x001FE6u, 180, 2 }, { 0x001FE7u, 182, 3 }, { 0x001FF2u, 185, 2 }, { 0x001FF3u, 187, 2 }, { 0x001FF4u, 189, 2 }, { 0x001FF6u, 191, 2 }, { 0x001FF7u, 193, 3 }, { 0x001FFCu, 196, 2 }, { 0x00FB00u, 198, 2 }, { 0x00FB01u, 200, 2 },
    { 0x00FB02u, 202, 2 }, { 0x00FB03u, 204, 3 }, { 0x00FB04u, 207, 3 }, { 0x00FB05u, 210, 2 }, { 0x00FB06u, 212, 2 }, { 0x00FB13u, 214, 2 }, { 0x00FB14u, 216, 2 }, { 0x00FB15u, 218, 2 }, { 0x00FB16u, 220, 2 }, { 0x00FB17u, 222, 2 },
};


static CSTR_U32 const internal_cstr_utf8_case_fold_pool[] = {
    0x000073u, 0x000073u, 0x000069u, 0x000307u, 0x0002BCu, 0x00006Eu, 0x00006Au, 0x00030Cu, 0x0003B9u, 0x000308u, 0x000301u, 0x0003C5u, 0x000308u, 0x000301u, 0x000565u, 0x000582u, 0x000068u, 0x000331u, 0x000074u, 0x000308u, 0x000077u, 0x00030Au,
    0x000079u, 0x00030Au, 0x000061u, 0x0002BEu, 0x000073u, 0x000073u, 0x0003C5u, 0x000313u, 0x0003C5u, 0x000313u, 0x000300u, 0x0003C5u, 0x000313u, 0x000301u, 0x0003C5u, 0x000313u, 0x000342u, 0x001F00u, 0x0003B9u, 0x001F01u, 0x0003B9u, 0x001F02u,
    0x0003B9u, 0x001F03u, 0x0003B9u, 0x001F04u, 0x0003B9u, 0x001F05u, 0x0003B9u, 0x001F06u, 0x0003B9u, 0x001F07u, 0x0003B9u, 0x001F00u, 0x0003B9u, 0x001F01u, 0x0003B9u, 0x001F02u, 0x0003B9u, 0x001F03u, 0x0003B9u, 0x001F04u, 0x0003B9u, 0x001F05u,
    0x0003B9u, 0x001F06u, 0x0003B9u, 0x001F07u, 0x0003B9u, 0x001F20u, 0x0003B9u, 0x001F21u, 0x0003B9u, 0x001F22u, 0x0003B9u, 0x001F23u, 0x0003B9u, 0x001F24u, 0x0003B9u, 0x001F25u, 0x0003B9u, 0x001F26u, 0x0003B9u, 0x001F27u, 0x0003B9u, 0x001F20u,
    0x0003B9u, 0x001F21u, 0x0003B9u, 0x001F22u, 0x0003B9u, 0x001F23u, 0x0003B9u, 0x001F24u, 0x0003B9u, 0x001F25u, 0x0003B9u, 0x001F26u, 0x0003B9u, 0x001F27u, 0x0003B9u, 0x001F60u, 0x0003B9u, 0x001F61u, 0x0003B9u, 0x001F62u, 0x0003B9u, 0x001F63u,
    0x0003B9u, 0x001F64u, 0x0003B9u, 0x001F65u, 0x0003B9u, 0x001F66u, 0x0003B9u, 0x001F67u, 0x0003B9u, 0x001F60u, 0x0003B9u, 0x001F61u, 0x0003B9u, 0x001F62u, 0x0003B9u, 0x001F63u, 0x0003B9u, 0x001F64u, 0x0003B9u, 0x001F65u, 0x0003B9u, 0x001F66u,
    0x0003B9u, 0x001F67u, 0x0003B9u, 0x001F70u, 0x0003B9u, 0x0003B1u, 0x0003B9u, 0x0003ACu, 0x0003B9u, 0x0003B1u, 0x000342u, 0x0003B1u, 0x000342u, 0x0003B9u, 0x0003B1u, 0x0003B9u, 0x001F74u, 0x0003B9u, 0x0003B7u, 0x0003B9u, 0x0003AEu, 0x0003B9u,
    0x0003B7u, 0x000342u, 0x0003B7u, 0x000342u, 0x0003B9u, 0x0003B7u, 0x0003B9u, 0x0003B9u, 0x000308u, 0x000300u, 0x0003B9u, 0x000308u, 0x000301u, 0x0003B9u, 0x000342u, 0x0003B9u, 0x000308u, 0x000342u, 0x0003C5u, 0x000308u, 0x000300u, 0x0003C5u,
    0x000308u, 0x000301u, 0x0003C1u, 0x000313u, 0x0003C5u, 0x000342u, 0x0003C5u, 0x000308u, 0x000342u, 0x001F7Cu, 0x0003B9u, 0x0003C9u, 0x0003B9u, 0x0003CEu, 0x0003B9u, 0x0003C9u, 0x000342u, 0x0003C9u, 0x000342u, 0x0003B9u, 0x0003C9u, 0x0003B9u,
    0x000066u, 0x000066u, 0x000066u, 0x000069u, 0x000066u, 0x00006Cu, 0x000066u, 0x000066u, 0x000069u, 0x000066u, 0x000066u, 0x00006Cu, 0x000073u, 0x000074u, 0x000073u, 0x000074u, 0x000574u, 0x000576u, 0x000574u, 0x000565u, 0x000574u, 0x00056Bu,
    0x00057Eu, 0x000576u, 0x000574u, 0x00056Du,
};


static CSTR_BOOL_T internal_cstr_utf8_is_whitespace( CSTR_U32 cp ) {
    CSTR_SIZE_T lo = 0;
    CSTR_SIZE_T hi = sizeof( internal_cstr_utf8_whitespace ) / sizeof( internal_cstr_utf8_whitespace[ 0 ] );
    while( lo < hi ) {
        CSTR_SIZE_T mid = lo + ( ( hi - lo ) >> 1 );
        struct internal_cstr_utf8_range_t const r = internal_cstr_utf8_whitespace[ mid ];
        if( cp < r.lo ) {
            hi = mid;
        } else if( cp > r.hi ) {
            lo = mid + 1;
        } else {
            return 1;
        }
    }
    return 0;
}


static CSTR_BOOL_T internal_cstr_utf8_decode( char const* s, char const* end, CSTR_U32* cp, CSTR_SIZE_T* bytes ) {
    if( !s || s >= end ) {
        if( cp ) *cp = 0;
        if( bytes ) *bytes = 0;
        return 0;
    }

    unsigned char const b0 = (unsigned char) s[ 0 ];
    if( b0 < 0x80u ) {
        if( cp ) *cp = (CSTR_U32) b0;
        if( bytes ) *bytes = 1;
        return 1;
    }

    if( ( b0 & 0xE0u ) == 0xC0u ) {
        if( s + 2 > end ) goto invalid;
        unsigned char const b1 = (unsigned char) s[ 1 ];
        if( ( b1 & 0xC0u ) != 0x80u ) goto invalid;
        CSTR_U32 v = ( (CSTR_U32)( b0 & 0x1Fu ) << 6 ) | (CSTR_U32)( b1 & 0x3Fu );
        if( v < 0x80u ) goto invalid;
        if( cp ) *cp = v;
        if( bytes ) *bytes = 2;
        return 1;
    }

    if( ( b0 & 0xF0u ) == 0xE0u ) {
        if( s + 3 > end ) goto invalid;
        unsigned char const b1 = (unsigned char) s[ 1 ];
        unsigned char const b2 = (unsigned char) s[ 2 ];
        if( ( b1 & 0xC0u ) != 0x80u || ( b2 & 0xC0u ) != 0x80u ) goto invalid;
        CSTR_U32 v = ( (CSTR_U32)( b0 & 0x0Fu ) << 12 ) | ( (CSTR_U32)( b1 & 0x3Fu ) << 6 ) | (CSTR_U32)( b2 & 0x3Fu );
        if( v < 0x800u ) goto invalid;
        if( v >= 0xD800u && v <= 0xDFFFu ) goto invalid;
        if( cp ) *cp = v;
        if( bytes ) *bytes = 3;
        return 1;
    }

    if( ( b0 & 0xF8u ) == 0xF0u ) {
        if( s + 4 > end ) goto invalid;
        unsigned char const b1 = (unsigned char) s[ 1 ];
        unsigned char const b2 = (unsigned char) s[ 2 ];
        unsigned char const b3 = (unsigned char) s[ 3 ];
        if( ( b1 & 0xC0u ) != 0x80u || ( b2 & 0xC0u ) != 0x80u || ( b3 & 0xC0u ) != 0x80u ) goto invalid;
        CSTR_U32 v = ( (CSTR_U32)( b0 & 0x07u ) << 18 ) | ( (CSTR_U32)( b1 & 0x3Fu ) << 12 ) | ( (CSTR_U32)( b2 & 0x3Fu ) << 6 ) | (CSTR_U32)( b3 & 0x3Fu );
        if( v < 0x10000u || v > 0x10FFFFu ) goto invalid;
        if( cp ) *cp = v;
        if( bytes ) *bytes = 4;
        return 1;
    }

invalid:
    if( cp ) *cp = 0xFFFDu;
    if( bytes ) *bytes = 1;
    return 0;
}


static CSTR_SIZE_T internal_cstr_utf8_encode( CSTR_U32 cp, char* out ) {
    if( cp <= 0x7Fu ) {
        out[0] = (char)cp;
        return 1;
    } else if( cp <= 0x7FFu ) {
        out[0] = (char)(0xC0u | (cp >> 6));
        out[1] = (char)(0x80u | (cp & 0x3Fu));
        return 2;
    } else if( cp <= 0xFFFFu ) {
        out[0] = (char)(0xE0u | (cp >> 12));
        out[1] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
        out[2] = (char)(0x80u | (cp & 0x3Fu));
        return 3;
    } else {
        out[0] = (char)(0xF0u | (cp >> 18));
        out[1] = (char)(0x80u | ((cp >> 12) & 0x3Fu));
        out[2] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
        out[3] = (char)(0x80u | (cp & 0x3Fu));
        return 4;
    }
}


static char const* internal_cstr_utf8_prev( char const* begin, char const* p ) {
    if( !begin || !p || p <= begin ) {
        return begin;
    }
    char const* q = p - 1;
    for( int i = 0; i < 3 && q > begin; ++i ) {
        unsigned char const b = (unsigned char) *q;
        if( ( b & 0xC0u ) != 0x80u ) {
            break;
        }
        --q;
    }
    return q;
}


static char const* internal_cstr_utf8_advance( char const* p, char const* end, CSTR_SIZE_T count ) {
    while( p < end && count > 0 ) {
        CSTR_U32 cp;
        CSTR_SIZE_T bytes;
        internal_cstr_utf8_decode( p, end, &cp, &bytes );
        if( bytes == 0 ) break;
        p += bytes;
        --count;
    }
    return p;
}


static CSTR_SIZE_T internal_cstr_utf8_codepoint_count( char const* p, char const* end ) {
    CSTR_SIZE_T count = 0;
    while( p < end && *p ) {
        CSTR_U32 cp;
        CSTR_SIZE_T bytes;
        internal_cstr_utf8_decode( p, end, &cp, &bytes );
        if( bytes == 0 ) break;
        p += bytes;
        ++count;
    }
    return count;
}


static CSTR_SIZE_T internal_cstr_utf8_byte_offset_for_codepoint_index( char const* str, CSTR_SIZE_T byte_len, CSTR_SIZE_T index ) {
    char const* p = str;
    char const* end = str + byte_len;
    p = internal_cstr_utf8_advance( p, end, index );
    return (CSTR_SIZE_T)( p - str );
}


static unsigned char internal_cstr_ascii_upper( unsigned char c ) {
    if( c >= (unsigned char)'a' && c <= (unsigned char)'z' ) {
        return (unsigned char)( c - (unsigned char)'a' + (unsigned char)'A' );
    }
    return c;
}


static unsigned char internal_cstr_ascii_lower( unsigned char c ) {
    if( c >= (unsigned char)'A' && c <= (unsigned char)'Z' ) {
        return (unsigned char)( c - (unsigned char)'A' + (unsigned char)'a' );
    }
    return c;
}


static CSTR_U32 internal_cstr_utf8_to_upper_cp_simple( CSTR_U32 cp ) {
    {
        CSTR_SIZE_T lo = 0;
        CSTR_SIZE_T hi = sizeof(internal_cstr_utf8_to_upper_ranges) / sizeof(internal_cstr_utf8_to_upper_ranges[0]);
        while( lo < hi ) {
            CSTR_SIZE_T mid = lo + ((hi - lo) >> 1);
            CSTR_U32 rlo = internal_cstr_utf8_to_upper_ranges[mid].lo;
            CSTR_U32 rhi = internal_cstr_utf8_to_upper_ranges[mid].hi;
            if( cp < rlo ) hi = mid;
            else if( cp > rhi ) lo = mid + 1;
            else return (CSTR_U32)((int)cp + internal_cstr_utf8_to_upper_ranges[mid].delta);
        }
    }

    {
        CSTR_SIZE_T lo = 0;
        CSTR_SIZE_T hi = sizeof(internal_cstr_utf8_to_upper_pairs) / sizeof(internal_cstr_utf8_to_upper_pairs[0]);
        while( lo < hi ) {
            CSTR_SIZE_T mid = lo + ((hi - lo) >> 1);
            CSTR_U32 k = internal_cstr_utf8_to_upper_pairs[mid].cp;
            if( cp < k ) hi = mid;
            else if( cp > k ) lo = mid + 1;
            else return internal_cstr_utf8_to_upper_pairs[mid].to;
        }
    }

    return cp;
}


static unsigned char internal_cstr_utf8_to_upper_next( char const** p, char const* end, CSTR_U32 out_cp[3] ) {
    if( !p || !*p || *p >= end ) return 0;

    CSTR_U32 cp;
    CSTR_SIZE_T bytes;
    internal_cstr_utf8_decode( *p, end, &cp, &bytes );
    if( bytes == 0 ) return 0;
    *p += bytes;

    if( cp < 0x80u ) {
        out_cp[0] = (CSTR_U32)internal_cstr_ascii_upper( (unsigned char)cp );
        return 1;
    }

    {
        CSTR_SIZE_T lo = 0;
        CSTR_SIZE_T hi = sizeof(internal_cstr_utf8_to_upper_multi) / sizeof(internal_cstr_utf8_to_upper_multi[0]);
        while( lo < hi ) {
            CSTR_SIZE_T mid = lo + ((hi - lo) >> 1);
            CSTR_U32 k = internal_cstr_utf8_to_upper_multi[mid].cp;
            if( cp < k ) hi = mid;
            else if( cp > k ) lo = mid + 1;
            else {
                CSTR_U32 off = internal_cstr_utf8_to_upper_multi[mid].offset;
                unsigned char cnt = (unsigned char)internal_cstr_utf8_to_upper_multi[mid].count;
                if( cnt > 3 ) cnt = 3;
                for( unsigned char i = 0; i < cnt; ++i ) {
                    out_cp[i] = internal_cstr_utf8_to_upper_pool[ off + i ];
                }
                return cnt;
            }
        }
    }

    out_cp[0] = internal_cstr_utf8_to_upper_cp_simple( cp );
    return 1;
}


static CSTR_U32 internal_cstr_utf8_to_lower_cp_simple( CSTR_U32 cp ) {
    {
        CSTR_SIZE_T lo = 0;
        CSTR_SIZE_T hi = sizeof(internal_cstr_utf8_to_lower_ranges) / sizeof(internal_cstr_utf8_to_lower_ranges[0]);
        while( lo < hi ) {
            CSTR_SIZE_T mid = lo + ((hi - lo) >> 1);
            CSTR_U32 rlo = internal_cstr_utf8_to_lower_ranges[mid].lo;
            CSTR_U32 rhi = internal_cstr_utf8_to_lower_ranges[mid].hi;
            if( cp < rlo ) hi = mid;
            else if( cp > rhi ) lo = mid + 1;
            else return (CSTR_U32)((int)cp + internal_cstr_utf8_to_lower_ranges[mid].delta);
        }
    }

    {
        CSTR_SIZE_T lo = 0;
        CSTR_SIZE_T hi = sizeof(internal_cstr_utf8_to_lower_pairs) / sizeof(internal_cstr_utf8_to_lower_pairs[0]);
        while( lo < hi ) {
            CSTR_SIZE_T mid = lo + ((hi - lo) >> 1);
            CSTR_U32 k = internal_cstr_utf8_to_lower_pairs[mid].cp;
            if( cp < k ) hi = mid;
            else if( cp > k ) lo = mid + 1;
            else return internal_cstr_utf8_to_lower_pairs[mid].to;
        }
    }

    return cp;
}


static unsigned char internal_cstr_utf8_to_lower_next( char const** p, char const* end, CSTR_U32 out_cp[3] ) {
    if( !p || !*p || *p >= end ) return 0;

    CSTR_U32 cp;
    CSTR_SIZE_T bytes;
    internal_cstr_utf8_decode( *p, end, &cp, &bytes );
    if( bytes == 0 ) return 0;
    *p += bytes;

    if( cp < 0x80u ) {
        out_cp[0] = (CSTR_U32)internal_cstr_ascii_lower( (unsigned char)cp );
        return 1;
    }

    {
        CSTR_SIZE_T lo = 0;
        CSTR_SIZE_T hi = sizeof(internal_cstr_utf8_to_lower_multi) / sizeof(internal_cstr_utf8_to_lower_multi[0]);
        while( lo < hi ) {
            CSTR_SIZE_T mid = lo + ((hi - lo) >> 1);
            CSTR_U32 k = internal_cstr_utf8_to_lower_multi[mid].cp;
            if( cp < k ) hi = mid;
            else if( cp > k ) lo = mid + 1;
            else {
                CSTR_U32 off = internal_cstr_utf8_to_lower_multi[mid].offset;
                unsigned char cnt = (unsigned char)internal_cstr_utf8_to_lower_multi[mid].count;
                if( cnt > 3 ) cnt = 3;
                for( unsigned char i = 0; i < cnt; ++i ) {
                    out_cp[i] = internal_cstr_utf8_to_lower_pool[ off + i ];
                }
                return cnt;
            }
        }
    }

    out_cp[0] = internal_cstr_utf8_to_lower_cp_simple( cp );
    return 1;
}


static CSTR_U32 internal_cstr_utf8_casefold_cp_simple( CSTR_U32 cp ) {
    {
        CSTR_SIZE_T lo = 0;
        CSTR_SIZE_T hi = sizeof(internal_cstr_utf8_case_fold_ranges) / sizeof(internal_cstr_utf8_case_fold_ranges[0]);
        while( lo < hi ) {
            CSTR_SIZE_T mid = lo + ((hi - lo) >> 1);
            CSTR_U32 rlo = internal_cstr_utf8_case_fold_ranges[mid].lo;
            CSTR_U32 rhi = internal_cstr_utf8_case_fold_ranges[mid].hi;
            if( cp < rlo ) hi = mid;
            else if( cp > rhi ) lo = mid + 1;
            else return (CSTR_U32)((int)cp + internal_cstr_utf8_case_fold_ranges[mid].delta);
        }
    }

    {
        CSTR_SIZE_T lo = 0;
        CSTR_SIZE_T hi = sizeof(internal_cstr_utf8_case_fold_pairs) / sizeof(internal_cstr_utf8_case_fold_pairs[0]);
        while( lo < hi ) {
            CSTR_SIZE_T mid = lo + ((hi - lo) >> 1);
            CSTR_U32 k = internal_cstr_utf8_case_fold_pairs[mid].cp;
            if( cp < k ) hi = mid;
            else if( cp > k ) lo = mid + 1;
            else return internal_cstr_utf8_case_fold_pairs[mid].to;
        }
    }

    return cp;
}


static unsigned char internal_cstr_utf8_casefold_next( char const** p, char const* end, CSTR_U32 out_cp[3] ) {
    if( !p || !*p || *p >= end ) return 0;

    unsigned char raw = (unsigned char)( *p )[ 0 ];

    CSTR_U32 cp;
    CSTR_SIZE_T bytes;
    CSTR_BOOL_T valid = internal_cstr_utf8_decode( *p, end, &cp, &bytes );
    if( bytes == 0 ) return 0;
    *p += bytes;

    if( !valid ) {
        out_cp[ 0 ] = 0x110000u + (CSTR_U32)raw;
        return 1;
    }

    if( cp < 0x80u ) {
        out_cp[0] = (CSTR_U32)internal_cstr_ascii_lower( (unsigned char)cp );
        return 1;
    }

    CSTR_SIZE_T lo = 0;
    CSTR_SIZE_T hi = sizeof(internal_cstr_utf8_case_fold_multi) / sizeof(internal_cstr_utf8_case_fold_multi[0]);
    while( lo < hi ) {
        CSTR_SIZE_T mid = lo + ((hi - lo) >> 1);
        CSTR_U32 k = internal_cstr_utf8_case_fold_multi[mid].cp;
        if( cp < k ) hi = mid;
        else if( cp > k ) lo = mid + 1;
        else {
            CSTR_U32 off = internal_cstr_utf8_case_fold_multi[mid].offset;
            unsigned char cnt = (unsigned char)internal_cstr_utf8_case_fold_multi[mid].count;
            CSTR_ASSERT( cnt <= 3, "Unicode casefold expansion table entry is too large" );
            if( cnt > 3 ) cnt = 3;
            for( unsigned char i = 0; i < cnt; ++i ) {
                out_cp[i] = internal_cstr_utf8_case_fold_pool[ off + i ];
            }
            return cnt;
        }
    }

    out_cp[0] = internal_cstr_utf8_casefold_cp_simple( cp );
    return 1;
}


#endif


static CSTR_U32 internal_cstr_hash( char const* str, CSTR_SIZE_T length ) {
    CSTR_U32 m = 0x5bd1e995u;
    CSTR_U32 h = 0x31313137u;
    while( length >= 4 ) {
        CSTR_U32 k;
        CSTR_MEMCPY( &k, str, sizeof( CSTR_U32 ) );
        k *= m;
        k ^= k >> 24;
        k *= m;
        h *= m;
        h ^= k;
        str += 4;
        length -= 4;
    }
    switch( length ) {
        case 3: h ^= ((CSTR_U32)((unsigned char)str[ 2 ])) << 16;
        case 2: h ^= ((CSTR_U32)((unsigned char)str[ 1 ])) << 8;
        case 1: h ^= ((CSTR_U32)((unsigned char)str[ 0 ]));
                h *= m;
    }
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;
    return h;
}


static struct cstr_slot_t* internal_cstr_interned( struct cstri_t* cstri, char const* str ) {
    if( str ) {
        for( CSTR_SIZE_T i = 0; i < cstri->blocks_count; ++i ) {
            if( str >= cstri->blocks[ i ].head + INTERNAL_CSTR_ITEM_HEADER_SIZE && str < cstri->blocks[ i ].tail ) {
                char const* item = str - INTERNAL_CSTR_ITEM_HEADER_SIZE;
                CSTR_U32 hash = *(CSTR_U32*)( item + sizeof( CSTR_SIZE_T ) + sizeof( CSTR_SIZE_T ) );
                CSTR_SIZE_T slot = ( hash & ( cstri->hash_table_capacity - 1 ) );
                while( cstri->hash_table[ slot ].string ) {
                    if( cstri->hash_table[ slot ].string == item ) {
                        return &cstri->hash_table[ slot ];
                    }
                    slot = ( slot + 1 ) & ( cstri->hash_table_capacity - 1 );
                }
                return NULL;
            }
        }
    }
    return NULL;
}


static struct cstr_slot_t* internal_cstr_find_slot( struct cstri_t* cstri, CSTR_U32 hash, char const* str, CSTR_SIZE_T len_bytes ) {
    CSTR_SIZE_T slot = ( hash & ( cstri->hash_table_capacity - 1 ) );
    while( cstri->hash_table[ slot ].string ) {
        if( cstri->hash_table[ slot ].hash == hash && cstri->hash_table[ slot ].byte_count == len_bytes ) {
            char const* slot_string = cstri->hash_table[ slot ].string + INTERNAL_CSTR_ITEM_HEADER_SIZE;
            if( CSTR_MEMCMP( slot_string, str, len_bytes ) == 0 ) {
                break;
            }
        }
        slot = ( slot + 1 ) & ( cstri->hash_table_capacity - 1 );
    }
    return &cstri->hash_table[ slot ];
}


static char const* internal_cstr_insert( struct cstri_t* cstri, char const* str, CSTR_SIZE_T len_bytes ) {
    if( !str ) {
        str = "";
        len_bytes = 0;
    }
    CSTR_U32 hash = internal_cstr_hash( str, len_bytes );
    struct cstr_slot_t* slot = internal_cstr_find_slot( cstri, hash, str, len_bytes );
    if( slot->string ) {
        return slot->string + INTERNAL_CSTR_ITEM_HEADER_SIZE;
    }

    if( cstri->hash_table_count >= cstri->hash_table_capacity / 3 ) {
        CSTR_SIZE_T old_capacity = cstri->hash_table_capacity;
        struct cstr_slot_t* old_table = cstri->hash_table;

        cstri->hash_table_capacity *= 2;

        cstri->hash_table = (struct cstr_slot_t*) CSTR_MALLOC( cstri->memctx,
            cstri->hash_table_capacity * sizeof( *cstri->hash_table ) );
        CSTR_MEMSET( cstri->hash_table, 0, cstri->hash_table_capacity * sizeof( *cstri->hash_table ) );

        for( CSTR_SIZE_T i = 0; i < old_capacity; ++i ) {
            if( old_table[ i ].string ) {
                CSTR_U32 entry_hash = old_table[ i ].hash;
                CSTR_SIZE_T new_slot = ( entry_hash & ( cstri->hash_table_capacity - 1 ) );
                while( cstri->hash_table[ new_slot ].string ) {
                    new_slot = ( new_slot + 1 ) & ( cstri->hash_table_capacity - 1 );
                }
                cstri->hash_table[ new_slot ] = old_table[ i ];
            }
        }

        CSTR_FREE( cstri->memctx, old_table );
        slot = internal_cstr_find_slot( cstri, hash, str, len_bytes );
    }

    CSTR_SIZE_T alloc_len = ( len_bytes + 1 + (CSTR_SIZE_T)INTERNAL_CSTR_ITEM_HEADER_SIZE + 0xf ) & ~(CSTR_SIZE_T)0xf;

    struct cstr_block_t* block = NULL;
    if( cstri->blocks_count > 0 ) {
        struct cstr_block_t* b = &cstri->blocks[ cstri->blocks_count - 1 ];
        if( (CSTR_SIZE_T)( b->end - b->tail ) >= alloc_len ) {
            block = b;
        }
    }
    if( !block ) {
        if( cstri->blocks_count >= cstri->blocks_capacity ) {
            cstri->blocks_capacity *= 2;
            void* new_blocks = CSTR_MALLOC( cstri->memctx, cstri->blocks_capacity * sizeof( struct cstr_block_t ) );
            CSTR_MEMCPY( new_blocks, cstri->blocks, cstri->blocks_count * sizeof( struct cstr_block_t ) );
            CSTR_FREE( cstri->memctx, cstri->blocks );
            cstri->blocks = (struct cstr_block_t*) new_blocks;
        }
        block = &cstri->blocks[ cstri->blocks_count++ ];
        CSTR_SIZE_T size = alloc_len <= CSTR_DEFAULT_BLOCK_SIZE ? CSTR_DEFAULT_BLOCK_SIZE : alloc_len;
        block->head = (char*) CSTR_MALLOC( cstri->memctx, size );
        block->tail = block->head;
        block->end = block->head + size;
    }

    char* item = block->tail;
    block->tail += alloc_len;

    slot->hash = hash;
    slot->byte_count = len_bytes;
    #ifdef CSTR_ASCII_ONLY
        slot->codepoint_count = len_bytes;
    #else
        slot->codepoint_count = internal_cstr_utf8_codepoint_count( str, str + len_bytes );
    #endif
    slot->string = item;
    ++cstri->hash_table_count;

    *(CSTR_SIZE_T*)item = len_bytes;
    item += sizeof( CSTR_SIZE_T );
    *(CSTR_SIZE_T*)item = slot->codepoint_count;
    item += sizeof( CSTR_SIZE_T );
    *(CSTR_U32*)item = hash;
    item += sizeof( CSTR_U32 );
    CSTR_MEMCPY( item, str, len_bytes );
    item[ len_bytes ] = '\0';
    return item;
}


static char* internal_cstr_temp_buffer( struct cstri_t* cstri, CSTR_SIZE_T capacity ) {
    if( cstri->temp_capacity <= capacity ) {
        CSTR_FREE( cstri->memctx, cstri->temp_buffer );
        while( cstri->temp_capacity <= capacity ) {
            cstri->temp_capacity *= 2;
        }
        cstri->temp_buffer = (char*) CSTR_MALLOC( cstri->memctx, cstri->temp_capacity );
    }
    return cstri->temp_buffer;
}


struct cstri_t* cstri_create( void* memctx ) {
    struct cstri_t* cstri = (struct cstri_t*) CSTR_MALLOC( memctx, sizeof( struct cstri_t ) );
    cstri->memctx = memctx;
    cstri->blocks_count = 0;
    cstri->blocks_capacity = 16;
    cstri->blocks = (struct cstr_block_t*) CSTR_MALLOC( memctx, cstri->blocks_capacity * sizeof( *cstri->blocks ) );
    cstri->hash_table_count = 0;
    cstri->hash_table_capacity = 1024;
    cstri->hash_table = (struct cstr_slot_t*) CSTR_MALLOC( memctx, cstri->hash_table_capacity * sizeof( *cstri->hash_table ) );
    CSTR_MEMSET( cstri->hash_table, 0, cstri->hash_table_capacity * sizeof( *cstri->hash_table ) );
    cstri->temp_capacity = 1024;
    cstri->temp_buffer = (char*) CSTR_MALLOC( memctx, cstri->temp_capacity );
    return cstri;
}


void cstri_destroy( struct cstri_t* cstri ) {
    CSTR_FREE( cstri->memctx, cstri->temp_buffer );
    CSTR_FREE( cstri->memctx, cstri->hash_table );
    for( CSTR_SIZE_T i = 0; i < cstri->blocks_count; ++i ) {
        CSTR_FREE( cstri->memctx, cstri->blocks[ i ].head );
    }
    CSTR_FREE( cstri->memctx, cstri->blocks );
    CSTR_FREE( cstri->memctx, cstri );
}


void cstri_reset( struct cstri_t* cstri ) {
    for( CSTR_SIZE_T i = 0; i < cstri->blocks_count; ++i ) {
        CSTR_FREE( cstri->memctx, cstri->blocks[ i ].head );
    }
    cstri->blocks_count = 0;
    cstri->hash_table_count = 0;
    CSTR_MEMSET( cstri->hash_table, 0, cstri->hash_table_capacity * sizeof( *cstri->hash_table ) );
}


struct cstr_restore_point_t* cstri_restore_point( struct cstri_t* cstri ) {
    return (struct cstr_restore_point_t*)( cstri->blocks_count > 0 ? cstri->blocks[ cstri->blocks_count - 1 ].tail : NULL );
}


void cstri_rollback( struct cstri_t* cstri, struct cstr_restore_point_t* restore_point ) {
    CSTR_SIZE_T index = cstri->blocks_count;
    for( CSTR_SIZE_T i = 0; i < cstri->blocks_count; ++i ) {
        if( restore_point >= (struct cstr_restore_point_t*)cstri->blocks[ i ].head
          && restore_point <= (struct cstr_restore_point_t*)cstri->blocks[ i ].tail ) {
            index = i;
            break;
        }
    }

    if( index >= cstri->blocks_count ) {
        return;
    }

    cstri->blocks[ index ].tail = (char*)restore_point;
    for( CSTR_SIZE_T i = index + 1; i < cstri->blocks_count; ++i ) {
        CSTR_FREE( cstri->memctx, cstri->blocks[ i ].head );
    }
    cstri->blocks_count = index + 1;

    cstri->hash_table_count = 0;
    CSTR_MEMSET( cstri->hash_table, 0, cstri->hash_table_capacity * sizeof( *cstri->hash_table ) );
    for( CSTR_SIZE_T i = 0; i < cstri->blocks_count; ++i ) {
        struct cstr_block_t* block = &cstri->blocks[ i ];
        char const* ptr = block->head;
        while( ptr + (CSTR_SIZE_T)INTERNAL_CSTR_ITEM_HEADER_SIZE < block->tail ) {
            CSTR_SIZE_T byte_count = *(CSTR_SIZE_T*)ptr;
            CSTR_SIZE_T codepoint_count = *(CSTR_SIZE_T*)( ptr + sizeof( CSTR_SIZE_T ) );
            CSTR_U32 hash = *(CSTR_U32*)( ptr + sizeof( CSTR_SIZE_T ) + sizeof( CSTR_SIZE_T ) );

            CSTR_SIZE_T slot = ( hash & ( cstri->hash_table_capacity - 1 ) );
            while( cstri->hash_table[ slot ].string ) {
                slot = ( slot + 1 ) & ( cstri->hash_table_capacity - 1 );
            }
            cstri->hash_table[ slot ].hash = hash;
            cstri->hash_table[ slot ].byte_count = byte_count;
            cstri->hash_table[ slot ].codepoint_count = codepoint_count;
            cstri->hash_table[ slot ].string = ptr;
            cstri->hash_table_count++;
            ptr += ( byte_count + 1 + (CSTR_SIZE_T)INTERNAL_CSTR_ITEM_HEADER_SIZE + 0xf ) & ~(CSTR_SIZE_T)0xf;
        }
    }

}


char const* cstri( struct cstri_t* cstri, char const* str ) {
    CSTR_SIZE_T len_bytes = str ? CSTR_STRLEN( str ) : 0;
    struct cstr_slot_t* interned = internal_cstr_interned( cstri, str );
    if( interned && interned->byte_count == len_bytes ) {
        return interned->string + INTERNAL_CSTR_ITEM_HEADER_SIZE;
    }

    return internal_cstr_insert( cstri, str, len_bytes );
}


char const* cstri_n( struct cstri_t* cstri, char const* str, CSTR_SIZE_T n ) {
    CSTR_SIZE_T len_bytes = 0;

    if( str ) {
    #ifdef CSTR_ASCII_ONLY
        while( str[ len_bytes ] && len_bytes < n ) {
            ++len_bytes;
        }
    #else
        char const* p = str;
        char const* end = str + CSTR_STRLEN( str );
        CSTR_SIZE_T remaining = n;

        while( p < end && *p && remaining > 0 ) {
            CSTR_U32 cp = 0;
            CSTR_SIZE_T bytes = 0;
            internal_cstr_utf8_decode( p, end, &cp, &bytes );
            if( bytes == 0 ) break;

            p += bytes;
            --remaining;
        }

        len_bytes = (CSTR_SIZE_T)( p - str );
    #endif
    }

    struct cstr_slot_t* interned = internal_cstr_interned( cstri, str );
    if( interned && interned->byte_count == len_bytes ) {
        return interned->string + INTERNAL_CSTR_ITEM_HEADER_SIZE;
    }

    return internal_cstr_insert( cstri, str, len_bytes );
}


CSTR_BOOL_T cstri_is_interned( struct cstri_t* cstri, char const* str ) {
    if( !str ) return 0;
    struct cstr_slot_t* slot = internal_cstr_interned( cstri, str );
    return slot != NULL;
}


CSTR_SIZE_T cstri_size( struct cstri_t* cstri, char const* str ) {
    if( !str ) return 0;
    struct cstr_slot_t* slot = internal_cstr_interned( cstri, str );
    if( slot ) {
        return slot->byte_count;
    } else {
        return CSTR_STRLEN( str );
    }
}


CSTR_SIZE_T cstri_len( struct cstri_t* cstri, char const* str ) {
    if( !str ) return 0;
    struct cstr_slot_t* slot = internal_cstr_interned( cstri, str );
    if( slot ) {
        return slot->codepoint_count;
    } else {
        #ifdef CSTR_ASCII_ONLY
            return CSTR_STRLEN( str );
        #else
            CSTR_SIZE_T byte_len = CSTR_STRLEN( str );
            return internal_cstr_utf8_codepoint_count( str, str + byte_len );
        #endif
    }
}


char const* cstri_cat( struct cstri_t* cstri, char const* a, char const* b ) {
    CSTR_SIZE_T len_a = cstri_size( cstri, a );
    CSTR_SIZE_T len_b = cstri_size( cstri, b );
    CSTR_SIZE_T len = len_a + len_b;
    char* temp = internal_cstr_temp_buffer( cstri, len );
    CSTR_MEMCPY( temp, a, len_a );
    CSTR_MEMCPY( temp + len_a, b, len_b );
    return internal_cstr_insert( cstri, temp, len );
}


char const* cstri_vformat( struct cstri_t* cstri, char const* format, CSTR_VA_LIST_T args ) {
    if( !format ) {
        return internal_cstr_insert( cstri, "", 0 );
    }
    CSTR_VA_LIST_T args_copy;
    CSTR_VA_COPY( args_copy, args );
    int size = CSTR_VSNPRINTF( cstri->temp_buffer, cstri->temp_capacity, format, args_copy );
    CSTR_VA_END( args_copy );
    if( size < 0 ) {
        return NULL;
    }
    if( (CSTR_SIZE_T) size >= cstri->temp_capacity ) {
        internal_cstr_temp_buffer( cstri, (CSTR_SIZE_T)size + 1u );
        CSTR_VSNPRINTF( cstri->temp_buffer, cstri->temp_capacity, format, args );
    }
    return internal_cstr_insert( cstri, cstri->temp_buffer, (CSTR_SIZE_T)size );
}


char const* cstri_format( struct cstri_t* cstri, char const* format, ... ) {
    if( !format ) {
        return internal_cstr_insert( cstri, "", 0 );
    }
    CSTR_VA_LIST_T args;
    CSTR_VA_START( args, format );
    char const* ret = cstri_vformat( cstri, format, args );
    CSTR_VA_END( args );
    return ret;
}


char const* cstri_trim( struct cstri_t* cstri, char const* str ) {
    if( !str ) {
        return internal_cstr_insert( cstri, "", 0 );
    }
    #ifdef CSTR_ASCII_ONLY
        CSTR_SIZE_T byte_len = cstri_size( cstri, str );
        char const* start = str;
        char const* end = str + byte_len;
        while( start < end && CSTR_ISSPACE( (unsigned char)*start ) ) {
            ++start;
        }
        while( end > start && CSTR_ISSPACE( (unsigned char)*( end - 1 ) ) ) {
            --end;
        }
        return internal_cstr_insert( cstri, start, (CSTR_SIZE_T)( end - start ) );
    #else
        CSTR_SIZE_T byte_len = cstri_size( cstri, str );
        char const* start = str;
        char const* end = str + byte_len;

        while( start < end ) {
            CSTR_U32 cp;
            CSTR_SIZE_T bytes;
            internal_cstr_utf8_decode( start, end, &cp, &bytes );
            if( bytes == 0 ) break;
            if( !internal_cstr_utf8_is_whitespace( cp ) ) break;
            start += bytes;
        }

        char const* p = start;
        char const* trim_end = start;

        while( p < end ) {
            CSTR_U32 cp;
            CSTR_SIZE_T bytes;
            internal_cstr_utf8_decode( p, end, &cp, &bytes );
            if( bytes == 0 ) break;
            p += bytes;

            if( !internal_cstr_utf8_is_whitespace( cp ) ) {
                trim_end = p;
            }
        }

        return internal_cstr_insert( cstri, start, (CSTR_SIZE_T)( trim_end - start ) );
    #endif
}


char const* cstri_ltrim( struct cstri_t* cstri, char const* str ) {
    if( !str ) {
        return internal_cstr_insert( cstri, "", 0 );
    }
    #ifdef CSTR_ASCII_ONLY
        CSTR_SIZE_T byte_len = cstri_size( cstri, str );
        char const* start = str;
        char const* end = str + byte_len;
        while( start < end && CSTR_ISSPACE( (unsigned char)*start ) ) {
            ++start;
        }
        return internal_cstr_insert( cstri, start, (CSTR_SIZE_T)( end - start ) );
    #else
        CSTR_SIZE_T byte_len = cstri_size( cstri, str );
        char const* start = str;
        char const* end = str + byte_len;

        while( start < end ) {
            CSTR_U32 cp;
            CSTR_SIZE_T bytes;
            internal_cstr_utf8_decode( start, end, &cp, &bytes );
            if( bytes == 0 ) break;
            if( !internal_cstr_utf8_is_whitespace( cp ) ) break;
            start += bytes;
        }

        return internal_cstr_insert( cstri, start, (CSTR_SIZE_T)( end - start ) );
    #endif
}


char const* cstri_rtrim( struct cstri_t* cstri, char const* str ) {
    if( !str ) {
        return internal_cstr_insert( cstri, "", 0 );
    }
    #ifdef CSTR_ASCII_ONLY
        CSTR_SIZE_T byte_len = cstri_size( cstri, str );
        char const* start = str;
        char const* end = str + byte_len;
        while( end > start && CSTR_ISSPACE( (unsigned char)*( end - 1 ) ) ) {
            --end;
        }
        return internal_cstr_insert( cstri, start, (CSTR_SIZE_T)( end - start ) );
    #else
        CSTR_SIZE_T byte_len = cstri_size( cstri, str );
        char const* start = str;
        char const* end = str + byte_len;
        char const* p = start;
        char const* trim_end = start;

        while( p < end ) {
            CSTR_U32 cp;
            CSTR_SIZE_T bytes;
            internal_cstr_utf8_decode( p, end, &cp, &bytes );
            if( bytes == 0 ) break;
            p += bytes;

            if( !internal_cstr_utf8_is_whitespace( cp ) ) {
                trim_end = p;
            }
        }

        return internal_cstr_insert( cstri, start, (CSTR_SIZE_T)( trim_end - start ) );
    #endif
}


char const* cstri_left( struct cstri_t* cstri, char const* str, CSTR_SIZE_T n ) {
    if( !str ) {
        return internal_cstr_insert( cstri, "", 0 );
    }
    #ifdef CSTR_ASCII_ONLY
        CSTR_SIZE_T byte_len = cstri_size( cstri, str );
        if( byte_len <= n ) {
            return internal_cstr_insert( cstri, str, byte_len );
        }
        return internal_cstr_insert( cstri, str, n );
    #else
        CSTR_SIZE_T byte_len = cstri_size( cstri, str );
        CSTR_SIZE_T off = internal_cstr_utf8_byte_offset_for_codepoint_index( str, byte_len, n );
        return internal_cstr_insert( cstri, str, off );
    #endif
}


char const* cstri_right( struct cstri_t* cstri, char const* str, CSTR_SIZE_T n ) {
    if( !str ) {
        return internal_cstr_insert( cstri, "", 0 );
    }
    #ifdef CSTR_ASCII_ONLY
        CSTR_SIZE_T byte_len = cstri_size( cstri, str );
        if( byte_len <= n ) {
            return internal_cstr_insert( cstri, str, byte_len );
        }
        return internal_cstr_insert( cstri, str + byte_len - n, n );
    #else
        CSTR_SIZE_T byte_len = cstri_size( cstri, str );
        CSTR_SIZE_T total_cp = cstri_len( cstri, str );
        if( total_cp <= n ) {
            return internal_cstr_insert( cstri, str, byte_len );
        }
        CSTR_SIZE_T off = internal_cstr_utf8_byte_offset_for_codepoint_index( str, byte_len, total_cp - n );
        return internal_cstr_insert( cstri, str + off, byte_len - off );
    #endif
}


char const* cstri_mid( struct cstri_t* cstri, char const* str, CSTR_SIZE_T start, CSTR_SIZE_T n ) {
    if( !str ) {
        return internal_cstr_insert( cstri, "", 0 );
    }
    #ifdef CSTR_ASCII_ONLY
        CSTR_SIZE_T byte_len = cstri_size( cstri, str );
        if( byte_len <= start ) {
            return internal_cstr_insert( cstri, "", 0 );
        }
        if( byte_len - start <= n || n == 0 ) {
            return internal_cstr_insert( cstri, str + start, byte_len - start );
        }
        return internal_cstr_insert( cstri, str + start, n );
    #else
        CSTR_SIZE_T byte_len = cstri_size( cstri, str );
        CSTR_SIZE_T off0 = internal_cstr_utf8_byte_offset_for_codepoint_index(str, byte_len, start);
        if( off0 >= byte_len ) return internal_cstr_insert(cstri, "", 0);
        if( n == 0 ) return internal_cstr_insert(cstri, str + off0, byte_len - off0);

        CSTR_SIZE_T off1 = internal_cstr_utf8_byte_offset_for_codepoint_index(str + off0, byte_len - off0, n);
        return internal_cstr_insert(cstri, str + off0, off1);
    #endif
}


char const* cstri_upper( struct cstri_t* cstri, char const* str ) {
    if( !str ) return internal_cstr_insert( cstri, "", 0 );

    CSTR_SIZE_T byte_len = cstri_size( cstri, str );

#ifdef CSTR_ASCII_ONLY
    char* temp = internal_cstr_temp_buffer( cstri, byte_len );
    for( CSTR_SIZE_T i = 0; i < byte_len; ++i ) temp[i] = (char)CSTR_TOUPPER( str[i] );
    return internal_cstr_insert( cstri, temp, byte_len );
#else
    char* temp = internal_cstr_temp_buffer( cstri, byte_len * 12 + 1 );
    char const* p = str;
    char const* end = str + byte_len;
    CSTR_SIZE_T out_len = 0;

    CSTR_U32 buf[3];
    unsigned char nbuf = 0;
    unsigned char ibuf = 0;

    for( ;; ) {
        if( ibuf >= nbuf ) {
            ibuf = 0;
            nbuf = internal_cstr_utf8_to_upper_next( &p, end, buf );
        }
        if( nbuf == 0 ) break;

        CSTR_U32 cp = buf[ ibuf++ ];
        out_len += internal_cstr_utf8_encode( cp, temp + out_len );
    }

    return internal_cstr_insert( cstri, temp, out_len );
#endif

}


char const* cstri_lower( struct cstri_t* cstri, char const* str ) {
    if( !str ) return internal_cstr_insert( cstri, "", 0 );

    CSTR_SIZE_T byte_len = cstri_size( cstri, str );

#ifdef CSTR_ASCII_ONLY
    char* temp = internal_cstr_temp_buffer( cstri, byte_len );
    for( CSTR_SIZE_T i = 0; i < byte_len; ++i ) temp[i] = (char)CSTR_TOLOWER( str[i] );
    return internal_cstr_insert( cstri, temp, byte_len );
#else
    char* temp = internal_cstr_temp_buffer( cstri, byte_len * 12 + 1 );
    char const* p = str;
    char const* end = str + byte_len;
    CSTR_SIZE_T out_len = 0;

    CSTR_U32 buf[3];
    unsigned char nbuf = 0;
    unsigned char ibuf = 0;

    for( ;; ) {
        if( ibuf >= nbuf ) {
            ibuf = 0;
            nbuf = internal_cstr_utf8_to_lower_next( &p, end, buf );
        }
        if( nbuf == 0 ) break;

        CSTR_U32 cp = buf[ ibuf++ ];
        out_len += internal_cstr_utf8_encode( cp, temp + out_len );
    }

    return internal_cstr_insert( cstri, temp, out_len );
#endif
}


char const* cstri_lpad( struct cstri_t* cstri, char const* str, char padding, CSTR_SIZE_T total_max_length ) {
    CSTR_SIZE_T cp_len = cstri_len( cstri, str );
    if( cp_len >= total_max_length ) {
        CSTR_SIZE_T byte_len = cstri_size( cstri, str );
        return internal_cstr_insert( cstri, str, byte_len );
    }
    CSTR_SIZE_T byte_len = cstri_size( cstri, str );
    CSTR_SIZE_T pad_len = total_max_length - cp_len;
    CSTR_SIZE_T new_len = byte_len + pad_len;
    char* temp = internal_cstr_temp_buffer( cstri, new_len );
    CSTR_MEMSET( temp, padding, pad_len );
    if( byte_len > 0 ) {
        CSTR_MEMCPY( temp + pad_len, str, byte_len );
    }
    return internal_cstr_insert( cstri, temp, new_len );
}


char const* cstri_rpad( struct cstri_t* cstri, char const* str, char padding, CSTR_SIZE_T total_max_length ) {
    CSTR_SIZE_T cp_len = cstri_len( cstri, str );
    if( cp_len >= total_max_length ) {
        CSTR_SIZE_T byte_len = cstri_size( cstri, str );
        return internal_cstr_insert( cstri, str, byte_len );
    }
    CSTR_SIZE_T byte_len = cstri_size( cstri, str );
    CSTR_SIZE_T pad_len = total_max_length - cp_len;
    CSTR_SIZE_T new_len = byte_len + pad_len;
    char* temp = internal_cstr_temp_buffer( cstri, new_len );
    if( byte_len > 0 ) {
        CSTR_MEMCPY( temp, str, byte_len );
    }
    CSTR_MEMSET( temp + byte_len, padding, pad_len );
    return internal_cstr_insert( cstri, temp, new_len );
}


char const* cstri_join( struct cstri_t* cstri, char const* a, char const* b, char const* separator ) {
    CSTR_SIZE_T len_sep = cstri_size( cstri, separator );
    if( len_sep == 0 ) {
        return cstri_cat( cstri, a, b );
    }
    CSTR_SIZE_T len_a = cstri_size( cstri, a );
    CSTR_SIZE_T len_b = cstri_size( cstri, b );
    if( len_a == 0 || len_b == 0 ) {
        return cstri_cat( cstri, a, b );
    }
    if( len_a >= len_sep && CSTR_MEMCMP( a + len_a - len_sep, separator, len_sep ) == 0 ) {
        return cstri_cat( cstri, a, b );
    }
    if( len_b >= len_sep && CSTR_MEMCMP( b, separator, len_sep ) == 0 ) {
        return cstri_cat( cstri, a, b );
    }
    CSTR_SIZE_T len = len_a + len_sep + len_b;
    char* temp = internal_cstr_temp_buffer( cstri, len );
    CSTR_MEMCPY( temp, a, len_a );
    CSTR_MEMCPY( temp + len_a, separator, len_sep );
    CSTR_MEMCPY( temp + len_a + len_sep, b, len_b );
    return internal_cstr_insert( cstri, temp, len );
}


char const* cstri_replace( struct cstri_t* cstri, char const* str, char const* find, char const* replacement ) {
    CSTR_SIZE_T len_str = cstri_size( cstri, str );
    if( len_str == 0 ) return internal_cstr_insert( cstri, "", 0 );

    CSTR_SIZE_T len_find = cstri_size( cstri, find );
    if( len_find == 0 ) return internal_cstr_insert( cstri, str, len_str );

    CSTR_SIZE_T len_rep = cstri_size( cstri, replacement );

    #ifdef CSTR_ASCII_ONLY
        CSTR_SIZE_T count = 0;
        char const* p = CSTR_STRSTR( str, find );
        while( p ) {
            ++count;
            p += len_find;
            p = CSTR_STRSTR( p, find );
        }
        if( count == 0 ) {
            return internal_cstr_insert( cstri, str, len_str );
        }
        CSTR_SIZE_T new_len = len_str - count * len_find + count * len_rep;
        char* temp = internal_cstr_temp_buffer( cstri, new_len );
        char* dst = temp;
        char const* src = str;
        p = CSTR_STRSTR( src, find );
        while( p ) {
            CSTR_SIZE_T chunk = (CSTR_SIZE_T)( p - src );
            if( chunk ) {
                CSTR_MEMCPY( dst, src, chunk );
                dst += chunk;
            }
            if( len_rep ) {
                CSTR_MEMCPY( dst, replacement, len_rep );
                dst += len_rep;
            }
            src = p + len_find;
            p = CSTR_STRSTR( src, find );
        }
        CSTR_SIZE_T tail = len_str - (CSTR_SIZE_T)( src - str );
        if( tail ) {
            CSTR_MEMCPY( dst, src, tail );
        }

        return internal_cstr_insert( cstri, temp, new_len );
    #else
        char const* end = str + len_str;

        CSTR_SIZE_T count = 0;
        for( char const* p = str; p + len_find <= end; ) {
            if( CSTR_MEMCMP( p, find, len_find ) == 0 ) {
                ++count;
                p += len_find;
                continue;
            }
            CSTR_U32 dummy; CSTR_SIZE_T bytes = 0;
            internal_cstr_utf8_decode( p, end, &dummy, &bytes );
            if( bytes == 0 ) break;
            p += bytes;
        }

        if( count == 0 ) return internal_cstr_insert( cstri, str, len_str );

        CSTR_SIZE_T new_len = len_str - count * len_find + count * len_rep;
        char* temp = internal_cstr_temp_buffer( cstri, new_len );

        char* dst = temp;
        char const* src = str;

        for( char const* p = str; p + len_find <= end; ) {
            if( CSTR_MEMCMP( p, find, len_find ) == 0 ) {
                CSTR_SIZE_T chunk = (CSTR_SIZE_T)( p - src );
                if( chunk ) { CSTR_MEMCPY( dst, src, chunk ); dst += chunk; }
                if( len_rep ) { CSTR_MEMCPY( dst, replacement, len_rep ); dst += len_rep; }

                src = p + len_find;
                p = src;
                continue;
            }
            CSTR_U32 dummy; CSTR_SIZE_T bytes = 0;
            internal_cstr_utf8_decode( p, end, &dummy, &bytes );
            if( bytes == 0 ) break;
            p += bytes;
        }

        CSTR_SIZE_T tail = (CSTR_SIZE_T)( end - src );
        if( tail ) CSTR_MEMCPY( dst, src, tail );

        return internal_cstr_insert( cstri, temp, new_len );
    #endif
}


char const* cstri_insert( struct cstri_t* cstri, char const* str, int position, char const* insertion ) {
    CSTR_SIZE_T len_str = cstri_size( cstri, str );
    CSTR_SIZE_T len_ins = cstri_size( cstri, insertion );
    if( len_ins == 0 ) {
        return internal_cstr_insert( cstri, str, len_str );
    }
    if( position < 0 ) {
        position = 0;
    }
    #ifdef CSTR_ASCII_ONLY
        if( (CSTR_SIZE_T)position > len_str ) {
            position = (int)len_str;
        }
        CSTR_SIZE_T new_len = len_str + len_ins;
        char* temp = internal_cstr_temp_buffer( cstri, new_len );
        if( position > 0 ) {
            CSTR_MEMCPY( temp, str, (CSTR_SIZE_T)position );
        }
        CSTR_MEMCPY( temp + position, insertion, len_ins );
        if( (CSTR_SIZE_T)position < len_str ) {
            CSTR_MEMCPY( temp + position + len_ins, str + position, len_str - (CSTR_SIZE_T)position );
        }
        return internal_cstr_insert( cstri, temp, new_len );
    #else
        CSTR_SIZE_T pos_byte = internal_cstr_utf8_byte_offset_for_codepoint_index( str ? str : "", len_str, (CSTR_SIZE_T)position );
        if( pos_byte > len_str ) pos_byte = len_str;
        CSTR_SIZE_T new_len = len_str + len_ins;
        char* temp = internal_cstr_temp_buffer( cstri, new_len );
        if( pos_byte > 0 ) {
            CSTR_MEMCPY( temp, str, pos_byte );
        }
        CSTR_MEMCPY( temp + pos_byte, insertion, len_ins );
        if( pos_byte < len_str ) {
            CSTR_MEMCPY( temp + pos_byte + len_ins, str + pos_byte, len_str - pos_byte );
        }
        return internal_cstr_insert( cstri, temp, new_len );
    #endif
}


char const* cstri_remove( struct cstri_t* cstri, char const* str, int start, int length ) {
    if( start < 0 ) {
        length += start;
        start = 0;
    }
    CSTR_SIZE_T len_str = cstri_size( cstri, str );
    if( len_str == 0 || length <= 0 ) {
        return internal_cstr_insert( cstri, str, len_str );
    }
    #ifdef CSTR_ASCII_ONLY
        if( (CSTR_SIZE_T) start >= len_str ) {
            return internal_cstr_insert( cstri, str, len_str );
        }
        CSTR_SIZE_T head = (CSTR_SIZE_T) start;
        if( head + (CSTR_SIZE_T) length > len_str ) {
            length = (int)( len_str - head );
        }
        CSTR_SIZE_T new_len = len_str - (CSTR_SIZE_T) length;
        char* temp = internal_cstr_temp_buffer( cstri, new_len );
        if( head > 0 ) {
            CSTR_MEMCPY( temp, str, head );
        }
        CSTR_SIZE_T tail = len_str - head - (CSTR_SIZE_T) length;
        if( tail > 0 ) {
            CSTR_MEMCPY( temp + head, str + head + length, tail );
        }
        return internal_cstr_insert( cstri, temp, new_len );
    #else
        CSTR_SIZE_T start_byte = internal_cstr_utf8_byte_offset_for_codepoint_index( str ? str : "", len_str, (CSTR_SIZE_T)start );
        if( start_byte >= len_str ) {
            return internal_cstr_insert( cstri, str, len_str );
        }
        CSTR_SIZE_T rem_byte = internal_cstr_utf8_byte_offset_for_codepoint_index( str + start_byte, len_str - start_byte, (CSTR_SIZE_T)length );
        if( rem_byte > len_str - start_byte ) rem_byte = len_str - start_byte;

        CSTR_SIZE_T new_len = len_str - rem_byte;
        char* temp = internal_cstr_temp_buffer( cstri, new_len );
        if( start_byte > 0 ) {
            CSTR_MEMCPY( temp, str, start_byte );
        }
        CSTR_SIZE_T tail = len_str - start_byte - rem_byte;
        if( tail > 0 ) {
            CSTR_MEMCPY( temp + start_byte, str + start_byte + rem_byte, tail );
        }
        return internal_cstr_insert( cstri, temp, new_len );
    #endif
}


char const* cstri_int( struct cstri_t* cstri, int i ) {
    return cstri_format( cstri, "%d", i );
}


char const* cstri_float( struct cstri_t* cstri, float f ) {
    return cstri_format( cstri, "%f", f );
}


CSTR_BOOL_T cstri_starts( struct cstri_t* cstri, char const* str, char const* start ) {
    CSTR_SIZE_T str_len = cstri_size( cstri, str );
    CSTR_SIZE_T start_len = cstri_size( cstri, start );
    return str_len >= start_len && CSTR_MEMCMP( str, start, start_len ) == 0;
}


CSTR_BOOL_T cstri_ends( struct cstri_t* cstri, char const* str, char const* end ) {
    CSTR_SIZE_T str_len = cstri_size( cstri, str );
    CSTR_SIZE_T end_len = cstri_size( cstri, end );
    return str_len >= end_len && CSTR_MEMCMP( str + str_len - end_len, end, end_len ) == 0;
}


CSTR_BOOL_T cstri_is_equal( struct cstri_t* cstri, char const* a, char const* b ) {
    if( a == b ) {
        return 1;
    }
    CSTR_SIZE_T len_a = cstri_size( cstri, a );
    CSTR_SIZE_T len_b = cstri_size( cstri, b );
    if( len_a != len_b ) {
        return 0;
    }
    return CSTR_MEMCMP( a, b, len_a ) == 0;
}


int cstri_compare( struct cstri_t* cstri, char const* a, char const* b ) {
    if( a == b ) {
        return 0;
    }
    CSTR_SIZE_T len_a = cstri_size( cstri, a );
    CSTR_SIZE_T len_b = cstri_size( cstri, b );
    CSTR_SIZE_T min_len = len_a < len_b ? len_a : len_b;
    return CSTR_MEMCMP( a ? a : "", b ? b : "", min_len + 1 );
}


int cstri_compare_nocase( struct cstri_t* cstri, char const* a, char const* b ) {
    if( a == b ) return 0;

#ifdef CSTR_ASCII_ONLY
    CSTR_SIZE_T len_a = cstri_size( cstri, a );
    CSTR_SIZE_T len_b = cstri_size( cstri, b );
    CSTR_SIZE_T min_len = len_a < len_b ? len_a : len_b;
    return CSTR_STRNICMP( a ? a : "", b ? b : "", min_len + 1 );
#else
    char const* pa = a ? a : "";
    char const* pb = b ? b : "";
    CSTR_SIZE_T la = cstri_size( cstri, a );
    CSTR_SIZE_T lb = cstri_size( cstri, b );
    char const* ea = pa + la;
    char const* eb = pb + lb;

    CSTR_U32 buf_a[3], buf_b[3];
    unsigned char na = 0, nb = 0;
    unsigned char ia = 0, ib = 0;

    for( ;; ) {
        if( ia >= na ) {
            ia = 0;
            na = internal_cstr_utf8_casefold_next( &pa, ea, buf_a );
        }
        if( ib >= nb ) {
            ib = 0;
            nb = internal_cstr_utf8_casefold_next( &pb, eb, buf_b );
        }

        if( na == 0 && nb == 0 ) return 0;
        if( na == 0 ) return -1;
        if( nb == 0 ) return 1;

        CSTR_U32 ca = buf_a[ia++];
        CSTR_U32 cb = buf_b[ib++];

        if( ca != cb ) return (ca < cb) ? -1 : 1;
    }
#endif
}


int cstri_find( struct cstri_t* cstri, char const* str, char const* find, int start ) {
    if( !str || !find || start < 0 ) return -1;

    CSTR_SIZE_T len_str_bytes  = cstri_size( cstri, str );

    #ifdef CSTR_ASCII_ONLY
        if( (CSTR_SIZE_T)start >= len_str_bytes ) return -1;
        char const* res = CSTR_STRSTR( str + start, find );
        return res ? (int)( res - str ) : -1;
    #else
        CSTR_SIZE_T len_find_bytes = cstri_size( cstri, find );

        if( (CSTR_SIZE_T)start >= cstri_len( cstri, str ) ) return -1;

        CSTR_SIZE_T start_byte = internal_cstr_utf8_byte_offset_for_codepoint_index( str, len_str_bytes, (CSTR_SIZE_T)start );
        if( start_byte >= len_str_bytes ) return -1;

        if( len_find_bytes == 0 ) return start;

        char const* p   = str + start_byte;
        char const* end = str + len_str_bytes;
        CSTR_SIZE_T cp  = (CSTR_SIZE_T)start;

        while( p + len_find_bytes <= end ) {
            if( CSTR_MEMCMP( p, find, len_find_bytes ) == 0 ) return (int)cp;

            CSTR_U32 dummy;
            CSTR_SIZE_T bytes = 0;
            internal_cstr_utf8_decode( p, end, &dummy, &bytes );
            if( bytes == 0 ) break;
            p += bytes;
            ++cp;
        }
        return -1;
    #endif
}


int cstri_rfind( struct cstri_t* cstri, char const* str, char const* find, int end ) {
    CSTR_SIZE_T len_str_bytes  = cstri_size( cstri, str );
    CSTR_SIZE_T len_find_bytes = cstri_size( cstri, find );
    if( len_str_bytes == 0 || len_find_bytes > len_str_bytes ) return -1;

    #ifdef CSTR_ASCII_ONLY
        if( len_find_bytes == 0 ) {
            if( end < 0 ) return end;
            CSTR_SIZE_T cp_count = cstri_len( cstri, str );
            return ( (CSTR_SIZE_T)end <= cp_count ) ? end : (int)cp_count;
        }
        CSTR_SIZE_T max_end = len_str_bytes - len_find_bytes;
        for( int i = ( end <= 0 || (CSTR_SIZE_T)end > max_end ) ? (int)max_end : end; i >= 0; --i ) {
            if( CSTR_MEMCMP( str + (CSTR_SIZE_T)i, find, len_find_bytes ) == 0 ) return i;
        }
        return -1;
    #else
        if( len_find_bytes == 0 ) {
            if( end < 0 ) return end;
            CSTR_SIZE_T cp_count = cstri_len( cstri, str );
            return ( (CSTR_SIZE_T)end <= cp_count ) ? end : (int)cp_count;
        }

        int last = -1;
        char const* p   = str;
        char const* endp = str + len_str_bytes;
        CSTR_SIZE_T cp = 0;

        while( p + len_find_bytes <= endp ) {
            if( end > 0 && cp > (CSTR_SIZE_T)end ) break;

            if( CSTR_MEMCMP( p, find, len_find_bytes ) == 0 ) last = (int)cp;

            CSTR_U32 dummy;
            CSTR_SIZE_T bytes = 0;
            internal_cstr_utf8_decode( p, endp, &dummy, &bytes );
            if( bytes == 0 ) break;
            p += bytes;
            ++cp;
        }
        return last;
    #endif
}


CSTR_U32 cstri_hash( struct cstri_t* cstri, char const* str ) {
    struct cstr_slot_t* slot = internal_cstr_interned( cstri, str );
    if( slot ) {
        return slot->hash;
    } else {
        return internal_cstr_hash( str ? str : "", str ? CSTR_STRLEN( str ) : 0 );
    }
}


#ifndef CSTR_ASCII_ONLY
static CSTR_BOOL_T internal_cstr_utf8_cp_in_separators( CSTR_U32 cp, char const* separators, CSTR_SIZE_T sep_len_bytes ) {
    if( !separators || sep_len_bytes == 0 ) return 0;

    char const* p   = separators;
    char const* end = separators + sep_len_bytes;

    while( p < end ) {
        CSTR_U32 scp = 0;
        CSTR_SIZE_T bytes = 0;
        internal_cstr_utf8_decode( p, end, &scp, &bytes );
        if( bytes == 0 ) break;
        if( scp == cp ) return 1;
        p += bytes;
    }
    return 0;
}
#endif


struct cstr_tokenizer_t cstri_tokenizer( struct cstri_t* cstri, char const* str ) {
    struct cstr_tokenizer_t tokenizer;
    tokenizer.internal = (void*)internal_cstr_insert( cstri, str, str ? CSTR_STRLEN( str ) : 0 );
    return tokenizer;
}


char const* cstri_tokenize( struct cstri_t* cstri, struct cstr_tokenizer_t* tokenizer, char const* separators ) {
    CSTR_SIZE_T sep_len_bytes = cstri_size( cstri, separators );
    char const* pos = (char const*) tokenizer->internal;

    #ifdef CSTR_ASCII_ONLY
        while( *pos ) {
            char ch = *pos;
            CSTR_BOOL_T is_sep = 0;
            for( CSTR_SIZE_T i = 0; i < sep_len_bytes; ++i ) {
                if( ch == separators[ i ] ) { is_sep = 1; break; }
            }
            if( !is_sep ) break;
            ++pos;
        }

        char const* token = pos;
        while( *pos ) {
            char ch = *pos;
            CSTR_BOOL_T is_sep = 0;
            for( CSTR_SIZE_T i = 0; i < sep_len_bytes; ++i ) {
                if( ch == separators[ i ] ) { is_sep = 1; break; }
            }
            if( is_sep ) break;
            ++pos;
        }

        tokenizer->internal = (void*) pos;

        if( *token == '\0' ) return NULL;
        return internal_cstr_insert( cstri, token, (CSTR_SIZE_T)( pos - token ) );
    #else
        if( !pos ) pos = "";

        char const* end = pos + CSTR_STRLEN( pos );

        while( pos < end && *pos ) {
            CSTR_U32 cp = 0;
            CSTR_SIZE_T bytes = 0;
            internal_cstr_utf8_decode( pos, end, &cp, &bytes );
            if( bytes == 0 ) break;

            if( !internal_cstr_utf8_cp_in_separators( cp, separators, sep_len_bytes ) ) break;
            pos += bytes;
        }

        char const* token = pos;
        while( pos < end && *pos ) {
            CSTR_U32 cp = 0;
            CSTR_SIZE_T bytes = 0;
            internal_cstr_utf8_decode( pos, end, &cp, &bytes );
            if( bytes == 0 ) break;

            if( internal_cstr_utf8_cp_in_separators( cp, separators, sep_len_bytes ) ) break;
            pos += bytes;
        }

        tokenizer->internal = (void*) pos;

        if( *token == '\0' ) return NULL;
        return internal_cstr_insert( cstri, token, (CSTR_SIZE_T)( pos - token ) );
    #endif
}

char* cstri_temp_buffer( struct cstri_t* cstri, CSTR_SIZE_T capacity ) {
    return internal_cstr_temp_buffer( cstri, capacity );
}


CSTR_SIZE_T cstri_temp_buffer_capacity( struct cstri_t* cstri ) {
    return cstri->temp_capacity - 1;
}


//// global api

#ifndef CSTR_NO_GLOBAL_API


static struct cstri_t* g_internal_cstr = NULL;


void internal_cstr_cleanup( void ) {
    if( g_internal_cstr ) {
        cstri_destroy( g_internal_cstr );
        g_internal_cstr = NULL;
    }
}


static void internal_cstr_instance( void ) {
    if( !g_internal_cstr ) {
        g_internal_cstr = cstri_create( CSTR_GLOBAL_API_MEMCTX );
        static int atexit_set = 0;
        if( !atexit_set ) {
            #ifndef __wasm__
            atexit( internal_cstr_cleanup );
            #endif
            atexit_set = 1;
        }
    }
}


void cstr_reset( void ) {
    CSTR_MUTEX_LOCK();
    internal_cstr_cleanup();
    CSTR_MUTEX_UNLOCK();
}


struct cstr_restore_point_t* cstr_restore_point( void ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    struct cstr_restore_point_t* ret = cstri_restore_point( g_internal_cstr );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


void cstr_rollback( struct cstr_restore_point_t* restore_point ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    cstri_rollback( g_internal_cstr, restore_point );
    CSTR_MUTEX_UNLOCK();
}


char const* cstr( char const* str ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    char const* ret = cstri( g_internal_cstr, str );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


char const* cstr_n( char const* str, CSTR_SIZE_T n ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    char const* ret = cstri_n( g_internal_cstr, str, n );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


CSTR_BOOL_T cstr_is_interned( char const* str ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    CSTR_BOOL_T ret = cstri_is_interned( g_internal_cstr, str );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


CSTR_SIZE_T cstr_len( char const* str ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    CSTR_SIZE_T ret = cstri_len( g_internal_cstr, str );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


CSTR_SIZE_T cstr_size( char const* str ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    CSTR_SIZE_T ret = cstri_size( g_internal_cstr, str );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


char const* cstr_cat( char const* a, char const* b ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    char const* ret = cstri_cat( g_internal_cstr, a, b );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


char const* cstr_vformat( char const* format, CSTR_VA_LIST_T args ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    char const* ret = cstri_vformat( g_internal_cstr, format, args );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


char const* cstr_format( char const* format, ... ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    CSTR_VA_LIST_T args;
    CSTR_VA_START( args, format );
    char const* ret = cstri_vformat( g_internal_cstr, format, args );
    CSTR_VA_END( args );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


char const* cstr_trim( char const* str ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    char const* ret = cstri_trim( g_internal_cstr, str );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


char const* cstr_ltrim( char const* str ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    char const* ret = cstri_ltrim( g_internal_cstr, str );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


char const* cstr_rtrim( char const* str ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    char const* ret = cstri_rtrim( g_internal_cstr, str );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


char const* cstr_left( char const* str, CSTR_SIZE_T n ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    char const* ret = cstri_left( g_internal_cstr, str, n );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


char const* cstr_right( char const* str, CSTR_SIZE_T n ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    char const* ret = cstri_right( g_internal_cstr, str, n );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


char const* cstr_mid( char const* str, CSTR_SIZE_T start, CSTR_SIZE_T n ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    char const* ret = cstri_mid( g_internal_cstr, str, start, n );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


char const* cstr_upper( char const* str ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    char const* ret = cstri_upper( g_internal_cstr, str );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


char const* cstr_lower( char const* str ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    char const* ret = cstri_lower( g_internal_cstr, str );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


char const* cstr_lpad( char const* str, char padding, CSTR_SIZE_T total_max_length ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    char const* ret = cstri_lpad( g_internal_cstr, str, padding, total_max_length );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


char const* cstr_rpad( char const* str, char padding, CSTR_SIZE_T total_max_length ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    char const* ret = cstri_rpad( g_internal_cstr, str, padding, total_max_length );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


char const* cstr_join( char const* a, char const* b, char const* separator ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    char const* ret = cstri_join( g_internal_cstr, a, b, separator );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


char const* cstr_replace( char const* str, char const* find, char const* replacement ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    char const* ret = cstri_replace( g_internal_cstr, str, find, replacement );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


char const* cstr_insert( char const* str, int position, char const* insertion ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    char const* ret = cstri_insert( g_internal_cstr, str, position,insertion );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


char const* cstr_remove( char const* str, int start, int length ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    char const* ret = cstri_remove( g_internal_cstr, str, start, length );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


char const* cstr_int( int i ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    char const* ret = cstri_int( g_internal_cstr, i );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


char const* cstr_float( float f ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    char const* ret = cstri_float( g_internal_cstr, f );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


CSTR_BOOL_T cstr_starts( char const* str, char const* start ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    CSTR_BOOL_T ret = cstri_starts( g_internal_cstr, str, start );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


CSTR_BOOL_T cstr_ends( char const* str, char const* end ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    CSTR_BOOL_T ret = cstri_ends( g_internal_cstr, str, end );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


CSTR_BOOL_T cstr_is_equal( char const* a, char const* b ) {
    if( a == b ) {
        return 1;
    }
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    int ret = cstri_is_equal( g_internal_cstr, a, b );
    CSTR_MUTEX_UNLOCK();
    return (CSTR_BOOL_T)ret;
}


int cstr_compare( char const* a, char const* b ) {
    if( a == b ) {
        return 0;
    }
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    int ret = cstri_compare( g_internal_cstr, a, b );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


int cstr_compare_nocase( char const* a, char const* b ) {
    if( a == b ) {
        return 0;
    }
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    int ret = cstri_compare_nocase( g_internal_cstr, a, b );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


int cstr_find( char const* str, char const* find, int start ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    int ret = cstri_find( g_internal_cstr, str, find, start );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


int cstr_rfind( char const* str, char const* find, int end ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    int ret = cstri_rfind( g_internal_cstr, str, find, end );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


CSTR_U32 cstr_hash( char const* str ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    CSTR_U32  ret = cstri_hash( g_internal_cstr, str );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


struct cstr_tokenizer_t cstr_tokenizer( char const* str ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    struct cstr_tokenizer_t ret = cstri_tokenizer( g_internal_cstr, str );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


char const* cstr_tokenize( struct cstr_tokenizer_t* tokenizer, char const* separators ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    char const* ret = cstri_tokenize( g_internal_cstr, tokenizer, separators );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


char* cstr_temp_buffer( CSTR_SIZE_T capacity ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    char* ret = cstri_temp_buffer( g_internal_cstr, capacity );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


CSTR_SIZE_T cstr_temp_buffer_capacity( void ) {
    CSTR_MUTEX_LOCK();
    if( !g_internal_cstr ) internal_cstr_instance();
    CSTR_SIZE_T ret = cstri_temp_buffer_capacity( g_internal_cstr );
    CSTR_MUTEX_UNLOCK();
    return ret;
}


#endif /* CSTR_NO_GLOBAL_API */



#endif /* CSTR_IMPLEMENTATION */

/*
----------------------
    TESTS
----------------------
*/


#ifdef CSTR_RUN_TESTS

#include "testfw.h"

void stress_tests( void ) {

    TESTFW_TEST_BEGIN( "Create very large (16MB) cstr" );
    char* src = (char*) malloc( 16 * 1024 * 1024 + 1);
    TESTFW_EXPECTED( src != NULL );
    memset( src, 'A', 16 * 1024 * 1024 );
    src[ 16 * 1024 * 1024 ] = '\0';
    TESTFW_EXPECTED( strlen( src ) == 16 * 1024 * 1024 );
    char const* str = cstr( src );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( str != src );
    TESTFW_EXPECTED( strcmp( str, src ) == 0 );
    free( src );
    TESTFW_EXPECTED( strlen( str ) == 16 * 1024 * 1024 );
    TESTFW_TEST_END();


    TESTFW_TEST_BEGIN( "Create many strings (4M), and use restore points to roll back" );

    char const** strings = (char const**) malloc( sizeof( char* ) * 4 * 1000 * 1000 );
    struct cstr_restore_point_t* restore_point_a = NULL;
    struct cstr_restore_point_t* restore_point_b = NULL;
    struct cstr_restore_point_t* restore_point_c = NULL;
    for( CSTR_SIZE_T i = 0; i < 4 * 1000 * 1000; ++i ) {
        if( i == 1 * 1000 * 1000 ) {
            restore_point_a = cstr_restore_point();
        }
        if( i == 2 * 1000 * 1000 ) {
            restore_point_b = cstr_restore_point();
        }
        if( i == 3 * 1000 * 1000 ) {
            restore_point_c = cstr_restore_point();
        }
        char src[ 64 ];
        sprintf( src, "STRING: %x", (int) i );
        char const* str = cstr( src );
        strings[ i ] = str;
        TESTFW_EXPECTED( str != NULL );
        TESTFW_EXPECTED( str != src );
        TESTFW_EXPECTED( strlen( str ) != 0 );
        TESTFW_EXPECTED( strcmp( str, src ) == 0 );
    }

    TESTFW_EXPECTED( restore_point_a != NULL );
    TESTFW_EXPECTED( restore_point_b != NULL );
    TESTFW_EXPECTED( restore_point_c != NULL );

    for( CSTR_SIZE_T i = 0; i < 4 * 1000 * 1000; ++i ) {
        TESTFW_EXPECTED( cstr_is_interned( strings[ i ] ) );
    }

    cstr_rollback( restore_point_c );
    for( CSTR_SIZE_T i = 0; i < 3 * 1000 * 1000; ++i ) {
        TESTFW_EXPECTED( cstr_is_interned( strings[ i ] ) );
    }
    for( CSTR_SIZE_T i = 3 * 1000 * 1000; i < 4 * 1000 * 1000; ++i ) {
        TESTFW_EXPECTED( !cstr_is_interned( strings[ i ] ) );
    }

    // intentionally not rolling back to restore_point_b to test skipping over restore points

    cstr_rollback( restore_point_a );
    for( CSTR_SIZE_T i = 0; i < 1 * 1000 * 1000; ++i ) {
        TESTFW_EXPECTED( cstr_is_interned( strings[ i ] ) );
    }
    for( CSTR_SIZE_T i = 1 * 1000 * 1000; i < 4 * 1000 * 1000; ++i ) {
        TESTFW_EXPECTED( !cstr_is_interned( strings[ i ] ) );
    }
    free( (char*)strings );

    TESTFW_TEST_END();


    cstr_reset();
}


void test_cstr( void ) {
    TESTFW_TEST_BEGIN( "Create cstr from NULL pointer" );
    char const* str = cstr( NULL );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Create cstr from static string" );
    char const* str = cstr( "Static test string" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    #if defined(__clang__)
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wstring-compare"
    #endif
    TESTFW_EXPECTED( str != (char*)"Static test string" );
    #if defined(__clang__)
        #pragma clang diagnostic pop
    #endif
    TESTFW_EXPECTED( strcmp( str, "Static test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Create cstr from char array" );
    char arr[] = "Char array string";
    char const* str = cstr( arr );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( str != arr );
    TESTFW_EXPECTED( strcmp( str, "Char array string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Create cstr from cstr" );
    char const* src = cstr( "Test string" );
    TESTFW_EXPECTED( cstr_is_interned( src ) );
    TESTFW_EXPECTED( src != NULL );
    TESTFW_EXPECTED( strcmp( src, "Test string" ) == 0 );
    char const* str = cstr( src );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( str == src );
    TESTFW_EXPECTED( strcmp( str, "Test string" ) == 0 );
    TESTFW_TEST_END();
}


void test_cstr_n( void ) {
    TESTFW_TEST_BEGIN( "Create cstr from NULL pointer and explicit length" );
    char const* str = cstr_n( NULL, 0 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Create cstr from static string and explicit length" );
    char const* str = cstr_n( "Static test string", 18 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    #if defined(__clang__)
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wstring-compare"
    #endif
    TESTFW_EXPECTED( str != (char*)"Static test string" );
    #if defined(__clang__)
        #pragma clang diagnostic pop
    #endif
    TESTFW_EXPECTED( strcmp( str, "Static test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Create cstr from char array and explicit length" );
    char arr[] = "Char array string";
    char const* str = cstr_n( arr, 17 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( str != arr );
    TESTFW_EXPECTED( strcmp( str, "Char array string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Create cstr from cstr and explicit length" );
    char const* src = cstr( "Test string" );
    TESTFW_EXPECTED( cstr_is_interned( src ) );
    TESTFW_EXPECTED( src != NULL );
    TESTFW_EXPECTED( strcmp( src, "Test string" ) == 0 );
    char const* str = cstr_n( src, 11 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( str == src );
    TESTFW_EXPECTED( strcmp( str, "Test string" ) == 0 );
    TESTFW_TEST_END();


    TESTFW_TEST_BEGIN( "Create cstr from NULL pointer and too long explicit length" );
    char const* str = cstr_n( NULL, 20 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Create cstr from static string and too long explicit length" );
    char const* str = cstr_n( "Static test string", 30 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    #if defined(__clang__)
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wstring-compare"
    #endif
    TESTFW_EXPECTED( str != (char*)"Static test string" );
    #if defined(__clang__)
        #pragma clang diagnostic pop
    #endif
    TESTFW_EXPECTED( strcmp( str, "Static test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Create cstr from char array and too long explicit length" );
    char arr[] = "Char array string";
    char const* str = cstr_n( arr, 30 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( str != arr );
    TESTFW_EXPECTED( strcmp( str, "Char array string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Create cstr from cstr and too long explicit length" );
    char const* src = cstr( "Test string" );
    TESTFW_EXPECTED( cstr_is_interned( src ) );
    TESTFW_EXPECTED( src != NULL );
    TESTFW_EXPECTED( strcmp( src, "Test string" ) == 0 );
    char const* str = cstr_n( src, 30 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( str == src );
    TESTFW_EXPECTED( strcmp( str, "Test string" ) == 0 );
    TESTFW_TEST_END();


    TESTFW_TEST_BEGIN( "Create cstr from static string and explicit length shorter than string" );
    char const* str = cstr_n( "Static test string", 6 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    #if defined(__clang__)
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wstring-compare"
    #endif
    TESTFW_EXPECTED( str != (char*)"Static" );
    #if defined(__clang__)
        #pragma clang diagnostic pop
    #endif
    TESTFW_EXPECTED( strcmp( str, "Static" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Create cstr from char array and explicit length shorter than string" );
    char arr[] = "Char array string";
    char const* str = cstr_n( arr, 4 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( str != arr );
    TESTFW_EXPECTED( strcmp( str, "Char" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Create cstr from cstr and explicit length shorter than string" );
    char const* src = cstr( "Test string" );
    TESTFW_EXPECTED( cstr_is_interned( src ) );
    TESTFW_EXPECTED( src != NULL );
    TESTFW_EXPECTED( strcmp( src, "Test string" ) == 0 );
    char const* str = cstr_n( src, 4 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( str != src );
    TESTFW_EXPECTED( strcmp( str, "Test" ) == 0 );
    TESTFW_TEST_END();
}


void test_cstr_size( void ) {
    TESTFW_TEST_BEGIN( "Get length of a cstr string" );
    char const* str = cstr( "Test string" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( cstr_size( str ) == 11 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Get length of a cstr string containing zero terminator" );
    char const* str = cstr_n( "Test string\0part2", 18 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( cstr_size( str ) == 11 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Get length of a string literal" );
    TESTFW_EXPECTED( cstr_size( "Test string" ) == 11 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Get length of a char array" );
    char str[ 18 ] = "Test string\0part2";
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( cstr_size( str ) == 11 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Get length of a cstr created from char array" );
    char arr[ 18 ] = "Test string\0part2";
    char const* str = cstr_n( arr, 18 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( cstr_size( str ) == 11 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Get length of a NULL string" );
    TESTFW_EXPECTED( cstr_size( NULL ) == 0 );
    TESTFW_TEST_END();
}


void test_cstr_cat( void ) {
    TESTFW_TEST_BEGIN( "Concatenate two strings" );
    char const* str = cstr_cat( "Test", "String" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "TestString" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Concatenate a string with a NULL string" );
    char const* str = cstr_cat( "Test", NULL );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Concatenate a NULL string with a string" );
    char const* str = cstr_cat( NULL, "String" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "String" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Concatenate two NULL strings" );
    char const* str = cstr_cat( NULL, NULL );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "" ) == 0 );
    TESTFW_TEST_END();
}


void test_cstr_format( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_format with a NULL format string" );
    char const* str = cstr_format( NULL );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_format to format a string" );
    char const* str = cstr_format( "Test format, int: %d, float: %f, str: %s", 7, 1.0f, "Test String" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test format, int: 7, float: 1.000000, str: Test String" ) == 0 );
    TESTFW_TEST_END();
}


void test_cstr_trim( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_trim with leading spaces" );
    char const* str = cstr_trim( "   test" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "test" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_trim with trailing spaces" );
    char const* str = cstr_trim( "test   " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "test" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_trim with leading and trailing spaces" );
    char const* str = cstr_trim( "   test   " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "test" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_trim with various whitespace chars" );
    char const* str = cstr_trim( "\t\r\ntest\t\r\n" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "test" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_trim with single leading and trailing space" );
    char const* str = cstr_trim( " test " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "test" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_trim with no leading or trailing spaces" );
    char const* str = cstr_trim( "test" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "test" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_trim with single non-space char" );
    char const* str = cstr_trim( "t" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "t" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_trim with single non-space char and single leading space" );
    char const* str = cstr_trim( " t" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "t" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_trim with single non-space char and single trailing space" );
    char const* str = cstr_trim( "t " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "t" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_trim with single non-space char and single leading and trailing spaces" );
    char const* str = cstr_trim( " t " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "t" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_trim with spaces in the middle of the string" );
    char const* str = cstr_trim( "  test test  " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "test test" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_trim with a string of all spaces" );
    char const* str = cstr_trim( "     " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_trim with a NULL string" );
    char const* str = cstr_trim( NULL );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "" ) == 0 );
    TESTFW_TEST_END();
}


void test_cstr_ltrim( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_ltrim with leading spaces" );
    char const* str = cstr_ltrim( "   test" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "test" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_ltrim with trailing spaces" );
    char const* str = cstr_ltrim( "test   " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "test   " ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_ltrim with leading and trailing spaces" );
    char const* str = cstr_ltrim( "   test   " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "test   " ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_ltrim with various whitespace chars" );
    char const* str = cstr_ltrim( "\t\r\ntest\t\r\n" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "test\t\r\n" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_ltrim with single leading and trailing space" );
    char const* str = cstr_ltrim( " test " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "test " ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_ltrim with no leading or trailing spaces" );
    char const* str = cstr_ltrim( "test" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "test" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_ltrim with single non-space char" );
    char const* str = cstr_ltrim( "t" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "t" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_ltrim with single non-space char and single leading space" );
    char const* str = cstr_ltrim( " t" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "t" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_ltrim with single non-space char and single trailing space" );
    char const* str = cstr_ltrim( "t " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "t " ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_ltrim with single non-space char and single leading and trailing spaces" );
    char const* str = cstr_ltrim( " t " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "t " ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_ltrim with spaces in the middle of the string" );
    char const* str = cstr_ltrim( "  test test  " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "test test  " ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_ltrim with a string of all spaces" );
    char const* str = cstr_ltrim( "     " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_ltrim with a NULL string" );
    char const* str = cstr_ltrim( NULL );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "" ) == 0 );
    TESTFW_TEST_END();
}


void test_cstr_rtrim( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_rtrim with leading spaces" );
    char const* str = cstr_rtrim( "   test" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "   test" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_rtrim with trailing spaces" );
    char const* str = cstr_rtrim( "test   " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "test" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_rtrim with leading and trailing spaces" );
    char const* str = cstr_rtrim( "   test   " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "   test" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_rtrim with various whitespace chars" );
    char const* str = cstr_rtrim( "\t\r\ntest\t\r\n" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "\t\r\ntest" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_rtrim with single leading and trailing space" );
    char const* str = cstr_rtrim( " test " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, " test" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_rtrim with no leading or trailing spaces" );
    char const* str = cstr_rtrim( "test" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "test" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_rtrim with single non-space char" );
    char const* str = cstr_rtrim( "t" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "t" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_rtrim with single non-space char and single leading space" );
    char const* str = cstr_rtrim( " t" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, " t" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_rtrim with single non-space char and single trailing space" );
    char const* str = cstr_rtrim( "t " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "t" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_rtrim with single non-space char and single leading and trailing spaces" );
    char const* str = cstr_rtrim( " t " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, " t" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_rtrim with spaces in the middle of the string" );
    char const* str = cstr_rtrim( "  test test  " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "  test test" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_rtrim with a string of all spaces" );
    char const* str = cstr_rtrim( "     " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_rtrim with a NULL string" );
    char const* str = cstr_rtrim( NULL );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "" ) == 0 );
    TESTFW_TEST_END();
}


void test_cstr_left( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_left to get beginning of string" );
    char const* str = cstr_left( "Test string", 4 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_left with length beyond end of string" );
    char const* str = cstr_left( "Test string", 20 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_left with NULL string" );
    char const* str = cstr_left( NULL, 4 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "" ) == 0 );
    TESTFW_TEST_END();
}


void test_cstr_right( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_right to get end of string" );
    char const* str = cstr_right( "Test string", 6 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_right with length beyond start of string" );
    char const* str = cstr_right( "Test string", 20 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_right with NULL string" );
    char const* str = cstr_right( NULL, 6 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "" ) == 0 );
    TESTFW_TEST_END();
}


void test_cstr_mid( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_mid to get middle of string" );
    char const* str = cstr_mid( "Test string", 2, 5 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "st st" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_mid with length beyond end of string" );
    char const* str = cstr_mid( "Test string", 2, 20 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "st string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_mid with start beyond end of string" );
    char const* str = cstr_mid( "Test string", 20, 5 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_mid on NULL string" );
    char const* str = cstr_mid( NULL, 2, 5 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "" ) == 0 );
    TESTFW_TEST_END();
}


void test_cstr_upper( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_upper with mixed case string" );
    char const* str = cstr_upper( "Test String" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "TEST STRING" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_upper with uppercase string" );
    char const* str = cstr_upper( "TEST STRING" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "TEST STRING" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_upper with lowercase string" );
    char const* str = cstr_upper( "test string" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "TEST STRING" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_upper with NULL string" );
    char const* str = cstr_upper( NULL );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "" ) == 0 );
    TESTFW_TEST_END();
}


void test_cstr_lower( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_lower with mixed case string" );
    char const* str = cstr_lower( "Test String" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_lower with uppercase string" );
    char const* str = cstr_lower( "TEST STRING" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_lower with lowercase string" );
    char const* str = cstr_lower( "test string" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_lower with NULL string" );
    char const* str = cstr_lower( NULL );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "" ) == 0 );
    TESTFW_TEST_END();
}


void test_cstr_lpad( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_lpad with space padding" );
    char const* str = cstr_lpad( "Test string", ' ', 16 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "     Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_lpad with zero padding" );
    char const* str = cstr_lpad( "1234", '0', 8 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "00001234" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_lpad with string longer than target length" );
    char const* str = cstr_lpad( "Test string", '.', 11 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_lpad with zero target length" );
    char const* str = cstr_lpad( "Test string", '.', 0 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_lpad with empty string" );
    char const* str = cstr_lpad( "", '.', 5 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "....." ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_lpad with NULL string" );
    char const* str = cstr_lpad( NULL, '.', 5 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "....." ) == 0 );
    TESTFW_TEST_END();
}


void test_cstr_rpad( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_rpad with space padding" );
    char const* str = cstr_rpad( "Test string", ' ', 16 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test string     " ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_rpad with zero padding" );
    char const* str = cstr_rpad( "1234", '0', 8 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "12340000" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_rpad with string longer than target length" );
    char const* str = cstr_rpad( "Test string", '.', 5 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_rpad with zero target length" );
    char const* str = cstr_rpad( "Test string", '.', 0 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_rpad with empty string" );
    char const* str = cstr_rpad( "", '.', 5 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "....." ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_rpad with NULL string" );
    char const* str = cstr_rpad( NULL, '.', 5 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "....." ) == 0 );
    TESTFW_TEST_END();
}


void test_cstr_join( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_join with separator insertion" );
    char const* str = cstr_join( "dir", "file.txt", "/" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "dir/file.txt" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_join with separator at end of first string" );
    char const* str = cstr_join( "dir/", "file.txt", "/" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "dir/file.txt" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_join with separator at start of second string" );
    char const* str = cstr_join( "dir", "/file.txt", "/" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "dir/file.txt" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_join with empty first string" );
    char const* str = cstr_join( "", "file.txt", "/" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "file.txt" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_join with empty second string" );
    char const* str = cstr_join( "dir", "", "/" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "dir" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_join with empty separator" );
    char const* str = cstr_join( "Test", "String", "" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "TestString" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_join with NULL first string" );
    char const* str = cstr_join( NULL, "file.txt", "/" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "file.txt" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_join with NULL second string" );
    char const* str = cstr_join( "dir", NULL, "/" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "dir" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_join with NULL separator" );
    char const* str = cstr_join( "Test", "String", NULL );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "TestString" ) == 0 );
    TESTFW_TEST_END();
}


void test_cstr_replace( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_replace with one occurrence" );
    char const* str = cstr_replace( "Test string", "string", "value" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test value" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_replace with multiple occurrences" );
    char const* str = cstr_replace( "One fish two fish", "fish", "cat" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "One cat two cat" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_replace with no occurrences" );
    char const* str = cstr_replace( "No match here", "fish", "cat" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "No match here" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_replace removing content" );
    char const* str = cstr_replace( "Remove this word", "this ", "" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Remove word" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_replace inserting longer replacement" );
    char const* str = cstr_replace( "Short", "o", "oooooo" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Shoooooort" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_replace inserting shorter replacement" );
    char const* str = cstr_replace( "Banananana", "na", "!" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Ba!!!!" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_replace with empty find string" );
    char const* str = cstr_replace( "Test string", "", "-" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_replace with NULL find string" );
    char const* str = cstr_replace( "Test string", NULL, "-" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_replace with NULL input string" );
    char const* str = cstr_replace( NULL, "a", "b" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_replace with NULL replacement string" );
    char const* str = cstr_replace( "ababa", "b", NULL );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "aaa" ) == 0 );
    TESTFW_TEST_END();
}


void test_cstr_insert( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_insert at middle of string" );
    char const* str = cstr_insert( "Test string", 5, "value " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test value string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_insert at beginning of string" );
    char const* str = cstr_insert( "string", 0, "Test " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_insert at end of string" );
    char const* str = cstr_insert( "Test", 4, " string" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_insert with position beyond end of string" );
    char const* str = cstr_insert( "Test", 100, " string" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_insert with negative position" );
    char const* str = cstr_insert( "string", -5, "Test " );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_insert into empty string" );
    char const* str = cstr_insert( "", 0, "Test string" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_insert into NULL string" );
    char const* str = cstr_insert( NULL, 0, "Test string" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_insert with empty insertion" );
    char const* str = cstr_insert( "Test string", 4, "" );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_insert with NULL insertion" );
    char const* str = cstr_insert( "Test string", 4, NULL );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test string" ) == 0 );
    TESTFW_TEST_END();
}


void test_cstr_remove( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_remove to delete middle part of string" );
    char const* str = cstr_remove( "Test value string", 5, 6 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_remove to delete from beginning" );
    char const* str = cstr_remove( "Test string", 0, 5 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_remove to delete at end" );
    char const* str = cstr_remove( "Test string", 5, 10 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test " ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_remove with length 0" );
    char const* str = cstr_remove( "Test string", 5, 0 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_remove with negative start" );
    char const* str = cstr_remove( "Test string", -2, 3 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "est string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_remove with start beyond string length" );
    char const* str = cstr_remove( "Test string", 100, 5 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_remove to delete whole string" );
    char const* str = cstr_remove( "Test string", 0, 100 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_remove with empty string" );
    char const* str = cstr_remove( "", 0, 5 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_remove with NULL string" );
    char const* str = cstr_remove( NULL, 0, 5 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "" ) == 0 );
    TESTFW_TEST_END();
}


void test_cstr_int( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_int with a positive number" );
    char const* str = cstr_int( 42 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "42" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_int with a negative number" );
    char const* str = cstr_int( -100 );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "-100" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_int with a large number" );
    char const* str = cstr_int( 2147483647  );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "2147483647" ) == 0 );
    TESTFW_TEST_END();
}


void test_cstr_float( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_float" );
    char const* str = cstr_float( 42.0f );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "42.000000" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_int with a small number" );
    char const* str = cstr_float( 42.43e-6f );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "0.000042" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_int with a really small number" );
    char const* str = cstr_float( 42.43e-25f );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "0.000000" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_int with a really large number" );
    char const* str = cstr_float( 42e15f );
    TESTFW_EXPECTED( cstr_is_interned( str ) );
    TESTFW_EXPECTED( str != NULL );
    TESTFW_EXPECTED( strcmp( str, "41999999856279552.000000" ) == 0 );
    TESTFW_TEST_END();
}


void test_cstr_starts( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_starts with matching substring" );
    TESTFW_EXPECTED( cstr_starts( "TestString", "Test" ) == true );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_starts with non-matching substring" );
    TESTFW_EXPECTED( cstr_starts( "Test", "TestString" ) == false );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_starts with identical strings" );
    TESTFW_EXPECTED( cstr_starts( "Test", "Test" ) == true );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_starts with first string empty" );
    TESTFW_EXPECTED( cstr_starts( "", "Test" ) == false );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_starts with second string empty" );
    TESTFW_EXPECTED( cstr_starts( "Test", "" ) == true );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_starts with two empty strings" );
    TESTFW_EXPECTED( cstr_starts( "", "" ) == true );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_starts with single char second string" );
    TESTFW_EXPECTED( cstr_starts( "TestString", "T" ) == true );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_starts with second string NULL" );
    TESTFW_EXPECTED( cstr_starts( "TestString", NULL ) == true );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_starts with first string NULL" );
    TESTFW_EXPECTED( cstr_starts( NULL, "TestString" ) == false );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_starts with both strings NULL" );
    TESTFW_EXPECTED( cstr_starts( NULL, NULL ) == true );
    TESTFW_TEST_END();
}


void test_cstr_ends( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_ends with matching substring" );
    TESTFW_EXPECTED( cstr_ends( "TestString", "String" ) == true );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_ends with non-matching substring" );
    TESTFW_EXPECTED( cstr_ends( "String", "TestString" ) == false );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_ends with identical strings" );
    TESTFW_EXPECTED( cstr_ends( "Test", "Test" ) == true );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_ends with first string empty" );
    TESTFW_EXPECTED( cstr_ends( "", "String" ) == false );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_ends with second string empty" );
    TESTFW_EXPECTED( cstr_ends( "String", "" ) == true );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_ends with two empty strings" );
    TESTFW_EXPECTED( cstr_ends( "", "" ) == true );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_ends with single char second string" );
    TESTFW_EXPECTED( cstr_ends( "TestString", "g" ) == true );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_ends with second string NULL" );
    TESTFW_EXPECTED( cstr_ends( "TestString", NULL ) == true );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_ends with first string NULL" );
    TESTFW_EXPECTED( cstr_ends( NULL, "TestString" ) == false );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_ends with both strings NULL" );
    TESTFW_EXPECTED( cstr_ends( NULL, NULL ) == true );
    TESTFW_TEST_END();
}


void test_cstr_is_equal( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_is_equal with two NULL strings" );
    TESTFW_EXPECTED( cstr_is_equal( NULL, NULL ) == true );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_is_equal with a NULL string and an empty string" );
    TESTFW_EXPECTED( cstr_is_equal( NULL, "" ) == true );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_is_equal with two string literals" );
    TESTFW_EXPECTED( cstr_is_equal( "Test string", "Test string" ) == true );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_is_equal with two cstr strings" );
    char const* a = cstr( "Test string" );
    char const* b = cstr( "Test string" );
    TESTFW_EXPECTED( cstr_is_equal( a, b ) == true );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_is_equal with a string literal and a cstr string" );
    char const* str = cstr( "Test string" );
    TESTFW_EXPECTED( cstr_is_equal( str, "Test string" ) == true );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_is_equal with two different string literals" );
    TESTFW_EXPECTED( cstr_is_equal( "Test string A", "Test string B" ) == false );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_is_equal with two different cstr strings" );
    char const* a = cstr( "Test string A" );
    char const* b = cstr( "Test string B" );
    TESTFW_EXPECTED( cstr_is_equal( a, b ) == false );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_is_equal with a string literal and a different cstr string" );
    char const* str = cstr( "Test string A" );
    TESTFW_EXPECTED( cstr_is_equal( str, "Test string B" ) == false );
    TESTFW_TEST_END();
}


void test_cstr_compare( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_compare with two NULL strings" );
    TESTFW_EXPECTED( cstr_compare( NULL, NULL ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_compare with a NULL string and an empty string" );
    TESTFW_EXPECTED( cstr_compare( NULL, "" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_compare with two string literals" );
    TESTFW_EXPECTED( cstr_compare( "Test string", "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_compare with two cstr strings" );
    char const* a = cstr( "Test string" );
    char const* b = cstr( "Test string" );
    TESTFW_EXPECTED( cstr_compare( a, b ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_compare with a string literal and a cstr string" );
    char const* str = cstr( "Test string" );
    TESTFW_EXPECTED( cstr_compare( str, "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_compare with two different string literals" );
    TESTFW_EXPECTED( cstr_compare( "Test string A", "Test string B" ) < 0 );
    TESTFW_EXPECTED( cstr_compare( "Test string B", "Test string A" ) > 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_compare with two different cstr strings" );
    char const* a = cstr( "Test string A" );
    char const* b = cstr( "Test string B" );
    TESTFW_EXPECTED( cstr_compare( a, b ) < 0 );
    TESTFW_EXPECTED( cstr_compare( b, a ) > 0 );
    TESTFW_TEST_END();
}


void test_cstr_compare_nocase( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_compare_nocase with two NULL strings" );
    TESTFW_EXPECTED( cstr_compare_nocase( NULL, NULL ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_compare_nocase with a NULL string and an empty string" );
    TESTFW_EXPECTED( cstr_compare_nocase( NULL, "" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_compare_nocase with two string literals" );
    TESTFW_EXPECTED( cstr_compare_nocase( "Test string", "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_compare_nocase with two cstr strings" );
    char const* a = cstr( "Test string" );
    char const* b = cstr( "Test string" );
    TESTFW_EXPECTED( cstr_compare_nocase( a, b ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_compare_nocase with a string literal and a cstr string" );
    char const* str = cstr( "Test string" );
    TESTFW_EXPECTED( cstr_compare_nocase( str, "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_compare_nocase with two different string literals" );
    TESTFW_EXPECTED( cstr_compare_nocase( "Test string A", "Test string B" ) < 0 );
    TESTFW_EXPECTED( cstr_compare_nocase( "Test string B", "Test string A" ) > 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_compare_nocase with two different cstr strings" );
    char const* a = cstr( "Test string A" );
    char const* b = cstr( "Test string B" );
    TESTFW_EXPECTED( cstr_compare_nocase( a, b ) < 0 );
    TESTFW_EXPECTED( cstr_compare_nocase( b, a ) > 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_compare_nocase with two string literals differing only by case" );
    TESTFW_EXPECTED( cstr_compare_nocase( "TEST STRING", "Test string" ) == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_compare_nocase with two cstr strings differing only by case" );
    char const* a = cstr( "TEST STRING" );
    char const* b = cstr( "Test string" );
    TESTFW_EXPECTED( cstr_compare_nocase( a, b ) == 0 );
    TESTFW_TEST_END();
}


void test_cstr_find( void ) {
    TESTFW_TEST_BEGIN( "cstr_find finds first match from start" );
    int result = cstr_find( "one two one two", "two", 0 );
    TESTFW_EXPECTED( result == 4 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "cstr_find finds match after offset" );
    int result = cstr_find( "one two one two", "two", 5 );
    TESTFW_EXPECTED( result == 12 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "cstr_find finds match at position 0" );
    int result = cstr_find( "hello world", "hello", 0 );
    TESTFW_EXPECTED( result == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "cstr_find returns -1 for no match" );
    int result = cstr_find( "hello", "xyz", 0 );
    TESTFW_EXPECTED( result == -1 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "cstr_find with NULL string" );
    int result = cstr_find( NULL, "x", 0 );
    TESTFW_EXPECTED( result == -1 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "cstr_find with NULL find string" );
    int result = cstr_find( "x", NULL, 0 );
    TESTFW_EXPECTED( result == -1 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "cstr_find with empty string input" );
    int result = cstr_find( "", "x", 0 );
    TESTFW_EXPECTED( result == -1 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "cstr_find with empty find string" );
    int result = cstr_find( "x", "", 0 );
    TESTFW_EXPECTED( result == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "cstr_find clamps start beyond end" );
    int result = cstr_find( "ababab", "ab", 100 );
    TESTFW_EXPECTED( result == -1 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "cstr_find from inside first match" );
    int result = cstr_find( "ababab", "ab", 1 );
    TESTFW_EXPECTED( result == 2 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "cstr_find loop finds all matches" );
    int start = 0;
    int pos = cstr_find( "one two one two", "two", start );
    TESTFW_EXPECTED( pos == 4 );
    start = pos + 3;
    pos = cstr_find( "one two one two", "two", start );
    TESTFW_EXPECTED( pos == 12 );
    start = pos + 3;
    pos = cstr_find( "one two one two", "two", start );
    TESTFW_EXPECTED( pos == -1 );
    TESTFW_TEST_END();
}


void test_cstr_rfind( void ) {
    TESTFW_TEST_BEGIN( "cstr_rfind finds last match from full string" );
    int result = cstr_rfind( "one two one two", "two", 0 );
    TESTFW_EXPECTED( result == 12 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "cstr_rfind finds match before limit" );
    int result = cstr_rfind( "one two one two", "two", 10 );
    TESTFW_EXPECTED( result == 4 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "cstr_rfind finds match at position 0" );
    int result = cstr_rfind( "hello world", "hello", 0 );
    TESTFW_EXPECTED( result == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "cstr_rfind returns -1 for no match" );
    int result = cstr_rfind( "hello", "xyz", 0 );
    TESTFW_EXPECTED( result == -1 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "cstr_rfind with NULL string" );
    int result = cstr_rfind( NULL, "x", 0 );
    TESTFW_EXPECTED( result == -1 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "cstr_rfind with NULL find string" );
    int result = cstr_rfind( "x", NULL, 0 );
    TESTFW_EXPECTED( result == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "cstr_rfind with empty string input" );
    int result = cstr_rfind( "", "x", 0 );
    TESTFW_EXPECTED( result == -1 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "cstr_rfind with empty find string" );
    int result = cstr_rfind( "x", "", 0 );
    TESTFW_EXPECTED( result == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "cstr_rfind clamps end larger than max" );
    int result = cstr_rfind( "ababab", "ab", 100 );
    TESTFW_EXPECTED( result == 4 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "cstr_rfind from inside first match" );
    int result = cstr_rfind( "ababab", "ab", 1 );
    TESTFW_EXPECTED( result == 0 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "cstr_rfind loop finds all matches" );
    int end = 0;
    int pos = cstr_rfind( "one two one two", "two", end );
    TESTFW_EXPECTED( pos == 12 );
    end = pos - 1;
    pos = cstr_rfind( "one two one two", "two", end );
    TESTFW_EXPECTED( pos == 4 );
    end = pos - 1;
    pos = cstr_rfind( "one two one two", "two", end );
    TESTFW_EXPECTED( pos == -1 );
    TESTFW_TEST_END();
}


void test_cstr_hash( void ) {
    TESTFW_TEST_BEGIN( "Use cstr_hash with NULL string" );
    TESTFW_EXPECTED( cstr_hash( NULL ) == 0x569ed9aa );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_hash with empty string" );
    TESTFW_EXPECTED( cstr_hash( NULL ) == 0x569ed9aa );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_hash with string literal" );
    TESTFW_EXPECTED( cstr_hash( "Test string" ) == 0x5b8cc1a9 );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Use cstr_hash with cstr string" );
    char const* str = cstr( "Test string" );
    TESTFW_EXPECTED( cstr_hash( str ) == 0x5b8cc1a9 );
    TESTFW_TEST_END();
}


void test_cstr_tokenize( void ) {
    TESTFW_TEST_BEGIN( "Can tokenize sample string" );

    struct cstr_tokenizer_t tokenizer = cstr_tokenizer( "A string\tof ,,tokens\nand some  more tokens    " );
    TESTFW_EXPECTED( tokenizer.internal != NULL );

    char const* token1 = cstr_tokenize( &tokenizer, " ,\t\n" );
    TESTFW_EXPECTED( cstr_is_interned( token1 ) );
    TESTFW_EXPECTED( token1 != NULL );
    TESTFW_EXPECTED( strcmp( token1, "A" ) == 0 );

    char const* token2 = cstr_tokenize( &tokenizer, " ,\t\n" );
    TESTFW_EXPECTED( cstr_is_interned( token2 ) );
    TESTFW_EXPECTED( token2 != NULL );
    TESTFW_EXPECTED( strcmp( token2, "string" ) == 0 );

    char const* token3 = cstr_tokenize( &tokenizer, " ,\t\n" );
    TESTFW_EXPECTED( cstr_is_interned( token3 ) );
    TESTFW_EXPECTED( token3 != NULL );
    TESTFW_EXPECTED( strcmp( token3, "of" ) == 0 );

    char const* token4 = cstr_tokenize( &tokenizer, " ,\t\n" );
    TESTFW_EXPECTED( cstr_is_interned( token4 ) );
    TESTFW_EXPECTED( token4 != NULL );
    TESTFW_EXPECTED( strcmp( token4, "tokens" ) == 0 );

    char const* token5 = cstr_tokenize( &tokenizer, " ,\t\n" );
    TESTFW_EXPECTED( cstr_is_interned( token5 ) );
    TESTFW_EXPECTED( token5 != NULL );
    TESTFW_EXPECTED( strcmp( token5, "and" ) == 0 );

    char const* token6 = cstr_tokenize( &tokenizer, " ,\t\n" );
    TESTFW_EXPECTED( cstr_is_interned( token6 ) );
    TESTFW_EXPECTED( token6 != NULL );
    TESTFW_EXPECTED( strcmp( token6, "some" ) == 0 );

    char const* token7 = cstr_tokenize( &tokenizer, " ,\t\n" );
    TESTFW_EXPECTED( cstr_is_interned( token7 ) );
    TESTFW_EXPECTED( token7 != NULL );
    TESTFW_EXPECTED( strcmp( token7, "more" ) == 0 );

    char const* token8 = cstr_tokenize( &tokenizer, " ,\t\n" );
    TESTFW_EXPECTED( cstr_is_interned( token8 ) );
    TESTFW_EXPECTED( token8 != NULL );
    TESTFW_EXPECTED( strcmp( token8, "tokens" ) == 0 );

    char const* token9 = cstr_tokenize( &tokenizer, " ,\t\n" );
    TESTFW_EXPECTED( token9 == NULL );

    TESTFW_TEST_END();


    TESTFW_TEST_BEGIN( "Can tokenize string containing only separators" );
    struct cstr_tokenizer_t tokenizer = cstr_tokenizer( " \t ,,\n   " );
    TESTFW_EXPECTED( tokenizer.internal != NULL );
    char const* token = cstr_tokenize( &tokenizer, " ,\t\n" );
    TESTFW_EXPECTED( token == NULL );
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "Can tokenize NULL string" );
    struct cstr_tokenizer_t tokenizer = cstr_tokenizer( NULL );
    TESTFW_EXPECTED( tokenizer.internal != NULL );
    char const* token = cstr_tokenize( &tokenizer, " ,\t\n" );
    TESTFW_EXPECTED( token == NULL );
    TESTFW_TEST_END();
}


#ifndef CSTR_ASCII_ONLY

static void internal_test_utf8_build( char* out, CSTR_SIZE_T* out_len, CSTR_U32 const* cps, CSTR_SIZE_T count ) {
    CSTR_SIZE_T n = 0;
    for( CSTR_SIZE_T i = 0; i < count; ++i ) {
        n += internal_cstr_utf8_encode( cps[ i ], out + n );
    }
    out[ n ] = '\0';
    if( out_len ) *out_len = n;
}

static void internal_test_bytes_build( char* out, CSTR_SIZE_T* out_len, unsigned char const* bytes, CSTR_SIZE_T count ) {
    for( CSTR_SIZE_T i = 0; i < count; ++i ) out[ i ] = (char)bytes[ i ];
    out[ count ] = '\0';
    if( out_len ) *out_len = count;
}

static CSTR_BOOL_T internal_test_utf8_is_valid( char const* s, CSTR_SIZE_T len ) {
    char const* p = s ? s : "";
    char const* end = p + len;
    while( p < end ) {
        CSTR_U32 cp;
        CSTR_SIZE_T bytes;
        CSTR_BOOL_T ok = internal_cstr_utf8_decode( p, end, &cp, &bytes );
        if( !ok || bytes == 0 ) return 0;
        p += bytes;
    }
    return 1;
}

void test_cstr_utf8( void ) {
    #ifdef _MSC_VER
        #pragma warning( push )
        #pragma warning( disable: 4310 ) // cast truncates constant value
    #endif

    TESTFW_TEST_BEGIN( "utf8: encode/decode boundary forms" );
    {
        struct { CSTR_U32 cp; unsigned char b[4]; unsigned char n; } cases[] = {
            { 0x00000001u, { 0x01u, 0, 0, 0 }, 1 },
            { 0x0000007Fu, { 0x7Fu, 0, 0, 0 }, 1 },

            { 0x00000080u, { 0xC2u, 0x80u, 0, 0 }, 2 },
            { 0x000007FFu, { 0xDFu, 0xBFu, 0, 0 }, 2 },

            { 0x00000800u, { 0xE0u, 0xA0u, 0x80u, 0 }, 3 },
            { 0x0000FFFFu, { 0xEFu, 0xBFu, 0xBFu, 0 }, 3 },

            { 0x00010000u, { 0xF0u, 0x90u, 0x80u, 0x80u }, 4 },
            { 0x0010FFFFu, { 0xF4u, 0x8Fu, 0xBFu, 0xBFu }, 4 },
        };

        for( CSTR_SIZE_T i = 0; i < (CSTR_SIZE_T)( sizeof( cases ) / sizeof( cases[0] ) ); ++i ) {
            char enc[ 8 ];
            CSTR_SIZE_T wrote = internal_cstr_utf8_encode( cases[i].cp, enc );
            TESTFW_EXPECTED( wrote == (CSTR_SIZE_T)cases[i].n );
            TESTFW_EXPECTED( memcmp( enc, cases[i].b, cases[i].n ) == 0 );

            {
                CSTR_U32 cp = 0;
                CSTR_SIZE_T bytes = 0;
                CSTR_BOOL_T ok = internal_cstr_utf8_decode( enc, enc + cases[i].n, &cp, &bytes );
                TESTFW_EXPECTED( ok );
                TESTFW_EXPECTED( bytes == (CSTR_SIZE_T)cases[i].n );
                TESTFW_EXPECTED( cp == cases[i].cp );
            }
        }
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: decode rejects overlong encodings" );
    {
        struct { unsigned char b[4]; unsigned char n; } bad[] = {
            { { 0xC0u, 0x80u, 0, 0 }, 2 },
            { { 0xC1u, 0x81u, 0, 0 }, 2 },
            { { 0xE0u, 0x80u, 0x80u, 0 }, 3 },
            { { 0xF0u, 0x80u, 0x80u, 0x80u }, 4 },
        };

        for( CSTR_SIZE_T i = 0; i < (CSTR_SIZE_T)( sizeof( bad ) / sizeof( bad[0] ) ); ++i ) {
            char s[ 8 ];
            CSTR_SIZE_T len = 0;
            internal_test_bytes_build( s, &len, bad[i].b, bad[i].n );

            CSTR_U32 cp = 0;
            CSTR_SIZE_T bytes = 0;
            CSTR_BOOL_T ok = internal_cstr_utf8_decode( s, s + len, &cp, &bytes );

            TESTFW_EXPECTED( !ok );
            TESTFW_EXPECTED( bytes == 1 );
            TESTFW_EXPECTED( cp == 0xFFFDu );
        }
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: decode rejects surrogates and out-of-range" );
    {
        struct { unsigned char b[4]; unsigned char n; } bad[] = {
            { { 0xEDu, 0xA0u, 0x80u, 0 }, 3 },
            { { 0xEDu, 0xBFu, 0xBFu, 0 }, 3 },
            { { 0xF4u, 0x90u, 0x80u, 0x80u }, 4 },
            { { 0xF5u, 0x80u, 0x80u, 0x80u }, 4 },
            { { 0xFFu, 0x80u, 0x80u, 0x80u }, 4 },
        };

        for( CSTR_SIZE_T i = 0; i < (CSTR_SIZE_T)( sizeof( bad ) / sizeof( bad[0] ) ); ++i ) {
            char s[ 8 ];
            CSTR_SIZE_T len = 0;
            internal_test_bytes_build( s, &len, bad[i].b, bad[i].n );

            CSTR_U32 cp = 0;
            CSTR_SIZE_T bytes = 0;
            CSTR_BOOL_T ok = internal_cstr_utf8_decode( s, s + len, &cp, &bytes );

            TESTFW_EXPECTED( !ok );
            TESTFW_EXPECTED( bytes == 1 );
            TESTFW_EXPECTED( cp == 0xFFFDu );
        }
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: decode handles truncated sequences (always consumes 1 byte)" );
    {
        struct { unsigned char b[4]; unsigned char n; } bad[] = {
            { { 0xC2u, 0, 0, 0 }, 1 },
            { { 0xE2u, 0x82u, 0, 0 }, 2 },
            { { 0xF0u, 0x9Fu, 0x92u, 0 }, 3 },
        };

        for( CSTR_SIZE_T i = 0; i < (CSTR_SIZE_T)( sizeof( bad ) / sizeof( bad[0] ) ); ++i ) {
            char s[ 8 ];
            CSTR_SIZE_T len = 0;
            internal_test_bytes_build( s, &len, bad[i].b, bad[i].n );

            CSTR_U32 cp = 0;
            CSTR_SIZE_T bytes = 0;
            CSTR_BOOL_T ok = internal_cstr_utf8_decode( s, s + len, &cp, &bytes );

            TESTFW_EXPECTED( !ok );
            TESTFW_EXPECTED( bytes == 1 );
            TESTFW_EXPECTED( cp == 0xFFFDu );
        }
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: decoder always makes progress on arbitrary bytes" );
    {
        unsigned int x = 0x12345678u;
        char s[ 1024 + 1 ];
        for( int i = 0; i < 1024; ++i ) {
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            s[ i ] = (char)( x & 0xFFu );
        }
        s[ 1024 ] = '\0';

        char const* p = s;
        char const* end = s + 1024;
        int steps = 0;
        while( p < end && steps < 5000 ) {
            CSTR_U32 cp;
            CSTR_SIZE_T bytes;
            (void)internal_cstr_utf8_decode( p, end, &cp, &bytes );
            TESTFW_EXPECTED( bytes > 0 );
            p += bytes;
            ++steps;
        }
        TESTFW_EXPECTED( steps < 5000 );
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: whitespace table used by trim/ltrim/rtrim" );
    {
        CSTR_U32 cps[] = { 0x000009u, 0x00000Du, 0x000020u, 0x0000A0u, 0x001680u, 0x00200Au, 0x002028u, 0x002029u, 0x00202Fu, 0x000058u,
                           0x00202Fu, 0x002029u, 0x002028u, 0x00200Au, 0x001680u, 0x0000A0u, 0x000020u, 0x00000Du, 0x000009u };
        char buf[ 256 ];
        CSTR_SIZE_T blen = 0;
        internal_test_utf8_build( buf, &blen, cps, (CSTR_SIZE_T)( sizeof( cps ) / sizeof( cps[ 0 ] ) ) );
        TESTFW_EXPECTED( internal_test_utf8_is_valid( buf, blen ) );

        char const* t  = cstr_trim( buf );
        char const* lt = cstr_ltrim( buf );
        char const* rt = cstr_rtrim( buf );

        TESTFW_EXPECTED( strcmp( t, "X" ) == 0 );
        TESTFW_EXPECTED( cstr_size( t ) == 1 );
        TESTFW_EXPECTED( cstr_len( t ) == 1 );
        TESTFW_EXPECTED( cstr_len( lt ) > 1 );
        TESTFW_EXPECTED( cstr_len( rt ) > 1 );
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: whitespace table covers every codepoint in each range" );
    {
        CSTR_SIZE_T n = (CSTR_SIZE_T)( sizeof( internal_cstr_utf8_whitespace ) / sizeof( internal_cstr_utf8_whitespace[0] ) );
        for( CSTR_SIZE_T i = 0; i < n; ++i ) {
            CSTR_U32 lo = internal_cstr_utf8_whitespace[i].lo;
            CSTR_U32 hi = internal_cstr_utf8_whitespace[i].hi;
            for( CSTR_U32 cp = lo; cp <= hi; ++cp ) {
                char buf[ 8 ];
                CSTR_SIZE_T blen = internal_cstr_utf8_encode( cp, buf );
                buf[ blen ] = '\0';

                char tmp[ 32 ];
                CSTR_MEMCPY( tmp, buf, blen );
                tmp[ blen ] = 'X';
                tmp[ blen + 1 ] = '\0';

                TESTFW_EXPECTED( strcmp( cstr_ltrim( tmp ), "X" ) == 0 );

                CSTR_MEMCPY( tmp + 1, buf, blen );
                tmp[ 0 ] = 'X';
                tmp[ 1 + blen ] = '\0';

                TESTFW_EXPECTED( strcmp( cstr_rtrim( tmp ), "X" ) == 0 );
            }
        }
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: trim does not remove non-whitespace codepoints (U+200B)" );
    {
        CSTR_U32 cps_in[]  = { 0x00200Bu, 0x000058u, 0x00200Bu };
        CSTR_U32 cps_out[] = { 0x00200Bu, 0x000058u, 0x00200Bu };
        char in[ 32 ], exp[ 32 ];
        CSTR_SIZE_T in_len = 0, exp_len = 0;
        internal_test_utf8_build( in, &in_len, cps_in,  (CSTR_SIZE_T)( sizeof( cps_in )  / sizeof( cps_in[ 0 ] ) ) );
        internal_test_utf8_build( exp, &exp_len, cps_out, (CSTR_SIZE_T)( sizeof( cps_out ) / sizeof( cps_out[ 0 ] ) ) );

        char const* t = cstr_trim( in );
        TESTFW_EXPECTED( cstr_size( t ) == exp_len );
        TESTFW_EXPECTED( memcmp( t, exp, exp_len ) == 0 );
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: trim does not remove non-whitespace codepoints (U+FEFF, U+200C, U+2060)" );
    {
        CSTR_U32 cps_in[]  = { 0x00FEFFu, 0x00200Cu, 0x002060u, 0x000058u, 0x002060u, 0x00200Cu, 0x00FEFFu };
        char in[ 64 ];
        CSTR_SIZE_T in_len = 0;
        internal_test_utf8_build( in, &in_len, cps_in,  (CSTR_SIZE_T)( sizeof( cps_in )  / sizeof( cps_in[ 0 ] ) ) );

        char const* t = cstr_trim( in );
        TESTFW_EXPECTED( cstr_size( t ) == in_len );
        TESTFW_EXPECTED( memcmp( t, in, in_len ) == 0 );
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: to_upper/to_lower range mapping (U+00E0 <-> U+00C0)" );
    {
        CSTR_U32 cps_lo[] = { 0x0000E0u };
        CSTR_U32 cps_up[] = { 0x0000C0u };
        char lo[ 8 ], up[ 8 ], exp_lo[ 8 ], exp_up[ 8 ];
        CSTR_SIZE_T lo_len = 0, up_len = 0, exp_lo_len = 0, exp_up_len = 0;
        internal_test_utf8_build( lo, &lo_len, cps_lo, 1 );
        internal_test_utf8_build( up, &up_len, cps_up, 1 );
        internal_test_utf8_build( exp_up, &exp_up_len, cps_up, 1 );
        internal_test_utf8_build( exp_lo, &exp_lo_len, cps_lo, 1 );

        char const* got_up = cstr_upper( lo );
        char const* got_lo = cstr_lower( up );

        TESTFW_EXPECTED( cstr_size( got_up ) == exp_up_len );
        TESTFW_EXPECTED( cstr_size( got_lo ) == exp_lo_len );
        TESTFW_EXPECTED( memcmp( got_up, exp_up, exp_up_len ) == 0 );
        TESTFW_EXPECTED( memcmp( got_lo, exp_lo, exp_lo_len ) == 0 );
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: to_upper pair mapping and casefold pair (U+00B5, U+039C, U+03BC)" );
    {
        CSTR_U32 cps_mu[]       = { 0x0000B5u };
        CSTR_U32 cps_MU[]       = { 0x00039Cu };
        CSTR_U32 cps_mu_greek[] = { 0x0003BCu };
        char mu[ 8 ], MU[ 8 ], mu_g[ 8 ];
        CSTR_SIZE_T mu_len = 0, MU_len = 0, mu_g_len = 0;
        internal_test_utf8_build( mu, &mu_len, cps_mu, 1 );
        internal_test_utf8_build( MU, &MU_len, cps_MU, 1 );
        internal_test_utf8_build( mu_g, &mu_g_len, cps_mu_greek, 1 );

        char const* got_up = cstr_upper( mu );
        char const* got_lo = cstr_lower( MU );

        TESTFW_EXPECTED( cstr_size( got_up ) == MU_len );
        TESTFW_EXPECTED( memcmp( got_up, MU, MU_len ) == 0 );
        TESTFW_EXPECTED( cstr_size( got_lo ) == mu_g_len );
        TESTFW_EXPECTED( memcmp( got_lo, mu_g, mu_g_len ) == 0 );

        TESTFW_EXPECTED( cstr_compare_nocase( mu, mu_g ) == 0 );
        TESTFW_EXPECTED( cstr_compare_nocase( MU, mu ) == 0 );
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: to_upper multi mapping (U+00DF -> \"SS\")" );
    {
        CSTR_U32 cps_sz[] = { 0x0000DFu };
        char sz[ 8 ];
        CSTR_SIZE_T sz_len = 0;
        internal_test_utf8_build( sz, &sz_len, cps_sz, 1 );

        char const* up = cstr_upper( sz );
        TESTFW_EXPECTED( strcmp( up, "SS" ) == 0 );
        TESTFW_EXPECTED( cstr_len( up ) == 2 );
        TESTFW_EXPECTED( cstr_compare_nocase( sz, "ss" ) == 0 );

        {
            CSTR_U32 cps_word[] = { 0x000061u, 0x0000DFu, 0x000062u };
            char word[ 16 ];
            CSTR_SIZE_T wlen = 0;
            internal_test_utf8_build( word, &wlen, cps_word, 3 );
            TESTFW_EXPECTED( cstr_compare_nocase( word, "assb" ) == 0 );
        }
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: to_lower multi mapping (U+0130 -> U+0069 U+0307)" );
    {
        CSTR_U32 cps_in[]  = { 0x000130u };
        CSTR_U32 cps_out[] = { 0x000069u, 0x000307u };
        char in[ 8 ], exp[ 16 ];
        CSTR_SIZE_T in_len = 0, exp_len = 0;
        internal_test_utf8_build( in, &in_len, cps_in, 1 );
        internal_test_utf8_build( exp, &exp_len, cps_out, 2 );

        char const* lo = cstr_lower( in );
        TESTFW_EXPECTED( cstr_size( lo ) == exp_len );
        TESTFW_EXPECTED( memcmp( lo, exp, exp_len ) == 0 );
        TESTFW_EXPECTED( internal_test_utf8_is_valid( lo, exp_len ) );
        TESTFW_EXPECTED( cstr_compare_nocase( in, exp ) == 0 );
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: casefold multi count=2 (U+0149 -> U+02BC 'n')" );
    {
        CSTR_U32 cps_in[]  = { 0x000149u };
        CSTR_U32 cps_out[] = { 0x0002BCu, 0x00006Eu };
        char in[ 8 ], exp[ 16 ];
        CSTR_SIZE_T in_len = 0, exp_len = 0;
        internal_test_utf8_build( in, &in_len, cps_in, 1 );
        internal_test_utf8_build( exp, &exp_len, cps_out, 2 );

        TESTFW_EXPECTED( cstr_compare_nocase( in, exp ) == 0 );
        TESTFW_EXPECTED( cstr_compare_nocase( exp, in ) == 0 );
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: casefold multi count=3 (U+0390 -> U+03B9 U+0308 U+0301)" );
    {
        CSTR_U32 cps_in[]  = { 0x000390u };
        CSTR_U32 cps_out[] = { 0x0003B9u, 0x000308u, 0x000301u };
        char in[ 8 ], exp[ 24 ];
        CSTR_SIZE_T in_len = 0, exp_len = 0;
        internal_test_utf8_build( in, &in_len, cps_in, 1 );
        internal_test_utf8_build( exp, &exp_len, cps_out, 3 );

        TESTFW_EXPECTED( cstr_compare_nocase( in, exp ) == 0 );
        TESTFW_EXPECTED( cstr_compare_nocase( exp, in ) == 0 );
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: casefold special pairs (U+212A, U+03C2)" );
    {
        CSTR_U32 cps_kelvin[] = { 0x00212Au };
        CSTR_U32 cps_k[]      = { 0x00006Bu };
        char kelvin[ 8 ], k[ 8 ];
        CSTR_SIZE_T kelvin_len = 0, k_len = 0;
        internal_test_utf8_build( kelvin, &kelvin_len, cps_kelvin, 1 );
        internal_test_utf8_build( k, &k_len, cps_k, 1 );

        TESTFW_EXPECTED( cstr_compare_nocase( kelvin, k ) == 0 );
        TESTFW_EXPECTED( cstr_compare_nocase( k, kelvin ) == 0 );

        {
            CSTR_U32 cps_final_sigma[] = { 0x0003C2u };
            CSTR_U32 cps_sigma[]       = { 0x0003C3u };
            char fs[ 8 ], s[ 8 ];
            CSTR_SIZE_T fs_len = 0, s_len = 0;
            internal_test_utf8_build( fs, &fs_len, cps_final_sigma, 1 );
            internal_test_utf8_build( s, &s_len, cps_sigma, 1 );

            TESTFW_EXPECTED( cstr_compare_nocase( fs, s ) == 0 );
            TESTFW_EXPECTED( cstr_compare_nocase( s, fs ) == 0 );
        }
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: casefold multi ligatures (U+FB01 -> \"fi\")" );
    {
        CSTR_U32 cps_lig[] = { 0x00FB01u };
        char lig[ 8 ];
        CSTR_SIZE_T lig_len = 0;
        internal_test_utf8_build( lig, &lig_len, cps_lig, 1 );

        TESTFW_EXPECTED( cstr_compare_nocase( lig, "fi" ) == 0 );
        TESTFW_EXPECTED( cstr_compare_nocase( "FI", lig ) == 0 );
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: codepoint_count differs from byte length" );
    {
        CSTR_U32 cps[] = { 0x000041u, 0x0020ACu, 0x0000DFu, 0x000042u };
        char buf[ 32 ];
        CSTR_SIZE_T blen = 0;
        internal_test_utf8_build( buf, &blen, cps, 4 );
        char const* s = cstr( buf );

        TESTFW_EXPECTED( cstr_size( s ) == blen );
        TESTFW_EXPECTED( cstr_len( s ) == 4 );

        char const* up = cstr_upper( s );
        TESTFW_EXPECTED( cstr_len( up ) == 5 );
        TESTFW_EXPECTED( internal_test_utf8_is_valid( up, cstr_size( up ) ) );
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: codepoint_count with 4-byte codepoints" );
    {
        CSTR_U32 cps[] = { 0x000041u, 0x00010000u, 0x000042u };
        char buf[ 32 ];
        CSTR_SIZE_T blen = 0;
        internal_test_utf8_build( buf, &blen, cps, 3 );

        TESTFW_EXPECTED( internal_test_utf8_is_valid( buf, blen ) );
        TESTFW_EXPECTED( cstr_len( buf ) == 3 );
        TESTFW_EXPECTED( cstr_size( buf ) == blen );

        char const* l = cstr_left( buf, 2 );
        TESTFW_EXPECTED( cstr_len( l ) == 2 );
        TESTFW_EXPECTED( internal_test_utf8_is_valid( l, cstr_size( l ) ) );
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: left/right/mid operate on codepoints, not bytes" );
    {
        CSTR_U32 cps[] = { 0x000041u, 0x0020ACu, 0x0000DFu, 0x000042u };
        char buf[ 32 ];
        CSTR_SIZE_T blen = 0;
        internal_test_utf8_build( buf, &blen, cps, 4 );
        char const* s = cstr( buf );

        char exp_left[ 32 ], exp_right[ 32 ], exp_mid[ 32 ];
        CSTR_SIZE_T exp_left_len = 0, exp_right_len = 0, exp_mid_len = 0;
        { CSTR_U32 e[] = { 0x000041u, 0x0020ACu }; internal_test_utf8_build( exp_left, &exp_left_len, e, 2 ); }
        { CSTR_U32 e[] = { 0x0000DFu, 0x000042u }; internal_test_utf8_build( exp_right, &exp_right_len, e, 2 ); }
        { CSTR_U32 e[] = { 0x0020ACu, 0x0000DFu }; internal_test_utf8_build( exp_mid, &exp_mid_len, e, 2 ); }

        char const* l = cstr_left( s, 2 );
        char const* r = cstr_right( s, 2 );
        char const* m = cstr_mid( s, 1, 2 );

        TESTFW_EXPECTED( cstr_size( l ) == exp_left_len );
        TESTFW_EXPECTED( memcmp( l, exp_left, exp_left_len ) == 0 );
        TESTFW_EXPECTED( cstr_size( r ) == exp_right_len );
        TESTFW_EXPECTED( memcmp( r, exp_right, exp_right_len ) == 0 );
        TESTFW_EXPECTED( cstr_size( m ) == exp_mid_len );
        TESTFW_EXPECTED( memcmp( m, exp_mid, exp_mid_len ) == 0 );
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: left/right/mid boundary behavior" );
    {
        CSTR_U32 cps[] = { 0x000041u, 0x0020ACu, 0x0000DFu, 0x000042u };
        char buf[ 32 ];
        CSTR_SIZE_T blen = 0;
        internal_test_utf8_build( buf, &blen, cps, 4 );
        char const* s = cstr( buf );

        TESTFW_EXPECTED( strcmp( cstr_left( s, 0 ), "" ) == 0 );
        TESTFW_EXPECTED( strcmp( cstr_right( s, 0 ), "" ) == 0 );
        TESTFW_EXPECTED( strcmp( cstr_mid( s, 999, 2 ), "" ) == 0 );

        {
            char const* all = cstr_left( s, 999 );
            TESTFW_EXPECTED( cstr_size( all ) == cstr_size( s ) );
            TESTFW_EXPECTED( memcmp( all, s, cstr_size( s ) ) == 0 );
        }

        {
            char const* all = cstr_right( s, 999 );
            TESTFW_EXPECTED( cstr_size( all ) == cstr_size( s ) );
            TESTFW_EXPECTED( memcmp( all, s, cstr_size( s ) ) == 0 );
        }

        {
            char const* tail = cstr_mid( s, 2, 999 );
            char exp[ 16 ];
            CSTR_SIZE_T exp_len = 0;
            { CSTR_U32 e[] = { 0x0000DFu, 0x000042u }; internal_test_utf8_build( exp, &exp_len, e, 2 ); }
            TESTFW_EXPECTED( cstr_size( tail ) == exp_len );
            TESTFW_EXPECTED( memcmp( tail, exp, exp_len ) == 0 );
        }
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: insert/remove positions measured in codepoints" );
    {
        CSTR_U32 cps[] = { 0x000041u, 0x0020ACu, 0x0000DFu, 0x000042u };
        char buf[ 32 ];
        CSTR_SIZE_T blen = 0;
        internal_test_utf8_build( buf, &blen, cps, 4 );
        char const* s = cstr( buf );

        char const* inserted = cstr_insert( s, 2, "X" );

        char exp_ins[ 64 ];
        CSTR_SIZE_T exp_ins_len = 0;
        { CSTR_U32 e[] = { 0x000041u, 0x0020ACu, 0x000058u, 0x0000DFu, 0x000042u }; internal_test_utf8_build( exp_ins, &exp_ins_len, e, 5 ); }
        TESTFW_EXPECTED( cstr_size( inserted ) == exp_ins_len );
        TESTFW_EXPECTED( memcmp( inserted, exp_ins, exp_ins_len ) == 0 );

        char const* removed = cstr_remove( s, 1, 2 );

        char exp_rem[ 16 ];
        CSTR_SIZE_T exp_rem_len = 0;
        { CSTR_U32 e[] = { 0x000041u, 0x000042u }; internal_test_utf8_build( exp_rem, &exp_rem_len, e, 2 ); }
        TESTFW_EXPECTED( cstr_size( removed ) == exp_rem_len );
        TESTFW_EXPECTED( memcmp( removed, exp_rem, exp_rem_len ) == 0 );
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: insert/remove boundary behavior" );
    {
        CSTR_U32 cps[] = { 0x000041u, 0x0020ACu, 0x0000DFu, 0x000042u };
        char buf[ 32 ];
        CSTR_SIZE_T blen = 0;
        internal_test_utf8_build( buf, &blen, cps, 4 );
        char const* s = cstr( buf );

        {
            char const* ins0 = cstr_insert( s, 0, "X" );
            TESTFW_EXPECTED( ins0[0] == 'X' );
            TESTFW_EXPECTED( cstr_len( ins0 ) == 5 );
        }
        {
            char const* ins_end = cstr_insert( s, 999, "X" );
            TESTFW_EXPECTED( cstr_len( ins_end ) == 5 );
            TESTFW_EXPECTED( ins_end[ cstr_size( ins_end ) - 1 ] == 'X' );
        }

        {
            char const* rm0 = cstr_remove( s, 0, 0 );
            TESTFW_EXPECTED( cstr_size( rm0 ) == cstr_size( s ) );
            TESTFW_EXPECTED( memcmp( rm0, s, cstr_size( s ) ) == 0 );
        }
        {
            char const* rm_all = cstr_remove( s, 0, 999 );
            TESTFW_EXPECTED( strcmp( rm_all, "" ) == 0 );
        }
        {
            char const* rm_tail = cstr_remove( s, 2, 999 );
            char exp[ 32 ];
            CSTR_SIZE_T exp_len = 0;
            { CSTR_U32 e[] = { 0x000041u, 0x0020ACu }; internal_test_utf8_build( exp, &exp_len, e, 2 ); }
            TESTFW_EXPECTED( cstr_size( rm_tail ) == exp_len );
            TESTFW_EXPECTED( memcmp( rm_tail, exp, exp_len ) == 0 );
        }
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: find/rfind/replace operate on codepoint indices" );
    {
        CSTR_U32 cps[] = { 0x000041u, 0x0020ACu, 0x000042u, 0x0020ACu, 0x000043u };
        char buf[ 64 ];
        CSTR_SIZE_T blen = 0;
        internal_test_utf8_build( buf, &blen, cps, 5 );
        char const* s = cstr( buf );

        char pat[ 16 ];
        CSTR_SIZE_T pat_len = 0;
        { CSTR_U32 p[] = { 0x0020ACu, 0x000042u }; internal_test_utf8_build( pat, &pat_len, p, 2 ); }

        int pos = cstr_find( s, pat, 0 );
        TESTFW_EXPECTED( pos == 1 );

        int end_cp = (int)cstr_len( s );
        int pos2 = cstr_rfind( s, pat, end_cp );
        TESTFW_EXPECTED( pos2 == 1 );

        char euro[ 8 ];
        CSTR_SIZE_T euro_len = 0;
        { CSTR_U32 p[] = { 0x0020ACu }; internal_test_utf8_build( euro, &euro_len, p, 1 ); }

        char const* rep = cstr_replace( s, euro, "X" );
        TESTFW_EXPECTED( strcmp( rep, "AXBXC" ) == 0 );

        /* also sanity-check that 'start' is a codepoint index */
        TESTFW_EXPECTED( cstr_find( s, euro, 2 ) == 3 );
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: find does not match inside multi-byte sequences" );
    {
        CSTR_U32 cps[] = { 0x000041u, 0x0020ACu, 0x000042u };
        char buf[ 32 ];
        CSTR_SIZE_T blen = 0;
        internal_test_utf8_build( buf, &blen, cps, 3 );
        char const* s = cstr( buf );

        char pat[ 2 ];
        pat[ 0 ] = (char)0x82;
        pat[ 1 ] = '\0';

        int pos = cstr_find( s, pat, 0 );
        TESTFW_EXPECTED( pos == -1 );
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: replace with invalid bytes is deterministic (byte-based)" );
    {
        unsigned char bad_bytes[] = { 0xC0u, 0xAFu, (unsigned char)'X' };
        char bad[ 8 ];
        CSTR_SIZE_T blen = 0;
        internal_test_bytes_build( bad, &blen, bad_bytes, 3 );

        {
            char const* rep = cstr_replace( bad, "X", "Y" );
            TESTFW_EXPECTED( rep != NULL );
            TESTFW_EXPECTED( cstr_size( rep ) == 3 );
            TESTFW_EXPECTED( (unsigned char)rep[ 0 ] == 0xC0u );
            TESTFW_EXPECTED( (unsigned char)rep[ 1 ] == 0xAFu );
            TESTFW_EXPECTED( rep[ 2 ] == 'Y' );
        }

        {
            char find[ 3 ];
            find[ 0 ] = (char)0xC0;
            find[ 1 ] = (char)0xAF;
            find[ 2 ] = '\0';
            char const* rep = cstr_replace( bad, find, "Z" );
            TESTFW_EXPECTED( rep != NULL );
            TESTFW_EXPECTED( cstr_size( rep ) == 2 );
            TESTFW_EXPECTED( rep[ 0 ] == 'Z' );
            TESTFW_EXPECTED( rep[ 1 ] == 'X' );
        }
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: internal prev/advance are consistent" );
    {
        CSTR_U32 cps[] = { 0x000041u, 0x0020ACu, 0x0000DFu, 0x000042u };
        char buf[ 32 ];
        CSTR_SIZE_T blen = 0;
        internal_test_utf8_build( buf, &blen, cps, 4 );

        char const* begin = buf;
        char const* end = buf + blen;

        char const* p = begin;
        p = internal_cstr_utf8_advance( p, end, 1 );
        TESTFW_EXPECTED( p > begin );

        p = internal_cstr_utf8_advance( p, end, 1 );
        TESTFW_EXPECTED( p > begin );

        char const* q = internal_cstr_utf8_prev( begin, p );
        TESTFW_EXPECTED( q < p );

        CSTR_U32 cp;
        CSTR_SIZE_T bytes;
        TESTFW_EXPECTED( internal_cstr_utf8_decode( q, end, &cp, &bytes ) );
        TESTFW_EXPECTED( q + bytes == p );
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: large mixed string codepoint_count stays correct" );
    {
        char buf[ 16384 ];
        CSTR_SIZE_T n = 0;
        for( int i = 0; i < 1000; ++i ) {
            buf[ n++ ] = 'A';
            n += internal_cstr_utf8_encode( 0x0020ACu, buf + n );
            n += internal_cstr_utf8_encode( 0x00010000u, buf + n );
            buf[ n++ ] = 'B';
        }
        buf[ n ] = '\0';

        TESTFW_EXPECTED( internal_test_utf8_is_valid( buf, n ) );
        TESTFW_EXPECTED( cstr_len( buf ) == (CSTR_SIZE_T)( 1000 * 4 ) );
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: behavior with invalid UTF-8 bytes (no crash, stable semantics)" );
    {
        unsigned char bad_bytes[] = { 0xC0u, 0xAFu, (unsigned char)'X' };
        char bad[ 8 ];
        CSTR_SIZE_T blen = 0;
        internal_test_bytes_build( bad, &blen, bad_bytes, 3 );

        char const* t = cstr_trim( bad );
        TESTFW_EXPECTED( t != NULL );
        TESTFW_EXPECTED( cstr_size( t ) == 3 );
        TESTFW_EXPECTED( memcmp( t, bad, 3 ) == 0 );

        char const* up = cstr_upper( bad );
        char const* lo = cstr_lower( bad );
        TESTFW_EXPECTED( up != NULL );
        TESTFW_EXPECTED( lo != NULL );
        TESTFW_EXPECTED( internal_test_utf8_is_valid( up, cstr_size( up ) ) );
        TESTFW_EXPECTED( internal_test_utf8_is_valid( lo, cstr_size( lo ) ) );
        TESTFW_EXPECTED( cstr_len( up ) == 3 );
        TESTFW_EXPECTED( cstr_len( lo ) == 3 );
    }
    TESTFW_TEST_END();

    TESTFW_TEST_BEGIN( "utf8: tokenize uses codepoint separators" );
    {
        CSTR_U32 cps[] = { 0x000041u, 0x0020ACu, 0x000042u, 0x0020ACu, 0x000043u };
        char buf[ 64 ];
        CSTR_SIZE_T blen = 0;
        internal_test_utf8_build( buf, &blen, cps, 5 );

        struct cstr_tokenizer_t tz = cstr_tokenizer( buf );
        TESTFW_EXPECTED( tz.internal != NULL );

        char euro[ 8 ];
        CSTR_SIZE_T euro_len = 0;
        { CSTR_U32 p[] = { 0x0020ACu }; internal_test_utf8_build( euro, &euro_len, p, 1 ); }

        char const* t1 = cstr_tokenize( &tz, euro );
        TESTFW_EXPECTED( t1 && strcmp( t1, "A" ) == 0 );

        char const* t2 = cstr_tokenize( &tz, euro );
        TESTFW_EXPECTED( t2 && strcmp( t2, "B" ) == 0 );

        char const* t3 = cstr_tokenize( &tz, euro );
        TESTFW_EXPECTED( t3 && strcmp( t3, "C" ) == 0 );

        char const* t4 = cstr_tokenize( &tz, euro );
        TESTFW_EXPECTED( t4 == NULL );
    }
    TESTFW_TEST_END();

    #ifdef _MSC_VER
        #pragma warning( pop )
    #endif
}

#else

void test_cstr_utf8( void ) { }

#endif /* CSTR_ASCII_ONLY */


int main( int argc, char** argv ) {
    (void) argc, (void) argv;

    TESTFW_INIT();

    test_cstr();
    test_cstr_n();
    test_cstr_size();
    test_cstr_cat();
    test_cstr_format();
    test_cstr_trim();
    test_cstr_ltrim();
    test_cstr_rtrim();
    test_cstr_left();
    test_cstr_right();
    test_cstr_mid();
    test_cstr_upper();
    test_cstr_lower();
    test_cstr_lpad();
    test_cstr_rpad();
    test_cstr_join();
    test_cstr_replace();
    test_cstr_insert();
    test_cstr_remove();
    test_cstr_int();
    test_cstr_float();
    test_cstr_starts();
    test_cstr_ends();
    test_cstr_is_equal();
    test_cstr_compare();
    test_cstr_compare_nocase();
    test_cstr_find();
    test_cstr_rfind();
    test_cstr_hash();
    test_cstr_tokenize();

    #ifndef CSTR_ASCII_ONLY
        test_cstr_utf8();
    #endif

    #ifndef CSTR_DISABLE_STRESS_TESTS
        stress_tests();
    #endif

    cstr_reset();
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

#define TESTFW_IMPLEMENTATION
#include "testfw.h"

#endif /* CSTR_RUN_TESTS */


/*
revision history:
    1.1     implemented lpad, rpad, join, replace, insert, remove, rfind
    1.0     first released version
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
