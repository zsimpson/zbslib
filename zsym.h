#error( "Deprecated" )

// @ZBS {
//		*MODULE_OWNER_NAME zsym
// }

#ifndef ZSYM_H
#define ZSYM_H

#include "zhashtable.h"

// ZSym is a universial URL-like symbol table
// Features:
//   Typed data, easy to access from either string or binary commands
//   Unified name space using urls
//   Reference counting
//   Symbolic links
//   Hookable protocols (memory, file, http*, ftp*, etc.)
//     * Not yet implemented

struct ZSymProtocol {
	// A protocol is a pure virtual interface for hooking
	// the different kinds of protocols such as file, http, etc
	// Eventually this will be extracted from this file and a factory will be made

	#define zsymPROTOCOL_COUNT (2)
	static ZSymProtocol *protocols[zsymPROTOCOL_COUNT];
	static char *protocolNames[zsymPROTOCOL_COUNT];
		// @TODO: This will become a factory at some point

	virtual int resolve( int segCount, char **segs, void **context, int *err, int action )=0;
		// Action codes:
		#define zsymOPEN_OR_FAIL   (0)
		#define zsymOPEN_OR_CREATE (1)

		// Err codes:
		#define zsymRESOLVE_LINK         (-1)
		#define zsymRESOLVE_FILE_NOT_DIR (-2)
		#define zsymRESOLVE_NOT_FOUND    (-3)
		#define zsymRESOLVE_CANNOT_OPEN  (-4)

	virtual void convertNodeType( void *context, int newNodeType, int *err, int safetyOverride )=0;
	virtual void getRaw( void *context, char **rawPtr, int *rawLen, int *err )=0;
	virtual void putRaw( void *context, int fileType, char *rawPtr, int rawLen, int *err )=0;
	virtual int bestConversion( int srcType )=0;
	virtual void dupContext( void *srcContext, void **dstContext )=0;
	virtual void getType( void *context, int *nodeType, int *fileType )=0;
	virtual int createLink( void *context, char *target, int *err )=0;
	virtual int rm( void *context, int *err, int safetyOverride )=0;
	virtual void incRef( void *context )=0;
	virtual void decRef( void *context )=0;
	virtual int cat( void *context, char *s, int len, int growBy, int *err )=0;
	virtual void *listStart( void *context )=0;
	virtual int listNext( void *listContext, char **s, int *len, int *type )=0;
	virtual void getName( void *context, char **name, int *len )=0;
	virtual void dump( void *context, int toStdout, int indent )=0;
};


//=====================================================================================
// PUBLIC
//=====================================================================================

//resolve
//switch the view of this zsym to the symbol
//uses flags to create the symbol if it doesn't exist
//if the last chaacter is a slash, this means create a dir otherwise it creates a folder
//if however, the symbol already exists, it will open it as whatever it is
//so if "/dir/" exists, then "/dir" will still open it as a dir
//if "/file" exists, opening "/file/" converts it to a 
//no you need flags if you want to convert
//
//zsymFORCE_TO_DIR
//zsymFORCE_TO_FILE
//zsymFORCE_AS_SPECIFIED
//zsymOPEN_OR_FAIL
//	// If the symbol doesn't exist, the zsym will be invalid
//zsymOPEN_OR_CREATE
//	// If the symbol doesn't exist, it will be created
//	// If the symbol does exists and the type is different than the force flags
//	// it will be removed and recreated as the force type as an empty key
//zsymNUL_ON_FAIL

// Symbols can have the following states:
//   Does not exist.  This is said to be "undefined"
//   Exists but has empty val.  This is said to be "empty"
//   Exists and has val.  This is said to be "defined"

// When you do a put of a file value on a dir, what should happen?
// Seems like it should fail.  If you wanted it to be a file, you
// should have passed that to the FORCE options of resolve
// However, put of a zsym (deep copy) should always succeed
// it should do whatever is necessary to copy


//void resolve( char *path, int pathLen=-1, int flags );
//void put( char *path, int pathLen=-1, char *val, int valLen=-1, int flags );
//
//puts are destructive unless the protocol requires a safety override
//a protocol and require a safetey override for some ops such as removing a directory
//puts will convert to what the protocol can accept
//
//puts check if the src and dst are 
//
//put from another zsym is a deep copy
//if the src is a dir then so will the target and it will destroy local/


struct ZSym {
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
	//       // You can use printf formats
	//
	//   s.putI( "b", 1 );
	//       // Puts the integer 1 into the LOCAL path b.
	//       // NOTE: this means that /a/b = 1 NOT /b=1
	//
	//   s.putS( "b", "str" );
	//       // Removes the int content of b, replaces it with a string
	//
	//   s.putS( "b", bindata, 10 );
	//       // Strings can be binary if len specified (internal storage
	//       // ALWATS stores a length prefix and adds a NUL even to bin data
	//       // just for good measure
	//
	//   s.putI( "b", 1 );
	//   s.getI( "b" );
	//        // Fetching what you put in, still relative path
	//
	//   s.getI( "/a/b" );
	//        // Fetch from a ABOLSUTE path, NOTE that this does NOT
	//        // change s to look at b, it is just temprary fetch!
	//
	//   ZSym("/a/b").getI()
	//        // A quick grab of a symbol with a local ZSym view
	//        // @TODO: This will create the path, Thomas thinks no?!
	//
	//   s.cd( "/c/", 1 );
	//        // Change the view of s over to the dir "c", the
	//        // extra argument is telling it to create c if if doesn't exist
	//   s.putI( "d", 10 );
	//        // This has created a d under c since that what s current points to
	//   ZSym("/c/d).getI() == 10
	//        // This is a true statement
	//
	//  s.cd( "/c/d" );
	//        // Unlike a typical shell, you can CD to a FILE not just a dir
	//  s.putI( 20 );
	//        // Now /c/d == 20
	//
	//  s.cd( "/" );
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
	//        // True statement.  Invalid symbols return 0 on type requests
	//  viewOfE.getS() == 0
	//        // True statement.  Invalid symbols return 0 on type requests
	//
	//  s.cd( "/a/b/c", 1 );
	//  s.putI( "10" );
	//        // cd creates the dir /a/b if asked to so s points to the FILE /a/b/c
	//        // NOT the dir since c is a file (it didn't end in a slash).  
	//        // This can be confusing since it is unlike shells.
	//  s.putI( "../d", 2 );
	//  ZSym( "/a/b/d" ).getI() == 2
	//        // True statement
	//        // relative paths are realtive to dir NOT file so although s
	//        // was pointing to the FILE c, the .. will create the file /a/b/d

	ZSymProtocol *protocol;
	void *context;
	int err;
	int safetyOverride;
	char *temp;
		// In the case that a type conversion occurs,
		// a temp buffer is needed to store the result.
		// @TODO For now, a duplicate is always made but a 
		// potential optimization in the future is for
		// it to recognize when there's been no type conversion
		// and not make a duplicate

	static void convert( int srcType, char *src, int srcLen, int dstType, char **dst, int *dstLen );
		// This is a private member that implemetned the typecasting matrix

	void dump( int toStdout=1, int indent=0 );

	// Type
	//----------------------------------------------------------
	int isValid();
	int isDir();
	int isFile();
	int isLink();
	int nodeType();
		#define zsymUNK  (0)
		#define zsymFILE (1)
		#define zsymDIR  (2)
		#define zsymLINK (3)
	int fileType();
		#define zsymZ (0)
		#define zsymI (1)
		#define zsymF (2)
		#define zsymD (3)
		#define zsymS (4)

	// Put, Cat
	//----------------------------------------------------------
	// All puts are destructive. Any value currently at
	// that path including subtrees will be removed.
	// For safety, some protocols (filesystem) require an override
	// to permit this behavior so that you can't accidentily erase
	// the whole harddrive.  This override lasts only for the duration of one call
	void overrideSafety();

	void put( ZSym &o );

	void putI( int i );
	void putI( char *path, int i );
	void putS( char *s, int len=-1 );
	void putS( char *path, char *s, int len=-1 );

	void catC( char c, int growBy=-1 );
	void catC( char *path, char c, int growBy=-1 );
	void catI( int i, int growBy=-1 );
	void catI( char *path, int i, int growBy=-1 );
	void catS( char *s, int len=-1, int growBy=-1 );
	void catS( char *path, char *s, int len=-1, int growBy=-1 );
		// Cat concatenates len bytes from s onto the sym
		// If requested to, it will grow the buffer by growBy
		// which is useful if you will be cating a large number small
		// strings to avoid contantly reallocating

	// Get, Len
	//----------------------------------------------------------
	int getI();
	int getI( char *path, int onEmpty=0 );
	char *getS( int *len=0 );
	char *getS( char *path, char *onEmpty="", int *len=0 );

	int len();
	int len( char *path);
		// Returns the length of the file or 0 if it isn't a file or empty
		// This is the raw length before any sort of conversion might be done

	// Create & Remove
	//----------------------------------------------------------
	int link( char *linkName, char *target );
		// Create a symbolic link from linkName to target
	int linkTo( char *target );
		// Create a symbolic link to target from current symbol
	int rm( char *path=0 );
		// Remove the path and all of its children if it is a dir
		// Be careful! This function can REMOVE entire harddrives in one call!
		// On some protocols (filesystem) this function will
		// not recurse unless safetyoveride is activated (see overrideSafety)
	int resolve( char *path, int action=zsymOPEN_OR_CREATE );
		// Given a local or global path, this resolves
		// the path and sets this ZSym up to mimic said path
		// If the path isn't found or can't be created (and action==zsymOPEN_OR_CREATE)
		// the this ends up pointing to an invalid context.  See "cd" below
		// for opposite behavior.
	int cd( char *path, ... );
		// Change to path.  OPEN_OR_FAIL by default
		// If this function fails to open the specified path, it
		// stays in the same path where it started.  Note the
		// difference between this and "resolve" which when it
		// fails points causes "this" to point to an invalid context;
	int cd( int action, char *fmt, ... );
		// Same as above with action override

	// Construct / Destruct
	//----------------------------------------------------------
	void incRef();
	void decRef();
	void clear();
	ZSym();
	ZSym( char *pathFmt, ... );
		// Opens the path or creates it if it doesn't exist
	ZSym( int action, char *pathFmt, ... );
		// This allows you to open without a create
		// The symbol will be invalid if it fails
		// ZSym a( zsymOPEN_OR_FAIL, "/a" );
		// assert( ! a.isValid() );

	ZSym( ZSym &o );
	ZSym &operator=( ZSym &o );

	~ZSym();
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
	ZSymLocal( char *dir=0 );
	~ZSymLocal();
};


struct ZSymDirList {
	// This enumerates a single directory non-recursively
	// Example:
	// for( ZSymDirList l("/"); l.next(); ) {
	//   printf( "%s = %d\n", l.name(), l.zsym().getI() );
	// }

	void *listingContext;
	char *_name;
	int _type;
	ZSym dirSym;
	ZSym currentSym;

	int next();
	char *name() { return _name; }
	ZSym &zsym() { return currentSym; }
	int type() { return _type; }

	ZSymDirList( char *pathFmt, ... );
	ZSymDirList( ZSym &sym );
};

struct ZSymDirListRecurse {
	// This enumerates a directory recursively
	// Example:
	// for( ZSymDirListRecurse l("/"); l.next(); ) {
	//   printf( "%s = %d\n", l.name(), l.zsym().getI() );
	// }

	char tempName[1024];
	int stackTop;
	#define zsymLIST_STACK_SIZE 256
	ZSymDirList *stack[zsymLIST_STACK_SIZE];
	int action;

	int depth() { return stackTop; }
	int next();
	char *name();
	ZSym &zsym() { return stack[stackTop]->zsym(); }
	int type() { return stack[stackTop]->type(); }

	ZSymDirListRecurse( char *pathFmt, ... );
	ZSymDirListRecurse( int _action, char *pathFmt, ... );
		#define zsymIGNORE_LINKS (1)
			// If this action is passed then links will not be recursed into

	ZSymDirListRecurse( ZSym &sym );
	~ZSymDirListRecurse();
};


void zsymDump( char *symbol=0, int toStdout=1, char *label=0 );
	// This is a handy debugging tool for dumping a memory symbol table to either stdout or
	// on windows to OutputDebugString

#endif


