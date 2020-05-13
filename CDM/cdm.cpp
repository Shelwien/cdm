
#define INC_FLEN
#include "common.inc"

#include "coro3b.inc"

#include "coro_fp2.inc"

#include "cdm.inc"

ALIGN(4096)
static union {
  CoroFileProc< CDM<0> > C;
  CoroFileProc< CDM<1> > D;
};





int main( int argc, char** argv ) {

//printf( "%i\n", _bit_scan_forward(512) );
  if( argc<4 ) return 1;

  uint f_DEC = (argv[1][0]=='d'); 

  FILE* f = fopen(argv[2],"rb"); if( f==0 ) return 2; // input dict
  FILE* g = fopen(argv[3],"wb"); if( g==0 ) return 3; // output dict

  if( f_DEC==0 ) {
    C.Init();
    C.processfile( f, g );
  } else {
    D.Init();
    D.processfile( f, g );
  }

  return 0;
}


