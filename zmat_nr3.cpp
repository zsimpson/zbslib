// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			ZMat fascade around NR3 Library routines
//		}
//		*SDK_DEPENDS nr3
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zmat_nr3.cpp zmat_nr3.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }

// Note that NR3 is not a public SDK, so this module is only usable
// by persons who have licensed the NR3 code and have it available
// in their private SDK folder.

#include "zmat_nr3.h"

#include "nr3.h"
#include "svd.h"
#include "ludcmp.h"
#include "qrdcmp.h"



//---------------------------------------------------------------------------------------------
// Handy debug/utils.

void dump_nrmat( MatDoub &m ) {
	for( int r=0; r<m.nrows(); r++ ) {
		for( int c=0; c<m.ncols(); c++ ) {
			printf( "%+3.2le ", m[r][c] );
		}
		printf( "\n" );
	}	
}

void copyNRMatToZMat( MatDoub &m, ZMat &z ) {
	// account for NR3 is rowmajor, ZMat is colmajor.
	int rows = m.nrows();
	int cols = m.ncols();
	if( z.rows != rows || z.cols != cols ) {
		z.alloc( rows, cols, zmatF64 );
	}
	for( int r=0; r<rows; r++ ) {
		for( int c=0; c<cols; c++ ) {
			z.putD( r, c, m[r][c] );
		}
	}
}

//---------------------------------------------------------------------------------------------
// ZMatLinEqSolver_NR3

ZMatLinEqSolver_NR3::ZMatLinEqSolver_NR3( ZMat &_A, int _colMajor ) : ZMatLinEqSolver( _A, _colMajor ) {
	assert( _A.type = zmatF64 );
	if( colMajor ) {
		// NR3 is rowmajor, so transpose to our local copy At
		zmatTranspose( _A, At );
		colMajor = 0;
		A = (double*)At.mat;
			// Note that we do not change rows and cols: the "shape" of the matrix is as was passed to
			// us - we have just transposed it and pointed to the newly transposed memory such that
			// the values are now in row-major order.
	}
	pNRa = new MatDoub ( rows, cols, A, 1 );
}

ZMatLinEqSolver_NR3::ZMatLinEqSolver_NR3( double *A, int rows, int cols, int colMajor ) : ZMatLinEqSolver( A, rows, cols, colMajor ) {
	assert( !colMajor && "NR3 must be in rowMajor" );
	pNRa = new MatDoub ( rows, cols, A, 1 );
}

ZMatLinEqSolver_NR3::~ZMatLinEqSolver_NR3() {
	delete pNRa;
}	

//---------------------------------------------------------------------------------------------
// ZMatLUSolver_NR3

ZMatLUSolver_NR3::~ZMatLUSolver_NR3() {
	if( pNRlu )
		delete pNRlu;
}
	
int ZMatLUSolver_NR3::decompose() {
	if( pNRlu )
		delete pNRlu;
	pNRlu = new LUdcmp( *pNRa );
	return 1;
}

int ZMatLUSolver_NR3::solve( double *B, double *x ) {
	// solve Ax = B, must call decompose() first.
	VecDoub b( rows, B, 1 );
	VecDoub _x( cols, x, 1 );
	pNRlu->solve( b, _x );
	return 1;
}

//---------------------------------------------------------------------------------------------
// ZMatSVDSolver_NR3

ZMatSVDSolver_NR3::~ZMatSVDSolver_NR3() {
	if( pNRsvd )
		delete pNRsvd;
}
	
int ZMatSVDSolver_NR3::decompose() {
	if( pNRsvd )
		delete pNRsvd;	
	pNRsvd = new SVD( *pNRa );
	return 1;
}

int ZMatSVDSolver_NR3::solve( double *B, double *x ) {
	// solve Ax = B, must call decompose() first.
	VecDoub b( rows, B, 1 );
	VecDoub _x( cols, x, 1 );
	pNRsvd->solve( b, _x );
	return 1;
}

int ZMatSVDSolver_NR3::rank() {
	return pNRsvd->rank();
}

void ZMatSVDSolver_NR3::zmatGet( ZMat &U, ZMat &S, ZMat &Vt ) {
	copyNRMatToZMat( pNRsvd->u, U );

	S.alloc( pNRsvd->w.size(), 1, zmatF64 );
	memcpy( S.mat, pNRsvd->w.v, S.rows*sizeof(double) );
		// w is a NRvector, not a NRMatrix

	ZMat V;
	copyNRMatToZMat( pNRsvd->v, V );
	zmatTranspose( V, Vt );
}

//---------------------------------------------------------------------------------------------
// ZMatQRSolver_NR3

ZMatQRSolver_NR3::~ZMatQRSolver_NR3() {
	if( pNRqr )
		delete pNRqr;
}
	
int ZMatQRSolver_NR3::decompose() {
	if( pNRqr )
		delete pNRqr;
	pNRqr = new QRdcmp( *pNRa );
	return 1;
}

int ZMatQRSolver_NR3::solve( double *b, double *x ) {
	// solve Ax = b, must call decompose() first.
	VecDoub _b( rows, b, 1 );
	VecDoub _x( cols, x, 1 );
	pNRqr->solve( _b, _x );
	return 1;
}

int ZMatQRSolver_NR3::solveMat( ZMat &B, ZMat &X ) {
	// solve AX = B, must call decompose() first.
	// Done by solving one column of B at a time.

	if( X.rows != this->cols || X.cols != B.cols ) {
		X.alloc( this->cols, B.cols, zmatF64 );
	}

	for( int i=0; i<B.cols; i++ ) {
		solve( B.getPtrD(0,i), X.getPtrD(0,i) );
	}
	
	return 1;
}


