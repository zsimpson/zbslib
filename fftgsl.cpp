// @ZBS {
//		*MODULE_NAME FFT GSL Wrapper
//		*SDK_DEPENDS gsl-1.8
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A wrapper around the various GSL FFT routines making
//			it easier to use non-packed arrays and have it decide
//			on which of the algorithms to use based on size
//		}
//		*PORTABILITY win32 unix macosx
//		*REQUIRED_FILES fftgsl.cpp fftgsl.h
//		*VERSION 1.0
//		+HISTORY {
//			1.0 ZBS First build 27 June 2007
//		}
//		+TODO {
//		}
//		*SELF_TEST yes console FFTGSL_SELF_TEST
//		*PUBLISH yes
// }


// OPERATING SYSTEM specific includes:
// SDK includes:
#include "gsl/gsl_fft_real.h"
#include "gsl/gsl_fft_complex.h"
// STDLIB includes:
#include "stdlib.h"
#include "assert.h"
// MODULE includes:
#include "fftgsl.h"
// ZBSLIB includes:


FFTGSL::FFTGSL() {
	wavetableCache = 0;
	workspaceCache = 0;
	count = 0;
	resultsReal = 0;
	results = 0;
}

FFTGSL::~FFTGSL() {
	flushCache();
}

int FFTGSL::isPow2( int x ) {
	int j = 1;
	for( int i=0; i<32; i++ ) {
		if( j == x ) {
			return 1;
		}
		j <<= 1;
	}
	return 0;
}

void FFTGSL::flushCache() {
	if( wavetableCache ) {
		gsl_fft_real_wavetable_free( (gsl_fft_real_wavetable*)wavetableCache );
		wavetableCache = 0;
	}
	if( workspaceCache ) {
		gsl_fft_real_workspace_free( (gsl_fft_real_workspace*)workspaceCache );
		workspaceCache = 0;
	}
	if( results ) {
		free( results );
		results = 0;
	}
	count = 0;
}

int FFTGSL::fftD( double *data, int _count, int stride ) {
	int err;

	if( !data ) {
		// Requesting that the previous results be used
		data = results;
		assert( ! _count );
		assert( resultsReal );
		assert( wavetableCache && workspaceCache );
	}
	else {
		if( _count != count ) {
			// If the cache is invalid, it must be reallocated
			flushCache();
			count = _count;
			results = new double[ count*2 ];
			wavetableCache = gsl_fft_real_wavetable_alloc( count );
			workspaceCache = gsl_fft_real_workspace_alloc( count );
		}

		// COPY the data into the results buffer which will also serve as input
		// since all the GSL FFT routines work in place.
		char *src = (char *)data;
		double *dst = results;
		for( int i=0; i<count; i++ ) {
			*dst++ = *(double *)src;
			src += stride;
		}
	}

	err = gsl_fft_real_transform( results, 1, count, (gsl_fft_real_wavetable*)wavetableCache, (gsl_fft_real_workspace*)workspaceCache );

	resultsReal = 0;
	return err;
}

int FFTGSL::ifftD( double *data, int _count, int stride ) {
	int err = 0;

	if( !data ) {
		// Requesting that the previous results be used
		data = results;
		assert( ! _count );
		assert( !resultsReal );
		assert( wavetableCache && workspaceCache );
	}
	else {
		if( _count != count ) {
			// If the cache is invalid, it must be reallocated
			flushCache();
			count = _count;
			results = new double[ count*2 ];
			wavetableCache = gsl_fft_complex_wavetable_alloc( count );
			workspaceCache = gsl_fft_complex_workspace_alloc( count );
		}

		// COPY the data into the results buffer which will also serve as input
		// since all the GSL FFT routines work in place.
		char *src = (char *)data;
		double *dst = results;
		for( int i=0; i<count; i++ ) {
			*dst++ = *(double *)src;
			src += stride;
		}
	}

	if( isPow2(count) ) {
		err = gsl_fft_complex_radix2_inverse( results, 1, count );
	}
	else {
// I can't tell what format all this shit is in!
//		err = gsl_fft_complex_inverse( results, 1, count, (gsl_fft_real_wavetable*)wavetableCache, (gsl_fft_real_workspace*)workspaceCache );
assert( 0 );
	}

	resultsReal = 1;
	return err;
}


double FFTGSL::getRealD( int i ) {
	if( resultsReal ) {
		assert( i >= 0 && i < count );
		return results[i];
	}
	else {
		if( count & 1 ) {
			// ODD
			if( i > count/2 ) {
				return +results[ (count-i)*2-1 ];
			}
			else if( i > 0 ) {
				return +results[ i*2-1 ];
			}
			else {
				return results[ 0 ];
			}
		}
		else {
			// EVEN
			if( i > count/2 ) {
				return +results[ (count-1-i)*2+1 ];
			}
			else if( i == count/2 ) {
				return +results[ (count-1-i)*2+1 ];
			}
			else if( i > 0 ) {
				return +results[ (i-1)*2+1 ];
			}
			else {
				return results[0];
			}
		}
	}
	return 0.0;
}

double FFTGSL::getImagD( int i ) {
	if( resultsReal ) {
		assert( i >= 0 && i < count );
		return results[i];
	}
	else {
		if( count & 1 ) {
			// ODD
			if( i > count/2 ) {
				return -results[ (count-i)*2   ];
			}
			else if( i > 0 ) {
				return +results[ i*2   ];
			}
			else {
				return 0.0;
			}
		}
		else {
			// EVEN
			if( i > count/2 ) {
				return -results[ (count  -i)*2   ];
			}
			else if( i == count/2 ) {
				return 0.0;
			}
			else if( i > 0 ) {
				return -results[ i*2 ];
			}
			else {
				return 0.0;
			}
		}
	}
	return 0.0;
}

void FFTGSL::getComplexD( int i, double &real, double &imag ) {
	if( resultsReal ) {
		assert( i >= 0 && i < count );
		real = results[i];
		imag = 0.0;
	}
	else {
		if( count & 1 ) {
			// ODD
			if( i > count/2 ) {
				real = +results[ (count-i)*2-1 ];
				imag = -results[ (count-i)*2   ];
			}
			else if( i > 0 ) {
				real = +results[ i*2-1 ];
				imag = +results[ i*2   ];
			}
			else {
				real = results[ 0 ];
				imag = 0.0;
			}

//			if i==0 then 0
//			if i>0 then i*2-1
//			if i>count/2 then (count-i)*2-1
//
//			real:
//			0 1 2 3 4
//			0 1 3 3 1
//
//			if i==0 then z
//			if i>0 then i*2
//			if i>count/2 then neg (count-i)*2
//
//			imag:
//			0 1 2  3  4
//			z 2 4 -4 -2
		}
		else {
			// EVEN
			if( i > count/2 ) {
				real = +results[ (count-1-i)*2+1 ];
				imag = -results[ (count  -i)*2   ];
			}
			else if( i == count/2 ) {
				real = +results[ (count-1-i)*2+1 ];
				imag = 0.0;
			}
			else if( i > 0 ) {
				real = +results[ (i-1)*2+1 ];
				imag = -results[ i*2 ];
			}
			else {
				real = results[0];
				imag = 0.0;
			}

//			real:
//
//			if i==0 then 0
//			if i > 0 then (i-1)*2+1
//			if i > count/2 then (count-1-i)*2+1
//
//			0 1 2 3 4 5
//			0 1 3 5 3 1
//
//
//			imag:
//			if i==0 then z
//			if i>0 then 2*i
//			if i==count/2 then z
//			if i>count/2 then neg (6-i)*2
//
//
//			0 1 2 3  4  5
//			z 2 4 z -4 -2
		}
	}
}

void FFTGSL::putComplexD( int i, double real, double imag ) {
	if( resultsReal ) {
		assert( i >= 0 && i < count );
		assert( imag == 0.0 );
		results[i] = real;
	}
	else {
	
		// @TODO: Put real code for non power of 2
	}
}



float FFTGSL::getRealF( int i ) {
	return (float)getRealD( i );
}

float FFTGSL::getImagF( int i ) {
	return (float)getImagD( i );
}

void FFTGSL::getComplexF( int i, float &real, float &imag ) {
	double _real, _imag;
	getComplexD( i, _real, _imag );
	real = (float)_real;
	imag = (float)_imag;
}

//#ifdef FFTGSL_SELF_TEST
//#endif
