// @ZBS {
//		*COMPONENT_NAME zuicheck
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuicheck.cpp
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
#include "zvars.h"

// STOPFLOW timing
#include "sfmod.h"

// ZUICheck
//--------------------------------------------------------------------------------------------------------------------------------

ZUI_IMPLEMENT(ZUICheck,ZUIButton);

void ZUICheck::factoryInit() {
	ZUIButton::factoryInit();
	putI( "zuiCheckExtraH", 5 );
		// to account for rendering the check outside of our official rect
}

ZUI::ZUILayoutCell *ZUICheck::layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly ) {
	char *text = getText();
	if( text ) {
		void *fontPtr = zglFontGet( getS("font") );
		if( getI("wordWrap") ) {
			printWordWrap( text, 0.f, 0.f, maxW, maxH, &w, &h );
		}
		else {
			float d;
			zglFontGetDim( text, w, h, d, 0, fontPtr );
		}
		h += getI( "zuiCheckExtraH" );

		float kw, kh;
		char key[64];
		if( getKeyBindingText( kw, kh, key ) ) {
			w += kw;
		}

		float boxW, boxH, boxD;
		zglFontGetDim( "Q", boxW, boxH, boxD, 0, fontPtr );
		w += boxW + 12.f;
	}
	return 0;
}

int ZUICheck::getKeyBindingText( float &kw, float &kh, char *key ) {
	kw=0.f;
	kh=0.f;
	char *s = getS( "keyBinding", "" );
	if( *s ) {
		char *p = &key[2];
		for( ; *s; s++ ) {
			if( *s != '.' ) {
				*p++ = *s;
			}
		}
		*p = 0;
		if( key[2] ) {
			key[0] = ' ';
			key[1] = '(';
			int len = strlen(key);
			key[len] = ')';
			key[len+1] = 0;
			assert( len < 64-2 );
			float kd;
			zglFontGetDim( key, kw, kh, kd, getS("font") );
			return 1;
		}
	}
	return 0;
}

void ZUICheck::render() {
	SFTIME_START (PerfTime_ID_Zlab_render_check, PerfTime_ID_Zlab_render_render);
	int selected = getSelected();
	checkSendSelectedMsg();

	void *fontPtr = zglFontGet( getS("font") );
	float _w, _h, _d;
	zglFontGetDim( "Q", _w, _h, _d, 0, fontPtr );

	int extraH = getI( "zuiCheckExtraH" );

	glPushMatrix();
	glTranslatef( 0.f, _d+0.f, 0.f );
	renderButtonBase( h-2-extraH, h-2-extraH, getI("circularButton") ? 0 : 1 );
	glTranslatef( 0.f, -2.f, 0.f );
	if( selected ) {
		renderCheck();
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
				zglFontPrint( key, boxAndPad + tw, h-_h+_d-2.f, 0, 0, fontPtr );
				glPopAttrib();
			}
		}
	glPopMatrix();
	SFTIME_END (PerfTime_ID_Zlab_render_check);
}

void ZUICheck::handleMsg( ZMsg *msg ) {
	if( getI("disabled") ) {
		ZUI::handleMsg( msg );
		return;
	}

	if( zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L) ) {
		setSelected( ! getSelected() );
		zMsgUsed();
		return;
	}
	if( zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,R) ) {
		if( has( "sendMsgOnRightClick" ) ) {
			zMsgQueue( "%s", getS( "sendMsgOnRightClick" ) );
			zMsgUsed();
		}
	}
	else if( zmsgIs(type,ZUIModalKeysClear) ) {
		putI( "keyState", 0 );
	}

	else if( zmsgIs(type,Key) ) {
		putI( "toggle", 1 );
		handleKeyBinding( msg );
	}

	ZUI::handleMsg( msg );
}
