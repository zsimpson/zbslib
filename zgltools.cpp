// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A collection of tools for dealing with open gl such as texture loading and some common geometry
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zgltools.cpp zgltools.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//			Need to elminate dependency on the zvec which is gratuitous
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#ifdef __APPLE__
#include "OpenGL/gl.h"
#include "OpenGL/glu.h"
#else
#include "GL/gl.h"
#include "GL/glu.h"
#endif
// STDLIB includes:
#include "assert.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "float.h"
#include "math.h"
// MODULE includes:
#include "zgltools.h"
// ZBSLIB includes:
#include "zbitmapdesc.h"
#include "zbits.h"
	// Eventually, zbits is to replace zbitmapdesc
#include "zvec.h"

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

static inline int __isPow2( int x ) {
	if( x == 0 ) return 1;
	int count = 0;
	for( int i=0; i<32; i++ ) {
		count += x&1;
		x >>= 1;
	}
	return( count == 1 );
}

float zglGetAspect() {
	float viewport[4];
	glGetFloatv( GL_VIEWPORT, viewport );
	return viewport[2] / viewport[3];
}

void zglMakeMipMapsFromBitmapDesc( ZBitmapDesc *desc ) {
	gluBuild2DMipmaps( GL_TEXTURE_2D, 4, desc->memW, desc->memH, GL_RGBA, GL_UNSIGNED_BYTE, desc->bits );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
}

void zglMakeMipMapsFromZBits( struct ZBits *desc ) {
	gluBuild2DMipmaps( GL_TEXTURE_2D, 4, desc->width, desc->height, GL_RGBA, GL_UNSIGNED_BYTE, desc->bits );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
}

int zglGenTexture() {
	GLuint texture = 0;
	glGenTextures( 1, &texture );
	glBindTexture( GL_TEXTURE_2D, texture );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0 );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0 );
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	return texture;
}

void zglLoadTextureFromBitmapDesc( struct ZBitmapDesc *desc, int texture, int vertFlip ) {
	ZBitmapDesc dstDesc( *desc );

	if( !__isPow2(desc->memW) || !__isPow2(desc->memH) ) {
//		dstDesc.memW = max( __nextPow2( dstDesc.w ), __nextPow2( dstDesc.h ) );
//		dstDesc.memH = max( __nextPow2( dstDesc.w ), __nextPow2( dstDesc.h ) );
		dstDesc.memW = __nextPow2( dstDesc.w );
		dstDesc.memH = __nextPow2( dstDesc.h );
			// Some function somewhere expects this to drive both the w
			// and h to the same power of 2.  However, planets doesn't want this
			// as the textures are significantly wider than tall.

		zBitmapDescConvert( *desc, dstDesc );
		desc = &dstDesc;
	}

	if( vertFlip ) {
		zBitmapDescInvertLines( dstDesc );
	}

	glBindTexture( GL_TEXTURE_2D, texture );
	static int dtogl[] = { 0, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		desc->d,
		desc->memW,
		desc->memH,
		0,
		dtogl[desc->d],
		GL_UNSIGNED_BYTE,
		desc->bits
	);
}

int zglGenTextureFromBitmapDesc( struct ZBitmapDesc *desc, int vertFlip ) {
	int texture = zglGenTexture();
	zglLoadTextureFromBitmapDesc( desc, texture, vertFlip );
	return texture;
}

int isPowerOf2( int a ) {
	int sum = 0;
	for( int i=0; i<32; i++ ) {
		sum += ( ( a&(1<<i) ) >> i );
	}
	return sum == 1;
}

void zglGeneralLoadTextureFromZBits( struct ZBits *desc, int texture, int glTexEnum, int glImgEnum, int vertFlip )
{
	int fmt = -1;
	if( desc->alphaChannel != 0 && desc->channels == 1 ) fmt = GL_LUMINANCE_ALPHA;
	else if( desc->alphaChannel == 0 && desc->channels == 1 ) fmt = GL_LUMINANCE;
	else if( desc->alphaChannel != 0 && desc->channels == 3 ) fmt = GL_RGBA;
	else if( desc->alphaChannel == 0 && desc->channels == 3 ) fmt = GL_RGB;
	assert( fmt != -1 );

	int type = -1;
	if( desc->channelDepthInBits == 8  && desc->depthIsSigned == 0 ) type = GL_UNSIGNED_BYTE;
	else if( desc->channelDepthInBits == 8  && desc->depthIsSigned != 0 ) type = GL_BYTE;
	else if( desc->channelDepthInBits == 16 && desc->depthIsSigned == 0 ) type = GL_UNSIGNED_SHORT;
	else if( desc->channelDepthInBits == 16 && desc->depthIsSigned != 0 ) type = GL_SHORT;
	else if( desc->channelDepthInBits == 32 && desc->depthIsSigned == 0 ) type = GL_UNSIGNED_INT;
	else if( desc->channelDepthInBits == 32 && desc->depthIsSigned != 0 ) type = GL_INT;
	else if( desc->channelTypeFloat ) type = GL_FLOAT;
	assert( type != -1 );

	// RAISE to next power of 2
	char *tempBuf = 0;
	int pow2W = desc->width;
	int pow2H = desc->height;
	if( ! isPowerOf2(desc->width) || ! isPowerOf2(desc->height) ) {
		pow2W = __nextPow2(desc->w());
		pow2H = __nextPow2(desc->h());

		// MAKE a copy into a contiguous buffer (this is absurd... if only teximage2d took a pitch parameter!)
		tempBuf = (char *)malloc( 4 * pow2W * pow2H );
		memset( tempBuf, 0, 4 * pow2W * pow2H );
		int d = desc->pixDepthInBytes();
		assert( d <= 4 );

		int w = desc->w();
		int h = desc->h();
		char *dst = tempBuf;
		if( vertFlip ) {
			for( int _y=h-1; _y>=0; _y-- ) {
				char *src = desc->lineChar( _y );
				memcpy( dst, src, w * d );
				dst += pow2W * d;
				src += desc->width * d;
			}
		}
		else {
			for( int _y=0; _y<h; _y++ ) {
				char *src = desc->lineChar( _y );
				memcpy( dst, src, w * d );
				dst += pow2W * d;
				src += desc->width * d;
			}
		}
	}

	glBindTexture( (GLenum) glTexEnum, texture );

	glTexImage2D(
		(GLenum) glImgEnum, 0, desc->alphaChannel ? desc->channels+1 : desc->channels, 
		pow2W, pow2H, 0, fmt, type, tempBuf ? tempBuf : desc->bits
	);

	if( tempBuf ) {
		free( tempBuf );
	}
}

void zglLoadTextureFromZBits( struct ZBits *desc, int texture, int vertFlip ) {
	zglGeneralLoadTextureFromZBits( desc, texture, GL_TEXTURE_2D, GL_TEXTURE_2D, vertFlip );
}

int zglGenTextureFromZBits( struct ZBits *desc, int vertFlip ) {
	int texture = zglGenTexture();
	zglLoadTextureFromZBits( desc, texture, vertFlip );
	return texture;
}




// Geometry Builder
//=================================================================================
unsigned int zglTexturedSphereList( double radius, int segments ) {
	//DVec3 mesh[17][16];
	DVec3 *mesh = (DVec3 *)malloc( sizeof(DVec3) * (segments+2) * (segments+2) );

	double er = radius;
	int y;
	for( y=0; y<=segments; y++ ) {
		double _y = er * cos( (double)y * 3.1415926539 / (double)segments );

		double r = er * cos( (3.1415926539/2.0) - acos(_y/er) );
		for( int x=0; x<segments; x++ ) {
			double _x = (double)x / (double)segments;
			//mesh[y][x] = DVec3( r * cos(PI2*_x), _y, r * sin(PI2*_x) );
			mesh[y*segments+x] = DVec3( r * cos(2.0*3.1415926539*_x), _y, r * sin(2.0*3.1415926539*_x) );
		}
	}

	unsigned int list = glGenLists( 1 );

	// NOTE: Faces are counter-clock wise to match OpenGL defaults.
	glNewList( list, GL_COMPILE );
	glBegin( GL_TRIANGLE_STRIP );
		for( y=0; y<=segments; y++ ) {
			// WIND a triangle strip around a constant latitude
			for( int x=0; x<=segments; x++ ) {
				DVec3 norm = mesh[ ((y+1)%(segments+1))*segments + (x%segments) ];
				norm.normalize();
				glNormal3dv( norm );
				glTexCoord2d( -(double)x / (double)segments, (double)(y+1) / (double)segments );
				glVertex3dv( mesh[ ((y+1)%(segments+1))*segments + (x%segments) ] );

				norm = mesh[ ((y+0)%(segments+1))*segments + (x%segments) ];
				norm.normalize();
				glNormal3dv( norm );
				glTexCoord2d( -(double)x / (double)segments, (double)y / (double)segments );
				glVertex3dv( mesh[ ((y+0)%(segments+1))*segments + (x%segments) ] );
			}
		}
	glEnd();
	glEndList();
	
	free( mesh );
	return list;
}

unsigned int zglWireSphereList( double radius, int segments ) {
	//DVec3 mesh[17][16];
	DVec3 *mesh = (DVec3 *)malloc( sizeof(DVec3) * segments * (segments+1) );

	double er = radius;
	int y;
	for( y=0; y<segments+1; y++ ) {
		double _y = er * cos( (double)y * 3.1415926539 / (double)segments );

		double r = er * cos( (3.1415926539/2.0) - acos(_y/er) );
		for( int x=0; x<segments; x++ ) {
			double _x = (double)x / (double)segments;
			//mesh[y][x] = DVec3( r * cos(PI2*_x), _y, r * sin(PI2*_x) );
			mesh[y*segments+x] = DVec3( r * cos(2.0*3.1415926539*_x), _y, r * sin(2.0*3.1415926539*_x) );
		}
	}

	unsigned int list = glGenLists( 1 );

	glNewList( list, GL_COMPILE );
	glBegin( GL_LINE_STRIP );
		for( y=0; y<segments+1; y++ ) {
			for( int x=0; x<=segments; x++ ) {
				glVertex3dv( mesh[ (y%(segments+1))*segments + (x%segments) ] );
				glVertex3dv( mesh[ ((y+1)%(segments+1))*segments + (x%segments) ] );
			}
		}
	glEnd();
	glEndList();

	free( mesh );
	return list;
}

unsigned int zglCubeList( float side ) {
	unsigned int list = glGenLists( 1 );
	glNewList( list, GL_COMPILE );
	float p = side / 2.f;
	float n = -side / 2.f;
	float verts[8][3] = {
		n, n, p,
		p, n, p,
		p, p, p,
		n, p, p,
		n, n, n,
		p, n, n,
		p, p, n,
		n, p, n,
	};
	glVertexPointer( 3, GL_FLOAT, 0, verts );
	glEnable( GL_VERTEX_ARRAY );
		// TFB sez: does this really work? I don't think you can use these
		// verts on the stack in a display list.  
	glBegin( GL_QUADS );
		// FRONT
		glNormal3f( 0.f, 0.f, -1.f );
		glTexCoord2f( 0.f, 0.f );
		glArrayElement( 0 );
		glTexCoord2f( 1.f, 0.f );
		glArrayElement( 1 );
		glTexCoord2f( 1.f, 1.f );
		glArrayElement( 2 );
		glTexCoord2f( 0.f, 1.f );
		glArrayElement( 3 );
		// BACK
		glNormal3f( 0.f, 0.f, 1.f );
		glTexCoord2f( 0.f, 0.f );
		glArrayElement( 7 );
		glTexCoord2f( 1.f, 0.f );
		glArrayElement( 6 );
		glTexCoord2f( 1.f, 1.f );
		glArrayElement( 5 );
		glTexCoord2f( 0.f, 1.f );
		glArrayElement( 4 );
		// LEFT
		glNormal3f( -1.f, 0.f, 0.f );
		glTexCoord2f( 0.f, 0.f );
		glArrayElement( 4 );
		glTexCoord2f( 1.f, 0.f );
		glArrayElement( 0 );
		glTexCoord2f( 1.f, 1.f );
		glArrayElement( 3 );
		glTexCoord2f( 0.f, 1.f );
		glArrayElement( 7 );
		// RIGHT
		glNormal3f( 1.f, 0.f, 0.f );
		glTexCoord2f( 0.f, 0.f );
		glArrayElement( 1 );
		glTexCoord2f( 1.f, 0.f );
		glArrayElement( 5 );
		glTexCoord2f( 1.f, 1.f );
		glArrayElement( 6 );
		glTexCoord2f( 0.f, 1.f );
		glArrayElement( 2 );
		// BOTTOM
		glNormal3f( 0.f, -1.f, 0.f );
		glTexCoord2f( 0.f, 0.f );
		glArrayElement( 4 );
		glTexCoord2f( 1.f, 0.f );
		glArrayElement( 5 );
		glTexCoord2f( 1.f, 1.f );
		glArrayElement( 1 );
		glTexCoord2f( 0.f, 1.f );
		glArrayElement( 0 );
		// TOP
		glNormal3f( 0.f, 1.f, 0.f );
		glTexCoord2f( 0.f, 0.f );
		glArrayElement( 3 );
		glTexCoord2f( 1.f, 0.f );
		glArrayElement( 2 );
		glTexCoord2f( 1.f, 1.f );
		glArrayElement( 6 );
		glTexCoord2f( 0.f, 1.f );
		glArrayElement( 7 );
	glEnd();
	glEndList();
	return list;
}

unsigned int zglWireCubeList( float side ) {
	unsigned int list = glGenLists( 1 );
	glNewList( list, GL_COMPILE );
	float p = side / 2.f;
	float n = -side / 2.f;
	float verts[8][3] = {
		n, n, p,
		p, n, p,
		p, p, p,
		n, p, p,
		n, n, n,
		p, n, n,
		p, p, n,
		n, p, n,
	};
	glBegin( GL_LINE_LOOP );
		// FRONT
		glVertex3fv( verts[0] );
		glVertex3fv( verts[1] );
		glVertex3fv( verts[2] );
		glVertex3fv( verts[3] );
	glEnd();
	glBegin( GL_LINE_LOOP );
		// BACK
		glVertex3fv( verts[7] );
		glVertex3fv( verts[6] );
		glVertex3fv( verts[5] );
		glVertex3fv( verts[4] );
	glEnd();
	glBegin( GL_LINES );
		// FRONT TO BACK
		glVertex3fv( verts[0] );
		glVertex3fv( verts[4] );
		
		glVertex3fv( verts[1] );
		glVertex3fv( verts[5] );

		glVertex3fv( verts[2] );
		glVertex3fv( verts[6] );

		glVertex3fv( verts[3] );
		glVertex3fv( verts[7] );
	glEnd();
	glEndList();
	return list;
}


unsigned int zglDiscList( int steps, float size ) {
	unsigned int list = glGenLists( 1 );
	glNewList( list, GL_COMPILE );
		glBegin( GL_TRIANGLE_FAN );
		glNormal3f( 0.f, 0.f, 1.f );
		glTexCoord2f( 0.5f, 0.5f );
		glVertex2f( 0.f, 0.f );
		float t = 0.f;
		float tStep = 2.f * 3.1415926539f / steps;
		for( int i=0; i<=steps; i++, t += tStep ) {
			float c = cosf( t );
			float s = sinf( t );
			glTexCoord2f( 0.5f+c/2.f, 0.5f+s/2.f );
			glVertex2f( size*c, size*s );
		}
		glEnd();
	glEndList();
	return list;
}

// Helper debug drawing functions.
//===========================================================================

void zglDrawPlanarAxis( float scale ) {
	glBegin( GL_QUADS );
	glColor3ub( 128, 128, 128 );
	glVertex3f( 0.f, 0.f, 0.f );
	glColor3ub( 255, 128, 128 );
	glVertex3f( scale, 0.f, 0.f );
	glColor3ub( 255, 255, 128 );
	glVertex3f( scale, scale, 0.f );
	glColor3ub( 128, 255, 128 );
	glVertex3f( 0.f, scale, 0.f );

	glColor3ub( 128, 128, 128 );
	glVertex3f( 0.f, 0.f, 0.f );
	glColor3ub( 255, 128, 128 );
	glVertex3f( scale, 0.f, 0.f );
	glColor3ub( 255, 128, 255 );
	glVertex3f( scale, 0.f, scale );
	glColor3ub( 128, 128, 255 );
	glVertex3f( 0.f, 0.f, scale );


	glColor3ub( 128, 128, 128 );
	glVertex3f( 0.f, 0.f, 0.f );
	glColor3ub( 128, 255, 128 );
	glVertex3f( 0.f, scale, 0.f );
	glColor3ub( 128, 255, 255 );
	glVertex3f( 0.f, scale, scale );
	glColor3ub( 128, 128, 255 );
	glVertex3f( 0.f, 0.f, scale );

	glEnd();

//	glPointSize( 1.f );
//	glColor3ub( 0, 0, 0 );
//	glBegin( GL_POINTS );
//	glVertex3f( scale, scale, scale );
//	glEnd();
}

void zglDrawStandardAxis( float scale ) {
	glPushAttrib( GL_CURRENT_BIT );
	glBegin( GL_LINES );
		glColor3ub( 255, 0, 0 );
		glVertex3fv( FVec3::Origin );
		glVertex3f( scale, 0.f, 0.f );
		glColor3ub( 0, 255, 0 );
		glVertex3fv( FVec3::Origin );
		glVertex3f( 0.f, scale, 0.f );
		glColor3ub( 0, 0, 255 );
		glVertex3fv( FVec3::Origin );
		glVertex3f( 0.f, 0.f, scale );
	glEnd();
	glPopAttrib();
}

void zglDrawVector(
	float oriX, float oriY, float oriZ,
	float dirX, float dirY, float dirZ,
	float arrowSize, float arrowAngleDeg)
{
	FVec3 origin(oriX, oriY, oriZ);
	FVec3 direction(dirX, dirY, dirZ);

	glBegin(GL_LINES);
	glVertex3fv(origin);
	origin.add(direction);
	glVertex3fv(origin);

	if (arrowSize != 0)
	{
		// origin is now the end of the normal.
		FMat3 rotMat0 = rotate3D_3x3(FVec3::ZAxis, 3.1415926539f*2.f*(arrowAngleDeg/360.f));

		if( arrowSize < 0.f ) {
			direction.normalize();
			arrowSize *= -1.f;
		}
		direction.mul(-arrowSize);
		FVec3 arrowOff0 = rotMat0.mul(direction);
		rotMat0.transpose();
		FVec3 arrowOff1 = rotMat0.mul(direction);

		arrowOff0.add(origin);
		arrowOff1.add(origin);

		glVertex3fv(origin);
		glVertex3fv(arrowOff0);

		glVertex3fv(origin);
		glVertex3fv(arrowOff1);
	}

	glEnd();
}

void zglDrawVector(
	double oriX, double oriY, double oriZ,
	double dirX, double dirY, double dirZ,
	double arrowSize, double arrowAngleDeg)
{
	DVec3 origin(oriX, oriY, oriZ);
	DVec3 direction(dirX, dirY, dirZ);

	glBegin(GL_LINES);
	glVertex3dv(origin);
	origin.add(direction);
	glVertex3dv(origin);

	if (arrowSize != 0)
	{
		// origin is now the end of the normal.
		DMat3 rotMat0 = rotate3D_3x3(DVec3::ZAxis, 3.1415926539*2.0*(arrowAngleDeg/360.0));

		if( arrowSize < 0.f ) {
			direction.normalize();
			arrowSize *= -1.f;
		}
		direction.mul(-arrowSize);
		DVec3 arrowOff0 = rotMat0.mul(direction);
		rotMat0.transpose();
		DVec3 arrowOff1 = rotMat0.mul(direction);

		arrowOff0.add(origin);
		arrowOff1.add(origin);

		glVertex3dv(origin);
		glVertex3dv(arrowOff0);

		glVertex3dv(origin);
		glVertex3dv(arrowOff1);
	}

	glEnd();
}

void zglDrawVectorF3(
	float *ori,
	float *dir,
	float arrowSize, float arrowAngleDeg)
{
	FVec3 origin(ori[0], ori[1], ori[2]);
	FVec3 direction(dir[0], dir[1], dir[2]);

	glBegin(GL_LINES);
	glVertex3fv(origin);
	origin.add(direction);
	glVertex3fv(origin);

	if (arrowSize != 0)
	{
		// origin is now the end of the normal.
		FMat3 rotMat0 = rotate3D_3x3(FVec3::ZAxis, 3.1415926539f*2.f*(arrowAngleDeg/360.f));

		if( arrowSize < 0.f ) {
			direction.normalize();
			arrowSize *= -1.f;
		}
		direction.mul(-arrowSize);
		FVec3 arrowOff0 = rotMat0.mul(direction);
		rotMat0.transpose();
		FVec3 arrowOff1 = rotMat0.mul(direction);

		arrowOff0.add(origin);
		arrowOff1.add(origin);

		glVertex3fv(origin);
		glVertex3fv(arrowOff0);

		glVertex3fv(origin);
		glVertex3fv(arrowOff1);
	}

	glEnd();
}

void zglDrawVectorF2(
	float *ori,
	float *dir,
	float arrowSize, float arrowAngleDeg)
{
	FVec3 origin(ori[0], ori[1], 0.f );
	FVec3 direction(dir[0], dir[1], 0.f );

	glBegin(GL_LINES);
	glVertex3fv(origin);
	origin.add(direction);
	glVertex3fv(origin);

	if (arrowSize != 0)
	{
		// origin is now the end of the normal.
		FMat3 rotMat0 = rotate3D_3x3(FVec3::ZAxis, 3.1415926539f*2.f*(arrowAngleDeg/360.f));

		if( arrowSize < 0.f ) {
			direction.normalize();
			arrowSize *= -1.f;
		}
		direction.mul(-arrowSize);
		FVec3 arrowOff0 = rotMat0.mul(direction);
		rotMat0.transpose();
		FVec3 arrowOff1 = rotMat0.mul(direction);

		arrowOff0.add(origin);
		arrowOff1.add(origin);

		glVertex3fv(origin);
		glVertex3fv(arrowOff0);

		glVertex3fv(origin);
		glVertex3fv(arrowOff1);
	}

	glEnd();
}

void zglDrawVectorF3Dst( float *ori, float *dst, float arrowSize, float arrowAngleDeg ) {
	FVec3 origin(ori[0], ori[1], ori[2]);
	FVec3 destination(dst[0], dst[1], dst[2]);
	FVec3 direction = destination;
	direction.sub( origin );

	glBegin(GL_LINES);
	glVertex3fv(ori);
	glVertex3fv(dst);

	if( arrowSize != 0 ) {
		// origin is now the end of the normal.
		FMat3 rotMat0 = rotate3D_3x3(FVec3::ZAxis, 3.1415926539f*2.f*(arrowAngleDeg/360.f));

		if( arrowSize < 0.f ) {
			direction.normalize();
			arrowSize *= -1.f;
		}
		direction.mul(-arrowSize);
		FVec3 arrowOff0 = rotMat0.mul(direction);
		rotMat0.transpose();
		FVec3 arrowOff1 = rotMat0.mul(direction);

		arrowOff0.add(destination);
		arrowOff1.add(destination);

		glVertex3fv(destination);
		glVertex3fv(arrowOff0);

		glVertex3fv(destination);
		glVertex3fv(arrowOff1);
	}

	glEnd();
}

void zglDrawVectorF2Dst( float *ori, float *dst, float arrowSize, float arrowAngleDeg ) {
	FVec3 origin(ori[0], ori[1], 0.f );
	FVec3 destination(dst[0], dst[1], 0.f );
	FVec3 direction = destination;
	direction.sub( origin );

	glBegin(GL_LINES);
	glVertex2fv(ori);
	glVertex2fv(dst);

	if( arrowSize != 0 ) {
		// origin is now the end of the normal.
		FMat3 rotMat0 = rotate3D_3x3(FVec3::ZAxis, 3.1415926539f*2.f*(arrowAngleDeg/360.f));

		if( arrowSize < 0.f ) {
			direction.normalize();
			arrowSize *= -1.f;
		}
		direction.mul(-arrowSize);
		FVec3 arrowOff0 = rotMat0.mul(direction);
		rotMat0.transpose();
		FVec3 arrowOff1 = rotMat0.mul(direction);

		arrowOff0.add(destination);
		arrowOff1.add(destination);

		glVertex3fv(destination);
		glVertex3fv(arrowOff0);

		glVertex3fv(destination);
		glVertex3fv(arrowOff1);
	}

	glEnd();
}

void zglDrawVectorDst( float oriX, float oriY, float oriZ, float dstX, float dstY, float dstZ, float arrowSize, float arrowAngleDeg ) {
	float ori[3] = { oriX, oriY, oriZ };
	float dst[3] = { dstX, dstY, dstZ };
	zglDrawVectorF3Dst( ori, dst, arrowSize, arrowAngleDeg );
}

void zglArrow( float x0, float y0, float x1, float y1, float headSize0, float headSize1, float thickness ) {
	FVec2 p0( x0, y0 );
	FVec2 p1( x1, y1 );
	FVec2 dir = p1;
	dir.sub( p0 );

	FVec2 perp = p0;
	perp.sub( p1 );
	perp.perp();
	perp.normalize();

	float _x0 = x0;
	float _y0 = y0;
	float _x1 = x1;
	float _y1 = y1;

	float headLen0 = headSize0 * thickness;
	float headLen1 = headSize1 * thickness;

	float len = dir.mag();

	if( thickness < 0.f ) {
		thickness = len * -thickness;
	}

	if( len < 1.1f * (headLen0 + headLen1) ) {
		thickness = 0.9f * len / (headSize0 + headSize1);
	}

	FVec2 perpThick = perp;
	perpThick.mul( thickness );

	if( headSize0 >= 1.f ) {
		glBegin( GL_TRIANGLE_FAN );
			headSize0 *= thickness;
			FVec2 p0to1 = p1;
			p0to1.sub( p0 );
			p0to1.normalize();
			p0to1.mul( headSize0 );
			_x0 += p0to1.x;
			_y0 += p0to1.y;
			glVertex2f( x0, y0 );
			glVertex2f( x0+p0to1.x+perp.x*headSize0, y0+p0to1.y+perp.y*headSize0 );
			glVertex2f( _x0 + perpThick.x, _y0 + perpThick.y );
			glVertex2f( _x0 - perpThick.x, _y0 - perpThick.y );
			glVertex2f( x0+p0to1.x-perp.x*headSize0, y0+p0to1.y-perp.y*headSize0 );
		glEnd();
	}
	if( headSize1 >= 1.f ) {
		glBegin( GL_TRIANGLE_FAN );
			headSize1 *= thickness;
			FVec2 p1to0 = p0;
			p1to0.sub( p1 );
			p1to0.normalize();
			p1to0.mul( headSize1 );
			_x1 += p1to0.x;
			_y1 += p1to0.y;
			glVertex2f( x1, y1 );
			glVertex2f( x1+p1to0.x-perp.x*headSize1, y1+p1to0.y-perp.y*headSize1 );
			glVertex2f( _x1 - perpThick.x, _y1 - perpThick.y );
			glVertex2f( _x1 + perpThick.x, _y1 + perpThick.y );
			glVertex2f( x1+p1to0.x+perp.x*headSize1, y1+p1to0.y+perp.y*headSize1 );
		glEnd();
	}

	glBegin( GL_QUADS );
		glVertex2f( _x1 + perpThick.x, _y1 + perpThick.y );
		glVertex2f( _x1 - perpThick.x, _y1 - perpThick.y );
		glVertex2f( _x0 - perpThick.x, _y0 - perpThick.y );
		glVertex2f( _x0 + perpThick.x, _y0 + perpThick.y );
	glEnd();
}

void zglCylinder( float radius0, float radius1, float height, int divs, int caps ) {
	glBegin( GL_QUAD_STRIP );
	float t = 0.f, tStep = 2.f * 3.1415926539f / divs;
	float base = 0.f, top = height;
	int i;
	for( i=0; i<=divs; i++, t += tStep ) {
		float c = cosf(t);
		float s = sinf(t);
		glNormal3f( c, s, 0.f );
		glVertex3f( radius0 * c, radius0 * s, base );
		glVertex3f( radius1 * c, radius1 * s, top );
	}
	glEnd();

	if( caps ) {
		glBegin( GL_TRIANGLE_FAN );
		glNormal3fv( FVec3::ZAxisMinus );
		glVertex3f( 0.f, 0.f, base );
		t = 0.f;
		int i;
		for( i=0; i<=divs; i++, t += tStep ) {
			float c = cosf(t);
			float s = sinf(t);
			glVertex3f( radius0 * c, radius0 * s, base );
		}
		glEnd();

		glBegin( GL_TRIANGLE_FAN );
		glNormal3fv( FVec3::ZAxisMinus );
		glVertex3f( 0.f, 0.f, top );
		t = 0.f;
		for( i=0; i<=divs; i++, t += tStep ) {
			float c = cosf(t);
			float s = sinf(t);
			glVertex3f( radius1 * c, radius1 * s, top );
		}
		glEnd();
	}
}

void zglInvCylinder( float radius0, float radius1, float height, int divs, int caps ) {
	glBegin( GL_QUAD_STRIP );
	float t = 0.f, tStep = 2.f * 3.1415926539f / divs;
	float base = 0.f, top = height;
	int i;
	for( i=0; i<=divs; i++, t += tStep ) {
		float c = cosf(t);
		float s = sinf(t);
		glNormal3f( -c, -s, 0.f );
		glVertex3f( radius0 * c, radius0 * s, base );
		glVertex3f( radius1 * c, radius1 * s, top );
	}
	glEnd();

	if( caps ) {
		glBegin( GL_TRIANGLE_FAN );
		glNormal3fv( FVec3::ZAxis);
		glVertex3f( 0.f, 0.f, base );
		t = 0.f;
		int i;
		for( i=0; i<=divs; i++, t += tStep ) {
			float c = cosf(t);
			float s = sinf(t);
			glVertex3f( radius0 * c, radius0 * s, base );
		}
		glEnd();

		glBegin( GL_TRIANGLE_FAN );
		glNormal3fv( FVec3::ZAxis );
		glVertex3f( 0.f, 0.f, top );
		t = 0.f;
		for( i=0; i<=divs; i++, t += tStep ) {
			float c = cosf(t);
			float s = sinf(t);
			glVertex3f( radius1 * c, radius1 * s, top );
		}
		glEnd();
	}
}

void zglCube( float dim ) {
	dim /= 2.f;
	glBegin( GL_QUADS );
		// front
		glNormal3fv( FVec3::ZAxis );
		glVertex3f( -dim, -dim, -dim );
		glVertex3f(  dim, -dim, -dim );
		glVertex3f(  dim,  dim, -dim );
		glVertex3f( -dim,  dim, -dim );

		// back
		glNormal3fv( FVec3::ZAxisMinus );
		glVertex3f(  dim, -dim,  dim );
		glVertex3f( -dim, -dim,  dim );
		glVertex3f( -dim,  dim,  dim );
		glVertex3f(  dim,  dim,  dim );

		// left
		glNormal3fv( FVec3::XAxisMinus );
		glVertex3f( -dim, -dim, -dim );
		glVertex3f( -dim, -dim,  dim );
		glVertex3f( -dim,  dim,  dim );
		glVertex3f( -dim,  dim, -dim );

		// right
		glNormal3fv( FVec3::XAxis );
		glVertex3f(  dim, -dim,  dim );
		glVertex3f(  dim, -dim, -dim );
		glVertex3f(  dim,  dim, -dim );
		glVertex3f(  dim,  dim,  dim );

		// top
		glNormal3fv( FVec3::YAxis );
		glVertex3f(  dim,  dim,  dim );
		glVertex3f(  dim,  dim, -dim );
		glVertex3f( -dim,  dim, -dim );
		glVertex3f( -dim,  dim,  dim );

		// bottom
		glNormal3fv( FVec3::YAxisMinus );
		glVertex3f( -dim, -dim,  dim );
		glVertex3f( -dim, -dim, -dim );
		glVertex3f(  dim, -dim, -dim );
		glVertex3f(  dim, -dim,  dim );
	glEnd();
}

void zglRectPrism( float dimX, float dimY, float dimZ ) {
	dimX /= 2.f;
	dimY /= 2.f;
	dimZ /= 2.f;
	glBegin( GL_QUADS );
		// front
		glNormal3fv( FVec3::ZAxis );
		glVertex3f( -dimX, -dimY, -dimZ );
		glVertex3f(  dimX, -dimY, -dimZ );
		glVertex3f(  dimX,  dimY, -dimZ );
		glVertex3f( -dimX,  dimY, -dimZ );

		// back
		glNormal3fv( FVec3::ZAxisMinus );
		glVertex3f(  dimX, -dimY,  dimZ );
		glVertex3f( -dimX, -dimY,  dimZ );
		glVertex3f( -dimX,  dimY,  dimZ );
		glVertex3f(  dimX,  dimY,  dimZ );

		// left
		glNormal3fv( FVec3::XAxisMinus );
		glVertex3f( -dimX, -dimY, -dimZ );
		glVertex3f( -dimX, -dimY,  dimZ );
		glVertex3f( -dimX,  dimY,  dimZ );
		glVertex3f( -dimX,  dimY, -dimZ );

		// right
		glNormal3fv( FVec3::XAxis );
		glVertex3f(  dimX, -dimY,  dimZ );
		glVertex3f(  dimX, -dimY, -dimZ );
		glVertex3f(  dimX,  dimY, -dimZ );
		glVertex3f(  dimX,  dimY,  dimZ );

		// top
		glNormal3fv( FVec3::YAxis );
		glVertex3f(  dimX,  dimY,  dimZ );
		glVertex3f(  dimX,  dimY, -dimZ );
		glVertex3f( -dimX,  dimY, -dimZ );
		glVertex3f( -dimX,  dimY,  dimZ );

		// bottom
		glNormal3fv( FVec3::YAxisMinus );
		glVertex3f( -dimX, -dimY,  dimZ );
		glVertex3f( -dimX, -dimY, -dimZ );
		glVertex3f(  dimX, -dimY, -dimZ );
		glVertex3f(  dimX, -dimY,  dimZ );
	glEnd();
}


void zglRectPrism( float *min, float *max ) {
	FVec3 p1( min );
	FVec3 p2( max );
	glBegin( GL_QUADS );
		// front
		glNormal3fv( FVec3::ZAxis );
		glVertex3f( p1.x, p1.y, p1.z );
		glVertex3f(  p2.x, p1.y, p1.z );
		glVertex3f(  p2.x,  p2.y, p1.z );
		glVertex3f( p1.x,  p2.y, p1.z );

		// back
		glNormal3fv( FVec3::ZAxisMinus );
		glVertex3f(  p2.x, p1.y,  p2.z );
		glVertex3f( p1.x, p1.y,  p2.z );
		glVertex3f( p1.x,  p2.y,  p2.z );
		glVertex3f(  p2.x,  p2.y,  p2.z );

		// left
		glNormal3fv( FVec3::XAxisMinus );
		glVertex3f( p1.x, p1.y, p1.z );
		glVertex3f( p1.x, p1.y,  p2.z );
		glVertex3f( p1.x,  p2.y,  p2.z );
		glVertex3f( p1.x,  p2.y, p1.z );

		// right
		glNormal3fv( FVec3::XAxis );
		glVertex3f(  p2.x, p1.y,  p2.z );
		glVertex3f(  p2.x, p1.y, p1.z );
		glVertex3f(  p2.x,  p2.y, p1.z );
		glVertex3f(  p2.x,  p2.y,  p2.z );

		// top
		glNormal3fv( FVec3::YAxis );
		glVertex3f(  p2.x,  p2.y,  p2.z );
		glVertex3f(  p2.x,  p2.y, p1.z );
		glVertex3f( p1.x,  p2.y, p1.z );
		glVertex3f( p1.x,  p2.y,  p2.z );

		// bottom
		glNormal3fv( FVec3::YAxisMinus );
		glVertex3f( p1.x, p1.y,  p2.z );
		glVertex3f( p1.x, p1.y, p1.z );
		glVertex3f(  p2.x, p1.y, p1.z );
		glVertex3f(  p2.x, p1.y,  p2.z );
	glEnd();
}

void zglColorI( int color ) {
	if( (color&0x000000FF) != 0xFF ) {
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	}
	else {
		glDisable( GL_BLEND );
	}
	glColor4ub( (color&0xFF000000)>>24, (color&0x00FF0000)>>16, (color&0x0000FF00)>>8, color&0x000000FF );
}

void zglDrawPlaneGridXY( float x0, float y0, float x1, float y1, float spacing ) {
	glBegin( GL_LINES );
		for( float x=x0; x<=x1; x+=spacing ) {
			glVertex3f( x, y0, 0.f );
			glVertex3f( x, y1, 0.f );
		}
		glVertex3f( x1, y0, 0.f );
		glVertex3f( x1, y1, 0.f );
		for( float y=y0; y<=y1; y+=spacing ) {
			glVertex3f( x0, y, 0.f );
			glVertex3f( x1, y, 0.f );
		}
		glVertex3f( x0, y1, 0.f );
		glVertex3f( x1, y1, 0.f );
	glEnd();
}

void zglDrawPlaneGridXZ( float x0, float z0, float x1, float z1, float spacing ) {
	glBegin( GL_LINES );
		for( float x=x0; x<=x1; x+=spacing ) {
			glVertex3f( x, 0.f, z0 );
			glVertex3f( x, 0.f, z1 );
		}
		glVertex3f( x1, 0.f, z0 );
		glVertex3f( x1, 0.f, z1 );
		for( float z=z0; z<=z1; z+=spacing ) {
			glVertex3f( x0, 0.f, z );
			glVertex3f( x1, 0.f, z );
		}
		glVertex3f( x0, 0.f, z1 );
		glVertex3f( x1, 0.f, z1 );
	glEnd();
}

/*
void zglDrawTwoColorGridXY( float x0, float y0, float x1, float y1, float majorSpacing, float minorSpacing, float majorR, float majorG, float majorB, float minorR, float minorG, float minorB ) {
	glBegin( GL_LINES );
		for( float x=x0; x<=x1; x+=spacing ) {
			glVertex3f( x, y0, 0.f );
			glVertex3f( x, y1, 0.f );
		}
		glVertex3f( x1, y0, 0.f );
		glVertex3f( x1, y1, 0.f );
		for( float y=y0; y<=y1; y+=spacing ) {
			glVertex3f( x0, y, 0.f );
			glVertex3f( x1, y, 0.f );
		}
		glVertex3f( x0, y1, 0.f );
		glVertex3f( x1, y1, 0.f );
	glEnd();
}
*/

void zglCalcShadowMatrix (float *outMat, float *planeNormal, float *pointOnPlane, float *lightPos ) {
	// FIND the last coefficient by back substitutions
	float planeNormalW = - ((planeNormal[0]*pointOnPlane[0]) + (planeNormal[1]*pointOnPlane[1]) + (planeNormal[2]*pointOnPlane[2]));

	// DOT product of plane and light position
	float dot =
		  planeNormal[0] * lightPos[0]
		+ planeNormal[1] * lightPos[1]
		+ planeNormal[2] * lightPos[2]
		+ planeNormalW * 1.f/*lightPos[3]*/
	;

	float destMat[4][4];
	destMat[0][0] = dot  - lightPos[0] * planeNormal[0];
	destMat[1][0] = 0.0f - lightPos[0] * planeNormal[1];
	destMat[2][0] = 0.0f - lightPos[0] * planeNormal[2];
	destMat[3][0] = 0.0f - lightPos[0] * planeNormalW;
	destMat[0][1] = 0.0f - lightPos[1] * planeNormal[0];
	destMat[1][1] = dot  - lightPos[1] * planeNormal[1];
	destMat[2][1] = 0.0f - lightPos[1] * planeNormal[2];
	destMat[3][1] = 0.0f - lightPos[1] * planeNormalW;
	destMat[0][2] = 0.0f - lightPos[2] * planeNormal[0];
	destMat[1][2] = 0.0f - lightPos[2] * planeNormal[1];
	destMat[2][2] = dot  - lightPos[2] * planeNormal[2];
	destMat[3][2] = 0.0f - lightPos[2] * planeNormalW;
	destMat[0][3] = 0.0f - 1.f/*lightPos[3]*/ * planeNormal[0];
	destMat[1][3] = 0.0f - 1.f/*lightPos[3]*/ * planeNormal[1];
	destMat[2][3] = 0.0f - 1.f/*lightPos[3]*/ * planeNormal[2];
	destMat[3][3] = dot  - 1.f/*lightPos[3]*/ * planeNormalW;

	memcpy(outMat, destMat, sizeof(destMat));
}

void zglShadowMatrix( float *planeNormal, float *pointOnPlane, float *lightPos ) {
	float destMat[4][4];
	zglCalcShadowMatrix ((float *) destMat, planeNormal, pointOnPlane, lightPos);
	glMultMatrixf( (const float *)destMat );
}

void zglProjectScreenCoordsOnWorldPlane( float screen[2], float pointOnPlane[3], float planeNorm[3], float result[3] ) {
	int viewport[4];
	double modelMatrix[16], projMatrix[16];
	glGetDoublev( GL_MODELVIEW_MATRIX, modelMatrix );
	glGetDoublev( GL_PROJECTION_MATRIX, projMatrix );
	glGetIntegerv( GL_VIEWPORT, viewport );

	double l1[3];
	double l2[3];
	gluUnProject( screen[0], screen[1], 0.0, modelMatrix, projMatrix, viewport, &l1[0], &l1[1], &l1[2] );
	gluUnProject( screen[0], screen[1], 1.0, modelMatrix, projMatrix, viewport, &l2[0], &l2[1], &l2[2] );

	// line / plane intersection
	float pointOnPlaneMinusl1[3];
	pointOnPlaneMinusl1[0] = pointOnPlane[0] - (float)l1[0];
	pointOnPlaneMinusl1[1] = pointOnPlane[1] - (float)l1[1];
	pointOnPlaneMinusl1[2] = pointOnPlane[2] - (float)l1[2];

	float lineDirection[3];
	lineDirection[0] = (float)l2[0] - (float)l1[0];
	lineDirection[1] = (float)l2[1] - (float)l1[1];
	lineDirection[2] = (float)l2[2] - (float)l1[2];

	float numer = planeNorm[0]*pointOnPlaneMinusl1[0] + planeNorm[1]*pointOnPlaneMinusl1[1] + planeNorm[2]*pointOnPlaneMinusl1[2];
	float denom = planeNorm[0]*lineDirection[0] + planeNorm[1]*lineDirection[1] + planeNorm[2]*lineDirection[2];

	// @todo: denom could be zero
	assert( fabs(denom) > FLT_EPSILON );

	float ratio = numer / denom;
	lineDirection[0] *= ratio;
	lineDirection[1] *= ratio;
	lineDirection[2] *= ratio;

	lineDirection[0] += (float)l1[0];
	lineDirection[1] += (float)l1[1];
	lineDirection[2] += (float)l1[2];

	memcpy( result, lineDirection, sizeof(lineDirection) );
}

void zglProjectWorldCoordsOnScreenPlane( float world[3], float win[3] ) {
	double projMat[16];
	double modelViewMat[16];
	int viewportI[4];
	glGetIntegerv( GL_VIEWPORT, viewportI );
	glGetDoublev( GL_PROJECTION_MATRIX, projMat );
	glGetDoublev( GL_MODELVIEW_MATRIX, modelViewMat );
	double wind[3];
	gluProject( world[0], world[1], world[2], modelViewMat, projMat, viewportI, &wind[0], &wind[1], &wind[2] );
	win[0] = (float)wind[0];
	win[1] = (float)wind[1];
	win[2] = (float)wind[2];
}

void zglProjectWorldCoordsVecOnScreenPlane( float *world, float *win, int count ) {
	double projMat[16];
	double modelViewMat[16];
	int viewportI[4];
	glGetIntegerv( GL_VIEWPORT, viewportI );
	glGetDoublev( GL_PROJECTION_MATRIX, projMat );
	glGetDoublev( GL_MODELVIEW_MATRIX, modelViewMat );
	double wind[3];
	for( int i=0; i<count; i++ ) {
		gluProject( world[0], world[1], world[2], modelViewMat, projMat, viewportI, &wind[0], &wind[1], &wind[2] );
		win[0] = (float)wind[0];
		win[1] = (float)wind[1];
		win[2] = (float)wind[2];
		win   += 3;
		world += 3;
	}
}


void zglProjectWorldCoordsOnScreenPlane( double world[3], double win[3] ) {
	// Note: duplicate of above, but with native double arguments; I only leave the
	// above to prevent breaking/generating warnings in other's code; remove at will.  (tfb)
	double projMat[16];
	double modelViewMat[16];
	int viewportI[4];
	glGetIntegerv( GL_VIEWPORT, viewportI );
	glGetDoublev( GL_PROJECTION_MATRIX, projMat );
	glGetDoublev( GL_MODELVIEW_MATRIX, modelViewMat );
	double wind[3];
	gluProject( world[0], world[1], world[2], modelViewMat, projMat, viewportI, &wind[0], &wind[1], &wind[2] );
	win[0] = wind[0];
	win[1] = wind[1];
	win[2] = wind[2];
}

char *zglMatrixToString( int whichMatrix ) {
	static char buffer[256];
	buffer[0] = 0;

	switch( whichMatrix ) {
		case GL_MODELVIEW_MATRIX:
		case GL_PROJECTION_MATRIX:
		case GL_TEXTURE_MATRIX:
			float mat[16];
			char tmp[64];
			glGetFloatv( whichMatrix, mat );
			sprintf( tmp, "%5.2f\t%5.2f\t%5.2f\t%5.2f\n", mat[0], mat[4], mat[8], mat[12] );
			strcat( buffer, tmp );	
			sprintf( tmp, "%5.2f\t%5.2f\t%5.2f\t%5.2f\n", mat[1], mat[5], mat[9], mat[13] );
			strcat( buffer, tmp );	
			sprintf( tmp, "%5.2f\t%5.2f\t%5.2f\t%5.2f\n", mat[2], mat[6], mat[10], mat[14] );
			strcat( buffer, tmp );	
			sprintf( tmp, "%5.2f\t%5.2f\t%5.2f\t%5.2f\n", mat[3], mat[7], mat[11], mat[15] );
			strcat( buffer, tmp );	
			break;

		default:
			strcpy( buffer, "invalid matrix type\n" );
	}
	return buffer;
}

void zglProjectionIdentity() {
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glMatrixMode( GL_MODELVIEW );
}

#define zglSTACK_SIZE (32)
static int zglStackTop = 0;
static float zglModelViewStack[zglSTACK_SIZE][16];
static float zglProjectionStack[zglSTACK_SIZE][16];

void zglPushBoth() {
	assert( zglStackTop < zglSTACK_SIZE );
	glGetFloatv( GL_MODELVIEW_MATRIX, zglModelViewStack[zglStackTop] );
	glGetFloatv( GL_PROJECTION_MATRIX, zglProjectionStack[zglStackTop] );
	zglStackTop++;
}

void zglPopBoth() {
	assert( zglStackTop > 0 );
	zglStackTop--;

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glMultMatrixf( zglProjectionStack[zglStackTop] );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glMultMatrixf( zglModelViewStack[zglStackTop] );
}

void zglQuadPatch( float l, float b, float w, float h, int divs ) {
	// Draws a quad patch on the xy plane
	float xStep = w / (float)divs;
	float yStep = h / (float)divs;

	divs ++;
	FVec3 *verts = (FVec3 *)malloc( divs * divs * sizeof(FVec3) );

	float yf = b;
	int y;
	for( y=0; y<divs; y++ ) {
		float xf = l;
		for( int x=0; x<divs; x++ ) {
			verts[y*divs+x] = FVec3( xf, yf, 0.f );
			xf += xStep;
		}
		yf += yStep;
	}

	glPushAttrib( GL_ENABLE_BIT );
	glEnable( GL_VERTEX_ARRAY );
	glVertexPointer( 3, GL_FLOAT, 0, verts );

	glNormal3f( 0.f, 0.f, 1.f );
	glBegin( GL_QUADS );
	for( y=0; y<divs-1; y++ ) {
		for( int x=0; x<divs-1; x++ ) {
			glArrayElement( (y+0)*divs + (x+0) );
			glArrayElement( (y+0)*divs + (x+1) );
			glArrayElement( (y+1)*divs + (x+1) );
			glArrayElement( (y+1)*divs + (x+0) );
		}
	}
	glEnd();

	glPopAttrib();
	free( verts );
}	

void zglMatrixFirstQuadrantAspectCorrect() {
	glLoadIdentity();

	float viewport[4];
	glGetFloatv( GL_VIEWPORT, viewport );

	// MAKE the lower left 0, 0 and scale = 1.0 = 1pixel
	glTranslatef( -1.0f, -1.0f, 0.f );
	if( viewport[2] > viewport[3] ) {
		// Width is dominant
		if( viewport[2] > 0 && viewport[3] > 0 ) {
			glScalef( 2.0f * viewport[3] / viewport[2], 2.f, 1.f );
		}
	}
	else {
		// Height is dominant
		if( viewport[2] > 0 && viewport[3] > 0 ) {
			glScalef( 2.f, 2.0f * viewport[2] / viewport[3], 1.f );
		}
	}
}

void zglPixelMatrixFirstQuadrant() {
	glLoadIdentity();

	int viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );

	// MAKE the lower left 0, 0 and scale = 1.0 = 1pixel
	glTranslatef( -1.0f, -1.0f, 0.f );
	if( viewport[2] > 0 && viewport[3] > 0 ) {
		glScalef( 2.0f/(float)viewport[2], 2.0f/(float)viewport[3], 1.f );
	}
}

void zglPixelMatrixInvertedFirstQuadrant() {
	glLoadIdentity();

	int viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );

	// MAKE the upper left 0, 0 and scale = 1.0 = 1pixel
	glTranslatef( -1.0f, 1.0f, 0.f );
	if( viewport[2] > 0 && viewport[3] > 0 ) {
		glScalef( 2.0f/(float)viewport[2], -2.0f/(float)viewport[3], 1.f );
	}
}

void zglMatrixFirstQuadrant() {
	glLoadIdentity();
	glTranslatef( -1.0, -1.0, 0.0 );
	glScalef( 2.0, 2.0, 1.0 );
}

void zglMatrixInvertedFirstQuadrant() {
	glLoadIdentity();
	glTranslatef( -1.0, +1.0, 0.0 );
	glScalef( 2.0, -2.0, 1.0 );
}

void zglTexRect( float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1 ) {
	glBegin( GL_QUADS );
		glTexCoord2f( u0, v0 );
		glVertex2f( x0, y0 );
		glTexCoord2f( u1, v0 );
		glVertex2f( x1, y0 );
		glTexCoord2f( u1, v1 );
		glVertex2f( x1, y1 );
		glTexCoord2f( u0, v1 );
		glVertex2f( x0, y1 );
	glEnd();
}

void zglSetAlpha( float alpha ) {
	float color[4];
	glGetFloatv( GL_CURRENT_COLOR, color );
	color[3] = alpha;
	glColor4fv( color );
}

void zglScaleAlpha( float scale ) {
	float color[4];
	glGetFloatv( GL_CURRENT_COLOR, color );
	color[3] *= scale;
	glColor4fv( color );
}

void zglPixelInWorldCoords( float &onePixelX, float &onePixelY ) {
	DMat4 projMat;
	DMat4 modelViewMat;
	int viewportI[4];
	glGetIntegerv( GL_VIEWPORT, viewportI );
	glGetDoublev( GL_PROJECTION_MATRIX, projMat );
	glGetDoublev( GL_MODELVIEW_MATRIX, modelViewMat );
	DVec3 p0, p1;
	gluUnProject( 0.0, 0.0, 0.0, modelViewMat, projMat, viewportI, &p0.x, &p0.y, &p0.z );
	gluUnProject( 1.0, 1.0, 0.0, modelViewMat, projMat, viewportI, &p1.x, &p1.y, &p1.z );
	onePixelX = (float)p1.x - (float)p0.x;
	onePixelY = (float)p1.y - (float)p0.y;
}

ZGLTex::ZGLTex( int _alloc ) {
	tex = 0;
	if( _alloc ) {
		tex = zglGenTexture();
	}
}

ZGLTex::~ZGLTex() {
	if( tex ) {
		glDeleteTextures( 1, &tex );
	}
}

void ZGLTex::clear() {
	if( tex ) {
		glDeleteTextures( 1, &tex );
		tex = 0;
	}
}

void ZGLTex::alloc() {
	tex = zglGenTexture();
}
