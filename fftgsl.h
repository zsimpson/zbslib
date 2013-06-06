// @ZBS {
//		*MODULE_OWNER_NAME fftgsl
// }

#ifndef FFTGSL_H
#define FFTGSL_H

struct FFTGSL {
	// A wrapper for the various GSL rouintes for FFT
	// This wrapper examines the size of the data and determines
	// which GSL function to call (if a power of two then it
	// can use the fast radix 2 algorithms).
	//
	// It provides a single interface for fetching arguments
	// which are in different orders depending on which algorithm is used
	//
	// These routes will cache the wavetable data from the last call
	// so you can safely make repetitive calls for the same sized data.
	// Note that this cache is a per-instance cache so reuse
	// the same instance for any given size

	void *wavetableCache;
	void *workspaceCache;
	int count;
	int resultsReal;
		// When this flag is true, the results array is all real and is cacheCount long
		// When is it false, it is in half-complex format
	double *results;

	int numCoef() {
		return resultsReal ? count : ( (count&1) ? count/2 : count/2+1 );
	}

	int isPow2( int x );

//	int fftF( float *data=0, int count=0, int stride=sizeof(float) );
	int fftD( double *data=0, int count=0, int stride=sizeof(double) );
		// Perform forward FFT on real data array of count length with given byte stride.
		// If data and count are zero then it assumes that you have previously
		// called the ifft function and the results array contains the
		// data you wish to transform
		// Returns the GSL error codes

//	int ifftF( float *data=0, int count=0, int stride=sizeof(float) );
	int ifftD( double *data=0, int count=0, int stride=sizeof(double) );
		// These inverse take UNPACKED data meaning that there
		// is a real/imag pair for EVERY count 
		// unless you pass in 0 for data in which case it knows that
		// this is the result of a previous forward transform
		// and it knows that it is actually in packed form (in results)
		// Returns the GSL error codes

	void flushCache();

	double getRealD( int i );
	double getImagD( int i );
	void getComplexD( int i, double &real, double &imag );
		// Fetch the i-th component.  These routines
		// hide all of the order rearrangement complexities
		// of these routines.

	float getRealF( int i );
	float getImagF( int i );
	void getComplexF( int i, float &real, float &imag );
		// These call the above and then typecast to float

	void putComplexD( int i, double real, double imag );

	FFTGSL();
	~FFTGSL();
};

#endif

