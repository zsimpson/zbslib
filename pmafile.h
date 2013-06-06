#ifndef PMAFILE_H
#define PMAFILE_H

#include "zbits.h"

// I want to keep this interface as simple as possible
// So I don't want to have to include the bitmap definitions

struct VidFile {
	int channels;
		// Number of color channels
	int channelDepthInBytes;
		// Depth of channels in bytes
	int w, h;
		// In pixels, not bytes
	int numFrames;
		// Number of frames (read mode)

	// OPEN / CLOSE
	//--------------------------------------------------------------------
	virtual int openRead( char *filename, int startFrame, int frameCount=-1, float *percentMonitor=0, int *haltSignal=0 )=0;
		// Open for read.
	virtual void preallocWrite( unsigned int preallocMemory, int preallocFrames )=0;
		// optionally preallocating frames
	virtual int openWrite( char *filename )=0;
		// Open for write
	virtual void close()=0;
		// Close the file, writing any preallocated frames

	// FETCH POINTERS
	//--------------------------------------------------------------------
	virtual unsigned char *getFrameU1( int frame, int line=0 )=0;
		// Fetch pointer to unsigned char
	virtual unsigned short *getFrameU2( int frame, int line=0 )=0;
		// Fetch pointer to unsigned short

	// WRITE POINTERS
	//--------------------------------------------------------------------
	virtual void writeFrame( unsigned int timeStamp, void *src, int bytePitch, int *extraInfo, int extraInfoLen )=0;
		// Write a frame, if there is a preallocation buffer then it writes to that

	// INFO
	//--------------------------------------------------------------------
	virtual unsigned int getFrameTime( int i )=0;
		// Fetch optional time-stamp
	virtual unsigned int writeMemFramesRemain()=0;
		// How many frames remain in the pre-allocated buffer
	virtual int isRecording()=0;
		// True if opened for recording
	virtual int getInfoI( char *infoStr )=0;
		// Fetch format specific data record
	virtual void setInfoI( char *infoStr, int val )=0;
		// Put format specific data record
	virtual void setMetaData( char *string )=0;
		// Attach arbitrary metadata in the form of a string
	virtual char *getMetaData()=0;
		// Retrieve arbitrary metadata in the form of a string

	// CONSTRUCT / DESTRUCT
	//--------------------------------------------------------------------
	virtual void clear() { channels = channelDepthInBytes = w = h = numFrames = 0; }
	VidFile() { clear(); }
	virtual ~VidFile() { }
};


struct SeqVidFile : VidFile {
	char *filenamePattern;
	ZBits *frameBits;
		// An array of the ZBits from each frame

	virtual int openRead( char *filename, int startFrame, int frameCount=-1, float *percentMonitor=0, int *haltSignal=0 );
	virtual void preallocWrite( unsigned int preallocMemory, int preallocFrames );
	virtual int openWrite( char *filename );
		// These filenames are really patterns in printf format like "dir/file%04d.png"

	virtual void close();

	virtual unsigned char *getFrameU1( int frame, int line=0 );
	virtual unsigned short *getFrameU2( int frame, int line=0 );
	virtual void writeFrame( unsigned int timeStamp, void *src, int bytePitch, int *extraInfo, int extraInfoLen );
	virtual unsigned int getFrameTime( int i );
	virtual unsigned int writeMemFramesRemain();
	virtual int isRecording();
	virtual int getInfoI( char *infoStr );
	virtual void setInfoI( char *infoStr, int val );
//	virtual void setMetaData( char *string );
//	virtual char *getMetaData();
	virtual ~SeqVidFile();
	SeqVidFile();
};


struct PMAVidFile : VidFile {
	#define PMADATA_FILE_HEADER_BYTES_VER1 (4)
	#define PMADATA_FRAME_HEADER_BYTES_VER1 (8)
	#define PMADATA_PIXEL_DEPTH_VER1 (2)

	#define PMADATA_FILE_HEADER_BYTES_VER2 (12+30+2)
	#define PMADATA_FRAME_HEADER_BYTES_VER2 (8+16)
	#define PMADATA_PIXEL_DEPTH_VER2 (2)

	static const int METADATA_BYTES = 2048;
	#define PMADATA_FILE_HEADER_BYTES_VER4 (12+2048+2)

	int x, y;
	int bin;
	int writeFrameNum;
	int writeAllocedFrames;
	unsigned short *data;
	unsigned int allocatedMemory;
	unsigned int *frameTimes;
	unsigned int *frameStates;
	void *recordFile;
	int blockWriting;
	int prealloc;
	char metaData[METADATA_BYTES];

	virtual int openRead( char *filename, int startFrame, int frameCount=-1, float *percentMonitor=0, int *haltSignal=0 );
	virtual void preallocWrite( unsigned int preallocMemory, int preallocFrames );
	virtual int openWrite( char *filename );
	virtual void close();
	virtual unsigned char *getFrameU1( int frame, int line=0 );
	virtual unsigned short *getFrameU2( int frame, int line=0 );
	virtual void writeFrame( unsigned int timeStamp, void *src, int bytePitch, int *extraInfo, int extraInfoLen );
	virtual unsigned int getFrameTime( int i );
	virtual unsigned int writeMemFramesRemain();
	virtual int isRecording();
	virtual int getInfoI( char *infoStr );
	virtual void setInfoI( char *infoStr, int val );
	virtual void setMetaData( char *string );
	virtual char *getMetaData();
	PMAVidFile();
	virtual ~PMAVidFile();
};

int pmaFileSplit( char *filename );

#endif