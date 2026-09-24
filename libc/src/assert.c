#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

noreturn void __cwasm_assert_fail( char const* expr, char const* file, int line ) {
    fprintf( stderr, "assertion failed: %s at %s:%d\n", expr ? expr : "(null)",
        file ? file : "(null)", line );
    abort();
    __builtin_unreachable();
}
