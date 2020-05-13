
struct pop2LUT {
  pop2LUT() { Init(); }

  byte LUT[256];

  uint operator() ( uint i ) const {
    return LUT[i];
  }

  void Init( void ) {
    uint c,i,j; 
    for( i=0; i<256; i++ ) {
      for( j=7,c=0; j!=-1; j-- ) c += (i>>j)&1;
      LUT[i] = c;
    }
  }

};

