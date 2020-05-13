
#include "common.inc"
#include "coro3b.inc"

#include "coro_fp2.inc"

#include "bwtl_transform.inc"

ALIGN(4096)
static union {
  CoroFileProc< BWTL<0> > C;
  CoroFileProc< BWTL<1> > D;
};

int main( int argc, char** argv ) {

  if( argc<4 ) return 1;

  uint f_DEC = (argv[1][0]=='d'); 

  uint chunksize = atoi( &argv[1][1] )*1024;
  if( (chunksize==0) || (chunksize>=(1<<29)) ) chunksize=32*1024;

  FILE* f = fopen(argv[2],"rb"); if( f==0 ) return 2; // input dict
  FILE* g = fopen(argv[3],"wb"); if( g==0 ) return 3; // output dict

  if( f_DEC==1 ) {
    fread( &chunksize, 1,sizeof(chunksize), f );
  } else {
    fwrite( &chunksize, 1,sizeof(chunksize), g );
  }

  printf( "f_DEC=%i chunksize=%i =%iK =%iM\n", f_DEC, chunksize,chunksize>>10,chunksize>>20 );

  if( f_DEC==0 ) {
    if( C.Init(chunksize) ) return 1;
    C.processfile( f, g );
    C.Quit();
  } else {
    if( D.Init(chunksize) ) return 1;
    D.processfile( f, g );
    D.Quit();
  }

  return 0;
}

