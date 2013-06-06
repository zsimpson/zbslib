// @ZBS {
//		*MODULE_NAME Perl Compatible Regular Expression Wrapper
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Provides a nice c++ facade around the 
//			Perl-Compatible-Regular-Expression (PCRE) SDK. (www.pcre.org)
//		}
//		+EXAMPLE {
//			ZRegExp exp( "^\s+(\S+)" );
//			if( exp.test( string ) ) \{
//				printf( "%s", exp.get(1) );
//			\}
//		}
//		*PORTABILITY win32 unix
//		*SDK_DEPENDS pcre
//		*REQUIRED_FILES zregexp.cpp zregexp.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//			Change the static buffer to a smarter allocation system
//		}
//		*SELF_TEST no
//		*PUBLISH yes
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
#include "pcreposix.h"
// STDLIB includes:
#include "string.h"
#include "stdlib.h"
// MODULE includes:
#include "zregexp.h"
// ZBSLIB includes:

char ZRegExp::buffs[4][4096];
int ZRegExp::buffCycle = 0;

ZRegExp::ZRegExp( char *pattern ) {
	testString = NULL;
	matchBuf = NULL;
	compiledPatBuf = (char *)malloc( sizeof(regex_t) );

	err = pcre_regcomp( (regex_t *)compiledPatBuf, pattern, 0 );
	if( !err ) {
		int parenCount = 0;
		for( char *c = pattern; *c; c++ ) {
			if( *c == '(' ) {
				parenCount++;
			}
		}

		matchBufSize = parenCount + 1;
		matchBuf = (char *)malloc( sizeof(regmatch_t) * matchBufSize );
	}
}

ZRegExp::~ZRegExp() {
	pcre_regfree( (regex_t *)compiledPatBuf );
	free( compiledPatBuf );
	if( matchBuf ) {
		free( matchBuf );
	}
	if( testString ) {
		free( testString );
	}
}

int ZRegExp::test( char *s ) {
	if( testString ) {
		free( testString );
		testString = NULL;
	}
	testString = strdup( s );
	int _err = 0;
	if( err == 0 ) {
		_err = pcre_regexec( (regex_t *)compiledPatBuf, s, matchBufSize, (regmatch_t *)matchBuf, 0 );
	}
	if( _err == REG_NOMATCH ) {
		int a = 1;
	}
	return _err == 0;
}

char *ZRegExp::get( int i ) {
	buffCycle = (buffCycle+1) % 4;
	return get( i, buffs[buffCycle], 4096 );
}

int ZRegExp::getI( int i ) {
	return atoi( get( i ) );
}

float ZRegExp::getF( int i ) {
	return (float)atof( get( i ) );
}

double ZRegExp::getD( int i ) {
	return atof( get( i ) );
}

char *ZRegExp::get( int i, char *dst, int dstSize ) {
	int pos = getPos( i );
	int len = getLen( i );
	len = len < dstSize ? len : dstSize;
	if( pos >= 0 ) {
		memcpy( dst, &testString[pos], len );
		if( len < dstSize ) {
			dst[len] = 0;
		}
	}
	else {
		*dst = 0;
	}
	return dst;
}

char *ZRegExp::getDup( int i ) {
	int len = getLen( i );
	if( len >= 0 ) {
		char *b = (char *)malloc( len + 1 );
		get( i, b, len+1 );
		b[len] = 0;
		return b;
	}
	return NULL;
}

int ZRegExp::getPos( int i ) {
	return ((regmatch_t *)matchBuf)[i].rm_so;
}

int ZRegExp::getLen( int i ) {
	return ((regmatch_t *)matchBuf)[i].rm_eo - ((regmatch_t *)matchBuf)[i].rm_so;
}

