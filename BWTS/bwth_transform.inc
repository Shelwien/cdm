
//#include "divsufsort/bwt.h"

template< int f_DEC >
struct BWTH : Coroutine {

#include "bwts.inc"

  bwts B;

  uint N; // blocksize
  byte* inpbuf;
  byte* outbuf;

  ALIGN(32)
  byte bin[256][8];

  uint Init( uint blocksize ) {
    uint r,i,j;

    for( i=0; i<256; i++ ) for( j=0; j<8; j++ ) bin[i][j]=(i>>(7-j))&1;

    N = blocksize; 
    r = B.init( f_DEC, blocksize*8 );
    inpbuf = B.inpbuf;
    outbuf = B.outbuf;
    return r;
  }

  void Quit( void ) {
    B.quit();
  }

  void do_process( void ) {
    uint c,i,j,l,r,n,flag;

//printf( "N=%i f_DEC=%i\n", N, f_DEC );

    while( f_quit==0 ) {

      for( i=0; i<N; ) {
        chkinp(); l=getinplen(); if( i+l>N ) l=N-i;

        for( j=0,c=1; j<l; j++ ) {
          c = inpptr[j];
          (qword&)inpbuf[(i+j)*8] = (qword&)bin[c][0];
        }

        i+=l; inpptr+=l;
        if( f_quit ) break;
      } n=i*8;

      B.inplen = n;
      r = B.process();
      //memcpy( outbuf, inpbuf, n );

      if( n>0 ) {
        //for( i=0; i<n; i++ ) put( outbuf[i] );
        uint b0,b1,b2,b3,b4,b5,b6,b7;

        for( i=0; i<n; i+=8 ) {
          b0 = outbuf[i+0];
          b1 = outbuf[i+1] + 2*b0;
          b2 = outbuf[i+2];
          b3 = outbuf[i+3] + 2*b2 + 4*b1;
          b4 = outbuf[i+4];
          b5 = outbuf[i+5] + 2*b4;
          b6 = outbuf[i+6];
          b7 = outbuf[i+7] + 2*b6 + 4*b5 + 16*b3;
          put( b7 );
        }
      }

    }

    yield(this,0);
  }

};
