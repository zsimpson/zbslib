#ifndef ZMAT_GSL_H
#define ZMAT_GSL_H

#include "zmat.h"
#include "gsl/gsl_linalg.h"

void zmatMUL_GSL( ZMat &aMat, ZMat &bMat, ZMat &cVec );
	// C = A * B

void zmatSVD_GSL( ZMat &inputMat, ZMat &uMat, ZMat &sVec, ZMat &vtMat );

void zmatQRSolve_GSL( ZMat &A, ZMat &B, ZMat &x );

class ZMatLUSolver_GSL : public ZMatLinEqSolver {
	// In factoring GSL dependency out of various zbslib & other routines, it is handy
	// to have an object which can hold state since it is often the case that one does
	// a decomposition, and then follows with multiple solve(), and importantly, other
	// libraries may not make the separate calls as atomic as GSL does.  See the zmat_eigen.h
	// declaration of this for Eigen, for example, and how these are both used by
	// zintegrator and kineticbase in zbslib.
	
	// gsl stuff
	gsl_matrix_view gslA;
	gsl_permutation *gslP;
	int sign;		

public:
	ZMatLUSolver_GSL( double *A, int rows, int cols, int colMajor=0 );
	~ZMatLUSolver_GSL();
	int decompose();
	int solve( double *B, double *x );
		// solve Ax = B, must call decompose() first.
};

#endif
