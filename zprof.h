// @ZBS {
//		*MODULE_OWNER_NAME zprof
// }

// This is the nth (I've lost count) attempt at a good interactive profiler
// Features:
//    * Simple to mark blocks of code using static strings as both the label and the index
//      This is very nice but requires that the static strings be put into a writable segment
//      so the header includes a pragma for this
//    * Allows for both heirarchical and non but a nanming convention
//
// I want to be able to see the percent of the whole frame
// Should be able to save out a reference time and compare to that
// I want to see all the things which are up and down vs the reference (last frame or last time)


#ifndef ZPROF_H
#define ZPROF_H

#include "memory.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Public interface
//

#ifdef WIN32
unsigned __int64 zprofTick();
#endif

#if defined(ZPROF) && defined(WIN32)
	#define zprofBeg(s) ZProf::beg( "\x0\x0\x0\x0" ## #s )
		// Call this to mark the beginning of a block like: zprofBeg(start block)
		// Note that you do NOT use quotes as this macro stringizes it and 
		// also prepends four extra spaces to the string so that it can be used 
		// as an index to avoid any extra lookups.
		// Begin the string with an underscore if this node
		// is a root node, otherwise the assumption is that this
		// node is a child of the last call on the stack

	#define zprofEnd() ZProf::end()
		// Call this note the end of a block of code

	#define zprofScope( s ) ZProfScope __##s; ZProf::beg( "\x0\x0\x0\x0" ## #s )
		// Use this to create a scoped profile which auto destructs on scope exit

	#define zprofReset( which ) ZProf::reset( which )
		// Resets accumulator (-1 resets both)

	#define zprofSortTree() ZProf::sortTree()
		// Sorts the tree for good printing

	#define zprofDumpToFile( file, rootName ) ZProf::dumpToFile( file, rootName )
		// Write the profile data out to the file
		// This function now calls sort

	#define zprofLoadReference( file ) ZProf::loadReference( file )
		// Load the reference file.  Note that only profile points that have already
		// been loaded can load the reference data.

	#define zprofAddLine( line ) ZProf::addLine( line )
		// Appends an information line to the profile display

	#pragma section( "thomas", read, write )
	#pragma const_seg("thomas") 
		// This pragma makes sure that the constants are put into a writable
		// chunk of memory.  This allows us to modify the constant stings to
		// pointers to the profile records without having to do any extra lookups
#else
	// If you don't define the ZPROF macro, then everything is stubbed out
	#define zprofBeg( s )
	#define zprofEnd()
	#define zprofScope( s )
	#define zprofReset( which )
	#define zprofSortTree()
	#define zprofDumpToFile( file, rootName ) 
	#define zprofLoadReference( file ) 
	#define zprofAddLine( line )
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Private interface
//

#if defined(ZPROF) && defined(WIN32)

#include "assert.h"

struct ZProf {
	static char *extraLines[64];
	static int extraLineCount;

	static ZProf *heapHead;
	ZProf *heapNext;

	#define ZPROF_STACK_SIZE (256)
	static ZProf *stack[ZPROF_STACK_SIZE];
	static int stackTop;

	int recursionCount;
	ZProf *parent;
	ZProf *chldHead;
	ZProf *siblNext;
		// The prev two are setup by the sortIntoHeirarchy()

	char *identString;
	unsigned __int64 startTick;
	unsigned __int64 accumTick[2];
	unsigned __int64 count[2];
	unsigned __int64 refTick[2];

	static ZProf *allocRec() {
		// @TODO: Change this to a custom heap?
		ZProf *rec = new ZProf;
		memset( rec, 0, sizeof(ZProf) );
		rec->heapNext = heapHead;
		rec->recursionCount = 0;
		heapHead = rec;
		return rec;
	}

	static void beg( char *ptr ) {
		ZProf **rec = (ZProf **)ptr;
		if( ! *rec ) {
			*rec = ZProf::allocRec();
			char *id = (*rec)->identString = ptr + sizeof(ZProf **);
			if( *id != '_' && stackTop > 0 ) {
				(*rec)->parent = stack[stackTop-1];
			}
		}

		stack[stackTop++] = *rec;
		if( (*rec)->recursionCount == 0 ) {
			assert( stackTop < ZPROF_STACK_SIZE );
			unsigned __int64 tick;
			__asm {
				// RDTSC
				_emit 0x0f
				_emit 0x31
				lea ecx, tick
				mov dword ptr [ecx], eax
				mov dword ptr [ecx+4], edx
			}
			(*rec)->startTick = tick;
		}
		(*rec)->recursionCount++;
	}

	static void end() {
		ZProf *rec = stack[--stackTop];
		rec->recursionCount--;
		if( rec->recursionCount == 0 ) {
			unsigned __int64 tick;
			__asm {
				// RDTSC
				_emit 0x0f
				_emit 0x31
				lea ecx, tick
				mov dword ptr [ecx], eax
				mov dword ptr [ecx+4], edx
			}
			assert( stackTop >= 0 );
			rec->count[0]++;
			rec->count[1]++;
			unsigned __int64 delta = tick - rec->startTick;
			rec->accumTick[0] += delta;
			rec->accumTick[1] += delta;
		}
	}

	static void dumpToFile( char *file, char *rootName );
	static void loadReference( char *file );
	static void sortTree();
	static void reset( int which=-1 );
		// which tells which of the accumulator buffers to clear
		// -1 clears them both
	static void addLine( char *line );
};

struct ZProfScope {
	~ZProfScope() { zprofEnd(); }
};

#endif

#endif // ZPROF_H
