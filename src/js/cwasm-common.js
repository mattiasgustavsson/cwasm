function makeMem( memory ) {
    let u8;
    let i8;
    let u16;
    let i16;
    let u32;
    let i32;
    let f32;
    let f64;
    const td = new TextDecoder();
    const te = new TextEncoder();
    function refresh() {
        const b = memory.buffer;
        u8 = new Uint8Array( b );
        i8 = new Int8Array( b );
        u16 = new Uint16Array( b );
        i16 = new Int16Array( b );
        u32 = new Uint32Array( b );
        i32 = new Int32Array( b );
        f32 = new Float32Array( b );
        f64 = new Float64Array( b );
    }
    refresh();
    return {
        refresh,
        grow: pages => { memory.grow( pages | 0 ); refresh(); },
        ensureFresh: () => { if( u8.buffer !== memory.buffer ) { refresh(); } },
        buffer: () => memory.buffer,
        size: () => u8.length,
        u8_read: p => u8[ p ],           u8_write: ( p, v ) => { u8[ p ] = v; },
        i8_read: p => i8[ p ],           i8_write: ( p, v ) => { i8[ p ] = v; },
        u16_read: p => u16[ p >> 1 ],    u16_write: ( p, v ) => { u16[ p >> 1 ] = v; },
        i16_read: p => i16[ p >> 1 ],    i16_write: ( p, v ) => { i16[ p >> 1 ] = v; },
        u32_read: p => u32[ p >> 2 ],    u32_write: ( p, v ) => { u32[ p >> 2 ] = v; },
        i32_read: p => i32[ p >> 2 ],    i32_write: ( p, v ) => { i32[ p >> 2 ] = v; },
        f32_read: p => f32[ p >> 2 ],    f32_write: ( p, v ) => { f32[ p >> 2 ] = v; },
        f64_read: p => f64[ p >> 3 ],    f64_write: ( p, v ) => { f64[ p >> 3 ] = v; },
        str_read: ( p, len ) => {
            if( !p ) { return ''; }
            if( typeof len !== 'number' ) {
                len = 0;
                while( p + len < u8.length && u8[ p + len ] ) { ++len; }
            } else if( len <= 0 ) {
                return '';
            }
            return td.decode( new Uint8Array( u8.subarray( p, p + len ) ) );
        },
        str_write: ( s, p, cap ) => {
            const buf = te.encode( String( s ) );
            const needed = buf.length + 1;
            if( !p || !cap ) { return needed; }
            let n = buf.length;
            if( needed > cap ) {
                for( n = cap - 1; n > 0 && ( buf[ n ] & 0xC0 ) == 0x80; --n ) { }
            }
            u8.set( buf.subarray( 0, n ), p );
            u8[ p + n ] = 0;
            return needed;
        },
        bytes_read: ( p, len ) => new Uint8Array( u8.subarray( p, p + len ) ),
        bytes_write: ( p, src ) => { u8.set( src, p ); },
        raw_view: ( p, len ) => u8.subarray( p, p + len ),
    };
}


function discoverEnvImports( source ) {
    const sjlj = [];
    let needCpp = false;
    let needLj = false;
    const consider = name => {
        if( name === '__cpp_exception' ) {
            needCpp = true;
        } else if( name === '__c_longjmp' ) {
            needLj = true;
        } else if( name.startsWith( 'invoke_' ) ||
            name === '_emscripten_throw_longjmp' ) {
            sjlj.push( name );
        }
    };
    if( source instanceof WebAssembly.Module ) {
        for( const im of WebAssembly.Module.imports( source ) ) {
            if( im.module === 'env' ) { consider( im.name ); }
        }
    } else {
        const env = ( source && source.env ) || {};
        for( const name of Object.keys( env ) ) { consider( name ); }
    }
    return { needCpp, needLj, sjlj };
}


function installSjlj( env, sjljNames, tableHolder ) {
    if( !sjljNames || !sjljNames.length ) { return; }
    const LJ = Symbol( 'cwasm.longjmp' );
    env._emscripten_throw_longjmp = () => { throw LJ; };
    for( const name of sjljNames ) {
        if( !name.startsWith( 'invoke_' ) ) { continue; }
        const sig = name.slice( 7 );
        if( !sig.length ) { continue; }
        const ret = sig[ 0 ];
        env[ name ] = ( ...args ) => {
            const fn = tableHolder.table.get( args[ 0 ] );
            if( !fn ) { throw new Error( 'invoke: bad table index ' + args[ 0 ] ); }
            try {
                const result = fn( ...args.slice( 1 ) );
                return ret === 'v' ? undefined : result;
            } catch( e ) {
                if( e !== LJ ) { throw e; }
                switch( ret ) {
                    case 'i': case 'j': case 'f': case 'd': return 0;
                    default: return undefined;
                }
            }
        };
    }
}
