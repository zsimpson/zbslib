// @ZBS {
//		*MODULE_NAME Direct Sound Wrapper
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A fascade for DirectSound.  Provides basic caching and the ability to
//			play a sample more than once without reloading.
//		}
//		*PORTABILITY win32
//		*WIN32_LIBS_DEBUG winmm.lib
//		*WIN32_LIBS_RELEASE winmm.lib
//      *SDK_DEPENDS dx8
//		*REQUIRED_FILES zsound.cpp zsound.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//			Need to change all the sprintf to use proper hash ints
//		}
//		*SELF_TEST yes console
//		*PUBLISH yes
// }
// OPERATING SYSTEM specific includes:
#define POINTER_64 __ptr64
#include "windows.h"
#include "dsound.h"
// STDLIB includes:
#include "assert.h"
#include "stdio.h"
// MODULE includes:
#include "zsound.h"
// ZBSLIB includes:
#include "zhashtable.h"

int soundInitialized = 0;

LPDIRECTSOUND directSound = NULL;

// Sound File
/////////////////////////////////////////////////////////////////////////////

int zsoundFileOpen( char *fileName, int &length, HMMIO &hmmio, WAVEFORMATEX &wfFormat, DWORD &dataOffset ) { 
    char        szFileName[256];    // filename of file to open 
    MMCKINFO    mmckinfoParent;     // parent chunk information 
    MMCKINFO    mmckinfoSubchunk;   // subchunk information structure 
    DWORD       dwFmtSize;          // size of "FMT" chunk 
    DWORD       dwDataSize;         // size of "DATA" chunk 

	strcpy( szFileName, fileName );

    // Open the file for reading with buffered I/O 
    // by using the default internal buffer 
    if(!(hmmio = mmioOpen(szFileName, NULL, MMIO_READ | MMIO_ALLOCBUF))) {
		return 0;
	}
 
    // Locate a "RIFF" chunk with a "WAVE" form type to make 
    // sure the file is a waveform-audio file. 
    mmckinfoParent.fccType = mmioFOURCC('W', 'A', 'V', 'E'); 
    if( mmioDescend(hmmio, (LPMMCKINFO) &mmckinfoParent, NULL, MMIO_FINDRIFF) ) {
        mmioClose(hmmio, 0); 
        return 0; 
    } 
    // Find the "FMT" chunk (form type "FMT"); it must be 
    // a subchunk of the "RIFF" chunk. 
    mmckinfoSubchunk.ckid = mmioFOURCC('f', 'm', 't', ' '); 
    if( mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoParent, MMIO_FINDCHUNK)) {
        mmioClose(hmmio, 0); 
        return 0; 
    } 
 
    // Get the size of the "FMT" chunk. Allocate 
    // and lock memory for it. 
    dwFmtSize = mmckinfoSubchunk.cksize; 
	dwFmtSize = min (dwFmtSize, sizeof (WAVEFORMATEX));
    // Read the "FMT" chunk. 
    if( mmioRead(hmmio, (HPSTR) &wfFormat, dwFmtSize) != (LONG)dwFmtSize ) {
        mmioClose(hmmio, 0); 
        return false; 
    }
	else {
		// Derive data from WAVEFORMAT to WAVEFORMATEX
		if (dwFmtSize < sizeof (WAVEFORMATEX)) {
			assert (wfFormat.wFormatTag == WAVE_FORMAT_PCM);
			wfFormat.wBitsPerSample = 
				(int)(8 * wfFormat.nAvgBytesPerSec) /
				((int)wfFormat.nChannels * (int)wfFormat.nSamplesPerSec);
			wfFormat.cbSize = 0;
		}
	}
 
    // Ascend out of the "FMT" subchunk. 
    mmioAscend( hmmio, &mmckinfoSubchunk, 0 ); 
 
    // Find the data subchunk. The current file position should be at 
    // the beginning of the data chunk; however, you should not make 
    // this assumption. Use mmioDescend to locate the data chunk. 
    mmckinfoSubchunk.ckid = mmioFOURCC('d', 'a', 't', 'a'); 
    if( mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoParent, MMIO_FINDCHUNK) ) {
        mmioClose(hmmio, 0); 
        return 0;
    } 
 
    // Get the size of the data subchunk. 
    dwDataSize = mmckinfoSubchunk.cksize; 
    if( dwDataSize == 0L ) { 
        mmioClose(hmmio, 0); 
        return 0; 
    } 
	length = dwDataSize;
	dataOffset = mmckinfoSubchunk.dwDataOffset;
	return 1;
}

int zsoundFileRead( char *lpData, int dwDataSize, HMMIO &hmmio ) {
    // Read the waveform-audio data subchunk. 
    if( mmioRead( hmmio, (HPSTR) lpData, dwDataSize ) != dwDataSize ) { 
        mmioClose( hmmio, 0 ); 
        return 0;
    } 
	return 1;
}

int zsoundFileSeek( DWORD dataOffset, HMMIO &hmmio ) {
	// Seek the file to the given offset, from its start.
	if( mmioSeek( hmmio, dataOffset, SEEK_SET) == -1 ) {
        mmioClose( hmmio, 0 ); 
        return 0;
	}
	return 1;
}

void zsoundFileClose( HMMIO &hmmio ) {
    // Close the file. 
    mmioClose( hmmio, 0 );
	hmmio = NULL;
}


/////////////////////////////////////////////////////////////////////////////
// Sound Interface
/////////////////////////////////////////////////////////////////////////////

int zsoundInit( void *hWnd ) {
	if( soundInitialized ) {
		return 0;
	}

	soundInitialized = 0;
	WAVEFORMATEX theWaveFormat;
	DSBUFFERDESC bufferDesc;
	if( SUCCEEDED(DirectSoundCreate(NULL, &directSound, NULL)) ) {
		if( SUCCEEDED(directSound->SetCooperativeLevel((HWND)hWnd, DSSCL_PRIORITY)) ) {
			// Set up wave format structure for primary buffer
			memset(&theWaveFormat, 0, sizeof(WAVEFORMATEX));
			theWaveFormat.wFormatTag       = WAVE_FORMAT_PCM;
			theWaveFormat.nChannels        = 1;
			theWaveFormat.nSamplesPerSec   = 44100;	
			theWaveFormat.nBlockAlign      = 2;
			theWaveFormat.wBitsPerSample   = 16;
			theWaveFormat.nAvgBytesPerSec  = theWaveFormat.nSamplesPerSec * theWaveFormat.nBlockAlign;

			// DSBUFFERDESC structure
			// lpwxFormat must be NULL for primary buffers at setup time
			memset( &bufferDesc, 0, sizeof(DSBUFFERDESC) );
			bufferDesc.dwSize            = sizeof(DSBUFFERDESC);
			bufferDesc.dwFlags           = DSBCAPS_PRIMARYBUFFER;
			bufferDesc.dwBufferBytes     = 0;
			bufferDesc.lpwfxFormat       = NULL;

			// CREATE the primary sound buffer
			LPDIRECTSOUNDBUFFER primaryBuffer = NULL;
			if( SUCCEEDED (directSound->CreateSoundBuffer(&bufferDesc, &primaryBuffer, NULL)) ) {
				// Set primary buffer to desired output format
				if( SUCCEEDED(primaryBuffer->SetFormat(&theWaveFormat)) ) {
					soundInitialized = true;
				}
			}
		}
	}

	if( !soundInitialized ) {
		if( directSound ) {
			directSound->Release();
			directSound = NULL;
		}
	}

	return soundInitialized;
}

void zsoundUninit() {
	if( directSound ) {
		directSound->Release();
	}
}

void zsoundShutdown() {
	if( !soundInitialized ) {
		return;
	}

	soundInitialized = 0;
	if( directSound ) {
		directSound->Release();
		directSound = NULL;
	}
}

struct SoundPtr {
	// This class maintains the pointers to the
	// various allocated sounds so that we don't reallocate
	// a sound that has laready been loaded.  To support
	// one sound playing multiple instances, a linked list
	// is maintained to each instance

	SoundPtr *nextIdentical;
	LPDIRECTSOUNDBUFFER buffer;
	int bufferSize;
	int baseFrequency;
	void *lockPtr;
	unsigned long lockActualSize;

	SoundPtr( SoundPtr *last, int size ) {
		if( last ) {
			last->nextIdentical = this;
		}
		nextIdentical = NULL;
		buffer = NULL;
		bufferSize = size;
	}

	int isPlaying() {
		if( buffer ) {
			unsigned long status;
			buffer->GetStatus( &status );
			return (int)(status & DSBSTATUS_PLAYING) != 0;
		}
		return 0;
	}

	int isLooping() {
		if( buffer ) {
			unsigned long status;
			buffer->GetStatus( &status );
			return (int)(status & DSBSTATUS_LOOPING) != 0;
		}
		return 0;
	}

	void getStats( int &_loop, float &_volume, float &_pan, float &_freq ) {
		if( buffer ) {
			unsigned long status;
			buffer->GetStatus( &status );
			_loop = (int)(status & DSBSTATUS_LOOPING) != 0;

			long volume;
			buffer->GetVolume( &volume );
			_volume = (float)(volume+5000) / 5000.f;

			long pan;			
			buffer->GetPan( &pan );
			_pan = (float)(pan) / 10000.f;

			unsigned long freq;			
			buffer->GetFrequency( &freq );
			_freq = (float)freq / (float)baseFrequency;
		}
	}

	void setStats( float _volume, float _pan, float _freq ) {
		if( buffer ) {
			int volume = (int)max( -10000.f, min( 0.f, _volume*5000.f - 5000.f ) );
			buffer->SetVolume( volume );

			int pan = (int)max( -10000.f, min( 10000.f, _pan*10000.f ) );
			buffer->SetPan( pan );

			int freq = (int)max( 100.f, min( 100000.f, baseFrequency * _freq ) );
			buffer->SetFrequency( freq );
		}
	}

	void play( int _loop, float _volume, float _pan, float _freq ) {
		if( buffer ) {
			setStats( _volume, _pan, _freq );
			buffer->Play( 0, 0, (_loop?DSBPLAY_LOOPING:0) );
		}
	}

	void stop() {
		if( buffer ) {
			buffer->Stop();
			buffer->SetCurrentPosition( 0 );
		}
	}
};

ZHashTable soundHash;
ZHashTable soundHashByUniqHandle;
int soundNextUniqHandle = 1;

int zsoundPlay( char *filename, int loop, float volume, float pan, float freq ) {
	if( ! soundInitialized ) {
		return 0;
	}

	// Look to see if there is a copy that has already been
	// allocated that isn't playing.  Otherwise allocate one.
	SoundPtr *found = (SoundPtr *)soundHash.getI( filename );

	// Go through each instance of this sound and see if
	// there are any instances which are not playing
	SoundPtr *lastFound = NULL;
	for( ; found; found=found->nextIdentical ) {
		if( !found->isPlaying() ) {
			found->play( loop, volume, pan, freq );
			char key[10];
			sprintf( key, "%d", ++soundNextUniqHandle );
			soundHashByUniqHandle.putI( key, (int)found );
			return soundNextUniqHandle;
		}
		lastFound = found;
	}

	int size = 0;
	HMMIO hmmio;
	WAVEFORMATEX wfFormat;
	DWORD dataOffset;
	int ret = zsoundFileOpen( filename, size, hmmio, wfFormat, dataOffset );
	if( !ret ) return 0;

	// Set up DSBUFFERDESC structure
	// lpwxFormat must be NULL for primary buffers at setup time
	DSBUFFERDESC bufferDesc;
	memset(&bufferDesc, 0, sizeof(DSBUFFERDESC));
	bufferDesc.dwSize            = sizeof(DSBUFFERDESC);
	bufferDesc.dwFlags           = (DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_STATIC);
	bufferDesc.dwBufferBytes     = size;
	bufferDesc.lpwfxFormat       = &wfFormat;

	// A non-playing sound was not found.  Allocate and load again
	SoundPtr *soundPtr = new SoundPtr( lastFound, size );
	if( !lastFound ) {
		soundHash.putI( filename, (int)soundPtr );
	}

	// Try to create secondary sound buffer
	soundPtr->buffer = NULL;
	int ok = directSound->CreateSoundBuffer(&bufferDesc, &soundPtr->buffer, NULL);
	if( !(SUCCEEDED(ok)) ) {
		return 0;
	}

	// Lock buffer
	void *destPtr = NULL;
	unsigned long actualSize;
	void *dummy1;
	unsigned long dummy2;
	if( !SUCCEEDED(soundPtr->buffer->Lock(0, size, &destPtr, &actualSize, &dummy1, &dummy2, 0)) ) {
		return 0;
	}
	else if( (int)actualSize != size ) {
		return 0;
	}

	ret = zsoundFileRead( (char*)destPtr, size, hmmio );
	assert( ret );
	soundPtr->buffer->Unlock( destPtr, actualSize, dummy1, dummy2 );
	zsoundFileClose( hmmio );

	soundPtr->baseFrequency = wfFormat.nSamplesPerSec;
	soundPtr->play( loop, volume, pan, freq );

	char buf[10];
	sprintf( buf, "%d",++soundNextUniqHandle );
	soundHashByUniqHandle.putI( buf, (int)soundPtr );
	return soundNextUniqHandle;
}

void zsoundStop( int sound ) {
	char buf[10];
	sprintf( buf, "%d", sound );
	SoundPtr *soundPtr = (SoundPtr *)soundHashByUniqHandle.getI( buf );
	if( soundPtr ) {
		soundPtr->stop();
	}
}

void zsoundGetStats( int sound, int &loop, float &volume, float &pan, float &freq ) {
	char buf[10];
	sprintf( buf, "%d", sound );
	SoundPtr *soundPtr = (SoundPtr *)soundHashByUniqHandle.getI( buf );
	if( soundPtr ) {
		soundPtr->getStats( loop, volume, pan, freq );
	}
}

void zsoundChange( int sound, float volume, float pan, float freq ) {
	char buf[10];
	sprintf( buf, "%d", sound );
	SoundPtr *soundPtr = (SoundPtr *)soundHashByUniqHandle.getI( buf );
	if( soundPtr ) {
		soundPtr->setStats( volume, pan, freq );
	}
}

void zsoundChangeVolume( int sound, float volume ) {
	char buf[10];
	sprintf( buf, "%d", sound );
	SoundPtr *soundPtr = (SoundPtr *)soundHashByUniqHandle.getI( buf );
	if( soundPtr ) {
		float _volume, _pan, _freq;
		int _loop;
		soundPtr->getStats( _loop, _volume, _pan, _freq );
		soundPtr->setStats( volume, _pan, _freq );
	}
}

void zsoundChangePan( int sound, float pan ) {
	char buf[10];
	sprintf( buf, "%d", sound );
	SoundPtr *soundPtr = (SoundPtr *)soundHashByUniqHandle.getI( buf );
	if( soundPtr ) {
		float _volume, _pan, _freq;
		int _loop;
		soundPtr->getStats( _loop, _volume, _pan, _freq );
		soundPtr->setStats( _volume, pan, _freq );
	}
}

void zsoundChangeFreq( int sound, float freq ) {
	char buf[10];
	sprintf( buf, "%d", sound );
	SoundPtr *soundPtr = (SoundPtr *)soundHashByUniqHandle.getI( buf );
	if( soundPtr ) {
		float _volume, _pan, _freq;
		int _loop;
		soundPtr->getStats( _loop, _volume, _pan, _freq );
		soundPtr->setStats( _volume, _pan, freq );
	}
}

/*

struct ZSoundCallbackDesc {
	DWORD threadId;
	ZSoundCallbackFuncPtr callbackFuncPtr;
	LPDIRECTSOUNDBUFFER dsoundBuffer;
	WAVEFORMATEX waveFormat;
	DSBUFFERDESC bufferDesc;
};

static ZSoundCallbackDesc soundCallbackDescs[1];
	// @TODO this will be a list

void zsoundCallbackThread( DWORD param ) {
	ZSoundCallbackDesc *callbackDesc = (ZSoundCallbackDesc *)param;

	int initialized = 0;
	int writePos = 0;
	int readPos = 0;

	int toLock = callbackDesc->waveFormat.nAvgBytesPerSec / 10;
		// For now, lock 100ms

	while( 1 ) {
		// QUERY how close is the read to the write?  If it is within
		// 50ms then we need to go stuff more data into the buffer
		callbackDesc->dsoundBuffer->GetCurrentPosition( (DWORD *)&readPos, NULL );
		while( readPos > writePos ) readPos -= callbackDesc->bufferDesc.dwBufferBytes;

		while( writePos - readPos < toLock ) {
			char *buf0=0, *buf1=0;
			DWORD len0=0, len1=0;

			callbackDesc->dsoundBuffer->Lock( writePos, toLock, (void**)&buf0, &len0, NULL, NULL, 0 );

			int wrote0 = 0, wrote1 = 0;
			wrote0 = (*callbackDesc->callbackFuncPtr)( buf0, len0 );

			writePos = ( writePos + wrote0 + wrote1 ) % callbackDesc->bufferDesc.dwBufferBytes;

			callbackDesc->dsoundBuffer->Unlock( buf0, wrote0, buf1, wrote1 );
		}

		if( !initialized ) {
			initialized++;
			callbackDesc->dsoundBuffer->Play( 0, 0, DSBPLAY_LOOPING );
		}

		Sleep( 1 );
	}
}

int zsoundSetupCallbackBuffer( ZSoundCallbackFuncPtr callbackFuncPtr, int sampleRate, int bytesPerSample ) {
	ZSoundCallbackDesc *callbackDesc = &soundCallbackDescs[0];
		// @todo this needs to be allocaing from a list

	callbackDesc->callbackFuncPtr = callbackFuncPtr;

	// ALLOCATE a dsound secondary buffer
	memset(&callbackDesc->waveFormat, 0, sizeof(callbackDesc->waveFormat));
	callbackDesc->waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	callbackDesc->waveFormat.nChannels = 1;
	callbackDesc->waveFormat.nSamplesPerSec = sampleRate;
	callbackDesc->waveFormat.nAvgBytesPerSec = sampleRate * bytesPerSample;
	callbackDesc->waveFormat.nBlockAlign = bytesPerSample;
	callbackDesc->waveFormat.wBitsPerSample = bytesPerSample * 8;
	callbackDesc->waveFormat.cbSize = 0;

	memset(&callbackDesc->bufferDesc, 0, sizeof(DSBUFFERDESC));
	callbackDesc->bufferDesc.dwSize = sizeof(DSBUFFERDESC);
	callbackDesc->bufferDesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
	callbackDesc->bufferDesc.dwBufferBytes = sampleRate * bytesPerSample;
	callbackDesc->bufferDesc.lpwfxFormat = &callbackDesc->waveFormat;

	// Try to create secondary sound buffer
	if( !(SUCCEEDED(directSound->CreateSoundBuffer(&callbackDesc->bufferDesc, &callbackDesc->dsoundBuffer, NULL))) ) {
		return 0;
	}

	// CREATE a thread to fill it
	HANDLE result = CreateThread(
		NULL,
		4096*2,
		(LPTHREAD_START_ROUTINE)zsoundCallbackThread,
		callbackDesc,
		0,
		&callbackDesc->threadId
	);
	return 1;
}


*/

int zsoundPlayBuffer( char *buffer, int bufferBytes, int samplesPerSecond, int bytesPerSample, int loop, float volume, float pan, float freq ) {
	if( ! soundInitialized ) {
		return 0;
	}

	// Set up DSBUFFERDESC structure
	// lpwxFormat must be NULL for primary buffers at setup time
	WAVEFORMATEX wfFormat;
	wfFormat.wFormatTag = WAVE_FORMAT_PCM;
	wfFormat.nChannels = 1;
	wfFormat.nSamplesPerSec = samplesPerSecond;
	wfFormat.nAvgBytesPerSec = samplesPerSecond * bytesPerSample;
	wfFormat.nBlockAlign = bytesPerSample;
	wfFormat.wBitsPerSample = bytesPerSample * 8;
	wfFormat.cbSize = 0;

	DSBUFFERDESC bufferDesc;
	memset( &bufferDesc, 0, sizeof(DSBUFFERDESC) );
	bufferDesc.dwSize            = sizeof(DSBUFFERDESC);
	bufferDesc.dwFlags           = (DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_STATIC);
	bufferDesc.dwBufferBytes     = bufferBytes;
	bufferDesc.lpwfxFormat       = &wfFormat;

	SoundPtr *soundPtr = new SoundPtr( 0, bufferBytes );

	// Try to create secondary sound buffer
	soundPtr->buffer = NULL;
	if( !(SUCCEEDED(directSound->CreateSoundBuffer(&bufferDesc, &soundPtr->buffer, NULL))) ) {
		return 0;
	}

	// Lock buffer
	void *destPtr = NULL;
	unsigned long actualSize = bufferBytes;
	void *dummy1;
	unsigned long dummy2;
	if( !SUCCEEDED(soundPtr->buffer->Lock( 0, bufferBytes, &destPtr, &actualSize, &dummy1, &dummy2, 0)) ) {
		return 0;
	}
	else if( (int)actualSize != bufferBytes ) {
		return 0;
	}

	memcpy( destPtr, buffer, bufferBytes );

	soundPtr->buffer->Unlock( destPtr, actualSize, dummy1, dummy2 );
	soundPtr->baseFrequency = wfFormat.nSamplesPerSec;
	soundPtr->play( loop, volume, pan, freq );

	char buf[10];
	sprintf( buf, "%d",++soundNextUniqHandle );
	soundHashByUniqHandle.putI( buf, (int)soundPtr );
	return soundNextUniqHandle;
}


char *zsoundLock( int sound, int offset, int len, int *actualLock ) {
	// @TODO: Change to use proper hash ints
	char buf[10];
	sprintf( buf, "%d", sound );
	SoundPtr *soundPtr = (SoundPtr *)soundHashByUniqHandle.getI( buf );
	if( soundPtr ) {
		void *destPtr = NULL;
		unsigned long actualSize;
		void *dummy1;
		unsigned long dummy2;

		if( len < 0 ) {
			len = soundPtr->bufferSize;
		}

		if( SUCCEEDED(soundPtr->buffer->Lock(0, len, &destPtr, &actualSize, &dummy1, &dummy2, 0)) ) {
			if( actualLock ) *actualLock = actualSize;

			soundPtr->lockPtr = destPtr;
			soundPtr->lockActualSize = actualSize;

			return (char *)destPtr;
		}
		else {
			if( actualLock ) *actualLock = 0;
			return 0;
		}
	}
	return 0;
}

void zsoundUnlock( int sound ) {
	char buf[10];
	sprintf( buf, "%d", sound );
	SoundPtr *soundPtr = (SoundPtr *)soundHashByUniqHandle.getI( buf );
	if( soundPtr ) {
		void *dummy1 = 0;
		unsigned long dummy2 = 0;
		soundPtr->buffer->Unlock( soundPtr->lockPtr, soundPtr->lockActualSize, dummy1, dummy2 );
	}
}

void zsoundGetStatus( int sound, int &readPos, int &writePos, int &playing ) {
	readPos = 0;
	writePos = 0;
	playing = 0;

	char buf[10];
	sprintf( buf, "%d", sound );
	SoundPtr *soundPtr = (SoundPtr *)soundHashByUniqHandle.getI( buf );
	if( soundPtr ) {
		if( soundPtr->isPlaying() ) {
			soundPtr->buffer->GetCurrentPosition( (DWORD *)&readPos, (DWORD *)&writePos );
		}
	}
}

void zsoundStart( int sound, int pos, int loop ) {
	char buf[10];
	sprintf( buf, "%d", sound );
	SoundPtr *soundPtr = (SoundPtr *)soundHashByUniqHandle.getI( buf );
	if( soundPtr ) {
		int ret = soundPtr->buffer->Play( 0, 0, (loop?DSBPLAY_LOOPING:0) );

		int a = soundPtr->isPlaying();

		soundPtr->buffer->SetCurrentPosition( pos );
	}
}

#ifdef SELF_TEST

void *getConsoleHwnd() {
	#ifdef WIN32
		#define MY_BUFSIZE 1024 // buffer size for console window titles
		HWND hwndFound;         // this is what is returned to the caller
		char pszNewWindowTitle[MY_BUFSIZE]; // contains fabricated WindowTitle
		char pszOldWindowTitle[MY_BUFSIZE]; // contains original WindowTitle
 
		// fetch current window title
		GetConsoleTitle(pszOldWindowTitle, MY_BUFSIZE);
 
		// format a "unique" NewWindowTitle
		wsprintf( pszNewWindowTitle,"%d/%d",GetTickCount(),GetCurrentProcessId() );
 
		// change current window title
		SetConsoleTitle( pszNewWindowTitle );
 
		// ensure window title has been updated
		Sleep(40);
 
		// look for NewWindowTitle
		hwndFound = FindWindow( NULL, pszNewWindowTitle );
 
		// restore original window title
		SetConsoleTitle( pszOldWindowTitle );
 
		return hwndFound;
	#else
		return NULL;
	#endif
}

#include "zwildcard.h"

void zsoundStatus( int &cached, int &cachedUnique, int &cachedBytes, int &playing ) {
	cached = 0;
	cachedUnique = 0;
	cachedBytes = 0;
	playing = 0;
	for( int i=0; i<soundHash.size(); i++ ) {
		char *k = soundHash.getKey(i);
		SoundPtr *v = (SoundPtr *)soundHash.getValI(i);
		if( k ) {
			cachedUnique++;
			for( ; v; v=v->nextIdentical ) {
				cached++;
				cachedBytes += v->bufferSize;
				playing += v->isPlaying() ? 1 : 0;
			}
		}
	}
}

void main() {
	try {
		zsoundInit( getConsoleHwnd() );
	
		char files[1024][128];
		int fileCount = 0;
		int handles[100000] = {0,};
		int count = 0;

		ZWildcard w( "/prj/sg/sound/*.wav" );
		while( w.next() ) {
			strcpy( files[fileCount++], w.getFullName() );
		}

		while( 1 ) {
			//if( !(rand() % 10) ) {
				int r = rand() % fileCount;
				handles[count++] = zsoundPlay( files[r], 0 );
			//}
			int cached, cachedUnique, cachedBytes, playing;
			zsoundStatus( cached, cachedUnique, cachedBytes, playing );
			printf( "%d %d %d %d %d %d\n", cached, cachedUnique, cachedBytes, playing, count, r );
	//		if( timer.isDone() ) {
	//			for( int i=0; i<count; i++ ) {
	//				
	//			}
	//		}

			Sleep( 20 );
		}
	}
	catch( ... ) {
		zsoundUninit();
	}
}


#endif