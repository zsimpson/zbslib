// @ZBS {
//		*COMPONENT_NAME zuiplot3d
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuiplot3d.cpp zuiplot3d.h
//		*MODULE_DEPENDS zviewpoint2.cpp
// }

// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#ifdef __APPLE__
#include "OpenGL/gl.h"
#include "OpenGL/glu.h"
#else
#include "GL/gl.h"
#include "GL/glu.h"
#endif
// STDLIB includes:
#include "string.h"
#include "math.h"
#include "assert.h"
// MODULE includes:
#include "zuiplot3d.h"
// ZBSLIB includes:
#include "plot.h"
#include "ztmpstr.h"
#include "zglfont.h"
#include "zmousemsg.h"
#include "zglfwtools.h"

ZUI_IMPLEMENT(ZUIPlot3d,ZUIPanel);

typedef void PlotUnrangedCallback( int userid );

#define ZUIPLOT3D_ORTHO 0
#define ZUIPLOT3D_YZ 1
#define ZUIPLOT3D_XZ 2

void getAnglesForViewmode( int mode, float &rotX, float &rotY ) {
	switch( mode ) {
		case ZUIPLOT3D_XZ:
			rotX = 0;
			rotY = 0;
			break;

		case ZUIPLOT3D_YZ:
			rotX = 0;
			rotY = -90;
			break;

		case ZUIPLOT3D_ORTHO:
			rotX = 22.5;
			rotY = -45.0;
			break;
	}
}

void ZUIPlot3d::factoryInit() {
	ZUIPanel::factoryInit();
	putF( "x0", -1.f );
	putF( "x1", 1.f );
	putF( "y0", -1.f );
	putF( "y1", 1.f );
	putF( "z0", -1.f );
	putF( "z1", 1.f );
	putI( "clipMessagesToBounds", 1 );
	vp.zviewpointSetButtons( 'L', 'R' );
	putI( "viewmode", ZUIPLOT3D_ORTHO );
	getAnglesForViewmode( ZUIPLOT3D_ORTHO, rotateX, rotateY );
	putI( "drawQuads", 1 );
	putS( "quadColor", "0x80808080" );
	putS( "quadGridColor", "0xA0A0A080" );
	putS( "axesColor", "0x000000FF" );
}

void ZUIPlot3d::getSceneDimsWithBorder( FVec3 &centroid, FVec3 &radius, 
									    float &_x0, float &_x1,
										float &_y0, float &_y1,
										float &_z0, float &_z1,
										float borderPercent ) {

	float x0 = getF("x0");
	float y0 = getF("y0");
	float z0 = getF("z0");
	float x1 = getF("x1");
	float y1 = getF("y1");
	float z1 = getF("z1");
	
	if( getI("plotLogZ") ) {
		z0 = z0>0 ? log10(z0) : 0;
		z1 = z1>0 ? log10(z1) : 0;
	}

	radius = FVec3( (x1-x0)/2.f, (y1-y0)/2.f, (z1-z0)/2.f );
	centroid = FVec3( x0 + radius.x, y0 + radius.y, z0 + radius.z );

	if( borderPercent != 0.f ) {
		radius.x *= 1.f + borderPercent;
		radius.y *= 1.f + borderPercent;
		radius.z *= 1.f + borderPercent;
	}

	_x0 = centroid.x - radius.x;
	_x1 = centroid.x + radius.x;
	_y0 = centroid.y - radius.y;
	_y1 = centroid.y + radius.y;
	_z0 = centroid.z - radius.z;
	_z1 = centroid.z + radius.z;
}

void ZUIPlot3d::getSceneScaledDimsWithBorder( FVec3 &centroid, FVec3 &radius, FVec3 &scale,
											  float &_x0, float &_x1,
											  float &_y0, float &_y1,
											  float &_z0, float &_z1,
											  float borderPercent ) {
	float x0 = getF("x0");
	float y0 = getF("y0");
	float z0 = getF("z0");
	float x1 = getF("x1");
	float y1 = getF("y1");
	float z1 = getF("z1");

	if( getI("plotLogZ") ) {
		z0 = z0>0 ? log10(z0) : 0;
		z1 = z1>0 ? log10(z1) : 0;
	}

	radius = FVec3( (x1-x0)/2.f, (y1-y0)/2.f, (z1-z0)/2.f );
	centroid = FVec3( x0 + radius.x, y0 + radius.y, z0 + radius.z );


	// up to this point the two Dims fns are identical, but now we will
	// compute a scale to present the data in a symmetric volume, and
	// will return the scene boundaries (which are used as clipping planes)
	// taking into account this scaling.

	float maxRadius = max( radius.x, max( radius.y, radius.z ) );
	scale = FVec3( maxRadius/radius.x, maxRadius/radius.y, maxRadius/radius.z );

	_x0 = scale.x * ( x0 - radius.x * borderPercent );
	_x1 = scale.x * ( x1 + radius.x * borderPercent );
	_y0 = scale.y * ( y0 - radius.y * borderPercent );
	_y1 = scale.y * ( y1 + radius.y * borderPercent );
	_z0 = scale.z * ( z0 - radius.z * borderPercent );
	_z1 = scale.z * ( z1 + radius.z * borderPercent );
}

void ZUIPlot3d::render() {
	ZUIPanel::render();

	// 
	// Validate dims and use scissoring to clip the client render
	//
	if( w <= 0 || h <= 0 ) {
		return;
	}
	glPushAttrib( GL_SCISSOR_BIT );
	glEnable( GL_SCISSOR_TEST );
	float x, y;
	getWindowXY( x, y );
	scissorIntersect( (int)x-1, (int)y-1, (int)ceilf(w)+1, (int)ceilf(h)+1 );

	//
	// Get dimensions,scale of scene including border area etc.
	//
	FVec3 radius, centroid, scale;
	float x0, x1, y0, y1, z0, z1;
	float borderPercent = 0.75;
	if( getI( "viewmode" ) != ZUIPLOT3D_ORTHO ) {
		borderPercent = 0.25;
	}

	getSceneScaledDimsWithBorder( centroid, radius, scale, x0, x1, y0, y1, z0, z1, borderPercent );

	begin3d();
		// this initializes all matrices to indentity

	glMatrixMode( GL_PROJECTION );
	glOrtho( x0, x1, y0, y1, -1000000.f, 1000000.f );
		// we prefer an orthographic projection for 3d plotting

	glMatrixMode( GL_MODELVIEW );
	glTranslatef( centroid.x * scale.x, centroid.y * scale.y, centroid.z * scale.z);
	glRotatef( rotateX, 1, 0, 0 );
	glRotatef( rotateY, 0, 1, 0 );
	glTranslatef( -centroid.x * scale.x, -centroid.y * scale.y, -centroid.z * scale.z);
		// rotate about the centroid of the scene

	vp.zviewpointTransAxes = centroid;
	vp.zviewpointScaleAxes = scale;
	vp.zviewpointSetupView();

	drawLegendAxes();

	void *p = getp( "unrangedCallback" );
	if( p ) {
		z0 = getF( "z0" );
		z1 = getF( "z1" );
		double transZ = z1;
		
		if( getI("plotLogZ") ) {
			z0 = log10(z0);
			z1 = log10(z1);
			transZ = z0 + z1;
		}
		
		glTranslatef( 0.f, 0.f, transZ );
		glScalef( 1.f, 1.f, -1.f );

		PlotUnrangedCallback *callback = (PlotUnrangedCallback *)p;
		int userid = getI( "userid" );
		(*callback)( userid );
	}

	end3d();

	// second callback - for anything that should not be drawn in the 3D context
	void *p2 = getp( "unrangedCallback2" );
	if( p2 ) {
		PlotUnrangedCallback *callback2 = (PlotUnrangedCallback *)p2;
		int userid = getI( "userid" );
		(*callback2)( userid );
	}

	glPopAttrib();
		// GL_SCISSOR_BIT
}

void zglLegendScale( FVec3 p1, FVec3 p2, FVec3 tickVec, float tickStart, float tickOffset, float tickStep, float tickCount, FVec3 &radius, char *label, int logZ ) {
	// draws a line from p1 to p2 with tick marks tickVec for labeling scenes in 3 dimensions.
	// the font draw is handled elsewhere to prevent dependency on fonts in this module.
	// tickStart is relative t
	//
	// radius is a hack to scale things so that the font draw works

	FVec3 t0,t1;
		// the endpoints of the ticks we'll draw

	FVec3 tStep( p2 );
	tStep.sub( p1 );
	tStep.normalize();
	t0 = tStep;
	t0.mul( tickOffset );
	t0.add( p1 );
	t1 = t0;
	t1.add( tickVec );
		// t0, t1 setup for first tick
	tStep.mul( tickStep );
		// tStep tells us how to step down the axis to place ticks

	//
	// Draw the line with tickmarks
	//
	FVec3 savet0 = t0;
	glBegin( GL_LINES );
		glVertex3fv( p1 );
		glVertex3fv( p2 );
			// axis

		//
		// draw the ticks
		//
		for( int i=0; i<tickCount; i++ ) {
			glVertex3fv( t0 );
			glVertex3fv( t1 );
			t0.add( tStep );
			t1.add( tStep );
		} 
	glEnd();

	//
	// Label tickmarks with values
	//
	//return;
	t0 = savet0;
	t1 = t0;
	t1.add( tickVec );

	int drawFonts = 1;
	if( drawFonts ) {
		float value = tickStart;
		FVec3 scaleForFont( 1000.f / radius.x, 1000.f / radius.y, 1000.f / radius.z );
		glScalef( 1.f / scaleForFont.x, 1.f / scaleForFont.y, 1.f / scaleForFont.z );
		for( int i=0; i<tickCount; i++ ) {
			// mkness: round value to a multiple of tickStep to clean up appearance (i.e. so 0 does not appear as -0).
			value = float (tickStep * floor ((value / tickStep) + 0.5));
 
			glTranslatef( 0.f, 0.f, scaleForFont.z * t1.z );
			ZTmpStr printValue;
			if( !logZ ) {
				printValue.set( "%g", value );
			}
			else {
				printValue.set( "1e%g", value );
			}
			zglFontPrint( printValue, scaleForFont.x * t1.x, scaleForFont.y * t1.y, "verdana60" );
			glTranslatef( 0.f, 0.f, scaleForFont.z * -t1.z );
			t0.add( tStep );
			t1.add( tStep );
			value += tickStep;
			
		}
		if( label ) {
			t0 = savet0;
			tStep.mul( ( ((int)tickCount)&1 ? tickCount : tickCount-1 ) / 2.f );
			t0.add( tStep );
			tickVec.mul( 2.f );
			t0.add( tickVec );
			glTranslatef( 0.f, 0.f, scaleForFont.z * t0.z );
			zglFontPrint( label, scaleForFont.x * t0.x, scaleForFont.y * t0.y, "verdana60" );
			glTranslatef( 0.f, 0.f, scaleForFont.z * -t0.z );
		}
		glScalef( scaleForFont.x, scaleForFont.y, scaleForFont.z );
	}
}

void legendTickValues( float v0, float v1, float &start, float &step, float &count ) {
	// Given values v0 < v1, find a start value and a step value for
	// placing tick marks on a legend.  i.e., find "nice round numbers"
	// that will make for a good graphic display.

	assert( v0 < v1 );
	
	int log=0;

	float dist		= (float) log ? ceil( v1 ) - v0 : v1 - v0;
	float logRange	= (float) log10( dist );
	step			= (float) ( log ? 1.f : powf( 10.0, floor( logRange ) ) );

	if( !log && step * 1.1f > dist ) { 
		step *= .1f;
	}

	count = floorf( dist / step ) + 1.f;
	start = ceil( v0 / step ) * step;
}

void ZUIPlot3d::drawLegendAxes() {
	// The legend (3 dimensional labeled axes, with optional gridlines/backing-surfaces) should be drawn 
	// such that it is in the space between the client scene and the clipping planes as defined by 
	// borderPercent.  We can get values for our axes endpoints using the same routine used
	// by ::render, but supplying a smaller borderPercent.

	//
	// Get dimensions,scale of scene including border area: the borderPercent specified 
	// determines where the axes are drawn (how close to scene, or how close to clip planes)
	//
	FVec3 radius, centroid, scale;
	float x0, x1, y0, y1, z0, z1;
	getSceneScaledDimsWithBorder( centroid, radius, scale, x0, x1, y0, y1, z0, z1, .1f );
	x0 /= scale.x;
	x1 /= scale.x;
	y0 /= scale.y;
	y1 /= scale.y;
	z0 /= scale.z;
	z1 /= scale.z;

	FVec3 p0( x0, y0, z0 );
	FVec3 p1( x0, y0, z1 );
	FVec3 p2( x1, y0, z1 );
	FVec3 p3( x1, y0, z0 );
	FVec3 p4( x0, y1, z0 );
	FVec3 p5( x0, y1, z1 );
	FVec3 p6( x1, y1, z1 );
	FVec3 p7( x1, y1, z0 );
	
	FVec3 tickx( ( x1 - x0 ) / 20.f, 0.f, 0.f ); 
	FVec3 tickz( 0.f, 0.f, ( z1 - z0 ) / 20.f );

	float xStart, xStep, xCount, yStart, yStep, yCount, zStart, zStep, zCount, z0Unscaled, z1Unscaled;
	z0Unscaled = getF("z0");
	z1Unscaled = getF("z1");
	int logZ = getI("plotLogZ");
	if( logZ ) {
		z0Unscaled = z0Unscaled>0 ? log10(z0Unscaled) : 0;
		z1Unscaled = z1Unscaled>0 ? log10(z1Unscaled) : 0;
	}

	legendTickValues( getF( "x0" ), getF( "x1" ), xStart, xStep, xCount );
	legendTickValues( getF( "y0"), getF( "y1" ), yStart, yStep, yCount );
	legendTickValues( z0Unscaled, z1Unscaled, zStart, zStep, zCount );
	
	setupColor( "axesColor" );
	glLineWidth( 1.f );
	zglLegendScale( p1, p5, tickz, yStart, yStart-y0, yStep, yCount, radius, getS( "y-label" ), 0 );
	zglLegendScale( p1, p2, tickz, xStart, xStart-x0, xStep, xCount, radius, getS( "x-label" ), 0 );
	zglLegendScale( p2, p3, tickx, zStart, zStart-z0, zStep, zCount, radius, getS( "z-label" ), logZ );
	zglLegendScale( p3, p7, tickx, yStart, yStart-y0, yStep, yCount, radius, getS( "y-label" ), 0 );
	

	//
	// Quads to back the scene
	//
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	int drawQuads = getI( "drawQuads" );
	if( drawQuads ) {
		setupColor( "quadColor" );
		glBegin( GL_QUADS );
			glNormal3f( 1.f, 0.f, 0.f );
			glVertex3fv( p0 );
			glVertex3fv( p4 );
			glVertex3fv( p5 );
			glVertex3fv( p1 );
			glNormal3f( 0.f, 0.f, 1.f );
			glVertex3fv( p0 );
			glVertex3fv( p3 );
			glVertex3fv( p7 );
			glVertex3fv( p4 );
			glNormal3f( 0.f, 1.f, 0.f );
			glVertex3fv( p0 );
			glVertex3fv( p3 );
			glVertex3fv( p2 );
			glVertex3fv( p1 );
		glEnd();
	}
	glDisable( GL_BLEND );

	//
	// Interior lines where quads meet
	//
	setupColor( "axesColor" );
	glBegin( GL_LINES );
		glVertex3fv( p0 );
		glVertex3fv( p1 );
		glVertex3fv( p0 );
		glVertex3fv( p3 );
		glVertex3fv( p0 );
		glVertex3fv( p4 );
	glEnd();


	// 
	// Gridlines on the quads
	//
	glEnable( GL_LINE_STIPPLE );
	glLineStipple( 1, 8192+2048+512+128+32+8+2 );
	
	int i;
	float x,y,z;

	setupColor( "quadGridColor" );
	glBegin( GL_LINES );
		for( i=0, z=zStart; i < zCount; i++, z+= zStep ) {
			glVertex3f( x0, y0, z );
			glVertex3f( x0, y1, z );
		}
		for( i=0, y=yStart; i < yCount; i++, y+= yStep ) {
			glVertex3f( x0, y, z0 );
			glVertex3f( x0, y, z1 );
		}

		for( i=0, y=yStart; i < yCount; i++, y+= yStep ) {
			glVertex3f( x0, y, z0 );
			glVertex3f( x1, y, z0 );
		}
		for( i=0, x=xStart; i < xCount; i++, x+= xStep ) {
			glVertex3f( x, y0, z0 );
			glVertex3f( x, y1, z0 );
		}

		for( i=0, x=xStart; i < xCount; i++, x+= xStep ) {
			glVertex3f( x, y0, z0 );
			glVertex3f( x, y0, z1 );
		}
		for( i=0, z=zStart; i < zCount; i++, z+= zStep ) {
			glVertex3f( x0, y0, z );
			glVertex3f( x1, y0, z );
		}

	glEnd();
	glDisable( GL_LINE_STIPPLE );
}

ZUI::ZUILayoutCell *ZUIPlot3d::layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly ) {
	w = 100;
	h = 100;
	return 0;
}

void ZUIPlot3d::stepAnimation( int step ) { 
	int frameIndex = getI( "frameIndex" );
	int numFrames = getI( "numFrames" );

	frameIndex += step;

	if( frameIndex < 0 ) {
		frameIndex = numFrames - 1;
	}
	else if( frameIndex >= numFrames ) {
		frameIndex = 0;
	}

	putI( "frameIndex", frameIndex );
}

void ZUIPlot3d::sendMouseMsg( char* mouseMsg, float localX, float localY, int shift, int ctrl, int alt ) {
// Send a mouse message.
	if( mouseMsg ) {
		// mkness: mapping as done in ZUIPlot is skipped here since I am not sure how to do it properly and I do not care for now.
		//     float mappedX = mapCoordX( localX );
		//     float mappedY = mapCoordY( localY );
		double mappedX = localX;
		double mappedY = localY;
		zMsgQueue( "%s x=%g y=%g shift=%d ctrl=%d alt=%d", mouseMsg, mappedX, mappedY, shift, ctrl, alt );
	}
}

void ZUIPlot3d::handleMsg( ZMsg *msg ) {
	// shift states which only exist for mouse click messages

	if( getI( "isIconic" ) ) {
		return;
	}

	int shift = zmsgI (shift);
	int ctrl  = zmsgI (ctrl);
	int alt   = zmsgI (alt);

	if( zmsgIs( type, ZUIPlot_Reset ) ) {
		int mode = getI( "viewmode" );
		getAnglesForViewmode( mode, rotateX, rotateY );
		vp.zviewpointReset();
		//zMsgUsed();
			// we would like this message to get sent to other plots in the group too
		return;
	}

	if( (zmsgIs(type,ZUIMouseClickOn) || zmsgIs(type,MouseClick)) && zmsgIs(dir,D) && (zmsgIs(which,R) || zmsgIs(which,M)) && zmsgI(shift) && zmsgI(ctrl) && zmsgI(alt) ) {
		int mode = getI( "viewmode" );
		getAnglesForViewmode( mode, rotateX, rotateY );
			// viewpoint will also reset, so don't mark msg as used
	}
	
	int animSync = zmsgI( animSync );	
		// hack I use to sync zuiplot3d anims by forcing multiple to handle same msg
	if( ! zMsgIsUsed() || animSync ) {
		float localX = (float)zMouseMsgX;
		float localY = (float)zMouseMsgY;
		getLocalXY( localX, localY );
		if( containsLocal( localX, localY ) || animSync ) {
			int asyncShift = zglfwGetShift();
			if( !asyncShift && zmsgIs( type, KeyDown ) ) {
				if( zmsgIs( which, up ) )  {
					putI( "frameIndex", 0 );
					zMsgUsed();
				}
				else if( zmsgIs( which, down ) )  {
					putI( "frameIndex", getI( "numFrames" ) - 1 );
					zMsgUsed();
				}
				else if( zmsgIs( which, left ) )  {
					stepAnimation( -1 );
					zMsgUsed();
				}
				else if( zmsgIs( which, right ) )  {
					stepAnimation( 1 );
					zMsgUsed();
				}
				else if( zmsgIs( which, ctrl_p ) ) {
					putI( "freezeAnim", !getI( "freezeAnim" ) );
					zMsgUsed();
				}
				if( animSync ) {
					return;
				}
				if( zMsgIsUsed() ) {
					void *p = getp( "animCallback" );
						// send this message to a callback which can sync animations of other things etc.
					if( p ) {
						msg->putI( "animSync", 1 );
						AnimCallback *callback = (AnimCallback *)p;
						(*callback)( this, msg );
					}
				}
			}
			// mouse click messages, which are mainly wanted by stopflow
			if( zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) ) {
				if( zmsgIs(which,L) ) {
					char *clickMsg = getS( "sendMsgOnLClick", 0 );
					if( clickMsg ) {
						sendMouseMsg( clickMsg, localX, localY, shift, ctrl, alt );
						// do NOT use up the message so the viewpoint handler can still get it
					}
				}
			}
			if( ! zMsgIsUsed() ) {
				vp.zviewpointHandleMsg( msg );
			}
		}
	}
	if( ! zMsgIsUsed() ) {
		ZUIPanel::handleMsg( msg );
	}
}
