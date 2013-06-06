// @ZBS {
//		*MODULE_OWNER_NAME ztmpstr
// }

#ifndef ZTMPSTR_H
#define ZTMPSTR_H

// This is a convenient class for creating inline
// strings using the familiar printf style.
// For example, suppose that you wanted to open
// a file with a filename constructed on the fly...
//
// new File( TmpStr( "%s.dat", filename ) );
//
#define ZTMPSTR_MAXS 1024
struct ZTmpStr {
	char s[ZTMPSTR_MAXS];
	ZTmpStr( char *fmt, ... );
	ZTmpStr() { s[0] = '\0'; }
	void set( char *fmt, ... );
	void append( char *fmt, ... );
	int getLen();
	char *allCaps();
	char *cap();
	operator char *() { return s; }
};

#endif

