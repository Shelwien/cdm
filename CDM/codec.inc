
uint process_EOF( uint f_EOF=0 ) {
  rc_BProcess( p_EOF, f_EOF );
//printf( "f_EOF=%i p_EoF=%i\n", f_EOF, p_EOF );
  return f_EOF;
}

uint process_freqtable( uint* cumfreqs, uint v_high, uint value=0 ) {

  uint i, step, v_total= cumfreqs[v_high];

  if( f_DEC==1 ) { 
    value = rc_GetFreq(v_total);

    step = (v_high+1)/2;
    for( i=0; step>0; step>>=1 ) if( (i+step<=v_high) && (cumfreqs[i+step]<=value) ) i+=step;

//    for( i=0; cumfreqs[i+1]<=value; i++ );

    value = i;
  }

  uint v_low  = cumfreqs[value];
  uint v_freq = cumfreqs[value+1]-v_low;

  rc_Process( v_low, v_freq, v_total );

  return value;
}

uint process_range( uint v_low, uint v_high, uint value=0 ) {

  uint v_size = v_high-v_low;
  value -= v_low;

  if( v_size>1 ) {
    if( f_DEC==1 ) value = rc_GetFreq(v_size);
    rc_Process( value, 1, v_size );
  } else value=0;

  value += v_low;
  return value;
}

#ifdef TRACE_ON
double
#else
void 
#endif
process_blklen( uint& blklen, uint& freq0, uint& f_compflag ) {
  uint bit;

#ifdef TRACE_ON
double rc_codelen = double(this->rc_outbytes)*8 + double((0x100000-LOG2(this->range>>16)))/65536;
#endif
  bit = (blklen!=maxblk);
  rc_BProcess( p_maxblk, bit );

  if( bit==0 ) blklen=maxblk, f_compflag=0; else {

/*
    uint l_blklen = _bit_scan_forward(blklen); // 0..maxblklog-1
    uint maxlen = maxblk;

    l_blklen = process_freqtable( p_lsb, maxblklog, l_blklen );

    blklen >>= (l_blklen+1);
    maxlen >>= (l_blklen+1);
    blklen = process_range( 0, maxlen, blklen );
    (blklen <<= (l_blklen+1)) |= (1<<l_blklen);
*/
    blklen--;
    blklen = process_freqtable( p_blk, DIM(p_blk)-1, blklen );
    blklen++;

    //rc_BProcess( p_compflag, f_compflag );
    f_compflag = process_freqtable( p_cfl, 3, f_compflag );
  }

  if( f_compflag!=0 ) {

    uint n = blklen*8+1, n_half = n/2;
    int k = freq0;

    bit = (k==n_half);
    rc_BProcess( p_freq_half, bit );

    if( bit==1 ) k=n_half; else {

      // [0..n/2)U(n/2..n] -> [0..n/2)U[n/2..n) = [0..n)
      k -= n_half; // [-n/2..0)U[0..n/2)

      bit = (k>=0);
      rc_BProcess( p_freq_sign, bit );

      k = bit ? k : -k;
      k -= 1;

      if( n_half<mid_nmin ) {
        k = process_range(0,n_half, k );
      } else {

        uint r1 = n_half/(mid_div1+2+mid_div2), r2 = n_half/mid_div2, r12 = r2-r1;

        uint bit = (k>=r1) + (k>=r2); // ((k>=r1) && (k<r2));
        bit = process_freqtable( p_mid, 3, bit );
        //rc_BProcess( p_freq_mid, bit );

        if( bit==1 ) {
          k = process_range(r1,r2, k );
        } else if( bit==0 ) {
          k = process_range(0,r1, k );
        } else {
          k = process_range(r2,n_half, k );
        }

      }

      k += 1;
      k = bit ? k : -k;

      k += n_half;
    }

    freq0 = k;

  }


#ifdef TRACE_ON
if( f_DEC==0 ) {
  double rc_codelen1 = double(this->rc_outbytes)*8 + double((0x100000-LOG2(this->range>>16)))/65536 - rc_codelen;
  printf( "!inppos=%-8I64i blklen=%4i freq0=%4i method=%i headerbits=%6.3lf ", inppos, blklen*8, freq0, f_compflag, rc_codelen1 );
}
  return rc_codelen;
#endif
}

// encode a block of blklen bytes from p
void encode_block( byte* blkbuf, uint blklen, uint f_compflag ) {
  uint c,i,j,k,bit,p,freq0,freq1;

  for( k=0,freq0=0; k<blklen; k++ ) {
    c = (f_compflag==2) ? GRAY.LUT[blkbuf[k]] : blkbuf[k];
    c = POP2(c); // number of 1s in byte
    //c = POP2( blkbuf[k] ); // number of 1s in byte
    freq0 += 8-c;
    //freq1 += c;
  }

#ifdef TRACE_ON
double rc_codelen0 = 
#endif

  process_blklen( blklen, freq0, f_compflag ); freq1=blklen*8-freq0;

#ifdef TRACE_ON
double rc_codelen = double(this->rc_outbytes)*8 + double((0x100000-LOG2(this->range>>16)))/65536;
#endif

  if( f_compflag==0 ) {

    //for( k=0; k<blklen; k++ ) process_range( 0,256, blkbuf[k] );
    for( k=0; k<blklen; k++ ) rs_process_byte( blkbuf[k] );

  } else {
/*
    for( k=0; k<blklen; k++ ) {
      //c = blkbuf[k]; 
      c = (f_compflag==2) ? GRAY.LUT[blkbuf[k]] : blkbuf[k];
      for( i=7; i!=-1; i-- ) {
        bit = (c>>i)&1;
        rb_Process( bit, freq0, freq1 );
      }
    }
*/
    int k;
    uint b0,b1,b2,b3,b4,b5,b6,b7;

    for( k=0; k<int(blklen-256/8); k++ ) {
      if( (freq0==0)+(freq1==0) ) goto AllBits;
      c = (f_compflag==2) ? GRAY.LUT[blkbuf[k]] : blkbuf[k];
      b0 = (c>>0)&1; b1 = (c>>1)&1; b2 = (c>>2)&1; b3 = (c>>3)&1; 
      b4 = (c>>4)&1; b5 = (c>>5)&1; b6 = (c>>6)&1; b7 = (c>>7)&1;
      rb_Process( b7, freq0, freq1 ); rc_Renorm2();
      rb_Process( b6, freq0, freq1 ); rc_Renorm2();
      rb_Process( b5, freq0, freq1 ); rc_Renorm2();
      rb_Process( b4, freq0, freq1 ); rc_Renorm2();
      rb_Process( b3, freq0, freq1 ); rc_Renorm2();
      rb_Process( b2, freq0, freq1 ); rc_Renorm2();
      rb_Process( b1, freq0, freq1 ); rc_Renorm2();
      rb_Process( b0, freq0, freq1 ); rc_Renorm2();
    }

    for( k=k; k<blklen; k++ ) {
      if( (freq0==0)+(freq1==0) ) goto AllBits;
      c = (f_compflag==2) ? GRAY.LUT[blkbuf[k]] : blkbuf[k];
      b0 = (c>>0)&1; b1 = (c>>1)&1; b2 = (c>>2)&1; b3 = (c>>3)&1; 
      b4 = (c>>4)&1; b5 = (c>>5)&1; b6 = (c>>6)&1; b7 = (c>>7)&1;
      rb_Process( b7, freq0, freq1 ); rc_Renorm1();
      rb_Process( b6, freq0, freq1 ); rc_Renorm1();
      rb_Process( b5, freq0, freq1 ); rc_Renorm1();
      rb_Process( b4, freq0, freq1 ); rc_Renorm1();
      rb_Process( b3, freq0, freq1 ); rc_Renorm1();
      rb_Process( b2, freq0, freq1 ); rc_Renorm1();
      rb_Process( b1, freq0, freq1 ); rc_Renorm1();
      rb_Process( b0, freq0, freq1 ); rc_Renorm1();
    }

AllBits: {}


  }

  rc_Renorm();

#ifdef TRACE_ON
double rc_codelen1 = double(this->rc_outbytes)*8 + double((0x100000-LOG2(this->range>>16)))/65536;
printf( "blockbits=%8.3lf gain=%7.3lf!\n", rc_codelen1-rc_codelen, double(blklen*8)-(rc_codelen1-rc_codelen0) );
#endif

} // encode_block

byte blkbuf[maxblk];

uint decode_block( void ) {
  uint l_quit = 0;
  uint c,i,j,k,bit,p, blklen=0, freq0=0,freq1=0;
  uint f_compflag = 0;

//printf( "FFNum=%i\n", FFNum );
//  if( FFNum>2 ) return 0;

  process_blklen( blklen, freq0, f_compflag ); freq1=blklen*8-freq0;

  if( f_compflag==0 ) {

    for( k=0; k<blklen; k++ ) blkbuf[k] = rs_process_byte();

  } else {

    int k;
    uint b0,b1,b2,b3,b4,b5,b6,b7;

    for( k=0; k<int(blklen-256/8); k++ ) {
      if( (freq0==0)+(freq1==0) ) {
FillBits:
        c = byte(-(freq0==0));
        for( k=k; k<blklen; k++ ) blkbuf[k]=c;
        goto AllBits;
      }
      rb_Process( b7, freq0, freq1 );                       rc_Renorm2();
      rb_Process( b6, freq0, freq1 ); b7+=b7+b6;            rc_Renorm2();
      rb_Process( b5, freq0, freq1 );                       rc_Renorm2();
      rb_Process( b4, freq0, freq1 ); b5=b7*4+b5*2+b4;      rc_Renorm2();
      rb_Process( b3, freq0, freq1 );                       rc_Renorm2();
      rb_Process( b2, freq0, freq1 ); b3+=b3+b2;            rc_Renorm2();
      rb_Process( b1, freq0, freq1 );                       rc_Renorm2();
      rb_Process( b0, freq0, freq1 ); c=b5*16+b3*4+b1*2+b0; rc_Renorm2();
      blkbuf[k] = c; 
    }

    for( k=k; k<blklen; k++ ) {
      if( (freq0==0)+(freq1==0) ) goto FillBits;
      rb_Process( b7, freq0, freq1 );                       rc_Renorm1();
      rb_Process( b6, freq0, freq1 ); b7+=b7+b6;            rc_Renorm1();
      rb_Process( b5, freq0, freq1 );                       rc_Renorm1();
      rb_Process( b4, freq0, freq1 ); b5=b7*4+b5*2+b4;      rc_Renorm1();
      rb_Process( b3, freq0, freq1 );                       rc_Renorm1();
      rb_Process( b2, freq0, freq1 ); b3+=b3+b2;            rc_Renorm1();
      rb_Process( b1, freq0, freq1 );                       rc_Renorm1();
      rb_Process( b0, freq0, freq1 ); c=b5*16+b3*4+b1*2+b0; rc_Renorm1();
      blkbuf[k] = c; 
    }

AllBits: {}

  }

  rc_Renorm();
//  if( FFNum>3 ) return 0;

  if( f_compflag==2 ) 
    for( k=0; k<blklen; k++ ) put( GRAY.unLUT[blkbuf[k]] );
  else
    for( k=0; k<blklen; k++ ) put( blkbuf[k] );

  return blklen;
}
