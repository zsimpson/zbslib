// @ZBS {
//		*COMPONENT_NAME zuitabpanel
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuitabpanel.cpp
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

// ZUIPanel
//--------------------------------------------------------------------------------------------------------------------------------

ZUI_IMPLEMENT(ZUITabPanel,ZUIPanel);

void ZUITabPanel::render() {
	float _x = 0.f;
	float _y = 0.f;
	float _w = this->w;
	float _h = this->h;

	int side = getS("buttonSide")[0];

	// READ the size of the first child which will be the buttons
	if( headChild ) {
		if( side == 't' || side == 'T' ) {
			_y = 0.f;
			_h -= headChild->h * 0.75f;
		}
		else if( side == 'l' || side == 'L' ) {
			_x = headChild->w - 10.f;
			_w -= _x;
		}
	}

	ZUIPanel::renderBase( _x, _y, _w, _h );
}

void ZUITabPanel::handleMsg( ZMsg *msg ) {
	ZUIPanel::handleMsg( msg );
	if( zMsgIsUsed() ) return;

	if( zmsgIs(type,ZUITabButtonSelected) ) {
		int slavePanel = zmsgI(slavePanel);
		ZUI *z = headChild;
		if( z ) {
			// SKIP over the tabs panel
			z = z->nextSibling;
		}
		for( ; z; z=z->nextSibling ) {
			if( z->getI("slavePanel") == slavePanel ) {
				zMsgQueue( "type=ZUIShow toZUI='%s' tracemsg='ZUIShow (sent by %s) for %s'", z->name, name, z->name );
			}
			else {
				zMsgQueue( "type=ZUIHide toZUI='%s' tracemsg='ZUIShow (sent by %s) for %s'", z->name, name, z->name );
			}
		}
	}
}
