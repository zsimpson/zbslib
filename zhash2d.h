// @ZBS {
//		*MODULE_OWNER_NAME zhash2d
// }

#ifndef ZHASH2D_H
#define ZHASH2D_H

// Objects to be inserted into this list must inherit from this structure
struct ZHash2DElement {
	ZHash2DElement *next;
	ZHash2DElement *prev;
	ZHash2DElement **bin;
	ZHash2DElement() { next = prev = (ZHash2DElement *)0; bin = (ZHash2DElement **)0; }
	void clear() { next = 0; prev = 0; bin = 0; }
};

struct ZHash2D {
	int xBins, yBins;
	float xMin, xMax, yMin, yMax;
	ZHash2DElement **hash;

	ZHash2D();
	ZHash2D( int _xBuckets, int _yBuckets, float _xMin, float _xMax, float _yMin, float _yMax );
	~ZHash2D();

	ZHash2DElement **getBin( float x, float y );
	ZHash2DElement **getBinByRowCol( int row, int col );
	void getBinRowCol( float x, float y, int &row, int &col );
	void getBinCentroid( int row, int col, float &x, float &y );

	void insert( ZHash2DElement *object, float x, float y );
	void insertIntoBin( ZHash2DElement *object, int binX, int binY );
	void remove( ZHash2DElement *object );
	void move( ZHash2DElement *object, float x, float y );

	void reset();
		// Just memsets the array

	void resize( int _xBuckets, int _yBuckets, float _xMin, float _xMax, float _yMin, float _yMax );
		// Assumes that there is nothing in the hash

	void init( int _xBuckets, int _yBuckets, float _xMin, float _xMax, float _yMin, float _yMax );
	void clear();

	int checkCorrupt();
		// Traverses all elements and looks for circular links, 
		// return 1 if corrupt
};

struct ZHash2DEnum {
	ZHash2D *hash;
	int currentRow, currentCol;
	ZHash2DElement *nextCache;
	ZHash2DElement *current;
	int minRow, maxRow;
	int minCol, maxCol;

	ZHash2DEnum( ZHash2D *_hash, float _x, float _y, float _radiusW, float _radiusH );
	ZHash2DEnum( ZHash2D *_hash );
	int next();
	ZHash2DElement *get() { return current; }
};

// Example usage:
//	ZHash2D hash( ... );
//	for( ZHash2DEnum b( &hash, x, y, 10, 10 ); b.next(); ) {
//		ZHash2DElement *be = b.get();
//	}


#endif

