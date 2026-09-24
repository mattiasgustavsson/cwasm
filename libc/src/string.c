#include <stddef.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <wasm_simd128.h>

int __cwasm_locale_collate_utf8( void );
int __cwasm_strcoll_utf8( char const* s1, char const* s2 );
size_t __cwasm_strxfrm_utf8( char* dest, char const* src, size_t n );

static const struct { int code; char const* msg; } cwasm_errlist[] = {
    { EPERM, "Operation not permitted" },
    { ENOENT, "No such file or directory" },
    { ESRCH, "No such process" },
    { EINTR, "Interrupted system call" },
    { EIO, "Input/output error" },
    { EBADF, "Bad file descriptor" },
    { EAGAIN, "Resource temporarily unavailable" },
    { ENOMEM, "Out of memory" },
    { EACCES, "Permission denied" },
    { EBUSY, "Device or resource busy" },
    { EEXIST, "File exists" },
    { EXDEV, "Cross-device link" },
    { ENODEV, "No such device" },
    { ENOTDIR, "Not a directory" },
    { EISDIR, "Is a directory" },
    { EINVAL, "Invalid argument" },
    { EMFILE, "Too many open files" },
    { ENOSPC, "No space left on device" },
    { ESPIPE, "Illegal seek" },
    { EDOM, "Domain error" },
    { ERANGE, "Result too large" },
    { EDEADLK, "Resource deadlock avoided" },
    { ENAMETOOLONG, "File name too long" },
    { ENOSYS, "Function not implemented" },
    { ENOTEMPTY, "Directory not empty" },
    { EILSEQ, "Illegal byte sequence" },
    { ENOTSUP, "Operation not supported" },
    { ETIMEDOUT, "Connection timed out" },
};

#define CWASM_ERRLIST_N ( sizeof( cwasm_errlist ) / sizeof( cwasm_errlist[ 0 ] ) )

void* memset( void* s, int c, size_t n ) {
    if( n ) { __builtin_memset( s, c, n ); }
    return s;
}

void* memcpy( void* dst, void const* src, size_t n ) {
    if( n ) { __builtin_memcpy( dst, src, n ); }
    return dst;
}

void* memmove( void* dst, void const* src, size_t n ) {
    if( n ) { __builtin_memmove( dst, src, n ); }
    return dst;
}

int memcmp( void const* s1, void const* s2, size_t n ) {
    unsigned char const* a = (unsigned char const*)s1;
    unsigned char const* b = (unsigned char const*)s2;

    while( n >= 16 ) {
        v128_t va = wasm_v128_load( a );
        v128_t vb = wasm_v128_load( b );
        if( !wasm_i8x16_all_true( wasm_i8x16_eq( va, vb ) ) ) {
            for( size_t i = 0; i < 16; ++i ) {
                if( a[ i ] != b[ i ] ) { return (int)a[ i ] - (int)b[ i ]; }
            }
        }
        a += 16;
        b += 16;
        n -= 16;
    }
    while( n-- ) {
        if( *a != *b ) { return (int)*a - (int)*b; }
        ++a;
        ++b;
    }
    return 0;
}

void* memchr( void const* s, int c, size_t n ) {
    unsigned char const* p = (unsigned char const*)s;
    unsigned char ch = (unsigned char)c;
    v128_t needle = wasm_i8x16_splat( (int8_t)ch );

    while( n >= 16 ) {
        v128_t chunk = wasm_v128_load( p );
        v128_t eq = wasm_i8x16_eq( chunk, needle );
        if( wasm_v128_any_true( eq ) ) {
            for( size_t i = 0; i < 16; ++i ) {
                if( p[ i ] == ch ) { return (void*)( p + i ); }
            }
        }
        p += 16;
        n -= 16;
    }
    while( n-- ) {
        if( *p == ch ) { return (void*)p; }
        ++p;
    }
    return NULL;
}

void* memccpy( void* dst, void const* src, int c, size_t n ) {
    unsigned char* d = (unsigned char*)dst;
    unsigned char const* s = (unsigned char const*)src;
    unsigned char ch = (unsigned char)c;
    while( n-- ) {
        if( ( *d++ = *s++ ) == ch ) { return d; }
    }
    return NULL;
}

void* mempcpy( void* dst, void const* src, size_t n ) {
    return (unsigned char*)memcpy( dst, src, n ) + n;
}

size_t strlen( char const* s ) {
    char const* p = s;
    v128_t zero = wasm_i8x16_splat( 0 );

    if( !s ) { return 0; }
    // An aligned v128 load cannot straddle the end of linear memory, an unaligned one can.
    while( (uintptr_t)p & 15u ) {
        if( !*p ) { return (size_t)( p - s ); }
        ++p;
    }
    for( ;; ) {
        v128_t chunk = wasm_v128_load( p );
        v128_t eq = wasm_i8x16_eq( chunk, zero );
        if( wasm_v128_any_true( eq ) ) {
            while( *p ) { ++p; }
            return (size_t)( p - s );
        }
        p += 16;
    }
}

int strncasecmp( char const* a, char const* b, size_t n ) {
    unsigned char ca;
    unsigned char cb;
    if( !a || !b ) {
        errno = EINVAL;
        if( !a && !b ) { return 0; }
        return a ? 1 : -1;
    }
    while( n-- ) {
        ca = (unsigned char)*a++;
        cb = (unsigned char)*b++;
        if( ca >= 'A' && ca <= 'Z' ) { ca = (unsigned char)( ca - 'A' + 'a' ); }
        if( cb >= 'A' && cb <= 'Z' ) { cb = (unsigned char)( cb - 'A' + 'a' ); }
        if( !ca || ca != cb ) { return (int)ca - (int)cb; }
    }
    return 0;
}

int strcasecmp( char const* a, char const* b ) {
    return strncasecmp( a, b, (size_t)-1 );
}

char* strcpy( char* d, char const* s ) {
    char* r = d;
    while( ( *d++ = *s++ ) ) {}
    return r;
}

char* strncpy( char* d, char const* s, size_t n ) {
    char* r = d;
    while( n && *s ) {
        *d++ = *s++;
        --n;
    }
    while( n-- ) { *d++ = 0; }
    return r;
}

char* strcat( char* d, char const* s ) {
    char* r = d;
    while( *d ) { ++d; }
    while( ( *d++ = *s++ ) ) {}
    return r;
}

char* strncat( char* d, char const* s, size_t n ) {
    char* r = d;
    while( *d ) { ++d; }
    while( n-- && ( *d = *s ) ) { d++; s++; }
    *d = 0;
    return r;
}

int strcmp( char const* a, char const* b ) {
    while( *a && *a == *b ) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp( char const* a, char const* b, size_t n ) {
    while( n && *a && *a == *b ) { a++; b++; n--; }
    return n ? (unsigned char)*a - (unsigned char)*b : 0;
}

char* strchr( char const* s, int c ) {
    char ch = (char)c;
    for( ;; ) {
        if( *s == ch ) { return (char*)s; }
        if( !*s ) { return NULL; }
        ++s;
    }
}

char* strrchr( char const* s, int c ) {
    char const* last = NULL;
    char ch = (char)c;
    do {
        if( *s == ch ) { last = s; }
    } while( *s++ );
    return (char*)last;
}

char* index( char const* s, int c ) { return strchr( s, c ); }
char* rindex( char const* s, int c ) { return strrchr( s, c ); }

char* strchrnul( char const* s, int c ) {
    char ch = (char)c;
    while( *s && *s != ch ) { ++s; }
    return (char*)s;
}

size_t strnlen( char const* s, size_t maxlen ) {
    size_t n = 0;
    while( n < maxlen && s[ n ] ) { ++n; }
    return n;
}

size_t strspn( char const* s, char const* accept ) {
    char const* p;
    size_t n = 0;
    for( ; *s; ++s ) {
        for( p = accept; *p && *p != *s; ++p ) { ; }
        if( !*p ) { break; }
        ++n;
    }
    return n;
}

size_t strcspn( char const* s, char const* reject ) {
    char const* p;
    size_t n = 0;
    for( ; *s; ++s ) {
        for( p = reject; *p && *p != *s; ++p ) { ; }
        if( *p ) { break; }
        ++n;
    }
    return n;
}

char* strpbrk( char const* s, char const* accept ) {
    char const* p;
    for( ; *s; ++s ) {
        for( p = accept; *p; ++p ) {
            if( *p == *s ) { return (char*)s; }
        }
    }
    return NULL;
}

char* strstr( char const* haystack, char const* needle ) {
    if( !*needle ) { return (char*)haystack; }
    for( ; *haystack; ++haystack ) {
        char const* h = haystack;
        char const* n = needle;
        while( *h && *n && *h == *n ) {
            ++h;
            ++n;
        }
        if( !*n ) { return (char*)haystack; }
    }
    return NULL;
}

char* strcasestr( char const* h, char const* n ) {
    if( !*n ) { return (char*)h; }
    for( ; *h; ++h ) {
        char const* hp = h;
        char const* np = n;
        while( *hp && *np ) {
            unsigned char a = (unsigned char)*hp;
            unsigned char b = (unsigned char)*np;
            if( a >= 'A' && a <= 'Z' ) { a = (unsigned char)( a - 'A' + 'a' ); }
            if( b >= 'A' && b <= 'Z' ) { b = (unsigned char)( b - 'A' + 'a' ); }
            if( a != b ) { break; }
            ++hp;
            ++np;
        }
        if( !*np ) { return (char*)h; }
    }
    return NULL;
}

int strcoll( char const* s1, char const* s2 ) {
    if( __cwasm_locale_collate_utf8() ) { return __cwasm_strcoll_utf8( s1, s2 ); }
    return strcmp( s1, s2 );
}

static char* __cwasm_strtok_save;

char* strtok_r( char* s, char const* delim, char** saveptr ) {
    char* token;

    if( !s ) { s = *saveptr; }
    if( !s ) { return NULL; }
    s += strspn( s, delim );
    if( !*s ) {
        *saveptr = NULL;
        return NULL;
    }
    token = s;
    s += strcspn( s, delim );
    if( *s ) {
        *s++ = 0;
        *saveptr = s;
    } else {
        *saveptr = NULL;
    }
    return token;
}

char* strtok( char* s, char const* delim ) {
    return strtok_r( s, delim, &__cwasm_strtok_save );
}

static char const* cwasm_strerror_msg( int errnum ) {
    size_t i;
    for( i = 0; i < CWASM_ERRLIST_N; ++i ) {
        if( cwasm_errlist[ i ].code == errnum ) { return cwasm_errlist[ i ].msg; }
    }
    return NULL;
}

int strerror_r( int errnum, char* buf, size_t buflen ) {
    char const* msg;

    if( !buf || buflen == 0 ) { return EINVAL; }
    msg = cwasm_strerror_msg( errnum );
    if( msg ) {
        size_t n = strlen( msg );
        if( n + 1 > buflen ) { return ERANGE; }
        memcpy( buf, msg, n + 1 );
        return 0;
    }
    if( snprintf( buf, buflen, "Unknown error %d", errnum ) >= (int)buflen ) {
        return ERANGE;
    }
    return 0;
}

char* strerror( int errnum ) {
    static char buf[ 64 ];
    if( strerror_r( errnum, buf, sizeof( buf ) ) != 0 ) {
        return (char*)"Unknown error";
    }
    return buf;
}

char* strdup( char const* s ) {
    size_t len = strlen( s );
    char* p = (char*)malloc( len + 1 );
    if( !p ) { return NULL; }
    memcpy( p, s, len + 1 );
    return p;
}

char* strndup( char const* s, size_t n ) {
    size_t len = strnlen( s, n );
    char* p = (char*)malloc( len + 1 );
    if( !p ) { return NULL; }
    memcpy( p, s, len );
    p[ len ] = 0;
    return p;
}

size_t strlcpy( char* dst, char const* src, size_t size ) {
    size_t len = strlen( src );
    if( size ) {
        size_t copy = ( len < size - 1 ) ? len : size - 1;
        memcpy( dst, src, copy );
        dst[ copy ] = 0;
    }
    return len;
}

size_t strxfrm( char* dest, char const* src, size_t n ) {
    if( __cwasm_locale_collate_utf8() ) { return __cwasm_strxfrm_utf8( dest, src, n ); }
    return strlcpy( dest, src, n );
}

size_t strlcat( char* dst, char const* src, size_t size ) {
    size_t dstlen = strnlen( dst, size );
    if( dstlen == size ) { return dstlen + strlen( src ); }
    size_t space = size - dstlen - 1;
    size_t srclen = strlen( src );
    size_t copy = ( srclen < space ) ? srclen : space;
    memcpy( dst + dstlen, src, copy );
    dst[ dstlen + copy ] = 0;
    return dstlen + srclen;
}

char* stpcpy( char* dst, char const* src ) {
    while( ( *dst = *src ) ) { dst++; src++; }
    return dst;
}

char* stpncpy( char* dst, char const* src, size_t n ) {
    while( n ) {
        *dst = *src;
        if( *src == 0 ) {
            char* ret = dst;
            dst++; src++; n--;
            while( n-- ) { *dst++ = 0; }
            return ret;
        }
        dst++; src++; n--;
    }
    return dst;
}

static int cwasm_vers_isdigit( unsigned char c ) {
    return c >= '0' && c <= '9';
}

int strverscmp( char const* a, char const* b ) {
    unsigned char const* s1 = (unsigned char const*)a;
    unsigned char const* s2 = (unsigned char const*)b;

    if( !a || !b ) { return a ? 1 : ( b ? -1 : 0 ); }

    for( ;; ) {
        if( !*s1 && !*s2 ) { return 0; }

        if( !cwasm_vers_isdigit( *s1 ) && !cwasm_vers_isdigit( *s2 ) ) {
            if( *s1 != *s2 ) { return (int)*s1 - (int)*s2; }
            if( !*s1 ) { return 0; }
            ++s1;
            ++s2;
            continue;
        }

        if( cwasm_vers_isdigit( *s1 ) && cwasm_vers_isdigit( *s2 ) ) {
            unsigned char const* z1 = s1;
            unsigned char const* z2 = s2;
            unsigned char const* e1;
            unsigned char const* e2;

            while( *z1 == '0' ) { ++z1; }
            while( *z2 == '0' ) { ++z2; }

            e1 = z1;
            while( cwasm_vers_isdigit( *e1 ) ) { ++e1; }
            e2 = z2;
            while( cwasm_vers_isdigit( *e2 ) ) { ++e2; }

            if( (size_t)( e1 - z1 ) != (size_t)( e2 - z2 ) ) {
                return ( e1 - z1 ) < ( e2 - z2 ) ? -1 : 1;
            }

            while( z1 < e1 ) {
                if( *z1 != *z2 ) { return (int)*z1 - (int)*z2; }
                ++z1;
                ++z2;
            }

            s1 = e1;
            s2 = e2;
            continue;
        }

        if( *s1 != *s2 ) { return (int)*s1 - (int)*s2; }
        if( !*s1 ) { return 0; }
        ++s1;
        ++s2;
    }
}
