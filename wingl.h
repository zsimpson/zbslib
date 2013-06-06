#ifndef _WINDOWS_
#ifdef WIN32
	#define WINGDIAPI __declspec(dllimport)
	#define APIENTRY __stdcall
	#ifndef _VC80_UPGRADE
		typedef unsigned short wchar_t;
	#endif
	#define CALLBACK __stdcall
	typedef void *HDC;
	typedef void *HGLRC;
	typedef unsigned long COLORREF;
	typedef int (__stdcall *PROC)();
#endif
#ifndef min
	#define min(a,b) ( ( (a) < (b) ) ? (a) : (b) )
#endif
#ifndef max
	#define max(a,b) ( ( (a) > (b) ) ? (a) : (b) )
#endif
#endif //_WINDOWS_

