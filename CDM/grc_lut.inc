
struct GRC_LUT {
  GRC_LUT() { Init(); }

  byte LUT[256];

  byte unLUT[256];

  uint operator() ( uint i ) const {
    return LUT[i];
  }

  void Init( void ) {
    uint c,i,j; 
    for( i=0; i<256; i++ ) {
      j = i ^ (i>>1);
      LUT[i] = j;
      unLUT[j] = i;
    }
//    for( i=0; i<256; i++ ) printf( "%02X %02X %02X %02X\n", i, unLUT[LUT[i]]!=i, LUT[i],unLUT[i] );
  }

};

