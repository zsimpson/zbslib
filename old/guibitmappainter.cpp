// @ZBS {
//		*COMPONENT_NAME guibitmappainter
//		*MODULE_OWNER_NAME glgui
//		*GROUP modules/gui
//		*REQUIRED_FILES guibitmappainter.h
// }
// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#include "GL/gl.h"
#include "GL/glu.h"
// STDLIB includes:
#include "string.h"
#include "assert.h"
#include "math.h"
// MODULE includes:
#include "guibitmappainter.h"
// ZBSLIB includes:
#include "ztmpstr.h"
#include "zgraphfile.h"
#include "zgltools.h"
#include "zmathtools.h"
#include "zbitmapdesc.h"

GUI_TEMPLATE_IMPLEMENT(GUIBitmapPainter);

void GUIBitmapPainter::init() {
	GUIPanel::init();
	setAttrS( "mouseRequest", "over" );
	texture = zglGenTexture();
	isEmpty = 1;
	setAttrI( "brushSize", 6 );
	setAttrS( "paintMode", "none" );
}

GUIBitmapPainter::~GUIBitmapPainter() {
	glDeleteTextures( 1, (GLuint *)&texture );
	if( this->attributeTable ) {
		freeBitmap();
	}
}

void GUIBitmapPainter::render() {
	ZBitmapDesc *bitmap = (ZBitmapDesc *)getAttrI( "bitmapPtr" );

	if( !isEmpty ) {
		glPushMatrix();
			glTranslatef( getAttrF("offsetX"), getAttrF("offsetY"), 0.f );
			glColor4ub( 255, 255, 255, 255 );
			glEnable( GL_TEXTURE_2D );
			glBindTexture( GL_TEXTURE_2D, texture );
			glEnable( GL_BLEND );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			glBegin( GL_QUADS );
				glTexCoord2f( 0.f, bitmap->memHRatioF() );
				glVertex2f( -1, -1 );

				glTexCoord2f( 0.f, 0.f );
				glVertex2f( -1, myH+1 );

				glTexCoord2f( bitmap->memWRatioF(), 0.f );
				glVertex2f( myW+1, myH+1 );

				glTexCoord2f( bitmap->memWRatioF(), bitmap->memHRatioF() );
				glVertex2f( myW+1, -1 );
			glEnd();
		glPopMatrix();
	}

	glDisable( GL_TEXTURE_2D );
	glDisable( GL_BLEND );

	if( !isAttrS("paintMode","none") ) {
		glPushMatrix();
		glTranslatef( mouseX, mouseY, 0.f );
		glScalef( myW/bitmap->w, myH/bitmap->h, 1.f );
		float brushRad = getAttrF( "brushSize" );

		if( isAttrS("paintMode","erase") ) {
			glBegin( GL_LINE_STRIP );
		}
		else if( isAttrS("paintMode","paint") ) {
			glBegin( GL_TRIANGLE_FAN );
			glVertex2f( 0.f, 0.f );
		}
		else {
		}
		glColor3ub( 255, 0, 0 );
		float tf = 0.f;
		for( int t=0; t<=16; t++, tf += PI2F/16.f ) {
			glVertex2f( (float)brushRad*cosf(tf), (float)brushRad*sinf(tf) );
		}
		glEnd();
		glPopMatrix();
	}
}

void GUIBitmapPainter::freeBitmap() {
	if( getAttrI("bitmapOwner") ) {
		ZBitmapDesc *bitmap = (ZBitmapDesc *)getAttrI( "bitmapPtr" );
		if( bitmap ) {
			delete bitmap;
		}
		setAttrI( "bitmapPtr", 0 );
	}
}

void GUIBitmapPainter::sendMsg() {
	if( hasAttr("sendMsg") ) {
		zMsgQueue( "%s", getAttrS("sendMsg") );
	}
}

void GUIBitmapPainter::drawCircle( int x, int y, int radius, int colr, int mask ) {
	// DRAW a circular brush into the bitmap at x y in bitmap coords
	ZBitmapDesc *bitmap = (ZBitmapDesc *)getAttrI( "bitmapPtr" );

	int l = (int)max( 0.f, x-radius-1 );
	int r = (int)min( bitmap->w-1, x+radius+1 );
	int t = (int)max( 0, y-radius-1 );
	int b = (int)min( bitmap->h-1, y+radius+1 );
	for( int _y=t; _y<=b; _y++ ) {
		for( int _x=l; _x<=r; _x++ ) {
			int dx = (x - _x);
			int dy = (y - _y);
			if( dx*dx+dy*dy < radius*radius ) {
				switch( bitmap->d ) {
					case 1: {
						unsigned char c = bitmap->getUchar( _x, _y );
						c &= ~mask;
						c |= colr;
						bitmap->setUchar( _x, _y, c );
						break;
					}
					case 2: {
						unsigned short c = bitmap->getUshort( _x, _y );
						c &= ~mask;
						c |= colr;
						bitmap->setUshort( _x, _y, c );
						break;
					}
					case 3: {
						unsigned int c = bitmap->getTriplet( _x, _y );
						c &= ~mask;
						c |= colr;
						bitmap->setTriplet( _x, _y, c );
						break;
					}
					case 4: {
						unsigned int c = bitmap->getUint( _x, _y );
						c &= ~mask;
						c |= colr;
						bitmap->setUint( _x, _y, c );
						break;
					}
				}
			}
		}
	}
}

void GUIBitmapPainter::handleMsg( ZMsg *msg ) {
	int dirty = 0;

	if( zmsgIs(type,MouseMove) || zmsgIs(type,MouseDrag) ) {
		mouseX = zmsgF(localX);
		mouseY = zmsgF(localY);
	}

	ZBitmapDesc *bitmap = (ZBitmapDesc *)getAttrI( "bitmapPtr" );

	if( isAttrS("paintMode","paint") || isAttrS("paintMode","erase") ) {
		int doPaint = 0;
		if( zmsgIs(type,MouseClickOn) && zmsgIs(which,L) && zmsgIs(dir,D) ) {
			requestExclusiveMouse( 0, 1 );
			doPaint = 1;
		}
		else if( zmsgIs(type,MouseDrag) ) {
			doPaint = 1;
		}
		else if( zmsgIs(type,MouseReleaseDrag) ) {
			sendMsg();
			requestExclusiveMouse( 0, 0 );
		}

		if( doPaint ) {
			if( bitmap ) {
				// DRAW a circular brush into the bitmap
				isEmpty = 0;
				drawCircle( 
					zmsgI(localX) * bitmap->w / (int)myW, 
					((int)myH-zmsgI(localY)) * bitmap->h / (int)myH, 
					getAttrI("brushSize"), getAttrI( "paintColor" ), getAttrI( "paintMask" )
				);
				dirty = 1;
			}
		}
	}

	if( zmsgIs(type,GUIBitmapPainter_AllocBitmap) ) {
		freeBitmap();
		setAttrI( "bitmapOwner", 1 );
		bitmap = new ZBitmapDesc;
		bitmap->allocP2( zmsgI(w), zmsgI(h), zmsgI(d) );
		memset( bitmap->bits, 0, bitmap->bytes() );
		setAttrI( "bitmapPtr", (int)bitmap );
	}
	else if( zmsgIs(type,GUIBitmapPainter_Save) ) {
		char *filename = zmsgS(filename);
		assert( *filename );
		if( bitmap ) {
			zGraphFileSave( filename, bitmap );
		}
	}
	else if( zmsgIs(type,GUIBitmapPainter_Load) ) {
		char *filename = zmsgS(filename);
		assert( *filename );

		ZBitmapDesc fileDesc;
		int ok = zGraphFileLoad( filename, &fileDesc );
		if( !ok ) return;
		isEmpty = 0;

		if( zmsgI(realloc) ) {
			freeBitmap();
			bitmap = new ZBitmapDesc;
			bitmap->allocP2( fileDesc.w, fileDesc.h, 2 );
			setAttrI( "bitmapPtr", (int)bitmap );
			zBitmapDescConvert( fileDesc, *bitmap );
		}
		else {
			assert( bitmap );
			if( fileDesc.w == bitmap->w && fileDesc.h == bitmap->h ) {
				zBitmapDescConvert( fileDesc, *bitmap );
			}
		}

		if( zmsgI(invert) ) {
			for( int y=0; y<bitmap->h; y++ ) {
				unsigned char *dst = bitmap->lineUchar( y );
				for( int x=0; x<bitmap->w; x++ ) {
					switch( bitmap->d ) {
						case 1:
							*dst++ = ~*dst;
							break;
						case 2:
							*dst++ = ~*dst;
							*dst++ = *dst;
							break;
						case 3:
							*dst++ = *dst;
							*dst++ = *dst;
							*dst++ = *dst;
							break;
						case 4:
							*dst++ = *dst;
							*dst++ = ~*dst;
							*dst++ = ~*dst;
							*dst++ = ~*dst;
							break;
					}
				}
			}
		}

		dirty = 1;
	}
	else if( zmsgIs(type,GUIBitmapPainter_Blur) ) {
		sendMsg();
		ZBitmapDesc copy;
		copy.copy( *bitmap );
		assert( bitmap->d == 4 );
		for( int y=0; y<bitmap->h; y++ ) {
			unsigned char *dst = bitmap->lineUchar( y );
			for( int x=0; x<bitmap->w; x++ ) {
				int l = x-2 <= 0 ? 0 : x-2;
				int r = x+3 >= bitmap->w ? bitmap->w : x+3;
				int t = y-2 <= 0 ? 0 : y-2;
				int b = y+3 >= bitmap->h ? bitmap->h : y+3;

				int aSum = 0;
				int rSum = 0;
				int gSum = 0;
				int bSum = 0;
				int count = 0;
				for( int _y=t; _y<b; _y++ ) {
					for( int _x=l; _x<r; _x++ ) {
						rSum += copy.lineUchar(_y)[_x*4+3];
						gSum += copy.lineUchar(_y)[_x*4+2];
						bSum += copy.lineUchar(_y)[_x*4+1];
						aSum += copy.lineUchar(_y)[_x*4+0];
						count++;
					}
				}
				dst[0] = aSum / count;
				dst[1] = bSum / count;
				dst[2] = gSum / count;
				dst[3] = rSum / count;
				dst += 4;
			}
		}
		dirty = 1;
	}
	else if( zmsgIs(type,GUIBitmapPainter_Clear) ) {
		sendMsg();
		memset( bitmap->lineChar(0), 0, bitmap->bytes() );
		dirty = 1;
		isEmpty = 1;
	}

	if( dirty ) {
		if( getAttrI("sendMsgOnPaint") ) {
			sendMsg();
		}
		zglLoadTextureFromBitmapDesc( bitmap, texture );
	}

	GUIPanel::handleMsg( msg );
}




