function makeSuspend( host ) {
    let signalResume = null;
    let signalDone = false;
    let pending = 0;

    return {
        installWaits: env => {
            env.__async_wait_ms = new WebAssembly.Suspending( ms =>
                new Promise( r => {
                    setTimeout( r, ms < 0 ? 0 : ms | 0 );
                } ) );
            env.__async_wait_frame = new WebAssembly.Suspending( () =>
                new Promise( r => {
                    host.waitFrame( () => r( 0 ) );
                } ) );
            env.__async_wait_signal = new WebAssembly.Suspending( () =>
                new Promise( r => {
                    if( pending === 0 ) { signalDone = false; r( 0 ); return; }
                    if( signalDone ) { signalDone = false; r( 0 ); return; }
                    signalResume = r;
                } ) );
            globalThis.__async_signal_arm = () => { pending++; };
        },

        afterInstantiate: () => {},

        run: ( entry, onExit, onError ) => {
            WebAssembly.promising( entry )().then(
                () => onExit(),
                e => {
                    if( e === CWASM.__exitSymbol ) { onExit(); } else { onError( e ); }
                }
            );
        },

        resumeSignal: () => {
            if( pending > 0 ) { pending--; }
            if( signalResume ) {
                const r = signalResume;
                signalResume = null;
                r( 0 );
            } else {
                signalDone = true;
            }
        },
    };
}
