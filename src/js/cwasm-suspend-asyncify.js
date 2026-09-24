function makeSuspend( host ) {
    // waitState: 0 running, 1 rewinding, 2 frame-wait, 3 timeout-wait, 4 signal-wait
    let asyncifyData = 0;
    let waitState = 0;
    let waitMs = 0;
    let signalDone = false;
    let entryFn = null;
    let onExitCb = null;
    let onErrorCb = null;
    let pending = 0;

    function resumeAsync() {
        const e = host.exports();
        e.asyncify_stop_unwind();
        e.asyncify_start_rewind( asyncifyData );
        drive();
    }

    function drive() {
        try {
            entryFn();
        } catch( e ) {
            if( e !== CWASM.__exitSymbol ) {
                if( onErrorCb ) { onErrorCb( e ); }
                return;
            }
        }
        if( waitState === 2 ) {
            waitState = 1;
            host.waitFrame( resumeAsync );
        } else if( waitState === 3 ) {
            waitState = 1;
            setTimeout( resumeAsync, waitMs );
        } else if( waitState === 4 ) {
            // park - resumeSignal() drives the rewind
        } else {
            if( onExitCb ) { onExitCb(); }
        }
    }

    return {
        installWaits: env => {
            env.__async_wait_ms = ms => {
                if( waitState === 1 ) {
                    waitState = 0;
                    host.exports().asyncify_stop_rewind();
                    return 0;
                }
                waitMs = ms < 0 ? 0 : ms | 0;
                waitState = 3;
                host.exports().asyncify_start_unwind( asyncifyData );
                return 0;
            };
            env.__async_wait_frame = () => {
                if( waitState === 1 ) {
                    waitState = 0;
                    host.exports().asyncify_stop_rewind();
                    return 0;
                }
                waitState = 2;
                host.exports().asyncify_start_unwind( asyncifyData );
                return 0;
            };
            env.__async_wait_signal = () => {
                if( waitState === 1 ) {
                    waitState = 0;
                    host.exports().asyncify_stop_rewind();
                    signalDone = false;
                    return 0;
                }
                if( pending === 0 ) { signalDone = false; return 0; }
                if( signalDone ) { signalDone = false; return 0; }
                waitState = 4;
                host.exports().asyncify_start_unwind( asyncifyData );
                return 0;
            };
            globalThis.__async_signal_arm = () => { pending++; };
        },

        afterInstantiate: () => {
            const e = host.exports();
            const s = e.__stack_high.value - e.__stack_low.value;
            asyncifyData = e.malloc( 8 + s );
            globalThis.mem.u32_write( asyncifyData, asyncifyData + 8 );
            globalThis.mem.u32_write( asyncifyData + 4, asyncifyData + 8 + s );
        },

        run: ( entry, onExit, onError ) => {
            entryFn = entry;
            onExitCb = onExit;
            onErrorCb = onError;
            drive();
        },

        resumeSignal: () => {
            if( pending > 0 ) { pending--; }
            signalDone = true;
            if( waitState === 4 ) {
                waitState = 1;
                resumeAsync();
            }
        },
    };
}
