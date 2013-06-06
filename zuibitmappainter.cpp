// @ZBS {
//		*COMPONENT_NAME zuibitmappainter
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuibitmappainter.cpp
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
// ZBSLIB includes:
#include "ztmpstr.h"
#include "zgraphfile.h"
#include "zgltools.h"
#include "zmathtools.h"
#include "zbitmapdesc.h"
#include "zui.h"
#include "zbits.h"

struct ZUIBitmapPainter : public ZUIPanel {
	ZUI_DEFINITION( ZUIBitmapPainter, ZUIPanel );

	// SPECIAL ATTRIBUTES:
	// paintMode = none | paint | erase
	// brushSize = pixelRadius
	// bitmapPtr = a pointer to the bitmap to paint into
	// bitmapOwner = 0 | 1: Does this gui own the bitmap memory
	// sendMsg = msg to send on change
	// sendMsgOnPaint = 0 | 1: should the sendMsg happen on every paint
	// paintColor = paint color
	// paintMask = bit mask when writing color
	//
	// SPEICAL MESSAGES:
	// ZUIBitmapPainter_AllocBitmap
	// ZUIBitmapPainter_Save
	// ZUIBitmapPainter_Load
	// ZUIBitmapPainter_Blur
	// ZUIBitmapPainter_Clear


	int texture;
	float mouseX, mouseY;
//	int isEmpty;

	void freeBitmap();
	void sendMsg();
	void drawCircle( int x, int y, int radius, int colr, int mask );

	ZUIBitmapPainter() : ZUIPanel() { }
	virtual void factoryInit();
	virtual void destruct();
	virtual void render();
	virtual void handleMsg( ZMsg *msg );
};

ZUI_IMPLEMENT( ZUIBitmapPainter, ZUIPanel );

void ZUIBitmapPainter::factoryInit() {
	ZUIPanel::factoryInit();
	putS( "mouseRequest", "over" );
	texture = zglGenTexture();
//	isEmpty = 1;
	putI( "brushSize", 6 );
	putS( "paintMode", "none" );
}

void ZUIBitmapPainter::destruct() {
	glDeleteTextures( 1, (GLuint *)&texture );
	freeBitmap();
}

void ZUIBitmapPainter::render() {
	ZBits *bitmap = (ZBits *)getI( "bitmapPtr" );
	if( !bitmap ) return;

	float w, h;
	getWH( w, h );

//	if( !isEmpty ) {
		glPushMatrix();
			glTranslatef( getF("offsetX"), getF("offsetY"), 0.f );
			glColor4ub( 255, 255, 255, 255 );
			glEnable( GL_TEXTURE_2D );
			glBindTexture( GL_TEXTURE_2D, texture );
			glEnable( GL_BLEND );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			glBegin( GL_QUADS );
				glTexCoord2f( 0.f, bitmap->th() );
				glVertex2f( -1, -1 );

				glTexCoord2f( 0.f, 0.f );
				glVertex2f( -1, h+1 );

				glTexCoord2f( bitmap->tw(), 0.f );
				glVertex2f( w+1, h+1 );

				glTexCoord2f( bitmap->tw(), bitmap->th() );
				glVertex2f( w+1, -1 );
			glEnd();
		glPopMatrix();
//	}

	glDisable( GL_TEXTURE_2D );
	glDisable( GL_BLEND );

	if( !isS("paintMode","none") ) {
		glPushMatrix();
		glTranslatef( mouseX, mouseY, 0.f );
		glScalef( w/bitmap->w(), h/bitmap->h(), 1.f );
		float brushRad = getF( "brushSize" );

		if( isS("paintMode","erase") ) {
			glBegin( GL_LINE_STRIP );
		}
		else if( isS("paintMode","paint") ) {
			glBegin( GL_TRIANGLE_FAN );
			glVertex2f( 0.f, 0.f );
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

void ZUIBitmapPainter::freeBitmap() {
	if( getI("bitmapOwner") ) {
		ZBits *bitmap = (ZBits *)getI( "bitmapPtr" );
		if( bitmap ) {
			delete bitmap;
		}
		putI( "bitmapPtr", 0 );
	}
}

void ZUIBitmapPainter::sendMsg() {
	if( has("sendMsg") ) {
		zMsgQueue( "%s", getS("sendMsg") );
	}
}

void ZUIBitmapPainter::drawCircle( int x, int y, int radius, int colr, int erase ) {
	// DRAW a circular brush into the bitmap at x y in bitmap coords
	ZBits *bitmap = (ZBits *)getI( "bitmapPtr" );

	assert( bitmap->channelDepthInBits == 8 );

	int l = (int)max( 0.f, x-radius-1 );
	int r = (int)min( bitmap->w()-1, x+radius+1 );
	int t = (int)max( 0, y-radius-1 );
	int b = (int)min( bitmap->h()-1, y+radius+1 );
	
	int color = erase ? 0 : 0xFF000000;
	
	for( int _y=t; _y<=b; _y++ ) {
		for( int _x=l; _x<=r; _x++ ) {
			int dx = (x - _x);
			int dy = (y - _y);
			if( dx*dx+dy*dy < radius*radius ) {
/*
				switch( bitmap->channels + (bitmap->alphaChannel ? 1 : 0) ) {
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
						//unsigned int c = bitmap->getUint( _x, _y );
						//c &= ~mask;
						//c |= colr;
						//bitmap->setUint( _x, _y, c );
*/
						bitmap->setUint( _x, _y, color );
/*
						break;
					}
				}
*/
			}
		}
	}
}

void ZUIBitmapPainter::handleMsg( ZMsg *msg ) {
	int dirty = 0;

	if( zmsgIs(type,ZUIMouseMove) || zmsgIs(type,ZUIMouseDrag) ) {
		mouseX = zmsgF(localX);
		mouseY = zmsgF(localY);
	}

	ZBits *bitmap = (ZBits *)getI( "bitmapPtr" );
	float w, h;
	getWH( w, h );

	if( isS("paintMode","paint") || isS("paintMode","erase") ) {
		int doPaint = 0;
		if( zmsgIs(type,ZUIMouseClickOn) && zmsgIs(which,L) && zmsgIs(dir,D) ) {
			requestExclusiveMouse( 0, 1 );
			doPaint = 1;
		}
		else if( zmsgIs(type,ZUIExclusiveMouseDrag) ) {
			mouseX = zmsgF(localX);
			mouseY = zmsgF(localY);
			doPaint = 1;
		}
		else if( zmsgIs(type,ZUIMouseReleaseDrag) ) {
			sendMsg();
			requestExclusiveMouse( 0, 0 );
			doPaint = 0;
		}
		
		if( doPaint ) {
			if( bitmap ) {
				// DRAW a circular brush into the bitmap
//				isEmpty = 0;

				int erase = isS("paintMode","erase");

				drawCircle( 
					zmsgI(localX) * bitmap->w() / (int)w, 
					((int)h-zmsgI(localY)) * bitmap->h() / (int)h, 
					getI("brushSize"), getI( "paintColor" ), erase
				);
				dirty = 1;
			}
		}
	}

	if( zmsgIs(type,ZUIBitmapPainter_AllocBitmap) ) {
		freeBitmap();
		putI( "bitmapOwner", 1 );
		bitmap = new ZBits;
		bitmap->create( zmsgI(w), zmsgI(h), zmsgI(d), 1, 1 );
		memset( bitmap->bits, 0, bitmap->lenInBytes() );
		putI( "bitmapPtr", (int)bitmap );
	}
	else if( zmsgIs(type,ZUIBitmapPainter_Save) ) {
		char *filename = zmsgS(filename);
		assert( *filename );
		if( bitmap ) {
			zGraphFileSave( filename, bitmap );
		}
	}
	else if( zmsgIs(type,ZUIBitmapPainter_Load) ) {
		char *filename = zmsgS(filename);
		assert( *filename );

		ZBits fileBits;
		int ok = zGraphFileLoad( filename, &fileBits );
		if( !ok ) return;
//		isEmpty = 0;

		if( zmsgI(realloc) ) {
			freeBitmap();
			bitmap = new ZBits;
			bitmap->create( fileBits.w(), fileBits.h(), 2 );
			putI( "bitmapPtr", (int)bitmap );
			zbitsConvert( &fileBits, bitmap );
		}
		else {
			assert( bitmap );
			if( fileBits.w() == bitmap->w() && fileBits.h() == bitmap->h() ) {
				zbitsConvert( &fileBits, bitmap );
			}
		}

		if( zmsgI(invert) ) {
			assert( bitmap->channelDepthInBits == 8 );
			assert( bitmap->channels == 1 );
			for( int y=0; y<bitmap->h(); y++ ) {
				unsigned char *dst = bitmap->lineUchar( y );
				for( int x=0; x<bitmap->w(); x++ ) {
//					switch( bitmap->d ) {
//						case 1:
							*dst++ = ~*dst;
//							break;
//						case 2:
//							*dst++ = ~*dst;
//							*dst++ = *dst;
//							break;
//						case 3:
//							*dst++ = *dst;
//							*dst++ = *dst;
//							*dst++ = *dst;
//							break;
//						case 4:
//							*dst++ = *dst;
//							*dst++ = ~*dst;
//							*dst++ = ~*dst;
//							*dst++ = ~*dst;
//							break;
//					}
				}
			}
		}

		dirty = 1;
	}
	else if( zmsgIs(type,ZUIBitmapPainter_Refresh) ) {
		dirty = 1;
	}
	else if( zmsgIs(type,ZUIBitmapPainter_Blur) ) {
		sendMsg();
		ZBits copy;
		copy.clone( bitmap );
		assert( bitmap->channels == 3 && bitmap->alphaChannel );
		for( int y=0; y<bitmap->h(); y++ ) {
			unsigned char *dst = bitmap->lineUchar( y );
			for( int x=0; x<bitmap->w(); x++ ) {
				int l = x-2 <= 0 ? 0 : x-2;
				int r = x+3 >= bitmap->w() ? bitmap->w() : x+3;
				int t = y-2 <= 0 ? 0 : y-2;
				int b = y+3 >= bitmap->h() ? bitmap->h() : y+3;

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
	else if( zmsgIs(type,ZUIBitmapPainter_Clear) ) {
		sendMsg();
		memset( bitmap->lineChar(0), 0, bitmap->lenInBytes() );
		dirty = 1;
//		isEmpty = 1;
	}

	if( dirty ) {
		if( getI("sendMsgOnPaint") ) {
			sendMsg();
		}
		zglLoadTextureFromZBits( bitmap, texture );
	}

	ZUIPanel::handleMsg( msg );
}




