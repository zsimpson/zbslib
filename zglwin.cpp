// @ZBS {
//		*MODULE_NAME WIN32 Open GL Window Toolkit (!GLUT)
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A glut-like window interface for OpenGL and windows.  Written because
//			glut had an unclear license status.
//		}
//		*PORTABILITY win32
//		*REQUIRED_FILES zglwin.cpp zglwin.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//			Implement a unix x-windows port
//		}
//		*SELF_TEST yes windows
//		*PUBLISH yes
// }
// OPERATING SYSTEM specific includes:
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#include "windows.h"
// SDK includes:
#include "GL/gl.h"
#include "GL/glu.h"
// STDLIB includes:
#include "assert.h"
// MODULE includes:
#include "zglwin.h"
// ZBSLIB includes:

static void adjustCoords( int *x, int *y, int *w, int *h ) {
	RECT rect;
	rect.left = *x;
	rect.top = *y;
	rect.right = *x + *w;
	rect.bottom = *y + *h;
	
	AdjustWindowRect( &rect, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW, FALSE );
	
	*x = rect.left;
	*y = rect.top;
	*w = rect.right - rect.left;
	*h = rect.bottom - rect.top;
}

struct ZGLWinWin {
	void *hwnd;
	void *hdc;
	void *context;
	void (*renderFunc)();
	void (*idleFunc)();
	void (*reshapeFunc)(int w, int h);
	void (*motionFunc)(int w, int h);
	void (*buttonFunc)( char which, int pressed, int x, int y );
	void (*keyFunc)( char *key, int pressed, int x, int y );
	void (*specialFunc)( char *message );
	void (*moveFunc)( int x, int y );
	int redisplay;
	int cx, cy, cw, ch; // client dimensions
	int wx, wy, ww, wh; // window dimensions
	int style; // current style (constants set with zglWinSetWindowStyle)

	void setActive() {
		HGLRC currentContext = wglGetCurrentContext();
		HDC currentDc = wglGetCurrentDC();
		if( currentContext != context || currentDc != hdc ) {
			int ret = wglMakeCurrent( (HDC)hdc, (HGLRC)context );
			assert( ret );
		}
	}

};

#define ZGLWIN_MAX_WINDOWS (16)
static ZGLWinWin zglWinWindows[ZGLWIN_MAX_WINDOWS] = {0,};
static ZGLWinWin *zglWinCurrentWindow = 0;
static long __stdcall zglWinWndProc( HWND hWnd, unsigned int message, unsigned int wParam, long lParam );


void zglWinInit() {
	static char *classname;
	WNDCLASS  wc;
	HINSTANCE hInstance = GetModuleHandle(NULL);
	memset( &wc, 0, sizeof(WNDCLASS) );
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = (WNDPROC)zglWinWndProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon( hInstance, "ZGLWIN_ICON" );
	wc.hCursor = LoadCursor( 0, IDC_ARROW );
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "zglWin";
	if( !wc.hIcon ) {
		wc.hIcon = LoadIcon( NULL, IDI_WINLOGO );
	}

	int ret = RegisterClass(&wc);
	assert( ret );

	zglWinCurrentWindow = 0;
}

int zglWinCreateWindow( char *title, int x, int y, int w, int h, int parentWinid ) {
	int winid = 0;
	for( winid=0; winid < ZGLWIN_MAX_WINDOWS; winid++ ) {
		if( !zglWinWindows[winid].hwnd ) {
			break;
		}
	}
	assert( winid < ZGLWIN_MAX_WINDOWS );

	zglWinCurrentWindow = &zglWinWindows[winid];
	ZGLWinWin &window = zglWinWindows[winid];

	window.wx = window.cx = x;
	window.wy = window.cy = y;
	window.ww = window.cw = w;
	window.wh = window.ch = h;
	adjustCoords( &window.wx, &window.wy, &window.ww, &window.wh );
	window.style = ZGLWIN_WINSTYLE_NORMAL;

	HWND parent = parentWinid == -1 ? NULL : (HWND)zglWinWindows[parentWinid].hwnd;

	window.hwnd = CreateWindow(
		"zglWin", "zglWin", WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW | (parent?WS_POPUP:0), 
		window.wx, window.wy, window.ww, window.wh, parent, NULL, GetModuleHandle(NULL), 0
	);

	window.hdc = GetDC( (HWND)window.hwnd );

	PIXELFORMATDESCRIPTOR pfd;
	memset( &pfd, 0, sizeof(PIXELFORMATDESCRIPTOR) );
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cAlphaBits = 8;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 32;
	pfd.cStencilBits = 32;

	int pixFormat = ChoosePixelFormat( (HDC)window.hdc, &pfd );
	assert( pixFormat > 0 );

	DescribePixelFormat( (HDC)window.hdc, pixFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd );

	int ret = SetPixelFormat( (HDC)window.hdc, pixFormat, &pfd );
	assert( ret );

	window.context = wglCreateContext( (HDC)window.hdc );
	assert( window.context );

	zglWinReshapeWindow( w, h );

	SetWindowText( (HWND)window.hwnd, title );
	SetWindowLong( (HWND)window.hwnd, GWL_USERDATA, (long)zglWinCurrentWindow );

	ShowWindow( (HWND)window.hwnd, SW_SHOWNORMAL );

	zglWinSetWindow( winid );
	return winid;
}

void zglWinDestroyWindow( int winid ) {
	DestroyWindow( (HWND)zglWinWindows[winid].hwnd );
	if( zglWinCurrentWindow == &zglWinWindows[winid] ) {
		zglWinCurrentWindow = 0;
	}
	memset( &zglWinWindows[winid], 0, sizeof(zglWinWindows[0]) );
}

int zglWinGetWindow() {
	return zglWinCurrentWindow - &zglWinWindows[0];
}

void zglWinSetWindow( int winid ) {
	zglWinCurrentWindow = &zglWinWindows[winid];
	zglWinCurrentWindow->setActive();
}

void zglWinSetFocus( int winid ) {
	SetForegroundWindow( (HWND)zglWinWindows[winid].hwnd );
}

void zglWinShowWindow( int winid ) {
	ShowWindow( (HWND)zglWinGetOSHandle( winid ), SW_SHOW );
}

void zglWinHideWindow( int winid ) {
	ShowWindow( (HWND)zglWinGetOSHandle( winid ), SW_HIDE );
}

int zglWinGetOSHandle( int winid ) {
	return (int)zglWinWindows[winid].hwnd;
}

void zglWinSetWindowStyle( int style ) {
	if( zglWinCurrentWindow->style != ZGLWIN_WINSTYLE_NORMAL && style == ZGLWIN_WINSTYLE_NORMAL ) {
		int style = GetWindowLong( (HWND)zglWinCurrentWindow->hwnd, GWL_STYLE );
		style &= WS_VISIBLE;
		SetWindowLong( (HWND)zglWinCurrentWindow->hwnd, GWL_STYLE, style | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW );
		RECT rect;
		GetWindowRect( (HWND)zglWinCurrentWindow->hwnd, &rect );
		zglWinCurrentWindow->cx = rect.left;
		zglWinCurrentWindow->cy = rect.top;
		zglWinCurrentWindow->cw = rect.right - rect.left;
		zglWinCurrentWindow->ch = rect.bottom - rect.top;
		zglWinCurrentWindow->wx = zglWinCurrentWindow->cx; 
		zglWinCurrentWindow->wy = zglWinCurrentWindow->cy; 
		zglWinCurrentWindow->ww = zglWinCurrentWindow->cw; 
		zglWinCurrentWindow->wh = zglWinCurrentWindow->ch;
		adjustCoords( &zglWinCurrentWindow->wx, &zglWinCurrentWindow->wy, &zglWinCurrentWindow->ww, &zglWinCurrentWindow->wh );
		MoveWindow( (HWND)zglWinCurrentWindow->hwnd, zglWinCurrentWindow->wx, zglWinCurrentWindow->wy, zglWinCurrentWindow->ww, zglWinCurrentWindow->wh, 1 );
		zglWinCurrentWindow->style = ZGLWIN_WINSTYLE_NORMAL;
	}
	else if( zglWinCurrentWindow->style != ZGLWIN_WINSTYLE_BORDERLESS && style == ZGLWIN_WINSTYLE_BORDERLESS ) {
		int style = GetWindowLong( (HWND)zglWinCurrentWindow->hwnd, GWL_STYLE );
		style &= WS_VISIBLE;
		SetWindowLong( (HWND)zglWinCurrentWindow->hwnd, GWL_STYLE, style | WS_CLIPSIBLINGS | WS_CLIPCHILDREN );
		RECT rect;
		GetWindowRect( (HWND)zglWinCurrentWindow->hwnd, &rect );
		zglWinCurrentWindow->wx = rect.left;
		zglWinCurrentWindow->wy = rect.top;
		zglWinCurrentWindow->ww = rect.right - rect.left;
		zglWinCurrentWindow->wh = rect.bottom - rect.top;
		int a = GetSystemMetrics(SM_CYCAPTION);
		int b = GetSystemMetrics(SM_CXSIZEFRAME);
		int c = GetSystemMetrics(SM_CYSIZEFRAME);
		zglWinCurrentWindow->cx = rect.left + b;
		zglWinCurrentWindow->cy = rect.top + a + c;
		zglWinCurrentWindow->cw = rect.right - rect.left - b - b;
		zglWinCurrentWindow->ch = rect.bottom - rect.top - a - c - c;
		MoveWindow( (HWND)zglWinCurrentWindow->hwnd, zglWinCurrentWindow->cx, zglWinCurrentWindow->cy, zglWinCurrentWindow->cw, zglWinCurrentWindow->ch, 1 );
		zglWinCurrentWindow->style = ZGLWIN_WINSTYLE_BORDERLESS;
	}
	InvalidateRect( NULL, NULL, 1 );
}

void zglWinSwapBuffers() {
	SwapBuffers( (HDC)zglWinCurrentWindow->hdc );
}

void zglWinPostRedisplay() {
	zglWinCurrentWindow->redisplay = 1;
}

void zglWinReshapeWindow( int w, int h ) {
	RECT changes;
	GetClientRect( (HWND)zglWinCurrentWindow->hwnd, &changes );

	POINT point;
	point.x = 0;
	point.y = 0;
	ClientToScreen( (HWND)zglWinCurrentWindow->hwnd, &point );
	changes.left = point.x;
	changes.top = point.y;
	changes.right = changes.left + w;
	changes.bottom = changes.top + h;

	int style = GetWindowLong( (HWND)zglWinCurrentWindow->hwnd, GWL_STYLE );
	AdjustWindowRect( &changes, style, FALSE );

	SetWindowPos( (HWND)zglWinCurrentWindow->hwnd, HWND_TOP, 
		changes.left, changes.top, 
		changes.right - changes.left, changes.bottom - changes.top, 
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING | SWP_NOZORDER
	);

	if( zglWinCurrentWindow->reshapeFunc ) {
		(*zglWinCurrentWindow->reshapeFunc)(w,h);
	}
}

int zglWinPump() {
	MSG msg;
	while( PeekMessage (&msg, NULL, 0, 0, PM_NOYIELD|PM_REMOVE) ) {
		if( msg.message == WM_QUIT ) {
			return 0/*msg.wParam*/;
		}
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}

	for( int i=0; i < ZGLWIN_MAX_WINDOWS; i++ ) {
		zglWinWindows[i].setActive();

		if( zglWinWindows[i].redisplay && zglWinWindows[i].renderFunc ) {
			(*zglWinWindows[i].renderFunc)();
			zglWinWindows[i].redisplay = 0;
		}

		if( zglWinWindows[i].idleFunc ) {
			(*zglWinWindows[i].idleFunc)();
		}
	}
	return 1;
}

int zglWinMainLoop() {
	while( zglWinPump() );
	return 1;
}


static int shiftState = 0, ctrlState = 0, altState = 0;

void zglWinModifiers( int &shift, int &ctrl, int &alt ) {
	shift = shiftState;
	ctrl = ctrlState;
	alt = altState;
}

void zglWinGetClientRect( int *x, int *y, int *w, int *h ) {
	RECT rect;
	GetWindowRect( (HWND)zglWinCurrentWindow->hwnd, &rect );
	int a = GetSystemMetrics(SM_CYCAPTION);
	int b = GetSystemMetrics(SM_CXSIZEFRAME);
	int c = GetSystemMetrics(SM_CYSIZEFRAME);
	*x = rect.left + b;
	*y = rect.top + a + c;
	*w = rect.right - rect.left - b - b;
	*h = rect.bottom - rect.top - a - c - c;
}

void zglWinSetClientRect( int x, int y, int w, int h ) {
	zglWinCurrentWindow->wx = zglWinCurrentWindow->cx = x;
	zglWinCurrentWindow->wy = zglWinCurrentWindow->cy = y;
	zglWinCurrentWindow->ww = zglWinCurrentWindow->cw = w;
	zglWinCurrentWindow->wh = zglWinCurrentWindow->ch = h;
	adjustCoords( &zglWinCurrentWindow->wx, &zglWinCurrentWindow->wy, &zglWinCurrentWindow->ww, &zglWinCurrentWindow->wh );
	MoveWindow( (HWND)zglWinCurrentWindow->hwnd, zglWinCurrentWindow->wx, zglWinCurrentWindow->wy, zglWinCurrentWindow->ww, zglWinCurrentWindow->wh, 1 );
}

void zglSetWindowSize( int w, int h ) {
	zglWinCurrentWindow->ww = zglWinCurrentWindow->cw = w;
	zglWinCurrentWindow->wh = zglWinCurrentWindow->ch = h;
	adjustCoords( &zglWinCurrentWindow->wx, &zglWinCurrentWindow->wy, &zglWinCurrentWindow->ww, &zglWinCurrentWindow->wh );
	MoveWindow( (HWND)zglWinCurrentWindow->hwnd, zglWinCurrentWindow->wx, zglWinCurrentWindow->wy, zglWinCurrentWindow->ww, zglWinCurrentWindow->wh, 1 );
}

void zglSetWindowPos( int x, int y ) {
	zglWinCurrentWindow->wx = zglWinCurrentWindow->cx = x;
	zglWinCurrentWindow->wy = zglWinCurrentWindow->cy = y;
	adjustCoords( &zglWinCurrentWindow->wx, &zglWinCurrentWindow->wy, &zglWinCurrentWindow->ww, &zglWinCurrentWindow->wh );
	MoveWindow( (HWND)zglWinCurrentWindow->hwnd, zglWinCurrentWindow->wx, zglWinCurrentWindow->wy, zglWinCurrentWindow->ww, zglWinCurrentWindow->wh, 1 );
}

static long __stdcall zglWinWndProc( HWND hWnd, unsigned int message, unsigned int wParam, long lParam ) {
	ZGLWinWin *win = (ZGLWinWin *)GetWindowLong( hWnd, GWL_USERDATA );
	if( !win ) return DefWindowProc( hWnd, message, wParam, lParam );

	ZGLWinWin *oldWin = zglWinCurrentWindow;
	int ret = 0;
	win->setActive();
	zglWinCurrentWindow = win;

	char button = -1;
	switch( message ) {
		case WM_COPYDATA: {
			PCOPYDATASTRUCT pMyCDS = (PCOPYDATASTRUCT)lParam;
			if( pMyCDS->dwData == 0xAABBCCDD ) {
				char *msg = (char *)pMyCDS->lpData;
				if( win->specialFunc ) {
					(*win->specialFunc)( msg );
				}
			}
		}
		break;

		{
		case WM_LBUTTONDOWN:
			button = 'L';
		case WM_MBUTTONDOWN:
			if( button == -1 ) {
				button = 'M';
			}
		case WM_RBUTTONDOWN:
			if( button == -1 ) {
				button = 'R';
			}
		    SetCapture(hWnd);
			int x = LOWORD(lParam);
			int y = HIWORD(lParam);
			if( win->buttonFunc ) {
				(*win->buttonFunc)( button, 1, x, y );
			}
			break;

		}
		{
		case WM_LBUTTONUP:
			button = 'L';
		case WM_MBUTTONUP:
			if( button == -1 ) {
				button = 'M';
			}
		case WM_RBUTTONUP:
			if( button == -1 ) {
				button = 'R';
			}
		    ReleaseCapture();
			int x = LOWORD(lParam);
			int y = HIWORD(lParam);
			win->setActive();
			if( win->buttonFunc ) {
				(*win->buttonFunc)( button, 0, x, y );
			}
			break;
		}

		case WM_MOUSEMOVE: {
			int x = LOWORD( lParam );
			int y = HIWORD( lParam );
			static int lastX = -1;
			static int lastY = -1;
			if( x != lastX || y != lastY ) {
				win->setActive();
				if( win->motionFunc ) {
					(*win->motionFunc)( x, y );
				}
			}
			lastX = x;
			lastY = y;
			ret = 0;
			goto EXIT_FUNCTION;
		}
		case WM_CLOSE:
			PostQuitMessage( 0 );
			ret = 0;
			goto EXIT_FUNCTION;

		case WM_PAINT:
			PAINTSTRUCT ps;
			BeginPaint( (HWND)win->hwnd, &ps );
			EndPaint( (HWND)win->hwnd, &ps );
			ret = 0;
			goto EXIT_FUNCTION;

		case WM_MOVE: {
			int x = (int)(short)LOWORD(lParam);
			int y = (int)(short)HIWORD(lParam);
			if( win->moveFunc ) {
				win->setActive();
				GdiFlush();
				(*win->moveFunc)( x, y );
			}
			break;
		}

		case WM_SIZE: {
			int w = LOWORD(lParam);
			int h = HIWORD(lParam);
			if( win->reshapeFunc ) {
				win->setActive();
				GdiFlush();
				(*win->reshapeFunc)( w, h );
			}
			break;
		}

		case WM_MOUSEWHEEL: {
			POINT p;
			GetCursorPos(&p);
			ScreenToClient( (HWND)win->hwnd, &p);
			win->setActive();
			if( win->keyFunc ) {
				short delta = (short)HIWORD(wParam);    
				char *key = delta > 0 ? (char *)"wheelforward" : (char *)"wheelbackward";
				(*win->keyFunc)( key, 2, p.x, p.y );
			}
		}

		case WM_SYSCHAR:
		case WM_CHAR: {
			POINT p;
			GetCursorPos(&p);
			ScreenToClient( (HWND)win->hwnd, &p);
			win->setActive();
			if( win->keyFunc ) {
				char *key = 0;
				char _key[4];
				switch( wParam ) {
					case 27: key = "escape"; break;
					case '\'': key = "quote"; break;
					case '\"': key = "doublequote"; break;
					case '\b': key = "backspace"; break;
					case '\t': key = "tab"; break;
					default:
						_key[0] = wParam;
						_key[1] = 0;
						key = _key;
						break;
				}
				(*win->keyFunc)( key, 2, p.x, p.y );
			}
		}

		case WM_KEYUP:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN: {
			POINT p;
			GetCursorPos(&p);
			ScreenToClient( (HWND)win->hwnd, &p);
			char *key = 0;
			switch (wParam) {
				case VK_F1: key = "f1"; break;
				case VK_F2: key = "f2"; break;
				case VK_F3: key = "f3"; break;
				case VK_F4: key = "f4"; break;
				case VK_F5: key = "f5"; break;
				case VK_F6: key = "f6"; break;
				case VK_F7: key = "f7"; break;
				case VK_F8: key = "f8"; break;
				case VK_F9: key = "f9"; break;
				case VK_F10: key = "f10"; break;
				case VK_F11: key = "f11"; break;
				case VK_F12: key = "f12"; break;
				case VK_LEFT: key = "left"; break;
				case VK_UP: key = "up"; break;
				case VK_RIGHT: key = "right"; break;
				case VK_DOWN: key = "down"; break;
				case VK_PRIOR: key = "pageup"; break;
				case VK_NEXT: key = "pagedown"; break;
				case VK_HOME: key = "home"; break;
				case VK_END: key = "end"; break;
				case VK_INSERT: key = "insert"; break;
				case VK_DELETE: key = "delete"; break;
			}
			if( key ) {
				win->setActive();
				if( win->keyFunc ) {
					if( message==WM_KEYDOWN || message==WM_SYSKEYDOWN ) {
						(*win->keyFunc)( key, 2, p.x, p.y );
					}
					(*win->keyFunc)( key, (message==WM_KEYUP||message==WM_SYSKEYUP)?0:1, p.x, p.y );
				}
			}
			else {
				int mod = 0;
				switch( wParam ) {
					case VK_SHIFT: shiftState = (message==WM_KEYUP||message==WM_SYSKEYUP)?0:1; mod=1; break;
					case VK_CONTROL: ctrlState = (message==WM_KEYUP||message==WM_SYSKEYUP)?0:1; mod=1; break;
					case VK_MENU: altState = (message==WM_KEYUP||message==WM_SYSKEYUP)?0:1; mod=1; break;
				}
			}
		}
		ret = 0;
		goto EXIT_FUNCTION;
	}
	
	ret = DefWindowProc( hWnd, message, wParam, lParam );


	EXIT_FUNCTION:

	oldWin->setActive();
	zglWinCurrentWindow = oldWin;

	return ret;
}

void zglWinDisplayFunc( void (*f)() ) {
	zglWinCurrentWindow->renderFunc = f;
}

void zglWinIdleFunc( void (*f)() ) {
	zglWinCurrentWindow->idleFunc = f;
}

void zglWinReshapeFunc( void (*f)(int w, int h) ) {
	zglWinCurrentWindow->reshapeFunc = f;

	RECT rect;
	GetClientRect( (HWND)zglWinCurrentWindow->hwnd, &rect );
	(*zglWinCurrentWindow->reshapeFunc)( rect.right-rect.left, rect.bottom-rect.top );
}

void zglWinMotionFunc( void (*f)(int x, int y) ) {
	zglWinCurrentWindow->motionFunc = f;
}

void zglWinKeyboardFunc( void (*f)(char *key, int state, int x, int y) ) {
	zglWinCurrentWindow->keyFunc = f;
}

void zglWinMouseFunc( void (*f)(char button, int pressed, int x, int y) ) {
	zglWinCurrentWindow->buttonFunc = f;
}

void zglWinSpecialFunc( void (*f)( char *message ) ) {
	zglWinCurrentWindow->specialFunc = f;
}

void zglWinMoveFunc( void (*f)( int x, int y ) ) {
	zglWinCurrentWindow->moveFunc = f;
}

//////////////////////////////////////////////////////////////////////////////////
// Client Code

#ifdef SELF_TEST

void render() {
	glClearColor( 1.f, 1.f, 1.f, 1.f );
	glClear( GL_COLOR_BUFFER_BIT );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	static float rot = 1.f;
	rot += 1.f;
	glRotatef( rot, 0.f, 1.f, 0.f );

	glColor3f( 1.f, 0.f, 0.f );
	glBegin( GL_QUADS );
		glVertex2f( 0.0f, 0.0f );
		glVertex2f( 0.0f, 1.0f );
		glVertex2f( 1.0f, 1.0f );
		glVertex2f( 1.0f, 0.0f );
	glEnd();

	glFlush();
	zglWinSwapBuffers();
}

void reshapeHandler( int w, int h ) {
	glViewport(0, 0, w, h);

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective( 45.f, (float)w/(float)h, 0.1f, 200.f );
	glTranslated( 0.0, 0.0, -20 );

	glMatrixMode( GL_MODELVIEW );
}

void mainLoop() {
	Sleep(1);
	zglWinPostRedisplay();
}

void mouseButtonHandler( char button, int state, int _x, int _y ) {
	int a = 1;
}

void mouseMotionHandler( int _x, int _y ) {
	int a = 1;
}

void keyHandler( char *key, int state, int x, int y ) {
	int a = 1;
}

int __stdcall WinMain( HINSTANCE _hInstance, HINSTANCE hPrevInstance, char *nCmdParam, int nCmdShow ) {
	zglWinInit();

	zglWinDisplayFunc( render );
	zglWinIdleFunc( mainLoop );
	zglWinReshapeFunc( reshapeHandler );
 	zglWinMouseFunc( mouseButtonHandler );
	zglWinMotionFunc( mouseMotionHandler );
	zglWinKeyboardFunc( keyHandler );

	zglWinCreateWindow( "Shadow Garden", 0, 0, 640, 480 );

	zglWinReshapeWindow( 1024, 768 );

	return zglWinMainLoop();
}

#endif

