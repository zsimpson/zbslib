// @ZBS {
//		*MODULE_NAME Video Capture Wrapper
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A simple C style interface around the abysmal DirectShow video capture API
//			Also allows for other drviers such as CMU 1394 to be attached
//		}
//		+EXAMPLE {
//		}
//		*PORTABILITY win32
//		*SDK_DEPENDS dx8
//		*REQUIRED_FILES zvidcap.cpp zvidcap.h
//		*OPTIONAL_FILES zvidcapdirectx.cpp zvidcapcmu1394.cpp zvidcapfirei.cpp
//		*VERSION 4.0
//		+HISTORY {
//			2.0 Revised to eliminate dependency on Microsoft's deprecated APIs.
//				Thanks to Jon Blow for the new improved way.
//			3.0 Revised how the modes enumerate themselves.  Now the
//              system can give back a list of modes and you can select on start.
//              Previously there was inernal black magic about how it
//              selected the mode, now the API can specify exactly.
//			4.0 Separated directx into a sepate file so that you could use
//				either the cmu1394 or the directx without the other
//		}
//		+TODO {
//          * Some sort of "pick the best mode" kind of function is probably inevitable
//          * Add a callback method thing for extrenally handling unsupported modes
//          * Add standard conversion to RGB and LUM
//		}
//		*SELF_TEST yes console
//		*PUBLISH yes
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "assert.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "stdarg.h"
// MODULE includes:
#include "zvidcap.h"
// ZBSLIB includes:


void *zVidcapTraceFile = 0;
	// A trace file where data will be sent if non-null

char *zVidcapDevices[zVidcapDevicesMax] = {0,};
int   zVidcapDevicePlugins[zVidcapDevicesMax] = {0,};
int zVidcapNumDevices = 0;
	// A static list of all the devices as found on the last enumeration
	// The zVidcapDevicePlugins tells us which plugin gave this device
	// How many elements are in the zVidcapDevices list
	// Note that the list is always null terminated

char *zVidcapModes[zVidcapModesMax] = {0,};
int zVidcapNumModes = 0;
char zVidcapLastDeviceModeQuery[256] = {0,};
	// A static list of all the modes as found on the last enumeration
	// Note this this will be overwritten when a different camera is selected
	// How many elements are in the zVidcapModes list
	// Note that the list is always null terminated

int zVidcapLastPlugin = 0;
char *zVidcapLastDevice = NULL;
char *zVidcapLastDeviceMode = NULL;
	// The last device and mode to get started

// Plugin architecture
//------------------------------------------------------------------------------------------

struct ZVidcapPluginInterface {
	char * name;
	void  (*zVidcapGetDevices)( char **names, int *count );
	void  (*zVidcapQueryModes)( char *deviceName, char **modes, int *count );
	int   (*zVidcapStartDevice)( char *deviceName, char *mode );
	void  (*zVidcapShutdownDevice)( char *deviceName );
	char *(*zVidcapLockNewest)( char *deviceName, int *frameNumber );
	void  (*zVidcapUnlock)( char *deviceName );
	int   (*zVidcapGetAvgFrameTimeInMils)( char *deviceName );
	void  (*zVidcapGetBitmapDesc)( char *deviceName, int &w, int &h, int &d );
	int   (*zVidcapShowFilterPropertyPageModalDialog)( char *deviceName );
	int   (*zVidcapShowPinPropertyPageModalDialog)( char *deviceName );
	void  (*zVidcapSendDriverMessage)( char *deviceName, char *msg );
};

#define zvidcapPluginMax (3)
ZVidcapPluginInterface zvidcapPlugins[zvidcapPluginMax];
int zvidcapPluginCount = 0;

ZVidcapPluginRegister::ZVidcapPluginRegister(
	char *_name,
	void *_zVidcapGetDevices,
	void *_zVidcapQueryModes,
	void *_zVidcapStartDevice,
	void *_zVidcapShutdownDevice,
	void *_zVidcapLockNewest,
	void *_zVidcapUnlock,
	void *_zVidcapGetAvgFrameTimeInMils,
	void *_zVidcapGetBitmapDesc,
	void *_zVidcapShowFilterPropertyPageModalDialog,
	void *_zVidcapShowPinPropertyPageModalDialog,
	void *_zVidcapSendDriverMessage
) {
	zvidcapPlugins[zvidcapPluginCount].name = strdup( _name );
	zvidcapPlugins[zvidcapPluginCount].zVidcapGetDevices = (void(*)(char**,int*))_zVidcapGetDevices;
	zvidcapPlugins[zvidcapPluginCount].zVidcapQueryModes = (void(*)(char*,char**,int*))_zVidcapQueryModes;
	zvidcapPlugins[zvidcapPluginCount].zVidcapStartDevice = (int(*)(char*,char*))_zVidcapStartDevice;
	zvidcapPlugins[zvidcapPluginCount].zVidcapShutdownDevice = (void(*)(char*))_zVidcapShutdownDevice;
	zvidcapPlugins[zvidcapPluginCount].zVidcapLockNewest = (char*(*)(char*,int*))_zVidcapLockNewest;
	zvidcapPlugins[zvidcapPluginCount].zVidcapUnlock = (void(*)(char*))_zVidcapUnlock;
	zvidcapPlugins[zvidcapPluginCount].zVidcapGetAvgFrameTimeInMils = (int(*)(char*))_zVidcapGetAvgFrameTimeInMils;
	zvidcapPlugins[zvidcapPluginCount].zVidcapGetBitmapDesc = (void(*)(char*,int&,int&,int&))_zVidcapGetBitmapDesc;
	zvidcapPlugins[zvidcapPluginCount].zVidcapShowFilterPropertyPageModalDialog = (int(*)(char*))_zVidcapShowFilterPropertyPageModalDialog;
	zvidcapPlugins[zvidcapPluginCount].zVidcapShowPinPropertyPageModalDialog = (int(*)(char*))_zVidcapShowPinPropertyPageModalDialog;
	zvidcapPlugins[zvidcapPluginCount].zVidcapSendDriverMessage = (void(*)(char*,char*))_zVidcapSendDriverMessage;
	zvidcapPluginCount++;
	assert( zvidcapPluginCount <= zvidcapPluginMax );
}

// vidcap public interfaces
//--------------------------------------------------------------

void zVidcapOpenTraceFile( char *filename ) {
	zVidcapCloseTraceFile();
	zVidcapTraceFile = fopen( filename, "wb" );
	fprintf( (FILE*)zVidcapTraceFile, "******************************\n" );
}

void zVidcapCloseTraceFile() {
	if( zVidcapTraceFile ) {
		fclose( (FILE*)zVidcapTraceFile );
		zVidcapTraceFile = 0;
	}
}

void zVidcapTrace( char *fmt, ... ) {
	if( zVidcapTraceFile ) {
		va_list argptr;
		va_start( argptr, fmt );
		vfprintf( (FILE*)zVidcapTraceFile, fmt, argptr );
		va_end( argptr );
		fflush( (FILE*)zVidcapTraceFile );
	}
}

int zVidcapPluginFromCameraName( char *name ) {
	for( int i=0; i<zVidcapDevicesMax; i++ ) {

// @TEMP HACK for cases where someone sends me a config and I want to load it on my camera
//if( !strncmp(name,"Unibrain",8) && zVidcapDevices[i] && !strncmp(zVidcapDevices[i],"Unibrain",8) ) {
//return zVidcapDevicePlugins[i];
//}

		if( zVidcapDevices[i] && !stricmp(zVidcapDevices[i],name) ) {
			return zVidcapDevicePlugins[i];
		}
	}
	return -1;
}

char *zVidcapPluginNameFromCameraName( char *name ) {
	for( int i=0; i<zVidcapDevicesMax; i++ ) {

// @TEMP HACK for cases where someone sends me a config and I want to load it on my camera
//if( !strncmp(name,"Unibrain",8) && zVidcapDevices[i] && !strncmp(zVidcapDevices[i],"Unibrain",8) ) {
//return zvidcapPlugins[zVidcapDevicePlugins[i]].name;
//}

		if( zVidcapDevices[i] && !stricmp(zVidcapDevices[i],name) ) {
			return zvidcapPlugins[zVidcapDevicePlugins[i]].name;
		}
	}
	return 0;
}

char **zVidcapGetDevices( int *count ) {
	// FREE all the current ones
	int i;
	for( i=0; i<zVidcapNumDevices; i++ ) {
		free( zVidcapDevices[i] );
		zVidcapDevices[i] = 0;
	}
	zVidcapNumDevices = 0;

	for( i=0; i<zvidcapPluginCount; i++ ) {
		int count = 0;
		(*zvidcapPlugins[i].zVidcapGetDevices)( &zVidcapDevices[zVidcapNumDevices], &count );
		for( int j=0; j<count; j++ ) {
			zVidcapDevicePlugins[zVidcapNumDevices+j] = i;
		}
		zVidcapNumDevices += count;
		assert( zVidcapNumDevices < zVidcapDevicesMax );
	}
	if( count ) {
		*count = zVidcapNumDevices;
	}
	return zVidcapDevices;
}

char **zVidcapQueryModes( char *deviceName, int *count ) {
	if( !deviceName ) deviceName = zVidcapLastDevice;
	if( !deviceName ) return 0;

	// CHECK the cache
	if( !stricmp(deviceName,zVidcapLastDeviceModeQuery) ) {
		if( count ) {
			*count = zVidcapNumModes;
		}
		return zVidcapModes;
	}

	zVidcapNumModes = 0;
	int plugin = zVidcapPluginFromCameraName( deviceName );
	if( plugin < 0 ) {
		return 0;
	}
	strcpy( zVidcapLastDeviceModeQuery, deviceName );
	(*zvidcapPlugins[plugin].zVidcapQueryModes)( deviceName, (char**)zVidcapModes, &zVidcapNumModes );
	assert( zVidcapNumModes < zVidcapModesMax );
	if( count ) {
		*count = zVidcapNumModes;
	}
	return zVidcapModes;
}

void zVidcapModeParse( char *modeStr, int &w, int &h, int &supported, char mode[64] ) {
	char *_modeStr = strdup( modeStr );
	mode[0] = 0;
	supported = 0;
	w = 0;
	h = 0;
	char *s1=0, *s2=0, *s3=0, *s4=0;

	// FETCH W
	s1 = strchr( _modeStr, ' ' );
	if( s1 ) {
		*s1 = 0;
		w = atoi( _modeStr );
		s2 = strchr( s1+1, ' ' );
	}

	// FETCH H
	if( s2 ) {
		*s2 = 0;
		h = atoi( s1+1 );
	}

	// FETCH supported
	s3 = strrchr( modeStr, ' ' );
	if( s3 ) {
		int offset = s3 - modeStr;
		s3 = &_modeStr[offset];
		if( strstr(s3,"supported") ) {
			supported = 1;
		}
		*s3 = 0;
		if( s2 ) {
			mode[63] = 0;
			strncpy( mode, s2+1, 63 );
		}
	}
	free( _modeStr );
}

int zVidcapStartDevice( char *deviceName, char *mode ) {
	zVidcapTrace( "%s %d. %s %s\n", __FILE__, __LINE__, deviceName, mode );
	if( !deviceName ) {
		// Start the first device
		char **devices = zVidcapGetDevices( 0 );
		deviceName = devices[0];
	}
	if( !deviceName ) return 0;
	int plugin = zVidcapPluginFromCameraName( deviceName );
	zVidcapTrace( "%s %d. %d\n", __FILE__, __LINE__, plugin );
	if( plugin < 0 ) {
		return 0;
	}
	zVidcapTrace( "%s %d\n", __FILE__, __LINE__ );

	// KLD: Detecting modes on my Logitech Ultravision takes forever, and is
	// not really needed.  So limit it here.  Yes, it is a crazy hack for a special case.
	// I hacked rather than try to add some big context to the maze of vid cap calls.
	const char *envName = getenv("sgUser");
	if( envName && !stricmp(envName,"kdemarest") )
	{
		static bool firstTime = true;
		if( !firstTime )
		{
			if( !mode ) {
				char *modes = 0;
				int modeCount = 0;
				(*zvidcapPlugins[plugin].zVidcapQueryModes)( deviceName, &modes, &modeCount );
				mode = modes;
			}
		}
		firstTime = false;
	}

	int ret = (*zvidcapPlugins[plugin].zVidcapStartDevice)( deviceName, mode );
	zVidcapTrace( "%s %d ret=%d\n", __FILE__, __LINE__, ret );
	if( zVidcapLastDevice ) {
		free( zVidcapLastDevice );
		zVidcapLastDevice = 0;
	}
	if( ret ) {
		zVidcapLastDevice = strdup( deviceName );
	}
	zVidcapTrace( "%s %d. %d\n", __FILE__, __LINE__, ret );
	return ret;
}

void zVidcapShutdownDevice( char *deviceName ) {
	if( !deviceName ) deviceName = zVidcapLastDevice;
	if( !deviceName ) return;
	int plugin = zVidcapPluginFromCameraName( deviceName );
	assert( plugin >= 0 );
	(*zvidcapPlugins[plugin].zVidcapShutdownDevice)( deviceName );
}

void zVidcapShutdownAll() {
	for( int i=0; i<zVidcapNumDevices; i++ ) {
		zVidcapShutdownDevice( zVidcapDevices[i] );
	}
}

void zVidcapGetBitmapDesc( char *deviceName, int &w, int &h, int &d ) {
	w = 0;
	h = 0;
	d = 0;
	if( !deviceName ) deviceName = zVidcapLastDevice;
	if( !deviceName ) return;
	int plugin = zVidcapPluginFromCameraName( deviceName );
	assert( plugin >= 0 );
	(*zvidcapPlugins[plugin].zVidcapGetBitmapDesc)( deviceName, w, h, d );
}

int zVidcapShowFilterPropertyPageModalDialog( char *deviceName ) {
	if( !deviceName ) deviceName = zVidcapLastDevice;
	if( !deviceName ) return 0;
	int plugin = zVidcapPluginFromCameraName( deviceName );
	assert( plugin >= 0 );
	(*zvidcapPlugins[plugin].zVidcapShowFilterPropertyPageModalDialog)( deviceName );
	return 0;
}

int zVidcapShowPinPropertyPageModalDialog( char *deviceName ) {
	if( !deviceName ) deviceName = zVidcapLastDevice;
	if( !deviceName ) return 0;
	int plugin = zVidcapPluginFromCameraName( deviceName );
	assert( plugin >= 0 );
	(*zvidcapPlugins[plugin].zVidcapShowPinPropertyPageModalDialog)( deviceName );
	return 0;
}

char *zVidcapLockNewest( char *deviceName, int *frameNumber ) {
	if( !deviceName ) deviceName = zVidcapLastDevice;
	if( !deviceName ) return 0;
	int plugin = zVidcapPluginFromCameraName( deviceName );
	assert( plugin >= 0 );
	return (*zvidcapPlugins[plugin].zVidcapLockNewest)( deviceName, frameNumber );
}

void zVidcapUnlock( char *deviceName ) {
	if( !deviceName ) deviceName = zVidcapLastDevice;
	if( !deviceName ) return;
	int plugin = zVidcapPluginFromCameraName( deviceName );
	(*zvidcapPlugins[plugin].zVidcapUnlock)( deviceName );
}

int zVidcapGetAvgFrameTimeInMils( char *deviceName ) {
	if( !deviceName ) deviceName = zVidcapLastDevice;
	if( !deviceName ) return 0;
	int plugin = zVidcapPluginFromCameraName( deviceName );
	assert( plugin >= 0 );
	return (*zvidcapPlugins[plugin].zVidcapGetAvgFrameTimeInMils)( deviceName );
}

void zVidcapSendDriverMessage( char *deviceName, char *str ) {
	if( !deviceName ) deviceName = zVidcapLastDevice;
	if( !deviceName ) return;
	int plugin = zVidcapPluginFromCameraName( deviceName );
	assert( plugin >= 0 );
	(*zvidcapPlugins[plugin].zVidcapSendDriverMessage)( deviceName, str );
}


//--------------------------------------------------------------------------------
// Test code shows the capture image in old-fashioned ASCII text!

#define SELF_TEST
#ifdef SELF_TEST

#include "windows.h"
#include "conio.h"

void clrscr() {
	HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
	COORD coordScreen = { 0, 0 };
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo( hConsole, &csbi );
	DWORD dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
	DWORD cCharsWritten;
	FillConsoleOutputCharacter( hConsole, (TCHAR) ' ', dwConSize, coordScreen, &cCharsWritten );
	GetConsoleScreenBufferInfo( hConsole, &csbi );
	FillConsoleOutputAttribute( hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten );
	SetConsoleCursorPosition( hConsole, coordScreen );
}

int menu( char **items, int count ) {
	int i;
	printf( "Select one:\n" );
	for( i=0; i<count; i++ ) {
		printf( "%2d. %s\n", i+1, items[i] );
	}
	scanf( "%d", &i );
	return i;
}

void main() {
	char *mainMenu[] = {
		"Select Camera",
		"List modes",
		"View Camera in ASCII text",
		"Pin Options",
		"Filter Options",
		"Quit"
	};

	int numCameras = 0;
	char **cameras = zVidcapGetDevices( &numCameras );
	char *selectedCamera = 0;

	int w, h, d;
	zVidcapGetBitmapDesc( selectedCamera, w, h, d );
	int lastFrame = 0;

	//zVidcapOpenTraceFile( "trace.txt" );

	char *selectedMode = strdup( "320 240 RGB24" );
	int lastStatus = 0;

	while( 1 ) {
		printf( "Selected Mode: %s\n", selectedMode );
		printf( "Selected Cam : %s\n", selectedCamera );
		printf( "Last status  : %d\n", lastStatus );
		switch( menu( mainMenu, sizeof(mainMenu)/sizeof(mainMenu[0]) ) ) {
			case 1: {
				int i = menu( cameras, numCameras );
				selectedCamera = strdup( cameras[i-1] );
				break;
			}

			case 2: {
				// List modes
				zVidcapQueryModes( selectedCamera );
				int i = menu( zVidcapModes, zVidcapNumModes );
				if( selectedMode ) free( selectedMode );
				selectedMode = strdup( zVidcapModes[i-1] );
				break;
			}

			case 3: {
				lastStatus = zVidcapStartDevice( selectedCamera, selectedMode );
				zVidcapGetBitmapDesc( selectedCamera, w, h, d );
				lastFrame = 0;

				while( !kbhit() ) {
					// LOCK the newest frame
					char *cap = zVidcapLockNewest( 0, &lastFrame );
					if( cap ) {
						#define W (78)
						#define H (24)
						static char disp[(W+1)*H+2];
						char *dst = disp;
						clrscr();
						for( int y=0; y<H; y++ ) {
							unsigned char *src = (unsigned char *)( cap + d*w*(h*y/H) );
							for( int x=0; x<W; x++ ) {
								unsigned char *_src = src + d * (x * w / W);
								*dst++ = "#&*H%!I|)>/;:-. "[ ((*_src) >> 4) & 15 ];
							}
							*dst++ = '\n';
						}
						printf( "%s", disp );

						// UNLOCK
						zVidcapUnlock();

						int mils = zVidcapGetAvgFrameTimeInMils();

						printf( "w=%d h=%d d=%d lastFrame=%d FPS=%2.1f\n", w, h, d, lastFrame, 1000.f / (float)mils );
					}

					Sleep( 10 );
						// Sleeping helps reduce flicker
				}
				zVidcapShutdownAll();
				break;
			}

			case 4: {
				zVidcapShowPinPropertyPageModalDialog( selectedCamera );
				zVidcapGetBitmapDesc( selectedCamera, w, h, d );
				lastFrame = 0;
				break;
			}

			case 5: {
				zVidcapShowFilterPropertyPageModalDialog( selectedCamera );
				zVidcapGetBitmapDesc( selectedCamera, w, h, d );
				lastFrame = 0;
				break;
			}

			case 6: {
				goto done;
			}
		}
	}

	done:
	zVidcapShutdownAll();
}

#endif
