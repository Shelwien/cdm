
struct log2LUT {
  log2LUT() { Init(); }

  enum {
    LUTsize = 65536+1, 
    Precision = 16
  };

  uint LUT[LUTsize];

  uint FAC[LUTsize];

  uint operator() ( uint i ) const {
//if( i>=LUTsize ) printf( "!LOG2LUT bug, i=%i!\n", i );
    return LUT[i];
  }

  void Init( void ) {
    uint i,j,k;
    qword factorial=0;
    for( i=0; i<LUTsize; i++ ) {
      qword w = i;
      for( j=0,k=0; j<Precision; j++ ) {
        w = w * w;
        k = k + k;
        while( w>=(1ULL<<32) ) w=(w+1)>>1, k++;
      }
      k += i>1?31:0;
      LUT[i]=k; factorial+=k;
      FAC[i]=(factorial+15)>>4;
//printf( "%5i %08X %08I64X\n", i, k, FAC[i] );
    }
  }

};

