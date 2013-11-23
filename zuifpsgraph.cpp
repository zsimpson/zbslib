// @ZBS {
//		*COMPONENT_NAME zuifpsgraph
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuifpsgraph.cpp
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
// MODULE includes:
#include "zuifpsgraph.h"
// ZBSLIB includes:
#include "ztime.h"
#include "zui.h"

ZUI_IMPLEMENT(ZUIFPSGraph,ZUIPanel);

void ZUIFPSGraph::factoryInit() {
	ZUIPanel::factoryInit();
	putI( "maxCount", 100 );
	putF( "maxScale", 0.f );
	setUpdate( 1 );
}

void ZUIFPSGraph::update() {
	// FPS graph is always animating so it is always dirty
	dirty();
}

void ZUIFPSGraph::render() {
	if( zTimeFrameCount != lastFrameCount ) {
		lastFrameCount = zTimeFrameCount;
		int maxCount = getI("maxCount");
		maxCount = min( HISTORY_SIZE, maxCount );
		histCount++;
		if( histCount >= maxCount ) {
			memmove( &history[0], &history[1], sizeof(float) * (maxCount-1) );
		}
		histCount = min( maxCount-1, histCount );
		history[histCount] = (float)zTimeFPS;
	}


	ZUIPanel::render();
	
	glLineWidth( 1.f );

	int maxCount = getI("maxCount");
	float scale = getF("scale");
	if( scale == 0.f ) {
		// CALCULATE auto scale
		for( int i=0; i<histCount; i++ ) {
			scale = max( history[i], scale );
		}
	}

	int resetScissor = 0;
	if( ! getI("clipToWindow") ) {
		resetScissor++;
		glPushAttrib( GL_ENABLE_BIT );
		glDisable( GL_SCISSOR_TEST );
	}

	float w, h;
	getWH( w, h );

	scale = scale / h;

	glBegin( GL_LINES );

		setupColor( "fpsGraphColor" );
		for( int i=0; i<histCount; i++ ) {
			glVertex2f( (float)i, 0.f );
			glVertex2f( (float)i, history[i] * scale );
		}

		setupColor( "fpsGridColor" );
		glVertex2f( 0.f, 15.f );
		glVertex2f( w, 15.f );

		glVertex2f( 0.f, 30.f );
		glVertex2f( w, 30.f );

		glVertex2f( 0.f, 45.f );
		glVertex2f( w, 45.f );

	glEnd();

	if( resetScissor ) {
		glPopAttrib();
	}
}

ZUI::ZUILayoutCell *ZUIFPSGraph::layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly ) {
	w = 100;
	h = 30;
	return 0;
}

