// @ZBS {
//		*MODULE_OWNER_NAME zcmdparse
// }

#ifndef ZCMDPARSE_H
#define ZCMDPARSE_H

#include "zhashtable.h"

void zCmdParseCommandLine( char *unparsedLine, ZHashTable &hash );
void zCmdParseCommandLine( int argc, char **argv, ZHashTable &hash );
	// Parses the command line, anything arg which starts with a '-'
	// is placed into the hash with any value assignment
	// A separate key is made for the index of every entry
	// For example:
	// argv == test.exe -d -o=option1 --print
	// hash would contain:
	// -d = *novalue*
	// -o = optiona1
	// --print = *novalue*
	// -d-index = 1
	// -o-index = 2
	// --print-index = 3

#endif

