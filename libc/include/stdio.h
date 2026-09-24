#ifndef __CWASM_STDIO_H__
#define __CWASM_STDIO_H__
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct FILE FILE;

typedef struct {
    off_t __off;
    unsigned char __mbstate_rd[ 16 ];
    unsigned char __mbstate_wr[ 16 ];
} fpos_t;

#ifndef NULL
    #define NULL ((void*)0)
#endif

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

#define BUFSIZ 4096
#define EOF (-1)

#define FOPEN_MAX 1024
#define FILENAME_MAX 4096
#define L_tmpnam 64
#define __CWASM_STDIO_UNGET 8

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define TMP_MAX 0xFFFFFFFFu

extern FILE __cwasm_stdin;
extern FILE __cwasm_stdout;
extern FILE __cwasm_stderr;

#define stdin (&__cwasm_stdin)
#define stdout (&__cwasm_stdout)
#define stderr (&__cwasm_stderr)

FILE* fdopen( int fd, char const* mode );

int remove( char const* filename );
int rename( char const* oldpath, char const* newpath );
FILE* tmpfile( void );
char* tmpnam( char* s );

FILE* fopen( char const* filename, char const* mode );
FILE* freopen( char const* filename, char const* mode, FILE* stream );
int fclose( FILE* stream );
int fflush( FILE* stream );
int fileno( FILE* stream );
int fwide( FILE* stream, int mode );
void setbuf( FILE* stream, char* buf );
int setvbuf( FILE* stream, char* buf, int mode, size_t size );

size_t fread( void* ptr, size_t size, size_t nmemb, FILE* stream );
size_t fwrite( void const* ptr, size_t size, size_t nmemb, FILE* stream );

int fseek( FILE* stream, long offset, int whence );
long ftell( FILE* stream );
int fseeko( FILE* stream, off_t offset, int whence );
off_t ftello( FILE* stream );
void rewind( FILE* stream );
int fgetpos( FILE* stream, fpos_t* pos );
int fsetpos( FILE* stream, fpos_t const* pos );

void clearerr( FILE* stream );
int feof( FILE* stream );
int ferror( FILE* stream );

int fgetc( FILE* stream );
int ungetc( int c, FILE* stream );
int fputc( int c, FILE* stream );
int fputs( char const* s, FILE* stream );
char* fgets( char* s, int n, FILE* stream );

int getc( FILE* stream );
int putc( int c, FILE* stream );

#ifndef __cplusplus
    #define getc(stream) fgetc(stream)
    #define putc(c, stream) fputc((c), (stream))
#endif

int getchar( void );
int putchar( int c );
int puts( char const* s );

int vsnprintf( char* str, size_t size, char const* format, va_list ap );
int vasprintf( char** strp, char const* format, va_list ap );
int vsprintf( char* str, char const* format, va_list ap );
int snprintf( char* str, size_t size, char const* format, ... );
int sprintf( char* str, char const* format, ... );
int vfprintf( FILE* stream, char const* format, va_list ap );
int fprintf( FILE* stream, char const* format, ... );
int vprintf( char const* format, va_list ap );
int printf( char const* format, ... );
void perror( char const* s );

int vsscanf( char const* s, char const* format, va_list ap );
int sscanf( char const* s, char const* format, ... );
int vfscanf( FILE* stream, char const* format, va_list ap );
int fscanf( FILE* stream, char const* format, ... );
int vscanf( char const* format, va_list ap );
int scanf( char const* format, ... );

void flockfile( FILE* stream );
void funlockfile( FILE* stream );
int ftrylockfile( FILE* stream );

#if __STDC_VERSION__ < 201112L
    char* gets( char* s );
#endif

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_STDIO_H__
