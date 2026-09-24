#ifndef __CWASM_WCTYPE_H__
#define __CWASM_WCTYPE_H__
#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

typedef uint32_t wctype_t;
typedef uint32_t wctrans_t;


#ifdef __cplusplus
    extern "C" {
#endif

int iswalpha( wint_t wc );
int iswdigit( wint_t wc );
int iswalnum( wint_t wc );
int iswspace( wint_t wc );
int iswblank( wint_t wc );
int iswlower( wint_t wc );
int iswupper( wint_t wc );
int iswxdigit( wint_t wc );
int iswcntrl( wint_t wc );
int iswprint( wint_t wc );
int iswgraph( wint_t wc );
int iswpunct( wint_t wc );
wint_t towlower( wint_t wc );
wint_t towupper( wint_t wc );

wctype_t wctype( char const* property );
int iswctype( wint_t wc, wctype_t desc );
wctrans_t wctrans( char const* property );
wint_t towctrans( wint_t wc, wctrans_t desc );

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_WCTYPE_H__
