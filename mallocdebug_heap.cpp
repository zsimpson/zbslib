// (c) 2000 by Zachary Booth Simpson
// This code may be freely used, modified, and distributed without any license
// as long as the original author is noted in the source file and all
// changes are made clear disclaimer using a "MODIFIED VERSION" disclaimer.
// There is NO warranty and the original author may not be held liable for any damages.
// http://www.totempole.net

#include "windows.h"
#include "stdlib.h"
#include "assert.h"
#include "stdio.h"
#include "zstacktrace.h"

#ifndef min
	#define min(a,b) ( ( (a) < (b) ) ? (a) : (b) )
#endif
#ifndef max
	#define max(a,b) ( ( (a) > (b) ) ? (a) : (b) )
#endif


// This yet another alternative heap.
// This one uses a true heap structure unlike mallocdebug_linear
// and thus is reasonably fast.  It still has better
// heap checking and monitoring than does the
// standard clib heap and can be used in cases where
// you want to hunt down memory leaks without a huge penalty
//
// Under MSDEV, linking this module will cause a linker error
// concerning duplicate defined symbols.  To supress this error under
// MSDEV, use /force:multiple as a linker option.  Unfortunatly,
// this means that you will not be able to use incremental linking.


#pragma comment(linker,"/force:multiple")


#define ghSTACK_COUNT   (16)
	// How many ips of the stack are stored for each malloc

#define ghPOST_PADDING (16)
	// The pading in bytes added to the end of the blocks

#define ghALLOC_SIZE   (0x4096*500)
	// How big each GlobalAlloc block will be.
	 // This may be changed arbitrarily

#define ghMAGIC_NUMBER (0xDEADBEEF)
	// Padded into the dwords before the allocated block
	 // This may be changed arbitrarily

#define ghFREE_CHAR    (0xfe)
	// Stuffed into free blocks
	 // This may be changed arbitrarily

#define gfALLOCED_CHAR (0xCF)
	// Allocated blocks are initialized with this
	 // This may be changed arbitrarily

#define ghGLOBAL_HEAPS (500)
	 // How many GlobalAlloc calls are allowed (pointers stored statically)
	 // This may be changed arbitrarily

struct GlobalHeap {
	int heapSize;
	char *heapPtr;
	struct HeapBlock *headFree;
	struct HeapBlock *headAlloc;
};

int heapCheckOnEachAlloc = 0;
int heapCheckFreeScan = 0;

int everAllocedRequested = 0;
int everAllocedActual = 0;
int nowAllocedRequested = 0;
int nowAllocedActual = 0;
int everAllocedCount = 0;
int nowAllocedCount = 0;

int everFreedRequested = 0;
int everFreedActual = 0;
int everFreedCount = 0;


int numGlobalHeaps = 0;
GlobalHeap globalHeaps[ghGLOBAL_HEAPS];
HANDLE hMutex = NULL;
HANDLE heap = NULL;


struct HeapBlock {
	HeapBlock *next;
	HeapBlock *prev;
	int markId;
	int stack[ghSTACK_COUNT];
	int heap;
		// Which of the global heaps is this in
	int flags;
		// Lowest bit means free
	size_t allocSize;
		// The requested size
	size_t actualSize;
		// The actual size including this header and all extra space added
	int prePad[4];
	char data[4*4];
		// This is really an array that allocSize + 16 bytes for post magic numbers

	void setAsFree( int _actualSize, int _heap ) {
		next = 0;
		prev = 0;
		memset( stack, 0, sizeof(stack) );
		heap = _heap;
		flags = 1;
		prePad[0] = ghMAGIC_NUMBER;
		prePad[1] = ghMAGIC_NUMBER;
		prePad[2] = ghMAGIC_NUMBER;
		prePad[3] = ghMAGIC_NUMBER;
		allocSize = _actualSize - sizeof(HeapBlock);
		actualSize = _actualSize;
		if( heapCheckFreeScan ) {
			memset( data, ghFREE_CHAR, _actualSize - sizeof(HeapBlock) );
		}
		*(int *)&data[allocSize +  0] = ghMAGIC_NUMBER;
		*(int *)&data[allocSize +  4] = ghMAGIC_NUMBER;
		*(int *)&data[allocSize +  8] = ghMAGIC_NUMBER;
		*(int *)&data[allocSize + 12] = ghMAGIC_NUMBER;
	}

	void setAsAllocated( int _allocSize, int _actualSize, int _heap ) {
		markId = everAllocedCount;
		allocSize = _allocSize;
		actualSize = _actualSize;
		flags = 0;
		heap = _heap;
		memset( data, gfALLOCED_CHAR, allocSize );
		*(int *)&data[ allocSize +  0 ] = ghMAGIC_NUMBER;
		*(int *)&data[ allocSize +  4 ] = ghMAGIC_NUMBER;
		*(int *)&data[ allocSize +  8 ] = ghMAGIC_NUMBER;
		*(int *)&data[ allocSize + 12 ] = ghMAGIC_NUMBER;
	}

};


_CRTIMP void * __cdecl malloc( size_t nSize ) {
	// GRAB the call stack
	unsigned int ips[ghSTACK_COUNT] = {0,};
//	zStackTrace( ghSTACK_COUNT, ips, NULL );

	// INIT a heap if first call
	if( !heap ) {
		heap = HeapCreate( 0, 0, 0 );
	}

	// MUTEX
	void *ptr = NULL;
	if( !hMutex ) {
		hMutex = CreateMutex( NULL, FALSE, "MallocMutex" );
		assert( hMutex );
	}
	while( WaitForSingleObject( hMutex, 1000 ) != WAIT_OBJECT_0 );
	__try {

	if( heapCheckOnEachAlloc ) {
		int heapCheck( int assertOnFailure );
		heapCheck( 1 );
	}

	// COMPUTE actual size needed, including blocksize and rounded to 16 bytes
	size_t actualAllocSize = max( 16, nSize );
	size_t remainder = actualAllocSize & 0x0F;
	actualAllocSize = nSize + (remainder ? 16 - remainder : 0) + sizeof(HeapBlock);

	// Traverse free blocks looking for something big enough
	size_t smallestFound = 0xFFFFFFFF;
	HeapBlock *smallest = 0;
	int smallestWhichHeap;
	for( int i=0; i<numGlobalHeaps; i++ ) {
		for( HeapBlock *b = globalHeaps[i].headFree; b; b=b->next ) {
			if( b->actualSize >= actualAllocSize && b->allocSize < smallestFound ) {
				smallest = b;
				smallestFound = b->allocSize;
				smallestWhichHeap = i;

				if( b->actualSize >= actualAllocSize ) {
					break;
				}
			}
		}
	}

	if( !smallest ) {
		// CREATE a new heap
		assert( numGlobalHeaps < ghGLOBAL_HEAPS );
		int toAlloc = max( actualAllocSize, ghALLOC_SIZE );
		globalHeaps[numGlobalHeaps].heapPtr = (char *)HeapAlloc( heap, 0, toAlloc );

		HeapBlock *head = (HeapBlock *)globalHeaps[numGlobalHeaps].heapPtr;
		head->setAsFree( toAlloc, numGlobalHeaps );

		globalHeaps[numGlobalHeaps].headFree = head;
		globalHeaps[numGlobalHeaps].headAlloc = 0;
		globalHeaps[numGlobalHeaps].heapSize = toAlloc;

		smallest = head;
		smallestWhichHeap = numGlobalHeaps;

		numGlobalHeaps++;
	}

	// UNLINK from the free chain
	if( smallest->prev ) {
		smallest->prev->next = smallest->next;
	}
	if( smallest->next ) {
		smallest->next->prev = smallest->prev;
	}
	if( smallest == globalHeaps[smallestWhichHeap].headFree ) {
		globalHeaps[smallestWhichHeap].headFree = smallest->next;
	}

	// BREAK into two blocks, depending on the size
	if( smallest->actualSize - actualAllocSize >= sizeof(HeapBlock)+64 ) {
		HeapBlock *secondBlock = (HeapBlock *)( (char *)smallest + actualAllocSize );
		secondBlock->setAsFree( smallest->actualSize - actualAllocSize, smallest->heap );

		if( globalHeaps[smallestWhichHeap].headFree ) {
			globalHeaps[smallestWhichHeap].headFree->prev = secondBlock;
		}
		secondBlock->next = globalHeaps[smallestWhichHeap].headFree;
		secondBlock->prev = 0;
		globalHeaps[smallestWhichHeap].headFree = secondBlock;
	}
	else {
		actualAllocSize = smallest->actualSize;
	}

	// LINK into the allocated chain
	smallest->prev = 0;
	smallest->next = globalHeaps[smallestWhichHeap].headAlloc;
	if( globalHeaps[smallestWhichHeap].headAlloc ) {
		globalHeaps[smallestWhichHeap].headAlloc->prev = smallest;
	}
	globalHeaps[smallestWhichHeap].headAlloc = smallest;

	smallest->setAsAllocated( nSize, actualAllocSize, smallestWhichHeap );

	// COPY the stack trace
	memcpy( smallest->stack, ips, ghSTACK_COUNT*4 );

	ptr = (void *)&smallest->data[0];

	everAllocedRequested += nSize;
	everAllocedActual += actualAllocSize;
	nowAllocedRequested += nSize;
	nowAllocedActual += actualAllocSize;
	everAllocedCount++;
	nowAllocedCount++;

	}
	__finally {
		ReleaseMutex(hMutex);
	}
	return ptr;
}

_CRTIMP void __cdecl free( void * pUserData ) {
	if( !pUserData ) {
		return;
	}
	if( !hMutex ) {
		hMutex = CreateMutex( NULL, FALSE, "MallocMutex" );
		assert( hMutex );
	}
	while( WaitForSingleObject( hMutex, 1000 ) != WAIT_OBJECT_0 );
	__try {


	// Check the magic numbers
	HeapBlock *block = (HeapBlock *)( (char *)pUserData - sizeof(HeapBlock) + ghPOST_PADDING );

	int size = block->allocSize;
	int actualSize = block->actualSize;

	assert( block->flags == 0 );
	assert( block->prePad[0] == ghMAGIC_NUMBER );
	assert( block->prePad[1] == ghMAGIC_NUMBER );
	assert( block->prePad[2] == ghMAGIC_NUMBER );
	assert( block->prePad[3] == ghMAGIC_NUMBER );

	assert( *(int *)&block->data[ block->allocSize +  0 ] == ghMAGIC_NUMBER );
	assert( *(int *)&block->data[ block->allocSize +  4 ] == ghMAGIC_NUMBER );
	assert( *(int *)&block->data[ block->allocSize +  8 ] == ghMAGIC_NUMBER );
	assert( *(int *)&block->data[ block->allocSize + 12 ] == ghMAGIC_NUMBER );
		// If any of these fail, something over-wrote the gap between mallocs

	// UNLINK from alloc chain
	if( block->prev ) {
		block->prev->next = block->next;
	}
	if( block->next ) {
		block->next->prev = block->prev;
	}
	if( globalHeaps[block->heap].headAlloc == block ) {
		globalHeaps[block->heap].headAlloc = block->next;
	}

	// SEE if the next door neighbors are free, if
	// so we can accumulate them into this block
	// @TODO
	// This requires maintaining another thread for the physical position

	// LINK into the free chain
	block->setAsFree( block->actualSize, block->heap );
	if( globalHeaps[block->heap].headFree ) {
		globalHeaps[block->heap].headFree->prev = block;
	}
	block->next = globalHeaps[block->heap].headFree;
	block->prev = 0;
	globalHeaps[block->heap].headFree = block;

	nowAllocedRequested -= size;
	nowAllocedActual -= actualSize;
	nowAllocedCount--;
	everFreedRequested += size;
	everFreedActual += actualSize;
	everFreedCount++;

	}
	__finally {
		ReleaseMutex(hMutex);
	}
}

_CRTIMP void * __cdecl realloc( void * pUserData, size_t nNewSize ) {
	char *ptr = 0;

	if( !hMutex ) {
		hMutex = CreateMutex( NULL, FALSE, "MallocMutex" );
		assert( hMutex );
	}
	while( WaitForSingleObject( hMutex, 1000 ) != WAIT_OBJECT_0 );
	__try {

	ptr = (char *)malloc( nNewSize );
	if( pUserData ) {
		HeapBlock *block = (HeapBlock *)( (char *)pUserData - sizeof(HeapBlock) + ghPOST_PADDING );
		assert( nNewSize >= block->allocSize );
			// You can't realloc a smaller block, can you?  Check in the clib source
		assert( block->flags == 0 );
			// You can't realloc a freed block
		memcpy( ptr, pUserData, block->allocSize );
		free( pUserData );
	}

	}
	__finally {
		ReleaseMutex(hMutex);
	}

	return ptr;
}


_CRTIMP char *  __cdecl strdup(const char *a) {
	char *ptr = 0;

	if( !hMutex ) {
		hMutex = CreateMutex( NULL, FALSE, "MallocMutex" );
		assert( hMutex );
	}
	while( WaitForSingleObject( hMutex, 1000 ) != WAIT_OBJECT_0 );
	__try {

	if( !a ) return 0;
	int len = strlen(a);
	ptr = (char *)malloc( len+1 );
	strcpy( ptr, a );

	}
	__finally {
		ReleaseMutex(hMutex);
	}

	return ptr;
}

void *operator new( size_t size ) {
	return malloc(size);
}

void *operator new( unsigned int cb, int nBlockUse, const char * szFileName, int nLine ) {
	return malloc( cb );
	// This is an alternate version of new that the iostream seems to use
	//return _nh_malloc_dbg( cb, 1, nBlockUse, szFileName, nLine );
}

void operator delete( void *pUserData ) {
	free(pUserData);
}

////////////////////////////////////////////////////////////////////
// Returns negative on error.
int heapCheck( int assertOnFailure ) {
	int err = 0;

	if( !hMutex ) {
		hMutex = CreateMutex( NULL, FALSE, "MallocMutex" );
		assert( hMutex );
	}
	while( WaitForSingleObject( hMutex, 1000 ) != WAIT_OBJECT_0 );
	__try {

	static int reentry = 0;
	assert( reentry == 0 );
	reentry++;

	int a = 0;

	int _nowAllocedRequested = nowAllocedRequested;
	int _nowAllocedActual = nowAllocedActual;
	int _nowAllocedCount = nowAllocedCount;
	for( int i=0; i<numGlobalHeaps; i++ ) {
		// First traverse through linearly
		// Then traverse though the alloc and freed threads
		// The sums should all be the same

		int freeSize = 0;
		int freeActual = 0;
		int freeCount = 0;
		int allocSize = 0;
		int allocActual = 0;
		int allocCount = 0;

		HeapBlock *p = (HeapBlock *)globalHeaps[i].heapPtr;
		HeapBlock *end = (HeapBlock *)( globalHeaps[i].heapPtr + globalHeaps[i].heapSize );
		while( p < end ) {
			if( p->heap != i ) { err=-1; goto error; }
			if( p->prePad[0] != ghMAGIC_NUMBER ) { err=-2; goto error; }
			if( p->prePad[1] != ghMAGIC_NUMBER ) { err=-3; goto error; }
			if( p->prePad[2] != ghMAGIC_NUMBER ) { err=-4; goto error; }
			if( p->prePad[3] != ghMAGIC_NUMBER ) { err=-5; goto error; }

			if( p->flags ) {
				// This is a free block
				freeSize += p->allocSize;
				freeActual += p->actualSize;
				freeCount++;

				if( heapCheckFreeScan ) {
					int _size = p->actualSize - sizeof(HeapBlock);
					for( char *fill = (char *)p->data; _size; _size--, fill++ ) {
						if( *fill != (char)ghFREE_CHAR ) {
							err=-6; goto error;
						}
							// Someone wrote to a freed buffer
					}
				}

			}
			else {
				_nowAllocedRequested -= p->allocSize;
				_nowAllocedActual -= p->actualSize;
				_nowAllocedCount--;

				allocSize += p->allocSize;
				allocActual += p->actualSize;
				allocCount++;
			}

			if( *(int *)&p->data[p->allocSize +  0] != ghMAGIC_NUMBER ) {
				err=-7; goto error;
			}
			if( *(int *)&p->data[p->allocSize +  4] != ghMAGIC_NUMBER ) {
				err=-8; goto error;
			}
			if( *(int *)&p->data[p->allocSize +  8] != ghMAGIC_NUMBER ) {
				err=-9; goto error;
			}
			if( *(int *)&p->data[p->allocSize + 12] != ghMAGIC_NUMBER ) {
				err=-10; goto error;
			}

			p = (HeapBlock *)( (char *)p + p->actualSize );
		}

		// Now go back through the threads and double check that the links are OK
		for( p=globalHeaps[i].headFree; p; p=p->next ) {
			if( p->flags != 1 ) { err=-11; goto error; }
			freeSize -= p->allocSize;
			freeActual -= p->actualSize;
			freeCount--;
		}
		for( p=globalHeaps[i].headAlloc; p; p=p->next ) {
			if( p->flags != 0 ) { err=-12; goto error; }
			allocSize -= p->allocSize;
			allocActual -= p->actualSize;
			allocCount--;
		}

		if( freeSize != 0 ) { err=-13; goto error; }
		if( freeActual != 0 ) { err=-14; goto error; }
		if( freeCount != 0 ) { err=-15; goto error; }
		if( allocSize != 0 ) { err=-16; goto error; }
		if( allocActual != 0 ) { err=-17; goto error; }
		if( allocCount != 0 ) { err=-18; goto error; }
	}

	if( _nowAllocedRequested != 0 )  { err=-19; goto error; }
	if( _nowAllocedActual != 0 )  { err=-20; goto error; }
	if( _nowAllocedCount != 0 )  { err=-21; goto error; }

	reentry--;
	return 1;

	error:

	if( assertOnFailure ) {
		assert( 0 );
	}
	reentry--;

	}
	__finally {
		ReleaseMutex(hMutex);
	}
	return err;
}

int heapGetMark() {
	return everAllocedCount;
}

void heapDump( int includeFree, FILE *file, int memoryMark, int binDumpBytes ) {
	assert( !includeFree );
		// You can't do this with this kind of heap (since frees are not really kept)

	zStackTraceBuildSymbolTable();
	int ret = zStackTraceOpenFiles();
	assert( ret );
	char dstBuf[1024];
	heapCheck( 1 );
	int i;
	try {

		fprintf( file, "SUMMARY\n" );

		fprintf( file, "  %010d everAllocedRequested\n", everAllocedRequested );
		fprintf( file, "  %010d everAllocedActual\n", everAllocedActual );
		fprintf( file, "  %010d nowAllocedRequested\n", nowAllocedRequested );
		fprintf( file, "  %010d nowAllocedActual\n", nowAllocedActual );
		fprintf( file, "  %010d everAllocedCount\n", everAllocedCount );
		fprintf( file, "  %010d nowAllocedCount\n", nowAllocedCount );
		fprintf( file, "  %010d everFreedRequested\n", everFreedRequested );
		fprintf( file, "  %010d everFreedActual\n", everFreedActual );
		fprintf( file, "  %010d everFreedCount\n", everFreedCount );
		fprintf( file, "  %010d numGlobalHeaps\n", numGlobalHeaps );

		fprintf( file, "GLOBAL HEAPS:\n", numGlobalHeaps );
		for( i=0; i<numGlobalHeaps; i++ ) {
			fprintf( file, "  %02d size=%08d heapPtr=%p headFree=%p deadAlloc=%p\n", i );
		}

		for( i=0; i<numGlobalHeaps; i++ ) {
			for( HeapBlock *p = globalHeaps[i].headAlloc; p; p=p->next ) {
			
				if( p->markId >= memoryMark ) {
					char *str = zStackTraceGetString( ghSTACK_COUNT-1, (unsigned int *)(p->stack), NULL );
						// Grab the stack trace, -1 because we want to skip the malloc call

					// CONVERT the str to add indentations for better readability
					char *dst = dstBuf;
					char *dstEnd = dstBuf + 1024 - 1;
					*dst++ = ' ';
					*dst++ = ' ';
					assert( str );
						// If this assert fails it is because you need to link with COFF debug to get the trace

					for( unsigned char *src = (unsigned char *)str; *src; src++ ) {
						if( *src > 127 || *src < 0x0a ) {
							*src = '?';
						}
						*dst++ = *src;
						if( *src == '\n' && *(src+1)!=0 && *(src+2)!=0 ) {
							*dst++ = ' ';
							*dst++ = ' ';
						}
						assert( dst < dstEnd );
					}
					*dst = 0;

					fprintf( file, "%p: size=%08X, alloc#=%08X %s\n", p, p->allocSize, p->markId, p->flags?"FREE":"" );

					unsigned int dumpSize = min( (unsigned)p->allocSize, (unsigned)binDumpBytes );
					unsigned char *pp = (unsigned char *)p->data;
					for( unsigned int i=0; i<dumpSize; i++ ) {
						int lineSize = min( dumpSize, 32 );
						for( int j=0; j<lineSize; j++ ) {	
							fprintf( file, "%02X ", pp[i+j] );
							if( j == 15 ) {
								fprintf( file, " " );
							}
						}
						fprintf( file, "  " );
						for( j=0; j<lineSize; j++ ) {	
							fprintf( file, "%c", pp[i+j] >= 32 && pp[i+j] < 127 ? pp[i+j] : '.' );
							if( j == 15 ) {
								fprintf( file, " " );
							}
						}
						fprintf( file, "\n" );
						i += lineSize;
					}

					fprintf( file, "%s", dstBuf );

					fflush(file);
				}
			}
		}
	} catch( ... ) {
		int a = 1;
	}
	zStackTraceCloseFiles();
}

void heapStatus( int &_allocCount, int &_allocBytes ) {
	_allocCount = nowAllocedCount;
	_allocBytes = nowAllocedRequested;
}


// SELF-TEST
////////////////////////////////////////////////////////////////////////////////////////////


//#define SELF_TEST
#ifdef SELF_TEST
#include "stdio.h"
#include "stdlib.h"

#ifdef WIN32
	#define ASSERTFUNC _CRTIMP void __cdecl _assert( void *msg, void *file, unsigned line )
#else
	#define ASSERTFUNC __dead void __assert __P((const char *msg,const char *file,int line)) 
#endif

ASSERTFUNC {
	printf( "Test failed: %s %s %d\n", msg, file, line );
}

#define NUM_TESTS (5000)
char *ptrsM[NUM_TESTS];
char *ptrsF[NUM_TESTS];
size_t sizes[NUM_TESTS];


void testSizes() {
	for( int i=0; i<NUM_TESTS; i++ ) {
		if( ptrsM[i] ) {
			HeapBlock *p = (HeapBlock *)( ptrsM[i] - sizeof(HeapBlock) + ghPOST_PADDING );
			if( sizes[i] != p->allocSize ) {
				assert( 0 );
			}
		}
	}
}


void main( int argc, char **argv ) {
	heapCheckFreeScan = 0;

	memset( ptrsM, 0, sizeof(char *) * NUM_TESTS );
	memset( ptrsF, 0, sizeof(char *) * NUM_TESTS );
	memset( sizes, 0, sizeof(char *) * NUM_TESTS );

	assert( heapCheck(0) > 0 );

	// Test that new and delete work
	char *n = new char[10];
	assert( *n == (char)gfALLOCED_CHAR );

	assert( heapCheck(0) > 0 );

	delete n;
	//assert( *n == (char)ghFREE_CHAR );

	assert( heapCheck(0) > 0 );

	for( int i=0; i<NUM_TESTS; i++ ) {
		printf( "%d of %d. nowalloc=%d nowalloccount=%d everfreecount=%d \n", i+1, NUM_TESTS, nowAllocedRequested, nowAllocedCount, everFreedCount );

		if( i==NUM_TESTS-1 ) {
			heapCheckFreeScan = 1;
		}
		testSizes();

		int size = rand();
		if( !(rand() % 100) ) {
			// Occasionally allocated a huge block
			size += ghALLOC_SIZE;
		}

		sizes[i] = size;
		ptrsM[i] = (char *)malloc( size );
		memset( ptrsM[i], 0xFF, size );
		
//		assert( heapCheck(0) > 0 );

		if( !(rand() % 3) && i ) {
			// Free a random block
			int which = rand() % i;
			if( ptrsM[which] ) {
				free( ptrsM[which] );
				ptrsF[which] = ptrsM[which];
				ptrsM[which] = NULL;
//				assert( heapCheck(0) > 0 );
			}
		}

		if( !(rand() % 8) ) {
			// Realloc a random block
			int which = rand() % i;
			if( ptrsM[which] ) {
				sizes[which] += rand();
				ptrsM[which] = (char *)realloc( ptrsM[which], sizes[which] );
//				assert( heapCheck(0) > 0 );
			}
		}

		if( !(rand() % 8) ) {
			// Deliberately corrupt the heap
			int which = rand() % i;
			
			if( ptrsM[which] ) {
				// Write before
				int *a = (int *)ptrsM[which];
				int w = (rand() % 4) + 1;
				int t = *(a-w);
				*(a-w) = 0;
//				assert( heapCheck(0) < 0 );
				*(a-w) = t;
//				assert( heapCheck(0) > 0 );
			}
			else {
				// Write into freed space
				int *a = (int *)ptrsF[which];
				int w = (rand() % 4) + 1;
				int t = *(a-w);
				*(a-w) = 0;
//				assert( heapCheck(0) < 0 );
				*(a-w) = t;
//				assert( heapCheck(0) > 0 );
			}
		}
	}
}
#endif

