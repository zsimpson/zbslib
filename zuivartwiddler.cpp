// @ZBS {
//		*COMPONENT_NAME zuivaredit
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuivaredit.cpp
// }

// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#include "GL/gl.h"
#include "zglfont.h"
// STDLIB includes:
#include "assert.h"
#include "math.h"
#include "string.h"
#include "stdlib.h"
// MODULE includes:
#include "zuivaredit.h"
// ZBSLIB includes:
#include "ztmpstr.h"
#include "zvars.h"
#include "zregexp.h"

#ifdef __USE_GNU
#define stricmp strcasecmp
#endif

// ZUIVarTwiddler
//==================================================================================

struct ZUIVarTwiddler : ZUI {
	ZUI_DEFINITION(ZUIVarTwiddler,ZUI);

	// SPECIAL ATTRIBUTES:
	//  ptr : a pointer to the value to be modified expressed as an integer
	//  ptrType : int | short | float | double
	//  value : a double value used if ptr and ptrType are not set
	//  lBound : lower bound [none]
	//  uBound : upper bound [none]
	//  twiddle : linear | exponential
	//  sendMsg: What to send when the variable changes value
	// SPECIAL MESSAGES:

	// @TODO: Convert these into the hash?
	int mouseOver;
	int selected;
	float selectX, selectY;
	float mouseX, mouseY;
	double startVal;

	double getToDouble( );
	void setFromDouble( double v );

	virtual void factoryInit();
	virtual void render();
	virtual void handleMsg( ZMsg *msg );
	virtual ZUI::ZUILayoutCell *layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly );
};

ZUI_IMPLEMENT(ZUIVarTwiddler,ZUI);


double ZUIVarTwiddler::getToDouble() {
	void *ptr = (void *)getI("ptr");
	if( ptr ) {
		char *ptrType = getS( "ptrType" );
		if( !strcmp(ptrType,"int") ) {
			return (double)*(int *)ptr + 0.4; 
		}
		else if( !strcmp(ptrType,"short") ) {
			return (double)*(short *)ptr + 0.4;
		}
		else if( !strcmp(ptrType,"float") ) {
			return (double)*(float *)ptr;
		}
		else if( !strcmp(ptrType,"double") ) {
			return (double)*(double *)ptr;
		}
		return 0.0;
	}
	// Else
	return getD( "value" );
}

void ZUIVarTwiddler::setFromDouble( double v ) {
	if( has("lBound") ) {
		v = max( getD( "lBound" ), v );
	}
	if( has("uBound") ) {
		v = min( getD( "uBound" ), v );
	}

	void *ptr = (void *)getI("ptr");
	if( ptr ) {
		char *ptrType = getS( "ptrType" );
		if( !strcmp(ptrType,"int") ) {
			*(int *)ptr = (int)v;
		}
		else if( !strcmp(ptrType,"short") ) {
			*(short *)ptr = (short)v;
		}
		else if( !strcmp(ptrType,"float") ) {
			*(float *)ptr = (float)v;
		}
		else if( !strcmp(ptrType,"double") ) {
			*(double *)ptr = (double)v;
		}
	}
	else {
		putD( "value", v );
	}
}

void ZUIVarTwiddler::factoryInit() {
	ZUI::factoryInit();
	selected = 0;
	mouseOver = -1;
	putS( "mouseRequest", "over" );
	putS( "noScissor", "1" );
}

void ZUIVarTwiddler::render() {
	glColor3ub( 255, 0, 0 );

	float myW, myH;
	getWH( myW, myH );
	float cenX = myW / 2.f;
	float cenY = myH / 2.f;
	float rad = cenX;

	if( selected ) {
		glBegin( GL_TRIANGLE_FAN );
		glVertex2f( cenX, cenY );
	}
	else {
		glBegin( GL_POINTS );
	}

	float t=0.f, td = 3.1415926539f/16.f;
	for( int i=0; i<=16; i++, t+=td ) {
		glVertex2f( cenX + rad*(float)cos(t), cenY + rad*(float)sin(t) );
	}

	glEnd();

	if( selected ) {
		glBegin( GL_LINES );
			glVertex2f( selectX, selectY );
			glVertex2f( selectX, mouseY );

			float sign = mouseY > selectY ? -1.f : 1.f;
			glVertex2f( selectX, mouseY );
			glVertex2f( selectX-5, mouseY + sign*5 );

			glVertex2f( selectX, mouseY );
			glVertex2f( selectX+5, mouseY + sign*5 );
		glEnd();
	}
}

void ZUIVarTwiddler::handleMsg( ZMsg *msg ) {
	float x = zmsgF(localX);
	float y = zmsgF(localY);

	if( zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L) ) {
		putI( "cursor", -1 );
		selected = 1;
		selectX = x;
		selectY = y;
		mouseX = x;
		mouseY = y;
		startVal = getToDouble();
		if( startVal == 0.0 ) startVal = 1e-1;
		requestExclusiveMouse( 1, 1 );
		zMsgUsed();
		return;
	}
	else if( zmsgIs(type,ZUIMouseReleaseDrag) ) {
		selected = 0;
		requestExclusiveMouse( 1, 0 );
		zMsgUsed();
		return;
	}
	else if( zmsgIs(type,ZUIExclusiveMouseDrag) ) {
		mouseX = x;
		mouseY = y;
		if( isS("twiddle","linear") ) {
			double delta = (mouseY-selectY) / 5.0;
			setFromDouble( startVal + delta );
		}
		else {
			double scale = exp( (mouseY-selectY) / 50.0 );
			double val = startVal * scale;
			if( val == 0.0 ) val = 1e-1;
			setFromDouble( val );
		}
	}
	ZUI::handleMsg( msg );
}

ZUI::ZUILayoutCell *ZUIVarTwiddler::layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly ) {
	w = 10;
	h = 10;
	return 0;
}
