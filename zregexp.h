// @ZBS {
//		*MODULE_OWNER_NAME zregexp
// }

// This is a C++ fascade for the posix interface to 
// PCRE - Perl Compatible Regular Expressions.
// This header has no dependiencies but the associated module
// depends on the pcre library.

#ifndef ZREGEXP_H
#define ZREGEXP_H

struct ZRegExp {
	static char buffs[4][4096];
	static int buffCycle;

	int matchBufSize;
	char *matchBuf;
	char *compiledPatBuf;
	char *testString;

	int err;
		// The last return code from pcre

	ZRegExp( char *pattern );
	~ZRegExp();

	int test( char *s );
		// Returns 1 if the string s matches the pattern.
		// A copy of this string is made and you can pull
		// matching parts out with the get methods below.

	char *get( int i );
		// Copies the ith into a local temp buffer and gives you a pointer
		// back to it.  This only works for short gets (under 4096 chars) 
		// and should be used with caution.  However, it is very useful
		// for simple little expressions like file name parsing.
		// There are only 4 temp buffers so be careful not to call mor than
		// 4 gets without making copies.

	char *get( int i, char *dst, int dstSize );
		// Copies the ith get into the dst buffer and returns pointer to it

	int getI( int i );
		// Give the buffer back as an int or zero if it cannot be converted
	float getF( int i );
		// Give the buffer back as an float or zero if it cannot be converted
	double getD( int i );
		// Give the buffer back as a double or zero if it cannot be converted

	char *getDup( int i );
		// Allocates a buffer for the output and gives you a pointer back to it.
		// Be sure to free it when you are done with it!

	int getPos( int i );
		// Gives the position of the ith element or -1 if not found
	int getLen( int i );
		// Gives the length of the ith element or -1 if not found
};

#endif

