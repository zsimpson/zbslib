// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A telnet handler function that can plug into the ztelnetserver class
//			Handles the getting and setting of zvars
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zmsgtelnet.cpp zmsgtelnet.h
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
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
// MODULE includes:
// ZBSLIB includes:
#include "zmsg.h"
#include "ztelnetserver.h"

int zMsgTelnetHandler( char *line, class ZTelnetServerSession *s ) {
	if( line == NULL ) {
		s->setLineMode();
		s->setEchoMode(0);
		zMsgQueue( "type=TmsgTelnetConnected fromTelnetSession=%p", s );
		return 1;
	}

	if( line == (char*)1 ) {
		zMsgQueue( "type=TmsgTelnetDisconnected fromTelnetSession=%p", s );
		return 1;
	}

	char *buf = (char *)malloc( strlen(line) + 50 );
	sprintf( buf, "%s fromTelnetSession=0x%X", line, s );
	zMsgQueue( buf );
	free( buf );

	return 1;
}
