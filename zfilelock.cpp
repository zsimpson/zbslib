// @ZBS {
//		*MODULE_NAME zFileLock
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A portable wrapper around basic file locking symantics
//		}
//		+EXAMPLE {
//		}
//		*PORTABILITY win32 unix macosx
//		*REQUIRED_FILES zfilelock.cpp zfilelock.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }

#ifdef WIN32
	#include "io.h"
	#include "string.h"

	// DUPLICATE defines to avoid windows.h

	struct OVERLAPPED {
		unsigned long Internal;
		unsigned long InternalHigh;
		unsigned long Offset;
		unsigned long OffsetHigh;
		unsigned long hEvent;
	};

	#define LOCKFILE_FAIL_IMMEDIATELY   0x00000001
	#define LOCKFILE_EXCLUSIVE_LOCK     0x00000002

	extern "C" {
		__declspec(dllimport) int __stdcall LockFileEx( void *hFile, unsigned long dwFlags, unsigned long dwReserved, unsigned long nNumberOfBytesToLockLow, unsigned long nNumberOfBytesToLockHigh, OVERLAPPED *lpOverlapped );
		__declspec(dllimport) int __stdcall UnlockFileEx( void *hFile, unsigned long dwReserved, unsigned long nNumberOfBytesToUnlockLow, unsigned long nNumberOfBytesToUnlockHigh, OVERLAPPED *lpOverlapped );
	}

#else
	#include "sys/file.h"
#endif
// OPERATING SYSTEM specific includes:
// MODULE includes
#include "zfilelock.h"
// SDK includes:
// STDLIB includes:
// ZBSLIB includes:

int zFileLock( int fd, int lockType ) {
	// Equivileance of flock and LockFile discovered at this python page
	// http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/65203/index_txt

	#ifdef WIN32
		void *handle = (void *)_get_osfhandle( fd );

		int flags = 0;
		if( lockType & ZFILELOCK_EXCLUSIVE ) flags |= LOCKFILE_EXCLUSIVE_LOCK;
		if( lockType & ZFILELOCK_SHARED ) flags |= 0;
		if( lockType & ZFILELOCK_NONBLOCKING ) flags |= LOCKFILE_FAIL_IMMEDIATELY;

		// http://msdn2.microsoft.com/en-us/library/aa365203(VS.85).aspx
		OVERLAPPED overlapped;
		memset( &overlapped, 0, sizeof(overlapped) );
		int locked = LockFileEx( handle, flags, 0, 0xFFFFFFFF, 0, &overlapped );
		return locked;
	#else
		int flags = 0;
		if( lockType & ZFILELOCK_EXCLUSIVE ) flags |= LOCK_EX;
		if( lockType & ZFILELOCK_SHARED ) flags |= LOCK_SH;
		if( lockType & ZFILELOCK_NONBLOCKING ) flags |= LOCK_NB;

		// http://www.daemon-systems.org/man/flock.2.html
		int ret = flock( fd, flags );
		return ret == 0 ? 1 : 0;
	#endif
}

int zFileUnlock( int fd ) {
	#ifdef WIN32
		void *handle = (void *)_get_osfhandle( fd );
		OVERLAPPED overlapped;
		memset( &overlapped, 0, sizeof(overlapped) );
		int unlocked = UnlockFileEx( handle, 0, 0xFFFFFFFF, 0, &overlapped );
		return unlocked;
	#else
		int ret = flock( fd, LOCK_UN );
		return ret == 0 ? 1 : 0;
	#endif
}




//#ifdef SELFTEST_ZFILELOCK
//
//// Run a bunch of these at once
//
//#include "assert.h"
//void main() {
//
//	char buf[10];
//
//	while( 1 ) {
//
//		FILE *f = fopen( "test.dat", "wb" );
//		if( f ) {
//			int locked = zFileLock( "test.dat", ZFILELOCK_EXCLUSIVE );
//			if( locked ) {
//				fwrite( buf, 10, 1, f );
//			}
//		}
//	}
//}
//
//#endif
