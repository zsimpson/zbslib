// @ZBS {
//		*COMPONENT_NAME zuiline
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuiline.cpp
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
#include "math.h"
#include "stdio.h"
#include "stdlib.h"
// MODULE includes:
#include "zui.h"
// ZBSLIB includes:
#include "zglfont.h"
#include "zmsg.h"

// STOPFLOW timing
#include "sfmod.h"

// ZUILine
//--------------------------------------------------------------------------------------------------------------------------------
// Parameters:
// lineVert
// lineEmbossed
// lineSpaceBot
// lineColor
// thickness
// lineSpaceTop
// lineSpaceBot

ZUI_IMPLEMENT(ZUILine,ZUI);

void ZUILine::factoryInit() {
	putS( "layout_cellFill", "wh" );
}

void ZUILine::render() {
	ZUI::render();

	SFTIME_START (PerfTime_ID_Zlab_render_lines, PerfTime_ID_Zlab_render_render);

	int vert = getI( "lineVert", 0 );

	if( getI("lineEmbossed") ) {
#ifndef SFFAST
		glPushAttrib( GL_CURRENT_BIT );
#endif
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glLineWidth( 1.f );
		glBegin( GL_LINES );
		int alpha = 0x80;
		float lineSpaceBot = getF( "lineSpaceBot" );
		if( !vert ) {
			// horizontal
			glColor4ub( 255, 255, 255, alpha );
			glVertex2f( 0.f, lineSpaceBot );
			glVertex2f( w, lineSpaceBot );

			glColor4ub( 0, 0, 0, alpha );
			glVertex2f( 0.f, lineSpaceBot+1 );
			glVertex2f( w, lineSpaceBot+1 );
		}
		else {
			// vertical
			glColor4ub( 0, 0, 0, alpha );
			glVertex2f( w/2.f, 0.f );
			glVertex2f( w/2.f, h );

			glColor4ub( 255, 255, 255, alpha );
			glVertex2f( w/2.f+1, 0.f );
			glVertex2f( w/2.f+1, h );
		}
		glEnd();
#ifndef SFFAST
		glPopAttrib();
#else
		glDisable( GL_BLEND );
#endif
	}
	else {
		setupColor( "lineColor" );
		float thickness = getF( "thickness", 1.f );
#ifndef SFFAST
		glPushAttrib( GL_LINE_BIT );
#endif
		glLineWidth( thickness );

		glBegin( GL_LINES );
		if( !vert ) {
			// horizontal
			glVertex2f( 0.f, h/2.f );
			glVertex2f( w, h/2.f );
			glVertex2f( 0.f, h/2.f+1 );
			glVertex2f( w, h/2.f+1 );
		}
		else {
			// vertical
			glVertex2f( w/2.f, 0.f );
			glVertex2f( w/2.f, h );
		}
		glEnd();

#ifndef SFFAST
		glPopAttrib();
#endif
	}
	
	SFTIME_END (PerfTime_ID_Zlab_render_lines);
}

ZUI::ZUILayoutCell *ZUILine::layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly ) {
	w = 1.f;
	h = 1.f + getF( "lineSpaceTop" ) + ( getI("lineEmbossed") ? getF( "lineSpaceBot" ) : getF( "thickness" ) );
	return 0;
}
