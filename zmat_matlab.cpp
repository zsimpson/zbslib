// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			ZMat fascade around Matlab routines
//		}
//		*SDK_DEPENDS matlab
//		*PORTABILITY win32 unix macosx
//		*REQUIRED_FILES zmat_matlab.cpp zmat_matlab.h
//		*VERSION 1.0
//		+HISTORY {
//			1.0 Created 9 June 2009
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }

// OPERATING SYSTEM specific includes:
// SDK includes:
#include "mat.h"
// STDLIB includes:
#include "sys/stat.h"
#include "string.h"
// MODULE includes:
#include "zmat_matlab.h"

int zmatLoad_Matlab( ZMat &loadInto, char *filename, char *varName ) {
	MATFile *f = matOpen( filename, "r" );
	assert( f );

	int count;
	char **dir = matGetDir( f, &count );
	if( ! varName ) {
		varName = dir[0];
	}

	mxArray *a = matGetVariable( f, varName );
	assert( a );
	
	const char *className = mxGetClassName( a );
	assert( !strcmp(className,"double") );
	// @TODO: Allow for other kinds of data

	mwSize elemSize = mxGetElementSize( a );
	assert( elemSize == sizeof(double) );
	// @TODO: Allow for other kinds of data, complex?

	mwSize numDims = mxGetNumberOfDimensions( a );
	assert( numDims = 2 );
	const mwSize *dims = mxGetDimensions( a );
	
	int rows = dims[0];
	int cols = dims[1];

	// ACCESS real-valued data as an array. Returns in column major order
	// @TODO: Deal with imaginary
	loadInto.copyD( (double *)mxGetData( a ), rows, cols );
	
	// this doesn't work because it tries to call the copy constructor. Argh
	return 1;
}

int zmatSave_Matlab( ZMat &saveFrom, char *filename, char *varName ) {
	assert( saveFrom.type == zmatF64 );
		// @TODO permit others types

	mxArray *m = mxCreateDoubleMatrix( saveFrom.rows, saveFrom.cols, mxREAL );

	memcpy( mxGetPr(m), saveFrom.mat, saveFrom.rows * saveFrom.cols * sizeof(double) );
	
	int exists = 0;
	struct stat s;
	if( stat(filename,&s)==0 ) {
		exists = 1;
	}

	char *mode = exists ? "u" : "w";

	MATFile *f = matOpen( filename, mode );
	if( !f ) {
		mxDestroyArray( m );
		return 0;
	}
	matPutVariable( f, varName, m );
	matClose( f );
	mxDestroyArray( m );

	return 1;
}
