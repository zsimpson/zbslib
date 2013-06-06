// @ZBS {
//		*MODULE_OWNER_NAME pmutex
// }

#ifndef PMUTEX_H
#define PMUTEX_H

#include "pthread.h"

//======================================================================================
// This started by not wanting to remember long pthread function names...

class PMutex {

	pthread_mutex_t mutex;

public:
	PMutex();
	~PMutex();

	int lock();
	int unlock();
};

#endif