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


self.onmessage = async () => {
    const b64 = '{{{embed}}}';
    if( !b64 ) {
        self.postMessage( null );
        return;
    }
    const bytes = new Uint8Array( await new Response(
        Base64Stream( b64 ).pipeThrough( new DecompressionStream( 'deflate-raw' ) )
    ).arrayBuffer() );
    self.postMessage( bytes, [ bytes.buffer ] );
};
