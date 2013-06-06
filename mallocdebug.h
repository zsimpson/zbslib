#ifndef MALLOCDEBUG_H
#define MALLOCDEBUG_H

#include "stdio.h"

// The "mallocdebug" modules are used to find memory leaks and memory corruptions
// They work by replaceing malloc, free, realloc, new, and delete with
// custom heaps.
// There are two versions:
//   mallocdebug_linear 
//     The mallocdebug_linear version is not really a heap at all,
//     it simply never frees any real memory so that heap corrupters
//     can easily be found that write over free memory.
//     Obviously this takes an enormous amount of memory and
//     slows things down greatly but it is very handy for finding corruptions
//   mallocdebug_heap
//     This module is a true heap so it doesn't take up as much memory
//     but has substantially better free checking than do typical heaps
//
// Both modules also do stack tracing to monitor who is allocating
// On every allocation, the stack is unwound and code pointers
// are stored in the heap.  When heapDump is called, the debug symbols
// are extracted from the exe and a nice readable dump is produced.
//   In order for this stack tracing functionality to work however,
// the project must be compiled with COFF (or BOTH) symbols. Also,
// you must add the linker option /force:multiple to override the
// duplicate symbols in the stdlib.

int heapCheck( int assertOnFailure );
	// Checks if the heap is OK.

int heapGetMark();
	// Returns a pointer (offset) of memory currently allocated
	// this is useful when heapDumping so that you can measure the
	// differential heap usage

void heapDump( int includeFree, FILE *file, int memoryMark=0, int binDumpBytes=64 );
	// Dump out nice stack traced information about the heap to file
	// The memoryMark is from heapGetMark() or 0 for all of heap
	// The trace will include a binary dump of the heap data up to binDumpBytes bytes

void heapStatus( int &_allocCount, int &_allocBytes );
	// Return the number of allocations and the total bytes

#endif