// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A few functions for reading & writing whole binary files
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zbinfile.cpp zbinfile.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "stdio.h"
#include "assert.h"
#include "stdlib.h"
// MODULE includes:
#include "zbinfile.h"
// ZBSLIB includes:

void zbinFileWrite( char *filename, void *a, int len ) {
	FILE *f = fopen( filename, "wb" );
	int ret = fwrite( a, len, 1, f );
	assert( ret == 1 );
	fclose( f );
}

void *zbinFileRead( char *filename, void *into, int &len ) {
	FILE *f = fopen( filename, "rb" );
	if( f ) {
		if( len == -1 ) {
			// DETERMINE the length of the file
			fseek( f, 0, SEEK_END );
			len = ftell( f );
			fseek( f, 0, SEEK_SET );
		}
		if( !into ) {
			into = malloc( len );
		}
		int ret = fread( into, len, 1, f );
		assert( ret == 1 );
		fclose( f );
		return into;
	}
	return 0;
}

