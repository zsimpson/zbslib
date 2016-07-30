// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			ZMat fascade around CLAPACK-3.2.1 Library routines
//		}
//		*SDK_DEPENDS clapack-3.2.1
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zmat_cla321.cpp zmat_cla321.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }

#include "zmat_cla321.h"
#include "string.h"
#include "stdlib.h"

// The CLAPACK headers MUST BE included LAST because they have all kinds of conflicts
// as they attempt to redefine abs and other funcs if they aren't already defined
extern "C" {
	#include "f2c.h"
	#include "clapack.h"
}

//---------------------------------------------------------------------------------------------
// ZMatLUSolver_GSL

ZMatLUSolver_CLA321::ZMatLUSolver_CLA321( double *A, int rows, int cols, int colMajor ) : ZMatLinEqSolver( A, rows, cols, colMajor ) {
	assert( colMajor && "CLA321 must be in colMajor" );
	ipiv = (int*)malloc( sizeof(int) * rows );
}

ZMatLUSolver_CLA321::~ZMatLUSolver_CLA321() {
	if( ipiv )
		free( ipiv );
}
	
int ZMatLUSolver_CLA321::decompose() {
	//int dgetrf_(integer *m, integer *n, doublereal *a, integer *lda, integer *ipiv, integer *info);
	int lda=rows,info,err;
	err = dgetrf_( &rows, &cols, A, &lda, ipiv, &info );
	return err==0;
}

int ZMatLUSolver_CLA321::solve( double *B, double *x ) {
	// solve Ax = B, must call decompose() first.
	// WARNING: B is overwritten here!

	//int dgetrs_(char *trans, integer *n, integer *nrhs, doublereal *a, integer *lda, integer *ipiv, doublereal *b, integer *
	//            ldb, integer *info);
	char trans = 'N';
	int nrhs = 1;
		// columns in B
	int err, info;
	err = dgetrs_( &trans, &rows, &nrhs, A, &rows, ipiv, B, &rows, &info );
	memcpy( x, B, sizeof(double)*rows );
	return err==0;
}
