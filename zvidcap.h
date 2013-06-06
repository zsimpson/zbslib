// @ZBS {
//		*MODULE_OWNER_NAME zvidcap
// }

#ifndef ZVIDCAP_H
#define ZVIDCAP_H

// This is a fascade around the incredibly poorly designed
// DirectShow API for Video Capture.
//
// This API is not intended to be all things to all people 
// (which is exactly the problem with the DirectShow API).
// Instead it should do most of what you want with minimal hassle
// If nothing else, it is a good starting place for more 
// complicated solutions.
//
// This code is free with no strings attached and no warranty.
// Author: Jon Blow & Zack Booth Simpson.  25 March 2003.  www.mine-control.com

extern char *zVidcapLastDevice;
extern char *zVidcapLastDeviceMode;
	// The name of the last device started

extern void *zVidcapTraceFile;
void zVidcapOpenTraceFile( char *filename );
	// Open a trace file for debugging
void zVidcapCloseTraceFile();
	// Close the trace file

void zVidcapTrace( char *fmt, ... );
	// Used to trace info out to the zVidcapTraceFile (must be openned first)

char **zVidcapGetDevices( int *count=0 );
	// Retrives a list of video capture devices.
	// The list is terminated by a NULL pointer
	// Or you may fetch the list size from the optional count pointer
	// Pass the names returned from this function to the startDevice function

char **zVidcapQueryModes( char *deviceName=(char*)0, int *count=0 );
	// Call this before you start a device and it will fill in the
	// modes which can then be retrieved with zVidcapGetModes
	// This caches the result list if you call the same thing again (because it can be slow)
	// Returns a list of text strings that describe the modes
	// that were enumerated by the video capture device. List ends in NULL
	// Or you can use the optional count argument to get the count
	// Each string is in the form: "640 480 RGB supported"
	// Where the first is the width, second is height, third is format
	// The list of formats is very large and only some of them are supported by this system.

void zVidcapModeParse( char *modeStr, int &w, int &h, int &supported, char mode[64] );
	// Parse the mode string for basics

int zVidcapStartDevice( char *deviceName=(char*)0, char *mode=(char*)0 );
	// Starts the specified device, the first one if none specified
	// If mode is null then it simply enumerates the modes and returns
	// otherwise it opens that specific mode string
	// Returns boolean success

void zVidcapShutdownDevice( char *deviceName=(char*)0 );
	// Shutdown the specificed device
	// DirectShow is very fussy about not shutting down devices
	// properly.  In NT it will survive but in early OSs it often keeps
	// a lock on the camera which means you have to reboot to use the camera again!

void zVidcapShutdownAll();
	// Shutdown all devices

char *zVidcapLockNewest( char *deviceName=(char*)0, int *frameNumber=(int*)0 );
	// Locks the newest frame available.
	// If deviceName is null it uses the last device you started
	// If you specify an (optional) frame number, it will
	// return NULL if there is no frame available newer than frameNumber 
	// in which case you need not unlock it.

void zVidcapUnlock( char *deviceName=(char*)0 );
	// Unlocks the buffer.  Be sure to call this as soon as you are
	// done processing the frame.

int zVidcapGetAvgFrameTimeInMils( char *deviceName=(char*)0 );
	// Get the avg speed at which frames are arriving from this device

void zVidcapGetBitmapDesc( char *deviceName, int &w, int &h, int &d );
	// Fetches the bitmap description.
	// Pass NULL to deviceName for last device
	// w = width in pixels, h = height in pixels, d = depth in bytes
	// Note that the image will be converted from 
	// bottom to top ordering and also from BGR to RGB automatically
	// This can only be called on started devices

int zVidcapShowFilterPropertyPageModalDialog( char *deviceName=(char*)0 );
	// Shows a modal dialog box for the device's filter properties
	// Returns 1 if the vidcap system has changed its size
	// in which case, if you do any high-level buffering of the dimensions,
	// you should update it.

int zVidcapShowPinPropertyPageModalDialog( char *deviceName=(char*)0 );
	// Shows a modal dialog box for the device's output pin properties
	// Returns 1 if the vidcap system has changed its size
	// in which case, if you do any high-level buffering of the dimensions,
	// you should update it.

void zVidcapSendDriverMessage( char *deviceName, char *str );
	// Send a specific message to the driver.
	// Examples are plugin specific, see the plugin files for details

char *zVidcapPluginNameFromCameraName( char *name );
	// Get the plugin name given the camera name (duh!)

//////////////////////////////////////////////////////////////////////////
// ZVidcapRegisterPlugin
//   This is so that each plugin can identify itself to the fascade

struct ZVidcapPluginRegister {
	ZVidcapPluginRegister(
		char *name,
		void *zVidcapGetDevices,
		void *zVidcapQueryModes,
		void *zVidcapStartDevice,
		void *zVidcapShutdownDevice,
		void *zVidcapLockNewest,
		void *zVidcapUnlock,
		void *zVidcapGetAvgFrameTimeInMils,
		void *zVidcapGetBitmapDesc,
		void *zVidcapShowFilterPropertyPageModalDialog,
		void *zVidcapShowPinPropertyPageModalDialog,
		void *zVidcapSendDriverMessage
	);
};

#define zVidcapDevicesMax (16)
#define zVidcapModesMax (128)

//////////////////////////////////////////////////////////////////////////
// Quick example, (see bottom of zvidcap.cpp for more complete example)

//	char **cameras = zVidcapGetDevices();
//
//	printf( "Modes:\n" );
//	char *mode = 0;
//	zVidcapQueryModes( cameras[0] );
//	for( char **c=zVidcapGetModes(); *c; c++ ) {
//		printf( "%s\n", *c );
//		if( !mode && strstr(*c,"supported") ) {
//			mode = *c;
//		}
//	}
//	
//	zVidcapStartDevice( cameras[0], mode );
//
//	int w, h, d;
//	zVidcapGetBitmapDesc( cameras[0], w, h, d );
//	
//	char *cap = zVidcapLockNewest();
//	   ... memcpy the bitmap to your application

#endif