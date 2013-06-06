// (c) 2000 by Zachary Booth Simpson
// This code may be freely used, modified, and distributed without any license
// as long as the original author is noted in the source file and all
// changes are made clear disclaimer using a "MODIFIED VERSION" disclaimer.
// There is NO warranty and the original author may not be held liable for any damages.
// http://www.totempole.net

#ifndef ZACKET_H
#define ZACKET_H

#include "zocket.h"
#include "hashtable.h"

// Lag profiles, see msgzocket.cpp for details
extern char *startLagOnAllMsgZockets;
extern char globalLagProfile[];
extern char constantLagProfile[];
extern char constantHugeLagProfile[];
extern char tinyLagProfile[];

extern unsigned int zocketGetTime();
	// This is a portable call returning time in milliseconds

struct BaseMsgZocket : public Zocket {
	// MsgZocket -- ZBS 18 March 98
	//
	// This class is a zocket that knows about the concept
	// of a "message."  A stream has no inherent idea
	// of where one message starts and another stops.  This
	// class creates all the logic for sending and recieving known
	// size elements.
	// Additionally, it has the following features:
	//   * Write buffering.  Many small messages can be accumulated
	//     and sent all at once when flush() is called
	//   * Artifical latency can be added using a histogram
	//     to describe the probability function.
	//   * Termination and connection are handled like any other 
	//     messages so everything will get processed in order even
	//     if the socket terminates.
	//   * Has internal support for various timouts
	//   * Maintains a static list of all MsgZockets
	//     to simplify the connection / deconnection logic
	//   * Timing information is kept to simplify timeout logic
	//   * Recording system allows you to capture and 
	//     analyze message traffic.
	//
	// This class is abstract.  A decendant is expected to
	// define the message specific operations such as
	// haveCompleteMsg().  This allows the complex part
	// of this code to be used with any desired protocol
	// which can deterime message size from context.

	int msgZocketFlags;
		#define mzKILLED          (0x01)
		#define mzTERMINATE       (0x02)
		#define mzUNCONNECTED     (0x04)
		#define mzRECVTIMEOUT     (0x08)
		#define mzACTIVITYTIMEOUT (0x10)
		#define mzLOGINTIMEOUT    (0x20)

	int recursion;
		// This is used to track when we are in a
		// state that we can not allow a delete to
		// occur.  kill() checks this.

	// Timeout
	int recvTime;
	int recvTimeout;
		// The last time that any data was received, in uncorreted seconds.

	int activityTime;
	int activityTimeout;
		// The last time that any data was sent or received, in uncorreted seconds.

	int loginTime;
	int loginTimeout;
		// The time when the connection was started. see setLoginTimeout()

	int readBufSize;
	char *readBuf;
	char *readPos;
	int writeBufSize;
	char *writeBuf;
	char *writePos;
		// bufSize is the size of buf in bytes
		// buf is the head of the storage buffer
		// pos is the current r/w head.
		// If buffers overflow, they are realloc'd.

	char *lagProfile;
	int lagProfileSum;
		// Used for artifical lag.  lagProfile is
		// a -1 terminated histogram by 100's of mils
		// sum is the total of the lag histogram.

	ZAddress remoteAddress;
		// The address of the last reception, this is
		// modified by the dispatcher so that it will
		// synchronize with any artifical lag.

	struct MsgCall {
		MsgCall *next;
		char *buf;
		int len;
		double timeToCall;
		ZAddress from;
	} *msgCallHead, *msgCallTail;
		// When artifical lag is added, the messages
		// do not get dispatched immediately.  Instead,
		// they are stuck into this list and
		// each one is dispatched when the time is right.
		// Note, order will always be preserved.

	// There is a global linked list of MsgZockets
	// so that they can all be scanned easily.
	BaseMsgZocket *next;
	BaseMsgZocket *prev;
	static BaseMsgZocket *head;
	static void pollAll();

	virtual void addToMsgZocketList();
		// There is no inherent order to the zocket list
		// by default.  If this is needed, you could overload 
		// this method.  This is called automatically
		// when a zocket is created in init()


	// PURE virtuals
	//--------------------------------------------------------
	virtual int haveCompleteMsg( char *buf, int len )=0;
		// This virtual tells the base class if the buffer
		// pointed to by buf of len contains at least one
		// good message.  If so, it is expected to return the
		// length of the first message.

	virtual void dispatch( char *buf, int len )=0;
		// Given a pointer to a message, dispatch it

	virtual BaseMsgZocket *newConnection()=0;
		// A listening zocket has got a new connection,
		// the decendant class should allocate a new
		// copy of itself and return a pointer to
		// be inserted into the static list.

	virtual void dispatchTermination() { }
		// This is called when the connection has died.
		// Will always be in order with other packets.

	virtual void dispatchConnected() { }
		// This is called when a non-blocking, non-listening 
		// zocket has connected to a server

	// Local Working Methods
	//--------------------------------------------------------
	
	void init();
		// Clears all variables, allocs buffers

	void postMsg( int len, ZAddress from );
		// sticks len bytes of readBuf into the queue
		// of messages to be dispatched.  If lag is off
		// it happens instantly, otherwise it gets buffered.
		// Two special values for len:
		//   -1 is terminate
		//   -2 is connected (a non-blocking connect succeeded)

	void postTermination();
		// calls postMsg with -1 len argument
	void postConnected();
		// calls postMsg with -2 len argument

	void checkDispatch();
		// If lag is on, this will see if its time
		// to dispatch any of the pending messages.


	// Public Working Methods
	//--------------------------------------------------------

	virtual int isConnected();
		// Overloaded so that the mzUNCONNECTED
		// flag can be set.

	virtual void kill();
		// You should always destroy with this method.
		// It will prevent any recursion from occuring
		// by flagging the zocket for death and it
		// will really be deleted on the next poll.

	virtual void requestTermination();
		// By calling this, you are posting a
		// call to terminate which will be dispatched
		// synchronously at the beginning of the next poll()

	virtual void poll( int block=-1 );
		// This checks the zocket for incoming data.
		// It then calls haveCompleteMsg() to find out
		// if there's enough in the buffer to consider
		// one message.  If so, it shifts down the remaining
		// data and then calls postMsg which will either
		// dispatch immediatly or buffer for artificial lag.

	virtual void write( char *buf, int len, int block=-1 );
		// Writes to the zocket.  May buffer if activatied.

	virtual void dgramWrite( char *buf, int len, ZAddress &zAddr, int block=-1 );
		// Writes to the zocket.  May buffer if activatied.
		// It is generally easier not to use this.  If
		// you call write() it will send to whatever address
		// this zocket was opened under.

	//virtual void flush();
		// If buffering is activated, this will send
		// all the messages accumulated thus far.
		// It is guaranteed that the other side will
		// receive all the messages at the same time
		// (An extra header is wrapped around these)

	void setLag( char *_lagProfile = globalLagProfile );
		// Used to create a simulation of latency.
		// Note, once lag is activated, it can't not be deactivated.
		// The profile array is a little probability distribution
		// profile by 100's of mils, terminated by 0xFF
		// Note, simulated lag is only on the receive end.

	void setRecvTimeout( int seconds );
		// If set to non-zero, will cause a TERMINATE if nothing
		// is received on this zocket in *seconds*

	void setActivityTimeout( int seconds );
		// If set to non-zero, will cause a TERMINATE if nothing
		// is sent or received on this zocket in *seconds*

	void setLoginTimeout( int seconds );
		// If set to non-zero, will cause a kill() (not a TERMINATE)
		// if this timer is not cleared in *seconds*
		// This is useful to get rid of connections that attach
		// but never login to identify themselves.
		// After a login has occured, call this with seconds = 0

	virtual ~BaseMsgZocket();

	BaseMsgZocket();
	BaseMsgZocket( char *zAddress, char *protocol=(char*)0, char *options=(char*)0, int port=0 );
	BaseMsgZocket( Zocket *listening );
};

//-------------------------------------------------------------
// The following class implements BaseMsgZocket with
// a very simple messaging structure.  Although this
// class is usable, it is mostly for demonstration
// purposes.  Real implementations would probably integrate
// into a more sophisticated or more highly integrated
// messaging system.
//
// Note the use of a hash table to do dispatching.
// This is a very simple and clean way.  The
// type field of the SimpleZacket is used as a 
// hash key.  There are three special keys:
//  -1 == termination
//  -2 == accepted connection (server)
//  -3 == connected (client)
//
// Also note the "appData" field.  For this example,
// I use this to store the address of a global handle.
// This allows me to use global SimpleMsgZocket pointers for
// writing purposes and have them automatically cleaned
// up when a SimpleMsgZocket dies.  I simply need to
// check the handle for null before I write to it.
//-------------------------------------------------------------

typedef void (*SimpleMsgHandler)(struct SimpleMsgZocket *, struct SimpleMsg *);

struct SimpleMsg {
	unsigned short type;
	unsigned short len;
};

struct SimpleMsgZocket : public BaseMsgZocket {
	static IntTable dispatchHashTable;
		// hash table used for dispatching

	// Implementations of pure methods in BaseMsgZocket...
	virtual int haveCompleteMsg( char *buf, int len );
	virtual void dispatch( char *buf, int len );
	virtual BaseMsgZocket *newConnection();
	virtual void dispatchTermination();
	virtual void dispatchConnected();

	void init();

  public:
	int appData;
		// This is application settable.  For example,
		// I use it to store the address of a global
		// handle which I then reset when it dies.

	SimpleMsgZocket();
	SimpleMsgZocket( char *zAddress, char *protocol=(char*)0, char *options=(char*)0, int port=0 );
	SimpleMsgZocket( Zocket *listening );

	virtual void write( SimpleMsg *z, int block=-1 );
		// Writes to the zocket.  May buffer if activatied.

	virtual void dgramWrite( SimpleMsg *z, ZAddress &zAddr, int block=-1 );
		// Writes to the zocket.  May buffer if activatied.
		// It is generally easier not to use this, but to set
		// the dgramWriteDefault variable instead.  Then
		// you can just call write() and not worry about it.

	static void registerHandler( int type, SimpleMsgHandler handler );
		// Registers a function call back.
		// type == 0 for termination.
};

#endif

