// @ZBS {
//		*MODULE_NAME symmetricmatrix
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A class for handling large symmetric matricies
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES symmetricmatrix.cpp symmetricmatrix.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH yes
// }

// OPERATING SYSTEM specific includes:
// STDLIB includes:
#include "stdio.h"
#include "stdlib.h"
#include "memory.h"
#include "assert.h"
// MODULE includes:
#include "symmetricmatrix.h"
// ZBSLIB includes:


void SymmetricMatrix::set( int a, int b, void *t ) {
	void *tp = getPtr( a, b );
	memcpy( tp, t, elemSize );
}

void SymmetricMatrix::setF( int a, int b, float x ) {
	void *p = getPtr( a, b );
	memcpy( p, (void *)&x, elemSize );
}

void SymmetricMatrix::setD( int a, int b, double x ) {
	void *p = getPtr( a, b );
	memcpy( p, (void *)&x, elemSize );
}

int SymmetricMatrix::getI( int a, int b ) {
	return *(int *)getPtr( a, b );
}

float SymmetricMatrix::getF( int a, int b ) {
	return *(float *)getPtr( a, b );
}

double SymmetricMatrix::getD( int a, int b ) {
	return *(double *)getPtr( a, b );
}

void SymmetricMatrix::alloc( int _count, int _elemSize ) {
	free();
	count = _count;
	elemSize = _elemSize;
	table = (char *)malloc( getSizeInBytes() );
	memset( table, 0, getSizeInBytes() );
}

void SymmetricMatrix::free() {
	if( table ) {
		::free( table );
	}
	table = 0;
	count = 0;
	elemSize = 0;
}

void SymmetricMatrix::saveBin( char *filename ) {
	FILE *f = fopen( filename, "wb" );
	unsigned int count = getCount();
	unsigned int elemSize = getElemSize();
	unsigned int size = getSizeInBytes();
	int a;
	a = fwrite( &count, sizeof(count), 1, f );
	assert( a == 1 );
	a = fwrite( &elemSize, sizeof(elemSize), 1, f );
	assert( a == 1 );
	a = fwrite( &size, sizeof(size), 1, f );
	assert( a == 1 );
	a = fwrite( getPtr(0,0), size, 1, f );
	assert( a == 1 );
	fclose( f );
}

int SymmetricMatrix::loadBin( char *filename ) {
	FILE *f = fopen( filename, "rb" );
	if( f ) {
		unsigned int count = 0;
		unsigned int elemSize = 0;
		unsigned int size = 0;
		fread( &count, sizeof(count), 1, f );
		fread( &elemSize, sizeof(elemSize), 1, f );
		fread( &size, sizeof(size), 1, f );
		alloc( count, elemSize );
		assert( size == getSizeInBytes() );
		int c = fread( getPtr(0,0), size, 1, f );
		assert( c == 1 );
		fclose( f );
		return 1;
	}
	return 0;
}

int SymmetricMatrix::loadTxt( char *filename, int hasRowHeader, int hasColHeader ) {
	const int bufSize = 1024 * 1024;
	char *lineBuf = (char *)malloc( bufSize );

	FILE *f = fopen( filename, "rt" );
	if( f ) {
		// PASS 1: Count lines
		int lineCount = 0;
		while( fgets( lineBuf, bufSize, f ) ) {
			lineCount ++;
		}

		if( hasRowHeader ) {
			lineCount--;
		}

		// ALLOC
		alloc( lineCount, sizeof(float) );

		// PASS 2: Load
		fseek( f, 0, SEEK_SET );
		int _lineCount = 0;

		if( hasRowHeader ) {
			// SKIP the first line
			fgets( lineBuf, bufSize, f );
		}

		while( fgets( lineBuf, bufSize, f ) ) {
			char *lastStart = lineBuf;
			int colCount = 0;
			for( char *c = lineBuf; ; c++ ) {
				int isLineTerm = (*c == '\n' || *c == '\r' || !*c);
				if( *c == '\t' || isLineTerm ) {
					*c = 0;

					if( hasColHeader && lastStart == lineBuf ) {
						lastStart = c+1;
						continue;
					}

					setF( _lineCount, colCount, (float)atof(lastStart) );
					lastStart = c+1;
					colCount++;
					if( isLineTerm ) {
						_lineCount++;
						break;
					}
				}
			}
			assert( colCount == lineCount );
		}

		fclose( f );
		return 1;
	}
	return 0;
}

