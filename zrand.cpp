// @ZBS {
//		*MODULE_NAME Random number generators
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A collection of random number generators
//		}
//		*PORTABILITY win32 unix macosx
//		*REQUIRED_FILES zrand.cpp zrand.h
//		*VERSION 1.0
//		+HISTORY {
//			1.0 Consolated the rand generators from goodrand.cpp and zmathtools.cpp
//				The high-quality random number generators come from GPL code.  See copyright below
//		}
//		*SELF_TEST no
//		*PUBLISH yes
// }

// OPERATING SYSTEM specific includes:
#ifdef WIN32
	extern "C" {
		typedef union _LARGE_INTEGER {
			struct { unsigned long LowPart; long HighPart; };
			struct { unsigned long LowPart; long HighPart; } u;
			__int64 QuadPart;
		} LARGE_INTEGER;
		__declspec(dllimport) int __stdcall QueryPerformanceCounter( LARGE_INTEGER *lpPerformanceCount );
	}
#else
	#include "sys/time.h"
	#include "unistd.h"
#endif
// STDLIB includes:
#include "stdlib.h"
#include "math.h"
// MODULE includes:
#include "zrand.h"
// ZBSLIB includes:

#ifdef WIN32
typedef unsigned __int64 U64;
#endif
#ifdef __USE_GNU
typedef unsigned long long U64;
#endif
U64 nrv = 4101842887655102017LL;
int zrandNRI( int minV, int maxV ) {
	nrv ^= nrv >> 21; nrv ^= nrv << 25; nrv ^= nrv >> 4;
	nrv = nrv * 2685821657736338717LL;
	unsigned int bottom = 0xFFFFFFFF & nrv;
	return bottom % (maxV - minV) + minV;
}

int zrandBiasI( int *map, int count, int sum ) {
	if( sum < 0 ) {
		sum = 0;
		for( int i=0; i<count; i++ ) {
			sum += map[i];
		}
	}
	else if ( sum == 0 ) {
		// Empty map. Return rand between 0 and count-1
		return zrandI( 0, count );
	}

	int r = zrandI( 0, sum );
	int _sum = 0;
	for( int i=0; i<count; i++ ) {
		_sum += map[i];
		if( _sum > r ) {
			return i;
		}
	}
	return count-1;
}

int zrandFastI( int m0, int m1 ) {
	if( m1 < m0 ) {
		int t = m1;
		m1 = m0;
		m0 = t;
	}
	else if( m1 == m0 ) {
		return m0;
	}
	int f = m0 + rand() % (m1-m0);
	return f;
}

float zrandFastF( float m0, float m1 ) {
	if( m1 < m0 ) {
		float t = m1;
		m1 = m0;
		m0 = t;
	}
	float f = m0 + (m1-m0) * ( (float)(rand()&0x7fff) / (float)0x8000 );
	return f;
}

double zrandFastD( double m0, double m1 ) {
	if( m1 < m0 ) {
		double t = m1;
		m1 = m0;
		m0 = t;
	}
	double f = m0 + (m1-m0) * ( (double)(rand()&0x7fff) / (double)0x8000 );
	return f;
}

double zrandZipf( double alpha ) {
	double am1 = alpha - 1.0;
	double b = pow( 2.0, am1 );
	double c = -1.0 / am1;
	while( 1 ) {
		double u = (double)zrandF();
		double v = (double)zrandF();
		double r = floor( pow( u, c ) );
		double t = pow( (1.0 + 1.0/r), am1 );
		if( v*r*(t-1.0)/(b-1.0) <= t/b ) {
			return r;
		}
	}
}

float zrandGaussianF() {
	static float last;
	static int lastGood = 0;

	if( lastGood ) {
		lastGood = 0;
		return last;
	}

	float x1, x2, w, y1;
	do {
		x1 = 2.f * zrandF() - 1.f;
		x2 = 2.f * zrandF() - 1.f;
		w = x1 * x1 + x2 * x2;
	} while( w >= 1.f );
	w = sqrtf( (-2.f * logf( w )) / w );
	y1 = x1 * w;
	last = x2 * w;
	lastGood = 1;
	return y1;
}

float zrandGaussianNormalizedF( float scale ) {
	float f = zrandGaussianF();
	f = f*scale + 0.5f;
	if( f < 0.f ) f = 0.f;
	else if( f > 1.f ) f = 1.f;
	return f;
}

int zrandSeedFromTime() {
	int seed;

	#ifdef WIN32
		__int64 counter;
		QueryPerformanceCounter( (LARGE_INTEGER *)&counter );
		seed = (int)counter;
	#else
		struct timeval tv;
		struct timezone tz;
		gettimeofday( &tv, &tz );
		seed = tv.tv_usec;
	#endif
	
	zrandSeed( seed );
	return seed;
}

// The following copyright applies to all of the following code
//
// A C-program for MT19937: Real number version                
// genrand() generates one pseudorandom real number (double) 
// which is uniformly distributed on [0,1]-interval, for each  
// call. sgenrand(seed) set initial values to the working area 
// of 624 words. Before genrand(), sgenrand(seed) must be      
// called once. (seed is any 32-bit integer except for 0).     
// Integer generator is obtained by modifying two lines.       
//
// Coded by Takuji Nishimura, considering the suggestions by 
// Topher Cooper and Marc Rieffel in July-Aug. 1997.           
// This library is free software; you can redistribute it and/or   
// modify it under the terms of the GNU Library General Public     
// License as published by the Free Software Foundation; either    
// version 2 of the License, or (at your option) any later         
// version.                                                        
//
// This library is distributed in the hope that it will be useful, 
// but WITHOUT ANY WARRANTY; without even the implied warranty of  
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.            
// See the GNU Library General Public License for more details.    
// You should have received a copy of the GNU Library General      
// Public License along with this library; if not, write to the    
// Free Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA   
// 02111-1307  USA                                                 
//
// Copyright (C) 1997 Makoto Matsumoto and Takuji Nishimura.       
// Any feedback is very welcome. For any question, comments,       
// see http://www.math.keio.ac.jp/matumoto/emt.html or email
// matumoto@math.keio.ac.jp

// Period parameters
#define N 624
#define M 397
#define MATRIX_A 0x9908b0df
	// constant vector a
#define UPPER_MASK 0x80000000
	// most significant w-r bits
#define LOWER_MASK 0x7fffffff
	// least significant r bits

// Tempering parameters
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define TEMPERING_SHIFT_U(y)  (y >> 11)
#define TEMPERING_SHIFT_S(y)  (y << 7)
#define TEMPERING_SHIFT_T(y)  (y << 15)
#define TEMPERING_SHIFT_L(y)  (y >> 18)

static unsigned long mt[N];
	// the array for the state vector
static int mti=N+1;
	// mti==N+1 means mt[N] is not initialized


double zrandD( double minV, double maxV ) {
	unsigned long y;
	static unsigned long mag01[2]={0x0, MATRIX_A};

	// mag01[x] = x * MATRIX_A	for x=0,1
	// GENERATE N words at one time
	if( mti >= N ) {
		int kk;

		if( mti == N+1 ) {
			// if sgenrand() has not been called, a default initial seed is used
			zrandSeed( 4357 );
		}

		for (kk=0;kk<N-M;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1];
		}
		for (;kk<N-1;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
		}
		y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
		mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1];

		mti = 0;
	}
 
	y = mt[mti++];
	y ^= TEMPERING_SHIFT_U(y);
	y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
	y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
	y ^= TEMPERING_SHIFT_L(y);

	double rr = ( (double)y / (double)(unsigned int)0xffffffff );
	rr *= maxV - minV;
	rr += minV;
	return rr;
}

float zrandF( float minV, float maxV ) {
	unsigned long y;
	static unsigned long mag01[2]={0x0, MATRIX_A};

	// mag01[x] = x * MATRIX_A	for x=0,1
	// GENERATE N words at one time
	if( mti >= N ) { 
		int kk;

		if( mti == N+1 ) {
			// if sgenrand() has not been called, a default initial seed is used
			zrandSeed( 4357 );
		}

		for (kk=0;kk<N-M;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1];
		}
		for (;kk<N-1;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
		}
		y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
		mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1];

		mti = 0;
	}
 
	y = mt[mti++];
	y ^= TEMPERING_SHIFT_U(y);
	y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
	y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
	y ^= TEMPERING_SHIFT_L(y);

	float rr = ( (float)y / (float)(unsigned int)0xffffffff );
	rr *= maxV - minV;
	rr += minV;
	return rr;
}

int zrandI( int minV, int maxV ) {
	unsigned long y;
	static unsigned long mag01[2]={0x0, MATRIX_A};

	if (minV == maxV)
		return minV;

	// mag01[x] = x * MATRIX_A	for x=0,1 */
	// GENERATE N words at one time
	if( mti >= N ) { 
		int kk;

		if( mti == N+1 ) {
			// if sgenrand() has not been called, a default initial seed is used
			zrandSeed( 4357 );
		}

		for (kk=0;kk<N-M;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1];
		}
		for (;kk<N-1;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
		}
		y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
		mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1];

		mti = 0;
	}
 
	y = mt[mti++];
	y ^= TEMPERING_SHIFT_U(y);
	y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
	y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
	y ^= TEMPERING_SHIFT_L(y);

	y %= (maxV - minV);
	y += minV;
	return y;
}

void zrandSeed( int seed ) {
	// SEED the high quality system

	// Setting initial seeds to mt[N] using the generator Line 25 of Table 1 in
	// [KNUTH 1981, The Art of Computer Programming Vol. 2 (2nd Ed.), pp102]
	// initializing the array with a NONZERO seed 
	mt[0]= seed & 0xffffffff;
	for( mti=1; mti<N; mti++ ) {
		mt[mti] = (69069 * mt[mti-1]) & 0xffffffff;
	}

	// SEED the fast system
	srand( seed );
}
