// @ZBS {
//		*MODULE_OWNER_NAME zwildcard
// }

#ifndef ZWILDCARD_H
#define ZWILDCARD_H

// Example usage:
//	ZWildcard w( "*.bmp" );
//	while( w.next() ) {
//		printf( "%s\n", w.getName() );
//	}

struct ZWildcard {
	char *pattern;
	void *findFileData;
	void *hFind;
	char *shortPattern;
	int flags;

	ZWildcard( char *_pattern );
	~ZWildcard();
	int next();
	char *getName();
	char *getFullName();
};

// @TODO: These should be move to zfilespec

int zWildcardFileExists( char *name );
	// Returns 1 if the file exists, 0 otherwise

int zWildcardOutOfDate( char *master, char *slave );
	// Returns 1 if slave is older or doesn't exist compared to master

void zWildcardTabExpand( char *partialString, char result[256] );
	// Given a partial string, expands as much as possible into result

int zWildcardFileIsFolder( char *name );
	// Returns 1 if the name is a folder

int zWildcardFileIsHidden( char *name );
	// Returns 1 if this file is hidden

int zWildcardNextSerialNumber( char *pattern );
	// Given a pattern like "images*.bmp" this assumes
	// that the start part of the pattern is a number
	// finds them all returns the largest+1 or zero if
	// none are found.
	

#endif

