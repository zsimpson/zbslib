// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			ZMat fascade around Eigen linear algebra routines, written originally
//			as alternative to GPL-licensed GSL routines in zmat_gsl.cpp. tfb
//		}
//		*SDK_DEPENDS eigen
//		*PORTABILITY win32 linux macosx
//		*REQUIRED_FILES zmat_eigen.cpp zmat_eigen.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }

// OPERATING SYSTEM specific includes:
// SDK includes:
#include <Eigen/Core>
#include <Eigen/SVD>
// STDLIB includes:
// MODULE includes:
#include "zmat_eigen.h"


using Eigen::Map;
using Eigen::Matrix;
using Eigen::MatrixXd;
using Eigen::MatrixXf;
using Eigen::JacobiSVD;
using Eigen::VectorXd;
using Eigen::RowMajor;
using Eigen::ColMajor;
using Eigen::Dynamic;

void zmatMUL_Eigen( ZMat &aMat, ZMat &bMat, ZMat &cMat ) {
	assert( aMat.cols == bMat.rows );
	assert( aMat.type == bMat.type );

	int cRows = aMat.rows;
	int cCols = bMat.cols;

	if( cMat.rows != cRows || cMat.cols != cCols || cMat.type != aMat.type ) {
		cMat.alloc( cRows, cCols, aMat.type );
	}

	if( aMat.type == zmatF64 ) {
		Map<MatrixXd> ma( (double*)aMat.mat, aMat.rows, aMat.cols );
		Map<MatrixXd> mb( (double*)bMat.mat, bMat.rows, bMat.cols );
		Map<MatrixXd> mc( (double*)cMat.mat, cMat.rows, cMat.cols );
		mc = ma * mb;
	}
	else if( aMat.type == zmatF32 ) {
		Map<MatrixXf> ma( (float*)aMat.mat, aMat.rows, aMat.cols );
		Map<MatrixXf> mb( (float*)bMat.mat, bMat.rows, bMat.cols );
		Map<MatrixXf> mc( (float*)cMat.mat, cMat.rows, cMat.cols );
		mc = ma * mb;
	}
	else {
		assert( ! "bad type on zmatMUL_Eigen" );
	}
}

void zmatSVD_Eigen( ZMat &inputMat, ZMat &uMat, ZMat &sVec, ZMat &vtMat ) {
	// Needs further testing.  Test with _labench3 - see kintek.

	//assert( inputMat.rows >= inputMat.cols );
		// GSL does not implement a M < N SVD, but Eigen has no problem with this.

	if( inputMat.type == zmatF64 ) {
		Map< MatrixXd > mInput( (double*)inputMat.mat, inputMat.rows, inputMat.cols );
		JacobiSVD<MatrixXd> svd( mInput, Eigen::ComputeThinU | Eigen::ComputeThinV );
		const MatrixXd &u = svd.matrixU();
		const MatrixXd &v = svd.matrixV();
		const VectorXd &s = svd.singularValues();
		if( uMat.rows != u.rows() || uMat.cols != u.cols() ) {
			uMat.alloc( u.rows(), u.cols(), zmatF64 );
		}
		memcpy( uMat.mat, u.data(), uMat.rows * uMat.cols * sizeof(double) );

		ZMat vMat;
		vMat.alloc( v.rows(), v.cols(), zmatF64 );
		memcpy( vMat.mat, v.data(), vtMat.rows * vtMat.cols * sizeof(double) );
		zmatTranspose( vMat, vtMat );

		if( sVec.rows != s.rows() || sVec.cols != 1 ) {
			sVec.alloc( s.rows(), 1, zmatF64 );
		}
		memcpy( sVec.mat, s.data(), sVec.rows * sizeof(double) );
	}
	else {
		// If you need zmatF32, just copy the above, and replace all Xd to Xf.
		assert( ! "not implemented on zmatSVD_Eigen" );
	}
}

void zmatQRSolve_Eigen( ZMat &A, ZMat &B, ZMat &x ) {
	assert( ! "not implemented" );
}

//---------------------------------------------------------------------------------------------
// ZMatLUSolver_Eigen

ZMatLUSolver_Eigen::ZMatLUSolver_Eigen( ZMat &A, int colMajor ) : ZMatLinEqSolver( A, colMajor ) {
	// Eigen can deal with col-major or row major, so either is ok for us.
}

ZMatLUSolver_Eigen::ZMatLUSolver_Eigen( double *A, int rows, int cols, int colMajor ) : ZMatLinEqSolver( A, rows, cols, colMajor ) {
}

ZMatLUSolver_Eigen::~ZMatLUSolver_Eigen() {
}
	
int ZMatLUSolver_Eigen::decompose() {
	if( colMajor ) {
		Map< Matrix < double, Dynamic, Dynamic, ColMajor > > ma( A, rows, cols );
		eigenA_lu.compute( ma );
	}
	else {
		Map< Matrix < double, Dynamic, Dynamic, RowMajor > > ma( A, rows, cols );
		eigenA_lu.compute( ma );
	}
	//printf( "Eigen: LU decompose\n" );
	return 1;
}

int ZMatLUSolver_Eigen::solve( double *B, double *x ) {
	// solve Ax = B, must call decompose() first.
	Map<MatrixXd> mb( B, rows, 1 );
	VectorXd _x = eigenA_lu.solve( mb );
	for( int i=0; i<rows; i++ ) {
		x[i] = _x(i);
	}
	return 1;
}


