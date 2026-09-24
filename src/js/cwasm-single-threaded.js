{{{common_js}}}
{{{suspend_js}}}
{{{main_handlers}}}

async function run( payloadB64, imports, moduleOpts ) {
    const EXIT = Symbol();
    CWASM.__exitSymbol = EXIT;
    let exitCode;

    const embedP = startEmbedWorker();
    const env = ( imports.env ??= {} );

    env.__exit = status => {
        exitCode = status | 0;
        CWASM.exitCode = exitCode;
        CWASM.onExit?.( exitCode );
        throw EXIT;
    };
    env.__grow_memory = pages => {
        globalThis.mem.grow( pages | 0 );
        return 0;
    };

    const disc = discoverEnvImports( imports );
    if( disc.needCpp && env.__cpp_exception == null &&
        typeof WebAssembly.Tag === 'function' ) {
        env.__cpp_exception = new WebAssembly.Tag( { parameters: [ 'i32' ] } );
    }
    if( disc.needLj && env.__c_longjmp == null &&
        typeof WebAssembly.Tag === 'function' ) {
        env.__c_longjmp = new WebAssembly.Tag( { parameters: [ 'i32' ] } );
    }

    const useJspi = !!( moduleOpts && moduleOpts.jspi );
    if( useJspi && typeof WebAssembly.Suspending !== 'function' ) {
        throw new Error(
            'JSPI required for C++ EH modules (WebAssembly.Suspending missing)' );
    }

    const wasmStream = () =>
        Base64Stream( payloadB64 ).pipeThrough( new DecompressionStream( 'deflate-raw' ) );

    const host = {
        exports: () => CWASM.asm,
        waitFrame: resume => requestAnimationFrame( resume ),
    };
    const suspend = makeSuspend( host );
    CWASM.runtime.core.run = ( entry, onOk, onErr ) => suspend.run( entry, onOk, onErr );
    CWASM.runtime.core.resumeSignal = () => suspend.resumeSignal();
    globalThis.__async_resume_signal = CWASM.runtime.core.resumeSignal;

    suspend.installWaits( env );
    const sjljTable = { table: null };
    installSjlj( env, disc.sjlj, sjljTable );

    const [ out, embedRaw ] = await Promise.all( [
        WebAssembly.instantiateStreaming(
            new Response( wasmStream(),
                { headers: { 'Content-Type': 'application/wasm' } } ),
            imports
        ),
        embedP,
    ] );

    CWASM.embedRaw = embedRaw;
    const exports = out.instance.exports;
    CWASM.asm = exports;
    sjljTable.table = exports.__indirect_function_table ?? null;

    let memory = exports.memory;
    if( !memory ) {
        for( const name in imports ) {
            const mod = imports[ name ];
            if( mod?.memory instanceof WebAssembly.Memory ) {
                memory = mod.memory;
                break;
            }
        }
    }
    if( !memory ) { throw new Error( 'CWASM: no memory' ); }
    globalThis.MEM = memory;
    globalThis.mem = makeMem( memory );
    fillCmdline();

    globalThis.__cwasm_main_handlers = __cwasm_build_main_handlers();
    globalThis.__cwasm_main_call = ( op, a ) =>
        __cwasm_main_handlers[ op ].apply( null, a );

    if( !exports.__stack_high || !exports.__stack_low ) {
        throw new Error( 'CWASM: missing __stack_high/__stack_low' );
    }

    suspend.afterInstantiate();
    CWASM.onStarted?.();

    if( typeof CWASM.runtime.core.onInstantiated !== 'function' ) {
        throw new Error( 'CWASM: missing runtime.core.onInstantiated (CRT boot init)' );
    }
    await CWASM.runtime.core.onInstantiated( exports, null );
    return exitCode;
}

run( '{{{wasm}}}', {{{imports}}}, {{{module_opts}}} );
