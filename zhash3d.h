// @ZBS {
//		*MODULE_OWNER_NAME zhash3d
// }

#ifndef ZHASH3D_H
#define ZHASH3D_H

// Objects to be inserted into this list must inherit from this structure
struct ZHash3DElement {
	ZHash3DElement *next;
	ZHash3DElement *prev;
	ZHash3DElement **bin;
	ZHash3DElement() { next = prev = (ZHash3DElement *)0; bin = (ZHash3DElement **)0; }
	void clear() { next = 0; prev = 0; bin = 0; }
};

struct ZHash3D {
	int xBins, yBins, zBins;
	float xMin, xMax, yMin, yMax, zMin, zMax;
	ZHash3DElement **hash;

	ZHash3D();
	ZHash3D( int _xBins, int _yBins, int _zBins, float _xMin, float _xMax, float _yMin, float _yMax, float _zMin, float _zMax );
	~ZHash3D();

	ZHash3DElement **getBin( float x, float y, float z );
	ZHash3DElement **getBinByIndices( int xi, int yi, int zi );
	void getBinIndices( float x, float y, float z, int &xi, int &yi, int &zi );
	int getLinearIndexXMajor( float x, float y, float z );
	int getLinearIndexZMajor( float x, float y, float z );
		// helpers for clients that are indexing data associated with bins;
		// note that this class stores the "hash" of elements in XMajor order.
	void getBinCentroid( int xi, int yi, int zi, float &x, float &y, float &z );

	void insert( ZHash3DElement *object, float x, float y, float z );
	void insertIntoBin( ZHash3DElement *object, int binX, int binY, int binZ );
	void remove( ZHash3DElement *object );
	void move( ZHash3DElement *object, float x, float y, float z );

	void reset();
		// Just memsets the array

	void resize( int _xBins, int _yBins, int _zBins, float _xMin, float _xMax, float _yMin, float _yMax, float _zMin, float _zMax );
		// Assumes that there is nothing in the hash

	void init( int _xBins, int _yBins, int _zBins, float _xMin, float _xMax, float _yMin, float _yMax, float _zMin, float _zMax );
	void clear();

	int checkCorrupt();
		// Traverses all elements and looks for circular links, 
		// return 1 if corrupt
};

struct ZHash3DEnum {
	ZHash3D *hash;
	int currentXi, currentYi, currentZi;
	ZHash3DElement *nextCache;
	ZHash3DElement *current;
	int minXi, maxXi;
	int minYi, maxYi;
	int minZi, maxZi;

	ZHash3DEnum( ZHash3D *_hash, float _x, float _y, float _z, float _radiusX, float _radiusY, float _radiusZ );
	ZHash3DEnum( ZHash3D *_hash );
	int next();
	ZHash3DElement *get() { return current; }
};

// Example usage:
//	ZHash3D hash( ... );
//	for( ZHash3DEnum b( &hash, x, y, z, 10, 10, 10 ); b.next(); ) {
//		ZHash3DElement *be = b.get();
//	}


#endif

