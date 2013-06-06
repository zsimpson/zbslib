// @ZBS {
//		*MODULE_NAME Filelist manager
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A class for building up lists of files and operating on them
//		}
//		+EXAMPLE {
//		}
//		*PORTABILITY win32
//		*REQUIRED_FILES zfilelist.cpp zfilelist.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }

// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "stdlib.h"
#include "string.h"
// MODULE includes:
// ZBSLIB includes:
#include "zfilelist.h"
#include "zregexp.h"
#include "zwildcard.h"

void ZFileList::clear() {
	if( list ) {
		for( int i=0; i<activeCount; i++ ) {
			free( list[i] );
		}
		free( list );
	}
	list = 0;
	allocCount = 0;
	activeCount = 0;
}

void ZFileList::addPat( char *dir, char *regexp ) {
	char filespec[256];
	strcpy( filespec, dir );
	if( dir[strlen(dir)-1] != '/' && dir[strlen(dir)-1] != '\\' ) {
		strcat( filespec, "/" );
	}
	strcat( filespec, "*" );

	ZRegExp regex( regexp );

	ZWildcard w( filespec );
	while( w.next() ) {
		if( regex.test(w.getName()) ) {
			addFile( w.getFullName() );
		}
	}
}

void ZFileList::addDirFile( char *dir, char *filename ) {
	char filespec[256];
	strcpy( filespec, dir );
	if( dir[strlen(dir)-1] != '/' && dir[strlen(dir)-1] != '\\' ) {
		strcat( filespec, "/" );
	}
	strcat( filespec, filename );
	addFile( filespec );
}

void ZFileList::addFile( char *filename ) {
	if( activeCount == allocCount ) {
		allocCount *= 2;
		if( allocCount < 16 ) allocCount = 16;
		list = (char **)realloc( list, sizeof(char *) * allocCount );
	}
	list[activeCount++] = strdup( filename );
}

int ZFileList::find( char *filename ) {
	for( int i=0; i<activeCount; i++ ) {
		if( !strcmp(filename,list[i]) ) {
			return i;
		}
	}
	return -1;
}

ZFileList::ZFileList() {
	list = 0;
	allocCount = 0;
	activeCount = 0;
}

ZFileList::ZFileList( char *dir, char *regexp ) {
	list = 0;
	allocCount = 0;
	activeCount = 0;
	addPat( dir, regexp );
}

ZFileList::~ZFileList() {
	clear();
}


