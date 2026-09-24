#ifndef __CWASM_GL_H__
#define __CWASM_GL_H__
#ifdef CWASM_GLES2
    #include <GLES2/gl2.h>
#else
    #include <GLES3/gl3.h>
#endif

#include <cwasm.h>

#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
    #define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
    #define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
    #define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
    #define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif
#define GL_EXT_texture_compression_s3tc 1

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CWASM_THREADS
    extern _Thread_local int CWASM_GL_EXT_texture_compression_s3tc;
#else
    extern int CWASM_GL_EXT_texture_compression_s3tc;
#endif
void cwasm_gl_find_extensions( void );
void cwasm_gl_watch_canvas_size( char const* css_id );
void glCanvasSize( int* w, int* h );

#ifdef __cplusplus
}
#endif

typedef struct gl_canvas_t {
    char id[ 64 ];
    int width;
    int height;
    int in_use;
} gl_canvas_t;

#define GL_CANVAS_ANTIALIAS_ON 1
#define GL_CANVAS_ANTIALIAS_OFF 0
#define GL_CANVAS_DEPTH_TRUE 1
#define GL_CANVAS_DEPTH_FALSE 0
#define GL_CANVAS_STENCIL_TRUE 1
#define GL_CANVAS_STENCIL_FALSE 0
#define GL_CANVAS_ALPHA_TRUE 1
#define GL_CANVAS_ALPHA_FALSE 0

#ifdef CWASM_THREADS

    __attribute__( ( import_module( "env" ), import_name( "__async_wait_signal" ) ) ) extern int cwasm_env_async_wait_signal( void );

    CWASM_JS_LIB( CANVAS, int, js_gl_acquire_canvas, ( char const* css_id ), {
        if (typeof WorkerGlobalScope === "undefined" || !(self instanceof WorkerGlobalScope)) return 0;
        __async_signal_arm();
        self.postMessage({ type: "transfer.request", op: "canvas", arg: css_id ? mem.str_read(css_id) : "" });
        return 1;
    })

    CWASM_JS_LIB( CANVAS, int, js_gl_take_canvas, ( void ), {
        CWASM.canvas = CWASM.runtime.transfer.acquired || null;
        CWASM.runtime.transfer.acquired = null;
        return CWASM.canvas ? 1 : 0;
    })

    CWASM_JS_MAIN_LIB_INIT( CANVAS,
    (
        (CWASM.runtime.transfer.ops = CWASM.runtime.transfer.ops || {}).canvas = function (id) {
            var el = document.getElementById(id);
            if (!el || el._cwasm_offscreen || el._cwasm_inuse || typeof el.transferControlToOffscreen !== "function") return null;
            el._cwasm_offscreen = true;
            el._cwasm_inuse = true;
            return el.transferControlToOffscreen();
        };
    ),
    void, js_gl_canvas_op_ready, (void), {})

#endif

CWASM_JS_MAIN( int, js_gl_enumerate_canvases, ( gl_canvas_t* out, int max ), {
    if (typeof document === "undefined" || !document.querySelectorAll) return 0;
    var els = document.querySelectorAll("canvas");
    var lim = max | 0, p = out | 0, n = 0;
    for (var i = 0; i < els.length && n < lim; i++) {
        var el = els[i];
        if (!el.id) continue;
        mem.str_write(el.id, p, 64);
        var w = el.clientWidth | 0, h = el.clientHeight | 0;
        mem.u32_write(p + 64, w > 0 ? w : (el.width | 0) || 1);
        mem.u32_write(p + 68, h > 0 ? h : (el.height | 0) || 1);
        mem.u32_write(p + 72, el._cwasm_inuse ? 1 : 0);
        p += 76;
        n++;
    }
    return n;
})

CWASM_JS_LIB_INIT( GL,
(
    const GLSCRATCH_N = 256, kUniforms = "u", kMaxUniformLength = "m", kMaxAttributeLength = "a", kMaxUniformBlockNameLength = "b";
    var GLctx;
    var GLcounter = 1;
    var GLlastError = 0;
    var GLbuffers = [];
    var GLprograms = [];
    var GLframebuffers = [];
    var GLtextures = [];
    var GLrenderbuffers = [];
    var GLuniforms = [];
    var GLshaders = [];
    var GLvaos = [];
    var GLqueries = [];
    var GLsyncs = [];
    var GLsamplers = [];
    var GLtransformFeedbacks = [];
    var GLprogramMeta = {};
    var GLpackAlignment = 4;
    var GLunpackAlignment = 4;
    var GLargCache = [];
    var GLscratchF32 = [];
    var GLscratchI32 = [];
    var GLmappedBuffers = {};
    for(let i = 0, fbuf = new Float32Array(GLSCRATCH_N), ibuf = new Int32Array(GLSCRATCH_N); i < GLSCRATCH_N; i++) {
        GLscratchF32[i] = fbuf.subarray(0, i + 1);
        GLscratchI32[i] = ibuf.subarray(0, i + 1);
    }

    function GLnewId(table) {
        var id = GLcounter++;
        while (table.length < id) table.push(null);
        return id;
    }

    function GLsetError(err) {
        if (!GLlastError) GLlastError = err;
    }

    function GLpixelView(type, format, width, height, pixels, internalFormat, align) {
        var channels;
        switch (format) {
            case 0x1906: case 0x1909: case 0x1902: case 0x1903: case 0x8D94: case 0x84F9: channels = 1; break;
            case 0x190A: case 0x8227: case 0x8228: channels = 2; break;
            case 0x1907: case 0x8D98: channels = 3; break;
            default: channels = 4; break;
        }
        var heap, bytes, packed = 0;
        switch (type) {
            case 0x1400: heap = new Int8Array(mem.buffer()); bytes = 1; break;
            case 0x1401: heap = new Uint8Array(mem.buffer()); bytes = 1; break;
            case 0x1402: heap = new Int16Array(mem.buffer()); bytes = 2; break;
            case 0x1403: case 0x140B: case 0x8D61: heap = new Uint16Array(mem.buffer()); bytes = 2; break;
            case 0x8363: case 0x8033: case 0x8034: heap = new Uint16Array(mem.buffer()); bytes = 2; packed = 1; break;
            case 0x1404: heap = new Int32Array(mem.buffer()); bytes = 4; break;
            case 0x1405: heap = new Uint32Array(mem.buffer()); bytes = 4; break;
            case 0x8368: case 0x8C3B: case 0x8C3E: case 0x84FA: heap = new Uint32Array(mem.buffer()); bytes = 4; packed = 1; break;
            case 0x1406: heap = new Float32Array(mem.buffer()); bytes = 4; break;
            default: heap = new Uint8Array(mem.buffer()); bytes = 1; break;
        }
        var pixelBytes = packed ? bytes : bytes * channels;
        var rowBytes = width * pixelBytes;
        var a = align || GLunpackAlignment;
        var alignedRow = Math.floor((rowBytes + a - 1) / a) * a;
        var total = height <= 0 ? 0 : (height - 1) * alignedRow + rowBytes;
        var start = (pixels / bytes) | 0;
        return heap.subarray(start, start + ((total / bytes) | 0));
    }

    function GLget(name, p, type) {
        if (!p) { GLsetError(0x501); return; }
        var put = function(idx, val) {
            if (type == 2) mem.f32_write(p + idx * 4, val);
            else if (type == 4) mem.raw_view(p + idx, 1)[0] = val ? 1 : 0;
            else mem.i32_write(p + idx * 4, val);
        };
        var i;
        switch (name) {
            case 0x8DFA: put(0, 1); return;
            case 0x8DF9: put(0, 0); return;
            case 0x8DF8: return;
            case 0x86A2: put(0, (GLctx.getParameter(0x86A3) || []).length); return;
        }
        var v = GLctx.getParameter(name);
        if (v == null)
            put(0, 0);
        else if (typeof v == "number" || typeof v == "boolean")
            put(0, +v);
        else if (typeof v == "string")
            GLsetError(0x500);
        else if (v.length !== undefined)
            for (i = 0; i < v.length; i++) put(i, +v[i]);
        else
            put(0, v.name | 0);
    }

    function GLwriteNumOrArr(data, params, type) {
        var write = type == 2 ? mem.f32_write : mem.i32_write, i;
        if (data != null && data.length !== undefined)
            for (i = 0; i < data.length; i++) write(params + i * 4, +data[i]);
        else
            write(params, +data);
    }

    function GLgetUniform(program, location, params, type) {
        GLwriteNumOrArr(GLctx.getUniform(GLprograms[program], GLuniforms[location]), params, type);
    }

    function GLgetVertexAttrib(index, pname, params, type) {
        var data = GLctx.getVertexAttrib(index, pname);
        if (pname == 0x889F) mem.i32_write(params, GLobjId(data));
        else GLwriteNumOrArr(data, params, type);
    }

    function GLeachId(n, ptr, fn) {
        for (var i = 0; i < n; i++) fn(i, mem.u32_read(ptr + 4 * i));
    }

    function GLgenObjects(n, buffers, createFunction, objectTable) {
        GLeachId(n, buffers, function (i) {
            var obj = GLctx[createFunction](), id = 0;
            if (!obj) GLsetError(0x502);
            else {
                id = GLnewId(objectTable);
                obj.name = id;
                objectTable[id] = obj;
            }
            mem.i32_write(buffers + 4 * i, id);
        });
    }

    function GLdeleteObjects(n, ptr, deleteFunction, objectTable) {
        GLeachId(n, ptr, function (i, id) {
            var obj = objectTable[id];
            if (!obj) return;
            GLctx[deleteFunction](obj);
            obj.name = 0;
            objectTable[id] = null;
        });
    }

    function GLobjId(handle) {
        return (handle && handle.name) | 0;
    }

    function GLcreateObject(createFunction, objectTable, arg) {
        var id = GLnewId(objectTable);
        var obj = GLctx[createFunction](arg);
        if (obj) obj.name = id;
        objectTable[id] = obj;
        return id;
    }

    function GLdeleteOne(deleteFunction, objectTable, id) {
        var obj = objectTable[id];
        if (!obj) return 0;
        GLctx[deleteFunction](obj);
        obj.name = 0;
        objectTable[id] = null;
        return 1;
    }

    function GLuniformVec(setter, location, count, comps, value, isInt) {
        var total = count * comps;
        var heap = isInt ? new Int32Array(mem.buffer()) : new Float32Array(mem.buffer());
        var first = value >> 2;
        var view;
        if (total <= GLSCRATCH_N) {
            view = (isInt ? GLscratchI32 : GLscratchF32)[total - 1];
            for (var i = 0; i < total; i++) view[i] = heap[first + i];
        }
        else view = heap.subarray(first, first + total);
        GLctx[setter](GLuniforms[location], view);
    }

    function GLinfoLog(getter, handle) {
        return GLctx[getter](handle) || "(unknown error)";
    }

    function GLoutStr(str, ptr, bufSize, lenPtr) {
        var wrote = (ptr && bufSize > 0) ? mem.str_write(str, ptr, bufSize) : 0;
        if (lenPtr) mem.i32_write(lenPtr, wrote);
    }

    function GLmaxNameLen(ptable, key, program, countEnum, nameAt) {
        if (ptable[key] < 0) {
            var p = GLprograms[program], longest = 0;
            var count = GLctx.getProgramParameter(p, countEnum);
            for (var i = 0; i < count; i++) {
                var nm = nameAt(p, i);
                if (nm && nm.length >= longest) longest = nm.length + 1;
            }
            ptable[key] = longest;
        }
        return ptable[key];
    }

    function GLuniformMatrixNfv(location, count, transpose, value, elems, fname) {
        var n = count * elems, view, i, heap = new Float32Array(mem.buffer());
        value >>= 2;
        if (n <= GLSCRATCH_N) {
            view = GLscratchF32[n - 1];
            for (i = 0; i != n; i++) view[i] = heap[value + i];
        }
        else view = heap.subarray(value, value + n);
        GLctx[fname](GLuniforms[location], !!transpose, view);
    }

    function GLuniformNuiv(location, count, value, mult, fname) {
        var n = count * mult, heap = new Uint32Array(mem.buffer());
        value >>= 2;
        GLctx[fname](GLuniforms[location], new Uint32Array(heap.subarray(value, value + n)));
    }
),

int, js_glSetupCanvasContext, ( int antialias, int depth, int stencil, int alpha, char const* css_id, char const* ctx_name ), {
    var id = css_id ? mem.str_read(css_id) : "";
    var canvas;
    if (typeof WorkerGlobalScope !== "undefined" && self instanceof WorkerGlobalScope) {
        canvas = CWASM.canvas;
    } else {
        canvas = (id && typeof document !== "undefined" && document.getElementById) ? document.getElementById(id) : null;
        if (canvas && canvas._cwasm_inuse) abort("WEBGL", "canvas in use: #" + id);
        if (canvas) { canvas._cwasm_inuse = true; CWASM.canvas = canvas; }
    }
    if (!canvas) abort("WEBGL", "no canvas for id" + (id ? ": #" + id : ""));
    var attr = { antialias: !!antialias, depth: !!depth, stencil: !!stencil, alpha: !!alpha };
    var msg = "";
    var onError = function(e) { msg = e.statusMessage || msg; };
    if (canvas.addEventListener) canvas.addEventListener("webglcontextcreationerror", onError, false);
    try {
        var ctx = mem.str_read(ctx_name);
        GLctx = canvas.getContext(ctx, attr);
        if (!GLctx) throw ctx + " unavailable";
        GLctx.getExtension("WEBGL_compressed_texture_s3tc");
    }
    catch (e) {
        abort("WEBGL", String(e) + (msg ? " (" + msg + ")" : ""));
    }
    finally {
        if (canvas.removeEventListener) canvas.removeEventListener("webglcontextcreationerror", onError, false);
    }
    if (canvas && canvas.addEventListener) {
        canvas.addEventListener("webglcontextlost", function(e) {
            if (e && e.preventDefault) e.preventDefault();
        }, false);
    }
    return 1;
})

CWASM_JS_LIB( GL, void, glActiveTexture, ( GLenum texture ), {
    GLctx.activeTexture(texture);
})

CWASM_JS_LIB( GL, void, glAttachShader, ( GLuint program, GLuint shader ), {
    GLctx.attachShader(GLprograms[program], GLshaders[shader]);
})

CWASM_JS_LIB( GL, void, glBindAttribLocation, ( GLuint program, GLuint index, GLchar const* name ), {
    GLctx.bindAttribLocation(GLprograms[program], index, mem.str_read(name));
})

CWASM_JS_LIB( GL, void, glBindBuffer, ( GLenum target, GLuint buffer ), {
    GLctx.bindBuffer(target, buffer ? GLbuffers[buffer] : null);
})

CWASM_JS_LIB( GL, void, glBindFramebuffer, ( GLenum target, GLuint framebuffer ), {
    GLctx.bindFramebuffer(target, framebuffer ? GLframebuffers[framebuffer] : null);
})

CWASM_JS_LIB( GL, void, glBindRenderbuffer, ( GLenum target, GLuint renderbuffer ), {
    GLctx.bindRenderbuffer(target, renderbuffer ? GLrenderbuffers[renderbuffer] : null);
})

CWASM_JS_LIB( GL, void, glBindTexture, ( GLenum target, GLuint texture ), {
    GLctx.bindTexture(target, texture ? GLtextures[texture] : null);
})

CWASM_JS_LIB( GL, void, glBlendColor, ( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ), {
    GLctx.blendColor(red, green, blue, alpha);
})

CWASM_JS_LIB( GL, void, glBlendEquation, ( GLenum mode ), {
    GLctx.blendEquation(mode);
})

CWASM_JS_LIB( GL, void, glBlendEquationSeparate, ( GLenum modeRGB, GLenum modeAlpha ), {
    GLctx.blendEquationSeparate(modeRGB, modeAlpha);
})

CWASM_JS_LIB( GL, void, glBlendFunc, ( GLenum sfactor, GLenum dfactor ), {
    GLctx.blendFunc(sfactor, dfactor);
})

CWASM_JS_LIB( GL, void, glBlendFuncSeparate, ( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha ), {
    GLctx.blendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
})

CWASM_JS_LIB( GL, void, glBufferData, ( GLenum target, GLsizeiptr size, void const* data, GLenum usage ), {
    if (!data) GLctx.bufferData(target, size, usage);
    else GLctx.bufferData(target, mem.raw_view(data, size), usage);
})

CWASM_JS_LIB( GL, void, glBufferSubData, ( GLenum target, GLintptr offset, GLsizeiptr size, void const* data ), {
    GLctx.bufferSubData(target, offset, mem.raw_view(data, size));
})

CWASM_JS_LIB( GL, GLenum, glCheckFramebufferStatus, ( GLenum target ), {
    return GLctx.checkFramebufferStatus(target);
})

CWASM_JS_LIB( GL, void, glClear, ( GLbitfield mask ), {
    GLctx.clear(mask);
})

CWASM_JS_LIB( GL, void, glClearColor, ( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ), {
    GLctx.clearColor(red, green, blue, alpha);
})

CWASM_JS_LIB( GL, void, glClearDepthf, ( GLfloat d ), {
    GLctx.clearDepth(d);
})

CWASM_JS_LIB( GL, void, glClearStencil, ( GLint s ), {
    GLctx.clearStencil(s);
})

CWASM_JS_LIB( GL, void, glColorMask, ( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha ), {
    GLctx.colorMask(!!red, !!green, !!blue, !!alpha);
})

CWASM_JS_LIB( GL, void, glCompileShader, ( GLuint shader ), {
    GLctx.compileShader(GLshaders[shader]);
})

CWASM_JS_LIB( GL, void, glCompressedTexImage2D, ( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, void const* data ), {
    GLctx.compressedTexImage2D(target, level, internalformat, width, height, border, mem.raw_view(data, imageSize));
})

CWASM_JS_LIB( GL, void, glCompressedTexSubImage2D, ( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, void const* data ), {
    GLctx.compressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, mem.raw_view(data, imageSize));
})

CWASM_JS_LIB( GL, void, glCopyTexImage2D, ( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ), {
    GLctx.copyTexImage2D(target, level, internalformat, x, y, width, height, border);
})

CWASM_JS_LIB( GL, void, glCopyTexSubImage2D, ( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ), {
    GLctx.copyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
})

CWASM_JS_LIB( GL, GLuint, glCreateProgram, ( void ), {
    return GLcreateObject("createProgram", GLprograms);
})

CWASM_JS_LIB( GL, GLuint, glCreateShader, ( GLenum type ), {
    return GLcreateObject("createShader", GLshaders, type);
})

CWASM_JS_LIB( GL, void, glCullFace, ( GLenum mode ), {
    GLctx.cullFace(mode);
})

CWASM_JS_LIB( GL, void, glDeleteBuffers, ( GLsizei n, GLuint const* buffers ), {
    GLdeleteObjects(n, buffers, "deleteBuffer", GLbuffers);
})

CWASM_JS_LIB( GL, void, glDeleteFramebuffers, ( GLsizei n, GLuint const* framebuffers ), {
    GLdeleteObjects(n, framebuffers, "deleteFramebuffer", GLframebuffers);
})

CWASM_JS_LIB( GL, void, glDeleteProgram, ( GLuint program ), {
    if (GLdeleteOne("deleteProgram", GLprograms, program)) GLprogramMeta[program] = null;
})

CWASM_JS_LIB( GL, void, glDeleteRenderbuffers, ( GLsizei n, GLuint const* renderbuffers ), {
    GLdeleteObjects(n, renderbuffers, "deleteRenderbuffer", GLrenderbuffers);
})

CWASM_JS_LIB( GL, void, glDeleteShader, ( GLuint shader ), {
    GLdeleteOne("deleteShader", GLshaders, shader);
})

CWASM_JS_LIB( GL, void, glDeleteTextures, ( GLsizei n, GLuint const* textures ), {
    GLdeleteObjects(n, textures, "deleteTexture", GLtextures);
})

CWASM_JS_LIB( GL, void, glDepthFunc, ( GLenum func ), {
    GLctx.depthFunc(func);
})

CWASM_JS_LIB( GL, void, glDepthMask, ( GLboolean flag ), {
    GLctx.depthMask(!!flag);
})

CWASM_JS_LIB( GL, void, glDepthRangef, ( GLfloat n, GLfloat f ), {
    GLctx.depthRangef(n, f);
})

CWASM_JS_LIB( GL, void, glDetachShader, ( GLuint program, GLuint shader ), {
    GLctx.detachShader(GLprograms[program], GLshaders[shader]);
})

CWASM_JS_LIB( GL, void, glDisable, ( GLenum cap ), {
    GLctx.disable(cap);
})

CWASM_JS_LIB( GL, void, glDisableVertexAttribArray, ( GLuint index ), {
    GLctx.disableVertexAttribArray(index);
})

CWASM_JS_LIB( GL, void, glDrawArrays, ( GLenum mode, GLint first, GLsizei count ), {
    GLctx.drawArrays(mode, first, count);
})

CWASM_JS_LIB( GL, void, glDrawElements, ( GLenum mode, GLsizei count, GLenum type, void const* indices ), {
    GLctx.drawElements(mode, count, type, indices);
})

CWASM_JS_LIB( GL, void, glEnable, ( GLenum cap ), {
    GLctx.enable(cap);
})

CWASM_JS_LIB( GL, void, glEnableVertexAttribArray, ( GLuint index ), {
    GLctx.enableVertexAttribArray(index);
})

CWASM_JS_LIB( GL, void, glFinish, ( void ), {
    GLctx.finish();
})

CWASM_JS_LIB( GL, void, glFlush, ( void ), {
    GLctx.flush();
})

CWASM_JS_LIB( GL, void, glFramebufferRenderbuffer, ( GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer ), {
    GLctx.framebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer ? GLrenderbuffers[renderbuffer] : null);
})

CWASM_JS_LIB( GL, void, glFramebufferTexture2D, ( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level ), {
    GLctx.framebufferTexture2D(target, attachment, textarget, texture ? GLtextures[texture] : null, level);
})

CWASM_JS_LIB( GL, void, glFrontFace, ( GLenum mode ), {
    GLctx.frontFace(mode);
})

CWASM_JS_LIB( GL, void, glGenBuffers, ( GLsizei n, GLuint* buffers ), {
    GLgenObjects(n, buffers, "createBuffer", GLbuffers);
})

CWASM_JS_LIB( GL, void, glGenerateMipmap, ( GLenum target ), {
    GLctx.generateMipmap(target);
})

CWASM_JS_LIB( GL, void, glGenFramebuffers, ( GLsizei n, GLuint* framebuffers ), {
    GLgenObjects(n, framebuffers, "createFramebuffer", GLframebuffers);
})

CWASM_JS_LIB( GL, void, glGenRenderbuffers, ( GLsizei n, GLuint* renderbuffers ), {
    GLgenObjects(n, renderbuffers, "createRenderbuffer", GLrenderbuffers);
})

CWASM_JS_LIB( GL, void, glGenTextures, ( GLsizei n, GLuint* textures ), {
    GLgenObjects(n, textures, "createTexture", GLtextures);
})

CWASM_JS_LIB( GL, void, glGetActiveAttrib, ( GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name ), {
    var info = GLctx.getActiveAttrib(GLprograms[program], index);
    if (!info) return;
    GLoutStr(info.name, name, bufSize, length);
    if (size) mem.i32_write(size, info.size);
    if (type) mem.i32_write(type, info.type);
})

CWASM_JS_LIB( GL, void, glGetActiveUniform, ( GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name ), {
    var info = GLctx.getActiveUniform(GLprograms[program], index);
    if (!info) return;
    GLoutStr(info.name, name, bufSize, length);
    if (size) mem.i32_write(size, info.size);
    if (type) mem.i32_write(type, info.type);
})

CWASM_JS_LIB( GL, void, glGetAttachedShaders, ( GLuint program, GLsizei maxCount, GLsizei* count, GLuint* shaders ), {
    var attached = GLctx.getAttachedShaders(GLprograms[program]) || [];
    var n = Math.min(attached.length, maxCount);
    for (var i = 0; i < n; i++) mem.i32_write(shaders + 4 * i, GLobjId(attached[i]));
    if (count) mem.i32_write(count, n);
})

CWASM_JS_LIB( GL, GLint, glGetAttribLocation, ( GLuint program, GLchar const* name ), {
    return GLctx.getAttribLocation(GLprograms[program], mem.str_read(name));
})

CWASM_JS_LIB( GL, void, glGetBooleanv, ( GLenum pname, GLboolean* data ), {
    GLget(pname, data, 4);
})

CWASM_JS_LIB( GL, void, glGetBufferParameteriv, ( GLenum target, GLenum pname, GLint* params ), {
    mem.i32_write(params, GLctx.getBufferParameter(target, pname));
})

CWASM_JS_LIB( GL, GLenum, glGetError, ( void ), {
    if (GLlastError) { var e = GLlastError; GLlastError = 0; return e; }
    return GLctx.getError();
})

CWASM_JS_LIB( GL, void, glGetFloatv, ( GLenum pname, GLfloat* data ), {
    GLget(pname, data, 2);
})

CWASM_JS_LIB( GL, void, glGetFramebufferAttachmentParameteriv, ( GLenum target, GLenum attachment, GLenum pname, GLint* params ), {
    var r = GLctx.getFramebufferAttachmentParameter(target, attachment, pname);
    mem.i32_write(params, (r && typeof r === "object") ? GLobjId(r) : r);
})

CWASM_JS_LIB( GL, void, glGetIntegerv, ( GLenum pname, GLint* data ), {
    GLget(pname, data, 0);
})

CWASM_JS_LIB( GL, void, glGetProgramiv, ( GLuint program, GLenum pname, GLint* params ), {
    if (program >= GLcounter) return GLsetError(0x501);
    var ptable = GLprogramMeta[program];
    if (!ptable) return GLsetError(0x502);
    var special = {
        0x8B84: function () { return GLinfoLog("getProgramInfoLog", GLprograms[program]).length + 1; },
        0x8B87: function () { return ptable[kMaxUniformLength]; },
        0x8B8A: function () {
            return GLmaxNameLen(ptable, kMaxAttributeLength, program, GLctx.ACTIVE_ATTRIBUTES,
                function (p, i) { return GLctx.getActiveAttrib(p, i).name; });
        },
        0x8A35: function () {
            return GLmaxNameLen(ptable, kMaxUniformBlockNameLength, program, GLctx.ACTIVE_UNIFORM_BLOCKS,
                function (p, i) { return GLctx.getActiveUniformBlockName(p, i); });
        }
    }[pname];
    mem.i32_write(params, special ? special() : GLctx.getProgramParameter(GLprograms[program], pname));
})

CWASM_JS_LIB( GL, void, glGetProgramInfoLog, ( GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog ), {
    GLoutStr(GLinfoLog("getProgramInfoLog", GLprograms[program]), infoLog, bufSize, length);
})

CWASM_JS_LIB( GL, void, glGetRenderbufferParameteriv, ( GLenum target, GLenum pname, GLint* params ), {
    mem.i32_write(params, GLctx.getRenderbufferParameter(target, pname));
})

CWASM_JS_LIB( GL, void, glGetShaderiv, ( GLuint shader, GLenum pname, GLint* params ), {
    var sh = GLshaders[shader], res;
    if (pname == 0x8B84)
        res = GLinfoLog("getShaderInfoLog", sh).length + 1;
    else if (pname == 0x8B88) {
        var src = GLctx.getShaderSource(sh);
        res = src ? src.length + 1 : 0;
    }
    else
        res = GLctx.getShaderParameter(sh, pname);
    mem.i32_write(params, res);
})

CWASM_JS_LIB( GL, void, glGetShaderInfoLog, ( GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog ), {
    GLoutStr(GLinfoLog("getShaderInfoLog", GLshaders[shader]), infoLog, bufSize, length);
})

CWASM_JS_LIB( GL, void, glGetShaderPrecisionFormat, ( GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision ), {
    var result = GLctx.getShaderPrecisionFormat(shadertype, precisiontype);
    if (!result) return GLsetError(0x500);
    mem.i32_write(range, result.rangeMin);
    mem.i32_write(range + 4, result.rangeMax);
    mem.i32_write(precision, result.precision);
})

CWASM_JS_LIB( GL, void, glGetShaderSource, ( GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* source ), {
    var src = GLctx.getShaderSource(GLshaders[shader]);
    if (src == null) return GLsetError(0x501);
    GLoutStr(src, source, bufSize, length);
})

CWASM_JS_LIB( GL, void, glGetTexParameterfv, ( GLenum target, GLenum pname, GLfloat* params ), {
    mem.f32_write(params, GLctx.getTexParameter(target, pname));
})

CWASM_JS_LIB( GL, void, glGetTexParameteriv, ( GLenum target, GLenum pname, GLint* params ), {
    mem.i32_write(params, GLctx.getTexParameter(target, pname));
})

CWASM_JS_LIB( GL, void, glGetUniformfv, ( GLuint program, GLint location, GLfloat* params ), {
    GLgetUniform(program, location, params, 2);
})

CWASM_JS_LIB( GL, void, glGetUniformiv, ( GLuint program, GLint location, GLint* params ), {
    GLgetUniform(program, location, params, 0);
})

CWASM_JS_LIB( GL, GLint, glGetUniformLocation, ( GLuint program, GLchar const* name ), {
    var ptable = GLprogramMeta[program];
    if (!ptable) return -1;
    var uname = mem.str_read(name), arrayOffset = 0;
    if (uname[uname.length - 1] == "]") {
        var lb = uname.lastIndexOf("[");
        arrayOffset = parseInt(uname.slice(lb + 1), 10);
        uname = uname.slice(0, lb);
    }
    var info = ptable[kUniforms][uname];
    if (info && arrayOffset >= 0 && arrayOffset < info[0]) return info[1] + arrayOffset;
    return -1;
})

CWASM_JS_LIB( GL, void, glGetVertexAttribfv, ( GLuint index, GLenum pname, GLfloat* params ), {
    GLgetVertexAttrib(index, pname, params, 2);
})

CWASM_JS_LIB( GL, void, glGetVertexAttribiv, ( GLuint index, GLenum pname, GLint* params ), {
    GLgetVertexAttrib(index, pname, params, 0);
})

CWASM_JS_LIB( GL, void, glGetVertexAttribPointerv, ( GLuint index, GLenum pname, void** pointer ), {
    mem.i32_write(pointer, GLctx.getVertexAttribOffset(index, pname));
})

CWASM_JS_LIB( GL, void, glHint, ( GLenum target, GLenum mode ), {
    GLctx.hint(target, mode);
})

CWASM_JS_LIB( GL, GLboolean, glIsBuffer, ( GLuint buffer ), {
    return !!GLctx.isBuffer(GLbuffers[buffer]);
})

CWASM_JS_LIB( GL, GLboolean, glIsEnabled, ( GLenum cap ), {
    return !!GLctx.isEnabled(cap);
})

CWASM_JS_LIB( GL, GLboolean, glIsFramebuffer, ( GLuint framebuffer ), {
    return !!GLctx.isFramebuffer(GLframebuffers[framebuffer]);
})

CWASM_JS_LIB( GL, GLboolean, glIsProgram, ( GLuint program ), {
    return !!GLctx.isProgram(GLprograms[program]);
})

CWASM_JS_LIB( GL, GLboolean, glIsRenderbuffer, ( GLuint renderbuffer ), {
    return !!GLctx.isRenderbuffer(GLrenderbuffers[renderbuffer]);
})

CWASM_JS_LIB( GL, GLboolean, glIsShader, ( GLuint shader ), {
    return !!GLctx.isShader(GLshaders[shader]);
})

CWASM_JS_LIB( GL, GLboolean, glIsTexture, ( GLuint texture ), {
    return !!GLctx.isTexture(GLtextures[texture]);
})

CWASM_JS_LIB( GL, void, glLineWidth, ( GLfloat width ), {
    GLctx.lineWidth(width);
})

CWASM_JS_LIB( GL, void, glLinkProgram, ( GLuint program ), {
    var p = GLprograms[program];
    GLctx.linkProgram(p);

    var utable = {}, longest = 0;
    var count = GLctx.getProgramParameter(p, GLctx.ACTIVE_UNIFORMS);
    for (var i = 0; i < count; i++) {
        var info = GLctx.getActiveUniform(p, i);
        if (info.name.length >= longest) longest = info.name.length + 1;

        var base = info.name;
        if (base.charAt(base.length - 1) === "]") base = base.slice(0, base.lastIndexOf("["));

        var loc = GLctx.getUniformLocation(p, base);
        if (!loc) continue;
        var uid = GLnewId(GLuniforms);
        GLuniforms[uid] = loc;
        utable[base] = [info.size, uid];
        for (var e = 1; e < info.size; e++)
            GLuniforms[GLnewId(GLuniforms)] = GLctx.getUniformLocation(p, base + "[" + e + "]");
    }

    GLprogramMeta[program] = {
        [kUniforms]: utable,
        [kMaxUniformLength]: longest,
        [kMaxAttributeLength]: -1,
        [kMaxUniformBlockNameLength]: -1
    };
})

CWASM_JS_LIB( GL, void, glPixelStorei, ( GLenum pname, GLint param ), {
    if (pname == 0x0CF5) GLunpackAlignment = param;
    if (pname == 0x0D05) GLpackAlignment = param;
    GLctx.pixelStorei(pname, param);
})

CWASM_JS_LIB( GL, void, glPolygonOffset, ( GLfloat factor, GLfloat units ), {
    GLctx.polygonOffset(factor, units);
})

CWASM_JS_LIB( GL, void, glReadPixels, ( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels ), {
    GLctx.readPixels(x, y, width, height, format, type, GLpixelView(type, format, width, height, pixels, 0, GLpackAlignment));
})

CWASM_JS_LIB( GL, void, glReleaseShaderCompiler, ( void ), {
})

CWASM_JS_LIB( GL, void, glRenderbufferStorage, ( GLenum target, GLenum internalformat, GLsizei width, GLsizei height ), {
    GLctx.renderbufferStorage(target, internalformat, width, height);
})

CWASM_JS_LIB( GL, void, glSampleCoverage, ( GLfloat value, GLboolean invert ), {
    GLctx.sampleCoverage(value, !!invert);
})

CWASM_JS_LIB( GL, void, glScissor, ( GLint x, GLint y, GLsizei width, GLsizei height ), {
    GLctx.scissor(x, y, width, height);
})

CWASM_JS_LIB( GL, void, glShaderBinary, ( GLsizei count, GLuint const* shaders, GLenum binaryFormat, void const* binary, GLsizei length ), {
    GLsetError(0x500);
})

CWASM_JS_LIB( GL, void, glShaderSource, ( GLuint shader, GLsizei count, GLchar const* const* string, GLint const* length ), {
    var source = "", i;
    for (i = 0; i < count; i++) {
        var ptr = mem.u32_read(string + i * 4);
        var len = length ? mem.i32_read(length + i * 4) : -1;
        source += (len < 0) ? mem.str_read(ptr) : mem.str_read(ptr, len);
    }
    GLctx.shaderSource(GLshaders[shader], source);
})

CWASM_JS_LIB( GL, void, glStencilFunc, ( GLenum func, GLint ref, GLuint mask ), {
    GLctx.stencilFunc(func, ref, mask);
})

CWASM_JS_LIB( GL, void, glStencilFuncSeparate, ( GLenum face, GLenum func, GLint ref, GLuint mask ), {
    GLctx.stencilFuncSeparate(face, func, ref, mask);
})

CWASM_JS_LIB( GL, void, glStencilMask, ( GLuint mask ), {
    GLctx.stencilMask(mask);
})

CWASM_JS_LIB( GL, void, glStencilMaskSeparate, ( GLenum face, GLuint mask ), {
    GLctx.stencilMaskSeparate(face, mask);
})

CWASM_JS_LIB( GL, void, glStencilOp, ( GLenum fail, GLenum zfail, GLenum zpass ), {
    GLctx.stencilOp(fail, zfail, zpass);
})

CWASM_JS_LIB( GL, void, glStencilOpSeparate, ( GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass ), {
    GLctx.stencilOpSeparate(face, sfail, dpfail, dppass);
})

CWASM_JS_LIB( GL, void, glTexImage2D, ( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, void const* pixels ), {
    GLctx.texImage2D(target, level, internalformat, width, height, border, format, type,
        pixels ? GLpixelView(type, format, width, height, pixels, internalformat) : null);
})

CWASM_JS_LIB( GL, void, glTexParameterf, ( GLenum target, GLenum pname, GLfloat param ), {
    GLctx.texParameterf(target, pname, param);
})

CWASM_JS_LIB( GL, void, glTexParameterfv, ( GLenum target, GLenum pname, GLfloat const* params ), {
    GLctx.texParameterf(target, pname, mem.f32_read(params));
})

CWASM_JS_LIB( GL, void, glTexParameteri, ( GLenum target, GLenum pname, GLint param ), {
    GLctx.texParameteri(target, pname, param);
})

CWASM_JS_LIB( GL, void, glTexParameteriv, ( GLenum target, GLenum pname, GLint const* params ), {
    GLctx.texParameteri(target, pname, mem.i32_read(params));
})

CWASM_JS_LIB( GL, void, glTexSubImage2D, ( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, void const* pixels ), {
    GLctx.texSubImage2D(target, level, xoffset, yoffset, width, height, format, type,
        pixels ? GLpixelView(type, format, width, height, pixels, 0) : null);
})

CWASM_JS_LIB( GL, void, glUniform1f, ( GLint location, GLfloat v0 ), {
    GLctx.uniform1f(GLuniforms[location], v0);
})

CWASM_JS_LIB( GL, void, glUniform1fv, ( GLint location, GLsizei count, GLfloat const* value ), {
    GLuniformVec("uniform1fv", location, count, 1, value, 0);
})

CWASM_JS_LIB( GL, void, glUniform1i, ( GLint location, GLint v0 ), {
    GLctx.uniform1i(GLuniforms[location], v0);
})

CWASM_JS_LIB( GL, void, glUniform1iv, ( GLint location, GLsizei count, GLint const* value ), {
    GLuniformVec("uniform1iv", location, count, 1, value, 1);
})

CWASM_JS_LIB( GL, void, glUniform2f, ( GLint location, GLfloat v0, GLfloat v1 ), {
    GLctx.uniform2f(GLuniforms[location], v0, v1);
})

CWASM_JS_LIB( GL, void, glUniform2fv, ( GLint location, GLsizei count, GLfloat const* value ), {
    GLuniformVec("uniform2fv", location, count, 2, value, 0);
})

CWASM_JS_LIB( GL, void, glUniform2i, ( GLint location, GLint v0, GLint v1 ), {
    GLctx.uniform2i(GLuniforms[location], v0, v1);
})

CWASM_JS_LIB( GL, void, glUniform2iv, ( GLint location, GLsizei count, GLint const* value ), {
    GLuniformVec("uniform2iv", location, count, 2, value, 1);
})

CWASM_JS_LIB( GL, void, glUniform3f, ( GLint location, GLfloat v0, GLfloat v1, GLfloat v2 ), {
    GLctx.uniform3f(GLuniforms[location], v0, v1, v2);
})

CWASM_JS_LIB( GL, void, glUniform3fv, ( GLint location, GLsizei count, GLfloat const* value ), {
    GLuniformVec("uniform3fv", location, count, 3, value, 0);
})

CWASM_JS_LIB( GL, void, glUniform3i, ( GLint location, GLint v0, GLint v1, GLint v2 ), {
    GLctx.uniform3i(GLuniforms[location], v0, v1, v2);
})

CWASM_JS_LIB( GL, void, glUniform3iv, ( GLint location, GLsizei count, GLint const* value ), {
    GLuniformVec("uniform3iv", location, count, 3, value, 1);
})

CWASM_JS_LIB( GL, void, glUniform4f, ( GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 ), {
    GLctx.uniform4f(GLuniforms[location], v0, v1, v2, v3);
})

CWASM_JS_LIB( GL, void, glUniform4fv, ( GLint location, GLsizei count, GLfloat const* value ), {
    GLuniformVec("uniform4fv", location, count, 4, value, 0);
})

CWASM_JS_LIB( GL, void, glUniform4i, ( GLint location, GLint v0, GLint v1, GLint v2, GLint v3 ), {
    GLctx.uniform4i(GLuniforms[location], v0, v1, v2, v3);
})

CWASM_JS_LIB( GL, void, glUniform4iv, ( GLint location, GLsizei count, GLint const* value ), {
    GLuniformVec("uniform4iv", location, count, 4, value, 1);
})

CWASM_JS_LIB( GL, void, glUniformMatrix2fv, ( GLint location, GLsizei count, GLboolean transpose, GLfloat const* value ), {
    GLuniformMatrixNfv(location, count, transpose, value, 4, "uniformMatrix2fv");
})

CWASM_JS_LIB( GL, void, glUniformMatrix3fv, ( GLint location, GLsizei count, GLboolean transpose, GLfloat const* value ), {
    GLuniformMatrixNfv(location, count, transpose, value, 9, "uniformMatrix3fv");
})

CWASM_JS_LIB( GL, void, glUniformMatrix4fv, ( GLint location, GLsizei count, GLboolean transpose, GLfloat const* value ), {
    GLuniformMatrixNfv(location, count, transpose, value, 16, "uniformMatrix4fv");
})

CWASM_JS_LIB( GL, void, glUseProgram, ( GLuint program ), {
    GLctx.useProgram(program ? GLprograms[program] : null);
})

CWASM_JS_LIB( GL, void, glValidateProgram, ( GLuint program ), {
    GLctx.validateProgram(GLprograms[program]);
})

CWASM_JS_LIB( GL, void, glVertexAttrib1f, ( GLuint index, GLfloat x ), {
    GLctx.vertexAttrib1f(index, x);
})

CWASM_JS_LIB( GL, void, glVertexAttrib1fv, ( GLuint index, GLfloat const* v ), {
    GLctx.vertexAttrib1f(index, mem.f32_read(v));
})

CWASM_JS_LIB( GL, void, glVertexAttrib2f, ( GLuint index, GLfloat x, GLfloat y ), {
    GLctx.vertexAttrib2f(index, x, y);
})

CWASM_JS_LIB( GL, void, glVertexAttrib2fv, ( GLuint index, GLfloat const* v ), {
    GLctx.vertexAttrib2f(index, mem.f32_read(v), mem.f32_read(v + 4));
})

CWASM_JS_LIB( GL, void, glVertexAttrib3f, ( GLuint index, GLfloat x, GLfloat y, GLfloat z ), {
    GLctx.vertexAttrib3f(index, x, y, z);
})

CWASM_JS_LIB( GL, void, glVertexAttrib3fv, ( GLuint index, GLfloat const* v ), {
    GLctx.vertexAttrib3f(index, mem.f32_read(v), mem.f32_read(v + 4), mem.f32_read(v + 8));
})

CWASM_JS_LIB( GL, void, glVertexAttrib4f, ( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ), {
    GLctx.vertexAttrib4f(index, x, y, z, w);
})

CWASM_JS_LIB( GL, void, glVertexAttrib4fv, ( GLuint index, GLfloat const* v ), {
    GLctx.vertexAttrib4f(index, mem.f32_read(v), mem.f32_read(v + 4), mem.f32_read(v + 8), mem.f32_read(v + 12));
})

CWASM_JS_LIB( GL, void, glVertexAttribPointer, ( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, void const* pointer ), {
    GLctx.vertexAttribPointer(index, size, type, !!normalized, stride, pointer);
})

CWASM_JS_LIB( GL, void, glViewport, ( GLint x, GLint y, GLsizei width, GLsizei height ), {
    GLctx.viewport(x, y, width, height);
})


#ifndef CWASM_GLES2

CWASM_JS_LIB( GL, void, glReadBuffer, ( GLenum src ), {
    GLctx.readBuffer(src);
})

CWASM_JS_LIB( GL, void, glDrawRangeElements, ( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, void const* indices ), {
    GLctx.drawRangeElements(mode, start, end, count, type, indices);
})

CWASM_JS_LIB( GL, void, glTexImage3D, ( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, void const* pixels ), {
    var pixelData = null;
    if (pixels) pixelData = GLpixelView(type, format, width, height * depth, pixels, internalformat);
    GLctx.texImage3D(target, level, internalformat, width, height, depth, border, format, type, pixelData);
})

CWASM_JS_LIB( GL, void, glTexSubImage3D, ( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, void const* pixels ), {
    var pixelData = null;
    if (pixels) pixelData = GLpixelView(type, format, width, height * depth, pixels, 0);
    GLctx.texSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixelData);
})

CWASM_JS_LIB( GL, void, glCopyTexSubImage3D, ( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height ), {
    GLctx.copyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
})

CWASM_JS_LIB( GL, void, glCompressedTexImage3D, ( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, void const* data ), {
    GLctx.compressedTexImage3D(target, level, internalformat, width, height, depth, border, data ? mem.raw_view(data, imageSize) : null);
})

CWASM_JS_LIB( GL, void, glCompressedTexSubImage3D, ( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, void const* data ), {
    GLctx.compressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, data ? mem.raw_view(data, imageSize) : null);
})

CWASM_JS_LIB( GL, void, glGenQueries, ( GLsizei n, GLuint* ids ), {
    GLgenObjects(n, ids, "createQuery", GLqueries);
})

CWASM_JS_LIB( GL, void, glDeleteQueries, ( GLsizei n, GLuint const* ids ), {
    GLdeleteObjects(n, ids, "deleteQuery", GLqueries);
})

CWASM_JS_LIB( GL, GLboolean, glIsQuery, ( GLuint id ), {
    var q = GLqueries[id];
    return (q ? GLctx.isQuery(q) : 0);
})

CWASM_JS_LIB( GL, void, glBeginQuery, ( GLenum target, GLuint id ), {
    GLctx.beginQuery(target, GLqueries[id]);
})

CWASM_JS_LIB( GL, void, glEndQuery, ( GLenum target ), {
    GLctx.endQuery(target);
})

CWASM_JS_LIB( GL, void, glGetQueryiv, ( GLenum target, GLenum pname, GLint* params ), {
    mem.i32_write(params, GLctx.getQuery(target, pname));
})

CWASM_JS_LIB( GL, void, glGetQueryObjectuiv, ( GLuint id, GLenum pname, GLuint* params ), {
    mem.i32_write(params, GLctx.getQueryParameter(GLqueries[id], pname));
})

CWASM_JS_LIB( GL, void, glGetBufferPointerv, ( GLenum target, GLenum pname, void** params ), {
    var m = GLmappedBuffers[target];
    mem.i32_write(params, (m ? m.ptr : 0));
})

CWASM_JS_LIB( GL, void, glDrawBuffers, ( GLsizei n, GLenum const* bufs ), {
    var arr = [], i;
    for (i = 0; i < n; i++) arr.push(mem.u32_read(bufs + i * 4));
    GLctx.drawBuffers(arr);
})

CWASM_JS_LIB( GL, void, glUniformMatrix2x3fv, ( GLint location, GLsizei count, GLboolean transpose, GLfloat const* value ), {
    GLuniformMatrixNfv(location, count, transpose, value, 6, "uniformMatrix2x3fv");
})

CWASM_JS_LIB( GL, void, glUniformMatrix3x2fv, ( GLint location, GLsizei count, GLboolean transpose, GLfloat const* value ), {
    GLuniformMatrixNfv(location, count, transpose, value, 6, "uniformMatrix3x2fv");
})

CWASM_JS_LIB( GL, void, glUniformMatrix2x4fv, ( GLint location, GLsizei count, GLboolean transpose, GLfloat const* value ), {
    GLuniformMatrixNfv(location, count, transpose, value, 8, "uniformMatrix2x4fv");
})

CWASM_JS_LIB( GL, void, glUniformMatrix4x2fv, ( GLint location, GLsizei count, GLboolean transpose, GLfloat const* value ), {
    GLuniformMatrixNfv(location, count, transpose, value, 8, "uniformMatrix4x2fv");
})

CWASM_JS_LIB( GL, void, glUniformMatrix3x4fv, ( GLint location, GLsizei count, GLboolean transpose, GLfloat const* value ), {
    GLuniformMatrixNfv(location, count, transpose, value, 12, "uniformMatrix3x4fv");
})

CWASM_JS_LIB( GL, void, glUniformMatrix4x3fv, ( GLint location, GLsizei count, GLboolean transpose, GLfloat const* value ), {
    GLuniformMatrixNfv(location, count, transpose, value, 12, "uniformMatrix4x3fv");
})

CWASM_JS_LIB( GL, void, glBlitFramebuffer, ( GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter ), {
    GLctx.blitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
})

CWASM_JS_LIB( GL, void, glRenderbufferStorageMultisample, ( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height ), {
    GLctx.renderbufferStorageMultisample(target, samples, internalformat, width, height);
})

CWASM_JS_LIB( GL, void, glFramebufferTextureLayer, ( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer ), {
    GLctx.framebufferTextureLayer(target, attachment, texture ? GLtextures[texture] : null, level, layer);
})

CWASM_JS_LIB( GL, void, glFlushMappedBufferRange, ( GLenum target, GLintptr offset, GLsizeiptr length ), {
    var m = GLmappedBuffers[target];
    if (!m) return GLsetError(0x502);
    GLctx.bufferSubData(target, m.offset + offset, mem.raw_view(m.ptr + offset, length));
})

CWASM_JS_LIB( GL, void, glBindVertexArray, ( GLuint array ), {
    GLctx.bindVertexArray(array ? GLvaos[array] : null);
})

CWASM_JS_LIB( GL, void, glDeleteVertexArrays, ( GLsizei n, GLuint const* arrays ), {
    GLdeleteObjects(n, arrays, "deleteVertexArray", GLvaos);
})

CWASM_JS_LIB( GL, void, glGenVertexArrays, ( GLsizei n, GLuint* arrays ), {
    GLgenObjects(n, arrays, "createVertexArray", GLvaos);
})

CWASM_JS_LIB( GL, GLboolean, glIsVertexArray, ( GLuint array ), {
    return !!GLctx.isVertexArray(GLvaos[array]);
})

CWASM_JS_LIB( GL, void, glGetIntegeri_v, ( GLenum target, GLuint index, GLint* data ), {
    var result = GLctx.getIndexedParameter(target, index);
    if (result === null) return GLsetError(0x500);
    if (typeof result === "number" || typeof result === "boolean")
        mem.i32_write(data, (result ? 1 : 0));
    else if (result instanceof Int32Array)
        for (var i = 0; i < result.length; i++) mem.i32_write(data + 4 * i, result[i]);
})

CWASM_JS_LIB( GL, void, glBeginTransformFeedback, ( GLenum primitiveMode ), {
    GLctx.beginTransformFeedback(primitiveMode);
})

CWASM_JS_LIB( GL, void, glEndTransformFeedback, ( void ), {
    GLctx.endTransformFeedback();
})

CWASM_JS_LIB( GL, void, glBindBufferRange, ( GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size ), {
    GLctx.bindBufferRange(target, index, buffer ? GLbuffers[buffer] : null, offset, size);
})

CWASM_JS_LIB( GL, void, glBindBufferBase, ( GLenum target, GLuint index, GLuint buffer ), {
    GLctx.bindBufferBase(target, index, buffer ? GLbuffers[buffer] : null);
})

CWASM_JS_LIB( GL, void, glTransformFeedbackVaryings, ( GLuint program, GLsizei count, GLchar const* const* varyings, GLenum bufferMode ), {
    var names = [];
    for (var i = 0; i < count; i++)
        names.push(mem.str_read(mem.u32_read(varyings + 4 * i)));
    GLctx.transformFeedbackVaryings(GLprograms[program], names, bufferMode);
})

CWASM_JS_LIB( GL, void, glGetTransformFeedbackVarying, ( GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLsizei* size, GLenum* type, GLchar* name ), {
    var info = GLctx.getTransformFeedbackVarying(GLprograms[program], index);
    if (!info) return;
    if (length) mem.i32_write(length, (bufSize > 0 && name ? mem.str_write(info.name, name, bufSize) : 0));
    if (size) mem.i32_write(size, info.size);
    if (type) mem.i32_write(type, info.type);
})

CWASM_JS_LIB( GL, void, glVertexAttribIPointer, ( GLuint index, GLint size, GLenum type, GLsizei stride, void const* pointer ), {
    GLctx.vertexAttribIPointer(index, size, type, stride, pointer);
})

CWASM_JS_LIB( GL, void, glGetVertexAttribIiv, ( GLuint index, GLenum pname, GLint* params ), {
    var data = GLctx.getVertexAttrib(index, pname);
    if (data === null) return;
    if (typeof data === "number") mem.i32_write(params, data);
    else for (var i = 0; i < data.length; i++) mem.i32_write(params + 4 * i, data[i]);
})

CWASM_JS_LIB( GL, void, glGetVertexAttribIuiv, ( GLuint index, GLenum pname, GLuint* params ), {
    var data = GLctx.getVertexAttrib(index, pname);
    if (data === null) return;
    if (typeof data === "number") mem.u32_write(params, data);
    else for (var i = 0; i < data.length; i++) mem.u32_write(params + 4 * i, data[i]);
})

CWASM_JS_LIB( GL, void, glVertexAttribI4i, ( GLuint index, GLint x, GLint y, GLint z, GLint w ), {
    GLctx.vertexAttribI4i(index, x, y, z, w);
})

CWASM_JS_LIB( GL, void, glVertexAttribI4ui, ( GLuint index, GLuint x, GLuint y, GLuint z, GLuint w ), {
    GLctx.vertexAttribI4ui(index, x, y, z, w);
})

CWASM_JS_LIB( GL, void, glVertexAttribI4iv, ( GLuint index, GLint const* v ), {
    GLctx.vertexAttribI4i(index, mem.i32_read(v), mem.i32_read(v + 4), mem.i32_read(v + 8), mem.i32_read(v + 12));
})

CWASM_JS_LIB( GL, void, glVertexAttribI4uiv, ( GLuint index, GLuint const* v ), {
    GLctx.vertexAttribI4ui(index, mem.u32_read(v), mem.u32_read(v + 4), mem.u32_read(v + 8), mem.u32_read(v + 12));
})

CWASM_JS_LIB( GL, void, glGetUniformuiv, ( GLuint program, GLint location, GLuint* params ), {
    var data = GLctx.getUniform(GLprograms[program], GLuniforms[location]);
    if (typeof data === "number") mem.u32_write(params, data);
    else for (var i = 0; i < data.length; i++) mem.u32_write(params + 4 * i, data[i]);
})

CWASM_JS_LIB( GL, GLint, glGetFragDataLocation, ( GLuint program, GLchar const* name ), {
    return GLctx.getFragDataLocation(GLprograms[program], mem.str_read(name));
})

CWASM_JS_LIB( GL, void, glUniform1ui, ( GLint location, GLuint v0 ), {
    GLctx.uniform1ui(GLuniforms[location], v0);
})

CWASM_JS_LIB( GL, void, glUniform2ui, ( GLint location, GLuint v0, GLuint v1 ), {
    GLctx.uniform2ui(GLuniforms[location], v0, v1);
})

CWASM_JS_LIB( GL, void, glUniform3ui, ( GLint location, GLuint v0, GLuint v1, GLuint v2 ), {
    GLctx.uniform3ui(GLuniforms[location], v0, v1, v2);
})

CWASM_JS_LIB( GL, void, glUniform4ui, ( GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 ), {
    GLctx.uniform4ui(GLuniforms[location], v0, v1, v2, v3);
})

CWASM_JS_LIB( GL, void, glUniform1uiv, ( GLint location, GLsizei count, GLuint const* value ), {
    GLuniformNuiv(location, count, value, 1, "uniform1uiv");
})

CWASM_JS_LIB( GL, void, glUniform2uiv, ( GLint location, GLsizei count, GLuint const* value ), {
    GLuniformNuiv(location, count, value, 2, "uniform2uiv");
})

CWASM_JS_LIB( GL, void, glUniform3uiv, ( GLint location, GLsizei count, GLuint const* value ), {
    GLuniformNuiv(location, count, value, 3, "uniform3uiv");
})

CWASM_JS_LIB( GL, void, glUniform4uiv, ( GLint location, GLsizei count, GLuint const* value ), {
    GLuniformNuiv(location, count, value, 4, "uniform4uiv");
})

CWASM_JS_LIB( GL, void, glClearBufferiv, ( GLenum buffer, GLint drawbuffer, GLint const* value ), {
    var view = GLscratchI32[3];
    view[0] = mem.i32_read(value); view[1] = mem.i32_read(value + 4); view[2] = mem.i32_read(value + 8); view[3] = mem.i32_read(value + 12);
    GLctx.clearBufferiv(buffer, drawbuffer, view);
})

CWASM_JS_LIB( GL, void, glClearBufferuiv, ( GLenum buffer, GLint drawbuffer, GLuint const* value ), {
    var view = GLscratchI32[3];
    view[0] = mem.u32_read(value); view[1] = mem.u32_read(value + 4); view[2] = mem.u32_read(value + 8); view[3] = mem.u32_read(value + 12);
    GLctx.clearBufferuiv(buffer, drawbuffer, view);
})

CWASM_JS_LIB( GL, void, glClearBufferfv, ( GLenum buffer, GLint drawbuffer, GLfloat const* value ), {
    var view = GLscratchF32[3];
    view[0] = mem.f32_read(value); view[1] = mem.f32_read(value + 4); view[2] = mem.f32_read(value + 8); view[3] = mem.f32_read(value + 12);
    GLctx.clearBufferfv(buffer, drawbuffer, view);
})

CWASM_JS_LIB( GL, void, glClearBufferfi, ( GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil ), {
    GLctx.clearBufferfi(buffer, drawbuffer, depth, stencil);
})

CWASM_JS_LIB( GL, void, glCopyBufferSubData, ( GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size ), {
    GLctx.copyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size);
})

CWASM_JS_LIB( GL, void, glGetUniformIndices, ( GLuint program, GLsizei uniformCount, GLchar const* const* uniformNames, GLuint* uniformIndices ), {
    var names = [];
    for (var i = 0; i < uniformCount; i++)
        names.push(mem.str_read(mem.u32_read(uniformNames + 4 * i)));
    var result = GLctx.getUniformIndices(GLprograms[program], names);
    for (var i = 0; i < result.length; i++)
        mem.u32_write(uniformIndices + 4 * i, result[i]);
})

CWASM_JS_LIB( GL, void, glGetActiveUniformsiv, ( GLuint program, GLsizei uniformCount, GLuint const* uniformIndices, GLenum pname, GLint* params ), {
    var indices = [];
    for (var i = 0; i < uniformCount; i++)
        indices.push(mem.u32_read(uniformIndices + 4 * i));
    var result = GLctx.getActiveUniforms(GLprograms[program], indices, pname);
    for (var i = 0; i < result.length; i++)
        mem.i32_write(params + 4 * i, result[i]);
})

CWASM_JS_LIB( GL, GLuint, glGetUniformBlockIndex, ( GLuint program, GLchar const* uniformBlockName ), {
    return GLctx.getUniformBlockIndex(GLprograms[program], mem.str_read(uniformBlockName));
})

CWASM_JS_LIB( GL, void, glGetActiveUniformBlockiv, ( GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params ), {
    var result = GLctx.getActiveUniformBlockParameter(GLprograms[program], uniformBlockIndex, pname);
    if (result === null) return;
    if (typeof result === "number") mem.i32_write(params, result);
    else for (var i = 0; i < result.length; i++) mem.i32_write(params + 4 * i, result[i]);
})

CWASM_JS_LIB( GL, void, glGetActiveUniformBlockName, ( GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformBlockName ), {
    var name = GLctx.getActiveUniformBlockName(GLprograms[program], uniformBlockIndex);
    if (length) mem.i32_write(length, (bufSize > 0 && uniformBlockName ? mem.str_write(name, uniformBlockName, bufSize) : 0));
})

CWASM_JS_LIB( GL, void, glUniformBlockBinding, ( GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding ), {
    GLctx.uniformBlockBinding(GLprograms[program], uniformBlockIndex, uniformBlockBinding);
})

CWASM_JS_LIB( GL, void, glDrawArraysInstanced, ( GLenum mode, GLint first, GLsizei count, GLsizei instancecount ), {
    GLctx.drawArraysInstanced(mode, first, count, instancecount);
})

CWASM_JS_LIB( GL, void, glDrawElementsInstanced, ( GLenum mode, GLsizei count, GLenum type, void const* indices, GLsizei instancecount ), {
    GLctx.drawElementsInstanced(mode, count, type, indices, instancecount);
})

CWASM_JS_LIB( GL, GLsync, glFenceSync, ( GLenum condition, GLbitfield flags ), {
    var sync = GLctx.fenceSync(condition, flags);
    if (!sync) return 0;
    var id = GLnewId(GLsyncs);
    GLsyncs[id] = sync;
    return id;
})

CWASM_JS_LIB( GL, GLboolean, glIsSync, ( GLsync sync ), {
    var s = GLsyncs[sync];
    return (s ? GLctx.isSync(s) : 0);
})

CWASM_JS_LIB( GL, void, glDeleteSync, ( GLsync sync ), {
    var s = GLsyncs[sync];
    if (!s) return;
    GLctx.deleteSync(s);
    GLsyncs[sync] = null;
})

CWASM_JS_LIB( GL, GLenum, glClientWaitSync, ( GLsync sync, GLbitfield flags, GLuint64 timeout ), {
    var s = GLsyncs[sync];
    if (!s) return 0x911C;
    return GLctx.clientWaitSync(s, flags, timeout);
})

CWASM_JS_LIB( GL, void, glWaitSync, ( GLsync sync, GLbitfield flags, GLuint64 timeout ), {
    var s = GLsyncs[sync];
    if (s) GLctx.waitSync(s, flags, timeout);
})

CWASM_JS_LIB( GL, void, glGetInteger64v, ( GLenum pname, GLint64* data ), {
    var result = GLctx.getParameter(pname);
    if (result === null) return GLsetError(0x500);
    if (typeof result === "number") {
        mem.i32_write(data, result | 0);
        mem.i32_write(data + 4, 0);
    }
    else if (result instanceof Array)
        for (var i = 0; i < result.length; i++) {
            var v = result[i];
            mem.i32_write(data + 8 * i, v | 0);
            mem.i32_write(data + 8 * i + 4, (v / 4294967296) | 0);
        }
})

CWASM_JS_LIB( GL, void, glGetSynciv, ( GLsync sync, GLenum pname, GLsizei count, GLsizei* length, GLint* values ), {
    var s = GLsyncs[sync];
    if (!s) return GLsetError(0x501);
    var result = GLctx.getSyncParameter(s, pname);
    if (length) mem.i32_write(length, 1);
    mem.i32_write(values, result);
})

CWASM_JS_LIB( GL, void, glGetInteger64i_v, ( GLenum target, GLuint index, GLint64* data ), {
    var result = GLctx.getIndexedParameter(target, index);
    if (result === null) return GLsetError(0x500);
    var v = (typeof result === "number" ? result : result[0]);
    mem.i32_write(data, v | 0);
    mem.i32_write(data + 4, (v / 4294967296) | 0);
})

CWASM_JS_LIB( GL, void, glGetBufferParameteri64v, ( GLenum target, GLenum pname, GLint64* params ), {
    var result = GLctx.getBufferParameter(target, pname);
    mem.i32_write(params, result | 0);
    mem.i32_write(params + 4, (result / 4294967296) | 0);
})

CWASM_JS_LIB( GL, void, glGenSamplers, ( GLsizei count, GLuint* samplers ), {
    GLgenObjects(count, samplers, "createSampler", GLsamplers);
})

CWASM_JS_LIB( GL, void, glDeleteSamplers, ( GLsizei count, GLuint const* samplers ), {
    GLdeleteObjects(count, samplers, "deleteSampler", GLsamplers);
})

CWASM_JS_LIB( GL, GLboolean, glIsSampler, ( GLuint sampler ), {
    var s = GLsamplers[sampler];
    return (s ? GLctx.isSampler(s) : 0);
})

CWASM_JS_LIB( GL, void, glBindSampler, ( GLuint unit, GLuint sampler ), {
    GLctx.bindSampler(unit, sampler ? GLsamplers[sampler] : null);
})

CWASM_JS_LIB( GL, void, glSamplerParameteri, ( GLuint sampler, GLenum pname, GLint param ), {
    GLctx.samplerParameteri(GLsamplers[sampler], pname, param);
})

CWASM_JS_LIB( GL, void, glSamplerParameteriv, ( GLuint sampler, GLenum pname, GLint const* param ), {
    GLctx.samplerParameteri(GLsamplers[sampler], pname, mem.i32_read(param));
})

CWASM_JS_LIB( GL, void, glSamplerParameterf, ( GLuint sampler, GLenum pname, GLfloat param ), {
    GLctx.samplerParameterf(GLsamplers[sampler], pname, param);
})

CWASM_JS_LIB( GL, void, glSamplerParameterfv, ( GLuint sampler, GLenum pname, GLfloat const* param ), {
    GLctx.samplerParameterf(GLsamplers[sampler], pname, mem.f32_read(param));
})

CWASM_JS_LIB( GL, void, glGetSamplerParameteriv, ( GLuint sampler, GLenum pname, GLint* params ), {
    mem.i32_write(params, GLctx.getSamplerParameter(GLsamplers[sampler], pname));
})

CWASM_JS_LIB( GL, void, glGetSamplerParameterfv, ( GLuint sampler, GLenum pname, GLfloat* params ), {
    mem.f32_write(params, GLctx.getSamplerParameter(GLsamplers[sampler], pname));
})

CWASM_JS_LIB( GL, void, glVertexAttribDivisor, ( GLuint index, GLuint divisor ), {
    GLctx.vertexAttribDivisor(index, divisor);
})

CWASM_JS_LIB( GL, void, glBindTransformFeedback, ( GLenum target, GLuint id ), {
    GLctx.bindTransformFeedback(target, id ? GLtransformFeedbacks[id] : null);
})

CWASM_JS_LIB( GL, void, glDeleteTransformFeedbacks, ( GLsizei n, GLuint const* ids ), {
    GLdeleteObjects(n, ids, "deleteTransformFeedback", GLtransformFeedbacks);
})

CWASM_JS_LIB( GL, void, glGenTransformFeedbacks, ( GLsizei n, GLuint* ids ), {
    GLgenObjects(n, ids, "createTransformFeedback", GLtransformFeedbacks);
})

CWASM_JS_LIB( GL, GLboolean, glIsTransformFeedback, ( GLuint id ), {
    var tf = GLtransformFeedbacks[id];
    return (tf ? GLctx.isTransformFeedback(tf) : 0);
})

CWASM_JS_LIB( GL, void, glPauseTransformFeedback, ( void ), {
    GLctx.pauseTransformFeedback();
})

CWASM_JS_LIB( GL, void, glResumeTransformFeedback, ( void ), {
    GLctx.resumeTransformFeedback();
})

CWASM_JS_LIB( GL, void, glGetProgramBinary, ( GLuint program, GLsizei bufSize, GLsizei* length, GLenum* binaryFormat, void* binary ), {
    var bin = GLctx.getProgramBinary(GLprograms[program]);
    if (!bin) return GLsetError(0x502);
    if (binaryFormat) mem.u32_write(binaryFormat, bin[0]);
    if (length) mem.i32_write(length, bin[1].byteLength);
    if (binary && bufSize > 0) mem.bytes_write(binary, new Uint8Array(bin[1]));
})

CWASM_JS_LIB( GL, void, glProgramBinary, ( GLuint program, GLenum binaryFormat, void const* binary, GLsizei length ), {
    GLctx.programBinary(GLprograms[program], binaryFormat, mem.raw_view(binary, length));
})

CWASM_JS_LIB( GL, void, glProgramParameteri, ( GLuint program, GLenum pname, GLint value ), {
    GLctx.programParameteri(GLprograms[program], pname, value);
})

CWASM_JS_LIB( GL, void, glInvalidateFramebuffer, ( GLenum target, GLsizei numAttachments, GLenum const* attachments ), {
    var arr = GLargCache[numAttachments];
    if (!arr) arr = GLargCache[numAttachments] = new Array(numAttachments);
    for (var i = 0; i < numAttachments; i++) arr[i] = mem.i32_read(attachments + 4 * i);
    GLctx.invalidateFramebuffer(target, arr);
})

CWASM_JS_LIB( GL, void, glInvalidateSubFramebuffer, ( GLenum target, GLsizei numAttachments, GLenum const* attachments, GLint x, GLint y, GLsizei width, GLsizei height ), {
    var arr = GLargCache[numAttachments];
    if (!arr) arr = GLargCache[numAttachments] = new Array(numAttachments);
    for (var i = 0; i < numAttachments; i++) arr[i] = mem.i32_read(attachments + 4 * i);
    GLctx.invalidateSubFramebuffer(target, arr, x, y, width, height);
})

CWASM_JS_LIB( GL, void, glTexStorage2D, ( GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height ), {
    GLctx.texStorage2D(target, levels, internalformat, width, height);
})

CWASM_JS_LIB( GL, void, glTexStorage3D, ( GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth ), {
    GLctx.texStorage3D(target, levels, internalformat, width, height, depth);
})

CWASM_JS_LIB( GL, void, glGetInternalformativ, ( GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint* params ), {
    var result = GLctx.getInternalformatParameter(target, internalformat, pname);
    if (!result) return GLsetError(0x500);
    for (var i = 0; i < count; i++) mem.i32_write(params + 4 * i, result[i]);
})

#endif

static inline int glEnumerateCanvases( gl_canvas_t* out, int max ) {
    return js_gl_enumerate_canvases( out, max );
}

static inline int glSetupCanvasContext( int antialias, int depth, int stencil, int alpha,
    char const* css_id ) {
    if( !css_id || !css_id[ 0 ] ) { return 0; }
    #ifdef CWASM_THREADS
        js_gl_canvas_op_ready();
        if( !js_gl_acquire_canvas( css_id ) ) { return 0; }
        cwasm_env_async_wait_signal();
        if( !js_gl_take_canvas() ) { return 0; }
    #endif
    #ifdef CWASM_GLES2
        char const* ctx_name = "webgl";
    #else
        char const* ctx_name = "webgl2";
    #endif
    if( !js_glSetupCanvasContext( antialias, depth, stencil, alpha, css_id, ctx_name ) ) {
        return 0;
    }
    cwasm_gl_watch_canvas_size( css_id );
    cwasm_gl_find_extensions();
    return 1;
}

#endif // __CWASM_GL_H__
