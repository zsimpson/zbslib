#ifdef __USE_GNU
typedef long long __int64;
#else
#ifndef _WINDOWS_
#ifndef max
	#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min
	#define min(a,b) ((a)<(b)?(a):(b))
#endif

extern "C" {
// TNB - I added these to support the USB key library
typedef void *PVOID;
typedef unsigned long ULONG;
typedef unsigned long DWORD;

typedef void *HANDLE;
typedef union _LARGE_INTEGER {
	struct { unsigned long LowPart; long HighPart; };
	struct { unsigned long LowPart; long HighPart; } u;
	__int64 QuadPart;
} LARGE_INTEGER;

__declspec(dllimport) unsigned int __stdcall GetCurrentDirectoryA( unsigned int nBufferLength, char *lpBuffer );
#define GetCurrentDirectory GetCurrentDirectoryA
__declspec(dllimport) void * __stdcall HeapCreate( unsigned long flOptions, unsigned long dwInitialSize, unsigned long dwMaximumSize );
__declspec(dllimport) int __stdcall QueryPerformanceCounter( LARGE_INTEGER * );
__declspec(dllimport) int __stdcall QueryPerformanceFrequency( LARGE_INTEGER * );
__declspec(dllimport) void __stdcall Sleep( unsigned long  );
__declspec(dllimport) int __stdcall GetLogicalDrives();
__declspec(dllimport) unsigned int __stdcall GetWindowsDirectoryA( char *, unsigned int  );
#define GetWindowsDirectory GetWindowsDirectoryA
__declspec(dllimport) int __stdcall ShowCursor( int );
__declspec(dllimport) short __stdcall GetKeyState( int );
#define VK_SHIFT          0x10
#define VK_CONTROL        0x11
#define VK_MENU           0x12
__declspec(dllimport) void __stdcall OutputDebugStringA( const char * );
#define OutputDebugString OutputDebugStringA
struct JOYINFO { unsigned intwXpos, intwYpos, intwZpos, intwButtons; };
struct JOYINFOEX { unsigned long dwSize, dwFlags, dwXpos, dwYpos, dwZpos, dwRpos, dwUpos, dwVpos, dwButtons, dwButtonNumber, dwPOV, dwReserved1, dwReserved2;  };
typedef unsigned int MMRESULT;   
__declspec(dllimport) MMRESULT __stdcall joyGetNumDevs(void);
__declspec(dllimport) MMRESULT __stdcall joyGetPos(unsigned int , JOYINFO *);
__declspec(dllimport) MMRESULT __stdcall joyGetPosEx(unsigned int , JOYINFOEX *);
#define JOYSTICKID1 0
#define JOYSTICKID2 1
#define JOYERR_NOERROR (0)
#define JOY_RETURNX		0x00000001l
#define JOY_RETURNY		0x00000002l
#define JOY_RETURNZ		0x00000004l
#define JOY_RETURNR		0x00000008l
#define JOY_RETURNU		0x00000010l
#define JOY_RETURNV		0x00000020l
#define JOY_RETURNPOV		0x00000040l
#define JOY_RETURNBUTTONS	0x00000080l
#define JOY_RETURNCENTERED	0x00000400l
#define JOY_RETURNALL		(JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ | JOY_RETURNR | JOY_RETURNU | JOY_RETURNV | JOY_RETURNPOV | JOY_RETURNBUTTONS)
#define INVALID_HANDLE_VALUE (void *)-1
struct FILETIME { unsigned long dwLowDateTime, dwHighDateTime; };
#pragma pack(push,8)
struct WIN32_FIND_DATA { unsigned long dwFileAttributes; FILETIME ftCreationTime; FILETIME ftLastAccessTime; FILETIME ftLastWriteTime; unsigned long nFileSizeHigh; unsigned long nFileSizeLow; unsigned long dwReserved0; unsigned long dwReserved1; char cFileName[ 260 ]; char cAlternateFileName[ 14 ]; };
#pragma pack(pop)
__declspec(dllimport) int __stdcall FindClose( void *lpFileName );
__declspec(dllimport) void *__stdcall FindFirstFileA( const char *, WIN32_FIND_DATA * );
#define FindFirstFile FindFirstFileA
__declspec(dllimport) int __stdcall FindNextFileA( void *, WIN32_FIND_DATA * );
#define FindNextFile FindNextFileA
typedef void *HWND;
typedef void *HDC;
typedef void *HRGN;
typedef void *HBRUSH;
typedef void *HINSTANCE;
typedef void *HICON;
typedef void *HCURSOR;
typedef void *HMENU;
__declspec(dllimport) int __stdcall ShowWindow( HWND , int  );
typedef long (__stdcall* WNDPROC)(HWND, unsigned int, unsigned int, long);
#define SW_SHOW 5
#define SW_HIDE 0
__declspec(dllimport) int __stdcall GetSystemMetrics( int  );
#define SM_CXSIZEFRAME 32
#define SM_CYSIZEFRAME 33
#define SM_CYCAPTION 4
#define WM_PAINT 0x000F
#define WM_NCPAINT 0x0085
#define WM_DESTROY 0x0002
#define WM_CLOSE 0x0010

__declspec(dllimport) HANDLE __stdcall GetModuleHandleA( char * );
#define GetModuleHandle          GetModuleHandleA

__declspec(dllimport) unsigned long __stdcall GetModuleFileNameA( HANDLE, char *, long  );
#define GetModuleFileName          GetModuleFileNameA


__declspec(dllimport) int __stdcall MoveWindow( HWND , int , int , int , int , int  );
__declspec(dllimport) HDC __stdcall GetDC( HWND );
__declspec(dllimport) HDC __stdcall GetDCEx( HWND , HRGN , unsigned int );
#define DCX_WINDOW           0x00000001L
#define DCX_INTERSECTRGN     0x00000080L
typedef struct tagRECT { long left, top, right, bottom; } RECT;
__declspec(dllimport) int __stdcall FillRect( HDC hDC, const RECT *, HBRUSH  );
__declspec(dllimport) void * __stdcall GetStockObject(int);
#define BLACK_BRUSH 4
__declspec(dllimport) int __stdcall ReleaseDC( HWND , HDC  );
#define WM_MOVE 0x0003
#define WM_SIZE 0x0005
__declspec(dllimport) int __stdcall GetWindowRect( HWND , RECT *);
__declspec(dllimport) long __stdcall CallWindowProcA( WNDPROC , HWND , unsigned int , unsigned int , long  );
#define CallWindowProc CallWindowProcA
__declspec(dllimport) int __stdcall InvalidateRect( HWND , const RECT *, int );
struct SYSTEMTIME { unsigned short wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds; };
__declspec(dllimport) void __stdcall GetSystemTime( SYSTEMTIME *);
__declspec(dllimport) void __stdcall PostQuitMessage( int );
__declspec(dllimport) int __stdcall ShowCursor( int );
__declspec(dllimport) HWND __stdcall SetFocus( HWND);
__declspec(dllimport) char *__stdcall GetCommandLineA( void );
#define GetCommandLine GetCommandLineA
__declspec(dllimport) long __stdcall GetWindowLongA( HWND, int );
#define GetWindowLong GetWindowLongA
__declspec(dllimport) long __stdcall SetWindowLongA( HWND, int, long );
#define SetWindowLong SetWindowLongA
#define GWL_WNDPROC         (-4)
#define WINGDIAPI __declspec(dllimport)
#define APIENTRY __stdcall
#ifndef __USE_GNU
#ifndef _VC80_UPGRADE
typedef unsigned short wchar_t;
#endif
#endif
#define CALLBACK __stdcall
__declspec(dllimport) long __stdcall DefWindowProcA(HWND, unsigned int, unsigned int, long );
#define DefWindowProc DefWindowProcA
struct WNDCLASSEX {unsigned int cbSize; unsigned int style; WNDPROC lpfnWndProc; int cbClsExtra; int cbWndExtra; HINSTANCE hInstance; HICON hIcon; HCURSOR hCursor; HBRUSH hbrBackground; const char *lpszMenuName; const char *lpszClassName; HICON hIconSm; };
#define CS_HREDRAW 2
#define CS_VREDRAW 1
__declspec(dllimport) unsigned short __stdcall RegisterClassExA(const WNDCLASSEX *);
#define RegisterClassEx RegisterClassExA
__declspec(dllimport) HWND __stdcall CreateWindowExA(unsigned long,const char *,const char *,unsigned long,int,int,int,int,HWND,HMENU,HINSTANCE,void *);
#define CreateWindowEx CreateWindowExA
#define WS_POPUP            0x80000000L
__declspec(dllimport) void __stdcall SwitchToThread();
__declspec(dllimport) int  __stdcall MessageBoxA( HWND, const char *lpText, const char *lpCaption, unsigned int uType);
#define MessageBox MessageBoxA
#define MB_ICONEXCLAMATION          0x00000030L
#define MB_APPLMODAL                0x00000000L
#define WS_OVERLAPPED       0x00000000L
#define WS_POPUP            0x80000000L
#define WS_CHILD            0x40000000L
#define WS_MINIMIZE         0x20000000L
#define WS_VISIBLE          0x10000000L
#define WS_DISABLED         0x08000000L
#define WS_CLIPSIBLINGS     0x04000000L
#define WS_CLIPCHILDREN     0x02000000L
#define WS_MAXIMIZE         0x01000000L
#define WS_CAPTION          0x00C00000L     /* WS_BORDER | WS_DLGFRAME  */
#define WS_BORDER           0x00800000L
#define WS_DLGFRAME         0x00400000L
#define WS_VSCROLL          0x00200000L
#define WS_HSCROLL          0x00100000L
#define WS_SYSMENU          0x00080000L
#define WS_THICKFRAME       0x00040000L
#define WS_GROUP            0x00020000L
#define WS_TABSTOP          0x00010000L
#define WS_MINIMIZEBOX      0x00020000L
#define WS_MAXIMIZEBOX      0x00010000L
#define WS_TILED            WS_OVERLAPPED
#define WS_ICONIC           WS_MINIMIZE
#define WS_SIZEBOX          WS_THICKFRAME
#define WS_TILEDWINDOW      WS_OVERLAPPEDWINDOW
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX)
#define WS_POPUPWINDOW      (WS_POPUP|WS_BORDER| WS_SYSMENU)
#define WS_CHILDWINDOW      (WS_CHILD)
#define GWL_WNDPROC         (-4)
#define GWL_HINSTANCE       (-6)
#define GWL_HWNDPARENT      (-8)
#define GWL_STYLE           (-16)
#define GWL_EXSTYLE         (-20)
#define GWL_USERDATA        (-21)
#define GWL_ID              (-12)
__declspec(dllimport) void __stdcall SetParent( HWND chlid, HWND parent );
__declspec(dllimport) void __stdcall BringWindowToTop(HWND);
typedef char *LPSTR;
typedef const char *LPCSTR;
typedef int (__stdcall  *PROC)();
PROC  __stdcall wglGetProcAddress(LPCSTR);
__declspec(dllimport) void __stdcall timeBeginPeriod(unsigned int period);
}
#endif//_WINDOWS_
#endif
