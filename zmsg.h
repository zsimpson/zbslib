// @ZBS {
//		*MODULE_OWNER_NAME zmsg
// }

#ifndef ZMSG_H
#define ZMSG_H

#include "zhashtable.h"

typedef ZHashTable ZMsg;
	// Each message is really a hash table that associates 
	// a set of keys and values.  All values are stored
	// as strings.  Numbers in decimal or prefixed with 0x.
	// Use the msgInt(), msgFloat(), msgStr() macros
	// to extract type-specific varibles more easily

typedef void (*ZMsgHandler)( ZMsg *msg );
	// All message handlers are of this type

char *escapeQuotes( char* string, int escapeBackSlashAlso=0 );
	// Often useful when passing text strings which contain quoted words. 
	// Example: zMsgQueue( "type=ZUISet key=text val='%s'", escapeQuotes( "Press the 'Resume' button to continue." ) );
	// Otherwise the quotes within the text will cause problems in zMsgQueue().
	//
	// escapeBlackSlashAlso: also escape instances of the character '\\'
	// This is useful when passing around text strings which contain
	// windows-style paths
	//
	// WARNING: makes use of a single static buffer holding the return value.

// ZMsg Mutex
//--------------------------------------------------------------------

// Sometimes one may want the msg system to be mutexed (if you have
// threads sending messages back and forth).  In which case this
// function ptr needs to be implemented.

extern void (*zMsgMutex)(int);

// ZMsg Dispatching
//--------------------------------------------------------------------

struct ZMsgRegister {
	// This structure is used for auto-initalizing free function
	// message handlers using the macro ZMSG_HANDLER below
	ZMsgRegister( char *typeName, ZMsgHandler handlerPtr );
};

#define ZMSG_HANDLER( type ) \
	void handle##type( ZMsg *msg ); \
	static ZMsgRegister __##type( #type, handle##type ); \
	void handle##type( ZMsg *msg )
	// This is a tricky macro which simultaneously declares a function
	// prototype and registers the function to be called on some
	// message 'type'.  For example, if you want to handle the message:
	//   "type=Quit exitCode=1"
	// You could write:
	//   ZMSG_HANDLER( Test ) {
	//     exit( msgI(exitCode) );
	//   }

extern ZHashTable *zMsgDispatchInstructions;
	// The hash table that stores the association between
	// message types and the functions that handle them
	// Use the previous macros to set this, or do it manually

// ZMsg functions and macrs for reading messages
//--------------------------------------------------------------------

// These macros assume are used for handlers where
// the naming convention (enforced by using ZMSG_HANDLER) says
// that the name of the ZMsg * argument to a handler is called "msg"
// Example:
//   ZMSG_HANDLER( Quit ) {
//     if( zmsgIs(which,now) && zmsgI(code)==1 ) {
//       do something

#define zmsgS(a) (msg->getS(#a,""))
#define zmsgI(a) (msg->getI(#a,0))
#define zmsgF(a) (msg->getF(#a,0.f))
#define zmsgD(a) (msg->getD(#a,0.0))
#define zmsgIs(a,b) (!strcmp(msg->getS(#a,""),#b))
#define zmsgHas(a) (msg->getS(#a,0)!=0)

// Use this macro when you want to mark the message as used so
// that it won't propagate to other things.
#define zMsgUsed() msg->putS("__used__","1")
#define zMsgIsUsed() msg->getI("__used__")

// ZMsg Queue
//--------------------------------------------------------------------
#define ZMSG_QUEUE_SIZE (8192)
extern ZMsg *zMsgs[ZMSG_QUEUE_SIZE];
extern int zMsgCount;
extern int zMsgAlloc;

ZMsg *zMsgDequeue();
	// Usually called by zMsgDispatch().  Not normally used in application code.

void zMsgQueue( ZMsg *msg );
	// Usually called by void zMsgQueue( char *msg, ... ).  Not normally used in application code.
	// Returns a pointer to the msg in case you want to stuff values directly into it

ZMsg *zMsgQueue( char *msg, ... );
	// This last function is the one that would typically be used.

void zMsgDispatch( double time=0.0 );
	// Dequeues messages one at a time and sends them to associated handlers
	// You can choose to use the time system by passing time in here
	// If you do so, only messages posted with the key="__time__" at less than time will dispatch

void zMsgDispatchNow( ZMsg *msg );
	// Dispatches the msg immediately.  Does not queue or dequeue it.
	// Immediate chained msgs are not allowed.  I.e. this message can
	// not result in more messages (it can, but they qill be queued, not
	// executed immediately.)
	// The pointer MUST be allocated on the heap and will
	// be deleted automatically unless it is pushed back on the queue

void zMsgSetHandler( char *type, ZMsgHandler d );
	// Set where to send messages that don't have a specific handler
	// Note that the type "default" is special.  It indicates
	// where you want messages which don't match to go

char zMsgTranslateCharacter( char *character );
	// Traslate a character passed by a Key message into a character
	// This deals with all the weird escapes like tab, quote, etc.
	// See zglWin.cpp for details on the mapping

#endif


