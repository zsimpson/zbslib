#ifndef ZPLATFORM_H
#define ZPLATFORM_H

#ifdef __USE_GNU
#define stricmp strcasecmp
#endif

void zplatformCopyToClipboard( char *buffer, int size );
void zplatformGetTextFromClipboard( char *buffer, int size );
int zplatformGetNumProcessors();

unsigned int zPlatformSpawnProcess( char *name, char *commandLine );
	// Returns pid or handle
int zPlatformGetNumberProcessors();
int zPlatformIsProcessRunning( int pidOrHandle );
int zPlatformKillProcess( int pidOrHandle );

void zPlatformNoErrorBox();
	// Sets so that no error box will come up on a fault (I think this may be a windows only)

void zPlatformGetMachineId( char *buffer, int size );
	// Get a string that uniquely identifies the machine we are running on.
	// This may be a MAC address, processor ID, etc depending on the platform.
	// It may be that this is not unique for images running in the cloud if it
	// is based on the OS install.

void zPlatformShowWebPage( char *url );
  // open the url in the default OS browser.

int zPlatformSystem( char *cmd, char *logfile=0 );
  // drop-in replacement for system(), but under windows uses CreateProcess
  // to avoid the automatic display of a console window.
  // Added optional logfile param to append to the given filename if desired,
  // captures both stdout and stderr appending to logfile.

#ifdef __USE_GNU
	#define atoi64 atoll
#else
	#define atoi64 _atoi64
#endif

#endif


