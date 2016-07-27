#ifndef ZMAT_CLA321_H
#define ZMAT_CLA321_H

// This will duplicate some functionality in the zmat_clapack module.  zmat_clapack was written
// by ZBS to link with the sdkpub/clapack.  When TFB needed to use sdkpub/levar, and needed a 
// clapack, that lib was not suitable - a more recent version was required.  So sdkpub/clapack-3.2.1
// was added.  This module is authored to link against that more recent version.  Eventually it
// would be nice to port any functionality in zmat_clapack to this module, and remove the older
// one.  tfb 2016 july

#include "zmat.h"

//------------------------------------------------------------------------------------------
// Matrix related, for which we use ZMat as input

/*
void zmatMUL_CLA321( ZMat &aMat, ZMat &bMat, ZMat &cVec );
	// C = A * B

void zmatSVD_CLA321( ZMat &inputMat, ZMat &uMat, ZMat &sVec, ZMat &vtMat );

void zmatQRSolve_CLA321( ZMat &A, ZMat &B, ZMat &x );
*/

class ZMatLUSolver_CLA321 : public ZMatLUSolver {
	int *ipiv;

public:
	ZMatLUSolver_CLA321( double *A, int dim, int colMajor=0 );
	~ZMatLUSolver_CLA321();
	int decompose();
	int solve( double *B, double *x );
		// solve Ax = B, must call decompose() first.
};

#endif
