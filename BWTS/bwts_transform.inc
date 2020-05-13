
//#include "divsufsort/bwt.h"

template< int f_DEC >
struct BWTS : Coroutine {

#include "bwts.inc"

  bwts B;

  uint N; // blocksize
  byte* inpbuf;
  byte* outbuf;

  uint Init( uint blocksize ) {
    uint r;
    N = blocksize; 
    r = B.init( f_DEC, blocksize );
    inpbuf = B.inpbuf;
    outbuf = B.outbuf;
    return r;
  }

  void Quit( void ) {
    B.quit();
  }

  void do_process( void ) {
    uint i,j,l,r,n,flag,lf;

//printf( "N=%i f_DEC=%i\n", N, f_DEC );

    while( f_quit==0 ) {

      for( i=0; i<N; ) {
        chkinp(); l=getinplen(); if( i+l>N ) l=N-i;
        memcpy( &inpbuf[i], (byte*)inpptr, l ); i+=l; inpptr+=l;
        if( f_quit ) break;
      } n=i;

      B.inplen = n;
      r = B.process();

      if( n>0 ) for( i=0; i<n; i++ ) put( outbuf[i] );

lf = get();
if( lf!=-1 ) put( lf );

    }

    yield(this,0);
  }

};
