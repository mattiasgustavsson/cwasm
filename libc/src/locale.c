#include <locale.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>

#include "cwasm_lock.h"

struct __cwasm_locale {
    int ctype_utf8;
    int collate_utf8;
    int numeric_utf8;
    int monetary_utf8;
    int time_utf8;
};

typedef struct {
    char const* const* wday_short;
    char const* const* wday_full;
    char const* const* mon_short;
    char const* const* mon_full;
    char const* am;
    char const* pm;
} cwasm_time_locale_t;

static int __cwasm_locale_name_utf8( char const* locale ) {
    if( !locale || !locale[ 0 ] ||
        ( locale[ 0 ] == 'C' && !locale[ 1 ] ) ||
        ( locale[ 0 ] == 'P' && locale[ 1 ] == 'O' && locale[ 2 ] == 'S' &&
        locale[ 3 ] == 'I' && locale[ 4 ] == 'X' && !locale[ 5 ] ) ) {
        return 0;
    }
    if( locale[ 0 ] == 'C' && locale[ 1 ] == '.' && locale[ 2 ] == 'U' &&
        locale[ 3 ] == 'T' && locale[ 4 ] == 'F' && locale[ 5 ] == '-' &&
        locale[ 6 ] == '8' && !locale[ 7 ] ) {
        return 1;
    }
    return -1;
}

void __cwasm_stdio_set_numeric_separators( char thousands, char decimal );

int __cwasm_locale_is_utf8;
int __cwasm_collate_utf8;
int __cwasm_mb_cur_max = 1;

int __cwasm_numeric_utf8;
int __cwasm_monetary_utf8;
int __cwasm_time_utf8;

static cwasm_lock_t g_locale_lock = CWASM_LOCK_INIT;

#ifdef CWASM_THREADS
    static _Thread_local locale_t __cwasm_tls_active_locale;
#else
    static locale_t __cwasm_active_locale;
#endif

static struct {
    struct lconv lc;
    char decimal_point[ 4 ];
    char thousands_sep[ 4 ];
    char grouping[ 4 ];
    char int_curr_symbol[ 8 ];
    char currency_symbol[ 8 ];
    char mon_decimal_point[ 4 ];
    char mon_thousands_sep[ 4 ];
    char mon_grouping[ 4 ];
    char positive_sign[ 4 ];
    char negative_sign[ 4 ];
} __cwasm_lconv;

static char const* const __cwasm_time_c_wday_short[ 7 ] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
static char const* const __cwasm_time_c_wday_full[ 7 ] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};
static char const* const __cwasm_time_c_mon_short[ 12 ] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
static char const* const __cwasm_time_c_mon_full[ 12 ] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

static cwasm_time_locale_t const __cwasm_time_locale_c = {
    __cwasm_time_c_wday_short, __cwasm_time_c_wday_full,
    __cwasm_time_c_mon_short, __cwasm_time_c_mon_full,
    "AM", "PM"
};

static cwasm_time_locale_t const* __cwasm_active_time_locale = &__cwasm_time_locale_c;

static struct __cwasm_locale __cwasm_locale_c;
static struct __cwasm_locale __cwasm_locale_utf8 = {
    .ctype_utf8 = 1, .collate_utf8 = 1,
    .numeric_utf8 = 0, .monetary_utf8 = 0, .time_utf8 = 0
};

static locale_t __cwasm_active_locale_get( void ) {
    #ifdef CWASM_THREADS
        return __cwasm_tls_active_locale;
    #else
        return __cwasm_active_locale;
    #endif
}

static void __cwasm_active_locale_set( locale_t loc ) {
    #ifdef CWASM_THREADS
        __cwasm_tls_active_locale = loc;
    #else
        __cwasm_active_locale = loc;
    #endif
}

void __cwasm_locale_reset_active( void ) {
    __cwasm_active_locale_set( NULL );
}

int __cwasm_locale_ctype_utf8( void ) {
    locale_t a = __cwasm_active_locale_get();
    if( a && a != LC_GLOBAL_LOCALE ) {
        return ( ( struct __cwasm_locale const* )a )->ctype_utf8;
    }
    return __cwasm_locale_is_utf8;
}

int __cwasm_locale_collate_utf8( void ) {
    locale_t a = __cwasm_active_locale_get();
    if( a && a != LC_GLOBAL_LOCALE ) {
        return ( ( struct __cwasm_locale const* )a )->collate_utf8;
    }
    return __cwasm_collate_utf8;
}

int __cwasm_mb_cur_max_get( void ) {
    locale_t a = __cwasm_active_locale_get();
    if( a && a != LC_GLOBAL_LOCALE ) {
        return ( ( struct __cwasm_locale const* )a )->ctype_utf8 ? 4 : 1;
    }
    return __cwasm_mb_cur_max;
}

static void __cwasm_lconv_bind( void ) {
    __cwasm_lconv.lc.decimal_point = __cwasm_lconv.decimal_point;
    __cwasm_lconv.lc.thousands_sep = __cwasm_lconv.thousands_sep;
    __cwasm_lconv.lc.grouping = __cwasm_lconv.grouping;
    __cwasm_lconv.lc.int_curr_symbol = __cwasm_lconv.int_curr_symbol;
    __cwasm_lconv.lc.currency_symbol = __cwasm_lconv.currency_symbol;
    __cwasm_lconv.lc.mon_decimal_point = __cwasm_lconv.mon_decimal_point;
    __cwasm_lconv.lc.mon_thousands_sep = __cwasm_lconv.mon_thousands_sep;
    __cwasm_lconv.lc.mon_grouping = __cwasm_lconv.mon_grouping;
    __cwasm_lconv.lc.positive_sign = __cwasm_lconv.positive_sign;
    __cwasm_lconv.lc.negative_sign = __cwasm_lconv.negative_sign;
}

static void __cwasm_apply_numeric_lconv( void ) {
    strcpy( __cwasm_lconv.decimal_point, "." );
    __cwasm_lconv.thousands_sep[ 0 ] = '\0';
    __cwasm_lconv.grouping[ 0 ] = '\0';
    __cwasm_stdio_set_numeric_separators( ',', '.' );
}

static void __cwasm_apply_monetary_lconv( void ) {
    __cwasm_lconv.int_curr_symbol[ 0 ] = '\0';
    __cwasm_lconv.currency_symbol[ 0 ] = '\0';
    __cwasm_lconv.mon_decimal_point[ 0 ] = '\0';
    __cwasm_lconv.mon_thousands_sep[ 0 ] = '\0';
    __cwasm_lconv.mon_grouping[ 0 ] = '\0';
    __cwasm_lconv.positive_sign[ 0 ] = '\0';
    __cwasm_lconv.negative_sign[ 0 ] = '\0';
    __cwasm_lconv.lc.frac_digits = __CWASM_LCONV_NOINFO;
    __cwasm_lconv.lc.p_cs_precedes = __CWASM_LCONV_NOINFO;
    __cwasm_lconv.lc.p_sep_by_space = __CWASM_LCONV_NOINFO;
    __cwasm_lconv.lc.n_cs_precedes = __CWASM_LCONV_NOINFO;
    __cwasm_lconv.lc.n_sep_by_space = __CWASM_LCONV_NOINFO;
    __cwasm_lconv.lc.p_sign_posn = __CWASM_LCONV_NOINFO;
    __cwasm_lconv.lc.n_sign_posn = __CWASM_LCONV_NOINFO;
    __cwasm_lconv.lc.int_frac_digits = __CWASM_LCONV_NOINFO;
    __cwasm_lconv.lc.int_p_cs_precedes = __CWASM_LCONV_NOINFO;
    __cwasm_lconv.lc.int_p_sep_by_space = __CWASM_LCONV_NOINFO;
    __cwasm_lconv.lc.int_n_cs_precedes = __CWASM_LCONV_NOINFO;
    __cwasm_lconv.lc.int_n_sep_by_space = __CWASM_LCONV_NOINFO;
    __cwasm_lconv.lc.int_p_sign_posn = __CWASM_LCONV_NOINFO;
    __cwasm_lconv.lc.int_n_sign_posn = __CWASM_LCONV_NOINFO;
}

static void __cwasm_apply_time_locale( void ) {
    __cwasm_active_time_locale = &__cwasm_time_locale_c;
}

void __cwasm_locale_apply_struct( struct __cwasm_locale const* loc ) {
    if( !loc ) { return; }
    __cwasm_locale_is_utf8 = loc->ctype_utf8;
    __cwasm_collate_utf8 = loc->collate_utf8;
    __cwasm_mb_cur_max = loc->ctype_utf8 ? 4 : 1;
    __cwasm_numeric_utf8 = 0;
    __cwasm_monetary_utf8 = 0;
    __cwasm_time_utf8 = 0;
    __cwasm_apply_numeric_lconv();
    __cwasm_apply_monetary_lconv();
    __cwasm_apply_time_locale();
}

char __cwasm_locale_radix_char( void ) {
    locale_t a = __cwasm_active_locale_get();

    if( a && a != LC_GLOBAL_LOCALE ) { return '.'; }
    return __cwasm_lconv.decimal_point[ 0 ] ? __cwasm_lconv.decimal_point[ 0 ] : '.';
}

cwasm_time_locale_t const* __cwasm_locale_time_tables( void ) {
    locale_t a = __cwasm_active_locale_get();

    (void)a;
    return __cwasm_active_time_locale;
}

static char const* __cwasm_locale_name_cat( int category ) {
    switch( category ) {
    case LC_NUMERIC:
    case LC_MONETARY:
    case LC_TIME:
        return "C";
    case LC_CTYPE:
        return __cwasm_locale_is_utf8 ? "C.UTF-8" : "C";
    case LC_COLLATE:
        return __cwasm_collate_utf8 ? "C.UTF-8" : "C";
    case LC_ALL:
    default:
        if( __cwasm_locale_is_utf8 == __cwasm_collate_utf8 ) {
            return __cwasm_locale_is_utf8 ? "C.UTF-8" : "C";
        }
        return __cwasm_locale_is_utf8 ? "C.UTF-8" : "C";
    }
}

char* setlocale( int category, char const* locale ) {
    int utf8;
    struct __cwasm_locale loc;
    char* ret;

    if( category != LC_ALL && category != LC_CTYPE && category != LC_COLLATE &&
        category != LC_NUMERIC && category != LC_MONETARY && category != LC_TIME ) {
        errno = EINVAL;
        return 0;
    }

    cwasm_lock( &g_locale_lock );

    if( !locale ) {
        ret = (char*)__cwasm_locale_name_cat( category );
        cwasm_unlock( &g_locale_lock );
        return ret;
    }

    utf8 = __cwasm_locale_name_utf8( locale );
    if( utf8 < 0 ) {
        errno = EINVAL;
        cwasm_unlock( &g_locale_lock );
        return 0;
    }

    switch( category ) {
    case LC_NUMERIC:
    case LC_MONETARY:
    case LC_TIME:
        ret = (char*)__cwasm_locale_name_cat( category );
        cwasm_unlock( &g_locale_lock );
        return ret;
    default:
        break;
    }

    loc.ctype_utf8 = __cwasm_locale_is_utf8;
    loc.collate_utf8 = __cwasm_collate_utf8;
    loc.numeric_utf8 = __cwasm_numeric_utf8;
    loc.monetary_utf8 = __cwasm_monetary_utf8;
    loc.time_utf8 = __cwasm_time_utf8;

    switch( category ) {
    case LC_ALL:
        loc.ctype_utf8 = utf8;
        loc.collate_utf8 = utf8;
        loc.numeric_utf8 = 0;
        loc.monetary_utf8 = 0;
        loc.time_utf8 = 0;
        break;
    case LC_CTYPE:
        loc.ctype_utf8 = utf8;
        break;
    case LC_COLLATE:
        loc.collate_utf8 = utf8;
        break;
    default:
        break;
    }

    __cwasm_locale_apply_struct( &loc );

    ret = (char*)__cwasm_locale_name_cat( category );
    cwasm_unlock( &g_locale_lock );
    return ret;
}

struct lconv* localeconv( void ) {
    static int inited;

    cwasm_lock( &g_locale_lock );
    if( !inited ) {
        __cwasm_lconv_bind();
        if( __cwasm_lconv.decimal_point[ 0 ] == '\0' ) {
            struct __cwasm_locale c = { 0, 0, 0, 0, 0 };
            __cwasm_locale_apply_struct( &c );
        }
        inited = 1;
    }
    cwasm_unlock( &g_locale_lock );
    return &__cwasm_lconv.lc;
}

static int __cwasm_locale_is_static( locale_t loc ) {
    return loc == (locale_t)&__cwasm_locale_c || loc == (locale_t)&__cwasm_locale_utf8;
}

static int __cwasm_locale_is_full_c( struct __cwasm_locale const* loc ) {
    return !loc->ctype_utf8 && !loc->collate_utf8;
}

static int __cwasm_locale_is_full_utf8( struct __cwasm_locale const* loc ) {
    return loc->ctype_utf8 && loc->collate_utf8;
}

static void __cwasm_global_locale_snapshot( struct __cwasm_locale* out ) {
    cwasm_lock( &g_locale_lock );
    out->ctype_utf8 = __cwasm_locale_is_utf8;
    out->collate_utf8 = __cwasm_collate_utf8;
    out->numeric_utf8 = __cwasm_numeric_utf8;
    out->monetary_utf8 = __cwasm_monetary_utf8;
    out->time_utf8 = __cwasm_time_utf8;
    cwasm_unlock( &g_locale_lock );
}

locale_t newlocale( int category_mask, char const* locale, locale_t base ) {
    struct __cwasm_locale tmpl;
    struct __cwasm_locale* loc;
    int utf8;

    if( base == LC_GLOBAL_LOCALE ) { base = (locale_t)0; }

    if( !category_mask ) {
        if( !base ) {
            errno = EINVAL;
            return (locale_t)0;
        }
        if( __cwasm_locale_is_static( base ) ) { return base; }
        loc = ( struct __cwasm_locale* )malloc( sizeof( *loc ) );
        if( !loc ) {
            errno = ENOMEM;
            return (locale_t)0;
        }
        *loc = *( struct __cwasm_locale* )base;
        return (locale_t)loc;
    }

    utf8 = __cwasm_locale_name_utf8( locale );
    if( utf8 < 0 ) {
        errno = EINVAL;
        return (locale_t)0;
    }

    if( category_mask & ~LC_ALL_MASK ) {
        errno = EINVAL;
        return (locale_t)0;
    }

    if( base ) { tmpl = *( struct __cwasm_locale* )base; }
    else { __cwasm_global_locale_snapshot( &tmpl ); }

    if( category_mask & LC_CTYPE_MASK ) { tmpl.ctype_utf8 = utf8; }
    if( category_mask & LC_COLLATE_MASK ) { tmpl.collate_utf8 = utf8; }

    if( __cwasm_locale_is_full_c( &tmpl ) ) { return (locale_t)&__cwasm_locale_c; }
    if( __cwasm_locale_is_full_utf8( &tmpl ) ) { return (locale_t)&__cwasm_locale_utf8; }

    loc = ( struct __cwasm_locale* )malloc( sizeof( *loc ) );
    if( !loc ) {
        errno = ENOMEM;
        return (locale_t)0;
    }
    *loc = tmpl;
    return (locale_t)loc;
}

void freelocale( locale_t loc ) {
    if( !loc || loc == LC_GLOBAL_LOCALE || __cwasm_locale_is_static( loc ) ) { return; }
    if( __cwasm_active_locale_get() == loc ) { __cwasm_active_locale_set( NULL ); }
    free( loc );
}

locale_t uselocale( locale_t loc ) {
    locale_t prev = __cwasm_active_locale_get();
    locale_t old_ret = prev ? prev : LC_GLOBAL_LOCALE;

    if( !loc ) { return old_ret; }
    if( loc == LC_GLOBAL_LOCALE ) {
        __cwasm_active_locale_set( NULL );
        return old_ret;
    }
    __cwasm_active_locale_set( loc );
    return old_ret;
}

#define CWASM_LOC_CALL(loc, expr) \
do { \
    locale_t _wl_old = __cwasm_active_locale_get(); \
    if (loc) \
        __cwasm_active_locale_set((loc) == LC_GLOBAL_LOCALE ? (locale_t)0 : (loc)); \
    expr; \
    if (loc) \
        __cwasm_active_locale_set(_wl_old); \
} while (0)

float strtof_l( char const* nptr, char** endptr, locale_t loc ) {
    float r;
    CWASM_LOC_CALL( loc, r = strtof( nptr, endptr ) );
    return r;
    }

double strtod_l( char const* nptr, char** endptr, locale_t loc ) {
    double r;
    CWASM_LOC_CALL( loc, r = strtod( nptr, endptr ) );
    return r;
}

long double strtold_l( char const* nptr, char** endptr, locale_t loc ) {
    long double r;
    CWASM_LOC_CALL( loc, r = strtold( nptr, endptr ) );
    return r;
}

long strtol_l( char const* nptr, char** endptr, int base, locale_t loc ) {
    long r;
    CWASM_LOC_CALL( loc, r = strtol( nptr, endptr, base ) );
    return r;
}

unsigned long strtoul_l( char const* nptr, char** endptr, int base, locale_t loc ) {
    unsigned long r;
    CWASM_LOC_CALL( loc, r = strtoul( nptr, endptr, base ) );
    return r;
}

long long strtoll_l( char const* nptr, char** endptr, int base, locale_t loc ) {
    long long r;
    CWASM_LOC_CALL( loc, r = strtoll( nptr, endptr, base ) );
    return r;
}

unsigned long long strtoull_l( char const* nptr, char** endptr, int base, locale_t loc ) {
    unsigned long long r;
    CWASM_LOC_CALL( loc, r = strtoull( nptr, endptr, base ) );
    return r;
}

int strcoll_l( char const* s1, char const* s2, locale_t loc ) {
    int r;
    CWASM_LOC_CALL( loc, r = strcoll( s1, s2 ) );
    return r;
}

size_t strxfrm_l( char* dest, char const* src, size_t n, locale_t loc ) {
    size_t r;
    CWASM_LOC_CALL( loc, r = strxfrm( dest, src, n ) );
    return r;
}

int toupper_l( int c, locale_t loc ) {
    int r;
    CWASM_LOC_CALL( loc, r = toupper( c ) );
    return r;
}

int tolower_l( int c, locale_t loc ) {
    int r;
    CWASM_LOC_CALL( loc, r = tolower( c ) );
    return r;
}

int wcscoll_l( wchar_t const* s1, wchar_t const* s2, locale_t loc ) {
    int r;
    CWASM_LOC_CALL( loc, r = wcscoll( s1, s2 ) );
    return r;
}

size_t wcsxfrm_l( wchar_t* dest, wchar_t const* src, size_t n, locale_t loc ) {
    size_t r;
    CWASM_LOC_CALL( loc, r = wcsxfrm( dest, src, n ) );
    return r;
}

int iswctype_l( wint_t wc, wctype_t desc, locale_t loc ) {
    int r;
    CWASM_LOC_CALL( loc, r = iswctype( wc, desc ) );
    return r;
}

int iswspace_l( wint_t wc, locale_t loc ) {
    int r;
    CWASM_LOC_CALL( loc, r = iswspace( wc ) );
    return r;
}

int iswprint_l( wint_t wc, locale_t loc ) {
    int r;
    CWASM_LOC_CALL( loc, r = iswprint( wc ) );
    return r;
}

int iswcntrl_l( wint_t wc, locale_t loc ) {
    int r;
    CWASM_LOC_CALL( loc, r = iswcntrl( wc ) );
    return r;
}

int iswupper_l( wint_t wc, locale_t loc ) {
    int r;
    CWASM_LOC_CALL( loc, r = iswupper( wc ) );
    return r;
}

int iswlower_l( wint_t wc, locale_t loc ) {
    int r;
    CWASM_LOC_CALL( loc, r = iswlower( wc ) );
    return r;
}

int iswalpha_l( wint_t wc, locale_t loc ) {
    int r;
    CWASM_LOC_CALL( loc, r = iswalpha( wc ) );
    return r;
}

int iswblank_l( wint_t wc, locale_t loc ) {
    int r;
    CWASM_LOC_CALL( loc, r = iswblank( wc ) );
    return r;
}

int iswdigit_l( wint_t wc, locale_t loc ) {
    int r;
    CWASM_LOC_CALL( loc, r = iswdigit( wc ) );
    return r;
}

int iswpunct_l( wint_t wc, locale_t loc ) {
    int r;
    CWASM_LOC_CALL( loc, r = iswpunct( wc ) );
    return r;
}

int iswxdigit_l( wint_t wc, locale_t loc ) {
    int r;
    CWASM_LOC_CALL( loc, r = iswxdigit( wc ) );
    return r;
}

wint_t towupper_l( wint_t wc, locale_t loc ) {
    wint_t r;
    CWASM_LOC_CALL( loc, r = towupper( wc ) );
    return r;
}

wint_t towlower_l( wint_t wc, locale_t loc ) {
    wint_t r;
    CWASM_LOC_CALL( loc, r = towlower( wc ) );
    return r;
}

size_t strftime_l( char* s, size_t max, char const* format, struct tm const* tm, locale_t loc ) {
    size_t r;
    CWASM_LOC_CALL( loc, r = strftime( s, max, format, tm ) );
    return r;
}

size_t mbrlen_l( char const* s, size_t n, mbstate_t* ps, locale_t loc ) {
    size_t r;
    CWASM_LOC_CALL( loc, r = mbrlen( s, n, ps ) );
    return r;
}

int mbtowc_l( wchar_t* pwc, char const* pmb, size_t max, locale_t loc ) {
    int r;
    CWASM_LOC_CALL( loc, r = mbtowc( pwc, pmb, max ) );
    return r;
}

size_t wcsnrtombs_l( char* dest, wchar_t const** src, size_t nwc, size_t len, mbstate_t* ps, locale_t loc ) {
    size_t r;
    CWASM_LOC_CALL( loc, r = wcsnrtombs( dest, src, nwc, len, ps ) );
    return r;
}

size_t mbsnrtowcs_l( wchar_t* dest, char const** src, size_t nms, size_t len, mbstate_t* ps, locale_t loc ) {
    size_t r;
    CWASM_LOC_CALL( loc, r = mbsnrtowcs( dest, src, nms, len, ps ) );
    return r;
}
