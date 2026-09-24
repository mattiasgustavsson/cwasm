#ifndef __CWASM_SCHED_H__
#define __CWASM_SCHED_H__
#ifdef __cplusplus
    extern "C" {
#endif

struct sched_param {
    int sched_priority;
};

int sched_yield( void );

#define SCHED_OTHER 0
#define SCHED_FIFO 1
#define SCHED_RR 2

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_SCHED_H__
