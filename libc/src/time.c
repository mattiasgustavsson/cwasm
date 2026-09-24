#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <stdatomic.h>
#include <cwasm.h>

__attribute__( ( import_module( "env" ), import_name( "__async_wait_ms" ) ) ) extern int cwasm_env_async_wait_ms( int ms );

__attribute__( ( import_module( "env" ), import_name( "__async_wait_frame" ) ) ) extern int cwasm_env_async_wait_frame( void );

typedef struct {
    char const* const* wday_short;
    char const* const* wday_full;
    char const* const* mon_short;
    char const* const* mon_full;
    char const* am;
    char const* pm;
} cwasm_time_locale_t;

cwasm_time_locale_t const* __cwasm_locale_time_tables( void );

CWASM_JS_LIB( TIME, double, js_perf_now_ms, ( void ), {
  return performance.now();
})

CWASM_JS_LIB( TIME, double, js_realtime_ms, ( void ), {
  return performance.timeOrigin + performance.now();
})

static void cwasm_realtime_timespec( struct timespec* tp ) {
    double ms = js_realtime_ms();
    int64_t ns = (int64_t)( ms * 1000000.0 );

    tp->tv_sec = (time_t)( ns / 1000000000LL );
    tp->tv_nsec = (long)( ns % 1000000000LL );
    if( tp->tv_nsec < 0 ) { tp->tv_nsec = 0; }
    if( tp->tv_nsec >= 1000000000L ) { tp->tv_nsec = 999999999L; }
}

#define CWASM_TIME_RANGE_SEC 8640000000000LL

static int const cwasm_mdays[ 12 ] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

clock_t clock( void ) { return (clock_t)( js_perf_now_ms() * 1000.0 ); }

double difftime( time_t time1, time_t time0 ) { return (double)time1 - (double)time0; }

time_t time( time_t* t ) {
    time_t r = (time_t)( js_realtime_ms() / 1000.0 );
    if( t ) { *t = r; }
    return r;
}

int clock_gettime( clockid_t clk_id, struct timespec* tp ) {
    if( !tp ) {
        errno = EINVAL;
        return -1;
    }

    if( clk_id == CLOCK_REALTIME ) {
        cwasm_realtime_timespec( tp );
    } else if( clk_id == CLOCK_MONOTONIC ) {
        double ms = js_perf_now_ms();
        int64_t ns = (int64_t)( ms * 1000000.0 );
        tp->tv_sec = (time_t)( ns / 1000000000LL );
        tp->tv_nsec = (long)( ns % 1000000000LL );
    } else {
        errno = EINVAL;
        return -1;
    }

    if( tp->tv_nsec < 0 ) { tp->tv_nsec = 0; }
    if( tp->tv_nsec >= 1000000000L ) { tp->tv_nsec = 999999999L; }
    return 0;
}

int nanosleep( struct timespec const* req, struct timespec* rem ) {
    struct timespec start;
    struct timespec now;
    int64_t target_ns;
    int64_t elapsed_ns;
    int64_t remain_ns;

    if( !req || req->tv_sec < 0 || req->tv_nsec < 0 || req->tv_nsec >= 1000000000L ) {
        errno = EINVAL;
        return -1;
    }

    target_ns = (int64_t)req->tv_sec * 1000000000LL + (int64_t)req->tv_nsec;
    if( target_ns <= 0 ) {
        if( rem ) {
            rem->tv_sec = 0;
            rem->tv_nsec = 0;
        }
        return 0;
    }

    if( clock_gettime( CLOCK_MONOTONIC, &start ) != 0 ) { return -1; }

    do {
        if( clock_gettime( CLOCK_MONOTONIC, &now ) != 0 ) { return -1; }
        elapsed_ns =
            ( (int64_t)now.tv_sec - (int64_t)start.tv_sec ) * 1000000000LL +
            ( (int64_t)now.tv_nsec - (int64_t)start.tv_nsec );
        if( elapsed_ns >= target_ns ) { break; }
        remain_ns = target_ns - elapsed_ns;
        #ifdef CWASM_THREADS
            static _Atomic uint32_t sleep_word;
            long long slice = remain_ns > 500000000LL ? 500000000LL : remain_ns;
            (void)__builtin_wasm_memory_atomic_wait32(
                (int*)(void*)&sleep_word, 0, slice );
        #else
            if( remain_ns > 4500000LL ) {
                int ms = (int)( ( remain_ns - 500000LL ) / 1000000LL );
                if( cwasm_env_async_wait_ms( ms ) != 0 ) { return -1; }
            } else {
                if( cwasm_env_async_wait_frame() != 0 ) { return -1; }
            }
        #endif
    } while( elapsed_ns < target_ns );

    if( rem ) {
        rem->tv_sec = 0;
        rem->tv_nsec = 0;
    }
    return 0;
}

int clock_nanosleep( clockid_t clk_id, int flags, struct timespec const* req, struct timespec* rem ) {
    struct timespec now;
    struct timespec delta;

    if( !req ) { return EINVAL; }
    if( req->tv_nsec < 0 || req->tv_nsec >= 1000000000L ) { return EINVAL; }
    if( flags & ~TIMER_ABSTIME ) { return EINVAL; }
    if( clk_id != CLOCK_REALTIME && clk_id != CLOCK_MONOTONIC ) { return EINVAL; }

    if( !( flags & TIMER_ABSTIME ) ) {
        if( nanosleep( req, rem ) != 0 ) { return errno; }
        return 0;
    }

    if( clock_gettime( clk_id, &now ) != 0 ) { return errno; }

    delta.tv_sec = req->tv_sec - now.tv_sec;
    delta.tv_nsec = req->tv_nsec - now.tv_nsec;
    if( delta.tv_nsec < 0 ) {
        delta.tv_nsec += 1000000000L;
        delta.tv_sec -= 1;
    }
    if( delta.tv_sec < 0 ) {
        if( rem ) {
            rem->tv_sec = 0;
            rem->tv_nsec = 0;
        }
        return 0;
    }
    if( nanosleep( &delta, rem ) != 0 ) { return errno; }
    return 0;
}

int timespec_get( struct timespec* ts, int base ) {
    if( !ts || base != TIME_UTC ) { return 0; }

    cwasm_realtime_timespec( ts );
    return TIME_UTC;
}

int gettimeofday( struct timeval* tv, void* tz ) {
    struct timespec ts;

    (void)tz;
    if( !tv ) {
        errno = EINVAL;
        return -1;
    }
    cwasm_realtime_timespec( &ts );
    tv->tv_sec = ts.tv_sec;
    tv->tv_usec = (long)( ts.tv_nsec / 1000L );
    if( tv->tv_usec < 0 ) { tv->tv_usec = 0; }
    if( tv->tv_usec >= 1000000L ) { tv->tv_usec = 999999L; }
    return 0;
}

static int cwasm_is_leap( int y ) {
    return ( y % 4 == 0 && y % 100 != 0 ) || ( y % 400 == 0 );
}

static int cwasm_fmt_int( char* dst, size_t dstlen, unsigned v, int width ) {
    char tmp[ 10 ];
    int i = 0;
    do {
        tmp[ i++ ] = (char)( '0' + ( v % 10 ) );
        v /= 10;
    } while( v && i < (int)sizeof( tmp ) );
    while( i < width && i < (int)sizeof( tmp ) ) { tmp[ i++ ] = '0'; }
    if( (size_t)i > dstlen ) { return -1; }
    for( int j = i - 1, k = 0; j >= 0; --j, ++k ) { dst[ k ] = tmp[ j ]; }
    return i;
}

static int cwasm_fmt_append( char* s, size_t max, size_t* len, unsigned v, int width, size_t room ) {
    char buf[ 32 ];
    int n = cwasm_fmt_int( buf, sizeof( buf ), v, width );
    if( n < 0 || *len + (size_t)n + room >= max ) { return 0; }
    for( int i = 0; i < n; ++i ) { s[( *len )++ ] = buf[ i ]; }
    return 1;
}

CWASM_JS_LIB( TIME, int32_t, js_localtime_parts, ( double sec, int32_t out ), {
  var d = new Date(sec * 1000);
  if (!isFinite(d.getTime())) return -1;
  var v = new Int32Array(mem.buffer(), out, 9);
  v[0] = d.getFullYear();
  v[1] = d.getMonth();
  v[2] = d.getDate();
  v[3] = d.getHours();
  v[4] = d.getMinutes();
  v[5] = d.getSeconds();
  v[6] = d.getDay();
  var yy = d.getFullYear();
  var cum = [0,31,59,90,120,151,181,212,243,273,304,334];
  var lp = (yy % 4 === 0 && yy % 100 !== 0) || yy % 400 === 0;
  v[7] = cum[d.getMonth()] + (d.getMonth() > 1 && lp ? 1 : 0) + d.getDate() - 1;
  var jan = new Date(yy, 0, 1).getTimezoneOffset();
  var jul = new Date(yy, 6, 1).getTimezoneOffset();
  var std = Math.max(jan, jul);
  v[8] = d.getTimezoneOffset() < std ? 1 : 0;
  return 0;
})

CWASM_JS_LIB( TIME, double, js_mktime_local, ( int32_t y, int32_t mon, int32_t mday, int32_t hour, int32_t min, int32_t sec ), {
  var d = new Date(y, mon, mday, hour, min, sec);
  var t = d.getTime();
  if (isNaN(t)) return NaN;
  return Math.floor(t / 1000);
})

CWASM_JS_LIB( TIME, int32_t, js_tz_offset_sec, ( double sec ), {
  return -new Date(sec * 1000).getTimezoneOffset() * 60;
})

CWASM_JS_LIB( TIME, int32_t, js_tz_name, ( double sec, uint32_t buf, uint32_t bufsz ), {
  var d = new Date(sec * 1000);
  var parts = new Intl.DateTimeFormat('en-US', { timeZoneName: 'short' }).formatToParts(d);
  var tz = 'UTC';
  for (var i = 0; i < parts.length; i++) {
    if (parts[i].type === 'timeZoneName') { tz = parts[i].value; break; }
  }
  var enc = new TextEncoder().encode(tz);
  var n = enc.length < bufsz - 1 ? enc.length : bufsz - 1;
  if (buf && bufsz) {
    mem.bytes_write(buf, enc.subarray(0, n));
    mem.u8_write(buf + n, 0);
  }
  return n;
})

static struct tm* cwasm_gmtime_r( time_t const* timer, struct tm* result ) {
    int64_t it;
    int64_t days;
    int64_t rem;
    int year;

    if( !timer || !result ) { return 0; }

    it = (int64_t) * timer;
    if( it > CWASM_TIME_RANGE_SEC || it < -CWASM_TIME_RANGE_SEC ) { return 0; }
    days = it / 86400;
    rem = it % 86400;
    if( rem < 0 ) {
        rem += 86400;
        --days;
    }

    int64_t const epoch_days = days; // floor-corrected above; the year/month loops eat `days`

    result->tm_hour = (int)( rem / 3600 );
    rem %= 3600;
    result->tm_min = (int)( rem / 60 );
    result->tm_sec = (int)( rem % 60 );

    year = 1970;
    if( days >= 0 ) {
        while( days > 0 ) {
            int yd = cwasm_is_leap( year ) ? 366 : 365;
            if( days >= yd ) {
                days -= yd;
                ++year;
            } else { break; }
        }
    } else {
        while( days < 0 ) {
            --year;
            days += cwasm_is_leap( year ) ? 366 : 365;
        }
    }

    result->tm_year = year - 1900;
    result->tm_yday = (int)days;

    int leap = cwasm_is_leap( year );
    int month = 0;
    while( 1 ) {
        int dm = cwasm_mdays[ month ] + ( month == 1 && leap );
        if( days >= dm ) {
            days -= dm;
            ++month;
        } else { break; }
    }
    result->tm_mon = month;
    result->tm_mday = (int)days + 1;

    int64_t wday = ( epoch_days + 4 ) % 7; // 1970-01-01 was a Thursday
    if( wday < 0 ) { wday += 7; }
    result->tm_wday = (int)wday;
    result->tm_isdst = 0;
    return result;
}

struct tm* gmtime( time_t const* timer ) {
    static struct tm buf;
    return cwasm_gmtime_r( timer, &buf );
}

struct tm* gmtime_r( time_t const* timer, struct tm* result ) {
    return cwasm_gmtime_r( timer, result );
}

static struct tm* cwasm_localtime_r( time_t const* timer, struct tm* result ) {
    int32_t parts[ 9 ];

    if( !timer || !result ) { return 0; }
    if( js_localtime_parts( (double)*timer, (int32_t)(uintptr_t)parts ) != 0 ) {
        return 0;
    }
    result->tm_year = parts[ 0 ] - 1900;
    result->tm_mon = parts[ 1 ];
    result->tm_mday = parts[ 2 ];
    result->tm_hour = parts[ 3 ];
    result->tm_min = parts[ 4 ];
    result->tm_sec = parts[ 5 ];
    result->tm_wday = parts[ 6 ];
    result->tm_yday = parts[ 7 ];
    result->tm_isdst = parts[ 8 ];
    return result;
}

struct tm* localtime( time_t const* timer ) {
    static struct tm buf;
    return cwasm_localtime_r( timer, &buf );
}

struct tm* localtime_r( time_t const* timer, struct tm* result ) {
    return cwasm_localtime_r( timer, result );
}

static int cwasm_tm_to_sec( struct tm const* tm, time_t* out ) {
    long year;
    long mon;
    double sec_d;

    if( !tm || !out ) { return -1; }

    year = tm->tm_year + 1900;
    mon = tm->tm_mon;
    while( mon < 0 ) {
        mon += 12;
        year -= 1;
    }
    while( mon > 11 ) {
        mon -= 12;
        year += 1;
    }
    if( year < 1900 ) { return -1; }

    sec_d = js_mktime_local( (int32_t)year, (int32_t)mon, (int32_t)tm->tm_mday,
        (int32_t)tm->tm_hour, (int32_t)tm->tm_min, (int32_t)tm->tm_sec );
    if( !( sec_d == sec_d ) ) { return -1; }
    *out = (time_t)sec_d;
    return 0;
}


time_t mktime( struct tm* tm ) {
    time_t sec;

    if( cwasm_tm_to_sec( tm, &sec ) != 0 ) { return (time_t)-1; }
    if( !cwasm_localtime_r( (time_t const*)&sec, tm ) ) { return (time_t)-1; }
    return sec;
}

static char const* cwasm_time_wday_short( int wday ) {
    cwasm_time_locale_t const* tl = __cwasm_locale_time_tables();
    return tl->wday_short[ wday % 7 ];
}

static char const* cwasm_time_wday_full( int wday ) {
    cwasm_time_locale_t const* tl = __cwasm_locale_time_tables();
    return tl->wday_full[ wday % 7 ];
}

static char const* cwasm_time_mon_short( int mon ) {
    cwasm_time_locale_t const* tl = __cwasm_locale_time_tables();
    return tl->mon_short[ mon % 12 ];
}

static char const* cwasm_time_mon_full( int mon ) {
    cwasm_time_locale_t const* tl = __cwasm_locale_time_tables();
    return tl->mon_full[ mon % 12 ];
}

static int cwasm_asctime_fmt( struct tm const* tm, char* buf, size_t bufsz ) {
    int p = 0;

    if( !tm || !buf || bufsz < 26u ) { return -1; }
    if( tm->tm_wday < 0 || tm->tm_wday > 6 || tm->tm_mon < 0 || tm->tm_mon > 11 ) {
        return -1;
    }

    buf[ p++ ] = cwasm_time_wday_short( tm->tm_wday )[ 0 ];
    buf[ p++ ] = cwasm_time_wday_short( tm->tm_wday )[ 1 ];
    buf[ p++ ] = cwasm_time_wday_short( tm->tm_wday )[ 2 ];
    buf[ p++ ] = ' ';
    buf[ p++ ] = cwasm_time_mon_short( tm->tm_mon )[ 0 ];
    buf[ p++ ] = cwasm_time_mon_short( tm->tm_mon )[ 1 ];
    buf[ p++ ] = cwasm_time_mon_short( tm->tm_mon )[ 2 ];
    if( tm->tm_mday < 10 ) { buf[ p++ ] = ' '; }
    buf[ p++ ] = ' ';
    int n = cwasm_fmt_int( &buf[ p ], bufsz - (size_t)p, (unsigned)tm->tm_mday, 1 );
    if( n < 1 ) { return -1; }
    p += n;
    buf[ p++ ] = ' ';
    n = cwasm_fmt_int( &buf[ p ], bufsz - (size_t)p, (unsigned)tm->tm_hour, 2 );
    if( n < 2 ) { return -1; }
    p += n;
    buf[ p++ ] = ':';
    n = cwasm_fmt_int( &buf[ p ], bufsz - (size_t)p, (unsigned)tm->tm_min, 2 );
    if( n < 2 ) { return -1; }
    p += n;
    buf[ p++ ] = ':';
    n = cwasm_fmt_int( &buf[ p ], bufsz - (size_t)p, (unsigned)tm->tm_sec, 2 );
    if( n < 2 ) { return -1; }
    p += n;
    buf[ p++ ] = ' ';
    n = cwasm_fmt_int( &buf[ p ], bufsz - (size_t)p, (unsigned)( tm->tm_year + 1900 ), 1 );
    if( n < 1 ) { return -1; }
    p += n;
    buf[ p++ ] = '\n';
    buf[ p++ ] = '\0';
    return p;
}

char* asctime( struct tm const* tm ) {
    static char buf[ 26 ];
    if( cwasm_asctime_fmt( tm, buf, sizeof( buf ) ) < 0 ) { return 0; }
    return buf;
}

char* asctime_r( struct tm const* tm, char* buf ) {
    if( !buf ) { return 0; }
    if( cwasm_asctime_fmt( tm, buf, 26 ) < 0 ) { return 0; }
    return buf;
}

char* ctime( time_t const* timer ) {
    struct tm* tm = localtime( timer );
    if( !tm ) { return 0; }
    return asctime( tm );
}

char* ctime_r( time_t const* timer, char* buf ) {
    struct tm tm_buf;
    if( !cwasm_localtime_r( timer, &tm_buf ) ) { return 0; }
    return asctime_r( &tm_buf, buf );
}

static int cwasm_fmt_append_str( char* s, size_t max, size_t* len, char const* txt, size_t room ) {
    size_t n = strlen( txt );
    if( *len + n + room >= max ) { return 0; }
    for( size_t i = 0; i < n; ++i ) { s[( *len )++ ] = txt[ i ]; }
    return 1;
}

static int cwasm_fmt_append_char( char* s, size_t max, size_t* len, char c, size_t room ) {
    if( *len + 1u + room >= max ) { return 0; }
    s[( *len )++ ] = c;
    return 1;
}

static int cwasm_week_sunday0( struct tm const* tm ) {
    return ( tm->tm_yday + 7 - tm->tm_wday ) / 7;
}

static int cwasm_week_monday0( struct tm const* tm ) {
    int w = tm->tm_wday;
    int mon = w ? w - 1 : 6;
    return ( tm->tm_yday + 7 - mon ) / 7;
}

static void cwasm_iso_week( struct tm const* tm, int* week, int* year ) {
    int y = tm->tm_year + 1900;
    int wday = tm->tm_wday == 0 ? 7 : tm->tm_wday;
    int day = tm->tm_yday + 1;
    int thursday_day = day - wday + 4;

    if( thursday_day < 1 ) {
        y -= 1;
        thursday_day += cwasm_is_leap( y ) ? 366 : 365;
    } else {
        int dim = cwasm_is_leap( y ) ? 366 : 365;
        if( thursday_day > dim ) {
            thursday_day -= dim;
            y += 1;
        }
    }
    *year = y;
    *week = ( thursday_day - 1 ) / 7 + 1;
}

static int cwasm_strftime_core( char* s, size_t max, size_t* len, char const* format,
    struct tm const* tm, time_t sec, int have_sec ) {
    for( ; *format && *len + 1 < max; ++format ) {
        if( *format != '%' ) {
            s[( *len )++ ] = *format;
            continue;
        }
        ++format;
        if( *format == 'E' || *format == 'O' ) { ++format; }
        if( !*format ) { break; }
        switch( *format ) {
        case '%':
            s[( *len )++ ] = '%';
            continue;
        case 'a':
            if( !cwasm_fmt_append_str( s, max, len, cwasm_time_wday_short( tm->tm_wday ), 0 ) ) {
                return 0;
            }
            continue;
        case 'A':
            if( !cwasm_fmt_append_str( s, max, len, cwasm_time_wday_full( tm->tm_wday ), 0 ) ) {
                return 0;
            }
            continue;
        case 'b':
        case 'h':
            if( !cwasm_fmt_append_str( s, max, len, cwasm_time_mon_short( tm->tm_mon ), 0 ) ) {
                return 0;
            }
            continue;
        case 'B':
            if( !cwasm_fmt_append_str( s, max, len, cwasm_time_mon_full( tm->tm_mon ), 0 ) ) {
                return 0;
            }
            continue;
        case 'c':
            if( !cwasm_strftime_core( s, max, len, "%a %b %e %H:%M:%S %Y", tm, sec, have_sec ) ) {
                return 0;
            }
            continue;
        case 'C':
            if( !cwasm_fmt_append( s, max, len, (unsigned)( ( tm->tm_year + 1900 ) / 100 ), 2, 0 ) ) {
                return 0;
            }
            continue;
        case 'd':
            if( !cwasm_fmt_append( s, max, len, (unsigned)tm->tm_mday, 2, 0 ) ) {
                return 0;
            }
            continue;
        case 'D':
            if( !cwasm_strftime_core( s, max, len, "%m/%d/%y", tm, sec, have_sec ) ) {
                return 0;
            }
            continue;
        case 'e':
            if( tm->tm_mday < 10 ) {
                if( !cwasm_fmt_append_char( s, max, len, ' ', 1 ) ) { return 0; }
            }
            if( !cwasm_fmt_append( s, max, len, (unsigned)tm->tm_mday, tm->tm_mday < 10 ? 1 : 2, 0 ) ) {
                return 0;
            }
            continue;
        case 'F':
            if( !cwasm_strftime_core( s, max, len, "%Y-%m-%d", tm, sec, have_sec ) ) {
                return 0;
            }
            continue;
        case 'H':
            if( !cwasm_fmt_append( s, max, len, (unsigned)tm->tm_hour, 2, 0 ) ) {
                return 0;
            }
            continue;
        case 'I': {
            unsigned h = (unsigned)( tm->tm_hour % 12 );
            if( h == 0 ) { h = 12; }
            if( !cwasm_fmt_append( s, max, len, h, 2, 0 ) ) { return 0; }
            continue;
        }
        case 'j':
            if( !cwasm_fmt_append( s, max, len, (unsigned)( tm->tm_yday + 1 ), 3, 0 ) ) {
                return 0;
            }
            continue;
        case 'm':
            if( !cwasm_fmt_append( s, max, len, (unsigned)( tm->tm_mon + 1 ), 2, 0 ) ) {
                return 0;
            }
            continue;
        case 'M':
            if( !cwasm_fmt_append( s, max, len, (unsigned)tm->tm_min, 2, 0 ) ) {
                return 0;
            }
            continue;
        case 'n':
            if( !cwasm_fmt_append_char( s, max, len, '\n', 1 ) ) { return 0; }
            continue;
        case 't':
            if( !cwasm_fmt_append_char( s, max, len, '\t', 1 ) ) { return 0; }
            continue;
        case 'p':
            if( !cwasm_fmt_append_str( s, max, len,
                tm->tm_hour < 12 ? __cwasm_locale_time_tables()->am
                : __cwasm_locale_time_tables()->pm, 0 ) ) {
                return 0;
            }
            continue;
        case 'P': {
            char const* ap = __cwasm_locale_time_tables()->am;
            char const* pm = __cwasm_locale_time_tables()->pm;
            char const* src = tm->tm_hour < 12 ? ap : pm;
            char lower[ 8 ];
            size_t i;
            for( i = 0; src[ i ] && i + 1 < sizeof( lower ); ++i ) {
                lower[ i ] = (char)( ( src[ i ] >= 'A' && src[ i ] <= 'Z' ) ? src[ i ] - 'A' + 'a' : src[ i ] );
            }
            lower[ i ] = '\0';
            if( !cwasm_fmt_append_str( s, max, len, lower, 0 ) ) { return 0; }
            continue;
        }
        case 'r':
            if( !cwasm_strftime_core( s, max, len, "%I:%M:%S %p", tm, sec, have_sec ) ) {
                return 0;
            }
            continue;
        case 'R':
            if( !cwasm_strftime_core( s, max, len, "%H:%M", tm, sec, have_sec ) ) {
                return 0;
            }
            continue;
        case 'S':
            if( !cwasm_fmt_append( s, max, len, (unsigned)tm->tm_sec, 2, 0 ) ) {
                return 0;
            }
            continue;
        case 'T':
            if( !cwasm_strftime_core( s, max, len, "%H:%M:%S", tm, sec, have_sec ) ) {
                return 0;
            }
            continue;
        case 'u': {
            unsigned d = (unsigned)( ( tm->tm_wday + 6 ) % 7 + 1 );
            if( !cwasm_fmt_append( s, max, len, d, 1, 0 ) ) { return 0; }
            continue;
        }
        case 'U':
            if( !cwasm_fmt_append( s, max, len, (unsigned)cwasm_week_sunday0( tm ), 2, 0 ) ) {
                return 0;
            }
            continue;
        case 'V': {
            int wk;
            int yr;
            cwasm_iso_week( tm, &wk, &yr );
            (void)yr;
            if( !cwasm_fmt_append( s, max, len, (unsigned)wk, 2, 0 ) ) { return 0; }
            continue;
        }
        case 'w':
            if( !cwasm_fmt_append( s, max, len, (unsigned)tm->tm_wday, 1, 0 ) ) {
                return 0;
            }
            continue;
        case 'W':
            if( !cwasm_fmt_append( s, max, len, (unsigned)cwasm_week_monday0( tm ), 2, 0 ) ) {
                return 0;
            }
            continue;
        case 'x':
            if( !cwasm_strftime_core( s, max, len, "%m/%d/%y", tm, sec, have_sec ) ) {
                return 0;
            }
            continue;
        case 'X':
            if( !cwasm_strftime_core( s, max, len, "%H:%M:%S", tm, sec, have_sec ) ) {
                return 0;
            }
            continue;
        case 'y':
            if( !cwasm_fmt_append( s, max, len, (unsigned)( ( tm->tm_year + 1900 ) % 100 ), 2, 0 ) ) {
                return 0;
            }
            continue;
        case 'Y':
            if( !cwasm_fmt_append( s, max, len, (unsigned)( tm->tm_year + 1900 ), 4, 0 ) ) {
                return 0;
            }
            continue;
        case 'G': {
            int wk;
            int yr;
            cwasm_iso_week( tm, &wk, &yr );
            (void)wk;
            if( !cwasm_fmt_append( s, max, len, (unsigned)yr, 4, 0 ) ) { return 0; }
            continue;
        }
        case 'g': {
            int wk;
            int yr;
            cwasm_iso_week( tm, &wk, &yr );
            (void)wk;
            if( !cwasm_fmt_append( s, max, len, (unsigned)( yr % 100 ), 2, 0 ) ) {
                return 0;
            }
            continue;
        }
        case 'z': {
            int off;
            if( !have_sec ) { return 0; }
            off = js_tz_offset_sec( (double)sec );
            int oh = off / 3600;
            int om = ( off < 0 ? -off : off ) % 3600 / 60;
            if( !cwasm_fmt_append_char( s, max, len, off >= 0 ? '+' : '-', 4 ) ) {
                return 0;
            }
            if( !cwasm_fmt_append( s, max, len, (unsigned)( oh < 0 ? -oh : oh ), 2, 2 ) ) {
                return 0;
            }
            if( !cwasm_fmt_append( s, max, len, (unsigned)om, 2, 0 ) ) { return 0; }
            continue;
        }
        case 'Z': {
            char tzbuf[ 64 ];
            if( !have_sec ) { return 0; }
            if( js_tz_name( (double)sec, (uint32_t)(uintptr_t)tzbuf, (uint32_t)sizeof( tzbuf ) ) < 0 ) {
                return 0;
            }
            if( !cwasm_fmt_append_str( s, max, len, tzbuf, 0 ) ) { return 0; }
            continue;
        }
        default:
            return 0;
        }
    }
    return 1;
}

size_t strftime( char* s, size_t max, char const* format, struct tm const* tm ) {
    size_t len = 0;
    time_t sec = 0;
    int have_sec;

    if( !s || !format || !tm || max == 0 ) { return 0; }

    have_sec = ( cwasm_tm_to_sec( tm, &sec ) == 0 );

    if( !cwasm_strftime_core( s, max, &len, format, tm, sec, have_sec ) ) { return 0; }
    s[ len ] = '\0';
    return len;
}
