// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Large general-sized matricies
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zmat.cpp zmat.h zendian.cpp zendian.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
#ifdef WIN32
	//#include "windows.h"
	// The following line is copied out of windows.h to avoid the header dependency
	// If there is a problem, comment it out and uncomment the include above.
	// This function is only used for the debugging function "debugDump()"
	// and could easily be removed entirely.
	extern "C" {
	__declspec(dllimport) void __stdcall OutputDebugStringA( const char *lpOutputString );
	}
	#define OutputDebugString OutputDebugStringA
#endif
// SDK includes:
// STDLIB includes:
#include "memory.h"
#include "assert.h"
#include "string.h"
#include "stdlib.h"
// MODULE includes:
#include "zmat.h"
#include "zendian.h"
// ZBSLIB includes:

unsigned int ZMat::fetchU( int r, int c) {
	switch( type ) {
		case zmatU32:     return (unsigned int)*getPtrU(r,c);
		case zmatS32:     return (unsigned int)*getPtrI(r,c);
		case zmatF32:     return (unsigned int)*getPtrF(r,c);
		case zmatF64:    return (unsigned int)*getPtrD(r,c);
		case zmatC32: return (unsigned int)getPtrFC(r,c)->r; // returns real part
		case zmatC64: return (unsigned int)getPtrDC(r,c)->r; // returns real part
	}
	assert( 0 );
	return 0;
}

int          ZMat::fetchI( int r, int c ) {
	switch( type ) {
		case zmatU32:     return (int)*getPtrU(r,c);
		case zmatS32:     return (int)*getPtrI(r,c);
		case zmatF32:     return (int)*getPtrF(r,c);
		case zmatF64:    return (int)*getPtrD(r,c);
		case zmatC32: return (int)getPtrFC(r,c)->r; // returns real part
		case zmatC64: return (int)getPtrDC(r,c)->r; // returns real part
	}
	assert( 0 );
	return 0;
}

float        ZMat::fetchF( int r, int c ) {
	switch( type ) {
		case zmatU32:     return (float)*getPtrU(r,c);
		case zmatS32:     return (float)*getPtrI(r,c);
		case zmatF32:     return (float)*getPtrF(r,c);
		case zmatF64:    return (float)*getPtrD(r,c);
		case zmatC32: return (float)getPtrFC(r,c)->r; // returns real part
		case zmatC64: return (float)getPtrDC(r,c)->r; // returns real part
	}
	assert( 0 );
	return 0.f;
}

double       ZMat::fetchD( int r, int c ) {
	switch( type ) {
		case zmatU32:     return (double)*getPtrU(r,c);
		case zmatS32:     return (double)*getPtrI(r,c);
		case zmatF32:     return (double)*getPtrF(r,c);
		case zmatF64:    return (double)*getPtrD(r,c);
		case zmatC32: return (double)getPtrFC(r,c)->r; // returns real part
		case zmatC64: return (double)getPtrDC(r,c)->r; // returns real part
	}
	assert( 0 );
	return 0.0;
}

FComplex     ZMat::fetchFC( int r, int c ) {
	switch( type ) {
		case zmatU32:     return FComplex( (float)*getPtrU(r,c) );
		case zmatS32:     return FComplex( (float)*getPtrI(r,c) );
		case zmatF32:     return FComplex( (float)*getPtrF(r,c) );
		case zmatF64:    return FComplex( (float)*getPtrD(r,c) );
		case zmatC32: return *getPtrFC(r,c);
		case zmatC64: double *d = (double *)getPtrDC(r,c); return FComplex( (float)d[0], (float)d[1] );
	}
	assert( 0 );
	return FComplex();
}

DComplex     ZMat::fetchDC( int r, int c ) {
	switch( type ) {
		case zmatU32:     return DComplex( (float)*getPtrU(r,c) );
		case zmatS32:     return DComplex( (float)*getPtrI(r,c) );
		case zmatF32:     return DComplex( (float)*getPtrF(r,c) );
		case zmatF64:    return DComplex( (float)*getPtrD(r,c) );
		case zmatC32: { float *f = (float *)getPtrFC(r,c); return DComplex( (double)f[0], (double)f[1] ); }
		case zmatC64: return *getPtrDC(r,c);
	}
	assert( 0 );
	return DComplex();
}

void ZMat::putU( int r, int c, unsigned int x ) {
	switch( type ) {
		case zmatU32:     *getPtrU(r,c) = (unsigned int)x; break;
		case zmatS32:     *getPtrI(r,c) = (int)x; break;
		case zmatF32:     *getPtrF(r,c) = (float)x; break;
		case zmatF64:    *getPtrD(r,c) = (double)x; break;
		case zmatC32: getPtrFC(r,c)->r = (float)x; getPtrFC(r,c)->i = 0.f; break;
		case zmatC64: getPtrDC(r,c)->r = (double)x; getPtrDC(r,c)->i = 0.0; break;
	}
}

void ZMat::putI( int r, int c, int x ) {
	switch( type ) {
		case zmatU32:     *getPtrU(r,c) = (unsigned int)x; break;
		case zmatS32:     *getPtrI(r,c) = (int)x; break;
		case zmatF32:     *getPtrF(r,c) = (float)x; break;
		case zmatF64:    *getPtrD(r,c) = (double)x; break;
		case zmatC32: getPtrFC(r,c)->r = (float)x; getPtrFC(r,c)->i = 0.f; break;
		case zmatC64: getPtrDC(r,c)->r = (double)x; getPtrDC(r,c)->i = 0.0; break;
	}
}

void ZMat::putF( int r, int c, float x ) {
	switch( type ) {
		case zmatU32:     *getPtrU(r,c) = (unsigned int)x; break;
		case zmatS32:     *getPtrI(r,c) = (int)x; break;
		case zmatF32:     *getPtrF(r,c) = (float)x; break;
		case zmatF64:    *getPtrD(r,c) = (double)x; break;
		case zmatC32: getPtrFC(r,c)->r = (float)x; getPtrFC(r,c)->i = 0.f; break;
		case zmatC64: getPtrDC(r,c)->r = (double)x; getPtrDC(r,c)->i = 0.0; break;
	}
}

void ZMat::putD( int r, int c, double x ) {
	switch( type ) {
		case zmatU32:     *getPtrU(r,c) = (unsigned int)x; break;
		case zmatS32:     *getPtrI(r,c) = (int)x; break;
		case zmatF32:     *getPtrF(r,c) = (float)x; break;
		case zmatF64:    *getPtrD(r,c) = (double)x; break;
		case zmatC32: getPtrFC(r,c)->r = (float)x; getPtrFC(r,c)->i = 0.f; break;
		case zmatC64: getPtrDC(r,c)->r = (double)x; getPtrDC(r,c)->i = 0.0; break;
	}
}

void ZMat::putFC( int r, int c, FComplex x ) {
	switch( type ) {
		case zmatU32:     *getPtrU(r,c) = (unsigned int)x.r; break;
		case zmatS32:     *getPtrI(r,c) = (int)x.r; break;
		case zmatF32:     *getPtrF(r,c) = (float)x.r; break;
		case zmatF64:    *getPtrD(r,c) = (double)x.r; break;
		case zmatC32: getPtrFC(r,c)->r = (float)x.r; getPtrFC(r,c)->i = (float)x.i; break;
		case zmatC64: getPtrDC(r,c)->r = (double)x.r; getPtrDC(r,c)->i = (double)x.i; break;
	}
}

void ZMat::putDC( int r, int c, DComplex x ) {
	switch( type ) {
		case zmatU32:     *getPtrU(r,c) = (unsigned int)x.r; break;
		case zmatS32:     *getPtrI(r,c) = (int)x.r; break;
		case zmatF32:     *getPtrF(r,c) = (float)x.r; break;
		case zmatF64:    *getPtrD(r,c) = (double)x.r; break;
		case zmatC32: getPtrFC(r,c)->r = (float)x.r; getPtrFC(r,c)->i = (float)x.i; break;
		case zmatC64: getPtrDC(r,c)->r = (double)x.r; getPtrDC(r,c)->i = (double)x.i; break;
	}
}


// The copyData functions assume that the source is the same size and type as the destination
void ZMat::copyDataU( unsigned int *m ) {
	assert( type == zmatU32 );
	memcpy( mat, m, elemSize * rows * cols );
}

void ZMat::copyDataI( int *m ) {
	assert( type == zmatS32 );
	memcpy( mat, m, elemSize * rows * cols );
}

void ZMat::copyDataF( float *m ) {
	assert( type == zmatF32 );
	memcpy( mat, m, elemSize * rows * cols );
}

void ZMat::copyDataD( double *m ) {
	assert( type == zmatF64 );
	memcpy( mat, m, elemSize * rows * cols );
}

void ZMat::copyDataFC( FComplex *m ) {
	assert( type == zmatC32 );
	memcpy( mat, m, elemSize * rows * cols );
}

void ZMat::copyDataDC( DComplex *m ) {
	assert( type == zmatC64 );
	memcpy( mat, m, elemSize * rows * cols );
}

void ZMat::copyDataRow( ZMat &src, int srcRow, int dstRow ) {
	assert( type == src.type && cols == src.cols );
	for( int c=0; c<cols; c++ ) {
		memcpy( getPtrU(dstRow,c), src.getPtrU(srcRow,c), elemSize );
	}
}

// The copy functions will reallocate if necessary to perform the copy
void ZMat::copyU( unsigned int *m, int _rows, int _cols ) {
	if( rows != _rows || cols != _cols || type != zmatU32 ) {
		alloc( _rows, _cols, zmatU32 );
	}
	memcpy( mat, m, elemSize * rows * cols );
}

void ZMat::copyI( int *m, int _rows, int _cols ) {
	if( rows != _rows || cols != _cols || type != zmatS32 ) {
		alloc( _rows, _cols, zmatS32 );
	}
	memcpy( mat, m, elemSize * rows * cols );
}

void ZMat::copyF( float *m, int _rows, int _cols ) {
	if( rows != _rows || cols != _cols || type != zmatF32 ) {
		alloc( _rows, _cols, zmatF32 );
	}
	memcpy( mat, m, elemSize * rows * cols );
}

void ZMat::copyD( double *m, int _rows, int _cols ) {
	if( rows != _rows || cols != _cols || type != zmatF64 ) {
		alloc( _rows, _cols, zmatF64 );
	}
	memcpy( mat, m, elemSize * rows * cols );
}

void ZMat::copyFC( FComplex *m, int _rows, int _cols ) {
	if( rows != _rows || cols != _cols || type != zmatC32 ) {
		alloc( _rows, _cols, zmatC32 );
	}
	memcpy( mat, m, elemSize * rows * cols );
}

void ZMat::copyDC( DComplex *m, int _rows, int _cols ) {
	if( rows != _rows || cols != _cols || type != zmatC64 ) {
		alloc( _rows, _cols, zmatC64 );
	}
	memcpy( mat, m, elemSize * rows * cols );
}

void ZMat::normalizeColumnsSumOne() {
	// NORMALIZE all columns to sum of 1
	int c, r;
	int _cols = cols;
	int _rows = rows;
	for( c=0; c<_cols; c++ ) {

		switch( type ) {
			case zmatU32: {
				double sum = 0.0;
				unsigned int *m = getPtrU( 0, c );
				for( r=0; r<_rows; r++ ) { sum += (double)*m; m ++; }
				m = getPtrU( 0, c );
				for( r=0; r<_rows; r++ ) { *m = (unsigned int)( (double)*m / sum ); m ++; }
				break;
			}
			case zmatS32: {
				double sum = 0.0;
				int *m = getPtrI( 0, c );
				for( r=0; r<_rows; r++ ) { sum += (double)*m; m ++; }
				m = getPtrI( 0, c );
				for( r=0; r<_rows; r++ ) { *m = (int)( (double)*m / sum ); m ++; }
				break;
			}
			case zmatF32: {
				double sum = 0.0;
				float *m = getPtrF( 0, c );
				for( r=0; r<_rows; r++ ) { sum += (double)*m; m ++; }
				m = getPtrF( 0, c );
				for( r=0; r<_rows; r++ ) { *m = (float)( (double)*m / sum ); m ++; }
				break;
			}
			case zmatF64: {
				double sum = 0.0;
				double *m = getPtrD( 0, c );
				for( r=0; r<_rows; r++ ) { sum += (double)*m; m ++; }
				m = getPtrD( 0, c );
				for( r=0; r<_rows; r++ ) { *m = (double)( (double)*m / sum ); m ++; }
				break;
			}
			default:
				// Unknown function for complex
				assert( 0 );
		}
	}
}

void ZMat::clear() {
	// FREE everything
	rows = 0;
	cols = 0;
	type = 0;
	elemSize = 0;
	bytePitch = 0;
	if( !(flags & zmatNOTOWNER) ) {
		if( mat ) {
			::free( mat );
			mat = 0;
		}
	}
	flags = 0;
}

void ZMat::alloc( int _rows, int _cols, int _type ) {
	// ALLOC with type define by consts above
	clear();
	rows = _rows;
	cols = _cols;
	type = _type;
	flags = 0;

	switch( type ) {
		case zmatU32: elemSize = sizeof(unsigned int); break;
		case zmatS32: elemSize = sizeof(int); break;
		case zmatF32: elemSize = sizeof(float); break;
		case zmatF64: elemSize = sizeof(double); break;
		case zmatC32: elemSize = sizeof(FComplex); break;
		case zmatC64: elemSize = sizeof(DComplex); break;
		default: assert( 0 );
	}
	bytePitch = elemSize * rows;
	mat = (char *)malloc( elemSize * rows * cols );
	memset( mat, 0, elemSize * rows * cols );
}

void ZMat::copy( ZMat &m ) {
	clear();
	if( m.type ) {
		alloc( m.rows, m.cols, m.type );
		memcpy( mat, m.mat, elemSize * rows * cols );
	}
}

int ZMat::binaryCompare( ZMat &ref ) {
	if( rows != ref.rows ) return 0;
	if( cols != ref.cols ) return 0;
	if( type != ref.type ) return 0;
	return memcmp( mat, ref.mat, elemSize * rows * cols ) == 0;
}

ZMat::ZMat() {
	rows = 0;
	cols = 0;
	type = 0;
	elemSize = 0;
	bytePitch = 0;
	mat = 0;
	flags = 0;
}

ZMat::ZMat( int _rows, int _cols, int _type ) {
	mat = 0;
	alloc( _rows, _cols, _type );
}

ZMat::ZMat( ZMat &copy ) {
	mat = 0;
	alloc( copy.rows, copy.cols, copy.type );
	memcpy( mat, copy.mat, elemSize * rows * cols );
}

ZMat::~ZMat() {
	clear();
}

int ZMat::saveBin( char *filename ) {
	FILE *f = fopen( filename, "wb" );
	if( f ) {
		int retVal = saveBin( f );
		fclose( f );
		return retVal;
	}
	return 0;
}

int ZMat::saveBin( FILE *f ) {
	// V 2.0 was row-major.
	// V 3.0 is col-major
	// V 4.0 added flags data member and saves sizeof(ZMat)
	// V 5.0 saves members individually to avoid writing pointer to disk
	//       as the latter makes zmat data incompat across 32-64 bit platforms.
	//		 Also saves endianess, for potential byteswap on load

	char *header = "ZMAT5.0\0";
	fwrite( header, 8, 1, f );

	int bigEndian = machineIsBigEndian();
	fwrite( &bigEndian, sizeof(bigEndian), 1, f );
		// save endianness

	// save each member instead of fwrite entire struct:
	// (don't write pointer)
	fwrite( &rows, sizeof(rows), 1, f );
	fwrite( &cols, sizeof(cols), 1, f );
	fwrite( &type, sizeof(type), 1, f );
	fwrite( &elemSize, sizeof(elemSize), 1, f );
	fwrite( &bytePitch, sizeof(bytePitch), 1, f );
	fwrite( &flags, sizeof(flags), 1, f );

	fwrite( mat, elemSize * rows * cols, 1, f );
	return 1;
}

int *ZMat::saveBin() {
	int size = sizeof(int)*10 + elemSize*rows*cols;
	int *i = (int *)malloc( size );
	int *j = i;
	*j++ = size;

	memcpy( j, "ZMAT5.0\0", 8 );
	j += 2;

	*j++ = machineIsBigEndian();
	*j++ = rows;
	*j++ = cols;
	*j++ = type;
	*j++ = elemSize;
	*j++ = bytePitch;
	*j++ = flags;
	memcpy( j, mat, elemSize * rows * cols );
	
	return i;
}

int ZMat::loadBin( char *filename ) {
	FILE *f = fopen( filename, "rb" );
	if( f ) {
		int retVal = loadBin( f );
		fclose( f );
		return retVal;
	}
	return 0;
}

int ZMat::loadBin( FILE *f ) {
	char header[8];
	fread( header, 1, 8, f );

	int shouldByteSwap = 0;

	if( !strcmp(header,"ZMAT4.0") ) {
		int zsize;
		fread( &zsize, sizeof(zsize), 1, f );
		if( zsize != sizeof( ZMat ) ) {
			return 0;
		}
		fread( this, sizeof(*this), 1, f );
	}
	else if ( !strcmp(header,"ZMAT5.0") ) {
		int dataEndian;
		fread( &dataEndian, sizeof(dataEndian), 1, f );
		dataEndian = dataEndian != 0 ? 1 : 0;
			// force to 1 or 0, since if endianness differs, dataEndian will not be 0x01
		shouldByteSwap = (int)( dataEndian != machineIsBigEndian() );

		freadEndian( &rows, sizeof(rows), 1, f, shouldByteSwap );
		freadEndian( &cols, sizeof(cols), 1, f, shouldByteSwap );
		freadEndian( &type, sizeof(type), 1, f, shouldByteSwap );
		freadEndian( &elemSize, sizeof(elemSize), 1, f, shouldByteSwap );
		freadEndian( &bytePitch, sizeof(bytePitch), 1, f, shouldByteSwap );
		freadEndian( &flags, sizeof(flags), 1, f, shouldByteSwap );
	}
	else {
		return 0;
	}
	if( rows * cols ) {
		alloc( rows, cols, type );
		freadEndian( mat, elemSize, rows * cols, f, shouldByteSwap );
	}
	return 1;
}

void ZMat::loadBin( int *j ) {
	clear();
	
	int size = *j++;
	assert( ! memcmp(j,"ZMAT5.0\0",8) );
	j += 2;
	
	int endian = *j++;
	assert( endian == machineIsBigEndian() );
	// @TODO: Need to reverse if necessary here

	rows = *j++;
	cols = *j++;
	type = *j++;
	elemSize = *j++;
	bytePitch = *j++;
	flags = *j++;
	mat = (char *)malloc( elemSize * rows * cols );
	memcpy( mat, j, elemSize * rows * cols );
}



static char *zmatTypeStrings[] = { "uint", "int", "float", "double", "fcomplex", "dcomplex" };

int ZMat::loadCSV( char *filename ) {
	FILE *f = fopen( filename, "rt" );
	if( f ) {
		fscanf( f, "rows,%d\n", &rows );
		fscanf( f, "cols,%d\n", &cols );
		char typeStr[256];
		fscanf( f, "type,%s\n", typeStr );

		     if( !strcmp(typeStr,zmatTypeStrings[0]) ) type = zmatU32;
		else if( !strcmp(typeStr,zmatTypeStrings[1]) ) type = zmatS32;
		else if( !strcmp(typeStr,zmatTypeStrings[2]) ) type = zmatF32;
		else if( !strcmp(typeStr,zmatTypeStrings[3]) ) type = zmatF64;
		else if( !strcmp(typeStr,zmatTypeStrings[4]) ) type = zmatC32;
		else if( !strcmp(typeStr,zmatTypeStrings[5]) ) type = zmatC64;

		alloc( rows, cols, type );

		for( int r=0; r<rows; r++ ) {
			for( int c=0; c<cols; c++ ) {
				unsigned int uintx;
				int intx;
				float floatx;
				double doublex;
				float fcomplexr, fcomplexi;
				double dcomplexr, dcomplexi;
				switch( type ) {
					case zmatU32: fscanf( f, "%ud", &uintx ); putU(r,c,uintx); break;
					case zmatS32: fscanf( f, "%d", &intx ); putI(r,c,intx); break;
					case zmatF32: fscanf( f, "%f", &floatx ); putF(r,c,floatx); break;
					case zmatF64: fscanf( f, "%lf", &doublex ); putD(r,c,doublex); break;
					case zmatC32: fscanf( f, "%f,%fi", &fcomplexr, &fcomplexi ); putFC(r,c,FComplex(fcomplexr,fcomplexi)); break;
					case zmatC64: fscanf( f, "%lf,%lfi", &dcomplexr, &dcomplexi ); putDC(r,c,DComplex(dcomplexr,dcomplexi)); break;
					default: assert( 0 );
				}
				if( c != cols-1 ) {
					fscanf( f, "," );
				}
			}
			fscanf( f, "\n" );
		}
		fclose( f );
		return 1;
	}
	return 0;
}

int ZMat::saveCSV( char *filename ) {
	FILE *f = fopen( filename, "wt" );
	if( f ) {
		fprintf( f, "rows,%d\n", rows );
		fprintf( f, "cols,%d\n", cols );
		fprintf( f, "type,%s\n", zmatTypeStrings[type-zmatU32] );
		for( int r=0; r<rows; r++ ) {
			for( int c=0; c<cols; c++ ) {
				switch( type ) {
					case zmatU32: fprintf( f, "%ud", getU(r,c) ); break;
					case zmatS32: fprintf( f, "%d", getI(r,c) ); break;
					case zmatF32: fprintf( f, "%f", getF(r,c) ); break;
					case zmatF64: fprintf( f, "%lf", getD(r,c) ); break;
					case zmatC32: fprintf( f, "%f,%fi", getFC(r,c).r, getFC(r,c).i ); break;
					case zmatC64: fprintf( f, "%lf,%lfi", getDC(r,c).r, getDC(r,c).i ); break;
					default: assert( 0 );
				}
				if( c != cols-1 ) {
					fprintf( f, "," );
				}
			}
			fprintf( f, "\n" );
		}
		fclose( f );
		return 1;
	}
	return 0;
}

void ZMat::dump() {
	char buf[4096] = {0,};

	for( int r=0; r<rows; r++ ) {
		for( int c=0; c<cols; c++ ) {
			sprintf( buf+strlen(buf), "%+3.2le ", fetchD(r,c) );
		}
		sprintf( buf+strlen(buf), "\n" );
	}

	#ifdef WIN32
		OutputDebugString( buf );
	#else
		printf( "%s", buf );
	#endif
}

void ZMat::flipOrder() {
	char *temp = (char *)malloc( elemSize * rows * cols );

	for( int r=0; r<rows; r++ ) {
		for( int c=0; c<cols; c++ ) {
			memcpy( &temp[(r*cols+c)*elemSize], &mat[(c*rows+r)*elemSize], elemSize );
		}
	}

	memcpy( mat, temp, elemSize * rows * cols );

	free( temp );
}

void ZMat::growCol( ZMat &rightMat ) {
	int c;
	assert( type == 0 || type == rightMat.type );

	int newRows = rows > rightMat.rows ? rows : rightMat.rows;
	int newCols = cols + rightMat.cols;

	ZMat newMat( newRows, newCols, rightMat.type );

	// COPY into newMat the old upper left rectangle
	for( c=0; c<cols; c++ ) {
		memcpy( (void *)newMat.getPtrI(0,c), (void *)this->getPtrI(0,c), elemSize * rows );
	}

	// COPY into newMat the new upper right rectangle
	for( c=0; c<rightMat.cols; c++ ) {
		memcpy( (void *)newMat.getPtrI(0,this->cols+c), (void *)rightMat.getPtrI(0,c), rightMat.elemSize * rightMat.rows );
	}

	// FREE memory from current
	clear();

	// SWAP the values from newMat into the current
	memcpy( this, &newMat, sizeof(*this) );
	
	// PREVENT the newMat from preeing the memory
	newMat.flags = zmatNOTOWNER;
}

void ZMat::growCol( double rightVal ) {
	ZMat rgt( 1, 1, zmatF64 );
	rgt.putD( 0, 0, rightVal );
	growCol( rgt );
}

void ZMat::growRow( double bottomVal ) {
	ZMat bot( 1, 1, zmatF64 );
	bot.putD( 0, 0, bottomVal );
	growRow( bot );
}

void ZMat::growRow( ZMat &botMat ) {
	int c;
	assert( type == 0 || type == botMat.type );

	int newRows = rows + botMat.rows;
	int newCols = cols > botMat.cols ? cols : botMat.cols;

	ZMat newMat( newRows, newCols, botMat.type );

	// COPY into newMat the old upper left rectangle
	for( c=0; c<cols; c++ ) {
		memcpy( (void *)newMat.getPtrI(0,c), (void *)this->getPtrI(0,c), elemSize * rows );
	}

	// COPY into newMat the new lower bot rectangle
	for( c=0; c<botMat.cols; c++ ) {
		memcpy( (void *)newMat.getPtrI(this->rows,c), (void *)botMat.getPtrI(0,c), botMat.elemSize * botMat.rows );
	}

	// FREE memory from current
	clear();

	// SWAP the values from newMat into the current
	memcpy( this, &newMat, sizeof(*this) );
	
	// PREVENT the newMat from preeing the memory
	newMat.flags = zmatNOTOWNER;
}

void ZMat::zero() {
	memset( mat, 0, elemSize * rows * cols );
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// NON-METHOD functions
//

void zmatIdentity( ZMat &mat ) {
	assert( mat.rows == mat.cols );
	memset( mat.mat, 0, mat.elemSize * mat.rows * mat.cols );
	for( int i=0; i<mat.rows; i++ ) {
		mat.setD( i, i, 1.0 );
	}
}

double zmatDot( ZMat &lft, ZMat &rgt ) {
	int i;
	assert( lft.rows == rgt.rows && lft.cols == rgt.cols );
	double sum = 0.0;
	if( lft.rows == 1 ) {
		switch( lft.type ) {
			case zmatU32: {
				for( i=0; i<lft.cols; i++ ) {
					sum += lft.getU( 0, i ) * rgt.getU( 0, i );
				}
				break;
			}
			case zmatS32: {
				for( i=0; i<lft.cols; i++ ) {
					sum += lft.getI( 0, i ) * rgt.getI( 0, i );
				}
				break;
			}
			case zmatF32: {
				for( i=0; i<lft.cols; i++ ) {
					sum += lft.getF( 0, i ) * rgt.getF( 0, i );
				}
				break;
			}
			case zmatF64: {
				for( i=0; i<lft.cols; i++ ) {
					sum += lft.getD( 0, i ) * rgt.getD( 0, i );
				}
				break;
			}
			case zmatC32: {
				assert( 0 );
					// @TODO
				break;
			}
			case zmatC64: {
				assert( 0 );
					// @TODO
				break;
			}
		}
	}
	else if( lft.cols == 1 ) {
		switch( lft.type ) {
			case zmatU32: {
				for( i=0; i<lft.rows; i++ ) {
					sum += lft.getU( i, 0 ) * rgt.getU( i, 0 );
				}
				break;
			}
			case zmatS32: {
				for( i=0; i<lft.rows; i++ ) {
					sum += lft.getI( i, 0 ) * rgt.getI( i, 0 );
				}
				break;
			}
			case zmatF32: {
				for( i=0; i<lft.rows; i++ ) {
					sum += lft.getF( i, 0 ) * rgt.getF( i, 0 );
				}
				break;
			}
			case zmatF64: {
				for( i=0; i<lft.rows; i++ ) {
					sum += lft.getD( i, 0 ) * rgt.getD( i, 0 );
				}
				break;
			}
			case zmatC32: {
				assert( 0 );
					// @TODO
				break;
			}
			case zmatC64: {
				assert( 0 );
					// @TODO
				break;
			}
		}
	}
	else {
		assert( 0 );
			// Vectors only!
	}
	return sum;
}

void zmatMul( ZMat &lft, ZMat &rgt, ZMat &res ) {
	// Multiply lft (NxM) by rgt (MxP) = res (NxP)

	assert( lft.cols == rgt.rows && lft.type == rgt.type );

	int resRows = lft.rows;
	int resCols = rgt.cols;

	if( res.rows != resRows || res.cols != resCols ) {
		res.alloc( resRows, resCols, lft.type );
	}
	
	int innerCount = lft.cols;
	int lftcols = lft.cols;

	switch( lft.type ) {
		case zmatU32: {
			for( int r=0; r<resRows; r++ ) {
				for( int c=0; c<resCols; c++ ) {
					unsigned int sum = 0;
					unsigned int *lftRow = lft.getPtrU( r, 0 );
					unsigned int *rgtCol = rgt.getPtrU( 0, c );
					for( int i=0; i<innerCount; i++ ) {
						sum += (*lftRow) * (*rgtCol);
						lftRow += resRows;
						rgtCol ++;
					}
					res.setU( r, c, sum );
				}
			}
			break;
		}
		case zmatS32: {
			for( int r=0; r<resRows; r++ ) {
				for( int c=0; c<resCols; c++ ) {
					int sum = 0;
					int *lftRow = lft.getPtrI( r, 0 );
					int *rgtCol = rgt.getPtrI( 0, c );
					for( int i=0; i<innerCount; i++ ) {
						sum += (*lftRow) * (*rgtCol);
						lftRow += resRows;
						rgtCol ++;
					}
					res.setI( r, c, sum );
				}
			}
			break;
		}
		case zmatF32: {
			for( int r=0; r<resRows; r++ ) {
				for( int c=0; c<resCols; c++ ) {
					float sum = 0.f;
					float *lftRow = lft.getPtrF( r, 0 );
					float *rgtCol = rgt.getPtrF( 0, c );
					for( int i=0; i<innerCount; i++ ) {
						sum += (*lftRow) * (*rgtCol);
						lftRow += resRows;
						rgtCol ++;
					}
					res.setF( r, c, sum );
				}
			}
			break;
		}
		case zmatF64: {
			for( int r=0; r<resRows; r++ ) {
				for( int c=0; c<resCols; c++ ) {
					double sum = 0.0;
					double *lftRow = lft.getPtrD( r, 0 );
					double *rgtCol = rgt.getPtrD( 0, c );
					for( int i=0; i<innerCount; i++ ) {
						sum += (*lftRow) * (*rgtCol);
						lftRow += resRows;
						rgtCol ++;
					}
					res.setD( r, c, sum );
				}
			}
			break;
		}
		case zmatC32: {
			for( int r=0; r<resRows; r++ ) {
				for( int c=0; c<resCols; c++ ) {
					FComplex sum;
					FComplex *lftRow = lft.getPtrFC( r, 0 );
					FComplex *rgtCol = rgt.getPtrFC( 0, c );
					for( int i=0; i<innerCount; i++ ) {
						sum.r += lftRow->r * rgtCol->r - lftRow->i * rgtCol->i;
						sum.i += lftRow->r * rgtCol->i + lftRow->i * rgtCol->r;
						lftRow += resRows;
						rgtCol ++;
					}
					res.setFC( r, c, sum );
				}
			}
			break;
		}
		case zmatC64: {
			for( int r=0; r<resRows; r++ ) {
				for( int c=0; c<resCols; c++ ) {
					DComplex sum;
					DComplex *lftRow = lft.getPtrDC( r, 0 );
					DComplex *rgtCol = rgt.getPtrDC( 0, c );
					for( int i=0; i<innerCount; i++ ) {
						sum.r += lftRow->r * rgtCol->r - lftRow->i * rgtCol->i;
						sum.i += lftRow->r * rgtCol->i + lftRow->i * rgtCol->r;
						lftRow += resRows;
						rgtCol ++;
					}
					res.setDC( r, c, sum );
				}
			}
			break;
		}
	}
}

void zmatTranspose( ZMat &mat, ZMat &res ) {
	// Transpose mat (NxM) into res (MxN)
	int srcRows = mat.rows;
	int srcCols = mat.cols;
	int resRows = mat.cols;
	int resCols = mat.rows;
	if( res.rows != resRows || res.cols != resCols || res.type != mat.type ) {
		res.alloc( resRows, resCols, mat.type );
	}

	int elemSize = mat.elemSize;

	for( int r=0; r<srcRows; r++ ) {
		char *srcRow = (char *)mat.getPtrU(r,0);
		char *dstCol = (char *)res.getPtrU(0,r);

		for( int c=0; c<resRows; c++ ) {
			memcpy( dstCol, srcRow, elemSize );
			srcRow += mat.bytePitch;
			dstCol += elemSize;
		}
	}
}

void zmatScalarMul( ZMat &lft, double scale, ZMat &res ) {
	int rows = lft.rows;
	int cols = lft.cols;

	if( res.rows != rows || res.cols != cols || res.type != lft.type ) {
		res.alloc( rows, cols, lft.type );
	}

	switch( lft.type ) {
		case zmatU32: {
			for( int r=0; r<rows; r++ ) {
				for( int c=0; c<cols; c++ ) {
					res.setU( r, c, (unsigned int)( (double)lft.getU( r, c ) * scale ) );
				}
			}
			break;
		}
		case zmatS32: {
			for( int r=0; r<rows; r++ ) {
				for( int c=0; c<cols; c++ ) {
					res.setI( r, c, (int)( (double)lft.getI( r, c ) * scale ) );
				}
			}
			break;
		}
		case zmatF32: {
			for( int r=0; r<rows; r++ ) {
				for( int c=0; c<cols; c++ ) {
					res.setF( r, c, (float)( (double)lft.getF( r, c ) * scale ) );
				}
			}
			break;
		}
		case zmatF64: {
			for( int r=0; r<rows; r++ ) {
				for( int c=0; c<cols; c++ ) {
					res.setD( r, c, (double)( (double)lft.getD( r, c ) * scale ) );
				}
			}
			break;
		}
		case zmatC32: {
			for( int r=0; r<rows; r++ ) {
				for( int c=0; c<cols; c++ ) {
					FComplex f = lft.getFC( r, c );
					f.i = (float)( (double)f.i * scale );
					f.r = (float)( (double)f.r * scale );
					res.setFC( r, c, f );
				}
			}
			break;
		}
		case zmatC64: {
			for( int r=0; r<rows; r++ ) {
				for( int c=0; c<cols; c++ ) {
					DComplex f = lft.getDC( r, c );
					f.i = (double)( (double)f.i * scale );
					f.r = (double)( (double)f.r * scale );
					res.setDC( r, c, f );
				}
			}
			break;
		}
	}
}

void zmatElemMul( ZMat &lft, ZMat &rgt, ZMat &res ) {
	int rows = lft.rows;
	int cols = lft.cols;

	assert( lft.rows == rgt.rows );
	assert( lft.cols == rgt.cols );
	assert( lft.type == rgt.type );

	if( res.rows != rows || res.cols != cols || res.type != lft.type ) {
		res.alloc( rows, cols, lft.type );
	}

	int s = rows * cols;
	switch( lft.type ) {
		case zmatU32: {
			unsigned int *lp = lft.getPtrU(0,0);
			unsigned int *rp = rgt.getPtrU(0,0);
			unsigned int *dp = res.getPtrU(0,0);
			for( int r=0; r<s; r++ ) {
				*dp++ = *lp++ * *rp++;
			}
			break;
		}
		case zmatS32: {
			int *lp = lft.getPtrI(0,0);
			int *rp = rgt.getPtrI(0,0);
			int *dp = res.getPtrI(0,0);
			for( int r=0; r<s; r++ ) {
				*dp++ = *lp++ * *rp++;
			}
			break;
		}
		case zmatF32: {
			float *lp = lft.getPtrF(0,0);
			float *rp = rgt.getPtrF(0,0);
			float *dp = res.getPtrF(0,0);
			for( int r=0; r<s; r++ ) {
				*dp++ = *lp++ * *rp++;
			}
			break;
		}
		case zmatF64: {
			double *lp = lft.getPtrD(0,0);
			double *rp = rgt.getPtrD(0,0);
			double *dp = res.getPtrD(0,0);
			for( int r=0; r<s; r++ ) {
				*dp++ = *lp++ * *rp++;
			}
			break;
		}
		case zmatC32: {
			FComplex *lp = lft.getPtrFC(0,0);
			FComplex *rp = rgt.getPtrFC(0,0);
			FComplex *dp = res.getPtrFC(0,0);
			for( int r=0; r<s; r++ ) {
				dp->r = lp->r * rp->r - lp->i * rp->i;
				dp->i = lp->r * rp->i + lp->i * rp->r;
				dp++;
				lp++;
				rp++;
			}
			break;
		}
		case zmatC64: {
			DComplex *lp = lft.getPtrDC(0,0);
			DComplex *rp = rgt.getPtrDC(0,0);
			DComplex *dp = res.getPtrDC(0,0);
			for( int r=0; r<s; r++ ) {
				dp->r = lp->r * rp->r - lp->i * rp->i;
				dp->i = lp->r * rp->i + lp->i * rp->r;
				dp++;
				lp++;
				rp++;
			}
			break;
		}
	}
}

void zmatElemDiv( ZMat &lft, ZMat &rgt, ZMat &res ) {
	int rows = lft.rows;
	int cols = lft.cols;

	assert( lft.rows == rgt.rows );
	assert( lft.cols == rgt.cols );
	assert( lft.type == rgt.type );

	if( res.rows != rows || res.cols != cols || res.type != lft.type ) {
		res.alloc( rows, cols, lft.type );
	}

	int s = rows * cols;
	switch( lft.type ) {
		case zmatU32: {
			unsigned int *lp = lft.getPtrU(0,0);
			unsigned int *rp = rgt.getPtrU(0,0);
			unsigned int *dp = res.getPtrU(0,0);
			for( int r=0; r<s; r++ ) {
				*dp++ = *lp++ / *rp++;
			}
			break;
		}
		case zmatS32: {
			int *lp = lft.getPtrI(0,0);
			int *rp = rgt.getPtrI(0,0);
			int *dp = res.getPtrI(0,0);
			for( int r=0; r<s; r++ ) {
				*dp++ = *lp++ / *rp++;
			}
			break;
		}
		case zmatF32: {
			float *lp = lft.getPtrF(0,0);
			float *rp = rgt.getPtrF(0,0);
			float *dp = res.getPtrF(0,0);
			for( int r=0; r<s; r++ ) {
				*dp++ = *lp++ / *rp++;
			}
			break;
		}
		case zmatF64: {
			double *lp = lft.getPtrD(0,0);
			double *rp = rgt.getPtrD(0,0);
			double *dp = res.getPtrD(0,0);
			for( int r=0; r<s; r++ ) {
				*dp++ = *lp++ / *rp++;
			}
			break;
		}
		case zmatC32: {
			FComplex *lp = lft.getPtrFC(0,0);
			FComplex *rp = rgt.getPtrFC(0,0);
			FComplex *dp = res.getPtrFC(0,0);
			for( int r=0; r<s; r++ ) {
				float denom = rp->r*rp->r + rp->i*rp->i;
				dp->r = ( lp->r * rp->r + lp->i * rp->i) / denom;
				dp->i = (-lp->r * rp->i) + (lp->i * rp->r) / denom;
				dp++;
				lp++;
				rp++;
			}
			break;
		}
		case zmatC64: {
			DComplex *lp = lft.getPtrDC(0,0);
			DComplex *rp = rgt.getPtrDC(0,0);
			DComplex *dp = res.getPtrDC(0,0);
			for( int r=0; r<s; r++ ) {
				double denom = rp->r*rp->r + rp->i*rp->i;
				dp->r = ( lp->r * rp->r + lp->i * rp->i) / denom;
				dp->i = (-lp->r * rp->i) + (lp->i * rp->r) / denom;
				dp++;
				lp++;
				rp++;
			}
			break;
		}
	}
}

void zmatAdd( ZMat &lft, ZMat &rgt, ZMat &res ) {
	int rows = lft.rows;
	int cols = lft.cols;

	assert( lft.rows == rgt.rows );
	assert( lft.cols == rgt.cols );
	assert( lft.type == rgt.type );

	if( res.rows != lft.rows || res.cols != lft.cols || res.type != lft.type ) {
		res.alloc( rows, cols, lft.type );
	}

	int s = rows * cols;
	switch( lft.type ) {
		case zmatU32: {
			unsigned int *lp = lft.getPtrU(0,0);
			unsigned int *rp = rgt.getPtrU(0,0);
			unsigned int *dp = res.getPtrU(0,0);
			for( int r=0; r<s; r++ ) {
				*dp++ = *lp++ + *rp++;
			}
			break;
		}
		case zmatS32: {
			int *lp = lft.getPtrI(0,0);
			int *rp = rgt.getPtrI(0,0);
			int *dp = res.getPtrI(0,0);
			for( int r=0; r<s; r++ ) {
				*dp++ = *lp++ + *rp++;
			}
			break;
		}
		case zmatF32: {
			float *lp = lft.getPtrF(0,0);
			float *rp = rgt.getPtrF(0,0);
			float *dp = res.getPtrF(0,0);
			for( int r=0; r<s; r++ ) {
				*dp++ = *lp++ + *rp++;
			}
			break;
		}
		case zmatF64: {
			double *lp = lft.getPtrD(0,0);
			double *rp = rgt.getPtrD(0,0);
			double *dp = res.getPtrD(0,0);
			for( int r=0; r<s; r++ ) {
				*dp++ = *lp++ + *rp++;
			}
			break;
		}
		case zmatC32: {
			FComplex *lp = lft.getPtrFC(0,0);
			FComplex *rp = rgt.getPtrFC(0,0);
			FComplex *dp = res.getPtrFC(0,0);
			for( int r=0; r<s; r++ ) {
				dp->r = lp->r + rp->r;
				dp->i = lp->i + rp->i;
				dp++;
				lp++;
				rp++;
			}
			break;
		}
		case zmatC64: {
			DComplex *lp = lft.getPtrDC(0,0);
			DComplex *rp = rgt.getPtrDC(0,0);
			DComplex *dp = res.getPtrDC(0,0);
			for( int r=0; r<s; r++ ) {
				dp->r = lp->r + rp->r;
				dp->i = lp->i + rp->i;
				dp++;
				lp++;
				rp++;
			}
			break;
		}
	}
}

void zmatSub( ZMat &lft, ZMat &rgt, ZMat &res ) {
	int rows = lft.rows;
	int cols = lft.cols;

	assert( lft.rows == rgt.rows );
	assert( lft.cols == rgt.cols );
	assert( lft.type == rgt.type );

	if( res.rows != lft.rows || res.cols != lft.cols || res.type != lft.type ) {
		res.alloc( rows, cols, lft.type );
	}

	int s = rows * cols;
	switch( lft.type ) {
		case zmatU32: {
			unsigned int *lp = lft.getPtrU(0,0);
			unsigned int *rp = rgt.getPtrU(0,0);
			unsigned int *dp = res.getPtrU(0,0);
			for( int r=0; r<s; r++ ) {
				*dp++ = *lp++ - *rp++;
			}
			break;
		}
		case zmatS32: {
			int *lp = lft.getPtrI(0,0);
			int *rp = rgt.getPtrI(0,0);
			int *dp = res.getPtrI(0,0);
			for( int r=0; r<s; r++ ) {
				*dp++ = *lp++ - *rp++;
			}
			break;
		}
		case zmatF32: {
			float *lp = lft.getPtrF(0,0);
			float *rp = rgt.getPtrF(0,0);
			float *dp = res.getPtrF(0,0);
			for( int r=0; r<s; r++ ) {
				*dp++ = *lp++ - *rp++;
			}
			break;
		}
		case zmatF64: {
			double *lp = lft.getPtrD(0,0);
			double *rp = rgt.getPtrD(0,0);
			double *dp = res.getPtrD(0,0);
			for( int r=0; r<s; r++ ) {
				*dp++ = *lp++ - *rp++;
			}
			break;
		}
		case zmatC32: {
			FComplex *lp = lft.getPtrFC(0,0);
			FComplex *rp = rgt.getPtrFC(0,0);
			FComplex *dp = res.getPtrFC(0,0);
			for( int r=0; r<s; r++ ) {
				dp->r = lp->r - rp->r;
				dp->i = lp->i - rp->i;
				dp++;
				lp++;
				rp++;
			}
			break;
		}
		case zmatC64: {
			DComplex *lp = lft.getPtrDC(0,0);
			DComplex *rp = rgt.getPtrDC(0,0);
			DComplex *dp = res.getPtrDC(0,0);
			for( int r=0; r<s; r++ ) {
				dp->r = lp->r - rp->r;
				dp->i = lp->i - rp->i;
				dp++;
				lp++;
				rp++;
			}
			break;
		}
	}
}

void zmatGetCol( ZMat &mat, int col, ZMat &res ) {
	if( res.cols != 1 || res.rows != mat.rows || res.type != mat.type ) {
		res.alloc( mat.rows, 1, mat.type );
	}
	int elemSize = mat.elemSize;
	for( int i=0; i<mat.rows; i++ ) {
		memcpy( res.getPtrU(i,0), mat.getPtrU(i,col), elemSize );
	}
}

void zmatGetRow( ZMat &mat, int row, ZMat &res ) {
	if( res.rows != 1 || res.cols != mat.cols || res.type != mat.type ) {
		res.alloc( 1, mat.cols, mat.type );
	}
	int elemSize = mat.elemSize;
	for( int i=0; i<mat.cols; i++ ) {
		memcpy( res.getPtrU(0,i), mat.getPtrU(row,i), elemSize );
	}
}

void zmatNonNegativeFactorStart( ZMat &inputMat, ZMat &archtypeMat, ZMat &coefMat, ZMat &errorMat, int rank ) {
	assert( inputMat.type == zmatF32 );

	int r, c;

	// CONSTANTS
	const float eps = 1e-9f;
	int n = inputMat.rows;
	int m = inputMat.cols;
	const int iter = 500;
	
	// ALLOCATE the working mats
	archtypeMat.alloc( n, rank, zmatF32 );
	coefMat.alloc( rank, m, zmatF32 );
	errorMat.alloc( n, m, zmatF32 );

	// ADD EPSILON to the rawCountMat
	for( r=0; r<n; r++ ) {
		for( c=0; c<m; c++ ) {
			*inputMat.getPtrF( r, c ) += eps;
		}
	}

	// FILL the working mats randomly
	for( c=0; c<rank; c++ ) {
		for( r=0; r<n; r++ ) {
			archtypeMat.setF( r, c, (float)(rand()%32000) / 32000.f );
		}
		for( r=0; r<m; r++ ) {
			coefMat.setF( c, r, (float)(rand()%32000) / 32000.f );
		}
	}

	// NORMALIZE archtypeMat
	archtypeMat.normalizeColumnsSumOne();
}

void zmatNonNegativeFactorIterate( ZMat &inputMat, ZMat &archtypeMat, ZMat &coefMat, ZMat &errorMat ) {
assert( 0 );
// This may be wrong now that I inversed emem order??

	assert( inputMat.type == zmatF32 && archtypeMat.type == zmatF32 && coefMat.type == zmatF32 && errorMat.type == zmatF32 );

	const float eps = 1e-9f;
	int rank = coefMat.rows;
	int r, c;
	int n = inputMat.rows;
	int m = inputMat.cols;

	// MAKE archtype * coef matrix
	for( r=0; r<n; r++ ) {
		// Computing dot products and storing the result into errorMat just temporarily.
		for( c=0; c<m; c++ ) {
			// COMPUTE dot product of r row of archType with c col of coefMat
			float sum = 0;
			float *aj = archtypeMat.getPtrF(r,0);
			float *cj = coefMat    .getPtrF(0,c);
			for( int j=0; j<rank; j++ ) {
				sum += *aj * *cj;
				aj += archtypeMat.rows;
				cj ++;
			}
			sum += eps;

			errorMat.setF( r, c, sum );
		}

		// Error mat = element divide of the raw count matrix over the result (which we stored in the error mat)
		float *er = errorMat.getPtrF(r,0);
		float *cr = inputMat.getPtrF(r,0);
		for( c=0; c<m; c++ ) {
			*er = *cr / *er;
			er += errorMat.rows;
			cr += coefMat.rows;
		}
	}

	// UPDATE coefMat
	for( r=0; r<rank; r++ ) {
		for( c=0; c<m; c++ ) {

			// DOT PRODUCT
			float sum = 0.f;
			float *aj = archtypeMat.getPtrF(0,r);
			float *ej = errorMat   .getPtrF(0,c);
			for( int j=0; j<n; j++ ) {
				sum += *aj * *ej;
				aj ++;
				ej ++;
			}

			*(coefMat.getPtrF(r,c)) *= sum;
		}
	}

	// UPDATE archtypeMat
	for( r=0; r<n; r++ ) {
		for( c=0; c<rank; c++ ) {

			// DOT PRODUCT
			float sum = 0.f;
			float *ej = errorMat.getPtrF(r,0);
			float *cj = coefMat .getPtrF(c,0);
			for( int j=0; j<m; j++ ) {
				sum += *ej * *cj;
				ej += errorMat.rows;
				cj += coefMat.rows;
			}

			*(archtypeMat.getPtrF(r,c)) *= sum;
		}
	}
		
	// NORMALIZE archtypeMat
	archtypeMat.normalizeColumnsSumOne();

}

void zmatVectorToDiagonalMatrix( ZMat &vec, ZMat &mat ) {
	assert( vec.type == zmatF64 );
		// @TODO: Make a type-agnostic copy

	mat.alloc( vec.rows, vec.rows, vec.type );
	for( int r=0; r<vec.rows; r++ ) {
		mat.putD( r, r, vec.fetchD(r,0) );
	}
}

void zmatDiagonalMatrixToVector( ZMat &mat, ZMat &vec ) {
	assert( mat.type == zmatF64 );
		// @TODO: Make a type-agnostic copy

	int minDim = mat.rows < mat.cols ? mat.rows : mat.cols;
	vec.alloc( minDim, 1, mat.type );
	for( int i=0; i<minDim; i++ ) {
		vec.putD( i, 0, mat.getD(i,i) );
	}
}

// doubleCompare duped from zmathtools.cpp to avoid dependency; remove at will. (tfb)
static int doubleCompare( const void *a, const void *b ) {
	double c = *(double *)a - *(double *)b;
	if( c < 0.0 ) return -1;
	if( c > 0.0 ) return 1;
	return 0;
}

void zmatSortColsByRow0Value( ZMat &mat ) {
	// This depends on ZMat being column-major
	assert( mat.type == zmatF64 );
		// If you want other types, just need to change compareFn below
	qsort( mat.mat, mat.cols, mat.elemSize * mat.rows, doubleCompare );
}
