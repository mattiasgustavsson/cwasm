#ifndef __CWASM_STRING_H__
#define __CWASM_STRING_H__
#include <stddef.h>
#include <errno.h>

#ifdef __cplusplus
    extern "C" {
#endif

void* memcpy( void* dst, void const* src, size_t n );
void* memmove( void* dst, void const* src, size_t n );
void* memset( void* s, int c, size_t n );
int memcmp( void const* s1, void const* s2, size_t n );
void* memchr( void const* s, int c, size_t n );
void* mempcpy( void* dst, void const* src, size_t n );
void* memccpy( void* dst, void const* src, int c, size_t n );
size_t strlen( char const* s );
int strcasecmp( char const* a, char const* b );
int strncasecmp( char const* a, char const* b, size_t n );
char* strcpy( char* d, char const* s );
char* strncpy( char* d, char const* s, size_t n );
char* strcat( char* d, char const* s );
char* strncat( char* d, char const* s, size_t n );
int strcmp( char const* a, char const* b );
int strncmp( char const* a, char const* b, size_t n );
char* strchr( char const* s, int c );
char* strrchr( char const* s, int c );
char* index( char const* s, int c );
char* rindex( char const* s, int c );
char* strchrnul( char const* s, int c );
size_t strnlen( char const* s, size_t maxlen );
size_t strspn( char const* s, char const* accept );
size_t strcspn( char const* s, char const* reject );
char* strpbrk( char const* s, char const* accept );
char* strstr( char const* haystack, char const* needle );
char* strcasestr( char const* h, char const* n );
int strcoll( char const* s1, char const* s2 );
size_t strxfrm( char* dest, char const* src, size_t n );

char* strtok_r( char* s, char const* delim, char** saveptr );
char* strtok( char* s, char const* delim );

char* strerror( int errnum );
int strerror_r( int errnum, char* buf, size_t buflen );
char* strdup( char const* s );
char* strndup( char const* s, size_t n );
size_t strlcpy( char* dst, char const* src, size_t size );
size_t strlcat( char* dst, char const* src, size_t size );
char* stpcpy( char* dst, char const* src );
char* stpncpy( char* dst, char const* src, size_t n );
int strverscmp( char const* a, char const* b );

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_STRING_H__
