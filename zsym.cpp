#error( "Deprecated" )

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
#ifdef WIN32
#include "malloc.h"
#endif
// MODULE includes:
#include "zsym.h"

#include "zwildcard.h"
	// This is only used for FILE until I reimplement
	// @TODO remove this dependency

//==================================================================================
// ZSymMemFile
//==================================================================================

struct ZSymMemFile {
	int type;
		// See ZSym fileType for defines
		// zsymS are allocLen and length prefix-encoded (may contain nuls)
		// and also always contain nul at end of string just for good measure

	union {
		int valI;
		float valF;
		double valD;
		char *valS;
	};

	// int
	//-----------------------------------------------
	void putI( int i );
	int geti();
	int getI();

	// str
	//-----------------------------------------------
	void putS( char *s, int len=-1 );
	void catS( char *s, int len, int growBy );
	char *gets( int *len=0 );
	char *getS( int *len=0 );

	// Construct / Destruct
	//-----------------------------------------------

//	ZSymMemFile *clone() {
//		ZSymMemFile *n = new ZSymMemFile;
//		memcpy( n, this, sizeof(*this) );
//		if( type == zsymS ) {
//			int allocSize = ((int *)valS)[0];
//			n->valS = (char *)malloc( allocSize );
//			memcpy( n->valS, valS, allocSize );
//		}
//		return n;
//	}

	void clear();

	ZSymMemFile() {
		type = zsymZ;
		valI = 0;
	}

	~ZSymMemFile() {
		clear();
	}
};

void ZSymMemFile::clear() {
	if( type == zsymS ) {
		free( valS );
	}
	type = zsymZ;
}

void ZSymMemFile::putI( int i ) {
	clear();
	type = zsymI;
	valI = i;
}

int ZSymMemFile::geti() {
	assert( type == zsymI );
	return valI;
}

int ZSymMemFile::getI() {
	switch( type ) {
		case zsymI:
			return valI;
		case zsymF:
			return (int)valF;
		case zsymS:
			assert( 0 );
			return 0;
			// @TODO, convert string to int
		default:
			// Corrupt
			assert( 0 );
			return 0;
	}
}

void ZSymMemFile::putS( char *s, int len ) {
	clear();
	type = zsymS;
	if( len == -1 ) {
		len = strlen( s );
	}
	int allocSize = 2*sizeof(int) + len + 1;
	allocSize += (4-(allocSize&3))&3;

	valS = (char *)malloc( allocSize );
	((int *)valS)[0] = allocSize;
	((int *)valS)[1] = len;
	memcpy( valS+2*sizeof(int), s, len );
	valS[ 2*sizeof(int) + len ] = 0;
}

void ZSymMemFile::catS( char *s, int len, int growBy ) {
	if( type != zsymS ) {
		clear();
		type = zsymS;
	}
	if( len == -1 ) {
		len = strlen( s );
	}
	if( growBy == -1 ) {
		growBy = 16;
	}

	int allocSize = 0;
	int oldLen = 0;
	if( valS ) {
		// Already allocated, check if needs to grow
		allocSize = ((int *)valS)[0];
		oldLen = ((int *)valS)[1];
		int remains = allocSize - oldLen - 2*sizeof(int);
		if( remains < len+1 ) {
			// +1 for the nul that we always want at the end
			allocSize += len + growBy + 1;
			allocSize += (4-(allocSize&3))&3;
			valS = (char *)realloc( valS, allocSize );
		}
	}
	else {
		allocSize = 2*sizeof(int) + len + growBy + 1;
		allocSize += (4-(allocSize&3))&3;
		valS = (char *)malloc( allocSize );
		((int *)valS)[0] = 0;
		((int *)valS)[1] = 0;
	}

	((int *)valS)[0] = allocSize;
	((int *)valS)[1] += len;
	int tail = 2*sizeof(int)+oldLen;
	memcpy( valS+tail, s, len );
	memset( valS+tail+len, 0, allocSize-tail-len );
}

char *ZSymMemFile::gets( int *len ) {
	assert( type == zsymS );
	if( len ) {
		*len = ((int *)valS)[1];
	}
	return valS + 2*sizeof(int);
}

char *ZSymMemFile::getS( int *len ) {
	switch( type ) {
		case zsymI:
			assert( 0 );
			// @TODO, convert int to str
			return 0;
		case zsymF:
			assert( 0 );
			// @TODO, convert float to str
			return 0;
		case zsymS:
			if( len ) {
				*len = ((int *)valS)[1];
			}
			return valS + 2*sizeof(int);
		default:
			// Corrupt
			assert( 0 );
			return 0;
	}
}

//==================================================================================
// ZSymContextMem
//==================================================================================

struct ZSymContextMem {
	int type;
	int refCount;
	union {
		ZSymContextMem *parent;
		ZSymContextMem *orphanPrev;
	};
	union {
		void *unk;
		ZSymMemFile *file;
		char *link;
		ZHashTable *dir;
		ZSymContextMem *orphanNext;
	};

	static ZSymContextMem *root;
	static ZSymContextMem *orphanHead;
	static int allocCount;
	static int orphanCount;

	void createLink( char *target );
		// Create a link to target

	void clear();
		// Deletes all allocated memory

	void invalidate();
		// Removes any pointers and marks as unknown
		// There may still be references to this node
		// and if they attempt to view it, they will see invalid

	void incRef() { refCount++; }
	void decRef();
		// Decrements the references. If refCount hits
		// zero then it deletes this

	void rm();
		// Removes this node, unlinking from parent directory, and all children
		// invalidates this node, but remember that the node is not
		// deleted until the refCount == 0

	void recurseRemoveChildren();
		// Recursively removes all of the children of this dir
		// but does not remove the directory itself

//	ZSymContextMem *clone();

	ZSymContextMem( int _type, ZSymContextMem *_parent );
	~ZSymContextMem();
};

ZSymContextMem *ZSymContextMem::root = 0;
ZSymContextMem *ZSymContextMem::orphanHead = 0;
int ZSymContextMem::allocCount = 0;
int ZSymContextMem::orphanCount = 0;

ZSymContextMem::ZSymContextMem( int _type, ZSymContextMem *_parent ) {
	type = _type;
	parent = _parent;
	refCount = 1;
	if( type == zsymDIR ) {
		dir = new ZHashTable;
	}
	else if( type == zsymFILE ) {
		file = new ZSymMemFile;
	}
	else if( type == zsymLINK ) {
		assert( 0 );
	}
	else {
		assert( 0 );
	}
	allocCount++;
}

void ZSymContextMem::clear() {
	if( type == zsymDIR ) {
		assert( dir->activeCount() == 0 );
		delete dir;
		dir = 0;
	}
	else if( type == zsymFILE ) {
		delete file;
		file = 0;
	}
	else if( type == zsymLINK ) {
		free( link );
		link = 0;
	}
	type = zsymUNK;
}

ZSymContextMem::~ZSymContextMem() {
	clear();
	if( type == zsymUNK ) {
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

void ZSymContextMem::createLink( char *target ) {
	clear();
	type = zsymLINK;
	link = strdup( target );
}

void ZSymContextMem::invalidate() {
	clear();
	type = zsymUNK;

	// LINK into the orphan list
	orphanCount++;
	if( orphanHead ) {
		orphanHead->orphanPrev = this;
	}
	orphanNext = orphanHead;
	orphanPrev = 0;
	orphanHead = this;
}

void ZSymContextMem::decRef() {
	refCount--;
	assert( refCount >= 0 );
	if( refCount == 0 ) {
		assert( type == zsymUNK );
		delete this;
	}
}

void ZSymContextMem::rm() {
	// REMOVE children
	if( type == zsymDIR ) {
		recurseRemoveChildren();
	}

	// REMOVE from parent directory
	if( parent ) {
		ZHashTable *parentDir = parent->dir;
		int size = parentDir->size();
		for( int i=0; i<size; i++ ) {
			ZSymContextMem *ni = (ZSymContextMem *)parentDir->getValp( i );
			if( ni == this ) {
				parentDir->del( parentDir->getKey(i) );
				decRef();
				break;
			}
		}
	}

	invalidate();
}

void ZSymContextMem::recurseRemoveChildren() {
	int i;
	assert( type == zsymDIR );
	int size = dir->size();
	for( i=0; i<size; i++ ) {
		ZSymContextMem *si = (ZSymContextMem *)dir->getValp(i);
		if( si ) {
			if( si->type == zsymDIR ) {
				si->recurseRemoveChildren();
			}
			dir->del( dir->getKey(i) );
			si->invalidate();
			si->decRef();
		}
	}
}

//ZSymContextMem *ZSymContextMem::clone() {
//	ZSymContextMem *n = new ZSymContextMem;
//	n->type = type;
//	n->refCount = 1;
//	n->parent = 0;
//	if( type == zsymFILE ) {
//		n->file = file->clone();
//	}
//	else if( type == zsymDIR ) {
//		n->dir = new ZHashTable();
//		n->dir->copyFrom( *dir );
//	}
//	else if( type == zsymLINK ) {
//		n->link = strdup( link );
//	}
//	return n;
//}

//==================================================================================
// ZSymProtocolMem
//==================================================================================

struct ZSymProtocolMem : ZSymProtocol {
	virtual int resolve( int segCount, char **segs, void **context, int *err, int action ) {
		*err = 0;

		// INIT root if it hasn't been
		if( !ZSymContextMem::root ) {
			ZSymContextMem::root = new ZSymContextMem( zsymDIR, 0 );
		}

		// SETUP the currentDir with either the current context or root by default
		int startSeg = 0;
		ZSymContextMem *contextNode = *(ZSymContextMem **)context;
		ZSymContextMem *currentDir = 0;
		if( segs[0][0] == 0 && segCount > 1 ) {
			// The segCount> 1 is here so that you can re-resolve a symvol 
			// for eample after it has been converted to a link by using an empty string
			// Root reference
			currentDir = ZSymContextMem::root;
			startSeg++;
			if( segCount == 2 && segs[1][0] == 0 ) {
				// Special case, root reference
				*(ZSymContextMem **)context = ZSymContextMem::root;
				*err = 0;
				return 2;
			}
		}

		// CHECK for links
		if( contextNode && contextNode->type == zsymLINK ) {
			*(char **)context = contextNode->link;
			*err = zsymRESOLVE_LINK;
			return 0;
		}

		if( segs[0][0] != 0 && contextNode && contextNode->type == zsymDIR ) {
			// Relative reference because root reference start with an empty segment
			currentDir = contextNode;
		}

		// EXAMINE the last segment. If it is blank then that means that they are asking for a dir
		int isDirLookup = 0;
		if( segs[segCount-1][0] == 0 ) {
			isDirLookup = 1;
			segCount--;
		}

		if( !currentDir ) {
			// Trying to reference off a non existing context
			*err = zsymRESOLVE_NOT_FOUND;
			return 0;
		}

		for( int i=startSeg; i < segCount; i++ ) {
			// CHECK for special . & ..
			if( segs[i][0] == '.' ) {
				if( segs[i][1] == '.' && segs[i][1] == '.' && segs[i][2] == 0 ) {
					// Parent reference, bounded to stop it going above root
					currentDir = currentDir->parent ? currentDir->parent : currentDir;
					*(ZSymContextMem **)context = currentDir;
					continue;
				}
				else if( segs[i][1] == 0 ) {
					// Self reference
					continue;
				}
			}

			ZSymContextMem *node = (ZSymContextMem *)currentDir->dir->getp( segs[i] );
			if( node ) {
				*(ZSymContextMem **)context = node;

				if( node->type == zsymLINK ) {
					// We have encounter a link so we stop processing and hand this link back to the caller
					*(char **)context = node->link;
					*err = zsymRESOLVE_LINK;
					return i+1;
				}
				else if( node->type == zsymDIR ) {
					currentDir = node;
				}
				else if( node->type == zsymFILE ) {
					if( i == segCount-1 ) {
						// The last segment is a file, all is good
						return i+1;
					}
					else {
						// Otherwise they are asking for a subdir of a file which is an error
						*err = zsymRESOLVE_FILE_NOT_DIR;
						return 0;
					}
				}
				else {
					// Corrupt
					assert( 0 );
				}
			}
			else if( action == zsymOPEN_OR_CREATE ) {
				// If the last segment is blank that means asking for a dir
				// otherwise it is assumed a file
				ZSymContextMem *newNode = 0;
				if( i == segCount-1 && !isDirLookup ) {
					// This is a file create because it is the last segment and it is not blank
					newNode = new ZSymContextMem( zsymFILE, currentDir );
				}
				else {
					// CREATE the directory node
					newNode = new ZSymContextMem( zsymDIR, currentDir );
				}
				currentDir->dir->putp( segs[i], newNode );
				currentDir = newNode;
				*(ZSymContextMem **)context = newNode;
			}
			else {
				// Symbol not found
				*err = zsymRESOLVE_NOT_FOUND;
				return i;
			}
		}

		return i + isDirLookup;
	}

	virtual void convertNodeType( void *context, int newNodeType, int *err, int safetyOverride ) {
		ZSymContextMem *node = (ZSymContextMem *)context;
		if( node ) {
			if( node->type == zsymDIR ) {
				if( newNodeType == zsymUNK ) {
				}
				else if( newNodeType == zsymFILE ) {
				}
				else if( newNodeType == zsymDIR ) {
				}
				else if( newNodeType == zsymLINK ) {
				}
				else {
					assert( 0 );
				}
			}
			else if( node->type == zsymFILE ) {
				if( newNodeType == zsymUNK ) {
				}
				else if( newNodeType == zsymFILE ) {
				}
				else if( newNodeType == zsymDIR ) {
				}
				else if( newNodeType == zsymLINK ) {
				}
				else {
					assert( 0 );
				}
			}
			else if( node->type == zsymLINK ) {
				if( newNodeType == zsymUNK ) {
				}
				else if( newNodeType == zsymFILE ) {
				}
				else if( newNodeType == zsymDIR ) {
				}
				else if( newNodeType == zsymLINK ) {
				}
				else {
					assert( 0 );
				}
			}
			else if( node->type == zsymUNK ) {
				if( newNodeType == zsymUNK ) {
				}
				else if( newNodeType == zsymFILE ) {
				}
				else if( newNodeType == zsymDIR ) {
				}
				else if( newNodeType == zsymLINK ) {
				}
				else {
					assert( 0 );
				}
			}
			else {
				assert( 0 );
			}
		}
	}

	virtual int bestConversion( int srcType ) {
		return srcType;
			// All types are supported in mem
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
			if( node->type == zsymFILE ) {
				*fileType = node->file->type;
			}
			*nodeType = node->type;
		}
	}

	virtual int rm( void *context, int *err, int safetyOverride ) {
		ZSymContextMem *node = (ZSymContextMem *)context;
		if( node ) {
			node->rm();
		}
		return 1;
	}

	virtual int createLink( void *context, char *target, int *err ) {
		ZSymContextMem *node = (ZSymContextMem *)context;
		if( node ) {
			node->createLink( target );
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
		assert( node->type == zsymFILE );
		switch( node->file->type ) {
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
		assert( node->type == zsymFILE || node->type == zsymUNK );
		node->clear();
		node->type = zsymFILE;
		node->file = new ZSymMemFile;
		node->file->type = fileType;
		switch( fileType ) {
			case zsymI:
				assert( rawLen == sizeof(int) );
				memcpy( &node->file->valI, rawPtr, rawLen );
				break;
			case zsymF:
				assert( rawLen == sizeof(float) );
				memcpy( &node->file->valF, rawPtr, rawLen );
				break;
			case zsymD:
				assert( rawLen == sizeof(float) );
				memcpy( &node->file->valD, rawPtr, rawLen );
				break;
			case zsymS:
				node->file->valS = (char *)malloc( rawLen + 2*sizeof(int) + 1 );
				((int *)node->file->valS)[0] = rawLen + 2*sizeof(int) + 1;
				((int *)node->file->valS)[1] = rawLen;
				memcpy( node->file->valS+2*sizeof(int), rawPtr, rawLen );
				*(node->file->valS+2*sizeof(int)+rawLen) = 0;
				break;
			default:
				break;
		}
	}

//	virtual int putI( void *context, int i, int *err ) {
//		ZSymContextMem *node = (ZSymContextMem *)context;
//		assert( node && node->type == zsymFILE );
//		node->file->putI( i );
//		*err = 0;
//		return 1;
//	}
//
//	virtual int getI( void *context, int *i, int *err ) {
//		ZSymContextMem *node = (ZSymContextMem *)context;
//		if( node && node->type == zsymFILE ) {
//			*i = node->file->getI();
//			*err = 0;
//			return 1;
//		}
//		else {
//			*err = 1;
//			*i = 0;
//			return 0;
//		}
//	}
//
//	virtual int putS( void *context, char *s, int len, int *err ) {
//		ZSymContextMem *node = (ZSymContextMem *)context;
//		if( node && node->type == zsymFILE ) {
//			node->file->putS( s, len );
//			*err = 0;
//			return 1;
//		}
//		*err = 1;
//		return 0;
//	}
//
//	virtual int getS( void *context, char **s, int *len, int *err ) {
//		ZSymContextMem *node = (ZSymContextMem *)context;
//		assert( node );
//		if( node->type == zsymFILE ) {
//			*s = node->file->getS( len );
//			*err = 0;
//			return 1;
//		}
//		else {
//			*s = 0;
//			*len = 0;
//			*err = 1;
//			return 0;
//		}
//	}

	virtual int cat( void *context, char *s, int len, int growBy, int *err ) {
		ZSymContextMem *node = (ZSymContextMem *)context;
		if( node && node->type == zsymFILE ) {
			node->file->catS( s, len, growBy );
			*err = 0;
			return 1;
		}
		*err = 1;
		return 0;
	}

	struct ZSymProtocolMemListContext {
		ZHashTable *dir;
		int i;
	};

	virtual void *listStart( void *context ) {
		ZSymContextMem *node = (ZSymContextMem *)context;
		if( node ) {
			if( node->type == zsymDIR ) {
				ZSymProtocolMemListContext *lc = new ZSymProtocolMemListContext;
				lc->dir = node->dir;
				lc->i = -1;
				return lc;
			}
		}
		return 0;
	}

	virtual int listNext( void *listContext, char **s, int *len, int *type ) {
		ZSymProtocolMemListContext *lc = (ZSymProtocolMemListContext *)listContext;
		if( !lc ) {
			return 0;
		}
		int size = lc->dir->size();

		char *key = 0;
		while( !key ) {
			lc->i++;
			if( lc->i >= size ) {
				delete lc;
				return 0;
			}
			key = lc->dir->getKey( lc->i );
		}
		*s = key;
		*len = strlen( key );
		*type = ((ZSymContextMem *)lc->dir->getValp(lc->i))->type;
		return 1;
	}

	virtual void getName( void *context, char **name, int *len ) {
		ZSymContextMem *node = (ZSymContextMem *)context;
		ZHashTable *parentDir = node->parent->dir;
		int size = parentDir->size();
		for( int i=0; i<size; i++ ) {
			ZSymContextMem *ni = (ZSymContextMem *)parentDir->getValp( i );
			if( ni == node ) {
				*name = parentDir->getKey( i );
				break;
			}
		}
	}

	virtual void dump( void *context, int toStdout, int _indent ) {
		ZSymContextMem *node = (ZSymContextMem *)context;

		char indent[128];
		memset( indent, ' ', _indent );
		indent[_indent] = 0;

		// FIND this symbol in the parent
		char *foundName = 0;
		if( node->parent ) {
			assert( node->parent->type == zsymDIR );
			int size = node->parent->dir->size();
			for( int i=0; i<size; i++ ) {
				if( node->parent->dir->getValp(i) == node ) {
					foundName = node->parent->dir->getKey( i );
					break;
				}
			}
		}
		printf( "%s%s\n", indent, foundName ? foundName : "<nnf>" );
		static char *typeStr[] = { "unk", "fil", "dir", "lnk" };
		static char *nodeTypeStr[] = { "nil", "int", "flt", "dbl", "str" };
		printf( "%snodePtr=%p parent=%p refCount=%d\n", indent, node, node->parent, node->refCount );
		printf( "%s%s", indent, typeStr[ node->type ] );
		if( node->type == zsymFILE ) {
			printf( " = (%s) ", nodeTypeStr[ node->file->type ] );
			if( node->file->type == zsymI ) {
				printf( "%d", node->file->getI() );
			}
			else if( node->file->type == zsymS ) {
				// @TODO: Use len and be smart since it might be binary
				int len = 0;
				char *bin = node->file->getS( &len );
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
			else if( node->file->type == zsymZ ) {
				printf( "<nil>" );
			}
			else {
				assert( 0 );
			}
		}
		else if( node->type == zsymDIR ) {
			printf( "%s with %d active records", indent, node->dir->activeCount() );
		}
		else if( node->type == zsymLINK ) {
			printf( "%s target: %s", indent, node->link );
		}
		printf( "\n" );
	}

};


//==================================================================================
// ZSymContextFile
// @TODO: Each protocol probably ought to live in its own file
//==================================================================================

struct ZSymContextFile {
	// @TODO: Rename ZSymContextMem to ZSymContext so that everything matches

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
	virtual int resolve( int segCount, char **segs, void **context, int *err, int action ) {
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

	virtual int bestConversion( int srcType ) {
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

	virtual int rm( void *context, int *err, int safetyOverride ) {
		ZSymContextFile *f = (ZSymContextFile *)context;
		if( f ) {
			// @TODO: Recursive remove
			assert( 0 );
		}
		return 1;
	}

	virtual int createLink( void *context, char *target, int *err ) {
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

//	virtual int putS( void *context, char *s, int len, int *err ) {
//		ZSymContextFile *f = (ZSymContextFile *)context;
//		if( f ) {
//			if( len == -1 ) {
//				len = strlen( s );
//			}
//			int ret = fwrite( s, len, 1, f->file );
//			if( ret == 1 ) {
//				fflush( f->file );
//				// TRUNCATE
//				_chsize( fileno(f->file), len );
//				*err = 0;
//				return 1;
//			}
//		}
//		*err = errno;
//		return 0;
//	}
//
//	virtual int getS( void *context, char **s, int *len, int *err ) {
//		ZSymContextFile *f = (ZSymContextFile *)context;
//		if( f ) {
//			fseek( f->file, 0, SEEK_END );
//			int _len = ftell( f->file );
//			if( len ) {
//				*len = _len;
//			}
//			fseek( f->file, 0, SEEK_SET );
//			f->lastRead = (char *)realloc( f->lastRead, _len+1 );
//			int ret = fread( f->lastRead, _len, 1, f->file );
//			if( ret == 1 ) {
//				f->lastRead[_len] = 0;
//				*s = f->lastRead;
//				*err = 0;
//				return 1;
//			}
//		}
//		*s = 0;
//		*err = errno;
//		return 0;
//	}
//
//	virtual int putI( void *context, int i, int *err ) {
//		return putS( context, (char *)&i, sizeof(int), err );
//	}
//
//	virtual int getI( void *context, int *i, int *err ) {
//		char *s;
//		int len;
//		int ret = getS( context, &s, &len, err );
//		if( ret ) {
//			// @TODO: Implement smart typing here. For now assume binary
//			*i = *(int *)s;
//		}
//		return ret;
//	}

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

	virtual int listNext( void *listContext, char **s, int *len, int *type ) {
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
				*type = isDir ? zsymDIR : zsymFILE;
				return 1;
			}
			else {
				delete lc->w;
				delete lc;
			}
		}
		return 0;
	}

	virtual void getName( void *context, char **name, int *len ) {	
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

ZSym::ZSym( char *pathFmt, ... ) {
	protocol = 0;
	context = 0;
	err = 0;
	temp = 0;
	safetyOverride = 0;
	char path[1024];
	zsymAssembleTo( path, pathFmt, 1024 );
	resolve( path );
}

ZSym::ZSym( int action, char *pathFmt, ... ) {
	protocol = 0;
	context = 0;
	err = 0;
	temp = 0;
	safetyOverride = 0;
	char path[1024];
	zsymAssembleTo( path, pathFmt, 1024 );
	resolve( path, action );
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

int ZSym::resolve( char *path, int action ) {
	if( *path == 0 && context && protocol ) {
		// This is a request for "this"
		return 1;
	}

	// LOOK for a protocol override
	ZSymProtocol *oldProtocol = protocol;
	void *oldContext = context;

	int p = 0;
	char *c = path;
	char *afterProtocol = path;
	for( ; *c; c++ ) {
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
	char *segSrc[zsymSEG_COUNT_MAX];

	char *last = afterProtocol;
	for( c = afterProtocol; ; c++ ) {
		if( c[0] == 0 || c[0] == '/' ) {
			// Found a slash, a root slash will create a blank segment
			int len = c-last; // -1 skips the slash
			segs[segCount] = (char *)alloca( len+1 );
			segSrc[segCount] = last;
			memcpy( segs[segCount], last, len );
			segs[segCount][len] = 0;
			segCount++;
			assert( segCount < zsymSEG_COUNT_MAX );
			last = c+1;
		}
		if( c[0] == 0 ) {
			break;
		}
	}

	// CALL the protocol's resolver
	int err=0, ret=0;
	int segsResolved = protocol->resolve( segCount, segs, &context, &err, action );
	if( err == 0 && segsResolved == segCount ) {
		// Everything resolved perfectly
		ret = 1;
	}
	else if( err == -1 ) {
		// Resolved as a link, expand and recurse
		char *target = (char *)context;
		int targetLen = strlen( target );
		int remainLen = segsResolved < segCount ? strlen( segSrc[segsResolved] ) : 0;
		char *expanded = (char *)alloca( targetLen + remainLen + 1 );
		memcpy( expanded, target, targetLen );
		// @TODO trim extra slashes according to some rule
		if( remainLen > 0 ) {
			strcpy( &expanded[targetLen], segSrc[segsResolved] );
		}
		expanded[targetLen+remainLen] = 0;
		context = 0;
		ret = resolve( expanded, action );
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

int ZSym::cd( char *fmt, ... ) {
	char path[1024];
	zsymAssembleTo(path,fmt,1024);
	ZSym temp = *this;
	int ret = temp.resolve( path, zsymOPEN_OR_FAIL );
	if( ret ) {
		*this = temp;
	}
	return ret;
}

int ZSym::cd( int action, char *fmt, ... ) {
	char path[1024];
	zsymAssembleTo(path,fmt,1024);
	ZSym temp = *this;
	int ret = temp.resolve( path, action );
	if( ret ) {
		*this = temp;
	}
	return ret;
}

int ZSym::rm( char *path ) {
	if( !path ) path = "";
	ZSym temp = *this;
	if( temp.resolve( path, zsymOPEN_OR_FAIL ) ) {
		temp.protocol->rm( temp.context, &err, safetyOverride );
		safetyOverride = 0;
		if( err == 0 ) {
			return 1;
		}
	}
	return 0;
}

int ZSym::link( char *linkName, char *target ) {
	if( resolve( linkName, zsymOPEN_OR_CREATE ) ) {
		return protocol->createLink( context, target, &err );
	}
	return 0;
}

int ZSym::linkTo( char *target ) {
	assert( protocol && context );
	int ret = protocol->createLink( context, target, &err );
	if( ret ) {
		// RE-RESOLVE by passing empty string now that this is a link
		return resolve( "", zsymOPEN_OR_FAIL );
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
			}
			break;

		case zsymI:
			switch( dstType ) {
				case zsymZ:
					assert( 0 );
					break;

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
	}
}

// Copy put
//----------------------------------------------------------

void ZSym::put( ZSym &o ) {
	if( protocol && context ) {
		int srcNodeType=0, srcFileType = 0;
		o.protocol->getType( o.context, &srcNodeType, &srcFileType );

		int dstNodeType=0, dstFileType = 0;
		protocol->getType( context, &dstNodeType, &dstFileType );

		if( dstNodeType == zsymDIR ) {
			protocol->rm( context, &err, safetyOverride );
			safetyOverride = 0;
		}

		// Either the src is a dir or a file
		// Which ever it is, THIS sym has to become that
		// If it is a dir then we also need to recurse

		assert( dstNodeType == srcNodeType );

		if( dstNodeType == zsymFILE ) {
			char *src;
			int srcLen;
			o.protocol->getRaw( o.context, &src, &srcLen, &err );

			char *_temp = 0;
			int _tempLen = 0;

			dstFileType = protocol->bestConversion( srcFileType );
			if( dstFileType != srcFileType ) {
				convert( srcFileType, src, srcLen, dstFileType, &_temp, &_tempLen );
				src = _temp;
				srcLen = _tempLen;
			}
			protocol->putRaw( context, dstFileType, src, srcLen, &err );
			if( _temp ) {
				free( _temp );
			}
		}
		else if( dstNodeType == zsymDIR ) {
/*
			int count = 0;
			for( ZSymDirListRecurse l(o); l.next(); count++ ) {
				int srcNodeType=0, srcFileType=0;
				l.zsym().protocol->getType( l.zsym().context, &srcNodeType, &srcFileType );

				char *n = l.name();

				if( srcNodeType == zsymDIR ) {
					// This is tricky I have to go into the dir locally
					// @TODO
	//				assert( 0 );
				}
				else {
					char *src;
					int srcLen;
					l.zsym().protocol->getRaw( l.zsym().context, &src, &srcLen, &err );

					char *_temp = 0;
					int _tempLen = 0;

					dstFileType = protocol->bestConversion( srcFileType );
					if( dstFileType != srcFileType ) {
						convert( srcFileType, src, srcLen, dstFileType, &_temp, &_tempLen );
						src = _temp;
						srcLen = _tempLen;
					}

					ZSym fileSym = currentDir;
					fileSym.resolve( n, zsymOPEN_OR_CREATE );

					fileSym.protocol->putRaw( fileSym.context, dstFileType, src, srcLen, &err );
					if( _temp ) {
						free( _temp );
					}
				}
			}
*/
		}
	}
}

// Int
//----------------------------------------------------------

void ZSym::putI( int i ) {
	if( protocol && context ) {
		int nodeType=0, fileType = 0;
		protocol->getType( context, &nodeType, &fileType );

		if( nodeType == zsymDIR ) {
zsymDump();
			protocol->convertNodeType( context, zsymFILE, &err, safetyOverride );
zsymDump();
			safetyOverride = 0;
		}

		char *_temp = 0;
		int _tempLen = 0;
		int dstType = protocol->bestConversion( zsymI );
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
	}
}

void ZSym::putI( char *path, int i ) {
	if( !context ) {
		resolve( path, zsymOPEN_OR_CREATE );
		putI( i );
	}
	else {
		ZSym temp = *this;
		temp.resolve( path, zsymOPEN_OR_CREATE );
		temp.putI( i );
	}
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
			char *src;
			int srcLen;
			protocol->getRaw( context, &src, &srcLen, &err );
			if( fileType != zsymI ) {
				convert( fileType, src, srcLen, zsymI, &temp, &tempLen );
				src = temp;
				assert( tempLen == sizeof(int) );
			}
			return *(int *)src;
		}
	}
	err = -1;
	return 0;
}

int ZSym::getI( char *path, int onEmpty ) {
	ZSym temp( *this );
	if( temp.resolve( path, zsymOPEN_OR_FAIL ) ) {
		int ret = temp.getI();
		if( err != -1 ) {
			return ret;
		}
	}
	
	// Not found or not a file, etc.
	return onEmpty;
}

// Str
//----------------------------------------------------------

void ZSym::putS( char *src, int srcLen ) {
	if( protocol && context ) {
		int nodeType=0, fileType = 0;
		protocol->getType( context, &nodeType, &fileType );

		if( nodeType == zsymDIR ) {
			protocol->rm( context, &err, safetyOverride );
			safetyOverride = 0;
		}

		if( srcLen == -1 ) {
			srcLen = strlen( src ) + 1;
		}
		char *_temp = 0;
		int _tempLen = 0;
		int dstType = protocol->bestConversion( zsymS );
		if( dstType != zsymS ) {
			convert( zsymS, src, srcLen, dstType, &_temp, &_tempLen );
			src = _temp;
			srcLen = _tempLen;
		}
		protocol->putRaw( context, dstType, src, srcLen, &err );
		if( _temp ) {
			free( _temp );
		}
	}
}

void ZSym::putS( char *path, char *s, int len ) {
	if( !context ) {
		resolve( path, zsymOPEN_OR_CREATE );
		putS( s, len );
	}
	else {
		ZSym temp = *this;
		temp.resolve( path, zsymOPEN_OR_CREATE );
		temp.putS( s, len );
	}
}

char *ZSym::getS( int *len ) {
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
			char *src;
			int srcLen;
			protocol->getRaw( context, &src, &srcLen, &err );
			if( fileType != zsymS ) {
				convert( fileType, src, srcLen, zsymS, &temp, &tempLen );
				src = temp;
				srcLen = tempLen;
			}
			if( len ) {
				*len = srcLen;
			}
			return src;
		}
	}
	err = -1;
	return "";
}

char *ZSym::getS( char *path, char *onEmpty, int *len ) {
	ZSym temp( *this );
	if( temp.resolve( path, zsymOPEN_OR_FAIL ) ) {
		char *ret = temp.getS( len );
		if( err != -1 ) {
			return ret;
		}
	}
	
	// Not found or not a file, etc.
	if( len ) {
		*len = strlen( onEmpty );
	}
	return onEmpty;
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

int ZSym::len( char *path ) {
	ZSym temp( *this );
	if( temp.resolve( path, zsymOPEN_OR_FAIL ) ) {
		return temp.len();
	}
	return 0;
}

// Cat
//----------------------------------------------------------

void ZSym::catC( char c, int growBy ) {
	if( isValid() ) {
		int nodeType, fileType;
		protocol->getType( context, &nodeType, &fileType );
		assert( nodeType == zsymFILE && fileType == zsymS );
			// @TODO: Eventuall cat should be smart enough to do type casting like put
		protocol->cat( context, &c, 1, growBy, &err );
	}
}

void ZSym::catC( char *path, char c, int growBy ) {
	if( !context ) {
		resolve( path, zsymOPEN_OR_CREATE );
		catC( c, growBy );
	}
	else {
		ZSym temp = *this;
		temp.resolve( path, zsymOPEN_OR_CREATE );
		temp.catC( c, growBy );
	}
}

void ZSym::catI( int i, int growBy ) {
	if( isValid() ) {
		int nodeType, fileType;
		protocol->getType( context, &nodeType, &fileType );
		assert( nodeType == zsymFILE && (fileType == zsymS || fileType == zsymZ) );
			// @TODO: Eventuall cat should be smart enough to do type casting like put
		protocol->cat( context, (char *)&i, sizeof(int), growBy, &err );
	}
}

void ZSym::catI( char *path, int i, int growBy ) {
	if( !context ) {
		resolve( path, zsymOPEN_OR_CREATE );
		catI( i, growBy );
	}
	else {
		ZSym temp = *this;
		temp.resolve( path, zsymOPEN_OR_CREATE );
		temp.catI( i, growBy );
	}
}

void ZSym::catS( char *s, int len, int growBy ) {
	if( isValid() ) {
		int nodeType, fileType;
		protocol->getType( context, &nodeType, &fileType );
		assert( nodeType == zsymFILE && fileType == zsymS );
			// @TODO: Eventuall cat should be smart enough to do type casting like put
		protocol->cat( context, s, len, growBy, &err );
	}
}

void ZSym::catS( char *path, char *s, int len, int growBy ) {
	if( !context ) {
		resolve( path, zsymOPEN_OR_CREATE );
		catS( s, len, growBy );
	}
	else {
		ZSym temp = *this;
		temp.resolve( path, zsymOPEN_OR_CREATE );
		temp.catS( s, len, growBy );
	}
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

ZSymLocal::ZSymLocal( char *dir ) {
	if( !dir ) {
		dir = "/";
	}
	serial++;
	localSerial = serial;
	char buf[32];
	sprintf( buf, "%s__temp%d/", dir, localSerial );
	resolve( buf );
}

ZSymLocal::~ZSymLocal() {
	char buf[32];
	sprintf( buf, "/__temp%d/", localSerial );
	rm( buf );
}

//==================================================================================
// ZSymDirList
//==================================================================================

ZSymDirList::ZSymDirList( char *pathFmt, ... ) {
	listingContext = 0;
	char path[1024];
	zsymAssembleTo( path, pathFmt, 1024 );
	dirSym.resolve( path );
	_name = 0;
	if( dirSym.isFile() ) {
		_name = strdup( path );
	}
	if( dirSym.protocol ) {
		listingContext = dirSym.protocol->listStart( dirSym.context );
	}
}

ZSymDirList::ZSymDirList( ZSym &sym ) {
	dirSym = sym;
	listingContext = 0;
	if( dirSym.protocol ) {
		listingContext = dirSym.protocol->listStart( dirSym.context );
	}
}

int ZSymDirList::next() {
	if( dirSym.protocol ) {
		int len = 0;
		if( dirSym.isFile() ) {
			if( dirSym.err == 0 ) {
				dirSym.protocol->getName( dirSym.context, &_name, &len );
				currentSym = dirSym;
				dirSym.err = -1;
				return 1;
			}
			return 0;
		}

		int ret = dirSym.protocol->listNext( listingContext, &_name, &len, &_type );
		if( ret ) {
			currentSym = dirSym;
			currentSym.resolve( _name, zsymOPEN_OR_FAIL );
			return 1;
		}
	}
	return 0;
}

//==================================================================================
// ZSymDirListRecurse
//==================================================================================

ZSymDirListRecurse::ZSymDirListRecurse( char *pathFmt, ... ) {
	char path[1024];
	zsymAssembleTo( path, pathFmt, 1024 );
	stackTop = 0;
	stack[stackTop] = new ZSymDirList( path );
	action = 0;
}

ZSymDirListRecurse::ZSymDirListRecurse( int _action, char *pathFmt, ... ) {
	char path[1024];
	zsymAssembleTo( path, pathFmt, 1024 );
	stackTop = 0;
	stack[stackTop] = new ZSymDirList( path );
	action = _action;
}

ZSymDirListRecurse::ZSymDirListRecurse( ZSym &sym ) {
	stackTop = 0;
	stack[stackTop] = new ZSymDirList( sym );
}

ZSymDirListRecurse::~ZSymDirListRecurse() {
	for( int i=0; i<=stackTop; i++ ) {
		delete stack[i];
	}
}

int ZSymDirListRecurse::next() {
	if( !(action==zsymIGNORE_LINKS && type()==zsymLINK) && stack[stackTop]->zsym().isDir() ) {
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

char *ZSymDirListRecurse::name() {
	tempName[0] = 0;
	for( int i=0; i<=stackTop; i++ ) {
		strcat( tempName, stack[i]->name() );
		if( stack[i]->zsym().isDir() ) {
			strcat( tempName, "/" );
		}
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
	printf( "ROOT: %p refCount=%d allocCount=%d activeCount=%d\n", ZSymContextMem::root, ZSymContextMem::root ? ZSymContextMem::root->refCount : 0, ZSymContextMem::allocCount, ZSymContextMem::root->dir->activeCount() );
	printf( "\n" );
	for( ZSymDirListRecurse l(zsymIGNORE_LINKS,symbol); l.next(); ) {
		int depth = l.depth();
		char indent[128];
		memset( indent, ' ', depth*2 );
		indent[depth*2] = 0;
		printf( "%s%s, ", indent, l.name() );
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
#pragma message( "BUILDING ZHASHTABLE_SELF_TEST" )

#include "zconsole.h"

void test() {
	int i;

	// TEST ref counting
	assert( ZSymContextMem::allocCount == 0 );
	{
		ZSym a( "/a" );
		a.putI( 10 );
		a.rm();
		assert( ZSymContextMem::orphanCount == 1 );
	}
	assert( ZSymContextMem::orphanCount == 0 );
	assert( ZSymContextMem::allocCount == 1 );
		// 1 because the root is still allocated

	// TEST switching symbol
	{
		ZSym a( "/a/" );
		a.putI( "1", 1 );
		assert( ZSym("/a/1").getI() == 1 );
		assert( a.getI("1") == 1 );
		a.putI( "2", 2 );
		assert( ZSym("/a/1").getI() == 1 );
		assert( ZSym("/a/2").getI() == 2 );
		assert( a.getI("2") == 2 );
		a.rm();
	}
	assert( ZSymContextFile::allocCount == 0 );
	{
		ZSym a( "file:///transfer/test/a" );
		a.putS( "test" );
	}
	assert( ZSymContextFile::allocCount == 0 );

	// TEST orphan
	ZSym a1( "/a" );
	a1.putI( 10 );
	ZSym a2( "/a" );
	assert( a2.getI() == 10 );
	a1.rm();
	assert( ! a1.isValid() );
	assert( ! a2.isValid() );

	// TEST nums, strings, dirs, links
	// /num10 = 10
	// /strA = "A"
	// /strBIN = B \x0 I \x0 N
	// /dir1/num100 = 100
	// /dir1/strAA = "AA"
	// /dir1/dir2/num1000 = 1000
	// /dir1/dir2/strAAA = "AAA"
	// /linkAAA -> /dir1/dir2/strAAA
	// /linkDir2 -> /dir1/dir2/
	// /linkToLink -> /linkDir2/

	ZSym( "/num10" ).putI( 10 );
	ZSym( "/strA" ).putS( "A" );
	
	char bin[] = { 'B', 0, 'I', 0, 'N' };
	ZSym( "/strBIN" ).putS( bin, sizeof(bin) );

	ZSym( "/dir1/num100" ).putI( 100 );
	ZSym( "/dir1/strAA" ).putS( "AA" );
	ZSym( "/dir1/dir2/num1000" ).putI( 1000 );
	ZSym( "/dir1/dir2/strAAA" ).putS( "AAA" );
	ZSym( "/linkAAA" ).linkTo( "/dir1/dir2/strAAA" );
	ZSym( "/linkDir2" ).linkTo( "/dir1/dir2/" );
	ZSym( "/linkToLink" ).linkTo( "/linkDir2/" );

	assert( ZSym( "/dir1/num100" ).getI() == 100 );
	assert( !strcmp( ZSym( "/dir1/strAA" ).getS(), "AA" ) );
	assert( ZSym( "/dir1/dir2/num1000" ).getI() == 1000 );
	assert( !strcmp( ZSym( "/dir1/dir2/strAAA" ).getS(), "AAA" ) );

	assert( !strcmp( ZSym( "/linkAAA" ).getS(), "AAA" ) );
	assert( ZSym( "/linkDir2/num1000" ).getI() == 1000 );
	assert( ZSym( "/linkDir2/" ).getI("num1000") == 1000 );
	assert( ZSym( "/linkToLink/num1000" ).getI() == 1000 );
	assert( ZSym( "/linkToLink/" ).getI("num1000") == 1000 );

	// TEST root
	ZSym root( "/" );
	assert( root.getI("num10") == 10 );
	assert( root.getI("dir1/dir2/num1000") == 1000 );

	// TEST nonexistant
	assert( root.getI( "nonexistant", -1 ) == -1 );

	// TEST cd
	root.cd( "dir1/" );
	assert( root.getI("num100") == 100 );
	assert( root.getI("notexist",-1) == -1 );
	root.cd( "/dir1/num100" );
	assert( root.getI() == 100  );

	// TEST relative paths
	root.cd( "/" );
	assert( root.getI("num10") == 10 );
	root.cd( "/dir1/" );
	assert( root.getI("../num10") == 10 );
	root.cd( "/dir1/dir2/" );
	assert( root.getI("../../num10") == 10 );
	assert( root.getI("./num1000") == 1000 );
	assert( root.getI("/dir1/../num10") == 10 );
	root.cd( "../" );
	assert( root.getI("dir2/num1000") == 1000 );

	ZSym a( "/" );
	assert( a.getI( "dir1/num100" ) == 100 );
	a.cd( "dir1/" );
	assert( a.getI( "num100" ) == 100 );

	// TEST file system

	// file:///transfer/test/filea = FILEA
	// file:///transfer/test/filenum10 = 10
	// file:///transfer/test/dir1/fileaa = FILEAA
	// /linkFile -> file:///transfer/test/
	ZSym( "file:///transfer/test/filea" ).putS( "FILEA" );
	ZSym( "file:///transfer/test/dir1/fileaa" ).putS( "FILEAA" );
	assert( !strcmp( ZSym("file:///transfer/test/filea").getS(), "FILEA" ) );
	assert( !strcmp( ZSym("file:///transfer/test/dir1/fileaa").getS(), "FILEAA" ) );
	ZSym( "file:///transfer/test/filenum10" ).putI( 10 );
	assert( ZSym( "file:///transfer/test/filenum10" ).getI() == 10 );
	ZSym f( "file:///transfer/test/" );
	assert( !strcmp( f.getS( "filea" ), "FILEA" ) );
	assert( !strcmp( f.getS( "dir1/fileaa" ), "FILEAA" ) );
	f.cd( "dir1/" );
	assert( !strcmp( f.getS( "fileaa" ), "FILEAA" ) );

	// TEST link to file
	ZSym( "/linkFile" ).linkTo( "file:///transfer/test/" );
	ZSym( "/linkFile/" ).putS( "a", "FILEA" );
	assert( !strcmp( ZSym( "/linkFile/a" ).getS(), "FILEA" ) );

	// TEST rm with some temporaries
	ZSym( "/temp/num10" ).putI( 10 );
	ZSym( "/temp/dir1/num10" ).putI( 10 );
	ZSym( "/temp/dir1/dir2/num10" ).putI( 10 );
	ZSym( "/temp/dir1/dir2/num20" ).putI( 20 );
	assert( ZSym( "/temp/num10" ).getI() == 10 );
	assert( ZSym( "/temp/dir1/num10" ).getI() == 10 );
	assert( ZSym( "/temp/dir1/dir2/num10" ).getI() == 10 );
	assert( ZSym( "/temp/dir1/dir2/num20" ).getI() == 20 );
	ZSym r( "/temp/dir1/dir2/" );
	r.rm( "num20" );
	assert( r.getI( "num20", -1 ) == -1 );

	// TEST fail action on construct
	ZSym fail( zsymOPEN_OR_FAIL, "/nonexistant" );
	assert( fail.getI() == 0 );
	assert( ! fail.isValid() );

	// TEST a bad conversion of a file into a dir
	assert( ZSym( "/dir1/num100" ).getI() == 100 );
	ZSym t( "/dir1/num100/badsubdir" );
	assert( ! t.isValid() );
	assert( t.getI() != 100 );

	// TEST that treating a directory like a file just gives back empties
	// Note that this can be confusing to someone used to a shell
	// In this syntax a dir must be qualified as a dir.  It doesn't
	// look at the actual type of the symbol to decide
	assert( ZSym( "/dir1/" ).isValid() );
	ZSym d( "/dir" );
	assert( d.getI( "num100", -1 ) == -1 );
	assert( ZSym( "/dir1" ).getI() == 0 );
	assert( ZSym( "/dir1/num100" ).getI() == 100 );

	// TEST that a put to an invalid should fail silently
	t.putI( 10 );
	assert( t.getI() != 10 );

	// TEST remove of a link... DONT RECURSE!
	ZSym l( "/linktest/dir1/" );
	l.putI( "file", 10 );
	ZSym l1( "/linktestlink" );
	l1.linkTo( "/linktest/dir1/" );
	assert( l1.getI( "file" ) == 10 );
	l1.rm();
	assert( ! l1.isValid() );
	assert( l1.getI( "file" ) == 0 );
	assert( ZSym("/linktest/dir1/file").getI() == 10 );
	assert( ZSym(zsymOPEN_OR_FAIL,"/linktestlink").isValid() == 0 );
	ZSym("/linktest/").rm();
	assert( ZSym(zsymOPEN_OR_FAIL,"/linktest/").isValid() == 0 );

	// TEST local
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

	// TEST listing
	for( i=0; i<10; i++ ) {
		ZSym( "/listtest/%d", i ).putI( i );
	}
	char found[10] = { 0, };
	for( ZSymDirList list( "/listtest/" ); list.next(); ) {
		int i = list.zsym().getI();
		assert( i >= 0 && i < 10 );
		found[i]++;
	}
	for( i=0; i<10; i++ ) {
		assert( found[i] );
	}
	ZSym( "/listtest/" ).rm();
	i = 0;
	for( ZSymDirList list1( "/listtest/" ); list1.next(); ) {
		i++;
	}
	assert( i == 0 );

	// TEST listing file
	for( i=0; i<10; i++ ) {
		ZSym( "file:///transfer/test/listtest/%d", i ).putI( i );
	}
	memset( found, 0, 10 );
	for( ZSymDirList list2( "file:///transfer/test/listtest/" ); list2.next(); ) {
		int i = list2.zsym().getI();
		assert( i >= 0 && i < 10 );
		found[i]++;
	}
	for( i=0; i<10; i++ ) {
		assert( found[i] );
	}

	// TEST that listing includes a single file
	{
		int foundSingleFile = 0;
		ZSym( "/file" ).putI( 100 );
		for( ZSymDirList l2( "/file" ); l2.next(); ) {
			foundSingleFile++;
			assert( l2.zsym().getI() == 100 );
		}
		assert( foundSingleFile );
	}


	// TEST copying zsyms and typecasts
	ZSym copy1( "/copy1" );
	copy1.putI( 100 );
	ZSym copy2( "/copy2" );
	copy2.put( copy1 );
	assert( copy2.getI() == 100 );

	// TEST blowing away a dir with a file
	{
		ZSym( "/dirtokill/a" ).putI( 1 );
		ZSym( "/dirtokill/b" ).putI( 2 );
		ZSym( "/dirtokill" ).putI( 2 );
		// For mem this is OK, for file this requires safety override
zsymDump();
		assert( ZSym( "/dirtokill" ).getI() == 2 );
	}

	// TEST copying a whole dir
	{
		ZSym( "/dirtocopy/a" ).putI( 1 );
		ZSym( "/dirtocopy/b" ).putI( 2 );
		ZSym( "/dirtocopy/dir/a" ).putI( 3 );
		ZSym f( "/dirtocopy/dir/b" );
		f.putI( 4 );
		ZSym copy( "/dirtodest" );
		copy.put( f );
		assert( ZSym("/dirtodest/a").getI() == 1 );
	}

	// @TODO: Test every possible conversion!
//	ZSym numstr( "/numstr" );
//	numstr.putS( "123" );
//	assert( ZSym( "/numstr" ).getI() == 100 );


	// @TODO: Implement and test file rm
//	ZSym( "file:///transfer/test/listtest/" ).rm();
//	i = 0;
//	for( ZSymDirList list3( "file:///transfer/test/listtest/" ); list3.next(); ) {
//		i++;
//	}
//	assert( i == 0 );

	// @TODO: Test recursive listing

	// @TODO: TEST circular link
	// @TODO: Test link orphans
}

void main() {
	zconsolePositionAt( 0, 0, 800, 1200 );
	test();
}

#endif

