// @ZBS {
//		*MODULE_NAME zbitTexCache
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			This interface manages the upload of large non-pow2 textures.  Given a ZBits * it creates
//			an array of smaller pow2 textures and stores them in an heap array which is itself
//			stored in a hash with the key being the ZBits pointer.
//			Freeing from this cache only deleted the texture array, it doesn't attempt to manage the ZBits *
//		}
//		+EXAMPLE {
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zbittexcache.cpp zbittexcache.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }


// SDK includes:
//#include "ipl.h"
//#include "cv.h"
#include "wingl.h"
#include "GL/gl.h"
#include "GL/glu.h"
// STDLIB includes:
#include "assert.h"
#include "stdlib.h"
#include "string.h"
// ZBSLIB includes:
#include "zbittexcache.h"
#include "zbits.h"

int zbitTexCacheComputeChunkSize( int w, int h ) {
	int chunkSize = min( w, h );
	int pow2 = 1;
	while( 1 ) {
		if( pow2 > chunkSize ) {
			pow2 >>= 1;
			break;
		}
		if( pow2 == chunkSize ) {
			break;
		}
		pow2 <<= 1;
	}

	// pow2 now has the largest power of 2 that is smaller or equal to the chunkSize
	chunkSize = min( 128, pow2 );
	chunkSize = max( 1, chunkSize );
	return chunkSize;
}

void zbitTexCacheGetDims( int *texCache, int &w, int &h ) {
	w = texCache[0];
	h = texCache[1];
}

int *zbitTexCacheLoad( ZBits *bits, int invert ) {
	int x, y;
	int chunkSize = zbitTexCacheComputeChunkSize( bits->w(), bits->h() );

	int bitsW = bits->w();

	assert( bitsW != 2 );
	// There is some bug I haven't figured out yet having to do with alignment causing width of 2 to be messed up

	int bitsH = bits->h();
	GLuint *textures = (GLuint *)malloc( sizeof(GLuint) * (bitsH/chunkSize+1) * (bitsW/chunkSize+1) + 3*sizeof(int) );
		// The +3 is for the width and height and numtextures that are stuck into the first three ints

	textures[0] = bits->w();
	textures[1] = bits->h();
	textures[2] = 0;
	int texCount = 3;

	int d = bits->pixDepthInBytes();
	char *tempBuf = (char *)malloc( d * chunkSize * chunkSize );

	for( y=0; y<bitsH; y+=chunkSize ) {
		int h = min( chunkSize, bits->h()-y );

		for( x=0; x<bitsW; x+=chunkSize ) {
			int w = min( chunkSize, bits->w()-x );

			// CREATE a texture and upload the bits
			glGenTextures( 1, &textures[texCount] );

			glBindTexture( GL_TEXTURE_2D, textures[texCount] );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0 );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0 );
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

			// UPLOAD the bits
			int fmt = -1;
			if( bits->alphaChannel != 0 && bits->channels == 1 ) fmt = GL_LUMINANCE_ALPHA;
			else if( bits->alphaChannel == 0 && bits->channels == 1 ) fmt = GL_LUMINANCE;
			else if( bits->alphaChannel != 0 && bits->channels == 3 ) fmt = GL_RGBA;
			else if( bits->alphaChannel == 0 && bits->channels == 3 ) fmt = GL_RGB;
			assert( fmt != -1 );

			int type = -1;
			if( bits->channelDepthInBits == 8  && bits->depthIsSigned == 0 ) type = GL_UNSIGNED_BYTE;
			else if( bits->channelDepthInBits == 8  && bits->depthIsSigned != 0 ) type = GL_BYTE;
			else if( bits->channelDepthInBits == 16 && bits->depthIsSigned == 0 ) type = GL_UNSIGNED_SHORT;
			else if( bits->channelDepthInBits == 16 && bits->depthIsSigned != 0 ) type = GL_SHORT;
			else if( bits->channelDepthInBits == 32 && bits->depthIsSigned == 0 ) type = GL_UNSIGNED_INT;
			else if( bits->channelDepthInBits == 32 && bits->depthIsSigned != 0 ) {
				type = GL_INT;
			}
			if( bits->channelTypeFloat ) type = GL_FLOAT;
			assert( type != -1 );

			// MAKE a copy into a contiguous buffer (this is absurd... if only teximage2d took a pitch parameter!)
			char *dst = tempBuf;
			if( invert ) {
				char *src = &bits->lineChar( bitsH - y - 1 )[x*d];
				for( int _y=0; _y<h; _y++ ) {
					memcpy( dst, src, w * d );
					dst += chunkSize * d;
					src -= bits->width * d;
				}
			}
			else {
				char *src = &bits->lineChar( y )[x*d];
				for( int _y=0; _y<h; _y++ ) {
					/*
					Comment this in and comment out the memcpy below to generate a test grid. - Ken
					char *_dst = dst;
					for( int _x=0 ; _x<w ; ++_x )
						if( (_y&31) < 8 || (_x&31) < 8 )
							*_dst++ = 0x0;
						else
							*_dst++ = 0x77;
					*/
					memcpy( dst, src, w * d );
					dst += chunkSize * d;
					src += bits->width * d;
				}
			}

			glTexImage2D(
				GL_TEXTURE_2D, 0, bits->alphaChannel ? bits->channels+1 : bits->channels,
				chunkSize, chunkSize, 0, fmt, type, tempBuf
			);
			GLenum a = glGetError();

			texCount++;
		}
	}

	textures[2] = texCount-3;

	free( tempBuf );
	return (int *)textures;
}

int *zbitTexCacheLoadFloatScale( struct ZBits *bits, int invert, float scale ) {
	int x, y;
	int chunkSize = zbitTexCacheComputeChunkSize( bits->w(), bits->h() );

	int bitsW = bits->w();
	int bitsH = bits->h();
	GLuint *textures = (GLuint *)malloc( sizeof(GLuint) * (bitsH/chunkSize+1) * (bitsW/chunkSize+1) + 3*sizeof(int) );
		// The +3 is for the width and height and numtextures that are stuck into the first three ints

	textures[0] = bits->w();
	textures[1] = bits->h();
	textures[2] = 0;
	int texCount = 3;

	assert( bits->channelTypeFloat );
	assert( bits->channels == 1 );
	assert( bits->channelDepthInBits == 32 );

	int d = bits->pixDepthInBytes();
	float *tempBuf = (float *)malloc( d * chunkSize * chunkSize );

	for( y=0; y<bitsH; y+=chunkSize ) {
		int h = min( chunkSize, bits->h()-y );

		for( x=0; x<bitsW; x+=chunkSize ) {
			int w = min( chunkSize, bits->w()-x );

			// CREATE a texture and upload the bits
			glGenTextures( 1, &textures[texCount] );

			glBindTexture( GL_TEXTURE_2D, textures[texCount] );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0 );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0 );
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

			// UPLOAD the bits
			int fmt = GL_LUMINANCE;
			int type = GL_FLOAT;

			// MAKE a copy into a contiguous buffer (this is absurd... if only teximage2d took a pitch parameter!)
			float *dst = tempBuf;
			if( invert ) {
				float *src = &bits->lineFloat( bitsH - y - 1 )[x];
				for( int _y=0; _y<h; _y++ ) {
					float *_dst = dst;
					float *_src = src;
					for( int _x=0; _x<w; _x++ ) {
						*_dst = *_src * scale;
						_dst++;
						_src++;
					}
					dst += chunkSize;
					src -= bits->width;
				}
			}
			else {
				float *src = &bits->lineFloat( y )[x];
				for( int _y=0; _y<h; _y++ ) {
					float *_dst = dst;
					float *_src = src;
					for( int _x=0; _x<w; _x++ ) {
						*_dst = *_src * scale;
						_src++;
						_dst++;
					}
					dst += chunkSize;
					src += bits->width;
				}
			}

			glTexImage2D(
				GL_TEXTURE_2D, 0, bits->channels,
				chunkSize, chunkSize, 0, fmt, type, tempBuf
			);
			GLenum a = glGetError();

			texCount++;
		}
	}

	textures[2] = texCount-3;

	free( tempBuf );
	return (int *)textures;
}

int *zbitTexCacheLoadDoubleScale( struct ZBits *bits, int invert, double scale ) {
	int x, y;
	int chunkSize = zbitTexCacheComputeChunkSize( bits->w(), bits->h() );

	int bitsW = bits->w();
	int bitsH = bits->h();
	GLuint *textures = (GLuint *)malloc( sizeof(GLuint) * (bitsH/chunkSize+1) * (bitsW/chunkSize+1) + 3*sizeof(int) );
		// The +3 is for the width and height and numtextures that are stuck into the first three ints

	textures[0] = bits->w();
	textures[1] = bits->h();
	textures[2] = 0;
	int texCount = 3;

	assert( bits->channelTypeFloat );
	assert( bits->channels == 1 );
	assert( bits->channelDepthInBits == 64 );

	float *tempBuf = (float *)malloc( sizeof(float) * chunkSize * chunkSize );

	for( y=0; y<bitsH; y+=chunkSize ) {
		int h = min( chunkSize, bits->h()-y );

		for( x=0; x<bitsW; x+=chunkSize ) {
			int w = min( chunkSize, bits->w()-x );

			// CREATE a texture and upload the bits
			glGenTextures( 1, &textures[texCount] );

			glBindTexture( GL_TEXTURE_2D, textures[texCount] );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0 );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0 );
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

			// UPLOAD the bits
			int fmt = GL_LUMINANCE;
			int type = GL_FLOAT;

			// MAKE a copy into a contiguous buffer (this is absurd... if only teximage2d took a pitch parameter!)
			float *dst = tempBuf;
			if( invert ) {
				double *src = (double *)bits->ptrVoid( x, bitsH - y - 1 );
				for( int _y=0; _y<h; _y++ ) {
					float *_dst = dst;
					double *_src = src;
					for( int _x=0; _x<w; _x++ ) {
						*_dst = float( *_src * scale );
						_dst++;
						_src++;
					}
					dst += chunkSize;
					src -= bits->width;
				}
			}
			else {
				double *src = (double *)bits->ptrVoid( x, y );
				for( int _y=0; _y<h; _y++ ) {
					float *_dst = dst;
					double *_src = src;
					for( int _x=0; _x<w; _x++ ) {
						*_dst = float( *_src * scale );
						_src++;
						_dst++;
					}
					dst += chunkSize;
					src += bits->width;
				}
			}

			glTexImage2D(
				GL_TEXTURE_2D, 0, bits->channels,
				chunkSize, chunkSize, 0, fmt, type, tempBuf
			);
			GLenum a = glGetError();

			texCount++;
		}
	}

	textures[2] = texCount-3;

	free( tempBuf );
	return (int *)textures;
}



void zbitTexCacheFree( int *texCache ) {
	if( !texCache ) return;
	int w = texCache[0];
	int h = texCache[1];
	int count = texCache[2];
	int chunkSize = zbitTexCacheComputeChunkSize( w, h );
	glDeleteTextures( count, (GLuint *)&texCache[3] );
	free( texCache );
}

void zbitTexCacheRender( int *texCache ) {
	if( !texCache ) return;
	int w = texCache[0];
	int h = texCache[1];
	int count = texCache[2];
	int texI = 3;

	int chunkSize = zbitTexCacheComputeChunkSize( w, h );
	int x, y;

	// RENDER the quads
	glPushAttrib( GL_ENABLE_BIT | GL_TEXTURE_BIT );
	glEnable( GL_TEXTURE_2D );
	int _y = h;
	for( y=0; y<h; y+=chunkSize, _y-=chunkSize ) {
		int _h = min( chunkSize, h-y );
		float th = (float)_h / (float)chunkSize;

		for( x=0; x<w; x+=chunkSize ) {
			int _w = min( chunkSize, w-x );
			float tw = (float)_w / (float)chunkSize;

			glBindTexture( GL_TEXTURE_2D, texCache[texI++] );

			glBegin( GL_QUADS );
				glTexCoord2f( 0.f, 0.f );
				glVertex2i( x, _y );

				glTexCoord2f( tw, 0.f );
				glVertex2i( x+_w, _y );

				glTexCoord2f( tw, th );
				glVertex2i( x+_w, _y-_h );

				glTexCoord2f( 0, th );
				glVertex2i( x, _y-_h );
			glEnd();
		}
	}
	glPopAttrib();
}
