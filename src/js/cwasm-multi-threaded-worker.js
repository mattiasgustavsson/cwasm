let MEM;

let MAIN_HDR = null;
let MAIN_ARGS = null;
let MAIN_ARGS64 = null;
let MAIN_RES = null;
let MAIN_RES64 = null;

function bindMainChannel( sab ) {
    MAIN_HDR = new Int32Array( sab, 0, 8 );
    MAIN_ARGS = new Float64Array( sab, 64, 16 );
    MAIN_ARGS64 = new BigInt64Array( sab, 64, 16 );
    MAIN_RES = new Float64Array( sab, 192, 1 );
    MAIN_RES64 = new BigInt64Array( sab, 192, 1 );
}


function __cwasm_main_call( opid, args ) {
    const h = MAIN_HDR;
    while( Atomics.compareExchange( h, 0, 0, 1 ) !== 0 ) { Atomics.wait( h, 0, 1 ); }
    const n = args.length > 16 ? 16 : args.length;
    let mask = 0;
    for( let i = 0; i < n; ++i ) {
        const v = args[ i ];
        if( typeof v === 'bigint' ) {
            MAIN_ARGS64[ i ] = v;
            mask |= ( 1 << i );
        } else {
            MAIN_ARGS[ i ] = +v;
        }
    }
    Atomics.store( h, 6, mask );
    Atomics.store( h, 4, n );
    Atomics.store( h, 3, opid | 0 );
    Atomics.store( h, 5, 0 );
    Atomics.store( h, 2, 0 );
    Atomics.store( h, 1, 1 );
    self.postMessage( { type: 'main.call' } );
    while( Atomics.load( h, 2 ) === 0 ) { Atomics.wait( h, 2, 0 ); }
    const err = Atomics.load( h, 5 );
    const r = Atomics.load( h, 7 ) ? MAIN_RES64[ 0 ] : MAIN_RES[ 0 ];
    Atomics.store( h, 0, 0 );
    Atomics.notify( h, 0 );
    if( err ) {
        abort( 'CWASM_JS_MAIN', 'op ' + opid + ' threw on page (see page console)' );
    }
    return r;
}

globalThis.__cwasm_main_call = __cwasm_main_call;


const __cwasm_worker_host = {
    exports: () => CWASM.asm,
    waitFrame: resume => {
        const sab = new SharedArrayBuffer( 4 );
        const i = new Int32Array( sab );
        Atomics.store( i, 0, 0 );
        self.postMessage( { type: 'frame.await', sab: sab } );
        if( Atomics.waitAsync ) {
            const w = Atomics.waitAsync( i, 0, 0 );
            w.async ? w.value.then( resume ) : resume();
        } else {
            Atomics.wait( i, 0, 0 );
            resume();
        }
    },
};


function installReapDetached() {
    CWASM.reapDetachedThread = tid => {
        const exp = CWASM.asm;
        if( !exp || !exp.__cwasm_thread_release_detached ) { return; }
        tid = tid | 0;
        if( !tid ) { return; }
        const lockPtr = exp.__cwasm_reap_lock_ptr ? exp.__cwasm_reap_lock_ptr() | 0 : 0;
        const idx = lockPtr >> 2;
        let i32 = null;
        if( lockPtr ) {
            i32 = new Int32Array( globalThis.MEM.buffer );
            while( Atomics.compareExchange( i32, idx, 0, 1 ) !== 0 ) {
                if( i32.buffer !== globalThis.MEM.buffer ) {
                    i32 = new Int32Array( globalThis.MEM.buffer );
                }
                Atomics.wait( i32, idx, 1 );
            }
        }
        try {
            const sp = exp.__stack_pointer;
            const top = exp.__cwasm_reap_stack_top ? exp.__cwasm_reap_stack_top() | 0 : 0;
            let old = 0;
            if( sp && top ) {
                old = sp.value;
                sp.value = top;
            }
            try {
                exp.__cwasm_thread_release_detached( tid );
            } finally {
                if( sp && top ) { sp.value = old; }
            }
        } finally {
            if( i32 ) {
                if( i32.buffer !== globalThis.MEM.buffer ) {
                    i32 = new Int32Array( globalThis.MEM.buffer );
                }
                Atomics.store( i32, idx, 0 );
                Atomics.notify( i32, idx );
            }
        }
    };
}


function finishThreadReturn( exp ) {
    const t = CWASM.threadTid | 0;
    if( t && exp && exp.__cwasm_thread_mark_exited ) { exp.__cwasm_thread_mark_exited( t ); }
    if( CWASM.reapDetachedThread ) { CWASM.reapDetachedThread( t ); }
    self.postMessage( {
        type: 'proc.exit',
        tid: t,
        code: CWASM.exitCode | 0,
        halt: CWASM.__processHalt ? 1 : 0,
    } );
}


self.onmessage = ev => {
    onMsg( ev.data ).catch( e => {
        let tid = ( typeof CWASM !== 'undefined' && CWASM ) ? ( CWASM.threadTid | 0 ) : 0;
        if( !tid && ev.data ) { tid = ev.data.tid | 0; }
        self.postMessage( { type: 'thread.error', tid: tid,
            error: String( e && e.stack || e ) } );
    } );
};


async function onMsg( msg ) {
    if( msg.type === 'thread.init' ) {
        MEM = msg.memory;
        globalThis.MEM = MEM;
        globalThis.mem = makeMem( MEM );
        if( msg.mainChannel ) { bindMainChannel( msg.mainChannel ); }
        globalThis.abort = ( t, m ) => {
            throw new Error( m == null ? String( t ) : t + ': ' + m );
        };
        globalThis.print = m => {
            self.postMessage( { type: 'log', text: String( m ) } );
        };
        self.CWASM = globalThis.CWASM = {
            threadTid: msg.tid | 0,
            print: globalThis.print,
            asm: null,
            cmdline: msg.cmdline,
            envstring: msg.envstring,
            embedRaw: msg.embedRaw || null,
            runtime: { core: {}, thread: {}, transfer: {} },
        };
        CWASM.__setViews = () => { globalThis.mem.refresh(); };
        globalThis.CWASM_EXISTING_MEMORY = MEM;

        const built = __cwasm_build_imports();
        const suspend = makeSuspend( __cwasm_worker_host );
        let exitCode = 0;
        CWASM.runtime.core.run = ( entry, onOk, onErr ) => suspend.run( entry, onOk, onErr );
        CWASM.runtime.core.resumeSignal = () => suspend.resumeSignal();
        globalThis.__async_resume_signal = CWASM.runtime.core.resumeSignal;

        if( msg.useJspi && typeof WebAssembly.Suspending !== 'function' ) {
            throw new Error(
                'JSPI required for C++ EH modules (WebAssembly.Suspending missing)' );
        }

        const disc = discoverEnvImports( msg.module );
        const env = Object.assign( {}, built.env || {} );
        env.memory = MEM;
        if( disc.needCpp ) { env.__cpp_exception = new WebAssembly.Tag( { parameters: [ 'i32' ] } ); }
        if( disc.needLj ) { env.__c_longjmp = new WebAssembly.Tag( { parameters: [ 'i32' ] } ); }
        env.__exit = s => {
            exitCode = s | 0;
            CWASM.exitCode = exitCode;
            CWASM.__processHalt = 1;
            self.postMessage( {
                type: 'proc.exit',
                tid: CWASM.threadTid | 0,
                code: exitCode,
                halt: 1,
            } );
            const park = new Int32Array( new SharedArrayBuffer( 4 ) );
            for( ;; ) { Atomics.wait( park, 0, 0 ); }
        };
        env.__grow_memory = pages => {
            globalThis.mem.grow( pages | 0 );
            self.postMessage( { type: 'memory.grew' } );
            return 0;
        };
        suspend.installWaits( env );
        const sjljTable = { table: null };
        installSjlj( env, disc.sjlj, sjljTable );

        const JS = Object.assign( {}, built.JS || {} );
        Object.keys( JS ).forEach( k => {
            const fn = JS[ k ];
            if( typeof fn === 'function' ) {
                JS[ k ] = function() {
                    globalThis.mem.ensureFresh();
                    return fn.apply( this, arguments );
                };
            }
        } );

        const importObj = Object.assign( {}, built );
        importObj.env = env;
        importObj.JS = JS;
        const inst = await WebAssembly.instantiate( msg.module, importObj );
        const instance = inst.instance || inst;
        const exp = instance.exports;
        CWASM.asm = exp;
        sjljTable.table = exp.__indirect_function_table || null;
        suspend.afterInstantiate();
        installReapDetached();

        if( typeof CWASM.runtime.core.onInstantiated === 'function' ) {
            await CWASM.runtime.core.onInstantiated( exp, msg );
        }
        return;
    }

    if( msg.type === 'transfer.deliver' ) {
        globalThis.CWASM.runtime.transfer.acquired = msg.obj || null;
        if( CWASM.runtime.core.resumeSignal ) { CWASM.runtime.core.resumeSignal(); }
        return;
    }

    ( CWASM.runtime.thread.onThreadMsg && CWASM.runtime.thread.onThreadMsg( msg ) );
}
