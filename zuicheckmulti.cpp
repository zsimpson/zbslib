// @ZBS {
//		*COMPONENT_NAME zuicheckmulti
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuicheckmulti.cpp
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
#include "zgltools.h"
#include "zmsg.h"
#include "zvars.h"

// ZUICheck
//--------------------------------------------------------------------------------------------------------------------------------

ZUI_IMPLEMENT(ZUICheckMulti,ZUICheck);

void ZUICheckMulti::factoryInit() {
	ZUICheck::factoryInit();
	putI( "zuiCheckExtraH", 0 );
		// cancel behavior of ZUICheck: we don't use checks that go outside
		// of the official rect.
}

void ZUICheckMulti::checkSendSelectedMsg() {
	int selected = getSelected();
	int lastSelected = getI( "lastSelectedState", -1 );
	if( selected != lastSelected ) {
		if( selected ) {
			zMsgQueue( "%s selected=%d lastSelected=%d", getS("sendMsgOnSelect"), selected, lastSelected );
		}
		else {
			zMsgQueue( "%s selected=%d lastSelected=%d", getS("sendMsgOnUnselect"), selected, lastSelected );
		}
	}
	putI( "lastSelectedState", selected );
}

void ZUICheckMulti::dirty( int el, int er, int et, int eb ) {
	ZUI::dirty( el, er+5, et+1, eb );
		// hack for kintek: these checkmulti are layout in a table column whose width is less
		// than we need to render, so we allow ourselves to render outside of our rect.  what
		// is the better way to do this?  It seems the table layout forces our width to its liking - 
		// can we stop it?
}

void ZUICheckMulti::render() {
	int selected = getSelected();
	checkSendSelectedMsg();

	void *fontPtr = zglFontGet( getS("font") );
	float _w, _h, _d;
	zglFontGetDim( "Q", _w, _h, _d, 0, fontPtr );

	glPushMatrix();
	int texID = getI( "textureID", -1 );
	int stateCount = getI( "stateCount", -1 );

	if( texID != -1 && stateCount != -1 ) {
		float aspectW = getF( "textureAspectW", 1.f );
		float aspectH = getF( "textureAspectH", 1.f );
		float widthOfIconInTexCoords = aspectW / (float)stateCount;
		float heightOfIconInTexCoords = aspectH;
		float texLft = (selected+0) * widthOfIconInTexCoords;
		float texRgt = (selected+1) * widthOfIconInTexCoords;

		glColor3ub( 255, 255, 255 );
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glEnable( GL_TEXTURE_2D );
		glBindTexture( GL_TEXTURE_2D, texID );

		glScalef( getF("scaleX",1.f), getF("scaleY",1.f), 1.f );

//		glTranslatef( 0.f, 3.f, 0.f );
		glBegin( GL_QUADS );

		float dimension = max( w, h );

//		float x1 = h * 1.15f;
//		float y1 = x1 * ( stateCount > 2 ? 1.25f : 1.7f ); 
//			// hack: stretch the icon according to special cases in _kin
//		float y_offset = 2.0f;
			// hack: move the icon image down a bit to line up with text
			glTexCoord2f( texLft, 0.0f );
			glVertex2f( 0.f, 0.f );

			glTexCoord2f( texRgt, 0.0f );
			glVertex2f( dimension, 0.f );

			glTexCoord2f( texRgt, heightOfIconInTexCoords );
			glVertex2f( dimension, dimension );

			glTexCoord2f( texLft, heightOfIconInTexCoords );
			glVertex2f( 0, dimension );
		glEnd();
	}
	glPopMatrix();

	glPushMatrix();
		int color = getU( "checkTextColor" );
		if( color ) {
			setColorI( color );
		}

		char *text = getText();
		if( text ) {
			float boxW = _w;
			float boxAndPad = _w + 4.f;
			float boxB = _d;
			float boxT = _w + _d - 1.f;

			if( getI("wordWrap") ) {
				printWordWrap( text, boxAndPad, 0.f, w, h, 0, 0 );
			}
			else {
				zglFontPrint( text, h + 2.f, _d+1.f, 0, 0, fontPtr );
			}

			// Key binding
			float kw, kh;
			char key[64];
			if( getKeyBindingText( kw, kh, key ) ) {
				float tw, th, td;
				zglFontGetDim( text, tw, th, td, 0, fontPtr );

				glPushAttrib( GL_CURRENT_BIT );
				if( getI("keyState") ) {
					setColorI( 0xFF0000FF );
				}
				zglFontPrint( key, boxAndPad + tw, h-_h+_d, 0, 0, fontPtr );
				glPopAttrib();
			}
		}
	glPopMatrix();
}

void ZUICheckMulti::handleMsg( ZMsg *msg ) {
	if( zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L) ) {
		int stateCount = getI( "stateCount", 2 );
		setSelected( ( getSelected( 0 ) + 1 ) % stateCount, 0 );
		zMsgUsed();
		return;
	}
	else if( zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,R) ) {
		setSelected( 0 );
		zMsgUsed();
		return;
	}

	ZUICheck::handleMsg( msg );
}

