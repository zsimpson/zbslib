// @ZBS {
//		*MODULE_OWNER_NAME zfilelock
// }

#ifndef ZFILELOCK_H
#define ZFILELOCK_H

#define ZFILELOCK_EXCLUSIVE (1)
	// Block until the lock is available
#define ZFILELOCK_SHARED (2)
	// Multiple shared locks can exist at the same time but not when there's an exclusive lock
#define ZFILELOCK_NONBLOCKING (4)
	// If the lock fails, do not block waiting for it

int zFileLock( int fd, int flags );
int zFileUnlock( int fd );


#endif



