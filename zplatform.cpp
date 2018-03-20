#include "assert.h"
#include "stdio.h"

#ifdef WIN32
	#include "windows.h"
	#include "shlwapi.h"
#endif

#ifdef __APPLE__
	#include "stdlib.h"
	#include "math.h"
	#include "Carbon/Carbon.h"
	#include "sys/sysctl.h"
	#include "sys/utsname.h"
#endif


#ifdef __APPLE__
	extern "C" {
        // printStackTrace() copied from https://oroboro.com/stack-trace-on-crash/
	    #include <execinfo.h>    
        static inline void printStackTrace( FILE *out = stderr, unsigned int max_frames = 127 ) {
            fprintf(out, "stack trace:\n");
            
            // storage array for stack trace address data
            void* addrlist[max_frames+1];
            
            // retrieve current stack addresses
            unsigned int addrlen = backtrace( addrlist, sizeof( addrlist ) / sizeof( void* ));
            
            if ( addrlen == 0 ) {
                fprintf( out, "  \n" );
                return;
            }
            
            // create readable strings to each frame.
            char** symbollist = backtrace_symbols( addrlist, addrlen );
            
            // print the stack trace.
            for ( unsigned int i = 1; i < addrlen; i++ ) {
                fprintf( out, "%s\n", symbollist[i]);
            }
            
            free(symbollist);
        }
        
		void __assert_fail(const char *assertion, const char *file, unsigned int line, const char *function) {
			char buffer[256];
			sprintf( buffer, "assert failed: '%s' at %s:%d func='%s'\n", assertion, file, line, function );
			FILE *f = fopen( "assert.txt", "wt" );
			if( f ) {
				fprintf( f, "%s", buffer );
                fflush( f );
                printStackTrace( f );
				fclose( f );
			}
			fprintf( stderr, "%s", buffer );
            printStackTrace( stderr );
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
	textData = CFDataCreate( kCFAllocatorDefault, (UInt8*)buffer, size );
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
				int len = strlen(buffer);
				// some programs like PDF viewers will have \r\n for newlines,
				// and we only want the \n.
				for (int i = 0; i < len; i++) {
					if (buffer[i] == '\r') {
						buffer[i] = ' ';
					}
				}
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

		int isUTF8 = UTTypeConformsTo(flavorType, CFSTR("public.utf8-plain-text"));
		int isUTF16 = UTTypeConformsTo(flavorType, CFSTR("public.utf16-plain-text"));

		if ( isUTF8 || isUTF16 ) {
			err = PasteboardCopyItemFlavorData( theClipboard, itemID, flavorType, &flavorData );
			flavorDataSize = CFDataGetLength( flavorData );
			// flavorDataSize = (flavorDataSize<OSX_FLAVORTEXT_SIZE-2) ? flavorDataSize : OSX_FLAVORTEXT_SIZE-2;
			
			CFStringEncoding enc = isUTF8 ? kCFStringEncodingUTF8 : kCFStringEncodingUTF16;
			CFStringRef s = CFStringCreateWithBytes( NULL, CFDataGetBytePtr(flavorData), flavorDataSize, enc, false );
			CFMutableStringRef ms = CFStringCreateMutableCopy( NULL, size, s );
			CFRelease(flavorData);
			CFRelease( s );

			// Get rid of strange Unicode line-separator that can show up copying&pasting with MS Word.
			CFRange range = CFRangeMake( 0, CFStringGetLength(ms) );
			CFStringFindAndReplace( ms, CFSTR("\U00002028"), CFSTR(""), range, 0 );

			// get rid of \r in favor of \n
			range = CFRangeMake( 0, CFStringGetLength(ms) );
			CFStringFindAndReplace( ms, CFSTR("\r"), CFSTR("\n"), range, 0 );

			CFStringGetCString( ms, buffer, size, kCFStringEncodingUTF8 );
			CFRelease( ms );

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
		int ncpu;
		size_t bufsize = sizeof( ncpu );
	    sysctlbyname( "hw.physicalcpu",&ncpu,&bufsize,NULL,0);
		return ncpu;
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

void zPlatformGetMachineId( char *buffer, int size ) {
	// Adapted from https://oroboro.com/unique-machine-fingerprint/

	#ifdef WIN32
		// use serial number of os disk.
		DWORD volumeSerialNumber;
		GetVolumeInformationA(NULL, NULL, NULL, &volumeSerialNumber, NULL, NULL, NULL, NULL);
		sprintf( buffer, "%X", volumeSerialNumber);
	#endif

	#ifdef __APPLE__
		char tmp[256];
		io_registry_entry_t ioRegistryRoot = IORegistryEntryFromPath(kIOMasterPortDefault, "IOService:/");
	    CFStringRef uuidCf = (CFStringRef) IORegistryEntryCreateCFProperty(ioRegistryRoot, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0);
	    IOObjectRelease(ioRegistryRoot);
	    CFStringGetCString(uuidCf, tmp, 256, kCFStringEncodingMacRoman);
	    CFRelease(uuidCf);  

 	// 	struct utsname u;
		// char *name = "osx";
 	//     if ( uname( &u ) >= 0 ) {
		// 	name = u.nodename;
		// }
		// sprintf( buffer, "%s:%s", name, tmp );
		strncpy( buffer, tmp, size );
		buffer[size-1] = 0;
	 #endif

	 #ifdef __linux__
	    // TODO
	 #endif
}

void zPlatformShowWebPage( char *url ) {
	// Try to open the default browswer for the operating system and show the url.
	#ifdef WIN32
		char path[MAX_PATH] = {0};
		char command[MAX_PATH*2];
		DWORD szPath = MAX_PATH;
		AssocQueryString(0, ASSOCSTR_EXECUTABLE, "http", "open", path, &szPath);
			// you'll need to link to shlwapi.lib to get this.
		sprintf(command, "%s %s", path, url);
		system(command);
	#endif

	#ifdef __APPLE__
		CFStringRef URL =  CFStringCreateWithCString( NULL, url, kCFStringEncodingASCII );
		CFURLRef pathRef = CFURLCreateWithString( NULL, URL, NULL );
		if( pathRef ) {
			LSOpenCFURLRef(pathRef, NULL);
			CFRelease(pathRef);
		}
		CFRelease(URL);
	#endif

	#ifdef __linux__
		// TODO
	#endif
}

int zPlatformSystem( char *cmd ) {
	// This is intended as a drop-in replacement for calls to system().  On non-windows
	// platforms, system() is called directly.  On windows, a more elaborate scheme is 
	// used, the purpose of which is to prevent the display of a console window.  If
	// You don't care about the console getting displayed, you can just call system()
	// instead.  This function blocks until the command is completed, just like 
	// system() does. If you don't want to block, and want to just launch a new
	// process instead, see zPlatformSpawnProcess() above.

	// Under windows, if the process was unable to be launched, -1 is returned.
	// If the process was launched, but the exitCode could not be obtained, -2 is
	// returned.  Otherwise, he exitcode of the process is returned.

	#ifndef WIN32
		return system( cmd );
	#else
	  PROCESS_INFORMATION p_info;
	  STARTUPINFO s_info;
	  
	  memset(&s_info, 0, sizeof(s_info));
	  memset(&p_info, 0, sizeof(p_info));
	  s_info.cb = sizeof(s_info);

	  int returnVal = -1;
	  if (CreateProcess( NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &s_info, &p_info))
	  {
	    WaitForSingleObject(p_info.hProcess, INFINITE);

	    DWORD exitCode;
	    if( GetExitCodeProcess( p_info.hProcess, &exitCode ) ) {
	    	returnVal = (int)exitCode;
	    }
	    CloseHandle(p_info.hProcess);
	    CloseHandle(p_info.hThread);
	  }
	  return returnVal;

	#endif
}
