// @ZBS {
//		*COMPONENT_NAME guigraph
//		*MODULE_OWNER_NAME glgui
//		*GROUP modules/gui
//		*REQUIRED_FILES guigraph.h
// }
// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#include "GL/gl.h"
// STDLIB includes:
#include "string.h"
#include "stdlib.h"
// MODULE includes:
#include "guigraph.h"
// ZBSLIB includes:

GUI_TEMPLATE_IMPLEMENT(GUIGraph);

void GUIGraph::init() {
	GUIPanel::init();
	setAttrI( "maxCount", 100 );
	setAttrF( "maxScale", 0.f );
}

void GUIGraph::render() {
	GUIPanel::render();

	int maxCount = getAttrI("maxCount");
	float scale = getAttrF("maxScale");
	if( scale == 0.f ) {
		// CALCULATE auto scale
		for( int i=0; i<history.count; i++ ) {
			scale = max( history.vec[i], scale );
		}
	}
	glColor3ub( 128, 128, 128 );
	glBegin( GL_LINES );
	for( int i=0; i<history.count; i++ ) {
		glVertex2f( (float)i*myW/(float)maxCount, 0.f );
		glVertex2f( (float)i*myW/(float)maxCount, (history.vec[i]*myH / scale) );
	}
	glEnd();
}

void GUIGraph::addAndShift( float x ) {
	int maxCount = getAttrI("maxCount");
	if( history.count >= maxCount ) {
		history.shiftLeft(0,1);
	}
	history.add( x );
}

void GUIGraph::queryOptSize( float &w, float &h ) {
	w = 100;
	h = 30;
}

