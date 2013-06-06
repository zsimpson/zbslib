// @ZBS {
//		*MODULE_OWNER_NAME zchangemon
// }

#ifndef ZCHANGEMON_H
#define ZCHANGEMON_H

struct ZChangeMon {
	struct ZChangeMonVariable {
		ZChangeMonVariable *next;
		void *lastCopy;
		void *cur;
		int size;
	};

	ZChangeMonVariable *head;
	int forceChange;
		// Set when you want to simulate the change (set by default on construnction)

	void add( void *_ptr, int _size );
	void del( void *_ptr );
	void clear();
	void latch();
	int hasChanged( int _latch=1 );
	int hasChanged( void *cur );
	void markChange() { forceChange = 1; }

	ZChangeMon() {
		head = 0;
		forceChange = 1;
	}

	~ZChangeMon() {
		clear();
	}
};

void zchangeMonAddVarsPattern( ZChangeMon *mon, char *pattern );

#endif

