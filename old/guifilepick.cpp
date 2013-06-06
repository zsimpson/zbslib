// @ZBS {
//		*COMPONENT_NAME guifilepick
//		*MODULE_OWNER_NAME glgui
//		*GROUP modules/gui
//		*REQUIRED_FILES guifilepick.cpp guifilepick.h
// }
// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#include "GL/gl.h"
// STDLIB includes:
#include "assert.h"
#include "string.h"
#include "stdlib.h"
// MODULE includes:
#include "guifilepick.h"
// ZBSLIB includes:
#include "zwildcard.h"
#include "zfilespec.h"

#ifdef __USE_GNU
#define stricmp strcasecmp
#endif

// GUIFilePick
//==================================================================================

GUI_TEMPLATE_IMPLEMENT(GUIFilePick);

// @TODO: Eventually it would be nice for this to turn into a 
// generic picker with directory expansion etc like an explorer
// It should have indentation, coloring, etc.

void GUIFilePick::populate( char *path ) {
	char fullPath[256] = {0,};

	clearView();

	char *dir = getAttrS( "dir" );
	if( dir ) {
		strcpy( fullPath, dir );
		strcat( fullPath, path );
	}
	else {
		strcpy( fullPath, path );
	}

	for( ZWildcard w( fullPath ); w.next(); ) {
		addLine( w.getName() );
	}
}

void GUIFilePick::handleMsg( ZMsg *msg ) {
	if( zmsgIs(type,GUIFilePick_populate) ) {
		populate( zmsgS(path) );
	}
	GUITextList::handleMsg( msg );
}

