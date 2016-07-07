// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			ZMat fascade around Gnu Science Library routines
//		}
//		*SDK_DEPENDS gsl-1.8
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zmat_gsl.cpp zmat_gsl.h
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
#include "gsl/gsl_matrix_double.h"
#include "gsl/gsl_linalg.h"
#include "gsl/gsl_blas.h"
// STDLIB includes:
// MODULE includes:
#include "zmat_gsl.h"

void zmatMUL_GSL( ZMat &aMat, ZMat &bMat, ZMat &cMat ) {
	assert( aMat.cols == bMat.rows );
	assert( aMat.type == bMat.type );

	int cRows = aMat.rows;
	int cCols = bMat.cols;

	if( cMat.rows != cRows || cMat.cols != cCols || cMat.type != aMat.type ) {
		cMat.alloc( cRows, cCols, aMat.type );
	}

	if( aMat.type == zmatF64 ) {
		cblas_dgemm( CblasColMajor, CblasNoTrans, CblasNoTrans, aMat.rows, bMat.cols, aMat.cols, 1.0, aMat.getPtrD(), aMat.rows, bMat.getPtrD(), bMat.rows, 1.0, cMat.getPtrD(), cMat.rows );
	}
	else if( aMat.type == zmatF32 ) {
		cblas_sgemm( CblasColMajor, CblasNoTrans, CblasNoTrans, aMat.rows, bMat.cols, aMat.cols, 1.0, aMat.getPtrF(), aMat.rows, bMat.getPtrF(), bMat.rows, 1.0, cMat.getPtrF(), cMat.rows );
	}
	else {
		assert( ! "bad type on zmatMUL_GSL" );
	}
}

void zmatSVD_GSL( ZMat &inputMat, ZMat &uMat, ZMat &sVec, ZMat &vtMat ) {
	assert( inputMat.rows >= inputMat.cols );
		// GSL does not implement a M < N SVD

	int r, c;
	int rows = inputMat.rows;
	int cols = inputMat.cols;

	// COPY the input matrix
	gsl_matrix *A = gsl_matrix_alloc( rows, cols );
	if( inputMat.type == zmatF32 ) {
		for( c=0; c<cols; c++ ) {
			for( r=0; r<rows; r++ ) {
				*gsl_matrix_ptr( A, r, c ) = (double)*inputMat.getPtrF( r, c );
			}
		}
	}
	else if( inputMat.type == zmatF64 ) {
		for( c=0; c<cols; c++ ) {
			for( r=0; r<rows; r++ ) {
				*gsl_matrix_ptr( A, r, c ) = *inputMat.getPtrD( r, c );
			}
		}
	}
	else {
		assert( ! "Wrong inputMat type" );
	}

	// PERFORM SVD
	gsl_matrix *V = gsl_matrix_alloc( cols, cols );
	gsl_vector *S = gsl_vector_alloc( cols );
	gsl_vector *WORK = gsl_vector_alloc( cols );
	int res = gsl_linalg_SV_decomp( A, V, S, WORK );
	assert( res == 0 );


	// STORE the uMat
	uMat.alloc( A->size1, A->size2, inputMat.type );
	if( uMat.type == zmatF32 ) {
		for( c=0; c<cols; c++ ) {
			for( r=0; r<rows; r++ ) {
				*uMat.getPtrF( r, c ) = (float)*gsl_matrix_ptr( A, r, c );
			}
		}
	}
	else if( uMat.type == zmatF64 ) {
		for( c=0; c<cols; c++ ) {
			for( r=0; r<rows; r++ ) {
				*uMat.getPtrD( r, c ) = *gsl_matrix_ptr( A, r, c );
			}
		}
	}
	else {
		assert( ! "Wrong uMat type" );
	}

	// STORE the vtMat. Note that GSL gives v not v transpose so we transpose it here
	// v is an NxN (cols x cols) matrix
	vtMat.alloc( V->size1, V->size2, inputMat.type );
	cols = V->size1;
	if( vtMat.type == zmatF32 ) {
		for( c=0; c<cols; c++ ) {
			for( r=0; r<cols; r++ ) {
				*vtMat.getPtrF( c, r ) = (float)*gsl_matrix_ptr( V, r, c );
			}
		}
	}
	else if( vtMat.type == zmatF64 ) {
		for( c=0; c<cols; c++ ) {
			for( r=0; r<cols; r++ ) {
				*vtMat.getPtrD( c, r ) = *gsl_matrix_ptr( V, r, c );
			}
		}
	}
	else {
		assert( ! "Wrong vtMat type" );
	}

	// STORE the sVec
	rows = (int)S->size;
	sVec.alloc( rows, 1, inputMat.type );
	if( sVec.type == zmatF32 ) {
		for( r=0; r<rows; r++ ) {
			sVec.setF( r, 0, (float)gsl_vector_get(S,r) );
		}
	}
	else if( sVec.type == zmatF64 ) {
		for( r=0; r<rows; r++ ) {
			sVec.setD( r, 0, gsl_vector_get(S,r) );
		}
	}
	else {
		assert( ! "Wrong sMat type" );
	}

	gsl_matrix_free( A );
	gsl_matrix_free( V );
	gsl_vector_free( S );
	gsl_vector_free( WORK );
}

void zmatQRSolve_GSL( ZMat &_A, ZMat &_B, ZMat &_x ) {
	// Solves the linear system Ax = B for x using QR decomposition.
	//
	// This was written for KinTek Explorer by tfb for dealign with 3d spectra.
	// We think of the data in A as being a set of column vectors, each with A.rows data points.
	//
	int dataCount = _A.rows;
	int components = _A.cols;
	
	if( _x.rows != components || _x.cols != _B.cols ) {
		_x.alloc( components, _B.cols, zmatF64 );
	}
	
	//
	// ZMat is col-major, but gsl is row major, so we transpose, get a gsl view on rows,
	// and do QR decomposition
	//
	ZMat A;
	zmatTranspose( _A, A );
	gsl_matrix_view gslA = gsl_matrix_view_array ( (double*)A.mat, dataCount, components );
	gsl_vector *tau = gsl_vector_alloc( components );
	int retval = gsl_linalg_QR_decomp( &gslA.matrix, tau );
	
	//
	// Now solve for one column of _B at a time, store the results in _x
	//
	gsl_vector *x = gsl_vector_alloc( components );
	gsl_vector *residual = gsl_vector_alloc( dataCount );
	
	for( int d=0; d < _B.cols; d++ ) {
		gsl_vector_view gslB = gsl_vector_view_array ( _B.getPtrD( 0, d ), dataCount );
		int retval = gsl_linalg_QR_lssolve ( &gslA.matrix, tau, &gslB.vector, x, residual );
		
		for( int c=0; c<components; c++ ) {
			_x.putD( c, d, gsl_vector_get( x, c ) );
		}
	}
	
	gsl_vector_free (x);
	gsl_vector_free( residual );
}

//---------------------------------------------------------------------------------------------
// ZMatLUSolver_GSL

ZMatLUSolver_GSL::ZMatLUSolver_GSL( double *A, int dim, int colMajor ) : ZMatLUSolver( A, dim, colMajor ) {
	assert( !colMajor && "GSL must be in rowMajor" );
	gslA = gsl_matrix_view_array( A, dim, dim );
	gslP = gsl_permutation_alloc( dim );
}

ZMatLUSolver_GSL::~ZMatLUSolver_GSL() {
	gsl_permutation_free( gslP );
}
	
int ZMatLUSolver_GSL::decompose() {
	int err = gsl_linalg_LU_decomp( &gslA.matrix, gslP, &sign );
	// printf( "GSL: LU decompose\n" );
	return err == GSL_SUCCESS;
}

int ZMatLUSolver_GSL::solve( double *B, double *x ) {
	// solve Ax = B, must call decompose() first.
	gsl_vector_view gslB = gsl_vector_view_array( B, dim );
	gsl_vector_view gslX = gsl_vector_view_array( x, dim );
	int err = gsl_linalg_LU_solve( &gslA.matrix, gslP, &gslB.vector, &gslX.vector );
	return err == GSL_SUCCESS;
}



