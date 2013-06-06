// @ZBS {
//		*COMPONENT_NAME guipluginview
//		*MODULE_OWNER_NAME glgui
//		*GROUP modules/gui
//		*REQUIRED_FILES guipluginview.h
// }
// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#include "GL/gl.h"
#include "GL/glu.h"
// STDLIB includes:
#include "string.h"
#include "assert.h"
#include "math.h"
// MODULE includes:
#include "guipluginview.h"
// ZBSLIB includes:
#include "zplugin.h"


// GUIPluginView
//===============================================================================

GUI_TEMPLATE_IMPLEMENT( GUIPluginView );

void GUIPluginView::render() {
	GUIPanel::render();

	glMatrixMode( GL_TEXTURE );
	glPushMatrix();
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();

	// The convention is that plugins start with an origin in the
	// center of their window and a width of 2 and height of 2
	glScalef( myW/2.f, myH/2.f, 1.f );
	glTranslatef( 1.f, 1.f, 0.f );

	glPushAttrib( GL_ALL_ATTRIB_BITS );

	typedef void (*RenderFnPtr)();
	attrS(plugin);
	if( plugin && *plugin ) {
		ZHashTable *propertyTable = zPluginGetPropertyTable( plugin );
		assert( propertyTable );
		propertyTable->putI( "width", (int)myW );
		propertyTable->putI( "height", (int)myH );

		RenderFnPtr render = (RenderFnPtr)zPluginGetI( plugin, "render" );
		if( render ) {
			(*render)();
		}
	}

	glPopAttrib();

	glMatrixMode( GL_TEXTURE );
	glPopMatrix();
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();
}

void GUIPluginView::update() {
	// @TODO: Move this update somewhere better
	typedef void (*UpdateFnPtr)();
	attrS(plugin);
	if( plugin && *plugin ) {
		UpdateFnPtr update = (UpdateFnPtr)zPluginGetI( plugin, "update" );
		if( update ) {
			(*update)();
		}
	}
	GUIPanel::update();
}

void GUIPluginView::handleMsg( ZMsg *msg ) {
	typedef void (*HandleMsgFnPtr)( ZMsg *msg );
	attrS(plugin);
	if( plugin && *plugin ) {
		HandleMsgFnPtr handleMsg = (HandleMsgFnPtr)zPluginGetI( plugin, "handleMsg" );
		if( handleMsg ) {
			(*handleMsg)( msg );
		}
	}

	GUIPanel::handleMsg( msg );
}
