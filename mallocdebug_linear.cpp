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


// This is a simple and robust debug heap.
// In fact, it isn't a heap at all, no memory is truley freed;
// rather, free blocks are merely marked as such and then
// filled with a magic character which is checked
// during every call to heapCheck.
//
// This heap will catch all of the following:
// 1. Double free's (check's at free time)
// 2. Writes to free blocks (checks during heapCheck)
// 3. Over or underwrites that whack the 16 bytes before an alloc
//
// Under MSDEV, linking this module will cause a linker error
// concerning duplicate defined symbols.  To supress this error under
// MSDEV, use /force:multiple as a linker option.  Unfortunatly,
// this means that you will not be able to use incremental linking.
//
// Obviously, this heap is slower and more importantly, consumes
// a huge amount of real memory, but it is very good at finding
// heap corruptors.

#pragma comment(linker,"/force:multiple")


#define ghSTACK_COUNT   (16)
	// How many ips of the stack are stored for each malloc

#define ghPADDING      (0x20)
	// Spacing before each allocated block sandwiching the stack trace
	// This is not an arbitrary constant.  Do not change without updating code

#define ghALLOC_SIZE   (0x4096*500)
	// How big each GlobalAlloc block will be.
	 // This may be changed arbitrarily

#define ghMAGIC_NUMBER (0xDEADBEEF)
	// Padded into the dwords before the allocated block
	 // This may be changed arbitrarily

#define ghFREE_CHAR    (0xff)
	// Stuffed into free blocks
	 // This may be changed arbitrarily

#define gfALLOCED_CHAR (0xCF)
	// Allocated blocks are initialized with this
	 // This may be changed arbitrarily

#define ghGLOBAL_HEAPS (500)
	 // How many GlobalAlloc calls are allowed (pointers stored statically)
	 // This may be changed arbitrarily

struct GlobalHeap {
	char *heapPtr;
	size_t size;
	size_t remaining;
};

int heapCheckOnEachAlloc = 0;
int totalAlloced = 0;
int totalFreed = 0;
int topGlobalHeap = -1;
GlobalHeap globalHeap[ghGLOBAL_HEAPS];
HANDLE hMutex = NULL;

#define MAX_ALLOCS 1000000
void *allocPtrs[MAX_ALLOCS];
unsigned int allocSizes[MAX_ALLOCS];
unsigned int allocCount = 0;
unsigned int freeCount = 0;

HANDLE heap = NULL;

_CRTIMP void * __cdecl malloc( size_t nSize ) {
	unsigned int ips[ghSTACK_COUNT];
	zStackTrace( ghSTACK_COUNT, ips, NULL );

	if( !heap ) {
		heap = HeapCreate( 0, 0, 0 );
	}

	int *ptr = NULL;
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

	nSize = max( 1, nSize );
	nSize += ghPADDING + ghSTACK_COUNT*4;
	if(
		topGlobalHeap == -1 || 
		nSize > globalHeap[topGlobalHeap].remaining
	) {
		topGlobalHeap++;
		assert( topGlobalHeap < ghGLOBAL_HEAPS );

		int toAlloc = max( ghALLOC_SIZE, nSize );
		globalHeap[topGlobalHeap].heapPtr = (char *)HeapAlloc( heap, 0, toAlloc );
		assert( globalHeap[topGlobalHeap].heapPtr );
		globalHeap[topGlobalHeap].size = toAlloc;
		globalHeap[topGlobalHeap].remaining = toAlloc;
	}
	ptr = (int *)(globalHeap[topGlobalHeap].heapPtr + (
		globalHeap[topGlobalHeap].size - globalHeap[topGlobalHeap].remaining
	));
	globalHeap[topGlobalHeap].remaining -= nSize;
	*ptr++ = ghMAGIC_NUMBER;
	*ptr++ = ghMAGIC_NUMBER;
	*ptr++ = ghMAGIC_NUMBER;
	*ptr++ = ghMAGIC_NUMBER;
	// Copy over the stack trace
	memcpy( ptr, ips, ghSTACK_COUNT*4 );
	ptr += ghSTACK_COUNT;
	*ptr++ = ghMAGIC_NUMBER;
	*ptr++ = ghMAGIC_NUMBER;
	*ptr++ = ghMAGIC_NUMBER;
	*ptr++ = nSize;

	totalAlloced += nSize;
	memset( ptr, gfALLOCED_CHAR, nSize-(ghPADDING+ghSTACK_COUNT*4) );

	allocPtrs[allocCount] = ptr;
	allocSizes[allocCount] = nSize;
	allocCount++;
	assert( allocCount < MAX_ALLOCS );

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


	int *p = (int *)pUserData;
	int size = *--p;
	assert( !( size & 0x80000000 ) );
		// Multiple free's
	*p |= 0x80000000;
	//--p;
	assert( *--p == ghMAGIC_NUMBER );
	assert( *--p == ghMAGIC_NUMBER );
	assert( *--p == ghMAGIC_NUMBER );
	p -= ghSTACK_COUNT;
	assert( *--p == ghMAGIC_NUMBER );
	assert( *--p == ghMAGIC_NUMBER );
	assert( *--p == ghMAGIC_NUMBER );
	assert( *--p == ghMAGIC_NUMBER );
		// If any of these fail, something over-wrote the gap between mallocs
	
	freeCount++;
	totalFreed += size;
	totalAlloced -= size;
	size -= ghPADDING + ghSTACK_COUNT*4;
	memset( pUserData, (char)ghFREE_CHAR, size );

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
		int *sizePtr = (int *)pUserData;
		int size = *--sizePtr;
		assert( !(size & 0x80000000) );
			// Reallocing a freed block
		memcpy( ptr, pUserData, size );
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
	int ret = 0;

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
	int err = 0;
	int _totalAlloced = 0;
	int _totalFreed   = 0;
	for( int i=0; i<=topGlobalHeap; i++ ) {
		char *p   = globalHeap[i].heapPtr;
		char *end = globalHeap[i].heapPtr + globalHeap[i].size - globalHeap[i].remaining;
		while( p < end ) {
			int *ptr = (int *)p;
			if( *ptr++ != ghMAGIC_NUMBER ) {
				err=-1; goto error;
			}
			if( *ptr++ != ghMAGIC_NUMBER ) { err=-2; goto error; }
			if( *ptr++ != ghMAGIC_NUMBER ) { err=-3; goto error; }
			if( *ptr++ != ghMAGIC_NUMBER ) { err=-4; goto error; }
			ptr += ghSTACK_COUNT;
				// Skip over the stack trace
			if( *ptr++ != ghMAGIC_NUMBER ) { err=-5; goto error; }
			if( *ptr++ != ghMAGIC_NUMBER ) { err=-6; goto error; }
			if( *ptr++ != ghMAGIC_NUMBER ) { err=-7; goto error; }
				// If any of these failed, someone overwrote the gap
			int size = *ptr++;

			assert( allocPtrs[a] == ptr );
			assert( allocSizes[a] == (unsigned)size || allocSizes[a] == unsigned(size & (~0x80000000)) );
			a++;

			if( size & 0x80000000 ) {
				size &= ~0x80000000;
				if( size < ghPADDING ) { err=-8; goto error; }
					// Corrupted heap
				_totalFreed += size;
				int _size = size - (ghPADDING+ghSTACK_COUNT*4);
#if 0
				for( char *fill = (char *)ptr; _size; _size--, fill++ ) {
					if( *fill != (char)ghFREE_CHAR ) {
						err=-9; goto error;
					}
						// Someone wrote to a freed buffer
				}
#endif
			}
			else {
				if( !(size >= ghPADDING) ) { err=-10; goto error; }
				_totalAlloced += size;
			}
			p += size;
		}
	}
	if( !(totalAlloced == _totalAlloced) ) { err=-11; goto error; }
	if( !(totalFreed   == _totalFreed) ) { err=-12; goto error; }
	reentry--;
	ret = 1;
	goto end;

	error:

	if( assertOnFailure ) {
		assert( 0 );
	}
	reentry--;
	ret = err;

	end:;

	}
	__finally {
		ReleaseMutex(hMutex);
	}

	return ret;

}

int heapGetMark() {
	return allocCount;
}

void heapDump( int includeFree, FILE *file, int memoryMark, int binDumpBytes ) {
	zStackTraceBuildSymbolTable();
	int ret = zStackTraceOpenFiles();
	assert( ret );
	char dstBuf[1024];
	heapCheck( 1 );
	try {
		int counter = 0;
		for( int i=0; i<=topGlobalHeap; i++ ) {
			char *p   = globalHeap[i].heapPtr;
			char *end = globalHeap[i].heapPtr + globalHeap[i].size - globalHeap[i].remaining;
			
			while( p < end ) {
				unsigned int *ptr = (unsigned int *)p;
				ptr += 4;
					// Skip of first filling gap
				char *str = NULL;
				if( counter >= memoryMark ) {
					str = zStackTraceGetString( ghSTACK_COUNT-1, (unsigned int *)(ptr+1), NULL );
						// Grab the stack trace, -1 because we want to skip the malloc call
				}
				ptr += ghSTACK_COUNT;
					// Skip over the stack trace
				ptr += 3;
					// Skip of filling gap
				unsigned int size = *ptr++;

				if( counter>=memoryMark && (includeFree || !(size & 0x80000000)) ) {
					// CONVERT the str to add indentations for better readability
					char *dst = dstBuf;
					char *dstEnd = dstBuf + 1024 - 1;
					*dst++ = ' ';
					*dst++ = ' ';
					assert( str );
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

					fprintf( file, "%p: size=%08X, alloc#=%08X %s\n", ptr, (size-(ghPADDING+ghSTACK_COUNT*4))&~0x80000000, counter, size&0x80000000?"FREE":"" );

					unsigned int dumpSize = min( (size-(ghPADDING+ghSTACK_COUNT*4))&~0x80000000, (unsigned)binDumpBytes );
					unsigned char *pp = (unsigned char *)ptr;
					for( unsigned int i=0; i<dumpSize; i++ ) {
						int lineSize = min( dumpSize, 32 );
						int j;
						for( j=0; j<lineSize; j++ ) {	
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
				size &= ~0x80000000;
				p += size;
				counter++;
			}
		}
	} catch( ... ) {
		int a = 1;
	}
	zStackTraceCloseFiles();
}

void heapStatus( int &_allocCount, int &_allocBytes ) {
	_allocCount = allocCount;
	_allocBytes = totalAlloced;
}

void heapStatusEx( int &_allocCount, int &_allocBytes, int &_freeCount, int &_freeBytes ) {
	_allocCount = allocCount;
	_allocBytes = totalAlloced;
	_freeCount  = freeCount;
	_freeBytes  = totalFreed;
}


// SELF-TEST
////////////////////////////////////////////////////////////////////////////////////////////


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

void main( int argc, char **argv ) {

	#define NUM_TESTS (200)

	char *ptrsM[NUM_TESTS];
	char *ptrsF[NUM_TESTS];
	int sizes[NUM_TESTS];
	memset( ptrsM, 0, sizeof(char *) * NUM_TESTS );
	memset( ptrsF, 0, sizeof(char *) * NUM_TESTS );
	memset( sizes, 0, sizeof(char *) * NUM_TESTS );

	// Test that new and delete work
	char *n = new char[10];
	assert( *n == (char)gfALLOCED_CHAR );

	delete n;
	assert( *n == (char)ghFREE_CHAR );

	for( int i=0; i<NUM_TESTS; i++ ) {
		printf( "%d of %d. %d allocated. %d free\n", i+1, NUM_TESTS, totalAlloced, totalFreed );

		int size = rand();
		if( !(rand() % 100) ) {
			// Occasionally allocated a huge block
			size += ghALLOC_SIZE;
		}

		sizes[i] = size;
		ptrsM[i] = (char *)malloc( size );
		memset( ptrsM[i], 0xFF, size );
		
		assert( heapCheck() );

		if( !(rand() % 4) ) {
			// Free a random block
			int which = rand() % i;
			if( ptrsM[which] ) {
				free( ptrsM[which] );
				ptrsF[which] = ptrsM[which];
				ptrsM[which] = NULL;
				assert( heapCheck() );
			}
		}

		if( !(rand() % 8) ) {
			// Realloc a random block
			int which = rand() % i;
			if( ptrsM[which] ) {
				ptrsM[which] = (char *)realloc( ptrsM[which], sizes[which]+rand() );
				assert( heapCheck() );
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
				assert( !heapCheck() );
				*(a-w) = t;
				assert( heapCheck() );
			}
			else {
				// Write into freed space
				int *a = (int *)ptrsF[which];
				int w = (rand() % 4) + 1;
				int t = *(a-w);
				*(a-w) = 0;
				assert( !heapCheck() );
				*(a-w) = t;
				assert( heapCheck() );
			}
		}
	}
}
#endif

