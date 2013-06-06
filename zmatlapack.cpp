// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			ZMat to lapack adaptor
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zmatlapack.cpp zmatlapack.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
//		*SDK_DEPENDS clapack
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "memory.h"
#include "stdlib.h"
#include "assert.h"
// MODULE includes:
#include "zmatlapack.h"
// ZBSLIB includes:
#include "zmat.h"

// The CLAPACK headers MUST BE included LAST because they have all kinds of conflicts
// as they attempt to redefine abs and other funcs if they aren't already defined
extern "C" {
#include "f2clibs/f2c.h"
#include "clapack.h"
}

void zmatEigenDecomposition( ZMat &mat, ZMat &resVecs, ZMat &resVals ) {
	assert( mat.rows == mat.cols );
	assert( sizeof(real) == sizeof(float) );

	// I'm using UPPERCASE naming conention for variables which are
	// inputs into the CLAPACK routines.

	long N = mat.rows;
	long LDA = N;

	// FIND the smallest j such that 2^j >= N. I think this quick search is faster than the taking the log
	int j = 1;
	for( int i=1; i<32; i++ ) {
		if( j >= N ) {
			break;
		}
		j <<= 1;
	}

	long LWORK = 1 + 5*N + 2*N*i + 3*N*N;
	long LIWORK = 2 + 5*N;
	long INFO = 0;

	if( resVecs.rows != mat.rows || resVecs.cols != mat.cols ) {
		resVecs.alloc( mat.rows, mat.cols, mat.type );
	}
	else {
		memcpy( resVecs.mat, mat.mat, sizeof(float) * mat.rows * mat.cols );
	}

	if( resVals.rows != 1 || resVals.cols != mat.rows ) {
		resVals.alloc( 1, resVecs.rows, zmat_FLOAT );
	}

	assert( mat.type == zmat_FLOAT );
	assert( resVecs.type == zmat_FLOAT );
	assert( resVals.type == zmat_FLOAT );

	real *A = (real *)resVecs.mat;
	real *W = (real *)resVals.mat;
	real *WORK = (real *)malloc( sizeof(real) * LWORK );
	long *IWORK = (long *)malloc( sizeof(long) * LIWORK * 2 );
		// There seems to be a memory corrupting bug in ssyevd that causes
		// it to overflow the IWORK buffer even though I'm allocating it
		// exactly as it say to do.  When I make the buffer bigger by a factor
		// of 2 I avoid the problem.

	ssyevd_( "V", "U", &N, A, &LDA, W, WORK, &LWORK, IWORK, &LIWORK, &INFO );

	free( WORK );
	free( IWORK );
}

/*
void zmatSVD( ZMat &aMat, ZMat &uMat, ZMat &sigmaMat, ZMat &vMat ) {
    // STEP 1 according to LAPACK: Convert general to bidiagonal
    integer m = aMat.rows;
    integer n = aMat.cols;
    assert( sizeof(real) == sizeof(float) );
    sgebrd( &m, &n, 
}
*/

