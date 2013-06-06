#include "wintiny.h"
#include "wingl.h"
#include "GL/gl.h"
#include "guiimage.h"
#include "string.h"
#include "zbitmapdesc.h"
#include "gltools.h"
	// This dependency could be eliminated with a little work

// GUIImage
//==================================================================================

GUI_TEMPLATE_IMPLEMENT(GUIImage);

void GUIImage::init() {
	GUIObject::init();
	texture = zglGenTexture();
}

GUIImage::~GUIImage() {
	glDeleteTextures( 1, (GLuint*)&texture );
}

void GUIImage::render() {
	glColor4ub( 255, 255, 255, 255 );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, texture );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glBegin( GL_QUADS );
		glTexCoord2f( 0.f, 1.f );  glVertex2f( -1, -1 );
		glTexCoord2f( 0.f, 0.f );  glVertex2f( -1, myH+1 );
		glTexCoord2f( 1.f, 0.f );  glVertex2f( myW+1, myH+1 );
		glTexCoord2f( 1.f, 1.f );  glVertex2f( myW+1, -1 );
	glEnd();
}

void GUIImage::queryOptSize( float &w, float &h ) {
	w = (float)imageW;
	h = (float)imageH;
}

void GUIImage::handleMsg( TMsg *msg ) {
	if( msgIs(type,GUIImage_Load) ) {
		char *filename = msgS(filename);
		ZBitmapDesc tempDesc;

		zglLoadTexture( filename, texture, &tempDesc );

		imageW = tempDesc.w;
		imageH = tempDesc.h;
		parent->postLayout();
	}
}

