// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A wrapper around a lot of simple, useful windows functions.
//			Elminiates the need to include windows.h for many simple tasks
//		}
//		*WIN32_LIBS_DEBUG ole32.lib
//		*PORTABILITY win32
//		*REQUIRED_FILES zwinutil.cpp zwinutil.h
//		*VERSION 1.1
//		+HISTORY {
//			1.1 Changed naming convention to standard
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }

// OPERATING SYSTEM specific includes:
//#include "windows.h"
//typedef _W64 long SHANDLE_PTR;
#ifdef WIN32
#include "shlobj.h"
#endif
// STDLIB includes:
#include "assert.h"
#include "stdlib.h"
#ifdef WIN32
#include "malloc.h"
#endif
// MODULE includes:
// ZBSLIB includes:

void zWinUtilMessageBox( char *buffer, char *title ) {
	#ifdef WIN32
	if( !title ) {
		title = "Warning";
	}
	MessageBox( NULL, buffer, title, MB_OK|MB_ICONEXCLAMATION );
	#else
	#endif
}

int zWinUtilYesNoBox( char *buffer ) {
	#ifdef WIN32
	return( MessageBox( NULL, buffer, "Warning", MB_YESNO ) == IDYES );
	#else
	return 0;
	#endif
}

int zWinUtilCreateShortcut( char *lpszPathObj, char *lpszPathLink, char *lpszDesc, char *workDir, char *args ) {
	#ifdef WIN32
    HRESULT hres; 
    IShellLink* psl; 
 
    // Get a pointer to the IShellLink interface.
    

    if(!SUCCEEDED(CoInitialize(NULL)))
    {
		return 0;
	}
        
	hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void **)&psl); 
    
	if (SUCCEEDED(hres))
	{ 
        IPersistFile* ppf; 
 
        // Set the path to the shortcut target, and add the 
        // description.

        psl->SetPath(lpszPathObj); 
        psl->SetDescription(lpszDesc); 
		psl->SetWorkingDirectory(workDir);
		psl->SetShowCmd(SW_SHOW);
		if (args != NULL)
		{
			psl->SetArguments (args);
		}
 
		// Query IShellLink for the IPersistFile interface for saving the 
		// shortcut in persistent storage. 

        hres = psl->QueryInterface(IID_IPersistFile, (void **)&ppf); 
 
        if (SUCCEEDED(hres))
		{ 
            WORD wsz[_MAX_PATH]; 
 
            // Ensure that the string is ANSI. 
            MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1, (LPWSTR)wsz, MAX_PATH); 
 
            // Save the link by calling IPersistFile::Save. 
            hres = ppf->Save( (LPCOLESTR)wsz, TRUE); 
            ppf->Release(); 
        } 
        psl->Release(); 

	    CoUninitialize();
    } 
    return SUCCEEDED(hres);
	#else
	return 0;
	#endif
} 

void *zWinUtilMemoryMapFileAsReadonly( char *filename ) {
	#ifdef WIN32
	SECURITY_ATTRIBUTES sa;
	memset (&sa, 0, sizeof(SECURITY_ATTRIBUTES));
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);

	HANDLE fh = CreateFile (
		filename, 
		GENERIC_READ, 
		FILE_SHARE_READ,
		&sa,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	assert (fh != INVALID_HANDLE_VALUE);

	HANDLE fm = CreateFileMapping (fh,NULL,PAGE_READONLY,0,0,NULL);
	assert (fm != NULL);	// I just love how consistent Windows error handling is!

	void *p = MapViewOfFile (fm, FILE_MAP_READ, 0, 0, 0);
	assert (p);

	return p;
	#else
	return 0;
	#endif
}

void zWinUtilUnmapFile( void *p ) {
	#ifdef WIN32
	UnmapViewOfFile( p );
	#endif
}


//Depends upon a dll I don't want to link
int zwinUtilLaunchURL(char *url) {
	#ifdef WIN32
	assert (url != NULL);

	HINSTANCE hApp = ShellExecute (GetDesktopWindow(), "open", url, NULL, NULL, SW_SHOW);

	if (hApp <= (HINSTANCE) 32) {
		// An error occurred.
		return FALSE;
	}

	// All is well, so return.
	return TRUE;
	#else
	return 0;
	#endif
}


int zWinUtilRebootSystem() {
	#ifdef WIN32
	char buf[_MAX_PATH];
	char buf2[_MAX_PATH];
	char winDir [_MAX_PATH];

	GetWindowsDirectory(winDir, 256);

	// Magic Craig Eisler Code!
	strcpy (buf2, winDir);
	strcat (buf2, "\\~~reboot.ini");
	WritePrivateProfileString("Reboot", "Reboot", "0", buf2);

	strcpy (buf, winDir);
	strcat (buf, "\\WININIT.INI");
	WritePrivateProfileString("Rename", "NUL", buf2, buf);

	return ExitWindowsEx( EWX_REBOOT, 0 );
	#else
	return 0;
	#endif
}

int zWinUtilFindWindowFromProcessId( unsigned long processId ) {
	#ifdef WIN32
	HWND hwnd = FindWindow( 0, 0 );
	while( hwnd ) {
		if( GetParent( hwnd ) == 0 ) {
			unsigned long thisProcId;
			GetWindowThreadProcessId( hwnd, &thisProcId );
			if( thisProcId == processId ) {
				return (int)hwnd;
			}
		}

		hwnd = GetWindow( hwnd, GW_HWNDNEXT );
	}
	#endif
	return 0;
}

void zWinUtilStartProcess( char *name, char *commandLine, int waitTillComplete ) {
	#ifdef WIN32
	STARTUPINFO s;
	memset (&s, 0, sizeof(STARTUPINFO));
	s.cb = sizeof(STARTUPINFO);

	PROCESS_INFORMATION pinfo;

	if( !name ) {
		name = "";
	}
	char *buffer = new char[strlen (name) + strlen (commandLine) + 2];
	assert (buffer != NULL);

	buffer[0] = '\0';
	if( name && *name ) {
		strcat (buffer, name);
		strcat (buffer, " ");
	}
	strcat (buffer, commandLine);

	int success = CreateProcess
	(
//		name, commandLine, NULL, NULL, FALSE, 0, NULL, NULL, &s, &pinfo
		NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &s, &pinfo
	);

	if( success ) {
		// FORCE to front.  I put this in because sometimes there's a weird issue
		// that's hard to reproduce where the child window ends up behind the parent
		HWND hwnd = (HWND)zWinUtilFindWindowFromProcessId( pinfo.dwProcessId );
		SetWindowPos( hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOREPOSITION|SWP_NOMOVE|SWP_SHOWWINDOW );
	}

	if (success && waitTillComplete)
	{
		// Wait for the process to complete its initialization, with a timeout of 5 seconds
		// If it times out, don't bother waiting for the process to complete.

		HANDLE hProcess = pinfo.hProcess;
		if (WaitForInputIdle (hProcess, 5000) == 0)
		{
			// Didn't timeout or have an error, so spin here until the process exits
			// The loop below will exit if there is an error in the function call or
			// the process is terminated by any means.

			WaitForSingleObject( hProcess, INFINITE );
/*			
			DWORD exitCode;
			while (GetExitCodeProcess (hProcess, &exitCode))
			{
				if (exitCode != STILL_ACTIVE)
				{
					break;
				}
			}
*/
		}
	}
	#endif
}

int zWinUtilStartThread( void *startRoutine, int argument ) {
	#ifdef WIN32
	DWORD thread;
	HANDLE result = CreateThread
	(
		NULL,
		1024*16,
		(LPTHREAD_START_ROUTINE)startRoutine,
		(LPVOID)argument,
		0,
		&thread
	);
	return (result != NULL);
	#else
	return 0;
	#endif
}

void zWinUtilGetTextFromClipboard( char *buffer, int size ) {
	#ifdef WIN32
	*buffer = 0;

	if (!IsClipboardFormatAvailable(CF_TEXT)) {
		return;
	}

	if (!OpenClipboard (NULL)) {
		return;
	}

	HANDLE hglb = GetClipboardData (CF_TEXT); 
	if (hglb != NULL) { 
		LPVOID lptstr = GlobalLock (hglb);
		if (lptstr != NULL) { 
			strncpy (buffer, (char*)lptstr, size);
			GlobalUnlock (hglb);
		} 
	} 
	CloseClipboard(); 
	#endif
}

void zWinUtilCopyTextToClipboard( char *buffer, int size ) {
	#ifdef WIN32

	// Copy the selected text from the buffer into the Windows clipboard
	// Open the clipboard, and empty it.

	if (!OpenClipboard(NULL)) {
		return; 
	}
	EmptyClipboard(); 

	// Allocate a global memory object for the text.
	HGLOBAL hglbCopy = GlobalAlloc(GMEM_DDESHARE, (size + 1) * sizeof(TCHAR)); 
	if (hglbCopy == NULL) { 
		CloseClipboard(); 
		return; 
	} 

	// Lock the handle and copy the text to the buffer.
	char *lptstrCopy = (char *) GlobalLock (hglbCopy); 
	memcpy(lptstrCopy, buffer, size * sizeof(char)); 
	lptstrCopy[size] = (char) 0;    // null terminator
	GlobalUnlock (hglbCopy); 

	// Place the handle on the clipboard.
	SetClipboardData (CF_TEXT, hglbCopy); 

	// Close the clipboard.
	CloseClipboard(); 
	#endif
}

// This is a local function just for internal use
#ifdef WIN32
static int zWinUtilParseRegKey (char *regPath, HKEY &rootKey, char *subKeyName, char *valueName) {
	rootKey = 0;

	char *temp = (char *)alloca( strlen( regPath )+1 );
	strcpy( temp, regPath );

	char *firstSlash = NULL;
	char *lastSlash = NULL;

	for( char *i=temp; *i; i++ ) {
		if( *i == '\\' || *i == '/' ) {
			if( !firstSlash ) firstSlash = i;
			lastSlash = i;
		}
	}

	if( lastSlash ) {
		*lastSlash = 0;
		strcpy( valueName, lastSlash+1 );
	}
	if( firstSlash ) {
		*firstSlash = 0;
		for( char *i=firstSlash+1; *i; i++ ) {
			if( *i == '/' ) *i = '\\';
		}
		strcpy( subKeyName, firstSlash+1 );
	}


	if( !stricmp( temp, "HKEY_CLASSES_ROOT" ) ) rootKey = HKEY_CLASSES_ROOT;
	if( !stricmp( temp, "HKEY_CURRENT_USER" ) ) rootKey = HKEY_CURRENT_USER;
	if( !stricmp( temp, "HKEY_LOCAL_MACHINE" ) ) rootKey = HKEY_LOCAL_MACHINE;
	if( !stricmp( temp, "HKEY_USERS" ) ) rootKey = HKEY_USERS;
	if( !stricmp( temp, "HKEY_CURRENT_CONFIG" ) ) rootKey = HKEY_CURRENT_CONFIG;
	if( !stricmp( temp, "HKEY_DYN_DATA" ) ) rootKey = HKEY_DYN_DATA;

	if( !stricmp( temp, "HKCR" ) ) rootKey = HKEY_CLASSES_ROOT;
	if( !stricmp( temp, "HKCU" ) ) rootKey = HKEY_CURRENT_USER;
	if( !stricmp( temp, "HKLM" ) ) rootKey = HKEY_LOCAL_MACHINE;
	if( !stricmp( temp, "HKU" ) ) rootKey = HKEY_USERS;
	if( !stricmp( temp, "HKCC" ) ) rootKey = HKEY_CURRENT_CONFIG;
	if( !stricmp( temp, "HKDD" ) ) rootKey = HKEY_DYN_DATA;

	return firstSlash && lastSlash && rootKey;
}
#endif

int zWinUtilCreateRegKey( char *regPath ) {
	#ifdef WIN32
	// Parse out the root key, subkey, and value name
	assert( regPath );

	char copy[1024];
	strcpy( copy, regPath );

	char subKeyName[256];
	char valueName[256];
	HKEY rootKey;

	if (!zWinUtilParseRegKey (copy, rootKey, subKeyName, valueName)) {
		return 0;
	}

	// Create the key.  The valueName, if given, is ignored.

	HKEY hKey;
	DWORD dwDisposition;

	LONG result = RegCreateKeyEx (
		rootKey,
		subKeyName,
		0,
		"",
		REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,
		NULL,
		&hKey,
		&dwDisposition
	);

	if (result == ERROR_SUCCESS) {
		RegCloseKey (hKey);
		return 1;
	}
	else {
		return 0;
	}
	#else
	return 0;
	#endif
}

int zWinUtilDeleteRegKey( char *regPath ) {
	#ifdef WIN32
	// NOTE: This will only work properly, i.e. delete the given key and all subkeys, under
	// Win95.  It will need to be rewritten to work under NT.

	// Parse out the root key, subkey, and value name
	assert( regPath );

	char copy[1024];
	strcpy( copy, regPath );

	char subKeyName[256];
	char valueName[256];
	HKEY rootKey;

	if( !zWinUtilParseRegKey (copy, rootKey, subKeyName, valueName) ) {
		return 0;
	}

	// Delete the given key.

	LONG result = RegDeleteKey (rootKey, subKeyName);
	return (result == ERROR_SUCCESS);
	#else
	return 0;
	#endif
}

int zWinUtilSetRegString( char *regPath, char *value ) {
	#ifdef WIN32
	// Parse out the root key, subkey, and value name
	char copy[1024];
	strcpy( copy, regPath );

	char subKeyName[256];
	char valueName[256];
	HKEY rootKey;

	if( !zWinUtilParseRegKey (copy, rootKey, subKeyName, valueName) ) {
		return 0;
	}

	// Attempt to open the given key.  If it cannot be openned, fail.

	HKEY hKey;
	DWORD dwDisposition;
	char lpClass[256];

	LONG result = RegCreateKeyEx (
		rootKey,
		subKeyName,
		0,
		lpClass,
		REG_OPTION_NON_VOLATILE,
		KEY_WRITE,
		NULL,
		&hKey,
		&dwDisposition
	);

	if (result != ERROR_SUCCESS) {
		return 0;
	}

	// Set the value for the given name as a string, even if the name is NULL.

	result = RegSetValueEx (
		hKey,
		valueName,
		0,
		REG_SZ,
		(CONST BYTE *) value,
		strlen (value) + 1
	);

	RegCloseKey (hKey);
	return (result == ERROR_SUCCESS);
	#else
	return 0;
	#endif
}

int zWinUtilSetRegValue( char *regPath, void *value, int length ) {
	#ifdef WIN32
	// Parse out the root key, subkey, and value name
	assert( regPath );

	char copy[1024];
	strcpy( copy, regPath );

	char subKeyName[256];
	char valueName[256];
	HKEY rootKey;

	if( !zWinUtilParseRegKey (copy, rootKey, subKeyName, valueName) ) {
		return 0;
	}

	// Must have a value name.

	if (valueName == NULL) {
		return 0;
	}

	// Attempt to open the given key.  If it cannot be openned, fail.

	HKEY hKey;
	DWORD dwDisposition;
	char lpClass[256];

	LONG result = RegCreateKeyEx (
		rootKey,
		subKeyName,
		0,
		lpClass,
		REG_OPTION_NON_VOLATILE,
		KEY_WRITE,
		NULL,
		&hKey,
		&dwDisposition
	);

	if (result != ERROR_SUCCESS) {
		return 0;
	}

	// Set the value for the given name as a binary value

	result = RegSetValueEx (
		hKey,
		valueName,
		0,
		REG_BINARY,
		(CONST BYTE *) value,
		length
	);

	RegCloseKey (hKey);
	return (result == ERROR_SUCCESS);
	#else
	return 0;
	#endif
}

int zWinUtilSetRegDWORD( char *regPath, unsigned int val ) {
	#ifdef WIN32
	// Parse out the root key, subkey, and value name
	assert( regPath );

	char copy[1024];
	strcpy( copy, regPath );

	char subKeyName[256];
	char valueName[256];
	HKEY rootKey;

	if( !zWinUtilParseRegKey (copy, rootKey, subKeyName, valueName) ) {
		return 0;
	}

	// Must have a value name.

	if (valueName == NULL) {
		return 0;
	}

	// Attempt to open the given key.  If it cannot be openned, fail.

	HKEY hKey;
	DWORD dwDisposition;
	char lpClass[256];

	LONG result = RegCreateKeyEx (
		rootKey,
		subKeyName,
		0,
		lpClass,
		REG_OPTION_NON_VOLATILE,
		KEY_WRITE,
		NULL,
		&hKey,
		&dwDisposition
	);

	if (result != ERROR_SUCCESS) {
		return 0;
	}

	// Set the value for the given name as a binary value

	result = RegSetValueEx (
		hKey,
		valueName,
		0,
		REG_DWORD,
		(CONST BYTE *) &val,
		sizeof(val)
	);

	RegCloseKey (hKey);
	return (result == ERROR_SUCCESS);
	#else
	return 0;
	#endif
}

int zWinUtilGetRegString( char *regPath, char *value, int max_length ) {
	#ifdef WIN32
	// Parse out the root key, subkey, and value name
	assert( regPath );

	char copy[1024];
	strcpy( copy, regPath );

	char subKeyName[256];
	char valueName[256];
	HKEY rootKey;

	if (!zWinUtilParseRegKey (copy, rootKey, subKeyName, valueName)) {
		return 0;
	}

	// Attempt to open the given key.  If it cannot be openned, fail.

	HKEY hKey;
	//DWORD dwDisposition;

	LONG result = RegOpenKeyEx (
		rootKey,
		subKeyName,
		0,
		KEY_READ,
		&hKey
	);

	if (result != ERROR_SUCCESS) {
		return 0;
	}

	// Attempt to read a string value from the given name, even if the name is NULL.

	DWORD type;
	DWORD length = max_length;
	result = RegQueryValueEx (
		hKey,
		valueName,
		0,
		&type,
		(BYTE *) value,
		&length
	);

	RegCloseKey (hKey);
	if (result == ERROR_SUCCESS && type == REG_SZ) {
		return length;
	}
	else {
		return 0;
	}
	#else
	return 0;
	#endif
}

int zWinUtilGetRegValue( char *regPath, void *value, int max_length ) {
	#ifdef WIN32
	// Parse out the root key, subkey, and value name
	assert( regPath );

	char copy[1024];
	strcpy( copy, regPath );

	char subKeyName[256];
	char valueName[256];
	HKEY rootKey;

	if (!zWinUtilParseRegKey (copy, rootKey, subKeyName, valueName)) {
		return 0;
	}

	// Must have a value name.

	if (valueName == NULL) {
		return 0;
	}

	// Attempt to open the given key.  If it cannot be openned, fail.

	HKEY hKey;
	//DWORD dwDisposition;

	LONG result = RegOpenKeyEx (
		rootKey,
		subKeyName,
		0,
		KEY_ALL_ACCESS,
		&hKey
	);

	if (result != ERROR_SUCCESS) {
		return 0;
	}

	// Attempt to read a string value from the given name, even if the name is NULL.

	DWORD type;
	DWORD length = max_length;
	result = RegQueryValueEx (
		hKey,
		valueName,
		0,
		&type,
		(BYTE *) value,
		&length
	);

	RegCloseKey (hKey);
	if (result == ERROR_SUCCESS && type == REG_BINARY) {
		return length;
	}
	else {
		return 0;
	}
	#else
	return 0;
	#endif
}

int zWinUtilRegIsEmpty (char *regPath) {
	#ifdef WIN32
	// Parse out the root key, subkey, and value name
	assert( regPath );

	char copy[1024];
	strcpy( copy, regPath );

	char subKeyName[256];
	char valueName[256];
	HKEY rootKey;

	if (!zWinUtilParseRegKey (copy, rootKey, subKeyName, valueName)) {
		return 0;
	}

	// Attempt to open the given key.  If it cannot be openned, fail.

	HKEY hKey;

	LONG result = RegOpenKeyEx (
		rootKey,
		subKeyName,
		0,
		KEY_ALL_ACCESS,
		&hKey
	);

	if (result != ERROR_SUCCESS) {
		return 0;
	}

	// Get information about the given key.

	char classBuf[128];
	DWORD cbClass = 127;
	DWORD cSubKeys;
	DWORD cbMaxSubKeyLen;
	DWORD cbMaxClassLen;
	DWORD cValues;
	DWORD cbMaxValueNameLen;
	DWORD cbMaxValueLen;
	DWORD cbSecurityDescriptor;
	FILETIME ftLastWriteTime;

	result = RegQueryInfoKey (
		hKey,	                // handle of key to query 
		classBuf,	            // address of buffer for class string 
		&cbClass,	            // address of size of class string buffer 
		NULL,	                // reserved 
		&cSubKeys,	            // address of buffer for number of subkeys 
		&cbMaxSubKeyLen,	    // address of buffer for longest subkey name length  
		&cbMaxClassLen,	        // address of buffer for longest class string length 
		&cValues,	            // address of buffer for number of value entries 
		&cbMaxValueNameLen,	    // address of buffer for longest value name length 
		&cbMaxValueLen,	        // address of buffer for longest value data length 
		&cbSecurityDescriptor,	// address of buffer for security descriptor length 
		&ftLastWriteTime 	    // address of buffer for last write time 
	);	

	RegCloseKey (hKey);
	if (result == ERROR_SUCCESS) {
		return (cSubKeys == 0 && cValues == 0);
	}
	else {
		return 0;
	}
	#else
	return 0;
	#endif
}

int zWinUtilGetWindowsDirectory (char *buffer, unsigned int size) {
	#ifdef WIN32
	return GetWindowsDirectory (buffer, size);
	#else
	return 0;
	#endif
}

int zWinUtilGetCommonProgramsFolderPath( char *buffer ) {
	#ifdef WIN32
	int ret = SHGetSpecialFolderPath( NULL, buffer, CSIDL_COMMON_PROGRAMS, 0 );
	return SUCCEEDED(ret);
	#else
	return 0;
	#endif
}

int zWinUtilGetStartupFolderPath( char *buffer ) {
	#ifdef WIN32
	int ret = SHGetSpecialFolderPath( NULL, buffer, CSIDL_STARTUP, 0 );
	return SUCCEEDED(ret);
	#else
	return 0;
	#endif
}

void zWinUtilGetScreenMetrics( int &w, int &h, int &d ) {
	#ifdef WIN32
    w = GetSystemMetrics(SM_CXSCREEN);
    h = GetSystemMetrics(SM_CYSCREEN);

	DEVMODE devm;
	EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &devm );
	d = devm.dmBitsPerPel / 8;
	#endif
}

#ifdef WIN32
static int zWinUtilSpoofing = 0;
static HHOOK zWinUtilSpoofHHook = 0;
static int zWinUtilSpoofQueueCount = 0;
struct ZWinUtilSpoofEvent {
	int whichEvent;
	int x;
	int y;
};
//static ZWinUtilSpoofEvent zWinUtilSpoofQueue[10];
static ZWinUtilSpoofEvent zWinUtilSpoofQueue[100];
	// There is some inexplicable limit at about 10 here.  After that and the events seem to disappear.  

LRESULT CALLBACK zWinUtilSpoofCallbackFunc(int nCode, WPARAM wParam, LPARAM lParam ) {
	// This is a callback called by windows journaling functions during the
	// fake "playback" operation used to spoof messages.

	static int modal = 0;
		// According to the windows docs, this func is required to stop
		// pushing messages if a sytem modal dialog box comes up

	if( nCode == HC_SYSMODALON ) {
		modal = 1;
	}
	else if( nCode == HC_SYSMODALOFF ) {
		modal = 0;
	}
	else if( nCode >= 0 && !modal ) {
		if(nCode == HC_SKIP) {
			return 0;
		}

		else if(nCode == HC_GETNEXT) {
			if( zWinUtilSpoofQueueCount > 0 ) {
				ZWinUtilSpoofEvent *e = &zWinUtilSpoofQueue[0];

				static int conversionTable[5] = { WM_LBUTTONDOWN, WM_LBUTTONUP, WM_RBUTTONDOWN, WM_RBUTTONUP, WM_MOUSEMOVE };
				assert( e->whichEvent >= 0 && e->whichEvent < 5 );

				LPEVENTMSG lpEvent = (LPEVENTMSG)lParam;
				lpEvent->message = conversionTable[e->whichEvent];
				lpEvent->paramL  = e->x;
				lpEvent->paramH  = e->y;
				lpEvent->time    = GetTickCount()+1;
				lpEvent->hwnd    = 0;

				zWinUtilSpoofQueueCount--;
				memmove( &zWinUtilSpoofQueue[0], &zWinUtilSpoofQueue[1], zWinUtilSpoofQueueCount * sizeof(zWinUtilSpoofQueue[0]) );
			}
			else {
				UnhookWindowsHookEx( zWinUtilSpoofHHook );
				zWinUtilSpoofing = 0;
			}

			return 0;
		}
	}

	return CallNextHookEx( zWinUtilSpoofHHook, nCode, wParam, lParam );
}
#endif

void zWinUtilSpoofMouseEvent( int whichEvent, int x, int y ) {
	#ifdef WIN32
	int size = sizeof(zWinUtilSpoofQueue)/sizeof(zWinUtilSpoofQueue[0]);
	if( zWinUtilSpoofQueueCount >= size ) {
		__asm int 3;
	}
	assert( zWinUtilSpoofQueueCount < size );

	zWinUtilSpoofQueue[zWinUtilSpoofQueueCount].whichEvent = whichEvent;
	zWinUtilSpoofQueue[zWinUtilSpoofQueueCount].x = x;
	zWinUtilSpoofQueue[zWinUtilSpoofQueueCount].y = y;

	zWinUtilSpoofQueueCount++;

	if( !zWinUtilSpoofing )	{
		HINSTANCE hInstance = GetModuleHandle(NULL);
		zWinUtilSpoofHHook = SetWindowsHookEx( WH_JOURNALPLAYBACK, zWinUtilSpoofCallbackFunc, hInstance, 0 );
		zWinUtilSpoofing = 1;
			// @TODO: The setting of this spoofing flag may need a mutex around it
	}
	#endif
}
