// @ZBS {
//		*MODULE_OWNER_NAME zgltools
// }

#ifndef ZGLTOOLS_H
#define ZGLTOOLS_H

// These are functions which are meant to perform common and simple tasks.
// They are not meant to be flexible or complete.
// If anything goes wrong, they assert.

int zglGenTexture();
	// calls glGenTexture to create one texture handle and
	// then tells this texture to be linear, non-wrapping, and modulated

void zglLoadTextureFromBitmapDesc( struct ZBitmapDesc *desc, int texture, int vertFlip=0 );
	// given a bitmap description, loads the image into the gl driver
	// If the desc is not a power of 2 in W & H, then it is converted

int zglGenTextureFromBitmapDesc( struct ZBitmapDesc *desc, int vertFlip=0 );
	// given a bitmap description, gens a texture and loads the image into the gl driver
	// If the desc is not a power of 2 in W & H, then it is converted

void zglMakeMipMapsFromBitmapDesc( struct ZBitmapDesc *desc );
	// Tells GL to make mip maps out of the currently bound texture with this desc
	// Usually called after a load like:
	//ZBitmapDesc desc;
	//texture = zglGenTextureRGBA( "tex1.bmp", &desc );
	//zglMipMap( &desc );

void zglLoadTextureFromZBits( struct ZBits *desc, int texture, int vertFlip=0 );
	// given a bitmap description, loads the image into the gl driver
	// If the desc is not a power of 2 in W & H, then it is converted

int zglGenTextureFromZBits( struct ZBits *desc, int vertFlip=0 );
	// given a bitmap description, gens a texture and loads the image into the gl driver
	// If the desc is not a power of 2 in W & H, then it is converted

void zglGeneralLoadTextureFromZBits( struct ZBits *desc, int texture, int glTexEnum, int glImgEnum, int vertFlip=0 );
	// given a bitmap description, loads the image into the gl driver using the given texture enumerator
	// If the desc is not a power of 2 in W & H, then it is converted

void zglMakeMipMapsFromZBits( struct ZBits *desc );
	// Tells GL to make mip maps out of the currently bound texture with this desc
	// Usually called after a load like:
	//ZBitmapDesc desc;
	//texture = zglGenTextureRGBA( "tex1.bmp", &desc );
	//zglMipMap( &desc );


void zglColorI( int color );
	// Sets the 4-color from an int.
	// If the alpha is non-zero it will also enable blending
	// otherwise it will disable blending

void zglCylinder( float radius0, float radius1, float height, int divs, int caps );
	// Draws a cylinder extruded from origin down positive z length "height"
	// The base radius (at the origin) is radius0, the other radius1
	// with divs circumfrential divisions and optional "caps" on the end
	// Normals are entered but texture coords are not.

void zglInvCylinder( float radius0, float radius1, float height, int divs, int caps );
	// Same as zglCylinder, except that the normals are inverted.

void zglCube( float dim );
	// A cube centered on the origin with length of side equal to dim

void zglRectPrism( float dimX, float dimY, float dimZ );
	// A rectangular prism centered on the origin
	
void zglRectPrism( float *p1min, float *p2max );
	// A rectangular prism defined by a min and max vectors (e.g. FVec3)

void zglProjectScreenCoordsOnWorldPlane( float screen[2], float pointOnPlane[3], float planeNorm[3], float result[3] );
	// project a screen point on a world plane.  This extracts the current
	// projection and modelview matiricies to do the work.

void zglProjectWorldCoordsOnScreenPlane( float world[3], float window[3] );
	// Given a coordinate in world coords, return the window coordinates in pixels
	// This extracts the projection and modelview matiricies to do the work.
	
void zglProjectWorldCoordsVecOnScreenPlane( float *world, float *win, int count );
	// Same as above, but project multiple coords in a single call

void zglProjectWorldCoordsOnScreenPlane( double world[3], double window[3] );
	// Given a coordinate in world coords, return the window coordinates in pixels
	// This extracts the projection and modelview matiricies to do the work.
	// This is a duplicate above with native double args as used by gl.

char *zglMatrixToString( int whichMatrix );
	// For diagnostic purposes, return a (static) print-ready formatted string illustrating
	// the contents of the given matrix.  whichMatrix is expected to be one of:
	// GL_MODELVIEW_MATRIX, GL_PROJECTION_MATRIX, GL_TEXTURE_MATRIX

void zglProjectionIdentity();
	// Set the projection matrix to identity and return the matrix model back to modelview

void zglPushBoth();
	// Push both the projection and the modelview matrix
	// Because the projection matrix stack can be quite small, an internal stack is used

void zglPopBoth();
	// Pop both the projection and the modelview matrix

void zglPixelMatrixFirstQuadrant();
	// Load the modelview matrix such that 1 unit = 1 pixel
	// And 0,0 in the lower-left

void zglPixelMatrixInvertedFirstQuadrant();
	// Load the modelview matrix such that 1 unit = 1 pixel
	// And 0,0 in the Upper-left

void zglMatrixFirstQuadrantAspectCorrect();
	// Load the modelview matrix such that lower-left = 0,0 and upper-right is 
	// set so that the larger axisd will be greater than one.

void zglMatrixFirstQuadrant();
	// Load the modelview matrix such that lower-left = 0,0 and upper-right = 1,1

void zglMatrixInvertedFirstQuadrant();
	// Load the modelview matrix such that top-left = 0,0 and lower-right = 1,1

void zglQuadPatch( float l, float b, float w, float h, int divs );
	// Draws a quad patch of the speicifed divs on the xy plane (z=0)
	// With the normal pointing out the positive z

void zglTexRect( float x0, float y0, float x1, float y1, float u0=0.f, float v0=0.f, float u1=1.f, float v1=1.f );
	// Draw a textured rectangle at z=0

float zglGetAspect();
	// Returns w / h of the viewport

void zglSetAlpha( float alpha );
	// Grabs the current color, overriding only the alpha channel

void zglScaleAlpha( float scale );
	// Grabs the current color, overriding only the alpha channel by the current value scaled by scale

void zglPixelInWorldCoords( float &onePixelX, float &onePixelY );
	// Returns the size of one pixel in world coords

// Geometry Builder
//=================================================================================

unsigned int zglWireSphereList( double radius=1.0, int segments=32 );
	// A wire sphere

unsigned int zglTexturedSphereList( double radius=1.0, int segments=32 );
	// A sphere with mercader texture coordinates

unsigned int zglWireCubeList( float side=1.f );
	// A wire cube

unsigned int zglCubeList( float side=1.f );
	// A cube with texture coords and normals

unsigned int zglCylinderList( float radius=1.f, float height = 1.f, int segments=16, int capped=1 );
	// A cube with texture coords and normals

unsigned int zglCircleList( int steps=32, float size=1.f );
	// A circle on the x/y place with centered texture and a normal facing out positive z

unsigned int zglDiscList( int steps=32, float size=1.f );
	// A disc on the x/y place with centered texture and a normal facing out positive z

// Extensions
//===========================================================================

int zglIsExtensionSupported( char *extension );

void *zglGetExtension( char *name );
	// This is here just to avoid a windows header dependency on 
	// systems which wish to use and extension pointer.

// Helper debug drawing functions.
//===========================================================================

void zglDrawStandardAxis( float scale=1.f );
void zglDrawPlanarAxis( float scale=1.f );

void zglArrow( float x0, float y0, float x1, float y1, float headSize0=0.f, float headSize1=2.f, float thickness=1.f );
	// Draw a nice looking arrow with arrow heads from x0,y0 - x1,y1
	// The headSizes are a scalar portion of the thickness, < 1 indicates no head
	// Positive thickness means absolute thickness
	// Negative thickness means relative to the length
	// In either case, if the arrow is so short that the heads would collide then it scales everything down

void zglDrawVector(
	float oriX, float oriY, float oriZ,
	float dirX, float dirY, float dirZ,
	float arrowSize = 0.2, float arrowAngleDeg = 45
);
	// If the arrowSize is positive, it is a scalar of the length
	// if the arrowSize is negative then it is an absolute length

void zglDrawVector(
	double oriX, double oriY, double oriZ,
	double dirX, double dirY, double dirZ,
	double arrowSize = 0.2, double arrowAngleDeg = 45
);

void zglDrawVectorF3( float *ori, float *dir, float arrowSize=0.2f, float arrowAngleDeg=45 );
void zglDrawVectorF2( float *ori, float *dir, float arrowSize=0.2f, float arrowAngleDeg=45 );
	// Same as above but take pointer to the vectors

void zglDrawVectorF3Dst( float *ori, float *dest, float arrowSize=0.2f, float arrowAngleDeg=45 );
void zglDrawVectorF2Dst( float *ori, float *dest, float arrowSize=0.2f, float arrowAngleDeg=45 );
	// Instead of a direction they take a destination

void zglDrawVectorDst( float oriX, float oriY, float oriZ, float dstX, float dstY, float dstZ, float arrowSize=0.2f, float arrowAngleDeg=45 );
	// Instead of a direction they take a destination

void zglDrawPlaneGridXY( float x0, float y0, float x1, float y1, float spacing );
	// Draws a grid on the X-Y plane with specific spacing
	// Does not set color.
void zglDrawPlaneGridXZ( float x0, float y0, float x1, float y1, float spacing );
	// Draws a grid on the X-Z plane with specific spacing
	// Does not set color.

void zglShadowMatrix( float *planeNormal, float *pointOnPlane, float *lightPos );
	// Mults the gl matrix for a shadow on the given plane

void zglCalcShadowMatrix (float *outMat, float *planeNormal, float *pointOnPlane, float *lightPos );
	// Calculates the gl matrix for a shadow on the given plane, storing the result into the given
	// matrix

// Texture allocators
//===========================================================================

struct ZGLTex {
	unsigned int tex;
	ZGLTex( int _alloc=1 );
	~ZGLTex();
	operator unsigned int() { if(!tex) alloc(); return tex; }
	operator int() { if(!tex) alloc(); return (int)tex; }
	void clear();
	void alloc();
};

#endif

