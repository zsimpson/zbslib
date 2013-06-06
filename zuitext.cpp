// @ZBS {
//		*COMPONENT_NAME zuitext
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuitext.cpp
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

// ZUIText
//--------------------------------------------------------------------------------------------------------------------------------

ZUI_IMPLEMENT(ZUIText,ZUI);

char *ZUIText::getText() {
	char *textPtr = (char *)getp("textPtr");
	if( textPtr ) {
		return (char *)textPtr;
	}
	return getS( "text", "" );
}

ZUI::ZUILayoutCell *ZUIText::layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly ) {
	char *text = getText();
	if( text ) {
		if( getI("wordWrap") ) {
			printWordWrap( text, 0.f, 0.f, maxW, maxH, &w, &h );
		}
		else {
			float d;
			void *fontPtr = zglFontGet( getS("font") );
			zglFontGetDim( text, w, h, d, 0, fontPtr );
		}
	}
	return 0;
}

void ZUIText::render() {
	ZUI::render();

//	setupColor( "textPanelColor" );
//	glBegin( GL_QUADS );
//		glVertex2f( 0.f, 0.f );
//		glVertex2f( w, 0.f );
//		glVertex2f( w, h );
//		glVertex2f( 0.f, h );
//	glEnd();

	glPushMatrix();
//		setupScroll();

//		int color = getI( "textColor" );
//		if( color ) {
//			setColorI( color );
//		}
//		else {
//			char *colorName = getS( "textColor" );
//			if( colorName ) {
//				setColorS( colorName );
//			}
//		}

//		scaleAlphaMouseOver();

		setupColor( "textColor" );

		char *text = getText();
		if( text ) {
			text = strdup( text );
			if( getI("wordWrap") ) {
				printWordWrap( text, 0.f, 0.f, w, h, 0, 0 );
			}
			else {
				float _w, _h, _d;
				void *fontPtr = zglFontGet( getS("font") );
				zglFontGetDim( "Q", _w, _h, _d, 0, fontPtr );
				zglFontPrint( text, 0.f, h - _h + _d, 0, 0, fontPtr );
			}
			free( text );
		}
	glPopMatrix();

	// @TODO: In order to draw the scroll bars, I have to be able to
	// determine the size of the text in the scroll area
//	if( getI("scrolls") ) {
//		float w, h;
//		getWH( w, h );
//		glBegin( GL_QUADS );
//			glColor3ub( 0, 0, 0 );
//			glVertex2f( w-4.f, 0.f );
//			glVertex2f( w, 0.f );
//			glVertex2f( w, h );
//			glVertex2f( w-4.f, h );
//
//			glColor3ub( 255, 255, 255 );
//			glVertex2f( w-4.f, 0.f );
//			glVertex2f( w, 0.f );
//			glVertex2f( w, h );
//			glVertex2f( w-4.f, h );
//		glEnd();
//	}
}

void ZUIText::handleMsg( ZMsg *msg ) {
	// @TODO: Make scrollsX, scrollsY independent

	if( has("sendMsg") ) {
		if( zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L) ) {
			zMsgQueue( "%s", getS( "sendMsg" ) );
			zMsgUsed();
		}
	}

//	if( getI("scrolls") ) {
//		if( zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,R) ) {
//			putF( "startScrollY", getF("scrollY") );
//			putF( "startScrollMouseY", zmsgF(localY) );
//			requestExclusiveMouse( 1, 1 );
//			zMsgUsed();
//		}
//		else if( zmsgIs(type,ZUIExclusiveMouseDrag) && zmsgI(r) ) {
//			float delta = zmsgF(localY) - getF("startScrollMouseY");
//			putF( "scrollY", getF("startScrollY")+delta );
//			zMsgUsed();
//		}
//		else if( zmsgIs(type,ZUIMouseReleaseDrag) ) {
//			requestExclusiveMouse( 1, 0 );
//			zMsgUsed();
//		}
//	}

	ZUI::handleMsg( msg );
}
