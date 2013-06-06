// @ZBS {
//		*MODULE_OWNER_NAME zhashtable
// }

#ifndef ZTELNETSERVER_H
#define	ZTELNETSERVER_H

// This module is used to allow you to use a telnet client
// to attach to a server and execute commands
// Requires: zockets

#include "zocket.h"

#define	zTelnetServer_OUT_SIZE (4800)
#define	zTelnetServer_IN_SIZE (160)
#define	zTelnetServer_MAX_SESSIONS (4)
#define zTelnetServer_ANSI_CLEAR_TO_EOL "\x1B" "[0K"
#define zTelnetServer_ANSI_CLEAR_DISPALY "\x1B" "[2J"

typedef int ZTelnetServerCmdHandler( char *line, class ZTelnetServerSession *session );
	// Typedef of a function pointer that will get called by the
	// telnet server when a new line has been entered by a client.
	// line == NULL on startup
	// line == (char*)1 on shutdown.  WARNING, assumes that (char*)1 is not a valid pointer
	// line == string of entered line otherwise
	// If this callback returns zero, it tells it to shutdown this session

class ZTelnetServerSession {
  protected:
	int recvState;
		// current receive state
	int inSubOpt;
		// receiving subOpt characters
	char cmdId;
		// current command code
	ZTelnetServerCmdHandler *handler;
		// copy of the server's command handler

	Zocket *z;
	char outBuffer[zTelnetServer_OUT_SIZE];
	char inBuffer[zTelnetServer_IN_SIZE];
	int inCursor;
		// Where the next incoming character is written
	int outCursor;
		// Where the next outgoing character is written
	char passwdChar;
		// Character echoed when in password mode
	int passwdMode;
		// true if echoing in password mode.
	int charMode;
		// true if handler to be called on each character
	int addCRMode;
		// When sending, it translates \n to \r\n
	int echoMode;
		// Should it echo everything that is sent

  public:
	unsigned long    appData;		// A place to store per-session application data.

	ZTelnetServerSession();
	~ZTelnetServerSession();

	int start( Zocket *_z, ZTelnetServerCmdHandler *_handler );

	int stop();
		// Closes the session.

	int isActive() { return z != 0; }

	unsigned int getSockFD() { return( z ? z->sockFD : -1 ); }

	void setEchoMode( int _echoMode ) { echoMode = _echoMode; }

	void setAddCRMode( int _addCRMode ) { addCRMode = _addCRMode; }

	void setCharMode() { charMode = 1; }
	void setLineMode() { charMode = 0; }

	void setCmdHandler( ZTelnetServerCmdHandler *_handler) {
		handler = _handler;
	}

	// Each of these routines return the number of characters written to the internal buffer.
	// They may return less than the number of characters in the string or buffer, respectively.
	int write( void *buffer, int len );
	int puts( char *s );
	int putC( char c );
	int printf( char *fmt, ... );
	int flush();

	int poll();
		// This routine handles the sending and receiving of data to/from the socket

	void setPasswordMode (int on=1, char _passwdChar='*') { passwdMode = on; passwdChar = _passwdChar; }
		// Causes all echoed characters to be replaced with the given character
};

class ZTelnetServer {
  protected:
	Zocket *listening;
	ZTelnetServerSession session[zTelnetServer_MAX_SESSIONS];
	ZTelnetServerCmdHandler *handler;

  public:
	ZTelnetServer();
	ZTelnetServer( int port, ZTelnetServerCmdHandler *_handler );
	~ZTelnetServer();

	int start( int port );
	int stop();
		// Kills the active connections, but keeps listening for more

	int isActive() { return listening && listening->isOpen(); }

	void setCmdHandler( ZTelnetServerCmdHandler *_handler );

	int poll();
		// Handles the accepting of new connections, clean up 
		// of closed sessions, and the polling of active sessions.

	int fillOutSet( void *readSet, void *writeSet, void *exceptSet );
		// Adds the active sockets for this server to the given sets.
		// If a set is NULL, it does nothing for that set.
		// Returns the largest socket value set, 0 if none.
};

#endif
