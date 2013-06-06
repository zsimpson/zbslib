// @ZBS {
//		*COMPONENT_NAME guiplot
//		*MODULE_OWNER_NAME glgui
//		*GROUP modules/gui
//		*REQUIRED_FILES guiplot.h
// }
// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#include "GL/gl.h"
#include "GL/glu.h"
// STDLIB includes:
#include "stdlib.h"
#include "string.h"
#include "assert.h"
#include "stdio.h"
#include "math.h"
#include "float.h"
// MODULE includes:
#include "guiplot.h"
// ZBSLIB includes:
#include "zglfont.h"
#include "zmathtools.h" // only used for round to int, reimplement
#include "zvec.h" // only for a inverse, consider reimplementing


// GUILabelSeries
//==================================================================================

void GUILabelSeries::clear() {
	for( int i=0; i<seriesData.count; i++ ) {
		free( seriesData[i] );
		seriesData[i] = 0;
	}
	seriesData.clear();
}

void GUILabelSeries::add( char *label ) {
	char *dupped = strdup(label);
	seriesData.add( dupped );
}

int GUILabelSeries::xCount() {
	return seriesData.count;
}

int GUILabelSeries::yCount() {
	return 0;
}

double *GUILabelSeries::getd( int x, int y ) {
	return 0;
}

char *GUILabelSeries::gets( int x, int y ) {
	return seriesData[ x ];
}

// GUI1dSeries
//==================================================================================

void GUI1dSeries::clear() {
	seriesData.clear();
}

void GUI1dSeries::add( double x ) {
	seriesData.add(x);
}

int GUI1dSeries::xCount() {
	return seriesData.count;
}

int GUI1dSeries::yCount() {
	return 0;
}

double *GUI1dSeries::getd( int x, int y ) {
	assert( y == 0 );
	return &seriesData[ x ];
}

char *GUI1dSeries::gets( int x, int y ) {
	return 0;
}

// GUI2dSeries
//==================================================================================

void GUI2dSeries::clear() {
	seriesData.clear();
}

void GUI2dSeries::add( double x, double y ) {
	seriesData.add(x);
	seriesData.add(y);
}

int GUI2dSeries::xCount() {
	return seriesData.count / 2;
}

int GUI2dSeries::yCount() {
	return 0;
}

double *GUI2dSeries::getd( int x, int y ) {
	assert( y == 0 );
	return &seriesData[ x * 2 ];
}

char *GUI2dSeries::gets( int x, int y ) {
	return 0;
}

// GUI3dSeries
//==================================================================================

GUI2dArraySeries::GUI2dArraySeries() {
	sizeX = 0;
	sizeY = 0;
}

void GUI2dArraySeries::clear() {
	seriesData.clear();
}

void GUI2dArraySeries::setSize( int x, int y ) {
	seriesData.expandTo(x*y);
	sizeX = x;
	sizeY = y;
}

void GUI2dArraySeries::add( int x, int y, double z ) {
	assert( x >= 0 && x < sizeX && y >= 0 && y < sizeY );
	*(double *)&seriesData[ sizeX * y + x ] = z;
}

int GUI2dArraySeries::xCount() {
	return sizeX;
}

int GUI2dArraySeries::yCount() {
	return sizeY;
}

double *GUI2dArraySeries::getd( int x, int y ) {
	return &seriesData[ sizeX * y + x ];
}

char *GUI2dArraySeries::gets( int x, int y ) {
	return 0;
}

// GUIHistogramSeries
//==================================================================================

GUIHistogramSeries::GUIHistogramSeries( int *data, int count, int numBuckets, int minV, int maxV ) {
	// TRANFORM into double
	double *temp = (double *)malloc( sizeof(double) * count );
	for( int i=0; i<count; i++ ) {
		temp[i] = (double)data[i];
	}
	init( temp, count, numBuckets, minV, maxV );
	free( temp );
}

GUIHistogramSeries::GUIHistogramSeries( float *data, int count, int numBuckets, float minV, float maxV ) {
	// TRANFORM into double
	double *temp = (double *)malloc( sizeof(double) * count );
	for( int i=0; i<count; i++ ) {
		temp[i] = (double)data[i];
	}
	init( temp, count, numBuckets, minV, maxV );
	free( temp );
}

GUIHistogramSeries::GUIHistogramSeries( double *data, int count, int numBuckets, double minV, double maxV ) {
	init( data, count, numBuckets, minV, maxV );
}

void GUIHistogramSeries::init( double *data, int count, int numBuckets, double minV, double maxV ) {
	if( minV == 0.0 && maxV == 0.0 ) {
		// FIND min and max
		minV =  DBL_MAX;
		maxV = -DBL_MAX;
		for( int i=0; i<count; i++ ) {
			minV = min( data[i], minV );
			maxV = max( data[i], maxV );
		}
	}
	seriesData = (double *)malloc( sizeof(double) * numBuckets );
	memset( seriesData, 0, sizeof(double) * numBuckets );
	bucketCount = numBuckets;
	double numBucketsD = (double)numBuckets;
	double rangeD = maxV - minV;
	for( int i=0; i<count; i++ ) {
		if( data[i] >= minV && data[i] <= maxV ) {
			int bucket = int( numBucketsD * (data[i] - minV) / rangeD );
			bucket = max( 0, min( numBuckets-1, bucket ) );
			seriesData[bucket]++;
		}
	}
}

void GUIHistogramSeries::clear() {
	if( seriesData ) {
		free( seriesData );
	}
	seriesData = 0;
}

int GUIHistogramSeries::xCount() {
	return bucketCount;
}

int GUIHistogramSeries::yCount() {
	return 0;
}

double *GUIHistogramSeries::getd( int x, int y ) {
	return &seriesData[ x ];
}

char *GUIHistogramSeries::gets( int x, int y ) {
	return 0;
}

// GUIPlot
//==================================================================================

void GUIPlot::init() {
	GUIPanel::init();
}

int GUIPlot::getSeriesCount() {
	return series.count;
}

GUIPlotSeries *GUIPlot::seriesGet( int i ) {
	return series[i];
}

void GUIPlot::seriesRemoveAll() {
	for( int i=0; i<series.count; i++ ) {
		series[i]->clear();
		delete series[i];
		series[i] = 0;
	}
	series.clear();
}

void GUIPlot::seriesAdd( GUIPlotSeries *s ) {
	series.add( s );
}

// GUI2DPlot
//==================================================================================

void GUI2DPlot::init() {
	GUIPlot::init();
	setAttrD( "axisScaleX", 1.0 );
	setAttrD( "axisScaleY", 1.0 );
}

void GUI2DPlot::buildSpaceFromMinMax() {
	attrD( minX );
	attrD( minY );
	attrD( maxX );
	attrD( maxY );
	attrD( axisScaleX );
	attrD( axisScaleY );
	attrD( marginL );
	attrD( marginR );
	attrD( marginT );
	attrD( marginB );
	glPushMatrix();
		glLoadIdentity();
		glTranslated( marginL, marginB, 0.0 );
		glScaled( axisScaleX, axisScaleY, 1.0 );
		glScaled( myW - (marginL + marginR), myH - (marginT + marginB), 1.0 );
		glScaled( 1.0/(maxX-minX), 1.0/(maxY-minY), 1.0 );
		glTranslated( -minX, -minY, 0.0 );
		glGetDoublev( GL_MODELVIEW_MATRIX, (double *)tm );

		// @todo: including zvec just for this inverse, consider reimplementing inverse here
		DMat4 _inversetm( tm );
		_inversetm.inverse();
		memcpy( inversetm, &_inversetm, sizeof(inversetm) );
	glPopMatrix();
	setAttrI( "builtSpace", 1 );
}

void GUI2DPlot::transform2( double *inPt, double *outPt ) {
	outPt[0] = inPt[0]*tm[0][0] + inPt[1]*tm[1][0] + tm[3][0];
	outPt[1] = inPt[0]*tm[0][1] + inPt[1]*tm[1][1] + tm[3][1];
}

void GUI2DPlot::inverseTransform2( double *inPt, double *outPt ) {
	outPt[0] = inPt[0]*inversetm[0][0] + inPt[1]*inversetm[1][0] + inversetm[3][0];
	outPt[1] = inPt[0]*inversetm[0][1] + inPt[1]*inversetm[1][1] + inversetm[3][1];
}

void GUI2DPlot::handleMsg( ZMsg *msg ) {
	if( zmsgIs(type,MouseClickOn) && zmsgIs(dir,D) ) {
		if( getAttrS("allowMouseRescale")[0] == zmsgS(which)[0] ) {
			zMsgUsed();
			requestExclusiveMouse( 1, 1 );
			setAttrF( "dragStartX", zmsgF(localX) );
			setAttrF( "dragStartY", zmsgF(localY) );
			setAttrF( "dragStartAxisScaleX", getAttrF("axisScaleX") );
			setAttrF( "dragStartAxisScaleY", getAttrF("axisScaleY") );
		}
	}
	else if( zmsgIs(type,MouseDrag) ) {
		attrF(marginL);
		attrF(dragStartX);
		attrF(dragStartY);
		float deltaX = zmsgF(localX) - dragStartX;
		float deltaY = zmsgF(localY) - dragStartY;
		attrD( axisScaleX );
		attrD( axisScaleY );
		attrD( dragStartAxisScaleX );
		attrD( dragStartAxisScaleY );
		axisScaleX = dragStartAxisScaleX * exp( deltaX / 100.0 );
		axisScaleY = dragStartAxisScaleY * exp( deltaY / 100.0 );
		setAttrD( "axisScaleX", axisScaleX );
		setAttrD( "axisScaleY", axisScaleY );
		buildSpaceFromMinMax();
	}
	else if( zmsgIs(type,MouseReleaseDrag) ) {
		requestExclusiveMouse( 1, 0 );
	}
}

void GUI2DPlot::computeGridLines() {
	attrI( gridMinorColor );
	attrI( gridMajorColor );

	attrI( gridVert );
	attrF( minY );
	attrF( axisScaleY );

	// INVERSE transform to find the plot coords of the top and bottom
	double rawPt[2] = { 0, 0 };
	double transPtB[2];
	inverseTransform2( rawPt, transPtB );
	rawPt[1] = myH;
	double transPtT[2];
	inverseTransform2( rawPt, transPtT );
	double vertSize = transPtT[1] - transPtB[1];

	double l = log10( vertSize );
	double ip = 0.0;
	double fracPart = modf( l, &ip );
	l = floor( l ) - 1.0;

	// CHECK for halving the line count
	gridMinorStep = pow( 10.0, l );
	gridStepBase = 10;
	if( fracPart > 0.5 || (fracPart < 0.0 && fracPart > -0.5) ) {
		gridStepBase = 5;
		gridMinorStep *= 2.0;
	}
	double stepInverse = pow( 10.0, -l );

	// SOLVE for the bottom and top represented in minor steps
	gridBot = zmathRoundToInt( transPtB[1] / gridMinorStep );
	gridTop = zmathRoundToInt( transPtT[1] / gridMinorStep );

	// DRAW the grid
	if( gridVert ) {
		for( int h = gridBot; h <= gridTop; h ++ ) {
			double rawPt[2] = { 0, h*gridMinorStep };
			double transPt[2];
			transform2( rawPt, transPt );
			gridVertLines[h-gridBot] = transPt[1];
		}
	}
}

void GUI2DPlot::renderGrid() {
	attrD( marginL );
	attrD( marginR );
	attrI( gridMinorColor );
	attrI( gridMajorColor );

	// DRAW the grid
	glBegin( GL_LINES );
	for( int h = gridBot; h <= gridTop; h ++ ) {
		if( h % gridStepBase == 0 ) setColorI( gridMajorColor );
		else setColorI( gridMinorColor );
		glVertex2d( marginL, gridVertLines[h-gridBot] );
		glVertex2d( myW-marginR, gridVertLines[h-gridBot] );
	}
	glEnd();

	// DRAW the zero lines
	setColorI( gridMajorColor );
	glLineWidth( 2.f );
	double rawPt[2] = { 0, 0 };
	double transPt[2];
	transform2( rawPt, transPt );
	glBegin( GL_LINES );
		glVertex2d( marginL, transPt[1] );
		glVertex2d( myW-marginR, transPt[1] );
	glEnd();

}

void GUI2DPlot::renderLabels() {
	const int MAX_LABELS = 50;
	char labels[MAX_LABELS][16];
	int labelCount = 0;
	float labelPos[MAX_LABELS][2];

	attrS( gridLabelsVertFmt );
	if( !*gridLabelsVertFmt ) {
		gridLabelsVertFmt = "%g";
	}

	attrI( labelVertColor );
	if( labelVertColor ) {
		setColorI( labelVertColor );
	}

	for( int h = gridBot; h <= gridTop; h ++ ) {
		int mod = h % gridStepBase;
		if( mod == 0 || (gridStepBase == 10 && (mod == gridStepBase/2 || mod == -gridStepBase/2)) ) {
			sprintf( labels[labelCount], gridLabelsVertFmt, h*gridMinorStep );

			double rawPt[2] = { 0, h };
			double transPt[2];
			transform2( rawPt, transPt );

			labelPos[labelCount][0] = 0.f;
			labelPos[labelCount][1] = (float)gridVertLines[h-gridBot];
			labelCount++;

			if( labelCount >= MAX_LABELS ) {
				break;
			}
		}
	}

	// DRAW the labels
	void *fontPtr = zglFontGet( getAttrS("font") );
	for( int i=0; i<labelCount; i++ ) {
		zglFontPrint( labels[i], labelPos[i][0], labelPos[i][1], 0, 1, fontPtr );
	}

//	// DRAW the line extensions from the grid
//	glLineWidth( 1.f );
//	attrD( marginL );
//	glBegin( GL_LINES );
//	for( i=0; i<labelCount; i++ ) {
//		glVertex2d( 0.0, labelPos[i][1] );
//		glVertex2d( marginL, labelPos[i][1] );
//	}
//	glEnd();
}

// GUILinePlot
//==================================================================================

GUI_TEMPLATE_IMPLEMENT(GUILinePlot);

void GUILinePlot::init() {
	GUI2DPlot::init();
	setAttrI( "color", 0xFFFFFFFF );
}

void GUILinePlot::render() {
	GUI2DPlot::render();

	if( getSeriesCount() <= 0 ) {
		return;
	}

	attrF( lineWidth );
	if( lineWidth ) {
		glLineWidth( lineWidth );
	}

	// FIND if there is a label series
	int startSeries = series[0]->isLabel() ? 1 : 0;
	for( int i=startSeries; i<getSeriesCount(); i++ ) {
		char buf[16];
		sprintf( buf, "seriesColor-%d", i );
		int color = getAttrI( buf );
		if( color ) {
			setColorI( color );
		}

		int count = series[i]->xCount();
		glBegin( GL_LINE_STRIP );
			for( int x=0; x<count; x++ ) {
				double *rawPt = series[i]->getd(x);
				double transPt[2];
				transform2( rawPt, transPt );
				glVertex2dv( transPt );
			}
		glEnd();
	}

	glLineWidth( 1.f );
	computeGridLines();
	renderGrid();
	renderLabels();
}

// GUIAreaPlot
//==================================================================================

GUI_TEMPLATE_IMPLEMENT(GUIAreaPlot);

void GUIAreaPlot::init() {
	GUI2DPlot::init();
	setAttrI( "color", 0xFFFFFFFF );
}

void GUIAreaPlot::render() {
	GUI2DPlot::render();

	if( getSeriesCount() <= 0 ) {
		return;
	}

	attrF( lineWidth );
	if( lineWidth ) {
		glLineWidth( lineWidth );
	}

	// FIND if there is a label series
	int startSeries = series[0]->isLabel() ? 1 : 0;
	for( int i=startSeries; i<getSeriesCount(); i++ ) {
		char buf[16];
		sprintf( buf, "seriesColor-%d", i );
		int color = getAttrI( buf );
		if( color ) {
			setColorI( color );
		}

		int count = series[i]->xCount();
		glBegin( GL_TRIANGLE_STRIP );
			for( int x=0; x<count; x++ ) {
				double *rawPt0 = series[i]->getd(x);
				double transPt0[2];
				double rawPt1[2];
				rawPt1[0] = *series[i]->getd(x);
				rawPt1[1] = 0.0;
				double transPt1[2];
				transform2( rawPt0, transPt0 );
				transform2( rawPt1, transPt1 );

				glVertex2dv( transPt0 );
				glVertex2dv( transPt1 );
			}
		glEnd();
	}

	computeGridLines();
	renderGrid();
	renderLabels();
}

// GUIColumnPlot
//==================================================================================

GUI_TEMPLATE_IMPLEMENT(GUIColumnPlot);

void GUIColumnPlot::init() {
	GUI2DPlot::init();
}

void GUIColumnPlot::render() {
	GUI2DPlot::render();

	if( getSeriesCount() <= 0 ) {
		return;
	}

	attrI( seriesInterleaved );

	// FIND if there is a label series
	int startSeries = series[0]->isLabel() ? 1 : 0;
	int seriesCount = getSeriesCount();
	for( int i=startSeries; i<seriesCount; i++ ) {
		char buf[32];
		sprintf( buf, "seriesColor-%d", i );
		int color = getAttrI( buf );
		sprintf( buf, "seriesEdgeColor-%d", i );
		int edgeColor = getAttrI( buf );

		int count = series[i]->xCount();
		glBegin( GL_QUADS );
			for( int x=0; x<count; x++ ) {
				double *y = series[i]->getd(x);
				double rawPt0[2];
				double rawPt1[2];
				if( seriesInterleaved ) {
					rawPt0[0] = (double)x * seriesCount + i;
					rawPt0[1] = 0.0;
					rawPt1[0] = (double)x * seriesCount + i + 1;
					rawPt1[1] = *y;
				}
				else {
					rawPt0[0] = (double)x;
					rawPt0[1] = 0.0;
					rawPt1[0] = (double)x+1;
					rawPt1[1] = *y;
				}
				double transPt0[2], transPt1[2];
				transform2( rawPt0, transPt0 );
				transform2( rawPt1, transPt1 );

				int offset = 0;
				if( edgeColor ) {
					setColorI( edgeColor );
					glVertex2d( transPt0[0], transPt0[1] );
					glVertex2d( transPt0[0], transPt1[1] );
					glVertex2d( transPt1[0], transPt1[1] );
					glVertex2d( transPt1[0], transPt0[1] );
					offset = 1;
				}

				if( color ) {
					setColorI( color );
				}
				glVertex2d( transPt0[0]+offset, transPt0[1]+offset );
				glVertex2d( transPt0[0]+offset, transPt1[1]-offset );
				glVertex2d( transPt1[0]-offset, transPt1[1]-offset );
				glVertex2d( transPt1[0]-offset, transPt0[1]+offset );
			}
		glEnd();
	}

	computeGridLines();
	renderGrid();
	renderLabels();
}

// GUIScatterPlot
//==================================================================================

GUI_TEMPLATE_IMPLEMENT(GUIScatterPlot);

void GUIScatterPlot::init() {
	GUI2DPlot::init();
	setAttrI( "color", 0xFFFFFFFF );
}

void GUIScatterPlot::render() {
	GUI2DPlot::render();

	if( getSeriesCount() <= 0 ) {
		return;
	}

	attrF( lineWidth );
	if( lineWidth ) {
		glLineWidth( lineWidth );
	}

	// FIND if there is a label series
	for( int i=0; i<getSeriesCount(); i++ ) {
		char buf[16];
		sprintf( buf, "seriesColor-%d", i );
		int color = getAttrI( buf );
		if( color ) {
			setColorI( color );
		}

		int count = series[i]->xCount();
		glPointSize( 3.f );
		glBegin( GL_POINTS );
			for( int j=0; j<count; j++ ) {
				double *rawPt0 = series[i]->getd(j);
				double transPt0[2];
				transform2( rawPt0, transPt0 );
				glVertex2dv( transPt0 );
			}
		glEnd();
	}

	computeGridLines();
	renderGrid();
	renderLabels();
}

// GUI3DPlot
//==================================================================================

void GUI3DPlot::init() {
	GUIPlot::init();
	setAttrD( "axisScaleX", 1.0 );
	setAttrD( "axisScaleY", 1.0 );
}

void GUI3DPlot::buildSpaceFromMinMax() {
	attrD( minX );
	attrD( minY );
	attrD( maxX );
	attrD( maxY );
	attrD( axisScaleX );
	attrD( axisScaleY );
	attrD( marginL );
	attrD( marginR );
	attrD( marginT );
	attrD( marginB );
	glPushMatrix();
		glLoadIdentity();
		glTranslated( marginL, marginB, 0.0 );
		glScaled( axisScaleX, axisScaleY, 1.0 );
		glScaled( myW - (marginL + marginR), myH - (marginT + marginB), 1.0 );
		glScaled( 1.0/(maxX-minX), 1.0/(maxY-minY), 1.0 );
		glTranslated( -minX, -minY, 0.0 );
		glGetDoublev( GL_MODELVIEW_MATRIX, (double *)tm );

		// @todo: including zvec just for this inverse, consider reimplementing inverse here
		DMat4 _inversetm( tm );
		_inversetm.inverse();
		memcpy( inversetm, &_inversetm, sizeof(inversetm) );
	glPopMatrix();
	setAttrI( "builtSpace", 1 );
}

//void GUI3DPlot::transform2( double *inPt, double *outPt ) {
//	outPt[0] = inPt[0]*tm[0][0] + inPt[1]*tm[1][0] + tm[3][0];
//	outPt[1] = inPt[0]*tm[0][1] + inPt[1]*tm[1][1] + tm[3][1];
//}
//
//void GUI3DPlot::inverseTransform2( double *inPt, double *outPt ) {
//	outPt[0] = inPt[0]*inversetm[0][0] + inPt[1]*inversetm[1][0] + inversetm[3][0];
//	outPt[1] = inPt[0]*inversetm[0][1] + inPt[1]*inversetm[1][1] + inversetm[3][1];
//}

void GUI3DPlot::handleMsg( ZMsg *msg ) {
	if( zmsgIs(type,MouseClickOn) && zmsgIs(dir,D) ) {
		if( getAttrS("allowMouseRescale")[0] == zmsgS(which)[0] ) {
			zMsgUsed();
			requestExclusiveMouse( 1, 1 );
			setAttrF( "dragStartX", zmsgF(localX) );
			setAttrF( "dragStartY", zmsgF(localY) );
			setAttrF( "dragStartAxisScaleX", getAttrF("axisScaleX") );
			setAttrF( "dragStartAxisScaleY", getAttrF("axisScaleY") );
		}
	}
	else if( zmsgIs(type,MouseDrag) ) {
		attrF(marginL);
		attrF(dragStartX);
		attrF(dragStartY);
		float deltaX = zmsgF(localX) - dragStartX;
		float deltaY = zmsgF(localY) - dragStartY;
		attrD( axisScaleX );
		attrD( axisScaleY );
		attrD( dragStartAxisScaleX );
		attrD( dragStartAxisScaleY );
		axisScaleX = dragStartAxisScaleX * exp( deltaX / 100.0 );
		axisScaleY = dragStartAxisScaleY * exp( deltaY / 100.0 );
		setAttrD( "axisScaleX", axisScaleX );
		setAttrD( "axisScaleY", axisScaleY );
		buildSpaceFromMinMax();
	}
	else if( zmsgIs(type,MouseReleaseDrag) ) {
		requestExclusiveMouse( 1, 0 );
	}
}

void GUI3DPlot::computeGridLines() {
//	attrI( gridMinorColor );
//	attrI( gridMajorColor );
//
//	attrI( gridVert );
//	attrF( minY );
//	attrF( axisScaleY );
//
//	// INVERSE transform to find the plot coords of the top and bottom
//	double rawPt[2] = { 0, 0 };
//	double transPtB[2];
//	inverseTransform2( rawPt, transPtB );
//	rawPt[1] = myH;
//	double transPtT[2];
//	inverseTransform2( rawPt, transPtT );
//	double vertSize = transPtT[1] - transPtB[1];
//
//	double l = log10( vertSize );
//	double ip = 0.0;
//	double fracPart = modf( l, &ip );
//	l = floor( l ) - 1.0;
//
//	// CHECK for halving the line count
//	gridMinorStep = pow( 10.0, l );
//	gridStepBase = 10;
//	if( fracPart > 0.5 || (fracPart < 0.0 && fracPart > -0.5) ) {
//		gridStepBase = 5;
//		gridMinorStep *= 2.0;
//	}
//	double stepInverse = pow( 10.0, -l );
//
//	// SOLVE for the bottom and top represented in minor steps
//	gridBot = zmathRoundToInt( transPtB[1] / gridMinorStep );
//	gridTop = zmathRoundToInt( transPtT[1] / gridMinorStep );
//
//	// DRAW the grid
//	if( gridVert ) {
//		for( int h = gridBot; h <= gridTop; h ++ ) {
//			double rawPt[2] = { 0, h*gridMinorStep };
//			double transPt[2];
//			transform2( rawPt, transPt );
//			gridVertLines[h-gridBot] = transPt[1];
//		}
//	}
}

void GUI3DPlot::renderGrid() {
//	attrD( marginL );
//	attrD( marginR );
//	attrI( gridMinorColor );
//	attrI( gridMajorColor );
//
//	// DRAW the grid
//	glBegin( GL_LINES );
//	for( int h = gridBot; h <= gridTop; h ++ ) {
//		if( h % gridStepBase == 0 ) setColorI( gridMajorColor );
//		else setColorI( gridMinorColor );
//		glVertex2d( marginL, gridVertLines[h-gridBot] );
//		glVertex2d( myW-marginR, gridVertLines[h-gridBot] );
//	}
//	glEnd();
//
//	// DRAW the zero lines
//	setColorI( gridMajorColor );
//	glLineWidth( 2.f );
//	double rawPt[2] = { 0, 0 };
//	double transPt[2];
//	transform2( rawPt, transPt );
//	glBegin( GL_LINES );
//		glVertex2d( marginL, transPt[1] );
//		glVertex2d( myW-marginR, transPt[1] );
//	glEnd();
//
}

void GUI3DPlot::renderLabels() {
//	const int MAX_LABELS = 50;
//	char labels[MAX_LABELS][16];
//	int labelCount = 0;
//	float labelPos[MAX_LABELS][2];
//
//	attrS( gridLabelsVertFmt );
//	if( !*gridLabelsVertFmt ) {
//		gridLabelsVertFmt = "%g";
//	}
//
//	attrI( labelVertColor );
//	if( labelVertColor ) {
//		setColorI( labelVertColor );
//	}
//
//	for( int h = gridBot; h <= gridTop; h ++ ) {
//		int mod = h % gridStepBase;
//		if( mod == 0 || (gridStepBase == 10 && (mod == gridStepBase/2 || mod == -gridStepBase/2)) ) {
//			sprintf( labels[labelCount], gridLabelsVertFmt, h*gridMinorStep );
//
//			double rawPt[2] = { 0, h };
//			double transPt[2];
//			transform2( rawPt, transPt );
//
//			labelPos[labelCount][0] = 0.f;
//			labelPos[labelCount][1] = (float)gridVertLines[h-gridBot];
//			labelCount++;
//
//			if( labelCount >= MAX_LABELS ) {
//				break;
//			}
//		}
//	}
//
//	// DRAW the labels
//	void *fontPtr = zglFontGet( getAttrS("font") );
//	for( int i=0; i<labelCount; i++ ) {
//		zglFontPrint( labels[i], labelPos[i][0], labelPos[i][1], 0, 1, fontPtr );
//	}

	// DRAW the line extensions from the grid
//	glLineWidth( 1.f );
//	attrD( marginL );
//	glBegin( GL_LINES );
//	for( i=0; i<labelCount; i++ ) {
//		glVertex2d( 0.0, labelPos[i][1] );
//		glVertex2d( marginL, labelPos[i][1] );
//	}
//	glEnd();
}

// GUI3DSurfacePlot
//==================================================================================

GUI_TEMPLATE_IMPLEMENT(GUI3DSurfacePlot);

void GUI3DSurfacePlot::init() {
	GUI3DPlot::init();
	setAttrI( "color", 0xFFFFFFFF );
}

#include "zgltools.h"
	// @todo eliminate this.  Just for pushing and popping both

void GUI3DSurfacePlot::render() {
	GUI3DPlot::render();

	if( getSeriesCount() <= 0 ) {
		return;
	}

	glColor3ub( 255, 255, 255 );
	glBegin( GL_LINES );
	glVertex2f( 0.f, 0.f );
	glVertex2f( 100.f, 100.f );
	glEnd();


	zglPushBoth();	
//	glMatrixMode(GL_PROJECTION);
//	glLoadIdentity();
//	gluPerspective( 30.f, myW / myH, 0.1, 10000 );
	glMatrixMode(GL_MODELVIEW);
	gluLookAt( 0.0, 0.0, 30.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0 );
		// @todo: set the eye algorithmicly
//	zTrackballSetupView();
		// @todo: extract trackball dependency

	int l = 0;
	int t = 0;
	int r = series[0]->xCount();
	int b = series[0]->yCount();

	glTranslatef( (float)-(r-l)/2, (float)-(b-t)/2, (float)0.f );

	glEnable( GL_DEPTH_TEST );

	// BUILD color table
	// @todo: factor this out
//	float color[256][4];
//	for( i=0; i<256; i++ ) {
//		float h = ( (i * 186 / 256 + 80) % 256 ) / 256.f;
//		zHSVF_to_RGBF( 0.99f-h, 1.f, 1.f, color[i][0], color[i][1], color[i][2] );
//		color[i][3] = 1.f;
//	}
//	color[0][3] = 0.f;

	double dataScale = 1.0;

	glColor3ub( 255, 255, 255 );
	for( int y=t; y<b-1; y++ ) {
		glBegin( GL_TRIANGLE_STRIP );
		for( int x=l; x<r; x++ ) {
			double *p0 = series[0]->getd(x,y);
			double *p1 = series[0]->getd(x,y+1);

			//glColor3f( p0 * colorScale, 0, 1.f-p0 * colorScale );
//			int c = max( 0, min( 255, (int)(p0*colorScale)) );
//			glColor4fv( color[c] );
			glVertex3d( (double)x, (double)y, *p0 * dataScale );

			//glColor3f( p1 * colorScale, 0, 1.f-p1 * colorScale );
//			c = max( 0, min( 255, (int)(p1*colorScale)) );
//			glColor4fv( color[c] );
			glVertex3d( (double)x, (double)(y+1), *p1 * dataScale );
		}
		glEnd();
	}


	// @todo: label series
//	// FIND if there is a label series
//	int startSeries = series[0]->isLabel() ? 1 : 0;
//	for( int i=startSeries; i<getSeriesCount(); i++ ) {
//		char buf[16];
//		sprintf( buf, "seriesColor-%d", i );
//		int color = getAttrI( buf );
//		if( color ) {
//			setColorI( color );
//		}
//
//		int count = series[i]->xCount();
//		glBegin( GL_LINE_STRIP );
//			for( int x=0; x<count; x++ ) {
//				double *rawPt = series[i]->getd(x);
//				double transPt[2];
//				transform2( rawPt, transPt );
//				glVertex2dv( transPt );
//			}
//		glEnd();
//	}

//	computeGridLines();
//	renderGrid();
//	renderLabels();
	zglPopBoth();	
}

