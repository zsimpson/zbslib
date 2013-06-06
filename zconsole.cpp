// @ZBS {
//		*MODULE_NAME ZConsole
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Generic Conolse Driver
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zconsole.cpp zconsole.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//			This has not yet been ported to unix but is intended
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }

// OPERATING SYSTEM specific includes:
#ifdef WIN32
#	include "windows.h"
#	include "conio.h"
#	include "fcntl.h"
#	include "io.h"
#else
	#include "sys/time.h"
	#include "termios.h"
#endif

// SDK includes:
// STDLIB includes:
#include "string.h"
#include "stdio.h"
#include "ios"
// MODULE includes:
// ZBSLIB includes:
#include "zconsole.h"

long zconsoleHandle = 0;


long zconsoleGetConsoleHwnd() {
	#ifdef WIN32
		// In modern windows interface there is a routine GetConsoleWindow
		// but under old windows you have to change the title and then find it.
		// To maintain compatibility, I'm doing it that old way.

		char pszNewWindowTitle[1024];
		char pszOldWindowTitle[1024];

		// FETCH current window title
		GetConsoleTitle( (LPSTR)pszOldWindowTitle, 1024 );

		// CREATE a "unique" NewWindowTitle
		wsprintf( (LPSTR)pszNewWindowTitle,(LPCSTR)"%d/%d",GetTickCount(),GetCurrentProcessId() );

		// CHANGE current window title
		SetConsoleTitle( (LPSTR)pszNewWindowTitle );

		// ENSURE window title has been updated
		Sleep(40);

		// FIND the windows
		long hwnd = (long)FindWindow( NULL, (LPSTR)pszNewWindowTitle );

		// RESTORE original window title
		SetConsoleTitle( (LPCSTR)pszOldWindowTitle );

		return hwnd;
	#endif
	return 0;
}

int zconsoleExists = 0;
void zconsoleCreate() {
	// Creates a console window and a thread for it.  Returns a handle to it.
	// Launches a separate window and a separate thread / process for it.
	#ifdef WIN32
		CONSOLE_SCREEN_BUFFER_INFO info;

		// CREATE the win32 console window
		AllocConsole();

		// DETERMINE it's dimension and update them to larger
		// @TODO: Separate?
		GetConsoleScreenBufferInfo( GetStdHandle(STD_OUTPUT_HANDLE), &info );
		info.dwSize.Y = 400;
		SetConsoleScreenBufferSize( GetStdHandle(STD_OUTPUT_HANDLE), info.dwSize );

		// REDIRECT stdout
		long hStdout = (long)GetStdHandle( STD_OUTPUT_HANDLE );
		*stdout = *_fdopen( _open_osfhandle( hStdout, _O_TEXT ), "w" );
		setvbuf( stdout, NULL, _IONBF, 0 ); 
		
		// REDIRECT stdin
		long hStdin = (long)GetStdHandle( STD_INPUT_HANDLE );
		*stdin = *_fdopen( _open_osfhandle( hStdin, _O_TEXT ), "r" );
		setvbuf( stdin, NULL, _IONBF, 0 );

		// REDIRECT stderr
		long hStderr = (long)GetStdHandle( STD_ERROR_HANDLE );
		*stderr = *_fdopen( _open_osfhandle( hStderr, _O_TEXT ), "w" );
		setvbuf( stderr, NULL, _IONBF, 0 );

		// REDIRECT cout, etc to the same console
		std::ios_base::sync_with_stdio();

		zconsoleHandle = zconsoleGetConsoleHwnd();
		zconsoleExists = 1;
	#endif
}

void zconsoleFree() {
	#ifdef WIN32
		if( zconsoleExists ) {
			FreeConsole();
			zconsoleHandle = 0;
			zconsoleExists = 0;
		}
	#endif
}

void zconsoleShow( int takeFocus ) {
	#ifdef WIN32
		// We don't want to lose the focus on the current window so we get it first and reset it
		long hwnd = (long)GetForegroundWindow();
		ShowWindow( (HWND)zconsoleHandle, SW_SHOW );
		// ENSURE window title has been updated
		Sleep(40);
		if( ! takeFocus ) {
			SetForegroundWindow( (HWND)hwnd );
		}
	#endif
}

void zconsolePositionAlongsideCurrent() {
	#ifdef WIN32
		// POSITION the console next to the window if possible
		RECT rect;
		GetWindowRect( (HWND)GetForegroundWindow(), &rect );
		MoveWindow( (HWND)zconsoleHandle, rect.right, rect.top, 400, rect.bottom-rect.top, 1 );
	#endif
}

void zconsolePositionAt( int x, int y, int w, int h ) {
	#ifdef WIN32
		// POSITION the console next to the window if possible
		RECT rect;
		GetWindowRect( (HWND)GetForegroundWindow(), &rect );
		MoveWindow( (HWND)zconsoleHandle, x, y, w, h, 1 );
	#endif
}

void zconsoleGetPosition( int &x, int &y, int &w, int &h ) {
	#ifdef WIN32
		RECT rect = {0,};
		GetWindowRect( (HWND)zconsoleHandle, &rect );
		x = rect.left;
		y = rect.top;
		w = rect.right - rect.left;
		h = rect.bottom - rect.top;
	#endif
}

void zconsoleHide() {
	#ifdef WIN32
		ShowWindow( (HWND)zconsoleHandle, SW_HIDE );
	#endif
}

int zconsoleIsVisible() {
	#ifdef WIN32
		return IsWindowVisible( (HWND)zconsoleHandle );
	#else
		return 0;
	#endif
}

void zconsoleCls() {
	#ifdef WIN32
		// Windows: Making the simple hard since 1980(tm)
		HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
		COORD coordScreen = { 0, 0 };
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo( hConsole, &csbi );
		DWORD dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
		DWORD cCharsWritten;
		FillConsoleOutputCharacter( hConsole, (TCHAR) ' ', dwConSize, coordScreen, &cCharsWritten );
		GetConsoleScreenBufferInfo( hConsole, &csbi );
		FillConsoleOutputAttribute( hConsole, csbi.wAttributes,	dwConSize, coordScreen, &cCharsWritten );
		SetConsoleCursorPosition( hConsole, coordScreen );
	#endif
}

void zconsoleGotoXY( int x, int y ) {
	#ifdef WIN32
		HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
		COORD coordScreen = { x, y };
		SetConsoleCursorPosition( hConsole, coordScreen );
	#endif
}

int zconsoleKbhit() {
	#ifdef WIN32
		return ::kbhit();
	#else
		timeval tv;
		fd_set read_fd;

		tv.tv_sec=0;
		tv.tv_usec=0;
		FD_ZERO(&read_fd);
		FD_SET(0,&read_fd);

		if(select(1, &read_fd, NULL, NULL, &tv) == -1)
			return 0;

		if(FD_ISSET(0,&read_fd))
			return 1;

		return 0;
	#endif
}

void zconsoleCharMode() {
	#ifndef WIN32

	termios settings;
	tcgetattr(0,&settings);
	
	// Disable canonical mode, and set buffer size to 1 byte
	settings.c_lflag &= (~ICANON);
	settings.c_cc[VTIME] = 0;
	settings.c_cc[VMIN] = 1;

	tcsetattr(0,TCSANOW,&settings);
	#endif
}

int zconsoleGetch() {
	#ifdef WIN32
		char c = getch();
		printf( "%c", c );
		return c;
	#else
		char c = getchar();
//		printf( "%c", c );
		return c;
	#endif
}

