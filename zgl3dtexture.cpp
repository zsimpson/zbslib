// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A collection of tools for dealing with 3d texture in open gl
//		}
//		*PORTABILITY win32 unix
//		*SDK_DEPENDS devil glew
//		*REQUIRED_FILES zgl3dtexture.cpp zgl3dtexture.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#include "GL/glew.h"
#include "GL/gl.h"
#include "GL/glu.h"
#include "IL/il.h"    // Used to load DDS volume textures
// STDLIB includes:
#include "assert.h"
#include "stdlib.h"
#include "string.h"
#include "float.h"
#include "math.h"
// MODULE includes:
#include "zgl3dtexture.h"
// ZBSLIB includes:
#include "ztmpstr.h"

//
// Stuff stolen from the windows headers
//
extern "C" {
__declspec(dllimport)
void
__stdcall
OutputDebugStringA(
    char * lpOutputString
    );
__declspec(dllimport)
void
__stdcall
OutputDebugStringW(
    const wchar_t *lpOutputString
    );
#ifdef UNICODE
#define OutputDebugString  OutputDebugStringW
#else
#define OutputDebugString  OutputDebugStringA
#endif // !UNICODE
}

//
// Macros
//

#define countof(x) (sizeof(x)/sizeof(x[0]))


//
// Locals
//
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

struct ILErrorDesc
{
	const char *text;
	ILenum code;
};

static const char *GetILErrorString( ILenum code )
{
#define DEF_IL_ERROR(c) {#c, c}
	static ILErrorDesc errors[] = 
	{
		DEF_IL_ERROR( IL_NO_ERROR ),
		DEF_IL_ERROR( IL_INVALID_ENUM ),
		DEF_IL_ERROR( IL_OUT_OF_MEMORY ),
		DEF_IL_ERROR( IL_FORMAT_NOT_SUPPORTED ),
		DEF_IL_ERROR( IL_INTERNAL_ERROR ),
		DEF_IL_ERROR( IL_INVALID_VALUE ),
		DEF_IL_ERROR( IL_ILLEGAL_OPERATION ),
		DEF_IL_ERROR( IL_ILLEGAL_FILE_VALUE ),
		DEF_IL_ERROR( IL_INVALID_FILE_HEADER ),
		DEF_IL_ERROR( IL_INVALID_PARAM ),
		DEF_IL_ERROR( IL_COULD_NOT_OPEN_FILE ),
		DEF_IL_ERROR( IL_INVALID_EXTENSION ),
		DEF_IL_ERROR( IL_FILE_ALREADY_EXISTS ),
		DEF_IL_ERROR( IL_OUT_FORMAT_SAME ),
		DEF_IL_ERROR( IL_STACK_OVERFLOW ),
		DEF_IL_ERROR( IL_STACK_UNDERFLOW ),
		DEF_IL_ERROR( IL_INVALID_CONVERSION ),
		DEF_IL_ERROR( IL_BAD_DIMENSIONS ),
		DEF_IL_ERROR( IL_FILE_READ_ERROR ),
		DEF_IL_ERROR( IL_FILE_WRITE_ERROR ),
		DEF_IL_ERROR( IL_LIB_GIF_ERROR ),
		DEF_IL_ERROR( IL_LIB_JPEG_ERROR ),
		DEF_IL_ERROR( IL_LIB_PNG_ERROR ),
		DEF_IL_ERROR( IL_LIB_TIFF_ERROR ),
		DEF_IL_ERROR( IL_LIB_MNG_ERROR ),
		DEF_IL_ERROR( IL_UNKNOWN_ERROR ),
	};
#undef DEF_IL_ERROR
	for (int i = 0; i < countof(errors); ++i)
	{
		if (errors[i].code == code)
		{
			return errors[i].text;
		}
	}
	return "Undefined error";
}

// error reporting
// *** Is there a zbslibs way of reporting an error?
#ifdef _WIN32
#define TRACE_ERR(text) \
	OutputDebugString (ZTmpStr ("%s(%d) : %s\n", __FILE__, __LINE__, text))
#else
#define TRACE_ERR(text)
#endif

//
// Publics
//

int zglGenTexture3D() {
	// 3D textures are only supported in version 1.2 and higher, or with an extension.
	// We'll bank on having the right version, which is likely on all current consumer hardware.
	GLuint texname = 0;
	if (GLEW_VERSION_1_2)
	{
		glGenTextures( 1, &texname );
		glBindTexture( GL_TEXTURE_3D, texname );
		glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT );
	}
	return texname;
}

extern int isPowerOf2( int a ); // in zgltools.cpp

void zglLoadTexture3DFromFile( const char * filename, int texture3d ) {
	// Make sure that the DevIL library is initialized.
	static bool devilInitialized = false;

	if (!devilInitialized)
	{
		ilInit();
		devilInitialized = true;
	}

	// Using the DevIL library, load an image from a file.
	// NOTE: We assume that the DevIL library is initialized already.
	// *** TODO: Add some detection of properly initialized DevIL.

	// Create and bind an image into which the bits will be loaded
	ILuint image;
	ilGenImages( 1, &image );
	ilBindImage( image );

	// Load the image
	if (ilLoadImage (const_cast<ILstring>(filename)) == IL_FALSE)
	{
		// We failed, so bail after reporting the error and cleaning up.
		TRACE_ERR( GetILErrorString ( ilGetError() ) );

		// Unbind and delete the image name
		ilBindImage(0);
		ilDeleteImages(1, &image);
		return;
	}

	// Get the image dimensions
	// NOTE: We'll consider a single planar image to be a volume image of height 1
	ILint width = ilGetInteger( IL_IMAGE_WIDTH );   // s
	ILint height = ilGetInteger( IL_IMAGE_HEIGHT ); // t
	ILint depth = ilGetInteger( IL_IMAGE_DEPTH );   // r

	// We require power-of-two dimensions, so validate them here.
	if (!isPowerOf2( width ) || !isPowerOf2( height ) || !isPowerOf2( depth ))
	{
		TRACE_ERR( "3D textures must be power of 2 in all dimensions" );

		// Unbind and delete the image name
		ilBindImage(0);
		ilDeleteImages(1, &image);
		return;
	}

	// DevIL's ilCopyPixels() method will convert the pixels into any ofthe supported formats, but we want
	// to choose a format that best matches the native format. This chunk of code does this by looking for
	// an exact match. If an exact match is not found, a default format is used.
	ILint ilFmt = ilGetInteger( IL_IMAGE_FORMAT );
	GLint glFmt = 0;
	int colorCount = 0;
	switch (ilFmt)
	{
		//case IL_COLOUR_INDEX:
		case IL_COLOR_INDEX:
		case IL_BGR:
			// Convert the color index and BGr formats to RGB format
			ilFmt = IL_RGB;
		case IL_RGB:
			glFmt = GL_RGB;
			colorCount = 3;
			break;

		case IL_BGRA:
			// Convert the BGRA format to RGBA
			ilFmt = IL_RGBA;
		case IL_RGBA:
			glFmt = GL_RGBA;
			colorCount = 4;
			break;

		case IL_LUMINANCE:
			glFmt = GL_LUMINANCE;
			colorCount = 1;
			break;

		case IL_LUMINANCE_ALPHA:
			glFmt = GL_LUMINANCE_ALPHA;
			colorCount = 2;
			break;

		default:
			// By default, use RGB format
			ilFmt = IL_RGB;
			glFmt = GL_RGB;
			colorCount = 3;
			break;
	}

	ILint ilBpp = ilGetInteger( IL_IMAGE_BYTES_PER_PIXEL );
	ILint ilType = ilGetInteger( IL_IMAGE_TYPE );
	GLint glType = 0;
	switch (ilType)
	{
		case IL_BYTE:
			glType = GL_BYTE;
			break;

		case IL_UNSIGNED_BYTE:
			glType = GL_UNSIGNED_BYTE;
			break;

		case IL_SHORT:
			glType = GL_SHORT;
			break;

		case IL_UNSIGNED_SHORT:
			glType = GL_UNSIGNED_SHORT;
			break;

		case IL_INT:
			glType = GL_INT;
			break;

		case IL_UNSIGNED_INT:
			glType = GL_UNSIGNED_INT;
			break;

		case IL_FLOAT:
			glType = GL_FLOAT;
			break;

		case IL_DOUBLE:
			glType = GL_DOUBLE;
			break;

	};

	// Allocate a buffer of the chosen format, copy the pixels into it, then use that buffer to set the
	// 3d texture's bits.
	char *buffer = new char[ilBpp * width * height * depth];
	if (ilCopyPixels (0, 0, 0, width, height, depth, ilFmt, IL_UNSIGNED_BYTE, buffer) == IL_FALSE)
	{
		// Failed to copy
		TRACE_ERR( GetILErrorString ( ilGetError() ) );
	}
	else
	{
		// The temporary buffer is now filled, so put the bits into the GL texture
		glBindTexture( GL_TEXTURE_3D, texture3d );

		glTexImage3D(
			GL_TEXTURE_3D, 0, colorCount, 
			width, height, depth, 0, glFmt, glType, buffer
		);
	}

	// Free the temporary buffer and the image, and we are done
	delete buffer;
	ilBindImage(0);
	ilDeleteImages(1, &image);
}

int zglGenTexture3DFromFile( const char * filename ) {
	int texture = zglGenTexture3D();
	zglLoadTexture3DFromFile( filename, texture );
	return texture;
}

//
// Texture allocator
//
ZGLTex3D::ZGLTex3D( int _alloc ) {
	tex = 0;
	if( _alloc ) {
		tex = zglGenTexture3D();
	}
}

ZGLTex3D::~ZGLTex3D() {
	if( tex ) {
		glDeleteTextures( 1, &tex );
	}
}

void ZGLTex3D::clear() {
	if( tex ) {
		glDeleteTextures( 1, &tex );
		tex = 0;
	}
}

void ZGLTex3D::alloc() {
	tex = zglGenTexture3D();
}
