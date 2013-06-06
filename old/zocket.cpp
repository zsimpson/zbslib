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
//		*PORTABILITY win32 unix
//		*WIN32_LIBS_DEBUG wsock32.lib
//		*WIN32_LIBS_RELEASE wsock32.lib
//		*REQUIRED_FILES zocket.cpp zocket.h zaddress.h
//		*VERSION 2.0
//		+HISTORY {
//		}
//		+TODO {
//			ZAddress is very dorky.  Eliminate the binary interface altogether for simplicity
//		}
//		*SELF_TEST no
//		*PUBLISH yes
// }
// OPERATING SYSTEM specific includes:
#ifdef WIN32
	#include "windows.h"
	#include "winsock.h"
	#include "wsipx.h"
	#define SOCKET_CLOSE        closesocket
	#define SOCKET_IOCTL		ioctlsocket
	#define SOCKET_WOULD_BLOCK  WSAEWOULDBLOCK
	#define SOCKET_EAGAIN       WSAEWOULDBLOCK
	#define	SOCKET_LAST_ERROR	WSAGetLastError()

	static WSAData wsaData;
	static int wsaStartupFlag = 0;

	#define	INIT_SYSTEM         initWinSock()
	#define socklen_t           int
#else
	#include "netdb.h"
	#include "unistd.h"
	#include "signal.h"
	#ifdef sgi
		#include "bstring.h"
	#endif
	#include "sys/types.h"
	#include "sys/time.h"
	#include "sys/socket.h"
	#include "sys/ioctl.h"
	#include "netinet/in.h"
	#include "arpa/inet.h"
	#include "sys/termios.h"
	#ifdef bsdi
		#include "errno.h"
		#include "netinet/tcp.h"
	#endif

	#include "ctype.h"
	#include "errno.h"
	#include "netinet/tcp.h"
	#ifndef min
		#define min(a,b) ( ( (a) < (b) ) ? (a) : (b) )
	#endif

	#define SOCKET_CLOSE		::close
	#define SOCKET_IOCTL		::ioctl
	#define SOCKET_WOULD_BLOCK  EINPROGRESS
	#define SOCKET_EAGAIN       EAGAIN
	#define	SOCKET_LAST_ERROR   errno
	#define SOCKET_ERROR        (-1)

	#define	INIT_SYSTEM         (1)

	int sigPipeInit = 0;
#endif

// SDK includes:
// STDLIB includes:
#include "string.h"
#include "stdlib.h"
#include "assert.h"
#include "stdio.h"
// MODULE includes:
#include "zocket.h"
#include "zaddress.h"
// ZBSLIB includes:
#include "zregexp.h"

// Windows utility functions
//----------------------------------------------------------------------------
#ifdef WIN32
static int initWinSock() {
	if( !wsaStartupFlag ) {
		if( WSAStartup(0x0101, &wsaData) ) {
			return 0;
		}
		wsaStartupFlag++;
	}
	return 1;
}
#endif

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

// Protocol tests
//-----------------------------------------------------------------------------

static int hasIPProtocol() {
	Zocket test( "udp,*:0,s" );
	return test.isOpen();
}

static int hasIPXProtocol() {
	Zocket test( "ipx,*:0,s" );
	return test.isOpen();
}

// ZAddress
//-----------------------------------------------------------------------------

int dottedDecimalToIp( char *dd ) {
	return inet_addr(dd);
}

char *ipToDottedDecimal( int ip ) {
	struct in_addr a;
	a.s_addr = ip;
	return inet_ntoa(a);
}

// Port
//---------------------

int ZAddress::getPort() {
	if( isIPX() ) {
		return ntohs( ((struct sockaddr_ipx *)&sockAddr)->sa_socket );
	}
	else if( isIP() || isLOOP() ) {
		return ntohs( ((struct sockaddr_in *)&sockAddr)->sin_port );
	}
	return 0;
}

void ZAddress::setPort( int port ) {
	if( isIPX() ) {
		((struct sockaddr_ipx *)&sockAddr)->sa_socket = htons( port );
	}
	else if( isIP() ) {
		((struct sockaddr_in *)&sockAddr)->sin_port = htons( port );
	}
	else if( isLOOP() ) {
	}
	else {
		// Illegal to set port on unknown protocol
		assert( 0 );
	}
}

// Protocol
//---------------------

char *ZAddress::getProtocol() {
	if( zocketFlags & zoTCP ) {
		return "tcp";
	}
	else if( zocketFlags & zoUDP ) {
		return "udp";
	}
	else if( zocketFlags & zoSPX ) {
		return "spx";
	}
	else if( zocketFlags & zoIPX ) {
		return "ipx";
	}
	else if( zocketFlags & zoLOOP ) {
		return "loop";
	}
	return "";
}

void ZAddress::setProtocol( char *protocol ) {
	if( !protocol || !*protocol ) return;
	zocketFlags &= ~zoPROTOCOLS;
	
	#ifdef __USE_GNU
	#define stricmp strcasecmp
	#endif
	
	if( !stricmp(protocol, "tcp") ) {
		sockAddr.sa_family = PF_INET;
		zocketFlags |= (zoTCP|zoIP_FAMILY);
	}
	else if( !stricmp(protocol, "udp") ) {
		sockAddr.sa_family = PF_INET;
		zocketFlags |= (zoUDP|zoIP_FAMILY);
	}
	else if( !stricmp(protocol, "spx") ) {
		sockAddr.sa_family = PF_IPX;
		zocketFlags |= (zoSPX|zoIPX_FAMILY);
	}
	else if( !stricmp(protocol, "ipx") ) {
		sockAddr.sa_family = PF_IPX;
		zocketFlags |= (zoIPX|zoIPX_FAMILY);
	}
	else if( !stricmp(protocol, "loop") ) {
		sockAddr.sa_family = PF_LOOP;
		zocketFlags |= (zoIPX|zoIPX_FAMILY);
	}
	else {
		assert( 0 );
		// Illegal protocol specified
	}
}

// Options
//---------------------

void ZAddress::setOptions( char *options ) {
	if( !options || !*options ) return;
	zocketFlags &= ~zoOPTIONS;
	for( char *p=options; *p; p++ ) {
		switch( tolower(*p) ) {
			case 'l': zocketFlags |= zoLISTENING; break;
			case 'b': zocketFlags |= zoNONBLOCKING; break;
			case 'r': zocketFlags |= zoREUSEADDR; break;
			case 'n': zocketFlags |= zoDISABLE_NAGLE; break;
		}
	}
}

int ZAddress::isNonBlocking() {
	return zocketFlags & zoNONBLOCKING;
}

void ZAddress::setNonBlocking( int x ) {
	if( x ) zocketFlags |= zoNONBLOCKING;
	else zocketFlags &= ~zoNONBLOCKING;
}

int ZAddress::isNagleDisabled() {
	return zocketFlags & zoDISABLE_NAGLE;
}

void ZAddress::setNagleDisabled( int x ) {
	if( x ) zocketFlags |= zoDISABLE_NAGLE;
	else zocketFlags &= ~zoDISABLE_NAGLE;
}

int ZAddress::isReuseAddr() {
	return zocketFlags & zoREUSEADDR;
}

void ZAddress::setReuseAddr( int x ) {
	if( x ) zocketFlags |= zoREUSEADDR;
	else zocketFlags &= ~zoREUSEADDR;
}

int ZAddress::isListening() {
	return zocketFlags & zoLISTENING;
}

void ZAddress::setListening( int x ) {
	if( x ) zocketFlags |= zoLISTENING;
	else zocketFlags &= ~zoLISTENING;
}


// Address
//---------------------

void ZAddress::setAddress( char *addr ) {
	if( !addr ) return;
	ZRegExp regExp1( "([0-9A-Fa-f]+)-([0-9A-Fa-f]+)-([0-9A-Fa-f]+)-([0-9A-Fa-f]+)-([0-9A-Fa-f]+)-([0-9A-Fa-f]+)-?([0-9A-Fa-f]+)?" );

	if( *addr == '*' ) {
		// This is a non-specified address
		memset( &sockAddr, 0, sizeof(sockAddr) );
		if( isIP() ) {
			struct sockaddr_in *a = (struct sockaddr_in *)&sockAddr;
			a->sin_family = AF_INET; 
		}
		else if( isIPX() ) {
			struct sockaddr_ipx *a = (struct sockaddr_ipx *)&sockAddr;
			a->sa_family = AF_IPX; 
		}
		else if( isLOOP() ) {
			struct sockaddr_in *a = (struct sockaddr_in *)&sockAddr;
			a->sin_family = AF_LOOP; 
		}
		else {	
			assert( 0 );
		}
	}
	else if( regExp1.test( addr ) ) {
		// This is an IPX address
		struct sockaddr_ipx *a = (struct sockaddr_ipx *)&sockAddr;
		memset( a, 0, sizeof(sockAddr) );
		a->sa_family  = AF_IPX;
		a->sa_socket = 0;

		int netNumExists = 0;
		if( regExp1.get(7) ) {
			// It is long enough to have a network
			netNumExists = 1;
			*((unsigned int *)a->sa_netnum) = htonl( strtoul( regExp1.get(1), NULL, 16 ) );
		}

		memset( a->sa_nodenum, 0, 6 );

		for( int i=0; i<6; i++ ) {
			a->sa_nodenum[i] = (char)strtol(
				regExp1.get(netNumExists+i+1), NULL, 16
			);
		}
	}
	else {
		// This is an IP address
		int ip = inet_addr( addr );
		if( ip == INADDR_NONE && stricmp(addr,"255.255.255.255") ) {
			// it wasn't in a dotted decimal notation, so try
			// to look it up as a DNS name.  This may block for a long time!
			// Note that the string 255.255.255.255 is a special case
			// as it evaluates to -1 which is what INADDR_NONE is
			// defined as.  Annoying.
			ip = 0;
			if( INIT_SYSTEM ) {
				struct hostent *hostEnt = gethostbyname( addr );
				if( hostEnt ) {
					ip = *(int *)hostEnt->h_addr;
				}
			}
		}

		memset( &sockAddr, 0, sizeof(sockAddr) );
		struct sockaddr_in *a = (struct sockaddr_in *)&sockAddr;
		a->sin_family = AF_INET; 
		a->sin_port = 0;
		a->sin_addr.s_addr = ip;
	}
}


// TCP/IP Binary
//---------------------

int ZAddress::isIP() {
	return zocketFlags & zoIP_FAMILY;
}

int ZAddress::getIP() {
	return ((struct sockaddr_in *)&sockAddr)->sin_addr.s_addr;
}

void ZAddress::setIP( int ip, int port ) {
	struct sockaddr_in *a = (struct sockaddr_in *)&sockAddr;
	a->sin_family = AF_INET; 
	if( port ) {
		a->sin_port = htons( port );
	}
	a->sin_addr.s_addr = ip;
	zocketFlags |= zoIP_FAMILY;
}

int ZAddress::isIPLoopback() {
	return isIP() && getIP() == 0x0100007f;
}

int ZAddress::fillMyIPAddress( int allowReserved ) {
	char buffer[256];
	memset( buffer, 0, 256 );

	// Ken Demarest fixed a bug concerning failing to call INIT_SYSTEM
	// before the call to gethostbyname on 31 Aug 2000.  Also, upped
	// buffer to 256 and added the allowReserved option

	sockAddr.sa_family = 0;
	if( !(INIT_SYSTEM) ) {
		return 0;
	}

	if( gethostname( buffer, 256-1 ) == SOCKET_ERROR ) {
		return 0;
	}

	// WARNING!  If you see lockups it is almost certainly
	// due to this call blocking everything.
	struct hostent *p;
	p = gethostbyname( buffer );

	if( p && p->h_addrtype == AF_INET ) {
		int isReserved = 0;
		int nicIndex = 0;
		int ip = 0;
		int isValid = 0;

		do {
			isValid =  ( p->h_addr_list[nicIndex] != 0 );

			if( isValid ) {
				ip = *(int *)p->h_addr_list[nicIndex++];
				set( ip );
				isReserved = ( (unsigned char)sockAddr.sa_data[2] == 192 || (unsigned char)sockAddr.sa_data[2] == 10 );
			}
		} while( isValid && isReserved && !allowReserved );
		
		// You can ask this routine to make sure it picks the non-reserved NIC on your machine
		// by passing in allowReserved==0

		if( ip == 0 ) {
			sockAddr.sa_family = 0;
		}

		return ip != 0;
	}
	sockAddr.sa_family = 0;
	return 0;
}

// SPX/IPX binary API
//----------------------------------

void ZAddress::set( ZAddress zAddress ) {
	memcpy( this, &zAddress, sizeof(*this) );
}

int ZAddress::isIPX() {
	return zocketFlags & zoIPX_FAMILY;
}

void ZAddress::getIPX( IPXBinaryAddr &addr ) {
	struct sockaddr_ipx *a = (struct sockaddr_ipx *)&sockAddr;
	memcpy( &addr[0], a->sa_netnum, 4 );
	memcpy( &addr[4], a->sa_nodenum, 6 );
}

void ZAddress::setIPX( IPXBinaryAddr addr, int port ) {
	struct sockaddr_ipx *a = (struct sockaddr_ipx *)&sockAddr;
	memset( a, 0, sizeof(sockAddr) );
	a->sa_family  = AF_IPX;
	a->sa_socket = 0;
	if( port ) {
		a->sa_socket = htons( port );
	}
	memcpy( a->sa_netnum, &addr[0], 4 );
	memcpy( a->sa_nodenum, &addr[4], 6 );
	zocketFlags |= zoIPX_FAMILY;
}

int ZAddress::fillMyIPXAddress() {
/* TODO
	Zocket temp( "spx,*:0,s" );
	int size = sizeof(sockAddr);
	if( temp.isOpen() ) {
		int a = getsockname( temp.getSockFD(), &sockAddr, &size );
		return a == 0;
	}
	sockAddr.sa_family = 0;
*/
	return 0;
}

int ZAddress::isLOOP() {
	return zocketFlags & zoLOOP;
}

// Constructors and general
//----------------------------------

void ZAddress::reset() {
	memset( this, 0, sizeof(*this) );
}

ZAddress::ZAddress() {
	reset();
}

ZAddress::ZAddress( int ip, int port ) {
	setIP( ip, port );
}

ZAddress::ZAddress( char *zAddress, char *protocol, char *options, int port ) {
	set( zAddress, protocol, options, port );
}

ZAddress::ZAddress( struct sockaddr &_sockAddr ) {
	sockAddr = _sockAddr;
}

int ZAddress::isValid() {
	// This checks all of the possbile things that can go wrong
	
	// Can only be one of the three protocol families
	int families = 
		(zocketFlags & zoIP_FAMILY ? 1 : 0) +
		(zocketFlags & zoIPX_FAMILY ? 1 : 0) +
		(zocketFlags & zoLOOP ? 1 : 0)
	;
	if( families != 1 ) {
		return 0;
	}

	// Can only be one of the subsets of IP family
	if( zocketFlags & zoIP_FAMILY ) {
		if( sockAddr.sa_family != PF_INET ) {
			return 0;
		}
		if( zocketFlags & (zoSPX|zoIPX|zoIPX_FAMILY|zoLOOP) ) {
			return 0;
		}
		if(
			(zocketFlags & zoTCP ? 1 : 0) +	(zocketFlags & zoUDP ? 1 : 0)
			!= 1
		) {
			return 0;
		}
	}

	// Can only be one of the subsets of IPX family
	if( zocketFlags & zoIPX_FAMILY ) {
		if( sockAddr.sa_family != PF_IPX ) {
			return 0;
		}
		if( zocketFlags & (zoTCP|zoUDP|zoIP_FAMILY|zoLOOP) ) {
			return 0;
		}
		if(
			(zocketFlags & zoSPX ? 1 : 0) +	(zocketFlags & zoIPX ? 1 : 0)
			!= 1
		) {
			return 0;
		}
	}

	if( zocketFlags & zoLOOP ) {
		if( sockAddr.sa_family != PF_LOOP ) {
			return 0;
		}
	}

	return sockAddr.sa_family != 0;
}

void ZAddress::setProtocolConnectionBased() {
	if( isIPX() ) {
		zocketFlags &= ~(zoPROTOCOLS);
		zocketFlags |= zoSPX|zoIPX_FAMILY;
	}
	else if( isIP() ) {
		zocketFlags &= ~(zoPROTOCOLS);
		zocketFlags |= zoTCP|zoIP_FAMILY;
	}
}

void ZAddress::setProtocolDgramBased() {
	if( isIPX() ) {
		zocketFlags &= ~(zoPROTOCOLS);
		zocketFlags |= zoIPX|zoIPX_FAMILY;
	}
	else if( isIP() ) {
		zocketFlags &= ~(zoPROTOCOLS);
		zocketFlags |= zoUDP|zoIP_FAMILY;
	}
}

void ZAddress::set( char *zAddress, char *protocol, char *options, int port ) {
	ZRegExp regExp( "([^:(]+)?(\\([a-zA-Z*]+\\))?:?//([^:/]+):?([0-9]+)?" );

	memset( &sockAddr, 0, sizeof(struct sockaddr) );
	zocketFlags = 0;

	char buf[80];

	if( regExp.test( zAddress ) ) {
		setProtocol( regExp.get(1,buf,80) );
		setOptions( regExp.get(2,buf,80) );
		setAddress( regExp.get(3,buf,80) );
		setPort( atoi( regExp.get(4,buf,80) ) );
	}

	// Any explicitly passed in parameters takes precedance
	if( protocol && *protocol ) {
		setProtocol( protocol );
	}
	if( options && *options ) {
		setOptions( options );
	}
	if( port ) {
		setPort( port );
	}

	assert( isValid() );
}

int ZAddress::getName( char *buffer, int bufferLen ) {
	int result = 0;

	if( zocketFlags & zoIP_FAMILY ) {
		struct hostent *host = gethostbyaddr(
			(const char *)&((struct sockaddr_in *)&sockAddr)->sin_addr.s_addr,
			4,
			PF_INET
		);

		if( host ) {
			int length = strlen(host->h_name) + 1;
			if( bufferLen >= length) {
				assert( buffer != NULL );
				strcpy( buffer, host->h_name );
				result = length;
			}
			else if( bufferLen == 0 ) {
				result = length;
			}
		}
	}

	return result;
}

char *ZAddress::getString( int withProtocol, int withOptions, int withPort ) {
	static char getStringBuffers[5][30];
	static int  getStringBufNum = 0;

	char *b = getStringBuffers[getStringBufNum];
	*b = 0;
	getStringBufNum = (getStringBufNum+1) % 5;

	if( withProtocol ) {
		if( zocketFlags & zoTCP ) {
			strcpy( b, "tcp" );
		}
		else if( zocketFlags & zoUDP ) {
			strcpy( b, "udp" );
		}
		else if( zocketFlags & zoSPX ) {
			strcpy( b, "spx" );
		}
		else if( zocketFlags & zoIPX ) {
			strcpy( b, "ipx" );
		}
		else if( zocketFlags & zoLOOP ) {
			strcpy( b, "loop" );
		}
	}

	if( withOptions && (zocketFlags & zoOPTIONS) ) {
		strcat( b, "(" );
		if( zocketFlags & zoLISTENING )     strcat( b, "l" );
		if( zocketFlags & zoNONBLOCKING )   strcat( b, "b" );
		if( zocketFlags & zoREUSEADDR )     strcat( b, "r" );
		if( zocketFlags & zoDISABLE_NAGLE ) strcat( b, "n" );
		strcat( b, ")" );
	}

	if( withProtocol || (withOptions && (zocketFlags & zoOPTIONS)) ) {
		strcat( b, ":" );
	}

	strcat( b, "//" );

	if( isIPX() ) {
		struct sockaddr_ipx *s = (struct sockaddr_ipx *)&sockAddr;
		if( *(int *)s->sa_netnum ) {
			sprintf( b+strlen(b), "%X-", ntohl( *(int *)s->sa_netnum ) );
		}
		sprintf( 
			b+strlen(b),
			"%02X-%02X-%02X-%02X-%02X-%02X", 
			(unsigned char)s->sa_nodenum[0], (unsigned char)s->sa_nodenum[1], 
			(unsigned char)s->sa_nodenum[2], (unsigned char)s->sa_nodenum[3], 
			(unsigned char)s->sa_nodenum[4], (unsigned char)s->sa_nodenum[5]
		);
	}
	else {
		if( getIP() == 0 ) {
			strcat( b, "*" );
		}
		else {
			sprintf( b+strlen(b), "%s", ipToDottedDecimal(getIP()) );
		}
	}

	if( withPort ) {
		sprintf( b+strlen(b), ":%d", getPort() );
	}

	return b;
}

/*
void testZAddress() {
//	ZAddress t1( "124.0.0.1" );

	int p, a, o;
	char *s;

	ZAddress t2( "tcp://124.0.0.1" );
		p = t2.getPort();
		a = t2.getIP();
	ZAddress t3( "tcp(nbr)://124.0.0.1" );
		p = t3.getPort();
		a = t3.getIP();
		o = t3.isNagleDisabled();
	ZAddress t4( "tcp(nbr)://124.0.0.1:100" );
		p = t4.getPort();
		a = t4.getIP();
	ZAddress t5( "spx://ff-fe-fd-fc-fb-fa:100" );
		o = t5.isIPX();
		p = t5.getPort();
		IPXBinaryAddr i;
		t5.getIPX(i);
	ZAddress t6( "ipx://deadbeef-01-02-03-04-05-06:100" );
		s = t6.getString( 1, 1, 1 );
}
*/

// Zocket
//-----------------------------------------------------------------------------

int Zocket::loopInitialized = 0;
Zocket *Zocket::loopPendingConnections[zlMAX_LOOP_ZOCKETS];
char *zocketErrorStrings[] = {
	"zeNO_ERROR",
	"zeUNSUPPORED_OR_BAD_PROTOCOL",
	"zeBAD_ADDRESS",
	"zeBAD_CLIENT_SERVER_OPTION",
	"zeBAD_IP_ADDRESS",
	"zeBAD_IPX_ADDRESS",
	"zeUNABLE_TO_START_WINDOWS",
	"zeUNABLE_TO_OPEN_SOCKET",
	"zeUNABLE_TO_LOOKUP_NAME",
	"zeUNABLE_TO_CONNECT",
	"zeUNABLE_TO_BIND",
	"zeUNABLE_TO_LISTEN",
	"zeUNABLE_TO_SET_OPTION",
};
	
char *Zocket::getNativeErrorString() {
	#ifdef WIN32
		return "";
			// Windows annoyingly has no constants for this stuff
			// and I'm too lazy to make a table of the 100 some odd errors
	#else
		return (char *)sys_errlist[lastErrorCode];
			// Unix, on the other hand, being a sane operating system
			// uses socket errors like any other errors and you should 
			// be able to just pull them out of sys_errlist
	#endif
}

void Zocket::reset() {
	ZAddress::reset();
	error              = zeNO_ERROR;
	lastErrorCode      = 0;
	sockFD             = 0;
	acceptedSockFD     = INVALID_SOCKET;
	loopSendBuffer     = NULL;
	loopSendBufferHead = NULL;
	loopSendBufferSize = 0;
	zocketStateFlags   = 0;
}

Zocket::Zocket() {
	reset();
}

Zocket::Zocket( Zocket *listeningZocket ) {
	*((ZAddress *)this) = *((ZAddress *)listeningZocket);
	setListening( 0 );
	error               = listeningZocket->error;
	lastErrorCode       = listeningZocket->lastErrorCode;
	sockFD              = listeningZocket->acceptedSockFD;
	acceptedSockFD      = INVALID_SOCKET;
	loopSendBuffer      = NULL;
	loopSendBufferHead  = NULL;
	loopSendBufferSize  = 0;
	zocketStateFlags    = zsOPEN;

	if( isLOOP() ) {
		((Zocket *)sockFD)->sockFD = (long)this;
		listeningZocket->acceptedSockFD = INVALID_SOCKET;
	}
}

Zocket::Zocket( int fd ) {
	reset();
	sockFD = fd;
	zocketStateFlags |= zsOPEN;
}

void Zocket::open( ZAddress zAddr ) {
	reset();

	set( zAddr );

	if( !loopInitialized ) {
		loopInitialized++;
		memset( loopPendingConnections, 0, sizeof(Zocket *) * zlMAX_LOOP_ZOCKETS );
	}

	lastErrorCode = 0;
	#define OPEN_ERROR(x) lastErrorCode = SOCKET_LAST_ERROR; error = x; sockFD != INVALID_SOCKET ? SOCKET_CLOSE( sockFD ) : 0; sockFD = INVALID_SOCKET;

	// Setup Windows and UNIX static environments
	//----------------------------------------------
	#ifdef WIN32
		// Startup the annoying windows crap only if we haven't do it before
		// and if this isn't a zlocal loopback socket
		if( !zAddr.isLOOP() ) {
			if( !INIT_SYSTEM ) {
				OPEN_ERROR( zeUNABLE_TO_START_WINDOWS );
				return;
			}
		}
	#else
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

	// Clear out address structures
	//----------------------------------------------
	assert( !(zocketStateFlags & zsOPEN) );

	acceptedSockFD = INVALID_SOCKET;
	error  = zeNO_ERROR;
	sockFD = INVALID_SOCKET;

	if( zocketFlags & zoIP_FAMILY ) {
		sockFD = socket( 
			PF_INET, 
			(zocketFlags&zoTCP) ? SOCK_STREAM : SOCK_DGRAM, 
			(zocketFlags&zoTCP) ? IPPROTO_TCP : IPPROTO_UDP
		);
		if( sockFD == INVALID_SOCKET ) {
			OPEN_ERROR( zeUNABLE_TO_OPEN_SOCKET );
			return;
		}

		if( zocketFlags & zoUDP ) {
			// Fix bug in Windows 95 scoket implementation per MSDN instructions
			int t = 1;
			int ret = setsockopt( sockFD, SOL_SOCKET, SO_BROADCAST, (const char *)&t, sizeof(int) );
			if( ret != 0 ) {
				OPEN_ERROR( zeUNABLE_TO_SET_OPTION );
				return;
			}
		}

		if( zocketFlags & zoDISABLE_NAGLE ) {
			int t = 1;
			int ret = setsockopt( sockFD, IPPROTO_TCP, TCP_NODELAY, (const char *)&t, sizeof(int) );
			if( ret != 0 ) {
				OPEN_ERROR( zeUNABLE_TO_SET_OPTION );
				return;
			}
		}

		if( (zocketFlags & zoREUSEADDR) || (
			zAddr.getIP() == INADDR_ANY && (zocketFlags & zoLISTENING)
		) ) {
			// If binding to INADDR_ANY, set SO_REUSEADDR before the first bind.
			// It appears that under Linux, in the case of a socket collision, BOTH the
			// socket attempting the bind and the already bound colliding socket must be
			// SO_REUSEADDR or the bind will fail. However, regardless of the reuse state,
			// if the colliding socket is listening, the bind will fail.  Does this mean
			// that it is safe to turn on SO_REUSEADDR for all server sockets? For now,
			// we will just do it for non-specific addresses. -TNB
			int t = 1;
			int ret = setsockopt( sockFD, SOL_SOCKET, SO_REUSEADDR, (const char *)&t, sizeof(int) );
			if( ret != 0 ) {
				OPEN_ERROR( zeUNABLE_TO_BIND );
				return;
			}
		}

		unsigned long nonblocking = isNonBlocking() ? 1 : 0;
		int result = SOCKET_IOCTL( sockFD, FIONBIO, &nonblocking );
		if( result != 0 ) {
			OPEN_ERROR( zeUNABLE_TO_SET_OPTION );
			return;
		}

		if( !isListening() && (zocketFlags & zoTCP) ) {
			int _connect = connect( sockFD, (struct sockaddr *)&sockAddr, sizeof(sockAddr) );
			
			#ifdef WIN32
				// I discovered this bug 1 Jun 2007.  If I didn't sleep here
				// then half the time it would fail to connect.  Very odd.
				// 6 Mar 2--8 seemingly it went up to 100 milliseconds!  Extrapolating to the
				// future I will have to wait for an hour by 2030
				Sleep( 100 );
			#endif
			if( _connect < 0 ) {
				if( isNonBlocking() && (SOCKET_LAST_ERROR == SOCKET_WOULD_BLOCK || SOCKET_LAST_ERROR == SOCKET_EAGAIN) ) {
					// This was set as a non-blocking socket, so the connection is
					// in progress.  This is a valid state, so fall through.
					// The socket will be marked as open, however, it will only
					// really be open when a selection on write succeeds.
				}
				else {
					OPEN_ERROR( zeUNABLE_TO_CONNECT );
					return;
				}
			}
		}
		else {
			ZAddress _zaddress = *(ZAddress*)this;
			if( getFlags() & zoUDP ) {
				_zaddress.setIP( 0 );
			}
			int _bind = bind( sockFD, (struct sockaddr *)&_zaddress, sizeof(sockAddr) );
			if( _bind == SOCKET_ERROR ) {
				OPEN_ERROR( zeUNABLE_TO_BIND );
				return;
			}

			if( zocketFlags & zoTCP ) {
				int _listen = listen( sockFD, 4 );
				if( _listen == SOCKET_ERROR ) {
					OPEN_ERROR( zeUNABLE_TO_LISTEN );
					return;
				}
			}
		}
	}
	else if( zocketFlags & zoIPX_FAMILY ) {
		#ifndef IPX_SUPPORT
			OPEN_ERROR( zeUNSUPPORED_OR_BAD_PROTOCOL );
			return;
		#else
		sockFD = socket( 
			PF_IPX, 
			(zocketFlags&zoSPX) ? SOCK_STREAM : SOCK_DGRAM, 
			(zocketFlags&zoSPX) ? NSPROTO_SPX : NSPROTO_IPX
		);

		if( sockFD == INVALID_SOCKET ) {
			OPEN_ERROR( zeUNABLE_TO_OPEN_SOCKET );
			return;
		}

		if( zocketFlags & zoIPX ) {
			// Fix bug in Windows 95 socket implementation per MSDN instructions
			int t = 1;
			int ret = setsockopt( sockFD, SOL_SOCKET, SO_BROADCAST, (const char *)&t, sizeof(int) );
			if( ret != 0 ) {
				OPEN_ERROR( zeUNABLE_TO_SET_OPTION );
				return;
			}
		}

		if( isReuseAddr() || (
			zAddr.getIP() == INADDR_ANY && isListening()
		) ) {
			int t = 1;
			int ret = setsockopt( sockFD, SOL_SOCKET, SO_REUSEADDR, (const char *)&t, sizeof(int) );
			if( ret != 0 ) {
				OPEN_ERROR( zeUNABLE_TO_BIND );
				return;
			}
		}

		unsigned long nonblocking = isNonBlocking() ? 1 : 0;
		int result = SOCKET_IOCTL( sockFD, FIONBIO, &nonblocking );
		if( result != 0 ) {
			OPEN_ERROR( zeUNABLE_TO_SET_OPTION );
			return;
		}

		if( !isListening() && (zocketFlags & zoSPX) ) {
			int _connect = connect( sockFD, (struct sockaddr *)&sockAddr, sizeof(struct sockaddr_ipx) );
			if( _connect < 0 ) {
				if( isNonBlocking() && (SOCKET_LAST_ERROR == SOCKET_WOULD_BLOCK || SOCKET_LAST_ERROR == SOCKET_EAGAIN) ) {
					// This was set as a non-blocking socket, so the connection is
					// in progress.  This is a valid state, so fall through.
					// The socket will be marked as open, however, it will only
					// really be open when a selection on write succeeds.
				}
				else {
					OPEN_ERROR( zeUNABLE_TO_CONNECT );
					return;
				}
			}
		}
		else {
			int _bind = bind( sockFD, (struct sockaddr *)&sockAddr, sizeof(struct sockaddr_ipx) );
			if( _bind == SOCKET_ERROR ) {
				OPEN_ERROR( zeUNABLE_TO_BIND );
				return;
			}

			if( zocketFlags & zoSPX ) {
				int _listen = listen( sockFD, 4 );
				if( _listen == SOCKET_ERROR ) {
					OPEN_ERROR( zeUNABLE_TO_LISTEN );
					return;
				}
			}
		}
		#endif
	}
	else if( isLOOP() ) {
		sockFD = INVALID_SOCKET;

		if( !isListening() ) {
			// Add ourselves to the pending connections list
			// if there's a server out there, it will pick us up
			// during the next poll
			int i;
			for( i=0; i<zlMAX_LOOP_ZOCKETS; i++ ) {
				if( !loopPendingConnections[i] ) {
					loopPendingConnections[i] = this;
					sockFD = 0;
					break;
				}
			}
			if( i >= zlMAX_LOOP_ZOCKETS ) {
				OPEN_ERROR( zeUNABLE_TO_CONNECT );
				return;
			}
		}
		else {
			// *** May want to add a check for no duplicate port assignments here
		}
	}
	zocketStateFlags |= zsOPEN;
}

void Zocket::close() {
	if( isLOOP() ) {
		if( loopSendBuffer ) {
			free( loopSendBuffer );
		}
		Zocket *other = (Zocket *)sockFD;
		if( other && sockFD != INVALID_SOCKET ) {
			other->zocketStateFlags |= zsLOOP_REMOTE_CLOSE;
			other->sockFD = INVALID_SOCKET;
		}

		for( int i=0; i<zlMAX_LOOP_ZOCKETS; i++ ) {
			if( loopPendingConnections[i] == this ) {
				loopPendingConnections[i] = NULL;
				break;
			}
		}
		sockFD = INVALID_SOCKET;
	}
	else if( isOpen() ) {
		if( sockFD != INVALID_SOCKET ) {
			if( zocketFlags & zoSPX ) {
				// There seems to be a bug in WinSock 1.0 SPX
				// that causes the remote side to fail to
				// detect the shutdown if there is any data
				// outgoing on the socket at the time of the
				// abortive close.  This 100 mils sleep
				// is a hacky way to fix the problem.
				//
				// TODO: Retest in socket 2.0
				//
				//extern void sleepMils( int mils );
				//sleepMils(500);
			}

			int _close = SOCKET_CLOSE( sockFD );
			sockFD = INVALID_SOCKET;
		}
	}
	lastErrorCode = SOCKET_LAST_ERROR;
	zocketStateFlags &= ~zsOPEN;
}

int Zocket::setBlocking( int block ) {
	if( block && isNonBlocking() ) {
		unsigned long nonblocking = 0;
		int result = SOCKET_IOCTL( sockFD, FIONBIO, &nonblocking );
		if( result != 0 ) {
			error = zeUNABLE_TO_SET_OPTION;
			return 0;
		}
		zocketFlags &= ~zoNONBLOCKING;
	}
	else if( !block && !isNonBlocking() ) {
		unsigned long nonblocking = 1;
		int result = SOCKET_IOCTL( sockFD, FIONBIO, &nonblocking );
		if( result != 0 ) {
			error = zeUNABLE_TO_SET_OPTION;
			return 0;
		}
		zocketFlags |= zoNONBLOCKING;
	}

	return 1;
}

int Zocket::isConnected() {
	if( (zocketFlags & zoDGRAMBASED) || !isOpen() ) {
		return FALSE;
	}

	if( isLOOP() ) {
		return sockFD && sockFD != INVALID_SOCKET;
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
		return -1;
	}

	if( FD_ISSET(sockFD, &exceptSet) ) {
		close();
		return -1;
	}

	return FD_ISSET( sockFD, &writeSet );
}

int Zocket::accept( int block ) {
	if( !(zocketFlags & zoLISTENING) ) {
		return INVALID_SOCKET;
	}

	if( isLOOP() ) {
		// Look through the list of pending connections
		// and pick up any that match our port address
		for( int i=0; i<zlMAX_LOOP_ZOCKETS; i++ ) {
			Zocket *z = loopPendingConnections[i];
			if( z && z->getPort() == getPort() ) {
				// Accept this zocket
				loopPendingConnections[i] = NULL;
				acceptedSockFD = (long)z;
			}
		}
		if( acceptedSockFD == INVALID_SOCKET ) {
			return 0;
		}
		return acceptedSockFD;
	}

	int resetBlockingStatus = 0;
	if( isNonBlocking() && block > 0 ) {
		// If the socket defaults to non-blocking, but
		// the local override is set to block, 
		// then temporarily turn blocking on
		resetBlockingStatus = 1;
		setBlocking( 1 );
	}
	else if( !isNonBlocking() && !block ) {
		// If the socket defaults to blocking, but
		// the local override is set to non-blocking, 
		// then temporarily turn blocking off
		resetBlockingStatus = 1;
		setBlocking( 0 );
	}

	struct sockaddr_in sockAddr;
	memset( &sockAddr, 0, sizeof(sockAddr) );
	int a = sizeof(sockAddr);
	acceptedSockFD = ::accept( sockFD, (struct sockaddr *)&sockAddr, (socklen_t*)&a );
	if( acceptedSockFD < 0 ) {
		lastErrorCode = SOCKET_LAST_ERROR;
		if( lastErrorCode == SOCKET_WOULD_BLOCK || SOCKET_LAST_ERROR == SOCKET_EAGAIN ) {
			acceptedSockFD = 0;
		}
		else {
			zocketStateFlags |= zsREAD_WRITE_FAIL;
			acceptedSockFD = -1;
		}
	}
	else {
		int a = 1;
	}

	if( resetBlockingStatus ) {
		setBlocking( isNonBlocking() );
	}

	return acceptedSockFD;
}

int Zocket::read( void *buffer, int bufferSize, int block ) {
	if( isLOOP() ) {
		if( zocketStateFlags & zsLOOP_REMOTE_CLOSE ) {
			zocketStateFlags |= zsREAD_WRITE_FAIL;
			return -1;
		}
		Zocket *other = (Zocket *)sockFD;
		if( !other || (long)other == INVALID_SOCKET ) {
			return 0;
		}

		int bytesInBuffer = other->loopSendBufferHead - other->loopSendBuffer;
		if( bytesInBuffer == 0 ) {
			return 0;
		}
		int bytes = min( bufferSize, bytesInBuffer );
		memcpy( buffer, other->loopSendBuffer, bytes );
		if( bytes < bytesInBuffer ) {
			memmove( other->loopSendBuffer, other->loopSendBuffer+bytes, bytesInBuffer - bytes );
			other->loopSendBufferHead -= bytes;
		}
		else {
			other->loopSendBufferHead = other->loopSendBuffer;
		}
		return bytes;
	}

	int resetBlockingStatus = 0;
	if( isNonBlocking() && block > 0 ) {
		// If the socket defaults to non-blocking, but
		// the local override is set to block, 
		// then temporarily turn blocking on
		resetBlockingStatus = 1;
		setBlocking( 1 );
	}
	else if( !isNonBlocking() && !block ) {
		// If the socket defaults to blocking, but
		// the local override is set to non-blocking, 
		// then temporarily turn blocking off
		resetBlockingStatus = 1;
		setBlocking( 0 );
	}

	int _recv = recv( sockFD, (char *)buffer, bufferSize, 0 );
	if( _recv <= 0 ) {
		// When the "non-blocking" code is turned on, 
		// the send can return -1 and WOULD_BLOCK_ERROR_CODE
		// which we deal with by returning 0
		lastErrorCode = SOCKET_LAST_ERROR;
		if( lastErrorCode == SOCKET_WOULD_BLOCK || SOCKET_LAST_ERROR == SOCKET_EAGAIN ) {
			_recv = 0;
		}
		else {
			zocketStateFlags |= zsREAD_WRITE_FAIL;
			_recv = -1;
		}
	}

	if( resetBlockingStatus ) {
		setBlocking( isNonBlocking() );
	}

	return _recv;
}

int Zocket::write( void *buffer, int bufferSize, int block ) {
	if( isLOOP() ) {
		if( zocketStateFlags & zsLOOP_REMOTE_CLOSE ) {
			zocketStateFlags |= zsREAD_WRITE_FAIL;
			return -1;
		}
		if( !loopSendBuffer ) {
			// Create the buffer for the remote side
			loopSendBufferSize = bufferSize + 1024;
			loopSendBuffer = (char *)malloc( loopSendBufferSize );
			loopSendBufferHead = loopSendBuffer;
		}

		int bytesInBuffer = loopSendBufferHead - loopSendBuffer;
		if( loopSendBufferSize - bytesInBuffer < bufferSize ) {
			loopSendBufferSize = bytesInBuffer + bufferSize + 1024;
			loopSendBuffer = (char *)realloc( loopSendBuffer, loopSendBufferSize );
			loopSendBufferHead = loopSendBuffer + bytesInBuffer;
		}

		memcpy( loopSendBufferHead, buffer, bufferSize );
		loopSendBufferHead += bufferSize;
		return bufferSize;
	}

	int resetBlockingStatus = 0;
	if( isNonBlocking() && block > 0 ) {
		// If the socket defaults to non-blocking, but
		// the local override is set to block, 
		// then temporarily turn blocking on
		resetBlockingStatus = 1;
		setBlocking( 1 );
	}
	else if( !isNonBlocking() && !block ) {
		// If the socket defaults to blocking, but
		// the local override is set to non-blocking, 
		// then temporarily turn blocking off
		resetBlockingStatus = 1;
		setBlocking( 0 );
	}

	int _send;
	if( getFlags() & zoDGRAMBASED ) {
		ZAddress zAddress = *this;
		_send = sendto(
			sockFD, (char *)buffer, 
			bufferSize, 0,
			(struct sockaddr *)&zAddress, sizeof(ZAddress)
		);
	}
	else {
		_send = send( sockFD, (char *)buffer, bufferSize, 0 );
	}
	if( _send < 0 ) {
		// When the "non-blocking" code is turned on, 
		// the send can return -1 and WOULD_BLOCK_ERROR_CODE
		// which we deal with by returning 0
		lastErrorCode = SOCKET_LAST_ERROR;
		if( lastErrorCode == SOCKET_WOULD_BLOCK || SOCKET_LAST_ERROR == SOCKET_EAGAIN ) {
			_send = 0;
		}
		else {
			zocketStateFlags |= zsREAD_WRITE_FAIL;
			_send = -1;
		}
	}

	if( resetBlockingStatus ) {
		setBlocking( isNonBlocking() );
	}

	return _send;
}

int Zocket::puts( char *string, int block ) {
	return write( string, strlen(string), block );
}

int Zocket::dgramRead( void *buffer, int bufferSize, ZAddress &zAddress, int block ) {
	assert( !isLOOP() );

	if( !(zocketFlags & zoDGRAMBASED) ) {
		return 0;
	}

	int resetBlockingStatus = 0;
	if( isNonBlocking() && block > 0 ) {
		// If the socket defaults to non-blocking, but
		// the local override is set to block, 
		// then temporarily turn blocking on
		resetBlockingStatus = 1;
		setBlocking( 1 );
	}
	else if( !isNonBlocking() && !block ) {
		// If the socket defaults to blocking, but
		// the local override is set to non-blocking, 
		// then temporarily turn blocking off
		resetBlockingStatus = 1;
		setBlocking( 0 );
	}

	int addressSize = sizeof(ZAddress);
	int _recv = recvfrom( sockFD, (char *)buffer, bufferSize, 0, (struct sockaddr *)&zAddress, (socklen_t*)&addressSize );
	if( _recv < 0 ) {
		// When the "non-blocking" code is turned on, 
		// the send can return -1 and WOULD_BLOCK_ERROR_CODE
		// which we deal with by returning 0
		lastErrorCode = SOCKET_LAST_ERROR;
		if( lastErrorCode == SOCKET_WOULD_BLOCK || SOCKET_LAST_ERROR == SOCKET_EAGAIN ) {
			_recv = 0;
		}
		else {
			zocketStateFlags |= zsREAD_WRITE_FAIL;
			_recv = -1;
		}
	}
	zAddress.zocketFlags = getFlags() & zoPROTOCOLS;

	if( resetBlockingStatus ) {
		setBlocking( isNonBlocking() );
	}

	return _recv;
}

int Zocket::dgramWrite( void *buffer, int bufferSize, ZAddress &zAddress, int block ) {
	assert( !isLOOP() );

	if( !(zocketFlags & zoDGRAMBASED) ) {
		return 0;
	}

	int resetBlockingStatus = 0;
	if( isNonBlocking() && block > 0 ) {
		// If the socket defaults to non-blocking, but
		// the local override is set to block, 
		// then temporarily turn blocking on
		resetBlockingStatus = 1;
		setBlocking( 1 );
	}
	else if( !isNonBlocking() && !block ) {
		// If the socket defaults to blocking, but
		// the local override is set to non-blocking, 
		// then temporarily turn blocking off
		resetBlockingStatus = 1;
		setBlocking( 0 );
	}

	int _send = sendto(
		sockFD, (char *)buffer, 
		bufferSize, 0,
		(struct sockaddr *)&zAddress, sizeof(ZAddress)
	);
	if( _send < 0 ) {
		// When the "non-blocking" code is turned on, 
		// the send can return -1 and WOULD_BLOCK_ERROR_CODE
		// which we deal with by returning 0
		lastErrorCode = SOCKET_LAST_ERROR;
		if( lastErrorCode == SOCKET_WOULD_BLOCK || SOCKET_LAST_ERROR == SOCKET_EAGAIN ) {
			_send = 0;
		}
		else {
			zocketStateFlags |= zsREAD_WRITE_FAIL;
			_send = -1;
		}
	}

	if( resetBlockingStatus ) {
		setBlocking( isNonBlocking() );
	}

	return _send;
}

ZAddress Zocket::getRemoteAddress() {
	if( isLOOP() ) {
		return ZAddress( "loop://*", NULL, NULL, getPort() );
	}

	struct sockaddr sa;
	memset( &sa, 0, sizeof(sa) );
	int l = sizeof( struct sockaddr );
	int a = getpeername( sockFD, &sa, (socklen_t*)&l );
	if( a < 0 ) {
		lastErrorCode = SOCKET_LAST_ERROR;
	}
	ZAddress zAddr( sa );
	zAddr.zocketFlags = getFlags() & zoPROTOCOLS;
	return zAddr;
}

