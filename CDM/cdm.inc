
//#include <windows.h>

//#define TRACE_ON

enum { SCALElog=15, SCALE=1<<SCALElog, mSCALE=SCALE-1, hSCALE=SCALE/2, eSCALE=16*SCALE };

#include "log2lut.inc"
#include "pop2lut.inc"
#include "grc_lut.inc"

#include "sh_v1m.inc"

//#include "sh_mapping.inc"
//#include "MOD/sh_model_h.inc"
#include "./sh_model_h.inc"

ALIGN(32) static const log2LUT LOG2;

ALIGN(32) static const pop2LUT POP2;

ALIGN(32) static const GRC_LUT GRAY;

uint div16( uint x ) { return (x+15)>>4; }

enum {
//  minblk = 16,
  maxblk = 256, Nblkset=64, maxblklog = 8, mb_prob_log=12, mb_prob_scale=1<<mb_prob_log,
//  maxblk = 2048, maxblklog = 11,
  optblk = (1<<19)-(maxblk+2), 
  optblk2 = optblk+(maxblk+2),
  tokblk = optblk+(maxblk+2),
  goaldiff = maxblk*32,
//  mid_div1 = 28, mid_div2 = 10, mid_nmin = (mid_div1+2+mid_div2)*2
};

static const uint p_EOF       = Max<int>(1,Min<int>(mSCALE,M_EOF));
static const uint p_freq_half = Max<int>(1,Min<int>(mSCALE,M_freq_half));
static const uint p_freq_sign = Max<int>(1,Min<int>(mSCALE,M_freq_sign));
static const uint mid_div1 = M_mid_div1;
static const uint mid_div2 = M_mid_div2;
static const uint mid_nmin = (mid_div1+2+mid_div2)*2;
             
static const int p_mid_0[4] = { M_mid0,M_mid1,M_mid2, 0 };
static const int p_cfl_0[4] = { M_cfl0,M_cfl1,M_cfl2, 0 };
static const int p_blk_0[] = { M_blk01,M_blk02,M_blk03,M_blk04,M_blk05,M_blk06,M_blk07,M_blk08,M_blk09,M_blk0A,M_blk0B,M_blk0C,M_blk0D,M_blk0E,M_blk0F,M_blk10,M_blk11,M_blk12,M_blk13,M_blk14,M_blk15,M_blk16,M_blk17,M_blk18,M_blk19,M_blk1A,M_blk1B,M_blk1C,M_blk1D,M_blk1E,M_blk1F,M_blk20,M_blk21,M_blk22,M_blk23,M_blk24,M_blk25,M_blk26,M_blk27,M_blk28,M_blk29,M_blk2A,M_blk2B,M_blk2C,M_blk2D,M_blk2E,M_blk2F,M_blk30,M_blk31,M_blk32,M_blk33,M_blk34,M_blk35,M_blk36,M_blk37,M_blk38,M_blk39,M_blk3A,M_blk3B,M_blk3C,M_blk3D,M_blk3E,M_blk3F,M_blk40,M_blk41,M_blk42,M_blk43,M_blk44,M_blk45,M_blk46,M_blk47,M_blk48,M_blk49,M_blk4A,M_blk4B,M_blk4C,M_blk4D,M_blk4E,M_blk4F,M_blk50,M_blk51,M_blk52,M_blk53,M_blk54,M_blk55,M_blk56,M_blk57,M_blk58,M_blk59,M_blk5A,M_blk5B,M_blk5C,M_blk5D,M_blk5E,M_blk5F,M_blk60,M_blk61,M_blk62,M_blk63,M_blk64,M_blk65,M_blk66,M_blk67,M_blk68,M_blk69,M_blk6A,M_blk6B,M_blk6C,M_blk6D,M_blk6E,M_blk6F,M_blk70,M_blk71,M_blk72,M_blk73,M_blk74,M_blk75,M_blk76,M_blk77,M_blk78,M_blk79,M_blk7A,M_blk7B,M_blk7C,M_blk7D,M_blk7E,M_blk7F,M_blk80,M_blk81,M_blk82,M_blk83,M_blk84,M_blk85,M_blk86,M_blk87,M_blk88,M_blk89,M_blk8A,M_blk8B,M_blk8C,M_blk8D,M_blk8E,M_blk8F,M_blk90,M_blk91,M_blk92,M_blk93,M_blk94,M_blk95,M_blk96,M_blk97,M_blk98,M_blk99,M_blk9A,M_blk9B,M_blk9C,M_blk9D,M_blk9E,M_blk9F,M_blkA0,M_blkA1,M_blkA2,M_blkA3,M_blkA4,M_blkA5,M_blkA6,M_blkA7,M_blkA8,M_blkA9,M_blkAA,M_blkAB,M_blkAC,M_blkAD,M_blkAE,M_blkAF,M_blkB0,M_blkB1,M_blkB2,M_blkB3,M_blkB4,M_blkB5,M_blkB6,M_blkB7,M_blkB8,M_blkB9,M_blkBA,M_blkBB,M_blkBC,M_blkBD,M_blkBE,M_blkBF,M_blkC0,M_blkC1,M_blkC2,M_blkC3,M_blkC4,M_blkC5,M_blkC6,M_blkC7,M_blkC8,M_blkC9,M_blkCA,M_blkCB,M_blkCC,M_blkCD,M_blkCE,M_blkCF,M_blkD0,M_blkD1,M_blkD2,M_blkD3,M_blkD4,M_blkD5,M_blkD6,M_blkD7,M_blkD8,M_blkD9,M_blkDA,M_blkDB,M_blkDC,M_blkDD,M_blkDE,M_blkDF,M_blkE0,M_blkE1,M_blkE2,M_blkE3,M_blkE4,M_blkE5,M_blkE6,M_blkE7,M_blkE8,M_blkE9,M_blkEA,M_blkEB,M_blkEC,M_blkED,M_blkEE,M_blkEF,M_blkF0,M_blkF1,M_blkF2,M_blkF3,M_blkF4,M_blkF5,M_blkF6,M_blkF7,M_blkF8,M_blkF9,M_blkFA,M_blkFB,M_blkFC,M_blkFD,M_blkFE,M_blkFF, 0 };

//uint p_maxblk = SCALE-16;
//uint p_EOF = SCALE-16;
//uint p_freq_half = SCALE-16;
//uint p_freq_sign = SCALE/2;
//uint p_freq_mid = 4096;
//uint mid_div1 = 28, mid_div2 = 10, mid_nmin = (mid_div1+2+mid_div2)*2;

#pragma pack()

#include "opt_calc.inc"

#include "opt_tok.inc"

template< int f_DEC >
struct CDM: Rangecoder_SH1m<f_DEC> {

  using Rangecoder_SH1m<f_DEC>::FFNum;
  using Rangecoder_SH1m<f_DEC>::inpptr;
  using Rangecoder_SH1m<f_DEC>::f_quit;

  enum { inpbufsize = 1<<16, inpbuftotal=inpbufsize*8, inpbufhalf=inpbuftotal/2 };
  ALIGN(4096)
  byte inpbuf[inpbuftotal];

  qword inppos;
  qword tokendpos;

  qword goalpos, s_goalpos; 
  uint goalptr;
  uint _pad1;

  byte* inpbegptr;
  byte* inpendptr;

  uint p_maxblk;

// precalc for block price estimation
  uint f_maxblk_0;
  uint f_maxblk_1;
  uint f_compflag_0;
  uint f_compflag_1;
  uint f_compflag_2;
  uint f_freq_half_0;
  uint f_freq_half_1;
  uint f_freq_sign_0;
  uint f_freq_sign_1;
//uint f_freq_mid_0;
//uint f_freq_mid_1;

  uint p_lsb[maxblklog+1];
  uint p_mid[3+1];
  uint p_cfl[3+1];

  uint p_blk[DIMM(p_blk_0)];
  uint p_blk_1[DIMM(p_blk_0)];
  uint p_blk_next[maxblk+1];

  ALIGN(32)
  token tok_array[tokblk];

  ALIGN(32)
  word pop_array[tokblk][2];
  uint pop_counter[2];

  ALIGN(32)
  uint e0_price[maxblk+1];
  uint e1_price[4*maxblk*maxblk+5*maxblk];
//  uint e2_price[4*maxblk*maxblk+5*maxblk];

  uint mb_price[mb_prob_scale][2][3];

  word len_array[tokblk]; // len array from parser opt

  void init_prices( void ) {
    f_maxblk_0    = (LOG2(SCALE)-LOG2(p_maxblk));
    f_maxblk_1    = (LOG2(SCALE)-LOG2(SCALE-p_maxblk));                

    f_compflag_0  = LOG2(p_cfl[3])-LOG2(p_cfl[0+1]-p_cfl[0]);
    f_compflag_1  = LOG2(p_cfl[3])-LOG2(p_cfl[1+1]-p_cfl[1]);
    f_compflag_2  = LOG2(p_cfl[3])-LOG2(p_cfl[2+1]-p_cfl[2]);

    f_freq_half_0 = (LOG2(SCALE)-LOG2(p_freq_half));                   
    f_freq_half_1 = (LOG2(SCALE)-LOG2(SCALE-p_freq_half));             
    f_freq_sign_0 = (LOG2(SCALE)-LOG2(p_freq_sign));                   
    f_freq_sign_1 = (LOG2(SCALE)-LOG2(SCALE-p_freq_sign));
  }

  void SWAP( uint& x, uint& y ) {
    uint t=x; x=y; y=t;
  }

  uint SWAP_if_lt( uint& x, uint& y ) {
    uint flag=0;
    if( x<y ) SWAP(x,y),flag=1;
    return flag;
  }


  uint Init( void ) {
    uint i,j, cumfreq, flag;

    coro_init();

//    LOG2.Init();
//    POP2.Init(); 
//    GRAY.Init();
    p_maxblk      = Max<int>(1,Min<int>(mSCALE,M_maxblk));

    pop_counter[0] = pop_counter[1] = 0;

    tok_array[0].Init(tokblk,p_maxblk);

//    for( i=0,cumfreq=0; i<=maxblklog; i++ ) { p_lsb[i]=cumfreq; cumfreq+=p_lsb_0[i]; }

    for( i=0,cumfreq=0; i<=3; i++ ) { p_mid[i]=cumfreq; cumfreq+=p_mid_0[i]; }

    for( i=0,cumfreq=0; i<=3; i++ ) { p_cfl[i]=cumfreq; cumfreq+=p_cfl_0[i]; }

    for( i=0; i<DIM(p_blk); i++ ) p_blk[i]=(p_blk_0[i]<<8) + (i^0xFF), p_blk_1[i]=p_blk_0[i];
    p_blk[1-1] |= 0xFFFF00;

    // sort blklen freqs
    do {
      for( i=0,flag=0; i<DIM(p_blk)-1; i++ ) flag+=SWAP_if_lt( p_blk[i],p_blk[i+1] );
    } while( flag );

//    for( i=0; i<DIM(p_blk); i++ ) printf( "%X,", p_blk[i] );
//    printf( "! %X !\n", p_blk[Nblkset] );

    // keep only Nblkset highest freqs (and blklen=1)
    for( i=Nblkset; i<DIM(p_blk); i++ ) p_blk_1[byte(p_blk[i]^0xFF)]=0;

    for( flag=maxblk-1,i=DIM(p_blk)-1; i!=-1; i-- ) {
      p_blk_next[i] = flag+1;
      if( p_blk_1[i]>0 ) flag=i;
    }
    p_blk_next[maxblk-1]=-1U;

//    for( i=0; i<DIM(p_blk); i++ ) printf( "%X,", p_blk_next[i] ); printf( "\n" );

//    for( i=0,j=1; i<DIM(p_blk); i++ ) printf( "%i:%X,", i,j ), j=p_blk_next[j-1]; printf( "\n" );

    for( i=0,cumfreq=0; i<DIM(p_blk); i++ ) { p_blk[i]=cumfreq; cumfreq+=p_blk_1[i]; }

    init_prices();

    {
      uint j, blklen,  r, c0,c1, e0,e1, O_price_i, mode, log2maxblk;
      uint P,Pu, e2, g_c0, g_c1;

//printf( "e0_price=%i\n", sizeof(e0_price) );
//printf( "e1_price=%i\n", sizeof(e1_price) );
//printf( "e2_price=%i\n", sizeof(e2_price) );

      for( i=0; i<mb_prob_scale; i++ ) {
        P = i<<(SCALElog-mb_prob_log); 
        P = Max<int>(1,Min<int>(mSCALE,P));

        Pu = LinearUpdate( P, 0, M_mb_wr, M_mb_mw );
        Pu = Max<int>(1,Min<int>(mSCALE,Pu));
        mb_price[i][0][0] = (LOG2(SCALE)-LOG2(Pu));
        mb_price[i][0][1] = (LOG2(SCALE)-LOG2(SCALE-Pu));
        mb_price[i][0][2] = Pu;

        Pu = LinearUpdate( P, 1, M_mb_wr, M_mb_mw );
        Pu = Max<int>(1,Min<int>(mSCALE,Pu));
        mb_price[i][1][0] = (LOG2(SCALE)-LOG2(Pu));
        mb_price[i][1][1] = (LOG2(SCALE)-LOG2(SCALE-Pu));
        mb_price[i][1][2] = Pu;
//      f_maxblk_0 = (LOG2(SCALE)-LOG2(P));
//      f_maxblk_1 = (LOG2(SCALE)-LOG2(SCALE-P));                
      }


      // blklen loop
      for( i=1; i<=maxblk; i++ ) {
        blklen = i*8;
        e0 = CalcPrice0(*this,i,log2maxblk);
        e0_price[i-1] = e0;

//printf( "%lf,", double(e0)/65536 );

        j = 4*(i-1)*(i-1)+5*(i-1);
        for( c0=0; c0<=blklen; c0++ ) {
          c1 = blklen-c0;

          e1 = CalcPrice1(*this,i,c0,c1,log2maxblk, f_compflag_2);
          e1_price[j+c0] = e1;
//          e2 = CalcPrice1(*this,i,c0,c1,log2maxblk, f_compflag_2);
//          e2_price[j+c0] = e2;
        }
      }
    }

    inppos = 0;
    return 0;
  }

  void do_process( void ) {
    rc_Init();
    if( f_DEC==0 ) do_encode(); else do_decode();
    rc_Quit(); 
    yield(this,0);
  }

#include "codec.inc"

  void do_encode( void ) {
    uint c,l,r,n,i,j,bit,p,blklen=0;
    qword curpos;

    inpbegptr = &inpbuf[0];
    inpendptr = &inpbuf[0];

    tokendpos = 0; // tok_array should be filled from inppos to tokendpos
    goalpos = 0;  
    s_goalpos = -1ULL; // goalpos for which len_array is filled atm
    goalptr = 0; // len_array pointer

    {
      word (&p)[2] = pop_array[(tokendpos+tokblk-0)%tokblk];
      p[0] = p[1] = 0;
    }

    uint l_quit = 0;
    uint n_kills;

    while(1) {

      // shift window to buf start
      if( inpbegptr>=inpbuf+inpbufhalf ) {
        l = inpendptr-inpbegptr;
        if( l>0 ) memcpy( inpbuf, (byte*)inpbegptr, l );
        inpbegptr = inpbuf+0;
        inpendptr = inpbuf+l;
      }

      // if there's place in inpbuf, read more from external input
      while( (l_quit==0) && ((n=inpbuf+inpbuftotal-inpendptr)>0) ) {
        chkinp(); l=getinplen(); if( l==0 ) { l_quit=1; f_quit=1; break; }
        l = Min<uint>( l, n );
        memcpy( (void*)inpendptr, (void*)inpptr, l ); inpptr+=l; inpendptr += l;
      }
      // if there's no more data, just quit
      if( l_quit && (inpbegptr>=inpendptr) ) { r=0; break; }

//if( !(inpbegptr+tokendpos-inppos + (l_quit?0:inpbufsize) > inpendptr) ) {
//printf( "{ %i\n", GetTickCount() );
//printf( "%I64i, ", tokendpos );

      // fill tok_array
      while(1) {
        if( inpbegptr+tokendpos-inppos + (l_quit?0:0) > inpendptr ) break;
        if( tokendpos-inppos >= optblk+1 ) break;

        c = (inpbegptr-1)[tokendpos-inppos];
        word (&p)[2] = pop_array[(tokendpos+tokblk-0)%tokblk];
        pop_counter[0] += 8-POP2( c );
        pop_counter[1] += 8-POP2( GRAY.LUT[c] );
        p[0] = pop_counter[0];
        p[1] = pop_counter[1];
//printf( "curpos=%I64i p0=%02X\n", tokendpos-1, p[0] );

        tok_array[ tokendpos%tokblk ].Compute( inpbegptr, inpendptr, tokendpos, *this );
        tokendpos++;
      }

//printf( "%I64i\n", tokendpos );
//printf( "} %i\n", GetTickCount() );
//}

      if( (goalpos+1<tokendpos) && (goalpos<inppos+goaldiff) ) goalpos=0;
      if( (goalptr==0) || (s_goalpos!=goalpos) ) {
        uint len;

Reparse:
        n_kills = 0;
        goalptr = 0;

//printf( "Reparse: inppos=%I64i/%I64i goalpos=%I64i s_goalpos=%I64i blklen=%i goalptr=%i pos=%.3lf\n", inppos,tokendpos,goalpos,s_goalpos,blklen,goalptr, double(tok_array[inppos%tokblk].price)/4096/8 ); fflush(stdout);

        if( goalpos<inppos+goaldiff ) goalpos=0;

        curpos = (goalpos==0) ? tokendpos-1 : goalpos;
        while(1) {
          token& T = tok_array[curpos%tokblk]; len=T.blklen;
          len_array[goalptr++] = len;
//printf( "inppos=%I64i curpos=%I64i len=%i\n", inppos, curpos, len );
          if( sqword(curpos-len)<=sqword(inppos) ) break;
          curpos -= len;
          if( goalpos==0 ) goalpos = curpos; // skip tail (potentially incomplete)
        }
        if( (goalpos==0) || (goalpos<inppos+goaldiff) ) goalpos = tokendpos-1;

        // Update tok_array if the path changed due to goalpos updates or something
        if( sqword(curpos-len)<sqword(inppos) ) {
          // check tok_array; kill tokens that lead below inppos
          for( curpos=inppos+1; curpos<goalpos; curpos++ ) {
            token& T1 = tok_array[curpos%tokblk];
            token& T0 = tok_array[(curpos-T1.blklen)%tokblk];
            if( (T1.pos!=curpos) || (T0.pos<inppos) || (T0.pos!=curpos-T1.blklen) ) T1.pos=-1ULL, n_kills++;
          }
          // recompute the killed tokens
          for( curpos=inppos+1; curpos<goalpos; curpos++ ) {
            token& T1 = tok_array[curpos%tokblk];
            if( T1.pos==token::pos_empty ) tok_array[curpos%tokblk].Compute( inpbegptr, inpendptr, curpos, *this );
          }
          goalpos=0;
//getc(stdin);
          goto Reparse;
        }

//        len = curpos-inppos;
//        if( len>maxblk ) printf( "!!! backtrack error: len=%i !!!\n", len );
//        blklen = len;
        s_goalpos = goalpos;
      }

      blklen = len_array[--goalptr];

//printf( "inppos=%I64i goalpos=%I64i s_goalpos=%I64i blklen=%i goalptr=%i\n", inppos,goalpos,s_goalpos,blklen,goalptr); fflush(stdout);

      process_EOF(0);

//printf( "%.3lf\n", double(tok_array[(inppos+blklen)%tokblk].price)/4096/8 );
//printf( "blklen=%i\n", blklen );

      encode_block( inpbegptr, blklen, tok_array[(inppos+blklen)%tokblk].f_compflag ); 

//printf( "%X %X; ", p_maxblk, tok_array[(inppos+0)%tokblk].p_maxlen );

      p_maxblk = LinearUpdate( p_maxblk, (blklen!=maxblk), M_mb_wr, M_mb_mw );

//printf( "%X %X\n", p_maxblk, tok_array[(inppos+blklen)%tokblk].p_maxlen );

      inpbegptr+=blklen; inppos+=blklen; 


    } // mainloop

    // encode EOF
    process_EOF(1);

  } // do_encode


  void do_decode( void ) {
    uint l_quit = 0, blklen=0;
    while(1) {
      if( process_EOF() ) break;
      blklen = decode_block(); //if( blklen==0 ) break;
      p_maxblk = LinearUpdate( p_maxblk, (blklen!=maxblk), M_mb_wr, M_mb_mw );
    }
  } // do_decode

};
