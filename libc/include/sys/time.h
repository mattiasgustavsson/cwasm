#ifndef __CWASM_SYS_TIME_H__
#define __CWASM_SYS_TIME_H__

typedef long long time_t;

struct timeval {
    time_t tv_sec;
    long tv_usec;
};


#ifdef __cplusplus
    extern "C" {
#endif

int gettimeofday( struct timeval* tv, void* tz );
int utimes( char const* path, struct timeval const times[ 2 ] );

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_SYS_TIME_H__
