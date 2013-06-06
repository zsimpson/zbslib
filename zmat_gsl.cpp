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

