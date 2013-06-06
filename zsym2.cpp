// @ZBS {
//		*MODULE_NAME ZSym
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A heirarchical file-system-like symbol table
//		}
//		*PORTABILITY win32 unix macosx
//		*REQUIRED_FILES zsym.cpp zsym.h
//		*VERSION 0.1
//		+HISTORY {
//			0.0 ZBS First draft
//			0.1 ZBS First working version 15 Jan 2007
//		}
//		*SELF_TEST yes console ZSYM_SELF_TEST
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
#ifdef WIN32
#include "malloc.h"
#endif
// MODULE includes:
#include "zsym2.h"

#include "zwildcard.h"
	// This is only used for FILE until I reimplement
	// @TODO remove this dependency

#ifndef min
	#define min(a,b) ( ( (a) < (b) ) ? (a) : (b) )
#endif
#ifndef max
	#define max(a,b) ( ( (a) > (b) ) ? (a) : (b) )
#endif

//==================================================================================
// ZSymProtocol
//==================================================================================

struct ZSymProtocol {
	// A protocol is a pure virtual interface for hooking
	// the different kinds of protocols such as file, http, etc
	// Eventually this will be extracted from this file and a factory will be made

	#define zsymPROTOCOL_COUNT (2)
	static ZSymProtocol *protocols[zsymPROTOCOL_COUNT];
	static char *protocolNames[zsymPROTOCOL_COUNT];
		// @TODO: This will become a factory at some point

	virtual void incRef( void *context )=0;
	virtual void decRef( void *context )=0;

	virtual int resolve( int segCount, char **segs, int *segLens, void **context, int *err, int flags )=0;

	virtual int bestConversion( void *context, int srcType )=0;
	virtual void getRaw( void *context, char **rawPtr, int *rawLen, int *err )=0;
	virtual void putRaw( void *context, int fileType, char *rawPtr, int rawLen, int *err )=0;
	virtual int cat( void *context, char *s, int len, int growBy, int *err )=0;

	virtual void getType( void *context, int *nodeType, int *fileType )=0;
	virtual void changeType( void *context, int nodeType )=0;

	virtual void *listStart( void *context )=0;
	virtual int listNext( void *listContext, char **s, int *len, int *nodeType )=0;

	virtual void dupContext( void *srcContext, void **dstContext )=0;
	virtual int createLink( void *context, char *target, int targetLen, int *err )=0;
	virtual int rm( void *context, int *err, int safetyOverride, int childrenOnly )=0;
	virtual void getName( void *context, char **name, int *len, int global )=0;
	virtual void dump( void *context, int toStdout, int indent )=0;

	virtual int getListStats( void *context, int &minVal, int &maxVal )=0;
};

//==================================================================================
// ZSymHash
//==================================================================================

struct ZSymHash {
	struct ZSymHashRecord {
		int flags;
			#define zhNONE    (0x00)
			#define zhDELETED (0x01)
			#define zhCHANGED (0x02)
		int keyLen;
		int valLen;
		char key[1];
			// This is really keyLen long
			// The val follows this after keyLen bytes
		char *getKeyPtr() { return key; }
		char *getValPtr() { return &key[ keyLen + ((4-(keyLen&3))&3) ]; }
	};

	int collisions;
		// Incremented each time the set() function collides.
		// Useful for profiling the hash function
	int initialSize;
		// How big the initial array is
	int size;
		// Number of pointers in the hash table
	int numRecords;
		// Actual active records in the table
	ZSymHashRecord **hashTable;
		// Array of hash records

	static unsigned int hash( char *key, int keyLen );
		// Hashing function. currently using the one from http://burtleburtle.net/bob/hash/doobs.html

	int set( ZSymHashRecord **_hashTable, int _size, char *key, int keyLen, ZSymHashRecord *_recPtr );
		// Internal method to functionalize rebuilds of tables
		// Returns 1 if the key is new, 0 if it is replacing an existing
	void init( int _initSize=64 );
		// Setup with initial size
	void clear( int _initSize=-1, int freeFirst=1 );
		// Kill everything, start fresh.  Will reset initSize if you request it

	char *get( char *key, int keyLen, int *valLen );
	char *put( char *key, int keyLen, char *val, int valLen );
	char *getKey( int i, int *keyLen=0 );
	void *getVal( int i, int *valLen=0 );
	void del( char *key, int keyLen ) { put( key, keyLen, (char*)0, 0 ); }
	void del( int i );

	ZSymHash( int _initialSize=64 );
	~ZSymHash();
};

unsigned int ZSymHash::hash( char *k, int keyLen ) {
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

int ZSymHash::set( ZSymHashRecord **_hashTable, int _size, char *key, int keyLen, ZSymHashRecord *_recPtr ) {
	// The _recPtr might be nul if we are deleting which is why key and keyLen are passed in separately
	// However, in the case that this isn't a delete, this fucntion expects that _recPtr already has the
	// key, keyLen, valLen and val all setup
	int index = hash( key, keyLen ) % _size;
	int total = _size;
	int totalDeleted = 0;
	int foundKey = 0;
	ZSymHashRecord *recPtr;
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
			else if( keyLen==recPtr->keyLen && !memcmp(key, recPtr->getKeyPtr(), keyLen) ) {
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
		index = (index+1) % _size;
	}
	assert( total + totalDeleted );
		// There should always be some room left since we keep
		// the list at 50% free all the time to avoid collisions
	
	// Here, destIndex contains the index of the found record. The found record value can
	// be either NULL, a pointer to a deleted entry, or a pointer to the found entry.
	
	ZSymHashRecord *delRec = _hashTable[dstIndex];

	if( _recPtr == NULL ) {
		// We are deleting the key, it is only marked as deleted it is not actually freed
		// until the value is replaced. This sucks because the val might be huge
		// and we don't need it around anymore.  So we do a check here to see if it is
		// worth reallocating the hash record.
		if( foundKey ) {
			// Only delete it if it already exists
			if( delRec->valLen > 64 ) {
				// Probably worth it to reallocate
				ZSymHashRecord *r = (ZSymHashRecord *)malloc( sizeof(ZSymHashRecord) + _recPtr->keyLen );
				memcpy( r, recPtr, sizeof(ZSymHashRecord) + _recPtr->keyLen );
				r->valLen = 0;
				free( delRec );
				_hashTable[dstIndex] = r;
				delRec = r;
			}
			delRec->flags = zhDELETED;
			return 1;
		}
		return 0;
	}
	else {
		// We are either adding a new key or setting the value of an old key.
		// Either way, if the destination record has a value, it must be deleted
		// before the new value is set.
		if( delRec ) {
			free( delRec );
		}
		_hashTable[dstIndex] = _recPtr;
	}

	return !foundKey;
}

ZSymHash::ZSymHash( int _initialSize ) {
	initialSize = _initialSize;
	hashTable = 0;
	clear();
}

ZSymHash::~ZSymHash() {
	if( hashTable ) {
		for( int i=0; i<size; i++ ) {
			ZSymHashRecord *p = hashTable[i];
			if( p ) {
				free( p );
			}
		}
		
		free( hashTable );
	}
}

char *ZSymHash::get( char *key, int keyLen, int *valLen ) {
	if( valLen ) *valLen = 0;
	if( !key ) {
		return 0;
	}
	if( keyLen==-1 ) keyLen = strlen( key ) + 1;
	int hashRec = hash(key,keyLen) % size;
	int total = size;
	while( total-- ) {
		ZSymHashRecord *recPtr = hashTable[hashRec];
		if( recPtr ) {
			if( !(recPtr->flags & zhDELETED) ) {
				// Non-deleted element, compare
				if( !memcmp(key, recPtr->getKeyPtr(), keyLen) ) {
					// Found it
					if( valLen ) *valLen = recPtr->valLen;
					return recPtr->getValPtr();
				}
			}
		}
		else {
			// Empty record found before match, terminate search
			break;
		}
		hashRec = (hashRec+1) % size;
	}

	return 0;
}

char *ZSymHash::getKey( int i, int *keyLen ) {
	if( keyLen ) *keyLen = 0;
	if( 0 <= i && i < size ) {
		ZSymHashRecord *p = hashTable[i];
		if( p ) {
			if( p->flags & zhDELETED ) return 0;
			if( keyLen ) *keyLen = p->keyLen;
			return p->getKeyPtr();
		}
	}
	return 0;
}

void *ZSymHash::getVal( int i, int *valLen ) {
	if( valLen ) *valLen = 0;
	if( 0 <= i && i < size ) {
		ZSymHashRecord *p = hashTable[i];
		if( p ) {
			if( p->flags & zhDELETED ) return 0;
			if( valLen ) *valLen = p->valLen;
			return p->getValPtr();
		}
	}
	return 0;
}

char *ZSymHash::put( char *key, int keyLen, char *val, int valLen ) {
	if( keyLen == -1 ) {
		keyLen = strlen( key ) + 1;
	}
	if( valLen == -1 ) {
		valLen = 0;
		if( val ) {
			valLen = strlen( val ) + 1;
		}
	}
	if( val && numRecords >= size / 2 - 1 ) {
		// If the table is 50% utilized, then go double its size
		// and rebuild the whole hash table
		int _numRecords = 0;
		int _size = size * 2;
		ZSymHashRecord **_hashTable = (ZSymHashRecord **)malloc( sizeof(ZSymHashRecord*) * _size );
		memset( _hashTable, 0, sizeof(ZSymHashRecord*) * _size );
		for( int i=0; i<size; i++ ) {
			if( hashTable[i] && !(hashTable[i]->flags & zhDELETED) ) {
				// a non-deleted record to add to the new table
				set( _hashTable, _size, hashTable[i]->key, hashTable[i]->keyLen, hashTable[i] );
				_numRecords++;
			}
		}
		free( hashTable );
		hashTable = _hashTable;
		size = _size;
		numRecords = _numRecords;
	}

	if( val ) {
		int keyPad = (4-(keyLen&3))&3;
		ZSymHashRecord *recPtr = (ZSymHashRecord *)malloc( sizeof(ZSymHashRecord) + keyLen + keyPad + valLen );
		recPtr->flags = 0;
		recPtr->keyLen = keyLen;
		recPtr->valLen = valLen;
		memcpy( recPtr->getKeyPtr(), key, keyLen );
		memset( recPtr->getKeyPtr()+keyLen, 0, keyPad );
		memcpy( recPtr->getValPtr(), val, valLen );
		numRecords += set( hashTable, size, key, keyLen, recPtr );
		return &recPtr->key[keyLen];
	}
	else {
		// Deleting record
		numRecords -= set( hashTable, size, key, keyLen, 0 );
		return 0;
	}
}

void ZSymHash::del( int i ) {
	int keyLen = 0;
	char *key = getKey( i, &keyLen );
	numRecords -= set( hashTable, size, key, keyLen, 0 );
}

void ZSymHash::init( int _initSize ) {
	clear( _initSize, 1 );
}

void ZSymHash::clear( int _initSize, int freeFirst ) {
	if( hashTable && freeFirst ) {
		for( int i=0; i<size; i++ ) {
			if( hashTable[i] ) {
				free( hashTable[i] );
			}
		}
		free( hashTable );
	}
	if( _initSize > 0 ) {
		initialSize = _initSize;
	}
	size = initialSize;
	numRecords = 0;
	hashTable = 0;
	collisions = 0;
	if( initialSize ) {
		hashTable = (ZSymHashRecord **)malloc( sizeof(ZSymHashRecord *) * size );
		memset( hashTable, 0, sizeof(ZSymHashRecord *) * size );
	}
}

//==================================================================================
// ZSymRoutine
//==================================================================================

struct ZSymRoutine {
	typedef int (*ZSymRoutineFuncPtr)();
	ZSymRoutineFuncPtr funcPtr;
	char argStr[1];

	void call( ZSym &args, ZSym &ret ) {
		int len = strlen( argStr );
		char *_argStr = (char *)alloca( len + 1 );
		memcpy( _argStr, argStr, len+1 );

		char *end = _argStr + len;

		for( char *c = end; c>=_argStr;  ) {
			// SCAN backwards for a space or start
			while( c > _argStr && *c != ' ' ) {
				// Found the beginning of an argument
				c--;
			}
			if( c > _argStr ) {
				c++;
			}
			char *nameStart = c;

			// SCAN forwards for a colon
			char *d = c;
			while( d < end && *d != ':' ) {
				d++;
			}
			*d = 0;

			d++;
			char *typeStart = d;
			d++;

			char *defaultValStart = 0;
			if( *d == '=' ) {
				// A default value is given
				// SCAN forward for space or end
				defaultValStart = d+1;
				while( d < end && *d != ' ' ) {
					d++;
				}
				*d = 0;
			}

			switch( *typeStart ) {
				case 'i': {
					int a = args.getI( nameStart, -1, defaultValStart ? atoi(defaultValStart) : 0 );
					int *push = (int *)alloca( 4 );
					*push = a;
					break;
				}
			}

			// SCAN backwards avoiding spaces
			c--;
			while( c > _argStr && *c == ' ' ) {
				c--;
			}
		}
		int r = (*funcPtr)();
		switch( *_argStr ) {
			case 'i': {
				ret.putI( r );
				break;
			}
			case 'z': {
				break;
			}
		}
	}
};

//==================================================================================
// ZSymContextMem
//==================================================================================

struct ZSymContextMem {
	struct ZSymMemFile {
		int fileType;
			// See fileTypes in header
		union {
			int valI;
			float valF;
			double valD;
			char *valS;
				// Prefixes the str with an int allocLen and another int "length"
				// Always nul terminates after the str
			int *valIP;
		};

		void clear() {
			if( fileType == zsymS ) {
				free( valS );
			}
			fileType = zsymZ;
			valD = 0.0;
		}

		ZSymMemFile() {
			fileType = zsymZ;
			valD = 0.0;
		}

		~ZSymMemFile() {
			clear();
		}
	};

	int nodeType;
	int refCount;
	union {
		ZSymContextMem *parent;
		ZSymContextMem *orphanPrev;
	};
	union {
		void *unk;
		ZSymMemFile *file;
		char *link;
		ZSymHash *dir;
		ZSymContextMem *orphanNext;
	};

	static ZSymContextMem *root;
	static ZSymContextMem *orphanHead;
	static int allocCount;
	static int orphanCount;

	void createLink( char *target, int targetLen=-1 ) {
		clear();
		nodeType = zsymLINK;
		link = strdup( target );
		// @TODO links should be able to be binary keys!
	}

	void clear() {
		if( nodeType == zsymDIR ) {
			assert( dir->numRecords == 0 );
			delete dir;
			dir = 0;
		}
		else if( nodeType == zsymFILE ) {
			delete file;
			file = 0;
		}
		else if( nodeType == zsymLINK ) {
			free( link );
			link = 0;
		}
		nodeType = zsymUNK;
	}

	void setType( int _nodeType ) {
		assert( nodeType == zsymUNK );
		nodeType = _nodeType;
		if( nodeType == zsymDIR ) {
			dir = new ZSymHash;
		}
		else if( nodeType == zsymFILE ) {
			file = new ZSymMemFile;
		}
		else if( nodeType == zsymLINK ) {
			assert( 0 );
		}
	}

	void orphan() {
		// Puts the context into the orphan list
		clear();

		// LINK into the orphan list
		orphanCount++;
		if( orphanHead ) {
			orphanHead->orphanPrev = this;
		}
		orphanNext = orphanHead;
		orphanPrev = 0;
		orphanHead = this;
	}

	void incRef() { refCount++; }

	void ZSymContextMem::decRef() {
		refCount--;
		assert( refCount >= 0 );
		if( refCount == 0 ) {
			assert( nodeType == zsymUNK );
			delete this;
		}
	}

	void rm( int childrenOnly ) {
		assert( this != root );

		// REMOVE children
		if( nodeType == zsymDIR ) {
			recurseRemoveChildren();
			if( childrenOnly ) {
				return;
			}
		}

		// REMOVE from parent directory
		if( parent ) {
			ZSymHash *parentDir = parent->dir;
			int size = parentDir->size;
			for( int i=0; i<size; i++ ) {
				int valLen = 0;
				ZSymContextMem **ni = (ZSymContextMem **)parentDir->getVal( i, &valLen );
				if( ni && *ni == this ) {
					assert( valLen == sizeof(void*) );
					parentDir->del( i );
					decRef();
					break;
				}
			}
		}

		orphan();
	}

	void recurseRemoveChildren() {
		int i;
		assert( nodeType == zsymDIR );
		int size = dir->size;
		for( i=0; i<size; i++ ) {
			int valLen = 0;
			ZSymContextMem **si = (ZSymContextMem **)dir->getVal(i,&valLen);
			if( si ) {
				assert( valLen == sizeof(void*) );
				if( (*si)->nodeType == zsymDIR ) {
					(*si)->recurseRemoveChildren();
				}
				int keyLen = 0;
				char *key = dir->getKey(i,&keyLen);
				dir->del( key, keyLen );
				(*si)->orphan();
				(*si)->decRef();
			}
		}
	}

	ZSymContextMem( int _nodeType, ZSymContextMem *_parent ) {
		parent = _parent;
		refCount = 1;
		nodeType = zsymUNK;
		setType( _nodeType );
		allocCount++;
	}

	~ZSymContextMem() {
		clear();
		if( nodeType == zsymUNK ) {
			// REMOVE from the orphan list
			orphanCount--;
			if( orphanPrev ) {
				orphanPrev->orphanNext = orphanNext;
			}
			else {
				assert( orphanHead == this );
				orphanHead = orphanNext;
			}
			if( orphanNext ) {
				orphanNext->orphanPrev = orphanPrev;
			}
		}
		allocCount--;
	}
};

ZSymContextMem *ZSymContextMem::root = 0;
ZSymContextMem *ZSymContextMem::orphanHead = 0;
int ZSymContextMem::allocCount = 0;
int ZSymContextMem::orphanCount = 0;


// @TODO:
// ALl this code would be a lot simpler if only the mem hash
// could be just one giant hash table instead of a heirarchy of
// nested tables.  This would require some cool tricky hash algorithm
// that would let you do fast searches like "/dir1/dir2/*"
// Of course it also would mean dealing with a ton of
// escape sequence issues since we want to be able to
// key on binary sequences.  Is there some clever hash that
// would have such a property?  It smells like some sort of compression
// algorithm where you see that a key like /dir1/dir2/ is common and
// you assign some sort of code to that.  I guess if I put the constraint
// that you can only search on a subdir bounray which is pretty reasonable
// then I could generate a hash key on each sub dir and then stick them
// together in some clever way like "/dir1" and "/dir2" the combo
// of them get concatted.  Then when you want the contents of /dir1/dir2
// you find everything that starts with the hash... but that starts to
// sound like storing the whole heirarchy again.  I wonder if there
// are cool disk systems that do this?
// 
// Here's an idea.  You break all things into key divided up by dir
// eg "/dir1/dir2/a" becomes: dir1 dir2 a.  You have a hash that
// assigns each atom to a prime number.  Now you do a lookup
// into the main hash by computing the composite of those primes
// and then it is a regular old hash table
// When you want to do wildcards, find all keys /dir1/*
// then you take the whole database index and you go through
// it and you can quickly divide to see if that composite
// is divisible with that prime.  Although you are scanning
// the whole index, the operation is fast, seems pretty clean
// since it is uncommon to want a wildcard and common to
// want to just read and write.


//==================================================================================
// ZSymProtocolMem
//==================================================================================

struct ZSymProtocolMem : ZSymProtocol {
	virtual int resolve( int segCount, char **segs, int *segLens, void **context, int *err, int flags ) {
		*err = 0;

		// INIT root if it hasn't been
		if( !ZSymContextMem::root ) {
			ZSymContextMem::root = new ZSymContextMem( zsymDIR, 0 );
		}

		// SETUP the currentDir with either the current context or root by default
		int startSeg = 0;
		ZSymContextMem *currentDir = 0;
		if( segLens[0] == 0 && segCount > 1 ) {
			// The segCount> 1 is here so that you can re-resolve a symvol 
			// for eample after it has been converted to a link by using an empty string
			currentDir = ZSymContextMem::root;
			startSeg++;
			if( segCount == 2 && segLens[1] == 0 ) {
				// Special case, root reference
				if( flags & zsymFORCE_TO_FILE ) {
					*err = zsymRESOLVE_CANNOT_CONVERT_ROOT;
					*context = 0;
					return 0;
				}

				*(ZSymContextMem **)context = ZSymContextMem::root;
				*err = 0;
				return 1;
			}
		}

		// CHECK for links
		ZSymContextMem *contextNode = *(ZSymContextMem **)context;
		if( contextNode && contextNode->nodeType == zsymLINK ) {
			// @TODO Links non nul terminated
			*(char **)context = contextNode->link;
			*err = zsymRESOLVE_LINK;
			return 0;
		}

		if( segLens[0] != 0 && contextNode && contextNode->nodeType == zsymDIR ) {
			// This is a relative reference
			currentDir = contextNode;
		}

		if( !currentDir ) {
			// Trying to reference a subdomain off a non existing context
			*err = zsymRESOLVE_NOT_FOUND;
			return 0;
		}

		ZSymContextMem *node = 0;
		for( int i=startSeg; i < segCount; i++ ) {
			if( segLens[i] == 0 ) {
				// Terminator
				break;
			}

			// CHECK for special . & ..
			if( segs[i][0] == '.' ) {
				if( segs[i][1] == '.' && segs[i][1] == '.' && segLens[i] == 2 ) {
					// Parent reference, bounded to stop it going above root
					if( currentDir->parent ) {
						currentDir = currentDir->parent;
					}
					else {
						*err = zsymRESOLVE_NOT_FOUND;
						return 0;
					}
					*(ZSymContextMem **)context = currentDir;
					node = currentDir;
					continue;
				}
				else if( segLens[i] == 1 ) {
					// Self reference
					continue;
				}
			}

			int valLen = 0;
			ZSymContextMem **nodePtr = (ZSymContextMem **)currentDir->dir->get( segs[i], segLens[i], &valLen );
			node = nodePtr ? *nodePtr : 0;
			if( node ) {
				assert( valLen == sizeof(void*) );

				*(ZSymContextMem **)context = node;

				if( node->nodeType == zsymLINK ) {
					// Encounter a link so stop processing and hand this link back to the caller
					// @TODO binary link names
					if( flags & zsymDONT_FOLLOW_LINKS ) {
						*err = 0;
						return 1;
					}
					else {
						*(char **)context = node->link;
						*err = zsymRESOLVE_LINK;
						return i+1;
					}
				}
				else if( node->nodeType == zsymDIR ) {
					currentDir = node;
				}
				else if( node->nodeType == zsymFILE ) {
					if( i == segCount-1 ) {
						// The last segment is a file, all is good
						break;
					}
					else {
						// Otherwise they are asking for a subdir of a file 
						// (we know this because this is not the last segment)
						// If they want to CREATE then that file has to be converted
						// to a subdirectory and the resolving has to continue
						if( flags & zsymOPEN_OR_CREATE ) {
							// CONVERT the file to a dir
							node->clear();
							node->nodeType = zsymDIR;
							node->dir = new ZSymHash;
							currentDir = node;
						}
						else {
							*err = zsymRESOLVE_FILE_NOT_DIR;
							return 0;
						}
					}
				}
				else {
					// Openning a nul.  This is allowed if you are
					//assert( 0 );
					int a = 1;
				}
			}
			else if( flags & zsymOPEN_OR_CREATE ) {
				// Node not found but we are requested to make it.
				// Make it as a directory.  If this isn't correct
				// it will be remade as a file below
				node = new ZSymContextMem( zsymDIR, currentDir );
				currentDir->dir->put( segs[i], segLens[i], (char*)&node, sizeof(void*) );
				currentDir = node;
				*(ZSymContextMem **)context = node;
			}
			else {
				// Symbol not found
				*err = zsymRESOLVE_NOT_FOUND;
				return 0;
			}
		}

		int wantsDir  = (flags&zsymFORCE_TO_DIR ) || ( (flags&zsymFORCE_AS_SPECIFIED) && segLens[segCount-1]==0 );
		int wantsFile = (flags&zsymFORCE_TO_FILE) || ( (flags&zsymFORCE_AS_SPECIFIED) && segLens[segCount-1]!=0 );
		assert( (flags & zsymOPEN_OR_FAIL) || (wantsDir ^ wantsFile) );

		if( node && node->nodeType == zsymDIR && wantsFile ) {
			// CONVERT the dir to a file
			if( ! node->parent ) {
				*err = zsymRESOLVE_CANNOT_CONVERT_ROOT;
				return 0;
			}
			node->recurseRemoveChildren();
			node->clear();
			node->nodeType = zsymFILE;
			node->file = new ZSymContextMem::ZSymMemFile;
		}
		else if( node && node->nodeType == zsymFILE && wantsDir ) {
			// CONVERT the file to dir
			node->clear();
			node->nodeType = zsymDIR;
			node->dir = new ZSymHash;
		}
		return 1;
	}

	virtual int bestConversion( void *context, int srcType ) {
		ZSymContextMem *node = (ZSymContextMem *)context;
		if( node && node->nodeType == zsymFILE ) {
			// If the target is a pointer type, then the conversion
			// will stay the dst pointer type...
			switch( node->file->fileType ) {
				case zsymIP: return zsymIPs;
				case zsymFP: return zsymFPs;
				case zsymDP: return zsymDPs;
				case zsymSP: return zsymSPs;
			}
		}
		// ... otherwise it will be the src type
		return srcType;
	}

	virtual void dupContext( void *srcContext, void **dstContext ) {
		*dstContext = srcContext;
		// There's no allocated data on this context so it is just a copy
		// @TODO: Explain better
	}

	virtual void getType( void *context, int *nodeType, int *fileType ) {
		*fileType = 0;
		*nodeType = 0;
		ZSymContextMem *node = (ZSymContextMem *)context;
		if( node ) {
			if( node->nodeType == zsymFILE ) {
				*fileType = node->file->fileType;
			}
			*nodeType = node->nodeType;
		}
	}

	virtual void changeType( void *context, int nodeType ) {
		ZSymContextMem *node = (ZSymContextMem *)context;
		if( node ) {
			if( node->nodeType != nodeType ) {
				node->clear();
				node->setType( nodeType );
			}
		}
	}

	virtual int rm( void *context, int *err, int safetyOverride, int childrenOnly ) {
		ZSymContextMem *node = (ZSymContextMem *)context;
		if( node ) {
			node->rm( childrenOnly );
		}
		return 1;
	}

	virtual int createLink( void *context, char *target, int targetLen, int *err ) {
		ZSymContextMem *node = (ZSymContextMem *)context;
		if( node ) {
			node->createLink( target, targetLen );
			*err = 0;
			return 1;
		}
		return 0;
	}

	virtual void incRef( void *context ) {
		ZSymContextMem *node = (ZSymContextMem *)context;
		if( node ) {
			node->incRef();
		}
	}

	virtual void decRef( void *context ) {
		ZSymContextMem *node = (ZSymContextMem *)context;
		if( node ) {
			node->decRef();
		}
	}

	virtual void getRaw( void *context, char **rawPtr, int *rawLen, int *err ) {
		*err = 0;
		ZSymContextMem *node = (ZSymContextMem *)context;
		assert( node );
		if( node->nodeType == zsymUNK ) {
			*rawPtr = 0;
			*rawLen = 0;
			return;
		}
		assert( node->nodeType == zsymFILE );
		switch( node->file->fileType ) {
			case zsymI:
				*(int **)rawPtr = &node->file->valI;
				*rawLen = sizeof(int);
				break;
			case zsymF:
				*(float **)rawPtr = &node->file->valF;
				*rawLen = sizeof(float);
				break;
			case zsymD:
				*(double **)rawPtr = &node->file->valD;
				*rawLen = sizeof(double);
				break;
			case zsymS:
				*rawPtr = node->file->valS + 2*sizeof(int);
				*rawLen = ((int *)node->file->valS)[1];
				break;
			case zsymIP:
				*(int ***)rawPtr = &node->file->valIP;
				*rawLen = sizeof(int);
				break;
			case zsymR:
				*rawPtr = node->file->valS + 2*sizeof(int);
				*rawLen = ((int *)node->file->valS)[1];
				break;
			default:
				*err = -1;
				*rawPtr = 0;
				*rawLen = 0;
				break;
		}
	}

	virtual void putRaw( void *context, int fileType, char *rawPtr, int rawLen, int *err ) {
		*err = 0;
		ZSymContextMem *node = (ZSymContextMem *)context;
		assert( node );
		assert( node->nodeType == zsymFILE || node->nodeType == zsymUNK );

		switch( fileType ) {
			// PROCESS setting of a pointer type
			case zsymIPs:
				assert( node->nodeType == zsymFILE && node->file->fileType == zsymIP );
				assert( rawLen == sizeof(int*) );
				*node->file->valIP = *(int *)rawPtr;
				return;
		}

		node->clear();
		node->nodeType = zsymFILE;
		node->file = new ZSymContextMem::ZSymMemFile;
		node->file->fileType = fileType;
		switch( fileType ) {
			case zsymI:
				assert( rawLen == sizeof(int) );
				node->file->valI = *(int *)rawPtr;
				break;
			case zsymF:
				assert( rawLen == sizeof(float) );
				node->file->valF = *(float *)rawPtr;
				break;
			case zsymD:
				assert( rawLen == sizeof(float) );
				node->file->valD = *(double *)rawPtr;
				break;
			case zsymR:
			case zsymS:
				node->file->valS = (char *)malloc( rawLen + 2*sizeof(int) + 1 );
				((int *)node->file->valS)[0] = rawLen + 2*sizeof(int) + 1;
				((int *)node->file->valS)[1] = rawLen;
				memcpy( node->file->valS+2*sizeof(int), rawPtr, rawLen );
				*(node->file->valS+2*sizeof(int)+rawLen) = 0;
				break;
			case zsymIP:
				assert( rawLen == sizeof(int*) );
				node->file->valIP = *(int **)rawPtr;
				break;
			default:
				break;
		}
	}

	virtual int cat( void *context, char *s, int len, int growBy, int *err ) {
		ZSymContextMem *node = (ZSymContextMem *)context;
		if( node && node->nodeType == zsymFILE ) {
			if( node->file->fileType != zsymS ) {
				node->clear();
				node->nodeType = zsymFILE;
				node->file = new ZSymContextMem::ZSymMemFile;
				node->file->fileType = zsymS;
			}
			if( len == -1 ) {
				len = strlen( s );
			}
			if( growBy == -1 ) {
				growBy = 64;
			}

			int allocSize = 0;
			int oldLen = 0;
			char *valS = node->file->valS;
			if( valS ) {
				// Already allocated, check if needs to grow
				allocSize = ((int *)valS)[0];
				oldLen = ((int *)valS)[1];
				int remains = allocSize - oldLen - 2*sizeof(int);
				if( remains < len+1 ) {
					// +1 for the nul that we always want at the end
					allocSize += len + growBy + 1;
					allocSize += (4-(allocSize&3))&3;
					valS = node->file->valS = (char *)realloc( valS, allocSize );
				}
			}
			else {
				allocSize = 2*sizeof(int) + len + growBy + 1;
				allocSize += (4-(allocSize&3))&3;
				valS = node->file->valS = (char *)malloc( allocSize );
				((int *)valS)[0] = 0;
				((int *)valS)[1] = 0;
			}

			((int *)valS)[0] = allocSize;
			((int *)valS)[1] += len;
			int tail = 2*sizeof(int)+oldLen;
			memcpy( valS+tail, s, len );
			memset( valS+tail+len, 0, allocSize-tail-len );

			*err = 0;
			return 1;
		}
		*err = 1;
		return 0;
	}

	struct ZSymProtocolMemListContext {
		ZSymHash *dir;
		int i;
	};

	virtual void *listStart( void *context ) {
		ZSymContextMem *node = (ZSymContextMem *)context;
		if( node ) {
			if( node->nodeType == zsymDIR ) {
				ZSymProtocolMemListContext *lc = new ZSymProtocolMemListContext;
				lc->dir = node->dir;
				lc->i = -1;
				return lc;
			}
		}
		return 0;
	}

	virtual int listNext( void *listContext, char **s, int *len, int *nodeType ) {
		ZSymProtocolMemListContext *lc = (ZSymProtocolMemListContext *)listContext;
		if( !lc ) {
			return 0;
		}
		int size = lc->dir->size;

		int keyLen = 0;
		char *key = 0;
		while( !key ) {
			lc->i++;
			if( lc->i >= size ) {
				delete lc;
				return 0;
			}
			key = lc->dir->getKey( lc->i, &keyLen );
		}
		*s = key;
		*len = keyLen;
		ZSymContextMem **node = (ZSymContextMem **)lc->dir->getVal(lc->i);
		assert( node );
		*nodeType = (*node)->nodeType;
		return 1;
	}

	virtual void getName( void *context, char **name, int *len, int global ) {
		ZSymContextMem *node = (ZSymContextMem *)context;

		char *_name = 0;
		int _nameLen = 0;

		while( node ) {
			if( !node->parent ) {
				// Reached the root, add a slash and be done
				_name = (char *)realloc( _name, _nameLen + 2 );
				memmove( &_name[1], _name, _nameLen+1 );
				_name[0] = '/';
				_nameLen++;
				break;
			}
			ZSymHash *parentDir = node->parent->dir;
			int size = parentDir->size;
			for( int i=0; i<size; i++ ) {
				ZSymContextMem **ni = (ZSymContextMem **)parentDir->getVal( i );
				if( ni && *ni == node ) {
					int keyLen = 0;
					char *key = parentDir->getKey( i, &keyLen );
					_name = (char *)realloc( _name, _nameLen + keyLen + 2 );
					if( node->nodeType == zsymDIR ) {
						memmove( &_name[keyLen+1], _name, _nameLen+1 );
						memcpy( _name, key, keyLen );
						_name[ keyLen ] = '/';
						_nameLen += keyLen + 1;
					}
					else {
						memmove( &_name[_nameLen], _name, _nameLen );
						memcpy( _name, key, keyLen );
						_name[keyLen] = 0;
						_nameLen += keyLen;
					}
					// NUL terminate for easy debugging if it is the first one processes (the last of the series)
					if( node == context ) {
						_name[ _nameLen ] = 0;
					}
					break;
				}
			}
			if( !global ) {
				break;
			}
			node = node->parent;
		}
		*name = _name;
		*len = _nameLen;
	}

	virtual void dump( void *context, int toStdout, int _indent ) {
		ZSymContextMem *node = (ZSymContextMem *)context;
		if( !node ) {
			printf( "<node nul>\n" );
			return;
		}

		char indent[128];
		memset( indent, ' ', _indent );
		indent[_indent] = 0;

		// FIND this symbol in the parent
		char *foundName = 0;
		int foundNameLen = 0;
		if( node->parent ) {
			assert( node->parent->nodeType == zsymDIR );
			int size = node->parent->dir->size;
			for( int i=0; i<size; i++ ) {
				ZSymContextMem **_node = (ZSymContextMem **)node->parent->dir->getVal(i);
				if( _node && *_node == node ) {
					foundName = node->parent->dir->getKey( i, &foundNameLen );
					break;
				}
			}
		}

		char *tmp = "<nnf>";
		if( foundName ) {
			tmp = (char *)alloca( foundNameLen + 1 );
			memcpy( tmp, foundName, foundNameLen );
			tmp[ foundNameLen ] = 0;
		}

		printf( "%s%s\n", indent, tmp );
		static char *nodeTypeStr[] = { "unk", "fil", "dir", "lnk" };
		static char *fileTypeStr[] = { "zsymZ","zsymI","zsymF","zsymD","zsymS","zsymR","zsymIP","zsymFP","zsymDP","zsymSP","zsymIPs","zsymFPs","zsymDPs","zsymSPs" };
		printf( "%snodePtr=%p parent=%p refCount=%d\n", indent, node, node->parent, node->refCount );
		printf( "%s%s", indent, nodeTypeStr[ node->nodeType ] );
		if( node->nodeType == zsymFILE ) {
			printf( " = (%s) ", fileTypeStr[ node->file->fileType ] );
			if( node->file->fileType == zsymI ) {
				printf( "%d", node->file->valI );
			}
			else if( node->file->fileType == zsymS ) {
				// @TODO: Use len and be smart since it might be binary
				char *bin = node->file->valS;
				int allocLen = ((int *)bin)[0];
				int len = ((int *)bin)[1];
				printf( "allocLen=%d len=%d\n", allocLen, len );
				bin += 8;
				if( (int)strlen(bin) == len ) {
					// Normal string, print it
					printf( "%s", bin );
				}
				else {
					// Binary string, show in hex
					printf( "binary length = %d\n%s", len, indent );
					for( int i=0; i<len; i+=16 ) {
						int j;
						int l = 16 < len-i ? 16 : len-i;
						printf( "  " );
						for( j=0; j<l; j++ ) {
							printf( "%02X ", (unsigned char)bin[i+j] );
						}
						for( j=l; j<16; j++ ) {
							printf( "   " );
						}
						printf( "  " );
						for( j=0; j<l; j++ ) {
							printf( "%c", ( bin[i+j] >= ' ' && bin[i+j] < 127 ) ? bin[i+j] : '.' );
						}
						if( i+16 < len ) {
							printf( "\n%s", indent );
						}
					}
				}
			}
			else if( node->file->fileType == zsymZ ) {
				printf( "<nil>" );
			}
			else {
				printf( "<unknown info>" );
			}
		}
		else if( node->nodeType == zsymDIR ) {
			printf( "%s with %d active records", indent, node->dir->numRecords );
		}
		else if( node->nodeType == zsymLINK ) {
			printf( "%s target: %s", indent, node->link );
		}
		printf( "\n" );
	}

	virtual int getListStats( void *context, int &minVal, int &maxVal ) {
		maxVal = INT_MIN;
		minVal = INT_MAX;
		ZSymContextMem *node = (ZSymContextMem *)context;
		if( node && node->nodeType == zsymDIR ) {
			int size = node->dir->size;
			for( int i=0; i<size; i++ ) {
				int keyLen = 0;
				char *k = node->dir->getKey( i, &keyLen );
				if( k ) {
					char *endPtr;
					int numKey = strtol( k, &endPtr, 0 );
					if( endPtr-k == keyLen ) {
						maxVal = max( maxVal, numKey );
						minVal = min( minVal, numKey );
					}
				}
			}
			return 1;
		}
		return 0;
	}
};


//==================================================================================
// ZSymContextFile
// @TODO: Each protocol probably ought to live in its own file
//==================================================================================

struct ZSymContextFile {
	static int allocCount;

	FILE *file;
	char *currentDir;
	char *lastRead;
	int refCount;
	int type;

	void incRef() {
		refCount++;
	}

	void decRef() {
		refCount--;
		assert( refCount >= 0 );
		if( refCount == 0 ) {
			delete this;
		}
	}

	void setDir( char *d ) {
		if( currentDir ) {
			free( currentDir );
		}
		currentDir = strdup( d );
	}

	ZSymContextFile() {
		refCount = 0;
			// refCount starts at zero unlike mem because it is not connected to a dir
		file = 0;
		currentDir = 0;
		lastRead = 0;
		type = zsymUNK;
		allocCount++;
	}

	~ZSymContextFile() {
		allocCount--;
		if( file ) {
			fclose( file );
		}
		if( currentDir ) {
			free( currentDir );
		}
		if( lastRead ) {
			free( lastRead );
		}
	}
};

int ZSymContextFile::allocCount = 0;

//==================================================================================
// ZSymProtocolFile
//==================================================================================

#include "sys/stat.h"
#include "direct.h"
#include "io.h"
	// For _chsize

struct ZSymProtocolFile : ZSymProtocol {
	virtual int resolve( int segCount, char **segs, int *segLens, void **context, int *err, int action ) {
		// @TODO: This needs to be reworked to use segLens and tested
		assert( 0 );

		struct stat s;
		int i;

		*err = 0;

		ZSymContextFile *_context = *(ZSymContextFile **)context;
		if( !_context ) {
			*context = new ZSymContextFile;
			_context = *(ZSymContextFile **)context;
		}

		char path[1024];
		path[0] = 0;
		int startSeg = 0;
		if( segs[0][0] == 0 ) {
			// This is an absolute path, don't prepend the currentDir
			strcpy( path, "/" );
			startSeg = 1;
		}
		else {
			// Prepend the currentDir onto the path
			if( _context->currentDir ) {
				strcpy( path, _context->currentDir );
			}
		}

		// EXAMINE the last segment. If it is blank then that means that they are asking for a dir
		_context->type = zsymFILE;
		int isDirLookup = 0;
		if( segs[segCount-1][0] == 0 ) {
			_context->type = zsymDIR;
			isDirLookup = 1;
		}

		// MKDIR up to the last non-file segment
		for( i=startSeg; i<segCount-1; i++ ) {
			// -1 on segCount because we do not wish to makdir on files or on the empty / at the end of a dir reference
			if( i != startSeg ) {
				strcat( path, "/" );
			}
			strcat( path, segs[i] );

			if( stat(path,&s) == 0 ) {
				// Path exists, ok
			}
			else if( action == zsymOPEN_OR_CREATE ) {
				if( mkdir( path ) != 0 ) {
					*err = zsymRESOLVE_CANNOT_OPEN;
					return i;
				}
			}
			else {
				*err = zsymRESOLVE_NOT_FOUND;
				return i;
			}
		}

		// OPEN the file if the last segment is a file
		if( !isDirLookup ) {
			_context->setDir( path );
				// In order not include the dir in the path, 
				// pre-set this before cating the file
			if( i != startSeg ) {
				strcat( path, "/" );
			}
			strcat( path, segs[i] );

			char *mode = "r+b";
			int isDir = 0;
			if( stat(path,&s)!=0 ) {
				// File doesn't already exist, so open in create mode
				mode = "wb";
			}
			else {
				#ifdef WIN32
					isDir = s.st_mode & _S_IFDIR;
				#else
					isDir = s.st_mode & S_IFDIR;
				#endif
			}

			if( isDir ) {
				// The symbol was actually found and is actually a dir, so don't open it
				strcat( path, "/" );
				_context->setDir( path );
				_context->type = zsymDIR;
			}
			else {
				_context->file = fopen( path, mode );
				if( !_context->file ) {
					*err = zsymRESOLVE_CANNOT_OPEN;
				}
			}
		}
		else {
			strcat( path, "/" );
			_context->setDir( path );
		}
		return segCount;
	}

	virtual int supportsFileType( int fileType ) {
		// File system only supports strings
		return fileType == zsymS ? 1 : 0;
	}

	virtual int bestConversion( void *context, int srcType ) {
		return zsymS;
			// Only str supported for files
	}

	virtual void dupContext( void *srcContext, void **dstContext ) {
		ZSymContextFile *src = (ZSymContextFile *)srcContext;
		ZSymContextFile *dst = new ZSymContextFile;
		dst->currentDir = strdup( src->currentDir );
		dst->file = 0;
		dst->lastRead = 0;
		dst->refCount = 1;
		*dstContext = dst;
	}

	virtual void getType( void *context, int *nodeType, int *fileType ) {
		*fileType = 0;
		*nodeType = 0;
		ZSymContextFile *f = (ZSymContextFile *)context;
		if( f ) {
			*fileType = zsymS;
			*nodeType = f->type;
		}
	}

	virtual void changeType( void *context, int nodeType ) {
		assert( 0 );
		// @TODO
	}

	virtual int rm( void *context, int *err, int safetyOverride, int childrenOnly ) {
		ZSymContextFile *f = (ZSymContextFile *)context;
		if( f ) {
			// @TODO: Recursive remove
			assert( 0 );
		}
		return 1;
	}

	virtual int createLink( void *context, char *target, int targetLen, int *err ) {
		// Links not supported in file system
		*err = 1;
		return 0;
	}

	virtual void incRef( void *context ) {
		ZSymContextFile *f = (ZSymContextFile *)context;
		if( f ) {
			f->incRef();
		}
	}

	virtual void decRef( void *context ) {
		ZSymContextFile *f = (ZSymContextFile *)context;
		if( f ) {
			f->decRef();
		}
	}

	virtual void getRaw( void *context, char **rawPtr, int *rawLen, int *err ) {
		*rawPtr = 0;
		*rawLen = 0;
		ZSymContextFile *f = (ZSymContextFile *)context;
		if( f ) {
			fseek( f->file, 0, SEEK_END );
			int _len = *rawLen = ftell( f->file );
			fseek( f->file, 0, SEEK_SET );
			f->lastRead = (char *)realloc( f->lastRead, _len+1 );
			int ret = fread( f->lastRead, _len, 1, f->file );
			if( ret == 1 ) {
				f->lastRead[_len] = 0;
				*rawPtr = f->lastRead;
				*err = 0;
				return;
			}
		}
		*rawPtr = 0;
		*err = -1;
	}

	virtual void putRaw( void *context, int fileType, char *rawPtr, int rawLen, int *err ) {
		ZSymContextFile *f = (ZSymContextFile *)context;
		if( f ) {
			int ret = fwrite( rawPtr, rawLen, 1, f->file );
			if( ret == 1 ) {
				fflush( f->file );
			
				// TRUNCATE
				_chsize( fileno(f->file), rawLen );
				*err = 0;
				return;
			}
		}
		*err = -1;
	}

	virtual int cat( void *context, char *s, int len, int growBy, int *err ) {
		// @TODO
		assert( 0 );
		return 0;
	}

	struct ZSymProtocolFileListContext {
		// @TODO: Switch this to using fnmatch and eliminate the dependency on zwildcard
		ZWildcard *w;
		ZSymContextFile *f;
	};

	virtual void *listStart( void *context ) {
		ZSymContextFile *f = (ZSymContextFile *)context;
		if( f ) {
			ZSymProtocolFileListContext *lc = new ZSymProtocolFileListContext;
			char buf[1024];
			strcpy( buf, f->currentDir );
			strcat( buf, "*" );
			lc->w = new ZWildcard( buf );
			lc->f = f;
			return lc;
		}
		return 0;
	}

	virtual int listNext( void *listContext, char **s, int *len, int *nodeType ) {
		ZSymProtocolFileListContext *lc = (ZSymProtocolFileListContext *)listContext;
		if( lc ) {
			int ret;
			do {
				ret = lc->w->next();
			} while( ret && lc->w->getName()[0] == '.' );
			if( ret ) {
				*s = lc->w->getName();
				*len = strlen( *s );
				struct stat buf;
				char path[1024];
				strcpy( path, lc->f->currentDir );
				strcat( path, *s );
				int s = stat( path, &buf );
				int isDir = 0;
				#ifdef WIN32
					isDir = buf.st_mode & _S_IFDIR;
				#else
					isDir = buf.st_mode & S_IFDIR;
				#endif
				*nodeType = isDir ? zsymDIR : zsymFILE;
				return 1;
			}
			else {
				delete lc->w;
				delete lc;
			}
		}
		return 0;
	}

	virtual void getName( void *context, char **name, int *len, int global ) {	
		assert( 0 );
		// @TODO
	}

	virtual void dump( void *context, int toStdout, int _indent ) {
		// @TODO
		ZSymContextFile *f = (ZSymContextFile *)context;
		if( f ) {
			char indent[128];
			memset( indent, ' ', _indent );
			indent[_indent] = 0;
			printf( "%scontext=%p fp=%p type=%d refCount=%d\n", indent, f, f->file, f->type, f->refCount );
			printf( "%scurrentDir=%s\n", indent, f->currentDir );
			// @TODO: Make this a binary dump if necessary (merge with mem code)
			printf( "%slastRead: %s\n", indent, f->lastRead );
		}
	}

	virtual int getListStats( void *context, int &minVal, int &maxVal ) {
		assert( 0 );
		// @TODO
		return 0;
	}
};

//==================================================================================
// ZSym
//==================================================================================

// @TODO These will get moved into separate files with a factory of some kind
ZSymProtocolMem zsymProtocolMem;
ZSymProtocolFile zsymProtocolFile;
char *ZSymProtocol::protocolNames[] = { "mem", "file" };
ZSymProtocol *ZSymProtocol::protocols[] = { &zsymProtocolMem, &zsymProtocolFile };

#define zsymAssembleTo(dest,fmt,maxlen) { va_list argptr; if (fmt != (dest)) { va_start (argptr, fmt); vsprintf((dest), fmt, argptr); va_end (argptr); } assert(strlen(dest)<maxlen); }

ZSym::ZSym() {
	protocol = 0;
	context = 0;
	err = 0;
	temp = 0;
	safetyOverride = 0;
}

ZSym::ZSym( char *path, int pathLen ) {
	protocol = 0;
	context = 0;
	err = 0;
	temp = 0;
	safetyOverride = 0;
	resolve( zsymOPEN_OR_CREATE | zsymFORCE_AS_SPECIFIED, path, pathLen );
}

ZSym::ZSym( int flags, char *path, int pathLen ) {
	protocol = 0;
	context = 0;
	err = 0;
	temp = 0;
	safetyOverride = 0;
	resolve( flags, path, pathLen );
}

ZSym::ZSym( ZSym &o ) {
	context = 0;
	protocol = o.protocol;
	err = o.err;
	temp = 0;
	safetyOverride = 0;
	if( protocol ) {
		protocol->dupContext( o.context, &context );
		protocol->incRef( context );
	}
}

ZSym &ZSym::operator=( ZSym &o ) {
	if( protocol ) {
		protocol->decRef( context );
	}
	protocol = o.protocol;
	err = o.err;
	safetyOverride = o.safetyOverride;
	if( protocol ) {
		protocol->dupContext( o.context, &context );
		protocol->incRef( context );
	}
	return *this;
}

ZSym::~ZSym() {
	if( temp ) {
		free( temp );
	}
	if( protocol ) {
//dump();
		protocol->decRef( context );
	}
}

void ZSym::incRef() {
	if( protocol && context ) {
		protocol->incRef( context );
	}
}

void ZSym::decRef() {
	if( protocol && context ) {
		protocol->decRef( context );
	}
}

void ZSym::clear() {
	if( protocol ) {
		protocol->decRef( context );
	}
	protocol = 0;
	context = 0;
	err = 0;
	if( temp ) {
		free( temp );
		temp = 0;
	}
}

int ZSym::resolve( char *path, int pathLen ) {
	return resolve( zsymOPEN_OR_CREATE | zsymFORCE_AS_SPECIFIED, path, pathLen );
}

int ZSym::resolve( int flags, char *path, int pathLen ) {
	// @TODO: THis is going to have to deal with escaped '/'

	if( *path == 0 && context && protocol ) {
		// This is a request for "this"
		return 1;
	}

	// LOOK for a protocol override
	ZSymProtocol *oldProtocol = protocol;
	void *oldContext = context;

	if( pathLen == -1 ) {
		pathLen = strlen( path );
	}

	int p = 0;
	char *c = path;
	char *afterProtocol = path;
	char *end = &path[ pathLen ];
	for( ; c<end; c++ ) {
		if( *c == ':' ) {
			for( p=0; p<zsymPROTOCOL_COUNT; p++ ) {
				if( !strncmp(ZSymProtocol::protocolNames[p],path,c-path) ) {
					break;
				}
			}
			assert( p < zsymPROTOCOL_COUNT );
				// Unidentified protocol
			assert( c[1] == '/' && c[2] == '/' );
			afterProtocol = c+3;
			protocol = ZSymProtocol::protocols[p];
			break;
		}
	}

	// ASSUME memory protocol if none specified
	if( !protocol ) {
		protocol = ZSymProtocol::protocols[0];
	}

	// BREAK the path into segments
	const int zsymSEG_COUNT_MAX = 32;
	int segCount = 0;
	char *segs[zsymSEG_COUNT_MAX];
	int segLens[zsymSEG_COUNT_MAX];

	char *last = afterProtocol;
	for( c = afterProtocol; ; c++ ) {
		if( c >= end || c[0] == 0 || c[0] == '/' ) {
			// Found a slash, a root slash will create a blank segment
			int len = c-last;
			segs[segCount] = last;
			segLens[segCount] = len;
			segCount++;
			assert( segCount < zsymSEG_COUNT_MAX );
			last = c+1;
				// +1 skips the slash
		}
		if( c >= end || c[0] == 0 ) {
			break;
		}
	}

	// CALL the protocol's resolver
	int err=0, ret=0;
	int segsResolved = protocol->resolve( segCount, segs, segLens, &context, &err, flags );
	if( err == 0 ) {
		// Everything resolved perfectly
		ret = 1;
	}
	else if( err == -1 ) {
		// Resolved as a link, expand and recurse
		// @TODO: allow links to be non nul terminated
		char *target = (char *)context;
		int targetLen = strlen( target );
		int remainLen = pathLen - (segs[segsResolved-1] - segs[0]) - segLens[segsResolved-1];
		int expandedLen = targetLen + remainLen;
		char *expanded = (char *)alloca( expandedLen );
		memcpy( expanded, target, targetLen );
		if( remainLen > 0 ) {
			memcpy( &expanded[targetLen], segs[segsResolved], remainLen );
		}
		context = 0;
		ret = resolve( flags, expanded, expandedLen );
	}
	else {
		context = 0;
	}

	// INCREMENT the new context and decrement the old
	if( protocol && context ) {
		protocol->incRef( context );
	}
	if( oldProtocol && oldContext ) {
		oldProtocol->decRef( oldContext );
	}

	return ret;
}

void ZSym::import( ZSym o, char *path, int pathLen ) {
	int dstNodeType=0, dstFileType=0;
	getType( dstNodeType, dstFileType );

	int srcNodeType=0, srcFileType=0;
	o.getType( srcNodeType, srcFileType );

	if( dstNodeType == zsymDIR ) {
		if( srcNodeType == zsymFILE ) {
			ZSym temp = *this;
			if( !path ) {
				// CREATE a temp name
				int collision = 0;
				int cnt = 0;
				char buf[32];
				do {
					sprintf( buf, "__temp%d", cnt++ );
					ZSym checkExist = *this;
					collision = checkExist.resolve( zsymOPEN_OR_FAIL, buf );
				} while( collision );
				path = buf;
				pathLen = strlen( path );
			}
			temp.resolve( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE, path, pathLen );
			temp.put( o );
		}
		else {
			for( ZSymDirList l(o); l.next(); ) {
				ZSym temp = *this;
				path = l.path( &pathLen );
				temp.resolve( path, pathLen );
				temp.put( l.zsym() );
			}
		}
	}
}

int ZSym::rm( char *path, int pathLen ) {
	if( !path ) path = "";
	ZSym temp = *this;
	if( temp.resolve( zsymOPEN_OR_FAIL, path, pathLen ) ) {
		temp.protocol->rm( temp.context, &err, safetyOverride, 0 );
		safetyOverride = 0;
		if( err == 0 ) {
			return 1;
		}
	}
	return 0;
}

// Thinking about lists:
// For now, you can write to any value you want
// When you do a foreach it will go through them
// sequentially
// When you push or pop it will have to find the
// smallest and largest values, this would be
// optimized by have the protocol store the
// largest and smallest at each put
// Pushing onto head will involve massive renames

/*
void ZSym::pushHead( ZSym &o ) {
	// If the directory is empty, then then child name will be 0
	// Otherwise we have to scan for the largest number and inc
	// Eventually this could be optimized by all mem contexts
	// could track if a non-push has occured and this would mean
	// "this might be out of date, so re-scan for the largest / smallest"
	// How is this going to work exactly?  It could be that different protocols
	// simply don't support head pushing for example
	// A protocol which supports a list will have to understand that reference for 0
	// means to use the list directory instead of a hash
	// How will something know that it is to make a list? Certainyl push and pop
	// But what if you want to want to just write a loop like a[0] = 1, etc
}

ZSym ZSym::popHead() {
}
*/

void ZSym::pushTail( ZSym &o ) {
	// ASK the protocol for the maximum numerical value
	// it will either have to search for this or it
	// can use some optimization to track it
	if( protocol && context ) {
		int minVal=INT_MAX, maxVal=INT_MIN;
		protocol->getListStats( context, minVal, maxVal );
		int val = 0;
		if( maxVal > INT_MIN ) {
			val = maxVal + 1;
		}
		char key[64];
		sprintf( key, "%d", val );
		resolve( zsymOPEN_OR_CREATE, key, -1 );
		put( o );
	}
	else {
assert( 0 );
// @TODO: What should this do, where does it create the list?
		resolve( zsymOPEN_OR_CREATE, "0", 1 );
		put( o );
	}
}

//ZSym ZSym::popTail() {
//}

/*
void ZSym::listStats( int &count, int &minimim, int &maximum ) {
}
*/

int ZSym::link( char *linkName, char *target, int linkNameLen, int targetLen ) {
	if( resolve( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE|zsymDONT_FOLLOW_LINKS, linkName, linkNameLen ) ) {
		return protocol->createLink( context, target, targetLen, &err );
	}
	return 0;
}

int ZSym::isValid() {
	if( protocol && context ) {
		int nodeType, fileType;
		protocol->getType( context, &nodeType, &fileType );
		return nodeType != zsymUNK;
	}
	return 0;
}

int ZSym::isDir() {
	if( protocol ) {
		int nodeType, fileType;
		protocol->getType( context, &nodeType, &fileType );
		return nodeType == zsymDIR ? 1 : 0;
	}
	return 0;
}

int ZSym::isFile() {
	if( protocol ) {
		int nodeType, fileType;
		protocol->getType( context, &nodeType, &fileType );
		return nodeType == zsymFILE ? 1 : 0;
	}
	return 0;
}

int ZSym::isLink() {
	if( protocol ) {
		int nodeType, fileType;
		protocol->getType( context, &nodeType, &fileType );
		return nodeType == zsymLINK ? 1 : 0;
	}
	return 0;
}

int ZSym::nodeType() {
	if( protocol ) {
		int _nodeType, _fileType;
		protocol->getType( context, &_nodeType, &_fileType );
		return _nodeType;
	}
	return 0;
}

int ZSym::fileType() {
	if( protocol ) {
		int _nodeType, _fileType;
		protocol->getType( context, &_nodeType, &_fileType );
		return _fileType;
	}
	return 0;
}

void ZSym::getType( int &nodeType, int &fileType ) {
	nodeType = 0;
	fileType = 0;
	if( protocol ) {
		protocol->getType( context, &nodeType, &fileType );
	}
}

void ZSym::convertNodeType( int dstNodeType ) {
	if( protocol && context ) {
		int srcNodeType=0, srcFileType=0;
		getType( srcNodeType, srcFileType );
		if( srcNodeType != dstNodeType ) {
			protocol->changeType( context, dstNodeType );
		}
	}
}

void ZSym::convertFileType( int dstFileType ) {
	if( protocol && context ) {
		int srcNodeType=0, srcFileType=0;
		getType( srcNodeType, srcFileType );
		if( srcNodeType == zsymFILE && srcFileType != dstFileType ) {
			char *src;
			int srcLen;
			protocol->getRaw( context, &src, &srcLen, &err );

			char *_temp = 0;
			int _tempLen = 0;
			convert( srcFileType, src, srcLen, dstFileType, &_temp, &_tempLen );

			protocol->putRaw( context, dstFileType, _temp, _tempLen, &err );
			if( _temp ) {
				free( _temp );
			}
		}
	}
}

char *ZSym::nameLocal( int *len ) {
	// Fetch the name of this symbol in the parent's space (mallocs, you must free)
	if( protocol ) {
		char *name;
		int nameLen;
		protocol->getName( context, &name, &nameLen, 0 );
		if( len ) {
			*len = nameLen;
		}
		return name;
	}
	if( len ) {
		*len = 0;
	}
	return 0;
}
	
char *ZSym::nameGlobal( int *len ) {
	// Fetch the name of this symbol in the global fully qualified space (mallocs, you must free)
	if( protocol ) {
		char *name;
		int nameLen;
		protocol->getName( context, &name, &nameLen, 1 );
		if( len ) {
			*len = nameLen;
		}
		return name;
	}
	if( len ) {
		*len = 0;
	}
	return 0;
}

void ZSym::convert( int srcType, char *src, int srcLen, int dstType, char **dst, int *dstLen ) {
	char *endPtr;
	int retI;
	float retF;
	double retD;

	switch( srcType ) {
		case zsymZ:
			switch( dstType ) {
				case zsymZ:
					assert( 0 );
					break;

				case zsymIPs:
				case zsymI:
					*dst = (char *)malloc( sizeof(int) );
					memset( *dst, 0, sizeof(int) );
					*dstLen = sizeof(int);
					break;

				case zsymF:
					*dst = (char *)malloc( sizeof(float) );
					*((float *)*dst) = 0.f;
					*dstLen = sizeof(float);
					break;

				case zsymD:
					*dst = (char *)malloc( sizeof(double) );
					*((double *)*dst) = 0.0;
					*dstLen = sizeof(double);
					break;

				case zsymS:
					*dst = (char *)malloc( 1 );
					(*dst)[0] = 0;
					*dstLen = 0;
					break;
			}
			break;

		case zsymI:
			switch( dstType ) {
				case zsymZ:
					assert( 0 );
					break;

				case zsymIPs:
				case zsymI:
					*dst = (char *)malloc( sizeof(int) );
					memcpy( *dst, src, sizeof(int) );
					*dstLen = sizeof(int);
					break;

				case zsymF:
					*dst = (char *)malloc( sizeof(float) );
					*((float *)*dst) = (float)*(int *)src;
					*dstLen = sizeof(float);
					break;

				case zsymD:
					*dst = (char *)malloc( sizeof(double) );
					*((double *)*dst) = (double)*(int *)src;
					*dstLen = sizeof(double);
					break;

				case zsymS:
					char temp[64];
					sprintf( temp, "%d", *(int *)src );
					*dst = strdup( temp );
					*dstLen = strlen( *dst );
					break;

			}
			break;

		case zsymF:
			switch( dstType ) {
				case zsymZ:
					assert( 0 );
					break;

				case zsymIPs:
				case zsymI:
					*dst = (char *)malloc( sizeof(int) );
					*((int *)*dst) = (int)*(float *)src;
					*dstLen = sizeof(int);
					break;

				case zsymF:
					*dst = (char *)malloc( sizeof(float) );
					memcpy( *dst, src, sizeof(float) );
					*dstLen = sizeof(float);
					break;

				case zsymD:
					*dst = (char *)malloc( sizeof(int) );
					*((int *)*dst) = (int)*(double *)src;
					*dstLen = sizeof(int);
					break;

				case zsymS:
					char temp[128];
					sprintf( temp, "%f", *(float *)src );
					*dst = strdup( temp );
					*dstLen = strlen( *dst );
					break;
			}
			break;

		case zsymD:
			switch( dstType ) {
				case zsymZ:
					assert( 0 );
					break;

				case zsymIPs:
				case zsymI:
					*dst = (char *)malloc( sizeof(int) );
					*((int *)*dst) = (int)*(double *)src;
					*dstLen = sizeof(int);
					break;

				case zsymF:
					*dst = (char *)malloc( sizeof(float) );
					*((float *)*dst) = (float)*(double *)src;
					*dstLen = sizeof(float);
					break;

				case zsymD:
					*dst = (char *)malloc( sizeof(double) );
					memcpy( *dst, src, sizeof(double) );
					*dstLen = sizeof(double);
					break;

				case zsymS:
					char temp[128];
					sprintf( temp, "%lf", *(double *)src );
					*dst = strdup( temp );
					*dstLen = strlen( *dst );
					break;
			}
			break;

		case zsymS:
			switch( dstType ) {
				case zsymZ:
					break;

				case zsymIPs:
				case zsymI:
					*dst = (char *)malloc( sizeof(int) );
					*dstLen = sizeof(int);
					retI = strtol( src, &endPtr, 0 );
					if( endPtr-src >= srcLen ) {
						// It successfully converted the str
						*((int *)*dst) = retI;
					}
					else {
						*((int *)*dst) = 0;
					}
					break;

				case zsymF:
					*dst = (char *)malloc( sizeof(float) );
					*dstLen = sizeof(float);
					retF = (float)strtod( src, &endPtr );
					if( endPtr-src >= srcLen ) {
						// It successfully converted the str
						*((float *)*dst) = retF;
					}
					else {
						*((float *)*dst) = 0.f;
					}
					break;

				case zsymD:
					*dst = (char *)malloc( sizeof(double) );
					*dstLen = sizeof(double);
					retD = strtod( src, &endPtr );
					if( endPtr-src >= srcLen ) {
						// It successfully converted the str
						*((double *)*dst) = retD;
					}
					else {
						*((double *)*dst) = 0.0;
					}
					break;

				case zsymS:
					*dst = (char *)malloc( srcLen );
					memcpy( *dst, src, srcLen );
					*dstLen = srcLen;
					break;
			}
			break;

		case zsymIP:
			switch( dstType ) {
				case zsymZ:
					assert( 0 );
					break;

				case zsymIPs:
				case zsymI:
					*dst = (char *)malloc( sizeof(int) );
					*((int *)*dst) = (int)**(int **)src;
					*dstLen = sizeof(int);
					break;

				case zsymF:
					*dst = (char *)malloc( sizeof(float) );
					*((float *)*dst) = (float)**(int **)src;
					*dstLen = sizeof(float);
					break;

				case zsymD:
					*dst = (char *)malloc( sizeof(double) );
					*((double *)*dst) = (double)**(int **)src;
					*dstLen = sizeof(double);
					break;

				case zsymS:
					char temp[64];
					sprintf( temp, "%d", **(int **)src );
					*dst = strdup( temp );
					*dstLen = strlen( *dst );
					break;

			}
			break;

		case zsymR:
			switch( dstType ) {
				case zsymZ:
				case zsymIPs:
				case zsymI:
					*dst = (char *)malloc( sizeof(int) );
					memset( *dst, 0, sizeof(int) );
					*dstLen = sizeof(int);
					break;

				case zsymF:
					*dst = (char *)malloc( sizeof(float) );
					*((float *)*dst) = 0.f;
					*dstLen = sizeof(float);
					break;

				case zsymD:
					*dst = (char *)malloc( sizeof(double) );
					*((double *)*dst) = 0.0;
					*dstLen = sizeof(double);
					break;

				case zsymS:
					*dst = (char *)malloc( 1 );
					(*dst)[0] = 0;
					*dstLen = 1;
					break;

				case zsymR:
					assert( 0 );
					break;

			}
			break;

	}
}

// Copy put
//----------------------------------------------------------

int ZSym::put( ZSym &o ) {
	if( protocol && context ) {
		ZSym temp;

		// FETCH the types of src and dst
		int dstNodeType=0, dstFileType=0;
		getType( dstNodeType, dstFileType );

		int srcNodeType=0, srcFileType=0;
		o.getType( srcNodeType, srcFileType );

		// REMOVE the contents of the destination if it is a directory
		if( dstNodeType == zsymDIR ) {
			protocol->rm( context, &err, safetyOverride, 1 );
			safetyOverride = 0;
		}

		// CONVERT the dst to the src node type
		if( dstNodeType != srcNodeType ) {
			dstNodeType = srcNodeType;
			protocol->changeType( context, srcNodeType );
		}

		int count = 0;
		for( ZSymDirListRecurse l(o); l.next(); count++ ) {
			l.zsym().protocol->getType( l.zsym().context, &srcNodeType, &srcFileType );

			int pathLen = 0;
			char *path = l.path( &pathLen );

			if( srcNodeType == zsymDIR ) {
				temp = *this;
				temp.resolve( zsymOPEN_OR_CREATE|zsymFORCE_TO_DIR, path, pathLen );
			}
			else {
				temp = *this;
				if( dstNodeType == zsymDIR ) {
					// IF the destination is not a dir it has to be converted
					temp.resolve( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE, path, pathLen );
				}

				char *src;
				int srcLen;
				l.zsym().protocol->getRaw( l.zsym().context, &src, &srcLen, &err );

				char *_temp = 0;
				int _tempLen = 0;

				int dstFileType = protocol->bestConversion( context, srcFileType );
				if( dstFileType != srcFileType ) {
					convert( srcFileType, src, srcLen, dstFileType, &_temp, &_tempLen );
					src = _temp;
					srcLen = _tempLen;
				}

				temp.protocol->putRaw( temp.context, dstFileType, src, srcLen, &err );
				if( _temp ) {
					free( _temp );
				}
			}
		}
	}
	return 1;
}

// Put
//----------------------------------------------------------

int ZSym::putI( char *path, int i, int pathLen ) {
	if( !context ) {
		resolve( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE, path, pathLen );
		return putI( i );
	}
	else {
		ZSym temp = *this;
		temp.resolve( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE, path, pathLen );
		return temp.putI( i );
	}
}

int ZSym::putI( int i ) {
	if( protocol && context ) {
		int nodeType=0, fileType = 0;
		protocol->getType( context, &nodeType, &fileType );

		if( nodeType == zsymDIR ) {
			// If you wanted to write to this as a file, you should have 
			// resolved the symbol to force it a file
			return 0;
		}

		char *_temp = 0;
		int _tempLen = 0;
		int dstType = protocol->bestConversion( context, zsymI );
		char *src = (char *)&i;
		int srcLen = sizeof(i);
		if( dstType != zsymI ) {
			convert( zsymI, src, srcLen, dstType, &_temp, &_tempLen );
			src = _temp;
			srcLen = _tempLen;
		}
		protocol->putRaw( context, dstType, src, srcLen, &err );
		if( _temp ) {
			free( _temp );
		}
		return 1;
	}
	return 0;
}

int ZSym::putS( char *path, char *val, int pathLen, int valLen ) {
	if( !context ) {
		resolve( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE, path, pathLen );
		return putS( val, valLen );
	}
	else {
		ZSym temp = *this;
		temp.resolve( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE, path, pathLen );
		return temp.putS( val, valLen );
	}
}

int ZSym::putS( char *val, int valLen ) {
	if( protocol && context ) {
		int nodeType=0, fileType = 0;
		protocol->getType( context, &nodeType, &fileType );

		if( nodeType == zsymDIR ) {
			protocol->rm( context, &err, safetyOverride, 0 );
			safetyOverride = 0;
		}

		if( valLen == -1 ) {
			valLen = strlen( val );
		}
		char *_temp = 0;
		int _tempLen = 0;
		int dstType = protocol->bestConversion( context, zsymS );
		if( dstType != zsymS ) {
			convert( zsymS, val, valLen, dstType, &_temp, &_tempLen );
			val = _temp;
			valLen = _tempLen;
		}
		protocol->putRaw( context, dstType, val, valLen, &err );
		if( _temp ) {
			free( _temp );
		}
		return 1;
	}
	return 0;
}

int ZSym::putR( char *path, void *funcPtr, char *argStr, int pathLen ) {
	if( !context ) {
		resolve( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE, path, pathLen );
		return putR( funcPtr, argStr );
	}
	else {
		ZSym temp = *this;
		temp.resolve( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE, path, pathLen );
		return temp.putR( funcPtr, argStr );
	}
}

int ZSym::putR( void *funcPtr, char *argStr ) {
	if( protocol && context ) {
		int nodeType=0, fileType = 0;
		protocol->getType( context, &nodeType, &fileType );

		if( nodeType == zsymDIR ) {
			protocol->rm( context, &err, safetyOverride, 0 );
			safetyOverride = 0;
		}

		int len = strlen( argStr );
		ZSymRoutine *r = (ZSymRoutine *)alloca( sizeof(r) + len + 1 );
		r->funcPtr = (ZSymRoutine::ZSymRoutineFuncPtr)funcPtr;
		memcpy( r->argStr, argStr, len+1 );

		int dstType = protocol->bestConversion( context, zsymR );
		assert( dstType == zsymR );
			// For now, there are no conversion of a routine

		protocol->putRaw( context, dstType, (char *)r, sizeof(void*)+len+1, &err );
		return 1;
	}
	return 0;
}


int ZSym::ptrI( char *path, int *i, int pathLen ) {
	if( !context ) {
		resolve( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE, path, pathLen );
		return ptrI( i );
	}
	else {
		ZSym temp = *this;
		temp.resolve( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE, path, pathLen );
		return temp.ptrI( i );
	}
}

int ZSym::ptrI( int *i ) {
	if( protocol && context ) {
		int nodeType=0, fileType = 0;
		protocol->getType( context, &nodeType, &fileType );

		if( nodeType == zsymDIR ) {
			// If you wanted to write to this as a file, you should have 
			// resolved the symbol to force it a file
			return 0;
		}

		char *_temp = 0;
		int _tempLen = 0;
		int dstType = protocol->bestConversion( context, zsymIP );
		char *src = (char *)&i;
		int srcLen = sizeof(i);
		if( dstType != zsymIP ) {
			convert( zsymIP, src, srcLen, dstType, &_temp, &_tempLen );
			src = _temp;
			srcLen = _tempLen;
		}
		protocol->putRaw( context, dstType, src, srcLen, &err );
		if( _temp ) {
			free( _temp );
		}
		return 1;
	}
	return 0;
}



// Cat
//----------------------------------------------------------

int ZSym::catC( char *path, char c, int pathLen, int growBy ) {
	if( !context ) {
		resolve( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE, path, pathLen );
		return catC( c, growBy );
	}
	else {
		ZSym temp = *this;
		temp.resolve( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE, path, pathLen );
		return temp.catC( c, growBy );
	}
}

int ZSym::catC( char c, int growBy ) {
	if( isValid() ) {
		int nodeType, fileType;
		protocol->getType( context, &nodeType, &fileType );
		if( nodeType == zsymFILE && (fileType == zsymS || fileType == zsymZ) ) {
			return protocol->cat( context, &c, 1, growBy, &err );
		}
		else {
			err = 1;
			return 0;
		}
	}
	err = 2;
	return 0;
}

int ZSym::catI( char *path, int i, int pathLen, int growBy ) {
	if( !context ) {
		resolve( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE, path, pathLen );
		return catI( i, growBy );
	}
	else {
		ZSym temp = *this;
		temp.resolve( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE, path, pathLen );
		return temp.catI( i, growBy );
	}
}

int ZSym::catI( int i, int growBy ) {
	if( isValid() ) {
		int nodeType, fileType;
		protocol->getType( context, &nodeType, &fileType );
		if( nodeType == zsymFILE && (fileType == zsymS || fileType == zsymZ) ) {
			return protocol->cat( context, (char *)&i, sizeof(int), growBy, &err );
		}
		else {
			err = 1;
			return 0;
		}
	}
	err = 2;
	return 0;
}

int ZSym::catS( char *path, char *val, int pathLen, int valLen, int growBy ) {
	if( !context ) {
		resolve( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE, path, pathLen );
		return catS( val, valLen, growBy );
	}
	else {
		ZSym temp = *this;
		temp.resolve( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE, path, pathLen );
		return temp.catS( val, valLen, growBy );
	}
}

int ZSym::catS( char *val, int valLen, int growBy ) {
	if( isValid() ) {
		int nodeType, fileType;
		protocol->getType( context, &nodeType, &fileType );
		if( nodeType == zsymFILE && (fileType == zsymS || fileType == zsymZ) ) {
			return protocol->cat( context, val, valLen, growBy, &err );
		}
		else {
			err = 1;
			return 0;
		}
	}
	err = 2;
	return 0;
}

// Get
//----------------------------------------------------------

int ZSym::has( char *path, int pathLen ) {
	ZSym temp = *this;
	temp.resolve( zsymOPEN_OR_FAIL, path, pathLen );
	return temp.isValid();
}

int ZSym::getI( char *path, int pathLen, int onFail ) {
	ZSym temp( *this );
	if( temp.resolve( zsymOPEN_OR_FAIL, path, pathLen ) ) {
		int ret = temp.getI();
		if( err != -1 ) {
			return ret;
		}
	}
	
	// Not found or not a file, etc.
	return onFail;
}

int ZSym::getI() {
	err = 0;
	if( protocol && context ) {
		int nodeType=0, fileType=0;
		protocol->getType( context, &nodeType, &fileType );
		if( nodeType == zsymFILE || nodeType == zsymUNK ) {
			if( temp ) {
				free( temp );
				temp = 0;
			}
			int tempLen;
			char *val;
			int valLen;
			protocol->getRaw( context, &val, &valLen, &err );
			if( fileType != zsymI ) {
				convert( fileType, val, valLen, zsymI, &temp, &tempLen );
				val = temp;
				assert( tempLen == sizeof(int) );
			}
			return *(int *)val;
		}
	}
	err = -1;
	return 0;
}

char *ZSym::getS( char *path, int pathLen, char *onFail, int *len ) {
	ZSym temp( *this );
	if( temp.resolve( zsymOPEN_OR_FAIL, path, pathLen ) ) {
		char *ret = temp.getS( len );
		if( err != -1 ) {
			return ret;
		}
	}
	
	// Not found or not a file, etc.
	if( len ) {
		*len = strlen( onFail );
	}
	return onFail;
}

char *ZSym::getS( int *valLen ) {
	err = 0;
	if( protocol && context ) {
		int nodeType=0, fileType=0;
		protocol->getType( context, &nodeType, &fileType );
		if( nodeType == zsymFILE || nodeType == zsymUNK ) {
			if( temp ) {
				free( temp );
				temp = 0;
			}
			int tempLen;
			char *val;
			int _valLen;
			protocol->getRaw( context, &val, &_valLen, &err );
			if( fileType != zsymS ) {
				convert( fileType, val, _valLen, zsymS, &temp, &tempLen );
				val = temp;
				_valLen = tempLen;
			}
			if( valLen ) {
				*valLen = _valLen;
			}
			return val;
		}
	}
	if( valLen ) {
		*valLen = 0;
	}
	err = -1;
	return "";
}

void ZSym::exeR( char *path, ZSym &args, ZSym &ret, int pathLen ) {
	ZSym temp( *this );
	if( temp.resolve( zsymOPEN_OR_FAIL, path, pathLen ) ) {
		temp.exeR( args, ret );
	}
}

void ZSym::exeR( ZSym &args, ZSym &ret ) {
	err = 0;
	if( protocol && context ) {
		int nodeType=0, fileType=0;
		protocol->getType( context, &nodeType, &fileType );
		if( nodeType == zsymFILE && fileType == zsymR ) {
			char *val;
			int _valLen;
			protocol->getRaw( context, &val, &_valLen, &err );
			((ZSymRoutine *)val)->call( args, ret );
		}
	}
	err = -1;
}

int ZSym::len( char *path, int pathLen ) {
	ZSym temp( *this );
	if( temp.resolve( zsymOPEN_OR_FAIL, path, pathLen ) ) {
		return temp.len();
	}
	return 0;
}

int ZSym::len() {
	if( protocol && context ) {
		int nodeType=0, fileType=0;
		protocol->getType( context, &nodeType, &fileType );
		if( nodeType == zsymFILE || nodeType == zsymUNK ) {
			char *src;
			int srcLen;
			protocol->getRaw( context, &src, &srcLen, &err );
			return srcLen;
		}
	}
	return 0;
}


// Dump
//----------------------------------------------------------

void ZSym::dump( int toStdout, int indent ) {
	// @TODO: Make this go to either stdout or outputdebug
	if( protocol ) {
		protocol->dump( context, toStdout, indent );
	}
}

//==================================================================================
// ZSymLocal
//==================================================================================

int ZSymLocal::serial = 0;

ZSymLocal::ZSymLocal( char *dir, int flags ) {
	if( !dir ) {
		dir = "/";
	}

	serial++;
	localSerial = serial;
	char buf[32];
	sprintf( buf, "%s__temp%d", dir, localSerial );
	resolve( zsymOPEN_OR_CREATE|flags, buf );
}

ZSymLocal::~ZSymLocal() {
	char buf[32];
	sprintf( buf, "/__temp%d", localSerial );
	rm( buf );
}

//==================================================================================
// ZSymDirList
//==================================================================================

ZSymDirList::ZSymDirList( char *path, int pathLen ) {
	_nodeType = 0;
	listingContext = 0;
	dirSym.resolve( zsymOPEN_OR_FAIL, path, pathLen );
	_path = 0;
	_pathLen = 0;
	if( pathLen < 0 ) {
		pathLen = strlen( path );
	}
	if( dirSym.isFile() ) {
		_path = (char *)malloc( pathLen );
		_pathLen = pathLen;
		memcpy( _path, path, pathLen );
	}
	if( dirSym.protocol ) {
		listingContext = dirSym.protocol->listStart( dirSym.context );
	}
}

ZSymDirList::ZSymDirList( ZSym &sym ) {
	_path = 0;
	_pathLen = 0;
	_nodeType = 0;
	dirSym = sym;
	listingContext = 0;
	if( dirSym.protocol ) {
		listingContext = dirSym.protocol->listStart( dirSym.context );
	}
}

int ZSymDirList::next() {
	if( dirSym.protocol ) {
		if( dirSym.isFile() ) {
			if( dirSym.err == 0 ) {
				dirSym.protocol->getName( dirSym.context, &_path, &_pathLen, 0 );
				currentSym = dirSym;
				dirSym.err = -1;
				return 1;
			}
			return 0;
		}

		int ret = dirSym.protocol->listNext( listingContext, &_path, &_pathLen, &_nodeType );
		if( ret ) {
			currentSym = dirSym;
			currentSym.resolve( zsymOPEN_OR_FAIL|zsymDONT_FOLLOW_LINKS, _path, _pathLen );
			return 1;
		}
	}
	return 0;
}

//==================================================================================
// ZSymDirListRecurse
//==================================================================================

ZSymDirListRecurse::ZSymDirListRecurse( int _flags, char *path, int pathLen ) {
	stackTop = 0;
	stack[stackTop] = new ZSymDirList( path, pathLen );
	flags = _flags;
	tempName = 0;
}

ZSymDirListRecurse::ZSymDirListRecurse( char *path, int pathLen ) {
	stackTop = 0;
	stack[stackTop] = new ZSymDirList( path, pathLen );
	flags = 0;
	tempName = 0;
}

ZSymDirListRecurse::ZSymDirListRecurse( ZSym &sym ) {
	stackTop = 0;
	stack[stackTop] = new ZSymDirList( sym );
	flags = 0;
	tempName = 0;
}

ZSymDirListRecurse::~ZSymDirListRecurse() {
	for( int i=0; i<=stackTop; i++ ) {
		delete stack[i];
	}
	if( tempName ) {
		free( tempName );
		tempName = 0;
	}
}

int ZSymDirListRecurse::next() {
	if( !(flags==zsymIGNORE_LINKS && nodeType()==zsymLINK) && stack[stackTop]->zsym().isDir() ) {
		stackTop++;
		assert( stackTop < zsymLIST_STACK_SIZE );
		stack[stackTop] = new ZSymDirList( stack[stackTop-1]->zsym() );
	}
	int ret = stack[stackTop]->next();
	while( !ret ) {
		delete stack[stackTop];
		stack[stackTop] = 0;
		--stackTop;
		if( stackTop < 0 ) {
			return 0;
		}
		ret = stack[stackTop]->next();
	}
	return 1;
}

char *ZSymDirListRecurse::path( int *pathLen ) {
	// @TODO: THis is going to have to deal with escaped '/'
	if( tempName ) {
		free( tempName );
		tempName = 0;
	}

	int tempLen = 0;
	for( int i=0; i<=stackTop; i++ ) {
		int _pathLen = 0;
		char *_path = stack[i]->path( &_pathLen );

		tempName = (char *)realloc( tempName, tempLen + _pathLen + 2 );
		memcpy( &tempName[tempLen], _path, _pathLen );
		tempLen += _pathLen;

		if( stack[i]->zsym().isDir() ) {
			tempName[tempLen] = '/';
			tempLen++;
		}
		tempName[tempLen] = 0;
	}
	if( pathLen ) {
		*pathLen = tempLen;
	}
	return tempName;
}


//==================================================================================
// Dump
//==================================================================================

void zsymDump( char *symbol, int toStdout, char *label ) {
	// @TODO: Send to either stdout or outputdebug
	if( !symbol ) {
		symbol = "/";
	}
	if( !label ) {
		label = "";
	}
	int labelLen = strlen( label );
	int starLen = 78 - labelLen;
	char stars[100];
	memset( stars, '*', starLen );
	stars[starLen] = 0;
	printf( "%s %s\n", stars, label );
	printf( "ROOT: %p refCount=%d allocCount=%d numRecords=%d\n", ZSymContextMem::root, ZSymContextMem::root ? ZSymContextMem::root->refCount : 0, ZSymContextMem::allocCount, ZSymContextMem::root->dir->numRecords );
	printf( "\n" );
	for( ZSymDirListRecurse l(zsymIGNORE_LINKS,symbol); l.next(); ) {
		int depth = l.depth();
		char indent[128];
		memset( indent, ' ', depth*2 );
		indent[depth*2] = 0;
		int pathLen = 0;
		char *path = l.path( &pathLen );
		char *_path = (char *)alloca( pathLen+1 );
		memcpy( _path, path, pathLen );
		_path[ pathLen ] =0;
		printf( "%s%s, ", indent, _path );
		l.zsym().dump( toStdout, depth*2 );
		printf( "\n" );
	}
	for( ZSymContextMem *o=ZSymContextMem::orphanHead; o; o=o->orphanNext ) {
		printf( "ORPHAN: %p refCount=%d\n", o, o->refCount );
	}
}


//==================================================================================
// SELF TEST
//==================================================================================

#ifdef ZSYM_SELF_TEST
#pragma message( "BUILDING ZSymHash_SELF_TEST" )

#include "zconsole.h"

int intRoutine( int a, int b ) {
	return a + b;
}

int proofThatVoidRoutineRan = 0;
void voidRoutine( int a, int b ) {
	proofThatVoidRoutineRan = 1;
}

void test() {
	// 1. Values:
	//   1.1  Int, Str with Get, Put
	//   1.2  catC, catI, catS
	//   1.3  Cat growBy
	//   1.4  onFail values
	//   1.5  binary strings

	// 2. Path format:
	//   2.1  Paths binary
	//   2.2  Binary paths with slashes escaped

	// 3. Resolving:
	//   3.1  Root
	//   3.2  Subdirs relative to root
	//   3.3  Relative paths
	//   3.4  Relative paths using . & ..
	//   3.5  Empty
	//   3.6  Flags: FORCEDIR, FORCEFILE, FORCEASSPECIFIED
	//   3.7  Attmpting to convert root to a file
	//   3.8  Attmpting to go past root upwards

	// 4. Context:
	//   4.1  Using a zsym to a non existant resource
	//   4.2  Switching a zsym to a different context that exists
	//   4.3  Switching a zsym to a different context that does not exist
	//   4.4  Using a put does NOT switch the context

	// 5. Reference counting:
	//   5.1  Reference counting
	//   5.2  Referencing an orphan
	//   5.3  Garbage collection

	// 6. Collisions:
	//   6.1  Converting a dir to a file
	//   6.2  Converting a file to a dir
	//   6.3  Collision with FAIL flag

	// 7. Copy & Conversion:
	//   7.1  Setting from another zsym
	//   7.2  Type conversion
	//   7.3  Deep copy
	//   7.4  Deep copy of a dir to a file should convert file to dir
	//   7.5  Putting a nil

	// 8. Links:
	//   8.1  Simple link within same protocol
	//   8.2  Link to a dir, referencing off of that link to a sub component
	//   8.3  Link from mem to file
	//   8.4  Removing a link
	//   8.5  Removing a target
	//   8.6  Circular links
	//   8.7  Link ref counting

	// 9. Removing:
	//   9.1  rm of a file
	//   9.2  Recursive rm of a dir
	//   9.3  Safty override on file system

	// 10. Listing:
	//   10.1  Listing of empty
	//   10.2  Listing of a single file
	//   10.3  Listing of a subdir recursively
	//   10.4  Removing while listing

	// 11. Locals:
	//   11.1  Creating a local and passing out of scope

	// 12. Pointer types:
	//   12.1  Creating a pointer type, setting it, removing it
	//   12.2  Setting a pointer type with a type cast

	// 13. Routine Ptrs
	//   13.1  Test routine call to something that returns int
	//   13.2  Test routine call to something that returns void
	//   13.3  Test default args
	
	// 14. Fetching local and global names
	//   14.1  Test fetch names with file termination
	//   14.2  Test fetch names with dir termination
	//   14.3  Test fetch names simple file

	// 15. Importing
	//   15.1  Import a single file
	//   15.2  Import a complex tree
	//   15.3  Incorrectly import to a file

	// 16. Lists
	//   16.1  Push onto tail of existing list
	//   16.2  Push onto tail of a empty list

	// 1.1  Int, Str with Get, Put
	//----------------------------------------------------------------
	{
		ZSym a( "/a" );
		a.putI( 0 );
		assert( a.getI() == 0 );
		a.putI( -100 );
		assert( a.getI() == -100 );

		ZSym b( "/b" );
		b.putS( "This is b" );
		assert( !strcmp(b.getS(),"This is b") );

		b.putS( "This is another b" );
		int len;
		char *s = b.getS( "", -1, "", &len );
		assert( len == 17 );
		assert( !strcmp(b.getS(),"This is another b") );
	}

	// 1.2  catC, catI, catS
	//----------------------------------------------------------------
	{
		ZSym c( "/c" );
		c.catC( 't' );
		c.catC( 'e' );
		c.catC( 's' );
		c.catC( 't' );
		assert( !memcmp(c.getS(),"test",4) );
		c.catS( "ing" );
		assert( !strcmp(c.getS(),"testing") );
		c.catI( 0x34333231 );
		assert( !strcmp(c.getS(),"testing1234") );
	}

	// 1.3  Cat growBy
	//----------------------------------------------------------------
	{
		ZSym d( "/d" );
		d.catI( 1, 32 );
		int *dptr = (int *)d.getS();
		assert( dptr[-1] == 4 );
		assert( dptr[-2] == 48 );
		d.catI( 2, 32 );
		dptr = (int *)d.getS();
		assert( dptr[-1] == 8 );
		assert( dptr[-2] == 48 );
		d.catS( "                                                    ", 44, 32 );
		dptr = (int *)d.getS();
		assert( dptr[-1] == 52 );
		assert( dptr[-2] == 128 );
	}

	// 1.4  onFail values
	//----------------------------------------------------------------
	{
		ZSym e( zsymOPEN_OR_FAIL, "/e" );
		assert( e.getI( "", -1, 0 ) == 0 );
		assert( !strcmp( e.getS( "", -1, "nil" ), "nil" ) );
	}

	// 1.5  binary strings
	//----------------------------------------------------------------
	{
		ZSym a( "/a" );
		char bin[] = { 'B', 0, 'I', 0, 'N' };
		a.putS( bin, 5 );
		int valLen = 0;
		char *_a = a.getS( &valLen );
		assert( valLen == 5 );
		assert( !memcmp( _a, bin, 5 ) );
	}


	// 2.1  Paths binary
	//----------------------------------------------------------------
	{
// @TODO
//		int binPath = 10;
//		ZSym a( (char *)&binPath, sizeof(int) );
//		a.putI( 100 );
//		assert( a.getI() == 100 );
	}

	// 2.2  Paths binary with slashes escaped
	//----------------------------------------------------------------
	{
		// @TODO
	}


	// 3.1  Root
	//----------------------------------------------------------------
	{
		ZSym a( "/" );
		a.putI( "a", 10 );
		assert( a.getI("a") == 10 );
		a.resolve( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE, "b", -1 );
		a.putI( 20 );
		assert( ZSym("/b").getI() == 20 );
	}

	// 3.2  Subdirs relative to root
	//----------------------------------------------------------------
	{
		ZSym a( "/" );
		a.putI( "b/b1", 10 );
		a.putI( "c/c1", 20 );
		assert( ZSym("/b/b1").getI() == 10 );
		assert( ZSym("/c/c1").getI() == 20 );
	}

	// 3.3  Relative paths
	//----------------------------------------------------------------
	{
		ZSym a( "/a/" );
		ZSym b( "/b/" );
		a.putI( "a1", 100 );
		b.putI( "b1", 200 );
		assert( a.getI("a1") == 100 );
		assert( b.getI("b1") == 200 );
		assert( ZSym("/a/a1").getI() == 100 );
		assert( ZSym("/b/b1").getI() == 200 );
	}

	// 3.4  Relative paths using . & ..
	//----------------------------------------------------------------
	{
		ZSym( "/a/one" ).putI( 1 );
		ZSym( "/a/a1/a11" ).putI( 111 );
		ZSym( "/a/a1/a12" ).putI( 112 );
		ZSym( "/a/a2/a11" ).putI( 211 );
		ZSym( "/a/a2/a12" ).putI( 212 );
		ZSym a1( "/a/a1/" );
		assert( a1.getI("a11") == 111 );
		assert( a1.getI("../one") == 1 );
		assert( a1.getI("../a2/a11") == 211 );
		assert( a1.getI("./../a2/a11") == 211 );
		assert( a1.getI("../a2/../one") == 1 );
	}

	// 3.5  Empty
	//----------------------------------------------------------------
	{
		ZSym a( "" );
		assert( ! a.isValid() );

		ZSym b;
		assert( ! b.isValid() );
	}

	// 3.6  Flags: FORCEDIR, FORCEFILE, FORCEASSPECIFIED
	//----------------------------------------------------------------
	{
		// TEST convert dir to file
		ZSym ad0( zsymOPEN_OR_CREATE|zsymFORCE_TO_DIR, "/a/" );
		ZSym af0( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE, "/a/" );
		af0.putI( 10 );
		assert( af0.getI() == 10 );

		ZSym ad1( zsymOPEN_OR_CREATE|zsymFORCE_TO_DIR, "/a/" );
		ZSym af1( zsymOPEN_OR_CREATE|zsymFORCE_AS_SPECIFIED, "/a" );
		af1.putI( 20 );
		assert( af1.getI() == 20 );

		// TEST convert file to dir
		ZSym af2( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE, "/a/" );
		ZSym ad2( zsymOPEN_OR_CREATE|zsymFORCE_TO_DIR, "/a" );
		ad2.putI( "file", 30 );
		assert( ad2.getI("file") == 30 );

		ZSym af3( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE, "/a/" );
		ZSym ad3( zsymOPEN_OR_CREATE|zsymFORCE_AS_SPECIFIED, "/a/" );
		ad3.putI( "file", 40 );
		assert( ad3.getI("file") == 40 );
	}

	// 3.7  Attmpting to convert root to a file
	//----------------------------------------------------------------
	{
		ZSym a( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE, "/" );
		assert( ! a.isValid() );

		ZSym b( "/a/" );
		assert( b.isDir() );
		b.resolve( zsymOPEN_OR_CREATE|zsymFORCE_TO_FILE, ".." );
		assert( ! b.isValid() );
	}

	// 3.8  Attmpting to go past root upwards
	//----------------------------------------------------------------
	{
		ZSym b( "/a/" );
		assert( b.isDir() );
		b.resolve( "../../b" );
		assert( ! b.isValid() );
	}

	// 4.1  Using a zsym to a non existing resource
	//----------------------------------------------------------------
	{
		ZSym a( zsymOPEN_OR_FAIL, "/nonexist" );
		assert( !a.isValid() );
		assert( !a.isDir() );
		assert( !a.isFile() );
		assert( !a.isLink() );
	}

	// 4.2  Switching a zsym to a different context that exists
	//----------------------------------------------------------------
	{
		ZSym a( "/a/" );
		a.putI( "aa", 10 );
		assert( a.getI("aa") == 10 );
		ZSym b( "/b/" );
		b.putI( "bb", 20 );
		assert( b.getI("bb") == 20 );

		b.resolve( "/a/" );
		assert( b.getI("aa") == 10 );
	}

	// 4.3  Switching a zsym to a different context that does not exist
	//----------------------------------------------------------------
	{
		ZSym a( "/a/" );
		a.putI( "aa", 10 );
		assert( a.getI("aa") == 10 );

		a.resolve( zsymOPEN_OR_FAIL, "/nonexists" );
		assert( !a.isValid() );
	}

	// 4.4  Using a put does NOT switch the context
	//----------------------------------------------------------------
	{
		ZSym a( "/a/" );
		a.putI( "aa1", 10 );
		a.putI( "aa2", 20 );
		assert( a.getI("aa1") == 10 );
		assert( a.getI("aa2") == 20 );
	}


	// 5.1  Reference counting
	//----------------------------------------------------------------
	{
		ZSym a0( "/reftest" );
		{
			ZSym a1( "/reftest" );
			assert( ((ZSymContextMem *)a1.context)->refCount == 3 );
			assert( ((ZSymContextMem *)a0.context)->refCount == 3 );
				// 3: 1 for the dir that points to it (root), 1 for a0, 1 for a1
		}
		assert( ((ZSymContextMem *)a0.context)->refCount == 2 );
	}

	// 5.2  Referencing an orphan
	//----------------------------------------------------------------
	{
		ZSym a0( "/reftest" );
		ZSym a1( "/reftest" );
		a0.rm();
		assert( !a1.isValid() );
	}

	// 5.3  Garbage collection
	//----------------------------------------------------------------
	{
		int allocOld = ZSymContextMem::allocCount;
		int orphanOld = ZSymContextMem::orphanCount;
		{
			ZSym a0( "/reftest" );
			ZSym a1( "/reftest" );
			a0.rm();
			assert( !a1.isValid() );
			assert( ZSymContextMem::orphanCount == orphanOld+1 );
		}
		assert( allocOld == ZSymContextMem::allocCount );
		assert( orphanOld == ZSymContextMem::orphanCount );
	}


	//   6.1  Converting a dir to a file
	//----------------------------------------------------------------
	{
		ZSym a( "/a/" );
		assert( a.isDir() );
		a.resolve( "/a" );
		assert( a.isFile() );
	}

	//   6.2  Converting a file to a dir
	//----------------------------------------------------------------
	{
		ZSym a( "/a" );
		assert( a.isFile() );
		a.resolve( "/a/" );
		assert( a.isDir() );
	}

	//   6.3  Collision with FAIL flag
	//----------------------------------------------------------------
	{
		ZSym a( "/a" );
		assert( a.isFile() );
		a.resolve( zsymOPEN_OR_FAIL, "/a/" );
		assert( ! a.isValid() );
		assert( ZSym("/a").isFile() );
	}


	//   7.1  Setting from another zsym
	//----------------------------------------------------------------
	{
		ZSym a( "/a" );
		a.putI( 100 );
		ZSym b( "/b" );
		b.put( a );
		assert( b.getI() == 100 );
	}

	//   7.2  Type conversion
	//----------------------------------------------------------------
	{
		ZSym a( "/a" );
		a.putI( 100 );
		assert( a.getI() == 100 );

		a.putS( "test" );
		assert( !strcmp(a.getS(),"test") );

		a.putI( 100 );
		assert( a.getI() == 100 );
	}

	//   7.3  Deep copy
	//----------------------------------------------------------------
	{
		ZSym( "/a/a1" ).putI( 1 );
		ZSym( "/a/a2" ).putI( 2 );
		ZSym( "/a/b/b1" ).putI( 3 );
		ZSym( "/a/b/c/" );
		ZSym a( "/acopy/" );
		a.put( ZSym("/a/") );
		assert( a.getI("a1") == 1 );
		assert( a.getI("a2") == 2 );
		assert( a.getI("b/b1") == 3 );
		assert( ZSym(zsymOPEN_OR_FAIL,"/acopy/b/c").isDir() );
	}

	//   7.4  Deep copy of a dir to a file should convert file to dir
	//----------------------------------------------------------------
	{
		ZSym dst( "/d" );
		dst.putI( 10 );
		assert( ZSym("/d").getI() == 10 );

		ZSym( "/e/f" ).putI( 10 );
		ZSym( "/e/g" ).putI( 20 );
		assert( ZSym("/e/f").getI() == 10 );
		assert( ZSym("/e/g").getI() == 20 );

		dst.put( ZSym("/e/") );
		assert( dst.getI( "f" ) == 10 );
		assert( dst.getI( "g" ) == 20 );
	}

	//   7.5  Putting a nil
	//----------------------------------------------------------------
	{
		ZSym dst( "/a" );
		ZSym src;
		dst.put( src );
		assert( dst.context );
		int nodeType, fileType;
		dst.getType( nodeType, fileType );
		assert( nodeType==zsymUNK && fileType==zsymZ );
	}

	//   8.1  Simple link within same protocol
	//----------------------------------------------------------------
	{
		ZSym( "/a" ).putI( 10 );
		ZSym().link( "/linktoa", "/a" );
		assert( ZSym( "/linktoa" ).getI() == 10 );
	}

	//   8.2  Link to a dir, referencing off of that link to a sub component
	//----------------------------------------------------------------
	{
		ZSym( "/a/b" ).putI( 20 );
		ZSym( "/a/c/d" ).putI( 30 );
		ZSym().link( "/linktoa", "/a/" );
		assert( ZSym( "/linktoa/b" ).getI() == 20 );
		assert( ZSym( "/linktoa/c/d" ).getI() == 30 );
	}

	//   8.3  Link from mem to file
	//----------------------------------------------------------------
	{
		// @TODO
	}

	//   8.4  Removing a link
	//----------------------------------------------------------------
	{
		ZSym( "/a" ).putI( 10 );
		ZSym().link( "/linktoa", "/a" );
		assert( ZSym( "/linktoa" ).getI() == 10 );
		assert( ZSym( "/a" ).getI() == 10 );
		ZSym( zsymOPEN_OR_FAIL|zsymDONT_FOLLOW_LINKS, "/linktoa" ).rm();
		assert( ZSym( zsymOPEN_OR_FAIL, "/linktoa" ).getI() == 0 );
		assert( ZSym( "/a" ).getI() == 10 );
	}

	//   8.5  Removing a target
	//----------------------------------------------------------------
	{
		ZSym( "/a" ).putI( 10 );
		ZSym().link( "/linktoa", "/a" );
		assert( ZSym( "/linktoa" ).getI() == 10 );
		assert( ZSym( "/a" ).getI() == 10 );
		ZSym( zsymOPEN_OR_FAIL, "/linktoa" ).rm();
		ZSym l( zsymOPEN_OR_FAIL|zsymDONT_FOLLOW_LINKS, "/linktoa" );
		assert( l.isLink() );
		assert( ZSym( "/a" ).getI() == 0 );
	}

	//   8.6  Circular links
	//----------------------------------------------------------------
	{
		// @TODO
	}

	//   8.7  Link ref counting
	//----------------------------------------------------------------
	{
		// @TODO
	}


	//   9.1  rm of a file
	//----------------------------------------------------------------
	{
		ZSym a( "/a" );
		a.putI( 10 );
		assert( a.getI() == 10 );
		a.rm();
		assert( ! a.isValid() );
		assert( ! ZSym( zsymOPEN_OR_FAIL, "/a" ).isValid() );
	}

	//   9.2  Recursive rm of a dir
	//----------------------------------------------------------------
	{
		ZSym a( "/a/" );
		a.putI( "b/c/d", 10 );
		a.putI( "c/d/e", 10 );
		a.putI( "d/e/f", 10 );
		a.rm();
		assert( ! a.isValid() );
		assert( ! ZSym( zsymOPEN_OR_FAIL, "/a" ).isValid() );
	}

	//   9.3  Safty override on file system
	//----------------------------------------------------------------
	{
		// @TODO
	}


	//   10.1  Listing of empty
	//----------------------------------------------------------------
	{
		int count = 0;
		for( ZSymDirListRecurse l(""); l.next(); ) {
			count++;
		}
		assert( count == 0 );
	}

	//   10.2  Listing of a single file
	//----------------------------------------------------------------
	{
		ZSym( "/a" ).putI( 10 );
		int count = 0;
		int val = 0;
		for( ZSymDirListRecurse l("/a"); l.next(); ) {
			count++;
			val = l.zsym().getI();
		}
		assert( count == 1 && val == 10 );
	}

	//   10.3  Listing of a subdir recursively
	//----------------------------------------------------------------
	{
		ZSym( "/a/a/a" ).putI( 1 );
		ZSym( "/a/a/b" ).putI( 2 );
		ZSym( "/a/b/a" ).putI( 3 );
		ZSym( "/a/c" ).putI( 4 );
		int count = 0;
		for( ZSymDirListRecurse l("/a"); l.next(); ) {
			char *p = l.path();
			count++;
		}
		assert( count == 6 );
	}

	//   10.4  Removing while listing
	//----------------------------------------------------------------
	{
		// @TODO
	}


	//   11.1  Creating a local and passing out of scope
	//----------------------------------------------------------------
	{
		int beforeAllocCount = ZSymContextMem::allocCount;
		int beforeOrphanCount = ZSymContextMem::orphanCount;
		{
			ZSymLocal l;
			l.putI( "a", 1 );
			l.putI( "b", 2 );
			assert( l.getI("a")==1 );
			assert( l.getI("b")==2 );
			assert( ZSymContextMem::allocCount == beforeAllocCount+3 );
		}
		assert( ZSymContextMem::allocCount == beforeAllocCount );
		assert( ZSymContextMem::orphanCount == beforeOrphanCount );
	}

	//   12.1  Creating a pointer type, setting it, removing it
	//----------------------------------------------------------------
	{
		int a = 1;
		ZSym( "/a" ).ptrI( &a );
		assert( ZSym( "/a" ).getI() == 1 );
		a = 2;
		assert( ZSym( "/a" ).getI() == 2 );
		ZSym( "/a" ).putI( 10 );
			// Note that this is tricky!  If you "put" on a symbol
			// which is alrady a pointer, it sets the indirect value, NOT THE SYMBOL!
			// Thus, if you wanted to change the symbol "/a" from a pointer
			// type to something else you have to remove it first!
		assert( a == 10 );

		ZSym( "/a" ).rm();
		ZSym( "/a" ).putI( 20 );
		assert( ZSym( "/a" ).getI() == 20 );
		assert( a == 10 );
	}

	//   12.2  Setting a pointer type with a type cast
	//----------------------------------------------------------------
	{
		int a = 1;
		ZSym( "/a" ).ptrI( &a );
		assert( ZSym( "/a" ).getI() == 1 );

		ZSym( "/a" ).putS( "20" );
		assert( a == 20 );

		ZSym( "/a" ).rm();
		assert( ! ZSym( zsymOPEN_OR_FAIL, "/a" ).isValid() );
	}


	//   13.1  Test routine call to something that returns int
	//----------------------------------------------------------------
	{
		ZSym testr( "/testr" );
		testr.putR( intRoutine, "i argx:i=5 argy:i=10" );

		ZSymLocal ret( 0, zsymFORCE_TO_FILE );
		ZSymLocal args;
		args.putI( "argx", 1 );
		args.putI( "argy", 2 );

		testr.exeR( args, ret );

		assert( ret.getI() == 3 );
	}

	//   13.2  Test routine call to something that returns void
	//----------------------------------------------------------------
	{
		ZSym testr( "/testr" );
		testr.putR( voidRoutine, "z argx:i=5 argy:i=10" );

		ZSymLocal ret( 0, zsymFORCE_TO_FILE );
		ZSymLocal args;
		args.putI( "argx", 1 );
		args.putI( "argy", 2 );

		proofThatVoidRoutineRan = 0;
		testr.exeR( args, ret );
		assert( proofThatVoidRoutineRan );
	}


	//   13.3  Test default args
	//----------------------------------------------------------------
	{
		ZSym testr( "/testr" );
		testr.putR( intRoutine, "i argx:i=5 argy:i=10" );

		ZSymLocal ret( 0, zsymFORCE_TO_FILE );
		ZSymLocal args;
		testr.exeR( args, ret );
		assert( ret.getI() == 15 );
	}


	//   14.1  Test fetch names with file termination
	//----------------------------------------------------------------
	{
		ZSym a( "/a/b/c" );
		a.putI( 1 );

		int nameLocalLen = 0;
		char *nameLocal = a.nameLocal( &nameLocalLen );
		assert( !strcmp(nameLocal,"c") );
		free( nameLocal );

		int nameGlobalLen = 0;
		char *nameGlobal = a.nameGlobal( &nameGlobalLen );
		assert( !strcmp(nameGlobal,"/a/b/c") );
		free( nameGlobal );
	}

	//   14.2  Test fetch names with dir termination
	//----------------------------------------------------------------
	{
		ZSym a( "/a/b/c/" );

		int nameLocalLen = 0;
		char *nameLocal = a.nameLocal( &nameLocalLen );
		assert( !strcmp(nameLocal,"c/") );
		free( nameLocal );

		int nameGlobalLen = 0;
		char *nameGlobal = a.nameGlobal( &nameGlobalLen );
		assert( !strcmp(nameGlobal,"/a/b/c/") );
		free( nameGlobal );
	}

	//   14.3  Test fetch names simple file
	//----------------------------------------------------------------
	{
		ZSym a( "/a" );
		a.putI( 1 );

		int nameLocalLen = 0;
		char *nameLocal = a.nameLocal( &nameLocalLen );
		assert( !strcmp(nameLocal,"a") );
		free( nameLocal );

		int nameGlobalLen = 0;
		char *nameGlobal = a.nameGlobal( &nameGlobalLen );
		assert( !strcmp(nameGlobal,"/a") );
		free( nameGlobal );
	}


	//   15.1  Import a single file, both with a name and with a temp
	//----------------------------------------------------------------
	{
		ZSym a( "/a" );
		a.putI( 10 );
		ZSym b( "/b/" );
		b.putI( "inb", 20 );
		b.import( a, "aimported" );
		assert( b.getI( "aimported" ) == 10 );
		assert( b.getI( "inb" ) == 20 );

		b.import( a );
		assert( b.getI( "aimported" ) == 10 );
		assert( b.getI( "inb" ) == 20 );
		assert( b.getI( "__temp0" ) == 10 );
	}

	//   15.2  Import a complex tree
	//----------------------------------------------------------------
	{
		ZSym( "/a/" ).rm();
		ZSym( "/b/" ).rm();

		ZSym( "/a/a1" ).putI( 10 );
		ZSym( "/a/a2" ).putI( 20 );
		ZSym( "/a/ad1/a1" ).putI( 30 );
		ZSym( "/a/ad1/a2" ).putI( 40 );
		ZSym( "/a/ad1/add/a1" ).putI( 50 );
		ZSym( "/a/ad2/add/a1" ).putI( 60 );

		ZSym a( "/a/" );
		ZSym b( "/b/" );
		b.putI( "inb1", 100 );
		b.putI( "inb2", 200 );
		b.import( a );
		assert( b.getI("inb1") == 100 );
		assert( b.getI("inb2") == 200 );
		assert( b.getI("a1") == 10 );
		assert( b.getI("a2") == 20 );
		assert( b.getI("ad1/a1") == 30 );
		assert( b.getI("ad1/a2") == 40 );
		assert( b.getI("ad1/add/a1") == 50 );
		assert( b.getI("ad2/add/a1") == 60 );
	}

	//   15.3  Incorrectly import to a file
	//----------------------------------------------------------------
	{
		ZSym( "/a/" ).rm();
		ZSym( "/b/" ).rm();
		ZSym( "/a/a1" ).putI( 10 );
		ZSym( "/a/a2" ).putI( 20 );
		ZSym a( "/a/" );
		ZSym b( "/b" );
		b.putI( 10 );
		b.import( a );
		assert( b.getI() == 10 );
	}

	//   16.1  Push onto tail of existing list
	//----------------------------------------------------------------
	{
		ZSym( "/listtest/0" ).putI( 10 );
		ZSym( "/listtest/1" ).putI( 20 );
		ZSym list( "/listtest/" );
		ZSym topush( "/test0" );
		topush.putI( 30 );
		list.pushTail( topush );
		topush.rm();
		assert( ZSym( "/listtest/2" ).getI() == 30 );
	}

	//   16.2  Push onto tail of a empty list
	//----------------------------------------------------------------
	{
	}
}

void main() {
	zconsolePositionAt( 0, 0, 800, 1200 );
	test();
}

#endif

