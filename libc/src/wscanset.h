#ifndef __CWASM_WSCANSET_H__
#define __CWASM_WSCANSET_H__

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

typedef struct cwasm_wrange {
    wchar_t lo;
    wchar_t hi;
} cwasm_wrange;

typedef struct cwasm_wscanset {
    int invert;
    cwasm_wrange small[ 16 ];
    cwasm_wrange* r;
    int n;
    int cap;
    int heap;
} cwasm_wscanset;

static void cwasm_wset_init( cwasm_wscanset* ws ) {
    ws->invert = 0;
    ws->r = ws->small;
    ws->n = 0;
    ws->cap = 16;
    ws->heap = 0;
}

static void cwasm_wset_free( cwasm_wscanset* ws ) {
    if( ws->heap ) { free( ws->r ); }
    ws->r = NULL;
    ws->n = ws->cap = ws->heap = 0;
}

static int cwasm_wset_add_range( cwasm_wscanset* ws, wchar_t a, wchar_t b ) {
    cwasm_wrange* nr;

    if( a > b ) {
        wchar_t t = a;
        a = b;
        b = t;
    }
    if( ws->n >= ws->cap ) {
        int nc = ws->cap * 2;

        if( nc <= ws->cap ) { return 0; }
        if( ws->heap ) {
            nr = (cwasm_wrange*)realloc( ws->r, (size_t)nc * sizeof( cwasm_wrange ) );
        } else {
            nr = (cwasm_wrange*)malloc( (size_t)nc * sizeof( cwasm_wrange ) );
            if( nr ) { memcpy( nr, ws->r, (size_t)ws->n * sizeof( cwasm_wrange ) ); }
        }
        if( !nr ) {
            errno = ENOMEM;
            return 0;
        }
        ws->r = nr;
        ws->cap = nc;
        ws->heap = 1;
    }
    ws->r[ ws->n ].lo = a;
    ws->r[ ws->n ].hi = b;
    ++ws->n;
    return 1;
}

static int cwasm_wset_contains( cwasm_wscanset const* ws, wchar_t wc ) {
    int i;
    int hit = 0;

    for( i = 0; i < ws->n; ++i ) {
        if( wc >= ws->r[ i ].lo && wc <= ws->r[ i ].hi ) {
            hit = 1;
            break;
        }
    }
    return ws->invert ? !hit : hit;
}

#endif // __CWASM_WSCANSET_H__
