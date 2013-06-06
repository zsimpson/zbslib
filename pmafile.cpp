// @ZBS {
//		+DESCRIPTION {
//			Video file read and write wrapper
//		}
//		*REQUIRED_FILES pmafile.h pmafile.cpp
// }

// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "sys/stat.h"
#include "stdio.h"
#include "assert.h"
#include "stdlib.h"
#include "memory.h"
#include "string.h"
// MODULE includes:
#include "pmafile.h"
// ZBSLIB includes:
#include "zbits.h"
#include "zgraphfile.h"
#include "ztime.h"
#include "zfilespec.h"
#include "ztmpstr.h"


#define min(a,b) ((a)<(b)?(a):(b))

int SeqVidFile::openRead( char *filename, int startFrame, int frameCount, float *percentMonitor, int *haltSignal ) {
	filenamePattern = strdup( filename );

	// COUNT how many files there are that match the pattern
	numFrames = 0;
	while( 1 ) {
		struct stat s;
		char buf[256];
		sprintf( buf, filename, numFrames );
		if( ! stat( buf, &s ) ) {
			numFrames++;
		}
		else {
			break;
		}
		if( haltSignal && *haltSignal ) {
			return 0;
		}
	}

	numFrames = frameCount == -1 ? numFrames : min( numFrames, frameCount );

	frameBits = new ZBits[ numFrames ];

	for( int i=0; i<numFrames; i++ ) {
		printf( "%0.2f%%\r", 100.f * (float)i / (float)numFrames );

		if( percentMonitor ) {
			*percentMonitor = (float)i / (float)numFrames;
		}

		char buf[256];
		sprintf( buf, filename, i+startFrame );

		int ret = zGraphFileLoad( buf, &frameBits[i], 0, 0 );
		assert( ret );

		// ERROR test that all frames are idential.
		if( i == 0 ) {
			// Use the description of the first frame to set the standard
			channels = frameBits[i].channels;
			channelDepthInBytes = frameBits[i].channelDepthInBits / 8;
			w = frameBits[i].w();
			h = frameBits[i].h();
		}
		assert( frameBits[i].channels == channels );
		assert( frameBits[i].channelDepthInBits == channelDepthInBytes*8 );
		assert( frameBits[i].w() == w );
		assert( frameBits[i].h() == h );

		if( haltSignal && *haltSignal ) {
			return 0;
		}
	}
	return numFrames > 0;
}

void SeqVidFile::preallocWrite( unsigned int preallocMemory, int preallocFrames ) {
	// @TODO
	assert( 0 );
}

int SeqVidFile::openWrite( char *filename) {
	// @TODO
	assert( 0 );
	return 0;
}

void SeqVidFile::close() {
	for( int i=0; i<numFrames; i++ ) {
		frameBits[i].clear();
	}
	delete[] frameBits;
	if( filenamePattern ) {
		free( filenamePattern );
		filenamePattern = 0;
	}
}

unsigned char *SeqVidFile::getFrameU1( int frame, int line ) {
	assert( 0 <= frame && frame < numFrames );
	return frameBits[frame].lineU1(line);
}

unsigned short *SeqVidFile::getFrameU2( int frame, int line ) {
	assert( 0 <= frame && frame < numFrames );
	return frameBits[frame].lineU2(line);
}

void SeqVidFile::writeFrame( unsigned int timeStamp, void *src, int bytePitch, int *extraInfo, int extraInfoLen ) {
	// @TODO
	assert( 0 );
}

unsigned int SeqVidFile::getFrameTime( int i ) {
	return 0;
}

unsigned int SeqVidFile::writeMemFramesRemain() {
	// @TODO
	assert( 0 );
	return 0;
}

int SeqVidFile::isRecording() {
	// @TODO
	return 0;
}

int SeqVidFile::getInfoI( char *infoStr ) {
	return 0;
}

void SeqVidFile::setInfoI( char *infoStr, int val ) {
}

SeqVidFile::~SeqVidFile() {
	close();
}

SeqVidFile::SeqVidFile() {
	filenamePattern = 0;
	frameBits = 0;
}


//--------------------------------------------------------------------------

PMAVidFile::PMAVidFile() {
	x = 0;
	y = 0;
	bin = 1;
	writeFrameNum = 0;
	writeAllocedFrames = 0;
	data = 0;
	frameTimes = 0;
	frameStates = 0;
	recordFile = 0;
	blockWriting = 1;
	allocatedMemory = 0;
	prealloc = 0;
	memset(metaData,0,sizeof(metaData));
}

int PMAVidFile::openRead( char *filename, int startFrame, int frameCount, float *percentMonitor, int *haltSignal ) {
	clear();

	FILE *f = fopen( filename, "rb" );
	if( !f ) return 0;

	int version = 0;
	fread( &version, sizeof(version), 1, f );

	if( version == 0x00000003 || version == 0x00000004 ) {
		// This is the new version that has x y and reserved encodings
		unsigned short a = 0;
		fread( &a, 2, 1, f ); x = (int)a;
		fread( &a, 2, 1, f ); y = (int)a;
		fread( &a, 2, 1, f ); w = (int)a;
		fread( &a, 2, 1, f ); h = (int)a;
		fread( &a, 2, 1, f ); bin = (int)a;
			
		// When a file has been recorded bin mode, it reports the w & h to the large size
		// even though the data is compacted by the factor "bin".  So a 512x512 ROI
		// with a bin factor of 2 will be 1/4 of the data but it will still report
		// the w and h as 512 x 512.

		int headerBytes = 0;
		char v3reserved[30];
		if( version == 0x00000003 ) {
			fread( v3reserved, sizeof(v3reserved), 1, f );
			headerBytes = PMADATA_FILE_HEADER_BYTES_VER2;
		}
		if( version == 0x00000004 ) {
			fread( metaData, sizeof(metaData), 1, f );
			headerBytes = PMADATA_FILE_HEADER_BYTES_VER4;
		}

		fseek( f, 0, SEEK_END );
		int fileSize = ftell( f );
		fseek( f, 0, SEEK_SET );

		unsigned int pixelCountPerFrameInFile = (w * h)/(bin*bin);

		numFrames = (fileSize-headerBytes) / (PMADATA_PIXEL_DEPTH_VER2 * pixelCountPerFrameInFile + PMADATA_FRAME_HEADER_BYTES_VER2);
			// The file has two shorts at the beginning that encode w and h
			// Then each frame is prefixed with:
			//   1 int frame number
			//   1 int time in tenths of milliseconds since start of movie

		numFrames = frameCount == -1 ? numFrames : min( numFrames, frameCount );

		data = (unsigned short *)malloc( numFrames * PMADATA_PIXEL_DEPTH_VER2 * w * h );
		frameTimes = (unsigned int *)malloc( numFrames * sizeof(unsigned int) );
		frameStates = (unsigned int *)malloc( numFrames * sizeof(unsigned int) );
		unsigned short *tail = &data[ numFrames * w * h ];

		unsigned short tempLine[1024];

		for( int i=0; i<numFrames; i++ ) {
			printf( "%0.2f%%\r", 100.f * (float)i / (float)numFrames );

			if( percentMonitor ) {
				*percentMonitor = (float)i / (float)numFrames;
			}

			fseek( f, (startFrame+i) * (PMADATA_FRAME_HEADER_BYTES_VER2 + PMADATA_PIXEL_DEPTH_VER2 * pixelCountPerFrameInFile) + headerBytes, SEEK_SET );
			int frameNum;
			fread( &frameNum, sizeof(frameNum), 1, f );
			assert( frameNum == startFrame+i );
			fread( &frameTimes[i], sizeof(frameTimes[i]), 1, f );
			fread( &frameStates[i], sizeof(frameStates[i]), 1, f );
			char reserved[16-4];
			fread( reserved, sizeof(reserved), 1, f );
			unsigned short *c = getFrameU2(i);
			assert( c+w*h <= tail );

			for( int y=0; y<h; y += bin ) {
				// READ in each line into a temp buffer and expand the bin
				fread( tempLine, PMADATA_PIXEL_DEPTH_VER2 * w / bin, 1, f );
				
				for( int yb=0; yb<bin; yb++ ) {

					//EXPAND
					unsigned short *dst = getFrameU2(i,y+yb);
					unsigned short *src = tempLine;
					for( int x=0; x<w; x += bin ) {
						for( int xb=0; xb<bin; xb++ ) {
							*dst++ = *src;
						}
						src++;
					}
				}
				if( haltSignal && *haltSignal ) {
					fclose(f);
					return 0;
				}
			}
		}
	}
	else if( version == 0x00000002 ) {
		// This is the new version that has x y and reserved encodings
		unsigned short a = 0;
		fread( &a, 2, 1, f ); x = (int)a;
		fread( &a, 2, 1, f ); y = (int)a;
		fread( &a, 2, 1, f ); w = (int)a;
		fread( &a, 2, 1, f ); h = (int)a;

		char reserved[32];
		fread( reserved, sizeof(reserved), 1, f );

		fseek( f, 0, SEEK_END );
		int fileSize = ftell( f );
		fseek( f, 0, SEEK_SET );

		numFrames = (fileSize-PMADATA_FILE_HEADER_BYTES_VER2) / (PMADATA_PIXEL_DEPTH_VER2 * w * h + PMADATA_FRAME_HEADER_BYTES_VER2);
			// The file has two shorts at the beginning that encode w and h
			// Then each frame is prefixed with:
			//   1 int frame number
			//   1 int time in tenths of milliseconds since start of movie

		numFrames = frameCount == -1 ? numFrames : min( numFrames, frameCount );

		data = (unsigned short *)malloc( numFrames * PMADATA_PIXEL_DEPTH_VER2 * w * h );
		frameTimes = (unsigned int *)malloc( numFrames * sizeof(unsigned int) );
		frameStates = (unsigned int *)malloc( numFrames * sizeof(unsigned int) );
		unsigned short *tail = &data[ numFrames * w * h ];

		for( int i=0; i<numFrames; i++ ) {
			printf( "%0.2f%%\r", 100.f * (float)i / (float)numFrames );

			if( percentMonitor ) {
				*percentMonitor = (float)i / (float)numFrames;
			}

			fseek( f, (startFrame+i) * (PMADATA_FRAME_HEADER_BYTES_VER2 + PMADATA_PIXEL_DEPTH_VER2 * w * h) + PMADATA_FILE_HEADER_BYTES_VER2, SEEK_SET );
			int frameNum;
			fread( &frameNum, sizeof(frameNum), 1, f );
			assert( frameNum == startFrame+i );
			fread( &frameTimes[i], sizeof(frameTimes[i]), 1, f );
			fread( &frameStates[i], sizeof(frameStates[i]), 1, f );
			char reserved[16-4];
			fread( reserved, sizeof(reserved), 1, f );
			unsigned short *c = getFrameU2(i);
			assert( c+w*h <= tail );
			fread( getFrameU2(i), PMADATA_PIXEL_DEPTH_VER2 * w * h, 1, f );

			if( haltSignal && *haltSignal ) {
				fclose(f);
				return 0;
			}

		}
	}
	else {
		// This is the old version before versioing was added and there was
		// no storage of the x/y position.  This is only valid for full sized 512x512 frames
		x = 0;
		y = 0;
		fseek( f, 0, SEEK_SET );
		fread( &w, 2, 1, f );
		fread( &h, 2, 1, f );
		assert( w == 512 && h == 512 );

		fseek( f, 0, SEEK_END );
		int fileSize = ftell( f );
		fseek( f, 0, SEEK_SET );

		numFrames = (fileSize-PMADATA_FILE_HEADER_BYTES_VER1) / (PMADATA_PIXEL_DEPTH_VER1 * w * h + PMADATA_FRAME_HEADER_BYTES_VER1);
			// The file has two shorts at the beginning that encode w and h
			// Then each frame is prefixed with:
			//   1 int frame number
			//   1 int time in tenths of milliseconds since start of movie

		data = (unsigned short *)malloc( numFrames * PMADATA_PIXEL_DEPTH_VER1 * w * h );
		frameTimes = (unsigned int *)malloc( numFrames * sizeof(unsigned int) );
		frameStates = (unsigned int *)malloc( numFrames * sizeof(unsigned int) );
		memset( frameStates, 0, numFrames * sizeof(unsigned int) );

		for( int i=0; i<numFrames; i++ ) {
			if( percentMonitor ) {
				*percentMonitor = (float)i / (float)numFrames;
			}

			fseek( f, (startFrame+i) * (PMADATA_FRAME_HEADER_BYTES_VER1 + PMADATA_PIXEL_DEPTH_VER1 * w * h) + PMADATA_FILE_HEADER_BYTES_VER1, SEEK_SET );
			int frameNum;
			fread( &frameNum, sizeof(frameNum), 1, f );
			assert( frameNum == startFrame+i );
			fread( &frameTimes[i], sizeof(frameTimes[i]), 1, f );
			fread( getFrameU2(i), PMADATA_PIXEL_DEPTH_VER1 * w * h, 1, f );
			if( haltSignal && *haltSignal ) {
				fclose(f);
				return 0;
			}

		}
	}

	fclose( f );
	return 1;
}

void PMAVidFile::preallocWrite( unsigned int preallocMemory, int preallocFrames ) {
	// @TODO: I think that all the write buffers need to be separated
	// from the read and after a capture then you switch buffers
	if( preallocMemory ) {
		if( data ) {
			free( data );
			data = 0;
		}
		writeAllocedFrames = preallocFrames;

		frameTimes = (unsigned int *)malloc( preallocFrames * sizeof(unsigned int) );
		memset( frameTimes, 0, preallocFrames * sizeof(unsigned int) );

		frameStates = (unsigned int *)malloc( preallocFrames * sizeof(unsigned int) );
		memset( frameStates, 0, preallocFrames * sizeof(unsigned int) );

		allocatedMemory = preallocMemory;
		data = (unsigned short *)malloc( preallocMemory );
		memset( data, 0, preallocMemory );

		prealloc = 1;
	}
}

int PMAVidFile::openWrite( char *filename ) {
	recordFile = (void *)fopen( filename, "wb" );
	assert( recordFile );

	int version = 0x00000004;
	fwrite( &version, sizeof(version), 1, (FILE *)recordFile );
	fwrite( &x, 2, 1, (FILE *)recordFile );
	fwrite( &y, 2, 1, (FILE *)recordFile );
	fwrite( &w, 2, 1, (FILE *)recordFile );
	fwrite( &h, 2, 1, (FILE *)recordFile );
	fwrite( &bin, 2, 1, (FILE *)recordFile );

	char v3reserved[30] = {0,};
	if( version == 0x00000003 ) {
		fwrite( &v3reserved, sizeof(v3reserved), 1, (FILE *)recordFile );
	}
	else {
		fwrite( &metaData, sizeof(metaData), 1, (FILE *)recordFile );
	}

	numFrames = 0;

	fflush( (FILE *)recordFile );

	writeFrameNum = 0;
	blockWriting = 0;

	return 1;
}

void PMAVidFile::close() {
	if( recordFile && data ) {
		// Writing was done with preallocated frames so now we need to write them

		blockWriting = 1;
		zTimeSleepMils( 200 );
			// Give time for the other thread to complete (we might need to put in a real mutex here)

		for( int i=0; i<writeFrameNum; i++ ) {
			// WRITE the frame header
			fwrite( &i, sizeof(i), 1, (FILE *)recordFile );
			fwrite( &frameTimes[i], sizeof(frameTimes[i]), 1, (FILE *)recordFile );
			fwrite( &frameStates[i], sizeof(frameStates[i]), 1, (FILE *)recordFile );

			char reserved[16-4] = {0,};
			fwrite( reserved, sizeof(reserved), 1, (FILE *)recordFile  );

			// WRITE the frame bits
			fwrite( (char *)&data[ i * w * h / (bin*bin)], w * h * PMADATA_PIXEL_DEPTH_VER2 / (bin*bin), 1, (FILE *)recordFile );
		}
	}
	if( recordFile ) {
		fclose( (FILE *)recordFile );
		blockWriting = 0;
		recordFile = 0;
	}

	if( !prealloc ) {
		if( data ) {
			free( data );
			data = 0;
		}
		if( frameTimes ) {
			free( frameTimes );
			frameTimes = 0;
		}
		if( frameStates ) {
			free( frameStates );
			frameStates = 0;
		}
	}
}

unsigned char *PMAVidFile::getFrameU1( int frame, int line ) {
	assert( 0 );
	return 0;
}

unsigned short *PMAVidFile::getFrameU2( int frame, int line ) {
	// This is the READ frame which is expanded if there was binning
	assert( line >= 0 && line < h && frame >= 0 && frame < numFrames );
	return &data[ frame * w * h + w * line ];
}

void PMAVidFile::writeFrame( unsigned int timeStamp, void *src, int bytePitch, int *extraInfo, int extraInfoLen ) {
	if( blockWriting ) {
		return;
	}

	int bin2 = bin*bin;

	assert( extraInfo );
	assert( extraInfoLen == sizeof(int) );
	if( data ) {
		// Writing to preallocated
		int totalMemInFrames = (int)( allocatedMemory / (unsigned int)(w*h*PMADATA_PIXEL_DEPTH_VER2/bin2) );
		if( writeFrameNum < writeAllocedFrames && writeFrameNum < totalMemInFrames ) {
			char *_src = (char *)src;
			char *dst = (char *)&data[ w * h * writeFrameNum / bin2 ];
			for( int y=0; y<h; y++ ) {
				memcpy( dst, _src, w * PMADATA_PIXEL_DEPTH_VER2 / bin );
				_src += bytePitch;
				dst += w * PMADATA_PIXEL_DEPTH_VER2 / bin;
			}
			frameTimes[writeFrameNum] = timeStamp;
			frameStates[writeFrameNum] = *extraInfo;
			writeFrameNum++;
		}
	}
	else {
		assert( recordFile );
		char *_src = (char *)src;

		// WRITE the frame header
		fwrite( &writeFrameNum, sizeof(writeFrameNum), 1, (FILE *)recordFile );
		fwrite( &timeStamp, sizeof(timeStamp), 1, (FILE *)recordFile );

		char reserved[16-4] = {0,};
		fwrite( extraInfo, sizeof(int), 1, (FILE *)recordFile  );
		fwrite( reserved, sizeof(reserved), 1, (FILE *)recordFile  );

		// WRITE the data
		for( int y=0; y<h/bin; y++ ) {
			fwrite( (char *)_src, w * PMADATA_PIXEL_DEPTH_VER2 / bin, 1, (FILE *)recordFile );
			_src += bytePitch;
		}
		fflush( (FILE *)recordFile );

		writeFrameNum++;
	}
}

unsigned int PMAVidFile::getFrameTime( int i ) {
	return frameTimes[ i ];
}

unsigned int PMAVidFile::writeMemFramesRemain() {
	int totalMemInFrames = (int)( allocatedMemory / (unsigned int)(w*h*2) );
	int memoryFramesRemains = totalMemInFrames - writeFrameNum;
	int framesRemain = writeAllocedFrames - writeFrameNum;
	
	return memoryFramesRemains < framesRemain ? memoryFramesRemains : framesRemain;
}

int PMAVidFile::isRecording() {
	return data && recordFile != 0;
}

int PMAVidFile::getInfoI( char *infoStr ) {
	if( !strcmp(infoStr,"x") ) {
		return x;
	}
	else if( !strcmp(infoStr,"y") ) {
		return y;
	}
	else if( !strcmp(infoStr,"bin") ) {
		return bin;
	}
	else if( !strcmp(infoStr,"allocatedMemoryLo") ) {
		return allocatedMemory & 0xFFFF;
	}
	else if( !strcmp(infoStr,"allocatedMemoryHi") ) {
		return (allocatedMemory & 0xFFFF0000) >> 16;
	}
	else if( !strncmp(infoStr,"flags-",6) ) {
		int frame = atoi( &infoStr[6] );
		return frameStates[frame];
	}
	return 0;
}

void PMAVidFile::setInfoI( char *infoStr, int val ) {
	if( !strcmp(infoStr,"x") ) {
		x = val;
	}
	else if( !strcmp(infoStr,"y") ) {
		y = val;
	}
	else if( !strcmp(infoStr,"bin") ) {
		bin = val;
	}
}

void PMAVidFile::setMetaData( char *string ) {
	memset(metaData,0,sizeof(metaData));
	strncpy(metaData,string,min(strlen(string)+1,sizeof(metaData)-1));
}

char *PMAVidFile::getMetaData() {
	return metaData;
}

PMAVidFile::~PMAVidFile() {
	close();

	if( frameTimes ) {
		free( frameTimes );
		frameTimes = 0;
	}
	if( frameStates ) {
		free( frameStates );
		frameStates = 0;
	}
	if( data ) {
		free( data );
		data = 0;
	}
}

int pmaFileSplit( char *filename ) { 
	FILE *fin = fopen( filename, "rb" );
	if( !fin ) return 0;

	ZFileSpec spec( filename );
	FILE *fout0 = fopen( ZTmpStr("%s%s%s-0.%s",spec.getDrive(),spec.getDir(),spec.getFile(0),spec.getExt()), "wb" );
	if( !fout0 ) return 0;

	FILE *fout1 = fopen( ZTmpStr("%s%s%s-1.%s",spec.getDrive(),spec.getDir(),spec.getFile(0),spec.getExt()), "wb" );
	if( !fout0 ) return 0;

	int version = 0;
	fread( &version, sizeof(version), 1, fin );

	assert( version == 0x00000003 || version == 0x00000004 );
	
	int x, y, w, h, bin;

	// This is the new version that has x y and reserved encodings
	unsigned short a = 0;
	fread( &a, 2, 1, fin ); x = (int)a;
	fread( &a, 2, 1, fin ); y = (int)a;
	fread( &a, 2, 1, fin ); w = (int)a;
	fread( &a, 2, 1, fin ); h = (int)a;
	fread( &a, 2, 1, fin ); bin = (int)a;

	int headerBytes = 0;
	char v3reserved[30];
	char metaData[PMAVidFile::METADATA_BYTES];
	if( version == 0x00000003 ) {
		fread( v3reserved, sizeof(v3reserved), 1, fin );
		headerBytes = PMADATA_FILE_HEADER_BYTES_VER2;
	}
	if( version == 0x00000004 ) {
		fread( metaData, sizeof(metaData), 1, fin );
		headerBytes = PMADATA_FILE_HEADER_BYTES_VER4;
	}

	fseek( fin, 0, SEEK_END );
	int fileSize = ftell( fin );
	fseek( fin, 0, SEEK_SET );

	unsigned int pixelCountPerFrameInFile = (w * h)/(bin*bin);

	int numFrames = (fileSize-headerBytes) / (PMADATA_PIXEL_DEPTH_VER2 * pixelCountPerFrameInFile + PMADATA_FRAME_HEADER_BYTES_VER2);
		// The file has two shorts at the beginning that encode w and h
		// Then each frame is prefixed with:
		//   1 int frame number
		//   1 int time in tenths of milliseconds since start of movie


	// WRITE fout0
	fwrite( &version, sizeof(version), 1, fout0 );
	fwrite( &x, 2, 1, fout0 );
	fwrite( &y, 2, 1, fout0 );
	fwrite( &w, 2, 1, fout0 );
	fwrite( &h, 2, 1, fout0 );
	fwrite( &bin, 2, 1, fout0 );

	if( version == 0x00000003 ) {
		fwrite( v3reserved, sizeof(v3reserved), 1, fout0 );
		headerBytes = PMADATA_FILE_HEADER_BYTES_VER2;
	}
	if( version == 0x00000004 ) {
		fwrite( metaData, sizeof(metaData), 1, fout0 );
		headerBytes = PMADATA_FILE_HEADER_BYTES_VER4;
	}

	unsigned int frameDataSize = PMADATA_PIXEL_DEPTH_VER2 * pixelCountPerFrameInFile;
	char *oneFrame = (char *)malloc( frameDataSize );

	int i = 0;
	int frameTime, frameState;
	for( i=0; i<numFrames/2; i++ ) {

		fseek( fin, i * (PMADATA_FRAME_HEADER_BYTES_VER2 + PMADATA_PIXEL_DEPTH_VER2 * pixelCountPerFrameInFile) + headerBytes, SEEK_SET );
		fseek( fout0, i * (PMADATA_FRAME_HEADER_BYTES_VER2 + PMADATA_PIXEL_DEPTH_VER2 * pixelCountPerFrameInFile) + headerBytes, SEEK_SET );

		int frameNum;
		fread( &frameNum, sizeof(frameNum), 1, fin );
		assert( frameNum == i );
		fread( &frameTime, sizeof(frameTime), 1, fin );
		fread( &frameState, sizeof(frameState), 1, fin );
		char reserved[16-4];
		fread( reserved, sizeof(reserved), 1, fin );

		fwrite( &frameNum, sizeof(frameNum), 1, fout0 );
		fwrite( &frameTime, sizeof(frameTime), 1, fout0 );
		fwrite( &frameState, sizeof(frameState), 1, fout0 );
		fwrite( reserved, sizeof(reserved), 1, fout0 );

		fread( oneFrame, frameDataSize, 1, fin );
		fwrite( oneFrame, frameDataSize, 1, fout0 );
	}

	// WRITE fout1
	fwrite( &version, sizeof(version), 1, fout1 );
	fwrite( &x, 2, 1, fout1 );
	fwrite( &y, 2, 1, fout1 );
	fwrite( &w, 2, 1, fout1 );
	fwrite( &h, 2, 1, fout1 );
	fwrite( &bin, 2, 1, fout1 );
	if( version == 0x00000003 ) {
		fwrite( v3reserved, sizeof(v3reserved), 1, fout1 );
	}
	if( version == 0x00000004 ) {
		fwrite( metaData, sizeof(metaData), 1, fout1 );
	}

	int startFrame = i;
	for( ; i<numFrames; i++ ) {

		fseek( fin, i * (PMADATA_FRAME_HEADER_BYTES_VER2 + PMADATA_PIXEL_DEPTH_VER2 * pixelCountPerFrameInFile) + headerBytes, SEEK_SET );
		fseek( fout0, i * (PMADATA_FRAME_HEADER_BYTES_VER2 + PMADATA_PIXEL_DEPTH_VER2 * pixelCountPerFrameInFile) + headerBytes, SEEK_SET );

		int frameNum;
		fread( &frameNum, sizeof(frameNum), 1, fin );
		assert( frameNum == i );
		fread( &frameTime, sizeof(frameTime), 1, fin );
		fread( &frameState, sizeof(frameState), 1, fin );
		char reserved[16-4];
		fread( reserved, sizeof(reserved), 1, fin );

		frameNum -= startFrame;
		fwrite( &frameNum, sizeof(frameNum), 1, fout1 );
		fwrite( &frameTime, sizeof(frameTime), 1, fout1 );
		fwrite( &frameState, sizeof(frameState), 1, fout1 );
		fwrite( reserved, sizeof(reserved), 1, fout1 );

		fread( oneFrame, frameDataSize, 1, fin );
		fwrite( oneFrame, frameDataSize, 1, fout1 );
	}

	fclose( fin );
	fclose( fout0 );
	fclose( fout1 );
	return 1;
}
