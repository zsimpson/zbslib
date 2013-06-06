#ifndef ZUIPLOT
#define ZUIPLOT

#include "zui.h"

typedef void PlotRangedCallback( int userid, float x0, float y0, float x1, float y1, float xd, float yd );
typedef void PlotUnrangedCallback( int userid );

struct ZUIPlot : ZUIPanel {
	ZUI_DEFINITION(ZUIPlot,ZUIPanel);

	// gridXLog = [0|1]
	// gridYLog = [0|1]
	// gridAxisColor = [color]
	// gridMajorColor = [color]
	// gridMinorColor = [color]
	// allowScaleX = [0|1] can the mouse interface permit scaling x
	// allowScaleY = [0|1] can the mouse interface permit scaling x
	// transX = [number] translation
	// transY = [number] translation
	// scaleX = [number] the span of plot coords over width of zui, default = 1
	// scaleY = [number] the span of plot coords over height of zui, default = 1
	// x0 = [number] min (left) domain of plot at default (trans=0, scale=1)
	// x1 = [number] max (right) domain of plot at default (trans=0, scale=1)
	// y0 = [number] min (bottom) domain of plot at default (trans=0, scale=1)
	// y1 = [number] max (top) domain of plot at default (trans=0, scale=1)
	// rangedCallback = [func pointer]
	// unrangedCallback = [func pointer]


	ZUIPlot() : ZUIPanel() { }
	virtual void factoryInit();
	virtual void render();
	virtual ZUILayoutCell *layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly=1 );
	virtual void handleMsg( ZMsg *msg );

protected:
	// helper functions for mouse coordinates
	float mapCoordX( float localX, int reportExponent=1 );
	float mapCoordY( float localY, int reportExponent=1 );
		// if the axis is on a log-scale, then reportExponent means the value reported is 
		// the exponent, which allows better precision; the actual mapped value, reported
		// in the case that reportExponent==0, is 10^exponent
	void sendMouseMsg( char* mouseMsg, float localX, float localY, int shift, int ctrl, int alt );
};


#endif

