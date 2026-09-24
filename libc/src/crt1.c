#include <stdlib.h>

#include "cmdline.h"
#include <cwasm.h>

#ifdef CWASM_THREADS
    void cwasm_start_main( void );
#endif

extern int __main_argc_argv( int argc, char** argv ) __attribute__( ( weak ) );
extern int __original_main( void ) __attribute__( ( weak ) );
extern void __wasm_call_ctors( void ) __attribute__( ( weak ) );

CWASM_JS_LIB_INIT( CRT,
(
    CWASM.runtime.core.onInstantiated = function (exports, msg) {
        if (typeof CWASM.runtime.thread.onInstantiated === "function")
            return CWASM.runtime.thread.onInstantiated(exports, msg);
        return new Promise(function (resolve, reject) {
            if (!exports || typeof exports._start !== "function") {
                reject(new Error("CWASM: no _start"));
                return;
            }
            if (typeof CWASM.runtime.core.run !== "function") {
                reject(new Error("CWASM: missing runtime.core.run"));
                return;
            }
            CWASM.runtime.core.run(exports._start, resolve, reject);
        });
    };
),
void, js_crt_host_ready, ( void ), {})

__attribute__( ( export_name( "_start" ) ) ) void _start( void ) {
    js_crt_host_ready();

    #ifdef CWASM_THREADS
        cwasm_start_main();
    #endif

    int argc = 0;
    char** argv = cwasm_build_argv( &argc );

    int ret;

    if( __wasm_call_ctors ) { __wasm_call_ctors(); }

    if( __main_argc_argv ) {
        ret = __main_argc_argv( argc, argv );
    } else {
        ret = __original_main();
    }
    cwasm_free_argv( argc, argv );
    exit( ret );
    __builtin_unreachable();
}
