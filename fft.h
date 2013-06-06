// @ZBS {
//		*MODULE_OWNER_NAME fft
// }

// This is a modified version.  See history in fft.cpp

#ifndef FFT_H
#define FFT_H

#if defined (_MSC_VER)
#pragma pack (push, 8)
#endif

#include "math.h"
#include "assert.h"

typedef float flt_t;
	// Change this typedef to use a different floating point type in your FFTs (i.e. float, double or long double).

class FFTReal {
  public:
	explicit FFTReal( const long length );
	~FFTReal ();
	void doFFT( flt_t f [], const flt_t x [] ) const;
	void doIFFT( const flt_t f [], flt_t x [] ) const;
	void rescale( flt_t x []) const;
	int getOrderedCoeffs( flt_t f[], flt_t reals[], flt_t imags[] );
		// This method stuffs the coefficients in order into the reals and imags arrays
		// The buffers must be at least length / 2 - 1
		// returns _length / 2 - 1

  protected:
	// Bit-reversed look-up table nested class
	class BitReversedLUT {
	  private:
		long *_ptr;
	  public:
		explicit BitReversedLUT( const int nbr_bits );
		~BitReversedLUT();
		const long *get_ptr() const {
			return _ptr;
		}
	};

	// Trigonometric look-up table nested class
	class TrigoLUT {
	  private:
		flt_t *_ptr;
	  public:
		explicit TrigoLUT( const int nbr_bits );
		~TrigoLUT();
		const flt_t	*get_ptr( const int level ) const {
			return( _ptr + (1L << (level - 1)) - 4 );
		};
	};

	const BitReversedLUT _bit_rev_lut;
	const TrigoLUT _trigo_lut;
	const flt_t _sqrt2_2;
	const long _length;
	const int _nbr_bits;
	flt_t *_buffer_ptr;
};

//////////////////////////////////////////////////////////////////////////
// FFTRealNonPower2
// Added by ZBS
//////////////////////////////////////////////////////////////////////////

struct FFT {
	// This class is less optimized but more general than FFTReal
	// It manages the buffers necessary in order to permit
	// clean non-power of 2 lengths.

	int numTimeSamples;
		// Actual number of samples in the input
	int pow2Size;
		// The size of the pow2 input buffer
	int pow2Bits;
		// Log base 2 of pow2Size

	float *pow2Time;
		// Pointer to the internally allocated pow2 input buffer
	float *pow2Freq;
		// Pointer to the internally allocated pow2 ouput buffer

	float *pow2PowerSpectrum;
		// Optionally allocated: Pointer to internally allocated power spectrum
	float *pow2PhaseSpectrum;
		// Optionally allocated: Pointer to internally allocated phase spectrum
	int powerSpectrumOwner;
		// True if the power specturm should be freed on clear

	static float *hanningLookup;
	static int hanningLookupSize;
		// Hanning lookup table is allocated and cached.  
		// Use clearStaticLookupTables to clear it


	// GET sizes
	//-------------------------------------------
	int getPow2Size() { return pow2Size; }
		// Returns the size of the pow2 buffers

	int getNumFreqCoefs() { return pow2Size / 2 + 1; }
		// Number of Fourier coefficients

	static int getNumFreqCoefsFromTime( int numTimeSamples );
		// Useful when you need to pre allocate buffers based on the num coefs of sometime time signal

	// FETCH / PUT time samples
	//-------------------------------------------
	float getPow2TimeSample( int i ) {
		assert( i >= 0 && i < pow2Size );
		return pow2Time[i];
	}

	void putPow2TimeSample( int i, float f ) {
		assert( i >= 0 && i < pow2Size );
		pow2Time[i] = f;
	}

	void zeroTime();
		// Fill whole buffer with zero

	float *getTimeCenterPtr();
		// Fetch the destination location for the centered signal

	void smoothTime();
		// Applies hanning window to time buffer

	void loadTimeSamples( float *signal );
		// Copies the signal of numTimeSamples length into the 
		// center of the pow2Time buffer and zeros out the edges
		// and then smoothes


	// FETCH / PUT Fourier coefficients
	//-------------------------------------------
	void zeroFreq();

	float getFreqCoefReal( int i ) {
		assert( i >= 0 && i < getNumFreqCoefs() );
		return pow2Freq[i];
	}

	float getFreqCoefImag( int i ) {
		assert( i >= 0 && i < getNumFreqCoefs() );
		if( i == 0 || i == pow2Size/2 ) return (float)0.f;
		return pow2Freq[pow2Size/2+i];
	}

	void getFreqCoef( int i, float &real, float &imag ) {
		assert( i >= 0 && i < getNumFreqCoefs() );
		real = getFreqCoefReal( i );
		imag = getFreqCoefImag( i );
	}

	void putFreqCoefReal( int i, float real ) {
		assert( i >= 0 && i < getNumFreqCoefs() );
		pow2Freq[i] = real;
	}

	void putFreqCoefImag( int i, float imag ) {
		assert( i >= 0 && i < getNumFreqCoefs() );
		if( i > 0 && i < pow2Size/2 ) {
			pow2Freq[pow2Size/2+i] = imag;
		}
	}

	void putFreqCoef( int i, float real, float imag ) {
		putFreqCoefReal( i, real );
		putFreqCoefImag( i, imag );
	}


	float getFreqCoefPower( int i ) {
		assert( i >= 0 && i < getNumFreqCoefs() );
		float re = getFreqCoefReal(i);
		float im = getFreqCoefImag(i);
		return re*re + im*im;
	}

	float getFreqCoefPhase( int i ) {
		float re = getFreqCoefReal(i);
		float im = getFreqCoefImag(i);
		return atan2f( im, re );
	}


	// CALCUATE
	//-------------------------------------------
	void fft();
		// Forward FFT from pow2Time to the pow2Freq

	void ifft();
		// Inverse FFT from pow2Freq to the pow2Time

	float *computePowerSpectrum();
		// Assumes that the pow2Freq is valid
		// Allocate memory for a power spectrum and compute it. Returns that pointer
		// which will be freed on destruct or clear()

	float *computePhaseSpectrum();
		// Assumes that the pow2Freq is valid
		// Allocate memory for a phase spectrum and compute it. Returns that pointer
		// which will be freed on destruct or clear()


	// UNIT CONVERSION
	//-------------------------------------------
	float getHertzFromFreqCoef( int i, float samplesPerSecond ) {
		assert( i >= 0 && i < getNumFreqCoefs() );
		return (float)i / (float)pow2Size * samplesPerSecond;
	}

	float getOmegaFromFreqCoef( int i, float samplesPerSecond ) {
		assert( i >= 0 && i < getNumFreqCoefs() );
		return 2.f * 3.1415926539f * getHertzFromFreqCoef( i, samplesPerSecond );
	}

	int getFreqCoefFromHertz( float hertz, float samplesPerSecond ) {
		int i = (int)( 2.f * (float)pow2Size * hertz / samplesPerSecond );
		assert( i >= 0 && i < getNumFreqCoefs() );
	}

	int getFreqCoefFromOmega( float omega, float samplesPerSecond ) {
		return getFreqCoefFromHertz( omega / (2.f * 3.1415926539f), samplesPerSecond );
	}

	// ALLOC, CLEAR, CONSTRUCT, DESTRUCT
	//-------------------------------------------
	void alloc( int actualNumTimeSamples );
		// This takes a non power of 2 number of samples
		// and builds all the necessary power of two buffers
		// Clears first.

	void clear();
		// Free all buffers

	static void clearStaticCachedLookupTables();
		// Free any static lookup buffers that were allocated

	FFT();
		// Does not allocate anything

	FFT( int _size );
		// Allocate the pow2 time and freq buffers but does't copy

	FFT( float *timeSignal, int actualNumTimeSamples );
		// Allocate the pow2 time and freq buffers and then timeSignal 
		// into the center of the pow2Time buffer and smooth.
		// Same as calling alloc(), loadTimeSamples()

	~FFT();
		// Calls clear, deallocaing all buffers except the static cache
};

float *fftPowerSpectrum( float *signal, int count, int stride, int &outputSize );
	// Stride is in bytes. Stride==0 means tightly packed so it is converted to sizeof(float)
	// The return array is malloced, free it when done

#if defined (_MSC_VER)
#pragma pack (pop)
#endif

#endif

