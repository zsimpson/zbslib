// @ZBS {
//		*MODULE_OWNER_NAME zglwin
// }

#ifndef ZGLWIN_H
#define ZGLWIN_H

// This is a very simple GLUT-like interface.
// It currently only supports only under WIN32 and has a limited implementation of GLUTs features

void zglWinInit();
	// Must be called before anything else

int zglWinCreateWindow( char *title, int x, int y, int w, int h, int parentWinid=-1 );
	// Creates a window (or child window).  Returns the winid of the window in the internal tables

void zglWinDestroyWindow( int winid );
	// Destroy a window by creation sequence (0 based)

int zglWinGetWindow();
	// Fetch the current winid

void zglWinSetWindowStyle( int style );
	#define ZGLWIN_WINSTYLE_NORMAL (1)
	#define ZGLWIN_WINSTYLE_BORDERLESS (2)
	// Sets the style of the window

void zglWinSetWindow( int winid );
	// Set the active window by creation sequence (0 based)

void zglWinSetFocus( int winid );
	// Set the focus on the winid (brings to front)

void zglWinShowWindow( int winid );
void zglWinHideWindow( int winid );

int zglWinGetOSHandle( int winid );
	// Fetches the operating system handle on the winid

void zglWinSwapBuffers();
	// Swap double buffers

void zglWinPostRedisplay();
	// Mark that it should call the display func ptr on the next frame

void zglWinReshapeWindow( int w, int h );
	// Force the window to reposition

int zglWinPump();
	// Calls the idle and display functions once

int zglWinMainLoop();
	// Infinite loops on zglWinPump

void zglWinModifiers( int &shift, int &ctrl, int &alt );
	// Get the modifier flags

void zglWinGetClientRect( int *x, int *y, int *w, int *h );
void zglWinSetClientRect( int x, int y, int w, int h );
	// Fetch and move the window based on the client dimensions

void zglSetWindowSize( int width, int height );
void zglSetWindowPos( int x, int y );

////////////////////////////////////////////////////////////////////////////
// Callback setups:
////////////////////////////////////////////////////////////////////////////

void zglWinDisplayFunc( void (*f)() );
	// Called every loop of the main loop when redisplay is posted

void zglWinIdleFunc( void (*f)() );
	// Called every loop of the main loop

void zglWinReshapeFunc( void (*f)(int w, int h) );
	// Called on window resize

void zglWinMoveFunc( void (*f)(int x, int y) );
	// Called when the window moves

void zglWinMouseFunc( void (*f)(char button, int pressed, int x, int y) );
	// Called 

void zglWinMotionFunc( void (*f)(int x, int y) );
	// Called on any mouse movement (regardless of mouse button state)

void zglWinKeyboardFunc( void (*f)(char *key, int state, int x, int y) );
	// Called on keyboard change state
	// key argument is a string which contains the key including "f1".."f12", "insert", "delete", etc.
	// state = 0 on key up, = 1 on key down, = 2 on char.
	// Key down events are KEY codes not characters so state=1 on 'A' but never 'a'/
	// state can be 2 on 'A' or 'a' and you will receive multiple on key repeat
	// x and y contain the mouse coordinates

void zglWinSpecialFunc( void (*f)(char *message) );
	// Special messages from other systems may arrive as strings to this point

#endif



