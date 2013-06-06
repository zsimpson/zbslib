// @ZBS {
//		*MASTER_FILE 1
//		*MODULE_NAME zhash3d
//		+DESCRIPTION {
//			Implements a 3D spatial hash, simple extension to ZHash2D
//		}
//		*PORTABILITY win32 unix macosx
//		*REQUIRED_FILES zhash3d.cpp zhash3d.h
//		*VERSION 1.0
//		+HISTORY {
//			1.0 Initial Version tfb
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
#include "zhash3d.h"
// ZBSLIB includes:

#ifndef min
	#define min(a,b) ( ( (a) < (b) ) ? (a) : (b) )
#endif
#ifndef max
	#define max(a,b) ( ( (a) > (b) ) ? (a) : (b) )
#endif

ZHash3D::ZHash3D() {
	hash = 0;
	clear();
}

ZHash3D::ZHash3D(
	int _xBins, int _yBins, int _zBins, float _xMin, float _xMax, float _yMin, float _yMax, float _zMin, float _zMax
) {
	init (_xBins, _yBins, _zBins, _xMin, _xMax, _yMin, _yMax, _zMin, _zMax );
}

ZHash3D::~ZHash3D() {
	if( hash ) free( hash );
}

void ZHash3D::init( int _xBins, int _yBins, int _zBins, float _xMin, float _xMax, float _yMin, float _yMax, float _zMin, float _zMax ) {
	xBins = _xBins;
	yBins = _yBins;
	zBins = _zBins;
	xMin = _xMin;
	xMax = _xMax;
	yMin = _yMin;
	yMax = _yMax;
	zMin = _zMin;
	zMax = _zMax;

	hash = (ZHash3DElement **)malloc( sizeof(ZHash3DElement **) * xBins * yBins * zBins );
	memset( hash, 0, sizeof(ZHash3DElement **) * xBins * yBins * zBins );
}

void ZHash3D::clear() {
	xBins=0;
	yBins=0;
	zBins=0;
	xMin=0;
	xMax=0;
	yMin=0;
	yMax=0;
	zMin=0;
	zMax=0;
	if( hash ) {
		free( hash );
		hash = 0;
	}
}

void ZHash3D::resize( int _xBins, int _yBins, int _zBins, float _xMin, float _xMax, float _yMin, float _yMax, float _zMin, float _zMax ) {
	// ASSERT that the hash is empty
	for( int i=0; i < xBins * yBins * zBins; i++ ) {
		assert( hash[i] == 0 );
	}

	xBins = _xBins;
	yBins = _yBins;
	zBins = _zBins;
	xMin = _xMin;
	xMax = _xMax;
	yMin = _yMin;
	yMax = _yMax;
	zMin = _zMin;
	zMax = _zMax;

	free( hash );
	hash = (ZHash3DElement **)malloc( sizeof(ZHash3DElement **) * xBins * yBins * zBins );
	memset( hash, 0, sizeof(ZHash3DElement **) * xBins * yBins * zBins );
}

void ZHash3D::reset() {
	memset( hash, 0, sizeof(ZHash3DElement **) * xBins * yBins * zBins );
}

ZHash3DElement **ZHash3D::getBin( float x, float y, float z ) {
	int xb = (int)( (x-xMin) / ((xMax - xMin) / (float)xBins) );
	int yb = (int)( (y-yMin) / ((yMax - yMin) / (float)yBins) );
	int zb = (int)( (z-zMin) / ((zMax - zMin) / (float)zBins) );
	xb = max( 0, min( xBins-1, xb ) );
	yb = max( 0, min( yBins-1, yb ) );
	zb = max( 0, min( zBins-1, zb ) );
	return &hash[ zb * yBins * xBins + yb * xBins + xb ];
}

ZHash3DElement **ZHash3D::getBinByIndices( int xi, int yi, int zi ) {
	return &hash[ zi * yBins * xBins + yi * xBins + xi ];
}

void ZHash3D::getBinIndices( float x, float y, float z, int &xi, int &yi, int &zi ) {
	xi = (int)( (x-xMin) / ((xMax - xMin) / (float)xBins) );
	yi = (int)( (y-yMin) / ((yMax - yMin) / (float)yBins) );
	zi = (int)( (z-zMin) / ((zMax - zMin) / (float)zBins) );
	xi = max( 0, min( xBins-1, xi ) );
	yi = max( 0, min( yBins-1, yi ) );
	zi = max( 0, min( zBins-1, zi ) );
}

int ZHash3D::getLinearIndexXMajor( float x, float y, float z ) {
	int xi, yi, zi;
	getBinIndices( x, y, z, xi,  yi, zi );
	return zi * yBins * xBins + yi * xBins + xi;
}

int ZHash3D::getLinearIndexZMajor( float x, float y, float z ) {
	int xi, yi, zi;
	getBinIndices( x, y, z, xi,  yi, zi );
	return xi * zBins * yBins + yi * zBins + zi;
}

void ZHash3D::getBinCentroid( int xi, int yi, int zi, float &x, float &y, float &z ) {
	x = xMin + (xMax - xMin) / (float)xBins * ( (float)xi + 0.5f );
	y = yMin + (yMax - yMin) / (float)yBins * ( (float)yi + 0.5f );
	z = zMin + (zMax - zMin) / (float)zBins * ( (float)zi + 0.5f );
}

void ZHash3D::insert( ZHash3DElement *object, float x, float y, float z ) {
	assert( object->bin == 0 );
		// Can't be in a bin list already

	ZHash3DElement **bin = getBin( x, y, z );

	// INSERT it at the head of the list
	object->next = *bin;
	object->prev = 0;
	if( *bin ) {
		(*bin)->prev = object;
	}
	*bin = object;
	object->bin = bin;
}

void ZHash3D::insertIntoBin( ZHash3DElement *object, int binX, int binY, int binZ ) {
	assert( object->bin == 0 );
		// Can't be in a bin list already

	ZHash3DElement **bin = getBinByIndices( binY, binX, binZ );

	// INSERT it at the head of the list
	object->next = *bin;
	object->prev = 0;
	if( *bin ) {
		(*bin)->prev = object;
	}
	*bin = object;
	object->bin = bin;
}

void ZHash3D::remove( ZHash3DElement *object ) {
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

void ZHash3D::move( ZHash3DElement *object, float x, float y, float z ) {
	remove( object );
	insert( object, x, y, z );
}

int ZHash3D::checkCorrupt() {
	for( int z=0; z<zBins; z++ ) {
		for( int y=0; y<yBins; y++ ) {
			for( int x=0; x<xBins; x++ ) {
				ZHash3DElement **b = getBinByIndices( x, y, z );
				int count = 0;
				for( ZHash3DElement *current=*b; current; current=current->next ) {
					if( count++ > 10000 ) {
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

ZHash3DEnum::ZHash3DEnum( ZHash3D *_hash ) {
	hash = _hash;
	minXi = 0;
	minYi = 0;
	minZi = 0;
	maxXi = _hash->xBins-1;
	maxYi = _hash->yBins-1;
	maxZi = _hash->zBins-1;
	currentXi = minXi-1;
		// Minus one so that when next is called it will increment to the valid Xindex
	currentYi = minYi;
	currentZi = minZi;
	current = 0;
	nextCache = 0;
}

ZHash3DEnum::ZHash3DEnum( ZHash3D *_hash, float _x, float _y, float _z, float _radiusX, float _radiusY, float _radiusZ ) {
	hash = _hash;
	hash->getBinIndices( _x - _radiusX, _y - _radiusY, _z - _radiusZ,  minXi, minYi, minZi );
	hash->getBinIndices( _x + _radiusX, _y + _radiusY, _z + _radiusZ,  maxXi, maxYi, maxZi );
	assert( minXi <= maxXi );
	assert( minYi <= maxYi );
	assert( minZi <= maxZi );
	currentXi = minXi-1;
		// Minus one so that when next is called it will increment to the valid Xindex
	currentYi = minYi;
	currentZi = minZi;
	current = 0;
	nextCache = 0;
}

int ZHash3DEnum::next() {
	if( !current || !current->next ) {
		do {
			// Time to switch to the next bin
			currentXi++;
			if( currentXi > maxXi ) {
				currentYi++;
				if( currentYi > maxYi ) {
					// Finished with this row, advance to the next
					currentZi++;
					if( currentZi > maxZi ) {
						// Completely finished
						return 0;
					}
					currentYi = minYi;
				}
				currentXi = minXi;
				current = *hash->getBinByIndices( currentXi, currentYi, currentZi );
					// This might be 0 because there's nothing in this new bin
			}
		} while( !current );
	}
	else {
		// This bin has more in it
		current = nextCache;
	}
	nextCache = current->next;
	return 1;
}


