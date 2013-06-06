// @ZBS {
//		*MODULE_OWNER_NAME zocket
// }

#ifndef ZOCKET_H
#define ZOCKET_H

#include "zaddress.h"


// @TODO:
// Note from Karl

//You might want to let Zack know that the use of ZAddress::set( address ) 
//in Zocket::new iMwin() causes arbitrary bits to be written 
//to the Zocket.  This is because the memcopy assignment uses 
//sizeof(*this) and Zockets, which extend ZAddress, have extra data 
//members that won’t necessarily be part of the incoming variable.
//Sample code:
//
//void breakMe( const int ip, const int port=0 )
//{
//Zocket zocket;
//ZAddress address;
//address.setProtocol(“tcp”);
//address.setIP( ip, port );
//zocket.open( address ); // memory written to with random bytes, may cause assert in Zockets.cpp line 834
//}
//
//Using sizeof(zAddress) instead of sizeof(*this) at line 429 fixes the problem, though 
//I personally believe it’s still safer to manually assign the values and the extra speed 
//isn’t required given how often a socket will be set during runtime.
//
//
//Also, I think that AZddress:setProtocol on line 215 should be setting zoLOOP 
//instead of (zoIPX|zoIPX_FAMILY) when the “loop” protocol is specified.
//
//Two more, both in Zocket::Zocket( listenerZocket ) :
//
//1) not all data members have initial values set (I added a call to reset()
//2) the new zocket's zsOPEN flag wasn't being set.
//
//I had a case where all zockets spawned off that constructor had
//zsREAD_WRITE_FAIL set, and traced it back to the uninitialized
//zocketStateFlags value.






// ZOCKETS II -- ZBS 18 March 98
//
// Zocket is a portable wrapper around the Berkley socket layer.
// The purpose of this abstraction class is several-fold.
//  * Provide a completely portable API.  All OS specific features
//    are encoded inside of the class.
//  * Eliminate the need to include windows.h in WIN32 apps that 
//    use networking
//  * Allow easy overriding of blocking / non-blocking in the
//    as parameters into read and write
//    non-blocking.
//  * Use strings for addressing to allow easy backward-compatibility
//    as new protocols are added.
//  * Allow for a non-socket level loopback.  Allows you to use the
//    networking layer even if sinsock isn't installed on a WIN32 machine.
//
// Zockets are stream based.  The inherited class "Zacket" adds a
// message aware protocol.

extern char *zocketErrorStrings[];
enum ZocketError {
	zeNO_ERROR = 0,
	zeUNSUPPORED_OR_BAD_PROTOCOL,
	zeBAD_ADDRESS,
	zeBAD_CLIENT_SERVER_OPTION,
	zeBAD_IP_ADDRESS,
	zeBAD_IPX_ADDRESS,
	zeUNABLE_TO_START_WINDOWS,
	zeUNABLE_TO_OPEN_SOCKET,
	zeUNABLE_TO_LOOKUP_NAME,
	zeUNABLE_TO_CONNECT,
	zeUNABLE_TO_BIND,
	zeUNABLE_TO_LISTEN,
	zeUNABLE_TO_SET_OPTION,
	// Remember to update the string list in zocket.cpp
};

struct Zocket : public ZAddress {
	int zocketStateFlags;
		#define zsOPEN                 (0x80000000)
		#define zsLOOP_REMOTE_CLOSE    (0x40000000)
		#define zsREAD_WRITE_FAIL      (0x20000000)

	long sockFD;
		// File Descriptor, under UNIX you can treat this as any old fd
	long acceptedSockFD;
		// Used for listening sockets to simplify creation

	// Used for the loop protocol...
	char *loopSendBuffer;			
	char *loopSendBufferHead;
	int   loopSendBufferSize;
	#ifndef zlMAX_LOOP_ZOCKETS
		#define zlMAX_LOOP_ZOCKETS (10)
	#endif
	static int loopInitialized;
	static Zocket *loopPendingConnections[zlMAX_LOOP_ZOCKETS];


	// Error handling...
	//  The most useful inforamtion is in the getNativeError()
	//  functions. The getError() which is mopstly just going 
	//  to tell you about misformed addresses, etc.
	ZocketError error;
	int lastErrorCode;


	ZocketError getZocketError() { return error; }
	char *getZocketErrorString() { return zocketErrorStrings[error]; }

	int getNativeError() { return lastErrorCode; }
	char *getNativeErrorString();

	int getSockFD() { return sockFD; }
		// In UNIX these sockets are just ordinary file descriptors
		// God knows what they are under Windows

	int isOpen() { return zocketStateFlags & zsOPEN; }
	
	virtual int isConnected();
		// Has a non-blocking connection been established
		// May be overloaded to set state flags.

	void reset();
		// Internal method for clearing all members
		// Does not close.  Do not call directly unless
		// you are sure you know what you are doing.

	void open( ZAddress zAddr );
	void open( char *zAddr ) { open( ZAddress(zAddr) ); }
		// Zocket addresses are encoded in a string for optimal forward compatibility
		// See zaddress.h for details.

	void close();
		// Closes socket.

	ZAddress getRemoteAddress();
		// Returns the address of the remote side
		// of the connection.

	int setBlocking( int block = 0 );
		// Sets the default blocking state.  This can be overloaded
		// by passing a parameter to read and write.  You can set this
		// on as a socket option in the address (which is required actually
		// if you want to do a non-blocking open.)

	int read ( void *buffer, int bufferSize, int block = -1 );
	int write( void *buffer, int bufferSize, int block = -1 );
	int puts( char *string, int block = -1 );
		// Read and write return the number of bytes written or read
		// except in the case of a closed connection return -1
		// The argument "block" can be:
		//   -1 == no change from default
		// 	  0 == override default, don't block
		// 	  1 == override default, block
		// If the zocket is dgram based, this will send to
		// the address that this zocket was opened as.

	int dgramRead ( void *buffer, int bufferSize, ZAddress &zAddress, int block=-1 );
	int dgramWrite( void *buffer, int bufferSize, ZAddress &zAddress, int block=-1 );
		// These functions are seperate from the read and write because they
		// must make a different call in order to retrieve the zAddress
		// from the sender.  However, if you don't care about this (unlikely for read)
		// then you should be able to just call read and write on a udp socket
		// Same return values as above.

	int accept( int block = -1 );
		// This only works for listening zockets
		// (Those that were created with the 'l' flag).
		// If it finds that a new connection is pending it returns a socketFD.
		// You can either do something with this or just create a new Zocket
		// based on the listening one (which will store this new FD).
		// If no new connection is pending, it returns 0
		// unless error then -1

	Zocket();
	Zocket( int fd );
		// This constructor is used when you already have a file
		// descriptor that should be used as a transceiver

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


	Zocket( char *zAddress, char *protocol=(char*)0, char *options=(char*)0, int port=0 ) {
		zocketStateFlags = 0;
		open( ZAddress(zAddress,protocol,options,port) );
	}

	Zocket( ZAddress zAddr ) { zocketStateFlags = 0; open( zAddr ); }
		// See docs for "open()"

	~Zocket() { close(); }
	virtual void kill() { delete this; }
		// This is just to make Zocket look more like Zacket
		// It isn't really necessary to call this like it is in Zacket
};

extern void zocketFdClr( unsigned int fd, void *set );
extern void zocketFdSet( unsigned int fd, void *set );
extern void zocketFdZero( void *set );
extern int  zocketFdIsSet( unsigned int fd, void *set );

#endif

