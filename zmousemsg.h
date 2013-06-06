// @ZBS {
//		*MODULE_OWNER_NAME zmousemsg
// }

#ifndef ZMOUSEMSG_H
#define ZMOUSEMSG_H

// This module isolates the logic concerning mouse motion messages
// A state machine monitors the state of the buttons and the motion
// to determine dragging logic.  An exclusive locking interface
// allows modules to request that they have exclusive drag messsages
// It also provides global state varaibles for those systems
// which need to have weird exclusive-bypassing rules such as 
// a text widget that needed to print out mouse coordinates.

//extern int zMouseMsgInWindow, zMouseMsgLastInWindow;
	// Current and last zMouse in window

extern int zMouseMsgX, zMouseMsgY;
extern int zMouseMsgLastX, zMouseMsgLastY;
	// Current and last zMouse x and y in window pixel coordinates (lower left = 0,0)

extern int zMouseMsgViewportW, zMouseMsgViewportH;
extern float zMouseMsgViewportWF, zMouseMsgViewportHF;
	// Last result of the viewportport call


extern int zMouseMsgLState, zMouseMsgRState, zMouseMsgMState;
	// Current state of the left and right zMouse buttons

void zMouseMsgRequestMovement( char *msg );
	// msg contains the message to send when zMouse motion occurs
	// Note that x, y, w, h, and zMouseMsg=1 is set on all queued messages

void zMouseMsgCancelMovement( char *msg );
	// Cancels a request by matching a message request

int zMouseMsgRequestExclusiveDrag( char *msg );
	// Requests that only this message will propagate while
	// the zMouse it pressed.  Returns zero if someone else
	// already has an exclusive lock or if the zMouse is not
	// currently in a zMouse down state.

void zMouseMsgCancelExclusiveDrag();
	// Cancels the current exclusive drag.  Only call if you actually got the lock!

int zMouseMsgIsExclusiveDrag();
	// True is the mouse is currently locked someone

void zMouseMsgUpdate();
	// Must be called once per frame to process the messages

void zMouseMsgUpdateKeyModifierState( int _shift, int _ctrl, int _alt );
	// This is used only by the GLFW system to let this system know
	// the state of the meta keys since GLFW can't query for itself

#endif

