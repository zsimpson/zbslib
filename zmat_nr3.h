#ifndef ZMAT_NR3_H
#define ZMAT_NR3_H

#include "zmat.h"

//------------------------------------------------------------------------------------------
// Matrix related, for which we use ZMat as input

void zmatMUL_NR3( ZMat &aMat, ZMat &bMat, ZMat &cVec );
	// C = A * B

void zmatSVD_NR3( ZMat &inputMat, ZMat &uMat, ZMat &sVec, ZMat &vtMat );

void zmatQRSolve_NR3( ZMat &A, ZMat &B, ZMat &x );

template <class T> class NRmatrix;
class ZMatLinEqSolver_NR3 : public ZMatLinEqSolver {
	// nr3 stuff common to all solvers
	NRmatrix<double>* pNRa;
public:
	ZMatLinEqSolver_NR3( ZMat &A, int colMajor=1 );
	ZMatLinEqSolver_NR3( double *A, int rows, int cols, int colMajor=0 );
	~ZMatLinEqSolver_NR3();
	virtual int decompose() = 0;
	virtual int solve( double *B, double *x ) = 0;
		// solve Ax = B, must call decompose() first.
};

struct LUdcmp;
class ZMatLUSolver_NR3 : public ZMatLinEqSolver_NR3 {
	// nr3 stuff
	LUdcmp* pNRlu;
public:
	ZMatLUSolver_NR3( ZMat &A, int colMajor=1 ) : ZMatLinEqSolver_NR3( A, colMajor ), pNRlu(0) {}
	ZMatLUSolver_NR3( double *A, int rows, int cols, int colMajor=0 ) : ZMatLinEqSolver_NR3( A, rows, cols, colMajor), pNRlu(0) {}
	~ZMatLUSolver_NR3();
	int decompose();
	int solve( double *B, double *x );
		// solve Ax = B, must call decompose() first.
};

struct SVD;
class ZMatSVDSolver_NR3 : public ZMatLinEqSolver_NR3 {
	// nr3 stuff
	SVD* pNRsvd;
public:
	ZMatSVDSolver_NR3( ZMat &A, int colMajor=1 ) : ZMatLinEqSolver_NR3( A, colMajor ), pNRsvd(0) {}
	ZMatSVDSolver_NR3( double *A, int rows, int cols, int colMajor=0 ) : ZMatLinEqSolver_NR3( A, rows, cols, colMajor), pNRsvd(0) {}
	~ZMatSVDSolver_NR3();
	int decompose();
	int solve( double *B, double *x );
		// solve Ax = B, must call decompose() first.
};

struct QRdcmp;
class ZMatQRSolver_NR3 : public ZMatLinEqSolver_NR3 {
	// nr3 stuff
	QR* pNRdcmp;
public:
	ZMatQRSolver_NR3( ZMat &A, int colMajor=1 ) : ZMatLinEqSolver_NR3( A, colMajor ), pNRqr(0) {}
	ZMatQRSolver_NR3( double *A, int rows, int cols, int colMajor=0 ) : ZMatLinEqSolver_NR3( A, rows, cols, colMajor), pNRqr(0) {}
	~ZMatQRSolver_NR3();
	int decompose();
	int solve( double *B, double *x );
		// solve Ax = B, must call decompose() first.
};

#endif
