// @ZBS {
//		*MODULE_OWNER_NAME zocket
// }

#ifndef ZOCKET_H
#define ZOCKET_H

#if !defined(STOPFLOW)

#ifdef WIN32
	#ifndef _WINSOCKAPI_
		struct in_addr {
			unsigned int s_addr;
		};

		struct sockaddr_in {
			short sin_family;
			unsigned short sin_port;
			struct in_addr sin_addr;
			unsigned char sin_zero[8];
		};
	#endif
#else
	#include "netinet/in.h"
#endif

struct ZocketAddress {
	// The sockaddr_in address:
	sockaddr_in address;
	int protocol;

	void decode( char *address );
		// Parse a string in the form "tcp://www.oink.com:50000"
	char *encode();
		// Generate a string in the form "tcp://www.oink.com:50000".
		// Returns a pointer to a static buffer
	char *reverseLookup();
		// Does a reverse DNS lookup on the address and returns pointer to a 
		// static buffer. Empty string on failure

	void clear();

	ZocketAddress();
};


struct Zocket {
	ZocketAddress zocketAddress;
		// binary encoded address
	unsigned int sockFD;
		// socket file description, in unix this is equiv to other file desciptor, under windows it isn't
		#define SOCKFD_NUL (0xFFFFFFFF)
	int acceptedFD;
		// Used when a listening socket accepts a connection
	int options;
		#define ZO_LISTENING (1)
		#define ZO_TCP (2)
		#define ZO_UDP (4)
		#define ZO_NONBLOCKING (8)
		#define ZO_DISABLE_NAGLE (16)
		#define ZO_EXCLUDE_FROM_POLL_LIST (32)
			// Used by ZMsgZocket

	int state;
		#define ZO_CLOSED (0)
		#define ZO_OPEN (1)
		#define ZO_ERROR (2)
		#define ZO_OPEN_WOULD_BLOCK (4)
		#define ZO_SENT_CONNECTED (8)
			// Used by ZMsgZocket

	int error;
		// last error code

	int blockingOverride( int block );
		// Helper uised by the read, write calls to temporarily override blocking defaults

	void open( ZocketAddress, int _options );
	void open( char *address, int _options );
	void open( char *address, int port, int _options );
	void close();

	ZocketAddress getRemoteAddress();

	int read ( void *buffer, int bufferSize, int block=-1 );
	int write( void *buffer, int bufferSize, int block=-1 );
	int writeString( char *buffer, int block=-1 );

	int dgramRead ( void *buffer, int bufferSize, ZocketAddress &address, int block=-1 );
	int dgramWrite( void *buffer, int bufferSize, ZocketAddress &address, int block=-1 );
		// These functions are seperate from the read and write because they
		// must make a different call in order to retrieve the zAddress
		// from the sender.  However, if you don't care about this (unlikely for read)
		// then you should be able to just call read and write on a udp socket
		// Same return values as above.

	int isListening();
	int isConnected();
	int isOpen();

	int accept( int block = -1 );

	void init();
	
	Zocket();

	Zocket( Zocket *listeningZocket );
		// This is used only in the case that a listening zocket has found
		// a new connection and you want to create a new Zocket for that connection
		// (typically you would leave the listening socket alone allowing it
		// to wait for another connection).
		// For example:
		//   int _accept = listeingZocket->accept( zoBLOCK );
		// 	 if( _accept != INVALID_SOCKET ) {
		//       Zocket *newZocket = new Zocket( listening );
		//   }
		// The listening socket caches the last accepted sockFD so it
		// doesn't have to be passed back in.


	Zocket( char *address, int _options ) {
		open( address, _options );
	}

	Zocket( char *address, int port, int _options ) {
		open( address, port, _options );
	}

	~Zocket();
};

extern void zocketFdClr( unsigned int fd, void *set );
extern void zocketFdSet( unsigned int fd, void *set );
extern void zocketFdZero( void *set );
extern int  zocketFdIsSet( unsigned int fd, void *set );

#endif

#endif




