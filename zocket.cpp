// @ZBS {
//		*MODULE_NAME Socket Wrapper
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A portable, simple, flexible socket fascade. Uses string based addressing
//			for greater simplicity, portability.
//		}
//		+EXAMPLE {
//			Zocket z( "tcp://127.0.0.1:80" );
//			if( z.isOpen() ) ...
//		}
//		*PORTABILITY win32 unix macosx
//		*WIN32_LIBS_DEBUG wsock32.lib
//		*WIN32_LIBS_RELEASE wsock32.lib
//		*REQUIRED_FILES zocket.cpp zocket.h
//		*VERSION 3.0
//		+HISTORY {
//			1996 Original version was IP and IPX compatible
//			2008 Massive revision to simplify and eliminate IPX.
//               Eliminated LOOPBACK because all machines have network stacks now.
//		}
//		+TODO {
//		}
//		*SELF_TEST yes console ZHASHTABLE_SELF_TEST
//		*PUBLISH yes
// }

#if !defined(STOPFLOW)

// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "stdio.h"
#include "string.h"
// MODULE includes:
// ZBSLIB includes:

#ifdef WIN32
	#include "assert.h"
	#include "windows.h"
	#include "zocket.h"

	#define SOCKET_CLOSE closesocket
	#define SOCKET_IOCTL ioctlsocket
	#define	SOCKET_LAST_ERROR WSAGetLastError()
	#define SOCKET_WOULD_BLOCK (SOCKET_LAST_ERROR == WSAEWOULDBLOCK)
	typedef int socklen_t;
	int wsaStartupInit = 0;
	WSAData wsaData;
#else
	#include "signal.h"
	#include "netdb.h"
	#include "errno.h"
	#include "sys/ioctl.h"
	#include "netinet/tcp.h"
	#include "stdlib.h" 
	#include "assert.h"
	#include "unistd.h"
	#include "zocket.h"

	#define SOCKET_CLOSE ::close
	#define SOCKET_IOCTL ::ioctl
	#define	SOCKET_LAST_ERROR errno
	#define SOCKET_WOULD_BLOCK (SOCKET_LAST_ERROR == EINPROGRESS || SOCKET_LAST_ERROR == EAGAIN)
#endif


void ZocketAddress::clear() {
	address.sin_family = 2;
		// 2 = IP protocol
	address.sin_port = 0;
	address.sin_addr.s_addr = 0;
	memset( address.sin_zero, 0, sizeof(address.sin_zero) );
}

ZocketAddress::ZocketAddress() {
	clear();
}

void ZocketAddress::decode( char *strAddress ) {
	// Parse a string in the form "tcp://www.oink.com:50000"

	address.sin_port = 0;
	address.sin_addr.s_addr = 0;
	memset( address.sin_zero, 0, sizeof(address.sin_zero) );
	protocol = 0;

	char *dup = strdup( strAddress );
	char *c = dup;
	char *name = 0;

	#ifdef __USE_GNU
		#define stricmp strcasecmp
	#endif

	// LOOKING for '://'
	char *colon1 = strstr( dup, "://" );
	if( colon1 ) {
		*colon1 = 0;
		if( !stricmp(c,"tcp") ) {
			protocol = ZO_TCP;
		}
		else if( !stricmp(c,"udp") ) {
			protocol = ZO_UDP;
		}

		if( colon1[1] == '/' && colon1[2] == '/' ) {
			char *colon2 = strchr( colon1+3, ':' );
			if( colon2 ) {
				address.sin_port = htons( atoi( colon2+1 ) );
				*colon2 = 0;
				name = colon1+3;
			}
		}
	}
	else {
		name = dup;
		char *colon2 = strchr( name, ':' );
		if( colon2 ) {
			address.sin_port = htons( atoi( colon2+1 ) );
			*colon2 = 0;
		}
	}

	// CONVERT from dotted decimal. If it isn't then DNS lookup
	if( name ) {
		int dottedDecimal = 0;
		char *dupName = strdup( name );
		char *dot1 = strchr( dupName, '.' );
		if( dot1 ) {
			*dot1 = 0;
			char *dot2 = strchr( dot1+1, '.' );
			if( dot2 ) {
				*dot2 = 0;
				char *dot3 = strchr( dot2+1, '.' );
				if( dot3 ) {
					*dot3 = 0;
					char *end0, *end1, *end2, *end3;
					int _b0 = strtol( dupName, &end0, 10 );
					int _b1 = strtol( dot1+1, &end1, 10 );
					int _b2 = strtol( dot2+1, &end2, 10 );
					int _b3 = strtol( dot3+1, &end3, 10 );
					if( !*end0 && !*end1 && !*end2 && !*end3 ) {
						// Converted ok
						((unsigned char *)&address.sin_addr.s_addr)[0] = _b0;
						((unsigned char *)&address.sin_addr.s_addr)[1] = _b1;
						((unsigned char *)&address.sin_addr.s_addr)[2] = _b2;
						((unsigned char *)&address.sin_addr.s_addr)[3] = _b3;
						dottedDecimal = 1;
					}
				}
			}
		}
		if( !dottedDecimal ) {
			#ifdef WIN32
				// Startup the annoying windows crap only if we haven't do it before
				// and if this isn't a zlocal loopback socket
				if( !wsaStartupInit ) {
					if( WSAStartup(0x0101, &wsaData) ) {
						assert( ! "Unable to start WSAStartup" );
					}
					wsaStartupInit = 1;
				}
			#endif

			// LOOKUP DNS. This may block for a long time.
			struct hostent *hostEnt = gethostbyname( name );
			if( hostEnt ) {
				((unsigned char *)&address.sin_addr.s_addr)[0] = (unsigned char)( (*(unsigned long*)hostEnt->h_addr_list[0] & 0x000000FF) >> 0 );
				((unsigned char *)&address.sin_addr.s_addr)[1] = (unsigned char)( (*(unsigned long*)hostEnt->h_addr_list[0] & 0x0000FF00) >> 8 );
				((unsigned char *)&address.sin_addr.s_addr)[2] = (unsigned char)( (*(unsigned long*)hostEnt->h_addr_list[0] & 0x00FF0000) >> 16 );
				((unsigned char *)&address.sin_addr.s_addr)[3] = (unsigned char)( (*(unsigned long*)hostEnt->h_addr_list[0] & 0xFF000000) >> 24 );
			}
		}

		free( dupName );
	}

	free( dup );
}

char *ZocketAddress::encode() {
	static char buf[128];
	unsigned char b0 = ((unsigned char *)&address.sin_addr.s_addr)[0];
	unsigned char b1 = ((unsigned char *)&address.sin_addr.s_addr)[1];
	unsigned char b2 = ((unsigned char *)&address.sin_addr.s_addr)[2];
	unsigned char b3 = ((unsigned char *)&address.sin_addr.s_addr)[3];
	sprintf( buf, "%s://%d.%d.%d.%d:%d", protocol==ZO_TCP?"tcp":(protocol==ZO_UDP?"udp":""), b0, b1, b2, b3, ntohs(address.sin_port) );
	return buf;
}

char *ZocketAddress::reverseLookup() {
	static char buf[128];

	buf[0] = 0;
	struct hostent *host = gethostbyaddr( (const char *)&((struct sockaddr_in *)this)->sin_addr.s_addr, 4, 2 );
	if( host ) {
		assert( strlen(host->h_name) < sizeof(buf) );
		strcpy( buf, host->h_name );
	}
	return buf;
}

// Zocket
//-----------------------------------------------------------------------------

void Zocket::init() {
	zocketAddress.clear();
	sockFD = SOCKFD_NUL;
	acceptedFD = 0;
	options = 0;
	state = 0;
	error = 0;
}

Zocket::~Zocket() {
	close();
}

Zocket::Zocket() {
	init();
}


Zocket::Zocket( Zocket *listeningZocket ) {
	init();
	zocketAddress = listeningZocket->zocketAddress;
	options = listeningZocket->options;
	options &= ~ZO_LISTENING;
	error = listeningZocket->error;
	state = ZO_OPEN;
	sockFD = listeningZocket->acceptedFD;
	acceptedFD = SOCKFD_NUL;
}

void Zocket::open( ZocketAddress zAddress, int _options ) {
	init();
	error = 0;
	#define OPEN_ERROR() error = SOCKET_LAST_ERROR; sockFD != SOCKFD_NUL ? SOCKET_CLOSE( sockFD ) : 0; sockFD = SOCKFD_NUL;

	// Setup Windows and UNIX static environments
	//----------------------------------------------
	#ifdef WIN32
		// Startup the annoying windows crap only if we haven't do it before
		// and if this isn't a zlocal loopback socket
		if( !wsaStartupInit ) {
			if( WSAStartup(0x0101, &wsaData) ) {
				assert( ! "Unable to start WSAStartup" );
			}
			wsaStartupInit = 1;
		}
	#else
		static int sigPipeInit = 0;
		if( !sigPipeInit ) {
			sigPipeInit = 1;
			signal( SIGPIPE, SIG_IGN );
				// Under UNIX, a SIGPIPE signal is generated
				// when we attempt to write to a non-blocking socket
				// which has been closed down on the remote side.
				// Normally, this causes an application terminate,
				// but we choose to ignore the message and handle it
				// on the next send().
		}
	#endif

	assert( state == ZO_CLOSED );

	sockFD = SOCKFD_NUL;
	acceptedFD = SOCKFD_NUL;

	// Use the protocol from the address unless explicitly overwritten by _options
	options = _options & ~(ZO_TCP|ZO_UDP);
	if( _options & (ZO_TCP|ZO_UDP) ) {
		options |= _options;
	}
	else {
		options |= zAddress.protocol;
	}

	error  = 0;

	sockFD = socket( 
		2, 
		(options&ZO_TCP) ? SOCK_STREAM : SOCK_DGRAM, 
		(options&ZO_TCP) ? IPPROTO_TCP : IPPROTO_UDP
	);
	if( sockFD == SOCKFD_NUL ) {
		OPEN_ERROR();
		return;
	}

	if( options & ZO_UDP ) {
		// FIX bug in Windows 95 scoket implementation per MSDN instructions
		int t = 1;
		int ret = setsockopt( sockFD, SOL_SOCKET, SO_BROADCAST, (const char *)&t, sizeof(int) );
		if( ret != 0 ) {
			OPEN_ERROR();
			return;
		}
	}

	if( options & ZO_DISABLE_NAGLE ) {
		int t = 1;
		int ret = setsockopt( sockFD, IPPROTO_TCP, TCP_NODELAY, (const char *)&t, sizeof(int) );
		if( ret != 0 ) {
			OPEN_ERROR();
			return;
		}
	}

	// SET reuse address
	int t = 1;
	int ret = setsockopt( sockFD, SOL_SOCKET, SO_REUSEADDR, (const char *)&t, sizeof(int) );
	if( ret != 0 ) {
		OPEN_ERROR();
		return;
	}

	unsigned long nonblocking = (options & ZO_NONBLOCKING) ? 1 : 0;
	int result = SOCKET_IOCTL( sockFD, FIONBIO, &nonblocking );
	if( result != 0 ) {
		OPEN_ERROR();
		return;
	}

	if( !(options & ZO_LISTENING) && (options & ZO_TCP) ) {
		int _connect = connect( sockFD, (struct sockaddr *)&zAddress, sizeof(struct sockaddr) );
		
		#ifdef WIN32
			// I discovered this bug 1 Jun 2007.  If I didn't sleep here
			// then half the time it would fail to connect.  Very odd.
			// On 8 mar 2008 I found it had to be even bigger: 100 mils!
			Sleep( 100 );
		#endif

		if( _connect < 0 ) {
			if( SOCKET_WOULD_BLOCK ) {
				// This was set as a non-blocking socket, so the connection is
				// in progress.  This is a valid state, so fall through.
				// The socket will be marked as open, however, it will only
				// really be open when a selection on write succeeds.
				state |= ZO_OPEN_WOULD_BLOCK;
			}
			else {
				OPEN_ERROR();
				return;
			}
		}
	}
	else {
		// Listening or UDP

		ZocketAddress _zAddress = zAddress;
		if( options & ZO_UDP ) {
			_zAddress.address.sin_addr.s_addr = 0;
		}
		int _bind = bind( sockFD, (struct sockaddr *)&_zAddress, sizeof(_zAddress) );
		if( _bind == SOCKFD_NUL ) {
			OPEN_ERROR();
			return;
		}

		if( options & ZO_TCP ) {
			int _listen = listen( sockFD, 4 );
			if( _listen == SOCKFD_NUL ) {
				OPEN_ERROR();
				return;
			}
		}
	}

	state = ZO_OPEN;
}

void Zocket::open( char *address, int _options ) {
	ZocketAddress zAddress;
	zAddress.decode( address );
	open( zAddress, _options );
}

void Zocket::open( char *address, int port, int _options ) {
	ZocketAddress zAddress;
	zAddress.decode( address );
	zAddress.address.sin_port = htons(port);
	open( zAddress, _options );
}

void Zocket::close() {
	if( state != ZO_CLOSED ) {
		if( sockFD != SOCKFD_NUL ) {
			SOCKET_CLOSE( sockFD );
			sockFD = SOCKFD_NUL;
		}
	}
	error = 0;
	state = ZO_CLOSED;
}

int Zocket::isListening() {
	return (options & ZO_LISTENING) ? 1 : 0;
}

int Zocket::isConnected() {
	if( (options & ZO_UDP) || !(state & ZO_OPEN) ) {
		return 0;
	}

	fd_set writeSet;
	FD_ZERO( &writeSet );
	FD_SET( (unsigned int)sockFD, &writeSet );

	fd_set exceptSet;
	FD_ZERO( &exceptSet );
	FD_SET( (unsigned int)sockFD, &exceptSet );

	struct timeval nullTime = {0,0};
	int _select = select( sockFD+1, NULL, &writeSet, &exceptSet, &nullTime );
	if( _select < 0 ) {
		close();
		return 0;
	}

	if( FD_ISSET(sockFD, &exceptSet) ) {
		close();
		return 0;
	}

	int writeIsSet = FD_ISSET( sockFD, &writeSet );
	if( writeIsSet ) {
		// The connect is now officially open
		state &= ~ZO_OPEN_WOULD_BLOCK;
	}
	return writeIsSet ? 1 : 0;
}

int Zocket::isOpen() {
	return (state & ZO_OPEN) && sockFD != SOCKFD_NUL;
}

int Zocket::blockingOverride( int block ) {
	if( block>0 && (options & ZO_NONBLOCKING) ) {
		unsigned long nonblocking = 0;
		SOCKET_IOCTL( sockFD, FIONBIO, &nonblocking );
		options &= ~ZO_NONBLOCKING;
		return 0;
	}
	else if( block==0 && !(options & ZO_NONBLOCKING) ) {
		unsigned long nonblocking = 1;
		SOCKET_IOCTL( sockFD, FIONBIO, &nonblocking );
		options |= ZO_NONBLOCKING;
		return 1;
	}
	return -1;
}

int Zocket::accept( int block ) {
	if( !(options & ZO_LISTENING) ) {
		return -1;
	}

	int reset = blockingOverride( block );

	struct sockaddr_in sockAddr;
	memset( &sockAddr, 0, sizeof(sockAddr) );
	socklen_t a = sizeof(sockAddr);
	acceptedFD = ::accept( sockFD, (struct sockaddr *)&sockAddr, &a );
	if( acceptedFD < 0 ) {
		if( SOCKET_WOULD_BLOCK ) {
			acceptedFD = 0;
		}
		else {
			error = SOCKET_LAST_ERROR;
			state = ZO_ERROR;
			acceptedFD = SOCKFD_NUL;
		}
	}

	blockingOverride( reset );

	return acceptedFD;
}

int Zocket::read( void *buffer, int bufferSize, int block ) {
	int reset = blockingOverride( block );

	int _recv = recv( sockFD, (char *)buffer, bufferSize, 0 );

	// There's a difference between windows and unix here
	// On a remote close, the recv function returns 0 on both.
	// However, on windows the SOCKET_WOULD_BLOCK call
	// fails and on unix it passes.  So, we have to check carefully
	// here for a _recv equal to zero to detect a remote close

	if( _recv == 0 ) {
		error = SOCKET_LAST_ERROR;
		state = ZO_ERROR;
		_recv = -1;
	}
	else if( _recv < 0 ) {
		if( SOCKET_WOULD_BLOCK ) {
			_recv = 0;
		}
		else {
			error = SOCKET_LAST_ERROR;
			state = ZO_ERROR;
			_recv = -1;
		}
	}

	blockingOverride( reset );

	return _recv;
}

int Zocket::write( void *buffer, int bufferSize, int block ) {
	int reset = blockingOverride( block );

	int _send;
	if( options & ZO_UDP ) {
		ZocketAddress zAddress = zocketAddress;
		_send = sendto( sockFD, (char *)buffer, bufferSize, 0, (struct sockaddr *)&zocketAddress, sizeof(zocketAddress) );
	}
	else {
		_send = send( sockFD, (char *)buffer, bufferSize, 0 );
	}
	if( _send < 0 ) {
		if( SOCKET_WOULD_BLOCK ) {
			_send = 0;
		}
		else {
			error = SOCKET_LAST_ERROR;
			state = ZO_ERROR;
			_send = -1;
		}
	}

	blockingOverride( reset );

	return _send;
}

int Zocket::writeString( char *buffer, int block ) {
	int reset = blockingOverride( block );

	int bufferSize = strlen( buffer );

	int _send;
	if( options & ZO_UDP ) {
		ZocketAddress zAddress = zocketAddress;
		_send = sendto( sockFD, (char *)buffer, bufferSize, 0, (struct sockaddr *)&zocketAddress, sizeof(zocketAddress) );
	}
	else {
		_send = send( sockFD, (char *)buffer, bufferSize, 0 );
	}
	if( _send < 0 ) {
		if( SOCKET_WOULD_BLOCK ) {
			_send = 0;
		}
		else {
			error = SOCKET_LAST_ERROR;
			state = ZO_ERROR;
			_send = -1;
		}
	}

	blockingOverride( reset );

	return _send;
}

int Zocket::dgramRead( void *buffer, int bufferSize, ZocketAddress &zAddress, int block ) {
	int reset = blockingOverride( block );

	socklen_t addressSize = sizeof(ZocketAddress);
	int _recv = recvfrom( sockFD, (char *)buffer, bufferSize, 0, (struct sockaddr *)&zocketAddress, &addressSize );
	if( _recv < 0 ) {
		if( SOCKET_WOULD_BLOCK ) {
			_recv = 0;
		}
		else {
			error = SOCKET_LAST_ERROR;
			state = ZO_ERROR;
			_recv = -1;
		}
	}

	blockingOverride( reset );

	return _recv;
}

int Zocket::dgramWrite( void *buffer, int bufferSize, ZocketAddress &zAddress, int block ) {
	int reset = blockingOverride( block );

	int _send = sendto( sockFD, (char *)buffer, bufferSize, 0, (struct sockaddr *)&zocketAddress, sizeof(zocketAddress) );
	if( _send < 0 ) {
		if( SOCKET_WOULD_BLOCK ) {
			_send = 0;
		}
		else {
			error = SOCKET_LAST_ERROR;
			state = ZO_ERROR;
			_send = -1;
		}
	}

	blockingOverride( reset );

	return _send;
}

ZocketAddress Zocket::getRemoteAddress() {
	ZocketAddress zAddress;
	socklen_t len = sizeof( struct sockaddr );
	int ret = getpeername( sockFD, (struct sockaddr *)&zAddress, &len );
	if( ret < 0 ) {
		error = SOCKET_LAST_ERROR;
	}
	return zAddress;
}

// FD_SET functions are provided so that networking modules
// that use Zocket don't have to include windows.h, etc for these.
//-----------------------------------------------------------------------------

void zocketFdClr( unsigned int fd, void *set ) {
	FD_CLR( fd, (fd_set*) set );
}

void zocketFdSet( unsigned int fd, void *set ) {
	FD_SET( fd, (fd_set*) set );
}

void zocketFdZero( void *set ) {
	FD_ZERO( (fd_set*) set );
}

int zocketFdIsSet( unsigned int fd, void *set ) {
	return FD_ISSET( fd, (fd_set*) set );
}


#ifdef ZOCKET_SELF_TEST
#pragma message( "BUILDING ZOCKET_SELF_TEST" )

#ifdef WIN32
#define sleep(x) Sleep(x*1000)
#endif

int main() {
	Zocket server( "tcp://*:4000", ZO_LISTENING|ZO_NONBLOCKING );
	Zocket client( "tcp://127.0.0.1:4000", ZO_NONBLOCKING );
	Zocket *serverSide = 0;

	int sockFD = server.accept();

	sleep( 1 );
		// These sleeps are necessary to give the kernel time to 
		// actually open and close things before we check on them

	if( sockFD > 0 ) {
		serverSide = new Zocket( &server );
	}

	sleep( 1 );

	int ret = client.write( (void *)"test", 5 );
	assert( ret == 5 );
	
	sleep( 1 );

	char buffer[10];
	ret = serverSide->read( buffer, sizeof(buffer) );
	assert( ret == 5 );

	assert( !strcmp( buffer, "test" ) );

	client.close();

	sleep( 1 );

	ret = serverSide->read( buffer, sizeof(buffer) );
	assert( ret == -1 );

	return 0;
}

#endif

#endif


