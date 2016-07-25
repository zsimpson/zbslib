#ifndef ZMAT_NR3_H
#define ZMAT_NR3_H

#include "zmat.h"

//------------------------------------------------------------------------------------------
// Matrix related, for which we use ZMat as input

void zmatMUL_NR3( ZMat &aMat, ZMat &bMat, ZMat &cVec );
	// C = A * B

void zmatSVD_NR3( ZMat &inputMat, ZMat &uMat, ZMat &sVec, ZMat &vtMat );

void zmatQRSolve_NR3( ZMat &A, ZMat &B, ZMat &x );

struct LUdcmp;
template <class T> class NRmatrix;
class ZMatLUSolver_NR3 : public ZMatLUSolver {
	// nr3 stuff
	NRmatrix<double>* pNRa;
	LUdcmp* pNRlu;

public:
	ZMatLUSolver_NR3( double *A, int dim, int colMajor=0 );
	~ZMatLUSolver_NR3();
	int decompose();
	int solve( double *B, double *x );
		// solve Ax = B, must call decompose() first.
};

#endif
