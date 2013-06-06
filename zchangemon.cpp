// @ZBS {
//		*MODULE_NAME Variable Change Monitor
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A class for monitoring changes in variables
//		}
//		+EXAMPLE {
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zchangemon.cpp zchangemon.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH yes
// }

// OPERATING SYSTEM specific includes:
// STDLIB includes:
#include "stdlib.h"
#include "memory.h"
// MODULE includes:
#include "zchangemon.h"
// ZBSLIB includes:
#include "zvars.h"
#include "zregexp.h"

void ZChangeMon::add( void *_ptr, int _size ) {
	ZChangeMonVariable *v = new ZChangeMonVariable;
	v->lastCopy = (void *)malloc( _size );
	memcpy( v->lastCopy, _ptr, _size );
	v->cur = _ptr;
	v->size = _size;
	v->next = head;
	head = v;
}

void ZChangeMon::del( void *_ptr ) {
	ZChangeMonVariable *prev = 0;
	for( ZChangeMonVariable *c=head; c; c=c->next ) {
		if( c->cur == _ptr ) {
			if( prev ) {
				prev->next = c->next;
			}
			else {
				head = c->next;
			}
			if( c->lastCopy ) {
				free( c->lastCopy );
			}
			delete c;
			break;
		}
		prev = c;
	}
}

void ZChangeMon::clear() {
	for( ZChangeMonVariable *v = head; v; ) {
		ZChangeMonVariable *next = v->next;
		if( v->lastCopy ) {
			free( v->lastCopy );
		}
		delete v;
		v = next;
	}
	head = 0;
}

void ZChangeMon::latch() {
	for( ZChangeMonVariable *v = head; v; v=v->next ) {
		memcpy( v->lastCopy, v->cur, v->size );
	}
	forceChange = 0;
}

int ZChangeMon::hasChanged( int _latch ) {
	if( forceChange ) {
		if( _latch ) {
			latch();
		}
		return 1;
	}
	int changed = 0;
	for( ZChangeMonVariable *v = head; v; v=v->next ) {
		changed |= memcmp( v->lastCopy, v->cur, v->size ) == 0 ? 0 : 1;
	}
	if( _latch ) {
		latch();
	}
	return changed ? 1 : 0;
}

int ZChangeMon::hasChanged( void *cur ) {
	if( forceChange ) {
		return 1;
	}
	int changed = 0;
	for( ZChangeMonVariable *v = head; v; v=v->next ) {
		if( v->cur == cur ) {
			return memcmp( v->lastCopy, v->cur, v->size ) == 0 ? 0 : 1;
		}
	}
	return 0;
}

void zchangeMonAddVarsPattern( ZChangeMon *mon, char *pattern ) {
	ZRegExp regExp( pattern );
	int last = -1;
	ZVarPtr *v;
	while( zVarsEnum( last, v ) ) {
		if( regExp.test( v->name ) ) {
			mon->add( v->ptr, v->getPtrSize() );
		}
	}
}
