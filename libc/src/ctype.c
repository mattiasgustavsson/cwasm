#include <ctype.h>
#include <stdio.h>

int __cwasm_locale_ctype_utf8( void );

static int cwasm_ctype_is_ascii( int c ) {
    return c != EOF && (unsigned)c < 128u;
}

static int cwasm_ctype_nonascii( int c ) {
    return __cwasm_locale_ctype_utf8() && !cwasm_ctype_is_ascii( c );
}

int isupper( int c ) {
    if( c == EOF || cwasm_ctype_nonascii( c ) ) { return 0; }
    return (unsigned)c - 'A' < 26u;
}

int islower( int c ) {
    if( c == EOF || cwasm_ctype_nonascii( c ) ) { return 0; }
    return (unsigned)c - 'a' < 26u;
}

int isalpha( int c ) {
    return isupper( c ) || islower( c );
}

int isdigit( int c ) {
    if( c == EOF || cwasm_ctype_nonascii( c ) ) { return 0; }
    return (unsigned)c - '0' < 10u;
}

int isxdigit( int c ) {
    if( c == EOF || cwasm_ctype_nonascii( c ) ) { return 0; }
    return isdigit( c ) || (unsigned)c - 'A' < 6u || (unsigned)c - 'a' < 6u;
}

int isspace( int c ) {
    if( c == EOF || cwasm_ctype_nonascii( c ) ) { return 0; }
    return c == ' ' || (unsigned)c - '\t' < 5u;
}

int ispunct( int c ) {
    if( c == EOF || cwasm_ctype_nonascii( c ) ) { return 0; }
    return ( c > 0x20 && c < 0x7f ) && !isalpha( c ) && !isdigit( c );
}

int isalnum( int c ) {
    return isalpha( c ) || isdigit( c );
}

int isprint( int c ) {
    if( c == EOF || cwasm_ctype_nonascii( c ) ) { return 0; }
    return (unsigned)c - 0x20u < 0x5fu;
}

int isgraph( int c ) {
    if( c == EOF || cwasm_ctype_nonascii( c ) ) { return 0; }
    return (unsigned)c - 0x21u < 0x5eu;
}

int isblank( int c ) {
    if( c == EOF || cwasm_ctype_nonascii( c ) ) { return 0; }
    return c == ' ' || c == '\t';
}

int iscntrl( int c ) {
    if( c == EOF || cwasm_ctype_nonascii( c ) ) { return 0; }
    return (unsigned)c < 0x20u || c == 0x7f;
}

int tolower( int c ) {
    if( c == EOF || cwasm_ctype_nonascii( c ) ) { return c; }
    return ( c >= 'A' && c <= 'Z' ) ? c + 0x20 : c;
}

int toupper( int c ) {
    if( c == EOF || cwasm_ctype_nonascii( c ) ) { return c; }
    return ( c >= 'a' && c <= 'z' ) ? c - 0x20 : c;
}
