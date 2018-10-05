// @ZBS {
//		*COMPONENT_NAME zuiplot
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuiplot.cpp
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
#include "assert.h"
// MODULE includes:
#include "zuiplot.h"
#include "zmousemsg.h"
// ZBSLIB includes:
#include "plot.h"
#include "zmathtools.h"
// STOPFLOW timing
#include "sfmod.h"

ZUI_IMPLEMENT(ZUIPlot,ZUIPanel);

typedef void PlotRangedCallback( int userid, float x0, float y0, float x1, float y1, float xd, float yd );
typedef void PlotUnrangedCallback( int userid );


// @TODO: cleanup, add colors and log

void plotCallback( int userid, float x0, float y0, float x1, float y1, float xd, float yd ) {
	glColor3ub( 255, 0, 0 );
	glBegin( GL_LINE_STRIP );
	for( float x=x0; x<=x1; x+=xd ) {
		glVertex2f( x, sinf(x*10.f) );
	}
	glEnd();
}

void ZUIPlot::factoryInit() {
	ZUIPanel::factoryInit();
	putF( "scaleX", 1.f );
	putF( "scaleY", 1.f );
	putF( "x0", -1.f );
	putF( "x1", 1.f );
	putF( "y0", -1.f );
	putF( "y1", 1.f );
	putI( "permitTransX", 1 );
	putI( "permitTransY", 1 );
	putI( "permitScaleX", 1 );
	putI( "permitScaleY", 1 );
}

void ZUIPlot::render() {
	ZUIPanel::render();

	SFTIME_START (PerfTime_ID_Zlab_render_plots, PerfTime_ID_Zlab_render_render);

	// SETUP matrix into plot coords
	float myW, myH;
	getWH( myW, myH );

	if( myW <= 0 && myH <= 0 ) {
		return;
	}

	glPushMatrix();

	glPushAttrib( GL_SCISSOR_BIT );
	glEnable( GL_SCISSOR_TEST );
	float x, y;
	getWindowXY( x, y );
	scissorIntersect( (int)x-1, (int)y-1, (int)ceilf(myW)+1, (int)ceilf(myH)+1 );

	float tx = getF( "transX" );
	float ty = getF( "transY" );
	float sx = getF( "scaleX" );
	float sy = getF( "scaleY" );

	float x0 = getF("x0");
	float y0 = getF("y0");
	float x1 = getF("x1");
	float y1 = getF("y1");
	x0 *= sx;
	x1 *= sx;
	y0 *= sy;
	y1 *= sy;

	int plotLogX = getI( "plotLogX" );
	if( plotLogX ) {
		x0 = (float)getD( "minLogscaleExponentX", 0 );
		x1 = x1 == 0 ? 0 : log10f( x1 );
		if( x1 <= x0 ) x1 = x0 + 1;
	}
	int plotLogY = getI( "plotLogY" );
	if( plotLogY ) {
		y0 = (float)getD( "minLogscaleExponentY", 0 );
		y1 = y1 == 0 ? 0 : ceilf( log10f( y1 ) );
		if( x1 <= x0 ) x1 = x0 + 1;
	}

	float xmag = x1 - x0;
	float ymag = y1 - y0;
	if( ymag == 0 ) {
		ymag = 0.01f;
	}


	float xd = xmag / myW;
	float yd = ymag / myH;
	x0 -= tx*xd;
	x1 -= tx*xd;
	y0 -= ty*yd;
	y1 -= ty*yd;
		// tfb: @TODO check what needs to change when log plotting is on for this section...

	glScalef( 1.f/xd, 1.f/yd, 1.f );
	glTranslatef( -x0, -y0, 0.f );

//@TODO: Load colors
//float gridMajorColor[3] = { 0.2f, 0.2f, 0.2f };
//float gridMinorColor[3] = { 0.75f, 0.75f, 0.75f };
//float gridAxisColor[3] = { 0.f, 0.f, 0.f };
//float gridLegendColor[3] = { 0.5f, 0.5f, 0.5f };
	if (!getI ("skipGrid"))
	{
		int legendColor = 0xFFFFFFFF;
		if( has( "gridLegendColor" ) ) {
			legendColor = getColorI( "gridLegendColor" );
			plotGridLegendColor3ub( legendColor>>24, legendColor>>16&0xff, legendColor>>8&0xff );
		}

		int axisColor = 0x000000FF;
		if( has( "gridAxisColor" ) ) {
			axisColor = getColorI( "gridAxisColor");
			plotGridAxisColor3ub( axisColor>>24, axisColor>>16&0xff, axisColor>>8&0xff );
		}
        

		int majorColor = 0x333333FF;
		if( has( "gridMajorColor" ) ) {
			majorColor = getColorI( "gridMajorColor" );
			plotGridMajorColor3ub( majorColor>>24, majorColor>>16&0xff, majorColor>>8&0xff );
		}

		int minorColor = 0xBFBFBFFF; 
		if( has( "gridMinorColor" ) ) {
			minorColor = getColorI( "gridMinorColor" );
			plotGridMinorColor3ub( minorColor>>24, minorColor>>16&0xff, minorColor>>8&0xff );
		}

		plotGridlinesLinLog( x0, y0, x1, y1, xd, yd, plotLogX, plotLogY, getI( "skipGridMinor" ), getI( "skipXLabels" ), getI( "skipYLabels" )  );
		// plotGridlines( x0, y0, x1, y1, xd, yd );
	}

	int userid = getI( "userid" );

	// SETUP the coord space so that if you draw in native units that
	// it will end up on this plot.  Setup scissoring.
	void *p = getp( "rangedCallback" );
	if( p ) {
		PlotRangedCallback *callback = (PlotRangedCallback *)p;
		(*callback)( userid, x0, y0, x1, y1, xd, yd );
	}
	else {
		p = getp( "unrangedCallback" );
		if( p ) {
			PlotUnrangedCallback *callback = (PlotUnrangedCallback *)p;
			(*callback)( userid );
		}
	}
	glPopMatrix();
	
	// Render zoom-box if enabled
	x1 = getF( "dragCurrentX", -1.f );
	if( x1 >= 0.f && getI( "boxZoom" ) ) {
		float x0 = getF( "dragStartX" );
		float y0 = getF( "dragStartY" );
		float y1 = getF( "dragCurrentY" );
		glColor4ub( 255, 255, 255, 92 );
		glDisable( GL_BLEND );
		glBegin( GL_LINE_LOOP );
		glVertex2d( x0, y0 );
		glVertex2d( x1, y0 );
		glVertex2d( x1, y1 );
		glVertex2d( x0, y1 );
		glEnd();
		glEnable( GL_BLEND );
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBegin( GL_QUADS );
		glVertex2d( x0, y0 );
		glVertex2d( x1, y0 );
		glVertex2d( x1, y1 );
		glVertex2d( x0, y1 );
		glEnd();
		glDisable( GL_BLEND );
	}

	// second callback - this is added to be consistent with ZUIPlot3D, which needs separate callbacks.
	void *p2 = getp( "unrangedCallback2" );
	if( p2 ) {
		PlotUnrangedCallback *callback2 = (PlotUnrangedCallback *)p2;
		(*callback2)( userid );
	}

	glPopAttrib();
	SFTIME_END (PerfTime_ID_Zlab_render_plots);
}

ZUI::ZUILayoutCell *ZUIPlot::layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly ) {
	w = 100;
	h = 100;
	return 0;
}

// Mapping window pixel coordinates to plot coordinates

float ZUIPlot::mapCoordX( float localX, int reportExponent ) {
//	printf( "x=%g ", localX );

	localX -= getF( "transX" );
		// translation should be applied in the local coordinate system - this 
		// translation is in pixels, as is 'localX'
	
	float mappedX;
	int plotLogX = getI( "plotLogX" );
	if( !plotLogX ) {
		// linear axes
		mappedX = ( localX / w ) * ( getF("x1") - getF("x0") ) + getF("x0");
		mappedX = mappedX * getF("scaleX");
	}
	else {
		// log axes -
		//   Value is reported as log10(x), to allow better precision.
		//   TODO - this is probably not right if the min exponent does not match x0.
		//   I also am not sure about the scale/trans part.
		// double x0 = getF("x0");
		double x0 = pow( 10.0, getD( "minLogscaleExponentX" ) );
			// I prefer this over getF("x0") because it is common that x0 is 0 (tfb)
		double x1 = getF("x1");
		x1 *= getF( "scaleX" );
			// I'm not really sure about how to apply this scale.  But this works empirically in my tests. (tfb)
		
		double gamm = 1.0 / log10(x1 / x0);
		double delt = -gamm * log10(x0);
		double phi  = localX / w;
		double logx = (phi - delt) / gamm;
		mappedX = float (logx);
		if( !reportExponent ) {
			mappedX = (float)pow( 10.0, logx );
				// this is what the actual value clicked on is.
		}
	}
//	printf( "mapped to %g (scale is %g, trans is %g)\n", mappedX, getF( "scaleX" ), getF( "transX" ) );
	return mappedX;
}

float ZUIPlot::mapCoordY( float localY, int reportExponent ) {
	// TODO - This is untested as I have no log y axes, code copied from x adjust which is tested.
	
	localY -= getF( "transY" );
	float mappedY;
	int plotLogY = getI( "plotLogY" );
	if( !plotLogY ) {
		// linear axes
		mappedY = ( localY / h ) * ( getF("y1") - getF("y0") ) + getF("y0");
		mappedY = mappedY * getF("scaleY");
	}
	else {
		// log axes -
		//   Value is reported as log10(y), to allow better precision.
		//   TODO - this is probably not right if the min exponent does not match y0.
		//   I also am not sure about the scale/trans part.
		double y0 = getF("y0");
		double y1 = getF("y1");
		y1 *= getF( "scaleY" );
		double gamm = 1.0 / log10(y1 / y0);
		double delt = -gamm * log10(y0);
		double phi  = localY / h;
		double logy = (phi - delt) / gamm;
		mappedY = float (logy);
		if( !reportExponent ) {
			mappedY = (float)pow( 10.0, logy );
				// this is what the actual value clicked on is.
		}
	}
	return mappedY;
}

void ZUIPlot::sendMouseMsg( char* mouseMsg, float localX, float localY, int shift, int ctrl, int alt ) {
// Send a message with mapped mouse coordinates.
	if( mouseMsg ) {
		float mappedX = mapCoordX( localX );
		float mappedY = mapCoordY( localY );
		zMsgQueue( "%s x=%g y=%g shift=%d ctrl=%d alt=%d", mouseMsg, mappedX, mappedY, shift, ctrl, alt );
	}
}

void ZUIPlot::handleMsg( ZMsg *msg ) {
	// shift states which only exist for mouse click messages
	int shift = zmsgI (shift);
	int ctrl  = zmsgI (ctrl);
	int alt   = zmsgI (alt);
	int beginDrag = 0;
	if( zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) ) {
		if( zmsgIs(which,R) || zmsgIs(which,M) ) {
			if( !shift && !ctrl && !alt ) {
				//
				// standard drag,trans,scale functionality uses right/middle button without
				// meta-key modifier (scale can also use mousewheel, which is a key - see below)
				//
				beginDrag = 1;
			}
			else {
				char *clickMsg = getS( "sendMsgOnRClick", 0 );
				if( clickMsg ) {
					sendMouseMsg( clickMsg, zmsgF(localX), zmsgF(localY), shift, ctrl, alt );
					zMsgUsed();
				}
				if( getI( "dragOnRClick" ) ) {
					beginDrag = 1;
				}
			}
		}
		else if( zmsgIs(which,L) ) {
			char *clickMsg = getS( "sendMsgOnLClick", 0 );
			if( clickMsg ) {
				sendMouseMsg( clickMsg, zmsgF(localX), zmsgF(localY), shift, ctrl, alt );
				zMsgUsed();
			}
			if( getI( "dragOnLClick" ) || getI( "boxZoom" ) ) {
				beginDrag = 1;
			}
		}
	}
	if( beginDrag ) {
		requestExclusiveMouse( 1, 1 );
		putF( "dragStartX", zmsgF(localX) );
		putF( "dragStartY", zmsgF(localY) );
		putF( "transStartX", getF("transX") );
		putF( "transStartY", getF("transY") );
		putF( "scaleStartX", getF("scaleX") );
		putF( "scaleStartY", getF("scaleY") );
		zMsgUsed();
	}

	if( zmsgIs(type,ZUIMouseReleaseDrag) ) {
		requestExclusiveMouse( 1, 0 );
		
		if( getI( "boxZoom" ) && has( "dragCurrentX" ) ) {
			float x0 = getF( "dragStartX" );
			float x1 = getF( "dragCurrentX" );
			float y0 = getF( "dragStartY" );
			float y1 = getF( "dragCurrentY" );
			
			if( x1!=x0 && y1!=y0 ) {
				float mx0 = mapCoordX( min(x0,x1), 0 );
				float mx1 = mapCoordX( max(x0,x1), 0 );
				float my0 = mapCoordY( min(y0,y1), 0 );
				float my1 = mapCoordY( max(y0,y1), 0 );
				
				
				if( !has( "boxZoomCachedExtents" ) ) {
					// Store the original extent values so that we may revert with ctrl-w.
					// Note that there is an assumption here that the client will either
					// recreate the plot object if bounds change, or clear boxZoomCachedExents
					// whenever he wants new max bounds to get used (because they have changed).
					putF( "orgX0", getF( "x0" ) );
					putF( "orgX1", getF( "x1" ) );
					putF( "orgY0", getF( "y0" ) );
					putF( "orgY1", getF( "y1" ) );
					putF( "orgScaleX", getF( "scaleX" ) );
					putF( "orgScaleY", getF( "scaleY" ) );
					putI( "boxZoomCachedExtents", 1 );
				}
				putF( "x0", mx0 );
				putF( "x1", mx1 );
				putF( "y0", my0 );
				putF( "y1", my1 );
				putF( "scaleX", 1.f );
				putF( "scaleY", 1.f );
				del( "transX" );
				del( "transY" );
			}
		}
		del( "dragCurrentX" );
		del( "dragCurrentY" );

		char *dragMsg = getS( "sendMsgOnDragRelease", 0 );
		if( dragMsg ) {
		sendMouseMsg( dragMsg, zmsgF(localX), zmsgF(localY), shift, ctrl, alt );
			zMsgUsed();
		}
		zMsgUsed();
	}

	if( zmsgIs(type,ZUIExclusiveMouseDrag) ) {
		float mx = zmsgF(localX);
		float my = zmsgF(localY);

		float dx = mx - getF( "dragStartX" );
		float dy = my - getF( "dragStartY" );

		if( zmsgI(l) ) {
			putF( "dragCurrentX", mx );
			putF( "dragCurrentY", my );
				// this is used for boxZoom, which is done with left button
		}

		if( zmsgI(r) ) {
			float transX = getF( "transStartX" ) + dx;
			float transY = getF( "transStartY" ) + dy;
			if( getI("permitTransX") ) {
				putF( "transX", transX );
			}
			if( getI("permitTransY") ) {
				putF( "transY", transY );
			}
			zMsgUsed();
		}

		if( zmsgI(m) ) {
			float scaleX = getF( "scaleStartX" ) * expf( -dx / 50.f );
			float scaleY = getF( "scaleStartY" ) * expf( -dy / 50.f );
			if( getI("permitScaleX") ) {
				putF( "scaleX", scaleX );
			}
			if( getI("permitScaleY") ) {
				putF( "scaleY", scaleY );
			}
			zMsgUsed();
		}

		char *dragMsg = getS( "sendMsgOnDrag", 0 );
		if( dragMsg ) {
			sendMouseMsg( dragMsg, mx, my, shift, ctrl, alt );
			zMsgUsed();
		}
	}

	const float scaleStep = 0.1f;

	float x = zmsgF( x );
	float y = zmsgF( y );
	getLocalXY( x, y );
	if( containsLocal( x, y ) ) {
		if( zmsgIs(type,Key) ) {
			if( zmsgIs(which,wheelforward) || !strcmp(msg->getS("which"),",") ) {
				if( getI("permitScaleX") ) {
					putF( "scaleX", getF("scaleX") * (1.f + scaleStep) );
				}
				if( getI("permitScaleY") ) {
					putF( "scaleY", getF("scaleY") * (1.f + scaleStep) );
				}
				zMsgUsed();
			}
			else if( zmsgIs(which,wheelbackward) || !strcmp(msg->getS("which"),".") ) {
				if( getI("permitScaleX") ) {
					putF( "scaleX", getF("scaleX") * (1.f - scaleStep) );
				}
				if( getI("permitScaleY") ) {
					putF( "scaleY", getF("scaleY") * (1.f - scaleStep) );
				}
				zMsgUsed();
			}
		}
	}
	x = (float)zMouseMsgX;
	y = (float)zMouseMsgY;
	getLocalXY( x, y );
	if( containsLocal( x, y ) && zmsgIs( type, KeyDown ) ) {
		if( zmsgIs( which, ctrl_w ) ) {
			if( has( "boxZoomCachedExtents" ) ) {
				// revert scaling to the original values that existing before
				// any boxZoom occured
				putF( "x0", getF( "orgX0" ) );
				putF( "x1", getF( "orgX1" ) );
				putF( "y0", getF( "orgY0" ) );
				putF( "y1", getF( "orgY1" ) );
				putF( "scaleX", getF( "orgScaleX" ) );
				putF( "scaleY", getF( "orgScaleY" ) );
				del( "transX" );
				del( "transY" );
				del( "boxZoomCachedExtents" );
			}
		}
	}


	ZUIPanel::handleMsg( msg );
}
