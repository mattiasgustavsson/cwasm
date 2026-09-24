#ifndef __CWASM_TIME_H__
#define __CWASM_TIME_H__
#include <stddef.h>
#include <sys/time.h>

#define CLOCKS_PER_SEC 1000000L
#define TIME_UTC 1
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1
#define TIMER_ABSTIME 1

#ifdef __cplusplus
    extern "C" {
#endif

typedef long long clock_t;
typedef int clockid_t;

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

struct timespec {
    time_t tv_sec;
    long tv_nsec;
};

clock_t clock( void );
double difftime( time_t time1, time_t time0 );
time_t time( time_t* t );
int timespec_get( struct timespec* ts, int base );
int clock_gettime( clockid_t clk_id, struct timespec* tp );
int nanosleep( struct timespec const* req, struct timespec* rem );
int clock_nanosleep( clockid_t clk_id, int flags, struct timespec const* req, struct timespec* rem );

struct tm* gmtime( time_t const* timer );
struct tm* gmtime_r( time_t const* timer, struct tm* result );
struct tm* localtime( time_t const* timer );
struct tm* localtime_r( time_t const* timer, struct tm* result );
time_t mktime( struct tm* tm );
char* asctime( struct tm const* tm );
char* asctime_r( struct tm const* tm, char* buf );
char* ctime( time_t const* timer );
char* ctime_r( time_t const* timer, char* buf );
size_t strftime( char* s, size_t max, char const* format, struct tm const* tm );

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_TIME_H__
