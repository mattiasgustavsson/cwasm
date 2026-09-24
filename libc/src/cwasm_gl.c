// Must NOT include cwasm_gl.h - apps choose GLES2 vs GLES3 via that header

#include <cwasm.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef unsigned char GLubyte;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef long GLintptr;
typedef long GLsizeiptr;

#define GL_EXTENSIONS 0x1F03

#ifdef CWASM_THREADS
    #define CWASM_GL_TLS _Thread_local
#else
    #define CWASM_GL_TLS
#endif

CWASM_GL_TLS int CWASM_GL_EXT_texture_compression_s3tc;

typedef struct gl_string_cache_entry_t {
    GLenum name;
    GLubyte const* ptr;
} gl_string_cache_entry_t;

typedef struct gl_stringi_cache_entry_t {
    GLenum name;
    GLuint index;
    GLubyte const* ptr;
} gl_stringi_cache_entry_t;

static CWASM_GL_TLS gl_string_cache_entry_t* gl_string_cache;
static CWASM_GL_TLS int gl_string_cache_len;
static CWASM_GL_TLS int gl_string_cache_cap;
static CWASM_GL_TLS gl_stringi_cache_entry_t* gl_stringi_cache;
static CWASM_GL_TLS int gl_stringi_cache_len;
static CWASM_GL_TLS int gl_stringi_cache_cap;

static void* gl_cache_reserve( void* buf, int len, int* cap, size_t elem ) {
    if( len < *cap ) { return buf; }
    int ncap = *cap ? *cap * 2 : 8;
    void* nbuf = realloc( buf, (size_t)ncap * elem );
    if( !nbuf ) { return NULL; }
    *cap = ncap;
    return nbuf;
}

static CWASM_GL_TLS int gl_canvas_css_size[ 3 ];

CWASM_JS( void, js_gl_bind_canvas_size, ( int* cell ), {
    CWASM.__glCanvasSize = cell | 0;
})

CWASM_JS_MAIN( void, js_gl_observe_canvas_size, ( char const* css_id, int* cell ), {
    var el = document.getElementById(mem.str_read(css_id));
    if (!el) return;
    var p = cell | 0;
    var sync = function () {
        var w = el.clientWidth | 0, h = el.clientHeight | 0;
        if (w < 1) w = 1;
        if (h < 1) h = 1;
        mem.u32_write(p, w);
        mem.u32_write(p + 4, h);
        mem.u32_write(p + 8, 1);
    };
    // Key observers by TLS cell so a later setup for a different css_id on this thread
    // disconnects the previous RO (other threads keep theirs)
    var map = CWASM.__glCanvasObservers || (CWASM.__glCanvasObservers = Object.create(null));
    var prev = map[p];
    if (prev && prev.el === el) { sync(); return; }
    if (prev) prev.ro.disconnect();
    sync();
    if (typeof ResizeObserver !== "function") return;
    var ro = new ResizeObserver(sync);
    ro.observe(el);
    map[p] = { ro: ro, el: el };
})

CWASM_JS( void, js_gl_get_canvas_size, ( int* w_out, int* h_out ), {
    var s = CWASM.__glCanvasSize | 0, c = CWASM.canvas;
    if (s && mem.u32_read(s + 8)) {
        var w = mem.u32_read(s), h = mem.u32_read(s + 4);
        if (c) {
            if ((c.width | 0) !== w) c.width = w;
            if ((c.height | 0) !== h) c.height = h;
        }
        mem.u32_write(s + 8, 0);
    }
    mem.u32_write(w_out, c ? (c.width | 0) || 1 : 1);
    mem.u32_write(h_out, c ? (c.height | 0) || 1 : 1);
})

void cwasm_gl_watch_canvas_size( char const* css_id ) {
    gl_canvas_css_size[ 0 ] = 1;
    gl_canvas_css_size[ 1 ] = 1;
    gl_canvas_css_size[ 2 ] = 1;
    js_gl_bind_canvas_size( gl_canvas_css_size );
    js_gl_observe_canvas_size( css_id, gl_canvas_css_size );
}

void glCanvasSize( int* w, int* h ) {
    js_gl_get_canvas_size( w, h );
}

CWASM_JS_LIB( GL, uint32_t, js_gl_get_string, ( GLenum name, char* buf, uint32_t len ),
{
    var ret = "";
    switch (name)
    {
        case 0x1F00: case 0x1F01: case 0x9245: case 0x9246:
            ret = GLctx.getParameter(name) || "";
            break;
        case 0x1F02: {
            var ver = GLctx.getParameter(0x1F02) || "";
            var es = (typeof WebGL2RenderingContext !== "undefined" &&
                      GLctx instanceof WebGL2RenderingContext) ? "3.0" : "2.0";
            ret = "OpenGL ES " + es + " (" + ver + ")";
            break;
        }
        case 0x8B8C:
            ret = GLctx.getParameter(0x8B8C) || "";
            break;
        case 0x1F03: {
            var exts = GLctx.getSupportedExtensions() || [];
            ret = exts.concat(exts.map(function(e) { return "GL_" + e; })).join(" ");
            break;
        }
        default:
            GLsetError(0x500);
            return 0;
    }
    return mem.str_write(ret, buf, len);
})

GLubyte const* glGetString( GLenum name );

static int cwasm_gl_has_ext( char const* exts, char const* name ) {
    size_t n = strlen( name );
    char const* loc;

    for( loc = exts; loc && ( loc = strstr( loc, name ) ) != NULL; ++loc ) {
        if( ( loc == exts || loc[ -1 ] == ' ' ) &&
            ( loc[ n ] == ' ' || loc[ n ] == '\0' ) ) {
            return 1;
        }
    }
    return 0;
}

void cwasm_gl_find_extensions( void ) {
    char const* ext = (char const*)glGetString( GL_EXTENSIONS );

    CWASM_GL_EXT_texture_compression_s3tc = 0;
    if( !ext ) { return; }

    if( cwasm_gl_has_ext( ext, "WEBGL_compressed_texture_s3tc" ) ||
        cwasm_gl_has_ext( ext, "GL_EXT_texture_compression_s3tc" ) ) {
        CWASM_GL_EXT_texture_compression_s3tc = 1;
    }
}

GLubyte const* glGetString( GLenum name ) {
    for( int i = 0; i < gl_string_cache_len; ++i ) {
        if( gl_string_cache[ i ].name == name ) { return gl_string_cache[ i ].ptr; }
    }

    void* slots = gl_cache_reserve( gl_string_cache, gl_string_cache_len,
        &gl_string_cache_cap, sizeof( *gl_string_cache ) );
    if( !slots ) { return NULL; }
    gl_string_cache = (gl_string_cache_entry_t*)slots;

    uint32_t need = js_gl_get_string( name, 0, 0 );
    if( !need ) { return NULL; }

    GLubyte* ptr = (GLubyte*)malloc( need );
    if( !ptr ) { return NULL; }
    js_gl_get_string( name, (char*)ptr, need );

    gl_string_cache[ gl_string_cache_len ].name = name;
    gl_string_cache[ gl_string_cache_len ].ptr = ptr;
    ++gl_string_cache_len;
    return ptr;
}

CWASM_JS_LIB( GL, uint32_t, js_gl_get_stringi, ( GLenum name, GLuint index, char* buf, uint32_t len ),
{
    var ret = "";
    if (name === 0x1F03) {
        var exts = GLctx.getSupportedExtensions() || [];
        var list = exts.concat(exts.map(function(e) { return "GL_" + e; }));
        if ((index >>> 0) >= list.length) {
            GLsetError(0x501);
            return 0;
        }
        ret = list[index >>> 0] || "";
    } else {
        GLsetError(0x500);
        return 0;
    }
    return mem.str_write(ret, buf, len);
})

GLubyte const* glGetStringi( GLenum name, GLuint index ) {
    for( int i = 0; i < gl_stringi_cache_len; ++i ) {
        if( gl_stringi_cache[ i ].name == name && gl_stringi_cache[ i ].index == index ) {
            return gl_stringi_cache[ i ].ptr;
        }
    }

    void* slots = gl_cache_reserve( gl_stringi_cache, gl_stringi_cache_len,
        &gl_stringi_cache_cap, sizeof( *gl_stringi_cache ) );
    if( !slots ) { return NULL; }
    gl_stringi_cache = (gl_stringi_cache_entry_t*)slots;

    uint32_t need = js_gl_get_stringi( name, index, 0, 0 );
    if( !need ) { return NULL; }

    GLubyte* ptr = (GLubyte*)malloc( need );
    if( !ptr ) { return NULL; }
    js_gl_get_stringi( name, index, (char*)ptr, need );

    gl_stringi_cache[ gl_stringi_cache_len ].name = name;
    gl_stringi_cache[ gl_stringi_cache_len ].index = index;
    gl_stringi_cache[ gl_stringi_cache_len ].ptr = ptr;
    ++gl_stringi_cache_len;
    return ptr;
}

CWASM_JS_LIB( GL, int, js_gl_map_buffer_range, ( GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access, void* ptr ),
{
    if (length < 0) { GLsetError(0x501); return 0; }
    if (GLmappedBuffers[target]) { GLsetError(0x502); return 0; }
    if (access & 0x0001) {
        try { GLctx.getBufferSubData(target, offset, mem.raw_view(ptr, length)); }
        catch (e) { GLsetError(0x502); return 0; }
    }
    GLmappedBuffers[target] = { ptr: ptr, size: length, offset: offset, access: access };
    return 1;
})

CWASM_JS_LIB( GL, void*, js_gl_unmap_buffer, ( GLenum target ),
{
    var m = GLmappedBuffers[target];
    if (!m) { GLsetError(0x502); return 0; }
    delete GLmappedBuffers[target];
    if ((m.access & 0x0002) && !(m.access & 0x0010)) {
        try { GLctx.bufferSubData(target, m.offset, mem.raw_view(m.ptr, m.size)); }
        catch (e) { GLsetError(0x502); }
    }
    return m.ptr;
})

void* glMapBufferRange( GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access ) {
    void* ptr = malloc( length > 0 ? (size_t)length : 1u );
    if( !ptr ) { return NULL; }
    if( !js_gl_map_buffer_range( target, offset, length, access, ptr ) ) {
        free( ptr );
        return NULL;
    }
    return ptr;
}

GLboolean glUnmapBuffer( GLenum target ) {
    void* ptr = js_gl_unmap_buffer( target );
    if( !ptr ) { return 0; }
    free( ptr );
    return 1;
}
