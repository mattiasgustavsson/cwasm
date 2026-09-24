#ifndef __CWASM_CMDLINE_H__
#define __CWASM_CMDLINE_H__

#include <stddef.h>

char** cwasm_build_argv( int* argc_out );
void cwasm_free_argv( int argc, char** argv );
int cwasm_fetch_cmdline_prefix( char* out, size_t outsz );
int cwasm_fetch_envstring( char* out, size_t outsz );
char* cwasm_getenv( char const* name );

#endif // __CWASM_CMDLINE_H__
