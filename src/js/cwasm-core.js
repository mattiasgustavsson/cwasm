(() => {
    const CWASM = globalThis.CWASM || ( globalThis.CWASM = {} );
    CWASM.runtime = CWASM.runtime || { core: {}, thread: {}, transfer: {} };
    const print = CWASM.print ||
        ( CWASM.print = msg => console.log( msg.replace( /\n$/, '' ) ) );


    function abort( tag, msg ) {
        throw new Error( msg == null ? String( tag ) : tag + ': ' + msg );
    }


    function Base64Stream( b64 ) {
        if( typeof Uint8Array.fromBase64 !== 'function' ) {
            return new Blob( [ Uint8Array.from( atob( b64 ),
                ch => ch.charCodeAt( 0 ) ) ] ).stream();
        }
        return new ReadableStream( {
            start( c ) {
                for( let i = 0; i < b64.length; ) {
                    let end = Math.min( i + 262144, b64.length );
                    if( end < b64.length ) { end -= ( end - i ) & 3; }
                    c.enqueue( Uint8Array.fromBase64( b64.slice( i, end ) ) );
                    i = end;
                }
                c.close();
            },
        } );
    }


    function startEmbedWorker() {
        const w = new Worker( URL.createObjectURL( new Blob( [ {{{embed_worker_js}}} ],
            { type: 'application/javascript' } ) ) );
        return new Promise( ( resolve, reject ) => {
            w.onmessage = e => {
                resolve( e.data );
                w.terminate();
            };
            w.onerror = e => {
                w.terminate();
                reject( e.error || e );
            };
            w.postMessage( 0 );
        } );
    }


    function fillCmdline() {
        const u = new URL( location.href );
        CWASM.envstring = u.hash ? u.hash.slice( 1 ) : '';
        let cmdline = u.origin + u.pathname;
        const query = u.search ? u.search.slice( 1 ) : '';
        if( query ) {
            cmdline += ' ' + decodeURIComponent( query.replace( /\+/g, ' ' ) );
        } else {
            const d = "{{{default_argv}}}";
            if( d ) { cmdline += ' ' + d; }
        }
        CWASM.cmdline = cmdline;
    }


    Object.assign( globalThis, { abort, print } );

    {{{threading_js}}}
})();
