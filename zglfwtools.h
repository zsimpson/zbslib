// @ZBS {
//		*MODULE_OWNER_NAME zglfwtools
// }

#ifndef ZGLFWTOOLS_H
#define ZGLFWTOOLS_H

void zglfwGetWindowGeom( int *x, int *y, int *w, int *h );
	// Compensates for borderless mode

void zglfwGetWindowGeomOfClientArea( int *x, int *y, int *w, int *h );
	// Of the client area.

void zglfwCharHandler( int code, int state );

void zglfwKeyHandler( int code, int state );

void zglfwMouseWheelHandler( int state );

void zglfwWinSetWindowStyle( int style );
	#define zglfwWinStyleBorder (1)
	#define zglfwWinStyleBorderless (2)

int zglfwWinGetWindowStyle();
	// Same as above

void zglfwSetMaskingWindow( int show, int x, int y, int w, int h );

void zglfwInitRemoteControlIntercept();

void zglfwTouchLastActivity();

double zglfwGetLastActivity();

int zglfwGetShift();

int zglfwGetCtrl();

int zglfwGetAlt();

#endif
