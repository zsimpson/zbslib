// @ZBS {
//		*COMPONENT_NAME zuibutton
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuibutton.cpp
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
#include "zplatform.h"


// ZUIButton
//--------------------------------------------------------------------------------------------------------------------------------

ZUI_IMPLEMENT(ZUIButton,ZUI);

void ZUIButton::factoryInit() {
	setNotifyMsg( "pressed", "type=ZUIDirty toZUI=$this" );
}

char *ZUIButton::getText() {
	char *textPtr = (char *)getp("textPtr");
	if( textPtr ) {
		return (char *)textPtr;
	}
	return getS( "text", "" );
}

int ZUIButton::getSelected( int forceBoolean ) {
	char *selectedVar = getS( "selectedVar", 0 );
	if( selectedVar ) {
		ZVarPtr *zVarPtr = zVarsLookup( selectedVar );
		if( zVarPtr ) {
			int val = (int)zVarPtr->getDouble(); 
			return forceBoolean ? ( val ? 1 : 0 ) : val;
		}
	}

	int *selectedPtr = (int *)getp( "selectedPtr" );
	if( selectedPtr ) {
		return *selectedPtr;
	}

	return getI( "selected" );
}

void ZUIButton::setSelected( int s, int forceBoolean ) {
	dirty();
	char *selectedVar = getS( "selectedVar", 0 );
	if( selectedVar ) {
		ZVarPtr *zVarPtr = zVarsLookup( selectedVar );
		if( zVarPtr ) {
			zVarPtr->setFromDouble( forceBoolean ? ( s ? 1.0 : 0.0 ) : s );
		}
		return;
	}

	int *selectedPtr = (int *)getp( "selectedPtr" );
	if( selectedPtr ) {
		*selectedPtr = forceBoolean ? ( s ? 1 : 0 ) : s;
		return;
	}

	putI( "selected", forceBoolean ? ( s ? 1 : 0 ) : s );
}

void ZUIButton::checkSendSelectedMsg() {
	int selected = getSelected();
	if( selected ) {
		int sent = getI( "sentSelectedMsg" );
		if( !sent ) {
			putI( "sentSelectedMsg", 1 );
			putI( "sentUnselectedMsg", 0 );
			zMsgQueue( "%s", getS("sendMsgOnSelect") );
		}
	}
	else {
		int sent = getI( "sentUnselectedMsg" );
		if( !sent ) {
			putI( "sentUnselectedMsg", 1 );
			putI( "sentSelectedMsg", 0 );
			zMsgQueue( "%s", getS("sendMsgOnUnselect") );
		}
	}
}

void ZUIButton::renderCheck() {
	glPushAttrib( GL_CURRENT_BIT );
	glDisable( GL_BLEND );
	extern int useDirtyRects;
	setupColor( "buttonCheckColor" );
	const float checkL = 0.f;
	const float checkM = 5.f;
	const float checkR = 14.f;
	const float checkBot = 4.f;
	const float checkTop = 19.f;
	const float checkMY = 8.f;
	const float checkLY = 9.f;
	glBegin( GL_POLYGON );
		glVertex2f( checkM, checkBot );
		glVertex2f( checkR, checkTop );
		glVertex2f( checkM, checkMY );
		glVertex2f( checkL, checkLY );
	glEnd();
	glPopAttrib();
}

void ZUIButton::renderButtonBase( float w, float h, int square ) {
	const float buttonRampT = 1.84f;
	const float buttonRampM = 1.22f;
	const float buttonRampB = 1.24f;

	int selected = getSelected();
	if( getI("disabled") ) {
		setupColor( "buttonDisabledColor" );
	}
	else if( getI("pressed") ) {
		setupColor( "buttonPressedColor" );
	}
	else if( selected ) {
		setupColor( "buttonSelectedColor" );
	}
	else {
		setupColor( "buttonColor" );
	}


	float color[4];
	glGetFloatv( GL_CURRENT_COLOR, color );

	float radius = h/2.f;
	int numPts = 8;
	float thetaStep = 3.14f / (float)(numPts-1);
	int numVerts = 0;
	float verts[64][2];
	int i;

	float theta;
	if( square ) {
		float step = h / (float)(numPts-1);
		float y = h;
		for( i=0; i<numPts; i++, y -= step) {
			verts[numVerts  ][0] = 0.f;
			verts[numVerts++][1] = y;
		}
	}
	else {
		float theta = 3.14f / 2.f;
		for( i=0; i<numPts; i++, theta += thetaStep ) {
			verts[numVerts  ][0] = radius*cosf(theta) + radius;
			verts[numVerts++][1] = radius*sinf(theta) + radius;
		}
	}

	if( square ) {
		float step = h / (float)(numPts-1);
		float y = 0.f;
		for( i=0; i<numPts; i++, y += step ) {
			verts[numVerts  ][0] = w - 1;
			verts[numVerts++][1] = y;
		}
	}
	else {
		theta = 3.f / 2.f * 3.14f;
		for( i=0; i<numPts; i++, theta += thetaStep ) {
			verts[numVerts  ][0] = radius*cosf(theta) + w - radius - 1;
			verts[numVerts++][1] = radius*sinf(theta) + radius;
		}
	}

	float half = (float)numPts / 2.f;
	glBegin( GL_QUAD_STRIP );
		for( i=0; i<numPts; i++ ) {
			float colorRamp = 1.f;
			if( i < numPts/2 ) {
				colorRamp = (float)i / half * (buttonRampM - buttonRampT) + buttonRampT;
			}
			else {
				colorRamp = ((float)i - half) / half * (buttonRampB - buttonRampM) + buttonRampM;
			}
			glColor4f( color[0]*colorRamp, color[1]*colorRamp, color[2]*colorRamp, color[3] );

			glVertex2fv( verts[i] );
			glVertex2fv( verts[numVerts-i-1] );
		}
	glEnd();

	glLineWidth( 1.f );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glColor4ub( 0, 0, 0, 0xa0 );
	glBegin( GL_LINE_STRIP );
	for( i=0; i<numPts; i++ ) {
		glVertex2f( verts[i][0], verts[i][1] );
	}
	glEnd();
	glBegin( GL_LINE_STRIP );
	for( i=numPts; i<numVerts; i++ ) {
		glVertex2f( verts[i][0], verts[i][1] );
	}
	glEnd();

	// There was a weird line bug where lines don't exactly perfectly
	// line up with a quad drawn at the same place so I converted the
	// top and bottoms lines to quads to get it to look nicer
	glBegin( GL_QUADS );
		glVertex2f( verts[numPts-1][0], verts[numPts-1][1] );
		glVertex2f( verts[numPts][0], verts[numPts][1] );
		glVertex2f( verts[numPts][0], verts[numPts][1]+1 );
		glVertex2f( verts[numPts-1][0], verts[numPts-1][1]+1 );

		glVertex2f( verts[numVerts-1][0], verts[numVerts-1][1]-1 );
		glVertex2f( verts[0][0], verts[0][1]-1 );
		glVertex2f( verts[0][0], verts[0][1]+0 );
		glVertex2f( verts[numVerts-1][0], verts[numVerts-1][1]+0 );
	glEnd();
}

void ZUIButton::render() {
	int selected = getSelected();
	checkSendSelectedMsg();

	renderButtonBase( w, h );

	char *text = getText();

	float winX=0.f, winY=0.f;
	getWindowXY( winX, winY );
	glPushAttrib( GL_SCISSOR_BIT );
	glEnable(GL_SCISSOR_TEST);

	int viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );
	scissorIntersect( viewport[0] + (int)winX+2, viewport[1] + (int)winY, (int)w-6, (int)h );

	void *fontPtr = 0;
	if( has("buttonFont") ) {
		fontPtr = zglFontGet( getS("buttonFont") );
	}
	else {
		fontPtr = zglFontGet( getS("font") );
	}

	float tw, th, td;
	zglFontGetDim( text, tw, th, td, 0, fontPtr );

	float checkMarkOffset = 0.f;

	float kw=0.f, kh=0.f, kd=0.f;
	char key[64]={0,};
	char *s = getS( "keyBinding", "" );
	if( *s ) {
		char *d = &key[2];
		for( ; *s; s++ ) {
			if( *s != '.' ) {
				*d++ = *s;
			}
		}
		*d = 0;
		if( key[2] ) {
			key[0] = ' ';
			key[1] = '(';
			int len = strlen(key);
			key[len] = ')';
			key[len+1] = 0;
			assert( len < sizeof(key)-2 );
			zglFontGetDim( key, kw, kh, kd, 0, fontPtr );
		}
	}
	float x = (w-(tw+kw))/2.f + checkMarkOffset;
	float y = ((h-th)/2.f)+td-1;

	if( getI("disabled") ) {
		setupColor( "buttonDisabledTextColor" );
	}
	else {
		setupColor( "buttonTextColor" );
	}
	zglFontPrint( text, x, y, 0, 0, fontPtr );

	// @TODO: Deal with word wrap properly
	if( key[0] ) {
		if( getI("keyState") ) {
			setColorI( 0xFF0000FF );
		}
		else {
			if( getI("disabled") ) {
				setupColor( "buttonDisabledTextColor" );
			}
			else {
				setupColor( "buttonTextColor" );
			}
		}
		zglFontPrint( key, x+tw, y, 0, 0, fontPtr );
	}

	if( getI("pressed") ) {
		if( getI("disabled") ) {
			setupColor( "buttonDisabledTextColor" );
		}
		else {
			setupColor( "buttonTextColor" );
		}
		zglFontPrint( text, x+1.f, y, 0, 0, fontPtr );

		if( key[1] ) {
			zglFontPrint( key, x+1.f+tw, y, 0, 0, fontPtr );
		}
	}
	glPopAttrib();
}

ZUI::ZUILayoutCell *ZUIButton::layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly ) {
	char text[256];
	strcpy( text, getText() );

	float kw=0.f, kh=0.f, kd=0.f;
	char key[64];
	char *s = getS( "keyBinding", "" );
	if( *s ) {
		char *d = key;
		for( ; *s; s++ ) if( *s != '.' ) *d++ = *s;
		*d = 0;
		strcat( text, " (" );
		strcat( text, key );
		strcat( text, ")" );
		assert( strlen(text) < sizeof(text)-1 );
	}

	float d;
	void *fontPtr = zglFontGet( getS("font") );
	zglFontGetDim( text, w, h, d, 0, fontPtr );
	
	w += 16.f;
	h += 8.f;

	return 0;
}

void ZUIButton::handleKeyBinding( ZMsg *msg ) {
	if( has("keyBinding") ) {
		char *key = zmsgS( which );
		char *keyBinding = strdup( getS( "keyBinding" ) );
		if( strchr( keyBinding, '.' ) ) {
			// When a partial selection has happened, for example, imagine
			// that the keybinding is "5-6".  When the "5" is pressed
			// the keyState is set to 1 to mean that the first level
			// has been selected.
			// This is accompanied by a call to the static GUI function
			// which tracks who has a pending binding.  The Gui dispatcher
			// will then only send Key events to those who have
			// a pending call.
			// All GUIs must release if they don't match

			// PARSE the keyBinding and match it to the key depending on the keyState
			int keyState = getI("keyState");

			int found = 0;
			int count = 0;
			char *split = strtok( keyBinding, "." );
			while( split ) {
				if( count++ == keyState ) {
					if( !stricmp(split,key) ) {
						// We match this key, so bind to the modal key events
						found = 1;
					}
				}
				split = strtok( NULL, "." );
			}

			// At this point count tells us how many elems there were, for example "1-2" there are 2

			if( found ) {
				if( keyState == count-1 ) {
					// We are the matching signal, send message
					//putI( "pressed", 1 );
					zMsgQueue( "type=ZUIButton_clearPressed toZUI=%s __countdown__=1", name );
					if( getI( "toggle" ) ) {
						setSelected( !getSelected() );
					}
					zMsgQueue( "%s", getS( "sendMsg" ) );
					modalKeysClear();
					putI( "keyState", 0 );
					zMsgUsed();
				}
				else {
					modalKeysBind( this );
					putI( "keyState", keyState+1 );
				}
			}
			else {
				if( keyState > 0 ) {
					putI( "keyState", 0 );
					modalKeysUnbind( this );
				}
			}

			free( keyBinding );
		}

		else if( !stricmp(keyBinding,key) ) {
			//putI( "pressed", 1 );
			zMsgQueue( "type=ZUIButton_clearPressed toZUI=%s __countdown__=1", name );
			if( getI( "toggle" ) ) {
				setSelected( !getSelected() );
			}
			zMsgQueue( "%s", getS( "sendMsg" ) );
		}
	}
}

void ZUIButton::setPressed( int pressed ) {
// Set the "pressed" state, and send down/up messages as appropriate for any change.
// Sending all changes to pressed state via this func should ensure that down/up messages are properly sent.
	int prev_pressed = getI( "pressed" );
	putI( "pressed", pressed );
	if( pressed != prev_pressed ) {
	    if( pressed ) {
			if( has("sendMsgOnDown") ) {
				zMsgQueue( "%s", getS( "sendMsgOnDown" ) );
			}
		}
		else {
			if( has("sendMsgOnUp") ) {
				zMsgQueue( "%s", getS( "sendMsgOnUp" ) );
			}
		}	    
	}
}

void ZUIButton::handleMsg( ZMsg *msg ) {
	if( zmsgIs(type,ZUIEnable) ) {
		if( !(zmsgHas(toZUI) || zmsgHas(toZUIGroup)) ) {
			return;
		}
		putI("disabled",0);
		layoutRequest();
	}

	if( zmsgIs(type,ZUIDisable) ) {
		if( !(zmsgHas(toZUI) || zmsgHas(toZUIGroup)) ) {
			return;
		}
		putI("disabled",1);
		layoutRequest();
	}

	if( getI("disabled") ) {
		ZUI::handleMsg( msg );
		return;
	}

	if( zmsgIs(type,ZUISelect) ) {
		if( !(zmsgHas(toZUI) || zmsgHas(toZUIGroup)) ) {
			return;
		}
		setSelected(1);
		layoutRequest();
	}

	if( zmsgIs(type,ZUIDeselect) ) {
		if( !(zmsgHas(toZUI) || zmsgHas(toZUIGroup)) ) {
			return;
		}
		setSelected(0);
		layoutRequest();
	}

	if( zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L) ) {
		zMsgUsed();
		requestExclusiveMouse( 0, 1 );
		setPressed( 1 );
		return;
	}
	else if( zmsgIs(type,ZUIMouseEnter) && zmsgI(dragging) ) {
		setPressed( 1 );
	}
	else if( zmsgIs(type,ZUIMouseLeave) && zmsgI(dragging) ) {
		setPressed( 0 );
	}
	else if( zmsgIs(type,ZUIMouseReleaseDrag) ) {
		if( getI("pressed") ) {
			int sendMsg=1;
			if( getI( "toggle" ) ) {
				if( !getI( "sticky" ) ) {
					setSelected( !getSelected() );
				}
				else {
					sendMsg=0;
					if( !getSelected() ) {
						setSelected( 1 );
						sendMsg=1;
					}
				}
			}
			if( sendMsg && has("sendMsg") ) {
				zMsgQueue( "%s", getS( "sendMsg" ) );
			}
		}
		setPressed( 0 );
		requestExclusiveMouse( 0, 0 );
	}
	else if( zmsgIs(type,ZUIModalKeysClear) ) {
		putI( "keyState", 0 );
	}
	else if( zmsgIs(type,Key) ) {
		handleKeyBinding( msg );
	}
	else if( zmsgIs(type,ZUIButton_clearPressed) ) {
		setPressed( 0 );
	}

	ZUI::handleMsg( msg );
}

