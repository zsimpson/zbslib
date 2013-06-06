// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			simple wrapper for pthread mutex
//		}
//		*PORTABILITY win32 unix macosx
//		*REQUIRED_FILES pmutex.cpp pmutex.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "assert.h"
// MODULE includes:
#include "pmutex.h"
// ZBSLIB includes:

//===============================================================================================

PMutex::PMutex() {
	int err = pthread_mutex_init( &mutex, 0 );
	assert( !err && "mutex init failed" );
}

//------------------------------------------------------------

PMutex::~PMutex() {
	int err = pthread_mutex_destroy( &mutex );
	assert( !err && "mutex destroy failed" );
}

//------------------------------------------------------------

int PMutex::lock() {
	int err = pthread_mutex_lock( &mutex );
	assert( !err && "mutex lock failed" );
	return err;
}

//------------------------------------------------------------

int PMutex::unlock() {
	int err = pthread_mutex_unlock( &mutex );
	assert( !err && "mutex unlock failed" );
	return err;
}

//------------------------------------------------------------
//------------------------------------------------------------


