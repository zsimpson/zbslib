// ZBSDISCLAIM

#ifdef WIN32
	#include "windows.h"
#endif
#include "stdio.h"
#include "stdlib.h"
#include "memory.h"
#include "string.h"
#include "assert.h"
#include "msgzocket.h"


// Lag Profiles
//------------------------------------------------------------

char *startLagOnAllMsgZockets = NULL;
	// Set this to point to any of the below
	// and all zockets will from then on have this
	// latency profile.  Note, lag can not be turned off.

char globalLagProfile[] = { 0, 4, 12, 6, 2, 1, 1, 1, 1, -1 };
	// I'm totally guessing at this profile from intuition
	// getting a real profile will take some serious profiling
	// under realistic TCP conditions.  Also, it will shift
	// significantly under a modem vs. LAN->WAN 

char constantLagProfile[] = { 0, 0, 0, 0, 1, -1 };
	// This is a constant profile that's good for testing cases
	// where you need good reproducibility

char constantHugeLagProfile[] = { 0, 0, 0, 0, 0, 0, 0, 1, -1 };
	// This is a constant profile that's good for testing cases
	// where you need good reproducibility.  This one is huge

char tinyLagProfile[] = { 1, -1 };
	// This is used when you want just enough latency so that packets
	// will get delayed to the next couple of frames.
	// Useful to ensure that all handler functions are 
	// called from the same place in the main loop


// BaseMsgZocket
//------------------------------------------------------------

unsigned int zocketGetTime() {
	#ifdef WIN32
		return (unsigned int )timeGetTime();
	#else
		// Untested:
		struct timeval tv;
		struct timezone tz;
		gettimeofday( &tv, &tz );
		return (unsigned int )(tv.tv_sec*1000 + tv.tv_usec/1000);
	#endif
}


BaseMsgZocket *BaseMsgZocket::head = NULL;

void BaseMsgZocket::pollAll() {
	BaseMsgZocket *next = NULL;
	for( BaseMsgZocket *i=head; i; i = next ) {
		next = i->next;
		i->poll();
	}
}

void BaseMsgZocket::addToMsgZocketList() {
	prev = NULL;
	next = head;
	if( head ) {
		head->prev = this;
	}
	head = this;
}

void BaseMsgZocket::postMsg( int len, ZAddress from ) {
	if( lagProfile ) {
		// Derive a random lag based on the profile
		// by picking a number and then walking through
		// the histogram until we exceed that sum
		int r = rand() % lagProfileSum;
		int s = 0;
		int lagMils = 0;
		for( int i=0; ; i++ ) {
			s += lagProfile[i];
			if( s >= r ) {
				lagMils = i * 100;
				break;
			}
		}

		// Creatge a new MsgCall structure which
		// keeps track of when we should call this
		MsgCall *msgCall = new MsgCall;
		msgCall->next = NULL;
		msgCall->from = from;
		msgCall->timeToCall = ((double)zocketGetTime() + (double)lagMils) / (double)1000.0;
		msgCall->len = len;
		if( len > 0 ) {
			msgCall->buf  = (char *)malloc( len );
			memcpy( msgCall->buf, readBuf, len );
		}
		if( msgCallTail ) {
			msgCallTail->next = msgCall;
		}
		msgCallTail = msgCall;
		if( !msgCallHead ) {
			msgCallHead = msgCall;
		}
	}
	else {
		if( len == -1 ) dispatchTermination();
		else if( len == -2 ) dispatchConnected();
		else if( len > 0 ) dispatch( readBuf, len );
	}
}

void BaseMsgZocket::checkDispatch() {
	if( lagProfile ) {
		MsgCall *next;
		double localTime = (double)zocketGetTime() / 1000.0;
		for( MsgCall *i = msgCallHead; i; i = next ) {
			// dispatch all messages that are ready to go.
			if( localTime >= i->timeToCall ) {
				if( i->len == -1 ) {
					dispatchTermination();
				}
				else if( i->len == -2 ) {
					dispatchConnected();
				}
				else {
					if( getFlags() & zoDGRAMBASED ) {
						remoteAddress = i->from;
					}
					dispatch( i->buf, i->len );
				}
				if( i->buf ) {
					free( i->buf );
				}
				next = i->next;
				if( msgCallHead == i ) {
					msgCallHead = i->next;
				}
				if( msgCallTail == i ) {
					msgCallTail = NULL;
				}
				delete i;
			}
			else {
				// Oreder must be presereved, so one
				// delayed message stalls all...
				break;
			}
		}
	}
}

static ZAddress nullAddress;
void BaseMsgZocket::postTermination() {
	postMsg(-1,nullAddress);
}
void BaseMsgZocket::postConnected() {
	postMsg(-2,nullAddress);
}

void BaseMsgZocket::poll( int block ) {
	#define TERMINATE postTermination(); kill(); recursion--; return;

	if( msgZocketFlags & mzKILLED ) {
		// The only legal way for msgzockets to die
		// is for the "kill" method to be called.
		// This flags it for death and it is actually
		// killed here.  This prevents recursion in
		// the dispatcher.
		delete this;
		return;
	}

	recursion++;

	// Check for timeouts
	//--------------------------------------------
	int time = zocketGetTime() / 1000;
	if( msgZocketFlags & mzLOGINTIMEOUT ) {
		// Check for timeout on unloggedin zockets
		if( time - loginTime > loginTimeout ) {
			recursion--;
			kill();
			return;
		}
	}

	if( msgZocketFlags & mzRECVTIMEOUT ) {
		if( time - recvTime > recvTimeout ) {
			TERMINATE;
		}
	}

	if( msgZocketFlags & mzACTIVITYTIMEOUT ) {
		if( time - activityTime > activityTimeout ) {
			TERMINATE;
		}
	}

	if( msgZocketFlags & mzTERMINATE ) {
		TERMINATE;
	}

	// Check listening zockets for new connection
	if( isListening() ) {
		int _accept = accept( block );
		if( _accept < 0 ) {
			TERMINATE;
		}
		if( _accept > 0 ) {
			BaseMsgZocket *z = newConnection();
			z->activityTime = 
			z->recvTime = 
			z->loginTime = zocketGetTime() / 1000;
		}
		recursion--;
		return;
	}

	if( startLagOnAllMsgZockets && !lagProfile ) {
		// A request to start latency on all msgzockets has occured,
		// set up the latency buffers for this msgzocket
		setLag( startLagOnAllMsgZockets );
	}

	checkDispatch();
		// First, check for any simulated lag buffered 
		// packets that are waiting to be dispatched.

	if( msgZocketFlags & mzKILLED ) {
		// checkDispatch() could have kill the zocket
		recursion--;
		kill();
		return;
	}

	if( (msgZocketFlags & mzUNCONNECTED) && (getFlags() & zoSTREAMBASED) ) {
		// If still unconnected, check for
		// the establishment of a connection.
		int isCon = isConnected();
		if( isCon < 0 ) {
			TERMINATE;
		}
		else if( isCon ) {
			msgZocketFlags &= ~mzUNCONNECTED;
			if( getFlags() & zoSTREAMBASED ) {
				remoteAddress = Zocket::getRemoteAddress();
				postConnected();
			}
		}
	}

	// Check for overflow of read buffer, realloc if nec.
	int bufUsed   = readPos - readBuf;
	int bufUnused = readBufSize - bufUsed;
	if( bufUnused == 0 ) {
		readBufSize *= 2;
		readBuf = (char *)realloc( readBuf, readBufSize );
		readPos = readBuf + bufUsed;
		bufUnused = readBufSize - bufUsed;
	}

	// Read up to the remaining size of the read buffer
	int _read;
	if( getFlags() & zoSTREAMBASED ) {
		_read = read( readPos, bufUnused, block );
	}
	else {
		_read = dgramRead( readPos, bufUnused, remoteAddress, block );
	}

	if( _read < 0 ) {
		TERMINATE;
	}
	if( _read > 0 ) {
		activityTime = recvTime = zocketGetTime() / 1000;
		readPos   += _read;
		bufUsed   += _read;
		bufUnused -= _read;
	}

	// TODO: Add buffering decode

	int _msgLen;
	while( 
		!(msgZocketFlags & (mzKILLED|mzTERMINATE)) &&
		(_msgLen = haveCompleteMsg( readBuf, readPos - readBuf ))
	) {
		// We have recieved a complete message,
		// post it to the internal tracker and
		// shift the remaining contents down.
		postMsg( _msgLen, remoteAddress );
		memmove( readBuf, &readBuf[_msgLen], bufUsed - _msgLen );
		readPos -= _msgLen;
		bufUsed -= _msgLen;
	}

	recursion--;
}

void BaseMsgZocket::write( char *buf, int len, int block ) {
	if( msgZocketFlags & (mzKILLED|mzTERMINATE) ) {
		// You can't send if unconnected
		return;
	}
	if(
		(getFlags() & zoSTREAMBASED) && 
		(msgZocketFlags & mzUNCONNECTED)
	) {
		return;
	}

	// TODO: Add buffering encode
	int _write = Zocket::write( buf, len, block );
	if( _write < len ) {
		postTermination();
	}
	activityTime = zocketGetTime() / 1000;
}

void BaseMsgZocket::dgramWrite( char *buf, int len, ZAddress &zAddr, int block ) {
	if( msgZocketFlags & (mzKILLED|mzTERMINATE) ) {
		// You can't send if unconnected
		return;
	}
	if(
		(getFlags() & zoSTREAMBASED) && 
		(msgZocketFlags & mzUNCONNECTED)
	) {
		return;
	}

	// TODO: Add buffering encode
	int _write = Zocket::dgramWrite( buf, len, zAddr, block );
	if( _write < len ) {
		postTermination();
	}
	activityTime = zocketGetTime() / 1000;
}

void BaseMsgZocket::setLag( char *_lagProfile ) {
	// Analyze the lag profile array, 
	// which is a -1 terminated string
	lagProfile = _lagProfile;
	lagProfileSum = 0;
	char *c = _lagProfile;
	int i;
	for( i=0; i<50 && *c!=-1; i++, c++ ) {
		lagProfileSum += (int)*c;
	}
	assert( i<50 );
}

BaseMsgZocket::~BaseMsgZocket() {
	assert( msgZocketFlags & mzKILLED );
		// You must use the kill() method to
		// destroy a msgzocket so that you can't
		// get into a bad recursive state.

	if( readBuf ) {
		delete readBuf;
	}
	if( writeBuf ) {
		delete writeBuf;
	}

	MsgCall *_next;
	for( MsgCall *i=msgCallHead; i; i=_next ) {
		_next = i->next;
		if( i->buf ) {
			free( i->buf );
			delete i;
		}
	}

	if( prev ) {
		prev->next = next;
	}
	if( next ) {
		next->prev = prev;
	}
	if( head == this ) {
		head = next;
	}
}

int BaseMsgZocket::isConnected() {
	int c = Zocket::isConnected();
	if( c ) {
		msgZocketFlags &= ~mzUNCONNECTED;
	}
	return c;
}

void BaseMsgZocket::kill() {
	msgZocketFlags |= mzKILLED;
	if( !recursion ) {
		delete this;
	}
}

void BaseMsgZocket::requestTermination() {
	msgZocketFlags |= mzTERMINATE;
}

void BaseMsgZocket::setRecvTimeout( int seconds ) {
	if( seconds == 0 ) msgZocketFlags &= ~mzRECVTIMEOUT;
	else msgZocketFlags |= mzRECVTIMEOUT;
	recvTimeout = seconds;
}

void BaseMsgZocket::setActivityTimeout( int seconds ) {
	if( seconds == 0 ) msgZocketFlags &= ~mzACTIVITYTIMEOUT;
	else msgZocketFlags |= mzACTIVITYTIMEOUT;
	activityTimeout = seconds;
}

void BaseMsgZocket::setLoginTimeout( int seconds ) {
	if( seconds == 0 ) msgZocketFlags &= ~mzLOGINTIMEOUT;
	else msgZocketFlags |= mzLOGINTIMEOUT;
	loginTimeout = seconds;
}

void BaseMsgZocket::init() {
	msgZocketFlags = 0;
	recursion = 0;
	recvTime = recvTimeout = 0;
	activityTime = activityTimeout = 0;
	loginTime = loginTimeout = 0;
	readBufSize = 1024;
	readBuf = (char *)malloc( readBufSize );
	readPos = readBuf;
	writeBufSize = 1024;
	writeBuf = (char *)malloc( writeBufSize );
	writePos = writeBuf;
	lagProfile = NULL;
	lagProfileSum = 0;
	msgCallHead = msgCallTail = NULL;
	next = prev = NULL;
	addToMsgZocketList();
	if( isNonBlocking() && !isListening() ) {
		msgZocketFlags |= mzUNCONNECTED;
	}
}

BaseMsgZocket::BaseMsgZocket() {
	init();
}

BaseMsgZocket::BaseMsgZocket( char *zAddress, char *protocol, char *options, int port )
 : Zocket( zAddress, protocol, options, port ) {
	init();
}

BaseMsgZocket::BaseMsgZocket( Zocket *listening )
 : Zocket( listening )
{
	init();
}


// SimpleMsgZocket
// Implement BaseMsgZocket pure virtuals using the
// messaging system designed for demonhunter
//------------------------------------------------------------

IntTable SimpleMsgZocket::dispatchHashTable;

int SimpleMsgZocket::haveCompleteMsg( char *buf, int len ) {
	// If there's not at least a simple header's
	// worth, then we can't determine the size.
	if( len >= sizeof(SimpleMsg) ) {
		int _len = ((SimpleMsg *)buf)->len;
		if( len >= _len ) {
			return _len;
		}
	}
	return 0;
}

void SimpleMsgZocket::dispatch( char *buf, int len ) {
	SimpleMsg *z = (SimpleMsg *)buf;
	assert( len == z->len );
	char numbuf[10];
	sprintf( numbuf, "%d", z->type );
	int ptr = dispatchHashTable.getInt( numbuf );
	if( ptr ) {
		SimpleMsgHandler handler = (SimpleMsgHandler)ptr;
		(*handler)( this, z );
	}
}

void SimpleMsgZocket::dispatchTermination() {
	int ptr = dispatchHashTable.getInt( "-1" );
	if( ptr ) {
		SimpleMsgHandler handler = (SimpleMsgHandler)ptr;
		(*handler)( this, NULL );
	}
}

BaseMsgZocket *SimpleMsgZocket::newConnection() {
	SimpleMsgZocket *z = new SimpleMsgZocket( this );
	int ptr = dispatchHashTable.getInt( "-2" );
	if( ptr ) {
		SimpleMsgHandler handler = (SimpleMsgHandler)ptr;
		(*handler)( z, NULL );
	}
	return z;
}

void SimpleMsgZocket::dispatchConnected() {
	int ptr = dispatchHashTable.getInt( "-3" );
	if( ptr ) {
		SimpleMsgHandler handler = (SimpleMsgHandler)ptr;
		(*handler)( this, NULL );
	}
}

void SimpleMsgZocket::write( SimpleMsg *z, int block ) {
	BaseMsgZocket::write( (char *)z, z->len, block );
}

void SimpleMsgZocket::dgramWrite( SimpleMsg *z, ZAddress &zAddr, int block ) {
	BaseMsgZocket::dgramWrite( (char *)z, z->len, zAddr, block );
}

void SimpleMsgZocket::registerHandler( int type, SimpleMsgHandler handler ) {
	char buf[10];
	sprintf( buf, "%d", type );
	dispatchHashTable.set( buf, (int)handler );
}

void SimpleMsgZocket::init() {
	appData = 0;
}

SimpleMsgZocket::SimpleMsgZocket() : BaseMsgZocket() {
	init();
}

SimpleMsgZocket::SimpleMsgZocket( char *zAddress, char *protocol, char *options, int port )
 : BaseMsgZocket( zAddress, protocol, options, port )
{
	init();
}

SimpleMsgZocket::SimpleMsgZocket( Zocket *listening )
 : BaseMsgZocket( listening )
{
	init();
}

