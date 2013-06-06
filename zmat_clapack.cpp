// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			ZMat fascade around clapack routines
//		}
//		*SDK_DEPENDS clapack
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zmat_clapack.cpp zmat_clapack.h
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
// STDLIB includes:
#include "stdlib.h"
#ifdef WIN32
#include "malloc.h"
#endif
// MODULE includes:
#include "zmat_clapack.h"

// The CLAPACK headers MUST BE included LAST because they have all kinds of conflicts
// as they attempt to redefine abs and other funcs if they aren't already defined
extern "C" {
	#include "f2clibs/f2c.h"
	#include "clapack.h"
}

int zmatSolve_CLAPACK( ZMat &inputMat, ZMat &inputVec, ZMat &solutionVec ) {
	assert( inputMat.type == zmatF32 );
	assert( inputVec.type == zmatF32 );

	assert( inputMat.rows == inputMat.cols );
	assert( inputVec.cols == 1 );

	/*
	*  N       (input) INTEGER
	*          The number of linear equations, i.e., the order of the
	*          matrix A.  N >= 0.
	*
	*  NRHS    (input) INTEGER
	*          The number of right hand sides, i.e., the number of columns
	*          of the matrix B.  NRHS >= 0.
	*
	*  A       (input/output) REAL array, dimension (LDA,N)
	*          On entry, the N-by-N coefficient matrix A.
	*          On exit, the factors L and U from the factorization
	*          A = P*L*U; the unit diagonal elements of L are not stored.
	*
	*  LDA     (input) INTEGER
	*          The leading dimension of the array A.  LDA >= max(1,N).
	*
	*  IPIV    (output) INTEGER array, dimension (N)
	*          The pivot indices that define the permutation matrix P;
	*          row i of the matrix was interchanged with row IPIV(i).
	*
	*  B       (input/output) REAL array, dimension (LDB,NRHS)
	*          On entry, the N-by-NRHS matrix of right hand side matrix B.
	*          On exit, if INFO = 0, the N-by-NRHS solution matrix X.
	*
	*  LDB     (input) INTEGER
	*          The leading dimension of the array B.  LDB >= max(1,N).
	*
	*  INFO    (output) INTEGER
	*          = 0:  successful exit
	*          < 0:  if INFO = -i, the i-th argument had an illegal value
	*          > 0:  if INFO = i, U(i,i) is exactly zero.  The factorization
	*                has been completed, but the factor U is exactly
	*                singular, so the solution could not be computed.
	*/

	long N = inputMat.rows;
	long NRHS = 1;
	ZMat A( inputMat );
	long LDA = N;
	ZMat IPIV( N, 1, zmatS32 );
	ZMat B( inputVec );
	long LDB = N;
	long INFO = 0;

	sgesv_( &N, &NRHS, A.getPtrF(0,0), &LDA, (long *)IPIV.getPtrI(0,0), B.getPtrF(0,0), &LDB, &INFO );
	if( INFO == 0 ) {
		solutionVec.copy( B );
		return 1;
	}
	return 0;
}


void zmatSVD_CLAPACK( ZMat &inputMat, ZMat &uMat, ZMat &sVec, ZMat &vtMat ) {
assert( 0 );
	// This is all untested

	// SVD using lapack is complicated.  You first have to convert the matrix
	// into bi-diagonal form and then do the SVD.  Eevntually, this is the right
	// thing to do.  For now, I'm going to try to use the Gnu Science Library

	assert( inputMat.type == zmatF32 );

	char JOBU  = 'A';
	char JOBVT = 'A';
	long M = inputMat.rows;
	long N = inputMat.cols;
	ZMat A( inputMat );
	long LDA = M;
	sVec.alloc( min(M,N), 1, inputMat.type );
	long LDU = M;
	uMat.alloc( LDU, N, inputMat.type );
	long LDVT = N;
	vtMat.alloc( LDVT, N, inputMat.type );
	ZMat WORKSIZE( 1, 1, inputMat.type );
	long LWORK = -1;
	long INFO = 0;

	// CALL SVD to discover the work size
	sgesvd_( &JOBU, &JOBVT, &M, &N, A.getPtrF(0,0), &LDA, sVec.getPtrF(0,0), uMat.getPtrF(0,0), &LDU, vtMat.getPtrF(0,0), &LDVT, WORKSIZE.getPtrF(0,0), &LWORK, &INFO );
	assert( INFO == 0 );

	// ALLOCATE the workspace based on the discovered size
	LWORK = (long)WORKSIZE.getF(0,0);
	ZMat WORK( LWORK, 1, inputMat.type );

	// COMPUTE the SVD for real now
	sgesvd_( &JOBU, &JOBVT, &M, &N, A.getPtrF(0,0), &LDA, sVec.getPtrF(0,0), uMat.getPtrF(0,0), &LDU, vtMat.getPtrF(0,0), &LDVT, WORK.getPtrF(0,0), &LWORK, &INFO );
	assert( INFO == 0 );

	//uMat.copy( A );

	//*  SGESVD computes the singular value decomposition (SVD) of a real
	//*  M-by-N matrix A, optionally computing the left and/or right singular
	//*  vectors. The SVD is written
	//*
	//*       A = U * SIGMA * transpose(V)
	//*
	//*  where SIGMA is an M-by-N matrix which is zero except for its
	//*  min(m,n) diagonal elements, U is an M-by-M orthogonal matrix, and
	//*  V is an N-by-N orthogonal matrix.  The diagonal elements of SIGMA
	//*  are the singular values of A; they are real and non-negative, and
	//*  are returned in descending order.  The first min(m,n) columns of
	//*  U and V are the left and right singular vectors of A.
	//*
	//*  Note that the routine returns V**T, not V.
	//*
	//*  Arguments
	//*  =========
	//*
	//*  JOBU    (input) CHARACTER*1
	//*          Specifies options for computing all or part of the matrix U:
	//*          = 'A':  all M columns of U are returned in array U:
	//*          = 'S':  the first min(m,n) columns of U (the left singular
	//*                  vectors) are returned in the array U;
	//*          = 'O':  the first min(m,n) columns of U (the left singular
	//*                  vectors) are overwritten on the array A;
	//*          = 'N':  no columns of U (no left singular vectors) are
	//*                  computed.
	//*
	//*  JOBVT   (input) CHARACTER*1
	//*          Specifies options for computing all or part of the matrix
	//*          V**T:
	//*          = 'A':  all N rows of V**T are returned in the array VT;
	//*          = 'S':  the first min(m,n) rows of V**T (the right singular
	//*                  vectors) are returned in the array VT;
	//*          = 'O':  the first min(m,n) rows of V**T (the right singular
	//*                  vectors) are overwritten on the array A;
	//*          = 'N':  no rows of V**T (no right singular vectors) are
	//*                  computed.
	//*
	//*          JOBVT and JOBU cannot both be 'O'.
	//*
	//*  M       (input) INTEGER
	//*          The number of rows of the input matrix A.  M >= 0.
	//*
	//*  N       (input) INTEGER
	//*          The number of columns of the input matrix A.  N >= 0.
	//*
	//*  A       (input/output) REAL array, dimension (LDA,N)
	//*          On entry, the M-by-N matrix A.
	//*          On exit,
	//*          if JOBU = 'O',  A is overwritten with the first min(m,n)
	//*                          columns of U (the left singular vectors,
	//*                          stored columnwise);
	//*          if JOBVT = 'O', A is overwritten with the first min(m,n)
	//*                          rows of V**T (the right singular vectors,
	//*                          stored rowwise);
	//*          if JOBU .ne. 'O' and JOBVT .ne. 'O', the contents of A
	//*                          are destroyed.
	//*
	//*  LDA     (input) INTEGER
	//*          The leading dimension of the array A.  LDA >= max(1,M).
	//*
	//*  S       (output) REAL array, dimension (min(M,N))
	//*          The singular values of A, sorted so that S(i) >= S(i+1).
	//*
	//*  U       (output) REAL array, dimension (LDU,UCOL)
	//*          (LDU,M) if JOBU = 'A' or (LDU,min(M,N)) if JOBU = 'S'.
	//*          If JOBU = 'A', U contains the M-by-M orthogonal matrix U;
	//*          if JOBU = 'S', U contains the first min(m,n) columns of U
	//*          (the left singular vectors, stored columnwise);
	//*          if JOBU = 'N' or 'O', U is not referenced.
	//*
	//*  LDU     (input) INTEGER
	//*          The leading dimension of the array U.  LDU >= 1; if
	//*          JOBU = 'S' or 'A', LDU >= M.
	//*
	//*  VT      (output) REAL array, dimension (LDVT,N)
	//*          If JOBVT = 'A', VT contains the N-by-N orthogonal matrix
	//*          V**T;
	//*          if JOBVT = 'S', VT contains the first min(m,n) rows of
	//*          V**T (the right singular vectors, stored rowwise);
	//*          if JOBVT = 'N' or 'O', VT is not referenced.
	//*
	//*  LDVT    (input) INTEGER
	//*          The leading dimension of the array VT.  LDVT >= 1; if
	//*          JOBVT = 'A', LDVT >= N; if JOBVT = 'S', LDVT >= min(M,N).
	//*
	//*  WORK    (workspace/output) REAL array, dimension (LWORK)
	//*          On exit, if INFO = 0, WORK(1) returns the optimal LWORK;
	//*          if INFO > 0, WORK(2:MIN(M,N)) contains the unconverged
	//*          superdiagonal elements of an upper bidiagonal matrix B
	//*          whose diagonal is in S (not necessarily sorted). B
	//*          satisfies A = U * B * VT, so it has the same singular values
	//*          as A, and singular vectors related by U and VT.
	//*
	//*  LWORK   (input) INTEGER
	//*          The dimension of the array WORK. LWORK >= 1.
	//*          LWORK >= MAX(3*MIN(M,N)+MAX(M,N),5*MIN(M,N)).
	//*          For good performance, LWORK should generally be larger.
	//*
	//*          If LWORK = -1, then a workspace query is assumed; the routine
	//*          only calculates the optimal size of the WORK array, returns
	//*          this value as the first entry of the WORK array, and no error
	//*          message related to LWORK is issued by XERBLA.
	//*
	//*  INFO    (output) INTEGER
	//*          = 0:  successful exit.
	//*          < 0:  if INFO = -i, the i-th argument had an illegal value.
	//*          > 0:  if SBDSQR did not converge, INFO specifies how many
	//*                superdiagonals of an intermediate bidiagonal form B
	//*                did not converge to zero. See the description of WORK
	//*                above for details.


}


int zmatEigen_CLAPACK_doubleSortComparator( const void *a, const void *b ) {
	double *aa = (double *)a;
	double *bb = (double *)b;
	double delta = bb[1] - aa[1];
	return delta < 0.0 ? -1 : ( delta > 0.0 ? 1 : 0 );
}

void zmatEigen_CLAPACK( ZMat &input, ZMat &eigVecs, ZMat &eigVals ) {
	/*
	*  Purpose
	*  =======
	*
	*  SGEEV computes for an N-by-N real nonsymmetric matrix A, the
	*  eigenvalues and, optionally, the left and/or right eigenvectors.
	*
	*  The right eigenvector v(j) of A satisfies
	*                   A * v(j) = lambda(j) * v(j)
	*  where lambda(j) is its eigenvalue.
	*  The left eigenvector u(j) of A satisfies
	*                u(j)**H * A = lambda(j) * u(j)**H
	*  where u(j)**H denotes the conjugate transpose of u(j).
	*
	*  The computed eigenvectors are normalized to have Euclidean norm
	*  equal to 1 and largest component real.
	*
	*  Arguments
	*  =========
	*
	*  JOBVL   (input) CHARACTER*1
	*          = 'N': left eigenvectors of A are not computed;
	*          = 'V': left eigenvectors of A are computed.
	*
	*  JOBVR   (input) CHARACTER*1
	*          = 'N': right eigenvectors of A are not computed;
	*          = 'V': right eigenvectors of A are computed.
	*
	*  N       (input) INTEGER
	*          The order of the matrix A. N >= 0.
	*
	*  A       (input/output) REAL array, dimension (LDA,N)
	*          On entry, the N-by-N matrix A.
	*          On exit, A has been overwritten.
	*
	*  LDA     (input) INTEGER
	*          The leading dimension of the array A.  LDA >= max(1,N).
	*
	*  WR      (output) REAL array, dimension (N)
	*  WI      (output) REAL array, dimension (N)
	*          WR and WI contain the real and imaginary parts,
	*          respectively, of the computed eigenvalues.  Complex
	*          conjugate pairs of eigenvalues appear consecutively
	*          with the eigenvalue having the positive imaginary part
	*          first.
	*
	*  VL      (output) REAL array, dimension (LDVL,N)
	*          If JOBVL = 'V', the left eigenvectors u(j) are stored one
	*          after another in the columns of VL, in the same order
	*          as their eigenvalues.
	*          If JOBVL = 'N', VL is not referenced.
	*          If the j-th eigenvalue is real, then u(j) = VL(:,j),
	*          the j-th column of VL.
	*          If the j-th and (j+1)-st eigenvalues form a complex
	*          conjugate pair, then u(j) = VL(:,j) + i*VL(:,j+1) and
	*          u(j+1) = VL(:,j) - i*VL(:,j+1).
	*
	*  LDVL    (input) INTEGER
	*          The leading dimension of the array VL.  LDVL >= 1; if
	*          JOBVL = 'V', LDVL >= N.
	*
	*  VR      (output) REAL array, dimension (LDVR,N)
	*          If JOBVR = 'V', the right eigenvectors v(j) are stored one
	*          after another in the columns of VR, in the same order
	*          as their eigenvalues.
	*          If JOBVR = 'N', VR is not referenced.
	*          If the j-th eigenvalue is real, then v(j) = VR(:,j),
	*          the j-th column of VR.
	*          If the j-th and (j+1)-st eigenvalues form a complex
	*          conjugate pair, then v(j) = VR(:,j) + i*VR(:,j+1) and
	*          v(j+1) = VR(:,j) - i*VR(:,j+1).
	*
	*  LDVR    (input) INTEGER
	*          The leading dimension of the array VR.  LDVR >= 1; if
	*          JOBVR = 'V', LDVR >= N.
	*
	*  WORK    (workspace/output) REAL array, dimension (MAX(1,LWORK))
	*          On exit, if INFO = 0, WORK(1) returns the optimal LWORK.
	*
	*  LWORK   (input) INTEGER
	*          The dimension of the array WORK.  LWORK >= max(1,3*N), and
	*          if JOBVL = 'V' or JOBVR = 'V', LWORK >= 4*N.  For good
	*          performance, LWORK must generally be larger.
	*
	*          If LWORK = -1, then a workspace query is assumed; the routine
	*          only calculates the optimal size of the WORK array, returns
	*          this value as the first entry of the WORK array, and no error
	*          message related to LWORK is issued by XERBLA.
	*
	*  INFO    (output) INTEGER
	*          = 0:  successful exit
	*          < 0:  if INFO = -i, the i-th argument had an illegal value.
	*          > 0:  if INFO = i, the QR algorithm failed to compute all the
	*                eigenvalues, and no eigenvectors have been computed;
	*                elements i+1:N of WR and WI contain eigenvalues which
	*                have converged.
	*
	*/

	assert( input.rows == input.cols );
	if( input.type == zmatF32 ) {

		int dim = input.rows;
		eigVecs.alloc( dim, dim, zmatF32 );
		eigVals.alloc( dim, 1  , zmatC32 );

		//real A[N*N], wr[N], wi[N], work[4*N], vl[N*N], vr[N*N];
		//float *A = (float *)alloca( sizeof(float) * dim * dim );
		float *vl = (float *)alloca( sizeof(float) * dim * dim );
		float *vr = (float *)alloca( sizeof(float) * dim * dim );
		float *wr = (float *)alloca( sizeof(float) * dim );
		float *wi = (float *)alloca( sizeof(float) * dim );
		float *wk = (float *)alloca( sizeof(float) * 4 * dim );

		long n=dim, lda=dim, ldvl=dim, ldvr=dim, lwork = 4*dim, info;

		// This routine overwrite the destination so we make a copy
		ZMat inputCopy;
		inputCopy.copy( input );

		sgeev_( "N", "V", &n, inputCopy.getPtrF(0,0), &lda, wr, wi, vl, &ldvl, vr, &ldvr, wk, &lwork, &info );

		// @TODO: I tested this for real valued eigen values but there's
		// weirdness about the eigen vecs if the eigen values are complex
		// so this has to be testd and compared to matlab

		for( int row=0; row<dim; row++ ) {
			eigVals.setFC( row, 0, FComplex( wr[row], wi[row] ) );

			for( int col=0; col<dim; col++ ) {
				eigVecs.setF( row, col, vr[col*dim + row] );
			}
		}
	}
	else if( input.type == zmatF64 ) {
		int dim = input.rows;
		eigVecs.alloc( dim, dim, zmatF64 );
		eigVals.alloc( dim, 1  , zmatC64 );

		//real A[N*N], wr[N], wi[N], work[4*N], vl[N*N], vr[N*N];
		//float *A = (float *)alloca( sizeof(float) * dim * dim );
		double *vl = (double *)alloca( sizeof(double) * dim * dim );
		double *vr = (double *)alloca( sizeof(double) * dim * dim );
		double *wr = (double *)alloca( sizeof(double) * dim );
		double *wi = (double *)alloca( sizeof(double) * dim );
		double *wk = (double *)alloca( sizeof(double) * 4 * dim );

		long n=dim, lda=dim, ldvl=dim, ldvr=dim, lwork = 4*dim, info;

		// This routine overwrite the destination so we make a copy
		ZMat inputCopy;
		inputCopy.copy( input );

		dgeev_( "N", "V", &n, inputCopy.getPtrD(0,0), &lda, wr, wi, vl, &ldvl, vr, &ldvr, wk, &lwork, &info );

		// @TODO: I tested this for real valued eigen values but there's
		// weirdness about the eigen vecs if the eigen values are complex
		// so this has to be testd and compared to matlab

		// SORT by eigen value
		int row;
		double *sorted = (double *)alloca( dim * 2 * sizeof(double) );
		for( row=0; row<dim; row++ ) {
			sorted[row*2+0] = row;
			sorted[row*2+1] = wr[row]*wr[row] + wi[row]*wi[row];
		}
		qsort( sorted, dim, sizeof(double)*2, zmatEigen_CLAPACK_doubleSortComparator );

		for( row=0; row<dim; row++ ) {
			int sortedRow = (int)sorted[row*2];

			eigVals.setDC( row, 0, DComplex( wr[sortedRow], wi[sortedRow] ) );

			for( int col=0; col<dim; col++ ) {
				eigVecs.setD( row, col, vr[col*dim + sortedRow] );
			}
		}
	}
	else {
		assert( !"Unsupported matrix type in zmatEigen_CLAPACK" );
	}
}
