#ifndef ZMAT_EIGEN_H
#define ZMAT_EIGEN_H

#include "zmat.h"
#include <Eigen/LU>


void zmatMUL_Eigen( ZMat &aMat, ZMat &bMat, ZMat &cVec );
	// C = A * B

void zmatSVD_Eigen( ZMat &inputMat, ZMat &uMat, ZMat &sVec, ZMat &vtMat );

void zmatQRSolve_Eigen( ZMat &A, ZMat &B, ZMat &x );

class ZMatLUSolver_Eigen : public ZMatLinEqSolver {
	// In factoring GSL dependency out of various zbslib & other routines, it is handy
	// to have an object which can hold state since it is often the case that one does
	// a decomposition, and then follows with multiple solve(), and importantly, other
	// libraries may not make the separate calls as atomic as GSL does.  See the zmat_eigen.h
	// declaration of this for Eigen, for example, and how these are both used by
	// zintegrator and kineticbase in zbslib.
	
	// Eigen stuff
	Eigen::PartialPivLU<Eigen::MatrixXd>  eigenA_lu;

public:
	ZMatLUSolver_Eigen( ZMat &a, int colMajor=1 );
	ZMatLUSolver_Eigen( double *A, int rows, int cols, int colMajor=1 );
	~ZMatLUSolver_Eigen();
	int decompose();
	int solve( double *B, double *x );
		// solve Ax = B, must call decompose() first.
};

#endif
