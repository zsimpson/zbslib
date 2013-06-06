// @ZBS {
//		*COMPONENT_NAME zuibutton
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuibutton.cpp
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
#include "zglfont.h"
#include "zmsg.h"
#include "zuimenu.h"

// ZUIMenu
//==================================================================================

ZUI_IMPLEMENT(ZUIMenu,ZUIPanel);

void ZUIMenu::factoryInit() {
	ZUIPanel::factoryInit();

	putS( "*layout_cellFill", "w" );
	putI( "*layout_padX", 4 );
	putI( "*layout_padY", 2 );
	putS( "layoutManager", "pack" );
	putS( "pack_side", "t" );
	putI( "color", 0x907070c0 );
	putI( "*mouseOverColor", 0x707090ff );
}

void ZUIMenu::handleMsg( ZMsg *msg ) {
	if( getI("modal") ) {
		if( zmsgIs(type,MouseClick) && !containsLocal( zmsgF(localX), zmsgF(localY) ) ) {
			die();
			return;
		}
		if( zmsgIs(type,Key) ) {
			die();
			return;
		}
	}
	if( zmsgIs( type, MenuItemSelected ) && isMyChild( zmsgS(fromZUI) ) ) {
		zMsgQueue( "type=MenuItemSelected fromZUI=%s", name );
		die();
		return;
	}
	else if( zmsgIs( type, Menu_ClearItems ) ) {
		killChildren();
	}
	else if( zmsgIs( type, Menu_AddItem ) ) {
		ZUI *mi = factory( 0, "ZUIMenuItem" );
		mi->attachTo( this );
		mi->putS( "text", zmsgS(label) );
		mi->putS( "sendMsg", zmsgS(sendMsg) );
	}
	ZUIPanel::handleMsg( msg );
}

// ZUIMenuItem
//==================================================================================

ZUI_IMPLEMENT(ZUIMenuItem,ZUI);

void ZUIMenuItem::factoryInit() {
	ZUI::factoryInit();
	putS( "mouseRequest", "over" );
}

void ZUIMenuItem::render() {
	if( getI("mouseOver") ) {
		setupColor( "menuOverTextColor" );
	}
	else {
		setupColor( "menuTextColor" );
	}

	float w, h, d;
	printSize( "Q", w, h, d );
	print( getS("text"), 0.f, h );
}

ZUI::ZUILayoutCell *ZUIMenuItem::layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly ) {
	float d;
	printSize( getS("text"), w, h, d );
	return 0;
}

void ZUIMenuItem::handleMsg( ZMsg *msg ) {
	if( zmsgIs( type, ZUIMouseEnter ) ) {
		putI( "mouseOver", 1 );
	}
	else if( zmsgIs( type, ZUIMouseLeave ) ) {
		putI( "mouseOver", 0 );
	}

	if( zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L) ) {
		zMsgUsed();
		if( getS( "sendMsg" ) ) {
			zMsgQueue( getS( "sendMsg" ) );
		}
		zMsgQueue( "type=MenuItemSelected fromZUI=%s", name );
	}

	ZUI::handleMsg( msg );
}

