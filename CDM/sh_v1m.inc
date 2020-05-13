
uint LinearUpdate( uint P, const int c, const int wr, const int Mw ) {
//  const int mw = c ? Mw : SCALE-Mw;
//  p = p*(SCALE-wr) + mw*wr;
  int dp = P + Mw-SCALE + ((SCALE-2*Mw)&(-c));
  dp = ((dp*wr)>>SCALElog);
  int q = P - dp + c;
  P = (q<mSCALE ) ? q : mSCALE;
  return P;
}

template< int f_DEC >
struct Rangecoder_SH1m : Coroutine {

  enum {
    SCALElog = 15,
    SCALE    = 1<<SCALElog
  };

  enum {
    NUM   = 4,
    sTOP  = 0x01000000U,
    gTOP  = 0x00010000U,
    Thres = 0xFF000000U
  };

//  int   f_DEC; // 0=encode, 1=decode;
  union {
    struct {
      uint  low;  
      uint  Carry;
    };
    qword lowc;
  };
  uint  code; 
  uint  FFNum;
  uint  Cache;
  uint  range;

#ifdef TRACE_ON
  uint  rc_outbytes;

  void put( uint c ) {
    Coroutine::put(c); rc_outbytes++;
  }
#endif


  void rs_Arrange( uint totFreq ) {
    range /= totFreq;
  }

  uint rs_GetFreq( void ) {
    return code/range;
  }

  void rs_Process( uint cumFreq, uint freq, uint totFreq ) {
    uint tmp = cumFreq*range;
    if( f_DEC ) code-=tmp; else lowc+=tmp;
    range *= freq;

    //if( range<sTOP ) 
    //ShiftStuff(); // totFreq is 256, so once is enough
    if( f_DEC==0 ) ShiftLow(); else ShiftCode();
  }

  uint rs_process_byte( uint value=0 ) {

    rs_Arrange( 256 );
    if( f_DEC==1 ) value = rs_GetFreq();
    rs_Process( value, 1, 256 );

    return value;
  }


//  uint mulRdiv( uint a, uint c ) { return (qword(a)*range)/c; }
  void rb_Process( uint& bit, uint& freq0, uint& freq1 ) {
    uint rlow, rtop;
//    uint rnew = mulRdiv( freq0, freq0+freq1 );

    uint itotal = rb_itotal[freq0+freq1]; 
//    uint itotal = 0xFFFFFFFFU/(freq0+freq1);
    itotal *= freq0;
    uint rnew = ((qword(range)*itotal+0xFFFFFFFFU)>>32);

//if( rnew!=rnew1 )
//printf( "rnew=%X %X rnew1=%X %X freq0=%i freq1=%i\n", rnew,range-rnew, rnew1,range-rnew1, freq0,freq1 );

    if( f_DEC==1 ) bit = (code>=rnew);

    freq0 -= bit^1;
    freq1 -= bit;

/*
    if( bit==0 ) {
      rlow = 0;    rtop = rnew-0;
      if( f_DEC ) code-=rlow; else lowc+=rlow;
      range = rtop;
    } else {
      rlow = rnew; rtop = range-rnew;
      if( f_DEC ) code-=rlow; else lowc+=rlow;
      range = rtop;
    }
*/

    rlow = rnew & (-bit);
    rtop = ((range-rnew-rnew) & (-bit)) + rnew;

    if( f_DEC ) code-=rlow; else lowc+=rlow;
    range = rtop;

//    if( t_renorm==2 ) rc_Renorm2();
//    if( t_renorm==1 ) rc_Renorm1();
  }



  uint muldivR( uint a, uint b ) { return (qword(a)*b)/range; }

  uint mulRdiv( uint a, uint c ) { return (qword(a)*range)/c; }

  void rc_Renorm( void ) {
    while( range<sTOP ) ShiftStuff();
  }

  void rc_Renorm0( void ) {}

  void rc_Renorm1( void ) {
    if( range<sTOP ) ShiftStuff();
  }

  void rc_Renorm2( void ) {
    if( range<gTOP ) ShiftStuff(),ShiftStuff(); else
    if( range<sTOP ) ShiftStuff();
  }

  void rc_Process( uint cumFreq, uint freq, uint totFreq ) {
    uint tmp = range-mulRdiv( totFreq-cumFreq, totFreq );
    uint rnew = range-mulRdiv( totFreq-(cumFreq+freq), totFreq );
    if( f_DEC ) code-=tmp; else lowc+=tmp;
    range = rnew - tmp;
    rc_Renorm();
  }

  void rc_Arrange( uint totFreq ) {}

  uint rc_GetFreq( uint totFreq ) {
    return muldivR( code, totFreq );
  }

  INLINE
  void rc_BProcess( uint freq, uint& bit ) { 
    uint rnew = (qword(range)*(freq<<(32-SCALElog)))>>32;
 //   uint rnew = ( range>=16*sTOP ) ? freq*(range>>SCALElog) : freq*(range>>(SCALElog-4))>>4;
 //   uint rnew = (range>>SCALElog)*freq;

    if( f_DEC ) bit = (code>=rnew);

    range = ((range-rnew-rnew)&(-bit)) + rnew;
    rnew &= -bit;

    if( f_DEC ) code -= rnew; else lowc += rnew;

    rc_Renorm2();
  }

  NOINLINE
  void ShiftStuff( void ) {
    if( f_DEC==0 ) ShiftLow(); else ShiftCode();
  }

  void ShiftCode( void ) {
    range = (range<<8)+0x00;
    code  = (code<<8)+0x00;
    uint c = get();
    FFNum += (c==-1);
    code += byte(c);
  }

  void ShiftLow( void ) {
    range = (range<<8)+0x00;
    if( low<Thres || Carry ) {
      if( Cache!=-1 ) put( Cache+Carry );
      for (;FFNum != 0;FFNum--) put( Carry-1 ); // (Carry-1)&255;
      Cache = low>>24;
      Carry = 0;
    } else FFNum++;
    low<<=8;
  }


  void rcInit( void ) { 
    range = 0xFFFFFFFF;
    low   = 0;
    FFNum = 0;
    Carry = 0;    
    Cache = -1;
  }

  uint rb_itotal[32768+1];
  
  void rc_Init( void ) {
    uint i;
    for( i=1; i<DIM(rb_itotal); i++ ) rb_itotal[i]=0xFFFFFFFFU/i;

    rcInit();
    if( f_DEC==1 ) {
      for(int _=0; _<NUM; _++) ShiftCode();
      range = 0xFFFFFFFF;
    }
  }

  void rc_Quit( void ) {
    if( f_DEC==0 ) {
      uint i, n = NUM;

      // cache .. FF x FFNum .. low
      qword llow=low;
      qword high=llow+range;

      if( qword(llow|0xFF) < high ) llow|=0xFF,n--;
      if( qword(llow|0xFFFF) < high ) llow|=0xFFFF,n--;
      if( qword(llow|0xFFFFFF) < high ) llow|=0xFFFFFF,n--;
      if( qword(llow|0xFFFFFFFF) < high ) llow|=0xFFFFFFFF,n--;
      low = uint(llow);
      if( low==0xFFFFFFFF ) FFNum=0; // they're also FFs

      if( Cache!=-1 ) put( Cache );
      for( i=0; i<FFNum; i++ ) put( 0xFF );
      for( i=0; i<n; i++ ) put( low>>24 ), low<<=8;
    }
  }

};

