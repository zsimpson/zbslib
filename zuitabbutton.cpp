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
#include "zvars.h"

// ZUITabButton
//--------------------------------------------------------------------------------------------------------------------------------

ZUI_IMPLEMENT(ZUITabButton,ZUIButton);

void ZUITabButton::factoryInit() {
	putI( "toggle", 1 );
	putI( "sticky", 1 );
	ZUIButton::factoryInit();
}

void ZUITabButton::checkSendSelectedMsg() {
	int selected = getSelected();
	if( selected ) {
		int sent = getI( "sentSelectedMsg" );
		if( !sent ) {
			// FIND the ZUITabPanel container
			ZUI *p = parent;
			for( ; p; p=p->parent ) {
				if( p->isS("class","ZUITabPanel") ) {
					break;
				}
			}
			if( p ) {
				zMsgQueue( "type=ZUITabButtonSelected toZUI='%s' fromZUI='%s' slavePanel=%d tracemsg='ZUITabButtonSelected from %s, slavePanel=%d'", p->name, name, getI("slavePanel"), name, getI("slavePanel") );
			}

			for( ZUI *z = parent->headChild; z; z=z->nextSibling ) {
				if( z != this && z->isS("class","ZUITabButton") ) {
					z->putI( "selected", 0 );
				}
			}

			putI( "sentSelectedMsg", 1 );
			putI( "sentUnselectedMsg", 0 );
			zMsgQueue( "%s", getS("sendMsgOnSelect") );
			zMsgQueue( "type=ZUISet key=selected val=0 toZUIGroup=%s exceptToZUI=%s", getS("group"), name );
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


