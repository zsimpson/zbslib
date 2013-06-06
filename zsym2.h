// @ZBS {
//		*MODULE_OWNER_NAME zsym
// }

#ifndef ZSYM_H
#define ZSYM_H


// Flags
//--------------------------------------------
#define zsymOPEN_OR_FAIL (0x0001)
	// If the symbol doesn't exist, the zsym will be invalid
#define zsymOPEN_OR_CREATE (0x0002)
	// If the symbol doesn't exist, it will be created
	// If the symbol does exists and the type is different than the force flags
	// it will be removed and recreated as the force type with an empty val
#define zsymFORCE_TO_DIR (0x0004)
	// The symbol will be created (or converted if already exists) as a directory
#define zsymFORCE_TO_FILE (0x0008)
	// The symbol will be created (or converted if already exists) as a file
#define zsymFORCE_AS_SPECIFIED (0x0020)
	// The symbol will be created (or converted if already exists) as specified
	// by the last character of the path.  "/a/" represents a dir.
#define zsymNUL_ON_FAIL (0x0040)
	// When you resolve a symbol that fails, the default is that the ZSym
	// will still be pointing at whereever it was before the failure. This
	// flag reverses the default, on failure it will point to nul
#define zsymDONT_FOLLOW_LINKS (0x0080)
	// The resolver will stop on the first link it hits

// Node Types
//--------------------------------------------
#define zsymUNK  (0)
#define zsymFILE (1)
#define zsymDIR  (2)
#define zsymLINK (3)

// File Types
//--------------------------------------------
#define zsymZ  ( 0) // Unknown file type
#define zsymI  ( 1) // signed 32 bit int
#define zsymF  ( 2) // 32 bit float
#define zsymD  ( 3) // 64 bit float
#define zsymS  ( 4) // String value (length prefix encoded and ALSO nul terminated)
#define zsymR  ( 5) // @TODO Routine (Function pointer)
#define zsymIP ( 6) // int pointer
#define zsymFP ( 7) // @TODO float pointer
#define zsymDP ( 8) // @TODO double pointer
#define zsymSP ( 9) // @TODO string pointer (generic memory pointer)

// The following are special psuedo-types that are used
// internally so that the conversion routines know to "set"
// the value pointed to by the pointer instead of the pointer itself.
// Do not use any of the following "s" symbols directly
#define zsymIPs (10) // int pointer set
#define zsymFPs (11) // @TODO float pointer set
#define zsymDPs (12) // @TODO double pointer set
#define zsymSPs (13) // @TODO string pointer (generic memory pointer) set

// Error Types
//--------------------------------------------
#define zsymRESOLVE_LINK                (-1)
#define zsymRESOLVE_FILE_NOT_DIR        (-2)
#define zsymRESOLVE_NOT_FOUND           (-3)
#define zsymRESOLVE_CANNOT_OPEN         (-4)
#define zsymRESOLVE_CANNOT_CONVERT_ROOT (-5)



//=====================================================================================
// PUBLIC
//=====================================================================================

struct ZSym {
	// @TODO: Add pointer logic to this doc
	//
	// This is the principal interface to using symbol tables
	// At any given time, this class points to some symbol node
	//
	// Examples:
	//
	//   ZSym s( "/a/" );
	//       // This would open a folder "a" under the root
	//       // if /a/ did not already exists, it would create it
	//
	//   ZSym s( "/%s/", "a" );
	//       // You can use printf formats, again would create symbol if it didn't exist
	//
	//   ZSym s( zsymOPEN_OR_FAIL, "/a/" );
	//       // If /a/ didn't exist, it would fail (s would be invalid)
	//
	//   ZSym s( zsymOPEN_OR_CREATE | zsymFORCE_TO_DIR, "/a" );
	//       // If /a/ didn't exist or it wasn't a dir, it would be created or
	//       // converted to a directory.  This is a potentially destructive operation
	//
	//   s.putI( 1, "b" );
	//       // Puts the integer 1 into the LOCAL path b.
	//       // NOTE: this means that /a/b = 1 NOT /b=1 because s was previous set to the dir /a/
	//
	//   s.putS( "str", -1, "b" );
	//       // Removes the int content of b, replaces it with the string "str"
	//       // the -1 tells it to use strlen on "str" otherwise you could pass len (3 in this case)
	//
	//   s.putS( bindata, 10, "b" );
	//       // Strings can be binary if len specified (internal storage
	//       // ALWATS stores a length prefix and adds a NUL even to bin data
	//       // just for good measure
	//
	//   s.putI( 1, "b" );
	//   s.getI( "b" );
	//        // Fetching what you put in, still relative path.
	//        // That is, the putI did not change the context if s
	//
	//   s.getI( "/a/b" );
	//        // Fetch from a ABOLSUTE path, NOTE that this does NOT
	//        // change s to look at b, it is just temprary fetch!
	//
	//   ZSym("/a/b").getI()
	//        // A quick grab of a symbol with a local ZSym view
	//        // Note that this will create the whole path /a/b if it doesn't exist
	//        // unless you override this behavior as in:
	//
	//   ZSym(zsymOPEN_OR_FAIL,"/a/b").getI()
	//        // If /a/b didn't exist, this would return 0
	//
	//  s.resolve( "/c/d" );
	//        // Unlike a typical shell, you can resolve (which is like cd in a shell) to a FILE not just a dir!
	//  s.putI( 20 );
	//        // Now /c/d == 20
	//
	//  s.resolve( "/" );
	//  s.putI( "e", 30 );
	//  ZSym viewOfE( "/e" );
	//  viewOfE.getI() == 30
	//        // True statement
	//  s.rm( "/e" );
	//        // Removing a symbol while some other ZSym is looking at it!
	//  viewOfE.isValid()
	//        // False statement.  The viewOfE points to a node which is invalid
	//        // Once viewOfE passes out of scope, its reference count on
	//        // the memory of what was /e will be removed and when the
	//        // refCoutn gets to zero then that memory will be freed
	//  viewOfE.getI() == 0
	//        // True statement.  Invalid symbols return 0 on get requests
	//
	//  s.resolve( "/a/b/c" );
	//  s.putI( "10" );
	//        // resolve creates the dir /a/b and the file c
	//  s.putI( "../d", 2 );
	//  ZSym( "/a/b/d" ).getI() == 2
	//        // True statement
	//        // relative paths are realtive to dir NOT file so although s
	//        // was pointing to the FILE c, the .. will create the file /a/b/d
	//
	//  ZSym url( "file:///transfer/foo" );
	//  url.putI( 1 );
	//        // Creates the path /transfer/ and then makes a 4 byte file called 'foo'
	//        // with the int value 1 in binary in the file
	//  ZSym url( "file:///" );
	//        // A dangerous reference to the entire file structure!
	//  url.putI( 1 );
	//        // This would WIPE OUT the entire file system and replace it with
	//        // a single file 1!! So this can't happen unless you explicitly
	//        // set the safetyOverride flag

	struct ZSymProtocol *protocol;
		// The protocol (mem, file, http, etc)
	void *context;
		// The result of a resolve
	int err;
		// Result of last protocol function called
	int safetyOverride;
		// Set temporarily to override dangerous operations
	char *temp;
		// In the case that a type conversion occurs,
		// a temp buffer is needed to store the result.

	static void convert( int srcType, char *src, int srcLen, int dstType, char **dst, int *dstLen );
		// This is a private member that implemetned the typecasting matrix

	// Resolve
	//----------------------------------------------------------
	int resolve( int flags, char *path, int pathLen=-1 );
		// Most specific version of resolve takes all arguments
	int resolve( char *path, int pathLen=-1 );
		// This version assumes zsymOPEN_OR_CREATE and zsymFORCE_AS_SPECIFIED

	// Import
	//----------------------------------------------------------
	void import( ZSym o, char *path=0, int pathLen=-1 );
		// Pull the contents of o into this symbol
		// This must be a dir
		// If o is a file then it is pulled in with either the path name or
		// if pathis 0 then a temp name is given.  If o is a dir then
		// the contents o are all sucked into this name space.

	// Type, Name
	//----------------------------------------------------------
	int isValid();
	int isDir();
	int isFile();
	int isLink();
	int nodeType();
	int fileType();
		// If nodeType is zsymFIL then this returns the file type otherwise zsymZ
	void getType( int &nodeType, int &fileType );
		// Gets both the nodeType and fileType in one call

	void convertNodeType( int nodeType );
		// Converts this node to the specified type 
	void convertFileType( int fileType );
		// Converts this file to the specified type using the convert matrix

	char *nameLocal( int *len=0 );
		// Fetch the name of this symbol in the parent's space (mallocs, you must free)
		
	char *nameGlobal( int *len=0 );
		// Fetch the name of this symbol in the global fully qualified space (mallocs, you must free)

	// Put, Cat
	//----------------------------------------------------------
	void overrideSafety();

	int put( ZSym &src );
		// This is a deep copy.  The destination will be detroyed
		// and converted to the type of the src 
		// unless an error occurs such as a safety override failure
		// Returns success

	// Each of the following puts resolves the path relative to current context,
	// using zsymOPEN_OR_CREATE and zsymFORCE_FILE.
	// This will DESTROY the dst unless an error occurs such as a safety override failure

	int putI( char *path, int i, int pathLen=-1 );
	int putI( int i );

	int putS( char *path, char *val, int pathLen=-1, int valLen=-1 );
	int putS( char *val, int valLen=-1 );

	int putR( char *path, void *funcPtr, char *argStr, int pathLen=-1 );
	int putR( void *funcPtr, char *argStr );


	int catC( char *path, char c, int pathLen=-1, int growBy=-1 );
	int catC( char c, int growBy=-1 );

	int catI( char *path, int i, int pathLen=-1, int growBy=-1 );
	int catI( int i, int growBy=-1 );

	int catS( char *val, int valLen=-1, int growBy=-1 );
	int catS( char *path, char *val, int pathLen=-1, int valLen=-1, int growBy=-1 );
		// 'cat' concatenates len bytes from s onto the sym
		// If requested to, it will grow the buffer by growBy
		// which is useful if you will be cating a large number of small
		// strings to avoid contantly reallocating

	// Ptr
	//----------------------------------------------------------
	int ptrI( char *path, int *i, int pathLen=-1 );
	int ptrI( int *i );

	// Get, Len, Exec
	//----------------------------------------------------------
	int has( char *path, int pathLen=-1 );
		// returns true is the path exists

	int getI( char *path, int pathLen=-1, int onFail=0 );
		// Returns the int or 'onFail' on failure
	int getI();
		// Returns the int or 0 on failure

	char *getS( char *path, int pathLen=-1, char *onFail="", int *len=0 );
		// Returns a raw pointer (NOT A COPY) and optionally fetches the valLen
	char *getS( int *valLen=0 );
		// Returns a raw pointer (NOT A COPY) and optionally fetches the valLen

	int len();
	int len( char *path, int pathLen=-1 );
		// Returns the length of the file or 0 if it isn't a file
		// This is the raw length before any sort of conversion might be done
		// This is only useful if you know that no conversion is going to take place

	void exeR( char *path, ZSym &args, ZSym &ret, int pathLen=-1 );
	void exeR( ZSym &args, ZSym &ret );

	// Links
	//----------------------------------------------------------
	int link( char *linkName, char *target, int linkNameLen=-1, int targetLen=-1 );
		// Create a symbolic link from linkName to target
		// The sym will resolve the linkName

	// Remove
	//----------------------------------------------------------
	int rm( char *path=0, int pathLen=-1 );
		// Remove the path and all of its children if it is a dir
		// Be careful! This function can REMOVE entire harddrives in one call!
		// On some protocols (filesystem) this function will
		// not recurse unless safetyoveride is activated (see overrideSafety)

	// List
	//----------------------------------------------------------
//	void pushHead( ZSym &o );
//	ZSym popHead();
	void pushTail( ZSym &o );
//	ZSym popTail();
//	void listStats( int &count, int &minimim, int &maximum );

	// Debugging
	//----------------------------------------------------------
	void dump( int toStdout=1, int indent=0 );

	// Construct / Destruct
	//----------------------------------------------------------
	void incRef();
	void decRef();

	void clear();
		// defRefs the current symbol and sets protocol, context to 0, frees temps

	ZSym();
		// Opens with invalid context
	ZSym( char *path, int pathLen=-1 );
		// Assumes zsymOPEN_OR_CREATE and zsymFORCE_AS_SPECIFIED
	ZSym( int flags, char *path, int pathLen=-1 );
		// Pass flags explicitily

	ZSym( ZSym &o );
		// Copies the context (and incRefs).  Does NOT copy the value!
	ZSym &operator=( ZSym &o );
		// Copies the context (and incRefs).  Does NOT copy the value!

	~ZSym();
		// decRefs the symbol
};


struct ZSymLocal : ZSym {
	// Creates a local directory with a temp name assigned 
	// to it under dir (or root if not specified) in form "/__tmpX"
	// and removes it on destruction.  Use it in situations where you want a local hash table
	// Example:
	// void func() {
	//   ZSymLocal l;
	//   l.putI( "a", 1 );
	//   l.putI( "b", 1 );
	// }
	// All of the symbols will be removed upon exit of func()
	static int serial;
	int localSerial;
	ZSymLocal( char *dir=0, int flags=zsymFORCE_TO_DIR );
		// Note that zsymOPEN_OR_CREATE is always set
	~ZSymLocal();
};

struct ZSymDirList {
	// This enumerates a single directory non-recursively
	// Example:
	// for( ZSymDirList l("/"); l.next(); ) {
	//   printf( "%s = %d\n", l.name(), l.zsym().getI() );
	// }

	void *listingContext;
	char *_path;
	int _pathLen;
	int _nodeType;
	ZSym dirSym;
	ZSym currentSym;

	int next();
	char *path( int *pathLen=0 ) { if( pathLen ) *pathLen = _pathLen; return _path; }
	int nodeType() { return _nodeType; }
	ZSym &zsym() { return currentSym; }

	ZSymDirList( char *path, int pathLen=-1 );
	ZSymDirList( ZSym &sym );
};

struct ZSymDirListRecurse {
	// This enumerates a directory recursively
	// Example:
	// for( ZSymDirListRecurse l("/"); l.next(); ) {
	//   printf( "%s = %d\n", l.name(), l.zsym().getI() );
	// }

	char *tempName;
	int stackTop;
	#define zsymLIST_STACK_SIZE 256
	ZSymDirList *stack[zsymLIST_STACK_SIZE];
	int flags;

	int depth() { return stackTop; }
	int next();
	char *path( int *pathLen=0 );
	int nodeType() { return stack[stackTop]->nodeType(); }
	ZSym &zsym() { return stack[stackTop]->zsym(); }

	ZSymDirListRecurse( int flags, char *path, int pathLen=-1 );
		#define zsymIGNORE_LINKS (1)
			// If this action is passed then links will not be recursed into
	ZSymDirListRecurse( char *path, int pathLen=-1 );
		// Assumes flags = 0

	ZSymDirListRecurse( ZSym &sym );
	~ZSymDirListRecurse();
};


void zsymDump( char *symbol=0, int toStdout=1, char *label=0 );
	// This is a handy debugging tool for dumping a memory symbol table to either stdout or
	// on windows to OutputDebugString
	// @TODO: Accept binary name

#endif


