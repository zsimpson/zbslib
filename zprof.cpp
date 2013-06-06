// @ZBS {
//		*MODULE_NAME zprof
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Interactive Profiler
//		}
//		*PORTABILITY win32
//		*REQUIRED_FILES zprof.cpp zprof.h
//		*VERSION 1.0
//		+HISTORY {
//			1.0 19 July 2005 Created
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH yes
// }

#if defined(ZPROF) && defined(WIN32)

#include "zprof.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

ZProf *ZProf::heapHead = 0;
ZProf *ZProf::stack[ZPROF_STACK_SIZE];
int ZProf::stackTop = 0;
char *ZProf::extraLines[64];
int ZProf::extraLineCount = 0;

void ZProf::addLine( char *line ) {
	if( extraLineCount < sizeof(extraLines)/sizeof(extraLines[0]) ) {
		extraLines[extraLineCount] = strdup( line );
		extraLineCount++;
	}
}

void ZProf::sortTree() {
	ZProf *i;

	// CLEAR all
	for( i = ZProf::heapHead; i; i=i->heapNext ) {
		i->siblNext = 0;
		i->chldHead = 0;
	}

	for( i = ZProf::heapHead; i; i=i->heapNext ) {
		// FIND the children of this record
		for( ZProf *j = ZProf::heapHead; j; j=j->heapNext ) {
			if( j->parent == i ) {
				j->siblNext = i->chldHead;
				i->chldHead = j;
			}
		}
	}
}

void ZProf::reset( int which ) {
	for( int i=0; i<extraLineCount; i++ ) {
		free( extraLines[i] );
		extraLines[i] = 0;
	}
	extraLineCount = 0;

	if( which == -1 ) {
		for( ZProf *i = ZProf::heapHead; i; i=i->heapNext ) {
			i->accumTick[0] = 0;
			i->count[0] = 0;
			i->accumTick[1] = 0;
			i->count[1] = 0;
		}
	}
	else {
		for( ZProf *i = ZProf::heapHead; i; i=i->heapNext ) {
			i->accumTick[which] = 0;
			i->count[which] = 0;
		}
	}
}

#ifdef WIN32
unsigned __int64 zprofTick() {
	unsigned __int64 tick;
	__asm {
		// RDTSC
		_emit 0x0f
		_emit 0x31
		lea ecx, tick
		mov dword ptr [ecx], eax
		mov dword ptr [ecx+4], edx
	}
	return tick;
}
#endif

FILE *zprofDumpFile = 0;
void zprofDumpRecurseOrig( ZProf *r, int depth, ZProf *root ) {
	char line[256] = {0};
	
	__int64 percentOfTotal_0 = 0;
	__int64 percentOfTotal_1 = 0;
	if( root && root->accumTick && root != r ) {
		percentOfTotal_0 = ( 10000 * r->accumTick[0] / root->accumTick[0] );
		percentOfTotal_1 = ( 10000 * r->accumTick[1] / root->accumTick[1] );
	}
	sprintf(
		line+strlen(line),
		"%04.1lf%% %04.1lf%% ",
		(double)percentOfTotal_0 / 100.0,
		(double)percentOfTotal_1 / 100.0
	);

	char *c = line+strlen(line);
	for( int j=0; j<depth*2; j++ ) {
		*c++ = ' ';
	}

	sprintf( line+strlen(line), "%012I64u %012I64u %08I64u %08I64u %012I64u %012I64u ", r->accumTick[0], r->accumTick[1], r->count[0], r->count[1], r->count[0]==0 ? 0 : r->accumTick[0]/r->count[0], r->count[1]==0 ? 0 : r->accumTick[1]/r->count[1] );

	__int64 percentOfParent_0 = 0;
	__int64 percentOfParent_1 = 0;
	if( r->parent && r->parent->accumTick ) {
		percentOfParent_0 = r->parent->accumTick[0]==0 ? 0 : ( 10000 * r->accumTick[0] / r->parent->accumTick[0] );
		percentOfParent_1 = r->parent->accumTick[1]==0 ? 0 : ( 10000 * r->accumTick[1] / r->parent->accumTick[1] );
	}
	sprintf(
		line+strlen(line),
		"%04.1lf%% %04.1lf%% ",
		(double)percentOfParent_0 / 100.0,
		(double)percentOfParent_1 / 100.0
	);

	strcat( line, r->identString );

	fprintf( zprofDumpFile, "%s\n", line );

	for( ZProf *i = r->chldHead; i; i=i->siblNext ) {
		zprofDumpRecurseOrig( i, depth + 1, root );
	}
}

// This is a modified version that is easier to read as it dumps only
// the 0 accumulator and doesn't print global percentages
void zprofDumpRecurse( ZProf *r, int depth, ZProf *root ) {
	char line[256] = {0};
	
	char *c = line;
	for( int j=0; j<depth*2; j++ ) {
		*c++ = ' ';
	}

	__int64 percentOfParent_0 = 0;
	__int64 percentOfParent_1 = 0;
	if( r->parent && r->parent->accumTick ) {
		percentOfParent_0 = r->parent->accumTick[0]==0 ? 0 : ( 10000 * r->accumTick[0] / r->parent->accumTick[0] );
		percentOfParent_1 = r->parent->accumTick[1]==0 ? 0 : ( 10000 * r->accumTick[1] / r->parent->accumTick[1] );
	}

	__int64 percentOfTotal = root->accumTick[0]==0 ? 0 : ( 10000 * r->accumTick[0] / root->accumTick[0] );

	sprintf(
		c,
		"%04.1lf%% %04.1lf%% ",
		(double)percentOfParent_0 / 100.0,
		(double)percentOfTotal / 100.0
	);

	sprintf( line+strlen(line), "%012I64u %08I64u %012I64u ", r->accumTick[0], r->count[0], r->count[0]==0 ? 0 : r->accumTick[0]/r->count[0] );

	strcat( line, r->identString );

	fprintf( zprofDumpFile, "%s\n", line );

	for( ZProf *i = r->chldHead; i; i=i->siblNext ) {
		zprofDumpRecurse( i, depth + 1, root );
	}
}

void ZProf::dumpToFile( char *file, char *rootName ) {
	sortTree();

	// FIND the root
	ZProf *root = 0;
	ZProf *i;

	for( i = ZProf::heapHead; i; i=i->heapNext ) {
		if( !strcmp(i->identString,rootName) ) {
			root = i;
			break;
		}
	}
	
	zprofDumpFile = fopen( file, "wt" );
	fprintf( zprofDumpFile, "parent%% tot%% tot count avg name\n" );

	// There can be multiple top level entry points
	for( i = ZProf::heapHead; i; i=i->heapNext ) {
		if( ! i->parent || i == root ) {
			zprofDumpRecurse( i, 0, root );
		}
	}
	fclose( zprofDumpFile );
}

void ZProf::loadReference( char *file ) {
	char line[256];

	zprofDumpFile = fopen( file, "rt" );
	if( zprofDumpFile ) {
		// SKIP header line
		fgets( line, sizeof(line), zprofDumpFile );

		while( fgets( line, sizeof(line), zprofDumpFile ) ) {
			float totPer[2];
			unsigned __int64 ticks[2];
			unsigned __int64 count[2];
			unsigned __int64 avgTicks[2];
			float parPer[2];
			char name[256];
			int a = sscanf(
				line,
				"%f%% %f%% %I64d %I64d %I64d %I64d %I64d %I64d %f%% %f%% %s",
				&totPer[0], &totPer[1], &ticks[0], &ticks[1], &count[0], &count[1], 
				&avgTicks[0], &avgTicks[1],
				&parPer[0], &parPer[1], name
			);
			if( a == 11 ) {
				for( ZProf *i = ZProf::heapHead; i; i=i->heapNext ) {
					if( ! strcmp(i->identString,name) ) {
						i->refTick[0] = avgTicks[0];
						i->refTick[1] = avgTicks[1];
					}
				}
			}
		}

		fclose( zprofDumpFile );
	}
}

#endif


