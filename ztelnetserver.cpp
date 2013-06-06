// @ZBS {
//		*MODULE_NAME Telnet Server
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A telnet server which can plug easily into an application and listen on
//			a port.  You may have more than one and it can be non-blocking.  A simple
//			callback function API is used to alert the app of incoming messages.
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES ztelnetserver.cpp ztelnetserver.h
//		*VERSION 2.0
//		+HISTORY {
//			1.0 Adpated from code by Tony Bratton for NetStorm
//			2.0 ZBS naming convention changed
//		}
//		+TODO {
//		}
//		*SELF_TEST yes console
//		*PUBLISH yes
// }
// OPERATING SYSTEM specific includes:
#ifdef SELF_TEST
	#include "winsock.h"
#endif
// SDK includes:
// STDLIB includes:
#include "assert.h"
#include "string.h"
#include "stdio.h"
#include "stdarg.h"
// MODULE includes:
#include "ztelnetserver.h"
// ZBSLIB includes:
#include "zocket.h"

#ifndef min
	#define min(a,b) ( ( (a) < (b) ) ? (a) : (b) )
#endif
#ifndef max
	#define max(a,b) ( ( (a) > (b) ) ? (a) : (b) )
#endif

#define	SE		"\xF0"
#define	GA		"\xF9"
#define	SB		"\xFA"
#define	WILL	"\xFB"
#define	WONT	"\xFC"
#define	DO		"\xFD"
#define	DONT	"\xFE"
#define	IAC		"\xFF"
#define	CMD_SE		0xF0
#define	CMD_GA		0xF9
#define CMD_SB		0xFA 
#define CMD_WILL	0xFB 
#define CMD_WONT	0xFC 
#define CMD_DO		0xFD 
#define CMD_DONT	0xFE 
#define CMD_IAC		0xFF 
#define	ECHO				"\x01"
#define	SUPPRESS_GO_AHEAD	"\x03"
#define	LINEMODE			"\x22"
#define	OLD_ENVIRON			"\x24"
#define OPT_ECHO				0x01
#define OPT_SUPPRESS_GO_AHEAD	0x03
#define OPT_LINEMODE			0x22
#define OPT_OLD_ENVIRON			0x24
#define	ST_DEFAULT		0		// receiving regular characters
#define	ST_ESCAPE		1		// got an escape, looking for next character
#define	ST_COMMAND		2		// got command, looking for command data
#define	READ_BUFF_SIZE	512

// ZTelnetServerSession
//--------------------------------------------------------------------

ZTelnetServerSession::ZTelnetServerSession() {
	z = NULL;
	recvState = ST_DEFAULT;
	inSubOpt = 0;
	appData = 0;
	passwdMode = 0;
	passwdChar = '*';
	charMode = 0;
	addCRMode = 1;
	echoMode = 1;
}

ZTelnetServerSession::~ZTelnetServerSession() {
	if( isActive() ) {
		stop();
	}
}

int ZTelnetServerSession::start( Zocket *_z, ZTelnetServerCmdHandler *_handler ) {
	// The zocket has just been opened, so reset it 
	// and perform the initial Telnet negotiation
	handler = _handler;
	z = _z;
	inCursor = 0;
	outCursor = 0;
	passwdMode = 0;

	if( puts( IAC WILL SUPPRESS_GO_AHEAD ) < 0 ) return -1;
	if( puts( IAC WILL ECHO ) < 0 ) return -1;

	return 1;
}

int ZTelnetServerSession::stop() {
	// Shutdown and reset.
	if( z != NULL ) {
		delete z;
	}
	z = NULL;

	if( handler != NULL ) {
		// Tell the session that it has been shutdown.
		// NOTE: This is kind of hacky. -TNB
		(*handler)( (char *) 1, this );
	}
	return 1;
}

int ZTelnetServerSession::write( void *buffer, int len ) {
	if( !z ) return 0;
	int result = z->write( buffer, len, 0 );
	if( result < 0 ) {
		stop();
		return -1;
	}
	return result;

/* ZBS replaced with immediate write 26 Apr 2001

	char *buf = (char *)buffer;
	if( outCursor + len > OUT_SIZE ) {
		return 0;
	}

	int outBytes = 0;
	char *d = outBuffer + outCursor;
	for( int i=0; i<len; i++ ) {
		if( addCRMode && buf[i]=='\n' ) {
			*d++ = '\r';
			outBytes++;
		}
		*d++ = buf[i];
		outBytes++;
		if( outCursor + outBytes >= OUT_SIZE ) {
			return 0;
		}
	}

	outCursor += outBytes;
	return outBytes;
*/
}

int ZTelnetServerSession::puts( char *s ) {
	return( write(s, strlen(s)) );
}

int ZTelnetServerSession::putC( char c ) {
	return( write(&c, 1) );
}

int ZTelnetServerSession::printf( char *fmt, ... ) {
	char buffer[1024];
	va_list argptr; 
	if( fmt != buffer ) {
		va_start( argptr, fmt );
		vsprintf( buffer, fmt, argptr );
		va_end( argptr );
	}
	buffer[1023] = '\0';
	return puts( buffer );
}

int ZTelnetServerSession::flush() {
	// Write all the available output to the zocket, blocking.
	while( outCursor > 0 ) {
		int result = z->write( outBuffer, outCursor, 0 );
			// This is a blocking call, we need to decide
			// if a write can ever block.  Need experiments
			// ZBS10OCT CHANGED TO non-blocking and changed error below to stop on underflow
		if( result < outCursor ) {
			stop();
			return 0;
		}
		else {
			memmove( outBuffer, outBuffer+result, outCursor-result );
			outCursor -= result;
		}
	}
	return 1;
}

//int ZTelnetServerSession::poll( ZTelnetServerCmdHandler *handler ) {
int ZTelnetServerSession::poll() {
	// Attempt to read some data from the zocket, without blocking.

	char readBuffer[READ_BUFF_SIZE];
	int len = z->read( readBuffer, READ_BUFF_SIZE, 0 );
	if( len == -1 ) {
		stop();
		return 0;
	}

	// If any data was read, scan it for escape codes.

	int index = 0;
	int count = len;
	while( count-- && inCursor < zTelnetServer_IN_SIZE-1 ) {
		unsigned char c = readBuffer[index];
		
		// Handle either as a command or a raw characters

		switch( recvState ) {
			case ST_DEFAULT:
				if( c == CMD_IAC ) {
					// Escaped character, change state
					recvState = ST_ESCAPE;
				}
				else {
					if( inSubOpt ) {
						// Add the character to the subOpt buffer
					}
					else {
						if( charMode ) {
							if( handler != NULL ) {
								if( echoMode ) write( &c, 1 );
								char buf[2];
								buf[0] = c;
								buf[1] = 0;
								if( !(*handler)(buf, this) ) {
									stop();
									return 0;
								}
							}
						}
						else {
						// If the character is a \r, send a newline as well, then terminate the command buffer and send
						// the line to the command processor.

							if( c == '\r' ) {
								// All carriage returns get converted to CRLF (on echo only)
								if( echoMode ) write( &c, 1 );
								c = '\n';
								if( echoMode ) write( &c, 1 );

								inBuffer[inCursor] = '\0';

								if( handler != NULL ) {
									if( !(*handler)(inBuffer, this) ) {
										stop();
										return 0;
									}
								}

								inCursor = 0;
							}
							else if( c == 0x08 ) {
								// Destructively backspace.

								if( inCursor > 0 ) {
									--inCursor;
									if( echoMode ) write( &c, 1 );
									c = ' ';
									if( echoMode ) write( &c, 1 );
									c = 0x08;
									if( echoMode ) write( &c, 1 );
								}
							}
							else if( c == '\n' || c == '\0' ) {
								// HACK: Swallow newlines in the input,
								// so that Microsoft's POS telnet client will look right.
								// For some reason, I was getting '\0' characters
								// being sent down the line. Swallow them too.
							}
							else {
								// This character is to be kept, so add it to the inBuffer
								inBuffer[inCursor] = c;
								++inCursor;

								// Echo this character.
								if( passwdMode ) {
									if( echoMode ) write( &passwdChar, 1 );
								}
								else {
									if( echoMode ) write( &c, 1 );
								}
							}
						}
					}
				}
				break;

			case ST_ESCAPE:
				if( c == CMD_IAC ) {
					// This was not a command, but simply an escaped
					// CMD_IAC character. Store a CMD_IAC in the in buffer and
					// return to the default state.
					if( inSubOpt ) {
						// Add the character to the subOpt buffer
					}
					else {
						inBuffer[inCursor++] = c;
					}
					recvState = ST_DEFAULT;
				}
				else {
					// This is a command sequence, store it and change the state.
					cmdId = c;
					recvState = ST_COMMAND;
				}
				break;

			case ST_COMMAND: {
					// The last part of a command sequence
					switch( c ) {
						case CMD_WILL:
						case CMD_WONT:
						case CMD_DO:
						case CMD_DONT:
							// A real telnet server would send the correct response here.
							// We, however, choose to send nothing for now.
							break;

						case CMD_SB:
							// Set sub opt mode
							inSubOpt = 1;
							break;

						case CMD_SE:
							// Swallow the next character, and reset sub opt mode
							inSubOpt = 0;
							break;

						default:
							// Fall through
							break;
					}
					recvState = ST_DEFAULT;
				}
				break;
		}

		// Process the next character 
		++index;
	}

	// Write any available output without blocking.
/* ZBS removed and converted to immediate write 
	if( outCursor > 0 ) {
		int result = z->write( outBuffer, outCursor, 0 );
		if( result == -1 ) {
			stop();
			return 0;
		}
		else {
			memmove( outBuffer, outBuffer+result, outCursor-result );
			outCursor -= result;
		}
	}
*/
	return 1;
}

// ZTelnetServer
//--------------------------------------------------------------------

ZTelnetServer::ZTelnetServer() {
	handler = NULL;
	listening = NULL;
}

ZTelnetServer::ZTelnetServer( int port, ZTelnetServerCmdHandler *_handler ) {
	handler = NULL;
	listening = NULL;
	start( port );
	setCmdHandler( _handler );
}

ZTelnetServer::~ZTelnetServer() {
	// Stop all active sessions
	stop ();
	handler = NULL;
}

int ZTelnetServer::start( int port ) {
	assert(listening == NULL);

	char zAddress[100];
	sprintf( zAddress, "tcp://*:%d", port );
	listening = new Zocket( zAddress, ZO_LISTENING|ZO_NONBLOCKING );

	if( !listening->isOpen() ) {
		return 0;
	}

	return 1;
}

int ZTelnetServer::stop() {
	if ( !listening )
	{
		return 1;
	}

	if ( listening->isOpen() )
	{
		for( int i=0; i<zTelnetServer_MAX_SESSIONS; ++i ) {
			if( session[i].isActive() ) {
				session[i].stop();
			}
		}
	}
	delete listening;
	listening = NULL;
	return 1;
}

void ZTelnetServer::setCmdHandler( ZTelnetServerCmdHandler *_handler ) {
	handler = _handler;
	for( int i = 0; i < zTelnetServer_MAX_SESSIONS; ++i ) {
		if( session[i].isActive() ) {
			session[i].setCmdHandler( _handler );
		}
	}
}

int ZTelnetServer::poll() {
	int i;

	if( listening->isOpen() ) {
		// Check for a new connection

		int sockFD = listening->accept( 0 );
		if( sockFD > 0 ) {
			Zocket *z = new Zocket( listening );

			// Find an inactive session and initialize it with this zocket.
			// If there is no inactive session, close the zocket.

			for( i = 0; i < zTelnetServer_MAX_SESSIONS; ++i ) {
				if( !session[i].isActive() ) {
					if( session[i].start(z, handler) ) {
						// Call the handler function with a NULL line, indicating that the connection was just established.
						if( handler != NULL ) {
							if( !(*handler)(NULL, &session[i]) ) {
								session[i].stop();
							}
						}
					}
					break;
				}
			}

			if( i >= zTelnetServer_MAX_SESSIONS ) {
				delete z;
			}
		}
	}

	// Poll all of the active sessions.
	for( i = 0; i < zTelnetServer_MAX_SESSIONS; ++i ) {
		if( session[i].isActive() ) {
			session[i].poll();
		}
	}

	return 1;
}

int ZTelnetServer::fillOutSet( void *readSet, void *writeSet, void *exceptSet ) {
	// Fill out the set for each catagory from the listening zocket and each active zocket.
	unsigned int largest = 0;
	if( listening->isOpen() ) {
		if( readSet != NULL ) {
			zocketFdSet( listening->sockFD, readSet );
			largest = max( largest, listening->sockFD );
		}

		if( writeSet != NULL ) {
			zocketFdSet( listening->sockFD, writeSet );
			largest = max( largest, listening->sockFD );
		}

		if( exceptSet != NULL ) {
			zocketFdSet( listening->sockFD, exceptSet );
			largest = max( largest, listening->sockFD );
		}

		for( int i = 0; i < zTelnetServer_MAX_SESSIONS; ++i ) {
			if( session[i].isActive() ) {
				if( readSet != NULL ) {
					zocketFdSet( session[i].getSockFD(), readSet );
					largest = max( largest, session[i].getSockFD() );
				}

				if( writeSet != NULL ) {
					zocketFdSet( session[i].getSockFD(), writeSet );
					largest = max( largest, session[i].getSockFD() );
				}

				if( exceptSet != NULL ) {
					zocketFdSet( session[i].getSockFD(), exceptSet );
					largest = max( largest, session[i].getSockFD() );
				}
			}
		}
	}
	return largest;
}


#ifdef SELF_TEST

int handler( char *line, class ZTelnetServerSession *session ) {
	if( line == NULL ) {
		printf( "New connection\n" );
	}
	else if( line == (char *)1 ) {
		printf( "Lost connection\n" );
	}
	else {
		printf( "Received command %s\n", line );
		if( !strcmp( line, "quit" ) ) {
			session->printf( "Goodbye!\n" );
			return 0;
		}
	}
	return 1;
}

void main() {
	ZTelnetServer rCmdServer( 2000, handler );
	printf( "Waiting for telnet connection on port 2000\n" );

	// The server runs a non-blocking interface that requires
	// an occasional call to poll.  You can either call this poll
	// once per frame in a main-loop like this:
	//
	// while( 1 ) {
	//     rCmdServer.poll();
	// }
	//
	// Or you can fill out a selection set and go to sleep, like this
	// which is the OS friendly way to do it.

	while( 1 ) {
		fd_set readSet;
		fd_set exceptSet;

		FD_ZERO( &readSet );
		FD_ZERO( &exceptSet );

		int largest = rCmdServer.fillOutSet( &readSet, NULL, &exceptSet );
		int _select = select( largest+1, &readSet, NULL, &exceptSet, NULL );
			// This select will block until one of the file handles has
			// data or exception pending.

		rCmdServer.poll();
	}
}

#endif

