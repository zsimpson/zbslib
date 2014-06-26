// @ZBS {
//		*MODULE_NAME Simple Hash
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A simple, fast, and decoupled hash table using string keys.
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zhashtable.cpp zhashtable.h zhashtyped.h zendian.cpp zendian.h
//		*VERSION 2.0
//		+HISTORY {
//			1.2 KLD fixed a bug on setting a key that had previously been deleted
//			1.3 ZBS changed memcmp to strncmp to fix a bug that made set read potenitally outside of bounds (causing Bounds Checker to complain)
//			1.4 ZBS changed hash function to code borrowed from Bob Jenkins
//			1.5 ZBS changed name to standard convention
//			2.0 ZBS reworked nomenclature to allow for clear binary or string storage and also added concept of buffer type
//			2.1 ZBS added typing flags to improve the get and put characteristics
//		}
//		+TODO {
//		}
//		*SELF_TEST yes console ZHASHTABLE_SELF_TEST
//		*PUBLISH yes
// }

// OPERATING SYSTEM specific includes:
#ifdef WIN32
	//#include "windows.h"
	// The following line is copied out of windows.h to avoid the header dependency
	// If there is a problem, comment it out and uncomment the include above.
	// This function is only used for the debugging function "debugDump()"
	// and could easily be removed entirely.
	extern "C" {
	__declspec(dllimport) void __stdcall OutputDebugStringA( const char *lpOutputString );
	}
	#define OutputDebugString OutputDebugStringA
#endif

// STDLIB includes:
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"
#include "assert.h"
#include "limits.h"
#include "ctype.h"
		// for isprint() -- see zHashTableUnpack()

// MODULE includes:
#include "zhashtable.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Endian
// ZBS Dec 2008 made redundant copies of these endian calls in zhashtable
// so that hashtable can compile alone
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


int zHashTableMachineIsBigEndian() {
	short word = 0x4321;
	if((*(char *)& word) != 0x21 ) {
		return 1;
	}
	return 0;
}

void zHashTableByteSwap( unsigned char * b, int size ) {
	int i = 0;
	int j = size-1;
	unsigned char c;
	while( i < j ) {
		c = b[i];
		b[i] = b[j];
		b[j] = c;
		i++, j--;
	}
}

#define BYTESWAP(x) zHashTableByteSwap((unsigned char *) &x,sizeof(x))
	// macro for atomic types

size_t zHashTableFreadEndian( void *buffer, size_t elemSize, size_t numElem, FILE *f, int _byteSwap ) {
	size_t elemsRead = fread( buffer, elemSize, numElem, f );
	if( !_byteSwap || elemSize==1 ) {
		return elemsRead;
	}
	unsigned char *b = (unsigned char *)buffer;
	for( size_t i=0; i<numElem; i++, b += elemSize ) {
		zHashTableByteSwap( b, elemSize );
	}
	return elemsRead;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ZHashTable 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// This hash function generator taken from:
// http://burtleburtle.net/bob/hash/doobs.html
// By Bob Jenkins, 1996.  bob_jenkins -AT- burtleburtle.net.
// It is a MAJOR improvement over my previous simple-minded implementations

unsigned int ZHashTable::hash( char *k, int keyLen ) {
	#define mix(a,b,c) \
	{ \
	  a -= b; a -= c; a ^= (c>>13); \
	  b -= c; b -= a; b ^= (a<<8); \
	  c -= a; c -= b; c ^= (b>>13); \
	  a -= b; a -= c; a ^= (c>>12);  \
	  b -= c; b -= a; b ^= (a<<16); \
	  c -= a; c -= b; c ^= (b>>5); \
	  a -= b; a -= c; a ^= (c>>3);  \
	  b -= c; b -= a; b ^= (a<<10); \
	  c -= a; c -= b; c ^= (b>>15); \
	}

	unsigned int a,b,c,len;
	len = (unsigned int)keyLen;
	a = b = 0x9e3779b9;
	c = 0;

	while( len >= 12 ) {
		a += (k[0] +((unsigned int)k[1]<<8) +((unsigned int)k[2]<<16) +((unsigned int)k[3]<<24));
		b += (k[4] +((unsigned int)k[5]<<8) +((unsigned int)k[6]<<16) +((unsigned int)k[7]<<24));
		c += (k[8] +((unsigned int)k[9]<<8) +((unsigned int)k[10]<<16)+((unsigned int)k[11]<<24));
		mix(a,b,c);
		k += 12;
		len -= 12;
	}

	// handle the last 11 bytes
	c += keyLen;
	switch(len) {
		case 11: c+=((unsigned int)k[10]<<24);
		case 10: c+=((unsigned int)k[9]<<16);
		case 9 : c+=((unsigned int)k[8]<<8);
		// the first byte of c is reserved for the length
		case 8 : b+=((unsigned int)k[7]<<24);
		case 7 : b+=((unsigned int)k[6]<<16);
		case 6 : b+=((unsigned int)k[5]<<8);
		case 5 : b+=k[4];
		case 4 : a+=((unsigned int)k[3]<<24);
		case 3 : a+=((unsigned int)k[2]<<16);
		case 2 : a+=((unsigned int)k[1]<<8);
		case 1 : a+=k[0];
		// case 0: nothing left to add
	}
	mix(a,b,c);
	return c;
}

int ZHashTable::internalSet( ZHashRecord **_hashTable, int _hashTableSize, char *key, int keyLen, ZHashRecord *_recPtr ) {
	// The _recPtr might be nul if we are deleting which is why key and keyLen are passed in separately
	// However, in the case that this isn't a delete, this fucntion expects that _recPtr already has the
	// key, keyLen, valLen and val all setup

	// NOTE: threadSafety is handled by calling function, since the calling function must be atomic in its
	// other modification of the hash.

	hasAnyChanged = 1;
	int index = hash( key, keyLen ) % _hashTableSize;
	int total = _hashTableSize;
	int totalDeleted = 0;
	int foundKey = 0;
	ZHashRecord *recPtr;
	int dstIndex = -1;
	while( total-- ) {
		recPtr = _hashTable[index];
		if( recPtr ) {
			if( recPtr->flags & zhDELETED ) {
				// Found a deleted key. Use this space unless the key is found.
				// NOTE: This will end up using farthest deleted key from the
				// head of the chain first.
				dstIndex = index;
				totalDeleted++;
			}
			else if( keyLen==recPtr->keyLen && !memcmp(key, recPtr->key, keyLen) ) {
				// Found already existing key. Use this slot.
				foundKey = 1;
				dstIndex = index;
				break;
			}
			else {
				collisions++;
			}
		}
		else {
			// Found an empty slot, marking the end of this chain.
			// Use this slot unless we found a deleted slot above.
			if( dstIndex == -1 ) {
				dstIndex = index;
			}
			break;
		}
		index = (index+1) % _hashTableSize;
	}
	assert( total + totalDeleted );
		// There should always be some room left since we keep
		// the list at 50% free all the time to avoid collisions
	
	// Here, destIndex contains the index of the found record. The found record value can
	// be either NULL, a pointer to a deleted entry, or a pointer to the found entry.
	
	ZHashRecord *delRec = _hashTable[dstIndex];

	if( _recPtr == NULL ) {
		// We are deleting the key, it is only marked as deleted it is not actually freed
		// until the value is replaced. This sucks because the val might be huge
		// and we don't need it around anymore.  So we do a check here to see if it is
		// worth reallocating the hash record.
		if( foundKey ) {
			// Only can deleted it if it already exists
			if( delRec->type == zhPFR ) {
				void **ptr = (void **)&delRec->key[delRec->keyLen];
				assert( *ptr );
				free( *ptr );
				*ptr = 0;
			}
			if( delRec->valLen > 64 ) {
				// Probably worth it to reallocate
				ZHashRecord *r = (ZHashRecord *)malloc( sizeof(ZHashRecord) + keyLen );
				memcpy( r, recPtr, sizeof(ZHashRecord) + keyLen );
				r->valLen = 0;
				free( delRec );
				_hashTable[dstIndex] = r;
				delRec = r;
			}
			delRec->flags = zhDELETED;
			delRec->type = 0;
			return 1;
		}
		return 0;
	}
	else {
		// We are either adding a new key or setting the value of an old key.
		// Either way, if the destination record has a value, it must be deleted
		// before the new value is set.
		if( delRec ) {
			if( delRec->type == zhPFR ) {
				void **ptr = (void **)&delRec->key[delRec->keyLen];
				assert( *ptr );
				free( *ptr );
				*ptr = 0;
			}
			free( delRec );
		}
		_recPtr->flags |= zhCHANGED;
		_hashTable[dstIndex] = _recPtr;
	}
	return !foundKey;
}

ZHashTable::ZHashTable( int _initialSize ) {
	last = 0;
	lastLen = 0;
	initialSize = _initialSize;
	hashTable = 0;
	threadSafe = 0;
	mutex = 0;
	clear();
}

ZHashTable::ZHashTable( const ZHashTable &copy ) {
	last = 0;
	lastLen = 0;
	initialSize = 64;
	hashTable = 0;
	threadSafe = 0;
	mutex = 0;
	clear();
	copyFrom( (ZHashTable &)copy );
}

ZHashTable::~ZHashTable() {
	clearLast();
	if( hashTable ) {
		for( int i=0; i<hashTableSize; i++ ) {
			ZHashRecord *p = hashTable[i];
			if( p ) {
				free( p );
			}
		}
		
		free( hashTable );
	}
#ifdef ZHASH_THREADSAFE
	if( mutex ) {
		delete mutex;
		mutex = 0;
	}
#endif
}

void ZHashTable::dump( int toStdout ) {
	lock();
	for( int i=0; i<size(); i++ ) {
		if( getKey(i) ) {
			char buf[512] = {0,};
			char keybuf[256] = {0,};

			// If the key is binary we convert it to hex
			int binary = 0;
			for( int j=0; j<hashTable[i]->keyLen-1; j++ ) {
				if( hashTable[i]->key[j] < ' ' || hashTable[i]->key[j] > 126 ) {
					binary = 1;
					break;
				}
			}

			if( binary ) {
				char *b = keybuf;
				for( int j=0; j<hashTable[i]->keyLen && b < keybuf; j++ ) {
					sprintf( b, "%02X", hashTable[i]->key[j] );
					b += 2;
					*b++ = ' ';
				}
			}
			else {
				strncpy( keybuf, hashTable[i]->key, 255 );
			}

			int type = hashTable[i]->type;
			char typeCode[] = { ' ', 'b', 's', 'i', 'u', 'l', 'f', 'd', 'p', 'o' };
			sprintf( buf, "%s = (%c)%s\n", keybuf, typeCode[type], getValS(i) );
			if( toStdout ) {
				printf( "%s", buf );
			}
			else {
				#ifdef WIN32
					OutputDebugString( buf );
				#else
					printf( "%s", buf );
				#endif
			}
		}
	}
	unlock();
}

char *ZHashTable::dumpToString() {
	int len = 0;
	char *string = 0;
	lock();
	for( int pass=0; pass<2; pass++ ) {
		for( int i=0; i<size(); i++ ) {
			if( getKey(i) ) {
				char buf[512] = {0,};
				char keybuf[256] = {0,};

				// If the key is binary we convert it to hex
				int binary = 0;
				for( int j=0; j<hashTable[i]->keyLen-1; j++ ) {
					if( hashTable[i]->key[j] < ' ' || hashTable[i]->key[j] > 126 ) {
						binary = 1;
						break;
					}
				}

				if( binary ) {
					char *b = keybuf;
					for( int j=0; j<hashTable[i]->keyLen && b < keybuf; j++ ) {
						sprintf( b, "%02X", hashTable[i]->key[j] );
						b += 2;
						*b++ = ' ';
					}
				}
				else {
					strncpy( keybuf, hashTable[i]->key, 255 );
				}

				int type = hashTable[i]->type;
				char typeCode[] = { 'b', 's', 'i', 'u', 'l', 'f', 'd', 'p', 'o' };
				sprintf( buf, "%s = (%c)%s\n", keybuf, typeCode[type], getValS(i) );

				if( pass==0 ) {
					len += strlen( buf );
				}
				else {
					strcat( string, buf );
				}
			}
		}
		if( pass==0 ) {
			string = new char[len+1];
			*string = 0;
		}
	}
	unlock();
	return string;
}

void ZHashTable::clearLast() {
	if( last ) {
		assert( !threadSafe );
			// last is only ever used by the convert() functionality, which is never 
			// threadSafe in the current impl.
		free( last );
		last = 0;
		lastLen = 0;
	}
}

void ZHashTable::init( int _initSize ) {
	lock();
	last = 0;
	lastLen = 0;
	clear( _initSize, 1 );
	unlock();
}

void ZHashTable::clear( int _initSize, int freeFirst ) {
	lock();
	if( hashTable && freeFirst ) {
		for( int i=0; i<hashTableSize; i++ ) {
			if( hashTable[i] ) {
				if( hashTable[i]->type == zhPFR ) {
					free( *(void **)&hashTable[i]->key[hashTable[i]->keyLen] );
				}
				free( hashTable[i] );
			}
		}
		free( hashTable );
	}
	if( _initSize > 0 ) {
		initialSize = _initSize;
	}
	hashTableSize = initialSize;
	numRecords = 0;
	hashTable = 0;
	hasAnyChanged = 0;
	collisions = 0;
	if( initialSize ) {
		hashTable = (ZHashRecord **)malloc( sizeof(ZHashRecord *) * hashTableSize );
		memset( hashTable, 0, sizeof(ZHashRecord *) * hashTableSize );
	}
	unlock();
}

void ZHashTable::copyFrom( ZHashTable &table ) {
	lock();
	int alreadyLocked=1;
	for( int i=0; i<table.size(); i++ ) {
		ZHashRecord *p = table.hashTable[i];
		if( p ) {
			if( !(p->flags & zhDELETED) ) {
				bputB( p->key, p->keyLen, &p->key[p->keyLen], p->valLen, p->type, p->flags, alreadyLocked );
			}
		}
	}
	unlock();
}

char **ZHashTable::getKeys( int &count ) {
	lock();
	char **ptrs = (char **)malloc( sizeof(char*) * numRecords );
	int cnt = 0;
	for( int i=0; i<hashTableSize; i++ ) {
		if( hashTable[i] ) {
			assert( cnt < numRecords );
			ptrs[cnt++] = strdup( hashTable[i]->key );
		}
	}
	
	assert( cnt == numRecords );
	count = cnt;
	unlock();
	return ptrs;
}

// THREAD SAFETY
//------------------------------------------------------------------------------------------

void ZHashTable::setThreadSafe( int safe ) {
	#ifdef ZHASH_THREADSAFE
		threadSafe = safe;
		if( threadSafe && !mutex ) {
			mutex = new PMutex;
		}
	#endif
}

int ZHashTable::isThreadSafe() {
	#ifdef ZHASH_THREADSAFE
		return threadSafe;
	#endif
	return 0;
}

void ZHashTable::lock() {
	#ifdef ZHASH_THREADSAFE
	if( threadSafe ) {
		mutex->lock();
	}
	#endif
}

void ZHashTable::unlock() {
	#ifdef ZHASH_THREADSAFE
	if( threadSafe ) {
		mutex->unlock();
	}
	#endif
}


// CONVERT
//------------------------------------------------------------------------------------------
void ZHashTable::convert( int srcType, char *src, int srcLen, int dstType, char **dst, int *dstLen ) {
	assert( !threadSafe && "can't use convert functionality for threadsafe hashtables! (tfb)" );
	char *endPtr = 0;

	switch( dstType ) {
		case zhS32: {
			*dst = (char *)malloc( sizeof(int) );
			*dstLen = sizeof(int);
			break;
		}
		case zhU32: {
			*dst = (char *)malloc( sizeof(int) );
			*dstLen = sizeof(int);
			break;
		}
		case zhS64: {
			*dst = (char *)malloc( sizeof(S64) );
			*dstLen = sizeof(S64);
			break;
		}
		case zhFLT: {
			*dst = (char *)malloc( sizeof(float) );
			*dstLen = sizeof(float);
			break;
		}
		case zhDBL: {
			*dst = (char *)malloc( sizeof(double) );
			*dstLen = sizeof(double);
			break;
		}
		case zhPTR: {
			*dst = (char *)malloc( sizeof(char*) );
			*dstLen = sizeof(char*);
			break;
		}
		case zhPFR: {
			*dst = (char *)malloc( sizeof(char*) );
			*dstLen = sizeof(char*);
			break;
		}
	}

	switch( srcType ) {
		case zhBIN: {
			switch( dstType ) {
				case zhBIN: {
					*dst = (char *)malloc( srcLen );
					*dstLen = srcLen;
					memcpy( *dst, src, srcLen );
					break;
				}
				case zhSTR: {
					*dst = (char *)malloc( srcLen );
					*dstLen = srcLen;
					memcpy( *dst, src, srcLen );
					break;
				}
				case zhS32: {
					assert( srcLen == sizeof(int) );
					*(signed int *)dst = *(signed int *)src;
					break;
				}
				case zhU32: {
					assert( srcLen == sizeof(int) );
					*(unsigned int *)dst = *(unsigned int *)src;
					break;
				}
				case zhS64: {
					assert( srcLen == sizeof(S64) );
					*(S64 *)dst = *(S64 *)src;
					break;
				}
				case zhFLT: {
					assert( srcLen == sizeof(float) );
					*(float *)dst = *(float *)src;
					break;
				}
				case zhDBL: {
					assert( srcLen == sizeof(double) );
					*(double *)dst = *(double *)src;
					break;
				}
				case zhPTR: {
					assert( srcLen == sizeof(char*) );
					*(char* *)dst = *(char* *)src;
					break;
				}
				case zhPFR: {
					assert( srcLen == sizeof(char*) );
					*(char* *)dst = *(char* *)src;
					break;
				}
			}
			break;
		}
		case zhSTR: {
			switch( dstType ) {
				case zhBIN: {
					*dst = (char *)malloc( srcLen+1 );
					*dstLen = srcLen+1;
					memcpy( *dst, src, srcLen+1 );
					break;
				}
				case zhSTR: {
					*dst = (char *)malloc( srcLen+1 );
					*dstLen = srcLen+1;
					memcpy( *dst, src, srcLen+1 );
					break;
				}
				case zhS32: {
					int ret;
					if( srcLen > 2 && src[0] == '0' && src[1] == 'x' ) {
						ret = strtoul( src, &endPtr, 0 );
					}
					else {
						ret = (int)strtod( src, &endPtr );
					}
					if( endPtr-src < srcLen && *endPtr=='.' ) {
						ret = (int)strtod( src, &endPtr );
					}
					if( endPtr-src >= srcLen ) {
						*((int *)*dst) = ret;
					}
					else {
						*((int *)*dst) = 0;
					}
					break;
				}
				case zhU32: {
					int ret;
					if( srcLen > 2 && src[0] == '0' && src[1] == 'x' ) {
						ret = strtoul( src, &endPtr, 0 );
					}
					else {
						ret = (unsigned int)strtod( src, &endPtr );
					}
					if( endPtr-src >= srcLen ) {
						*((unsigned int *)*dst) = ret;
					}
					else {
						*((unsigned int *)*dst) = 0;
					}
					break;
				}
				case zhS64: {
					S64 ret = (S64)strtod( src, &endPtr );
					if( endPtr-src >= srcLen ) {
						*((S64 *)*dst) = ret;
					}
					else {
						*((S64 *)*dst) = 0;
					}
					break;
				}
				case zhFLT: {
					float ret = (float)strtod( src, &endPtr );
					if( endPtr-src >= srcLen ) {
						*((float *)*dst) = ret;
					}
					else {
						*((float *)*dst) = 0;
					}
					break;
				}
				case zhDBL: {
					double ret = (double)strtod( src, &endPtr );
					if( endPtr-src >= srcLen ) {
						*((double *)*dst) = ret;
					}
					else {
						*((double *)*dst) = 0;
					}
					break;
				}
				case zhPTR: {
					char* ret = (char*)strtoul( src, &endPtr, 0 );
					if( endPtr-src >= srcLen ) {
						*((char* *)*dst) = ret;
					}
					else {
						*((char* *)*dst) = 0;
					}
					break;
				}
				case zhPFR: {
					char* ret = (char*)strtoul( src, &endPtr, 0 );
					if( endPtr-src >= srcLen ) {
						*((char* *)*dst) = ret;
					}
					else {
						*((char* *)*dst) = 0;
					}
					break;
				}
			}
			break;
		}
		case zhS32: {
			switch( dstType ) {
				case zhBIN: {
					*dst = (char *)malloc( sizeof(int) );
					*dstLen = sizeof(int);
					memcpy( *dst, src, sizeof(int) );
					break;
				}
				case zhSTR: {
					char buf[128];
					sprintf( buf, "%d", *(int *)src );
					int len = strlen( buf ) + 1;
					*dst = (char *)malloc( len );
					*dstLen = len;
					memcpy( *dst, buf, *dstLen );
					break;
				}
				case zhS32: {
					*(int *)*dst = *(int *)src;
					break;
				}
				case zhU32: {
					*(unsigned int *)*dst = *(unsigned int *)src;
					break;
				}
				case zhS64: {
					*(S64 *)*dst = (S64)*(int *)src;
					break;
				}
				case zhFLT: {
					*(float *)*dst = (float)*(int *)src;
					break;
				}
				case zhDBL: {
					*(double *)*dst = (double)*(int *)src;
					break;
				}
				case zhPTR: {
					*(char* *)*dst = (char *)*(int *)src;
					break;
				}
				case zhPFR: {
					*(char* *)*dst = (char *)*(int *)src;
					break;
				}
			}
			break;
		}
		case zhU32: {
			switch( dstType ) {
				case zhBIN: {
					*dst = (char *)malloc( sizeof(unsigned int) );
					*dstLen = sizeof(unsigned int);
					memcpy( *dst, src, sizeof(unsigned int) );
					break;
				}
				case zhSTR: {
					char buf[128];
					sprintf( buf, "%u", *(unsigned int *)src );
					int len = strlen( buf ) + 1;
					*dst = (char *)malloc( len );
					*dstLen = len;
					memcpy( *dst, buf, *dstLen );
					break;
				}
				case zhS32: {
					*(int *)*dst = *(int *)src;
					break;
				}
				case zhU32: {
					*(unsigned int *)*dst = (unsigned int)*(unsigned int *)src;
					break;
				}
				case zhS64: {
					*(S64 *)*dst = (S64)*(unsigned int *)src;
					break;
				}
				case zhFLT: {
					*(float *)*dst = (float)*(unsigned int *)src;
					break;
				}
				case zhDBL: {
					*(double *)*dst = (double)*(unsigned int *)src;
					break;
				}
				case zhPTR: {
					*(char* *)*dst = (char*)*(unsigned int *)src;
					break;
				}
				case zhPFR: {
					*(char* *)*dst = (char*)*(unsigned int *)src;
					break;
				}
			}
			break;
		}
		case zhS64: {
			switch( dstType ) {
				case zhBIN: {
					*dst = (char *)malloc( sizeof(S64) );
					*dstLen = sizeof(S64);
					memcpy( *dst, src, sizeof(S64) );
					break;
				}
				case zhSTR: {
					char buf[128];
#ifdef WIN32
					sprintf( buf, "%I64d", *(S64*)src );
#else
					sprintf( buf, "%lld", *(S64*)src );
#endif
					int len = strlen( buf ) + 1;
					*dst = (char *)malloc( len );
					*dstLen = len;
					memcpy( *dst, buf, *dstLen );
					break;
				}
				case zhS32: {
					*(int *)*dst = (int)*(S64 *)src;
					break;
				}
				case zhU32: {
					*(unsigned int *)*dst = (unsigned int)*(S64 *)src;
					break;
				}
				case zhS64: {
					*(S64 *)*dst = (S64)*(S64 *)src;
					break;
				}
				case zhFLT: {
					*(float *)*dst = (float)*(S64 *)src;
					break;
				}
				case zhDBL: {
					*(double *)*dst = (double)*(S64 *)src;
					break;
				}
				case zhPTR: {
					assert( 0 );
					//*(char* *)*dst = (char*)*(S64 *)src;
					break;
				}
				case zhPFR: {
					assert( 0 );
					//*(char* *)*dst = (char*)*(S64 *)src;
					break;
				}
			}
			break;
		}
		case zhFLT: {
			switch( dstType ) {
				case zhBIN: {
					*dst = (char *)malloc( sizeof(float) );
					*dstLen = sizeof(float);
					memcpy( *dst, src, sizeof(float) );
					break;
				}
				case zhSTR: {
					char buf[128];
					sprintf( buf, "%f", *(float*)src );
					int len = strlen( buf ) + 1;
					*dst = (char *)malloc( len );
					*dstLen = len;
					memcpy( *dst, buf, *dstLen );
					break;
				}
				case zhS32: {
					*(int *)*dst = (int)*(float *)src;
					break;
				}
				case zhU32: {
					*(unsigned int *)*dst = (unsigned int)*(float *)src;
					break;
				}
				case zhS64: {
					*(S64 *)*dst = (S64)*(float *)src;
					break;
				}
				case zhFLT: {
					*(float *)*dst = (float)*(float *)src;
					break;
				}
				case zhDBL: {
					*(double *)*dst = (double)*(float *)src;
					break;
				}
				case zhPTR: {
					assert( 0 );
					break;
				}
				case zhPFR: {
					assert( 0 );
					break;
				}
			}
			break;
		}
		case zhDBL: {
			switch( dstType ) {
				case zhBIN: {
					*dst = (char *)malloc( sizeof(double) );
					*dstLen = sizeof(double);
					memcpy( *dst, src, sizeof(double) );
					break;
				}
				case zhSTR: {
					char buf[128];
					sprintf( buf, "%lf", *(double*)src );
					int len = strlen( buf ) + 1;
					*dst = (char *)malloc( len );
					*dstLen = len;
					memcpy( *dst, buf, *dstLen );
					break;
				}
				case zhS32: {
					*(int *)*dst = (int)*(double *)src;
					break;
				}
				case zhU32: {
					*(unsigned int *)*dst = (unsigned int)*(double *)src;
					break;
				}
				case zhS64: {
					*(S64 *)*dst = (S64)*(double *)src;
					break;
				}
				case zhFLT: {
					*(float *)*dst = (float)*(double *)src;
					break;
				}
				case zhDBL: {
					*(double *)*dst = (double)*(double *)src;
					break;
				}
				case zhPTR: {
					assert( 0 );
					break;
				}
				case zhPFR: {
					assert( 0 );
					break;
				}
			}
			break;
		}
		case zhPTR: {
			switch( dstType ) {
				case zhBIN: {
					*dst = (char *)malloc( sizeof(char*) );
					*dstLen = sizeof(char*);
					memcpy( *dst, src, sizeof(char*) );
					break;
				}
				case zhSTR: {
					char buf[128];
					sprintf( buf, "0x%lX", (long)(*(char**)src) );
					int len = strlen( buf ) + 1;
					*dst = (char *)malloc( len );
					*dstLen = len;
					memcpy( *dst, buf, *dstLen );
					break;
				}
				case zhS32: {
					#if defined(__LP64__) || defined(_Wp64)
					assert( 0 && "64bit ptr illegal typecast (zhashtable)" ); 
					#else
					*(int *)*dst = (int)*(char* *)src;
					#endif
					break;
				}
				case zhU32: {
					#if defined(__LP64__) || defined(_Wp64)
					assert( 0 && "64bit ptr illegal typecast (zhashtable)"); 
					#else
					*(unsigned int *)*dst = (unsigned int)*(char* *)src;
					#endif
					break;
				}
				case zhS64: {
					assert(0);
					//*(S64 *)*dst = (S64)*(char* *)src;
					break;
				}
				case zhFLT: {
					assert( 0 );
					break;
				}
				case zhDBL: {
					assert( 0 );
					break;
				}
				case zhPTR: {
					*(char* *)*dst = (char*)*(char* *)src;
					break;
				}
				case zhPFR: {
					*(char* *)*dst = (char*)*(char* *)src;
					break;
				}
			}
			break;
		}
		case zhPFR: {
			switch( dstType ) {
				case zhBIN: {
					*dst = (char *)malloc( sizeof(char*) );
					*dstLen = sizeof(char*);
					memcpy( *dst, src, sizeof(char*) );
					break;
				}
				case zhSTR: {
					char buf[128];
					sprintf( buf, "0x%lX", (long)(*(char**)src) );
					int len = strlen( buf ) + 1;
					*dst = (char *)malloc( len );
					*dstLen = len;
					memcpy( *dst, buf, *dstLen );
					break;
				}
				case zhS32: {
					#if defined(__LP64__) || defined(_Wp64)
					assert( 0 && "64bit ptr illegal typecast (zhashtable)" ); 
					#else
					*(int *)*dst = (int)*(char* *)src;
					#endif
					break;
				}
				case zhU32: {
					#if defined(__LP64__) || defined(_Wp64)
					assert( 0 && "64bit ptr illegal typecast (zhashtable)" ); 
					#else
					*(unsigned int *)*dst = (unsigned int)*(char* *)src;
					#endif
					break;
				}
				case zhS64: {
					assert(0);
					//*(S64 *)*dst = (S64)*(char* *)src;
					break;
				}
				case zhFLT: {
					assert( 0 );
					break;
				}
				case zhDBL: {
					assert( 0 );
					break;
				}
				case zhPTR: {
					*(char* *)*dst = (char*)*(char* *)src;
					break;
				}
				case zhPFR: {
					*(char* *)*dst = (char*)*(char* *)src;
					break;
				}
			}
			break;
		}
	}
}

// GET
//------------------------------------------------------------------------------------------

ZHashRecord *ZHashTable::lookup( char *key, int keyLen, int alreadyLocked ) {
	if( !alreadyLocked ) lock();
	if( keyLen == -1 ) keyLen = strlen( key ) + 1;
	int hashRec = hash(key,keyLen) % hashTableSize;
	int total = hashTableSize;
	while( total-- ) {
		ZHashRecord *recPtr = hashTable[hashRec];
		if( recPtr ) {
			if( !(recPtr->flags & zhDELETED) ) {
				// Non-deleted element, compare
				if( !memcmp(key, recPtr->key, keyLen) ) {
					if( !alreadyLocked ) unlock();
					return hashTable[hashRec];
				}
			}
		}
		else {
			// Empty record found before match, terminate search
			if( !alreadyLocked) unlock();
			return 0;
		}
		hashRec = (hashRec+1) % hashTableSize;
	}
	if( !alreadyLocked ) unlock();
	return 0;
}

int ZHashTable::has( char *key, int keyLen ) {
	lock();
	int alreadyLocked = 1;
	char *a = bgetb( key, keyLen, 0, 0, 0, 0, 0, alreadyLocked );
	int val = a ? 1 : 0;
	unlock();
	return val;
}

int ZHashTable::getType ( char *key, int keyLen ) {
	int srcType = 0;
	int srcLen = 0;
	bgetb (key, -1, &srcLen, &srcType);

	return srcType;
}

char *ZHashTable::getS( char *key, char *onEmpty, char *userBuf, int *userBufLen, int alreadyLocked ) {
	if( threadSafe && !alreadyLocked ) assert( userBuf && "must supply userbuffer for threadsafe!" );
		// otherwise bgetb will alloc the buffer which is unexpected behavior at this
		// point so I prefer this assert for now. (tfb)
	clearLast();
	int srcType = 0;
	int srcLen = 0;
	char *src = bgetb( key, -1, &srcLen, &srcType, 0, userBuf, userBufLen, alreadyLocked );
	if( src ) {
		if( srcType != zhSTR ) {
			convert( srcType, src, srcLen, zhSTR, &last, &lastLen );
			return (char *)last;
		}
		else {
			return src;
		}
	}
	return onEmpty;
}

char *ZHashTable::getSLocked( char *key, char *onEmpty ) {
	// This fn assumes the user has already locked, and will unlock after using the 
	// returned pointer.  
	return getS( key, onEmpty, 0, 0, 1 );
}

int ZHashTable::getI( char *key, int onEmpty ) {
	clearLast();
	int srcType = 0;
	int srcLen = 0;
	lock();
	int alreadyLocked = 1;
	char *src = bgetb( key, -1, &srcLen, &srcType, 0, 0, 0, alreadyLocked );
	if( src ) {
		if( srcType != zhS32 ) {
			convert( srcType, src, srcLen, zhS32, &last, &lastLen );
			unlock();
			return *(int *)last;
		}
		else {
			int val = *(int *)src;
			unlock();
			return val;
		}
	}
	unlock();
	return onEmpty;
}

unsigned int ZHashTable::getU( char *key, unsigned int onEmpty ) {
	clearLast();
	int srcType = 0;
	int srcLen = 0;
	lock();
	int alreadyLocked = 1;
	char *src = bgetb( key, -1, &srcLen, &srcType, 0, 0, 0, alreadyLocked );
	if( src ) {
		if( srcType != zhU32 ) {
			convert( srcType, src, srcLen, zhU32, &last, &lastLen );
			unlock();
			return *(unsigned int *)last;
		}
		else {
			unsigned int val = *(unsigned int *)src;
			unlock();
			return val;
		}
	}
	unlock();
	return onEmpty;
}

S64 ZHashTable::getL( char *key, S64 onEmpty ) {
	clearLast();
	int srcType = 0;
	int srcLen = 0;
	lock();
	int alreadyLocked = 1;
	char *src = bgetb( key, -1, &srcLen, &srcType, 0, 0, 0, alreadyLocked );
	if( src ) {
		if( srcType != zhS64 ) {
			convert( srcType, src, srcLen, zhS64, &last, &lastLen );
			unlock();
			return *(S64 *)last;
		}
		else {
			S64 val = *(S64 *)src;
			unlock();
			return val;
		}
	}
	unlock();
	return onEmpty;
}

float ZHashTable::getF( char *key, float onEmpty ) {
	clearLast();
	int srcType = 0;
	int srcLen = 0;
	lock();
	int alreadyLocked = 1;
	char *src = bgetb( key, -1, &srcLen, &srcType, 0, 0, 0, alreadyLocked );
	if( src ) {
		if( srcType != zhFLT ) {
			convert( srcType, src, srcLen, zhFLT, &last, &lastLen );
			unlock();
			return *(float *)last;
		}
		else {
			float val = *(float *)src;
			unlock();
			return val;
		}
	}
	unlock();
	return onEmpty;
}

double ZHashTable::getD( char *key, double onEmpty ) {
	clearLast();
	int srcType = 0;
	int srcLen = 0;
	lock();
	int alreadyLocked = 1;
	char *src = bgetb( key, -1, &srcLen, &srcType, 0, 0, 0, alreadyLocked );
	if( src ) {
		if( srcType != zhDBL ) {
			convert( srcType, src, srcLen, zhDBL, &last, &lastLen );
			unlock();
			return *(double *)last;
		}
		else {
			double val = *(double *)src;
			unlock();
			return val;
		}
	}
	unlock();
	return onEmpty;
}

void *ZHashTable::getp( char *key, void *onEmpty ) {
	assert( !threadSafe );
	int type = 0;
	int valLen = 0;
	char *s = bgetb( key, -1, &valLen, &type );
	if( s ) {
		assert( type == zhPTR || type == zhPFR );
		assert( valLen == sizeof(void *) );
		return *(void **)s;
	}
	return onEmpty;
}

void *ZHashTable::getpLocked( char *key, void *onEmpty ) {
    // This assumes that the caller has already manually locked the hashtable
	int type = 0;
	int valLen = 0;
    int alreadyLocked = 1;
	char *s = bgetb( key, -1, &valLen, &type, 0, 0, 0, alreadyLocked );
	if( s ) {
		assert( type == zhPTR || type == zhPFR );
		assert( valLen == sizeof(void *) );
		return *(void **)s;
	}
	return onEmpty;
}


char *ZHashTable::getb( char *key, void *onEmpty ) {
	assert( !threadSafe );
	// this gets you the address of the value in the hash (tfb 11feb2009)
	int type = 0;
	int valLen = 0;
	char *s = bgetb( key, -1, &valLen, &type );
	if( s ) {
		return s;
	}
	return (char*)onEmpty;
}


int ZHashTable::geti( char *key, int onEmpty ) {
	int type = 0;
	int valLen = 0;
	lock();
	int alreadyLocked = 1;
	char *s = bgetb( key, -1, &valLen, &type, 0, 0, 0, alreadyLocked );
	if( s ) {
		assert( type == zhS32 );
		assert( valLen == sizeof(int) );
		int val = *(int *)s;
		unlock();
		return val;
	}
	unlock();
	return onEmpty;
}

unsigned int ZHashTable::getu( char *key, unsigned int onEmpty ) {
	int type = 0;
	int valLen = 0;
	lock();
	int alreadyLocked = 1;
	char *s = bgetb( key, -1, &valLen, &type, 0, 0, 0, alreadyLocked );
	if( s ) {
		assert( type == zhU32 );
		assert( valLen == sizeof(unsigned int) );
		unsigned int val = *(unsigned int *)s;
		unlock();
		return val;
	}
	unlock();
	return onEmpty;
}

S64 ZHashTable::getl( char *key, S64 onEmpty ) {
	int type = 0;
	int valLen = 0;
	lock();
	int alreadyLocked = 1;
	char *s = bgetb( key, -1, &valLen, &type, 0, 0, 0, alreadyLocked );
	if( s ) {
		assert( type == zhS64 );
		assert( valLen == sizeof(S64) );
		S64 val = *(S64 *)s;
		unlock();
		return val;
	}
	unlock();
	return onEmpty;
}

float  ZHashTable::getf( char *key, float onEmpty ) {
	int type = 0;
	int valLen = 0;
	lock();
	int alreadyLocked = 1;
	char *s = bgetb( key, -1, &valLen, &type, 0, 0, 0, alreadyLocked );
	if( s ) {
		assert( type == zhFLT );
		assert( valLen == sizeof(float) );
		float val = *(float *)s;
		unlock();
		return val;
	}
	unlock();
	return onEmpty;
}

double ZHashTable::getd( char *key, double onEmpty ) {
	int type = 0;
	int valLen = 0;
	lock();
	int alreadyLocked = 1;
	char *s = bgetb( key, -1, &valLen, &type, 0, 0, 0, alreadyLocked );
	if( s ) {
		assert( type == zhDBL );
		assert( valLen == sizeof(double) );
		double val = *(double *)s;
		unlock();
		return val;
	}
	unlock();
	return onEmpty;
}

char  *ZHashTable::bgetS( void *key, int keyLen, char *onEmpty ) {
	assert( !threadSafe );
	clearLast();
	int srcType = 0;
	int srcLen = 0;
	char *src = bgetb( key, keyLen, &srcLen, &srcType );
	if( src ) {
		if( srcType != zhSTR ) {
			convert( srcType, src, srcLen, zhSTR, &last, &lastLen );
			return (char*)last;
		}
		else {
			return (char*)src;
		}
	}
	return onEmpty;
}

int ZHashTable::bgetI( void *key, int keyLen, int onEmpty ) {
	clearLast();
	int srcType = 0;
	int srcLen = 0;
	lock();
	int alreadyLocked = 1;
	char *src = bgetb( key, keyLen, &srcLen, &srcType, 0, 0, 0, alreadyLocked );
	if( src ) {
		if( srcType != zhS32 ) {
			convert( srcType, src, srcLen, zhS32, &last, &lastLen );
			unlock();
			return *(int *)last;
		}
		else {
			int val = *(int*)src;
			unlock();
			return val;
		}
	}
	unlock();
	return onEmpty;
}

unsigned int ZHashTable::bgetU( void *key, int keyLen, unsigned int onEmpty ) {
	clearLast();
	int srcType = 0;
	int srcLen = 0;
	lock();
	int alreadyLocked = 1;
	char *src = bgetb( key, keyLen, &srcLen, &srcType, 0, 0, 0, alreadyLocked );
	if( src ) {
		if( srcType != zhU32 ) {
			convert( srcType, src, srcLen, zhU32, &last, &lastLen );
			unlock();
			return *(unsigned int *)last;
		}
		else {
			unsigned int val = *(unsigned int *)src;
			unlock();
			return val;
		}
	}
	unlock();
	return onEmpty;
}

S64 ZHashTable::bgetL( void *key, int keyLen, S64 onEmpty ) {
	clearLast();
	int srcType = 0;
	int srcLen = 0;
	lock();
	int alreadyLocked = 1;
	char *src = bgetb( key, keyLen, &srcLen, &srcType, 0, 0, 0, alreadyLocked );
	if( src ) {
		if( srcType != zhS64 ) {
			convert( srcType, src, srcLen, zhS64, &last, &lastLen );
			unlock();
			return *(S64 *)last;
		}
		else {
			S64 val = *(S64 *)src;
			unlock();
			return val;
		}
	}
	unlock();
	return onEmpty;
}

float ZHashTable::bgetF( void *key, int keyLen, float onEmpty ) {
	clearLast();
	int srcType = 0;
	int srcLen = 0;
	lock();
	int alreadyLocked = 1;
	char *src = bgetb( key, keyLen, &srcLen, &srcType, 0, 0, 0, alreadyLocked );
	if( src ) {
		if( srcType != zhFLT ) {
			convert( srcType, src, srcLen, zhFLT, &last, &lastLen );
			unlock();
			return *(float *)last;
		}
		else {
			float val = *(float *)src;
			unlock();
			return val;
		}
	}
	unlock();
	return onEmpty;
}

double ZHashTable::bgetD( void *key, int keyLen, double onEmpty ) {
	clearLast();
	int srcType = 0;
	int srcLen = 0;
	lock();
	int alreadyLocked = 1;
	char *src = bgetb( key, keyLen, &srcLen, &srcType, 0, 0, 0, alreadyLocked );
	if( src ) {
		if( srcType != zhDBL ) {
			convert( srcType, src, srcLen, zhDBL, &last, &lastLen );
			unlock();
			return *(double *)last;
		}
		else {
			double val = *(double *)src;
			unlock();
			return val;
		}
	}
	unlock();
	return onEmpty;
}

char *ZHashTable::bgetb( void *key, int keyLen, int *valLen, int *type, int touch, char *userbuf, int *userbuflen, int alreadyLocked ) {
	// This is the workhorse "getter" that many other ZHashTable get routines use.  It returns a pointer to
	// within the hash - which has implications for thread-safety.  In the threadSafe impl, we need to lock()
	// around retrieval and use of this pointer.  If a pointer will be returned, we need to copy the record 
	// into the provided userbuf and return that instead, limiting our copy to userbuflen, and returning
	// the length of the record that was requested in userbuflen.  
	if( !key ) {
		if( valLen ) *valLen = 0;
		return 0;
	}
	if( keyLen==-1 ) keyLen = strlen( (char*)key ) + 1;

	if( !alreadyLocked ) lock();

	int hashRec = hash((char*)key,keyLen) % hashTableSize;
	int total = hashTableSize;
	while( total-- ) {
		ZHashRecord *recPtr = hashTable[hashRec];
		if( recPtr ) {
			if( !(recPtr->flags & zhDELETED) ) {
				// Non-deleted element, compare
				if( !memcmp(key, recPtr->key, keyLen) ) {
					// Found it
					if( touch ) {
						recPtr->flags |= zhCHANGED;
						hasAnyChanged = 1;
					}
					if( type ) {
						*type = recPtr->type;
					}
					if( valLen ) {
						*valLen = recPtr->valLen;
						if( recPtr->type == zhSTR ) {
							// Strings are one less than they say because we don't want to count the nul
							(*valLen)--;
						}
					}
					if( userbuf || (threadSafe && !alreadyLocked) ) {
						// If the caller has not already locked the hash, then we cannot safely
						// return a pointer to a value inside the hash.  We must make use of the 
						// userbuffer.
						assert( userbuf && userbuflen && "must supply userbuffer for thread-safety!" );
						int memcpyLen = recPtr->valLen;
						if( memcpyLen > *userbuflen ) {
							memcpyLen = *userbuflen;
						}
						memcpy( userbuf, &recPtr->key[recPtr->keyLen], memcpyLen );
						*userbuflen = recPtr->valLen;
							// note that we return the length of the value, even if all of it could not
							// be copied, allowing the caller to discover the length required.
						if( !alreadyLocked ) unlock();
						return userbuf;
					}
					if( !alreadyLocked ) unlock();
					return &recPtr->key[recPtr->keyLen];
				}
			}
		}
		else {
			// Empty record found before match, terminate search
			break;
		}
		hashRec = (hashRec+1) % hashTableSize;
	}
	if( !alreadyLocked ) unlock();

	if( valLen ) {
		*valLen = 0;
	}
	return 0;
}

void  *ZHashTable::bgetp( void *key, int keyLen, void *onEmpty ) {
	assert( !threadSafe );
	int type = 0;
	int valLen = 0;
	char *s = bgetb( key, keyLen, &valLen, &type );
	if( s ) {
		assert( type == zhPTR || type == zhPFR );
		assert( valLen == sizeof(void *) );
		return *(void **)s;
	}
	return onEmpty;
}

int ZHashTable::bgeti( void *key, int keyLen, int onEmpty ) {
	int type = 0;
	int valLen = 0;
	lock();
	int alreadyLocked = 1;
	char *s = bgetb( key, keyLen, &valLen, &type, 0, 0, 0, alreadyLocked );
	if( s ) {
		assert( type == zhS32 );
		assert( valLen == sizeof(int) );
		int val = *(int *)s;
		unlock();
		return val;
	}
	unlock();
	return onEmpty;
}

unsigned int ZHashTable::bgetu( void *key, int keyLen, unsigned int onEmpty ) {
	int type = 0;
	int valLen = 0;
	lock();
	int alreadyLocked = 1;
	char *s = bgetb( key, keyLen, &valLen, &type, 0, 0, 0, alreadyLocked );
	if( s ) {
		assert( type == zhU32 );
		assert( valLen == sizeof(unsigned int) );
		unsigned int val = *(unsigned int *)s;
		unlock();
		return val;
	}
	unlock();
	return onEmpty;
}

S64 ZHashTable::bgetl( void *key, int keyLen, S64 onEmpty ) {
	int type = 0;
	int valLen = 0;
	lock();
	int alreadyLocked = 1;
	char *s = bgetb( key, keyLen, &valLen, &type, 0, 0, 0, alreadyLocked );
	if( s ) {
		assert( type == zhS64 );
		assert( valLen == sizeof(S64) );
		S64 val = *(S64 *)s;
		unlock();
		return val;
	}
	unlock();
	return onEmpty;
}

float ZHashTable::bgetf( void *key, int keyLen, float onEmpty ) {
	int type = 0;
	int valLen = 0;
	lock();
	int alreadyLocked = 1;
	char *s = bgetb( key, keyLen, &valLen, &type, 0, 0, 0, alreadyLocked );
	if( s ) {
		assert( type == zhFLT );
		assert( valLen == sizeof(float) );
		float val = *(float *)s;
		unlock();
		return val;
	}
	unlock();
	return onEmpty;
}

double ZHashTable::bgetd( void *key, int keyLen, double onEmpty ) {
	int type = 0;
	int valLen = 0;
	lock();
	int alreadyLocked = 1;
	char *s = bgetb( key, keyLen, &valLen, &type, 0, 0, 0, alreadyLocked );
	if( s ) {
		assert( type == zhDBL );
		assert( valLen == sizeof(double) );
		double val = *(double *)s;
		unlock();
		return val;
	}
	unlock();
	return onEmpty;
}

char *ZHashTable::getKey( int i, int *keyLen, int alreadyLocked ) {
	assert( !threadSafe || alreadyLocked );
		// TODO: can make threadsafe by using userbuf
	if( keyLen ) *keyLen = 0;
	if( 0 <= i && i < hashTableSize ) {
		ZHashRecord *p = hashTable[i];
		if( p ) {
			if( p->flags & zhDELETED ) return 0;
			if( keyLen ) *keyLen = p->keyLen; 
			return p->key;
		}
	}
	return 0;
}

void  *ZHashTable::getValb( int i, int *valLen, int alreadyLocked  ) {
	assert( !threadSafe || alreadyLocked );
		// If alreadyLocked, we are assuming the caller is aware that the returned
		// value points into the hash and should be treated appropriately.
		// TODO: can make threadsafe even if not alreadyLocked by using userbuf

	if( valLen ) *valLen = 0;
	if( 0 <= i && i < hashTableSize ) {
		ZHashRecord *p = hashTable[i];
		if( p ) {
			if( p->flags & zhDELETED ) {
				return 0;
			}
			if( valLen ) {
				*valLen = p->valLen;
			}
			return &p->key[p->keyLen];
		}
	}
	return 0;
}

int ZHashTable::getFlags( int i ) {
	lock();
	if( 0 <= i && i < hashTableSize ) {
		ZHashRecord *p = hashTable[i];
		if( p ) {
			int val = p->flags;
			unlock();
			return val;
		}
	}
	unlock();
	return 0;
}

int ZHashTable::getType( int i, int alreadyLocked ) {
	if( !alreadyLocked ) lock();
	if( 0 <= i && i < hashTableSize ) {
		ZHashRecord *p = hashTable[i];
		if( p ) {
			int val = p->type;
			if( !alreadyLocked ) unlock();
			return val;
		}
	}
	if( !alreadyLocked ) unlock();
	return 0;
}

char *ZHashTable::getValS( int i ) {
	assert( !threadSafe );
		// TODO: can make threadsafe by using userbuf
	clearLast();
	if( 0 <= i && i < hashTableSize ) {
		ZHashRecord *p = hashTable[i];
		if( p ) {
			if( p->flags & zhDELETED ) return 0;
			if( p->type != zhSTR ) {
				convert( p->type, &p->key[p->keyLen], p->valLen, zhSTR, &last, &lastLen );
				return last;
			}
			else {
				return &p->key[p->keyLen];
			}
		}
	}
	return 0;
}

int ZHashTable::getValI( int i ) {
	clearLast();
	lock();
	if( 0 <= i && i < hashTableSize ) {
		ZHashRecord *p = hashTable[i];
		if( p ) {
			if( p->flags & zhDELETED ) {
				unlock();
				return 0;
			}
			if( p->type != zhS32 ) {
				convert( p->type, &p->key[p->keyLen], p->valLen, zhS32, &last, &lastLen );
				unlock();
				return *(int *)last;
			}
			else {
				int val = *(int *)&p->key[p->keyLen];
				unlock();
				return val;
			}
		}
	}
	unlock();
	return 0;
}

unsigned int ZHashTable::getValU( int i ) {
	clearLast();
	lock();
	if( 0 <= i && i < hashTableSize ) {
		ZHashRecord *p = hashTable[i];
		if( p ) {
			if( p->flags & zhDELETED ) {
				unlock();
				return 0;
			}
			if( p->type != zhU32 ) {
				convert( p->type, &p->key[p->keyLen], p->valLen, zhU32, &last, &lastLen );
				unlock();
				return *(unsigned int *)last;
			}
			else {
				unsigned int val = *(unsigned int *)&p->key[p->keyLen];
				unlock();
				return val;
			}
		}
	}
	unlock();
	return 0;
}

S64 ZHashTable::getValL( int i ) {
	clearLast();
	lock();
	if( 0 <= i && i < hashTableSize ) {
		ZHashRecord *p = hashTable[i];
		if( p ) {
			if( p->flags & zhDELETED ) {
				unlock();
				return 0;
			}
			if( p->type != zhS64 ) {
				convert( p->type, &p->key[p->keyLen], p->valLen, zhS64, &last, &lastLen );
				unlock();
				return *(S64 *)last;
			}
			else {
				S64 val = *(S64 *)&p->key[p->keyLen];
				unlock();
				return val;
			}
		}
	}
	unlock();
	return 0;
}

float  ZHashTable::getValF( int i ) {
	clearLast();
	lock();
	if( 0 <= i && i < hashTableSize ) {
		ZHashRecord *p = hashTable[i];
		if( p ) {
			if( p->flags & zhDELETED ) {
				unlock();
				return 0;
			}
			if( p->type != zhFLT ) {
				convert( p->type, &p->key[p->keyLen], p->valLen, zhFLT, &last, &lastLen );
				unlock();
				return *(float *)last;
			}
			else {
				float val = *(float *)&p->key[p->keyLen];
				unlock();
				return val;
			}
		}
	}
	unlock();
	return 0;
}

double ZHashTable::getValD( int i ) {
	clearLast();
	lock();
	if( 0 <= i && i < hashTableSize ) {
		ZHashRecord *p = hashTable[i];
		if( p ) {
			if( p->flags & zhDELETED ) {
				unlock();
				return 0;
			}
			if( p->type != zhDBL ) {
				convert( p->type, &p->key[p->keyLen], p->valLen, zhDBL, &last, &lastLen );
				unlock();
				return *(double *)last;
			}
			else {
				double val = *(double *)&p->key[p->keyLen];
				unlock();
				return val;
			}
		}
	}
	unlock();
	return 0;
}

void  *ZHashTable::getValp( int i ) {
	assert( !threadSafe );
	int valLen = 0;
	void *s = getValb( i, &valLen );
	if( s ) {
		assert( valLen == sizeof(void *) );
		return *(void **)s;
	}
	return 0;
}

int ZHashTable::getVali( int i ) {
	int valLen = 0;
	lock();
	int alreadyLocked = 1;
	void *s = getValb( i, &valLen, alreadyLocked );
	if( s ) {
		assert( valLen == sizeof(int) );
		int val = *(int *)s;
		unlock();
		return val;
	}
	unlock();
	return 0;
}

unsigned int ZHashTable::getValu( int i ) {
	int valLen = 0;
	lock();
	int alreadyLocked = 1;
	void *s = getValb( i, &valLen, alreadyLocked );
	if( s ) {
		assert( valLen == sizeof(int) );
		unsigned int val = *(unsigned int *)s;
		unlock();
		return val;
	}
	unlock();
	return 0;
}

float ZHashTable::getValf( int i ) {
	int valLen = 0;
	lock();
	int alreadyLocked = 1;
	void *s = getValb( i, &valLen, alreadyLocked );
	if( s ) {
		assert( valLen == sizeof(float) );
		float val = *(float *)s;
		unlock();
		return val;
	}
	unlock();
	return 0.f;
}

double ZHashTable::getVald( int i ) {
	int valLen = 0;
	lock();
	int alreadyLocked = 1;
	void *s = getValb( i, &valLen, alreadyLocked );
	if( s ) {
		assert( valLen == sizeof(double) );
		double val = *(double *)s;
		unlock();
		return val;
	}
	unlock();
	return 0.0;
}

int ZHashTable::isS( char *key, char *cmp ) {
	lock();
	int alreadyLocked = 1;
	char *val = getS( key, 0, 0, 0, alreadyLocked );
	if( val && cmp ) {
		int result = !strcmp( val, cmp );
		unlock();
		return result;
	}
	unlock();
	return 0;
}

// PUT
//------------------------------------------------------------------------------------------

void ZHashTable::putB( char *key, char *val, int valLen ) {
	bputB( key, strlen(key)+1, val, valLen, zhBIN );
}

void ZHashTable::putS( char *key, char *val, int valLen ) {
	bputB( key, strlen(key)+1, val, valLen, zhSTR );
}

void ZHashTable::putS( const char *key, char *val, int valLen ) {
	bputB( (char*)key, strlen(key)+1, val, valLen, zhSTR );
}

void ZHashTable::putS( char *key, const char *val, int valLen ) {
	bputB( key, strlen(key)+1, (char*)val, valLen, zhSTR );
}

void ZHashTable::putS( const char *key, const char *val, int valLen ) {
	bputB( (char*)key, strlen(key)+1, (char*)val, valLen, zhSTR );
}

void ZHashTable::putI( char *key, int val ) {
	bputB( key, strlen(key)+1, (char *)&val, sizeof(val), zhS32 );
}

void ZHashTable::putU( char *key, unsigned int val ) {
	bputB( key, strlen(key)+1, (char *)&val, sizeof(val), zhU32 );
}

void ZHashTable::putL( char *key, S64 val ) {
	bputB( key, strlen(key)+1, (char *)&val, sizeof(val), zhS64 );
}

void ZHashTable::putF( char *key, float val ) {
	bputB( key, strlen(key)+1, (char *)&val, sizeof(val), zhFLT );
}

void ZHashTable::putD( char *key, double val ) {
	bputB( key, strlen(key)+1, (char *)&val, sizeof(val), zhDBL );
}

void ZHashTable::putP( char *key, void *ptr, int freeOnReplace, int delOnNul ) {
	if( delOnNul && !ptr ) {
		del( key );
	}
	else {
		bputB( key, -1, (char *)&ptr, sizeof(ptr), freeOnReplace ? zhPFR : zhPTR );
	}
}

void ZHashTable::bputB( void *key, int keyLen, char *val, int valLen, int type, int flags, int alreadyLocked ) {
	if( keyLen == -1 ) {
		keyLen = strlen( (char*)key ) + 1;
	}
	if( valLen == -1 ) {
		valLen = 0;
		if( val ) {
			valLen = strlen( val ) + 1;
		}
	}
	if( !alreadyLocked ) lock();
	hasAnyChanged = 1;
	if( val && numRecords >= hashTableSize / 2 - 1 ) {
		// If the table is 50% utilized, then go double its size
		// and rebuild the whole hash table
		int _numRecords = 0;
		int _hashTableSize = hashTableSize * 2;
		ZHashRecord **_hashTable = (ZHashRecord **)malloc( sizeof(ZHashRecord*) * _hashTableSize );
		memset( _hashTable, 0, sizeof(ZHashRecord*) * _hashTableSize );
		for( int i=0; i<hashTableSize; i++ ) {
			if( hashTable[i] && !(hashTable[i]->flags & zhDELETED) ) {
				// a non-deleted record to add to the new table
				internalSet( _hashTable, _hashTableSize, hashTable[i]->key, hashTable[i]->keyLen, hashTable[i] );
				_numRecords++;
			}
		}
		free( hashTable );
		hashTable = _hashTable;
		hashTableSize = _hashTableSize;
		numRecords = _numRecords;

		resetCallback();
	}

	if( val ) {
		ZHashRecord *recPtr = (ZHashRecord *)malloc( sizeof(ZHashRecord) + keyLen + valLen );
		recPtr->flags = flags;
		recPtr->type = type;
		recPtr->keyLen = keyLen;
		recPtr->valLen = valLen;
		memcpy( recPtr->key, key, keyLen );
		memcpy( &recPtr->key[keyLen], val, valLen );
		numRecords += internalSet( hashTable, hashTableSize, (char*)key, keyLen, recPtr );
	}
	else {
		// Deleting record
		numRecords -= internalSet( hashTable, hashTableSize, (char *)key, keyLen, NULL );
	}
	if( !alreadyLocked ) unlock();
}

char *ZHashTable::bputBPtr( void *key, int keyLen, char *val, int valLen, int type, int flags ) {
	assert( !threadSafe );
	if( keyLen == -1 ) {
		keyLen = strlen( (char*)key ) + 1;
	}
	if( valLen == -1 ) {
		valLen = 0;
		if( val ) {
			valLen = strlen( val ) + 1;
		}
	}
	hasAnyChanged = 1;
	if( val && numRecords >= hashTableSize / 2 - 1 ) {
		// If the table is 50% utilized, then go double its size
		// and rebuild the whole hash table
		int _numRecords = 0;
		int _hashTableSize = hashTableSize * 2;
		ZHashRecord **_hashTable = (ZHashRecord **)malloc( sizeof(ZHashRecord*) * _hashTableSize );
		memset( _hashTable, 0, sizeof(ZHashRecord*) * _hashTableSize );
		for( int i=0; i<hashTableSize; i++ ) {
			if( hashTable[i] && !(hashTable[i]->flags & zhDELETED) ) {
				// a non-deleted record to add to the new table
				internalSet( _hashTable, _hashTableSize, hashTable[i]->key, hashTable[i]->keyLen, hashTable[i] );
				_numRecords++;
			}
		}
		free( hashTable );
		hashTable = _hashTable;
		hashTableSize = _hashTableSize;
		numRecords = _numRecords;

		resetCallback();
	}

	if( val ) {
		ZHashRecord *recPtr = (ZHashRecord *)malloc( sizeof(ZHashRecord) + keyLen + valLen );
		recPtr->flags = flags;
		recPtr->type = type;
		recPtr->keyLen = keyLen;
		recPtr->valLen = valLen;
		memcpy( recPtr->key, key, keyLen );
		memcpy( &recPtr->key[keyLen], val, valLen );
		numRecords += internalSet( hashTable, hashTableSize, (char*)key, keyLen, recPtr );
		return &recPtr->key[keyLen];
	}
	else {
		// Deleting record
		numRecords -= internalSet( hashTable, hashTableSize, (char *)key, keyLen, NULL );
		return 0;
	}
}

void ZHashTable::bputS( void *key, int keyLen, char *val, int valLen ) {
	bputB( key, keyLen, val, valLen, zhSTR );
}

void ZHashTable::bputI( void *key, int keyLen, int val ) {
	bputB( key, keyLen, (char *)&val, sizeof(val), zhS32 );
}

void ZHashTable::bputU( void *key, int keyLen, unsigned int val ) {
	bputB( key, keyLen, (char *)&val, sizeof(val), zhU32 );
}

void ZHashTable::bputL( void *key, int keyLen, S64 val ) {
	bputB( key, keyLen, (char *)&val, sizeof(val), zhS64 );
}

void ZHashTable::bputF( void *key, int keyLen, float val ) {
	bputB( key, keyLen, (char *)&val, sizeof(val), zhFLT );
}

void ZHashTable::bputD( void *key, int keyLen, double val ) {
	bputB( key, keyLen, (char *)&val, sizeof(val), zhDBL );
}

void ZHashTable::bputP( void *key, int keyLen, void *ptr, int freeOnReplace, int delOnNul ) {
	lock();
	if( delOnNul && !ptr ) {
		del( (char*)key, keyLen );
	}
	else {
		int alreadyLocked = 1;
		bputB( key, keyLen, (char *)&ptr, sizeof(ptr), freeOnReplace ? zhPFR : zhPTR, 0, alreadyLocked );
	}
	unlock();
}

void ZHashTable::putEncoded( char *string ) {
	if( !string || !*string ) return;

	lock();

	const int _START_KEY = 1;
	const int _START_UNIT = 2;
	const int _UNIT_QUOTE_ENDS = 3;
	const int _UNIT_SPACE_ENDS = 4;
	const int _UNIT_DONE = 5;
	const int _START_VAL = 6;

	int state = _START_KEY;
	char *buf = NULL;
	int bufCount;
	const int BUFSIZE = 8192;
		// tfb doubled this on 27 june 2014
	char key[BUFSIZE];
	char val[BUFSIZE];

	for( char *c=string; ; c++ ) {
		switch( state ) {
			case _START_KEY:
				state = _START_UNIT;
				buf = key;
				bufCount = 0;
				c--;
				break;

			case _START_UNIT:
				if( *c == '\'' ) {
					state = _UNIT_QUOTE_ENDS;
				}
				else if( *c != ' ' ) {
					state = _UNIT_SPACE_ENDS;
					c--;
				}
				break;

			case _UNIT_QUOTE_ENDS:
				if( *c == '\\' ) {
					// Escape
					c++;
					buf[bufCount++] = *c;
					assert( bufCount < BUFSIZE );
				}
				else {
					if( *c == '\'' ) {
						buf[bufCount] = 0;
						state = _UNIT_DONE;
					}
					else {
						buf[bufCount++] = *c;
						assert( bufCount < BUFSIZE );
					}
				}
				break;

			case _UNIT_SPACE_ENDS:
				if( *c == ' ' || (buf==key && *c == '=') || *c == 0 ) {
					c--;
					buf[bufCount] = 0;
					state = _UNIT_DONE;
				}
				else {
					buf[bufCount++] = *c;
					assert( bufCount < BUFSIZE );
				}
				break;

			case _UNIT_DONE:
				if( buf == key ) {
					state = _START_VAL;
				}
				else {
					putS( key, val );
					state = _START_KEY;
				}
				c--;
				break;

			case _START_VAL:
				buf = val;
				bufCount = 0;
				if( *c == '=' ) {
					state = _START_UNIT;
				}
				break;
		}
		if( *c == 0 && c >= string ) {
			break;
		}
	}

	unlock();
}

// TOUCH
//------------------------------------------------------------------------------------------

char *ZHashTable::touchS( char *key, int keyLen ) {
	assert( !threadSafe );
	char *ptr = bgetb( key, keyLen, 0, 0, 1 );
	if( !ptr ) {
		char *nulstr = "";
		ptr = bputBPtr( key, keyLen, nulstr, 1, zhSTR );
	}
	return ptr;
}

int *ZHashTable::touchI( char *key, int keyLen ) {
	assert( !threadSafe );
	char *ptr = bgetb( key, keyLen, 0, 0, 1 );
	if( !ptr ) {
		int zero = 0;
		ptr = bputBPtr( key, keyLen, (char *)&zero, sizeof(zero), zhS32 );
	}
	return (int *)ptr;
}

unsigned int *ZHashTable::touchU( char *key, int keyLen ) {
	assert( !threadSafe );
	char *ptr = bgetb( key, keyLen, 0, 0, 1 );
	if( !ptr ) {
		unsigned int zero = 0;
		ptr = bputBPtr( key, keyLen, (char *)&zero, sizeof(zero), zhU32 );
	}
	return (unsigned int *)ptr;
}

S64 *ZHashTable::touchL( char *key, int keyLen ) {
	assert( !threadSafe );
	char *ptr = bgetb( key, keyLen, 0, 0, 1 );
	if( !ptr ) {
		S64 zero = 0;
		ptr = bputBPtr( key, keyLen, (char *)&zero, sizeof(zero), zhS64 );
	}
	return (S64 *)ptr;
}

float *ZHashTable::touchF( char *key, int keyLen ) {
	assert( !threadSafe );
	char *ptr = bgetb( key, keyLen, 0, 0, 1 );
	if( !ptr ) {
		float zero = 0;
		ptr = bputBPtr( key, keyLen, (char *)&zero, sizeof(zero), zhFLT );
	}
	return (float *)ptr;
}

double *ZHashTable::touchD( char *key, int keyLen ) {
	assert( !threadSafe );
	char *ptr = bgetb( key, keyLen, 0, 0, 1 );
	if( !ptr ) {
		double zero = 0;
		ptr = bputBPtr( key, keyLen, (char *)&zero, sizeof(zero), zhDBL );
	}
	return (double *)ptr;
}

int ZHashTable::hasChanged() {
	return hasAnyChanged;
}

int ZHashTable::hasChanged( int i ) {
	lock();
	if( 0 <= i && i < hashTableSize ) {
		ZHashRecord *c = hashTable[i];
		if( c && !(c->flags & zhDELETED) ) {
			int flags = c->flags;
			unlock();
			return (flags & zhCHANGED);
		}
	}
	unlock();
	return 0;
}

int ZHashTable::hasChanged( char *key, int keyLen ) {
	lock();
	int alreadyLocked = 1;
	ZHashRecord *rec = lookup( key, keyLen, alreadyLocked );
	if( rec && !(rec->flags & zhDELETED) ) {
		int f = rec->flags;
		unlock();
		return (f & zhCHANGED);
	}
	unlock();
	return 0;
}

void ZHashTable::setChanged( int i ) {
	lock();
	if( 0 <= i && i < hashTableSize ) {
		ZHashRecord *c = hashTable[i];
		if( c && !(c->flags & zhDELETED) ) {
			c->flags |= zhCHANGED;
		}
	}
	unlock();
}

void ZHashTable::setChanged( char *key, int keyLen ) {
	lock();
	ZHashRecord *rec = lookup( key, keyLen );
	if( rec && !(rec->flags & zhDELETED) ) {
		rec->flags |= zhCHANGED;
	}
	unlock();
}

void ZHashTable::clearChanged( int i ) {
	lock();
	if( 0 <= i && i < hashTableSize ) {
		ZHashRecord *c = hashTable[i];
		if( c && !(c->flags & zhDELETED) ) {
			c->flags &= ~zhCHANGED;
		}
	}
	unlock();
}

void ZHashTable::clearChanged( char *key, int keyLen ) {
	lock();
	ZHashRecord *rec = lookup( key, keyLen );
	if( rec && !(rec->flags & zhDELETED) ) {
		rec->flags &= ~zhCHANGED;
	}
	unlock();
}

void ZHashTable::clearChangedAll() {
	lock();
	for( int i=0; i<hashTableSize; i++ ) {
		clearChanged( i );
	}
	hasAnyChanged = 0;
	unlock();
}

// FREE FUNCTIONS
//------------------------------------------------------------------------------------------

ZHashTable *zHashTable( char *fmt, ... ) {
	static char temp[4096*4];
	{
		va_list argptr;
		va_start( argptr, fmt );
		vsprintf( temp, fmt, argptr );
		va_end( argptr );
		assert( strlen(temp) < sizeof(temp) );
	}

	ZHashTable *h = new ZHashTable();
	h->putEncoded( temp );
	return h;
}

char *zHashTablePack( ZHashTable &hash, unsigned int *mallocSize/*=0*/ ) {

	hash.lock();
	int alreadyLocked = 1;

	int size = hash.size();
	int i = 0;

	char *k, *v;
	int keyLen = 0;
	int valLen = 0;
	int count = 0;

	assert( sizeof(int) == 4 );
	int len = 4 + 4 + 4 + 4;
		// Prefixed by version, endianness, len, and count

	for( i=0; i<size; i++ ) {
		k = (char *)hash.getKey( i, &keyLen, alreadyLocked );
		v = (char *)hash.getValb( i, &valLen, alreadyLocked );
		if( k ) {
			len += 4 + 4 + 4; // size of key, val, type
			len += keyLen;
			len += valLen;
			// ADD padding
			len += (4-(keyLen&3)) & 3;
			len += (4-(valLen&3)) & 3;
			count++;
		}
	}

	char zero[4] = { 0, };
	char *buf = (char *)malloc( len );
	if( mallocSize ) {
		*mallocSize = len;
	}
	char *d = buf + 16;
	for( i=0; i<size; i++ ) {
		k = (char *)hash.getKey( i, &keyLen, alreadyLocked );
		v = (char *)hash.getValb( i, &valLen, alreadyLocked );

		int keyPad = (4-(keyLen&3)) & 3;
		int valPad = (4-(valLen&3)) & 3;

		int type = hash.getType( i, alreadyLocked );
		if( k ) {
			memcpy( d, &type, 4 );
			d += 4;
			memcpy( d, &keyLen, 4 );
			d += 4;
			memcpy( d, &valLen, 4 );
			d += 4;
			memcpy( d, k, keyLen );
			d += keyLen;
			memcpy( d, zero, keyPad );
			d += keyPad;
			memcpy( d, v, valLen );
			d += valLen;
			memcpy( d, zero, valPad );
			d += valPad;
		}
	}
	
	char *version = "1.0\0";
	strcpy( buf+0, version );
		// version: so we can read older formats if changes are introduced

	*(int *)(buf+4) = zHashTableMachineIsBigEndian();
		// endianness, for possible byteswap in unpack

	*(int *)(buf+8) = len;
	*(int *)(buf+12) = count;

	hash.unlock();

	return buf;
}

int zHashTableUnpack( char *bin, ZHashTable &hash ) {

	hash.lock();
	int alreadyLocked = 1;

	char *s = bin;

	// GET VERSION
	char version[4];
	memcpy( version, s, 4 );
	assert( !strcmp( version, "1.0" ) );
	s += 4;

	// DEAL WITH ENDIAN
	int byteswap = 0;
	int bigendian = *(int *)s;
	if( bigendian > 0 ) { bigendian = 1; }
	if( bigendian != zHashTableMachineIsBigEndian() ) {
		byteswap = 1;
	}
	s += 4;

	// LENGTH and COUNT for hash entries
	int len = *(int *)s;					if( byteswap ) { BYTESWAP(len); }
	int origLen = len;
	s += 4;
	int count = *(int *)s;					if( byteswap ) { BYTESWAP(count); }
	s += 4;

	// REMOVE header
	len -= 16;

	// EXTRACT each field with padding
	for( int i=0; i<count; i++ ) {
		int type = *(int *)s;				if( byteswap ) { BYTESWAP(type); }
		s += 4;

		int keyLen = *(int *)s;				if( byteswap ) { BYTESWAP(keyLen); }
		int keyPad = (4-(keyLen&3)) & 3;	
		s += 4;

		int valLen = *(int *)s;				if( byteswap ) { BYTESWAP(valLen); }
		int valPad = (4-(valLen&3)) & 3;	
		s += 4;

		char *key = s;
		// Does the key need to be byteswapped?
		// HACKY solution: we don't know the "type" of the key, so we can't know when it needs
		// to be byte-swapped; typically keys are strings, which don't need to be.  However, 
		// if the key happens to be a numeric (atomic) type, it DOES need to be.  So try to
		// detect the use of integer keys, and byteswap them.
		if( byteswap && keyLen==4 ) {
			printf( "WARNING: ambiguous key in zHashTableUnpack(): " );
			if( isprint(key[0]) && isprint(key[1]) && isprint(key[2]) && key[3]==0 ) {
				printf( "(%s) - not swapped.\n", key );
				// looks like a string, don't byteswap
			}
			else {
				zHashTableByteSwap( (unsigned char *)key, keyLen );
				printf( "(%d) - swapped.\n", *(int*)key );
			}
		}
		s += keyLen + keyPad;
		char *val = s;						if( byteswap && type!=zhBIN && type!=zhSTR ) { zHashTableByteSwap( (unsigned char *)val, valLen ); }

		s += valLen + valPad;

		hash.bputB( key, keyLen, val, valLen, type, 0, alreadyLocked );

		len -= keyLen + valLen + keyPad + valPad + 4 + 4 + 4;
	}
	assert( len == 0 );

	hash.unlock();
	return origLen;
}

void zHashTableLoad( FILE *f, ZHashTable &hash ) {
	// f points to the size of the packed hash, but we need to peek a bit
	// further ahead to get at the endian flag in the hash.  This inelegance
	// is because the pack/unpack were originally written to contain their
	// own endian info for network etc, and by doing this look-ahead we don't
	// have to require the caller to pass a byteswap flag to this function.

	int info[3];
	fread( info, sizeof(int), 3, f );
	int dataIsBigEndian = info[2] ? 1 : 0;
	int byteswap = dataIsBigEndian == zHashTableMachineIsBigEndian() ? 0 : 1;

	fseek( f, -(long)sizeof(int)*3, SEEK_CUR );

	unsigned int bufSize;
	zHashTableFreadEndian( &bufSize, sizeof( bufSize ), 1, f, byteswap );
	char *buf = (char *)malloc( bufSize );
	fread( buf, bufSize, 1, f );
	zHashTableUnpack( buf, hash );
	free( buf );
}

void zHashTableSave( ZHashTable &hash, FILE *f ) {
	unsigned int bufSize;
	char *buf = zHashTablePack( hash, &bufSize );
	fwrite( &bufSize, sizeof( bufSize ), 1, f );
	fwrite( buf, bufSize, 1, f );
	free( buf );
}


#ifdef ZHASHTABLE_SELF_TEST
#pragma message( "BUILDING ZHASHTABLE_SELF_TEST" )
#include "math.h"

void main() {
	int i, j;

	// INSERT a bunch of things into the table and remember them in a separate list
	#define TESTS (10000)

	struct {
		char *key;
		int keyLen;
		int type;
		int size;
		union {
			char *bin;
			char *str;
			int s32;
			unsigned int u32;
			S64 s64;
			float flt;
			double dbl;
		};
	} tests[TESTS];

	ZHashTable hash;

	srand( 0 );

	for( int trials=0; trials<1000; trials++ ) {
		printf( "trial %d\n", trials );

		for( i=0; i<TESTS; i++ ) {
			//printf( "put: i = %d\n", i );
			tests[i].keyLen = rand() % 16 + 4;
			tests[i].key = (char *)malloc( tests[i].keyLen );
			for( j=0; j<tests[i].keyLen; j++ ) {
				tests[i].key[j] = rand() % 256;
			}

			tests[i].type = ( rand() % (zhDBL - zhBIN) ) + zhBIN;
			switch( tests[i].type ) {
				case zhBIN:
					tests[i].size = rand() % 100;
					tests[i].bin = (char *)malloc( tests[i].size );
					for( j=0; j<tests[i].size; j++ ) {
						tests[i].bin[j] = rand() % 256;
					}
					hash.bputB( tests[i].key, tests[i].keyLen, tests[i].bin, tests[i].size );
					break;

				case zhSTR:
					tests[i].size = rand() % 100;
					tests[i].str = (char *)malloc( tests[i].size+1 );
					for( j=0; j<tests[i].size; j++ ) {
						tests[i].str[j] = (rand() % 255) + 1;
					}
					tests[i].str[ tests[i].size ] = 0;
					hash.bputS( tests[i].key, tests[i].keyLen, tests[i].str );
					break;

				case zhS32:
					tests[i].s32 = rand();
					hash.bputI( tests[i].key, tests[i].keyLen, tests[i].s32 );
					break;

				case zhU32:
					tests[i].u32 = rand();
					hash.bputU( tests[i].key, tests[i].keyLen, tests[i].u32 );
					break;

				case zhS64:
					tests[i].s64 = (rand()<<32) | rand();
					hash.bputL( tests[i].key, tests[i].keyLen, tests[i].s64 );
					break;

				case zhFLT:
					tests[i].flt = (float)rand();
					hash.bputF( tests[i].key, tests[i].keyLen, tests[i].flt );
					break;

				case zhDBL:
					tests[i].dbl = (double)rand();
					hash.bputD( tests[i].key, tests[i].keyLen, tests[i].dbl );
					break;

				//case zhPTR:
				//case zhPFR:
			}
		}

		// QUERY back out
		for( i=0; i<TESTS; i++ ) {
			//printf( "get: i = %d\n", i );
			switch( tests[i].type ) {
				case zhBIN: {
					int valLen;
					char *val = hash.bgetb( tests[i].key, tests[i].keyLen, &valLen );
					assert( valLen == tests[i].size );
					assert( ! memcmp( val, tests[i].bin, tests[i].size ) );
					break;
				}

				case zhSTR: {
					char *val = hash.bgetS( tests[i].key, tests[i].keyLen );
					assert( (int)strlen(val) == tests[i].size );
					assert( ! strcmp( val, tests[i].str ) );
					break;
				}

				case zhS32: {
					int val = hash.bgetI( tests[i].key, tests[i].keyLen );
					assert( val == tests[i].s32 );
					break;
				}

				case zhU32: {
					unsigned int val = hash.bgetU( tests[i].key, tests[i].keyLen );
					assert( val == tests[i].u32 );
					break;
				}

				case zhS64: {
					S64 val = hash.bgetL( tests[i].key, tests[i].keyLen );
					assert( val == tests[i].s64 );
					break;
				}

				case zhFLT: {
					float val = hash.bgetF( tests[i].key, tests[i].keyLen );
					assert( val == tests[i].flt );
					break;
				}

				case zhDBL: {
					double val = hash.bgetD( tests[i].key, tests[i].keyLen );
					assert( val == tests[i].dbl );
					break;
				}

				//case zhPTR:
				//case zhPFR:
			}
		}
	}

	// Test that last is getting deleted
	// Test conversions
	// Test all the gets and puts
	// Test the get(int i) are workign
	// Test isS
	// Test touches
	// Test Encoded
	// Test del
	// Test change flags
	// Test pointers, pointer frees
	// test on empty


/*
	ZHashTable table( 1 );
	table.putS( "key1", "val1" );
	table.putS( "key2", "val2" );

	// test that it doesn't find key3
	assert( table.getS( "key3" ) == 0 );

	// test that it does find keys 1 & 2
	assert( !strcmp( table.getS( "key1" ), "val1" ) );
	assert( !strcmp( table.getS( "key2" ), "val2" ) );

	// test deleting key1
	table.putS( "key1", 0 );
	assert( table.getS( "key1" ) == 0 );

	// test all of the various binary and text funcs
	char binKey[4] = { (char)0x00, (char)0x01, (char)0x00, (char)0x02 };
	char binVal[4] = { (char)0x00, (char)0x00, (char)0xFF, (char)0x00 };
	table.bputB( binKey, 4, binVal, 4 );

	int fetchBinValLen = 0;
	char *fetchBinVal = table.get ( binKey, 4, &fetchBinValLen );
	assert( !memcmp( fetchBinVal, binVal, 4 ) );
	assert( fetchBinValLen == 4 );

	table.putS( "key3", "123456789", 4 );
	assert( !memcmp( table.getS("key3"), "1234", 4 ) );

	table.putI( "key4", 1234 );
	assert( table.getI("key4") == 1234 );

	table.putF( "key5", 1234.56f );
	assert( fabs(table.getF("key5") - 1234.56f) < 0.01f );

	table.putD( "key6", 1234.56789 );
	assert( table.getD("key6") == 1234.56789 );

	table.puti( "key7", 0xFFFFFFFF );
	assert( table.geti("key7") == -1 );

	table.putf( "key8", 1234.56f );
	assert( table.getf("key8") == 1234.56f );

	table.putd( "key9", 1234.56 );
	assert( table.getd("key9") == 1234.56 );

	// test the buffer usage
	char *buf0 = (char *)malloc( 6 );
	strcpy( buf0, "test0" );
	char *buf1 = (char *)malloc( 6 );
	strcpy( buf1, "test1" );
	table.putp( "key10", buf0, 1 );
	assert( table.getp("key10") == buf0 );

	// test that it get's freed when it is overwritten
	table.putp( "key10", buf1, 1 );

	#ifdef _DEBUG
	#ifdef WIN32
	// In win32 debug the freed buffer will be overwritten with "0xdd"
	assert( table.getp("key10") == buf1 );
	assert( !memcmp( buf0, "\xdd\xdd\xdd\xdd\xdd", 5 ) );
	#endif
	#endif

	// test that it isn't freed if it isn't asked to
	table.putp( "key11", buf1, 0 );
	table.putp( "key11", 0 );
	assert( table.getp("key11") == 0 );
	assert( !memcmp( buf1, "test1", 5 ) );


	table.iputS( 0, "123456789", 4 );
	assert( !memcmp( table.igetS(0), "1234", 4 ) );

	table.iputI( 1, 1234 );
	assert( table.igetI(1) == 1234 );

	table.iputF( 2, 1234.56f );
	assert( fabs(table.igetF(2) - 1234.56f) < 0.01f );

	table.iputD( 3, 1234.56789 );
	assert( table.igetD(3) == 1234.56789 );

	table.iputi( 4, 0xFFFFFFFF );
	assert( table.igeti(4) == -1 );

	table.iputf( 5, 1234.56f );
	assert( table.igetf(5) == 1234.56f );

	table.iputd( 6, 1234.56 );
	assert( table.igetd(6) == 1234.56 );

	char *buf2 = (char *)malloc( 5 );
	strcpy( buf2, "test" );
	table.iputp( 7, buf2 );
	assert( !strcmp((char*)table.igetp(7),"test") );

	// @TODO
	// test all the the getVal types
	// test that the pointer type works and frees correctly
	// even when there is a double delete

	// test double delete
	table.clear( 4 );
	table.putS( "key1", "val1" );
	table.putS( "key1", 0 );
	table.putS( "key1", "val2" );
	table.putS( "key1", 0 );
	table.putS( "key1", "val3" );


	table.putEncoded( "key1=val1 'key 2'=val2 key3='val 3' 'key\\'4'='val\\'4'" );
	assert( !strcmp(table.getS( "key1" ), "val1") );
	assert( !strcmp(table.getS( "key 2" ), "val2") );
	assert( !strcmp(table.getS( "key3" ), "val 3") );
	char *a = table.getS( "key'4" );
	assert( !strcmp(a, "val'4") );

	table.putI( "inttest", 0xffffffff );
	int b = table.getI( "inttest" );
*/
}

#endif


