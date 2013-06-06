// @ZBS {
//		*MODULE_NAME Simple Inline String Constructor
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			This is a convenient class for creating inline
//			strings using the familiar printf style without 
//			having to declare a separate buffer.  Originally written by Ken Demarest
//		}
//		+EXAMPLE {
//			new File( ZTmpStr( "%s.dat", filename ) );
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES ztmpstr.cpp ztmpstr.h
//		*VERSION 2.0
//		+HISTORY {
//			1.0 KLD original idea implemented in NetStorm
//			2.0 ZBS isolated, simplified, made portable
//		}
//		+TODO {
//		}
//		*SELF_TEST yes console
//		*PUBLISH yes
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "string.h"
#include "ctype.h"
#include "stdarg.h"
#include "stdio.h"
#include "assert.h"
// MODULE includes:
#include "ztmpstr.h"
// ZBSLIB includes:


#define assembleTo(dest,fmt,maxlen) { va_list argptr; if (fmt != (dest)) { va_start (argptr, fmt); vsprintf((dest), fmt, argptr); va_end (argptr); } assert(strlen(dest)<maxlen); }
	// I had to put this thing all on one line because it
	// wouldn't compile properly undex UNIX.
	// I know that its ugly and hard to read, but it needs to stay this way.


ZTmpStr::ZTmpStr( char *fmt, ... ) {
	assembleTo(s,fmt,ZTMPSTR_MAXS);
}

void ZTmpStr::set( char *fmt, ... ) {
	assembleTo(s,fmt,ZTMPSTR_MAXS);
}

void ZTmpStr::append( char *fmt, ... ) {
	assembleTo(s+strlen(s),fmt,ZTMPSTR_MAXS-strlen(s));
}

int ZTmpStr::getLen() {
	return strlen(s);
}

char *ZTmpStr::allCaps() {
	char *ss = s;
	while( *ss ) {
		*ss = toupper(*ss);
		++ss;
	}
	return s;
}

char *ZTmpStr::cap() {
	char *ss = s;
	while( *ss && !isalpha(*ss) ) {
		++ss;
	}

	if( *ss ) {
		*ss = toupper(*ss);
	}
	return s;
}

#ifdef SELF_TEST

int main() {
	ZTmpStr t0;
	assert( t0.getLen() == 0 );
		// Test inititalization

	assert( !strcmp( ZTmpStr( "This is a test" ), "This is a test" ) );
		// Test automatic conversion to a (char *) inline

	ZTmpStr t1( "this %s a test %d", "IS", -6 );
	t1.cap();
	assert( !strcmp( t1, "This IS a test -6" ) );

	ZTmpStr t2( "this IS a test" );
	t2.allCaps();
	assert( !strcmp( t2, "THIS IS A TEST" ) );

	ZTmpStr t3;
	t3.allCaps();
	assert( !strcmp( t3, "" ) );

	return 0;
}

#endif

