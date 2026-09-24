// cwasm __cxa_atexit -- register C++ global destructors via atexit(3).

extern "C" int __cwasm_cxa_atexit( void ( *func )( void* ), void* arg );

extern "C" int __cxa_atexit( void ( *func )( void* ), void* arg, void* dso_handle ) {
    (void)dso_handle;
    if( !func ) { return 0; }
    return __cwasm_cxa_atexit( func, arg );
}
