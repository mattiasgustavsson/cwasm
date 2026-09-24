#include <signal.h>
#include <errno.h>
#include <stdlib.h>

#include "cwasm_lock.h"

static sig_handler_t __cwasm_signal_table[ NSIG ];
static cwasm_lock_t g_signal_lock = CWASM_LOCK_INIT;

static void __cwasm_default_signal_action( int sig ) {
    (void)sig;
    abort();
}

sig_handler_t signal( int sig, sig_handler_t handler ) {
    sig_handler_t old;

    if( sig <= 0 || sig >= NSIG || sig == SIGKILL || sig == SIGSTOP ) {
        errno = EINVAL;
        return SIG_ERR;
    }

    cwasm_lock( &g_signal_lock );
    old = __cwasm_signal_table[ sig ];
    __cwasm_signal_table[ sig ] = handler;
    cwasm_unlock( &g_signal_lock );
    return old;
}

int raise( int sig ) {
    sig_handler_t handler;

    if( sig <= 0 || sig >= NSIG ) {
        errno = EINVAL;
        return -1;
    }

    cwasm_lock( &g_signal_lock );
    handler = __cwasm_signal_table[ sig ];
    cwasm_unlock( &g_signal_lock );

    if( handler == SIG_ERR ) {
        errno = EINVAL;
        return -1;
    }

    if( handler == SIG_IGN ) { return 0; }

    if( handler == SIG_DFL || sig == SIGKILL || sig == SIGSTOP ) {
        __cwasm_default_signal_action( sig );
        return 0;
    }

    handler( sig );
    return 0;
}
