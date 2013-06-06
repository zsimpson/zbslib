// @ZBS {
//		*MODULE_NAME zbird
//		+DESCRIPTION {
//			An interface to the flock of birds system
//			using direct serial I/O to avoid various problems with
//			the flock of birds' sdk.
//		}
//		*PORTABILITY win32 unix
//		*ZBSLIB_DEPENDS zserial
//		*SDK_DEPENDS bird
//		*WIN32_DEPENDS
//		*UNIX_DEPENDS
//		*REQUIRED_FILES zbird.cpp zbird.h
//		*OPTIONAL_FILES
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }

// OPERATING SYSTEM specific includes:

// STDLIB includes:
#include "assert.h"

// MODULE includes:
#include "zbird.h"

// ZBSLIB includes:
#include "zserial.h"


int zbirdPort = 0;
int zbirdNumUnits = 0;
int zbirdSampleMode = -1;
int zbirdTransmitterAddress = 0;

#define zbirdMODE_POS (0)
#define zbirdMODE_POSANGLES (1)
#define zbirdMODE_POSMATRIX (2)
#define zbirdMODE_POSQUATERNION (3)
#define zbirdMODE_QUATERNION (4)
#define zbirdMODE_MATRIX (5)
#define zbirdMODE_ANGLES (6)


extern "C" {
__declspec(dllimport) void __stdcall Sleep( unsigned long       dwMilliseconds );
}

static void zBirdSleepMils( int mils ) {
	#ifdef WIN32


//		__declspec(dllimport) void __stdcall Sleep( unsigned long  );
//		__declspec(dllimport) void __stdcall Sleep( unsigned long dwMilliseconds );
		Sleep( mils );
	#else
		assert( 0 );	// TODO: Use select()
	#endif
}

void zbirdInit( int port, int numUnits, int transmitterAddress ) {
	zbirdPort = port;
	zbirdNumUnits = numUnits;
	zbirdTransmitterAddress = transmitterAddress;

	int ok = zSerialOpen( port, 115200, 0, 8, 1 );
	//int ok = zSerialOpen( port, 19200, 0, 8, 1 );
	assert( ok );

	zBirdSleepMils( 600 );
		// This is a sleep function.  Replace with any sleep
		// function if you can't comile this module

	char autoCfgCmd[3] = { 'P', (char)50, (unsigned char)numUnits };
	ok = zSerialWrite( zbirdPort, autoCfgCmd, 3 );

	zBirdSleepMils( 600 );
		// This is a sleep function.  Replace with any sleep
		// function if you can't comile this module

	char nextXmitCmd[2] = { '0', (char)transmitterAddress << 4 };
	ok = zSerialWrite( zbirdPort, nextXmitCmd, 2 );
}

void zbirdClose() {
	zSerialClose( zbirdPort );
	zbirdPort = 0;
	zbirdNumUnits = 0;
}

void zbirdMode( int mode ) {
	zbirdSampleMode = mode;
	char code = 0;
	switch( mode ) {
		case zbirdMODE_POS: code = 'V'; break;
		case zbirdMODE_POSANGLES: code = 'Y'; break;
		case zbirdMODE_POSMATRIX: code = 'Z'; break;
		case zbirdMODE_POSQUATERNION: code = ']'; break;
		case zbirdMODE_QUATERNION: code = '\\'; break;
		case zbirdMODE_MATRIX: code = 'X'; break;
		case zbirdMODE_ANGLES: code = 'W'; break;
	}

	for( int i=1; i<=zbirdNumUnits; i++ ) {
		if( i != zbirdTransmitterAddress ) {
			char sendPosCmd[2] = { (char)0xF0 | (char)i, code };
			int ok = zSerialWrite( zbirdPort, sendPosCmd, 2 );
			assert( ok==2 );
		}
	}
}


void zbirdHemisphere( int which ) {
	char b1, b2;
	switch( which ) {
		case zbirdHEMI_FORWARD:  b1 = 0x00; b2 = 0x00; break;
		case zbirdHEMI_REAR:     b1 = 0x00; b2 = 0x01; break;
		case zbirdHEMI_UPPER:    b1 = 0x0c; b2 = 0x01; break;
		case zbirdHEMI_LOWER:    b1 = 0x0c; b2 = 0x00; break;
		case zbirdHEMI_LEFT:     b1 = 0x06; b2 = 0x01; break;
		case zbirdHEMI_RIGHT:    b1 = 0x06; b2 = 0x00; break;
	}

	// Put into upper hemisphere.  Make this a separate call
	for( int i=1; i<=zbirdNumUnits; i++ ) {
		if( i != zbirdTransmitterAddress ) {
			char hemisphereCmd[4] = { (char)0xF0 | (char)i, 'L', b1, b2 };
			zSerialWrite( zbirdPort, hemisphereCmd, 4 );
		}
	}
}

void zbirdSamplePos( float pos[][3] ) {
	if( !zbirdPort ) return;
	assert( zbirdSampleMode == zbirdMODE_POS );

	int unit = 0;
	for( int i=1; i<=zbirdNumUnits; i++ ) {
		if( i != zbirdTransmitterAddress ) {
			char sendPosCmd[2] = { (char)0xF0 | (char)i, 'B' };
			int ok = zSerialWrite( zbirdPort, sendPosCmd, 2 );
			assert( ok==2 );
			char buffer[6];
			int count = zSerialReadCount( 1, buffer, 6 );

			signed short x = (int(buffer[1]&0x7F)<<9) | (int(buffer[0]&0x7F)<<2);
			signed short y = (int(buffer[3]&0x7F)<<9) | (int(buffer[2]&0x7F)<<2);
			signed short z = (int(buffer[5]&0x7F)<<9) | (int(buffer[4]&0x7F)<<2);

			pos[unit][0] = (float)x * 144.f / 32768.f;
			pos[unit][1] = (float)y * 144.f / 32768.f;
			pos[unit][2] = (float)z * 144.f / 32768.f;
			unit++;
		}
	}
}

void zbirdSamplePosQuaternion( float pos[][3], float quat[][4] ) {
	if( !zbirdPort ) return;
	assert( zbirdSampleMode == zbirdMODE_POSQUATERNION );

	int unit = 0;
	for( int i=1; i<=zbirdNumUnits; i++ ) {
		if( i != zbirdTransmitterAddress ) {
			char sendPosCmd[2] = { (char)0xF0 | (char)i, 'B' };
			int ok = zSerialWrite( zbirdPort, sendPosCmd, 2 );
			assert( ok==2 );
			char buffer[14];
			int count = zSerialReadCount( 1, buffer, 14 );

			signed short x = (int(buffer[1]&0x7F)<<9) | (int(buffer[0]&0x7F)<<2);
			signed short y = (int(buffer[3]&0x7F)<<9) | (int(buffer[2]&0x7F)<<2);
			signed short z = (int(buffer[5]&0x7F)<<9) | (int(buffer[4]&0x7F)<<2);

			pos[unit][0] = (float)x * 144.f / 32768.f;
			pos[unit][1] = (float)y * 144.f / 32768.f;
			pos[unit][2] = (float)z * 144.f / 32768.f;

			signed short q0 = (int(buffer[7 ]&0x7F)<<9) | (int(buffer[6 ]&0x7F)<<2);
			signed short q1 = (int(buffer[9 ]&0x7F)<<9) | (int(buffer[8 ]&0x7F)<<2);
			signed short q2 = (int(buffer[11]&0x7F)<<9) | (int(buffer[10]&0x7F)<<2);
			signed short q3 = (int(buffer[13]&0x7F)<<9) | (int(buffer[12]&0x7F)<<2);

			quat[unit][0] = (float)q0 / 32768.f;
			quat[unit][1] = (float)q1 / 32768.f;
			quat[unit][2] = (float)q2 / 32768.f;
			quat[unit][3] = (float)q3 / 32768.f;

			unit++;
		}
	}
}