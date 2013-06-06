#include "zendian.h"
#include "assert.h"

int machineIsBigEndian() {
	short word = 0x4321;
	if((*(char *)& word) != 0x21 ) {
		return 1;
	}
	return 0;
}

#define BYTESWAP(x) byteSwap((unsigned char *) &x,sizeof(x))
	// macro for atomic types

void byteSwap( unsigned char * b, int size ) {
	int i = 0;
	int j = size-1;
	unsigned char c;
	while( i < j ) {
		c = b[i];
		b[i] = b[j];
		b[j] = c;
		i++, j--;
	}
}

size_t freadEndian( void *buffer, size_t elemSize, size_t numElem, FILE *f, int _byteSwap ) {
	size_t elemsRead = fread( buffer, elemSize, numElem, f );
	if( !_byteSwap || elemSize==1 ) {
		return elemsRead;
	}
	unsigned char *b = (unsigned char *)buffer;
	for( size_t i=0; i<numElem; i++, b += elemSize ) {
		byteSwap( b, elemSize );
	}
	return elemsRead;
}

//-------------------------------------------------------------------------------------------------
// We may run into two varieties of architecture difference that affect the loading/storing of
// binary data.  (1) endianness.  (2) sizeof basic data types (for 32bit vs 64bit builds).
//
// For (2) however, the problem is less severe than might be thought: we really only expect
// to run into either the LP64 model (linux, osx) or the LLP64 model (win64, if we end up
// building 64bit versions of zlab on win64, not done currently).
//
// In LP64, only longs and pointers become 64bit.  On LLP64, only the pointers.  So we can
// safely use "int" and know it is 4 bytes, "short" is 2, "char" is one.  I'm not sure about
// float and double, but they are likely the same on these platforms as well.
//
// So a primary concern is to NOT write structures that contain pointers to disk in binary
// format.  ZMat unfortantely does this currently.  There are also longs to deal with,
// which are not used frequently.
//
// As far as (1), endianness, we may be best served by some simple replacements for fwrite
// and fread that take byteswap flags (or some static class object that does this).  One 
// thing that comes to mind here is the binary pack and unpack of hashtables, which 
// probably pack/unpack pointers as well.

const int BinaryCompatData_Version = 20080627;

void saveBinaryCompatData( FILE *f ) {
	// Note that all sizes are written as ints, which we expect to be 32bit on all
	// encountered platforms (see comments above).
	int bigEndian = machineIsBigEndian();
	fwrite( &bigEndian, sizeof(bigEndian), 1, f );
	assert( sizeof(int)==4 );
	fwrite( &BinaryCompatData_Version, sizeof(BinaryCompatData_Version), 1, f );
	int longSize = sizeof(long);
	fwrite( &longSize, sizeof(longSize), 1, f );
	enum test { testSize=0 };
	int enumSize = sizeof( testSize );
	fwrite( &enumSize, sizeof(enumSize), 1, f );
}

bool loadBinaryCompatData( FILE *f, int &shouldByteSwap, int &dataLongSize ) {
	assert( sizeof( int ) == 4 );
	// we expect to run into only LP64 and LLP64 64bit models, so we
	// read/write all sizes as ints.

	shouldByteSwap = false;
	dataLongSize   = 4;

	// this hack is to allow ken to load files with data that were
	// written before the verion was added:
	char oldEndian;
	int  intSize;
	fread( &oldEndian, 1, 1, f );
	fread( &intSize, 4, 1, f );
	if( oldEndian != 0 || intSize != 4 ) {
		fseek( f, -5, SEEK_CUR );
			// setup to read version per normal
	}
	else {
		return false;
			// for now don't allow the load of old data.
			// (the problem is loading the packed hashtables)
	}

	// should we byteswap reads?
	int bigEndian;
	fread( &bigEndian, sizeof(bigEndian), 1, f );
	if( bigEndian != 0 ) bigEndian = 1;
	if( bigEndian != machineIsBigEndian() ) {
		shouldByteSwap = true;
	}

	int version;
	freadEndian( &version, sizeof(version), 1, f, shouldByteSwap );
	switch( version ) {
		case 20080627: {
			freadEndian( &dataLongSize, sizeof(dataLongSize), 1, f, shouldByteSwap );
				// we don't flag this as an error, since we take care not to save
				// longs in our current binary data.

			// Not sure if enums are always ints for all compilers by default?
			int enumSize;
			freadEndian( &enumSize, sizeof(enumSize), 1, f, shouldByteSwap );
			enum test { testSize=0 };
			if( enumSize != sizeof(testSize) ) {
				printf( "ERROR loading data: enumsize is not compatible with binary data!\n" );
				return false;
			}
			// @TODO:
			// do we need to worry about sizeof(float) or sizeof(double)?
		}
		break;

		default:
			return false;
	}
	return true;
}


