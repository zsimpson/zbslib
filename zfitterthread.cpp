// @ZBS {
//		*MODULE_NAME ZFitterThread
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Adds thread support fot ZFitter
//		}
//		*PORTABILITY win32 unix maxosx
//		*REQUIRED_FILES zfitterthread.cpp zfitterthread.h
//		*VERSION 1.0
//		+HISTORY {
//			1.0 ZBS Jun 2009, based on refactoring of fitter v 3.0
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
// MODULE includes:
#include "zfitterthread.h"
#include "zfitter.h"
#include "ztime.h"
// ZBSLIB includes:

void *ZFitterThread::threadMain( void *arg ) {
	ZFitterThread *_this = (ZFitterThread *)arg;
	
	_this->renderCopy->clear();
	while( _this->masterCopy->step() == ZF_RUNNING ) {

		// MONITOR the breakpoint so the outside thread can watch each step and request pauses, etc
//		if( _this->threadSendMsg ) {
//			zMsgQueue( _this->threadSendMsg );
//		}
		if( _this->threadMonitorStep ) {
			while( 1 ) {
				_this->threadPaused = 1;
				if( _this->threadStepFlag ) {
					// An outside thread has signalled to proceed
					_this->threadStepFlag = 0;
					_this->threadPaused = 0;
					break;
				}
				else {
					zTimeSleepMils( 500 );
				}
			}
		}

		_this->lock();
		_this->renderCopy->copy( _this->masterCopy );
		_this->unlock();
			// The renderFit is now safe place to be rendered by the rendering thread
			// without fear that the fitting thread is going to manipulate something during render
	}

	// FINAL update of the render copy
	_this->lock();
	_this->renderCopy->copy( _this->masterCopy );
	_this->unlock();

	_this->masterCopy->stop();

	return 0;
}

void ZFitterThread::lock() {
	if( mutex ) {
		pthread_mutex_lock( &mutex );
	}
}

void ZFitterThread::unlock() {
	if( mutex ) {
		pthread_mutex_unlock( &mutex );
	}
}


void ZFitterThread::threadStart( ZFitter *_masterCopy, ZFitter *_renderCopy, int _monitorStep ) {
	threadMonitorStep = _monitorStep;
	threadStepFlag = 0;
	threadSendMsg = 0;
	threadPaused = 0;

	masterCopy = _masterCopy;
	renderCopy = _renderCopy;

	// CREATE a mutex for renderable variables
	pthread_mutex_init( &mutex, 0 );

	// LAUNCH a thread to run this fit.
	pthread_attr_t attr;
	pthread_attr_init( &attr );
	pthread_attr_setstacksize( &attr, 1024*4096 );
	pthread_create( &threadID, &attr, &threadMain, (void*)this );
	pthread_attr_destroy( &attr );
}
