// @ZBS {
//		*MODULE_NAME zhash2d
//		+DESCRIPTION {
//			Implements a 2D spatial hash
//		}
//		*PORTABILITY win32 unix macosx
//		*REQUIRED_FILES zhash2d.cpp zhash2d.h
//		*VERSION 2.0
//		+HISTORY {
//			2.0 Renamed to ZHash2D from ZBucketHash, converted to float
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "stdlib.h"
#include "memory.h"
#include "assert.h"
#include "stdio.h"
// MODULE includes:
#include "zhash2d.h"
// ZBSLIB includes:

#ifndef min
	#define min(a,b) ( ( (a) < (b) ) ? (a) : (b) )
#endif
#ifndef max
	#define max(a,b) ( ( (a) > (b) ) ? (a) : (b) )
#endif

ZHash2D::ZHash2D() {
	hash = 0;
	clear();
}

ZHash2D::ZHash2D(
	int _xBuckets, int _yBuckets, float _xMin, float _xMax, float _yMin, float _yMax
) {
	init (_xBuckets, _yBuckets, _xMin, _xMax, _yMin, _yMax);
}

ZHash2D::~ZHash2D() {
	if( hash ) free( hash );
}

void ZHash2D::init( int _xBuckets, int _yBuckets, float _xMin, float _xMax, float _yMin, float _yMax ) {
	xBins = _xBuckets;
	yBins = _yBuckets;
	xMin = _xMin;
	xMax = _xMax;
	yMin = _yMin;
	yMax = _yMax;

	hash = (ZHash2DElement **)malloc( sizeof(ZHash2DElement **) * xBins * yBins );
	memset( hash, 0, sizeof(ZHash2DElement **) * xBins * yBins );
}

void ZHash2D::clear() {
	xBins=0;
	yBins=0;
	xMin=0;
	xMax=0;
	yMin=0;
	yMax=0;
	if( hash ) {
		free( hash );
		hash = 0;
	}
}

void ZHash2D::resize( int _xBuckets, int _yBuckets, float _xMin, float _xMax, float _yMin, float _yMax ) {
	// ASSERT that the hash is empty
	for( int i=0; i < xBins * yBins; i++ ) {
		assert( hash[i] == 0 );
	}

	xBins = _xBuckets;
	yBins = _yBuckets;
	xMin = _xMin;
	xMax = _xMax;
	yMin = _yMin;
	yMax = _yMax;

	free( hash );
	hash = (ZHash2DElement **)malloc( sizeof(ZHash2DElement **) * xBins * yBins );
	memset( hash, 0, sizeof(ZHash2DElement **) * xBins * yBins );
}

void ZHash2D::reset() {
	memset( hash, 0, sizeof(ZHash2DElement **) * xBins * yBins );
}

ZHash2DElement **ZHash2D::getBin( float x, float y ) {
	int xb = (int)( (x-xMin) / ((xMax - xMin) / (float)xBins) );
	int yb = (int)( (y-yMin) / ((yMax - yMin) / (float)yBins) );
	xb = max( 0, min( xBins-1, xb ) );
	yb = max( 0, min( yBins-1, yb ) );
//	assert( hash );
	return &hash[ yb * xBins + xb ];
}

ZHash2DElement **ZHash2D::getBinByRowCol( int row, int col ) {
//	assert( col >= 0 && col < xBins );
//	assert( row >= 0 && row < yBins );
//	assert( hash );
	return &hash[ row * xBins + col ];
}

void ZHash2D::getBinRowCol( float x, float y, int &row, int &col ) {
	col = (int)( (x-xMin) / ((xMax - xMin) / (float)xBins) );
	row = (int)( (y-yMin) / ((yMax - yMin) / (float)yBins) );
	col = max( 0, min( xBins-1, col ) );
	row = max( 0, min( yBins-1, row ) );
}

void ZHash2D::getBinCentroid( int row, int col, float &x, float &y ) {
	x = xMin + (xMax - xMin) / (float)xBins * ( (float)col + 0.5f );
	y = yMin + (yMax - yMin) / (float)yBins * ( (float)row + 0.5f );
}

void ZHash2D::insert( ZHash2DElement *object, float x, float y ) {
	assert( object->bin == 0 );
		// Can't be in a bin list already

	ZHash2DElement **bin = getBin( x, y );

	// INSERT it at the head of the list
	object->next = *bin;
	object->prev = 0;
	if( *bin ) {
		(*bin)->prev = object;
	}
	*bin = object;
	object->bin = bin;
}

void ZHash2D::insertIntoBin( ZHash2DElement *object, int binX, int binY ) {
	assert( object->bin == 0 );
		// Can't be in a bin list already

	ZHash2DElement **bin = getBinByRowCol( binY, binX );

	// INSERT it at the head of the list
	object->next = *bin;
	object->prev = 0;
	if( *bin ) {
		(*bin)->prev = object;
	}
	*bin = object;
	object->bin = bin;
}

void ZHash2D::remove( ZHash2DElement *object ) {
	assert( object->bin );
		// It must be in a bin list already

	// UNLINK bi-directional link
	if( object->prev ) {
		object->prev->next = object->next;
	}
	if( object->next ) {
		object->next->prev = object->prev;
	}

	// UNLINK head pointer if at head
	if( *object->bin == object ) {
		// I must be at the head of the list which should 
		// also mean that mean that my prev is 0
		assert( object->prev == 0 );
		*(object->bin) = object->next;
	}
	object->next = 0;
	object->prev = 0;
	object->bin = 0;
}

void ZHash2D::move( ZHash2DElement *object, float x, float y ) {
	remove( object );
	insert( object, x, y );
}

int ZHash2D::checkCorrupt() {
	for( int y=0; y<yBins; y++ ) {
		for( int x=0; x<xBins; x++ ) {
			ZHash2DElement **b = getBinByRowCol( x, y );
			int count = 0;
			for( ZHash2DElement *current=*b; current; current=current->next ) {
				if( count++ > 10000 ) {
					return 1;
				}
			}
		}
	}
	return 0;
}

ZHash2DEnum::ZHash2DEnum( ZHash2D *_hash ) {
	hash = _hash;
	minCol = 0;
	minRow = 0;
	maxCol = _hash->xBins-1;
	maxRow = _hash->yBins-1;
	currentRow = minRow;
	currentCol = minCol-1;
		// Minus one so that when next is called it will increment to the valid column
	current = 0;
	nextCache = 0;
}

ZHash2DEnum::ZHash2DEnum( ZHash2D *_hash, float _x, float _y, float _radiusW, float _radiusH ) {
	hash = _hash;
	hash->getBinRowCol( _x - _radiusW, _y - _radiusH, minRow, minCol );
	hash->getBinRowCol( _x + _radiusW, _y + _radiusH, maxRow, maxCol );
	assert( minCol <= maxCol );
	assert( minRow <= maxRow );
	currentRow = minRow;
	currentCol = minCol-1;
		// Minus one so that when next is called it will increment to the valid column
	current = 0;
	nextCache = 0;
}

int ZHash2DEnum::next() {
	if( !current || !current->next ) {
		do {
			// Time to switch to the next bin
			currentCol++;
			if( currentCol > maxCol ) {
				// Finished with this row, advance to the next
				currentRow++;
				if( currentRow > maxRow ) {
					// Completely finished
					return 0;
				}
				currentCol = minCol;
			}
			current = *hash->getBinByRowCol( currentRow, currentCol );
				// This might be 0 because there's nothing in this new bin
		} while( !current );
	}
	else {
		// This bin has more in it
		current = nextCache;
	}
	nextCache = current->next;
	return 1;
}


