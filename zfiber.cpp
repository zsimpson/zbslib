// @ZBS {
//		*MODULE_NAME Fiber Wrapper
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Provides a wrapper around Win32 fiber API
//		}
//		*PORTABILITY win32
//		*REQUIRED_FILES zfiber.cpp zfiber.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH yes
// }
// OPERATING SYSTEM specific includes:
#ifndef _WIN32_WINNT
#define _WIN32_WINNT (0x0500)
#endif
#include "windows.h"
// SDK includes:
// STDLIB includes:
#include "assert.h"
// MODULE includes:
#include "zfiber.h"
// ZBSLIB includes:


#define ZFIBERS_MAX (100)
void *zfibers[ZFIBERS_MAX] = {0,};
int zfibersToDie[ZFIBERS_MAX] = {0,};
int zfibersToDieCount = 0;

void zfiberYield() {
	SwitchToFiber( zfibers[0] );
}

void zfiberStart( void *func, int arg ) {
	int i;
	void *newFiber = CreateFiber( 4096, (LPFIBER_START_ROUTINE)func, (void *)arg );
	for( i=1; i<ZFIBERS_MAX; i++ ) {
		if( !zfibers[i] ) {
			zfibers[i] = newFiber;
			break;
		}
	}
	assert( i < ZFIBERS_MAX );
}

void zfiberDie() {
	void *currentFiber = GetCurrentFiber();
	for( int i=1; i<ZFIBERS_MAX; i++ ) {
		if( zfibers[i] == currentFiber ) {
			zfibersToDie[zfibersToDieCount++] = i;
			zfiberYield();
		}
	}
}

void zfiberDoAll() {
	int i;
	
	if( !zfibers[0] ) {
		// Initialization of the primate fiber
		zfibers[0] = ConvertThreadToFiber( NULL );
	}

	for( i=1; i<ZFIBERS_MAX; i++ ) {
		if( zfibers[i] ) {
			SwitchToFiber( zfibers[i] );
		}
	}

	// Go though and delete any that have marked themselves
	// as ready to die.  This is sort of a weird hack because
	// for some reason DeleteFiber on any running fiber causes
	// it to call ExitThread and thus exit the whole program!
	for( i=0; i<zfibersToDieCount; i++ ) {
		if( zfibers[zfibersToDie[i]] ) {
			DeleteFiber( zfibers[zfibersToDie[i]] );
			zfibers[zfibersToDie[i]] = 0;
		}
	}
	zfibersToDieCount = 0;
}
