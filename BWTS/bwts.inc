
//#define malloc x1x
//#define calloc x2x
//#define free   x3x

#define chr(i) (cs==sizeof(int) ? ((int*)T)[i] : ((byte*)T)[i])
#define isLMS(_a) ((bvec_get(F1,(_a))!=0) || ((bvec_get(F2,(_a))!=0) && (bvec_get(F2,(_a)-1)==0)))

struct bwts {

  uint mode; // 0=forward, 1=inverse
  uint inplen; // block size
  uint F_size; // n/8+1 value
  uint alc_ptr; // local alloc pointer
  uint alcsize; // local alloc size

  byte* alcbuf; // local allocation buf, for lw_suffixsort()
  byte* inpbuf; // = new byte[inplen]; if( inpbuf==0 ) return 1;
  byte* outbuf; // = new byte[inplen]; if( outbuf==0 ) return 1;
  int * ptrtbl; // = new int [inplen]; if( ptrtbl==0 ) return 1;
  byte* V; // [inplen], for unbwts()
  byte* F; // F = (byte*)calloc( (n/8+1), sizeof(byte) ); for bwts()

  byte* alc_malloc( uint len ) {
    byte* r = 0;
    if( alc_ptr+len<=alcsize ) { r=&alcbuf[alc_ptr]; alc_ptr+=len; }
//printf( "alloc %i/%i: r=%I64X\n", len,alcsize, r );
    return r;
  }

  void alc_free( void* ptr ) {}

  byte* alc_calloc( uint num, uint size=1 ) {
    uint len = num*size;
    byte* r = alc_malloc( len );
    if( r ) memset( r, 0, len );
    return r;
  }

  uint init( uint _mode, uint _inplen ) {
    mode = _mode; inplen=_inplen; F_size = inplen/8+1; alcsize = (inplen<(1<<17))?(1<<22):inplen*2;
    alcbuf=0; inpbuf=0; outbuf=0; ptrtbl=0; V=0; F=0;
    alcbuf = new byte[alcsize]; if( alcbuf==0 ) return 1;
    inpbuf = new byte[inplen]; if( inpbuf==0 ) return 1;
    outbuf = new byte[inplen]; if( outbuf==0 ) return 1;
    ptrtbl = new int [inplen]; if( ptrtbl==0 ) return 1;
    //V      = new byte[inplen]; if( V==0 ) return 1;
    V = alcbuf;
    F      = new byte[F_size]; if( F==0 ) return 1;
    return 0;
  }

  void quit( void ) {
//printf( "alc_ptr=%i\n", alc_ptr );
    if( alcbuf ) { delete[] alcbuf; alcbuf=0; }
    if( inpbuf ) { delete[] inpbuf; inpbuf=0; }
    if( outbuf ) { delete[] outbuf; outbuf=0; }
    if( ptrtbl ) { delete[] ptrtbl; ptrtbl=0; }
    //if( V ) { delete[] V; V=0; }
    if( F ) { delete[] F; F=0; }
  }



  uint process( void ) {
    int r = 0;
    alc_ptr = 0; // reset local alloc pool for next run
    if( mode==0 ) {
      memset( alcbuf, 0, alcsize );
      memset( ptrtbl, 0, sizeof(int)*inplen );
      r = obwt_bwts( inpbuf, outbuf, ptrtbl, inplen );
    } else {
      r = obwt_unbwts( inpbuf, outbuf, ptrtbl, inplen );
    }
    return r;
  }

  int bvec_get( byte* B, int i ) {
    return ( B[i>>3] >> (i&7) ) & 1;
  }

  void bvec_set( byte* B, int i, int b ) {
    if( b ) {
      B[i>>3] |= byte(  1U<<(i&7) );
    } else {
      B[i>>3] &= byte(~(1U<<(i&7)));
    }
  }

  uint bvec_next( byte* B, uint i ) {
    uint j;
    uint c;
    i += 1;
    j = i >> 3;
    c = B[j] >> ( i& 7 );
    if( c==0 ) {
      i += 8-(i&7);
      j += 1;
      for(; (c=B[j])==0; j++, i+=8 );
    }
    for(; (c&1)==0; i++, c>>=1 );
    return i;
  }

  int bvec_prev( byte* B, int i ) {
    int j;
    uint c;
    i -= 1;
    if ( 0 <= i ) {
      j = i >> 3;
      c = ( B[j] << ( 7-( i& 7 ) ) )& 0xff;
      if ( c == 0 ) {
        i -= ( i& 7 ) + 1;
        j -= 1;
        for ( ; ( 0 <= j ) && ( ( c = B[j] ) == 0 ); --j, i -= 8 ){}
        if ( c == 0 ) {
          c = 128;
        }
      }
      for ( ; ( c& 128 ) == 0; --i, c <<= 1 ){}
    }
    return i;
  }


  void getCounts( byte* T, int* C, int n, int k, int cs ) {
    int i;
    for( i=0; i<k; ++i ) C[i]=0;
    for( i=0; i<n; ++i ) ++C[chr(i)];
  }

  void getBuckets( int* C, int* B, int k, int end ) {
    int i, sum = 0;
    if ( end ) {
      for( i=0; i<k; ++i ) {
        sum += C[i];
        B[i] = sum;
      }
    } else {
      for( i=0; i<k; ++i ) {
        sum += C[i];
        B[i] = sum - C[i];
      }
    }
  }


  void induceSA( byte* T, int* SA, int* C, int* B, byte* F1, byte* F2, byte* F3, int n, int k, int cs ) {
    int i, j, p;

    if( C==B ) getCounts( T, C, n, k, cs );
    getBuckets( C, B, k, 0 );

    p = bvec_prev( F3, n );

    for( i=0; i<n; ++i ) {
   
      while( (p>=0) && (B[chr(p)]==i) ) {
        j = bvec_next( F1, p ) - 1;
        SA[B[chr(j)]++] = j;
        p = bvec_prev( F3, p );
      }
      j = SA[i];
      if( j<0 ) continue;

      if( bvec_get(F1,j)==0 ) j-=1;
      else j=bvec_next(F1,j)-1;

      if( bvec_get(F2,j)==0 ) SA[B[chr(j)]++]=j;
    }

    if( C==B ) getCounts( T, C, n, k, cs );

    getBuckets( C, B, k, 1 );

    for( i=n-1; i>=0; i-- ) {
      j = SA[i];
      if( j<0 ) continue;
      if( bvec_get(F1,j)==0 ) if( bvec_get(F2,--j)!=0 ) SA[--B[chr(j)]]=j;
    }
  }

  int lw_suffixsort( byte* T, int* SA, byte* F1, int fs, int n, int k, int cs ) {
//printf( "lw_suffixsort: T=%I64X n=%i\n", T, n );

    byte* F2, * F3, * F4;
    int* C, * B, * RA;
    int i, j, c, m, p, q, plen, qlen, name;
    int c0, c1;
    int diff;

    F2 = alc_calloc( (n+7)/8 );
    F3 = alc_calloc( (n/8+1) );
    if( (F2==0) || (F3==0) ) { alc_free(F2); alc_free(F3); return -2; }

    for ( p = 0; p < n; p = q ) {
      q = bvec_next( F1, p );
      for ( i = q - 2, c = 0, c1 = chr( q - 1 ), m = 0; p <= i; --i, c1 = c0 ) {
        bvec_set( F2, i + 1, c );
        if ( ( c0 = chr( i ) ) < ( c1 + c ) ) {
          c = 1;
        } else if ( c != 0 ) {
          m += 1;
          c = 0;
        }
      }
      bvec_set( F2, p, 1 );
      if ( ( m == 0 ) && ( c == 0 ) ) {
        bvec_set( F3, p, 1 );
      }
    }
    bvec_set( F3, n, 1 );

    if ( k <= fs ) {
      C = SA + n;
      B = ( k <= ( fs - k ) ) ? C + k: C;
    } else {
      B = (int*)alc_malloc( k*sizeof(int) ); C=B;
      if( C==0 ) { alc_free( F2 ); alc_free( F3 ); return -2; }
    }

    getCounts( T, C, n, k, cs );
    getBuckets( C, B, k, 1 );
    for( i=0; i<n; ++i ) SA[i] =  - 1;

    for( i=0; i<n; ++i ) {
      if ( isLMS( i ) && ( bvec_get( F3, i ) == 0 ) ) {
        SA[--B[chr( i )]] = i;
      }
    }

    induceSA( T, SA, C, B, F1, F2, F3, n, k, cs );

    if( fs<k ) alc_free( C );

    for ( i = 0, m = 0; i<n; ++i ) {
      p = SA[i];
      if ( isLMS( p ) && ( bvec_get( F3, p ) == 0 ) ) {
        SA[m++] = p;
      }
    }

    for( i=m; i<n; ++i ) SA[i] = 0;

    for ( p = 0; p < n; p = q ) {
      q = bvec_next( F1, p );
      for ( i = q - 2, c = 0, c1 = chr( q - 1 ), j = q; p <= i; --i, c1 = c0 ) {
        if ( ( c0 = chr( i ) ) < ( c1 + c ) ) {
          c = 1;
        } else if ( c != 0 ) {
          SA[m + ( ( i + 1 ) >> 1 )] = j - i - 1;
          j = i + 1;
          c = 0;
        }
      }
      if ( ( j < q ) || ( c != 0 ) ) {
        SA[m + ( p >> 1 )] = j - p;
      }
    }

    for ( i = 0, name = 0, q = n, qlen = 0; i<m; ++i ) {
      p = SA[i], plen = SA[m + ( p >> 1 )], diff = 1;
      if ( plen == qlen ) {
        for ( j = 0; ( j < plen ) && ( chr( p + j ) == chr( q + j ) ); j++ ){}
        if ( j == plen ) {
          diff = 0;
        }
      }
      if ( diff != 0 ) {
        ++name, q = p, qlen = plen;
      }
      SA[m + ( p >> 1 )] = name;
    }

    if ( name < m ) {
      F4 = alc_calloc( (m/8+1) );
      if( F4==NULL ) { alc_free(F2); alc_free(F3); return -1; }

      for ( i = 0, j = 0; i<n; ++i ) {
        if ( isLMS( i ) && ( bvec_get( F3, i ) == 0 ) ) {
          bvec_set( F4, j++, bvec_get( F1, i ) );
        }
      }
      bvec_set( F4, m, 1 );
      RA = SA + n + fs - m;
      for ( i = n - 1, j = m - 1; m <= i; --i ) {
        if ( SA[i] != 0 ) {
          RA[j--] = SA[i] - 1;
        }
      }

      if( lw_suffixsort( ( byte* )RA, SA, F4, fs + n - m * 2, m, name, sizeof(int) ) != 0 ) {
        alc_free( F2 ); alc_free( F3 ); alc_free( F4 ); return -2;
      }

      alc_free( F4 );

      for ( i = 0, j = 0; i<n; ++i ) {
        if( isLMS(i) && (bvec_get(F3,i)==0) ) RA[j++] = i;
      }
      for( i=0; i<m; ++i ) SA[i] = RA[SA[i]];
    }

    if( k<=fs ) {
      C = SA + n;
      B = ( k <= ( fs - k ) ) ? C + k: C;
    } else {
      B = (int*)alc_malloc( k*sizeof(int) ); C=B;
      if( C==0 ) { alc_free( F2 ); alc_free( F3 ); return -2; }
    }

    getCounts( T, C, n, k, cs );
    getBuckets( C, B, k, 1 );

    for( i=m; i<n; i++ ) SA[i]=-1;

    for ( i = m - 1; 0 <= i; --i ) {
      j = SA[i], SA[i] =  - 1;
      SA[--B[chr(j)]] = j;
    }
    induceSA( T, SA, C, B, F1, F2, F3, n, k, cs );

    if( fs<k ) alc_free( C );

    alc_free( F2 );
    alc_free( F3 );
    return 0;
  }

  int obwt_bwts( byte* T, byte* U, int* A, int n ) {
    //byte* F;
    int i, j, k, p, q;
    int c1, c2;

    if( (T==0) || (U==0) || (A==0) || (n<0) ) return -1;

    if( n<=1 ) { if( n==1 ) U[0] = T[0]; return 0; }

    //F = (byte*)calloc( (n/8+1), sizeof(byte) ); if( F==NULL ) return -2;
    memset( F, 0, F_size );

    for( i=0, j=k=1; j<=n; ) {
      for ( p = i, q = j;; ) {
        c1 = ( p < n ) ? T[p]:  - 1;
        c2 = ( q < n ) ? T[q]:  - 1;
        ++p, ++q;
        if ( c1 != c2 ) {
          break;
        }
        if ( p == j ) {
          k = q;
          p = i;
        }
      }
      if ( c1 < c2 ) {
        j = k = q;
      } else {
        bvec_set( F, i, 1 );
        i = k;
        j = ( k += 1 );
      }
    }
    bvec_set( F, n, 1 );

    if( lw_suffixsort( T, A, F, 0, n, 256, sizeof(byte)) != 0 ) {
      //free( A );
      //free( F );
      return  - 2;
    }

    if ( T == U ) {
      for( i=0; i<n; ++i ) {
        p = A[i];
        if ( bvec_get( F, p ) == 0 ) {
          p -= 1;
        } else {
          p = bvec_next( F, p ) - 1;
        }
        c1 = T[i];
        U[i] = byte( ( i <= p ) ? T[p]: A[p] );
        A[i] = c1;
      }
    } else if ( ( ( T < U ) && ( U < ( T + n ) ) ) || ( ( U < T ) && ( T < ( U + n ) ) ) ) {
      for( i=0; i<n; ++i ) {
        p = A[i];
        if ( bvec_get( F, p ) == 0 ) {
          p -= 1;
        } else {
          p = bvec_next( F, p ) - 1;
        }
        A[i] = T[p];
      }
      for( i=0; i<n; ++i ) {
        U[i] = ( byte )A[i];
      }
    } else {
      for( i=0; i<n; ++i ) {
        p = A[i];
        if ( bvec_get( F, p ) == 0 ) {
          p -= 1;
        } else {
          p = bvec_next( F, p ) - 1;
        }
        U[i] = T[p];
      }
    }

    //free( F );

    return 0;
  }


  int obwt_unbwts( byte* T, byte* U, int* LF, int n ) {
    int C[256];
    int i, j, p, t;

    if( (T==0) || (U==0) || (LF==0) || (n<0) ) return -1;

    if( n<=1 ) { if( n==1 ) U[0]=T[0]; return 0; }

//    V  = new byte[n];

    if( LF && V ) {

      for( i=0; i<256; ++i ) C[i] = 0;

      for( i=0; i<n; ++i ) C[V[i] = T[i]] += 1;

      for( i=0, j=0; i<256; ++i ) { t=C[i]; C[i]=j; j+=t; }

      for( i=0; i<n; ++i ) LF[i] = C[V[i]]++;

      for( i=0, j=n-1; j>=0; ++i ) {
        if( LF[i]>=0 ) {
          p = i;
          do {
            U[j--] = V[p];
            t = LF[p];
            LF[p]=-1;
            p = t;
          } while( LF[p]>=0 );
        }
      }
    }

//    if( V ) delete[] V;

    return 0;
  }

};

#undef chr
#undef isLMS

