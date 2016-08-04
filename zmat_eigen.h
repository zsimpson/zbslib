#ifndef ZMAT_EIGEN_H
#define ZMAT_EIGEN_H

#include "zmat.h"
#include <Eigen/LU>

//
// I pulled the Eigen SDK into sdkpub when exploring it as a non-GPL replacement for GSL
// for commercial software (kintek).  Eigen turns out to be quite fast at very large
// systems, but due to overhead during setup, for smaller systems it is much slower
// than GSL.  For kintek, we ended up using NR3 instead.  See zmat_nr3.h
// tfb july 2016
//


void zmatMUL_Eigen( ZMat &aMat, ZMat &bMat, ZMat &cVec );
	// C = A * B

void zmatSVD_Eigen( ZMat &inputMat, ZMat &uMat, ZMat &sVec, ZMat &vtMat );
	// needs further testing, esp non-square systems.  Add to _labench3 (kintek)

void zmatQRSolve_Eigen( ZMat &A, ZMat &B, ZMat &x );

class ZMatLUSolver_Eigen : public ZMatLinEqSolver {
	// In factoring GSL dependency out of various zbslib & other routines, it is handy
	// to have an object which can hold state since it is often the case that one does
	// a decomposition, and then follows with multiple solve(), and importantly, other
	// libraries may not make the separate calls as atomic as GSL does. e.g. see how
	// these are used by zintegrator and kineticbase in zbslib.
	
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
