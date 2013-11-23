// @ZBS {
//		*COMPONENT_NAME zuibutton
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuibutton.cpp
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
#include "math.h"
#include "string.h"
// MODULE includes:
#include "zui.h"
// ZBSLIB includes:
#include "zglfont.h"
#include "zmsg.h"

// ZUIRadioButton
//==================================================================================

#include "zui.h"

struct ZUIRadioButton : ZUICheck {
	ZUI_DEFINITION(ZUIRadioButton,ZUICheck);

	virtual void factoryInit();
	virtual void checkSendSelectedMsg();
	virtual void handleMsg( ZMsg *msg );
};

ZUI_IMPLEMENT(ZUIRadioButton,ZUICheck);

void ZUIRadioButton::factoryInit() {
	ZUICheck::factoryInit();
	putI( "circularButton", 1 );
}

void ZUIRadioButton::checkSendSelectedMsg() {
	int selected = getSelected();
	if( selected ) {
		int sent = getI( "sentSelectedMsg" );
		if( !sent ) {
			putI( "sentSelectedMsg", 1 );
			putI( "sentUnselectedMsg", 0 );
			zMsgQueue( "%s", getS("sendMsgOnSelect") );

			// UNSELECT all the others
			for( ZUI *z = parent->headChild; z; z=z->nextSibling ) {
				if( z != this && z->isS("class","ZUIRadioButton") ) {
					if( getI( "selected" ) ) {
						z->dirty();
						z->putI( "selected", 0 );
					}
				}
			}

		}
	}
	else {
		int sent = getI( "sentUnselectedMsg" );
		if( !sent ) {
			putI( "sentUnselectedMsg", 1 );
			putI( "sentSelectedMsg", 0 );
			zMsgQueue( "%s", getS("sendMsgOnUnselect") );
		}
	}
}

void ZUIRadioButton::handleMsg( ZMsg *msg ) {
	// Overload ZUICheck handling of a click: for radio buttons, clicking
	// an already-selected button should do nothing.
	if( !getI("disabled") ) {

		if( zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L) ) {
			if( !getSelected() ) {
				setSelected( 1 );
			}
			zMsgUsed();
			return;
		}
	}

	ZUICheck::handleMsg( msg );
}
