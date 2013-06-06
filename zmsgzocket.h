// @ZBS {
//		*MODULE_OWNER_NAME zmsgzocket
// }

#ifndef ZMSGZOCKET_H
#define ZMSGZOCKET_H

#include "zocket.h"
#include "zmsg.h"

#if !defined(STOPFLOW)

struct ZMsgZocket : Zocket {
	static int listCount;
	static ZMsgZocket **list;
	static int idCounter;

	int id;
		// This is an incrementing id which rolls at 0x80000000
		// so that all the zocket communication can be done on this unique id

	int header[2];
		// @TODO: Make this 32 bit under all compilations
	char *readBuffer;
	int readBufferRemain;
	int readBufferFlags;
	int readBufferOffset;
	int readBufferReadToHTTPEnd;
	int readBufferHTTPAllocSize;
	int readBufferHTTPSize;

	char *sendOnConnect;
	char *sendOnDisconnect;

	void addToList();
		// Called internally by the constructors

	ZMsgZocket();
	ZMsgZocket( char *addresss, int _options=0, char *onConnect=0, char *onDisconnect=0 );
	ZMsgZocket( ZMsgZocket *listeningZocket );
	~ZMsgZocket();

	void setSendOnConnect( char *msg );
	void setSendOnDisconnect( char *msg );
		// If neither of these are set, it will send a "type=Connect fromRemote=id" or "type=Disconnect fromRemote=id"

	int read( ZMsg **retMsg=0, int blockForFullMessage=0 );
		// If retMsg arg is zero then it queues the recieved message into the normal
		// message queue, otherwise it new's up the message and sets the pointer

	void write( ZMsg *msg );
		// Normally messages would be sent via the dispatcher, but in
		// the case that you aren't using the dispatching mechanisms
		// this allows you to write an msg directly

	static void wait( int milliseconds, int includeConsole );
		// Sleep the thread until either network activity on the list
		// or milliseconds occurs.  If includeConsole is set, then
		// it adds the console handle to the activity list.
		// Unfortunately, in windows this isn't possible so
		// it just sets the milliseconds to 100 so that you will get
		// reasonable console response under windows with minimal CPU hit

	static ZMsgZocket *find( int id );
		// Find the ZMsgZocket given the id

	static void readList();
		// Read from all zockets including listening

	static void dispatch( ZMsg *msg );
		// Dispatch messages toRemote

	static void dropId( int id );
		// deletes the id

	static int isIdConnected( int id );

};

#endif

#endif
