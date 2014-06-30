#include "assert.h"

#ifdef WIN32
	#include "windows.h"
#endif

#ifdef __APPLE__
	#include "stdio.h"
	#include "stdlib.h"
	#include "math.h"
	#include "Carbon/Carbon.h"
#endif


#ifdef __APPLE__
	extern "C" {
		void __assert_fail(const char *assertion, const char *file, unsigned int line, const char *function) {
			char buffer[256];
			sprintf( buffer, "assert failed: '%s' at %s:%d func='%s'\n", assertion, file, line, function );
			FILE *f = fopen( "assert.txt", "wt" );
			if( f ) {
				fprintf( f, "%s", buffer );
				fclose( f );
			}
			fprintf( stderr, "%s", buffer );
       		fflush( stderr );
       		exit(1);
		}
	}
#endif


void zplatformCopyToClipboard( char *buffer, int size ) {
	//---WIN32-----------------------------------------
	#ifdef WIN32
		// COPY the selected text from the buffer into the Windows clipboard
		if( !OpenClipboard(NULL) ) {
			return; 
		}
		EmptyClipboard(); 

		// ALLOCATE a global memory object for the text.
		HGLOBAL hglbCopy = GlobalAlloc( GMEM_DDESHARE, (size + 1) * sizeof(TCHAR) );
		if( hglbCopy == NULL ) { 
			CloseClipboard(); 
			return; 
		} 

		// LOCK the handle and copy the text to the buffer.
		char *lptstrCopy = (char *)GlobalLock(hglbCopy); 
		memcpy( lptstrCopy, buffer, size * sizeof(char) );
		lptstrCopy[size] = (char)0;
		GlobalUnlock (hglbCopy); 

		// PLACE the handle on the clipboard.
		SetClipboardData (CF_TEXT, hglbCopy); 

		// CLOSE the clipboard.
		CloseClipboard(); 
	#endif

	//---APPLE-----------------------------------------
	#ifdef __APPLE__
	OSStatus err = noErr;
	PasteboardRef theClipboard;
	CFDataRef textData = NULL;
	err = PasteboardCreate( kPasteboardClipboard, &theClipboard );
	err = PasteboardClear( theClipboard );
	textData = CFDataCreate( kCFAllocatorDefault, (UInt8*)buffer, size + 1 );
	err = PasteboardPutItemFlavor( theClipboard, (PasteboardItemID)1, CFSTR("public.utf8-plain-text"), textData, 0 );
	CFRelease (textData);
	#endif

	//---LINUX-----------------------------------------
	#ifdef __linux__
	#endif
}


void zplatformGetTextFromClipboard( char *buffer, int size ) {
	//---WIN32-----------------------------------------
	#ifdef WIN32
		*buffer = 0;

		if( !IsClipboardFormatAvailable(CF_TEXT) ) {
			return;
		}

		if( !OpenClipboard (NULL) ) {
			return;
		}

		HANDLE hglb = GetClipboardData( CF_TEXT );
		if( hglb != NULL ) { 
			LPVOID lptstr = GlobalLock (hglb);
			if( lptstr != NULL ) { 
				strncpy (buffer, (char*)lptstr, size);
				GlobalUnlock (hglb);
			} 
		} 
		CloseClipboard(); 
	#endif

	//---APPLE-----------------------------------------
	#ifdef __APPLE__
	// This is probably a LOT more complicated than it needs to be - it deals with the 
	// possibility that the item in the clipboard has other "flavors" than just text
	// and finds the text...  Taken from an apple example that was even more involved...
	OSStatus err = noErr;
	PasteboardRef theClipboard;
	err = PasteboardCreate( kPasteboardClipboard, &theClipboard );
	
	PasteboardItemID    itemID;
	CFArrayRef          flavorTypeArray;
	CFIndex             flavorCount;

	err = PasteboardGetItemIdentifier( theClipboard, 1, &itemID );
	err = PasteboardCopyItemFlavors( theClipboard, itemID, &flavorTypeArray );
	flavorCount = CFArrayGetCount( flavorTypeArray );// 5

	#define OSX_FLAVORTEXT_SIZE 16384
	static char flavorText[OSX_FLAVORTEXT_SIZE];
		// not threadsafe

	for( CFIndex flavorIndex = 0; flavorIndex < flavorCount; flavorIndex++ ) {
		CFStringRef             flavorType;
		CFDataRef               flavorData;
		CFIndex                 flavorDataSize;
		
		flavorType = (CFStringRef)CFArrayGetValueAtIndex( flavorTypeArray, flavorIndex );
		if (UTTypeConformsTo(flavorType, CFSTR("public.utf8-plain-text"))) {
			err = PasteboardCopyItemFlavorData( theClipboard, itemID, flavorType, &flavorData );
			flavorDataSize = CFDataGetLength( flavorData );
			flavorDataSize = (flavorDataSize<OSX_FLAVORTEXT_SIZE-2) ? flavorDataSize : OSX_FLAVORTEXT_SIZE-2;
			for( short dataIndex = 0; dataIndex <= flavorDataSize; dataIndex++ ) {
				char byte = *(CFDataGetBytePtr( flavorData ) + dataIndex);
				flavorText[dataIndex] = byte;
			}
			flavorText[flavorDataSize] = '\0';
			flavorText[flavorDataSize+1] = '\n';
			CFRelease (flavorData);
			strncpy( buffer, flavorText, size );
			break;
		}
	}
	CFRelease (flavorTypeArray);
	#endif

	//---LINUX-----------------------------------------
	#ifdef __linux__
	#endif
}

int zplatformGetNumProcessors() {
	//---WIN32-----------------------------------------
	#ifdef WIN32
		SYSTEM_INFO si;
		GetSystemInfo( &si );
		return si.dwNumberOfProcessors;
	#endif

	//---APPLE-----------------------------------------
	#ifdef __APPLE__
		// @TODO
		return 1;
	#endif

	//---LINUX-----------------------------------------
	#ifdef __linux__
		// @TODO
		return 1;
	#endif
}


unsigned int zPlatformSpawnProcess( char *name, char *commandLine ) {

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

		int success = CreateProcess( NULL, (LPSTR)buffer, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &s, &pinfo );

		return (unsigned int)pinfo.hProcess;
	#else
		// @TODO: tfb commented out this code as it fails to compile on both linux & osx.
		// At the very least a header declaring the spawn stuff is required.
		return 0;
		/*
		posix_spawnattr_t attr;
		pid_t pid;
		
		char *argv[512] = {
			name,
			0
		};
		int argc = 1;

		// EXTRACT from the command line to a list of argv		
		char *cmdLine = strdup( commandLine );
		char *lastStart = cmdLine;
		for( char *c=cmdLine; ; c++ ) {
			if( *c == ' ' || *c == 0 ) {
				*c = 0;
				argv[argc++] = lastStart;
				lastStart = c+1;
			}
			if( *c == 0 ) {
				break;
			}
		}
		argv[argc] = 0;
		
		char *env[1] = {
			0
		};
	
		int ret = posix_spawn( &pid,  name, 0, &attr, argv, env );

		free( cmdLine );
		
		return ret == 0 ? pid : 0;
		 */
	#endif
}

int zPlatformGetNumberProcessors() {
	#ifdef WIN32
		SYSTEM_INFO systemInfo;
		GetSystemInfo( &systemInfo );
		return systemInfo.dwNumberOfProcessors;
	#else
		// @TODO: I can't find a simple way to do this
		return 1;
	#endif
}

int zPlatformIsProcessRunning( int pidOrHandle ) {
	#ifdef WIN32
		int exitCode;
		int ok = GetExitCodeProcess( (HANDLE)pidOrHandle, (LPDWORD)&exitCode );
		int running = ok && exitCode == STILL_ACTIVE;
		return running;
	#else
		// @TODO: fix this; breaks linux builds (tfb commented out)
		return 0;
		/*
		int stat_loc = 0;		
		int ret = waitpid( pidOrHandle, &stat_loc, WNOHANG );
		return ret == 0 ? 1 : 0;
		*/
	#endif
}

int zPlatformKillProcess( int pidOrHandle ) {
	#ifdef WIN32
		int ok = TerminateProcess( (HANDLE)pidOrHandle, 0 );
		return ok;
	#else
		// @TODO:
		// tfb commented this out: someone forget a header?  its not obvious on osx or I'd add it.
		return 0;
		/*
		int err = kill( pidOrHandle, SIGKILL );
		int stat_loc;
		int ret = waitpid( pidOrHandle, &stat_loc, 0 );
		return err == 0 ? 1 : 0;
		*/
	#endif
}

void zPlatformNoErrorBox() {
	#ifdef WIN32
		SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX );
	#endif
}
