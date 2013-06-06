// @ZBS {
//		*COMPONENT_NAME zuislider
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuislider.cpp zuislider.h
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
#include "zuislider.h"
// ZBSLIB includes:
#include "ztmpstr.h"

#ifndef min
	#define min(a,b) ( ( (a) < (b) ) ? (a) : (b) )
#endif
#ifndef max
	#define max(a,b) ( ( (a) > (b) ) ? (a) : (b) )
#endif

// ZUISlider
//==================================================================================

ZUI_IMPLEMENT(ZUISlider,ZUIPanel);

void ZUISlider::factoryInit() {
	ZUIPanel::factoryInit();
	putS( "mouseRequest", "over" );
	putI( "min", 100 );
	putI( "max", 100 );
}

ZUI::ZUILayoutCell *ZUISlider::layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly ) {
	w = 20.f;
	h = 16.f;
	return 0;
}

void ZUISlider::render() {
	ZUIPanel::render();

	float val = getF("val");
	float _max = getF( "max" );
	float _min = getF( "min" );
	float denom = _max-_min;
	float p = 0.f;
	if( denom != 0.f ) {
		p = (val-_min) / (_max-_min);
	}

	float lastVal = getF("lastVal");
	if( val != lastVal ) {
		if( has("sendMsg") ) {
			zMsgQueue( "%s val=%f", getS("sendMsg"), val );
		}
		putF( "lastVal", val );
	}

	setupColor( "sliderLineColor" );
	float w, h;
	getWH( w, h );
	float opposite = w < h ? w/2.f : h/2.f;
	float s = getF( "size", opposite );
	float x, y;
	if( w < h ) {
		x = w/2.f;
		y = p * (h-s*2.f) + s;
		glBegin( GL_LINES );
			glVertex2f( w/2.f, s );
			glVertex2f( w/2.f, h-s );
		glEnd();
	}
	else {
		x = p * (w-s*2.f) + s;
		y = h/2.f;
		glBegin( GL_LINES );
			glVertex2f( s, h/2.f );
			glVertex2f( w-s, h/2.f );
		glEnd();
	}

	setupColor( "sliderHandleColor" );
	glBegin( GL_QUADS );
		glVertex2f( x, y-s );
		glVertex2f( x-s, y );
		glVertex2f( x, y+s );
		glVertex2f( x+s, y );
	glEnd();
}

void ZUISlider::handleMsg( ZMsg *msg ) {
	if( zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L) ) {
		requestExclusiveMouse( 1, 1 );
		zMsgUsed();
		return;
	}
	else if( zmsgIs(type,ZUIMouseReleaseDrag) ) {
		requestExclusiveMouse( 1, 0 );
		zMsgUsed();
		return;
	}
	else if( zmsgIs(type,ZUIExclusiveMouseDrag) && zmsgI(l) ) {
		float w, h;
		getWH( w, h );
		float pos = 0.f;
		if( w < h ) {
			pos = zmsgF(localY) / h;
		}
		else {
			pos = zmsgF(localX) / w;
		}
		pos = max( 0.f, min( 1.f, pos ) );
		float val = pos*(getF("max")-getF("min")) + getF("min");
		putF( "val", val );
		dirty(); 
	}
	ZUIPanel::handleMsg( msg );
}
