// @ZBS {
//		*MODULE_NAME zvidcap plugin for Unibrain Fire-i API
//		*MASTER_FILE 1
//		*PORTABILITY win32
//		*SDK_DEPENDS firei
//		*REQUIRED_FILES zvidcap.cpp zvidcapfirei.cpp
//		*VERSION 1.0
//		+HISTORY {
//			1.0 After separating zvidcap into a plugin architecture
//				this was added as the third plugin
//			NOTE: If the fiInitialize() fails, ensure that you don't have 
//				  local DLLs that are overriding those installed when the 
//				  UBCore drivers are installed, specifically ub*.dll
//		}
//		+TODO {
//			Need to add multi-camera into this.
//		}
//		+BUILDINFO {
//          This module requires that the cameras be running the ubCore drivers
//			See instruction in the SDK for installing them
//		}
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
#include "windows.h"
// SDK includes:
#include "firei.h"
// STDLIB includes:
#include "assert.h"
#include "stdio.h"
#include "string.h"
// MODULE includes:
#include "zvidcap.h"
// ZBSLIB includes:

namespace ZVidcapFirei {

int fiCameraInitialized = 0;

struct ZVidcapFireiCamera {
	int frame;
	char *buff;
	FIREi_CAMERA_HANDLE camHandle;
	FIREi_ISOCH_ENGINE_HANDLE hIsochEngine;
	int shutdownNotice;
	int frameTimes[16];
	char name[256];
	FIREi_CAMERA_GUID guid;
	int running;
	int w;
	int h;
	int d;
};

#define MAX_CAMERAS (16)
ZVidcapFireiCamera theCameras[MAX_CAMERAS] = {0,};

int zVidcapFireiGetCameraIndexFromDeviceName( char *name ) {
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

void zVidcapFireiAcquireThread( int camIdx ) {
	// @TODO: More than one camera
	while( ! theCameras[camIdx].shutdownNotice ) {
		unsigned int start = timeGetTime();

		FIREi_CAMERA_FRAME cameraFrame;
		FIREi_STATUS fiStatus = FiGetNextCompleteFrame( &cameraFrame, theCameras[camIdx].hIsochEngine, 2500 );
		assert( fiStatus == FIREi_STATUS_SUCCESS );
		
		theCameras[camIdx].buff = (char *)cameraFrame.pCameraFrameBuffer;

		unsigned int stop = timeGetTime();
		theCameras[camIdx].frameTimes[ theCameras[camIdx].frame % (sizeof(theCameras[camIdx].frameTimes)/sizeof(theCameras[camIdx].frameTimes[0])) ] = stop - start;
		theCameras[camIdx].frame ++;
	}
	theCameras[camIdx].shutdownNotice = 0;
}

void zVidcapFireiGetDevices( char **names, int *count ) {
	FIREi_STATUS fiStatus;

	if( ! fiCameraInitialized ) {
		fiCameraInitialized = 1;
		fiStatus = FiInitialize();
			// if this fails, see comment at top of file.
		if( fiStatus != FIREi_STATUS_SUCCESS ) {
			char *string = FiStatusString( fiStatus );
			return;
		}
	}

	int ret = 0;
	*count = 0;

	// CLEAR the camera names if any
	for( int i=0; i<MAX_CAMERAS; i++ ) {
		theCameras[i].name[0] = 0;
	}

	FIREi_CAMERA_GUID camArray[16] = {0,};
	ULONG numberOfCameras = 16;
	fiStatus = FiLocateCameras( camArray, FIREi_LOCATE_ALL_CAMERAS, &numberOfCameras );
	if( fiStatus == FIREi_STATUS_SUCCESS ) {
		*count = numberOfCameras;

		// GET the camera handle
		FIREi_CAMERA_HANDLE camHandle;
		fiStatus = FiOpenCameraHandle( &camHandle, &camArray[0] );
		if( fiStatus != FIREi_STATUS_SUCCESS ) {
			return;
		}

		char vendor[256];	
		char model[256];	
		FiQueryCameraRegister( camHandle, OID_VENDOR_NAME, vendor, sizeof(vendor) );
		FiQueryCameraRegister( camHandle, OID_MODEL_NAME, model, sizeof(model) );
		
		for( int i=0; i<(int)numberOfCameras; i++ ) {
			char name[128];
			sprintf( name, "FIREI %s %s %02x%02x%02x%02x:%02x%02x%02x%02x",
				vendor,
				model,
				camArray[i].Bytes[0],
				camArray[i].Bytes[1],
				camArray[i].Bytes[2],
				camArray[i].Bytes[3],
				camArray[i].Bytes[4],
				camArray[i].Bytes[5],
				camArray[i].Bytes[6],
				camArray[i].Bytes[7]
			);
			names[i] = strdup( name );
			strcpy( theCameras[i].name, name );
			theCameras[i].guid = camArray[i];
		}

		fiStatus = FiCloseCameraHandle( camHandle );
		assert( fiStatus == FIREi_STATUS_SUCCESS );
	}
}

static char *zVidcapFireiModeStrings[] = {
	"160 120 yuv444",
	"320 240 yuv422",
	"640 480 yuv411",
	"640 480 yuv422",
	"640 480 rgb",
	"640 480 mono",
};

static char *zVidcapFireiRateStrings[] = {
	"1.875 FPS",
	"3.75 FPS",
	"7.5 FPS",
	"15 FPS",
	"30 FPS",
	"60 FPS",
	"120 FPS",
	"240 FPS",
};

void zVidcapFireiQueryModes( char *deviceName, char **modes, int *count ) {
	FIREi_STATUS fiStatus;

	if( ! fiCameraInitialized ) {
		fiCameraInitialized = 1;
		fiStatus = FiInitialize();
			// if this fails, see comment at top of file.
		assert( fiStatus == FIREi_STATUS_SUCCESS );
	}

	int camIdx = zVidcapFireiGetCameraIndexFromDeviceName( deviceName );
	assert( camIdx >= 0 );
	ZVidcapFireiCamera &camera = theCameras[camIdx];

	fiStatus = FiOpenCameraHandle( &camera.camHandle, &camera.guid );
	assert( fiStatus == FIREi_STATUS_SUCCESS );

	FIREi_CAMERA_SUPPORTED_FORMAT format;
	memset( &format, 0, sizeof(format) );
	FiQueryCameraRegister( camera.camHandle, OID_CAMERA_VIDEO_FORMATS, &format, sizeof(format) );


	for( int i=0; i<8; i++ ) {

		if( format.SupportedMode[i].uMode ) {

			// For now we are only supportined mode 5
			// The following code is the more general purpose code:
			for( int mode=5; mode<6; mode++ ) {
			//for( int mode=0; mode<6; mode++ ) {

				if( format.SupportedMode[i].FrameRate[mode] ) {

					int fastestRate = -1;
					for( int rate=0; rate<6; rate++ ) {
						if( (1<<rate) & format.SupportedMode[i].FrameRate[mode] ) {
							fastestRate = rate;
							break;
						}
					}
					
					if( fastestRate >= 0 ) {
						fastestRate = 7 - fastestRate;

						char name[256];
						sprintf( name, "%s %s (supported)", zVidcapFireiModeStrings[mode], zVidcapFireiRateStrings[fastestRate] );
						modes[*count] = strdup( name );
						(*count)++;

					}
				}
			}
		}
		else {
			break;
		}
	}

	modes[*count] = 0;
	
	fiStatus = FiCloseCameraHandle( camera.camHandle );
	assert( fiStatus == FIREi_STATUS_SUCCESS );
	camera.camHandle = 0;
}

int zVidcapFireiStartDevice( char *deviceName, char *mode ) {
	FIREi_STATUS fiStatus;

	if( ! fiCameraInitialized ) {
		fiCameraInitialized = 1;
		fiStatus = FiInitialize();
			// if this fails, see comment at top of file.
		assert( fiStatus == FIREi_STATUS_SUCCESS );
	}

	int camIdx = zVidcapFireiGetCameraIndexFromDeviceName( deviceName );
	assert( camIdx >= 0 );
	ZVidcapFireiCamera &camera = theCameras[camIdx];

	camera.running = 1;

	fiStatus = FiOpenCameraHandle( &camera.camHandle, &camera.guid );
	assert( fiStatus == FIREi_STATUS_SUCCESS );

	// For now we only support mode 5 so we don't even have to parse the mode string
	camera.w = 640;
	camera.h = 480;
	camera.d = 1;

	// @TODO: Use the mode string
	FIREi_CAMERA_STARTUP_INFO startupInfo;
	startupInfo.Tag = FIREi_CAMERA_STARTUP_INFO_TAG;
	startupInfo.FrameRate     = fps_30;
	startupInfo.VideoMode     = Mode_5;
	startupInfo.VideoFormat   = Format_0;
	startupInfo.TransmitSpeed = S400;
	startupInfo.IsochSyCode   = 1;
	startupInfo.ChannelNumber = (UCHAR)0;
	fiStatus = FiStartCamera( camera.camHandle, &startupInfo );
//	assert( fiStatus == FIREi_STATUS_SUCCESS );

	fiStatus = FiCreateIsochReceiveEngine( &camera.hIsochEngine );
	assert( fiStatus == FIREi_STATUS_SUCCESS );

	fiStatus = FiStartIsochReceiveEngine( camera.hIsochEngine, &startupInfo, 1 );
	assert( fiStatus == FIREi_STATUS_SUCCESS );

	theCameras[camIdx].frame = 0;
	theCameras[camIdx].shutdownNotice = 0;

	// LAUNCH acquire thread
	DWORD thread;
	HANDLE result = CreateThread(
		NULL, 4096, (LPTHREAD_START_ROUTINE)zVidcapFireiAcquireThread, 
		(LPVOID)camIdx, 0, &thread
	);
	return (result != NULL);
}

void zVidcapFireiShutdownDevice( char *deviceName ) {
	FIREi_STATUS fiStatus;

	int camIdx = zVidcapFireiGetCameraIndexFromDeviceName( deviceName );
	assert( camIdx >= 0 );
	ZVidcapFireiCamera &camera = theCameras[camIdx];

	if( camera.running ) {
		theCameras[camIdx].shutdownNotice = 1;
		Sleep( 200 );

		fiStatus = FiStopIsochReceiveEngine( camera.hIsochEngine );
		assert( fiStatus == FIREi_STATUS_SUCCESS );

		fiStatus = FiDeleteIsochReceiveEngine( camera.hIsochEngine );
		assert( fiStatus == FIREi_STATUS_SUCCESS );

		fiStatus = FiStopCamera( camera.camHandle );
		assert( fiStatus == FIREi_STATUS_SUCCESS );

		fiStatus = FiCloseCameraHandle( camera.camHandle );
		assert( fiStatus == FIREi_STATUS_SUCCESS );
		
		camera.running = 0;
	}
}

void zVidcapFireiGetBitmapDesc( char *deviceName, int &w, int &h, int &d ) {
	int camIdx = zVidcapFireiGetCameraIndexFromDeviceName( deviceName );
	assert( camIdx >= 0 );
	ZVidcapFireiCamera &camera = theCameras[camIdx];
	
	w = camera.w;
	h = camera.h;
	d = camera.d;
}

int zVidcapFireiShowFilterPropertyPageModalDialog( char *deviceName ) {
	// @TODO: More than one camera
	return 0;
}

int zVidcapFireiShowPinPropertyPageModalDialog( char *deviceName ) {
	return 0;
}

char *zVidcapFireiLockNewest( char *deviceName, int *frameNumber ) {
	int camIdx = zVidcapFireiGetCameraIndexFromDeviceName( deviceName );
	assert( camIdx >= 0 );

	// @TODO: Use threads and locking, track frame number

	if( frameNumber ) {
		// SKIP if there is no newer frame.
		if( theCameras[camIdx].frame <= *frameNumber ) {
			return 0;
		}
	}

	if( frameNumber ) {
		*frameNumber = theCameras[camIdx].frame;
	}

	return (char*)theCameras[camIdx].buff;
}

void zVidcapFireiUnlock( char *deviceName ) {
}

int zVidcapFireiGetAvgFrameTimeInMils( char *deviceName ) {
	int camIdx = zVidcapFireiGetCameraIndexFromDeviceName( deviceName );
	assert( camIdx >= 0 );

	int sum = 0;
	for( int i=0; i<16; i++ ) {
		sum += theCameras[camIdx].frameTimes[i];
	}
	return sum / 16;
}

void zVidcapFireiSendDriverMessage( char *deviceName, char *msg ) {
	int camIdx = zVidcapFireiGetCameraIndexFromDeviceName( deviceName );
	assert( camIdx >= 0 );
	ZVidcapFireiCamera &camera = theCameras[camIdx];
	
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
	
	FIREi_CAMERA_FEATURE_CONTROL_REGISTER controlReg = {0,};
	controlReg.Tag = FIREi_CAMERA_FEATURE_CONTROL_REGISTER_TAG;
	int oid;

	if( words[1] && !stricmp(words[1],"auto") ) {
		controlReg.bSetAuto = 1;
	}
	else if( words[1] && !stricmp(words[1],"manual") ) {
		controlReg.bSetAuto = 0;
	}
	else if( words[1] ) {
		char *endPtr; int val = strtoul( words[1], &endPtr, 0 );
		controlReg.bSetAuto = 0;
		controlReg.ushCurValue = val;
	}

	if( words[0] && !stricmp(words[0],"brightness") ) {
		oid = OID_BRIGHTNESS_CONTROL;
	}
	if( words[0] && !stricmp(words[0],"exposure") ) {
		oid = OID_AUTO_EXPOSURE_CONTROL;
	}
	if( words[0] && !stricmp(words[0],"sharpness") ) {
		oid = OID_SHARPNESS_CONTROL;
	}
	if( words[0] && !stricmp(words[0],"hue") ) {
		oid = OID_HUE_CONTROL;
	}
	if( words[0] && !stricmp(words[0],"saturation") ) {
		oid = OID_SATURATION_CONTROL;
	}
	if( words[0] && !stricmp(words[0],"gamma") ) {
		oid = OID_GAMMA_CONTROL;
	}
	if( words[0] && !stricmp(words[0],"shutter") ) {
		oid = OID_SHUTTER_CONTROL;
	}
	if( words[0] && !stricmp(words[0],"gain") ) {
		oid = OID_GAIN_CONTROL;
	}
	if( words[0] && !stricmp(words[0],"iris") ) {
		oid = OID_IRIS_CONTROL;
	}
	if( words[0] && !stricmp(words[0],"focus") ) {
		oid = OID_FOCUS_CONTROL;
	}
	if( words[0] && !stricmp(words[0],"zoom") ) {
		oid = OID_ZOOM_CONTROL;
	}

	FiSetCameraRegister( camera.camHandle, oid, &controlReg, sizeof(controlReg) );

	free( _msg );

}

static ZVidcapPluginRegister zvidcapFireiRegister(
	"Firei",
	zVidcapFireiGetDevices,
	zVidcapFireiQueryModes,
	zVidcapFireiStartDevice,
	zVidcapFireiShutdownDevice,
	zVidcapFireiLockNewest,
	zVidcapFireiUnlock,
	zVidcapFireiGetAvgFrameTimeInMils,
	zVidcapFireiGetBitmapDesc,
	zVidcapFireiShowFilterPropertyPageModalDialog,
	zVidcapFireiShowPinPropertyPageModalDialog,
	zVidcapFireiSendDriverMessage
);

} // End namespace
