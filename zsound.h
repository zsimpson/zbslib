// @ZBS {
//		*MODULE_OWNER_NAME zsound
// }

#ifndef ZSOUND_H
#define ZSOUND_H

int zsoundInit( void *hWnd );

int zsoundPlay( char *filename, int loop=0, float volume=1.f, float pan=0.f, float freq=1.f );
	// In all these functions, volume, pan, and freq are normalized.
	// volume: 0 = no sound, 1.0 = full volume.  Note, this is very non-linear.
	// pan: -1.0 = left, 1.0 = right
	// freq: 1.0 = base frequency, 2.0 = double speed
	// RETURNS a handle that can be used in the following functions or zero.
	// This handle is a unique handle which you own always, even if the
	// sound dies, this handle will not be reused (until wrap around at 2^31!)

void zsoundStop( int sound );

void zsoundGetStats( int sound, int &loop, float &volume, float &pan, float &freq );

void zsoundChange( int sound, float volume=1.f, float pan=0.f, float freq=1.f );
void zsoundChangeVolume( int sound, float volume=1.f );
void zsoundChangePan( int sound, float pan=0.f );
void zsoundChangeFreq( int sound, float freq=1.f );

// For creating dynamic algorithmic sounds, you can setup a callback 
//typedef int (*ZSoundCallbackFuncPtr)( char *buf, int len );
//int zsoundSetupCallbackBuffer( ZSoundCallbackFuncPtr callbackFuncPtr, int sampleRate, int bytesPerSample );
int zsoundPlayBuffer( char *buffer, int bufferBytes, int samplesPerSecond, int bytesPerSample, int loop=0, float volume=1.f, float pan=0.f, float freq=1.f );
	// Remember that this buffer in 8-bit is unsigned with 127 meaning silence



char *zsoundLock( int sound, int offset=0, int len=-1, int *actualLock=0 );
void zsoundUnlock( int sound );
void zsoundGetStatus( int sound, int &readPos, int &writePos, int &playing );
void zsoundStart( int sound, int pos=0, int loop=0 );


#endif