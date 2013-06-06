// @ZBS {
//		*MODULE_OWNER_NAME zfilespec
// }

#ifndef ZFILESPEC_H
#define ZFILESPEC_H

// This is a handy little class to build and parse directory & filenames.
// It is OS independent.  If you feed it '\' under Unix, it will convert them
// and vice versa.
//
// The "zFileSpecMake" function allows you to build up paths from components.
// Specify each component like: makeFileSpec (FS_DIR, "\\", FS_DIR, workDir, FS_FILE, "oink.cpp");
// The tokens allow it to intelligently apply slashes.  Simple filenames like "oink.cpp" will
// get converted to ".\oink.cpp" to normalize it.
// 
// The ZFileSpec class will parse a string.  Its pretty heavy weight as it does this
// parse each time you call a "get" member.

#define ZFILESPEC_WORK_BUFFER_COUNT 16
#define ZFILESPEC_PATH 256

char *zFileSpecMake( int firstToken, ... );
	#define FS_END   (0)
	#define FS_DRIVE (-1)
	#define FS_DIR   (-2)
	#define FS_FILE  (-3)
	#define FS_EXT   (-4)

	// The zFileSpecMake function constructs and normalizes a path based on the
	// tokens that you pass in.
	// You must always pass a token for each string.
	// If anything is wrong the function will return NULL
	//
	// >>>>>>>>>>>>>>> DON'T FORGET TO TERMINATE THE CHAIN WITH FS_END!!! <<<<<<<<<<<<<
	//
	// RULES
	// ------------------------------------------------------------------------
	// The FS_DRIVE token will add the ':' if nec. unless the path starts with UNC \\
	// The FS_DIR will add slashes before and after if nec.
	// The FS_FILE will not add anything 
	// The FS_EXT will add a '.' if nec.
	//
	// Addition rules:
	// FS_DRIVE + FS_DRIVE -- ILLEGAL
	// FS_DRIVE + FS_DIR will always make the FS_DIR in root. (No default drive paths allowed for UNIX compatibility)
	// FS_DRIVE + FS_FILE will always make the FS_FILE in root. (No default drive paths allowed for UNIX compatibility)
	// FS_DIRVE + FS_EXT -- ILLEGAL
	//
	// FS_DIR + FS_DRIVE -- ILLEGAL
	// FS_DIR + FS_DIR will add slashes as necessary
	// FS_DIR + FS_FILE will add slash as necessary
	// FS_DIR + FS_EXT -- ILLEGAL
	//
	// FS_FILE + FS_DRIVE -- ILLEGAL
	// FS_FILE + FS_DIR   -- ILLEGAL
	// FS_FILE + FS_FILE  -- ILLEGAL
	// FS_FILE + FS_EXT   as '.' as necessary
	//
	// FS_EXT + FS_DRIVE -- ILLEGAL
	// FS_EXT + FS_DIR   -- ILLEGAL
	// FS_EXT + FS_FILE  -- ILLEGAL
	// FS_EXT + FS_EXT   as '.' as necessary

#ifdef WIN32
	typedef __int64 S64;
#endif
#ifdef __USE_GNU
	typedef long long S64;
#endif

struct ZFileSpec {
	char path[ZFILESPEC_PATH];
	char drive[ZFILESPEC_PATH];
	char dir[ZFILESPEC_PATH];
	char dirs[64][ZFILESPEC_PATH];
	char file[ZFILESPEC_PATH];
	char ext[ZFILESPEC_PATH];

	static int workBuffer;
	static char workBuffers[ZFILESPEC_WORK_BUFFER_COUNT][ZFILESPEC_PATH];
	static char *getNextWorkBuffer();

	void parse( char *_path, int standardizeSlashses );

	ZFileSpec();
	ZFileSpec( char *_path, int standardizeSlashses=1 );
	void set( char *_path, int standardizeSlashses=1 );

	int isRelative();
	int isAbsolute();
	void makeRelative();
	void makeAbsolute(int _resolveDoubleDots = 0);

	void getCwd();
		// Gets the current working directory into this

	int getNumDirs();

	char *getDrive();
		// "c:/foo/bar.cpp" -> "c:"
		// "/foo/bar.cpp" -> ""
		// "//computer/foo/bar.cpp" -> "computer"

	char *getDir( int n=-1 );
		// n==-1: "c:/foo/bar/file1.txt" -> "/foo/bar/"
		// n==0: "c:/foo/bar/file1.txt" -> "/"
		// n==1: "c:/foo/bar/file1.txt" -> "foo/"
		// n==2: "c:/foo/bar/file1.txt" -> "bar/"
		// n==3: "c:/foo/bar/file1.txt" -> ""
		// n==0: "./foo/bar/file1.txt" -> "./"
		// n==0: "../foo/bar/file1.txt" -> "../"
		// n==0: "foo/bar/file1.txt" -> "foo/"
		// n==1: "foo/bar/file1.txt" -> "bar/"
		// n==2: "foo/bar/file1.txt" -> ""

	char *getDirUpTo( int n=0 );
		// n==0: "c:/foo/bar/file1.txt" -> "/"
		// n==1: "c:/foo/bar/file1.txt" -> "/foo/"
		// n==2: "c:/foo/bar/file1.txt" -> "/foo/bar/"
		// n==3: "c:/foo/bar/file1.txt" -> "/foo/bar/"

	char *getFile( int withExtension=1 );
		// w==1: "c:/foo/bar/file1.txt" -> "file1.txt"
		// w==1: "c:/foo/bar/" -> ""
		// w==1: "c:/foo/bar/file1" -> "file1"
		// w==0: "c:/foo/bar/file1.txt" -> "file1"
		// w==0: "c:/foo/bar/file1.txt.dat" -> "file1.txt"
		// w==1: "c:/foo/bar/file1.txt.dat" -> "file1.txt.dat"

	char *getExt();
		// "c:/foo/bar/file1.txt" -> "txt"
		// "c:/foo/bar/" -> ""
		// "c:/foo/bar/file1" -> ""
		// "c:/foo/bar/file1.txt" -> "txt"
		// "c:/foo/bar/file1.txt.dat" -> "dat"

	unsigned int getTime();

	S64 getSize();

	char *get() {
		return path;
	}

};

int zFileSpecChdir( char *dir );
	// Platform independent chdir; on windows, this will also change the drive
	// if necessary (unlike the windows command prompt)

void zFileSpecUnescapeHTTPURL( char *url );
	// Given an URL in HTTP encoding (%20 for space for example)
	// Overwrites the url


#endif


