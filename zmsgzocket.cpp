// @ZBS {
//		*MODULE_NAME zmsgzocket
//		*MASTER_FILE 1
//		+DESCRIPTION {
//		}
//		+EXAMPLE {
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zmsgzocket.cpp zmsgzocket.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }

#if !defined(STOPFLOW)

// OPERATING SYSTEM specific includes:
#ifdef WIN32
	#include "windows.h"
	#include "winsock.h"
#else
    #include "sys/select.h"
#endif
// SDK includes:
// STDLIB includes:
#include "assert.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#ifdef WIN32
    #include "malloc.h"
#endif
// MODULE includes:
#include "zmsgzocket.h"
// ZBSLIB includes:

#ifndef max
#define max(a,b) ( ( (a) > (b) ) ? (a) : (b) )
#endif

int ZMsgZocket::listCount = 0;
ZMsgZocket **ZMsgZocket::list = 0;
int ZMsgZocket::idCounter = 1;

void ZMsgZocket::addToList() {
	listCount++;
	list = (ZMsgZocket **)realloc( list, listCount * sizeof(ZMsgZocket*) );
	list[listCount-1] = this;
}

ZMsgZocket::ZMsgZocket() {
	idCounter = ((idCounter + 1) % (0x80000000-2)) + 1;
	id = idCounter;

	readBuffer = 0;
	readBufferRemain = 0;
	readBufferFlags = 0;
	readBufferOffset = 0;
	readBufferReadToHTTPEnd = 0;
	readBufferHTTPAllocSize = 0;
	readBufferHTTPSize = 0;
	sendOnConnect = 0;
	sendOnDisconnect = 0;

	addToList();
}

ZMsgZocket::ZMsgZocket( char *address, int _options, char *onConnect, char *onDisconnect ) {
	idCounter = ((idCounter + 1) % (0x80000000-2)) + 1;
	id = idCounter;

	readBuffer = 0;
	readBufferRemain = 0;
	readBufferFlags = 0;
	readBufferOffset = 0;
	readBufferReadToHTTPEnd = 0;
	readBufferHTTPAllocSize = 0;
	readBufferHTTPSize = 0;
	sendOnConnect = 0;
	sendOnDisconnect = 0;

	// TNB - We should have the messages set up before opening to ensure that the message is sent if the
	// socket connects before open() returns.
	if( onConnect ) sendOnConnect = strdup( onConnect );
	if( onDisconnect ) sendOnDisconnect = strdup( onDisconnect );

	open( address, _options );

	addToList();
}

ZMsgZocket::ZMsgZocket( ZMsgZocket *listeningZocket ) : Zocket( listeningZocket ) {
	idCounter = ((idCounter + 1) % (0x80000000-2)) + 1;
	id = idCounter;

	readBuffer = 0;
	readBufferRemain = 0;
	readBufferFlags = 0;
	readBufferOffset = 0;
	readBufferReadToHTTPEnd = 0;
	readBufferHTTPAllocSize = 0;
	readBufferHTTPSize = 0;
	sendOnConnect = 0;
	sendOnDisconnect = 0;
	if( listeningZocket->sendOnConnect ) sendOnConnect = strdup( listeningZocket->sendOnConnect );
	if( listeningZocket->sendOnDisconnect ) sendOnDisconnect = strdup( listeningZocket->sendOnDisconnect );

	addToList();
}

ZMsgZocket::~ZMsgZocket() {
	if( readBuffer ) {
		free( readBuffer );
		readBuffer = 0;
	}

	if( sendOnConnect ) {
		free( sendOnConnect );
		sendOnConnect = 0;
	}
	if( sendOnDisconnect ) {
		free( sendOnDisconnect );
		sendOnDisconnect = 0;
	}

	// FIND this in the list
	for( int i=0; i<listCount; i++ ) {
		if( list[i] == this ) {
			memmove( &list[i], &list[i+1], sizeof(ZMsgZocket*)*(listCount-i-1) );
			listCount--;
			break;
		}
	}
}

void ZMsgZocket::setSendOnConnect( char *msg ) {
	if( sendOnConnect ) {
		free( sendOnConnect );
	}
	sendOnConnect = strdup( msg );
}

void ZMsgZocket::setSendOnDisconnect( char *msg ) {
	if( sendOnDisconnect ) {
		free( sendOnDisconnect );
	}
	sendOnDisconnect = strdup( msg );
}

int ZMsgZocket::read( ZMsg **retMsg, int blockForFullMessage ) {
	int i;

	if( retMsg ) {
		*retMsg = 0;
	}

	if( ! isOpen() ) {
		return -1;
	}

	if( options & ZO_LISTENING ) {
		// ACCEPT incoming connections
		int sockFD = accept( 0 );
		if( sockFD > 0 ) {
			new ZMsgZocket( this );
		}
		return 1;
	}
	else {
		// Check for a valid connection. If we have one, and we haven't sent a message yet, do so.
		// WARNING: isConnected() can close the socket, in which case the it will return FALSE, which will
		// cause this method to return 0. The next call to read() will return -1.
		if( isConnected() ) {
			if( !sendOnConnect || (sendOnConnect && *sendOnConnect) ) {
				if( !(state & ZO_SENT_CONNECTED) ) {
					state |= ZO_SENT_CONNECTED;
					if( retMsg ) {
						*retMsg = new ZMsg;
						(*retMsg)->putS( "type", sendOnConnect?sendOnConnect:(char*)"type=Connect" );
						(*retMsg)->putI( "fromRemote", id );
						return 1;
					}
					else {
						zMsgQueue( "%s fromRemote=%d", sendOnConnect?sendOnConnect:"type=Connect", id );
					}
				}
			}
		}
		else {
			// We are not connected, which could mean two things:
			//  1) We haven't connected yet
			//  2) We failed to connect and will never become connected
			return 0;
		}
	}

	// READ incoming data

	// Header:
	// int flags
	//  0=NOP
	//  1=StrCmd (send to zmsg dispatcher)
	//  2=BinCodedHash
	// int len
	//  binary data of len length

	if( readBufferRemain == 0 ) {
		// AWAITING a new header
		int len = Zocket::read( header, 8, blockForFullMessage );
		if( len == -1 ) {
			close();
			return -1;
		}
		else if( len == 8 ) {
			// CHECK for HTTP request
			if( !memcmp(header,"GET ",4) ) {
				readBufferReadToHTTPEnd = 1;
				readBufferHTTPAllocSize = 1024;
				readBuffer = (char *)malloc( readBufferHTTPAllocSize );
				memcpy( readBuffer, header, 8 );
				readBufferHTTPSize = 8;
			}
			// CHECK for a FLASH security request
			else if( !memcmp(header,"<policy-",8) ) {
				char policyLine[256];
				len = Zocket::read( policyLine, 15 );
				if( len == 15 ) {
					if( !memcmp( policyLine, "file-request/>", 15 ) ) {
						ZMsg *msg = new ZMsg;
						msg->putS( "type", "FlashPolicyFileRequest" );
						msg->putI( "fromRemote", id );
						zMsgQueue( msg );
					}
				}
			}
			else {
				readBufferReadToHTTPEnd = 0;
				readBufferFlags = header[0];
				readBufferRemain = header[1];
					// @TODO: Security here should limit this so that it can't be ridiculously big and therefore crash
				readBufferOffset = 0;
				readBuffer = (char *)malloc( readBufferRemain + 1 );
				memset( readBuffer, 0, readBufferRemain + 1 );
			}
		}
		else if( len != 0 ) {
			// This implies that we read less than a header meaning that something that
			// doesn't talk our protocol has attempted to communicate
			close();
			return -1;
		}
	}

	if( readBufferReadToHTTPEnd ) {
		// AWAITING arrival of two \n indicating the end of the header
		char buffer[1024];
		int bytesRead = Zocket::read( &buffer, sizeof(buffer), blockForFullMessage );
		if( bytesRead == -1 ) {
			close();
			return -1;
		}
		else {
			if( readBufferHTTPAllocSize - readBufferHTTPSize <= bytesRead ) {
				readBufferHTTPAllocSize += bytesRead + 1024;
				readBuffer = (char *)realloc( readBuffer, readBufferHTTPAllocSize );
			}
			memcpy( &readBuffer[readBufferHTTPSize], buffer, bytesRead );
			readBufferHTTPSize += bytesRead;

			// DOES it contain two LFs?
			int count = 0;
			for( i=0; i<readBufferHTTPSize; i++ ) {
				if( readBuffer[i] == '\n' ) {
					count++;
					if( count >= 2 ) {
						break;
					}
				}
				else if( readBuffer[i] != '\r' ) {
					count = 0;
				}
			}
			if( count >= 2 ) {
				ZMsg *msg = new ZMsg;
				msg->putS( "type", "HTTPRequest" );
				msg->putI( "fromRemote", id );

				// PARSE the first line
				int lastStart = 0;

				// SCAN forwards for a space
				for( i=0; i<readBufferHTTPSize; i++ ) {
					if( readBuffer[i] == ' ' ) {
						readBuffer[i] = 0;
						msg->putS( "command", &readBuffer[0] );

						// SCAN forwards for a space
						for( int j=i+1; j<readBufferHTTPSize; j++ ) {
							if( readBuffer[j] == ' ' ) {
								readBuffer[j] = 0;
								msg->putS( "url", &readBuffer[i+1] );

								// SCAN forwards for a eol
								for( int l=j+1; l<readBufferHTTPSize; l++ ) {
									if( readBuffer[l] == '\n' || readBuffer[l] == '\r' ) {
										readBuffer[l] = 0;
										msg->putS( "HTTPVersion", &readBuffer[j+1] );
										lastStart = l;
										break;
									}
								}
								break;
							}
						}
						break;
					}
				}

				// PARSE remaining into key val pairs by CR and colon
				for( i=0; i<readBufferHTTPSize; i++ ) {
					if( readBuffer[i] == '\n' || readBuffer[i] == '\r' ) {
						readBuffer[i] = 0;
						for( int j=lastStart; j<i; j++ ) {
							if( readBuffer[j] == ':' ) {
								readBuffer[j] = 0;
								// EAT whitespace
								j++;
								for( ; j<i; j++ ) {
									if( readBuffer[j]!=' ' ) {
										break;
									}
								}
								msg->putS( &readBuffer[lastStart], &readBuffer[j] );
								break;
							}
						}
						lastStart = i+1;
					}
				}

				if( retMsg ) {
					*retMsg = msg;
				}
				else {
					zMsgQueue( msg );
				}
			}
		}
	}
	else if( blockForFullMessage || readBufferRemain > 0 ) {
		// AWAITING completion of a new data chunk
		int bytesRead = Zocket::read( &readBuffer[readBufferOffset], readBufferRemain, blockForFullMessage );
		if( bytesRead == -1 ) {
			close();
			return -1;
		}
		else {
			readBufferOffset += bytesRead;
			readBufferRemain -= bytesRead;
			if( readBufferRemain == 0 ) {
				// Data chunk is complete

				if( readBufferFlags == 1 ) {
					// A simple text string to send to dispatcher
					ZMsg *msg = zHashTable( "%s", readBuffer );
					msg->putI( "fromRemote", id );
					if( retMsg ) {
						*retMsg = msg;
					}
					else {
						zMsgQueue( msg );
					}
				}
				else if( readBufferFlags == 2 ) {
					// A binary coded hash
					ZMsg *msg = new ZMsg;
					zHashTableUnpack( readBuffer, *msg );
					msg->putI( "fromRemote", id );
					if( retMsg ) {
						*retMsg = msg;
					}
					else {
						zMsgQueue( msg );
					}
				}
				free( readBuffer );
				readBuffer = 0;
				readBufferRemain = 0;
				readBufferFlags = 0;
				readBufferOffset = 0;
			}
		}
	}

	if( ! isConnected() ) {
		// FREE on disconnect
		if( readBuffer ) {
			free( readBuffer );
			readBuffer = 0;
			readBufferRemain = 0;
			readBufferFlags = 0;
			readBufferOffset = 0;
		}
		return -1;
	}
	return 1;
}

void ZMsgZocket::write( ZMsg *msg ) {
	msg->del( "toRemote" );
	msg->del( "fromRemote" );
	msg->del( "__serial__" );
	msg->del( "fromZUI" );

	char *buf = zHashTablePack( *msg );

	// @TODO: Byte ordering

	int header[2];
	header[0] = 2;
	int len = *(int *)buf;
	header[1] = len;

	Zocket::write( header, 8, 1 );
	Zocket::write( buf, len, 1 );

	free( buf );
}

void ZMsgZocket::readList() {
	for( int i=0; i<listCount; i++ ) {
		// [TNB Apr2008] I fixed a subtle problem here. It is possible that we could be open here,
		// the close in read() without returning < 0, in which case read() won't ever be called again
		// and the instance will be forever stuck in the closed state, but whout ever having sent
		// any messages and without being deleted.
		// To solve this, I check both the return result and ask again if the socket is open. If either
		// is true, then we proceed with an orderly shutdown.
		// [ZBS Apr2008] I think I fixed it by putting correct -1 returns into ::read but
		// I don't see anyt harm in calling list[i]->isOpen().  
		if( list[i]->isOpen() && !(list[i]->options & ZO_EXCLUDE_FROM_POLL_LIST) ) {
			int result = list[i]->read();
			if( result < 0 || !list[i]->isOpen() ) {
				// SEND disconnect message if we were open and now we're not
				if( !list[i]->sendOnDisconnect || (list[i]->sendOnDisconnect && *list[i]->sendOnDisconnect) ) {
					zMsgQueue( "%s fromRemote=%d everConnected=%d", list[i]->sendOnDisconnect ? list[i]->sendOnDisconnect : "type=Disconnect", list[i]->id, (list[i]->state & ZO_SENT_CONNECTED) ? 1 : 0 );
				}
				list[i]->state &= ~ZO_SENT_CONNECTED;

				delete list[i];

				// This delete will cause a shift of one so we decrement i
				i--;
			}
		}
	}
}

void ZMsgZocket::dispatch( ZMsg *msg ) {
	int toRemote = msg->getI( "toRemote" );

	// FIND the zocket
	ZMsgZocket *z = 0;
	for( int i=0; i<ZMsgZocket::listCount; i++ ) {
		if( list[i]->id == toRemote ) {
			z = list[i];
			break;
		}
	}

	if( z || toRemote < 0 ) {
		msg->del( "toRemote" );
		msg->del( "__serial__" );
		msg->del( "fromZUI" );

		char *buf = zHashTablePack( *msg );

		// @TODO: Byte ordering

		int header[2];
		header[0] = 2;
		int len = *(int *)buf;
		char version[4];
		memcpy( version, buf, 4 );
		if( !strcmp( version, "1.0" ) ) {
			len = *(int *)(buf+8);
		}
		header[1] = len;

		int fromRemote = msg->getI( "fromRemote" );

		int dispatched=0;
		if( toRemote < 0 ) {
			// SEND to all zockets except in the case of -2 which means to all except the original sender
			for( int i=0; i<listCount; i++ ) {
				if( toRemote == -1 || list[i]->id != fromRemote ) {
					if( ! (list[i]->options & ZO_LISTENING) ) {
						list[i]->Zocket::write( header, 8, 1 );
						list[i]->Zocket::write( buf, len, 1 );
						dispatched=1;
					}
				}
			}
		}
		else {
			// SEND to a particular zocket
			z->Zocket::write( header, 8, 1 );
			z->Zocket::write( buf, len, 1 );
			dispatched=1;
		}
//		if( dispatched ) {
//			printf("( ZMsgZocket::dispatch --> type=%s\n", msg->getS( "type" ) );			
//		}

		free( buf );

		zMsgUsed();
	}
}

void ZMsgZocket::wait( int milliseconds, int includeConsole ) {
	unsigned int largest = 0;

	fd_set readSet;
	fd_set writeSet;
	fd_set exceptSet;

	FD_ZERO( &readSet );
	FD_ZERO( &writeSet );
	FD_ZERO( &exceptSet );

	// ADD all of the zockets into the select list
	for( int i=0; i<ZMsgZocket::listCount; i++ ) {
		if( !(ZMsgZocket::list[i]->options & ZO_EXCLUDE_FROM_POLL_LIST) ) {
			FD_SET( ZMsgZocket::list[i]->sockFD, &readSet );
			FD_SET( ZMsgZocket::list[i]->sockFD, &exceptSet );
			largest = max( largest, ZMsgZocket::list[i]->sockFD );
		}
	}

	if( includeConsole ) {
		#ifdef WIN32
			// In stupid windows you can't select on the console io.  So, you have to
			// either give up on having the server accept commands or
			// you have to set the timeout to something like 100 milliseconds and
			// then check if there's a key press which is what I'm going to do here
			milliseconds = 100;
		#else
			int stdinFD = fileno(stdin);
			FD_SET( stdinFD, &readSet );
			largest = max( largest, stdinFD );
		#endif
	}

	struct timeval t;
	t.tv_sec  = milliseconds / 1000;
	t.tv_usec = milliseconds % 1000;

	// BLOCK until network traffic arrives or timeout
	select( largest+1, &readSet, &writeSet, &exceptSet, ( milliseconds > 0 ) ? &t : 0 );
}


// @TODO: Problem here is that some how in the windows version it
// seems to be non blocking but it is set blocking so of course it is
// locking up when something connects. So I need to either make it
// non blocking or I have to somehow lok at the ISSET flag and pass it on
// so that the readList only calls on those which means putting the readSET
// etc intot he zocket or setting a flag in the state field

ZMsgZocket *ZMsgZocket::find( int id ) {
	for( int i=0; i<listCount; i++ ) {
		if( list[i]->id == id ) {
			return list[i];
		}
	}
	return 0;
}


void ZMsgZocket::dropId( int id ) {
	for( int i=0; i<listCount; i++ ) {
		if( list[i]->id == id ) {
			delete list[i];
			return;
		}
	}
}

int ZMsgZocket::isIdConnected( int id ) {
	ZMsgZocket *zocket = ZMsgZocket::find( id );
	if( zocket ) {
		return zocket->isConnected();
	}
	return 0;
}

#endif

