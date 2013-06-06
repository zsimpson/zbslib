#ifndef ZENDIAN_H
#define ZENDIAN_H

#include "stdio.h"

//-----------------------------------------------------------------------------
// binary file compatibility util fns

void saveBinaryCompatData( FILE *f);
bool loadBinaryCompatData( FILE *f, int &shouldByteSwap, int &dataLongSize );
	// these were written for KinTek, but can be used by any app to load/save a 
	// chunk of binary data that gives details about the machine/os arhitecture 
	// that the subsequent binary data is saved in.  It provides versioning.

int machineIsBigEndian();
	// is the machine big endian?  

void byteSwap( unsigned char * b, int size );
	// swap general-sized buffer

size_t freadEndian( void *buffer, size_t elemSize, size_t numElem, FILE *f, int _byteSwap );
	// wrapper for fread that does byteswapping according to _byteSwap

#define BYTESWAP(x) byteSwap((unsigned char *) &x,sizeof(x))
	// macro for atomic types for which sizeof() works

#endif

