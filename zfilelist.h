// @ZBS {
//		*MODULE_OWNER_NAME zfilelist
// }

#ifndef ZFILELIST_H
#define ZFILELIST_H

// ZFileList is a list of files that matches a pattern

struct ZFileList {
	int allocCount;
	int activeCount;
	char **list;

	void clear();
	void addFile( char *filename );
	void addDirFile( char *dir, char *filename );
	void addPat( char *dir, char *regexp );
	int count() { return activeCount; }
	char *get( int i, int mod=1 ) { if( !mod ) if( i < 0 || i >= activeCount ) return 0; else return list[i]; else return list[i%activeCount]; }
	int find( char *filename );

	ZFileList();
	ZFileList( char *dir, char *regexp );
	~ZFileList();
};

#endif