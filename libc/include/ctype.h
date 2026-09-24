#ifndef __CWASM_CTYPE_H__
#define __CWASM_CTYPE_H__
// Byte ctype is ASCII-only (EOF and unsigned char values below 0x80).
// In C.UTF-8, non-ASCII UTF-8 bytes are not single characters here;
// use <wctype.h> (isw*) for Unicode code points.

#ifdef __cplusplus
    extern "C" {
#endif

int isupper( int c );
int islower( int c );
int isalpha( int c );
int isdigit( int c );
int isxdigit( int c );
int isspace( int c );
int ispunct( int c );
int isalnum( int c );
int isprint( int c );
int isgraph( int c );
int isblank( int c );
int iscntrl( int c );

int tolower( int c );
int toupper( int c );

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_CTYPE_H__
