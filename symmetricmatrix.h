// @ZBS {
//		*MODULE_OWNER_NAME symmetricmatrix
// }

#ifndef SYMMETRICMATRIX_H
#define SYMMETRICMATRIX_H

struct SymmetricMatrix {
	unsigned int count;
	unsigned int elemSize;
	char *table;

  public:
	unsigned int getSize() {
		return count * (count+1) / 2;
	}

	unsigned int getSizeInBytes() {
		return getSize() * elemSize;
	}

	unsigned int getCount() {
		return count;
	}

	unsigned int getElemSize() {
		return elemSize;
	}

	void free();
	void alloc( int _count, int _elemSize );

	void *getPtr( int a, int b ) {
		if( a < b ) { int t = a; a = b; b = t; }
		return &table[ ((a*(a+1))/2 + b) * elemSize ];
	}

	int getI( int a, int b );
	float getF( int a, int b );
	double getD( int a, int b );
		// WARNING: only use these if you have correctly allocated the matrix
		// to the correct size.  They DO NOT assert that the size is correct!

	void set( int a, int b, void *t );
	void setF( int a, int b, float x );
	void setD( int a, int b, double x );
		// WARNING: only use these if you have correctly allocated the matrix
		// to the correct size.  They DO NOT assert that the size is correct!

	SymmetricMatrix() {
		count = 0;
		elemSize = 0;
		table = 0;
	}

	SymmetricMatrix( int _count, int _elemSize ) {
		count = 0;
		elemSize = 0;
		table = 0;
		alloc( _count, _elemSize );
	}

	~SymmetricMatrix() {
		free();
	}

	void saveBin( char *filename );
	int loadBin( char *filename );
	int loadTxt( char *filename, int hasRowHeader, int hasColHeader );
};

#endif

