#ifndef ZHASHTYPED_H
#define ZHASHTYPED_H

template <class T>
struct ZHashTyped : public ZHashTable {
	T get( char *key, T onEmpty=(T)0 ) {
		char *val = getS( key, (char *)0 );
		return *(T *)val;
	}
	void set( char *key, T val ) {
		putS( key, (char *)&val, sizeof(val) );
	}
};

#endif
