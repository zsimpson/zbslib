// @ZBS {
//		*MODULE_NAME zvidcap plugin for CMU1394 capture
//		*MASTER_FILE 1
//		*PORTABILITY win32
//		*SDK_DEPENDS cmu1394
//		*REQUIRED_FILES zvidcap.cpp zvidcap.h zvidcapcmu1394.cpp
//		*VERSION 1.0
//		+HISTORY {
//			1.0 After separating zvidcap into a plugin architecture
//				this was added as the second plugin
//		}
//		+TODO {
//			Need to add multi-camera into this.
//		}
//		+BUILDINFO {
//          This module requires the headers from cmu1394\include
//          This module requires the following libraries
//			 * cmu1394\lib\1394camera.lib (CMU1394)
//			 * winmm.lib (win32)
//          This module requires that the cameras be running the CMU1394 driver
//			See instruction in the SDK for installing them
//		}
//		*PUBLISH yes
// }
// OPERATING SYSTEM specific includes:
#include "windows.h"
// SDK includes:
#include "1394camera.h"
// STDLIB includes:
#include "assert.h"
#include "stdio.h"
#include "string.h"
// MODULE includes:
#include "zvidcap.h"
// ZBSLIB includes:

//#include "zvars.h"
//ZVAR( int, twiddle, 128 );

namespace ZVidcapCMU1394 {

struct ZVidcap1394Camera {
	int frame;
	char *buff;
	int shutdownNotice;
	int frameTimes[16];
	C1394Camera camera;
	char name[256];
};

#define MAX_CAMERAS (16)
ZVidcap1394Camera theCameras[MAX_CAMERAS] = {0,};

int zVidcap1394GetCameraIndexFromDeviceName( char *name ) {

// Temp hack for when I need to use an external configuarion
//return 0;

	for( int i=0; i<MAX_CAMERAS; i++ ) {
		if( !theCameras[i].name[0] ) {
			return -1;
		}
		if( !strcmp(name,theCameras[i].name) ) {
			return i;
		}
	}
	return -1;
}

void zVidcap1394AcquireThread( int camIdx ) {
	// @TODO: More than one camera
	while( ! theCameras[camIdx].shutdownNotice ) {
		int thisDropped = 0;

		unsigned int start = timeGetTime();
		int ret = theCameras[camIdx].camera.AcquireImageEx( 1, &thisDropped );
		unsigned int stop = timeGetTime();
		theCameras[camIdx].frameTimes[ theCameras[camIdx].frame % (sizeof(theCameras[camIdx].frameTimes)/sizeof(theCameras[camIdx].frameTimes[0])) ] = stop - start;
		theCameras[camIdx].frame += 1+thisDropped;
	}
	theCameras[camIdx].shutdownNotice = 0;
}

void zVidcap1394GetDevices( char **names, int *count ) {
	int ret = 0;
	*count = 0;

	// CLEAR the camera names if any
	for( int i=0; i<MAX_CAMERAS; i++ ) {
		theCameras[i].name[0] = 0;
	}

	// ENUM the cameras into both the callers "names" and the local "theCameraNames"
	ret = theCameras[0].camera.CheckLink();

	int numCams = theCameras[0].camera.GetNumberCameras();
	for( int i=0; i<numCams; i++ ) {

		theCameras[i].camera.CheckLink();
		theCameras[i].camera.SelectCamera(i);
		theCameras[i].camera.InitCamera();

		char name[128];
		C1394Camera &camera = theCameras[i].camera;

		sprintf( name, "%s %s %08x%08x", camera.m_nameVendor, camera.m_nameModel, camera.m_UniqueID.LowPart, camera.m_UniqueID.HighPart );
		names[*count] = strdup( name );
		
		strcpy( theCameras[i].name, name );
		
		(*count)++;
	}
}

static char *zVidcap1394ModeStrings[] = {
	"160 120 yuv444",
	"320 240 yuv422",
	"640 480 yuv411",
	"640 480 yuv422",
	"640 480 rgb",
	"640 480 mono",

	"800 600 yuv422",
	"800 600 rgb",
	"800 600 mono",
	"1024 768 yuv422",
	"1024 768 rgb",
	"1024 768 mono",

	"1280 960 yuv422",
	"1280 960 rgb",
	"1280 960 mono",
	"1600 1200 yuv422",
	"1600 1200 rgb",
	"1600 1200 mono",
};

static char *zVidcap1394RateStrings[] = {
	"1.875 FPS",
	"3.75 FPS",
	"7.5 FPS",
	"15 FPS",
	"30 FPS",
	"60 FPS",
};

void zVidcap1394QueryModes( char *deviceName, char **modes, int *count ) {
	// @TODO: This needs to select the right camera.  Right now it is assuming there is only one!

	int camIdx = zVidcap1394GetCameraIndexFromDeviceName( deviceName );
	assert( camIdx >= 0 );

	for( int format=0; format<3; format++ ) {
		for( int mode=0; mode<6; mode++ ) {

			int fastestRate = -1;
			for( int rate=0; rate<6; rate++ ) {
				if( theCameras[camIdx].camera.m_videoFlags[format][mode][rate] ) {
					fastestRate = rate;
				}
			}

			if( fastestRate >= 0 ) {
				char name[256];
				sprintf( name, "%s %s (supported)", zVidcap1394ModeStrings[format*6+mode], zVidcap1394RateStrings[fastestRate] );
				modes[*count] = strdup( name );
				(*count)++;
			}
		}
	}
}

int zVidcap1394StartDevice( char *deviceName, char *mode ) {
	int camIdx = zVidcap1394GetCameraIndexFromDeviceName( deviceName );
	assert( camIdx >= 0 );
	C1394Camera &camera = theCameras[camIdx].camera;

	camera.InquireControlRegisters();
	camera.StatusControlRegisters();

	// Pick out the first two words of the mode for matching
	char modeStr[128] = {0,};
	char *d = modeStr;
	int spaceCount = 0;
	for( char *c=mode; *c; c++ ) {
		if( *c==' ' ) spaceCount++;
		if( spaceCount == 3 ) break;
		*d++ = *c;
	}
	int len = strlen( modeStr );

	int f, m;
	for( f=0; f<3; f++ ) {
		for( m=0; m<6; m++ ) {
			if( !strncmp(modeStr,zVidcap1394ModeStrings[f*6+m],len) ) {
				camera.SetVideoFormat(f);
				camera.SetVideoMode(m);

				int fastestRate = -1;
				for( int rate=0; rate<6; rate++ ) {
					if( camera.m_videoFlags[f][m][rate] ) {
						fastestRate = rate;
					}
				}
				if( fastestRate >= 0 ) {
					camera.SetVideoFrameRate(fastestRate);
				}

				int ret = camera.StartImageAcquisition();
				if( ret != CAM_SUCCESS ) {
					return 0;
				}

				int w = camera.m_width;
				int h = camera.m_height;
				theCameras[camIdx].frame = 0;
				theCameras[camIdx].buff = (char *)malloc( w * h * 3 );
				theCameras[camIdx].shutdownNotice = 0;

				// LAUNCH acquire thread
				DWORD thread;
				HANDLE result = CreateThread(
					NULL, 4096, (LPTHREAD_START_ROUTINE)zVidcap1394AcquireThread, 
					(LPVOID)camIdx, 0, &thread
				);
				return (result != NULL);

				goto foundJump;
			}
		}
	}
	foundJump:;

	// @TODO: This is going to need to create a thread
	return 1;
}

void zVidcap1394ShutdownDevice( char *deviceName ) {
	int camIdx = zVidcap1394GetCameraIndexFromDeviceName( deviceName );
	assert( camIdx >= 0 );

	theCameras[camIdx].shutdownNotice = 1;
	Sleep( 200 );
	theCameras[camIdx].camera.StopImageAcquisition();
	free( theCameras[camIdx].buff );
	theCameras[camIdx].buff = 0;
}

void zVidcap1394GetBitmapDesc( char *deviceName, int &w, int &h, int &d ) {
	int camIdx = zVidcap1394GetCameraIndexFromDeviceName( deviceName );
	assert( camIdx >= 0 );

	w = theCameras[camIdx].camera.m_width;
	h = theCameras[camIdx].camera.m_height;
	d = 3;
}

int zVidcap1394ShowFilterPropertyPageModalDialog( char *deviceName ) {
	// @TODO: More than one camera
	return 0;
}
/*
long _zconsoleGetConsoleHwnd() {
	#ifdef WIN32
		// In modern windows interface there is a routine GetConsoleWindow
		// but under old windows you have to change the title and then find it.
		// To maintain compatibility, I'm doing it that old way.

		char pszNewWindowTitle[1024];
		char pszOldWindowTitle[1024];

		// FETCH current window title
		GetConsoleTitle( pszOldWindowTitle, 1024 );

		// CREATE a "unique" NewWindowTitle
		wsprintf( pszNewWindowTitle,"%d/%d",GetTickCount(),GetCurrentProcessId() );

		// CHANGE current window title
		SetConsoleTitle( pszNewWindowTitle );

		// ENSURE window title has been updated
		Sleep(40);

		// FIND the windows
		long hwnd = (long)FindWindow( NULL, pszNewWindowTitle );

		// RESTORE original window title
		SetConsoleTitle( pszOldWindowTitle );

		return hwnd;
	#endif
	return 0;
}
*/
int zVidcap1394ShowPinPropertyPageModalDialog( char *deviceName ) {
	int camIdx = zVidcap1394GetCameraIndexFromDeviceName( deviceName );
	assert( camIdx >= 0 );

	// @TODO, not working!
	HWND hwnd = GetForegroundWindow();
	CameraControlDialog( hwnd, &theCameras[camIdx].camera, TRUE );
	HWND hwnd2 = GetForegroundWindow();
//	CameraControlDialog( 0, &theCamera, TRUE );
//	HWND w = (HWND)_zconsoleGetConsoleHwnd();
//	CameraControlSizeDialog( hwnd, &theCamera );
	MSG msg;
//	while( GetMessage(&msg, NULL, 0, 0) > 0 ) {
//		printf( "%d\n", msg.message );
//		if( msg.message == WM_DESTROY || msg.message == WM_CLOSE ) {
//			int a = 1;
//		}
//		TranslateMessage(&msg);
//		DispatchMessage(&msg);
//	}

while( GetMessage(&msg, NULL, 0, 0) > 0 ) {
	if( !IsDialogMessage(hwnd2,&msg) ) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}


	return 0;
}

char *zVidcap1394LockNewest( char *deviceName, int *frameNumber ) {
	int camIdx = zVidcap1394GetCameraIndexFromDeviceName( deviceName );
	assert( camIdx >= 0 );

	// @TODO: Use threads and locking, track frame number

	int w = theCameras[camIdx].camera.m_width;
	int h = theCameras[camIdx].camera.m_height;

	if( frameNumber ) {
		// SKIP if there is no newer frame.
		if( theCameras[camIdx].frame <= *frameNumber ) {
			return 0;
		}
	}

	// The 1394 system doesn't need locks because it seems to make a copy
	theCameras[camIdx].camera.getRGB( (unsigned char *)theCameras[camIdx].buff );

	if( frameNumber ) {
		*frameNumber = theCameras[camIdx].frame;
	}

	return (char*)theCameras[camIdx].buff;
}

void zVidcap1394Unlock( char *deviceName ) {
	// The 1394 system doesn't need locks because it seems to make a copy
}

int zVidcap1394GetAvgFrameTimeInMils( char *deviceName ) {
	int camIdx = zVidcap1394GetCameraIndexFromDeviceName( deviceName );
	assert( camIdx >= 0 );

	int sum = 0;
	for( int i=0; i<16; i++ ) {
		sum += theCameras[camIdx].frameTimes[i];
	}
	return sum / 16;
}

void zVidcap1394SendDriverMessage( char *deviceName, char *msg ) {
	int camIdx = zVidcap1394GetCameraIndexFromDeviceName( deviceName );
	assert( camIdx >= 0 );
	C1394Camera &camera = theCameras[camIdx].camera;
	
	char *_msg = strdup( msg );

	int wordCount = 0;
	char *words[16] = {0,};
	words[wordCount++] = _msg;
	for( char *c = _msg; *c; c++ ) {
		if( *c == ' ' ) {
			*c = 0;
			words[wordCount++] = c+1;
		}
		if( *c == 0 ) {
			break;
		}
	}

	if( words[0] && !stricmp(words[0],"brightness") ) {
		if( words[1] && !stricmp(words[1],"auto") ) camera.m_controlBrightness.SetAutoMode(1);
		else if( words[1] && !stricmp(words[1],"manual") ) camera.m_controlBrightness.SetAutoMode(0);
		else if( words[1] ) {
			char *endPtr; int val = strtoul( words[1], &endPtr, 0 );
			camera.SetBrightness(val);
		}
	}

	if( words[0] && !stricmp(words[0],"exposure") ) {
		if( words[1] && !stricmp(words[1],"auto") ) camera.m_controlAutoExposure.SetAutoMode(1);
		else if( words[1] && !stricmp(words[1],"manual") ) camera.m_controlAutoExposure.SetAutoMode(0);
		else if( words[1] ) {
			char *endPtr; int val = strtoul( words[1], &endPtr, 0 );
			camera.SetAutoExposure(val);
		}
	}

	if( words[0] && !stricmp(words[0],"sharpness") ) {
		if( words[1] && !stricmp(words[1],"auto") ) camera.m_controlSharpness.SetAutoMode(1);
		else if( words[1] && !stricmp(words[1],"manual") ) camera.m_controlSharpness.SetAutoMode(0);
		else if( words[1] ) {
			char *endPtr; int val = strtoul( words[1], &endPtr, 0 );
			camera.SetSharpness(val);
		}
	}

	// @TODO: whitebalance

	if( words[0] && !stricmp(words[0],"hue") ) {
		if( words[1] && !stricmp(words[1],"auto") ) camera.m_controlHue.SetAutoMode(1);
		else if( words[1] && !stricmp(words[1],"manual") ) camera.m_controlHue.SetAutoMode(0);
		else if( words[1] ) {
			char *endPtr; int val = strtoul( words[1], &endPtr, 0 );
			camera.SetHue(val);
		}
	}

	if( words[0] && !stricmp(words[0],"saturation") ) {
		if( words[1] && !stricmp(words[1],"auto") ) camera.m_controlSaturation.SetAutoMode(1);
		else if( words[1] && !stricmp(words[1],"manual") ) camera.m_controlSaturation.SetAutoMode(0);
		else if( words[1] ) {
			char *endPtr; int val = strtoul( words[1], &endPtr, 0 );
			camera.SetSaturation(val);
		}
	}

	if( words[0] && !stricmp(words[0],"gamma") ) {
		if( words[1] && !stricmp(words[1],"auto") ) camera.m_controlGamma.SetAutoMode(1);
		else if( words[1] && !stricmp(words[1],"manual") ) camera.m_controlGamma.SetAutoMode(0);
		else if( words[1] ) {
			char *endPtr; int val = strtoul( words[1], &endPtr, 0 );
			camera.SetGamma(val);
		}
	}

	if( words[0] && !stricmp(words[0],"shutter") ) {
		if( words[1] && !stricmp(words[1],"auto") ) camera.m_controlShutter.SetAutoMode(1);
		else if( words[1] && !stricmp(words[1],"manual") ) camera.m_controlShutter.SetAutoMode(0);
		else if( words[1] ) {
			char *endPtr; int val = strtoul( words[1], &endPtr, 0 );
			camera.SetShutter(val);
		}
	}

	if( words[0] && !stricmp(words[0],"gain") ) {
		if( words[1] && !stricmp(words[1],"auto") ) camera.m_controlGain.SetAutoMode(1);
		else if( words[1] && !stricmp(words[1],"manual") ) camera.m_controlGain.SetAutoMode(0);
		else if( words[1] ) {
			char *endPtr; int val = strtoul( words[1], &endPtr, 0 );
			camera.SetGain(val);
		}
	}

	if( words[0] && !stricmp(words[0],"iris") ) {
		if( words[1] && !stricmp(words[1],"auto") ) camera.m_controlIris.SetAutoMode(1);
		else if( words[1] && !stricmp(words[1],"manual") ) camera.m_controlIris.SetAutoMode(0);
		else if( words[1] ) {
			char *endPtr; int val = strtoul( words[1], &endPtr, 0 );
			camera.SetIris(val);
		}
	}

	if( words[0] && !stricmp(words[0],"focus") ) {
		if( words[1] && !stricmp(words[1],"auto") ) camera.m_controlFocus.SetAutoMode(1);
		else if( words[1] && !stricmp(words[1],"manual") ) camera.m_controlFocus.SetAutoMode(0);
		else if( words[1] ) {
			char *endPtr; int val = strtoul( words[1], &endPtr, 0 );
			camera.SetFocus(val);
		}
	}

	if( words[0] && !stricmp(words[0],"zoom") ) {
		if( words[1] && !stricmp(words[1],"auto") ) camera.m_controlZoom.SetAutoMode(1);
		else if( words[1] && !stricmp(words[1],"manual") ) camera.m_controlZoom.SetAutoMode(0);
		else if( words[1] ) {
			char *endPtr; int val = strtoul( words[1], &endPtr, 0 );
			camera.SetZoom(val);
		}
	}


	free( _msg );
}

static ZVidcapPluginRegister zvidcapDirectXRegister(
	"CMU1394",
	zVidcap1394GetDevices,
	zVidcap1394QueryModes,
	zVidcap1394StartDevice,
	zVidcap1394ShutdownDevice,
	zVidcap1394LockNewest,
	zVidcap1394Unlock,
	zVidcap1394GetAvgFrameTimeInMils,
	zVidcap1394GetBitmapDesc,
	zVidcap1394ShowFilterPropertyPageModalDialog,
	zVidcap1394ShowPinPropertyPageModalDialog,
	zVidcap1394SendDriverMessage
);

} // End namespace
