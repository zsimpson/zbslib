// @ZBS {
//		*COMPONENT_NAME zuimenu
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuimenu.cpp zuimenu.h
// }

#ifndef ZUIMENU_H
#define ZUIMENU_H

// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
// MODULE includes:
#include "zui.h"
// ZBSLIB includes:


// ZUIMenu
//==================================================================================

// A menu is panel that responds to the clicking
// of it's children's item by closing itself.
// It's children can be created either by attaching any
// other object to this or by using the itemN attributes.
// All children objects must send a "MenuItemSelected"
// selected command to it's parent.  Menu's can be heirarchical
// becuase the ZUIMenu always sends the same command to its parent

struct ZUIMenu : ZUIPanel {
	ZUI_DEFINITION(ZUIMenu,ZUIPanel);

	// SPECIAL ATTRIBUTES:

	int serialNum;
		// This is just used by the constructor to communicate.  Ignore.

	ZUIMenu() : ZUIPanel() { }
	virtual void factoryInit();
	virtual void handleMsg( ZMsg *msg );
};

// A menu item is just a static text that knows to send
// the MenuItemSelected command to its parent when it is clicked
struct ZUIMenuItem : ZUI {
	ZUI_DEFINITION(ZUIMenuItem,ZUI);

	// SPECIAL ATTRIBUTS:
	//   menuOverTextColor: the color of the panel when the mouse is over it
	//   menuTextColor: color of the text
	//   sendMsg: what to send when clicked (always sends a MenuItemSelected as well)

	ZUIMenuItem() : ZUI() { }
	virtual void factoryInit();
	virtual void render();
	virtual ZUILayoutCell *layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly=1 );
	virtual void handleMsg( ZMsg *msg );
};


#endif