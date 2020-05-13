
template< class Codec >
uint CalcPrice_blklen( Codec& C, uint blklen ) {
/*
  uint l_blklen = _bit_scan_forward(blklen); // 0..maxblklog-1
  uint maxblklen = maxblk;
  maxblklen >>= (l_blklen+1);
  uint log2maxblk = LOG2(C.p_lsb[maxblklog]) - LOG2(C.p_lsb[l_blklen+1]-C.p_lsb[l_blklen])  + LOG2(maxblklen);
*/
//    blklen = process_freqtable( p_blk, DIM(p_blk)-1, blklen );

  uint log2maxblk = LOG2(C.p_blk[maxblk-1]) - LOG2(C.p_blk[blklen-1+1]-C.p_blk[blklen-1]);

  return log2maxblk;
}

template< class Codec >
uint CalcPrice0( Codec& C, uint blklen, uint& log2maxblk ) {
  const uint f_maxblk_0=0,f_maxblk_1=0;

  uint e; 

  if( blklen==maxblk ) {
    log2maxblk = 0;
    e = f_maxblk_0 + (blklen<<(3+16));
  } else {
    log2maxblk = CalcPrice_blklen( C, blklen );
    e = (f_maxblk_1 + log2maxblk + C.f_compflag_0) + (blklen<<(3+16));
//    e = log2maxblk; //(f_maxblk_1 + log2maxblk + f_compflag_0) + (blklen<<(3+16));
  }

  return e;
}

template< class Codec >
uint CalcPrice1( Codec& C, uint blklen, uint c0, uint c1, uint _log2maxblk, uint f_compflag_x ) {
  const uint f_maxblk_0=0,f_maxblk_1=0;
  uint e = 0xFFF80000;

  if( blklen==maxblk ) return e;

  uint log2maxblk = f_maxblk_1 + _log2maxblk + f_compflag_x;

  e = (LOG2.FAC[c0+c1] - LOG2.FAC[c0] - LOG2.FAC[c1])<<4;

  uint n = blklen*8+1, n_half = n/2;
  int k = c0; //freq0;
  uint bit = (k==n_half);
  uint log2freq = bit ? C.f_freq_half_1 : C.f_freq_half_0; // k==n_half flag

  if( bit==0 ) {
    k -= n_half;
    bit = (k>=0);
    log2freq += bit ? C.f_freq_sign_1 : C.f_freq_sign_0;
    k = bit ? k : -k;
    k -= 1;
    if( n_half<mid_nmin ) {
      log2freq += LOG2(n_half);
    } else {
      uint r1 = n_half/(mid_div1+2+mid_div2), r2 = n_half/mid_div2, r12 = r2-r1;

      bit = (k>=r1) + (k>=r2); // ((k>=r1) && (k<r2));
      log2freq += LOG2(C.p_mid[3]) - LOG2(C.p_mid[bit+1]-C.p_mid[bit]);

      if( bit==1 ) {
        log2freq += LOG2(r12);
      } else if( bit==0 ) {
        log2freq += LOG2(r1);
      } else {
        log2freq += LOG2(n_half-r2);
      }

    } // mid_nmin
  } // f_half

  e = log2maxblk + log2freq + e;

  return e;
}
