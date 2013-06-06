// @ZBS {
//		*MODULE_NAME Serial Wrapper
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A simple serial I/O interface
//		}
//		*PORTABILITY win32
//		*REQUIRED_FILES zserial.cpp zserial.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH yes
// }


// OPERATING SYSTEM specific includes:
#include "windows.h"
#include "winioctl.h"

// STDLIB includes:
#include "assert.h"

// MODULE includes:
#include "zserial.h"

// ZBSLIB includes:


struct ZSerialInfo {
	HANDLE handle;
	int connected;
	int port;
};

#define MAX_SERIAL_PORTS 8
static ZSerialInfo serialPorts[MAX_SERIAL_PORTS];

int zSerialWrite( int port, char *buffer, int len ) {
	ZSerialInfo *serialInfo = &serialPorts[port];

	unsigned long bytesWritten = 0;
	int ok = WriteFile( serialInfo->handle, buffer, len, &bytesWritten, NULL );
	if( !ok ) {
		return 0;
	}
	return bytesWritten;
}

int zSerialOpen( int port, int speed, int parity, int bits, int stopBits ) {
	// Open a one-based port
	assert( ! serialPorts[port].connected );

	memset( &serialPorts[port], 0, sizeof(ZSerialInfo) );
	serialPorts[port].port = port;

	char portName[5] = "COM ";
	portName[3] = port + '0';

	// open COMM device
	serialPorts[port].handle = CreateFile(
		portName, GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL /*| FILE_FLAG_OVERLAPPED*/,
		NULL
	);

	DCB dcb;
	dcb.DCBlength = sizeof( DCB );
	GetCommState( serialPorts[port].handle, &dcb );

	dcb.BaudRate = speed;							// current baud rate			  
	dcb.fBinary = 1;								// binary mode, no EOF check	  
	dcb.fParity = 1;								// enable parity checking		  
	dcb.fOutxCtsFlow = 0;							// CTS output flow control		  
	dcb.fOutxDsrFlow = 0;							// DSR output flow control		  
	dcb.fDtrControl = DTR_CONTROL_DISABLE;			// DTR flow control type		  
	dcb.fDsrSensitivity = 0;						// DSR sensitivity				  
	dcb.fTXContinueOnXoff = 0;						// XOFF continues Tx			  
	dcb.fOutX = 0;									// XON/XOFF out flow control	  
	dcb.fInX = 0;									// XON/XOFF in flow control 	  
	dcb.fErrorChar = 0; 							// enable error replacement 	  
	dcb.fNull = 0;									// enable null stripping		  
	dcb.fRtsControl = RTS_CONTROL_DISABLE;			// RTS flow control 			  
	dcb.fAbortOnError = 0;							// abort reads/writes on error	  
	dcb.fDummy2 = 0;								// reserved 					  
	dcb.wReserved = 0;								// not currently used			  
	dcb.XonLim = 2048;								// transmit XON threshold		  
	dcb.XoffLim = 512;								// transmit XOFF threshold		  
	dcb.ByteSize = bits;							// number of bits/byte, 4-8 	  
	dcb.Parity = parity;							// 0-4=no,odd,even,mark,space	  
	dcb.StopBits = stopBits-1;						// 0,1,2 = 1, 1.5, 2			  
	dcb.XonChar = 0x11; 							// Tx and Rx XON character		  
	dcb.XoffChar = 0x13;							// Tx and Rx XOFF character 	  
	dcb.ErrorChar = 0;								// error replacement character	  
	dcb.EofChar = 0;								// end of input character		  
	dcb.EvtChar = 0;								// received event character 	  
	dcb.wReserved1 = 0; 							// reserved; do not use 		  

	int ret = SetCommState( serialPorts[port].handle, &dcb ) ;

	// setup device buffers
	SetupComm( serialPorts[port].handle, 8192, 100 );

	EscapeCommFunction( serialPorts[port].handle, SETDTR ) ;
	EscapeCommFunction( serialPorts[port].handle, SETRTS ) ;

	// get any early notifications
	SetCommMask( serialPorts[port].handle, EV_RXCHAR | EV_CTS | EV_DSR | EV_RLSD | EV_ERR | EV_RING );

	serialPorts[port].connected = 1;

	EscapeCommFunction( serialPorts[port].handle, CLRRTS ) ;

	return 1;
}

void zSerialEscapeFunction( int port, int function ) {
	EscapeCommFunction( serialPorts[port].handle, function ) ;
}

int zSerialRead( int port, char *buffer, int max, int blocking ) {
	if( blocking ) {
		while( 1 ) {
			unsigned long eventMask = 0;
			WaitCommEvent( serialPorts[port].handle, &eventMask, NULL );
			if( eventMask & EV_RXCHAR ) {
				unsigned long bytesRead = 0;
				int a = ReadFile( serialPorts[port].handle, buffer, max, &bytesRead, NULL );
				if( !a ) {
					return -1;
				}
				return bytesRead;
			}
		}
	}
	else {
		unsigned long bytesRead = 0;
		int a = ReadFile( serialPorts[port].handle, buffer, max, &bytesRead, NULL );
		if( !a ) {
			return -1;
		}
		return bytesRead;
	}
	return 0;
}

int zSerialReadCount( int port, char *buffer, int count ) {
	int i = 0;
	while( i < count ) {
		int read = zSerialRead( port, &buffer[i], count-i, 1 );
		if( read > 0 ) {
			i += read;
		}
		else if( read < 0 ) {
			return -1;
		}
	}
	return count;
}


void zSerialClose( int port ) {
	ZSerialInfo *p = &serialPorts[port];
	if( p->handle ) {
		CloseHandle( p->handle );
		p->connected = 0;
		p->handle = 0;
	}
}

