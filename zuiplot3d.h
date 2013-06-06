#ifndef ZUIPLOT3D
#define ZUIPLOT3D

#include "zui.h"
#include "zviewpoint2.h"

typedef void PlotUnrangedCallback( int userid );
typedef void AnimCallback( ZUI *plot, ZMsg *msg );


struct ZUIPlot3d : ZUIPanel {
	ZUI_DEFINITION(ZUIPlot3d,ZUIPanel);

	// unrangedCallback = [func pointer]
	// animCallback		= [func pointer]

	ZViewpoint vp;
	float rotateX;
	float rotateY;

	ZUIPlot3d() : ZUIPanel() {}
	virtual void factoryInit();
	virtual void render();
	virtual ZUILayoutCell *layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly=1 );
	virtual void handleMsg( ZMsg *msg );

	void stepAnimation( int step );
		// track frame number to support tracking state info for simple anims

	void getSceneDimsWithBorder( FVec3 &centroid, FVec3 &radius, float &x0, float &x1,
								   float &y0, float &y1, float &z0, float &z1, float borderPercent );
		// compute centroid (object coords) and radius of scene, as well as clipping plane values x0...
		// based on the scene boundaries stored in our hash (getI( "x0" ) etc) and the borderPercent
		
	void getSceneScaledDimsWithBorder( FVec3 &centroid, FVec3 &radius, FVec3 &scale,
									   float &_x0, float &_x1, float &_y0, float &_y1, float &_z0, float &_z1,
									   float borderPercent );
		// as above, but the scene should be scaled such that a symmetric volume is displayed -- as is desired
		// for plotting 3-dimensional data whose axes are on different scales.  See ::render for usage.

	void drawLegendAxes();
		// draw 3 dimensional axes with tickmarks and labeling

	void sendMouseMsg( char* mouseMsg, float localX, float localY, int shift, int ctrl, int alt );
		// Send a mouse message.
};


#endif

