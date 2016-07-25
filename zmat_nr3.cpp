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
#include "ludcmp.h"

//---------------------------------------------------------------------------------------------
// ZMatLUSolver_GSL

ZMatLUSolver_NR3::ZMatLUSolver_NR3( double *A, int dim, int colMajor ) : ZMatLUSolver( A, dim, colMajor ), pNRlu(0) {
	assert( !colMajor && "NR3 must be in rowMajor" );
	pNRa = new MatDoub ( dim, dim, A, 1 );
}

ZMatLUSolver_NR3::~ZMatLUSolver_NR3() {
	delete pNRa;
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
	VecDoub b( dim, B, 1 );
	VecDoub _x( dim, x, 1 );
	pNRlu->solve( b, _x );
	return 1;
}
