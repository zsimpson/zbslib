// @ZBS {
//		*COMPONENT_NAME zuipanel
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuipanel.cpp
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
#include "assert.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
// MODULE includes:
#include "zui.h"
// ZBSLIB includes:
#include "zglfont.h"
#include "zmsg.h"
#include "zmathtools.h"

// STOPFLOW timing
#include "sfmod.h"

// ZUIPanel
//--------------------------------------------------------------------------------------------------------------------------------

ZUI_IMPLEMENT(ZUIPanel,ZUI);

void ZUIPanel::factoryInit() {
	ZUI::factoryInit();
	container = 1;
}

void ZUIPanel::render() {
	renderBase( 0.f, 0.f, w, h );
}

void ZUIPanel::renderBase( float _x, float _y, float _w, float _h ) {
	ZUI::render();
	setupColor( "panelColor" );

	glPushMatrix();
	glTranslatef( _x, _y, 0.f );

	int color0, color1, color2, color3;
	int useGradient = getI("panelUseColorGradient");
	if( has("panelColorGradient0") ) {
		color0 = getColorI( "panelColorGradient0" );
		color1 = getColorI( "panelColorGradient1" );
		color2 = getColorI( "panelColorGradient2" );
		color3 = getColorI( "panelColorGradient3" );
	}

	int frame = getI("panelFrame");
	SFTIME_START (PerfTime_ID_Zlab_render_panels, PerfTime_ID_Zlab_render_render);

	switch( frame ) {
		case 0: {
			glBegin( GL_QUADS );
				if( useGradient ) glColor4ubv( (unsigned char *)&color0 );
				glVertex2f( 0.f, 0.f );
				if( useGradient ) glColor4ubv( (unsigned char *)&color1 );
				glVertex2f( w, 0.f );
				if( useGradient ) glColor4ubv( (unsigned char *)&color2 );
				glVertex2f( w, h );
				if( useGradient ) glColor4ubv( (unsigned char *)&color3 );
				glVertex2f( 0.f, h );
			glEnd();
			break;
		}

		case 1: {
			// This is a simple outline frame
			glBegin( GL_QUADS );
				if( useGradient ) glColor4ubv( (unsigned char *)&color0 );
				glVertex2f( 0.f, 0.f );
				if( useGradient ) glColor4ubv( (unsigned char *)&color1 );
				glVertex2f( w, 0.f );
				if( useGradient ) glColor4ubv( (unsigned char *)&color2 );
				glVertex2f( w, h );
				if( useGradient ) glColor4ubv( (unsigned char *)&color3 );
				glVertex2f( 0.f, h );
			glEnd();

			glColor3ub( 0, 0, 0 );
			glBegin( GL_LINE_LOOP );
				glVertex2f( 0.f, 0.f );
				glVertex2f( _w-1, 0.f );
				glVertex2f( _w-1, _h-1 );
				glVertex2f( 0, _h-1 );
			glEnd();
			break;
		}

		case 2: {
			// This is a 3d sunken frame like in windows.  Needs some work
			glBegin( GL_QUADS );
				if( useGradient ) glColor4ubv( (unsigned char *)&color0 );
				glVertex2f( 1.f, 1.f );
				if( useGradient ) glColor4ubv( (unsigned char *)&color1 );
				glVertex2f( w-1, 1.f );
				if( useGradient ) glColor4ubv( (unsigned char *)&color2 );
				glVertex2f( w-1, _h-1 );
				if( useGradient ) glColor4ubv( (unsigned char *)&color3 );
				glVertex2f( 1.f, _h-1 );
			glEnd();

#ifndef SFFAST
			glPushAttrib( GL_ENABLE_BIT );
#endif
			glEnable( GL_BLEND );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			int alpha = 0xc0;
			glBegin( GL_LINE_STRIP );
				glColor4ub( 255, 255, 255, alpha );
				glVertex2f( 0.f, 0.f );
				glVertex2f( _w-1, 0.f );
				glVertex2f( _w-1, _h-1 );
				glColor4ub( 0, 0, 0, alpha );
				glVertex2f( _w-1, _h-1 );
				glVertex2f( 1.f, _h-1 );
				glVertex2f( 1.f, 1.f );
					// hack: why do the last two x coords need to be 1.f to cause the frame to butt up against the 
					// interior rect?  Otherwise you see a sliver of the background panel show through... (tfb)
					// see the experiment description in kintek, for example
			glEnd();
#ifndef SFFAST
			glPopAttrib();
#else
			glDisable( GL_BLEND );
#endif
			break;
		}

		case 3:
		case 4: {
			// These are 3d sunken and relief frame with rounded corners
#ifndef SFFAST
			glPushAttrib( GL_ENABLE_BIT | GL_LINE_BIT );
#endif
			float radius = 10.f;
			int numPts = 4;
			float thetaStep = 3.14f / 2.f / (float)(numPts-1);
			int numVerts = 0;
			float verts[64][2];
			int colors[64];

			int i;
			float theta = 3.14f;
			for( i=0; i<numPts; i++, theta += thetaStep ) {
				colors[numVerts] = color0;
				verts[numVerts  ][0] = radius*cosf(theta) + radius;
				verts[numVerts++][1] = radius*sinf(theta) + radius;
			}

			theta = 3.f / 2.f * 3.14f;
			for( i=0; i<numPts; i++, theta += thetaStep ) {
				colors[numVerts] = color1;
				verts[numVerts  ][0] = radius*cosf(theta) + _w - radius - 1;
				verts[numVerts++][1] = radius*sinf(theta) + radius;
			}

			theta = 0.f;
			for( i=0; i<numPts; i++, theta += thetaStep ) {
				colors[numVerts] = color2;
				verts[numVerts  ][0] = radius*cosf(theta) + _w - radius - 1;
				verts[numVerts++][1] = radius*sinf(theta) + _h - radius - 1;
			}

			theta = 3.14f / 2.f;
			for( i=0; i<numPts; i++, theta += thetaStep ) {
				colors[numVerts] = color3;
				verts[numVerts  ][0] = radius*cosf(theta) + radius;
				verts[numVerts++][1] = radius*sinf(theta) + _h - radius - 1;
			}

			glBegin( GL_TRIANGLE_FAN );
			for( i=0; i<numVerts; i++ ) {
				if( useGradient ) {
					glColor4ubv( (unsigned char *)&colors[i] );
				}
				glVertex2fv( verts[i] );
			}
			glEnd();

			glEnable( GL_BLEND );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			int alpha = 0x80;
			glLineWidth( 2.f );
			glEnable( GL_LINE_SMOOTH );
			glBegin( GL_LINE_STRIP );

			if( frame == 3 ) {
				glColor4ub( 0, 0, 0, alpha );
			}
			else {
				glColor4ub( 0xFF, 0xFF, 0xFF, alpha );
			}
			for( i=0; i<1; i++ ) {
				glVertex2fv( verts[i] );
			}

			if( frame == 3 ) {
				glColor4ub( 0xFF, 0xFF, 0xFF, alpha );
			}
			else {
				glColor4ub( 0, 0, 0, alpha );
			}
			for( ; i<numPts*2+1; i++ ) {
				glVertex2fv( verts[i] );
			}

			if( frame == 3 ) {
				glColor4ub( 0, 0, 0, alpha );
			}
			else {
				glColor4ub( 0xFF, 0xFF, 0xFF, alpha );
			}
			for( ; i<numVerts; i++ ) {
				glVertex2fv( verts[i] );
			}
			glVertex2fv( verts[0] );

			glEnd();
#ifndef SFFAST
			glPopAttrib();
#else
			glDisable( GL_BLEND );
			glDisable( GL_LINE_SMOOTH );
#endif
			break;
		}
	}
	SFTIME_END (PerfTime_ID_Zlab_render_panels);
	
	glPopMatrix();
}

extern void trace( char *msg, ... );
void ZUIPanel::handleMsg( ZMsg *msg ) {
	int permitScrollX   = getI("permitScrollX");
	int permitScrollY   = getI("permitScrollY");
	float maxScrollX	= getF( "maxScrollX" );
	float maxScrollY	= getF( "maxScrollY" );
	int permitTranslate = getI("permitTranslate");
	int permitDragDropInit  = getI("permitDragDropInit");
	int permitDragDropAccept= getI("permitDragDropAccept");

	if( zmsgIs( type, ZUIMouseClickOn ) ) {
		if( getI( "panelConsumeClicks" ) ) {
			zMsgUsed();
				// prevent "click-through" to covered UI on dialog boxes etc...
		}
		if( zmsgIs(which,L) && zmsgIs(dir,D) ) {
			if( permitDragDropInit ) {
				requestExclusiveMouse( 1, 1 );
				zMsgUsed();
			}
			char *sendMsg = getS( "sendMsg", 0 );
			if( sendMsg ) {
				zMsgQueue( sendMsg );
				zMsgUsed();
			}
			if( permitTranslate ) {
				dirty();
				requestExclusiveMouse( 1, 1 );
				putF( "translateStartMouseX", zmsgF(x) );
				putF( "translateStartMouseY", zmsgF(y) );
				zMsgUsed();
			}
			if( zMsgIsUsed() ) {
				return;
			}
		}
		else if( zmsgIs(dir,D) && zmsgIs(which,R) ) {
			int doScrollX = permitScrollX && maxScrollX > 0.f;
			int doScrollY = permitScrollY && maxScrollY > 0.f;
				// don't scroll if the contents aren't big enough to require it

			if( doScrollX || doScrollY ) {
				dirty();
				requestExclusiveMouse( 1, 1 );
				zMsgUsed();
			}
			if( doScrollX ) {
				putF( "scrollStartX", scrollX );
				putF( "scrollStartMouseX", zmsgF(x) );
			}
			if( doScrollY ) {
				putF( "scrollStartY", scrollY );
				putF( "scrollStartMouseY", zmsgF(y) );
			}
		}
		return;
	}
	else if( zmsgIs(type,Key) && zmsgI(on) ) {
		if( zmsgIs(which,wheelforward) ) {
			if( getI( "panelConsumeClicks" ) ) {
				zMsgUsed();
				// prevent "scroll-through" to covered UI on dialog boxes etc...
			}
#if !defined(STOPFLOW)
			if( permitScrollY && scrollY > 0 ) {
#else
			// stopflow is not yet properly set up for this scroll control
			if( permitScrollY ) {
#endif
					dirty();
					scrollY = max( floorf( getF("scrollYHome") ), floorf( scrollY ) - getI( "scrollYStep", 80 ) );
					zMsgUsed();
			}
		}
		else if( zmsgIs(which,wheelbackward) ) {
			if( getI( "panelConsumeClicks" ) ) {
				zMsgUsed();
				// prevent "scroll-through" to covered UI on dialog boxes etc...
			}

#if !defined(STOPFLOW)
			if( permitScrollY && scrollY < maxScrollY ) {
#else
			// stopflow is not yet properly set up for this scroll control
			if( permitScrollY ) {
#endif
				dirty();
				scrollY = max( floorf( getF("scrollYHome") ), floorf( getF("scrollY") ) + getI( "scrollYStep", 80 ) );
				zMsgUsed();
			}
		}
		return;
	}
	else if( zmsgIs(type,ZUIMouseReleaseDrag) ) {
		dirty();
		requestExclusiveMouse( 1, 0 );
		if( getI( "dragAndDropping" ) ) {
			endDragAndDrop();
		}
		zMsgUsed();
		return;
	}
	else if( zmsgIs(type,ZUIExclusiveMouseDrag) ) {
		if( getI( "dragAndDropping" ) ) {
			updateDragAndDrop();
			zMsgUsed();
		}
		else if( zmsgI(r) ) {
			if( permitScrollX && maxScrollX > 0.f ) {
				dirty();
				float deltaX = zmsgF(x) - getF( "scrollStartMouseX" );
				scrollX = min( floorf( getF("scrollXHome") ), floorf( getF("scrollStartX") ) + deltaX );
				dirty();
				zMsgUsed();
			}
			if( permitScrollY && maxScrollY > 0.f ) {
				dirty();
				float deltaY = zmsgF(y) - getF( "scrollStartMouseY" );
				scrollY = max( floorf( getF("scrollYHome") ), floorf( getF("scrollStartY") ) + deltaY );
				dirty();
				zMsgUsed();
			}
		}
		else if( zmsgI(l) ) {
			if( permitTranslate ) {
				dirty();
				translateX += zmsgF(x) - getF( "translateStartMouseX" );
				translateY += zmsgF(y) - getF( "translateStartMouseY" );
				putF( "translateStartMouseX", zmsgF(x) );
				putF( "translateStartMouseY", zmsgF(y) );
				dirty();
				zMsgUsed();
			}
			else if( permitDragDropInit ) {
				if( !getI( "dragAndDropping" ) ) {
					beginDragAndDrop( zmsgF(localX), zmsgF(localY) );
				}
				zMsgUsed();
			}
		}
		return;
	}
	else if( zmsgIs(type,ZUIReshape) ) {
		if( permitScrollY || permitScrollX ) {
			dirty();
			int viewport[4];
			glGetIntegerv( GL_VIEWPORT, viewport );

			//  @TODO: how to handle this more elegantly?
			// We only really need to reset scroll params if we will be
			// resized due to the reshape, and this is only true if we are
			// attached to a zui that fills the vertical or horizontal 
			// dimension.  In this case we typically set very large value
			// for h or w to allow an "infinite" scroll area.  So look for 
			// very large values of h or w, and reset the scroll for those
			// only.  This is to avoid having to reset the scroll for smaller
			// panels, which is problematic since we need our parent to have
			// resized first, but will only get this message later.
				
			if( permitScrollY && h > 5000 ) {
				scrollY = (float)( int(-h) + viewport[3] );
				putI( "scrollYHome", int(-h) + viewport[3] );
			}
			if( permitScrollX && w > 5000 ) {
				scrollX = (float)( int(-w) + viewport[2] );
				putI( "scrollXHome", int(-w) + viewport[2] );
			}
		}
		return;
	}
	if( !zMsgIsUsed() ) {
		ZUI::handleMsg( msg );
	}
}
