#ifndef __CWASM_SIGNAL_H__
#define __CWASM_SIGNAL_H__
#include <errno.h>

// Handlers are in-process only (no OS delivery, as there's no OS for wasm)

typedef int sig_atomic_t;
typedef void ( *sig_handler_t )( int );

#define SIG_DFL ((sig_handler_t)0)
#define SIG_IGN ((sig_handler_t)-2)
#define SIG_ERR ((sig_handler_t)-1)

#define SIGABRT 6
#define SIGFPE 8
#define SIGILL 4
#define SIGKILL 9
#define SIGSEGV 11
#define SIGPIPE 13
#define SIGALRM 14
#define SIGTERM 15
#define SIGUSR1 16
#define SIGUSR2 17
#define SIGCHLD 18
#define SIGCONT 19
#define SIGSTOP 20
#define SIGTSTP 21
#define SIGTTIN 22
#define SIGTTOU 23
#define SIGINT 2
#define SIGHUP 1

#define NSIG 32


#ifdef __cplusplus
    extern "C" {
#endif

sig_handler_t signal( int sig, sig_handler_t handler );
int raise( int sig );

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_SIGNAL_H__
