#ifndef __CWASM_LOCALE_H__
#define __CWASM_LOCALE_H__
// Only "C" and "C.UTF-8" are supported.
// C.UTF-8 enables UTF-8 ctype/collate only; numeric, monetary, and time stay "C".
// setlocale(LC_NUMERIC|LC_MONETARY|LC_TIME, name) accepts only "C", "POSIX",
// or "C.UTF-8" as no-ops; other names return NULL with EINVAL.
// In C.UTF-8, <ctype.h> classifies ASCII bytes only; use <wctype.h> for Unicode.
// Collation: "C" is bytewise strcmp; C.UTF-8 uses NFD, strips combining marks,
// and case-folds before compare (accent- and case-insensitive primary).

#include <limits.h>
#include <stddef.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>

#define LC_ALL 0
#define LC_COLLATE 1
#define LC_CTYPE 2
#define LC_MONETARY 3
#define LC_NUMERIC 4
#define LC_TIME 5
#define LC_MESSAGES 6

struct lconv {
    char* decimal_point;
    char* thousands_sep;
    char* grouping;

    char* int_curr_symbol;
    char* currency_symbol;
    char* mon_decimal_point;
    char* mon_thousands_sep;
    char* mon_grouping;
    char* positive_sign;
    char* negative_sign;
    char int_frac_digits;
    char frac_digits;
    char p_cs_precedes;
    char p_sep_by_space;
    char n_cs_precedes;
    char n_sep_by_space;
    char p_sign_posn;
    char n_sign_posn;

    char int_p_cs_precedes;
    char int_p_sep_by_space;
    char int_n_cs_precedes;
    char int_n_sep_by_space;
    char int_p_sign_posn;
    char int_n_sign_posn;
};

#define __CWASM_LCONV_NOINFO ((char)0x7f)

#define LC_COLLATE_MASK (1 << LC_COLLATE)
#define LC_CTYPE_MASK (1 << LC_CTYPE)
#define LC_MONETARY_MASK (1 << LC_MONETARY)
#define LC_NUMERIC_MASK (1 << LC_NUMERIC)
#define LC_TIME_MASK (1 << LC_TIME)
#define LC_MESSAGES_MASK (1 << LC_MESSAGES)
#define LC_ALL_MASK      (LC_COLLATE_MASK | LC_CTYPE_MASK | LC_MONETARY_MASK | \
    LC_NUMERIC_MASK | LC_TIME_MASK | LC_MESSAGES_MASK)

typedef struct __cwasm_locale* locale_t;

#define LC_GLOBAL_LOCALE ((locale_t)(void *)-1)

#ifdef __cplusplus
    extern "C" {
#endif

char* setlocale( int category, char const* locale );
struct lconv* localeconv( void );

locale_t newlocale( int category_mask, char const* locale, locale_t base );
void freelocale( locale_t loc );
locale_t uselocale( locale_t loc );

float strtof_l( char const* nptr, char** endptr, locale_t loc );
double strtod_l( char const* nptr, char** endptr, locale_t loc );
long double strtold_l( char const* nptr, char** endptr, locale_t loc );
long strtol_l( char const* nptr, char** endptr, int base, locale_t loc );
unsigned long strtoul_l( char const* nptr, char** endptr, int base, locale_t loc );
long long strtoll_l( char const* nptr, char** endptr, int base, locale_t loc );
unsigned long long strtoull_l( char const* nptr, char** endptr, int base, locale_t loc );
int strcoll_l( char const* s1, char const* s2, locale_t loc );
size_t strxfrm_l( char* dest, char const* src, size_t n, locale_t loc );
int toupper_l( int c, locale_t loc );
int tolower_l( int c, locale_t loc );
int wcscoll_l( wchar_t const* s1, wchar_t const* s2, locale_t loc );
size_t wcsxfrm_l( wchar_t* dest, wchar_t const* src, size_t n, locale_t loc );
int iswctype_l( wint_t wc, wctype_t desc, locale_t loc );
int iswspace_l( wint_t wc, locale_t loc );
int iswprint_l( wint_t wc, locale_t loc );
int iswcntrl_l( wint_t wc, locale_t loc );
int iswupper_l( wint_t wc, locale_t loc );
int iswlower_l( wint_t wc, locale_t loc );
int iswalpha_l( wint_t wc, locale_t loc );
int iswblank_l( wint_t wc, locale_t loc );
int iswdigit_l( wint_t wc, locale_t loc );
int iswpunct_l( wint_t wc, locale_t loc );
int iswxdigit_l( wint_t wc, locale_t loc );
wint_t towupper_l( wint_t wc, locale_t loc );
wint_t towlower_l( wint_t wc, locale_t loc );
size_t strftime_l( char* s, size_t max, char const* format, struct tm const* tm, locale_t loc );
size_t mbrlen_l( char const* s, size_t n, mbstate_t* ps, locale_t loc );
int mbtowc_l( wchar_t* pwc, char const* pmb, size_t max, locale_t loc );
size_t wcsnrtombs_l( char* dest, wchar_t const** src, size_t nwc, size_t len, mbstate_t* ps, locale_t loc );
size_t mbsnrtowcs_l( wchar_t* dest, char const** src, size_t nms, size_t len, mbstate_t* ps, locale_t loc );

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_LOCALE_H__
