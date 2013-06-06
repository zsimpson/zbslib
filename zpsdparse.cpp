// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A Photoshop psd file parser.  Adapted from code by Chris Hacker
//		}
//		*PORTABILITY win32
//		*REQUIRED_FILES zpsdparse.cpp zpsdparse.h
//		*VERSION 1.1
//		+HISTORY {
//			1.1 ZBS adpated from Chris Hecker's code
//		}
//		+TODO {
//			Finish conversion of bracking style ing naming convention to library standard
//			Convert to deal with all the formats of ZBits
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "assert.h"
#include "stdlib.h"
#include "stdio.h"
#include "memory.h"
// MODULE includes:
#include "zpsdparse.h"
// ZBSLIB includes:

typedef   signed char   int8;
typedef unsigned char  uint8;
typedef   signed short  int16;
typedef unsigned short uint16;
typedef   signed long   int32;
typedef unsigned long  uint32;

#pragma pack(1)
// PSD file header, everything is bigendian
struct PSDFile {
    uint32 Signature;
    uint16 Version;
    uint8  Reserved[6];
    uint16 Channels;
    uint32 Rows;
    uint32 Columns;
    uint16 Depth;
    uint16 Mode;
};

#define SWAP16(w) (uint16((((w)>>8)&0xFF)|(((w)<<8)&0xFF00)))
#define SWAP32(w) (uint32((((w)>>24)&0xFF)|(((w)<<24)&0xFF000000)|(((w)<<8)&0xFF0000)|(((w)>>8)&0xFF00)))
// followed by chunks (uint32 Length as first field):
// Color mode data section
// Image resources section
// Layer and Mask information section
// uint16 Compression = 0 for raw, = 1 for RLE
// Image data section, one channel at a time
// then bits, scanline order, no pad

// request channel -1 for information only

void zPsdFree( ZPsdMonoBitmap *data ) {
	if( data->bits ) {
		free(data->bits);
		data->bits = NULL;
	}
}

int zPsdRead( ZPsdMonoBitmap *map, char *fileName, int channel ) {
	int flag = 0;

	assert(fileName);
	FILE *file = fopen( fileName, "rb" );
	if( !file ) {
		return 0;
	}
	fseek( file, 0, SEEK_END );
	unsigned int size = ftell( file );
	fseek( file, 0, SEEK_SET );
	void *FileData = malloc( size );
	fread( FileData, size, 1, file );
	fclose( file );

//	HANDLE File = CreateFile(fileName,GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
//	if (File == INVALID_HANDLE_VALUE)
//		return 0;
//	HANDLE FileMap = CreateFileMapping(File,0,PAGE_READONLY,0,0,0);
//	if (FileMap == INVALID_HANDLE_VALUE)
//		return 0;
//	void *FileData = MapViewOfFile(FileMap,FILE_MAP_READ,0,0,0);
//	if (FileData == NULL )
//		return 0;
	uint8 *FileBytes = (uint8 *)FileData;
	if(FileBytes)
	{
		if(FileBytes[0] == '8' && FileBytes[1] == 'B' &&
		   FileBytes[2] == 'P' && FileBytes[3] == 'S')
		{
			// it's a Photoshop PSD file
			PSDFile *Psd = (PSDFile *)FileBytes;
			int Width = SWAP32(Psd->Columns);
			int Height = SWAP32(Psd->Rows);
			assert(SWAP16(Psd->Version) == 1);
			assert(SWAP16(Psd->Depth) == 8);

			map->w = Width;
			map->h = Height;
			map->bits = NULL;
			map->channels = SWAP16(Psd->Channels);
			if (channel == -1) { flag = 1; goto leave; }
			map->bits = (unsigned char*)  malloc(Width * Height);

			unsigned char *data = map->bits;

			if((SWAP16(Psd->Version) == 1) && 
			   (SWAP16(Psd->Depth) == 8) && (channel < SWAP16(Psd->Channels)))
			{
				int LineStride = int(Width);   // walk down the bitmap
				flag = 1;

				FileBytes += sizeof(PSDFile);
				// skip Color mode
				FileBytes += SWAP32(*(uint32 *)(FileBytes)) + 4;
				// skip Image resources
				FileBytes += SWAP32(*(uint32 *)(FileBytes)) + 4;
				// skip Layers
				FileBytes += SWAP32(*(uint32 *)(FileBytes)) + 4;
				uint16 Compression = SWAP16(*(uint16*)FileBytes);
				FileBytes += 2;
				if(Compression == 0)
				{
					// uncompressed data

					// skip to the specified channel
					uint32 ChannelSize = Width * Height;
					FileBytes += ChannelSize * channel;

					// rip out the specified channel
					for(int i = 0;i < Height;++i)
					{
						for(int j = 0;j < Width;++j)
						{
							data[j] = *FileBytes++;
						}
						data += LineStride;
					}
				}
				else
				{
					// RLE compressed data
					// header of uint16 line lengths
					// n b+
					// n=[00,7F]: copy n+1 b's
					// n=[81,FF]: dupe b -n+1 times
					// n=80: finished/skip

					// figure out where this channel's scanlines start
					int BytesToChannel = 2*Height*channel;
					int LineOffset = 2*Height*SWAP16(Psd->Channels);
					int i = 0;
					while(i < BytesToChannel) {
						LineOffset += SWAP16(*(uint16*)(FileBytes+i));
						i += 2;
					}
					FileBytes += LineOffset;

					// now we're at Channel
					int BytesToWrite = Width*Height;
					int LineCount = 0;
					while(BytesToWrite > 0) {
						// decompress run
						int Count = *(int8*)FileBytes;
						++FileBytes;
						if(Count >= 0) {
							// copy
							Count += 1;
							BytesToWrite -= Count;
							LineCount += Count;
							while(Count--) {
							   *data++ = *FileBytes++;
							}
						} else
						if(Count > -128) {
							// run
							Count = -Count + 1;
							BytesToWrite -= Count;
							memset(data, *FileBytes++, Count);
							data += Count;
							LineCount += Count;
						}
						// line end?
						assert(LineCount <= Width);
						if(LineCount >= Width) {
							// go back to start of this line and then to next
							LineCount = 0;
							data += LineStride - Width;
						}
						assert(BytesToWrite >= 0);
					}
				}

			}
		}
	}

   leave:
//	UnmapViewOfFile(FileData);
//	CloseHandle(FileMap);
//	CloseHandle(File);
	free( FileData );

	return flag;
} 

char *zPsdRead4( char *file, int r, int g, int b, int a, int *w, int *h ) {
	int channels[4] = { r,g,b,a };

	ZPsdMonoBitmap mono;
	if (!zPsdRead(&mono, file, -1)) {
		return 0;
	}

	if (mono.channels < 3) {
		return 0;
	}

	char *buffer = (char *)malloc( mono.w * mono.h * 4 );

	if( w ) *w = mono.w;
	if( h ) *h = mono.h;

	for (int i=0; i < 4; i++) {
		int j,n = mono.h * mono.w;
		int c = channels[i];
		if (c < 0) {
			for (j=0; j < n; ++j) {
				buffer[j*4+i] = c;
			}
		}
		else  if (c < mono.channels) {
			zPsdRead(&mono, file, i);
			for (j=0; j < n; ++j) {
				buffer[j*4+i] = mono.bits[j];
			}
			zPsdFree(&mono);
		}
		else {
			for (j=0; j < n; ++j) {
				buffer[j*4+i] = (char) 255;
			}
		}
	}

	return buffer;
}

