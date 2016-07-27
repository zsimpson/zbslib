#ifndef ZMAT_H
#define ZMAT_H

#include "assert.h"
#include "stdio.h"

struct FComplex {
	float r, i;
	FComplex( float _r=0.f, float _i=0.f ) { r = _r; i = _i; }
};

struct DComplex {
	double r, i;
	DComplex( double _r=0.0, double _i=0.0 ) { r = _r; i = _i; }
};

struct ZMat {
	int rows, cols;
	int type;
		#define zmatU32 (101)
			// unsigned int
		#define zmatS32 (102)
			// signed int
		#define zmatF32 (103)
			// float
		#define zmatF64 (104)
			// double
		#define zmatC32 (105)
			// complex float
		#define zmatC64 (106)
			// complex double

	int elemSize;
	int bytePitch;
		// These two values are derived from the type. Do not modify.
	int flags;
		#define zmatNOTOWNER (1)
			// If this flag is set, the clear will not free the mat

	char *mat;
		// Stored in column-major order. i.e. each column is memory contigious [col][row]
		// This matches the ordering of both OpenGL and CLAPACK (based on Fortran's ordering)
		// It DOES NOT match the ordering of GSL, so all GSL wrappers need to switch

	// None of the following are type checked:
	unsigned int* getPtrU( int row=0, int col=0 ) { assert( row >= 0 && row < rows && col >=0 && col < cols ); return (unsigned int *)&mat[ col*bytePitch + row*elemSize ]; }
	int*          getPtrI( int row=0, int col=0 ) { assert( row >= 0 && row < rows && col >=0 && col < cols ); return (int *)&mat[ col*bytePitch + row*elemSize ]; }
	float*        getPtrF( int row=0, int col=0 ) { assert( row >= 0 && row < rows && col >=0 && col < cols ); return (float *)&mat[ col*bytePitch + row*elemSize ]; }
	double*       getPtrD( int row=0, int col=0 ) { assert( row >= 0 && row < rows && col >=0 && col < cols ); return (double *)&mat[ col*bytePitch + row*elemSize ]; }
	FComplex*     getPtrFC( int row=0, int col=0 ) { assert( row >= 0 && row < rows && col >=0 && col < cols ); return (FComplex *)&mat[ col*bytePitch + row*elemSize ]; }
	DComplex*     getPtrDC( int row=0, int col=0 ) { assert( row >= 0 && row < rows && col >=0 && col < cols ); return (DComplex *)&mat[ col*bytePitch + row*elemSize ]; }

	unsigned int getU( int row=0, int col=0 ) { return *getPtrU( row, col ); }
	int          getI( int row=0, int col=0 ) { return *getPtrI( row, col ); }
	float        getF( int row=0, int col=0 ) { return *getPtrF( row, col ); }
	double       getD( int row=0, int col=0 ) { return *getPtrD( row, col ); }
	FComplex     getFC( int row=0, int col=0 ) { return *getPtrFC( row, col ); }
	DComplex     getDC( int row=0, int col=0 ) { return *getPtrDC( row, col ); }

	void setU( int row, int col, unsigned int x ) { *getPtrU( row, col ) = x; }
	void setI( int row, int col, int x ) { *getPtrI( row, col ) = x; }
	void setF( int row, int col, float x ) { *getPtrF( row, col ) = x; }
	void setD( int row, int col, double x ) { *getPtrD( row, col ) = x; }
	void setFC( int row, int col, FComplex x ) { *getPtrFC( row, col ) = x; }
	void setDC( int row, int col, DComplex x ) { *getPtrDC( row, col ) = x; }

	// The following all do intelligent type-casting:
	unsigned int fetchU( int row=0, int col=0 );
	int          fetchI( int row=0, int col=0 );
	float        fetchF( int row=0, int col=0 );
	double       fetchD( int row=0, int col=0 );
	FComplex     fetchFC( int row=0, int col=0 );
	DComplex     fetchDC( int row=0, int col=0 );
	void putU( int row, int col, unsigned int x );
	void putI( int row, int col, int x );
	void putF( int row, int col, float x );
	void putD( int row, int col, double x );
	void putFC( int row, int col, FComplex x );
	void putDC( int row, int col, DComplex x );

	// The copyData functions assume that the source is the same size, type, and ordering as the destination
	void copyDataU( unsigned int *m );
	void copyDataI( int *m );
	void copyDataF( float *m );
	void copyDataD( double *m );
	void copyDataFC( FComplex *m );
	void copyDataDC( DComplex *m );
	void copyDataRow( ZMat &src, int srcRow, int dstRow );

	// The copy functions will reallocate if necessary to perform the copy
	void copyU( unsigned int *m, int _rows, int _cols );
	void copyI( int *m, int _rows, int _cols );
	void copyF( float *m, int _rows, int _cols );
	void copyD( double *m, int _rows, int _cols );
	void copyFC( FComplex *m, int _rows, int _cols );
	void copyDC( DComplex *m, int _rows, int _cols );

	void normalizeColumnsSumOne();
		// normalizes all columns to sum of 1

	void clear();
		// Free everything

	void alloc( int _rows, int _cols, int _type=zmatF32 );
		// Alloc with type define by consts above

	void copy( ZMat &mat );
	int binaryCompare( ZMat &ref );
		// performs a binary compare. Returns 1 if they are binary identical.

	void flipOrder();
		// Flips the memory ordering.

	void growCol( ZMat &rightMat );
		// Grows the cols by the size of rightMat.cols and places the value of rightMat on the right side
		// This is not efficent
		
	void growCol( double rightVal );
		// Assumes that the matrix is a row vector, adds this scalar to the right
		// This is not efficent

	void growRow( ZMat &botMat );
		// Grows the rows by the size of botMat.rows and places the value of botMat on the bottom
		// This is not efficent

	void growRow( double bottomVal );
		// Assumes that the matrix is a col vector, adds this scalar to the bottom
		
	void zero();
		// Fill with all zeros
		

	ZMat();
	ZMat( int _rows, int _cols, int _type=zmatF32 );
	ZMat( ZMat &copy );
	~ZMat();

	int loadBin( char *filename );
	int loadBin( FILE *f );
	int saveBin( char *filename );
	int saveBin( FILE *f );
		// The fileformat is a 4 byte version tag, sizeof(ZMat), and then a binary copy of this structure 
		// (including the ptr although it is trash) followed by a binary dump of the mat contents
		// Note that the there is currently no compensation of byte ordering

	int *saveBin();
	void loadBin( int *i );
		// Saves to an encoded string. The first in stores the length of the buffer
		// that should be written to a file for example

	int loadCSV( char *filename );
	int saveCSV( char *filename );
		// Maximum precision CSV

	void dump();
};

/*
struct ZMatSym {
	int size;
	int type;
		#define zmat_U_INT (1)
		#define zmat_S_INT (2)
		#define zmatF32 (3)
		#define zmat_DOUBLE (4)
		#define zmat_F_COMPLEX (5)
		#define zmat_D_COMPLEX (6)

	int elemSize;
	int bytePitch;
		// These two values are derived from the type. Do not modify.

	char *mat;
		// Stored in row-major order. i.e. each row is memory contigious [row][col]
		// Note that this is the transpose of OpenGL's ordering but matches MatLab
		// and any other typical matematical ordering

	// None of the following are type checked:
	

	unsigned int* getPtrU( int row, int col=0 ) { assert( row >= 0 && row < size && col >=0 && col < size ); if( row < col ) { int t = row; row = col; col = t; } return (unsigned int *)&mat[ ((row*(row+1))/2 + col) * elemSize ]; }
	int*          getPtrI( int row, int col=0 ) { assert( row >= 0 && row < size && col >=0 && col < size ); if( row < col ) { int t = row; row = col; col = t; } return (int *)&mat[ ((row*(row+1))/2 + col) * elemSize ]; }
	float*        getPtrF( int row, int col=0 ) { assert( row >= 0 && row < size && col >=0 && col < size ); if( row < col ) { int t = row; row = col; col = t; } return (float *)&mat[ ((row*(row+1))/2 + col) * elemSize ]; }
	double*       getPtrD( int row, int col=0 ) { assert( row >= 0 && row < size && col >=0 && col < size ); if( row < col ) { int t = row; row = col; col = t; } return (double *)&mat[ ((row*(row+1))/2 + col) * elemSize ]; }
	FComplex*     getPtrFC( int row, int col=0 ) { assert( row >= 0 && row < size && col >=0 && col < size ); if( row < col ) { int t = row; row = col; col = t; } return (FComplex *)&mat[ ((row*(row+1))/2 + col) * elemSize ]; }
	DComplex*     getPtrDC( int row, int col=0 ) { assert( row >= 0 && row < size && col >=0 && col < size ); if( row < col ) { int t = row; row = col; col = t; } return (DComplex *)&mat[ ((row*(row+1))/2 + col) * elemSize ]; }

	unsigned int getU( int row, int col ) { return *getPtrU( row, col ); }
	int          getI( int row, int col ) { return *getPtrI( row, col ); }
	float        getF( int row, int col ) { return *getPtrF( row, col ); }
	double       getD( int row, int col ) { return *getPtrD( row, col ); }
	FComplex     getFC( int row, int col ) { return *getPtrFC( row, col ); }
	DComplex     getDC( int row, int col ) { return *getPtrDC( row, col ); }

	void setU( int row, int col, unsigned int x ) { *getPtrU( row, col ) = x; }
	void setI( int row, int col, int x ) { *getPtrI( row, col ) = x; }
	void setF( int row, int col, float x ) { *getPtrF( row, col ) = x; }
	void setD( int row, int col, double x ) { *getPtrD( row, col ) = x; }
	void setFC( int row, int col, FComplex x ) { *getPtrFC( row, col ) = x; }
	void setDC( int row, int col, DComplex x ) { *getPtrDC( row, col ) = x; }

	// The following all do intelligent type-casting:
	unsigned int fetchU( int row, int col );
	int          fetchI( int row, int col );
	float        fetchF( int row, int col );
	double       fetchD( int row, int col );
	FComplex     fetchFC( int row, int col );
	DComplex     fetchDC( int row, int col );

	// The copyData functions assume that the source is the same size and type as the destination
	void copyDataU( unsigned int *m );
	void copyDataI( int *m );
	void copyDataF( float *m );
	void copyDataD( double *m );
	void copyDataFC( FComplex *m );
	void copyDataDC( DComplex *m );
	void copyDataRow( ZMat &src, int srcRow, int dstRow );

	// The copy functions will reallocate if necessary to perform the copy
	void copyU( unsigned int *m, int _rows, int _cols );
	void copyI( int *m, int _rows, int _cols );
	void copyF( float *m, int _rows, int _cols );
	void copyD( double *m, int _rows, int _cols );
	void copyFC( FComplex *m, int _rows, int _cols );
	void copyDC( DComplex *m, int _rows, int _cols );

	void normalizeColumnsSumOne();
		// normalizes all columns to sum of 1

	void clear();
		// Free everything

	void alloc( int _rows, int _cols, int _type );
		// Alloc with type define by consts above

	ZMat();
	ZMat( int _rows, int _cols, int _type=zmatF32 );
	ZMat( ZMat &copy );
	~ZMat();

	int loadBin( char *filename );
	int saveBin( char *filename );
		// Load and save store a 4 byte version and then a binary copy
		// of this structure (including the ptr although it is trash) followed
		// by a binary dump of the data
};
*/



// All of the matrix operations will check the dimensions of res.
// If they dimensions fit the desired result, it will not be reallocated
// otherwise it will be.
// See zmatlapack.h for adpator routines with 

void zmatIdentity( ZMat &mat );
	// Asserts that mat is NxN, makes it identity

double zmatDot( ZMat &lft, ZMat &rgt );
	// Given two equal sized vectors, compute dot

void zmatMul( ZMat &lft, ZMat &rgt, ZMat &res );
	// Multiply lft (NxM) by rgt (MxP) = res (NxP)

void zmatAdd( ZMat &lft, ZMat &rgt, ZMat &res );
	// Add lft and rgt into res

void zmatSub( ZMat &lft, ZMat &rgt, ZMat &res );
	// subtract rgt from lft into res

void zmatTranspose( ZMat &mat, ZMat &res );
	// Transpose mat (NxM) into res (MxN)

void zmatScalarMul( ZMat &lft, double scale, ZMat &res );
	// Multiply every term in lft by scale into res

void zmatElemMul( ZMat &lft, ZMat &rgt, ZMat &res );
	// Element multiply

void zmatElemDiv( ZMat &lft, ZMat &rgt, ZMat &res );
	// Element divide

void zmatGetCol( ZMat &mat, int col, ZMat &res );
	// Extract the given col from the mat into res

void zmatGetRow( ZMat &mat, int row, ZMat &res );
	// Extract the given row from the mat into res

void zmatNonNegativeFactorStart( ZMat &inputMat, ZMat &archtypeMat, ZMat &coeZMat, ZMat &errorMat, int rank );
void zmatNonNegativeFactorIterate( ZMat &inputMat, ZMat &archtypeMat, ZMat &coeZMat, ZMat &errorMat );
	// These both assert that all of the matiricies are of type float
	// Given an inputMat, do a non-negative matrix factorization into archtypeMat and coeZMat.
	// The three mats: archtypeMat, coeZMat, and errorMat are allocated by the start function and are modified by each call to iterate
	// The rank is the number of columns in the archtypeMat, i.e. how many archtypes you want

void zmatVectorToDiagonalMatrix( ZMat &vec, ZMat &mat );
	// Makes mat a vec.rows by vec.rows matrix and places the
	// vector into the diagonal
	
void zmatDiagonalMatrixToVector( ZMat &mat, ZMat &vec );
	// Makes the diagonal of the mat into a vector

void zmatSortColsByRow0Value( ZMat &mat );
	// qsorts the cols based on values in row 0

class ZMatLUSolver {
	// TODO?  This is really general linear algebra, and does not use
	// ZMat internally, but seems appropriate to place here with matrix.
	// See zmat_gsl and zmat_eigen for concrete subclasses.
protected:
	// the main reason I make this protected is to prevent construction
	// of these objects with null constructor.
	double *A;
		// input matrix, not owned.
	int dim;
		// dimension of A
	ZMatLUSolver() {}
public:
	int colMajor;
	// stride type

	ZMatLUSolver( double *_A, int _dim, int _colMajor=1 ) : A(_A), dim(_dim), colMajor(_colMajor) {}
	virtual ~ZMatLUSolver() {}
	virtual int decompose() = 0;
	virtual int solve( double *B, double *x ) = 0;
};


#endif


