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
// ZMatLinEqSolver_NR3

ZMatLinEqSolver_NR3::ZMatLinEqSolver_NR3( ZMat &_A, int _colMajor ) : ZMatLinEqSolver( _A, _colMajor ) {
	assert( _A.rows == _A.cols );
	assert( _A.type = zmatF64 );
	if( colMajor ) {
		// NR3 is rowmajor, so transpose to our local copy At
		zmatTranspose( _A, At );
		colMajor = 0;
		A = (double*)At.mat;
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
// ZMatLU Solver_NR3

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
	VecDoub _x( rows, x, 1 );
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
	VecDoub _x( rows, x, 1 );
	pNRsvd->solve( b, _x );
	return 1;
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

int ZMatQRSolver_NR3::solve( double *B, double *x ) {
	// solve Ax = B, must call decompose() first.
	VecDoub b( rows, B, 1 );
	VecDoub _x( rows, x, 1 );
	pNRqr->solve( b, _x );
	return 1;
}


