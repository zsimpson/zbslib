// @ZBS {
//		*MODULE_NAME zcsvfile
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Fast CSV file reader
//		}
//		+EXAMPLE {
//		}
//		*PORTABILITY win32 unix macosx
//		*REQUIRED_FILES zcsvfile.cpp zcsvfile.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST yes console
//		*PUBLISH yes
// }
// OPERATING SYSTEM specific includes:
// STDLIB includes:
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "assert.h"
// MODULE includes:
// ZBSLIB includes:


char **zcsvFileParse( char *buf, int &rowCount, int &colCount, int tabMode ) {
	// In tabMode, the quotes are not removed and a tab is substitued for comma
	// For now, this enforeces that all rows must have the same number of fields

	int ptrAlloc = 100;
	int ptrCount = 0;
	char **ptrs = (char **)malloc( sizeof(char*) * ptrAlloc );
	#define ADD_PTR(x) \
		if( ptrCount == ptrAlloc ) { \
			ptrAlloc *= 2; \
			ptrs = (char **)realloc( ptrs, sizeof(char*) * ptrAlloc ); \
		} \
		ptrs[ptrCount++] = x;


	size_t dstAlloc = 12 * strlen( buf ) / 10;
	int dstCount = 0;
	int di = 0;
	int dstStart;
	char *dst = (char *)malloc( sizeof(char) * dstAlloc );
	#define ADD_DST(x,len) \
		dstStart = dstCount; \
		for( di=0; di<=len; di++ ) { \
			if( dstCount == dstAlloc ) { \
				dstAlloc *= 2; \
				dst = (char *)realloc( dst, sizeof(char) * dstAlloc ); \
			} \
			if( di==len ) { \
				dst[dstCount++] = 0; \
			} \
			else { \
				dst[dstCount++] = x[di]; \
			} \
		} \
		ADD_PTR( &dst[dstStart] );
	
	enum {
		START_FIELD,
		IN_QUOTED_FIELD,
		IN_UNQUOTED_FIELD,
		CR_EXPECTING_LF,
		ERROR,
	};
	
	char separator = tabMode ? '\t' : ',';

	rowCount = 0;
	colCount = 0;

	int recStartPtrCount = 0;
	int fieldStart = 0;
	int fieldStop = 0;
	int fieldLen = 0;
	int recordEnd = 0;
	int fieldEnd = 0;
	int state = START_FIELD;
	int i=0;
	for( i=0; ; i++ ) {
		switch( state ) {
			case START_FIELD:
				fieldStart = i;
				if( buf[i] == separator ) {
					fieldStop = i;
					fieldEnd = 1;
				}
				else if( buf[i] == '\r' ) {
					state = CR_EXPECTING_LF;
				}
				else if( buf[i] == '\n' ) {
					fieldStop = i;
					fieldEnd = 1;
					recordEnd = 1;
				}
				else if( buf[i] == 0 ) {
					fieldStop = i;
					fieldEnd = 1;
					recordEnd = 1;
				}
				else if( buf[i] == '\"' ) {
					fieldStart = i;
					state = IN_QUOTED_FIELD;
				}
				else {
					state = IN_UNQUOTED_FIELD;
				}
				break;

			case IN_QUOTED_FIELD:
				if( buf[i] == '\"' ) {
					state = IN_UNQUOTED_FIELD;
				}
				else if( buf[i] == 0 ) {
					state = ERROR;
				}
				break;

			case IN_UNQUOTED_FIELD:
				if( buf[i] == separator ) {
					fieldStop = i;
					fieldEnd = 1;
				}
				else if( buf[i] == '\r' ) {
					state = CR_EXPECTING_LF;
				}
				else if( buf[i] == '\n' ) {
					fieldStop = i;
					fieldEnd = 1;
					recordEnd = 1;
				}
				else if( buf[i] == 0 ) {
					fieldStop = i;
					fieldEnd = 1;
					recordEnd = 1;
				}
				break;
				
			case CR_EXPECTING_LF:
				if( buf[i] == '\n' ) {
					fieldStop = i-1;
					fieldEnd = 1;
					recordEnd = 1;
				}
				else {
					state = ERROR;
				}
				break;
				
			case ERROR:
				break;
		}
		
		if( fieldEnd ) {
			if( !tabMode && buf[fieldStart] == '\"' ) {
				assert( buf[fieldStop-1] == '\"' );
				fieldStart++;
				fieldStop--;
			}
			fieldLen = fieldStop - fieldStart;
			ADD_DST( (&buf[fieldStart]), fieldLen );
			state = START_FIELD;
		}
		if( recordEnd ) {
			if( rowCount == 0 ) {
				colCount = ptrCount;
			}
			else {
				assert( colCount == ptrCount-recStartPtrCount );
			}
			rowCount++;
			recStartPtrCount = ptrCount;
			state = START_FIELD;
			if( buf[i+1] == 0 ) {
				break;
			}
		}

		fieldEnd = 0;
		recordEnd = 0;

		if( buf[i] == 0 ) {
			break;
		}
	}
	return ptrs;
}

char **zcsvFileRead( char *filename, int &rowCount, int &colCount, int tabMode ) {
	FILE *f = fopen( filename, "rb" );
	assert( f );
	fseek( f, 0, SEEK_END );
	int fileSize = ftell( f );
	fseek( f, 0, SEEK_SET );

	char *buf = (char *)malloc( fileSize+1 );
	fread( buf, fileSize, 1, f );
	fclose( f );
	buf[fileSize] = 0;
		// Marks EOF

	char **ptrs = zcsvFileParse( buf, rowCount, colCount, tabMode );
	free( buf );
	return ptrs;
}


void zcsvFileFree( char **ptrs ) {
	free( ptrs[0] );
	free( ptrs );
}

#ifdef SELF_TEST

int main() {
	char **ptrs;

	int cols, rows;	

	ptrs = zcsvFileParse( "1,2,3\n4,5,6", rows, cols );
	assert( cols==3 && rows==2 );
	assert( !strcmp( ptrs[0], "1" ) );
	assert( !strcmp( ptrs[1], "2" ) );
	assert( !strcmp( ptrs[2], "3" ) );
	assert( !strcmp( ptrs[3], "4" ) );
	assert( !strcmp( ptrs[4], "5" ) );
	assert( !strcmp( ptrs[5], "6" ) );
	zcsvFileFree( ptrs );

	ptrs = zcsvFileParse( "1,,3\n,,", rows, cols );
	assert( cols==3 && rows==2 );
	assert( !strcmp( ptrs[0], "1" ) );
	assert( !strcmp( ptrs[1], "" ) );
	assert( !strcmp( ptrs[2], "3" ) );
	assert( !strcmp( ptrs[3], "" ) );
	assert( !strcmp( ptrs[4], "" ) );
	assert( !strcmp( ptrs[5], "" ) );
	zcsvFileFree( ptrs );
	
	ptrs = zcsvFileParse( "1\n2\n", rows, cols );
	assert( cols==1 && rows==2 );
	assert( !strcmp( ptrs[0], "1" ) );
	assert( !strcmp( ptrs[1], "2" ) );
	zcsvFileFree( ptrs );
	
	ptrs = zcsvFileParse( "1,2\r\n3,4\r\n", rows, cols );
	assert( cols==2 && rows==2 );
	assert( !strcmp( ptrs[0], "1" ) );
	assert( !strcmp( ptrs[1], "2" ) );
	assert( !strcmp( ptrs[2], "3" ) );
	assert( !strcmp( ptrs[3], "4" ) );
	zcsvFileFree( ptrs );

	ptrs = zcsvFileParse( "\"abc\",2,3\n4,\"def\",6", rows, cols );
	assert( cols==3 && rows==2 );
	assert( !strcmp( ptrs[0], "abc" ) );
	assert( !strcmp( ptrs[1], "2" ) );
	assert( !strcmp( ptrs[2], "3" ) );
	assert( !strcmp( ptrs[3], "4" ) );
	assert( !strcmp( ptrs[4], "def" ) );
	assert( !strcmp( ptrs[5], "6" ) );
	zcsvFileFree( ptrs );
	
	ptrs = zcsvFileParse( "1111,2222,3333\n4444,5555,6666", rows, cols );
	assert( cols==3 && rows==2 );
	assert( !strcmp( ptrs[0], "1111" ) );
	assert( !strcmp( ptrs[1], "2222" ) );
	assert( !strcmp( ptrs[2], "3333" ) );
	assert( !strcmp( ptrs[3], "4444" ) );
	assert( !strcmp( ptrs[4], "5555" ) );
	assert( !strcmp( ptrs[5], "6666" ) );
	zcsvFileFree( ptrs );

	return 0;
}

#endif


