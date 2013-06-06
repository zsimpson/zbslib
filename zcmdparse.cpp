// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Create a nice hash from the command line arguments
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zcmdparse.cpp zcmdparse.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH yes
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "string.h"
#include "stdio.h"
#include "assert.h"
// MODULE includes:
// ZBSLIB includes:
#include "zhashtable.h"
#include "zcmdparse.h"

void zCmdParseCommandLine( char *unparsedLine, ZHashTable &hash ) {
	int argc = 0;
	char *argv[512];
	

	#define LOOKING_FOR_START (1)
	#define IN_WORD (2)
	int state = LOOKING_FOR_START;

	for( char *c = unparsedLine; *c; c++ ) {
		switch( state ) {
			case LOOKING_FOR_START:
				if( *c == 0 ) {
					goto done;
				}
				else if( *c != ' ' ) {
					argv[argc++] = c;
					assert( argc < sizeof(argv)/sizeof(argv[0]) );
					state = IN_WORD;
				}
				break;

			case IN_WORD:
				if( *c == ' ' || *c == 0 ) {
					*c = 0;
					state = LOOKING_FOR_START;
				}
				break;
		}
	}

	done:;

	zCmdParseCommandLine( argc, argv, hash );

}

void zCmdParseCommandLine( int argc, char **argv, ZHashTable &hash ) {
	int nonOptionArgCount=0;
	for( int i=0; i<argc; i++ ) {
		if( argv[i][0] == '-' ) {
			// Does the option have a value assignment?
			char *valPtr = 0;
			for( int j=0; argv[i][j]; j++ ) {
				if( argv[i][j] == '=' ) {
					valPtr = &argv[i][j+1];
					argv[i][j] = 0;
					break;
				}
			}

			hash.putS( argv[i], valPtr ? valPtr : (char *)"*novalue*" );

			int len = strlen( argv[i] );
			char *indexKey = new char[len + 16];
			strcpy( indexKey, argv[i] );
			strcat( indexKey, "-index" );

			hash.putI( indexKey, i );
		}
		else {
			char key[32];
			sprintf( key, "nonOptionArg%d", nonOptionArgCount++ ); 
			hash.putS( key, argv[i] );
		}
	}
}
