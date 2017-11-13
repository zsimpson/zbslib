// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Isolates the logic concerning mouse motion messages; provides
//			global state variables of mouse state
//			This version interfaces to the zglwin window manager
//		}
//		*PORTABILITY win32
//		*REQUIRED_FILES zmousemsg.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#include "GL/gl.h"
// STDLIB includes:
#include "assert.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
// MODULE includes:
#include "zmousemsg.h"
// ZBSLIB includes:
#include "zhashtable.h"
#include "zglwin.h"
#include "zmsg.h"

ZHashTable zMouseMsgRequests;
char zMouseMsgExclusive[256];

void zMouseMsgRequestMovement( char *msg ) {
	zMouseMsgRequests.putI( msg, 1 );
}

void zMouseMsgCancelMovement( char *msg ) {
	zMouseMsgRequests.putS( msg, 0 );
}

int zMouseMsgRequestExclusiveDrag( char *msg ) {
	if( !zMouseMsgExclusive[0] ) {
		strcpy( zMouseMsgExclusive, msg );
		return 1;
	}
	return 0;
}

void zMouseMsgCancelExclusiveDrag() {
	zMouseMsgExclusive[0] = 0;
}

int zMouseMsgIsExclusiveDrag() {
	return zMouseMsgExclusive[0];
}


////////////////////////////////////////////////////////////////////
// handlers

int zMouseMsgInWindow, zMouseMsgLastInWindow;
int zMouseMsgRealX, zMouseMsgRealY;
int zMouseMsgLastRealX, zMouseMsgLastRealY;
	// The reals are not spoofable
int zMouseMsgX, zMouseMsgY;
int zMouseMsgLastX, zMouseMsgLastY;
int zMouseMsgLState, zMouseMsgRState, zMouseMsgMState;
int zMouseMsgMoved = 0;
int zMouseMsgFrameCounter = 0;
int zMouseMsgHook = 0;













void zMouseMsgUpdateKeyModifierState( int _shift, int _ctrl, int _alt, int _super ) {
	// This is only to be used by the GLFW system
	assert( 0 );
}

void zMouseMsgGetKeyModifier(int &shift, int &ctrl, int &alt ) {
	zglWinModifiers( shift, ctrl, alt );
}

void zMouseMsgButtonHandler( char button, int state, int _x, int _y ) {
	if( button=='L' ) zMouseMsgLState = state;
	if( button=='R' ) zMouseMsgRState = state;
	if( button=='M' ) zMouseMsgMState = state;

	int viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );
	int shift, ctrl, alt;
	zMouseMsgGetKeyModifier( shift, ctrl, alt );

	char buf[256];

	if( zMouseMsgExclusive[0] && state==0 ) {
		int viewport[4];
		glGetIntegerv( GL_VIEWPORT, viewport );
		sprintf( buf, "%s x=%d y=%d w=%d h=%d zMouseMsg=1 releaseDrag=1 which=%c", zMouseMsgExclusive, zMouseMsgX, zMouseMsgY, viewport[2], viewport[3], button );
		ZMsg *msg = (ZMsg *)zHashTable( buf );
		zMsgDispatchNow( msg );
			// These messages have to be immediately dispatched
			// because of the case that a zMouse click could go up and
			// down in a single click and we would miss the
			// release because exclusive wouldn't have a chance to get set
	}
	else {
		sprintf( buf, "type=MouseClick dir=%c which=%c x=%d y=%d shift=%d ctrl=%d alt=%d w=%d h=%d",
			state==0 ? 'U' : 'D',
			button,
			_x, viewport[3] - _y,
			shift, ctrl, alt,
			viewport[2], viewport[3]
		);

		ZMsg *msg = (ZMsg *)zHashTable( buf );
		zMsgDispatchNow( msg );
	}
}

void zMouseMsgMotionHandler( int _x, int _y ) {
	// Prevent more than one update per frame
	static int lastFrameCount = -1;
	if( zMouseMsgFrameCounter != lastFrameCount ) {
		lastFrameCount = zMouseMsgFrameCounter;
		zMouseMsgX = _x;
		int viewport[4];
		glGetIntegerv( GL_VIEWPORT, viewport );
		zMouseMsgY = viewport[3] - _y;
		zMouseMsgRealX = zMouseMsgX;
		zMouseMsgRealY = zMouseMsgY;
		if( zMouseMsgX != zMouseMsgLastX || zMouseMsgY != zMouseMsgLastY ) {
			zMouseMsgMoved = 1;
		}
	}
}

void zMouseMsgUpdate() {
	// Dispatch messages to any who have asked for it or exclusive
	if( !zMouseMsgHook ) {
		zMouseMsgHook = 1;
		zglWinMouseFunc( zMouseMsgButtonHandler );
		zglWinMotionFunc( zMouseMsgMotionHandler );
	}

	zMouseMsgFrameCounter++;
		// Increment a frame counter used to make sure we don't
		// get more than one motion event per frame.

	zMouseMsgLastX = zMouseMsgX;
	zMouseMsgLastY = zMouseMsgY;
	zMouseMsgLastRealX = zMouseMsgRealX;
	zMouseMsgLastRealY = zMouseMsgRealY;

	if( zMouseMsgMoved ) {
		zMouseMsgMoved = 0;
		int viewport[4];
		glGetIntegerv( GL_VIEWPORT, viewport );
		char buf[256];
		int shift, ctrl, alt;
		zMouseMsgGetKeyModifier( shift, ctrl, alt );
		sprintf( buf, "x=%d y=%d w=%d h=%d zMouseMsg=1 l=%d r=%d m=%d shift=%d ctrl=%d alt=%d", zMouseMsgX, zMouseMsgY, viewport[2], viewport[3], zMouseMsgLState, zMouseMsgRState, zMouseMsgMState, shift, ctrl, alt );

		if( (zMouseMsgLState || zMouseMsgRState || zMouseMsgMState) && zMouseMsgExclusive[0] ) {
			zMsgQueue( "%s %s", zMouseMsgExclusive, buf );
		}
		else {
			// Send movement messages to anyone that asked for them
			for( int i=0; i<zMouseMsgRequests.size(); i++ ) {
				char *key = zMouseMsgRequests.getKey(i);
				if( key ) {
					zMsgQueue( "%s %s", key, buf );
				}
			}
		}
	}
}


ZMSG_HANDLER( MouseSpoof ) {
	float viewport[4];
	glGetFloatv( GL_VIEWPORT, viewport );
	if( zmsgIs(action,MouseClick) ) {
		int shift, ctrl, alt;
		zMouseMsgGetKeyModifier( shift, ctrl, alt );
		if( zmsgIs(dir,D) ) {
			if( zmsgIs(which,L) ) zMouseMsgLState = 1;
			if( zmsgIs(which,R) ) zMouseMsgRState = 1;
			if( zmsgIs(which,M) ) zMouseMsgMState = 1;
		}
		else if( zmsgIs(dir,U) ) {
			if( zmsgIs(which,L) ) zMouseMsgLState = 0;
			if( zmsgIs(which,R) ) zMouseMsgRState = 0;
			if( zmsgIs(which,M) ) zMouseMsgMState = 0;
		}
		char which = zmsgS(which)[0];

		zMouseMsgX = zmsgI(inPixels) ? zmsgI(x) : int(zmsgF(x) * viewport[2]);
		zMouseMsgY = zmsgI(inPixels) ? zmsgI(y) : int(zmsgF(y) * viewport[3]);

		zMouseMsgButtonHandler( which, zmsgIs(dir,D) ? 1 : 0, zMouseMsgX, (int)viewport[3] - zMouseMsgY );
//		zMsgQueue( "type=MouseClick dir=%s which=%s x=%d y=%d shift=%d ctrl=%d alt=%d w=%d h=%d spoofing=1 area=%f",
//			zmsgS(dir),
//			zmsgS(which),
//			int(zmsgF(x) * viewport[2]),
//			int(zmsgF(y) * viewport[3]),
//			zmsgI(shift), zmsgI(ctrl), zmsgI(alt),
//			viewport[2], viewport[3], zmsgF(area)
//		);
	}
	else if( zmsgIs(action,MouseMove) ) {
		zMouseMsgX = zmsgI(inPixels) ? zmsgI(x) : int(zmsgF(x) * viewport[2]);
		zMouseMsgY = zmsgI(inPixels) ? zmsgI(y) : int(zmsgF(y) * viewport[3]);

//		zMouseMsgX = (int)(zmsgF(x) * viewport[2]);
//		zMouseMsgY = /*viewport[3] -*/ (int)(zmsgF(y) * viewport[3]);
		zMouseMsgMoved = 1;
	}
}


