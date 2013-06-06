// @ZBS {
//		*MODULE_NAME ZBits
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A bitmap description class with the aim of being easily Intel PPL compatible
//		}
//		*PORTABILITY win32 unix macosx
//		*REQUIRED_FILES zbits.cpp zbits.h
//		*VERSION 2.0
//		+HISTORY {
//			This is at least the third attempt at getting this to the right balance between
//			usability and simplicity
//		}
//		*SELF_TEST no
//		*PUBLISH yes
// }

void ZBits::create( int w, int h, int fmt, int _alloc=1, int _pow2=0 ) {
	this->w = w;
	this->h = h;
	this->fmt = fmt;

	memW = w;
	memH = h;

	appData = 0;
	pixDepthInBytes = ((fmt & ZBITS_DEPTH_MASK) >> 3) * (((fmt & ZBITS_FMT_MASK) >> 12) + 1);

	if( pow2 ) {
		memW = nextPow2( memW );
		memH = nextPow2( memH );
	}

	if( alloc ) {
		alloc();
	}

}

void ZBits::create( ZBits *toCopy, int _alloc, int _pow2, int _copyBits ) {
	clear();

	w = toCopy->w;
	h = toCopy->h;
	memW = toCopy->memW;
	memH = toCopy->memH;
	appData = toCopy->appData;
	pixDepthInBytes = toCopy->pixDepthInBytes;

	if( pow2 ) {
		memW = nextPow2( memW );
		memH = nextPow2( memH );
	}

	if( alloc ) {
		alloc();
	}

	if( _copyBits ) {
		memcpy( bits, toCopy->bits, toCopy->memSizeInBytes() );
	}
}

void ZBits::alloc() {
	mallocBits = (char *)malloc( memSizeInBytes() + 16 );

	#ifdef WIN32
		bits = mallocBits + (0x10 - ((__int64)mallocBits & 0x0F));
	#else
		bits = mallocBits + (0x10 - ((long long)mallocBits & 0x0F));
	#endif

	owner = 1;
}

void dealloc() {
	if( mallocBits ) {
		free( mallocBits );
	}
	bits = 0;
	mallocBits = 0;
	owner = 0;
}

void ZBits::clear() {
	w = 0;
	h = 0;
	memW = 0;
	memH = 0;
	appData = 0;
	pixDepthInBytes = 0;
	owner = 0;
	mallocBits = 0;
	bits = 0;
}

void ZBits::fill( int value ) {
	if( bits ) {
		memset( bits, value, memSizeInBytes() );
	}
}

void ZBits::transferOwnershipFrom( ZBits *src ) {
	src->owner = 0;
	owner = 1;
}


