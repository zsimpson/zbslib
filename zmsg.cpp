// @ZBS {
//		*MODULE_NAME Hash-based Message Dispatcher
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A hashtable based method of passing messages around.  Allows for
//			a header-free method of passing a message from one module to another
//			via direct and indirect dispatching. Includes features for queing messages
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zmsg.cpp zmsg.h
//		*VERSION 2.0
//		+HISTORY {
//			2.0 New naming convention
//		}
//		+TODO {
//		}
//		*SELF_TEST yes console
//		*PUBLISH yes
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include "stdarg.h"
#include "string.h"
// MODULE includes:
#include "zmsg.h"
// ZBSLIB includes:

//#define TRACEMSG
	// Defining this will trace every message that gets queued, telling you what type, and to,from fields.
	// Very instructive in realizing the huge number of zui messages due to moving, resizing etc...
#ifdef TRACEMSG
	// debug: stuff "tracemsg='blah blah'" into any message and have it traced during dispatch
	#include "zui.h" 
	#include "ztmpstr.h"
	extern void trace( char *msg, ... );
#endif

static char zMsgTempString[4096*4];

ZHashTable *zMsgDispatchInstructions = NULL;
ZMsg *zMsgs[ZMSG_QUEUE_SIZE];
	// TODO: dynamic allocate queue with realloc
int zMsgRead = 0;
int zMsgWrite = 0;
int zMsgDispatching = 0;

void (*zMsgMutex)(int) = 0;

ZMsgRegister::ZMsgRegister( char *typeName, ZMsgHandler handlerPtr ) {
	if( !zMsgDispatchInstructions ) {
		zMsgDispatchInstructions = new ZHashTable();
	}
	zMsgDispatchInstructions->putP( typeName, (void*)handlerPtr );
}

//#define NO_LAYOUTS_QUEUED_DURING_DISPATCH
#ifdef NO_LAYOUTS_QUEUED_DURING_DISPATCH
#define CATCH_ZUILAYOUT(msg) assert( !( zMsgDispatching && !strcmp( msg->getS( "type", "" ), "ZUILayout" ) ) && "ZUILayout queued during a dispatch!" );
	// one should never queue a ZUILayout in the context of a dispatch; ZUILayout typically should
	// result from Hide/Show/Resize type messages; if you are poking "hidden" attribute or some 
	// such and need to force a layout, just ZUI::layoutRequest()
#else
#define CATCH_ZUILAYOUT(msg) 
#endif

char *escapeQuotes( char* string, int escapeBackSlashAlso ) {
	static char escaped[2048];

	assert(strlen(string)<2048-1);

	char *s = string;
	char *d = escaped;
	while( *s && ( s-string < 2048 ) ) {
		if( *s == '\'' || *s == '"' || ( escapeBackSlashAlso && *s == '\\' ) ) {
			*d++ = '\\';
		}
		*d++ = *s++;
	}
	*d = 0;
	return escaped;
}

void zMsgQueue( ZMsg *msg ) {

	CATCH_ZUILAYOUT( msg );

	if( zMsgMutex ) {
		(*zMsgMutex)(1);
	}
	
	#ifdef TRACEMSG
		msg->putS( "tracemsg", ZTmpStr( "%s from=%s to=%s", msg->getS( "type", "Unknown" ), msg->getS( "fromZUI", "?" ), msg->getS( "toZUI", "?" ) ) );
	#endif

	zMsgs[ zMsgWrite ] = msg;
	zMsgWrite = (zMsgWrite + 1) % ZMSG_QUEUE_SIZE;
	msg->putS( "__queued__", "1" );
	static int serialNum = 0;
	msg->putI( "__serial__", serialNum++ );
	assert( zMsgWrite != zMsgRead );

	if( zMsgMutex ) {
		(*zMsgMutex)(0);
	}
}

ZMsg *zMsgQueue( char *msg, ... ) {
	if( zMsgMutex ) {
		(*zMsgMutex)(1);
	}

	{
		va_list argptr;
		va_start( argptr, msg );
		vsprintf( zMsgTempString, msg, argptr );
		va_end( argptr );
		assert( strlen(zMsgTempString) < sizeof(zMsgTempString) );
	}

	// Search for semi-colon separated strings
	// Must be outside of quote contexts
	char *msgStart = zMsgTempString;
	int sQuote = 0, dQuote = 0;

	for( char *c=zMsgTempString; *c; c++ ) {
		if( *c == '\'' && !sQuote ) sQuote++;
		else if( *c == '\'' && sQuote ) sQuote--;
		else if( *c == '\"' && !dQuote ) dQuote++;
		else if( *c == '\"' && dQuote ) dQuote--;
		if( !dQuote && !sQuote && *c == ';' ) {
			*c = 0;

			//zMsgQueue( zHashTable( msgStart ) );
			// Code duplicated from above to avoid double mutex lock
			
			// ZHashTable *hash = zHashTable( msgStart );
				// tfb: the above zHashTable makes use of a static char array to decode
				// via variable args vsprintf: this is not threadsafe!  Instead,
				// just encode directly since we've already handled the var args:
			ZHashTable *hash = new ZHashTable();
			hash->putEncoded( msgStart );
			#ifdef TRACEMSG
			hash->putS( "tracemsg", ZTmpStr( "%s from=%s to=%s", hash->getS( "type", "Unknown" ), hash->getS( "fromZUI", "?" ), hash->getS( "toZUI", "?" ) ) );
			#endif

			CATCH_ZUILAYOUT( hash );

			zMsgs[ zMsgWrite ] = hash;
			zMsgWrite = (zMsgWrite + 1) % ZMSG_QUEUE_SIZE;
			hash->putS( "__queued__", "1" );
			static int serialNum = 0;
			hash->putI( "__serial__", serialNum++ );
			assert( zMsgWrite != zMsgRead );

			msgStart = c+1;
		}
	}


	//zMsgQueue( hash );
	// Code duplicated from above to avoid double mutex lock

	//ZHashTable *hash = zHashTable( msgStart );
		// tfb: the above zHashTable makes use of a static char array to decode
		// via variable args vsprintf: this is not threadsafe!  Instead,
		// just encode directly since we've already handled the var args:
	ZHashTable *hash = new ZHashTable();
	hash->putEncoded( msgStart );
	CATCH_ZUILAYOUT( hash );
	#ifdef TRACEMSG
	hash->putS( "tracemsg", ZTmpStr( "%s from=%s to=%s", hash->getS( "type", "Unknown" ), hash->getS( "fromZUI", "?" ), hash->getS( "toZUI", "?" ) ) );
	#endif

	zMsgs[ zMsgWrite ] = hash;
	zMsgWrite = (zMsgWrite + 1) % ZMSG_QUEUE_SIZE;
	hash->putS( "__queued__", "1" );
	static int serialNum = 0;
	hash->putI( "__serial__", serialNum++ );
	assert( zMsgWrite != zMsgRead );

	if( zMsgMutex ) {
		(*zMsgMutex)(0);
	}

	return hash;
}

ZMsg *zMsgDequeue() {
	if( zMsgMutex ) {
		(*zMsgMutex)(1);
	}

	ZHashTable *q = 0;
	if( zMsgRead != zMsgWrite ) {
		q = zMsgs[ zMsgRead ];
		zMsgRead = (zMsgRead + 1) % ZMSG_QUEUE_SIZE;
		q->del( "__queued__" );
	}

	if( zMsgMutex ) {
		(*zMsgMutex)(0);
	}

	return q;
}

void zMsgSetHandler( char *type, ZMsgHandler d ) {
	if( !zMsgDispatchInstructions ) {
		zMsgDispatchInstructions = new ZHashTable();
	}
	zMsgDispatchInstructions->putP( type, (void*)d );
}

//#include "ztime.h"
//#include "zprof.h"
void zMsgDispatch( double time ) {

	zMsgDispatching++;

	#define ZMSG_REQUEUE_SIZE (64)
	ZMsg *requeue[ZMSG_REQUEUE_SIZE];
	int requeueCount = 0;

	ZMsgHandler defH = (ZMsgHandler)zMsgDispatchInstructions->getp( "default" );

	// To prevent infinite loops, the dispatcher notes the last of
	// the msgs and only goes to that one.  Anything that gets added
	// on to the end of the queue will no be processed until next time.

//ZTime longest = 0.0;
//char longestStr[1024];
//longestStr[0] = 0;

	int zMsgWriteAtStart = zMsgWrite;
	for( ZMsg *msg = zMsgDequeue(); msg;  ) {
		#ifdef TRACEMSG
			char *tracemsg = msg->getS( "tracemsg" );
			if( tracemsg ) {
				int msgsInQueue = ( zMsgWrite - zMsgRead + ZMSG_QUEUE_SIZE ) % ZMSG_QUEUE_SIZE;
				trace( " [%d, %d] %s\n", ZUI::zuiFrameCounter, msgsInQueue, tracemsg );
			}
		#endif
		if( time ) {
			if( zmsgF(__timer__) ) {
				// Convert the timer field into a time field
				msg->putD( "__time__", zmsgF(__timer__) + time );
				msg->putS( "__timer__", (char*)NULL );
			}

			// Using the __time__ field
			float _time = zmsgF(__time__);
			if( time < _time ) {
				// REQUEUE this one
				requeue[requeueCount++] = msg;
				assert( requeueCount < ZMSG_REQUEUE_SIZE );
				msg = zMsgDequeue();
				continue;
			}
		}
		int countDown = zmsgI(__countdown__);
		if( countDown > 0 ) {
			// REQUEUE this one with countdown decremented
			msg->putI( "__countdown__", countDown-1 );
			requeue[requeueCount++] = msg;
			assert( requeueCount < ZMSG_REQUEUE_SIZE );
			msg = zMsgDequeue();
			continue;
		}

//ZTime start= zTimeNow();

		// LOOKUP the dispatching instructions
		char *type = zmsgS(type);
		ZMsgHandler h = (ZMsgHandler)zMsgDispatchInstructions->getp( type );
		if( defH ) {
			(*defH)( msg );
		}
		if( h && !zmsgI(__used__) ) {
			(*h)( msg );
		}

//ZTime duration = zTimeNow() - start;
//if( duration > longest ) {
//	longest = duration;
//	sprintf( longestStr, "%s %s", msg->getS("type"), msg->getS("toZUI") );
//}

		// It is possible that this message has been pushed back
		// into the queue in which case we cannot delete it!
		if( ! zmsgI(__queued__) ) {
			delete msg;
		}

		// FETCH the next one as long as we haven't gotten to the last
		// one as of the start of the function
		msg = zMsgDequeue();
	}

	for( int i=0; i<requeueCount; i++ ) {
		zMsgQueue( requeue[i] );
	}

//zprofAddLine( longestStr );

	zMsgDispatching--;
}

void zMsgDispatchNow( ZMsg *msg ) {
	ZMsgHandler defH = (ZMsgHandler)zMsgDispatchInstructions->getp( "default" );

	// LOOKUP the dispatching instructions
	ZMsgHandler h = (ZMsgHandler)zMsgDispatchInstructions->getp( zmsgS(type) );
	if( defH ) {
		(*defH)( msg );
	}
	if( h && !zmsgI(__used__) ) {
		(*h)( msg );
	}

	// It is possible that this message has been pushed back
	// into the queue in which case we cannot delete it!
	if( ! zmsgI(__queued__) ) {
		delete msg;
	}
}

char zMsgTranslateCharacter( char *character ) {
	if( !strcmp(character,"escape") ) {
		return 27;
	}
	if( !strcmp(character,"quote") ) {
		return '\'';
	}
	if( !strcmp(character,"doublequote") ) {
		return '\"';
	}
	if( !strcmp(character,"backspace") ) {
		return '\b';
	}
	if( !strcmp(character,"tab") ) {
		return '\t';
	}
	return *character;
}


#if SELF_TEST

#define ZMSG_HANDLER( Oink ) {
	printf( "Received type=%s, flags=%d\n", msg->get("type"), zMsgGetInt(msg,"flags") );
}

void main() {
	zMsgQueue( zMsg( "type=Oink flags=1" ) );
	zMsgQueue( zMsg( "type=Oink flags=2" ) );
	zMsgDispatch();
}

#endif
