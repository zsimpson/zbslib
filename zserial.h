// @ZBS {
//		*MODULE_OWNER_NAME zserial
// }

#ifndef ZSERIAL_H
#define ZSERIAL_H

int zSerialOpen( int port, int speed, int parity, int bits, int stopBits );
	// Open the serial port

void zSerialClose( int port );
	// Close the serial port

int zSerialRead( int port, char *buffer, int max, int blocking );
	// zSerialRead will block if requested until some data arrives
	// Returns -1 on an error, 0 for no data, or data count on a successful read
	// When non blocking, it will return 0 if there is nothing to read.

int zSerialReadCount( int port, char *buffer, int count );
	// zSerialReadCount will block until it has received count
	// bytes from the port.

int zSerialWrite( int port, char *buffer, int len );
	// Writes the data to the port

void zSerialEscapeFunction( int port, int function );
	// These macros are stolen from winbase.h
	#define zSerialSETXOFF             1       // Simulate XOFF received
	#define zSerialSETXON              2       // Simulate XON received
	#define zSerialSETRTS              3       // Set RTS high
	#define zSerialCLRRTS              4       // Set RTS low
	#define zSerialSETDTR              5       // Set DTR high
	#define zSerialCLRDTR              6       // Set DTR low
	#define zSerialRESETDEV            7       // Reset device if possible
	#define zSerialSETBREAK            8       // Set the device break line.
	#define zSerialCLRBREAK            9       // Clear the device break line.


#endif
