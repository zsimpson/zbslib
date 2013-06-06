// @ZBS {
//		*COMPONENT_NAME zuipicture
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuipicture.cpp
// }

// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#include "GL/gl.h"
// STDLIB includes:
#include "assert.h"
#include "string.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"
// MODULE includes:
#include "zui.h"
// ZBSLIB includes:
#include "zglfont.h"
#include "zmsg.h"
#include "zglgfiletools.h"
#include "zbitmapdesc.h"

// ZUIPicture
//--------------------------------------------------------------------------------------------------------------------------------

ZUI_IMPLEMENT(ZUIPicture,ZUI);

void ZUIPicture::cachePicture() {
	char *filename = getS( "filename", "" );
	char *lastLoadedFilename = getS( "lastLoadedFilename", "" );
	if( strcmp(filename,lastLoadedFilename) ) {
		// The filename is different, reload
		int tex = getI( "tex" );
		if( tex ) {
			glDeleteTextures( 1, (GLuint *)&tex );
		}
		// @TODO: Change to ZBits, do correction for non-power of 2
		ZBitmapDesc desc;
		tex = zglGenTexture( filename, &desc, 1 );
		putF( "pictureW", (float)desc.w );
		putF( "pictureH", (float)desc.h );
		putF( "texW", desc.memWRatioF() );
		putF( "texH", desc.memHRatioF() );
		putI( "tex", tex );
		putS( "lastLoadedFilename", filename );
	}
}

void ZUIPicture::render() {
	ZUI::render();

	cachePicture();

	setupColor( "pictureColor" );

	float texW = getF( "texW", 1.f );
	float texH = getF( "texH", 1.f );
	float w, h;
	getWH( w, h );

	int tex = getI( "tex" );
	if( tex ) {
		glPushAttrib( GL_ENABLE_BIT );
		glBindTexture( GL_TEXTURE_2D, tex );
		glEnable (GL_TEXTURE_2D);
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glBegin( GL_QUADS );
			glTexCoord2f( 0.f, 0.f );
			glVertex2f( 0.f, 0.f );
			glTexCoord2f( texW, 0.f );
			glVertex2f( w, 0.f );
			glTexCoord2f( texW, texH );
			glVertex2f( w, h );
			glTexCoord2f( 0.f, texH );
			glVertex2f( 0.f, h );
		glEnd();
		glPopAttrib();
	}
}

ZUI::ZUILayoutCell *ZUIPicture::layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly ) {
	cachePicture();
	float pictureW = getF( "pictureW" );
	float pictureH = getF( "pictureH" );
	w = pictureW;
	h = pictureH;
	return 0;
}
