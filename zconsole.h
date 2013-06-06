#ifndef ZCONSOLE_H
#define ZCONSOLE_H

// @ZBS {
//		*MODULE_OWNER_NAME zconsole
// }

extern int zconsoleExists;

long zconsoleGetConsoleHwnd();
	// This is only meaningly for windows, uses a stupid
	// title naming hack to fetch the hwnd

void zconsoleCreate();
	// Creates a console window and a thread for it.
	// Launches a separate window and a separate thread / process for it.
	// Note you can only have one onsole per process so there is
	// no need for this to return a handle

void zconsoleFree();
	// Free the console, if it exists

void zconsoleHide();
	// Hide an existing console

void zconsoleShow( int focus=0 );
	// Show the hidden console.  If focus, allow it to take focus

int zconsoleIsVisible();
	// Is the console hidden

void zconsolePositionAt( int x, int y, int w, int h );
	// Position the window at specified place with width and height

void zconsoleGetPosition( int &x, int &y, int &w, int &h );
	// Get console position

void zconsolePositionAlongsideCurrent();
	// Position the console window as a companion alongside the current window

void zconsoleCls();
	// Clear the console

void zconsoleGotoXY( int x, int y );
	// Goto XY

int zconsoleKbhit();
	// Non-blocking returns if a key is in the queue
	// Under Windows you then have to call getch() to get the key.  NOT getc or others!

void zconsoleCharMode();
	// in non-win32, set buffer size to 1 char such that characters are returned from
	// getchar() etc as they are typed

int zconsoleGetch();
	// Under windows this calls getch and echos and under linux it calls getchar

#endif




