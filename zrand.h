// @ZBS {
//		*MODULE_OWNER_NAME zrand
// }

#ifndef ZRAND_H
#define ZRAND_H

void zrandSeed( int seed );
int zrandSeedFromTime();

// Linear Generators (return v where minV <= v < maxV)
// All of these will swap minV and maxV if they are in the wrong order
//------------------------------------------------------------------------

// The following use the generic stdlib random number generator (7-bit)
// These are approximately twice as fast as the higher-quality version
int    zrandFastI( int minV, int maxV );
float  zrandFastF( float minV=0.f, float maxV=1.f );
double zrandFastD( double minV=0.0, double maxV=1.0 );

// The following use a high-quality MT19937 random number generator
// See the CPP file for Gnu License information
int    zrandI( int minV, int maxV );
float  zrandF( float minV=0.f, float maxV=1.f );
double zrandD( double minV=0.0, double maxV=1.0 );

// The following uses the high-quality random number generator
// and a map of biases so that the distribution will follow the bias.
// sum of -1 means it will compute the sum for you
int zrandBiasI( int *map, int count, int sum=-1 );

// Gaussian Generators (return v where 0.f <= v < 1.f )
//------------------------------------------------------------------------
float zrandGaussianF();
	// Returns guassians centered on 0 with a stddev = 1

float zrandGaussianNormalizedF( float scale=0.133f );
	// Return "guassian" centered on 0.5 and scaled 

// Power-law / Zipf distribution
double zrandZipf( double alpha=1.8 );

int zrandNRI( int minV, int maxV );

#endif


