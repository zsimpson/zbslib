// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Manages the import/export of configuration files which consists
//			of key/value pairs.
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zconfig.cpp zconfig.h
//		*VERSION 1.1
//		+HISTORY {
//			1.1 Updated naming convention. Eliminated out of date macros, simplified
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#ifdef WIN32
#include "malloc.h"
#endif
// MODULE includes:
#include "zconfig.h"
// ZBSLIB includes:
#include "zhashtable.h"
#include "zregexp.h"

int zConfigLoadFile( char *filename, ZHashTable& hash, int markAsChanged ) {
	if( !filename ) {
		return 0;
	}
	FILE *file = fopen( filename, "rt" );
	if( !file ) {
		return 0;
	}
	int a = ftell( file );
	fseek( file, 0, SEEK_END );
	int size = ftell( file );
	fseek( file, a, SEEK_SET );
	char *buffer = (char *)malloc( size+1 );
	buffer[size] = 0;
	// used to be:
	//     fread( buffer, size, 1, file );
	// but this does not return a useful count.
	int bytes_read = fread (buffer, 1, size, file);
	// null terminate based on character count after text mode adjustments (CR-LF to single character).
	// size as determined from ftell() is NOT CORRECT!
	buffer [bytes_read] = 0;
	fclose( file );
	zConfigParse( buffer, hash, markAsChanged );
	free( buffer );
	return 1;
}

#define countof(x) (sizeof(x)/sizeof(x[0]))
void zConfigParse( char *info, ZHashTable& hash, int markAsChanged ) {
	ZRegExp regExp1( "^\\s*//" );
		// Comments
	ZRegExp regExp2( "^\\s*\\[([a-zA-Z0-9.]*)\\]" );
		// [key]
	ZRegExp regExp3( "^\\s*\"?([^\"\\s]+)\"?[ \\t]*=\\s*\"([^\"\\n\\r]*)\"" );
		// key = "val"
		// "key" = "val"
	ZRegExp regExp4( "^\\s*\"?([^\"\\s]+)\"?\\s*=\\s*([^\\n\\r]*)" );
		// key = val
		// "key" = val

	char key[80];
	char val[4096];
	int insideMultiline = 0;
	int valLen = 0;

	char *line = info;
	int len = strlen(line);

	while( line-info < len ) {
		char *lf = strchr( line, '\n' );
		if( lf ) {
			*lf = 0;
		}

		if( insideMultiline ) {
			if( regExp2.test( line ) ) {
				hash.putS( key, val );
				if( markAsChanged ) {
					hash.setChanged( key );
				}
				*key = *val = 0;
				regExp2.get( 1, key, 80 );
				insideMultiline = 0;
				if( *key ) {
					insideMultiline = 1;
					valLen = 0;
				}
			}
			else {
				int lineLen = strlen(line);
				if( valLen + lineLen < countof(val)-1 ) {
					strcpy( &val[valLen], line );
					valLen += lineLen;
					val[valLen] = '\n';
					++valLen;
					// NOTE: valLen indicates where to write the next part of the string,
					// so we don't increment it beyond the terminator.
					val[valLen] = '\0';
				}
				else {
					// *** TODO: ZBSLib needs some sort of standard trace or error reporting mechanism.
					//fprintf (stderr, "Multiline value truncated!\n");
				}
			}
		}
		else {
			// Is this line commented out?
			if( regExp1.test( line ) ) {
			}

			// Is this a multi-line key?
			else if( regExp2.test( line ) ) {
				insideMultiline = 1;
				valLen = 0;
				regExp2.get( 1, key, 80 );
			}

			// Is this a simple key with a quoted value?
			else if( regExp3.test( line ) ) {
				regExp3.get( 1, key, 80 );
				regExp3.get( 2, val, 1024 );
				hash.putS( key, val );
				if( markAsChanged ) {
					hash.setChanged( key );
				}
				*key = *val = 0;
			}

			// Is this a simple key with a non-quoted value?
			else if( regExp4.test( line ) ) {
				regExp4.get( 1, key, 80 );
				regExp4.get( 2, val, 1024 );
				hash.putS( key, val );
				if( markAsChanged ) {
					hash.setChanged( key );
				}
				*key = *val = 0;
			}
		}

		line += strlen( line )+1;
	}
}

static int configStringCompare(const void *a, const void *b) {
#ifdef __USE_GNU
#define stricmp strcasecmp
#endif
	return stricmp( *(char**)a, *(char**)b );
}

int zConfigSaveFile( char *filename, ZHashTable &table ) {
	int count = 0;
	char **sorted = (char **)alloca( sizeof(char *) * table.size() );
	for( int i=0; i<table.size(); i++ ) {
		char *key = table.getKey(i);
		if( key ) {
			sorted[count++] = key;
		}
	}
	qsort( sorted, count, sizeof(char*), configStringCompare );

	// @TODO: Make the file a little cleaner by deciding if quotes or multiline is needed
	// For now, fuck it, no quotes until we need them
	FILE *file = fopen( filename, "wt" );
	if( file ) {
		for( int i=0; i<count; i++ ) {
			char *key = sorted[i];
			char *val = table.getS(key);
			// @TODO: Escape code quotes

			int quoteVal = 0, quoteKey = 0;

			if( val ) {
				int valLen = strlen(val);
				for( int j=0; j<valLen; j++ ) {
					if( val[j] < '!' || val[j] >= '~' ) {
						quoteVal = 1;
						break;
					}
				}
			}

			if( key ) {
				int keyLen = strlen(key);
				for( int j=0; j<keyLen; j++ ) {
					if( key[j] < '!' || key[j] >= '~' ) {
						quoteKey = 1;
						break;
					}
				}

				if( quoteKey ) {
					fprintf( file, "\"%s\" = ", key );
				}
				else {
					fprintf( file, "%s = ", key );
				}

				if( quoteVal ) {
					fprintf( file, "\"%s\"\n", val );
				}
				else {
					fprintf( file, "%s\n", val );
				}
			}
		}
		fclose( file );
		// success
		return 1;
	}
	// failed to open file
	return 0;
}

