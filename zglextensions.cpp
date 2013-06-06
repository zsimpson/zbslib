// @ZBS {
//		+DESCRIPTION {
//			Deals with gl extensions.
//		}
//		*PORTABILITY win32
//		*REQUIRED_FILES zglextensions.cpp zglextensions.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//			Only barely implemented
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
#include "windows.h"
// SDK includes:
#include "GL/gl.h"
// STDLIB includes:
#include "assert.h"
// MODULE includes:
#include "zglextensions.h"
// ZBSLIB includes:

// Extensions
//===========================================================================
/*
int zglIsExtensionSupported( char *extension ) {
	char *start = NULL;
	char *where, *terminator;

	// Extension names should not have spaces.
	where = strchr( extension, ' ' );
	if( where || *extension == '\0' ) {
		return 0;
	}

	char *extensions = (char *)glGetString(GL_EXTENSIONS);
	// It takes a bit of care to be fool-proof about parsing the
	//  OpenGL extensions string. Don't be fooled by sub-strings, etc.

	char *winExtensions = "";		
	static PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = 0;
	if( !wglGetExtensionsStringARB ) {
		wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress( "wglGetExtensionsStringARB" );
	}
	if( wglGetExtensionsStringARB ) {
		winExtensions = (char*)wglGetExtensionsStringARB( wglGetCurrentDC() );
	}

	start = extensions;
	while( 1 ) {
		where = strstr( start, extension );
		if( !where ) break;
		terminator = where + strlen( extension );
		if( where == start || *(where - 1) == ' ' ) {
			if( *terminator == ' ' || *terminator == '\0' ) {
				return 1;
			}
		}
		start = terminator;
	}

	start = winExtensions;
	while( 1 ) {
		where = strstr( start, extension );
		if( !where ) break;
		terminator = where + strlen( extension );
		if( where == start || *(where - 1) == ' ' ) {
			if( *terminator == ' ' || *terminator == '\0' ) {
				return 1;
			}
		}
		start = terminator;
	}

	return 0;
}

// This is here just to avoid a windows header dependency on 
// systems which wish to use and extension pointer.
void *zglGetExtension( char *name ) {
	return (void *)wglGetProcAddress( name );
}
*/

GLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB = NULL;
GLACTIVETEXTUREARBPROC glActiveTextureARB = NULL;
GLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB = NULL;
int zglNumMultiTextures = 0;
GLCOLORTABLEEXTPROC glColorTableEXT = NULL;

void zglUseExtension( char *funcName ) {
	if( !strcmp(funcName,"glMultiTexCoord2fARB" ) ) {
		glMultiTexCoord2fARB = (GLMULTITEXCOORD2FARBPROC)wglGetProcAddress( "glMultiTexCoord2fARB" );
		assert( glMultiTexCoord2fARB );
	}
	else if( !strcmp(funcName,"glActiveTextureARB" ) ) {
		glGetIntegerv( GL_MAX_TEXTURES_UNITS_ARB, &zglNumMultiTextures );

		glActiveTextureARB = (GLACTIVETEXTUREARBPROC)wglGetProcAddress( "glActiveTextureARB" );
		assert( glActiveTextureARB );
	}
	else if( !strcmp(funcName,"glClientActiveTextureARB" ) ) {
		glClientActiveTextureARB = (GLCLIENTACTIVETEXTUREARBPROC)wglGetProcAddress( "glClientActiveTextureARB" );
		assert( glClientActiveTextureARB );
	}
	else if( !strcmp(funcName,"glColorTableEXT" ) ) {
		glColorTableEXT = (GLCOLORTABLEEXTPROC)wglGetProcAddress( "glColorTableEXT" );
		assert( glColorTableEXT );
	}
}
