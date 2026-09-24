/*
------------------------------------------------------------------------------
		  Licensing information can be found at the end of the file.
------------------------------------------------------------------------------

cwasm.c - build/packaging utility for C wasm programs

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <errno.h>
#include <limits.h>

#include "subprocess.h"

#include <sys/stat.h>
#if !defined( _WIN32 )
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <dirent.h>
#else
    #include <windows.h>
    #include <direct.h>
    #include <io.h>
    #include <process.h>
    #include <time.h>
    #if !defined( PATH_MAX )
        #define PATH_MAX 1024
    #endif
    #if !defined( S_ISDIR )
        #define S_ISDIR( m ) ( ( ( m ) & _S_IFMT ) == _S_IFDIR )
    #endif
    #define lstat stat
    #define rmdir _rmdir
    #define unlink _unlink
    #define access _access
    #define R_OK 4
#endif

#if defined( _WIN32 )
    #define DIR_WINDOWS
#else
    #define DIR_POSIX
#endif
#define DIR_IMPLEMENTATION
#include "dir.h"

#define WHEREAMI_IMPLEMENTATION
#include "whereami.h"

#define ARRAY_IMPLEMENTATION
#include "array.h"

#if defined( __GNUC__ ) || defined( __clang__ )
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-function"
#endif
#define CSTR_IMPLEMENTATION
#include "cstr.h"
#if defined( __GNUC__ ) || defined( __clang__ )
    #pragma GCC diagnostic pop
#endif

#define PATHSTR_CSTR
#define PATHSTR_IMPLEMENTATION
#include "pathstr.h"

typedef char const* cstr_t; // interned, never freed


#if defined( _WIN32 )

    static char* mkdtemp( char* tmpl ) {
        if( !cstr_ends( tmpl, "XXXXXX" ) ) { return NULL; }
        cstr_t stem = cstr_left( tmpl, cstr_len( tmpl ) - 6 );
        unsigned rnd = (unsigned)_getpid() ^ (unsigned)time( NULL );
        for( int attempt = 0; attempt < 4096; ++attempt ) {
            rnd = rnd * 1103515245u + 12345u;
            cstr_t path = cstr_format( "%s%06u", stem, ( rnd >> 8 ) % 1000000u );
            if( _mkdir( path ) == 0 ) { return (char*)path; }
            if( errno != EEXIST ) { return NULL; }
        }
        return NULL;
    }

#endif

#define SDEFL_IMPLEMENTATION
#include "sdefl.h"

static int cwasm_deflate( void* out, void const* in, int n ) {
    struct sdefl* s;
    int nout;

    if( n < 0 ) { return -1; }
    s = (struct sdefl*)malloc( sizeof( *s ) );
    if( !s ) { return -1; }
    memset( s, 0, sizeof( *s ) );
    nout = sdeflate( s, out, in, n, SDEFL_LVL_MAX );
    free( s );
    return nout;
}


static float cwasm_ceilf( float x ) {
    float i = (float)(long long)x;
    if( x > 0.0f && i != x ) { return i + 1.0f; }
    return i;
}


static float cwasm_floorf( float x ) {
    float i = (float)(long long)x;
    if( x < 0.0f && i != x ) { return i - 1.0f; }
    return i;
}

#define STBIR_NO_SIMD
#define STBIR_CEILF( v ) cwasm_ceilf( v )
#define STBIR_FLOORF( v ) cwasm_floorf( v )

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#if defined( __GNUC__ ) || defined( __clang__ )
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-function"
#endif
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#if defined( __GNUC__ ) || defined( __clang__ )
    #pragma GCC diagnostic pop
#endif

#ifndef CWASM_VERSION
    #define CWASM_VERSION "dev"
#endif


static int g_no_minify = 0;
static int g_no_asyncify = 0;
static cstr_t g_scratch = NULL;


typedef struct buf_t {
    uint8_t* p;
    size_t n;
    size_t cap;
    int failed;
} buf_t;


static int buf_init( buf_t* b, size_t cap ) {
    b->p = (uint8_t*)malloc( cap + 1 );
    b->n = 0;
    b->cap = cap;
    b->failed = b->p == NULL;
    if( b->p ) { b->p[ 0 ] = '\0'; }
    return !b->failed;
}


static void buf_free( buf_t* b ) {
    free( b->p );
    memset( b, 0, sizeof( *b ) );
}


static int buf_put( buf_t* b, void const* src, size_t n ) {
    if( b->failed || !b->p || n > b->cap - b->n ) {
        b->failed = 1;
        return 0;
    }
    memcpy( b->p + b->n, src, n );
    b->n += n;
    b->p[ b->n ] = '\0';
    return 1;
}


static int buf_str( buf_t* b, char const* s ) {
    return buf_put( b, s, strlen( s ) );
}


static int buf_u8( buf_t* b, uint8_t v ) {
    return buf_put( b, &v, 1 );
}


// WFS pack and wasm are little-endian
static int buf_u32( buf_t* b, uint32_t v ) {
    uint8_t bytes[ 4 ] = {
        (uint8_t)( v ),
        (uint8_t)( v >> 8 ),
        (uint8_t)( v >> 16 ),
        (uint8_t)( v >> 24 ),
    };
    return buf_put( b, bytes, 4 );
}


static int buf_i32( buf_t* b, int32_t v ) {
    return buf_u32( b, (uint32_t)v );
}


static long file_size( char const* path ) {
    FILE* f = fopen( path, "rb" );
    if( !f ) { return -1; }
    fseek( f, 0, SEEK_END );
    long sz = ftell( f );
    fclose( f );
    return sz;
}


static int load_file_into( char const* path, buf_t* b ) {
    long sz = file_size( path );
    if( b->failed || !b->p || sz < 0 || (size_t)sz > b->cap - b->n ) {
        b->failed = 1;
        return 0;
    }
    FILE* f = fopen( path, "rb" );
    if( !f ) {
        b->failed = 1;
        return 0;
    }
    size_t got = fread( b->p + b->n, 1, (size_t)sz, f );
    fclose( f );
    if( got != (size_t)sz ) {
        b->failed = 1;
        return 0;
    }
    b->n += got;
    b->p[ b->n ] = '\0';
    return 1;
}


static int load_file( char const* path, buf_t* b ) {
    long sz = file_size( path );
    return sz >= 0 && buf_init( b, (size_t)sz ) && load_file_into( path, b );
}


static int save_file( char const* path, void const* data, size_t n ) {
    FILE* f = fopen( path, "wb" );
    if( !f ) { return 0; }
    int ok = ( fwrite( data, 1, n, f ) == n );
    if( fclose( f ) != 0 ) { ok = 0; }
    return ok;
}


static int run_tool( char* const argv[], char* out, size_t out_cap ) {
    int options = subprocess_option_inherit_environment | subprocess_option_no_window |
        subprocess_option_enable_async;
    if( !out ) { options |= subprocess_option_combined_stdout_stderr; }
    struct subprocess_s sp;
    if( subprocess_create( (char const* const*)argv, options, &sp ) != 0 ) { return -1; }

    FILE* child_in = subprocess_stdin( &sp );
    if( child_in ) {
        fclose( child_in );
        sp.stdin_file = NULL;
    }

    size_t out_n = 0;
    char buf[ 4096 ];
    for( ;; ) {
        unsigned n = subprocess_read_stdout( &sp, buf, sizeof( buf ) );
        if( n == 0 ) { break; }
        if( !out ) {
            fwrite( buf, 1, n, stderr );
        } else if( out_n + 1 < out_cap ) {
            size_t take = n < out_cap - 1 - out_n ? n : out_cap - 1 - out_n;
            memcpy( out + out_n, buf, take );
            out_n += take;
        }
    }
    if( out ) {
        out[ out_n ] = '\0';
        for( ;; ) {
            unsigned n = subprocess_read_stderr( &sp, buf, sizeof( buf ) );
            if( n == 0 ) { break; }
            fwrite( buf, 1, n, stderr );
        }
    }

    int code = -1;
    if( subprocess_join( &sp, &code ) != 0 ) { code = -1; }
    subprocess_destroy( &sp );
    return code;
}


static cstr_t self_dir( void ) {
    char path[ PATH_MAX ];
    int dir_len = 0;
    int len = wai_getExecutablePath( path, (int)sizeof( path ) - 1, &dir_len );
    if( len <= 0 || len >= (int)sizeof( path ) - 1 || dir_len <= 0 ) { return NULL; }
    path[ dir_len ] = '\0';
    return cstr_path_to_posix( path );
}


static cstr_t tool_path_next_to_self( char const* name ) {
    cstr_t dir = self_dir();
    if( !dir ) { return NULL; }
    #if defined( _WIN32 )
        return cstr_format( "%s/%s%s", dir, name, cstr_find( name, ".", 0 ) >= 0 ? "" : ".exe" );
    #else
        return cstr_format( "%s/%s", dir, name );
    #endif
}


static cstr_t find_tool_next_to_self( char const* name ) {
    #if defined( _WIN32 )
        int mode = R_OK;
    #else
        int mode = X_OK;
    #endif
    cstr_t path = tool_path_next_to_self( name );
    return path && access( path, mode ) == 0 ? path : NULL;
}


static int minify_file( char const* type, char const* src, size_t src_n, buf_t* out ) {
    cstr_t minify_path = g_no_minify ? NULL : find_tool_next_to_self( "minify" );
    if( !minify_path ) {
        static int warned = 0;
        if( !g_no_minify && !warned ) {
            warned = 1;
            fprintf( stderr,
                "warning: no sibling minify -- output is not minified.\n"
                "         Pass --no-minify if this is intended.\n" );
        }
        return buf_init( out, src_n ) && buf_put( out, src, src_n );
    }

    cstr_t in_path = cstr_format( "%s/minify-in.%s", g_scratch, type );
    cstr_t out_path = cstr_format( "%s/minify-out.%s", g_scratch, type );
    char* argv[] = {
        (char*)minify_path,
        (char*)"-q",
        (char*)cstr_format( "--type=%s", type ),
        (char*)"-o",
        (char*)out_path,
        (char*)in_path,
        NULL
    };
    if( !save_file( in_path, src, src_n ) ) { return 0; }
    if( run_tool( argv, NULL, 0 ) != 0 ) {
        return buf_init( out, src_n ) && buf_put( out, src, src_n );
    }
    return load_file( out_path, out );
}


#include "wasm_mod.h"
#include "jsgen.h"
#include "pack.h"


static int ext_is( char const* p, char const* ext ) {
    return cstr_compare_nocase( cstr_path_extname( p ), ext ) == 0;
}


static int is_js_path( char const* p ) {
    return ext_is( p, ".js" );
}


static int is_html_path( char const* p ) {
    return ext_is( p, ".html" ) || ext_is( p, ".htm" );
}


static int is_wasm_path( char const* p ) {
    return ext_is( p, ".wasm" );
}


static int is_cxx_source( char const* p ) {
    return cstr_is_equal( cstr_path_extname( p ), ".C" ) || ext_is( p, ".cc" ) ||
        ext_is( p, ".cpp" ) || ext_is( p, ".cxx" ) || ext_is( p, ".c++" );
}


static int is_source_path( char const* p ) {
    // exact .c only, .C is C++
    return cstr_is_equal( cstr_path_extname( p ), ".c" ) || is_cxx_source( p );
}


// accepted only with --cc -x c / -x c++
static int is_header_path( char const* p ) {
    return ext_is( p, ".h" ) || ext_is( p, ".hh" ) || ext_is( p, ".hpp" ) || ext_is( p, ".hxx" ) ||
        ext_is( p, ".h++" );
}


static int is_compile_input_path( char const* p ) {
    return is_source_path( p ) || is_header_path( p );
}


static void rm_rf( char const* path ) {
    struct stat st;
    if( lstat( path, &st ) != 0 ) { return; }
    if( S_ISDIR( st.st_mode ) ) {
        dir_t* d = dir_open( path );
        if( d ) {
            for( ;; ) {
                dir_entry_t* de = dir_read( d );
                if( !de ) { break; }
                char const* name = dir_name( de );
                if( !name || cstr_is_equal( name, "." ) || cstr_is_equal( name, ".." ) ) {
                    continue;
                }
                rm_rf( cstr_format( "%s/%s", path, name ) );
            }
            dir_close( d );
        }
        rmdir( path );
    } else {
        unlink( path );
    }
}


static int scratch_create( void ) {
    #if defined( _WIN32 )
        char const* tmp = getenv( "TEMP" );
        if( !tmp ) { tmp = getenv( "TMP" ); }
        if( !tmp ) { tmp = "."; }
    #else
        char const* tmp = getenv( "TMPDIR" );
        if( !tmp ) { tmp = "/tmp"; }
    #endif
    char tmpl[ PATH_MAX ];
    snprintf( tmpl, sizeof( tmpl ), "%s/cwasm-XXXXXX", tmp );
    char* dir = mkdtemp( tmpl );
    if( !dir ) {
        fprintf( stderr, "error: mkdtemp failed\n" );
        return 0;
    }
    g_scratch = cstr( dir );
    return 1;
}


static void scratch_cleanup( void ) {
    if( g_scratch ) {
        rm_rf( g_scratch );
        g_scratch = NULL;
    }
}


// version: first digits/dots token, leading 'v' allowed, else the whole line
static void print_tool_version( char const* tool ) {
    cstr_t path = find_tool_next_to_self( tool );
    if( !path ) { return; }
    char* argv[] = { (char*)path, (char*)"--version", NULL };
    char out[ 4096 ];
    if( run_tool( argv, out, sizeof( out ) ) != 0 ) { return; }
    cstr_tokenizer_t lines = cstr_tokenizer( out );
    cstr_t line = cstr_tokenize( &lines, "\r\n" );
    if( !line ) { line = cstr( "" ); }

    char const* best = NULL;
    cstr_tokenizer_t toks = cstr_tokenizer( line );
    for( cstr_t tok = cstr_tokenize( &toks, " \t" ); tok; tok = cstr_tokenize( &toks, " \t" ) ) {
        char const* d = tok + ( tok[ 0 ] == 'v' || tok[ 0 ] == 'V' ? 1 : 0 );
        if( *d < '0' || *d > '9' ) { continue; }
        int only = 1;
        for( char const* q = d; *q; ++q ) {
            if( !( ( *q >= '0' && *q <= '9' ) || *q == '.' ) ) {
                only = 0;
                break;
            }
        }
        if( only ) {
            best = d;
            break;
        }
    }
    printf( "%s %s\n", tool, best ? best : line );
}


typedef struct flag_desc_t {
    char const* names; // "|"-separated aliases
    char const* value; // usage placeholder, "" when none
    char const* help;
} flag_desc_t;


static flag_desc_t const cwasm_flag_table[] = {
    { "--help", "", "Show this help" },
    { "--version", "", "Print cwasm version, then sibling clang version" },
    { "-o|--output", "PATH", "Output .js or .html (repeatable; default .html)" },
    { "--no-minify", "", "Skip sibling minify" },
    { "--no-asyncify", "", "JSPI suspends; do not run wasm-opt (needs a modern browser)" },
    { "--node", "", "Node-oriented JS runner (default .js; no .html)" },
    { "--template", "F", "HTML template file (default: cwasm-template.html slice)" },
    { "--cc", "...", "Extra compiler args (until next cwasm flag or --)" },
    { "--ld", "...", "Extra linker args / .o/.a (same gobble rules)" },
    { "--threads", "", "Threaded CRT + link flags" },
    { "--exceptions", "", "EH/RTTI libs + JSPI wasm-opt path" },
    { "--sysroot=PATH", "", "Override default sysroot (../sysroot from bin/)" },
    { "--default-argv", "STR", "Default argv when URL has no query" },
    { "--title", "STR", "HTML title" },
    { "--icon", "PATH", "Favicon / iOS icons" },
    { "--embed", "name path", "Named WFS embed entry (repeatable)" },
    { "--embed-stdin", "path", "Embed a single blob as wasm stdin" },
    { "--files", "dir", "Write CWASM_FILES next to HTML content dir" },
};


static void usage( void ) {
    fprintf( stderr,
        "usage: cwasm [sources...|.wasm] [-o out.js|-o out.html] [flags]\n"
        "\n"
        "Inputs: one or more sources (.c/.cpp/...) OR exactly one .wasm\n"
        "Outputs: -o/--output PATH (.js and/or .html; default: <basename>.html)\n"
        "\n"
        "Flags:\n" );
    for( size_t i = 0; i < sizeof( cwasm_flag_table ) / sizeof( *cwasm_flag_table ); ++i ) {
        flag_desc_t const* f = &cwasm_flag_table[ i ];
        cstr_t names = cstr_replace( cstr( f->names ), "|", ", " );
        cstr_t left = f->value[ 0 ] ? cstr_format( "%s %s", names, f->value ) : names;
        fprintf( stderr, "  %-20s  %s\n", left, f->help );
    }
}


static int is_known_cwasm_flag( char const* a ) {
    if( cstr_is_equal( a, "--" ) || cstr_starts( a, "--sysroot=" ) ) { return 1; }
    for( size_t i = 0; i < sizeof( cwasm_flag_table ) / sizeof( *cwasm_flag_table ); ++i ) {
        cstr_tokenizer_t names = cstr_tokenizer( cwasm_flag_table[ i ].names );
        for( cstr_t n = cstr_tokenize( &names, "|" ); n; n = cstr_tokenize( &names, "|" ) ) {
            if( cstr_is_equal( a, n ) ) { return 1; }
        }
    }
    return 0;
}


static void gobble_args( int argc, char** argv, int* i, array_param_t(char*)* outp ) {
    array_t(char*)* out;
    array_from_param( out, outp );
    ++( *i );
    while( *i < argc ) {
        char const* a = argv[ *i ];
        if( cstr_is_equal( a, "--" ) ) {
            ++( *i );
            break;
        }
        if( is_known_cwasm_flag( a ) ) { break; }
        if( is_compile_input_path( a ) || is_wasm_path( a ) || is_js_path( a ) ||
            is_html_path( a ) ) {
            break;
        }
        array_add( out, (char*)cstr( a ) );
        ++( *i );
    }
}


// last -x / -xLANG wins over the file suffix
static int cc_lang_override( array_param_t(char*)* extra_ccp, int* out_is_cxx ) {
    array_t(char*)* extra_cc;
    array_from_param( extra_cc, extra_ccp );
    int found = 0;
    for( int i = 0; i < (int)extra_cc->count; ++i ) {
        char const* a = extra_cc->items[ i ];
        char const* lang = NULL;
        if( cstr_starts( a, "-x" ) && a[ 2 ] != '\0' ) {
            lang = a + 2;
        } else if( cstr_is_equal( a, "-x" ) && i + 1 < (int)extra_cc->count ) {
            lang = extra_cc->items[ ++i ];
        } else {
            continue;
        }
        int is_cxx = cstr_find( lang, "c++", 0 ) >= 0;
        int is_c = !is_cxx && ( cstr_is_equal( lang, "c" ) || cstr_is_equal( lang, "c-header" ) ||
            cstr_is_equal( lang, "cpp-output" ) || cstr_starts( lang, "objective-c" ) ||
            cstr_is_equal( lang, "assembler-with-cpp" ) );
        if( !is_cxx && !is_c ) { continue; }
        *out_is_cxx = is_cxx;
        found = 1;
    }
    return found;
}


#define PUSH( s ) array_add( av, (char*)( s ) )


static int compile_source( char const* clang_path, char const* sysroot, char const* src,
    char const* obj_out, int want_threads, int want_exceptions, int is_cxx,
    array_param_t(char*)* extra_ccp ) {

    array_t(char*)* extra_cc;
    array_from_param( extra_cc, extra_ccp );
    array_t(char*)* av;
    array_create( av );
    int ok = 0;

    PUSH( clang_path );
    PUSH( "--target=wasm32" );
    PUSH( "-nostdinc" );

    // include order: threads-config, cxx/include, include
    if( is_cxx ) {
        PUSH( "-nostdinc++" );
        if( want_threads ) { PUSH( cstr_format( "-isystem%s/cxx/threads-config", sysroot ) ); }
        PUSH( cstr_format( "-isystem%s/cxx/include", sysroot ) );
    }
    PUSH( cstr_format( "-isystem%s/include", sysroot ) );

    PUSH( "-msimd128" );
    if( !( is_cxx && want_threads ) ) { PUSH( "-fno-threadsafe-statics" ); }
    PUSH( "-fno-common" );
    PUSH( "-ffunction-sections" );
    PUSH( "-fdata-sections" );
    PUSH( "-O2" );
    PUSH( is_cxx ? "-std=c++17" : "-std=c11" );
    PUSH( "-Wall" );
    PUSH( "-Wextra" );
    if( is_cxx ) { PUSH( "-Drestrict=__restrict" ); }

    if( want_exceptions ) {
        PUSH( "-fwasm-exceptions" );
        PUSH( "-mllvm" );
        PUSH( "-wasm-use-legacy-eh=false" );
        PUSH( "-mllvm" );
        PUSH( "-wasm-enable-sjlj" );
    } else {
        if( is_cxx ) {
            PUSH( "-fno-exceptions" );
            PUSH( "-fno-rtti" );
        }
        PUSH( "-mllvm" );
        // JSPI cannot suspend across the invoke_* JS frame of Emscripten sjlj
        PUSH( g_no_asyncify ? "-wasm-enable-sjlj" : "-enable-emscripten-sjlj" );
    }

    PUSH( "-mbulk-memory" );

    if( want_threads ) {
        PUSH( "-matomics" );
        PUSH( "-DCWASM_THREADS" );
    } else {
        // a header definition is too late for the standard's #ifndef probe
        PUSH( "-D__STDC_NO_THREADS__=1" );
    }

    for( int i = 0; i < (int)extra_cc->count; ++i ) {
        char const* a = extra_cc->items[ i ];
        if( cstr_starts( a, "-std=" ) ) {
            char const* s = a + 5;
            int cxx_std = cstr_starts( s, "c++" ) || cstr_starts( s, "gnu++" );
            if( cxx_std != is_cxx ) { continue; }
        }
        if( cstr_starts( a, "-x" ) && a[ 2 ] != '\0' ) { continue; }
        if( cstr_is_equal( a, "-x" ) ) {
            if( i + 1 < (int)extra_cc->count ) { ++i; }
            continue;
        }
        PUSH( a );
    }
    PUSH( "-x" );
    PUSH( is_cxx ? "c++" : "c" );

    PUSH( "-c" );
    PUSH( src );
    PUSH( "-o" );
    PUSH( obj_out );
    PUSH( NULL );

    ok = run_tool( av->items, NULL, 0 ) == 0;
    array_destroy( av );
    return ok;
}


static int link_wasm( char const* lld_path, char const* sysroot, char** objs, int nobjs,
    int have_cxx, int want_threads, int want_exceptions, array_param_t(char*)* extra_ldp,
    char const* wasm_out ) {

    array_t(char*)* extra_ld;
    array_from_param( extra_ld, extra_ldp );
    array_t(char*)* av;
    array_create( av );

    PUSH( lld_path );
    PUSH( "-strip-all" );
    PUSH( "-gc-sections" );
    PUSH( "-allow-undefined" );
    // wasm-ld default is 64KiB; single frames reach ~35KiB
    PUSH( "-z" );
    PUSH( "stack-size=1048576" );
    PUSH( "--export=_start" );
    PUSH( "--export=__original_main" );
    PUSH( "--export=__wasm_call_ctors" );
    PUSH( "--export=malloc" );
    PUSH( "--export=free" );
    PUSH( "--export=realloc" );
    PUSH( "--export=__stack_high" );
    PUSH( "--export=__stack_low" );
    PUSH( "--export=__stack_pointer" );
    PUSH( "--export-table" );

    if( want_threads ) {
        PUSH( "--shared-memory" );
        PUSH( "--import-memory" );
        PUSH( "--export-memory" );
        PUSH( "--max-memory=1073741824" );
        PUSH( "--export=__pthread_trampoline" );
        PUSH( "--export=__cwasm_thread_init" );
        PUSH( "--export=__cwasm_thread_mark_exited" );
        PUSH( "--export=__cwasm_thread_release_detached" );
        PUSH( "--export=__cwasm_reap_stack_top" );
        PUSH( "--export=__cwasm_reap_lock_ptr" );
        PUSH( "--export=__cwasm_host_futex_ptr" );
        PUSH( "--export=__wasm_init_tls" );
        PUSH( "--export=__tls_size" );
        PUSH( "--export=__tls_align" );
    }

    for( int i = 0; i < nobjs; ++i ) { PUSH( objs[ i ] ); }

    if( have_cxx ) {
        char const* quad;
        int with_unwind = 0;
        if( want_threads && want_exceptions ) {
            quad = "eh-threads";
            with_unwind = 1;
        } else if( want_threads ) {
            quad = "noeh-threads";
        } else if( want_exceptions ) {
            quad = "eh";
            with_unwind = 1;
        } else {
            quad = "noeh";
        }

        PUSH( cstr_format( "%s/cxx/%s/libcxx.a", sysroot, quad ) );
        PUSH( cstr_format( "%s/cxx/%s/libcxxabi.a", sysroot, quad ) );
        if( with_unwind ) { PUSH( cstr_format( "%s/cxx/%s/libunwind.a", sysroot, quad ) ); }
    }

    PUSH( "--whole-archive" );
    PUSH( cstr_format( "%s/lib/%s", sysroot,
        want_threads ? "cwasmcrt-threads.a" : "cwasmcrt.a" ) );
    PUSH( "--no-whole-archive" );

    for( int i = 0; i < (int)extra_ld->count; ++i ) { PUSH( extra_ld->items[ i ] ); }

    PUSH( "-o" );
    PUSH( wasm_out );
    PUSH( NULL );

    int ec = run_tool( av->items, NULL, 0 );
    array_destroy( av );
    return ec == 0;
}


// in_path and out_path may be the same file
static int opt_wasm( char const* opt_path, char const* in_path, char const* out_path,
    int want_threads, int want_exceptions ) {

    array_t(char*)* av;
    array_create( av );

    #define PUSH_FEATURES() do { \
            PUSH( opt_path ); \
            PUSH( "--enable-sign-ext" ); \
            PUSH( "--enable-nontrapping-float-to-int" ); \
            PUSH( "--enable-bulk-memory" ); \
            PUSH( "--enable-simd" ); \
            if( want_threads ) { \
                PUSH( "--enable-threads" ); \
            } \
        } while( 0 )

    PUSH_FEATURES();

    if( want_exceptions ) {
        PUSH( "--enable-exception-handling" );
        PUSH( "--enable-reference-types" );
        PUSH( "--enable-multivalue" );
        PUSH( "-O1" );
        PUSH( in_path );
        PUSH( "-o" );
        PUSH( out_path );
        PUSH( NULL );
        int ec = run_tool( av->items, NULL, 0 );
        array_destroy( av );
        return ec == 0;
    }

    PUSH( "--asyncify" );
    PUSH( "--pass-arg=asyncify-imports@env.__async_wait_signal,env.__async_wait_ms,"
        "env.__async_wait_frame" );
    PUSH( in_path );
    PUSH( "-o" );
    PUSH( out_path );
    PUSH( NULL );
    int ec = run_tool( av->items, NULL, 0 );
    array_destroy( av );
    if( ec != 0 ) { return 0; }

    array_create( av );
    PUSH_FEATURES();
    PUSH( "-O1" );
    PUSH( out_path );
    PUSH( "-o" );
    PUSH( out_path );
    PUSH( NULL );
    ec = run_tool( av->items, NULL, 0 );
    array_destroy( av );
    return ec == 0;
    #undef PUSH_FEATURES
}

#undef PUSH


// returns the module path in the scratch dir, NULL on failure
static cstr_t build_from_sources( char** sources, int nsources, int want_threads,
    int want_exceptions, char const* sysroot, array_param_t(char*)* extra_ccp,
    array_param_t(char*)* extra_ldp ) {

    array_t(char*)* objs;
    array_create( objs );

    #define CLEANUP_AND_RETURN( PATH ) (\
            array_destroy( objs ), \
            (PATH) )

    cstr_t clang_path = find_tool_next_to_self( "clang" );
    if( !clang_path ) {
        fprintf( stderr, "error: missing sibling clang\n" );
        return CLEANUP_AND_RETURN( NULL );
    }
    cstr_t lld_path = find_tool_next_to_self( "wasm-ld" );
    if( !lld_path ) {
        fprintf( stderr, "error: missing sibling wasm-ld\n" );
        return CLEANUP_AND_RETURN( NULL );
    }
    cstr_t opt_path = find_tool_next_to_self( "wasm-opt" );
    if( !opt_path && !g_no_asyncify ) {
        fprintf( stderr,
            "warning: no sibling wasm-opt -- falling back to a --no-asyncify build.\n"
            "         Suspends use JSPI, so output requires a JSPI-capable browser, and the\n"
            "         module is unoptimised (no -O1). Pass --no-asyncify if this is what you\n"
            "         want; cwasm then never looks for wasm-opt.\n" );
        g_no_asyncify = 1;
    }

    int have_cxx = 0;
    int lang_ov = 0;
    int have_lang_ov = cc_lang_override( extra_ccp, &lang_ov );

    for( int i = 0; i < nsources; ++i ) {
        if( is_header_path( sources[ i ] ) ) {
            if( !have_lang_ov ) {
                fprintf( stderr,
                    "error: header input requires --cc -x c or --cc -x c++: %s\n",
                    sources[ i ] );
                return CLEANUP_AND_RETURN( NULL );
            }
        } else if( !is_source_path( sources[ i ] ) ) {
            fprintf( stderr, "error: unsupported source: %s\n", sources[ i ] );
            return CLEANUP_AND_RETURN( NULL );
        }
        int cxx = have_lang_ov ? lang_ov : is_cxx_source( sources[ i ] );
        if( cxx ) { have_cxx = 1; }
        char* obj = (char*)cstr_format( "%s/%d.o", g_scratch, i );
        array_add( objs, obj );
        if( !compile_source( clang_path, sysroot, sources[ i ], obj, want_threads,
            want_exceptions, cxx, extra_ccp ) ) {
            fprintf( stderr, "error: compile failed: %s\n", sources[ i ] );
            return CLEANUP_AND_RETURN( NULL );
        }
    }

    cstr_t out_wasm = cstr_format( "%s/a.wasm", g_scratch );

    if( !link_wasm( lld_path, sysroot, objs->items, (int)objs->count, have_cxx, want_threads,
        want_exceptions, extra_ldp, out_wasm ) ) {
        fprintf( stderr, "error: link failed\n" );
        return CLEANUP_AND_RETURN( NULL );
    }

    if( !g_no_asyncify && !opt_wasm( opt_path, out_wasm, out_wasm, want_threads,
        want_exceptions ) ) {
        fprintf( stderr, "error: wasm-opt failed\n" );
        return CLEANUP_AND_RETURN( NULL );
    }

    return CLEANUP_AND_RETURN( out_wasm );
    #undef CLEANUP_AND_RETURN
}


static int package_wasm( char const* path_wasm, char const* out_js, char const* out_html,
    bundle_t const* bundle, char const* template_path, char const* embed_stdin_path,
    array_param_t(wfs_embed_t)* embedsp, char const* default_argv, char const* title_arg,
    char const* icon_path, char const* files_dir, int jspi ) {

    array_t(wfs_embed_t)* embeds;
    array_from_param( embeds, embedsp );
    array_t(wfunc_t)* wfuncs;
    array_t(wfunc_t)* mfuncs;
    array_t(import_rec_t)* imports;
    array_create( wfuncs );
    array_create( mfuncs );
    array_create( imports );
    buf_t wasm = { 0 };
    buf_t wasm2 = { 0 };
    buf_t cmp = { 0 };
    buf_t b64 = { 0 };
    buf_t embed = { 0 };
    buf_t embed_b64 = { 0 };
    buf_t fill = { 0 };
    buf_t js = { 0 };
    buf_t tpl = { 0 };
    buf_t html = { 0 };
    buf_t min = { 0 };

    #define CLEANUP_AND_RETURN( STATUS ) (\
            buf_free( &wasm ), \
            buf_free( &wasm2 ), \
            buf_free( &cmp ), \
            buf_free( &b64 ), \
            buf_free( &embed ), \
            buf_free( &embed_b64 ), \
            buf_free( &fill ), \
            buf_free( &js ), \
            buf_free( &tpl ), \
            buf_free( &html ), \
            buf_free( &min ), \
            array_destroy( mfuncs ), \
            array_destroy( wfuncs ), \
            array_destroy( imports ), \
            (STATUS) )

    size_t core_n = 0;
    char const* core = bundle_slice( bundle, "cwasm-core.js", &core_n );
    if( !core || !load_file( path_wasm, &wasm ) ) {
        fprintf( stderr, "error: failed to load cwasm-core.js slice or wasm\n" );
        return CLEANUP_AND_RETURN( 0 );
    }

    size_t tpl_n = 0;
    char const* tpl_p = NULL;
    if( out_html ) {
        if( template_path ) {
            if( load_file( template_path, &tpl ) ) {
                tpl_p = (char const*)tpl.p;
                tpl_n = tpl.n;
            }
        } else {
            tpl_p = bundle_slice( bundle, "cwasm-template.html", &tpl_n );
        }
        if( !tpl_p ) {
            fprintf( stderr, "error: failed to load HTML template\n" );
            return CLEANUP_AND_RETURN( 0 );
        }
    }

    if( !buf_init( &wasm2, wasm.n ) || !rewrite_wasm_and_collect( wasm.p, wasm.n, &wasm2,
        array_to_param( wfuncs ), array_to_param( mfuncs ), array_to_param( imports ) ) ) {
        fprintf( stderr, "error: wasm rewrite/import parse failed\n" );
        return CLEANUP_AND_RETURN( 0 );
    }
    buf_free( &wasm );
    if( !deflate_to_buf( wasm2.p, wasm2.n, &cmp ) ) {
        fprintf( stderr, "error: deflate failed\n" );
        return CLEANUP_AND_RETURN( 0 );
    }
    buf_free( &wasm2 );
    if( !b64_encode( cmp.p, cmp.n, &b64 ) ) {
        fprintf( stderr, "error: out of memory\n" );
        return CLEANUP_AND_RETURN( 0 );
    }
    buf_free( &cmp );

    cstr_t imports_factory = NULL;
    cstr_t imports_expr = gen_imports_expr( array_to_param( wfuncs ), array_to_param( mfuncs ),
        array_to_param( imports ), &imports_factory );
    if( !imports_expr ) {
        fprintf( stderr, "error: out of memory\n" );
        return CLEANUP_AND_RETURN( 0 );
    }
    cstr_t main_handlers = gen_main_handlers( array_to_param( mfuncs ) );
    if( !main_handlers ) {
        main_handlers = cstr( "function __cwasm_build_main_handlers(){return [];}" );
    }
    cstr_t module_opts = gen_module_opts( array_to_param( imports ), jspi );
    char const* dav = default_argv ? default_argv : "";

    int have_embed = embed_stdin_path || embeds->count > 0;
    if( embed_stdin_path && !load_file( embed_stdin_path, &embed ) ) {
        fprintf( stderr, "error: embed-stdin pack failed\n" );
        return CLEANUP_AND_RETURN( 0 );
    }
    if( !embed_stdin_path && embeds->count > 0 && !wfs_pack_build( embedsp, &embed ) ) {
        fprintf( stderr, "error: wfs pack failed\n" );
        return CLEANUP_AND_RETURN( 0 );
    }
    if( have_embed ) {
        if( !deflate_to_buf( embed.p, embed.n, &cmp ) ||
            !b64_encode( cmp.p, cmp.n, &embed_b64 ) ) {
            fprintf( stderr, "error: embed deflate failed\n" );
            return CLEANUP_AND_RETURN( 0 );
        }
        buf_free( &embed );
        buf_free( &cmp );
    }
    char const* embed_ins = embed_b64.p ? (char const*)embed_b64.p : "";

    int threading_marker = find_sub( core, core_n, "{{{threading_js}}}" ) != NULL;
    int threads = threading_marker && imports_declare_threads( array_to_param( imports ) );
    if( threads ) {
        if( !minify_file( "js", imports_factory, cstr_size( imports_factory ), &min ) ||
            !buf_init( &fill, 4 * min.n + 2 ) ||
            !buf_js_string( &fill, (char const*)min.p, min.n ) ) {
            fprintf( stderr, "error: out of memory\n" );
            return CLEANUP_AND_RETURN( 0 );
        }
        buf_free( &min );
    }
    char const* imports_ins = threads ? (char const*)fill.p : imports_expr;
    size_t imports_ins_n = threads ? fill.n : cstr_size( imports_expr );

    size_t slices_n = 0;
    for( int i = 0; i < (int)bundle->v->count; ++i ) {
        slices_n += 4 * bundle->v->items[ i ].n + 2;
    }
    // bound: every slice at most twice, escaped
    size_t js_bound = core_n + 2 * slices_n + b64.n + 4 * embed_b64.n + imports_ins_n +
        cstr_size( module_opts ) + cstr_size( main_handlers ) + cstr_size( dav ) + 64;
    if( !buf_init( &js, js_bound ) || !buf_put( &js, core, core_n ) ) {
        fprintf( stderr, "error: out of memory\n" );
        return CLEANUP_AND_RETURN( 0 );
    }

    if( threading_marker ) {
        size_t thr_n = 0;
        size_t sus_n = 0;
        size_t common_n = 0;
        size_t emb_n = 0;
        size_t wrk_n = 0;
        char const* thr = bundle_slice( bundle,
            threads ? "cwasm-multi-threaded.js" : "cwasm-single-threaded.js", &thr_n );
        char const* sus = bundle_slice( bundle,
            jspi ? "cwasm-suspend-jspi.js" : "cwasm-suspend-asyncify.js", &sus_n );
        char const* common = bundle_slice( bundle, "cwasm-common.js", &common_n );
        char const* emb = bundle_slice( bundle, "cwasm-embed-worker.js", &emb_n );
        char const* wrk = threads
            ? bundle_slice( bundle, "cwasm-multi-threaded-worker.js", &wrk_n ) : NULL;
        if( !thr || !sus || !common || !emb || ( threads && !wrk ) ) {
            fprintf( stderr, "error: missing cwasm piece in cwasm.js bundle\n" );
            return CLEANUP_AND_RETURN( 0 );
        }
        // worker builds take common and suspend as quoted literals
        if( !splice( &js, "{{{threading_js}}}", thr, thr_n, SPLICE_RAW ) ||
            !splice_slice( &js, "{{{common_js}}}", common, common_n, threads ) ||
            !splice_slice( &js, "{{{suspend_js}}}", sus, sus_n, threads ) ||
            !splice_slice( &js, "{{{multithread_worker_js}}}", wrk, wrk_n, 1 ) ||
            !splice_slice( &js, "{{{embed_worker_js}}}", emb, emb_n, 1 ) ) {
            return CLEANUP_AND_RETURN( 0 );
        }
    }

    if( !splice( &js, "{{{wasm}}}", (char const*)b64.p, b64.n, SPLICE_RAW ) ||
        !splice( &js, "{{{imports}}}", imports_ins, imports_ins_n, SPLICE_RAW ) ||
        !splice( &js, "{{{module_opts}}}", module_opts, cstr_size( module_opts ), SPLICE_RAW ) ||
        !splice( &js, "{{{main_handlers}}}", main_handlers, cstr_size( main_handlers ),
            SPLICE_RAW ) ) {
        return CLEANUP_AND_RETURN( 0 );
    }
    buf_free( &b64 );
    buf_free( &fill );

    if( find_sub( (char const*)js.p, js.n, "{{{common_js_raw}}}" ) ) {
        size_t craw_n = 0;
        char const* craw = bundle_slice( bundle, "cwasm-common.js", &craw_n );
        if( !craw ) {
            fprintf( stderr, "error: missing cwasm-common.js in bundle\n" );
            return CLEANUP_AND_RETURN( 0 );
        }
        if( !splice( &js, "{{{common_js_raw}}}", craw, craw_n, SPLICE_RAW ) ) {
            return CLEANUP_AND_RETURN( 0 );
        }
    }

    if( !find_sub( (char const*)js.p, js.n, "{{{embed}}}" ) ) {
        fprintf( stderr, "error: missing marker {{{embed}}}\n" );
        return CLEANUP_AND_RETURN( 0 );
    }
    if( !find_sub( (char const*)js.p, js.n, "{{{default_argv}}}" ) ) {
        fprintf( stderr, "error: missing marker {{{default_argv}}}\n" );
        return CLEANUP_AND_RETURN( 0 );
    }
    // {{{embed}}} and {{{default_argv}}} sit inside escaped literals
    if( !splice( &js, "{{{embed}}}", embed_ins, embed_b64.n, SPLICE_BODY ) ||
        !splice( &js, "{{{default_argv}}}", dav, cstr_size( dav ), SPLICE_BODY ) ) {
        return CLEANUP_AND_RETURN( 0 );
    }
    buf_free( &embed_b64 );

    if( out_js ) {
        int wrote = minify_file( "js", (char const*)js.p, js.n, &min ) &&
            save_file( out_js, min.p, min.n );
        buf_free( &min );
        if( !wrote ) {
            fprintf( stderr, "error: write failed: %s\n", out_js );
            return CLEANUP_AND_RETURN( 0 );
        }
    }

    if( out_html ) {
        if( !html_build( tpl_p, tpl_n, &js, out_html, title_arg, icon_path, &html ) ) {
            fprintf( stderr, "error: html metadata failed\n" );
            return CLEANUP_AND_RETURN( 0 );
        }
        buf_free( &js );
        int wrote = minify_file( "html", (char const*)html.p, html.n, &min ) &&
            save_file( out_html, min.p, min.n );
        buf_free( &min );
        if( !wrote ) {
            fprintf( stderr, "error: write failed: %s\n", out_html );
            return CLEANUP_AND_RETURN( 0 );
        }
        if( files_dir && !generate_cwasm_files( files_dir ) ) {
            fprintf( stderr, "error: CWASM_FILES generation failed for %s\n", files_dir );
            return CLEANUP_AND_RETURN( 0 );
        }
    }

    return CLEANUP_AND_RETURN( 1 );
    #undef CLEANUP_AND_RETURN
}


typedef enum embed_mode_t {
    EMBED_NONE,
    EMBED_FILES,
    EMBED_STDIN,
} embed_mode_t;


typedef struct value_flag_t {
    char const* flag;
    char const** value;
} value_flag_t;


int main( int argc, char** argv ) {
    array_t(char*)* sources;
    array_t(char*)* extra_cc;
    array_t(char*)* extra_ld;
    array_t(wfs_embed_t)* embeds;
    bundle_t bundle = { 0 };
    array_create( sources );
    array_create( extra_cc );
    array_create( extra_ld );
    array_create( embeds );

    #define CLEANUP_AND_RETURN( STATUS ) (\
            bundle_free( &bundle ), \
            array_destroy( extra_cc ), \
            array_destroy( extra_ld ), \
            array_destroy( sources ), \
            array_destroy( embeds ), \
            scratch_cleanup(), \
            (STATUS) )
    char const* wasm_in = NULL;
    char const* out_js = NULL;
    char const* out_html = NULL;
    char const* out_wasm = NULL;
    char const* template_path = NULL;
    char const* sysroot_arg = NULL;
    char const* embed_stdin_path = NULL;
    char const* default_argv = NULL;
    char const* title_arg = NULL;
    char const* icon_path = NULL;
    char const* files_dir = NULL;
    int want_threads = 0;
    int want_exceptions = 0;
    int node_mode = 0;
    embed_mode_t embed_mode = EMBED_NONE;
    value_flag_t value_flags[] = {
        { "--template", &template_path },
        { "--default-argv", &default_argv },
        { "--title", &title_arg },
        { "--icon", &icon_path },
        { "--files", &files_dir },
    };

    for( int i = 1; i < argc; ) {
        char const* a = argv[ i ];
        if( cstr_is_equal( a, "--help" ) ) {
            usage();
            return CLEANUP_AND_RETURN( EXIT_SUCCESS );
        }
        if( cstr_is_equal( a, "--version" ) ) {
            printf( "cwasm %s\n", CWASM_VERSION );
            fflush( stdout );
            cstr_t clang_path = find_tool_next_to_self( "clang" );
            if( !clang_path ) {
                fprintf( stderr, "error: missing sibling clang\n" );
                return CLEANUP_AND_RETURN( EXIT_FAILURE );
            }
            // --version prints the host triple, -dumpversion the bare version
            char* cargv[] = { (char*)clang_path, (char*)"-dumpversion", NULL };
            char ver[ 256 ];
            if( run_tool( cargv, ver, sizeof( ver ) ) != 0 ) {
                fprintf( stderr, "error: sibling clang -dumpversion failed\n" );
                return CLEANUP_AND_RETURN( EXIT_FAILURE );
            }
            cstr_tokenizer_t lines = cstr_tokenizer( ver );
            cstr_t line = cstr_tokenize( &lines, "\r\n" );
            printf( "clang %s\n", line ? line : "" );
            print_tool_version( "wasm-opt" );
            print_tool_version( "minify" );
            return CLEANUP_AND_RETURN( EXIT_SUCCESS );
        }
        if( cstr_is_equal( a, "-o" ) || cstr_is_equal( a, "--output" ) ) {
            if( i + 1 >= argc ) {
                fprintf( stderr, "error: %s needs a path\n", a );
                return CLEANUP_AND_RETURN( EXIT_FAILURE );
            }
            char const* p = argv[ i + 1 ];
            if( is_js_path( p ) ) {
                if( out_js ) {
                    fprintf( stderr, "error: multiple .js outputs\n" );
                    return CLEANUP_AND_RETURN( EXIT_FAILURE );
                }
                out_js = p;
            } else if( is_html_path( p ) ) {
                if( out_html ) {
                    fprintf( stderr, "error: multiple .html outputs\n" );
                    return CLEANUP_AND_RETURN( EXIT_FAILURE );
                }
                out_html = p;
            } else if( is_wasm_path( p ) ) {
                if( out_wasm ) {
                    fprintf( stderr, "error: multiple .wasm outputs\n" );
                    return CLEANUP_AND_RETURN( EXIT_FAILURE );
                }
                out_wasm = p;
            } else {
                fprintf( stderr, "error: output must end in .js, .html or .wasm: %s\n", p );
                return CLEANUP_AND_RETURN( EXIT_FAILURE );
            }
            i += 2;
            continue;
        }
        if( cstr_is_equal( a, "--no-minify" ) ) {
            g_no_minify = 1;
            ++i;
            continue;
        }
        if( cstr_is_equal( a, "--no-asyncify" ) ) {
            g_no_asyncify = 1;
            ++i;
            continue;
        }
        if( cstr_is_equal( a, "--node" ) ) {
            node_mode = 1;
            ++i;
            continue;
        }
        if( cstr_is_equal( a, "--threads" ) ) {
            want_threads = 1;
            ++i;
            continue;
        }
        if( cstr_is_equal( a, "--exceptions" ) ) {
            want_exceptions = 1;
            ++i;
            continue;
        }
        if( cstr_starts( a, "--sysroot=" ) ) {
            sysroot_arg = a + 10;
            if( !sysroot_arg[ 0 ] ) {
                fprintf( stderr, "error: empty --sysroot=\n" );
                return CLEANUP_AND_RETURN( EXIT_FAILURE );
            }
            ++i;
            continue;
        }
        if( cstr_is_equal( a, "--sysroot" ) ) {
            fprintf( stderr, "error: use --sysroot=PATH (equals form only)\n" );
            return CLEANUP_AND_RETURN( EXIT_FAILURE );
        }
        if( cstr_is_equal( a, "--cc" ) ) {
            gobble_args( argc, argv, &i, array_to_param( extra_cc ) );
            continue;
        }
        if( cstr_is_equal( a, "--ld" ) ) {
            gobble_args( argc, argv, &i, array_to_param( extra_ld ) );
            continue;
        }
        value_flag_t* vf = NULL;
        for( size_t k = 0; k < sizeof( value_flags ) / sizeof( *value_flags ); ++k ) {
            if( cstr_is_equal( a, value_flags[ k ].flag ) ) { vf = &value_flags[ k ]; }
        }
        if( vf ) {
            if( i + 1 >= argc ) {
                fprintf( stderr, "error: %s needs a value\n", a );
                return CLEANUP_AND_RETURN( EXIT_FAILURE );
            }
            *vf->value = argv[ i + 1 ];
            i += 2;
            continue;
        }
        if( cstr_is_equal( a, "--embed" ) ) {
            if( embed_mode == EMBED_STDIN ) {
                fprintf( stderr, "error: cannot mix --embed and --embed-stdin\n" );
                return CLEANUP_AND_RETURN( EXIT_FAILURE );
            }
            if( i + 2 >= argc ) {
                fprintf( stderr, "error: --embed needs name and path\n" );
                return CLEANUP_AND_RETURN( EXIT_FAILURE );
            }
            embed_mode = EMBED_FILES;
            wfs_embed_push( array_to_param( embeds ), argv[ i + 1 ], argv[ i + 2 ] );
            i += 3;
            continue;
        }
        if( cstr_is_equal( a, "--embed-stdin" ) ) {
            if( embed_mode == EMBED_FILES ) {
                fprintf( stderr, "error: cannot mix --embed and --embed-stdin\n" );
                return CLEANUP_AND_RETURN( EXIT_FAILURE );
            }
            if( embed_mode == EMBED_STDIN ) {
                fprintf( stderr, "error: only one --embed-stdin allowed\n" );
                return CLEANUP_AND_RETURN( EXIT_FAILURE );
            }
            if( i + 1 >= argc ) {
                fprintf( stderr, "error: --embed-stdin needs a path\n" );
                return CLEANUP_AND_RETURN( EXIT_FAILURE );
            }
            embed_mode = EMBED_STDIN;
            embed_stdin_path = argv[ i + 1 ];
            i += 2;
            continue;
        }
        if( cstr_is_equal( a, "--" ) ) {
            fprintf( stderr, "error: bare -- outside --cc/--ld\n" );
            return CLEANUP_AND_RETURN( EXIT_FAILURE );
        }
        if( a[ 0 ] == '-' ) {
            fprintf( stderr, "error: unknown flag: %s\n", a );
            return CLEANUP_AND_RETURN( EXIT_FAILURE );
        }
        if( is_js_path( a ) || is_html_path( a ) ) {
            fprintf( stderr, "error: bare .js/.html not allowed as input: %s\n", a );
            return CLEANUP_AND_RETURN( EXIT_FAILURE );
        }
        if( is_wasm_path( a ) ) {
            if( wasm_in ) {
                fprintf( stderr, "error: multiple .wasm inputs\n" );
                return CLEANUP_AND_RETURN( EXIT_FAILURE );
            }
            wasm_in = a;
            ++i;
            continue;
        }
        if( is_compile_input_path( a ) ) {
            array_add( sources, (char*)a );
            ++i;
            continue;
        }
        fprintf( stderr, "error: unrecognized input path: %s\n", a );
        return CLEANUP_AND_RETURN( EXIT_FAILURE );
    }

    if( sources->count > 0 && wasm_in ) {
        fprintf( stderr, "error: cannot mix sources and .wasm input\n" );
        return CLEANUP_AND_RETURN( EXIT_FAILURE );
    }
    if( sources->count == 0 && !wasm_in ) {
        fprintf( stderr, "error: need sources or a .wasm input\n" );
        usage();
        return CLEANUP_AND_RETURN( EXIT_FAILURE );
    }

    if( !out_js && !out_html && !out_wasm ) {
        char const* first = sources->count > 0 ? sources->items[ 0 ] : wasm_in;
        cstr_t out = cstr_path_replace_extension( cstr_path_basename( first ),
            node_mode ? ".js" : ".html" );
        if( node_mode ) {
            out_js = out;
        } else {
            out_html = out;
        }
    }

    if( node_mode && !out_js ) {
        fprintf( stderr, "error: --node requires a .js output\n" );
        return CLEANUP_AND_RETURN( EXIT_FAILURE );
    }
    if( node_mode && out_html ) {
        fprintf( stderr, "error: --node cannot be used with .html output\n" );
        return CLEANUP_AND_RETURN( EXIT_FAILURE );
    }
    if( files_dir && !out_html ) {
        fprintf( stderr, "error: --files requires .html output\n" );
        return CLEANUP_AND_RETURN( EXIT_FAILURE );
    }
    if( ( title_arg || icon_path ) && !out_html ) {
        fprintf( stderr, "error: --title/--icon require .html output\n" );
        return CLEANUP_AND_RETURN( EXIT_FAILURE );
    }
    if( template_path && !out_html ) {
        fprintf( stderr, "error: --template requires .html output\n" );
        return CLEANUP_AND_RETURN( EXIT_FAILURE );
    }
    if( embed_mode == EMBED_FILES && files_dir ) {
        fprintf( stderr, "error: cannot mix --embed and --files\n" );
        return CLEANUP_AND_RETURN( EXIT_FAILURE );
    }
    if( ( want_threads || want_exceptions ) && sources->count == 0 ) {
        fprintf( stderr, "error: --threads/--exceptions require source inputs "
            "(not package-only .wasm)\n" );
        return CLEANUP_AND_RETURN( EXIT_FAILURE );
    }
    if( out_wasm ) {
        if( embed_mode != EMBED_NONE || default_argv ) {
            fprintf( stderr,
                "error: --embed/--embed-stdin/--default-argv require .js or .html output\n" );
            return CLEANUP_AND_RETURN( EXIT_FAILURE );
        }
        if( sources->count == 0 ) {
            fprintf( stderr, "error: .wasm output requires source inputs\n" );
            return CLEANUP_AND_RETURN( EXIT_FAILURE );
        }
        if( out_js || out_html ) {
            fprintf( stderr,
                "error: .wasm output cannot be combined with .js/.html output\n" );
            return CLEANUP_AND_RETURN( EXIT_FAILURE );
        }
    }

    char const* sysroot = sysroot_arg;
    if( !sysroot ) {
        cstr_t dir = self_dir();
        if( !dir ) {
            fprintf( stderr, "error: cannot resolve self directory\n" );
            return CLEANUP_AND_RETURN( EXIT_FAILURE );
        }
        sysroot = cstr_format( "%s/../sysroot", dir );
    }
    if( !scratch_create() ) { return CLEANUP_AND_RETURN( EXIT_FAILURE ); }

    char const* path_wasm = wasm_in;
    if( sources->count > 0 ) {
        path_wasm = build_from_sources( sources->items, (int)sources->count, want_threads,
            want_exceptions, sysroot, array_to_param( extra_cc ), array_to_param( extra_ld ) );
        if( !path_wasm ) { return CLEANUP_AND_RETURN( EXIT_FAILURE ); }
    }

    if( out_wasm ) {
        buf_t module = { 0 };
        int wrote = load_file( path_wasm, &module ) && save_file( out_wasm, module.p, module.n );
        buf_free( &module );
        if( !wrote ) {
            fprintf( stderr, "error: cannot write %s\n", out_wasm );
            return CLEANUP_AND_RETURN( EXIT_FAILURE );
        }
        return CLEANUP_AND_RETURN( EXIT_SUCCESS );
    }

    int jspi = 0;
    if( sources->count > 0 ) {
        // build_from_sources may set g_no_asyncify
        jspi = want_exceptions || g_no_asyncify;
    } else {
        buf_t module = { 0 };
        cwasm_facts_t f;
        int probed = load_file( path_wasm, &module ) && probe_wasm( module.p, module.n, &f );
        buf_free( &module );
        if( !probed ) {
            fprintf( stderr, "error: cannot parse %s as a wasm module\n", path_wasm );
            return CLEANUP_AND_RETURN( EXIT_FAILURE );
        }

        cstr_t opt_path = find_tool_next_to_self( "wasm-opt" );
        int have_opt = opt_path != NULL;

        char const* err = NULL;
        if( f.asyncified && f.cpp_eh ) {
            err = "module has both C++ exception handling and asyncify applied; asyncify\n"
                "       breaks wasm exceptions, so this module cannot work";
        } else if( f.cpp_eh && f.em_sjlj ) {
            err = "module has both C++ exception handling and Emscripten sjlj (invoke_*).\n"
                "       Exceptions force JSPI, and JSPI cannot suspend across the JS frame\n"
                "       invoke_* introduces, so there is no runtime that can host it";
        } else if( f.asyncified && g_no_asyncify ) {
            err = "--no-asyncify given, but the module already has asyncify applied";
        } else if( f.em_sjlj && !f.asyncified && ( g_no_asyncify || !have_opt ) ) {
            err = g_no_asyncify
                ? "module uses Emscripten sjlj (invoke_*), which requires asyncify: JSPI\n"
                  "       cannot suspend across the JS frame invoke_* introduces.\n"
                  "       --no-asyncify cannot be honoured for this module"
                : "module uses Emscripten sjlj (invoke_*), which requires asyncify, but\n"
                  "       there is no sibling wasm-opt to apply it. JSPI cannot suspend\n"
                  "       across the JS frame invoke_* introduces";
        }
        if( err ) {
            fprintf( stderr, "error: %s\n", err );
            return CLEANUP_AND_RETURN( EXIT_FAILURE );
        }

        if( f.asyncified ) {
            jspi = 0;
        } else if( f.cpp_eh ) {
            jspi = 1;
        } else if( g_no_asyncify ) {
            jspi = 1;
        } else if( !have_opt ) {
            fprintf( stderr,
                "warning: no sibling wasm-opt -- packaging as a --no-asyncify build.\n"
                "         Suspends use JSPI, so output requires a JSPI-capable browser.\n"
                "         Pass --no-asyncify if this is what you want; cwasm then never\n"
                "         looks for it.\n" );
            jspi = 1;
        } else {
            cstr_t pkg_wasm = cstr_format( "%s/pkg.wasm", g_scratch );
            if( !opt_wasm( opt_path, path_wasm, pkg_wasm, f.threads, 0 ) ) {
                fprintf( stderr, "error: failed to apply asyncify to %s\n", path_wasm );
                return CLEANUP_AND_RETURN( EXIT_FAILURE );
            }
            path_wasm = pkg_wasm;
            jspi = 0;
        }
    }

    cstr_t bundle_path = tool_path_next_to_self( "cwasm.js" );
    if( !bundle_path || access( bundle_path, R_OK ) != 0 ) {
        fprintf( stderr, "error: missing sibling cwasm.js\n" );
        return CLEANUP_AND_RETURN( EXIT_FAILURE );
    }
    if( !bundle_load( &bundle, bundle_path ) ) {
        fprintf( stderr, "error: failed to parse cwasm.js bundle\n" );
        return CLEANUP_AND_RETURN( EXIT_FAILURE );
    }

    int ok = package_wasm( path_wasm, out_js, out_html, &bundle, template_path,
        embed_stdin_path, array_to_param( embeds ), default_argv, title_arg, icon_path,
        files_dir, jspi );
    return CLEANUP_AND_RETURN( ok ? EXIT_SUCCESS : EXIT_FAILURE );
    #undef CLEANUP_AND_RETURN
}

/*
------------------------------------------------------------------------------

This software is available under 2 licenses - you may choose the one you like.

------------------------------------------------------------------------------

ALTERNATIVE A - MIT License

Copyright (c) 2026 Mattias Gustavsson

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

------------------------------------------------------------------------------

ALTERNATIVE B - Public Domain (www.unlicense.org)

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

------------------------------------------------------------------------------
*/
