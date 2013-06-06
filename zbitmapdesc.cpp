// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Yet another bitmap description class
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zbitmapdesc.cpp zbitmapdesc.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//			A shorter name would be nice
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "memory.h"
#include "stdio.h"
#include "assert.h"
#include "math.h"
#include "stdlib.h"
#ifdef WIN32
#include "malloc.h"
#endif
// MODULE includes:
#include "zbitmapdesc.h"
// ZBSLIB includes:


// reimplemented here to eliminate dependecy on mathtools.cpp
static inline int nextPow2( int x ) {
	int p = 2;
	for( int i=1; i<31; i++ ) {
		if( p >= x ) {
			return p;
		}
		p *= 2;
	}
	return 0;
}

static inline int nextQWord( int x ) {
	int remainder = x & 7;
	if( remainder ) {
		return x+(8-remainder);
	}

	return x;
}

#ifndef min
	#define min(a,b) ( ( (a) < (b) ) ? (a) : (b) )
#endif
#ifndef max
	#define max(a,b) ( ( (a) > (b) ) ? (a) : (b) )
#endif


// ZBitmapDesc
//==================================================================================

ZBitmapDesc::ZBitmapDesc() {
	memset( this, 0, sizeof(*this) );
}

ZBitmapDesc::ZBitmapDesc( ZBitmapDesc &copy ) {
	memcpy( this, &copy, sizeof(*this) );
	bits = NULL;
	owner = 0;
	iplImage = NULL;
	bitmapInfoHeader = NULL;
}

ZBitmapDesc::~ZBitmapDesc() {
	clear();
}

void ZBitmapDesc::_clear() {
	if( bits && owner ) free(bits);
	memset( this, 0, sizeof(*this) );
}

void ZBitmapDesc::copy( ZBitmapDesc &src ) {
	clear();
	owner = 1;
	w = src.w;
	h = src.h;
	d = src.d;
	memW = src.memW;
	memH = src.memH;
	if( src.bits ) {
		bits = (char *)malloc( src.memH * src.memW * src.d );
		memcpy( bits, src.bits, src.memH * src.memW * src.d );
	}
}

void ZBitmapDesc::init   ( int _w, int _h, int _d, char *_bits ) {
	clear();
	w = _w;
	h = _h;
	d = _d;
	memW = w;
	memH = h;
	bits = _bits;
}

void ZBitmapDesc::initP2 ( int _w, int _h, int _d, char *_bits ) {
	clear();
	w = _w;
	h = _h;
	d = _d;
	memW = nextPow2(w);
	memH = nextPow2(h);
	bits = _bits;
}

void ZBitmapDesc::initP2W( int _w, int _h, int _d, char *_bits ) {
	clear();
	w = _w;
	h = _h;
	d = _d;
	memW = nextPow2(w);
	memH = h;
	bits = _bits;
}

void ZBitmapDesc::initSP2 ( int _w, int _h, int _d, char *_bits ) {
	clear();
	w = _w;
	h = _h;
	d = _d;
	memW = max( nextPow2(w), nextPow2(h) );
	memH = max( nextPow2(w), nextPow2(h) );
	bits = _bits;
}

void ZBitmapDesc::initQWord ( int _w, int _h, int _d, char *_bits ) {
	clear();
	w = _w;
	h = _h;
	d = _d;
	memW = nextQWord(w);
	memH = h;
	bits = _bits;
}

void ZBitmapDesc::alloc  ( int _w, int _h, int _d ) {
	init( _w, _h, _d, NULL );
	bits = (char *)malloc( memW * memH * d );
	memset( bits, 0, memW * memH * d );
	owner = 1;
}

void ZBitmapDesc::allocP2( int _w, int _h, int _d ) {
	initP2( _w, _h, _d, NULL );
	bits = (char *)malloc( memW * memH * d );
	memset( bits, 0, memW * memH * d );
	owner = 1;
}

void ZBitmapDesc::allocP2W( int _w, int _h, int _d ) {
	initP2W( _w, _h, _d, NULL );
	bits = (char *)malloc( memW * memH * d );
	memset( bits, 0, memW * memH * d );
	owner = 1;
}

void ZBitmapDesc::allocSP2( int _w, int _h, int _d ) {
	initSP2( _w, _h, _d, NULL );
	bits = (char *)malloc( memW * memH * d );
	memset( bits, 0, memW * memH * d );
	owner = 1;
}

void ZBitmapDesc::allocQWord( int _w, int _h, int _d ) {
	initQWord( _w, _h, _d, NULL );
	bits = (char *)malloc( memW * memH * d );
	memset( bits, 0, memW * memH * d );
	owner = 1;
}

void ZBitmapDesc::fill( int val ) {
	memset( bits, val, bytes() );
}

// ZBitmapDesc Tools Interface
//==================================================================================

void zBitmapDescInvertLines( ZBitmapDesc &src ) {
	assert( src.bits );
	int p = src.p();
	char *temp = (char *)alloca( p );
	char *s = src.bits;
	char *d = src.bits + (src.memH-1) * p;
	for( int y=0; y<src.memH/2; y++ ) {
		memcpy( temp, s, p );
		memcpy( s, d, p );
		memcpy( d, temp, p );
		s += p;
		d -= p;
	}
}

void zBitmapDescSetChannel( ZBitmapDesc &src, int channel, int value ) {
}

void zBitmapDescInvertBGR( ZBitmapDesc &src ) {
	assert( src.d == 3 || src.d == 4 );
	if( src.d == 3 ) {
		for( int y=0; y<src.h; y++ ) {
			unsigned char *s = src.lineUchar( y );
			for( int x=0; x<src.w; x++ ) {
				unsigned char temp;
				temp = s[0];
				s[0] = s[2];
				s[2] = temp;
				s += 3;
			}
		}
	}
	else if( src.d == 4 ) {
		for( int y=0; y<src.h; y++ ) {
			unsigned char *s = src.lineUchar( y );
			for( int x=0; x<src.w; x++ ) {
				unsigned char temp;
				temp = s[0];
				s[0] = s[2];
				s[2] = temp;
				s += 4;
			}
		}
	}
}

void zBitmapDescSetTransparent( ZBitmapDesc &src, int keyColor, int alphaValue ) {
	unsigned char alpha = (unsigned char)alphaValue;
	unsigned char keyR = (keyColor&0xFF000000) >> 24;
	unsigned char keyG = (keyColor&0x00FF0000) >> 16;
	unsigned char keyB = (keyColor&0x0000FF00) >> 8;

	assert( src.d == 4 );

	for( int y=0; y<src.h; y++ ) {
		unsigned char *s = src.lineUchar( y );
		for( int x=0; x<src.w; x++ ) {
			if( keyColor == bdANY || (s[0] == keyR && s[1] == keyG && s[2] == keyB) ) {
				s[3] = alpha;
			}
			s += 4;
		}
	}
}

void zBitmapDescConvert( ZBitmapDesc &src, ZBitmapDesc &dst ) {
	// Assumes that if dst.bits is already allocated that we should write there
	// otherwise that it needs to be allocated

	// ALLOC dst bits if needed
	if( !dst.bits ) {
		dst.bits = (char *)malloc( dst.p() * dst.memH );
		dst.owner = 1;
	}

	// CLEAR dst to all 0
	memset( dst.bits, 0, dst.memH * dst.p() );

	assert( src.w == dst.w && src.h == dst.h );
		// TODO: implement stretching

	if( src.d == 1 ) {
		if( dst.d == 1 ) {
			for( int y=0; y<src.h; y++ ) {
				char *s = src.lineChar( y );
				char *d = dst.lineChar( y );
				memcpy( d, s, src.w * src.d );
			}
		}
		else if( dst.d == 2 ) {
			assert( 0 );
				// TODO; Implement 565 conversion
		}
		else if( dst.d == 3 ) {
			for( int y=0; y<src.h; y++ ) {
				char *s = src.lineChar( y );
				char *d = dst.lineChar( y );
				for( int x=0; x<src.w; x++ ) {
					*d++ = *s;
					*d++ = *s;
					*d++ = *s;
					s++;
				}
			}
		}
		else if( dst.d == 4 ) {
			for( int y=0; y<src.h; y++ ) {
				char *s = src.lineChar( y );
				char *d = dst.lineChar( y );
				for( int x=0; x<src.w; x++ ) {
					*d++ = *s;
					*d++ = *s;
					*d++ = *s;
					*d++ = (char)255;
					s++;
				}
			}
		}
		else {
			assert( 0 );
				// Unknown depth
		}
	}
	else if( src.d == 2 ) {
		assert( dst.d == 2 );	// For now only 2 to 2
			for( int y=0; y<src.h; y++ ) {
				unsigned char *s = src.lineUchar( y );
				unsigned char *d = dst.lineUchar( y );
				memcpy( d, s, src.p() );
			}
	}
	else if( src.d == 3 ) {
		if( dst.d == 1 ) {
			for( int y=0; y<src.h; y++ ) {
				unsigned char *s = src.lineUchar( y );
				unsigned char *d = dst.lineUchar( y );
				for( int x=0; x<src.w; x++ ) {
					unsigned char r = *s++;
					unsigned char g = *s++;
					unsigned char b = *s++;
					*d++ = max( r, max( g, b ) );
				}
			}
		}
		else if( dst.d == 2 ) {
			assert( 0 );
				// TODO; Implement 565 conversion
		}
		else if( dst.d == 3 ) {
			for( int y=0; y<src.h; y++ ) {
				char *s = src.lineChar( y );
				char *d = dst.lineChar( y );
				memcpy( d, s, src.w * src.d );
			}
		}
		else if( dst.d == 4 ) {
			for( int y=0; y<src.h; y++ ) {
				char *s = src.lineChar( y );
				char *d = dst.lineChar( y );
				for( int x=0; x<src.w; x++ ) {
					*d++ = *s++;
					*d++ = *s++;
					*d++ = *s++;
					*d++ = (char)255;
				}
			}
		}
		else {
			assert( 0 );
				// Unknown depth
		}
	}
	else if( src.d == 4 ) {
		if( dst.d == 1 ) {
			for( int y=0; y<src.h; y++ ) {
				unsigned char *s = src.lineUchar( y );
				unsigned char *d = dst.lineUchar( y );
				for( int x=0; x<src.w; x++ ) {
					unsigned char r = *s++;
					unsigned char g = *s++;
					unsigned char b = *s++;
					s++;
					*d++ = max( r, max( g, b ) );
				}
			}
		}
		else if( dst.d == 2 ) {
			assert( 0 );
				// TODO; Implement 565 conversion
		}
		else if( dst.d == 3 ) {
			for( int y=0; y<src.h; y++ ) {
				char *s = src.lineChar( y );
				char *d = dst.lineChar( y );
				for( int x=0; x<src.w; x++ ) {
					*d++ = *s++;
					*d++ = *s++;
					*d++ = *s++;
					s++;
				}
			}
		}
		else if( dst.d == 4 ) {
			for( int y=0; y<src.h; y++ ) {
				char *s = src.lineChar( y );
				char *d = dst.lineChar( y );
				memcpy( d, s, src.w * src.d );
			}
		}
		else {
			assert( 0 );
				// Unknown depth
		}
	}
	else {
		assert( 0 );
			// Unknown depth
	}
}

void zBitmapDescBrightnessAdjust( ZBitmapDesc &bd, int lowerBound, int upperBound ) {
	// array of 256 output levels
	int outputLevels[256];

	assert( lowerBound <= upperBound );

	float step = ((float)upperBound - (float)lowerBound) / 255.f;
	float out = (float)lowerBound;

	for( int i=0; i<256; i++ ) {
		int o = 0;
		float f = (float)floor(out);
		if( out - f < 0.5f ) o = (int)f;
		else o = (int)f + 1;
		outputLevels[i] = max( 0, min( 255, o ) );
		out += step;
	}
	 
	int depth = bd.d == 4 ? 3 : bd.d;
	int skip  = bd.d == 4 ? 1 : 0;
	for( int y=0; y<bd.h; y++ ) {
		unsigned char *s = bd.lineUchar( y );
		for( int x=0; x<bd.w; x++ ) {
			for( int d=0; d<depth; d++ ) {
				*s++ = outputLevels[*s];
			}
			s += skip;
		}
	}
}

void zBitmapDescWriteInto( ZBitmapDesc &src, ZBitmapDesc &dst, int x, int y ) {
	assert( src.d == 4 && dst.d == 4 );

	int dl = max( 0, min( dst.w-1, x ) );
	int dr = max( 0, min( dst.w-1, x + src.w-1 ) );
	int dt = max( 0, min( dst.w-1, y ) );
	int db = max( 0, min( dst.w-1, y + src.h-1 ) );

	int sl = x < 0 ? x : 0;
	int sr = sl + (dr - dl);
	int st = y < 0 ? y : 0;
	int sb = st + (db - dt);

	assert( sr-sl == dr-dl );
	assert( sb-st == db-dt );

	int sy=st;
	for( int dy=dt; dy <= db; dy++, sy++ ) {
		unsigned char *d = &dst.lineUchar( dy )[dl*4];
		unsigned char *s = &src.lineUchar( sy )[sl*4];
		for( int dx=dl; dx <= dr; dx++ ) {

			unsigned int srcAlpha = *(s+3);
			unsigned int dstAlpha = *(d+3);
//			*d++ = max( 0, min( 255, srcAlpha + dstAlpha ) );
//			s++;

			*d++ = max( 0, min( 255, (srcAlpha * (unsigned int)*s)/256 + (255 - srcAlpha) * (unsigned int)*d / 256 ) );
			s++;
			*d++ = max( 0, min( 255, (srcAlpha * (unsigned int)*s)/256 + (255 - srcAlpha) * (unsigned int)*d / 256 ) );
			s++;
			*d++ = max( 0, min( 255, (srcAlpha * (unsigned int)*s)/256 + (255 - srcAlpha) * (unsigned int)*d / 256 ) );
			s++;
			*d++ = max( 0, min( 255, (srcAlpha * (unsigned int)*s)/256 + (255 - srcAlpha) * (unsigned int)*d / 256 ) );
			s++;

//			*d++ = *s++;
//			*d++ = *s++;
//			*d++ = *s++;
//			*d++ = *s++;
		}
	}
}

/*  This is test AVI code.  For future reference
#include "vfw.h"
void plugInStartup( char *options ) {
	createTexturedSphereList( 1.0, sphereList, 16 );
	//texture = zglGenTexture( "art\\eye.bmp" );
	backgroundSelect( NULL, 0x000000FF );

	AVIFileInit();

	PAVIFILE ppfile;
	int ret = AVIFileOpen( &ppfile, "\\video\\eye\\eyeblink3.avi", OF_SHARE_DENY_WRITE, 0 );
	
	PAVISTREAM ppstream;
	ret = AVIFileGetStream( ppfile, &ppstream, streamtypeVIDEO, 0 );
	assert( !ret );

	AVISTREAMINFOW psi;
	ret = AVIStreamInfoW( ppstream, &psi, sizeof(psi) );
	assert( !ret );

	BITMAPINFOHEADER biWanted;
	biWanted.biBitCount = 24;
	biWanted.biClrImportant = 0;
	biWanted.biCompression = BI_RGB;
	biWanted.biHeight = psi.rcFrame.bottom - psi.rcFrame.top;
	biWanted.biWidth = psi.rcFrame.right - psi.rcFrame.left;
	biWanted.biPlanes = 1;
	biWanted.biSize = 40;
	biWanted.biXPelsPerMeter = 0;
	biWanted.biYPelsPerMeter = 0;
	biWanted.biSizeImage = (((biWanted.biWidth * 3) + 3) & 0xFFFC) * biWanted.biHeight;
	PGETFRAME pGetFrame = AVIStreamGetFrameOpen( ppstream, &biWanted );
	assert( pGetFrame );

	for( int i=0; i<13; i++ ) {
		BITMAPINFOHEADER *biGot;
		biGot = (BITMAPINFOHEADER *)AVIStreamGetFrame( pGetFrame, i );
		assert( biGot );
		ZBitmapDesc srcDesc, dstDesc;
		srcDesc.init( biGot->biWidth, biGot->biHeight, 3, (char *)biGot + sizeof(BITMAPINFOHEADER) );
		dstDesc.allocP2( biGot->biWidth, biGot->biHeight, 4 );
		bdInvertBGR( srcDesc );
		bdConvert( srcDesc, dstDesc );
		texture[i] = zglGenTextureFromBitmapDesc( &dstDesc );
	}

	AVIFileRelease( ppfile );
	AVIFileExit();
}
*/

