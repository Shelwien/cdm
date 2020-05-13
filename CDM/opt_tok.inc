
struct token {
  qword blklen:9; // 
  qword p_maxlen:15;
  qword price:40; // its .12 in bits, so .15 in bytes, so 33 bits left, which is 8gb; really needs is log2(tokblk)=19, so 14 extra?

  qword pos:62; // 4TB is 42 bits, hard to expect more there?
  qword f_compflag:2;

  static const qword pos_empty = (-1ULL)>>2;

  void Init( uint n, uint p_maxblk ) {
    uint i;
    for( i=0; i<n; i++ ) {
      this[i].pos=0; //-1ULL-i;
      this[i].price=0;
      this[i].blklen=maxblk;
      this[i].f_compflag=0;
      this[i].p_maxlen = p_maxblk;
    }
  }

  void PrefetchData( void* restrict Addr ) {
    //byte Prefetchbyte = *(volatile byte* restrict)Addr;
    //__builtin_prefetch(Addr, 0, 0);
    //_mm_prefetch((const char*)Addr, 1/*_MM_HINT_T0*/);
  }

  template< class Codec >
INLINE
  void Compute( byte* restrict p, byte* restrict q, qword curpos, Codec& C ) {
    uint i,j, r, c0,c1, e0,e1, O_price_i, mode, log2maxblk, P,Pu,bit;
    uint e2, g_c0, g_c1, T0_price, cur_price;

    uint LOG2SCALE = 15<<16; //LOG2(SCALE);

    uint maxlen = Min<uint>(curpos+1-C.inppos,maxblk+1);

    // init current token if its not initialized yet
    if( pos!=curpos ) { pos=curpos; price=-1ULL; blklen=maxblk; f_compflag=0; p_maxlen = C.p_maxblk; }

    p += curpos - C.inppos;
    c0=0; c1=0; g_c0=0; g_c1=0;

    cur_price = price;

    word (& restrict q1)[2] = C.pop_array[(curpos-0)%tokblk];
    uint q1_0 = q1[0], q1_1 = q1[1];

    // base price
//#pragma unroll(256)
//    for( i=1,j=0; i<maxlen; j+=i*8+1,i++ ) {
    for( i=1,j=0; i<maxlen; i=C.p_blk_next[i-1] /*j+=i*8+1,i++*/ ) {
      j = i*(4*i-3)-1; // j = 4*(i-1)*(i-1)+5*(i-1);

//printf( "i=%i %i\n", i, C.p_blk_next[i-1] );

      word (& restrict q0)[2] = C.pop_array[(curpos-i)%tokblk];

/*
      r = 8-POP2( *(p-i) ); // number of 1s in byte
      c0 += r;
//      c1 += 8-r;
      r = 8-POP2( GRAY.LUT[*(p-i)] );
      g_c0 += r;
//      g_c1 += 8-r;
*/

      c0 = word(q1_0-q0[0]);
      g_c0 = word(q1_1-q0[1]);

      // get base price to reach curpos-i
      token& T0 = C.tok_array[(curpos-i)%tokblk];
//      if( T0.pos!=curpos-i ) { T0.pos = curpos-i; T0.price=0; T0.blklen=0; }
      T0_price = T0.price;

//      P = T0.p_maxlen; P = LinearUpdate( P, (i!=maxblk), M_mb_wr, M_mb_mw );
//      f_maxblk_0 = (LOG2SCALE-LOG2(P));
//      f_maxblk_1 = (LOG2SCALE-LOG2(SCALE-P));

      bit = (i!=maxblk);
      uint (& restrict LUT)[3] = C.mb_price[T0.p_maxlen>>(SCALElog-mb_prob_log)][bit];
      uint f_maxblk_0 = LUT[0];
      uint f_maxblk_1 = LUT[1];
      P = LUT[2];

//printf( "curpos=%I64i p0=%02X %02X\n", curpos-i, q[0], r );
      e1 = C.e1_price[j+c0];
      e2 = C.e1_price[j+g_c0]; 
      e0 = C.e0_price[i-1];

      e0 += ((i==maxblk)?C.f_maxblk_0:C.f_maxblk_1);
      e1 += C.f_compflag_1-C.f_compflag_2 + C.f_maxblk_1;
      e2 += C.f_maxblk_1;

/*
      e0 = CalcPrice0(C,i,log2maxblk);
      e1 = CalcPrice1(C,i,c0,c1,log2maxblk, f_compflag_1);
      e2 = CalcPrice1(C,i,g_c0,g_c1,log2maxblk, f_compflag_2);
*/

//      PrefetchData( &C.e1_price[j+i*8+1+c0+4] );
//      PrefetchData( &C.e1_price[j+i*8+1+g_c0+4] );


//      O_price_i = Min<uint>( Min<uint>(e0*4+0,e1*4+1), e2*4+2 );
//      mode = O_price_i&3; O_price_i = (O_price_i+63)/64;

      mode = e1<e0;
      O_price_i = mode ? e1 : e0;

      mode = e2<O_price_i ? 2 : mode;
      O_price_i = div16( (mode==2) ? e2 : O_price_i );

//printf( "%3i %08X %08X %08X  %08X %i\n", i, e0, e1, e2, O_price_i, mode );

      // remember the [curpos-i..curpos] price if its better than previos one
      if( cur_price>T0_price+O_price_i ) { blklen=i; cur_price=T0_price+O_price_i; f_compflag=mode; p_maxlen=P; }

    }

    price = cur_price;

  } // Compute

};
