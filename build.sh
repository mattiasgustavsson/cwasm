#!/usr/bin/env bash
# ./build.sh sysroot   dist/sysroot, dist/bin/cwasm.js, dist/templates
# ./build.sh native    dist/bin tools, cwasm executable, SDK archive
# ./build.sh           both
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"

LLVM_VERSION=$(cat "$ROOT/llvm-version")
BINARYEN_VERSION=$(cat "$ROOT/binaryen-version")
MINIFY_VERSION=$(cat "$ROOT/minify-version")
[ -n "$LLVM_VERSION" ] || { echo "cannot read llvm-version (LLVM pin file)" >&2; exit 1; }
[ -n "$BINARYEN_VERSION" ] || { echo "cannot read binaryen-version (Binaryen pin file)" >&2; exit 1; }
[ -n "$MINIFY_VERSION" ] || { echo "cannot read minify-version (minify pin file)" >&2; exit 1; }

CWASM_REVISION=$(cat "$ROOT/cwasm-revision" 2>/dev/null || true)
CWASM_VERSION=${LLVM_VERSION}${CWASM_REVISION}

DLTMP=$(mktemp -d)
trap 'rm -rf "$DLTMP"' EXIT
DIST=$ROOT/dist
SYSROOT=$DIST/sysroot
DIST_BIN=$DIST/bin
TOOLS=$ROOT/.tools
VENDOR=$ROOT/vendor
BUILDTMP=$ROOT/buildtmp

case "$(uname -s)" in
    Linux*)                HOST=linux ;;
    Darwin*)               HOST=macos ;;
    MINGW*|MSYS*|CYGWIN*)  HOST=windows ;;
    *) echo "error: unsupported host $(uname -s)" >&2; exit 1 ;;
esac
case "$(uname -m)" in
    x86_64|amd64)   ARCH=x64 ;;
    arm64|aarch64)  ARCH=arm64 ;;
    *) echo "error: unsupported arch $(uname -m)" >&2; exit 1 ;;
esac
EXE=; [ "$HOST" = windows ] && EXE=.exe

LLVM_HOST_VERSION=$LLVM_VERSION
if [ "$HOST-$ARCH" = macos-x64 ]; then
    LLVM_HOST_VERSION=20.1.7
fi

JOBS=$( (nproc || sysctl -n hw.ncpu || echo 4) 2>/dev/null | head -1 )

llvm_asset() {
    case "$HOST-$ARCH" in
        linux-x64)    echo "LLVM-${LLVM_HOST_VERSION}-Linux-X64.tar.xz" ;;
        linux-arm64)  echo "LLVM-${LLVM_HOST_VERSION}-Linux-ARM64.tar.xz" ;;
        macos-arm64)  echo "LLVM-${LLVM_HOST_VERSION}-macOS-ARM64.tar.xz" ;;
        macos-x64)    echo "LLVM-${LLVM_HOST_VERSION}-macOS-X64.tar.xz" ;;
        windows-x64)  echo "clang+llvm-${LLVM_HOST_VERSION}-x86_64-pc-windows-msvc.tar.xz" ;;
        *) echo "error: no LLVM ${LLVM_HOST_VERSION} binary release for $HOST-$ARCH" >&2; exit 1 ;;
    esac
}

binaryen_asset() {
    case "$HOST-$ARCH" in
        linux-x64)    echo "binaryen-version_${BINARYEN_VERSION}-x86_64-linux.tar.gz" ;;
        linux-arm64)  echo "binaryen-version_${BINARYEN_VERSION}-aarch64-linux.tar.gz" ;;
        macos-arm64)  echo "binaryen-version_${BINARYEN_VERSION}-arm64-macos.tar.gz" ;;
        macos-x64)    echo "binaryen-version_${BINARYEN_VERSION}-x86_64-macos.tar.gz" ;;
        windows-x64)  echo "binaryen-version_${BINARYEN_VERSION}-x86_64-windows.tar.gz" ;;
        *) echo "error: no Binaryen ${BINARYEN_VERSION} binary release for $HOST-$ARCH" >&2; exit 1 ;;
    esac
}

minify_asset() {
    case "$HOST-$ARCH" in
        linux-x64)    echo "minify_linux_amd64.tar.gz" ;;
        linux-arm64)  echo "minify_linux_arm64.tar.gz" ;;
        macos-arm64)  echo "minify_darwin_arm64.tar.gz" ;;
        macos-x64)    echo "minify_darwin_amd64.tar.gz" ;;
        windows-x64)  echo "minify_windows_amd64.zip" ;;
        *) echo "error: no minify ${MINIFY_VERSION} binary release for $HOST-$ARCH" >&2; exit 1 ;;
    esac
}

fetch() { # url dest-file
    echo "fetching $1"
    curl -fsSL --retry 3 -o "$2" "$1"
}

install_llvm() {
    local dest=$TOOLS/llvm-$LLVM_HOST_VERSION
    if [ -x "$dest/bin/clang$EXE" ]; then return 0; fi
    local asset root tmp
    asset=$(llvm_asset)
    root=${asset%.tar.xz}
    tmp=$DLTMP/llvm; mkdir -p "$tmp"
    fetch "https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_HOST_VERSION}/${asset}" "$tmp/$asset"
    mkdir -p "$dest/bin" "$dest/lib"
    if [ "$HOST" = windows ]; then
        tar xJf "$tmp/$asset" -C "$tmp" \
            "$root/bin/clang.exe" "$root/bin/clang++.exe" \
            "$root/bin/lld.exe" "$root/bin/wasm-ld.exe" \
            "$root/bin/llvm-ar.exe" "$root/bin/llvm-ranlib.exe" \
            "$root/lib/clang"
    else
        tar xJf "$tmp/$asset" -C "$tmp" \
            "$root/bin/clang" "$root/bin/clang++" "$root/bin/clang-${LLVM_HOST_VERSION%%.*}" \
            "$root/bin/lld" "$root/bin/wasm-ld" \
            "$root/bin/llvm-ar" "$root/bin/llvm-ranlib" \
            "$root/lib/clang"
    fi
    cp -a "$tmp/$root/bin/." "$dest/bin/"
    cp -a "$tmp/$root/lib/clang" "$dest/lib/"
    echo "installed LLVM $LLVM_HOST_VERSION -> $dest"
    "$dest/bin/clang$EXE" --version | head -1
}

install_binaryen() {
    local dest=$TOOLS/binaryen-$BINARYEN_VERSION
    if [ -x "$dest/bin/wasm-opt$EXE" ] && [ -f "$dest/LICENSE" ]; then
        if [ "$HOST" != macos ] || [ -f "$dest/lib/libbinaryen.dylib" ]; then return 0; fi
    fi
    local asset root tmp
    asset=$(binaryen_asset)
    root=binaryen-version_${BINARYEN_VERSION}
    tmp=$DLTMP/binaryen; mkdir -p "$tmp"
    fetch "https://github.com/WebAssembly/binaryen/releases/download/version_${BINARYEN_VERSION}/${asset}" "$tmp/$asset"
    mkdir -p "$dest/bin"
    if [ "$HOST" = macos ]; then
        tar xzf "$tmp/$asset" -C "$tmp" "$root/bin/wasm-opt" "$root/lib/libbinaryen.dylib"
        mkdir -p "$dest/lib"
        install -m 755 "$tmp/$root/lib/libbinaryen.dylib" "$dest/lib/libbinaryen.dylib"
    else
        tar xzf "$tmp/$asset" -C "$tmp" "$root/bin/wasm-opt$EXE"
    fi
    install -m 755 "$tmp/$root/bin/wasm-opt$EXE" "$dest/bin/wasm-opt$EXE"
    fetch "https://raw.githubusercontent.com/WebAssembly/binaryen/version_${BINARYEN_VERSION}/LICENSE" "$tmp/LICENSE"
    [ -s "$tmp/LICENSE" ] || { echo "error: empty Binaryen LICENSE" >&2; exit 1; }
    install -m 644 "$tmp/LICENSE" "$dest/LICENSE"
    echo "installed Binaryen $BINARYEN_VERSION -> $dest"
}

install_minify() {
    local dest=$TOOLS/minify-$MINIFY_VERSION
    if [ -x "$dest/bin/minify$EXE" ] && [ -f "$dest/LICENSE" ]; then return 0; fi
    local asset tmp
    asset=$(minify_asset)
    tmp=$DLTMP/minify; mkdir -p "$tmp"
    fetch "https://github.com/tdewolff/minify/releases/download/v${MINIFY_VERSION}/${asset}" "$tmp/$asset"
    mkdir -p "$dest/bin"
    case "$asset" in
        *.zip) (cd "$tmp" && { unzip -q "$asset" || 7z x -y "$asset" >/dev/null; }) ;;
        *)     tar xzf "$tmp/$asset" -C "$tmp" ;;
    esac
    install -m 755 "$tmp/minify$EXE" "$dest/bin/minify$EXE"
    [ -s "$tmp/LICENSE" ] || { echo "error: no LICENSE in $asset -- cannot redistribute minify" >&2; exit 1; }
    install -m 644 "$tmp/LICENSE" "$dest/LICENSE"
    echo "installed minify $MINIFY_VERSION -> $dest"
}

fetch_llvm_sources() {
    if [ "$(cat "$VENDOR/.llvm-version" 2>/dev/null)" = "$LLVM_VERSION" ]; then return 0; fi
    local archive root tmp
    archive="llvm-project-${LLVM_VERSION}.src.tar.xz"
    root="llvm-project-${LLVM_VERSION}.src"
    tmp=$DLTMP/llvm-src; mkdir -p "$tmp"
    fetch "https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}/${archive}" "$tmp/$archive"
    tar xJf "$tmp/$archive" -C "$tmp" \
        "$root/libcxx/include" "$root/libcxx/src" \
        "$root/libcxxabi/include" "$root/libcxxabi/src" \
        "$root/libunwind/include" "$root/libunwind/src" \
        "$root/libc/shared" "$root/libc/src/__support" "$root/libc/hdr" "$root/libc/include" \
        "$root/compiler-rt/lib/builtins"
    rm -rf "$VENDOR"
    mkdir -p "$VENDOR/llvm-libcxx" "$VENDOR/llvm-libcxxabi" "$VENDOR/llvm-libunwind"
    cp -a "$tmp/$root/libcxx/include"    "$VENDOR/llvm-libcxx/include"
    cp -a "$tmp/$root/libcxx/src"        "$VENDOR/llvm-libcxx/src"
    cp -a "$tmp/$root/libcxxabi/include" "$VENDOR/llvm-libcxxabi/include"
    cp -a "$tmp/$root/libcxxabi/src"     "$VENDOR/llvm-libcxxabi/src"
    cp -a "$tmp/$root/libunwind/include" "$VENDOR/llvm-libunwind/include"
    cp -a "$tmp/$root/libunwind/src"     "$VENDOR/llvm-libunwind/src"
    cp -a "$tmp/$root/compiler-rt/lib/builtins" "$VENDOR/llvm-compiler-rt-builtins"
    mkdir -p "$VENDOR/llvm-libcxx/src/shared" "$VENDOR/llvm-libcxx/src/src"
    cp -a "$tmp/$root/libc/shared/."       "$VENDOR/llvm-libcxx/src/shared/"
    cp -a "$tmp/$root/libc/src/__support"  "$VENDOR/llvm-libcxx/src/src/"
    cp -a "$tmp/$root/libc/hdr"            "$VENDOR/llvm-libcxx/src/"
    mkdir -p "$VENDOR/llvm-libcxx/src/include"
    cp -a "$tmp/$root/libc/include/."      "$VENDOR/llvm-libcxx/src/include/"
    echo "$LLVM_VERSION" > "$VENDOR/.llvm-version"
    echo "installed llvm-project $LLVM_VERSION sources -> $VENDOR"
}

builtins_sources() {
    awk '
        /^set\((GENERIC_SOURCES|GENERIC_TF_SOURCES)$/ { grab = 1; next }
        grab && /^\)/ { grab = 0 }
        grab && /^[ \t]+[A-Za-z0-9_.\/]+\.(c|S)$/ { gsub(/[ \t]/, ""); print }
    ' "$VENDOR/llvm-compiler-rt-builtins/CMakeLists.txt"
}

run_parallel() {
    tr '\n' '\0' | xargs -0 -P "$JOBS" -n 1 sh -c
}

build_sysroot() {
    install_llvm
    fetch_llvm_sources

    local CC=$TOOLS/llvm-$LLVM_HOST_VERSION/bin/clang$EXE
    local CXX=$TOOLS/llvm-$LLVM_HOST_VERSION/bin/clang++$EXE
    local AR=$TOOLS/llvm-$LLVM_HOST_VERSION/bin/llvm-ar$EXE

    mkdir -p "$SYSROOT/include" "$SYSROOT/cxx/include" "$SYSROOT/cxx/threads-config" "$SYSROOT/lib"
    cp -a "$ROOT/libc/include/." "$SYSROOT/include/"
    cp -a "$VENDOR/llvm-libcxx/include/." "$SYSROOT/cxx/include/"
    cp "$ROOT/libcxx/config/__config_site"       "$SYSROOT/cxx/include/__config_site"
    cp "$ROOT/libcxx/config/__assertion_handler" "$SYSROOT/cxx/include/__assertion_handler"
    cp "$ROOT/libcxx/config/__config_site_threads" "$SYSROOT/cxx/threads-config/__config_site"

    local WASMFLAGS_BASE="--target=wasm32 -nostdinc -isystem$SYSROOT/include -msimd128 \
        -mbulk-memory -fno-threadsafe-statics -fno-common -ffunction-sections -fdata-sections -DNDEBUG"
    local CXX_EH_FLAGS="-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false"
    local CXX_NOEH_FLAGS="-fno-exceptions -fno-rtti"
    local SYS_CFLAGS="$WASMFLAGS_BASE -mllvm -enable-emscripten-sjlj -I$ROOT/libc/src -Os -std=gnu11 -fno-builtin -Wall"
    local SJLJ_CFLAGS="$WASMFLAGS_BASE $CXX_EH_FLAGS -mllvm -wasm-enable-sjlj -I$ROOT/libc/src -Os -std=gnu11 -fno-builtin -Wall"
    local THREADS_CFLAGS="$WASMFLAGS_BASE -matomics -DCWASM_THREADS -I$ROOT/libc/src -Os -std=gnu11 -fno-builtin -Wall -DUSE_LOCKS=1 -DUSE_SPIN_LOCKS=1"
    local BUILTINS_CFLAGS="--target=wasm32 -nostdinc -isystem$SYSROOT/include -I$VENDOR/llvm-compiler-rt-builtins \
        -Os -std=c11 -fno-builtin -ffunction-sections -fdata-sections -fno-common -DNDEBUG -Wall"

    local SYS_CXX_BASE="--target=wasm32 -nostdinc -isystem$SYSROOT/cxx/include -isystem$SYSROOT/include \
        -Os -std=c++23 -fno-threadsafe-statics -DNDEBUG -Wall -ffunction-sections -fdata-sections -fno-common \
        -Drestrict=__restrict -I$VENDOR/llvm-libcxxabi/include -I$VENDOR/llvm-libcxx/src -I$VENDOR/llvm-libunwind/include"
    local SYS_CXX_EH="$SYS_CXX_BASE $CXX_EH_FLAGS -mllvm -wasm-enable-sjlj"
    local SYS_CXX_NOEH="$SYS_CXX_BASE $CXX_NOEH_FLAGS -mllvm -enable-emscripten-sjlj"
    local SYS_CXX_THREADS_BASE="--target=wasm32 -nostdinc -isystem$SYSROOT/cxx/threads-config \
        -isystem$SYSROOT/cxx/include -isystem$SYSROOT/include \
        -Os -std=c++23 -DNDEBUG -Wall -ffunction-sections -fdata-sections -fno-common \
        -Drestrict=__restrict -matomics -mbulk-memory -DCWASM_THREADS \
        -I$VENDOR/llvm-libcxxabi/include -I$VENDOR/llvm-libcxx/src -I$VENDOR/llvm-libunwind/include"
    local SYS_CXX_EH_THREADS="$SYS_CXX_THREADS_BASE $CXX_EH_FLAGS -mllvm -wasm-enable-sjlj"
    local SYS_CXX_NOEH_THREADS="$SYS_CXX_THREADS_BASE $CXX_NOEH_FLAGS -mllvm -enable-emscripten-sjlj"

    local LIBCXX_CPPFLAGS="-D_LIBCPP_BUILDING_LIBRARY -D_LIBCPP_HAS_WIDE_CHARACTERS=1"
    local LIBCXX_CPPFLAGS_EH="$LIBCXX_CPPFLAGS -DLIBCXX_BUILDING_LIBCXXABI"
    local LIBCXXABI_CPPFLAGS="-D_LIBCXXABI_BUILDING_LIBRARY -D_LIBCXXABI_HAS_NO_THREADS \
        -D_LIBCXXABI_DISABLE_VISIBILITY_ANNOTATIONS -D_LIBCPP_ENABLE_CXX17_REMOVED_UNEXPECTED_FUNCTIONS"
    local LIBCXXABI_THREADS_CPPFLAGS="-D_LIBCXXABI_BUILDING_LIBRARY \
        -D_LIBCXXABI_DISABLE_VISIBILITY_ANNOTATIONS -D_LIBCPP_ENABLE_CXX17_REMOVED_UNEXPECTED_FUNCTIONS"
    local UNWIND_CFLAGS="--target=wasm32 -nostdinc -isystem$SYSROOT/include -Os -std=c11 \
        -fexceptions $CXX_EH_FLAGS -DNDEBUG -Wall -Wno-c23-extensions \
        -D_LIBUNWIND_HIDE_SYMBOLS -fvisibility=hidden \
        -I$VENDOR/llvm-libunwind/include -I$VENDOR/llvm-libunwind/src \
        -ffunction-sections -fdata-sections -fno-common"

    local LIBC_SRCS="assert.c ctype.c complex.c locale.c string.c stdlib.c dlmalloc.c \
        uchar.c stdio.c cwasm_fs.c cmdline.c time.c math.c \
        wchar.c wprintf.c wctype.c fenv.c inttypes.c atomic.c \
        unistd.c fcntl.c stat.c dirent.c signal.c setjmp_asyncify.c setjmp_wasm_eh.c cwasm_gl.c"
    local LIBC_THREAD_SRCS="$LIBC_SRCS pthread.c threads.c"
    local LIBCXXABI_EH_SRCS="cxa_guard.cpp cxa_virtual.cpp abort_message.cpp \
        cxa_exception.cpp cxa_personality.cpp cxa_exception_storage.cpp \
        cxa_handlers.cpp cxa_default_handlers.cpp cxa_aux_runtime.cpp \
        cxa_vector.cpp stdlib_exception.cpp stdlib_stdexcept.cpp \
        stdlib_typeinfo.cpp private_typeinfo.cpp fallback_malloc.cpp cxa_demangle.cpp"
    local LIBCXXABI_NOEH_SRCS="cxa_guard.cpp cxa_virtual.cpp abort_message.cpp cxa_noexception.cpp"
    local LIBCXXABI_EH_OWN="cxa_atexit.cpp"
    local LIBCXXABI_NOEH_OWN="cxa_atexit.cpp exception_ops.cpp"
    local LIBCXX_IGNORE="thread.cpp mutex.cpp future.cpp shared_mutex.cpp barrier.cpp \
        atomic.cpp condition_variable.cpp condition_variable_destructor.cpp \
        mutex_destructor.cpp random_shuffle.cpp"
    local LIBCXX_IGNORE_NOEH="$LIBCXX_IGNORE exception.cpp typeinfo.cpp"
    local LIBCXX_THREADS_IGNORE="random_shuffle.cpp"
    local LIBCXX_THREADS_IGNORE_NOEH="$LIBCXX_THREADS_IGNORE exception.cpp typeinfo.cpp"
    local LIBCXX_EXTRA_SRCS="ryu/d2fixed.cpp ryu/d2s.cpp ryu/f2s.cpp \
        filesystem/directory_entry.cpp filesystem/directory_iterator.cpp \
        filesystem/filesystem_clock.cpp filesystem/filesystem_error.cpp \
        filesystem/operations.cpp filesystem/path.cpp"

    mkdir -p "$BUILDTMP"

    echo "building cwasmcrt ($JOBS jobs)"
    local cmds=$BUILDTMP/cmds.txt
    : > "$cmds"
    local f o
    mkdir -p "$BUILDTMP/libc" "$BUILDTMP/libc-threads"
    for f in crt1.c $LIBC_SRCS; do
        o=$BUILDTMP/libc/${f%.c}.o
        case "$f" in
            setjmp_wasm_eh.c) echo "$CC $SJLJ_CFLAGS -c $ROOT/libc/src/$f -o $o" ;;
            dlmalloc.c)       echo "$CC $SYS_CFLAGS -Wno-null-pointer-arithmetic -Wno-unused-but-set-variable -Wno-unused-function -c $ROOT/libc/src/$f -o $o" ;;
            *)                echo "$CC $SYS_CFLAGS -c $ROOT/libc/src/$f -o $o" ;;
        esac >> "$cmds"
    done
    for f in crt1.c $LIBC_THREAD_SRCS; do
        o=$BUILDTMP/libc-threads/${f%.c}.o
        case "$f" in
            setjmp_wasm_eh.c) echo "$CC $THREADS_CFLAGS $CXX_EH_FLAGS -mllvm -wasm-enable-sjlj -c $ROOT/libc/src/$f -o $o" ;;
            dlmalloc.c)       echo "$CC $THREADS_CFLAGS -mllvm -enable-emscripten-sjlj -Wno-null-pointer-arithmetic -Wno-unused-but-set-variable -Wno-unused-function -c $ROOT/libc/src/$f -o $o" ;;
            *)                echo "$CC $THREADS_CFLAGS -mllvm -enable-emscripten-sjlj -c $ROOT/libc/src/$f -o $o" ;;
        esac >> "$cmds"
    done
    local builtins_objs=
    for f in $(builtins_sources); do
        o=$BUILDTMP/builtins/$f.o
        mkdir -p "$(dirname "$o")"
        echo "$CC $BUILTINS_CFLAGS -c $VENDOR/llvm-compiler-rt-builtins/$f -o $o" >> "$cmds"
        builtins_objs="$builtins_objs $o"
    done
    run_parallel < "$cmds"

    local libc_objs libc_thread_objs
    libc_objs="$BUILDTMP/libc/crt1.o $(for f in $LIBC_SRCS; do printf '%s ' "$BUILDTMP/libc/${f%.c}.o"; done)"
    libc_thread_objs="$BUILDTMP/libc-threads/crt1.o $(for f in $LIBC_THREAD_SRCS; do printf '%s ' "$BUILDTMP/libc-threads/${f%.c}.o"; done)"
    rm -f "$SYSROOT/lib/cwasmcrt.a" "$SYSROOT/lib/cwasmcrt-threads.a"
    "$AR" rcs "$SYSROOT/lib/cwasmcrt.a" $libc_objs $builtins_objs
    "$AR" rcs "$SYSROOT/lib/cwasmcrt-threads.a" $libc_thread_objs $builtins_objs

    echo "building libc++ quadrants ($JOBS jobs)"
    : > "$cmds"
    local quad flags cppflags abisrcs ownsrcs ignore src stem
    for quad in eh noeh eh-threads noeh-threads; do
        case "$quad" in
            eh)           flags=$SYS_CXX_EH;           cppflags=$LIBCXX_CPPFLAGS_EH; abisrcs=$LIBCXXABI_EH_SRCS;   ownsrcs=$LIBCXXABI_EH_OWN;   ignore=$LIBCXX_IGNORE;              abicpp=$LIBCXXABI_CPPFLAGS ;;
            noeh)         flags=$SYS_CXX_NOEH;         cppflags=$LIBCXX_CPPFLAGS;    abisrcs=$LIBCXXABI_NOEH_SRCS; ownsrcs=$LIBCXXABI_NOEH_OWN; ignore=$LIBCXX_IGNORE_NOEH;         abicpp=$LIBCXXABI_CPPFLAGS ;;
            eh-threads)   flags=$SYS_CXX_EH_THREADS;   cppflags=$LIBCXX_CPPFLAGS_EH; abisrcs=$LIBCXXABI_EH_SRCS;   ownsrcs=$LIBCXXABI_EH_OWN;   ignore=$LIBCXX_THREADS_IGNORE;      abicpp=$LIBCXXABI_THREADS_CPPFLAGS ;;
            noeh-threads) flags=$SYS_CXX_NOEH_THREADS; cppflags=$LIBCXX_CPPFLAGS;    abisrcs=$LIBCXXABI_NOEH_SRCS; ownsrcs=$LIBCXXABI_NOEH_OWN; ignore=$LIBCXX_THREADS_IGNORE_NOEH; abicpp=$LIBCXXABI_THREADS_CPPFLAGS ;;
        esac
        mkdir -p "$BUILDTMP/cxx-$quad/ryu" "$BUILDTMP/cxx-$quad/filesystem" "$BUILDTMP/cxxabi-$quad" "$SYSROOT/cxx/$quad"
        for src in "$VENDOR"/llvm-libcxx/src/*.cpp; do
            stem=$(basename "$src")
            case " $ignore " in *" $stem "*) continue ;; esac
            echo "$CXX -x c++ $flags $cppflags -c $src -o $BUILDTMP/cxx-$quad/${stem%.cpp}.o" >> "$cmds"
        done
        for src in $LIBCXX_EXTRA_SRCS; do
            echo "$CXX -x c++ $flags $cppflags -c $VENDOR/llvm-libcxx/src/$src -o $BUILDTMP/cxx-$quad/${src%.cpp}.o" >> "$cmds"
        done
        for src in $abisrcs; do
            echo "$CXX -x c++ $flags $abicpp -c $VENDOR/llvm-libcxxabi/src/$src -o $BUILDTMP/cxxabi-$quad/${src%.cpp}.o" >> "$cmds"
        done
        for src in $ownsrcs; do
            echo "$CXX -x c++ $flags -c $ROOT/libcxx/src/$src -o $BUILDTMP/cxxabi-$quad/${src%.cpp}.o" >> "$cmds"
        done
    done
    mkdir -p "$BUILDTMP/unwind-eh" "$BUILDTMP/unwind-eh-threads"
    echo "$CC $UNWIND_CFLAGS -c $VENDOR/llvm-libunwind/src/Unwind-wasm.c -o $BUILDTMP/unwind-eh/Unwind-wasm.o" >> "$cmds"
    echo "$CC $UNWIND_CFLAGS -matomics -mbulk-memory -DCWASM_THREADS -c $VENDOR/llvm-libunwind/src/Unwind-wasm.c -o $BUILDTMP/unwind-eh-threads/Unwind-wasm.o" >> "$cmds"
    run_parallel < "$cmds"

    for quad in eh noeh eh-threads noeh-threads; do
        rm -f "$SYSROOT/cxx/$quad/libcxx.a" "$SYSROOT/cxx/$quad/libcxxabi.a"
        "$AR" rcs "$SYSROOT/cxx/$quad/libcxx.a" "$BUILDTMP/cxx-$quad"/*.o "$BUILDTMP/cxx-$quad"/ryu/*.o "$BUILDTMP/cxx-$quad"/filesystem/*.o
        "$AR" rcs "$SYSROOT/cxx/$quad/libcxxabi.a" "$BUILDTMP/cxxabi-$quad"/*.o
    done
    rm -f "$SYSROOT/cxx/eh/libunwind.a" "$SYSROOT/cxx/eh-threads/libunwind.a"
    "$AR" rcs "$SYSROOT/cxx/eh/libunwind.a" "$BUILDTMP/unwind-eh/Unwind-wasm.o"
    "$AR" rcs "$SYSROOT/cxx/eh-threads/libunwind.a" "$BUILDTMP/unwind-eh-threads/Unwind-wasm.o"

    mkdir -p "$DIST_BIN"
    local bundle=$DIST_BIN/cwasm.js prefix dashes
    cat "$ROOT/src/js/cwasm-license-header.txt" > "$bundle"
    for f in cwasm-core.js cwasm-common.js cwasm-single-threaded.js \
             cwasm-multi-threaded.js cwasm-multi-threaded-worker.js \
             cwasm-embed-worker.js cwasm-suspend-asyncify.js cwasm-suspend-jspi.js \
             cwasm-template.html; do
        printf '\n' >> "$bundle"
        prefix="-------- $f "
        dashes=$(printf '%*s' $(( 80 - ${#prefix} )) '' | tr ' ' '-')
        printf '%s%s\n\n' "$prefix" "$dashes" >> "$bundle"
        cat "$ROOT/src/js/$f" >> "$bundle"
    done

    mkdir -p "$DIST/templates"
    cp "$ROOT/wasm-template.html" "$ROOT/wasm-template-debug.html" "$DIST/templates/"
    cp "$ROOT/LICENSE" "$DIST/LICENSE"

    echo "sysroot ready: $SYSROOT"
}

build_native() {
    [ -d "$SYSROOT" ] || {
        echo "error: $SYSROOT missing -- run './build.sh sysroot' first (or unpack the CI sysroot artifact into dist/)" >&2
        exit 1
    }
    install_llvm
    install_binaryen
    install_minify
    mkdir -p "$DIST_BIN"

    echo "building cwasm"
    if [ "$HOST" = macos ] && [ -z "${SDKROOT:-}" ]; then
        SDKROOT=$(xcrun --show-sdk-path); export SDKROOT
    fi

    # The version reaches cwasm.c as a forced include, not -D: quoting a string define
    # through cmd.exe into cl is not portable across Git-bash / MSYS2 / Cygwin.
    mkdir -p "$BUILDTMP"
    local verhdr=$BUILDTMP/cwasm_version.h
    printf '#define CWASM_VERSION "%s"\n' "$CWASM_VERSION" > "$verhdr"

    # A shipped binary must not need a runtime redistributable. Windows: MSVC /MT.
    # Linux: -static. macOS has no static libc, so it stays dynamic.
    local vcvars=
    if [ "$HOST" = windows ]; then
        local vswhere="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
        if [ -x "$vswhere" ]; then
            local vsdir
            vsdir=$("$vswhere" -latest -property installationPath 2>/dev/null | tr -d '\r')
            if [ -n "$vsdir" ]; then
                vcvars="$(cygpath -u "$vsdir")/VC/Auxiliary/Build/vcvars64.bat"
                [ -f "$vcvars" ] || vcvars=
            fi
        fi
    fi

    if [ -n "$vcvars" ]; then
        local bat=$BUILDTMP/build_cwasm.bat
        printf '@echo off\r\ncall "%s" >nul || exit /b 1\r\ncl /nologo /O2 /W3 /MT /D_CRT_SECURE_NO_WARNINGS /FI"%s" /Fo"%s" /Fe"%s" "%s"\r\nexit /b %%errorlevel%%\r\n' \
            "$(cygpath -w "$vcvars")" "$(cygpath -w "$verhdr")" "$(cygpath -w "$BUILDTMP/cwasm.obj")" \
            "$(cygpath -w "$DIST_BIN/cwasm.exe")" "$(cygpath -w "$ROOT/src/cwasm.c")" > "$bat"
        MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL='*' cmd /c "$(cygpath -w "$bat")"
    else
        local HOSTCC=$TOOLS/llvm-$LLVM_HOST_VERSION/bin/clang$EXE
        local rtflags=
        if [ "$HOST" = windows ]; then
            echo "note: no MSVC found -- building cwasm with the pinned clang" >&2
            rtflags=-fms-runtime-lib=static
        elif [ "$HOST" = linux ]; then
            rtflags=-static
        fi
        "$HOSTCC" -O2 -Wall -Wextra $rtflags -include "$verhdr" \
            -o "$DIST_BIN/cwasm$EXE" "$ROOT/src/cwasm.c"
    fi

    local llvm=$TOOLS/llvm-$LLVM_HOST_VERSION
    if [ "$HOST" = windows ]; then
        cp -L "$llvm/bin/clang.exe" "$DIST_BIN/clang.exe"
        cp -L "$llvm/bin/lld.exe"   "$DIST_BIN/wasm-ld.exe"
    else
        cp -L "$llvm/bin/clang-${LLVM_HOST_VERSION%%.*}" "$DIST_BIN/clang"
        cp -L "$llvm/bin/lld"                       "$DIST_BIN/wasm-ld"
    fi
    cp -L "$TOOLS/binaryen-$BINARYEN_VERSION/bin/wasm-opt$EXE" "$DIST_BIN/wasm-opt$EXE"
    cp "$TOOLS/binaryen-$BINARYEN_VERSION/LICENSE" "$DIST_BIN/wasm-opt.LICENSE"
    if [ "$HOST" = macos ]; then
        mkdir -p "$DIST/lib"
        cp -L "$TOOLS/binaryen-$BINARYEN_VERSION/lib/libbinaryen.dylib" "$DIST/lib/libbinaryen.dylib"
    fi
    cp -L "$TOOLS/minify-$MINIFY_VERSION/bin/minify$EXE" "$DIST_BIN/minify$EXE"
    cp "$TOOLS/minify-$MINIFY_VERSION/LICENSE" "$DIST_BIN/minify.LICENSE"
    chmod +x "$DIST_BIN"/*

    cp "$ROOT/LICENSE" "$DIST/LICENSE"

    local pkg=cwasm-$CWASM_VERSION-$HOST-$ARCH
    if [ "$HOST" = windows ]; then
        pkg=$pkg.zip
        "$(cygpath -u "$WINDIR")/System32/tar.exe" --format zip -cf "$(cygpath -w "$ROOT/$pkg")" \
            -C "$(cygpath -w "$DIST")" LICENSE sysroot bin templates
    elif [ "$HOST" = macos ]; then
        pkg=$pkg.tar.gz
        tar czf "$ROOT/$pkg" -C "$DIST" LICENSE sysroot bin lib templates
    else
        pkg=$pkg.tar.gz
        tar czf "$ROOT/$pkg" -C "$DIST" LICENSE sysroot bin templates
    fi
    echo "packaged $pkg"
}

case "${1:-all}" in
    sysroot) build_sysroot ;;
    native)  build_native ;;
    all)     build_sysroot; build_native ;;
    *) echo "usage: $0 [sysroot|native]" >&2; exit 1 ;;
esac
