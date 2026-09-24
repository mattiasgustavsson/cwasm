#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#include "wscanset.h"

extern int __cwasm_stdio_orient_wide( FILE* stream );
extern char __cwasm_locale_radix_char( void );

enum cwasm_wlen {
    CWASM_WLEN_NONE,
    CWASM_WLEN_hh,
    CWASM_WLEN_h,
    CWASM_WLEN_l,
    CWASM_WLEN_ll,
    CWASM_WLEN_j,
    CWASM_WLEN_z,
    CWASM_WLEN_t,
    CWASM_WLEN_L,
};

typedef struct {
    wchar_t* ws;
    size_t n;
    size_t len;
    int err;
} cwasm_wout;

static void cwasm_wout_put( cwasm_wout* o, wchar_t wc ) {
    if( o->err ) { return; }
    if( o->n > 0 && o->ws && o->len + 1 < o->n ) { o->ws[ o->len ] = wc; }
    ++o->len;
}

static void cwasm_wout_repeat( cwasm_wout* o, wchar_t wc, int count ) {
    while( count-- > 0 ) { cwasm_wout_put( o, wc ); }
}

static void cwasm_wout_put_narrow( cwasm_wout* o, char const* s, int nn ) {
    for( int i = 0; i < nn; ++i ) { cwasm_wout_put( o, (unsigned char)s[ i ] ); }
}

static size_t cwasm_mbs_wide_count( char const* s, int prec_bytes ) {
    mbstate_t st;
    size_t wide_count = 0;
    size_t byte_count = 0;

    memset( &st, 0, sizeof( st ) );
    if( !s ) { s = "(null)"; }

    while( *s ) {
        wchar_t wc;
        size_t r;

        if( prec_bytes >= 0 && (int)byte_count >= prec_bytes ) { break; }
        r = mbrtowc( &wc, s, (size_t)( prec_bytes >= 0 ? prec_bytes - byte_count : 4 ), &st );
        if( r == 0 ) { break; }
        if( r == (size_t)-1 || r == (size_t)-2 ) { return (size_t)-1; }
        if( prec_bytes >= 0 && byte_count + r > (size_t)prec_bytes ) { break; }
        s += r;
        byte_count += r;
        ++wide_count;
    }
    return wide_count;
}

static size_t cwasm_wout_put_mbs( cwasm_wout* o, char const* s, int prec_bytes ) {
    mbstate_t st;
    size_t wide_count = 0;
    size_t byte_count = 0;

    memset( &st, 0, sizeof( st ) );
    if( !s ) { s = "(null)"; }

    while( *s ) {
        wchar_t wc;
        size_t r;

        if( prec_bytes >= 0 && (int)byte_count >= prec_bytes ) { break; }
        r = mbrtowc( &wc, s, (size_t)( prec_bytes >= 0 ? prec_bytes - byte_count : 4 ), &st );
        if( r == 0 ) { break; }
        if( r == (size_t)-1 || r == (size_t)-2 ) {
            o->err = 1;
            errno = EILSEQ;
            return wide_count;
        }
        if( prec_bytes >= 0 && byte_count + r > (size_t)prec_bytes ) { break; }
        cwasm_wout_put( o, wc );
        s += r;
        byte_count += r;
        ++wide_count;
    }
    return wide_count;
}

static size_t cwasm_ws_count( wchar_t const* s, int prec_wide ) {
    size_t i = 0;

    if( !s ) { s = L"(null)"; }
    while( s[ i ] && ( prec_wide < 0 || (int)i < prec_wide ) ) { ++i; }
    return i;
}

static size_t cwasm_wout_put_ws( cwasm_wout* o, wchar_t const* s, int prec_wide ) {
    size_t i = 0;

    if( !s ) { s = L"(null)"; }
    while( s[ i ] && ( prec_wide < 0 || (int)i < prec_wide ) ) {
        cwasm_wout_put( o, s[ i ] );
        ++i;
    }
    return i;
}

static int cwasm_wbuild_narrow_fmt( char* out, size_t outsz, wchar_t conv, enum cwasm_wlen len,
    int left, int plus, int space, int alt, int zero,
    int width, int width_specified,
    int prec, int prec_specified ) {
    int n = 0;
    int m;

    if( conv > 127 ) { return -1; }

    if( outsz == 0 ) { return -1; }

    out[ n++ ] = '%';
    if( left ) { out[ n++ ] = '-'; }
    if( plus ) { out[ n++ ] = '+'; }
    if( space ) { out[ n++ ] = ' '; }
    if( alt ) { out[ n++ ] = '#'; }
    if( zero ) { out[ n++ ] = '0'; }

    if( width_specified ) {
        m = snprintf( out + n, outsz - (size_t)n, "%d", width );
        if( m < 0 || (size_t)m >= outsz - (size_t)n ) { return -1; }
        n += m;
    }

    if( prec_specified && prec >= 0 ) {
        m = snprintf( out + n, outsz - (size_t)n, ".%d", prec );
        if( m < 0 || (size_t)m >= outsz - (size_t)n ) { return -1; }
        n += m;
    }

    switch( len ) {
    case CWASM_WLEN_hh:
        if( (size_t)n + 2 >= outsz ) { return -1; }
        out[ n++ ] = 'h';
        out[ n++ ] = 'h';
        break;
    case CWASM_WLEN_h:
        if( (size_t)n + 1 >= outsz ) { return -1; }
        out[ n++ ] = 'h';
        break;
    case CWASM_WLEN_l:
        if( (size_t)n + 1 >= outsz ) { return -1; }
        out[ n++ ] = 'l';
        break;
    case CWASM_WLEN_ll:
        if( (size_t)n + 2 >= outsz ) { return -1; }
        out[ n++ ] = 'l';
        out[ n++ ] = 'l';
        break;
    case CWASM_WLEN_j:
        if( (size_t)n + 1 >= outsz ) { return -1; }
        out[ n++ ] = 'j';
        break;
    case CWASM_WLEN_z:
        if( (size_t)n + 1 >= outsz ) { return -1; }
        out[ n++ ] = 'z';
        break;
    case CWASM_WLEN_t:
        if( (size_t)n + 1 >= outsz ) { return -1; }
        out[ n++ ] = 't';
        break;
    case CWASM_WLEN_L:
        if( conv == L'f' || conv == L'F' || conv == L'e' || conv == L'E' ||
            conv == L'g' || conv == L'G' || conv == L'a' || conv == L'A' ) {
            // wasm32 long double is double; narrow snprintf has no %L support.
            break;
        }
        if( (size_t)n + 1 >= outsz ) { return -1; }
        out[ n++ ] = 'L';
        break;
    default:
        break;
    }

    if( (size_t)n + 2 > outsz ) { return -1; }
    out[ n++ ] = (char)conv;
    out[ n ] = '\0';
    return 0;
}

static void cwasm_va_skip( va_list* ap, wchar_t conv, enum cwasm_wlen len ) {
    switch( conv ) {
    case L'd':
    case L'i':
        switch( len ) {
        case CWASM_WLEN_hh:
        case CWASM_WLEN_h:
            (void)va_arg( *ap, int );
            break;
        case CWASM_WLEN_l:
            (void)va_arg( *ap, long );
            break;
        case CWASM_WLEN_ll:
            (void)va_arg( *ap, long long );
            break;
        case CWASM_WLEN_j:
            (void)va_arg( *ap, intmax_t );
            break;
        case CWASM_WLEN_z:
            (void)va_arg( *ap, size_t );
            break;
        case CWASM_WLEN_t:
            (void)va_arg( *ap, ptrdiff_t );
            break;
        default:
            (void)va_arg( *ap, int );
            break;
        }
        break;
    case L'u':
    case L'o':
    case L'x':
    case L'X':
        switch( len ) {
        case CWASM_WLEN_hh:
        case CWASM_WLEN_h:
            (void)va_arg( *ap, unsigned int );
            break;
        case CWASM_WLEN_l:
            (void)va_arg( *ap, unsigned long );
            break;
        case CWASM_WLEN_ll:
            (void)va_arg( *ap, unsigned long long );
            break;
        case CWASM_WLEN_j:
            (void)va_arg( *ap, uintmax_t );
            break;
        case CWASM_WLEN_z:
            (void)va_arg( *ap, size_t );
            break;
        case CWASM_WLEN_t:
            (void)va_arg( *ap, size_t );
            break;
        default:
            (void)va_arg( *ap, unsigned int );
            break;
        }
        break;
    case L'f':
    case L'F':
    case L'e':
    case L'E':
    case L'g':
    case L'G':
    case L'a':
    case L'A':
        if( len == CWASM_WLEN_L ) { (void)va_arg( *ap, long double ); }
        else { (void)va_arg( *ap, double ); }
        break;
    case L'c':
        if( len == CWASM_WLEN_l ) { (void)va_arg( *ap, wint_t ); }
        else { (void)va_arg( *ap, int ); }
        break;
    case L's':
        if( len == CWASM_WLEN_l ) { (void)va_arg( *ap, wchar_t* ); }
        else { (void)va_arg( *ap, char* ); }
        break;
    case L'p':
        (void)va_arg( *ap, void* );
        break;
    default:
        break;
    }
}

static int cwasm_wconv_is_narrow( wchar_t conv ) {
    switch( conv ) {
    case L'd':
    case L'i':
    case L'u':
    case L'o':
    case L'x':
    case L'X':
    case L'f':
    case L'F':
    case L'e':
    case L'E':
    case L'g':
    case L'G':
    case L'a':
    case L'A':
    case L'p':
        return 1;
    default:
        return 0;
    }
}

static int cwasm_vsnprintf_dyn( char const* fmt, va_list* ap, char** out, int* out_owned ) {
    char stack[ 128 ];
    char* buf = stack;
    size_t cap = sizeof( stack );
    va_list ap2;
    int nn;

    *out_owned = 0;
    for( ;; ) {
        va_copy( ap2, *ap );
        nn = vsnprintf( buf, cap, fmt, ap2 );
        va_end( ap2 );
        if( nn < 0 ) { return nn; }
        if( (size_t)nn + 1 <= cap ) {
            *out = buf;
            return nn;
        }
        cap = (size_t)nn + 1;
        if( buf != stack ) { free( buf ); }
        buf = (char*)malloc( cap );
        if( !buf ) {
            errno = ENOMEM;
            return -1;
        }
        *out_owned = 1;
    }
}

static void cwasm_wout_store_n( cwasm_wout* o, enum cwasm_wlen len, va_list* ap ) {
    switch( len ) {
    case CWASM_WLEN_hh:
        *va_arg( *ap, signed char* ) = (signed char)o->len;
        break;
    case CWASM_WLEN_h:
        *va_arg( *ap, short* ) = (short)o->len;
        break;
    case CWASM_WLEN_l:
        *va_arg( *ap, long* ) = (long)o->len;
        break;
    case CWASM_WLEN_ll:
        *va_arg( *ap, long long* ) = (long long)o->len;
        break;
    case CWASM_WLEN_j:
        *va_arg( *ap, intmax_t* ) = (intmax_t)o->len;
        break;
    case CWASM_WLEN_z:
        *va_arg( *ap, size_t* ) = o->len;
        break;
    case CWASM_WLEN_t:
        *va_arg( *ap, ptrdiff_t* ) = (ptrdiff_t)o->len;
        break;
    default:
        *va_arg( *ap, int* ) = (int)o->len;
        break;
    }
}

static int cwasm_wout_emit_spec( cwasm_wout* o, wchar_t const* end,
    enum cwasm_wlen len, int width, int prec,
    int width_specified, int prec_specified,
    int left, int plus, int space, int alt, int zero,
    va_list* ap ) {
    wchar_t conv = end[ -1 ];
    size_t out;

    if( conv == L'n' ) {
        cwasm_wout_store_n( o, len, ap );
        return 0;
    }

    if( conv == L'c' ) {
        wchar_t wc;

        if( len == CWASM_WLEN_l ) {
            wc = (wchar_t)va_arg( *ap, wint_t );
        } else {
            char ch = (char)va_arg( *ap, int );
            mbstate_t st;
            memset( &st, 0, sizeof( st ) );
            if( mbrtowc( &wc, &ch, 1, &st ) == (size_t)-1 ) {
                o->err = 1;
                errno = EILSEQ;
                return -1;
            }
        }

        if( !left && width > 1 ) {
            cwasm_wout_repeat( o, zero ? L'0' : L' ', width - 1 );
        }
        cwasm_wout_put( o, wc );
        if( left && width > 1 ) { cwasm_wout_repeat( o, L' ', width - 1 ); }
        return 0;
    }

    if( conv == L's' ) {
        if( len == CWASM_WLEN_l ) {
            wchar_t const* ws = va_arg( *ap, wchar_t const* );
            out = cwasm_ws_count( ws, prec );
            if( !left && width > (int)out ) {
                cwasm_wout_repeat( o, L' ', width - (int)out );
            }
            if( cwasm_wout_put_ws( o, ws, prec ) != out || o->err ) { return -1; }
        } else {
            char const* s = va_arg( *ap, char const* );
            out = cwasm_mbs_wide_count( s, prec );
            if( out == (size_t)-1 ) {
                o->err = 1;
                errno = EILSEQ;
                return -1;
            }
            if( !left && width > (int)out ) {
                cwasm_wout_repeat( o, L' ', width - (int)out );
            }
            if( cwasm_wout_put_mbs( o, s, prec ) != out || o->err ) { return -1; }
        }
        if( left && width > (int)out ) {
            cwasm_wout_repeat( o, L' ', width - (int)out );
        }
        return 0;
    }

    if( !cwasm_wconv_is_narrow( conv ) ) {
        o->err = 1;
        errno = EINVAL;
        return -1;
    }

    char nfmt[ 64 ];
    char* nbuf;
    int nbuf_owned;
    int nn;

    if( cwasm_wbuild_narrow_fmt( nfmt, sizeof( nfmt ), conv, len, left, plus, space, alt,
        zero, width, width_specified, prec, prec_specified ) != 0 ) {
        o->err = 1;
        errno = EINVAL;
        return -1;
    }

    if( len == CWASM_WLEN_L &&
        ( conv == L'f' || conv == L'F' || conv == L'e' || conv == L'E' ||
        conv == L'g' || conv == L'G' || conv == L'a' || conv == L'A' ) ) {
        long double ld = va_arg( *ap, long double );
        char stack[ 128 ];
        char* buf = stack;
        size_t cap = sizeof( stack );
        int owned = 0;

        for( ;; ) {
            nn = snprintf( buf, cap, nfmt, (double)ld );
            if( nn < 0 ) {
                if( owned ) { free( buf ); }
                o->err = 1;
                return -1;
            }
            if( (size_t)nn + 1 <= cap ) { break; }
            cap = (size_t)nn + 1;
            if( owned ) { free( buf ); }
            buf = (char*)malloc( cap );
            if( !buf ) {
                o->err = 1;
                errno = ENOMEM;
                return -1;
            }
            owned = 1;
        }
        cwasm_wout_put_narrow( o, buf, nn );
        if( owned ) { free( buf ); }
        return 0;
    }

    nn = cwasm_vsnprintf_dyn( nfmt, ap, &nbuf, &nbuf_owned );
    if( nn < 0 ) {
        o->err = 1;
        return -1;
    }
    cwasm_va_skip( ap, conv, len );
    cwasm_wout_put_narrow( o, nbuf, nn );
    if( nbuf_owned ) { free( nbuf ); }
    return 0;
}

static int cwasm_vswprintf_core( wchar_t* restrict ws, size_t n,
    wchar_t const* restrict format, va_list ap ) {
    cwasm_wout out = { .ws = ws, .n = n, .len = 0, .err = 0 };

    if( !format ) {
        errno = EINVAL;
        return -1;
    }

    while( *format ) {
        if( *format != L'%' ) {
            cwasm_wout_put( &out, *format++ );
            continue;
        }

        int left = 0;
        int plus = 0;
        int space = 0;
        int alt = 0;
        int zero = 0;
        int width = 0;
        int width_specified = 0;
        int prec = -1;
        int prec_specified = 0;
        enum cwasm_wlen len = CWASM_WLEN_NONE;

        ++format;
        if( *format == L'%' ) {
            cwasm_wout_put( &out, L'%' );
            ++format;
            continue;
        }

        while( *format == L'-' || *format == L'+' || *format == L' ' ||
            *format == L'#' || *format == L'0' ) {
            if( *format == L'-' ) {
                left = 1;
                zero = 0;
            }
            if( *format == L'+' ) { plus = 1; }
            if( *format == L' ' ) { space = 1; }
            if( *format == L'#' ) { alt = 1; }
            if( *format == L'0' && !left ) { zero = 1; }
            ++format;
        }

        if( *format == L'*' ) {
            width = va_arg( ap, int );
            width_specified = 1;
            if( width < 0 ) {
                left = 1;
                zero = 0;
                width = -width;
            }
            ++format;
        } else {
            while( *format >= L'0' && *format <= L'9' ) {
                width_specified = 1;
                width = width * 10 + (int)( *format++ - L'0' );
            }
        }

        if( *format == L'.' ) {
            ++format;
            prec_specified = 1;
            if( *format == L'*' ) {
                prec = va_arg( ap, int );
                ++format;
                if( prec < 0 ) {
                    prec = -1;
                    prec_specified = 0;
                }
            } else {
                prec = 0;
                while( *format >= L'0' && *format <= L'9' ) {
                    prec = prec * 10 + (int)( *format++ - L'0' );
                }
            }
        }

        if( *format == L'h' ) {
            ++format;
            len = ( *format == L'h' ) ? ( format++, CWASM_WLEN_hh ) : CWASM_WLEN_h;
        } else if( *format == L'l' ) {
            ++format;
            len = ( *format == L'l' ) ? ( format++, CWASM_WLEN_ll ) : CWASM_WLEN_l;
        } else if( *format == L'j' ) {
            len = CWASM_WLEN_j;
            ++format;
        } else if( *format == L'z' ) {
            len = CWASM_WLEN_z;
            ++format;
        } else if( *format == L't' ) {
            len = CWASM_WLEN_t;
            ++format;
        } else if( *format == L'L' ) {
            len = CWASM_WLEN_L;
            ++format;
        }

        if( !*format ) {
            out.err = 1;
            errno = EINVAL;
            break;
        }

        ++format;
        if( cwasm_wout_emit_spec( &out, format, len, width, prec,
            width_specified, prec_specified,
            left, plus, space, alt, zero, &ap ) != 0 ) {
            break;
        }
    }

    if( out.n > 0 && out.ws ) {
        size_t term = out.len < out.n ? out.len : out.n - 1;
        out.ws[ term ] = L'\0';
    }
    return out.err ? -1 : (int)out.len;
}

int vswprintf( wchar_t* restrict ws, size_t n, wchar_t const* restrict format, va_list ap ) {
    int r = cwasm_vswprintf_core( ws, n, format, ap );

    if( r < 0 || (size_t)r >= n ) { return -1; }
    return r;
}

int swprintf( wchar_t* restrict ws, size_t n, wchar_t const* restrict format, ... ) {
    va_list ap;
    int r;

    va_start( ap, format );
    r = vswprintf( ws, n, format, ap );
    va_end( ap );
    return r;
}

static int cwasm_wprintf_stream( FILE* stream, wchar_t const* format, va_list ap ) {
    wchar_t stack[ 256 ];
    wchar_t* buf = stack;
    size_t cap = sizeof( stack ) / sizeof( wchar_t );
    va_list ap2;
    int r;

    for( ;; ) {
        va_copy( ap2, ap );
        r = cwasm_vswprintf_core( buf, cap, format, ap2 );
        va_end( ap2 );
        if( r < 0 ) {
            if( buf != stack ) { free( buf ); }
            return r;
        }
        if( (size_t)r + 1 <= cap ) { break; }
        cap = (size_t)r + 1;
        if( buf != stack ) { free( buf ); }
        buf = (wchar_t*)malloc( cap * sizeof( wchar_t ) );
        if( !buf ) {
            errno = ENOMEM;
            return -1;
        }
    }

    if( fputws( buf, stream ) == EOF ) { r = -1; }

    if( buf != stack ) { free( buf ); }
    return r < 0 ? -1 : r;
}

int vfwprintf( FILE* restrict stream, wchar_t const* restrict format, va_list ap ) {
    int r;

    if( !stream ) {
        errno = EINVAL;
        return -1;
    }
    flockfile( stream );
    if( __cwasm_stdio_orient_wide( stream ) != 0 ) {
        funlockfile( stream );
        return -1;
    }
    r = cwasm_wprintf_stream( stream, format, ap );
    funlockfile( stream );
    return r;
}

int fwprintf( FILE* restrict stream, wchar_t const* restrict format, ... ) {
    va_list ap;
    int r;

    va_start( ap, format );
    r = vfwprintf( stream, format, ap );
    va_end( ap );
    return r;
}

int vwprintf( wchar_t const* restrict format, va_list ap ) {
    return vfwprintf( stdout, format, ap );
}

int wprintf( wchar_t const* restrict format, ... ) {
    va_list ap;
    int r;

    va_start( ap, format );
    r = vwprintf( format, ap );
    va_end( ap );
    return r;
}



enum {
    CWASM_WSCAN_OK = 1,
    CWASM_WSCAN_MATCH_FAIL = 0,
    CWASM_WSCAN_INPUT_FAIL = -1
};

enum cwasm_wscan_src {
    CWASM_WSCAN_SRC_STR,
    CWASM_WSCAN_SRC_FILE
};

enum cwasm_wscan_len {
    CWASM_WSCAN_LEN_NONE,
    CWASM_WSCAN_LEN_HH,
    CWASM_WSCAN_LEN_H,
    CWASM_WSCAN_LEN_L,
    CWASM_WSCAN_LEN_LL,
    CWASM_WSCAN_LEN_J,
    CWASM_WSCAN_LEN_Z,
    CWASM_WSCAN_LEN_T,
    CWASM_WSCAN_LEN_CAP_L
};

#ifndef CWASM_WSCAN_PB_N
    #define CWASM_WSCAN_PB_N 64
#endif

typedef struct {
    enum cwasm_wscan_src src;
    void* ctx;
    size_t count;
    wint_t small_pb[ CWASM_WSCAN_PB_N ];
    wint_t* pb;
    int pb_count;
    int pb_cap;
    int pb_heap;
    int oom;
    int restore_error;
} cwasm_wscan_in;

static void cwasm_wscan_init( cwasm_wscan_in* in, enum cwasm_wscan_src src, void* ctx ) {
    in->src = src;
    in->ctx = ctx;
    in->count = 0;
    in->pb = in->small_pb;
    in->pb_count = 0;
    in->pb_cap = CWASM_WSCAN_PB_N;
    in->pb_heap = 0;
    in->oom = 0;
    in->restore_error = 0;
}

static size_t cwasm_wscan_wchar_nbytes( wchar_t wc ) {
    char tmp[ MB_LEN_MAX ];
    mbstate_t st;
    size_t n;

    memset( &st, 0, sizeof( st ) );
    n = wcrtomb( tmp, wc, &st );
    return ( n == (size_t)-1 ) ? 1u : n;
}

static void cwasm_wscan_finish( cwasm_wscan_in* in ) {
    int i;

    if( in->src == CWASM_WSCAN_SRC_STR ) {
        wchar_t const** ps = (wchar_t const**)in->ctx;

        for( i = 0; i < in->pb_count; ++i ) { --( *ps ); }
    } else if( in->pb_count > 0 ) {
        FILE* fp = (FILE*)in->ctx;
        int restored = 0;

        if( in->pb_count > (int)__CWASM_STDIO_UNGET ) {
            fpos_t pos;
            size_t nbytes = 0;

            for( i = 0; i < in->pb_count; ++i ) {
                nbytes += cwasm_wscan_wchar_nbytes( (wchar_t)in->pb[ i ] );
            }
            if( fgetpos( fp, &pos ) == 0 && pos.__off >= (off_t)nbytes ) {
                pos.__off -= (off_t)nbytes;
                if( fsetpos( fp, &pos ) == 0 ) { restored = 1; }
            }
        }
        if( !restored ) {
            for( i = 0; i < in->pb_count; ++i ) {
                if( ungetwc( in->pb[ i ], fp ) == WEOF ) {
                    in->restore_error = 1;
                    break;
                }
            }
        }
    }
    if( in->pb_heap ) { free( in->pb ); }
    in->pb = NULL;
    in->pb_count = 0;
    in->pb_cap = 0;
}

static int cwasm_wscan_grow_pb( cwasm_wscan_in* in ) {
    int new_cap = in->pb_cap * 2;
    wint_t* np;

    if( new_cap <= in->pb_cap ) { return 0; }
    if( in->pb_heap ) {
        np = (wint_t*)realloc( in->pb, (size_t)new_cap * sizeof( wint_t ) );
    } else {
        np = (wint_t*)malloc( (size_t)new_cap * sizeof( wint_t ) );
        if( np ) { memcpy( np, in->pb, (size_t)in->pb_count * sizeof( wint_t ) ); }
    }
    if( !np ) {
        in->oom = 1;
        errno = ENOMEM;
        return 0;
    }
    in->pb = np;
    in->pb_cap = new_cap;
    in->pb_heap = 1;
    return 1;
}

static wint_t cwasm_wscan_get( cwasm_wscan_in* in ) {
    wint_t wc;

    if( in->pb_count > 0 ) { wc = in->pb[ --in->pb_count ]; }
    else if( in->src == CWASM_WSCAN_SRC_STR ) {
        wchar_t const** ps = (wchar_t const**)in->ctx;
        if( !** ps ) { wc = WEOF; }
        else {
            wc = (wint_t)** ps;
            ++( *ps );
        }
    } else { wc = fgetwc( (FILE*)in->ctx ); }
    if( wc != WEOF ) { ++in->count; }
    return wc;
}

static wint_t cwasm_wscan_unget( cwasm_wscan_in* in, wint_t wc ) {
    if( wc == WEOF ) { return WEOF; }
    if( in->pb_count >= in->pb_cap && !cwasm_wscan_grow_pb( in ) ) { return WEOF; }
    in->pb[ in->pb_count++ ] = wc;
    if( in->count ) { --in->count; }
    return wc;
}

static wint_t cwasm_wscan_get_w( cwasm_wscan_in* in, int* rem ) {
    wint_t wc;

    if( *rem == 0 ) { return WEOF; }
    wc = cwasm_wscan_get( in );
    if( wc != WEOF ) { --( *rem ); }
    return wc;
}

static void cwasm_wscan_unget_w( cwasm_wscan_in* in, wint_t wc, int* rem ) {
    if( cwasm_wscan_unget( in, wc ) != WEOF ) { ++( *rem ); }
}

static void cwasm_wscan_skip_ws( cwasm_wscan_in* in ) {
    wint_t wc;

    do {
        wc = cwasm_wscan_get( in );
    } while( wc != WEOF && iswspace( wc ) );
    if( wc != WEOF ) { (void)cwasm_wscan_unget( in, wc ); }
}

static int cwasm_wscan_return( cwasm_wscan_in* in, int assigned, int status ) {
    int rc;

    cwasm_wscan_finish( in );
    if( in->oom || in->restore_error ) { status = CWASM_WSCAN_INPUT_FAIL; }
    if( status == CWASM_WSCAN_INPUT_FAIL && assigned == 0 ) { rc = EOF; }
    else { rc = assigned; }
    return rc;
}

static int cwasm_wscan_digit_val( wint_t wc ) {
    if( wc >= L'0' && wc <= L'9' ) { return (int)( wc - L'0' ); }
    if( wc >= L'a' && wc <= L'f' ) { return (int)( wc - L'a' ) + 10; }
    if( wc >= L'A' && wc <= L'F' ) { return (int)( wc - L'A' ) + 10; }
    return -1;
}

static void cwasm_wscan_accum( uintmax_t* acc, int* overflow, int base, int d ) {
    uintmax_t lim = ~(uintmax_t)0;

    if( *overflow ) { return; }
    if( *acc > ( lim - (uintmax_t)d ) / (uintmax_t)base ) {
        *acc = lim;
        *overflow = 1;
    } else { *acc = ( *acc * (uintmax_t)base ) + (uintmax_t)d; }
}

typedef struct {
    int wide;
    union {
        unsigned long narrow;
        uintmax_t wide_val;
    } u;
} cwasm_wscan_mag;

static int cwasm_wscan_uint( cwasm_wscan_in* in, int requested_base, int width,
    enum cwasm_wscan_len len, cwasm_wscan_mag* out_mag, int* out_neg ) {
    int rem = ( width > 0 ) ? width : INT_MAX;
    wint_t c;
    wint_t c2;
    wint_t c3;
    int neg = 0;
    int base = requested_base;
    int digits = 0;
    int overflow = 0;
    int wide = ( len == CWASM_WSCAN_LEN_LL || len == CWASM_WSCAN_LEN_J );
    uintmax_t acc_wide = 0;
    unsigned long acc_narrow = 0;

    #define CWASM_WSCAN_ACCUM(d_) do { \
    if (wide) cwasm_wscan_accum(&acc_wide, &overflow, base, (d_)); \
    else { \
        uintmax_t t = (uintmax_t)acc_narrow; \
        cwasm_wscan_accum(&t, &overflow, base, (d_)); \
        if (!overflow && t > (uintmax_t)ULONG_MAX) \
            overflow = 1; \
        acc_narrow = overflow ? ULONG_MAX : (unsigned long)t; \
    } \
} while (0)

    c = cwasm_wscan_get_w( in, &rem );
    if( c == WEOF ) { return CWASM_WSCAN_INPUT_FAIL; }
    if( c == L'+' || c == L'-' ) {
        neg = ( c == L'-' );
        if( rem == 0 ) { return CWASM_WSCAN_MATCH_FAIL; }
    } else { cwasm_wscan_unget_w( in, c, &rem ); }

    c = cwasm_wscan_get_w( in, &rem );
    if( c == WEOF ) { return CWASM_WSCAN_MATCH_FAIL; }

    if( base == 0 ) {
        if( c == L'0' ) {
            base = 8;
            CWASM_WSCAN_ACCUM( 0 );
            digits = 1;
            if( rem > 0 ) {
                c2 = cwasm_wscan_get_w( in, &rem );
                if( c2 == L'x' || c2 == L'X' ) {
                    base = 16;
                    if( rem > 0 ) {
                        c3 = cwasm_wscan_get_w( in, &rem );
                        if( c3 != WEOF && cwasm_wscan_digit_val( c3 ) >= 0 &&
                            cwasm_wscan_digit_val( c3 ) < 16 ) {
                            acc_wide = 0;
                            acc_narrow = 0;
                            overflow = 0;
                            CWASM_WSCAN_ACCUM( cwasm_wscan_digit_val( c3 ) );
                        } else if( c3 != WEOF ) { cwasm_wscan_unget_w( in, c3, &rem ); }
                    }
                } else if( c2 != WEOF ) { cwasm_wscan_unget_w( in, c2, &rem ); }
            }
        } else {
            int d = cwasm_wscan_digit_val( c );
            base = 10;
            if( d < 0 || d >= 10 ) {
                cwasm_wscan_unget_w( in, c, &rem );
                return CWASM_WSCAN_MATCH_FAIL;
            }
            CWASM_WSCAN_ACCUM( d );
            digits = 1;
        }
    } else if( base == 16 ) {
        if( c == L'0' && rem > 0 ) {
            c2 = cwasm_wscan_get_w( in, &rem );
            if( c2 == L'x' || c2 == L'X' ) {
                if( rem > 0 ) {
                    c3 = cwasm_wscan_get_w( in, &rem );
                    if( c3 != WEOF && cwasm_wscan_digit_val( c3 ) >= 0 &&
                        cwasm_wscan_digit_val( c3 ) < 16 ) {
                        CWASM_WSCAN_ACCUM( cwasm_wscan_digit_val( c3 ) );
                        digits = 1;
                    } else {
                        if( c3 != WEOF ) { cwasm_wscan_unget_w( in, c3, &rem ); }
                        CWASM_WSCAN_ACCUM( 0 );
                        digits = 1;
                    }
                } else {
                    CWASM_WSCAN_ACCUM( 0 );
                    digits = 1;
                }
            } else {
                if( c2 != WEOF ) { cwasm_wscan_unget_w( in, c2, &rem ); }
                CWASM_WSCAN_ACCUM( 0 );
                digits = 1;
            }
        } else {
            int d = cwasm_wscan_digit_val( c );
            if( d < 0 || d >= 16 ) {
                cwasm_wscan_unget_w( in, c, &rem );
                return CWASM_WSCAN_MATCH_FAIL;
            }
            CWASM_WSCAN_ACCUM( d );
            digits = 1;
        }
    } else {
        int d = cwasm_wscan_digit_val( c );
        if( d < 0 || d >= base ) {
            cwasm_wscan_unget_w( in, c, &rem );
            return CWASM_WSCAN_MATCH_FAIL;
        }
        CWASM_WSCAN_ACCUM( d );
        digits = 1;
    }

    while( rem > 0 ) {
        int d;
        c = cwasm_wscan_get_w( in, &rem );
        if( c == WEOF ) { break; }
        d = cwasm_wscan_digit_val( c );
        if( d < 0 || d >= base ) {
            cwasm_wscan_unget_w( in, c, &rem );
            break;
        }
        CWASM_WSCAN_ACCUM( d );
        ++digits;
    }

    if( digits == 0 ) { return CWASM_WSCAN_MATCH_FAIL; }
    out_mag->wide = wide;
    if( wide ) { out_mag->u.wide_val = acc_wide; }
    else { out_mag->u.narrow = acc_narrow; }
    *out_neg = neg;
#undef CWASM_WSCAN_ACCUM
    return CWASM_WSCAN_OK;
}

static void cwasm_wscan_store_signed( va_list* ap, enum cwasm_wscan_len len, intmax_t v ) {
    switch( len ) {
    case CWASM_WSCAN_LEN_HH:
        *va_arg( *ap, signed char* ) = (signed char)v;
        break;
    case CWASM_WSCAN_LEN_H:
        *va_arg( *ap, short* ) = (short)v;
        break;
    case CWASM_WSCAN_LEN_L:
        *va_arg( *ap, long* ) = (long)v;
        break;
    case CWASM_WSCAN_LEN_LL:
        *va_arg( *ap, long long* ) = (long long)v;
        break;
    case CWASM_WSCAN_LEN_J:
        *va_arg( *ap, intmax_t* ) = v;
        break;
    case CWASM_WSCAN_LEN_Z:
    case CWASM_WSCAN_LEN_T:
        *va_arg( *ap, ptrdiff_t* ) = (ptrdiff_t)v;
        break;
    default:
        *va_arg( *ap, int* ) = (int)v;
        break;
    }
}

static unsigned long cwasm_wscan_signed_posmax( enum cwasm_wscan_len len ) {
    switch( len ) {
    case CWASM_WSCAN_LEN_HH: return (unsigned long)SCHAR_MAX;
    case CWASM_WSCAN_LEN_H: return (unsigned long)SHRT_MAX;
    case CWASM_WSCAN_LEN_L: return (unsigned long)LONG_MAX;
    case CWASM_WSCAN_LEN_Z:
    case CWASM_WSCAN_LEN_T: return (unsigned long)PTRDIFF_MAX;
    default: return (unsigned long)INT_MAX;
    }
}

static long cwasm_wscan_signed_negmin( enum cwasm_wscan_len len ) {
    switch( len ) {
    case CWASM_WSCAN_LEN_HH: return SCHAR_MIN;
    case CWASM_WSCAN_LEN_H: return SHRT_MIN;
    case CWASM_WSCAN_LEN_L: return LONG_MIN;
    case CWASM_WSCAN_LEN_Z:
    case CWASM_WSCAN_LEN_T: return (long)PTRDIFF_MIN;
    default: return INT_MIN;
    }
}

static void cwasm_wscan_store_unsigned( va_list* ap, enum cwasm_wscan_len len, uintmax_t v ) {
    switch( len ) {
    case CWASM_WSCAN_LEN_HH:
        *va_arg( *ap, unsigned char* ) = (unsigned char)v;
        break;
    case CWASM_WSCAN_LEN_H:
        *va_arg( *ap, unsigned short* ) = (unsigned short)v;
        break;
    case CWASM_WSCAN_LEN_L:
        *va_arg( *ap, unsigned long* ) = (unsigned long)v;
        break;
    case CWASM_WSCAN_LEN_LL:
        *va_arg( *ap, unsigned long long* ) = (unsigned long long)v;
        break;
    case CWASM_WSCAN_LEN_J:
        *va_arg( *ap, uintmax_t* ) = v;
        break;
    case CWASM_WSCAN_LEN_Z:
    case CWASM_WSCAN_LEN_T:
        *va_arg( *ap, size_t* ) = (size_t)v;
        break;
    default:
        *va_arg( *ap, unsigned int* ) = (unsigned int)v;
        break;
    }
}

static int cwasm_wscan_append_wc( wchar_t** buf, size_t* len, size_t* cap,
    wchar_t* small, wchar_t wc ) {
    if( *len + 1 >= *cap ) {
        size_t nc = ( *cap < 64 ) ? 128 : ( *cap * 2 );
        wchar_t* nb;

        if( nc <= *cap ) { return 0; }
        if( *buf == small ) {
            nb = (wchar_t*)malloc( nc * sizeof( wchar_t ) );
            if( nb ) { memcpy( nb, *buf, *len * sizeof( wchar_t ) ); }
        } else { nb = (wchar_t*)realloc( *buf, nc * sizeof( wchar_t ) ); }
        if( !nb ) {
            errno = ENOMEM;
            return 0;
        }
        *buf = nb;
        *cap = nc;
    }
    ( *buf )[( *len )++ ] = wc;
    ( *buf )[ *len ] = 0;
    return 1;
}

static int cwasm_wscan_digit10( wint_t wc ) {
    return wc >= L'0' && wc <= L'9';
}

static int cwasm_wscan_is_radix( wint_t wc ) {
    return wc == L'.' || wc == (wint_t)(unsigned char) __cwasm_locale_radix_char();
}

static wint_t cwasm_wscan_lower( wint_t wc ) {
    if( wc >= L'A' && wc <= L'Z' ) { return wc - L'A' + L'a'; }
    return wc;
}

static int cwasm_wscan_float_get_append( cwasm_wscan_in* in, int* rem,
    wchar_t** buf, size_t* len, size_t* cap,
    wchar_t* small ) {
    wint_t wc = cwasm_wscan_get_w( in, rem );
    if( wc == WEOF ) { return (int)WEOF; }
    if( !cwasm_wscan_append_wc( buf, len, cap, small, (wchar_t)wc ) ) {
        return -2;
    }
    return (int)wc;
}

#define cwasm_wscan_float_peek(in, rem) ({ \
    wint_t _wc = cwasm_wscan_get_w((in), (rem)); \
    if (_wc != WEOF) cwasm_wscan_unget_w((in), _wc, (rem)); \
    _wc; \
    })

static int cwasm_wscan_float_token( cwasm_wscan_in* in, int width,
    wchar_t** out_buf, size_t* out_len,
    wchar_t* small, size_t small_cap,
    size_t* out_commit_len ) {
    int rem = ( width > 0 ) ? width : INT_MAX;
    wchar_t* buf = small;
    size_t len = 0;
    size_t cap = small_cap;
    size_t commit_len = 0;
    wint_t wc;
    int saw_digit = 0;

    small[ 0 ] = 0;
    wc = cwasm_wscan_get_w( in, &rem );
    if( wc == WEOF ) { return CWASM_WSCAN_INPUT_FAIL; }

    if( wc == L'+' || wc == L'-' ) {
        if( !cwasm_wscan_append_wc( &buf, &len, &cap, small, (wchar_t)wc ) ) {
            goto oom;
        }
        if( rem == 0 ) { goto fail_consumed; }
        wc = cwasm_wscan_get_w( in, &rem );
        if( wc == WEOF ) { goto fail_consumed; }
    }

    if( cwasm_wscan_lower( wc ) == L'i' ) {
        static wchar_t const inf[] = L"inf";
        static wchar_t const inity[] = L"inity";
        int i;
        if( !cwasm_wscan_append_wc( &buf, &len, &cap, small, (wchar_t)wc ) ) {
            goto oom;
        }
        for( i = 1; i < 3; ++i ) {
            if( rem == 0 ) { goto fail_consumed; }
            wc = cwasm_wscan_get_w( in, &rem );
            if( wc == WEOF || cwasm_wscan_lower( wc ) != inf[ i ] ) {
                if( wc != WEOF ) { cwasm_wscan_unget_w( in, wc, &rem ); }
                goto fail_consumed;
            }
            if( !cwasm_wscan_append_wc( &buf, &len, &cap, small, (wchar_t)wc ) ) {
                goto oom;
            }
        }
        for( i = 0; i < 5; ++i ) {
            wc = cwasm_wscan_float_peek( in, &rem );
            if( wc == WEOF || cwasm_wscan_lower( wc ) != inity[ i ] ) { break; }
            if( cwasm_wscan_float_get_append( in, &rem, &buf, &len, &cap, small ) == -2 ) {
                goto oom;
            }
        }
        goto done;
    }

    if( cwasm_wscan_lower( wc ) == L'n' ) {
        static wchar_t const nan[] = L"nan";
        int i;
        if( !cwasm_wscan_append_wc( &buf, &len, &cap, small, (wchar_t)wc ) ) {
            goto oom;
        }
        for( i = 1; i < 3; ++i ) {
            if( rem == 0 ) { goto fail_consumed; }
            wc = cwasm_wscan_get_w( in, &rem );
            if( wc == WEOF || cwasm_wscan_lower( wc ) != nan[ i ] ) {
                if( wc != WEOF ) { cwasm_wscan_unget_w( in, wc, &rem ); }
                goto fail_consumed;
            }
            if( !cwasm_wscan_append_wc( &buf, &len, &cap, small, (wchar_t)wc ) ) {
                goto oom;
            }
        }
        wc = cwasm_wscan_float_peek( in, &rem );
        if( wc == L'(' ) {
            size_t mark = len;
            int paren_ok = 0;
            if( cwasm_wscan_float_get_append( in, &rem, &buf, &len, &cap, small ) == -2 ) {
                goto oom;
            }
            while( rem > 0 ) {
                wc = cwasm_wscan_get_w( in, &rem );
                if( wc == WEOF ) { break; }
                if( !cwasm_wscan_append_wc( &buf, &len, &cap, small, (wchar_t)wc ) ) {
                    goto oom;
                }
                if( wc == L')' ) {
                    paren_ok = 1;
                    break;
                }
            }
            if( !paren_ok ) {
                while( len > mark ) { (void)cwasm_wscan_unget( in, buf[ --len ] ); }
                buf[ len ] = 0;
            }
        }
        goto done;
    }

    if( wc == L'0' ) {
        wint_t p;
        if( !cwasm_wscan_append_wc( &buf, &len, &cap, small, (wchar_t)wc ) ) {
            goto oom;
        }
        saw_digit = 1;
        p = cwasm_wscan_float_peek( in, &rem );
        if( p == L'x' || p == L'X' ) {
            int hex_digits = 0;
            if( cwasm_wscan_float_get_append( in, &rem, &buf, &len, &cap, small ) == -2 ) {
                goto oom;
            }
            while( rem > 0 ) {
                p = cwasm_wscan_float_peek( in, &rem );
                if( cwasm_wscan_digit_val( p ) < 0 || cwasm_wscan_digit_val( p ) >= 16 ) {
                    break;
                }
                if( cwasm_wscan_float_get_append( in, &rem, &buf, &len, &cap, small ) == -2 ) {
                    goto oom;
                }
                ++hex_digits;
            }
            p = cwasm_wscan_float_peek( in, &rem );
            if( cwasm_wscan_is_radix( p ) ) {
                if( cwasm_wscan_float_get_append( in, &rem, &buf, &len, &cap, small ) == -2 ) {
                    goto oom;
                }
                while( rem > 0 ) {
                    p = cwasm_wscan_float_peek( in, &rem );
                    if( cwasm_wscan_digit_val( p ) < 0 || cwasm_wscan_digit_val( p ) >= 16 ) {
                        break;
                    }
                    if( cwasm_wscan_float_get_append( in, &rem, &buf, &len, &cap, small ) == -2 ) {
                        goto oom;
                    }
                    ++hex_digits;
                }
            }
            if( hex_digits == 0 ) { goto fail_consumed; }
            p = cwasm_wscan_float_peek( in, &rem );
            if( p == L'p' || p == L'P' ) {
                int exp_digits = 0;
                if( cwasm_wscan_float_get_append( in, &rem, &buf, &len, &cap, small ) == -2 ) {
                    goto oom;
                }
                p = cwasm_wscan_float_peek( in, &rem );
                if( p == L'+' || p == L'-' ) {
                    if( cwasm_wscan_float_get_append( in, &rem, &buf, &len, &cap, small ) == -2 ) {
                        goto oom;
                    }
                }
                while( rem > 0 ) {
                    p = cwasm_wscan_float_peek( in, &rem );
                    if( !cwasm_wscan_digit10( p ) ) { break; }
                    if( cwasm_wscan_float_get_append( in, &rem, &buf, &len, &cap, small ) == -2 ) {
                        goto oom;
                    }
                    ++exp_digits;
                }
                if( exp_digits == 0 ) { commit_len = len; }
            }
            goto done;
        }
    } else if( cwasm_wscan_digit10( wc ) ) {
        if( !cwasm_wscan_append_wc( &buf, &len, &cap, small, (wchar_t)wc ) ) {
            goto oom;
        }
        saw_digit = 1;
    } else if( cwasm_wscan_is_radix( wc ) ) {
        if( !cwasm_wscan_append_wc( &buf, &len, &cap, small, (wchar_t)wc ) ) {
            goto oom;
        }
    } else {
        cwasm_wscan_unget_w( in, wc, &rem );
        goto fail_consumed;
    }

    while( rem > 0 ) {
        wc = cwasm_wscan_float_peek( in, &rem );
        if( !cwasm_wscan_digit10( wc ) ) { break; }
        if( cwasm_wscan_float_get_append( in, &rem, &buf, &len, &cap, small ) == -2 ) {
            goto oom;
        }
        saw_digit = 1;
    }
    wc = cwasm_wscan_float_peek( in, &rem );
    if( cwasm_wscan_is_radix( wc ) ) {
        if( cwasm_wscan_float_get_append( in, &rem, &buf, &len, &cap, small ) == -2 ) {
            goto oom;
        }
        while( rem > 0 ) {
            wc = cwasm_wscan_float_peek( in, &rem );
            if( !cwasm_wscan_digit10( wc ) ) { break; }
            if( cwasm_wscan_float_get_append( in, &rem, &buf, &len, &cap, small ) == -2 ) {
                goto oom;
            }
            saw_digit = 1;
        }
    }
    if( !saw_digit ) { goto fail_consumed; }
    wc = cwasm_wscan_float_peek( in, &rem );
    if( wc == L'e' || wc == L'E' ) {
        int exp_digits = 0;
        if( cwasm_wscan_float_get_append( in, &rem, &buf, &len, &cap, small ) == -2 ) {
            goto oom;
        }
        wc = cwasm_wscan_float_peek( in, &rem );
        if( wc == L'+' || wc == L'-' ) {
            if( cwasm_wscan_float_get_append( in, &rem, &buf, &len, &cap, small ) == -2 ) {
                goto oom;
            }
        }
        while( rem > 0 ) {
            wc = cwasm_wscan_float_peek( in, &rem );
            if( !cwasm_wscan_digit10( wc ) ) { break; }
            if( cwasm_wscan_float_get_append( in, &rem, &buf, &len, &cap, small ) == -2 ) {
                goto oom;
            }
            ++exp_digits;
        }
        if( exp_digits == 0 ) { commit_len = len; }
    }

done:
    buf[ len ] = 0;
    *out_buf = buf;
    *out_len = len;
    *out_commit_len = commit_len;
    return CWASM_WSCAN_OK;

fail_consumed:
    if( buf != small ) { free( buf ); }
    return CWASM_WSCAN_MATCH_FAIL;

oom:
    if( buf != small ) { free( buf ); }
    errno = ENOMEM;
    return CWASM_WSCAN_INPUT_FAIL;
    }

static int cwasm_wscan_float( cwasm_wscan_in* in, int width, enum cwasm_wscan_len len,
    int suppress, va_list* ap ) {
    wchar_t small[ 128 ];
    wchar_t* buf = small;
    size_t token_len = 0;
    size_t commit_len = 0;
    wchar_t* endp;
    int st;

    st = cwasm_wscan_float_token( in, width, &buf, &token_len, small,
        sizeof( small ) / sizeof( small[ 0 ] ), &commit_len );
    if( st != CWASM_WSCAN_OK ) { return st; }

    double dv = 0.0;
    long double ldv = 0.0L;
    size_t keep_len;

    if( len == CWASM_WSCAN_LEN_CAP_L ) { ldv = wcstold( buf, &endp ); }
    else { dv = wcstod( buf, &endp ); }

    if( endp == buf ) {
        if( buf != small ) { free( buf ); }
        return CWASM_WSCAN_MATCH_FAIL;
    }
    keep_len = (size_t)( endp - buf );
    if( keep_len < commit_len ) { keep_len = commit_len; }
    while( token_len > keep_len ) {
        (void)cwasm_wscan_unget( in, buf[ --token_len ] );
    }
    if( !suppress ) {
        if( len == CWASM_WSCAN_LEN_NONE ) { *va_arg( *ap, float* ) = (float)dv; }
        else if( len == CWASM_WSCAN_LEN_CAP_L ) {
            *va_arg( *ap, long double* ) = ldv;
        } else { *va_arg( *ap, double* ) = dv; }
    }
    if( buf != small ) { free( buf ); }
    return CWASM_WSCAN_OK;
}

static wchar_t const* cwasm_wscan_build_set( wchar_t const* fmt, cwasm_wscanset* ws, int* ok ) {
    wchar_t prev = 0;
    int have_prev = 0;

    *ok = 0;
    cwasm_wset_init( ws );
    if( *fmt == L'^' ) {
        ws->invert = 1;
        ++fmt;
    }
    if( *fmt == L']' ) {
        if( !cwasm_wset_add_range( ws, L']', L']' ) ) { goto fail; }
        prev = L']';
        have_prev = 1;
        ++fmt;
    }
    while( *fmt && *fmt != L']' ) {
        if( *fmt == L'-' && have_prev && fmt[ 1 ] && fmt[ 1 ] != L']' ) {
            wchar_t end = fmt[ 1 ];
            fmt += 2;
            if( !cwasm_wset_add_range( ws, prev, end ) ) { goto fail; }
            prev = end;
            have_prev = 1;
        } else {
            wchar_t wc = *fmt++;
            if( !cwasm_wset_add_range( ws, wc, wc ) ) { goto fail; }
            prev = wc;
            have_prev = 1;
        }
    }
    if( *fmt != L']' ) { goto fail; }
    ++fmt;
    *ok = 1;
    return fmt;

fail:
    cwasm_wset_free( ws );
    return fmt;
}

static int cwasm_wscan_put_mbs( char* dst, size_t* npos, size_t cap, wchar_t wc, mbstate_t* st ) {
    char tmp[ MB_LEN_MAX ];
    size_t n = wcrtomb( tmp, wc, st );

    if( n == (size_t)-1 ) { return 0; }
    if( *npos + n > cap ) { return 0; }
    memcpy( dst + *npos, tmp, n );
    *npos += n;
    return 1;
}

// mode 0 = scanset, 1 = %s (stop on whitespace).
static int cwasm_wscan_run( cwasm_wscan_in* in, int width, enum cwasm_wscan_len len,
    int suppress, va_list* ap, int mode, cwasm_wscanset const* wset ) {
    int rem = ( width > 0 ) ? width : INT_MAX;
    int n = 0;
    int hit_eof = 0;
    int as_mbs = ( len != CWASM_WSCAN_LEN_L );

    if( mode == 1 ) { cwasm_wscan_skip_ws( in ); }

    if( !as_mbs ) {
        wchar_t* dst = suppress ? NULL : va_arg( *ap, wchar_t* );

        while( rem > 0 ) {
            wint_t wc = cwasm_wscan_get_w( in, &rem );
            if( wc == WEOF ) {
                hit_eof = 1;
                break;
            }
            if( mode == 1 ? iswspace( wc ) : !cwasm_wset_contains( wset, (wchar_t)wc ) ) {
                cwasm_wscan_unget_w( in, wc, &rem );
                break;
            }
            if( !suppress ) { dst[ n ] = (wchar_t)wc; }
            ++n;
        }
        if( n == 0 ) { return hit_eof ? CWASM_WSCAN_INPUT_FAIL : CWASM_WSCAN_MATCH_FAIL; }
        if( !suppress ) { dst[ n ] = 0; }
    } else {
        char stack[ 256 ];
        char* dst = suppress ? NULL : va_arg( *ap, char* );
        char* tmp = suppress ? stack : dst;
        size_t tpos = 0;
        size_t tcap = suppress ? sizeof( stack ) : SIZE_MAX;
        mbstate_t st;

        memset( &st, 0, sizeof( st ) );
        if( !suppress && !dst ) { return CWASM_WSCAN_MATCH_FAIL; }
        if( suppress ) { tmp[ 0 ] = '\0'; }

        while( rem > 0 ) {
            wint_t wc = cwasm_wscan_get_w( in, &rem );
            if( wc == WEOF ) {
                hit_eof = 1;
                break;
            }
            if( mode == 1 ? iswspace( wc ) : !cwasm_wset_contains( wset, (wchar_t)wc ) ) {
                cwasm_wscan_unget_w( in, wc, &rem );
                break;
            }
            if( !suppress ) {
                if( !cwasm_wscan_put_mbs( tmp, &tpos, tcap,
                    (wchar_t)wc, &st ) ) {
                    return CWASM_WSCAN_INPUT_FAIL;
                }
            } else {
                char sink[ MB_LEN_MAX ];
                size_t nn = wcrtomb( sink, (wchar_t)wc, &st );
                if( nn == (size_t)-1 ) { return CWASM_WSCAN_INPUT_FAIL; }
            }
            ++n;
        }
        if( n == 0 ) { return hit_eof ? CWASM_WSCAN_INPUT_FAIL : CWASM_WSCAN_MATCH_FAIL; }
        // %s / %[ require a terminator, leave one byte of headroom
        if( !suppress ) {
            if( tpos >= tcap ) { return CWASM_WSCAN_INPUT_FAIL; }
            tmp[ tpos ] = '\0';
        }
    }
    return CWASM_WSCAN_OK;
}

static int cwasm_wscan_chars( cwasm_wscan_in* in, int width, enum cwasm_wscan_len len,
    int suppress, va_list* ap ) {
    int n = ( width > 0 ) ? width : 1;
    int i;
    int as_mbs = ( len != CWASM_WSCAN_LEN_L );

    if( !as_mbs ) {
        wchar_t* dst = suppress ? NULL : va_arg( *ap, wchar_t* );

        for( i = 0; i < n; ++i ) {
            wint_t wc = cwasm_wscan_get( in );
            if( wc == WEOF ) { return CWASM_WSCAN_INPUT_FAIL; }
            if( !suppress ) { dst[ i ] = (wchar_t)wc; }
        }
    } else {
        char* dst = suppress ? NULL : va_arg( *ap, char* );
        mbstate_t st;
        size_t pos = 0;

        memset( &st, 0, sizeof( st ) );
        for( i = 0; i < n; ++i ) {
            wint_t wc = cwasm_wscan_get( in );
            if( wc == WEOF ) { return CWASM_WSCAN_INPUT_FAIL; }
            if( !suppress ) {
                // %c does not append a terminator (C / POSIX).
                if( !cwasm_wscan_put_mbs( dst, &pos, SIZE_MAX, (wchar_t)wc, &st ) ) {
                    return CWASM_WSCAN_INPUT_FAIL;
                }
            }
        }
    }
    return CWASM_WSCAN_OK;
}

static int cwasm_wscan_try_nil( cwasm_wscan_in* in, int width ) {
    static wchar_t const nil[] = L"(nil)";
    int rem = ( width > 0 ) ? width : INT_MAX;
    wint_t got[ 5 ];
    int i;

    for( i = 0; i < 5; ++i ) {
        wint_t wc;
        wint_t expect;

        if( rem == 0 ) { break; }
        wc = cwasm_wscan_get_w( in, &rem );
        expect = nil[ i ];
        if( wc == WEOF || towlower( wc ) != expect ) {
            if( wc != WEOF ) { cwasm_wscan_unget_w( in, wc, &rem ); }
            while( i > 0 ) { (void)cwasm_wscan_unget( in, got[ --i ] ); }
            return 0;
        }
        got[ i ] = wc;
    }
    if( i == 5 ) { return 1; }
    while( i > 0 ) { (void)cwasm_wscan_unget( in, got[ --i ] ); }
    return 0;
}

static int cwasm_vxwscanf( enum cwasm_wscan_src src, void* ctx,
    wchar_t const* fmt, va_list ap ) {
    cwasm_wscan_in in;
    int assigned = 0;
    va_list args;

    if( !fmt ) {
        errno = EINVAL;
        return EOF;
    }
    va_copy( args, ap );
    cwasm_wscan_init( &in, src, ctx );

    #define CWASM_WSCAN_RETURN(_st) \
        do { \
            int _rc = cwasm_wscan_return(&in, assigned, (_st)); \
            va_end(args); \
            return _rc; \
        } while (0)

    while( *fmt ) {
        int suppress = 0;
        int width = 0;
        enum cwasm_wscan_len len = CWASM_WSCAN_LEN_NONE;
        wchar_t conv;

        if( iswspace( (wint_t)*fmt ) ) {
            while( iswspace( (wint_t)*fmt ) ) { ++fmt; }
            cwasm_wscan_skip_ws( &in );
            continue;
        }

        if( *fmt != L'%' ) {
            wint_t wc = cwasm_wscan_get( &in );
            if( wc == WEOF ) { CWASM_WSCAN_RETURN( CWASM_WSCAN_INPUT_FAIL ); }
            if( wc != (wint_t)*fmt ) {
                (void)cwasm_wscan_unget( &in, wc );
                CWASM_WSCAN_RETURN( CWASM_WSCAN_MATCH_FAIL );
            }
            ++fmt;
            continue;
        }

        ++fmt;
        if( *fmt == L'%' ) {
            wint_t wc = cwasm_wscan_get( &in );
            if( wc == WEOF ) { CWASM_WSCAN_RETURN( CWASM_WSCAN_INPUT_FAIL ); }
            if( wc != L'%' ) {
                (void)cwasm_wscan_unget( &in, wc );
                CWASM_WSCAN_RETURN( CWASM_WSCAN_MATCH_FAIL );
            }
            ++fmt;
            continue;
        }

        if( *fmt == L'*' ) {
            suppress = 1;
            ++fmt;
        }
        while( *fmt >= L'0' && *fmt <= L'9' ) {
            if( width <= ( INT_MAX - 9 ) / 10 ) {
                width = width * 10 + (int)( *fmt - L'0' );
            } else { width = INT_MAX; }
            ++fmt;
        }

        if( *fmt == L'h' ) {
            ++fmt;
            if( *fmt == L'h' ) {
                ++fmt;
                len = CWASM_WSCAN_LEN_HH;
            } else { len = CWASM_WSCAN_LEN_H; }
        } else if( *fmt == L'l' ) {
            ++fmt;
            if( *fmt == L'l' ) {
                ++fmt;
                len = CWASM_WSCAN_LEN_LL;
            } else { len = CWASM_WSCAN_LEN_L; }
        } else if( *fmt == L'j' ) {
            ++fmt;
            len = CWASM_WSCAN_LEN_J;
        } else if( *fmt == L'z' ) {
            ++fmt;
            len = CWASM_WSCAN_LEN_Z;
        } else if( *fmt == L't' ) {
            ++fmt;
            len = CWASM_WSCAN_LEN_T;
        } else if( *fmt == L'L' ) {
            ++fmt;
            len = CWASM_WSCAN_LEN_CAP_L;
        }

        conv = *fmt++;
        if( conv == 0 ) { CWASM_WSCAN_RETURN( CWASM_WSCAN_MATCH_FAIL ); }

        if( conv == L'c' ) {
            int st = cwasm_wscan_chars( &in, width, len, suppress, &args );
            if( st != CWASM_WSCAN_OK ) { CWASM_WSCAN_RETURN( st ); }
            if( !suppress ) { ++assigned; }
            continue;
        }

        if( conv == L's' ) {
            int st = cwasm_wscan_run( &in, width, len, suppress, &args, 1, NULL );
            if( st != CWASM_WSCAN_OK ) { CWASM_WSCAN_RETURN( st ); }
            if( !suppress ) { ++assigned; }
            continue;
        }

        if( conv == L'[' ) {
            int ok = 0;
            int st;
            cwasm_wscanset wset;

            fmt = cwasm_wscan_build_set( fmt, &wset, &ok );
            if( !ok ) { CWASM_WSCAN_RETURN( CWASM_WSCAN_MATCH_FAIL ); }
            st = cwasm_wscan_run( &in, width, len, suppress, &args, 0, &wset );
            cwasm_wset_free( &wset );
            if( st != CWASM_WSCAN_OK ) { CWASM_WSCAN_RETURN( st ); }
            if( !suppress ) { ++assigned; }
            continue;
        }

        if( conv == L'n' ) {
            if( !suppress ) {
                cwasm_wscan_store_signed( &args, len, (intmax_t)in.count );
            }
            continue;
        }

        if( conv == L'p' ) {
            cwasm_wscan_mag mag;
            int neg = 0;
            int st;

            if( len != CWASM_WSCAN_LEN_NONE ) {
                CWASM_WSCAN_RETURN( CWASM_WSCAN_MATCH_FAIL );
            }
            cwasm_wscan_skip_ws( &in );
            if( cwasm_wscan_try_nil( &in, width ) ) {
                if( !suppress ) {
                    *va_arg( args, void** ) = NULL;
                    ++assigned;
                }
                continue;
            }
            st = cwasm_wscan_uint( &in, 16, width, CWASM_WSCAN_LEN_NONE, &mag, &neg );
            if( st != CWASM_WSCAN_OK ) { CWASM_WSCAN_RETURN( st ); }
            if( !suppress ) {
                (void)neg;
                *va_arg( args, void** ) = (void*)(uintptr_t)mag.u.narrow;
                ++assigned;
            }
            continue;
        }

        if( conv == L'd' || conv == L'i' || conv == L'o' || conv == L'u' ||
            conv == L'x' || conv == L'X' ) {
            cwasm_wscan_mag mag;
            int neg = 0;
            int base;
            int st;
            int signed_conv = ( conv == L'd' || conv == L'i' );

            if( len == CWASM_WSCAN_LEN_CAP_L ) {
                CWASM_WSCAN_RETURN( CWASM_WSCAN_MATCH_FAIL );
            }
            if( conv == L'i' ) { base = 0; }
            else if( conv == L'o' ) { base = 8; }
            else if( conv == L'x' || conv == L'X' ) { base = 16; }
            else { base = 10; }
            cwasm_wscan_skip_ws( &in );
            st = cwasm_wscan_uint( &in, base, width, len, &mag, &neg );
            if( st != CWASM_WSCAN_OK ) { CWASM_WSCAN_RETURN( st ); }
            if( !suppress ) {
                if( signed_conv ) {
                    intmax_t v;
                    if( mag.wide ) {
                        uintmax_t m = mag.u.wide_val;
                        uintmax_t posmax = (uintmax_t)INTMAX_MAX;
                        if( !neg ) { v = ( m > posmax ) ? INTMAX_MAX : (intmax_t)m; }
                        else if( m >= posmax + (uintmax_t)1 ) { v = INTMAX_MIN; }
                        else { v = -(intmax_t)m; }
                    } else {
                        unsigned long posmax = cwasm_wscan_signed_posmax( len );
                        unsigned long m = mag.u.narrow;
                        if( !neg ) {
                            v = ( m > posmax ) ? (intmax_t)posmax : (intmax_t)m;
                        } else if( m > posmax + 1ul ) {
                            v = cwasm_wscan_signed_negmin( len );
                        } else { v = -(intmax_t)m; }
                    }
                    cwasm_wscan_store_signed( &args, len, v );
                } else if( mag.wide ) {
                    uintmax_t m = mag.u.wide_val;
                    cwasm_wscan_store_unsigned( &args, len, neg ? ( 0u - m ) : m );
                } else {
                    unsigned long m = mag.u.narrow;
                    cwasm_wscan_store_unsigned( &args, len, neg ? ( 0ul - m ) : m );
                }
                ++assigned;
            }
            continue;
        }

        if( conv == L'f' || conv == L'F' || conv == L'e' || conv == L'E' ||
            conv == L'g' || conv == L'G' || conv == L'a' || conv == L'A' ) {
            int st;

            if( len != CWASM_WSCAN_LEN_NONE && len != CWASM_WSCAN_LEN_L &&
                len != CWASM_WSCAN_LEN_CAP_L ) {
                CWASM_WSCAN_RETURN( CWASM_WSCAN_MATCH_FAIL );
            }
            cwasm_wscan_skip_ws( &in );
            st = cwasm_wscan_float( &in, width, len, suppress, &args );
            if( st != CWASM_WSCAN_OK ) { CWASM_WSCAN_RETURN( st ); }
            if( !suppress ) { ++assigned; }
            continue;
        }

        CWASM_WSCAN_RETURN( CWASM_WSCAN_MATCH_FAIL );
}

    CWASM_WSCAN_RETURN( CWASM_WSCAN_OK );
    #undef CWASM_WSCAN_RETURN
}

int vfwscanf( FILE* restrict stream, wchar_t const* restrict format, va_list ap ) {
    int r;

    if( !stream || !format ) {
        errno = EINVAL;
        return EOF;
    }
    flockfile( stream );
    if( __cwasm_stdio_orient_wide( stream ) != 0 ) {
        funlockfile( stream );
        return EOF;
    }
    r = cwasm_vxwscanf( CWASM_WSCAN_SRC_FILE, stream, format, ap );
    funlockfile( stream );
    return r;
}

int vswscanf( wchar_t const* restrict s, wchar_t const* restrict format, va_list ap ) {
    wchar_t const* p;

    if( !s || !format ) {
        errno = EINVAL;
        return EOF;
    }
    p = s;
    return cwasm_vxwscanf( CWASM_WSCAN_SRC_STR, &p, format, ap );
}

int vwscanf( wchar_t const* restrict format, va_list ap ) {
    return vfwscanf( stdin, format, ap );
}

int fwscanf( FILE* restrict stream, wchar_t const* restrict format, ... ) {
    va_list ap;
    int r;

    va_start( ap, format );
    r = vfwscanf( stream, format, ap );
    va_end( ap );
    return r;
}

int swscanf( wchar_t const* restrict s, wchar_t const* restrict format, ... ) {
    va_list ap;
    int r;

    va_start( ap, format );
    r = vswscanf( s, format, ap );
    va_end( ap );
    return r;
}

int wscanf( wchar_t const* restrict format, ... ) {
    va_list ap;
    int r;

    va_start( ap, format );
    r = vwscanf( format, ap );
    va_end( ap );
    return r;
}
