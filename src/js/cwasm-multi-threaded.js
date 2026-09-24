{{{common_js_raw}}}
{{{main_handlers}}}

function installMainBridge() {
    const channel = new SharedArrayBuffer( 256 );
    const hdr = new Int32Array( channel, 0, 8 );
    const argsF = new Float64Array( channel, 64, 16 );
    const argsI = new BigInt64Array( channel, 64, 16 );
    const resF = new Float64Array( channel, 192, 1 );
    const resI = new BigInt64Array( channel, 192, 1 );
    const H = __cwasm_build_main_handlers();
    const serve = () => {
        if( Atomics.load( hdr, 1 ) !== 1 ) { return; }
        Atomics.store( hdr, 1, 0 );
        const opid = Atomics.load( hdr, 3 ) | 0;
        const n = Atomics.load( hdr, 4 ) | 0;
        const mask = Atomics.load( hdr, 6 ) | 0;
        const args = new Array( n );
        for( let i = 0; i < n; ++i ) {
            args[ i ] = mask & ( 1 << i ) ? argsI[ i ] : argsF[ i ];
        }
        globalThis.mem.ensureFresh();
        let r = 0;
        let err = 0;
        try {
            r = H[ opid ].apply( null, args );
        } catch( e ) {
            console.error( 'cwasm JSMAIN op', opid, e );
            err = 1;
        }
        if( typeof r === 'bigint' ) {
            resI[ 0 ] = r;
            Atomics.store( hdr, 7, 1 );
        } else {
            resF[ 0 ] = err ? 0 : +r;
            Atomics.store( hdr, 7, 0 );
        }
        Atomics.store( hdr, 5, err );
        Atomics.store( hdr, 2, 1 );
        Atomics.notify( hdr, 2 );
    };
    return { channel, serve };
}


function startThreads( module, memory, meta, bridge ) {
    const useJspi = !!meta.useJspi;
    const workers = new Map();
    const freePool = [];
    const allWorkers = new Set();
    const workerURL = URL.createObjectURL( new Blob(
        [ {{{imports}}} + {{{common_js}}} + {{{suspend_js}}} + {{{multithread_worker_js}}} ],
        { type: 'application/javascript' } ) );
    const signalSpawnFail = addr => {
        addr = addr | 0;
        if( !addr || !globalThis.MEM ) { return; }
        const i = new Int32Array( globalThis.MEM.buffer );
        Atomics.store( i, addr >> 2, 2 );
        Atomics.notify( i, addr >> 2 );
    };
    const killWorker = w => {
        w._cwasmDead = true;
        allWorkers.delete( w );
        w.terminate();
    };
    const forgetWorker = worker => {
        for( const [ t, w ] of workers ) {
            if( w === worker ) {
                workers.delete( t );
                break;
            }
        }
    };
    CWASM.runtime.thread.dispose = () => {
        URL.revokeObjectURL( workerURL );
        workers.clear();
        freePool.length = 0;
        for( const w of Array.from( allWorkers ) ) { killWorker( w ); }
        allWorkers.clear();
    };
    if( typeof window !== 'undefined' ) {
        window.addEventListener( 'pagehide', CWASM.runtime.thread.dispose );
    }

    const bindWorker = ( worker, slot ) => {
        worker.onerror = e => {
            console.error( 'thread', slot.tid, e.error || e );
            if( slot.futexAddr ) { signalSpawnFail( slot.futexAddr ); }
        };
        worker.onmessage = ev => {
            const m = ev.data;
            if( m.type === 'thread.started' ) {
                CWASM.onStarted?.();
                return;
            }
            if( m.type === 'log' ) {
                CWASM.print( m.text );
                return;
            }
            if( m.type === 'thread.error' ) {
                console.error( 'thread', m.tid, m.error );
                if( slot.futexAddr ) { signalSpawnFail( slot.futexAddr ); }
                if( ( m.tid | 0 ) === 0 && CWASM.runtime.thread.mainDone ) {
                    CWASM.exitCode = 1;
                    CWASM.onExit?.( 1 );
                    CWASM.runtime.thread.mainDone( 1 );
                    CWASM.runtime.thread.dispose();
                } else {
                    forgetWorker( worker );
                    for( let i = freePool.length - 1; i >= 0; --i ) {
                        if( freePool[ i ].worker === worker ) { freePool.splice( i, 1 ); }
                    }
                    killWorker( worker );
                }
                return;
            }
            if( m.type === 'main.call' ) {
                bridge.serve();
                return;
            }
            if( m.type === 'frame.await' ) {
                requestAnimationFrame( () => {
                    const i = new Int32Array( m.sab );
                    Atomics.store( i, 0, 1 );
                    Atomics.notify( i, 0 );
                } );
                return;
            }
            if( m.type === 'memory.grew' ) {
                globalThis.mem.refresh();
                workers.forEach( w => {
                    if( w !== worker ) { w.postMessage( { type: 'memory.grew' } ); }
                } );
                freePool.forEach( e => e.worker.postMessage( { type: 'memory.grew' } ) );
                return;
            }
            if( m.type === 'transfer.request' ) {
                const ops = CWASM.runtime.transfer.ops || {};
                let obj = null;
                try {
                    obj = ops[ m.op ] ? ops[ m.op ]( m.arg ) : null;
                } catch( e ) {
                    console.error( 'cwasm transfer op', m.op, e );
                }
                worker.postMessage( { type: 'transfer.deliver', obj }, obj ? [ obj ] : [] );
                return;
            }
            if( onWorkerMsg( worker, slot.tid, m ) ) { return; }
        };
    };

    const makeWorker = ( tid, stackHigh, futexAddr ) => {
        const entry = freePool.pop();
        if( entry ) {
            const { worker, slot } = entry;
            slot.tid = tid;
            slot.futexAddr = futexAddr | 0;
            workers.set( tid, worker );
            try {
                worker.postMessage( { type: 'thread.rerun', tid, stackTop: stackHigh } );
            } catch( e ) {
                console.error( 'thread', tid, e );
                forgetWorker( worker );
                killWorker( worker );
                signalSpawnFail( futexAddr );
            }
            return;
        }
        const worker = new Worker( workerURL );
        allWorkers.add( worker );
        const slot = { tid, futexAddr: futexAddr | 0 };
        workers.set( tid, worker );
        bindWorker( worker, slot );
        worker._cwasmSlot = slot;
        try {
            worker.postMessage( {
                type: 'thread.init',
                module,
                memory,
                tid,
                useJspi,
                stackTop: stackHigh,
                cmdline: CWASM.cmdline,
                envstring: CWASM.envstring,
                embedRaw: CWASM.embedRaw || null,
                mainChannel: bridge.channel,
            } );
        } catch( e ) {
            console.error( 'thread', tid, e );
            signalSpawnFail( futexAddr );
        }
    };

    const recycleThis = worker => {
        if( !worker || worker._cwasmDead ) { return; }
        forgetWorker( worker );
        if( freePool.some( e => e.worker === worker ) ) { return; }
        const slot = worker._cwasmSlot;
        slot.tid = 0;
        slot.futexAddr = 0;
        freePool.push( { worker, slot } );
    };

    const onWorkerMsg = ( worker, tid, m ) => {
        if( m.type === 'thread.spawn' ) {
            makeWorker( m.tid, m.stackHigh, m.futexAddr );
            return true;
        }
        if( m.type === 'thread.exited' ) {
            forgetWorker( worker );
            killWorker( worker );
            return true;
        }
        if( m.type === 'proc.exit' ) {
            if( m.halt || m.tid === 0 ) {
                CWASM.exitCode = m.code | 0;
                CWASM.onExit?.( m.code | 0 );
                if( CWASM.runtime.thread.mainDone ) {
                    CWASM.runtime.thread.mainDone( m.code | 0 );
                }
                CWASM.runtime.thread.dispose();
            } else {
                recycleThis( worker );
            }
            return true;
        }
        return false;
    };
    CWASM.runtime.thread.runUserMain = () =>
        new Promise( resolve => {
            CWASM.runtime.thread.mainDone = code => resolve( code | 0 );
            makeWorker( 0, undefined, 0 );
        } );
}


async function run() {
    if( typeof crossOriginIsolated !== 'undefined' && !crossOriginIsolated ) {
        throw new Error( 'THREADS: page must be crossOriginIsolated (COOP/COEP)' );
    }

    const moduleOpts = {{{module_opts}}};
    const memDesc = moduleOpts && moduleOpts.memory;
    if( !memDesc || !memDesc.shared ) {
        throw new Error( 'THREADS: missing shared memory descriptor in module_opts' );
    }
    const sharedMem = new WebAssembly.Memory( {
        initial: memDesc.initial | 0,
        maximum: memDesc.maximum | 0,
        shared: true,
    } );
    globalThis.MEM = sharedMem;
    globalThis.mem = makeMem( sharedMem );

    const bridge = installMainBridge();

    const wasmStream = () =>
        Base64Stream( '{{{wasm}}}' ).pipeThrough( new DecompressionStream( 'deflate-raw' ) );
    const embedP = startEmbedWorker();

    const [ module, embedRaw ] = await Promise.all( [
        WebAssembly.compileStreaming(
            new Response( wasmStream(),
                { headers: { 'Content-Type': 'application/wasm' } } )
        ),
        embedP,
    ] );
    CWASM.embedRaw = embedRaw;
    fillCmdline();

    startThreads( module, sharedMem, { useJspi: !!moduleOpts.jspi }, bridge );
    const exitCode = await CWASM.runtime.thread.runUserMain();
    CWASM.exitCode = exitCode;
    CWASM.runtime.thread.dispose();
    return exitCode;
}

run();
