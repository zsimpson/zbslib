// @ZBS {
//		*MODULE_NAME zglfwtools
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Tools for doing extra manipulation to the glfw window toolkit
//		}
//		+EXAMPLE {
//		}
//		*PORTABILITY win32 unix macosx
//		*REQUIRED_FILES zglfwtools.cpp zglfwtools.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH yes
//		*SDK_DEPENDS glfw-2.7.2
// }

// OPERATING SYSTEM specific includes:
#ifdef WIN32
#include "windows.h"
#endif
#include "GL/glfw.h"
// STDLIB includes:
#include "stdlib.h"
#include "ctype.h"
#include "string.h"
// MODULE includes:
// ZBSLIB includes:
#include "zmsg.h"
#include "zmousemsg.h"
#include "zglfwtools.h"
#include "ztime.h"

ZTime zglfwLastActivity = 0.0;

void zglfwTouchLastActivity() {
	zglfwLastActivity = zTimeNow();
}

double zglfwGetLastActivity() {
	return zglfwLastActivity;
}

void zglfwCharHandler( int code, int state ) {
	zglfwTouchLastActivity();
	if( state ) {
		static int shift = 0, ctrl = 0, alt = 0;
		zMouseMsgUpdateKeyModifierState( shift, ctrl, alt );
		ZMsg *msg = new ZMsg;
		msg->putS( "type", "Key" );
		msg->putI( "shift", shift?1:0 );
		msg->putI( "ctrl", ctrl?1:0 );
		msg->putI( "alt", alt?1:0 );
		char buf[2];
		buf[0] = code;
		buf[1] = 0;
		msg->putS( "which", buf );
		zMsgQueue( msg );
	}
}

static int shift = 0, ctrl = 0, alt = 0;

int zglfwGetShift() {
	return shift;
}

int zglfwGetCtrl() {
	return ctrl;
}

int zglfwGetAlt() {
	return alt;
}

void zglfwKeyHandler( int code, int state ) {
	zglfwTouchLastActivity();
	#ifdef __APPLE__
		// For some reason the GLFW under apple returns
		// a different code than does win32
		if( code == 13 ) code = GLFW_KEY_ENTER;
	#endif
	
	#if GLFW_VERSION_MINOR > 6
		// In the 2.7.2 version of GLFW we get the LSUPER and RSUPER keys as well: on a mac this is
		// the command key.  We don't want this for now:
		if( code == GLFW_KEY_LSUPER || code == GLFW_KEY_RSUPER ) {
			return;
		}
	#endif
	
	static int keyStates[GLFW_KEY_LAST+1] = {0,};
	char charBuf[16] = {0,};
	char *s = 0;
	int sendAsChar = 0;
	switch( code ) {
		case GLFW_KEY_UNKNOWN: break;
		case GLFW_KEY_SPACE:
			s = " ";
			break;
		case GLFW_KEY_SPECIAL: s = ""; break;
		case GLFW_KEY_ESC: s = "escape"; sendAsChar=1; break;
		case GLFW_KEY_F1: s = "f1"; sendAsChar=1; break;
		case GLFW_KEY_F2: s = "f2"; sendAsChar=1; break;
		case GLFW_KEY_F3: s = "f3"; sendAsChar=1; break;
		case GLFW_KEY_F4: s = "f4"; sendAsChar=1; break;
		case GLFW_KEY_F5: s = "f5"; sendAsChar=1; break;
		case GLFW_KEY_F6: s = "f6"; sendAsChar=1; break;
		case GLFW_KEY_F7: s = "f7"; sendAsChar=1; break;
		case GLFW_KEY_F8: s = "f8"; sendAsChar=1; break;
		case GLFW_KEY_F9: s = "f9"; sendAsChar=1; break;
		case GLFW_KEY_F10: s = "f10"; sendAsChar=1; break;
		case GLFW_KEY_F11: s = "f11"; sendAsChar=1; break;
		case GLFW_KEY_F12: s = "f12"; sendAsChar=1; break;
		case GLFW_KEY_F13: s = "f13"; sendAsChar=1; break;
		case GLFW_KEY_F14: s = "f14"; sendAsChar=1; break;
		case GLFW_KEY_F15: s = "f15"; sendAsChar=1; break;
		case GLFW_KEY_F16: s = "f16"; sendAsChar=1; break;
		case GLFW_KEY_F17: s = "f17"; sendAsChar=1; break;
		case GLFW_KEY_F18: s = "f18"; sendAsChar=1; break;
		case GLFW_KEY_F19: s = "f19"; sendAsChar=1; break;
		case GLFW_KEY_F20: s = "f20"; sendAsChar=1; break;
		case GLFW_KEY_F21: s = "f21"; sendAsChar=1; break;
		case GLFW_KEY_F22: s = "f22"; sendAsChar=1; break;
		case GLFW_KEY_F23: s = "f23"; sendAsChar=1; break;
		case GLFW_KEY_F24: s = "f24"; sendAsChar=1; break;
		case GLFW_KEY_F25: s = "f25"; sendAsChar=1; break;
		case GLFW_KEY_UP: s = "up"; sendAsChar=1; break;
		case GLFW_KEY_DOWN: s = "down"; sendAsChar=1; break;
		case GLFW_KEY_LEFT: s = "left"; sendAsChar=1; break;
		case GLFW_KEY_RIGHT: s = "right"; sendAsChar=1; break;
		case GLFW_KEY_LSHIFT: s = "lshift"; break;
		case GLFW_KEY_RSHIFT: s = "rshift"; break;
		case GLFW_KEY_LCTRL: s = "lctrl"; break;
		case GLFW_KEY_RCTRL: s = "rctrl"; break;
		case GLFW_KEY_LALT: s = "lalt"; break;
		case GLFW_KEY_RALT: s = "ralt"; break;
#if GLFW_VERSION_MINOR > 6
		case GLFW_KEY_LSUPER: s = "lsuper"; break;
		case GLFW_KEY_RSUPER: s = "rsuper"; break;
#endif
		case GLFW_KEY_TAB: s = "tab"; sendAsChar=1; break;
		case GLFW_KEY_ENTER: s = "\n"; sendAsChar=1; break;
		case GLFW_KEY_KP_ENTER: s = "\n"; sendAsChar=1; break;
		case GLFW_KEY_BACKSPACE: s = "backspace"; sendAsChar=1; break;
		case GLFW_KEY_INSERT: s = "insert"; sendAsChar=1; break;
		case GLFW_KEY_DEL: s = "delete"; sendAsChar=1; break;
		case GLFW_KEY_PAGEUP: s = "pageup"; sendAsChar=1; break;
		case GLFW_KEY_PAGEDOWN: s = "pagedown"; sendAsChar=1; break;
		case GLFW_KEY_HOME: s = "home"; sendAsChar=1; break;
		case GLFW_KEY_END: s = "end"; sendAsChar=1; break;
		case GLFW_KEY_KP_0: s = "0"; break;
		case GLFW_KEY_KP_1: s = "1"; break;
		case GLFW_KEY_KP_2: s = "2"; break;
		case GLFW_KEY_KP_3: s = "3"; break;
		case GLFW_KEY_KP_4: s = "4"; break;
		case GLFW_KEY_KP_5: s = "5"; break;
		case GLFW_KEY_KP_6: s = "6"; break;
		case GLFW_KEY_KP_7: s = "7"; break;
		case GLFW_KEY_KP_8: s = "8"; break;
		case GLFW_KEY_KP_9: s = "9"; break;
		case GLFW_KEY_KP_DIVIDE: s = "/"; break;
		case GLFW_KEY_KP_MULTIPLY: s = "*"; break;
		case GLFW_KEY_KP_SUBTRACT: s = "-"; break;
		case GLFW_KEY_KP_ADD: s = "+"; break;
		case GLFW_KEY_KP_DECIMAL: s = "."; break;
		case GLFW_KEY_KP_EQUAL: s = "="; break;
#if GLFW_VERSION_MINOR > 6
			// The following added with glfw 2.7.2
		case GLFW_KEY_KP_NUM_LOCK: s = "numlock"; sendAsChar=1; break;
		case GLFW_KEY_CAPS_LOCK: s = "casplock"; sendAsChar=1; break;
		case GLFW_KEY_SCROLL_LOCK: s = "scrolllock"; sendAsChar=1; break;
		case GLFW_KEY_PAUSE: s = "pause"; sendAsChar=1; break;
		case GLFW_KEY_MENU: s = "menu"; sendAsChar=1; break;
#endif
		default:
			if( code >= 32 && code <= 255 ) {
				if( ! shift && code >= 'A' && code <= 'Z' ) {
					code = tolower( code );
				}
				charBuf[0] = (char)code;
				charBuf[1] = 0;
				s = charBuf;
				if( ctrl ) {
					charBuf[0] = 'c';
					charBuf[1] = 't';
					charBuf[2] = 'r';
					charBuf[3] = 'l';
					charBuf[4] = '_';
					charBuf[5] = (char)code;
					charBuf[6] = 0;
					s = charBuf;
					sendAsChar = 1;
				}
				else if( alt ) {
					charBuf[0] = 'a';
					charBuf[1] = 'l';
					charBuf[2] = 't';
					charBuf[3] = '_';
					charBuf[4] = (char)code;
					charBuf[5] = 0;
					s = charBuf;
					sendAsChar = 1;
				}
			}
			else {
				strcpy( charBuf, "unknown_key" );
				s = charBuf;
				sendAsChar = 1;
			}
			break;
	}

	ZMsg *zMsg0 = 0, *zMsg1 = 0;
	int meta = 0;
	if( state == GLFW_RELEASE ) {
		keyStates[code]=0;
		zMsg0 = zMsgQueue( "type=KeyUp which='%s'", s );

		switch( code ) {
			case GLFW_KEY_LSHIFT: meta = 1; shift &= ~1; break;
			case GLFW_KEY_RSHIFT: meta = 1; shift &= ~2; break;
			case GLFW_KEY_LCTRL: meta = 1; ctrl &= ~1; break;
			case GLFW_KEY_RCTRL: meta = 1; ctrl &= ~2; break;
			case GLFW_KEY_LALT: meta = 1; alt &= ~1; break;
			case GLFW_KEY_RALT: meta = 1; alt &= ~2; break;
		}
	}
	else if( state == GLFW_PRESS ) {
		zMsg0 = zMsgQueue( "type=KeyDown which='%s'", s );

		keyStates[code]=1;
		switch( code ) {
			case GLFW_KEY_LSHIFT: meta = 1; shift |= 1; break;
			case GLFW_KEY_RSHIFT: meta = 1; shift |= 2; break;
			case GLFW_KEY_LCTRL: meta = 1; ctrl |= 1; break;
			case GLFW_KEY_RCTRL: meta = 1; ctrl |= 2; break;
			case GLFW_KEY_LALT: meta = 1; alt |= 1; break;
			case GLFW_KEY_RALT: meta = 1; alt |= 2; break;
		}

		if( !meta && s && sendAsChar ) {
			zMsg1 = zMsgQueue( "type=Key shift=%d ctrl=%d alt=%d which='%s'", shift?1:0, ctrl?1:0, alt?1:0, s );
		}
	}

	if( zMsg0 && *s=='\'' ) {
		// Quote is a speical case because it is interpreted by the parser
		zMsg0->putS( "which", "'" );
	}
	if( zMsg1 && *s=='\'' ) {
		// Quote is a speical case because it is interpreted by the parser
		zMsg1->putS( "which", "'" );
	}

	if( meta ) {
		zMouseMsgUpdateKeyModifierState( shift, ctrl, alt );
	}
}

void zglfwMouseWheelHandler( int state ) {
	zglfwTouchLastActivity();
	static int lastState = 0;
	if( state > lastState ) {
		zMsgQueue( "type=Key which=wheelforward x=%d y=%d", zMouseMsgX, zMouseMsgY );
	}
	else if( state < lastState ) {
		zMsgQueue( "type=Key which=wheelbackward x=%d y=%d", zMouseMsgX, zMouseMsgY );
	}
	lastState = state;
}

static int zglfwStyle = 0;

void zglfwGetWindowGeomOfClientArea( int *x, int *y, int *w, int *h ) {
	glfwGetWindowGeom( x, y, w, h );
	#ifdef WIN32
	if( zglfwWinGetWindowStyle() != zglfwWinStyleBorderless ) {
		int a = GetSystemMetrics(SM_CXSIZEFRAME);
		int b = GetSystemMetrics(SM_CYSIZEFRAME);
		int c = GetSystemMetrics(SM_CYCAPTION);

		*x += a;
		*y += b;
		*y += c;
		//*w -= a*2;
		//*h -= b*2 + c;
	}
	#endif
}

void zglfwGetWindowGeom( int *x, int *y, int *w, int *h ) {
	glfwGetWindowGeom( x, y, w, h );
	#ifdef WIN32
	if( zglfwStyle == zglfwWinStyleBorderless ) {
		int a = GetSystemMetrics(SM_CXSIZEFRAME);
		int b = GetSystemMetrics(SM_CYSIZEFRAME);
		int c = GetSystemMetrics(SM_CYCAPTION);

		*x -= a;
		*y -= b;
		*y -= c;
		*w += a*2;
		*h += b*2 + c;
	}
	#endif
}

int zglfwWinGetWindowStyle() {
	#ifdef WIN32
	HWND hwnd = (HWND)glfwZBSExt_GetHWND();
	int style = GetWindowLong( hwnd, GWL_STYLE );
	if( style & WS_POPUP ) {
		return zglfwWinStyleBorderless;
	}
	else {
		return zglfwWinStyleBorder;
	}
	#endif
}

void zglfwWinSetWindowStyle( int _style ) {
	#ifdef WIN32
	HWND hwnd = (HWND)glfwZBSExt_GetHWND();
	RECT rect;

	if( _style == zglfwWinStyleBorder ) {
		int style = GetWindowLong( hwnd, GWL_STYLE );
		style &= WS_VISIBLE;
		SetWindowLong( hwnd, GWL_STYLE, style | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW );

		GetWindowRect( hwnd, &rect );
		AdjustWindowRect( &rect, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW, FALSE );
		
		MoveWindow( hwnd, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, 1 );
		SetWindowPos( hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE );
	}
	else if( _style == zglfwWinStyleBorderless ) {
		int style = GetWindowLong( hwnd, GWL_STYLE );
		style &= WS_VISIBLE;
		SetWindowLong( hwnd, GWL_STYLE, style | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP );

		GetWindowRect( hwnd, &rect );
		int a = GetSystemMetrics(SM_CYCAPTION);
		int b = GetSystemMetrics(SM_CXSIZEFRAME);
		int c = GetSystemMetrics(SM_CYSIZEFRAME);
		int cx = rect.left + b;
		int cy = rect.top + a + c;
		int cw = rect.right - rect.left - b - b;
		int ch = rect.bottom - rect.top - a - c - c;
		MoveWindow( hwnd, cx, cy, cw, ch, 1 );
		SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE );
	}
	InvalidateRect( NULL, NULL, 1 );

	zglfwStyle = _style;
	#endif
}

void zglfwSetMaskingWindow( int show, int x, int y, int	w, int h ) {
	#ifdef WIN32
	if( x==-1 ) x = 0;
	if( y==-1 ) y = 0;
	if( w==-1 ) w = GetSystemMetrics( SM_CXSCREEN );
	if( h==-1 ) h = GetSystemMetrics( SM_CYSCREEN );

	static int init = 0;
	static HWND black = 0;
	if( !init ) {
		init++;
		WNDCLASS wc;
		HINSTANCE hInstance = GetModuleHandle(NULL);
		memset( &wc, 0, sizeof(WNDCLASS) );
		wc.style = CS_OWNDC;
		wc.lpfnWndProc = DefWindowProc;
		wc.hInstance = hInstance;
		wc.hIcon = LoadIcon( NULL, IDI_WINLOGO );
		wc.hCursor = LoadCursor( 0, IDC_ARROW );
		wc.hbrBackground = (HBRUSH)BLACK_BRUSH+1;
		wc.lpszMenuName = NULL;
		wc.lpszClassName = "blackwnd";
		RegisterClass(&wc);
		black = CreateWindow(
			"blackwnd", "blackwnd", WS_POPUP, 
			x, y, w, h, NULL, NULL, GetModuleHandle(NULL), 0
		);
	}
	ShowWindow( black, show ? SW_SHOWNORMAL : SW_HIDE );
	SetWindowPos( black, HWND_TOPMOST, x, y, w, h, 0 );
	#endif
}

#ifdef WIN32
    
static void *zglfwOldWindowProc = 0;

static long __stdcall zglfwRemoteControlInterceptWndProc( HWND hWnd, unsigned int message, unsigned int wParam, long lParam ) {
	if( message == WM_COPYDATA ) {
		PCOPYDATASTRUCT pMyCDS = (PCOPYDATASTRUCT)lParam;
		if( pMyCDS->dwData == 0xAABBCCDD ) {
			char *msg = (char *)pMyCDS->lpData;
			zMsgQueue( msg );
		}
	}
	return CallWindowProc( (WNDPROC)zglfwOldWindowProc, hWnd, message, wParam, lParam );
}

#endif

void zglfwInitRemoteControlIntercept() {
	#ifdef WIN32
	// INSTALL wndproc intercept to grab the remote messages
	if( !zglfwOldWindowProc ) {
		HWND hwnd = (HWND)glfwZBSExt_GetHWND();
		#ifdef POINTER_64
				zglfwOldWindowProc = (void *)GetWindowLong( hwnd, GWLP_WNDPROC );
				SetWindowLong( hwnd, GWLP_WNDPROC, (long)zglfwRemoteControlInterceptWndProc );
		#else
				zglfwOldWindowProc = (void *)GetWindowLong(hwnd, GWL_WNDPROC);
				SetWindowLong(hwnd, GWL_WNDPROC, (long)zglfwRemoteControlInterceptWndProc);
		#endif
	}
	#endif
}

