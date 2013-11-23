// @ZBS {
//		*COMPONENT_NAME zuicolor
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuicolor.cpp
// }

// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#ifdef __APPLE__
#include "OpenGL/gl.h"
#else
#include "GL/gl.h"
#endif
// STDLIB includes:
#include "string.h"
#include "stdio.h"
#include "math.h"
// MODULE includes:
// ZBSLIB includes:
#include "zrgbhsv.h"
#include "zmathtools.h"
#include "zui.h"

// ZUIColor
//==================================================================================

struct ZUIColor : ZUI {
	ZUI_DEFINITION(ZUIColor,ZUI);

	// SPECIAL ATTRIBUTES:
	// h : hue (0..1)
	// s : saturation (0..1)
	// v : value (0..1)
	// a : alpha (0..1)
	// sendMsg : what to send when changes.  h,s,v,r,g,b,a are all added on to the message
	// ZUIColor_SetColor : send to it to set a color in either rgba or hsva

	int draggingV;
	int draggingA;

	void sendMsg();

	virtual void factoryInit();
	virtual void render();
	virtual ZUI::ZUILayoutCell *layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly );
	virtual void handleMsg( ZMsg *msg );
};

ZUI_IMPLEMENT(ZUIColor,ZUI);

static const float rad0 = 0.85f;
static const float rad1 = 1.0f;

void ZUIColor::factoryInit() {
	ZUI::factoryInit();
	putF( "val", 0.5f );
	putF( "sat", 0.5f );
	putF( "hue", 0.5f );
	putF( "alpha", 1.f );
	draggingV = 0;
}

void ZUIColor::render() {
	float r, g, b;
	float val = getF( "val" );
	float alpha = getF( "alpha" );
	zHSVF_to_RGBF( 0.f, 0.f, val, r, g, b );

	float wid, hei;
	getWH( wid, hei );
	float minorAxis = min( wid, hei );
	float xRad = minorAxis/2.f;
	float yRad = minorAxis/2.f;
	
	int i;
	glBegin( GL_TRIANGLE_FAN ); {
		glColor3f( r, g, b );
		glVertex2f( xRad, yRad );
		float t=0.f, tStep = PI2F / 32.f;
		for( i=0; i<=32; i++, t+=tStep ) {
			float tt = i == 32 ? 0.f : t / PI2F;
			zHSVF_to_RGBF( tt, 1.f, val, r, g, b );
			glColor3f( r, g, b );
			glVertex2f( xRad + rad0*xRad*(float)cosf(t), yRad + rad0*yRad*(float)sinf(t) );
		}
	}
	glEnd();

	glBegin( GL_QUAD_STRIP );
		float tStep = PIF / 32.f;
		float t=0.f;
		for( i=0; i<=32; i++, t+=tStep ) {
			glColor3f( (float)i/32.f, (float)i/32.f, (float)i/32.f );
			glVertex2f( xRad + rad0*xRad*(float)cosf(t), yRad + rad0*yRad*(float)sinf(t) );
			glVertex2f( xRad + rad1*xRad*(float)cosf(t), yRad + rad1*yRad*(float)sinf(t) );
		}
	glEnd();

	glBegin( GL_QUAD_STRIP );
		tStep = PIF / 32.f;
		t=PIF;
		for( i=0; i<=32; i++, t+=tStep ) {
			glColor3f( (float)i/32.f, (float)i/32.f, (float)i/32.f );
			glVertex2f( xRad + rad0*xRad*(float)cosf(t), yRad + rad0*yRad*(float)sinf(t) );
			glVertex2f( xRad + rad1*xRad*(float)cosf(t), yRad + rad1*yRad*(float)sinf(t) );
		}
	glEnd();

	glLineWidth( 2.f );
	glBegin( GL_LINES );
		val *= PIF;
		glColor3ub( 255, 40, 40 );
		glVertex2f( xRad + rad0*xRad*(float)cosf(val), yRad + rad0*yRad*(float)sinf(val) );
		glVertex2f( xRad + rad1*xRad*(float)cosf(val), yRad + rad1*yRad*(float)sinf(val) );

		alpha *= PIF;
		alpha += PIF;
		glColor3ub( 255, 40, 40 );
		glVertex2f( xRad + rad0*xRad*(float)cosf(alpha), yRad + rad0*yRad*(float)sinf(alpha) );
		glVertex2f( xRad + rad1*xRad*(float)cosf(alpha), yRad + rad1*yRad*(float)sinf(alpha) );
	glEnd();

	float hue = getF("hue") * PI2F;
	float sat = getF("sat");
	val = getF( "val" );

	if( val > 0.5f ) {
		glColor3ub( 0, 0, 0 );
	}
	else {
		glColor3ub( 255, 255, 255 );
	}
	glPointSize( 4.f );
	glBegin( GL_POINTS );
		while( hue < 0.f ) hue += PI2F;
		while( hue > PI2F ) hue -= PI2F;
		sat = max( 0.f, min( 1.f, sat ) );
		glVertex2f( xRad + rad0*xRad*sat*(float)cosf(hue), yRad + rad0*yRad*sat*(float)sinf(hue) );
	glEnd();
}

ZUI::ZUILayoutCell *ZUIColor::layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly ) {
	w = 100;
	h = 100;
	return 0;
}

void ZUIColor::sendMsg() {
	if( has("sendMsg" ) ) {
		char buf[128];
		float r, g, b;
		zHSVF_to_RGBF( getF("hue"), getF("sat"), getF("val"), r, g, b);
		unsigned int hex = zRGBAF_to_RGBAI( r, g, b, getF("alpha") );
		sprintf( buf, "h=%f s=%f v=%f r=%f g=%f b=%f a=%f rgbaHex=0x%X", getF("hue"), getF("sat"), getF("val"), r, g, b, getF("alpha"), hex );
		zMsgQueue( "%s %s", getS("sendMsg"), buf );
	}
}

void ZUIColor::handleMsg( ZMsg *msg ) {
	int clicked = 0;
	float wid, hei;
	getWH( wid, hei );
	float hue=0.f, sat=0.f, val=0.f, alpha=0.f;
	if( zmsgIs(type,ZUIMouseClickOn) && zmsgIs(which,L) && zmsgIs(dir,D) ) {
		float minorAxis = min( wid, hei );
		float x = zmsgF(localX) - minorAxis/2.f;
		float y = zmsgF(localY) - minorAxis/2.f;
		x /= minorAxis/2.f;
		y /= minorAxis/2.f;
		float s = (float)sqrt(x*x + y*y);
		draggingV = 0;
		draggingA = 0;
		if( s > rad0 ) {
			if( zmsgF(localY) > hei/2.f ) {
				draggingV = 1;
			}
			else {
				draggingA = 1;
			}
		}
		requestExclusiveMouse( draggingV || draggingA, 1 );
		sendMsg();
		clicked = 1;
		zMsgUsed();
	}
	if( clicked || zmsgIs(type,ZUIExclusiveMouseDrag) ) {
		float minorAxis = min( wid, hei );
		float x = zmsgF(localX) - minorAxis/2.f;
		float y = zmsgF(localY) - minorAxis/2.f;
		if( draggingV ) {
			val = (float)atan2( y, x );
			if( val < 0.f && val > -PIF/2.f) val = 0.f;
			if( val < -PIF/2.f ) val = PIF;
			val /= PIF;
			putF( "val", val );
		}
		else if( draggingA ) {
			alpha = (float)atan2( y, x );
			if( alpha > 0.f && alpha < PIF/2.f) alpha = 0.f;
			if( alpha > PIF/2.f ) alpha = -PIF;
			alpha /= -PIF;
			alpha = 1.f - alpha;
			putF( "alpha", alpha );
		}
		else {
			hue = (float)atan2( y, x );
			while( hue > PI2F ) hue -= PI2F;
			while( hue < 0.f ) hue += PI2F;
			putF( "hue", hue/PI2F );
			x /= minorAxis/2.f * rad0;
			y /= minorAxis/2.f * rad0;
			sat = (float)sqrt(x*x + y*y);
			sat = min( 1.f, sat );
			putF( "sat", sat );
		}
		sendMsg();
		zMsgUsed();
	}
	else if( zmsgIs(type,ZUIMouseReleaseDrag) ) {
		requestExclusiveMouse( 0, 0 );
		zMsgUsed();
	}
	else if( zmsgIs(type,ZUIColor_SetColor) ) {
		if( zmsgHas(r) )
		{
			zRGBF_to_HSVF( zmsgF(r), zmsgF(g), zmsgF(b), hue, sat, val );
			putF( "hue", hue );
			putF( "sat", sat );
			putF( "val", val );
		}
		else if ( zmsgHas(hue) )
		{
			putF( "hue", zmsgF(h) );
			putF( "sat", zmsgF(s) );
			putF( "val", zmsgF(v) );
		}
		else if( zmsgHas(colorI) ) {
			float r,g,b,a;
			zRGBAI_to_RGBAF( zmsgI(colorI), r, g, b, a );
			zRGBF_to_HSVF( r, g, b, hue, sat, val );
			putF( "hue", hue );
			putF( "sat", sat );
			putF( "val", val );
			putF( "alpha", a );
		}
		if ( zmsgHas(a) ) {
			putF( "alpha", zmsgF(a) );
		}
		sendMsg();
	}
}

