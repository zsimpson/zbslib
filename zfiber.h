// @ZBS {
//		*MODULE_OWNER_NAME zfiber
// }

#ifndef ZFIBER_H
#define ZFIBER_H

void zfiberStart( void *func, int arg=0 );
	// Start a fiber at the given function which can take one argument

void zfiberDie();
	// Marks the fiber to die at end of next doAll

void zfiberDoAll();
	// Call once per main loop to give all the fibers a chance to run

void zfiberYield();
	// The current fiber yields the CPU

#endif

