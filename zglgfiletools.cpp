// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A collection of tools for loading graph files directly into textures
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zgltools.cpp zgltools.h zglgfiletools.h zglgfiletools.cpp
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "assert.h"
#include "string.h"
// MODULE includes:
#include "zgltools.h"
#include "zglgfiletools.h"
// ZBSLIB includes:
#include "zhashtable.h"
#include "zgraphfile.h"
#include "zbitmapdesc.h"
#include "zbits.h"

static inline int __nextPow2( int x ) {
	// This is copied here from mathtools.h to eliminate dependency just for this one little function
	int p = 2;
	for( int i=1; i<31; i++ ) {
		if( p >= x ) {
			return p;
		}
		p *= 2;
	}
	return 0;
}

void zglLoadTexture( char *filename, int texture, struct ZBitmapDesc *desc, int vertFlip ) {
	// LOAD the file
	ZBitmapDesc dstDesc;
	int ret = zGraphFileLoad( filename, &dstDesc );
	if( ret ) {
		if( vertFlip ) {
			zBitmapDescInvertLines( dstDesc );
		}

		zglLoadTextureFromBitmapDesc( &dstDesc, texture );

		if( desc ) {
			// If the caller wants to retain a copy, then hand it over
			memcpy( desc, &dstDesc, sizeof(*desc) );
			desc->owner = 1;
			dstDesc.owner = 0;
				// Prevent the dstDesc from erasing the memory.
		}
	}
}

int zglGenTexture( char *filename, struct ZBitmapDesc *desc, int vertFlip ) {
	int texture = zglGenTexture();
	zglLoadTexture( filename, texture, desc, vertFlip );
	return texture;
}

void zglLoadTextureRGBA( char *filename, int texture, struct ZBitmapDesc *desc, int vertFlip ) {
	// LOAD the file
	ZBitmapDesc srcDesc;
	int ret = zGraphFileLoad( filename, &srcDesc );
	assert( ret );

	if( vertFlip ) {
		zBitmapDescInvertLines( srcDesc );
	}

	// CONVERT to RGBA
	ZBitmapDesc dstDesc( srcDesc );
	dstDesc.d = 4;
	dstDesc.memW = __nextPow2( dstDesc.w );
	dstDesc.memH = __nextPow2( dstDesc.h );
		// Some function somewhere expects this to drive both the w
		// and h to the same power of 2.  However, planets doesn't want this
		// as the textures are significantly wider than tall.

	zBitmapDescConvert( srcDesc, dstDesc );

	zglLoadTextureFromBitmapDesc( &dstDesc, texture );

	if( desc ) {
		// If the caller wants to retain a copy, then hand it over
		memcpy( desc, &dstDesc, sizeof(*desc) );
		desc->owner = 1;
		dstDesc.owner = 0;
			// Prevent the dstDesc from erasing the memory.
	}
}

int zglGenTextureRGBA( char *filename, struct ZBitmapDesc *desc, int vertFlip ) {
	int texture = zglGenTexture();
	zglLoadTextureRGBA( filename, texture, desc, vertFlip );
	return texture;
}

void zglLoadTextureAndAlphaRGBA_( char *filename, char *alphaFilename, int texture, struct ZBitmapDesc *desc, int invert, int vertFlip ) {
	// LOAD the file
	ZBitmapDesc srcDesc;
	int ret = zGraphFileLoad( filename, &srcDesc );
	assert( ret );

	// CONVERT to RGBA
	ZBitmapDesc dstDesc( srcDesc );
	dstDesc.d = 4;
	dstDesc.memW = __nextPow2( dstDesc.w );
	dstDesc.memH = __nextPow2( dstDesc.h );
		// Some function somewhere expects this to drive both the w
		// and h to the same power of 2.  However, planets doesn't want this
		// as the textures are significantly wider than tall.
	zBitmapDescConvert( srcDesc, dstDesc );

	// LOAD the alpha channel
	ZBitmapDesc aDesc;
	ret = zGraphFileLoad( alphaFilename, &aDesc );
	assert( ret );
	assert( aDesc.d == 1 || aDesc.d == 3 || aDesc.d == 4 );
	assert( aDesc.w == srcDesc.w && aDesc.h == srcDesc.h );

	for( int y=0; y<aDesc.h; y++ ) {
		unsigned char *s = aDesc.lineUchar( y );
		unsigned char *d = dstDesc.lineUchar( y );
		for( int x=0; x<aDesc.w; x++ ) {
			unsigned char alpha = 0;
			if( aDesc.d == 1 ) {
				alpha = *s++;
			}
			else if( aDesc.d == 3 ) {
				// uses the first channel if there are three
				alpha = *s;
				s += 3;
			}
			else if( aDesc.d == 4 ) {
				// uses the last channel if there are four
				alpha = *(s+3);
				s += 4;
			}
			else {
				assert( 0 );
			}
			d += 3;
			if( invert ) {
				*d++ = 255 - alpha;
			}
			else {
				*d++ = alpha;
			}
		}
	}

	if( vertFlip ) {
		zBitmapDescInvertLines( dstDesc );
	}

	zglLoadTextureFromBitmapDesc( &dstDesc, texture );
	if( desc ) {
		// If the caller wants to retain a copy, then hand it over
		memcpy( desc, &dstDesc, sizeof(*desc) );
		desc->owner = 1;
		dstDesc.owner = 0;
			// Prevent the dstDesc from erasing the memory.
	}
}

void zglLoadTextureAndAlphaRGBANoinvert( char *filename, char *alphaFilename, int texture, struct ZBitmapDesc *desc, int vertFlip ) {
	zglLoadTextureAndAlphaRGBA_( filename, alphaFilename, texture, desc, 0, vertFlip );
}

void zglLoadTextureAndAlphaRGBAInvert( char *filename, char *alphaFilename, int texture, struct ZBitmapDesc *desc, int vertFlip ) {
	zglLoadTextureAndAlphaRGBA_( filename, alphaFilename, texture, desc, 1, vertFlip );
}

int zglGenTextureAndAlphaRGBA_( char *filename, char *alphaFilename, struct ZBitmapDesc *desc, int invert, int vertFlip ) {
	int texture = zglGenTexture();
	zglLoadTextureAndAlphaRGBA_( filename, alphaFilename, texture, desc, invert, vertFlip );
	return texture;
}

int zglGenTextureAndAlphaRGBANoinvert( char *filename, char *alphaFilename, struct ZBitmapDesc *desc, int vertFlip ) {
	return zglGenTextureAndAlphaRGBA_( filename, alphaFilename, desc, 0, vertFlip );
}

int zglGenTextureAndAlphaRGBAInvert( char *filename, char *alphaFilename, struct ZBitmapDesc *desc, int vertFlip ) {
	return zglGenTextureAndAlphaRGBA_( filename, alphaFilename, desc, 1, vertFlip );
}


// Using ZBits.  The above is deprecated
//-------------------------------------------------------------------------------------------------------------------------------------

void zglLoadTextureZBits( char *filename, int texture, struct ZBits *desc, int vertFlip ) {
	// LOAD the file
	ZBits dstDesc;
	zGraphFileLoad( filename, &dstDesc );

	if( vertFlip ) {
		zbitsInvert( &dstDesc );
	}

	zglLoadTextureFromZBits( &dstDesc, texture );

	if( desc ) {
		// If the caller wants to retain a copy, then hand it over
		memcpy( desc, &dstDesc, sizeof(*desc) );
		desc->owner = 1;
		dstDesc.owner = 0;
			// Prevent the dstDesc from erasing the memory.
	}
}

int zglGenTextureZBits( char *filename, struct ZBits *desc, int vertFlip ) {
	int texture = zglGenTexture();
	zglLoadTextureZBits( filename, texture, desc, vertFlip );
	return texture;
}

void zglLoadTextureRGBAZBits( char *filename, int texture, struct ZBits *desc, int vertFlip ) {
	// LOAD the file
	ZBits srcDesc;
	int ret = zGraphFileLoad( filename, &srcDesc );
	assert( ret );

	if( vertFlip ) {
		zbitsInvert( &srcDesc );
	}

	// CONVERT to RGBA
	ZBits dstDesc( &srcDesc, 0, 1 );
	dstDesc.channels = 3;
	dstDesc.alphaChannel = 3;
	dstDesc.alloc();

	zbitsConvert( &srcDesc, &dstDesc );

	#ifdef __BIG_ENDIAN__
	zbitsByteReorder( &dstDesc );
	#endif
	
	zglLoadTextureFromZBits( &dstDesc, texture );

	if( desc ) {
		// If the caller wants to retain a copy, then hand it over
		memcpy( desc, &dstDesc, sizeof(*desc) );
		desc->owner = 1;
		dstDesc.owner = 0;
			// Prevent the dstDesc from erasing the memory.
	}
}

int zglGenTextureRGBAZBits( char *filename, struct ZBits *desc, int vertFlip ) {
	int texture = zglGenTexture();
	zglLoadTextureRGBAZBits( filename, texture, desc, vertFlip );
	return texture;
}

void zglLoadTextureAndAlphaRGBA_( char *filename, char *alphaFilename, int texture, struct ZBits *desc, int invert, int vertFlip ) {
	// LOAD the file
	ZBits srcDesc;
	int ret = zGraphFileLoad( filename, &srcDesc );
	assert( ret );

	// CONVERT to RGBA
	ZBits dstDesc( &srcDesc, 0, 1 );
	dstDesc.channels = 3;
	dstDesc.alphaChannel = 3;
	dstDesc.alloc();
	zbitsConvert( &srcDesc, &dstDesc );

	// LOAD the alpha channel
	ZBits aDesc;
	ret = zGraphFileLoad( alphaFilename, &aDesc );
	assert( ret );
	assert( aDesc.channels == 1 || aDesc.channels == 3 );
	assert( aDesc.alphaChannel == 0 );
	assert( aDesc.channelDepthInBits == 8 );
	assert( aDesc.w() == srcDesc.w() && aDesc.h() == srcDesc.h() );

	for( int y=0; y<aDesc.h(); y++ ) {
		unsigned char *s = aDesc.lineUchar( y );
		unsigned char *d = dstDesc.lineUchar( y );
		for( int x=0; x<aDesc.w(); x++ ) {
			unsigned char alpha = 0;
			if( aDesc.channels == 1 ) {
				alpha = *s++;
			}
			else if( aDesc.channels == 3 ) {
				// uses the first channel if there are three
				alpha = *s;
				s += 3;
			}
			else {
				assert( 0 );
			}
			d += 3;
			if( invert ) {
				*d++ = 255 - alpha;
			}
			else {
				*d++ = alpha;
			}
		}
	}

	if( vertFlip ) {
		zbitsInvert( &dstDesc );
	}

	zglLoadTextureFromZBits( &dstDesc, texture );
	if( desc ) {
		// If the caller wants to retain a copy, then hand it over
		memcpy( desc, &dstDesc, sizeof(*desc) );
		desc->owner = 1;
		dstDesc.owner = 0;
			// Prevent the dstDesc from erasing the memory.
	}
}



void zglLoadTextureAndAlphaRGBANoinvertZBits( char *filename, char *alphaFilename, int texture, struct ZBits *desc, int vertFlip ) {
	zglLoadTextureAndAlphaRGBA_( filename, alphaFilename, texture, desc, 0, vertFlip );
}

void zglLoadTextureAndAlphaRGBAInvertZBits( char *filename, char *alphaFilename, int texture, struct ZBits *desc, int vertFlip ) {
	zglLoadTextureAndAlphaRGBA_( filename, alphaFilename, texture, desc, 1, vertFlip );
}


int zglGenTextureAndAlphaRGBA_( char *filename, char *alphaFilename, struct ZBits *desc, int invert, int vertFlip ) {
	int texture = zglGenTexture();
	zglLoadTextureAndAlphaRGBA_( filename, alphaFilename, texture, desc, invert, vertFlip );
	return texture;
}

int zglGenTextureAndAlphaRGBANoinvertZBits( char *filename, char *alphaFilename, struct ZBits *desc, int vertFlip ) {
	return zglGenTextureAndAlphaRGBA_( filename, alphaFilename, desc, 0, vertFlip );
}

int zglGenTextureAndAlphaRGBAInvertZBits( char *filename, char *alphaFilename, struct ZBits *desc, int vertFlip ) {
	return zglGenTextureAndAlphaRGBA_( filename, alphaFilename, desc, 1, vertFlip );
}
