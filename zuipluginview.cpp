// @ZBS {
//		*COMPONENT_NAME zuipluginview
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuipluginview.cpp
// }

// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#include "GL/gl.h"
// STDLIB includes:
#include "assert.h"
#include "string.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"
// MODULE includes:
#include "zui.h"
// ZBSLIB includes:
#include "zmsg.h"
#include "zplugin.h"

// STOPFLOW timing
#include "sfmod.h"

// ZUIPluginView
//===============================================================================

struct ZUIPluginView : ZUI {
	ZUI_DEFINITION( ZUIPluginView, ZUI );

	ZUIPluginView() : ZUI() { }
	virtual void factoryInit();
	virtual void update();
	virtual void render();
	virtual void handleMsg( ZMsg *msg );
};


ZUI_IMPLEMENT( ZUIPluginView, ZUI );

void ZUIPluginView::factoryInit() {
	setUpdate( 1 );
}

void ZUIPluginView::render() {
	ZUI::render();
	if( !w || !h ) {
		// early out to prevent GL drivers from choking on 0 width or height in glViewport
		// (found on intel duo mackbook running winXP Debug build)
		return;
	}

	// Check first to see if the plugin exports a render, since if it does not (it
	// may do all of it's rendering inside of ZUIs) we can skip a lot of GL stuff
	// below that really enhances performance (originally skipped using MMK's SFFAST mod
	typedef void (*RenderFnPtr)();
	RenderFnPtr render = 0;
	char *plugin = getS("plugin");
	if( plugin && *plugin ) {
		render = (RenderFnPtr)zPluginGetP( plugin, "render" );
	}

	if( render ) {
		SFTIME_START (PerfTime_ID_Zlab_render_plugin, PerfTime_ID_Zlab_render_render);

//	#ifndef SFFAST
// SFFAST_TFBMOD - rather than #ifdef out, we now skip all this code if the plugin
// do NOT export a render fn

		// mmk - Not needed by stopflow, and this has significant performance impact.
		// removing the push/pop had visual effects, so I removed this entirely.
		glMatrixMode( GL_TEXTURE );
		glPushMatrix();
		glMatrixMode( GL_PROJECTION );
		glPushMatrix();
		glMatrixMode( GL_MODELVIEW );
		glPushMatrix();

		// The convention is that plugins start with an origin in the
		// center of their window and a width of 2 and height of 2
		glScalef( w/2.f, h/2.f, 1.f );
		glTranslatef( 1.f, 1.f, 0.f );

		glPushAttrib( GL_ALL_ATTRIB_BITS );      // mmk - seems to be needed even though slow

		int viewport[4];
		glGetIntegerv( GL_VIEWPORT, viewport );
		glViewport( viewport[0]+(int)x, viewport[1]+(int)y, (int)w, (int)h );

	//	glColor3ub( 255, 255, 255 );
	//	glBegin( GL_QUADS );
	//		glVertex2f( +1.f, -1.f );
	//		glVertex2f( -1.f, -1.f );
	//		glVertex2f( -1.f, +1.f );
	//		glVertex2f( +1.f, +1.f );
	//	glEnd();

		// SFFAST_TFBMOD: logic moved above to find if render is valid or not and early out if not...
		//typedef void (*RenderFnPtr)();
		//char *plugin = getS("plugin");
		//if( plugin && *plugin ) {
		//	RenderFnPtr render = (RenderFnPtr)zPluginGetP( plugin, "render" );
		//	if( render ) {
				(*render)();
		//	}
		//}

		glPopAttrib();    // mmk - Seems to be needed even though slow

		// DISABLE states that might have gotten set in plugins
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		glMatrixMode( GL_TEXTURE );
		glPopMatrix();
		glMatrixMode( GL_PROJECTION );
		glPopMatrix();
		glMatrixMode( GL_MODELVIEW );
		glPopMatrix();

//	#endif
// SFFAST_TFBMOD - rather than #ifdef out, we now skip all this code if the plugin
// do NOT export a render fn

		SFTIME_END (PerfTime_ID_Zlab_render_plugin);
	}
}

void ZUIPluginView::update() {
	typedef void (*UpdateFnPtr)();
	char *plugin = getS("plugin");
	if( plugin && *plugin ) {
		UpdateFnPtr update = (UpdateFnPtr)zPluginGetP( plugin, "update" );
		if( update ) {
			(*update)();
		}
	}
	ZUI::update();
}

void ZUIPluginView::handleMsg( ZMsg *msg ) {
	typedef void (*HandleMsgFnPtr)( ZMsg *msg );
	char *plugin = getS("plugin");
	if( plugin && *plugin ) {
		HandleMsgFnPtr handleMsg = (HandleMsgFnPtr)zPluginGetP( plugin, "handleMsg" );
		if( handleMsg ) {
			(*handleMsg)( msg );
		}
	}

	ZUI::handleMsg( msg );
}
