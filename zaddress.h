// @ZBS {
//		*MODULE_OWNER_NAME zocket
// }

#ifndef ZADDRESS_H
#define	ZADDRESS_H

// This header file duplicates the sockaddr structures to ensure
// that you won't have to include winsock.h (and thus windows.h)
// in your application code.

// This should compile on most flavors of UNIX also

#define AF_LOOP (20)
#define PF_LOOP (20)

#ifdef WIN32
	#ifndef _WINSOCKAPI_
		#define AF_INET   (2)
		#define PF_INET   (2)
		#define AF_IPX    (6)
		#define PF_IPX    (6)

		extern "C" {
			unsigned long __stdcall  ntohl( unsigned long netlong );
			unsigned short __stdcall ntohs( unsigned short netshort );
			unsigned long __stdcall  htonl( unsigned long netlong );
			unsigned short __stdcall htons( unsigned short netshort );
		}

		struct in_addr {
			union {
				struct { unsigned char s_b1,s_b2,s_b3,s_b4; } S_un_b;
				struct { unsigned short s_w1,s_w2; } S_un_w;
				unsigned long S_addr;
			} S_un;
			#define s_addr  S_un.S_addr
			#define s_host  S_un.S_un_b.s_b2
			#define s_net   S_un.S_un_b.s_b1
			#define s_imp   S_un.S_un_w.s_w2
			#define s_impno S_un.S_un_b.s_b4
			#define s_lh    S_un.S_un_b.s_b3
		};

		struct sockaddr {
			unsigned short sa_family;
			char    sa_data[14];
		};

		struct sockaddr_in {
			short   sin_family;
			unsigned short sin_port;
			struct  in_addr sin_addr;
			char    sin_zero[8];
		};

		#define INVALID_SOCKET -1
		typedef int SOCKET;
	#endif

	#define IPX_SUPPORT
		// For now, I'm only compliing in IPX support for Windows
#endif

#ifndef _WSIPX_
	struct sockaddr_ipx {
		short sa_family;
		char sa_netnum[4];
		char sa_nodenum[6];
		unsigned short sa_socket;
	};
#endif

#ifndef WIN32
	#ifdef sgi
		#include "sys/endian.h"
	#endif
	#ifdef __linux__
		#include "netinet/in.h"
	#endif
	#ifdef bsdi
		#include "sys/types.h"
		#include "sys/socket.h"
		#include "netinet/in.h"
		#include "sys/param.h"
	#endif
	#ifdef __APPLE__
#include "sys/types.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "sys/param.h"
#endif

	#define INVALID_SOCKET -1
	typedef int SOCKET;
#endif


#ifndef TRUE
	#define TRUE 1
#endif

#ifndef FALSE
	#define FALSE 0
#endif

typedef char IPXBinaryAddr[10];

// This is all kind of silly since ntoh and hton do the same damn thing!
// The nice thing about ntohX is that it is properly defined on each machine
// implementation, although making a function call is obviously sub-optimal

// C++ overloads for the ntoh functions
inline short ntoh( short x ) { return ntohs(x); }
inline int   ntoh( int   x ) { return ntohl(x); }
inline long  ntoh( long  x ) { return ntohl(x); }
inline short hton( short x ) { return htons(x); }
inline int   hton( int   x ) { return htonl(x); }
inline long  hton( long  x ) { return htonl(x); }

// Byte ordering overloads for each atomic data type
inline void reorderMe( char   &x ) { }
inline void reorderMe( short  &x ) { x = ntohs(x); }
inline void reorderMe( int    &x ) { x = ntohl(x); }
inline void reorderMe( long   &x ) { x = ntohl(x); }
inline void reorderMe( float  &x ) { int a = *((int *)&x); x = *(float *)&a; }
inline void reorderMe( double &x ) { /*** Need to add this*/ }

int dottedDecimalToIp( char *dd );
	// Given a string in the form "208.201.96.13" returns the int ip address

char *ipToDottedDecimal( int ip );
	// Gives a string in dotted decimal form from and int ip.
	// Uses the system call which typically uses returns the same buffer,
	// so don't hold on to the pointer

// ZAddress
//--------------------------------------------------------------------------------
// This structure is a massive simplification of the unions
// used in the socket structures.  The source for this is in zocket.cpp

struct ZAddress {
	// This is my little address class which hides all of the crazy
	// overloaded union structs that are used for different address types.
	// 
	// It also adds options and protocol information that a
	// socket address doesn't have like: TCP vs UDP.
	//
	// Addresses can be set and retrieved either binary or
	// string based.  There is a standard format for the
	// string represenation that is conviently human readable:
	//
	//   protocol(options)://address:port/
	//
	// Where protocol is: (tcp | udp | ipx | spx | loop)
	//       options  is:  (b = non-blocking, r = reuse addr, n = disable-nagle, l = listening)
	//       port     is: decimal port number
	//       address  is: FQDN or dotted-decimal or semicolon'd-hexidecimal
	//                     IP examples:
	//                        "grinch.titanic.com"
	//                        "128.0.0.2"
	//                     IPX examples: (IPX must be in hex)
	//                        "4f-23-56-12-00-e3"
	//                        "0001-4f-23-56-12-00-e3"
	//                          (if they have > 6 parts, high part assumed to be network)
	//
	// Note that all binary functions do appropriate byte reordering
	// automatically.  All values in and out should be native endian

  friend struct Zocket;
  protected:
	struct sockaddr sockAddr;
	int zocketFlags;
		// This list of flags has all kinds of illegal
		// configurations and should never be touched
		// directly.  Each possible variable is given its
		// own flag to simplify the writing of zocket code.
		#define zoTCP              (0x001)
		#define zoUDP              (0x002)
		#define zoIP_FAMILY        (0x004)
		#define zoSPX              (0x008)
		#define zoIPX              (0x010)
		#define zoIPX_FAMILY       (0x020)
		#define zoLOOP             (0x040)
		#define zoLISTENING        (0x080)
		#define zoNONBLOCKING      (0x100)
		#define zoREUSEADDR        (0x200)
		#define zoDISABLE_NAGLE    (0x400)

		#define zoPROTOCOLS       (zoTCP|zoUDP|zoIP_FAMILY|zoSPX|zoIPX|zoIPX_FAMILY|zoLOOP)
		#define zoOPTIONS         (zoLISTENING|zoNONBLOCKING|zoREUSEADDR|zoDISABLE_NAGLE)
		#define zoDGRAMBASED      (zoUDP|zoIPX)
		#define zoSTREAMBASED     (zoTCP|zoSPX)

  public:
	// Protocol Independant binary API
	int getFlags() { return zocketFlags; }

	int getPort();
	void setPort( int port );

	char *getProtocol();
	void setProtocol( char *protocol );

	void setAddress( char *address );
		// sets only the address part
		// see: getString() for retrievig this value

	// Individual options.  Note, not all options
	// are applicable to all protocols.  It doesn't hurt
	// to turn on an option for a no-supported protocol.
	//----------------------------------
	void setOptions( char *options );

	// non-blocking = 'b'
	int isNonBlocking();
	void setNonBlocking( int x=1 );

	// nagle disable = 'n'
	int isNagleDisabled();
	void setNagleDisabled( int x=1 );

	// resuse address = 'r'
	int isReuseAddr();
	void setReuseAddr( int x=1 );

	// listening address = 'l'
	int isListening();
	void setListening( int x=1 );


	// TCP/IP binary API
	//----------------------------------
	int isIP();
	int getIP();
	void setIP( int ip, int port=0 );
	int isIPLoopback();
		// Is the address 127.0.0.1
	int fillMyIPAddress( int allowReserved=1 );
		// Fills out this struct with my IP address derived from 
		// gethost and gethostbyname.
		// You can ask this routine to make sure it picks the non-reserved NIC on your machine
		// by passing in allowReserved==0 (Added by KLD 31 Aug 2000)
		// Returns 0 if it fails, 1 otherwise

	// SPX/IPX binary API
	//----------------------------------
	int isIPX();
	void getIPX( IPXBinaryAddr &addr );
	void setIPX( IPXBinaryAddr addr, int port=0 );
	int fillMyIPXAddress();
		// Fills out ipx address by creating a temporary socket
		// and doing a getsockname() on it

	// Loop binary API
	//----------------------------------
	int isLOOP();
		// Loop protocol has no address, only port


	// Constructors and general
	//----------------------------------
	ZAddress();
	ZAddress( int ip, int port=0 );
	ZAddress( struct sockaddr &_sockAddr );
	ZAddress( char *zAddress, char *protocol=(char*)0, char *options=(char*)0, int port=0 );
		// Calls set(), see below

	void reset();
		// Clears

	void set( ZAddress zAddress );

	void set( char *zAddress, char *protocol=(char*)0, char *options=(char*)0, int port=0 );
		// This is the most flexible of the constructors, the
		// zAddress is encoded string as noted above.
		// You may overtly set the protocol, options, and port;
		// if you do this, they will overload anything specified
		// in the zAddress string.

	int isValid();
		// Has the address been initialized?

	int getName( char *buffer, int bufferlen );
		// Does a reverse name lookup and writes the result into buffer
		// This may block for quite sometime.  Not available for all protocols.
		// Returns zero if lookup failed.

	char *getString( int withProtocol=0, int withOptions=0, int withPort=1 );
		// Returns a standard encoded string appropriate for
		// the protocol.  Uses a queue of static strings
		// so don't keep the pointer around for long.

	void setProtocolConnectionBased();
		// If IP family, switches to TCP.  If ipx switches to SPX

	void setProtocolDgramBased();
		// If IP family, switches to UDP.  If ipx switches to IPX
};

#endif


