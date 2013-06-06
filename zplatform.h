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

#ifdef __USE_GNU
	#define atoi64 atoll
#else
	#define atoi64 _atoi64
#endif

#endif


