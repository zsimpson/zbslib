// @ZBS {
//		*MODULE_NAME Assert Wrapper
//		+DESCRIPTION {
//			An improved assert handler that does stack tracing and emailing
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zassert.cpp zassert.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH yes
// }
// OPERATING SYSTEM specific includes:
#ifdef WIN32
	#include "windows.h"
#else
	#include "netdb.h"
	#include "unistd.h"
	#include "sys/types.h"
	#include "sys/socket.h"
	#include "netinet/in.h"
	#include "arpa/inet.h"
#endif
// SDK includes:
// STDLIB includes:
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#ifdef WIN32
#include "malloc.h"
#endif
// MODULE includes:
#include "zstacktrace.h"
#include "zassert.h"
// ZBSLIB includes:

#ifdef WIN32
	#define CLOSESOCKET closesocket
#else
	#define CLOSESOCKET ::close
#endif

#ifdef WIN32

// Assert Box Global Variables.
HWND hAssertBox = NULL;
HWND hTextArea = NULL;
HWND hEditArea = NULL;
HWND hOKButton = NULL;
HWND hEmailButton = NULL;
HWND hCopyButton = NULL;
void (*emailButtonFuncPtr)(char *);

// WndProc that runs the assert dialog box.
long CALLBACK assertBoxWndProc( HWND hWnd, UINT message, UINT wParam, long lParam ) {
	switch( message ) {	
		case WM_SIZE: {
			HDC hDC = GetDC( hOKButton );
			SIZE s;
			GetTextExtentPoint32( hDC, "W", 1, &s );
			int margin = 10;
			int w = LOWORD(lParam);
			int h = HIWORD(lParam);
			int bw = (w - 4*margin) / 3;
			int bh = s.cy + 6;
			int upperH = h - 3*margin - bh;
			int textH  = (upperH * 6) / 10 - margin;
			int editH  = (upperH * 4) / 10 ;
			int editY  = 2*margin + textH;
			int x = margin;
			int y = h - margin - bh;
			MoveWindow( hOKButton,    x, y, bw, bh, TRUE ); x += bw+margin;
			MoveWindow( hEmailButton, x, y, bw, bh, TRUE ); x += bw+margin;
			MoveWindow( hCopyButton,  x, y, bw, bh, TRUE ); x += bw+margin;
			MoveWindow( hTextArea, margin, margin, w-2*margin, textH, TRUE );
			MoveWindow( hEditArea, margin, editY,  w-2*margin, editH, TRUE );
			break;
		}
		case WM_PAINT: {
			// Paint the loading screen
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint( hWnd, &ps );
			EndPaint( hWnd, &ps );
			break;
		}
		case WM_COMMAND: {
			HWND who = (HWND)lParam;
			if( who == hOKButton ) {
				PostMessage( hWnd, WM_CLOSE, 0, 0 );
			}
			else if( who == hEmailButton ) {
				int textSize = 
					GetWindowTextLength( hTextArea )
					+ GetWindowTextLength( hEditArea )
				;
				char *msg = (char *)alloca( textSize );
				GetWindowText( hTextArea, msg, textSize+1 );
				GetWindowText( hEditArea, msg+strlen(msg), textSize-strlen(msg)+1 );
				if( emailButtonFuncPtr ) {
					(*emailButtonFuncPtr)(msg);
				}
				EnableWindow( hEmailButton, FALSE );
			}
			else if( who == hCopyButton ) {
				int textSize = 
					GetWindowTextLength( hTextArea )
					+ GetWindowTextLength( hEditArea )
				;
				if( !OpenClipboard(NULL) ) {
					break;
				}
				EmptyClipboard();
				HGLOBAL hglbCopy = GlobalAlloc( GMEM_DDESHARE, (textSize+1) * sizeof(TCHAR) ); 
				if( hglbCopy == NULL ) {
					CloseClipboard();
					break;
				}
				char *globalStr = (char *)GlobalLock( hglbCopy );
				GetWindowText( hTextArea, globalStr, textSize+1 );
				GetWindowText( hEditArea, globalStr+strlen(globalStr), textSize-strlen(globalStr)+1 );
				GlobalUnlock (hglbCopy);
				SetClipboardData( CF_TEXT, hglbCopy );
				CloseClipboard(); 
				EnableWindow( hCopyButton, FALSE );
			}
		}
	}
	return DefWindowProc(hWnd, message, wParam, lParam );
}

void *createAssertBox(
	char *msg, 
	void (*emailButtonFuncPtr)(char *),
	char *titleText,
	char *okButtonText,
	char *emailButtonText,
	char *copyButtonText,
	char *editAreaText,
	void *parentBox
) {
	if( !titleText ) {
		titleText = "Assertion Failed";
	}
	if( !okButtonText ) {
		okButtonText = "OK";
	}
	if( !emailButtonText ) {
		emailButtonText = "E-Mail";
	}
	if( !copyButtonText ) {
		copyButtonText = "Copy";
	}
	if( !editAreaText ) {
		editAreaText =
			"Please enter description of what you were "
			"doing at the time this assertion occured here."
		;
	}
	::emailButtonFuncPtr = emailButtonFuncPtr;

	WNDCLASSEX  wc;
	memset(&wc, 0, sizeof(WNDCLASSEX) );
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.lpfnWndProc = (WNDPROC)assertBoxWndProc;
	wc.lpszClassName = "AssertBox";
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	RegisterClassEx(&wc);
	hAssertBox = CreateWindowEx( 
		WS_EX_DLGMODALFRAME, "AssertBox", titleText, WS_SIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 100, 100, NULL, NULL, NULL, NULL
	);
	hTextArea = CreateWindowEx( 
		WS_EX_STATICEDGE, "STATIC", msg, WS_CHILD,
		0, 0, 0, 0, hAssertBox, NULL, NULL, NULL
	);
	hEditArea = CreateWindowEx( 
		WS_EX_STATICEDGE, "EDIT", editAreaText, WS_CHILD|ES_MULTILINE|ES_WANTRETURN|ES_AUTOVSCROLL,
		0, 0, 0, 0, hAssertBox, NULL, NULL, NULL
	);
	hOKButton = CreateWindowEx(
		0, "BUTTON", okButtonText, BS_CENTER | BS_VCENTER | BS_DEFPUSHBUTTON | WS_CHILD,
		0, 0, 0, 0, hAssertBox, NULL, NULL, NULL
	);
	hEmailButton = CreateWindowEx(
		0, "BUTTON", emailButtonText, BS_CENTER | BS_VCENTER | BS_PUSHBUTTON | WS_CHILD,
		0, 0, 0, 0, hAssertBox, NULL, NULL, NULL
	);
	hCopyButton = CreateWindowEx(
		0, "BUTTON", copyButtonText, BS_CENTER | BS_VCENTER | BS_PUSHBUTTON | WS_CHILD,
		0, 0, 0, 0, hAssertBox, NULL, NULL, NULL
	);
	if( !emailButtonFuncPtr ) {
		EnableWindow( hEmailButton, FALSE );
	}
	int cx = GetSystemMetrics(SM_CXSCREEN);
	int cy = GetSystemMetrics(SM_CYSCREEN);
	MoveWindow ( hAssertBox, (cx-640)/2, (cy-300)/2, 640, 300, TRUE );
	ShowWindow( hTextArea, SW_SHOW );
	ShowWindow( hEditArea, SW_SHOW );
	ShowWindow( hOKButton, SW_SHOW );
	ShowWindow( hEmailButton, SW_SHOW );
	ShowWindow( hCopyButton, SW_SHOW );
	ShowWindow( hAssertBox, SW_SHOW );
	HFONT hGuiFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	HFONT hFixFont = (HFONT)GetStockObject(ANSI_FIXED_FONT);
	SendMessage( hTextArea,    WM_SETFONT, (int)hFixFont, MAKELONG( TRUE, 0 ) );
	SendMessage( hEditArea,    WM_SETFONT, (int)hGuiFont, MAKELONG( TRUE, 0 ) );
	SendMessage( hOKButton,    WM_SETFONT, (int)hGuiFont, MAKELONG( TRUE, 0 ) );
	SendMessage( hEmailButton, WM_SETFONT, (int)hGuiFont, MAKELONG( TRUE, 0 ) );
	SendMessage( hCopyButton,  WM_SETFONT, (int)hGuiFont, MAKELONG( TRUE, 0 ) );
	SetFocus( hEditArea );
	SendMessage( hEditArea, EM_SETSEL, 0, MAKELONG(1000, -1) );
	return hAssertBox;
}

void runAssertBox( void *hWnd ) {
	MSG m;
	while( GetMessage( &m, (HWND)hWnd, 0, 0) ) {
		if( m.message == WM_CLOSE ) {
			PostQuitMessage( 0 );
			return;
		}
		TranslateMessage( &m );
		DispatchMessage( &m );
	}
}

#endif

int emailMsgTo( char *msg, char *emailAddress ) {
	struct hostent *hostEnt;
	int ip = 0;
	sockaddr_in sockAddr;
	int sockFD;
	int _send, _recv, _connect;	
	char *computerName = "";
	char *emailAt;
	char *s0 = (char *)alloca( strlen(msg) + 256 ); // send buffer
	char *s1 = (char *)alloca( strlen(msg) + 256 ); // send buffer
	char r[2048]; // recv buffer
	char *d, *s;
	
	#ifdef WIN32
		WSAData wsaData;
		int ret = WSAStartup(0x0101, &wsaData);
		if( ret != 0 ) goto error;
	#endif

	// Add CRs to empty LFs in the msg
	d = s0;
	for( s = msg; *s; s++, d++ ) {
		*d = *s;
		if( *s == '\r' && *(s+1) == '\n' ) {
			// Matched pair, skip the \n
			s++;
		}
		else if( *s == '\r' && *(s+1) != '\n' ) {
			// CR without LF
			d++;
			*d = '\n';
		}
		else if( *s == '\n' ) {
			// Unmatched LF
			*d++ = '\r';
			*d = '\n';
		}
	}
	*d = 0;

	sockFD = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP );
	if( sockFD < 0 ) goto error;

	// Parse the emailAddress to get the copmuter name after the '@'
	emailAt = strchr( emailAddress, '@' );
	if( emailAt ) {
		computerName = emailAt+1;
	}

	ip = inet_addr( computerName );
	if( ip == INADDR_NONE ) {
		//#ifdef WIN32
		// @TODO: Deal with MX records
		//DNS_RECORD *result = 0;
		//DnsQuery( computerName,  DNS_TYPE_MX, DNS_QUERY_STANDARD, NULL, &result, NULL );
		//#endif

		hostEnt = gethostbyname( computerName );
		if( hostEnt ) {
			ip = *(int *)hostEnt->h_addr;
		}
	}
	memset( &sockAddr, 0, sizeof(sockAddr) );
	sockAddr.sin_family = AF_INET; 
	sockAddr.sin_port = htons(25);	// 25 = SMTP port
	sockAddr.sin_addr.s_addr = ip;

	_connect = connect( sockFD, (struct sockaddr *)&sockAddr, sizeof(sockAddr) );
	if( _connect < 0 ) goto error;

	memset( r, 0, 256 );
	_recv = recv( sockFD, r, 256, 0 );
	if( r[0] != '2' ) goto error;

	memset( r, 0, 256 );
	sprintf( s, "HELO eden.com\r\n" );
	_send = send( sockFD, s, strlen(s), 0 );
	if( _send < 0 ) goto error;
	_recv = recv( sockFD, r, 256, 0 );
	if( r[0] != '2' ) goto error;

	memset( r, 0, 256 );
	sprintf( s, "MAIL FROM: assertbox@eden.com\r\n" );
	_send = send( sockFD, s, strlen(s), 0 );
	if( _send < 0 ) goto error;
	_recv = recv( sockFD, r, 256, 0 );
	if( r[0] != '2' ) goto error;

	memset( r, 0, 256 );
	sprintf( s, "RCPT TO: %s\r\n", emailAddress );
	_send = send( sockFD, s, strlen(s), 0 );
	if( _send < 0 ) goto error;
	_recv = recv( sockFD, r, 256, 0 );
	if( r[0] != '2' ) goto error;

	memset( r, 0, 256 );
	sprintf( s, "DATA\r\n" );
	_send = send( sockFD, s, strlen(s), 0 );
	if( _send < 0 ) goto error;
	_recv = recv( sockFD, r, 256, 0 );
	if( r[0] != '3' ) goto error;

	memset( r, 0, 256 );
	sprintf( s1, "From assertbox@eden.com\r\nSubject: Assert Box\r\n%s\r\n.\r\n", s0 );
	_send = send( sockFD, s1, strlen(s1), 0 );
	if( _send < 0 ) goto error;
	_recv = recv( sockFD, r, 256, 0 );
	if( r[0] != '2' ) goto error;

	memset( r, 0, 256 );
	sprintf( s, "QUIT\r\n" );
	_send = send( sockFD, s, strlen(s), 0 );
	if( _send < 0 ) goto error;
	_recv = recv( sockFD, r, 256, 0 );
	CLOSESOCKET( sockFD );
	return 1;

	error:
	CLOSESOCKET( sockFD );
	return 0;
}