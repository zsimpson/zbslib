// @ZBS {
//		*MODULE_NAME Plot
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A colleciton of free function tools for makine GL plots
//			The goal is to keep the functions as simple and un-gui as possible
//		}
//		*PORTABILITY win32 unix macosx
//		*REQUIRED_FILES plot.cpp plot.h
//		*VERSION 1.0
//		+HISTORY {
//			1.0 Extracted from _plot2d
//		}
//		+TODO {
//			This has a lot of dependencies on things like viewpoint,
//			zgltools, ztmpstr, etc which could be removed
//		}
//		*SELF_TEST no
//		*PUBLISH yes
// }

// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#include "GL/gl.h"
// STDLIB includes:
#include "math.h"
#include "float.h"
// MODULE includes:
#include "plot.h"
// ZBSLIB includes:
#include "zvec.h"
#include "zviewpoint.h"
#include "zgltools.h"
#include "ztmpstr.h"
#include "zglfont.h"

float gridMajorColor[3] = { 0.2f, 0.2f, 0.2f };
float gridMinorColor[3] = { 0.75f, 0.75f, 0.75f };
float gridAxisColor[3] = { 0.f, 0.f, 0.f };
float gridLegendColor[3] = { 1.f, 1.f, 1.f };

void plotGridMajorColor3f( float r, float g, float b ) {
	gridMajorColor[0] = r;
	gridMajorColor[1] = g;
	gridMajorColor[2] = b;
}

void plotGridMajorColor3ub( unsigned char r, unsigned char g, unsigned char b ) {
	gridMajorColor[0] = (float)r / 255.f;
	gridMajorColor[1] = (float)g / 255.f;
	gridMajorColor[2] = (float)b / 255.f;
}

void plotGridMinorColor3f( float r, float g, float b ) {
	gridMinorColor[0] = r;
	gridMinorColor[1] = g;
	gridMinorColor[2] = b;
}

void plotGridMinorColor3ub( unsigned char r, unsigned char g, unsigned char b ) {
	gridMinorColor[0] = (float)r / 255.f;
	gridMinorColor[1] = (float)g / 255.f;
	gridMinorColor[2] = (float)b / 255.f;
}

void plotGridAxisColor3f( float r, float g, float b ) {
	gridAxisColor[0] = r;
	gridAxisColor[1] = g;
	gridAxisColor[2] = b;
}

void plotGridAxisColor3ub( unsigned char r, unsigned char g, unsigned char b ) {
	gridAxisColor[0] = (float)r / 255.f;
	gridAxisColor[1] = (float)g / 255.f;
	gridAxisColor[2] = (float)b / 255.f;
}

void plotGridLegendColor3ub( unsigned char r, unsigned char g, unsigned char b ) {
	gridLegendColor[0] = (float)r / 255.f;
	gridLegendColor[1] = (float)g / 255.f;
	gridLegendColor[2] = (float)b / 255.f;
}

void plotUsingViewpointGetBoundFullscreen( float &l, float &t, float &r, float &b ) {
	float viewport[4];
	glGetFloatv( GL_VIEWPORT, viewport );

	// WINDOW bounds of the plot (in plot coords)
	FVec2 lftBot = (FVec2)zviewpointProjOnPlane( 0, 0, FVec3::Origin, FVec3::ZAxis );
	FVec2 rgtTop = (FVec2)zviewpointProjOnPlane( (int)viewport[2], (int)viewport[3], FVec3::Origin, FVec3::ZAxis );

	l = lftBot.x;
	r = rgtTop.x;
	b = lftBot.y;
	t = rgtTop.y;
}

void plotUsingViewpointGetPixelDXDY( float &dx, float &dy ) {
	float viewport[4];
	glGetFloatv( GL_VIEWPORT, viewport );

	// WINDOW bounds of the plot (in plot coords)
	FVec2 lftBot = (FVec2)zviewpointProjOnPlane( 0, 0, FVec3::Origin, FVec3::ZAxis );
	FVec2 rgtTop = (FVec2)zviewpointProjOnPlane( (int)viewport[2], (int)viewport[3], FVec3::Origin, FVec3::ZAxis );

	float l = lftBot.x;
	float r = rgtTop.x;
	float b = lftBot.y;
	float t = rgtTop.y;

	// PIXEL size in world coords
	dx = (r - l) / viewport[2];
	dy = (t - b) / viewport[3];
}

void plotUsingViewpointGridlinesFullscreen() {
	float viewport[4];
	glGetFloatv( GL_VIEWPORT, viewport );

	// WINDOW bounds of the plot (in plot coords)
	FVec2 lftBot = (FVec2)zviewpointProjOnPlane( 0, 0, FVec3::Origin, FVec3::ZAxis );
	FVec2 rgtTop = (FVec2)zviewpointProjOnPlane( (int)viewport[2], (int)viewport[3], FVec3::Origin, FVec3::ZAxis );

	plotGridlines( lftBot.x, lftBot.y, rgtTop.x, rgtTop.y, (rgtTop.x-lftBot.x)/viewport[2], (rgtTop.y-lftBot.y)/viewport[3] );
}

void plotGridlinesLinLog( double x0, double y0, double x1, double y1, double pixDX, double pixDY, int logx, int logy, int skipMinor, int skipXLabels, int skipYLabels ) {

	// x0,y0,x1,y1  : the extent of this plot in world coordinates (i.e. in the basis of the data being plotted)
	// pixDX, pixDY : the 'size' of pixels in world coordinates along the x & y axes
	// logx, logy   : should either axis be plotted on log10 scale?

	if( ( x1-x0 <=0 ) || ( y1-y0 <= 0 ) ) {
		return;
	}

	// Determine major gridline parameters in world coordinates:
	// majorNum : how many major gridlines to draw
	// majorInc : the distance, in world coordinates, between major gridlines
	// majorBeg : the world coordinate at which to draw the first major gridline
	//
	int majorNumX, majorNumY;
	double majorIncX, majorIncY, majorBegX, majorBegY;
	double logRangeX, logRangeY;

	//-----------------------------------------------------------------------
	// SETUP PARAMS
	// x-axis
	double dist = logx ? ceil( x1 ) - x0 : x1 - x0;
	logRangeX   = log10( dist );
	majorIncX   = logx ? 1 : pow( 10.0, floor( logRangeX ) );
	if( !logx && majorIncX * 1.1 > dist ) majorIncX *= .1;
	majorNumX = (int) floor( dist / majorIncX ) + 2;
	majorBegX = floor( x0 / majorIncX ) * majorIncX;

	// y-axis
	dist	  = logy ? ceil( y1 ) - y0 : y1 - y0;
	logRangeY = log10( dist );
	majorIncY = logy ? 1 : pow( 10.0, floor( logRangeY ) );
	if( !logy && majorIncY * 1.1 > dist ) majorIncY *= .1;
	majorNumY = (int) floor( dist / majorIncY ) + 2;
	majorBegY = floor( y0 / majorIncY ) * majorIncY;

	//-----------------------------------------------------------------------
	// PLOT MINOR GRIDLINES
	int i,j;
	double x,y;
	double majX = majorBegX;

	if( !skipMinor ) {
		glLineWidth( 1.f );
		glColor3fv( gridMinorColor );
		glBegin( GL_LINES ); 
		for( i=0; i<majorNumX; i++ ) {
			for( j=1; j<=9; j++ ) {
				if( logx ) {
					x = majX + majorIncX * log10( (double)j );
				}
				else {
					x = majX + majorIncX * j / 10.0;
				}
				glVertex2d( x, y0 );
				glVertex2d( x, y1 );
			}
			majX += majorIncX;
		}
		glEnd();

		glBegin( GL_LINES ); 
		double majY = majorBegY;
		for( i=0; i<majorNumY; i++ ) {
			for( j=1; j<=9; j++ ) {
				if( logy ) {
					y = majY + majorIncY * log10( (double)j );
				}
				else {
					y = majY + majorIncY * j / 10.0;
				}
				glVertex2d( x0, y );
				glVertex2d( x1, y );
			}
			majY += majorIncY;
		}
		glEnd();
	}


	//-----------------------------------------------------------------------
	// PLOT MAJOR GRIDLINES

	glLineWidth( 2.f );
	glColor3fv( gridMajorColor );

	x = majorBegX;
	glBegin( GL_LINES ); 
	for( i=0; i<majorNumX; i++ ) {
		glVertex2d( x, y0 );
		glVertex2d( x, y1 );
		x += majorIncX;
	}
	glEnd();

	y = majorBegY;
	glBegin( GL_LINES ); 
	for( i=0; i<majorNumY; i++ ) {
		glVertex2d( x0, y );
		glVertex2d( x1, y );
		y += majorIncY;
	}
	glEnd();


	//-----------------------------------------------------------------------
	// PLOT PRINCIPAL GRIDLINES
	glLineWidth( 4.f );
	glColor3fv( gridAxisColor );
	glBegin( GL_LINES ); 
		glVertex2d( x0, 0 );
		glVertex2d( x1, 0 );
		glVertex2d( 0, y0 );
		glVertex2d( 0, y1 );
	glEnd();

	//-----------------------------------------------------------------------
	// DRAW AXIS LABELS
	// Note: the font draw requires coordinates in window space, not world.
	double winLinesOrg[3], winDataOrg[3];
	double linesOrg[3] = { majorBegX, majorBegY, 0 };
	double dataOrg[3]  = { x0, y0, 0 };

	zglProjectWorldCoordsOnScreenPlane( linesOrg, winLinesOrg );
	zglProjectWorldCoordsOnScreenPlane( dataOrg, winDataOrg );
		// the coords in winDataOrg are guaranteed to be onscreen 

	glPushMatrix();
	zglPixelMatrixFirstQuadrant();
	glColor3fv( gridLegendColor );

	double winX, winY;

	if( !skipXLabels ) {
		x = majorBegX;
		winX = winLinesOrg[0];
		winY = winDataOrg[1];  
		for( i=0; i<majorNumX; i++ ) {
			if( fabs(x) < DBL_EPSILON ) {
				x = 0;
				x += majorIncX;
				winX += majorIncX / pixDX;
				continue;
					// printing the x=0 label often interferes with y-axis labelling in
					// a confusing and sometimes misleading way.
			}
			if( logx ) {
				zglFontPrint( ZTmpStr( "1e%g", x ), (float)winX+2, (float)winY, "controls" );
			}
			else {
				zglFontPrint( ZTmpStr( "%g", x ), (float)winX+2, (float)winY, "controls" );
			}
			x += majorIncX;
			winX += majorIncX / pixDX;
		}
	}

	if( !skipYLabels ) {
		y = majorBegY;
		winX = winDataOrg[0];
		winY = winLinesOrg[1];
		for( i=0; i<majorNumY; i++ ) {
			if( fabs(y) < DBL_EPSILON ) {
				y = 0;
			}
			if( logy ) {
				zglFontPrint( ZTmpStr( "10e%g", y ), (float)winX+2, (float)winY, "controls" );
			}
			else {
				zglFontPrint( ZTmpStr( "%g", y ), (float)winX+2, (float)winY, "controls" );
			}
			y += majorIncY;
			winY += majorIncY / pixDY;
		}
	}

	glPopMatrix();
}

void plotGridlines( float x0, float y0, float x1, float y1, float xd, float yd ) {

	// @TODO: cleanup

	float l = x0;
	float r = x1;
	float b = y0;
	float t = y1;
	if( r - l <= 0 || t - b <= 0 ) {
		return;
	}

	// PIXEL size in world coords
	float dx = xd;
	float dy = yd;

	// PLOT the grid lines
	float logPowerX = floorf( logf( r - l ) / logf( 10.f ) );
	float logPowerY = floorf( logf( t - b ) / logf( 10.f ) );
	float gridStepX = powf( 10.f, logPowerX );
	// ADJUST gridsteps if current value will result in a single label
	// that will likely render offscreen;
	if( true && gridStepX * 1.10f > r-l ) {
		gridStepX /= 10.f;
	}
	float gridStepY = powf( 10.f, logPowerY );
	if( true && gridStepY * 1.10f > t-b ) {
		gridStepY /= 10.f;
	}
	float gridL = floorf( l / gridStepX ) * gridStepX - gridStepX;
	float gridR = floorf( r / gridStepX ) * gridStepX + gridStepX;
	float gridB = floorf( b / gridStepY ) * gridStepY - gridStepY;
	float gridT = floorf( t / gridStepY ) * gridStepY + gridStepY;

	float mantL = gridL * powf( 10.f, -logPowerX );
	float mantB = gridB * powf( 10.f, -logPowerY );

	// PLOT the minor grid
	float x, y;
	glLineWidth( 1.f );
	glColor3fv( gridMinorColor );
	gridStepX /= 10.f;
	gridStepY /= 10.f;
	glBegin( GL_LINES ); 
		for( x=gridL; x<=gridR; x+=gridStepX ) {
			glVertex2f( (float)x, b );
			glVertex2f( (float)x, t );
		}
		for( y=gridB; y<=gridT; y+=gridStepY ) {
			glVertex2f( l, (float)y );
			glVertex2f( r, (float)y );
		}
	glEnd();

	// PLOT the major grid
	gridStepX *= 10.f;
	gridStepY *= 10.f;
	glLineWidth( 2.f );
	glColor3fv( gridMajorColor );
	glBegin( GL_LINES ); 
		for( x=gridL; x<=gridR; x+=gridStepX ) {
			glVertex2f( (float)x, b );
			glVertex2f( (float)x, t );
		}
		for( y=gridB; y<=gridT; y+=gridStepY ) {
			glVertex2f( l, (float)y );
			glVertex2f( r, (float)y );
		}
	glEnd();

	// PLOT the principal axis
	glLineWidth( 4.f );
	glColor3fv( gridAxisColor );
	glBegin( GL_LINES ); 
		glVertex2f( l, 0.f );
		glVertex2f( r, 0.f );
		glVertex2f( 0.f, b );
		glVertex2f( 0.f, t );
	glEnd();

	// PLOT the legend
	
	glColor3fv( gridLegendColor );

	// NEW: we don't want to plot the x-axis labels on the y=0 line if that is offscreen; so
	// let's make our origin actually the coordinate corresponding to the intersection of the
	// first actual visible major grids that are onscreen.  Easier is the lower right data point!
	float originX = l;
	float originY = b;

	float winLB[3], winRT[3], winOr[3];
	float worldOr[3] = { originX, originY, 0.f };
	float worldLB[3] = { gridL, gridB, 0.f };
	float worldRT[3] = { gridR, gridT, 0.f };
	zglProjectWorldCoordsOnScreenPlane( worldOr, winOr );
	zglProjectWorldCoordsOnScreenPlane( worldLB, winLB );
	zglProjectWorldCoordsOnScreenPlane( worldRT, winRT );

	if( xd <= 0.f || yd <= 0.f ) {
		return;
	}

	glPushMatrix();
	zglPixelMatrixFirstQuadrant();

	float pixW = (r-l) / xd;
	float pixH = (t-b) / yd;

	char *str;
	float winStep = (winRT[0]-winLB[0]) / ((gridR-gridL)/gridStepX);
	y = min( winOr[1]+pixH, max( 5.f, winOr[1] ) );
	if( winStep > 0.f ) {
		float worldX = gridL;
		for( x=winLB[0]; x<=winRT[0]; x+=winStep ) {

			if( fabs(worldX) < FLT_EPSILON ) {
				worldX = 0.f;
				worldX += gridStepX;
				continue;
					// printing the x=0 label often interferes with y-axis labelling in
					// a confusing and sometimes misleading way.
			}
			str = ZTmpStr("%g",worldX);
			zglFontPrint( str, x+2.f, y, "controls" );
			worldX += gridStepX;
		}

		winStep = (winRT[1]-winLB[1]) / ((gridT-gridB)/gridStepY);
		if( winStep > 0.f ) {
			float worldY = gridB;
			x = min( winOr[0]+pixW, max( 5.f, winOr[0] ) );
			for( y=winLB[1]; y<=winRT[1]; y+=winStep ) {

				if( fabs(worldY) < FLT_EPSILON ) {
					worldY = 0.f;
				}
				str = ZTmpStr("%g",worldY);

				zglFontPrint( str, x+2.f, y, "controls" );
				worldY += gridStepY;
			}
		}
	}

	glPopMatrix();
}

void plotGridlines3d( float x0, float y0, float z0, float x1, float y1, float z1, float xd, float yd, float zd ) {
}




