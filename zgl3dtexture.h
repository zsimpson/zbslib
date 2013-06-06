// @ZBS {
//		*MODULE_OWNER_NAME zgl3dtexture
// }

#ifndef ZGL3DTEXTURE_H
#define ZGL3DTEXTURE_H

// These are functions which are meant to perform common and simple tasks.
// They are not meant to be flexible or complete.
// If anything goes wrong, they assert.

int zglGenTexture3D();
	// calls glGenTexture to create one texture handle and
	// then tells this texture to be 3d, linear, and repeating.
	// It leaves the texture bound as the 3D texture

void zglLoadTexture3DFromFile( const char * filename, int texture3d );
	// given a filename and a 3D texure id, this loads the image bits and sets them into the
	// texture. Flat images are treated as having a depth of 1.

int zglGenTexture3DFromFile( const char *filename );
	// given a filename , this generates a 3D texture then loads the image bits and sets them into the
	// texture. Flat images are treated as having a depth of 1.

// Texture allocators
//===========================================================================

struct ZGLTex3D {
	unsigned int tex;
	ZGLTex3D( int _alloc=1 );
	~ZGLTex3D();
	operator unsigned int() { if(!tex) alloc(); return tex; }
	operator int() { if(!tex) alloc(); return (int)tex; }
	void clear();
	void alloc();
};

#endif

