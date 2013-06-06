// @ZBS {
//		*COMPONENT_NAME guicolor
//		*MODULE_OWNER_NAME glgui
//		*GROUP modules/gui
//		*REQUIRED_FILES guicolor.h
// }
// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#include "GL/gl.h"
// STDLIB includes:
#include "string.h"
#include "stdio.h"
#include "math.h"
// MODULE includes:
#include "guicolor.h"
// ZBSLIB includes:
#include "zrgbhsv.h"
#include "zmathtools.h"

GUI_TEMPLATE_IMPLEMENT(GUIColor);

static const float rad0 = 0.85f;
static const float rad1 = 1.0f;

void GUIColor::init() {
	GUIObject::init();
	setAttrF( "v", 0.5f );
	setAttrF( "s", 0.5f );
	setAttrF( "h", 0.5f );
	setAttrF( "a", 1.f );
	draggingV = 0;
}

void GUIColor::render() {
	float r, g, b;
	float v = getAttrF( "v" );
	float a = getAttrF( "a" );
	zHSVF_to_RGBF( 0.f, 0.f, v, r, g, b );

	float minorAxis = min( myW, myH );
	float xRad = minorAxis/2.f;
	float yRad = minorAxis/2.f;
	
	glBegin( GL_TRIANGLE_FAN ); {
		glColor3f( r, g, b );
		glVertex2f( xRad, yRad );
		float t=0.f, tStep = PI2F / 32.f;
		for( int i=0; i<=32; i++, t+=tStep ) {
			float tt = i == 32 ? 0.f : t / PI2F;
			zHSVF_to_RGBF( tt, 1.f, v, r, g, b );
			glColor3f( r, g, b );
			glVertex2f( xRad + rad0*xRad*(float)cos(t), yRad + rad0*yRad*(float)sin(t) );
		}
	}
	glEnd();

	glBegin( GL_QUAD_STRIP );
		float tStep = PIF / 32.f;
		float t=0.f;
		for( int i=0; i<=32; i++, t+=tStep ) {
			glColor3f( (float)i/32.f, (float)i/32.f, (float)i/32.f );
			glVertex2f( xRad + rad0*xRad*(float)cos(t), yRad + rad0*yRad*(float)sin(t) );
			glVertex2f( xRad + rad1*xRad*(float)cos(t), yRad + rad1*yRad*(float)sin(t) );
		}
	glEnd();

	glBegin( GL_QUAD_STRIP );
		tStep = PIF / 32.f;
		t=PIF;
		for( i=0; i<=32; i++, t+=tStep ) {
			glColor3f( (float)i/32.f, (float)i/32.f, (float)i/32.f );
			glVertex2f( xRad + rad0*xRad*(float)cos(t), yRad + rad0*yRad*(float)sin(t) );
			glVertex2f( xRad + rad1*xRad*(float)cos(t), yRad + rad1*yRad*(float)sin(t) );
		}
	glEnd();

	glLineWidth( 2.f );
	glBegin( GL_LINES );
		v *= PIF;
		v += 0.f;
		glColor3ub( 255, 40, 40 );
		glVertex2f( xRad + rad0*xRad*(float)cos(v), yRad + rad0*yRad*(float)sin(v) );
		glVertex2f( xRad + rad1*xRad*(float)cos(v), yRad + rad1*yRad*(float)sin(v) );

		a *= PIF;
		a += PIF;
		glColor3ub( 255, 40, 40 );
		glVertex2f( xRad + rad0*xRad*(float)cos(a), yRad + rad0*yRad*(float)sin(a) );
		glVertex2f( xRad + rad1*xRad*(float)cos(a), yRad + rad1*yRad*(float)sin(a) );
	glEnd();

	float h = getAttrF("h") * PI2F;
	float s = getAttrF("s");
	v = getAttrF( "v" );

	if( v > 0.5 ) {
		glColor3ub( 0, 0, 0 );
	}
	else {
		glColor3ub( 255, 255, 255 );
	}
	glPointSize( 4.f );
	glBegin( GL_POINTS );
		while( h < 0.f ) h += PI2F;
		while( h > PI2F ) h -= PI2F;
		s = max( 0.f, min( 1.f, s ) );
		glVertex2f( xRad + rad0*xRad*s*(float)cos(h), yRad + rad0*yRad*s*(float)sin(h) );
	glEnd();
}

void GUIColor::queryOptSize( float &w, float &h ) {
	w = 100;
	h = 100;
}

void GUIColor::sendMsg() {
	if( hasAttr("sendMsg" ) ) {
		char buf[128];
		float r, g, b;
		zHSVF_to_RGBF( getAttrF("h"), getAttrF("s"), getAttrF("v"), r, g, b);
		unsigned int hex = zRGBAF_to_RGBAI( r, g, b, getAttrF("a") );
		sprintf( buf, "h=%f s=%f v=%f r=%f g=%f b=%f a=%f rgbaHex=0x%X", getAttrF("h"), getAttrF("s"), getAttrF("v"), r, g, b, getAttrF("a"), hex );
		zMsgQueue( "%s %s", getAttrS("sendMsg"), buf );
	}
}

void GUIColor::handleMsg( ZMsg *msg ) {
	int clicked = 0;
	if( zmsgIs(type,MouseClickOn) && zmsgIs(which,L) && zmsgIs(dir,D) ) {
		float minorAxis = min( myW, myH );
		float x = zmsgF(localX) - minorAxis/2.f;
		float y = zmsgF(localY) - minorAxis/2.f;
		x /= minorAxis/2.f;
		y /= minorAxis/2.f;
		float s = (float)sqrt(x*x + y*y);
		draggingV = 0;
		draggingA = 0;
		if( s > rad0 ) {
			if( zmsgF(localY) > myH/2.f ) {
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
	if( clicked || zmsgIs(type,MouseDrag) ) {
		float minorAxis = min( myW, myH );
		float x = zmsgF(localX) - minorAxis/2.f;
		float y = zmsgF(localY) - minorAxis/2.f;
		if( draggingV ) {
			float v = (float)atan2( y, x );
			if( v < 0.f && v > -PIF/2.f) v = 0.f;
			if( v < -PIF/2.f ) v = PIF;
			v /= PIF;
			setAttrF( "v", v );
		}
		else if( draggingA ) {
			float a = (float)atan2( y, x );
			if( a > 0.f && a < PIF/2.f) a = 0.f;
			if( a > PIF/2.f ) a = -PIF;
			a /= -PIF;
			a = 1.f - a;
			setAttrF( "a", a );
		}
		else {
			float h = (float)atan2( y, x );
			while( h > PI2F ) h -= PI2F;
			while( h < 0.f ) h += PI2F;
			setAttrF( "h", h/PI2F );
			x /= minorAxis/2.f * rad0;
			y /= minorAxis/2.f * rad0;
			float s = (float)sqrt(x*x + y*y);
			s = min( 1.f, s );
			setAttrF( "s", s );
		}
		sendMsg();
	}
	else if( zmsgIs(type,MouseReleaseDrag) ) {
		requestExclusiveMouse( 0, 0 );
		zMsgUsed();
	}
	else if( zmsgIs(type,GUIColor_SetColor) ) {
		if( zmsgHas(r) )
		{
			float h, s, v;
			zRGBF_to_HSVF( zmsgF(r), zmsgF(g), zmsgF(b), h, s, v );
			setAttrF( "h", h );
			setAttrF( "s", s );
			setAttrF( "v", v );
		}
		else if ( zmsgHas(h) )
		{
			setAttrF( "h", zmsgF(h) );
			setAttrF( "s", zmsgF(s) );
			setAttrF( "v", zmsgF(v) );
		}
		if ( zmsgHas(a) ) {
			setAttrF( "a", zmsgF(a) );
		}
		sendMsg();
	}
}

