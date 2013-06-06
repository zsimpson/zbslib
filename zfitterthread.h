// @ZBS {
//		*MODULE_OWNER_NAME zfitterthread
// }

#ifndef ZFITTERTHREAD_H
#define ZFITTERTHREAD_H


// OPERATING SYSTEM specific includes:
// SDK includes:
#include "pthread.h"
// STDLIB includes:
// MODULE includes:
// ZBSLIB includes:

struct ZFitterThread {
	int threadMonitorStep;
	int threadStepFlag;
	char *threadSendMsg;
	int threadPaused;

	pthread_t threadID;
	pthread_mutex_t mutex;

	struct ZFitter *masterCopy;
	struct ZFitter *renderCopy;

	static void *threadMain( void *args );
	void threadStart( ZFitter *_masterCopy, ZFitter *_renderCopy, int _monitorStep );
		// The _fitter needs to already be started

	void lock();
	void unlock();
};

#endif

