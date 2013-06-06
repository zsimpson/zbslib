// @ZBS {
//		*MODULE_NAME Win32 Stack Tracking and Symbol Decoding
//		+DESCRIPTION {
//			stack tracking for win32
//		}
//		*PORTABILITY win32
//		*REQUIRED_FILES zstacktrace.cpp zstacktrace.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST yes console
//		*PUBLISH yes
// }
// OPERATING SYSTEM specific includes:
#ifdef WIN32
	#include "windows.h"
#endif
// SDK includes:
// STDLIB includes:
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
// MODULE includes:
#include "zstacktrace.h"
// ZBSLIB includes:


int zStackTrace( int size, unsigned int *ips, unsigned int *args ) {
	// Walks up the stack placing instruction pointers and
	// argument pointers into the arrays of 'size' length
	// returns the number of entries extracted.

return 0;
// In MSDEV9.0 the try / catch isn't working in the debugger so for now
// I'm just skipping this code.

	#ifdef _DEBUG
		unsigned int baseAddress = (int)GetModuleHandle( NULL );

		int i = 0;
		volatile unsigned int *currBP;
		__asm mov [currBP], ebp;
		try {
			for( i=0; i<size; i++ ) {
				if( args ) args[i] = (unsigned)(currBP+2);
				if( currBP == 0 ) {
					break;
				}

				if( ips ) ips[i] = *(currBP+1) - baseAddress;
				currBP = (unsigned *)*(currBP);
			}
		}
		catch( ... ) {
		}
		return i;
	#else
		return 0;
	#endif
}

char *zStackTraceAndGetString() {
	unsigned int ips[30];
	unsigned int args[30];
	int size = zStackTrace( 30, ips, args );
	return zStackTraceGetString( size, ips, args );
}

static HANDLE stackTrace_hFile = NULL;
static HANDLE stackTrace_hFileMapping = NULL;
static char *stackTrace_basePtr = NULL;

int zStackTraceOpenFiles() {
	// Get the exe name from the command line
	// It is surrounded by quotes in some cases
	char *exeName = GetCommandLine();
	if( *exeName == '"' ) {
		exeName++;
	}
	char *space = strchr( exeName, ' ' );
	if( space ) {
		*space = 0;
	}
	char *quote = strchr( exeName, '"' );
	if( quote ) {
		*quote = 0;
	}

	stackTrace_hFile = CreateFile( exeName, GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0
	);
	if( stackTrace_hFile == INVALID_HANDLE_VALUE ) {
		return 0;
	}
	stackTrace_hFileMapping = CreateFileMapping( stackTrace_hFile, NULL, PAGE_READONLY, 0, 0, NULL );
	if( stackTrace_hFileMapping == NULL ) {
		CloseHandle( stackTrace_hFile );
		stackTrace_hFile = NULL;
		return 0;
	}
	stackTrace_basePtr = (char *)MapViewOfFile( stackTrace_hFileMapping, FILE_MAP_READ, 0, 0, 0 );
	if( stackTrace_basePtr == NULL ) {
		CloseHandle( stackTrace_hFileMapping );
		CloseHandle( stackTrace_hFile );
		stackTrace_hFileMapping = NULL;
		stackTrace_hFile = NULL;
		return 0;
	}
	return 1;
}

void zStackTraceCloseFiles() {
	if( stackTrace_hFileMapping ) CloseHandle( stackTrace_hFileMapping );
	if( stackTrace_hFile ) CloseHandle( stackTrace_hFile );
}

static int symbolPtrCompare( const void *a, const void *b ) {
	IMAGE_SYMBOL **aa = (IMAGE_SYMBOL **)a;
	IMAGE_SYMBOL **bb = (IMAGE_SYMBOL **)b;
	return (*aa)->Value > (*bb)->Value ? 1 : ((*aa)->Value < (*bb)->Value ? -1 : 0);
}

static IMAGE_SYMBOL *binarySearchSymbol( IMAGE_SYMBOL **base, int num, unsigned int ip ) {
	int width = sizeof( IMAGE_SYMBOL *);
	char *lo = (char *)base;
	char *hi = (char *)base + (num - 1) * width;
	char *mid;
	unsigned int half;

	while (lo <= hi) {
		if( half = num / 2 ) {
			mid = lo + (num & 1 ? half : (half - 1)) * width;

			IMAGE_SYMBOL *midSym = *(IMAGE_SYMBOL **)mid;
			if( midSym->Value == ip ) {
				return(midSym);
			}
			else if( ip < midSym->Value ) {
				hi = mid - width;
				num = num & 1 ? half : half-1;
			}
			else {
				lo = mid + width;
				num = half;
			}
		}
		else if( num ) {
			IMAGE_SYMBOL *a = *(IMAGE_SYMBOL **)lo;
			while( a->Value > ip ) {
				lo -= width;
				a = *(IMAGE_SYMBOL **)lo;
			}
			return a;
		}
		else {
			break;
		}
	}
	return 0;
}



static int stackTracePrebuiltSymbolTable = 0;
static IMAGE_SYMBOL **stackTraceSymbolTable = 0;
static int stackTraceSymbolTableSize = 0;
static char *stackTraceStringTable = 0;
static IMAGE_COFF_SYMBOLS_HEADER *stackTraceCoffHeader = 0;
static int stackTraceCoffSymbolCount = 0;
static IMAGE_NT_HEADERS *stackTraceNtHeader = 0;
static unsigned int stackTraceBaseAddress = 0;

void zStackTraceBuildSymbolTable() {
	if( stackTracePrebuiltSymbolTable ) {
		// Already built
		return;
	}

	// Figure the base load address with GetModuleHandle
	stackTraceBaseAddress = (unsigned int)GetModuleHandle( NULL );

	// Memory map the exec file.  This is done becuase
	// it is much harder to parse in memory since it
	// parts of it are moved around by the loader.
	// Also, we must use CreateFile here because of file sharing
	// since the file is currently opened by the process.
	zStackTraceOpenFiles();

	// Now parse the file extracting the debug information
	// This involves quite a lot of header parsing
	// Ultimately, we will find there three variables:
	int numDebugFormats = 0;
		// How many debug format are in this exe
	int coffDebugInfoOffset = 0;
		// where the coff debug info starts
	IMAGE_DEBUG_DIRECTORY *debugDir = NULL;
		// primary debug headers

	// These two macros simplify traversing the structures
	#define MAKEPTR(type, var, offset) type *var = (type *)(stackTrace_basePtr + offset)
		// This macro help us make pointer relative to the base
	#define SETPTR(type, var, offset) var = (type *)(stackTrace_basePtr + offset)
		// This macro help us make pointer relative to the base

	// Get pointers to the primary headers...
	MAKEPTR( IMAGE_DOS_HEADER, dosHeader, 0 );
	SETPTR( IMAGE_NT_HEADERS, stackTraceNtHeader, dosHeader->e_lfanew );

	// Go through each section looking for the .rdata section
	int i;
	for( i=0; i<stackTraceNtHeader->FileHeader.NumberOfSections; i++ ) {
		IMAGE_SECTION_HEADER *secHead = &(IMAGE_FIRST_SECTION( stackTraceNtHeader ))[i];
		if( ! stricmp( (char *)&secHead->Name[0], ".rdata" ) ) {
			numDebugFormats = 
				stackTraceNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size
				/ sizeof(IMAGE_DEBUG_DIRECTORY)
			;
			if( numDebugFormats != 0 ) {
				SETPTR( IMAGE_DEBUG_DIRECTORY, debugDir, 
					secHead->PointerToRawData +
					(
						stackTraceNtHeader->OptionalHeader.DataDirectory
						[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress
						- secHead->VirtualAddress
					)
				);
				break;
			}
		}
	}

	// Now have pointer to primary debug headers (debugDir) and
	// a count of number of debug formats (numDebugFormats)
	// Parse these to find the COFF headers
	stackTraceCoffHeader = 0;
	for( i=0; i<numDebugFormats; i++, debugDir++ ) {
		if(
			debugDir->Type == IMAGE_DEBUG_TYPE_COFF &&
		    stackTraceNtHeader->FileHeader.PointerToSymbolTable
		) {
			SETPTR( IMAGE_COFF_SYMBOLS_HEADER, stackTraceCoffHeader, debugDir->PointerToRawData );
			break;
		}
	}

	// Now have pointer to coff info. (stackTraceCoffHeader)
	// Use this to find line numbers,

	// Now extract the COFF symbol table
	MAKEPTR( IMAGE_SYMBOL, coffSym, stackTraceNtHeader->FileHeader.PointerToSymbolTable );
	stackTraceCoffSymbolCount = stackTraceNtHeader->FileHeader.NumberOfSymbols;

	// The string table apparently starts right after the symbol table
	stackTraceStringTable = (PSTR)&coffSym[stackTraceCoffSymbolCount]; 

	stackTraceSymbolTable = (IMAGE_SYMBOL **)malloc( stackTraceCoffSymbolCount * sizeof(IMAGE_SYMBOL*) );
	stackTraceSymbolTableSize = 0;
	for( i=0; i<stackTraceCoffSymbolCount-1; i++ ) {
		if( coffSym->Type == 0x20 ) {
			stackTraceSymbolTable[stackTraceSymbolTableSize++] = coffSym;
			// Take into account any aux symbols
		}
		i += coffSym->NumberOfAuxSymbols;
		coffSym += coffSym->NumberOfAuxSymbols;
		coffSym++;
	}

	// SORT the pointers by address
	qsort( stackTraceSymbolTable, stackTraceSymbolTableSize, sizeof(IMAGE_SYMBOL*), symbolPtrCompare );

	zStackTraceCloseFiles();

	stackTracePrebuiltSymbolTable++;
}

char *zStackTraceGetString( int _size, unsigned int *_ips, unsigned int *_args ) {
	if( !stackTracePrebuiltSymbolTable ) {
		return 0;
		// You must pre build the symbol table in preparation
		// for a fatal or mem dump so that no mallocs have
		// to happen when this function is running.
	}

	static char output[2048];
	output[0] = 0;
	#define OUTS sprintf( output+strlen(output), 

	if( !stackTraceCoffHeader ) {
		// There was not COFF debug information available.
		OUTS "Stack symbol information only available "
		     "when linked with COFF debug symbols.\n"
		     "Dumping hex trace instead:\n"
		);

		for( int i=0; i<_size; i++ ) {
			OUTS "%08X(", stackTraceBaseAddress+_ips[i] );
			try {
				if( _args ) {
					int *a = (int *)_args[i+1];
					for( int j=0; j<5; j++, a++ ) {
						OUTS "%s 0x%X", j==0?"":",", *a );
					}
				}
			}
			catch( ... ) { }
			OUTS ")\r\n" );
		}

		zStackTraceCloseFiles();
		return output;
	}

	for( int i=0; i<_size; i++ ) {
		IMAGE_SYMBOL *sym = binarySearchSymbol( stackTraceSymbolTable, stackTraceSymbolTableSize, _ips[i] );
		if( !sym ) {
			OUTS "%X = Undefined Symbol\n", _ips[i] );
		}
		else {
			char *symName = 
				(char *)sym->N.Name.Short != 0 ? 
				(char *)sym->N.ShortName : 
				(char *)(stackTraceStringTable + sym->N.Name.Long)
			;
			char args[30]={0,};
			char funcName[128]={0,};
			char *open = NULL;
			int numArgs = 0;

			//if( skipAssert && strstr(symName,"assert") ) {
			//	continue;
			//}

			// The function names are mangled, which tells us
			// what the  arguments are.  Unfortunatly, the name
			// demangler is not perfect (since I had to reverse
			// engineer it).  So in cases when it fails, we
			// just print five arguments hoping that this is
			// sufficient.
			char *success = zStackTraceDemangleMSDEVFuncName( symName, funcName, 128, args, 30, numArgs );
			if( strstr( symName, "mainCRTStartup" ) ) {
				break;
			}
			OUTS "%s(", funcName );
			if( success ) {
				// Name demangler succeeded.  Print
				// out a trace of the arguments.  For
				// pointer args, print out a small
				// memory dump of that pointer
				if( _args ) {
					unsigned int *a = (unsigned int *)_args[i+1];
					for( int j=0; j<numArgs; j++, a++ ) {
						try {
							OUTS "%s ", j==0?"":"," );
							switch( args[j] ) {
								case 'D': case 'E': case 'F':
								case 'G': case 'H': case 'I':
									if( *a >= 0x10000 ) {
										OUTS "0x%X", *a );
									}
									else {
										OUTS "%d", *a );
									}
									break;
								case 'M':
									OUTS "%f", *a );
									break;
								case 'N':
									// TODO
									break;
								case 'P':
									OUTS "PTR(0x%X)", *a );
									break;
								default:
									OUTS "?" );
							}
						}
						catch( ... ) { }
					}
				}
			}
			else {
				// The name demanagler failed, so just
				// print 5 arguments under the theory that
				// it is better than nothing.
				try {
					OUTS "?? " );
					if( _args ) {
						int *a = (int *)_args[i+1];
						for( int j=0; j<5; j++, a++ ) {
							if( a ) {
								OUTS "%s 0x%X", j==0?"":",", *a );
							}
						}
					}
				} catch(...) { }
			}
			OUTS " )\n" );

			if( success ) {
				// Name mangler succeeded, go back and
				// print out a hex dump for each pointer.
				try {
					if( _args ) {
						int *a = (int *)_args[i+1];
						for( int j=0; j<numArgs; j++, a++ ) {
							if( args[j] == 'P' ) {
								unsigned char *ptr = (unsigned char *)*a;
								if( ptr >= (unsigned char *)stackTraceBaseAddress ) {
									for( int k=0; k<3; k++, ptr+=16 ) {
										OUTS "  %X: ", ptr );
										int l;
										for( l=0; l<16; l++ ) OUTS "%02X ", ptr[l] );
										OUTS "  " );
										for( l=0; l<16; l++ ) OUTS "%c", ptr[l]>=31&&ptr[l]<127?ptr[l]:'.' );
										OUTS "\n" );
									}
								}
							}
						}
					}
				}
				catch( ... ) { }
			}
		}
	}

	return output;
}

#ifdef WIN32
static char *zStackTraceDemangleMSDEVFuncName(
	char *mangledName, 
	char funcName[], int funcNameMax,
	char args[], int maxArgs, 
	int &numArgs
) {
	// Print out arguments.  This is a big pain in the ass
	// because the argument list needs to be parsed.
	// This fills in the funcName and args buffers.
	// Both of which need to pass in buffer sizes.
	// numArgs is filled out.
	// Returns a full string ready for printing or NULL on error
	// The args array is encoded as follows:
	// D=char, E=uchar, F=short, G=ushort, H=int, I=uint, 
	// J=, K=, L=, M=float, N=double, O=, P=ptr, Q=, R=, 
	// S=, T=, U=STRUCT, V=, W=, X=void, Y=, Z=...
	static char demangled[128] = {0,};
	memset( demangled, 0, 128 );
	numArgs = 0;
	char *atomicTypes[] = { "REF", "", "", "char", "uchar", "short",
		"ushort", "int", "uint", "", "", "",
		"float", "double", "", "PTR", "", "", "", "",
		"STRUCT", "", "", "void", "", "..."
	};
	int retType = 0, funcType;
	enum {sSTART,sNAME,sCLASSNAME,sFUNCYPE,sERROR,sRETTYPE,sSTRUCT,sARGS,sTYPE,sRETTYPEDONE,sARGSDONE,sEND};
	enum {ftFREEFUNC, ftMETHOD, ftVIRTUAL, ftSTATIC,};
	char name[128]={0,};
	char classname[128]={0,};
	int state = sSTART;
	int stateStack[30];
	int stateStackTop = 0;
	int lastType = 0;
	#define PUSHSTATE stateStack[stateStackTop++] = state;
	#define POPSTATE state = stateStack[--stateStackTop];

	char *p = mangledName;
	while( *p || state == sEND ) {
		switch( state ) {
			case sSTART:
				if( *p == '_' ) {
					strncpy( funcName, p, funcNameMax-1 );
					funcName[funcNameMax-1] = 0;
					return NULL;
				}
				if( *p == '?' ) state = sNAME;
				else state = sERROR;
				break;
			case sNAME:
				if( *p=='@' && *(p+1)=='@' ) {
					p++;
					state = sFUNCYPE;
				}
				else if( *p=='@' ) state = sCLASSNAME;
				else name[strlen(name)]=*p;
				break;
			case sCLASSNAME:
				if( *p == '@' && *(p+1)=='@' ) {
					p++;
					state = sFUNCYPE;
				}
				else classname[strlen(classname)]=*p;
				break;
			case sFUNCYPE:
				if( *p=='Y' ) funcType = ftFREEFUNC;
				if( *p=='Q' ) funcType = ftMETHOD;
				if( *p=='U' ) funcType = ftVIRTUAL;
				if( *p=='S' ) funcType = ftSTATIC;
				// Skip const, volatile, etc.
				p++;
				if( funcType == ftMETHOD || funcType == ftVIRTUAL ) {
					p++;// Some unknown parameter, mostly 'E'
						// Maybe it stands for "this"
					//args[numArgs++] = 'P';
					//if( numArgs >= maxArgs ) state = sERROR;
				}
				state = sRETTYPE;
				break;
			case sTYPE:
				if( *p == '?' ) {
					p++;
					if( *p != 'B' && *p != 'A' ) state = sERROR; // 'B' means const, 'A' means copy?
					break;
				}
				else if( *p == 'P' || *p == 'Q' || *p == 'A' ) {
					if(lastType==0) lastType='P';
					p++;	// Read the modifier
					if( !(*p>='A' && *p<='C') ) state=sERROR;
					else {
						PUSHSTATE;
					}
				}
				else if( *p>='D' && *p<='O' || *p == 'X' ) {
					// Simple atomic type
					if(lastType==0) lastType=*p;
				}
				else if( *p == 'U' ) {
					if(lastType==0) lastType=*p;
					// A structure, gobble it up
					while( *p++ != '@' );
				}
				else if( *p == 'Y' ) {
					// An array length, gobble it up
					do {
						p++;
					} while( *p >= '0' && *p <= '9' );
					p--;
					break;
				}
				if( stateStackTop > 0 ) POPSTATE;
				break;
			case sRETTYPE:
				lastType = 0;
				state = sRETTYPEDONE;
				PUSHSTATE;
				state = sTYPE;
				p--;
				break;
			case sRETTYPEDONE:
				retType = lastType;
				state = sARGS;
				p--;
				break;
			case sARGS:
				if( *p=='@' ) state = sEND;
				else if( *p=='Z' && *(p+1)!='Z' ) {
					//argCount = 0;
					state = sEND;
				}
				else if( *p=='Z' && *(p+1)=='Z' ) {
					args[numArgs++] = 'Z';
					if( numArgs >= maxArgs ) state = sERROR;
					else state = sEND;
				}
				else {
					lastType = 0;
					state = sARGSDONE;
					PUSHSTATE;
					state = sTYPE;
					p--;
				}
				break;
			case sARGSDONE:
				args[numArgs++] = lastType;
				if( numArgs >= maxArgs ) state = sERROR;
				else state = sARGS;
				p--;
				break;
			case sEND: {
				strcat( demangled, classname );
				if( *classname ) strcat( demangled, "::" );
				strcat( demangled, name );
				strncpy( funcName, demangled, funcNameMax-1 );
				funcName[funcNameMax-1] = 0;
				*demangled = 0;
				if( retType >= 'A' ) {
					strcat( demangled, atomicTypes[retType-'A'] );
					strcat( demangled, " " );
				}
				strcat( demangled, classname );
				if( *classname ) strcat( demangled, "::" );
				strcat( demangled, name );
				strcat( demangled, "(" );
				for( int i=0; i<numArgs; i++ ) {
					if( args[i] >= 'A' && args[i] <= 'Z' ) {
						strcat( demangled, i==0?" ":", " );
						strcat( demangled, atomicTypes[args[i]-'A'] );
					}
				}
				strcat( demangled, " )" );
				goto stop;
			}

			case sERROR:
				return NULL;
				goto stop;
		}
		p++;
	}
	stop:;
	return demangled;
}
#endif



#ifdef SELF_TEST

// This is test code for the name mangler parser

void main( int argc, char **argv ) {
	char *cases[] = {
		"void oink( Oink * )",          "?oink@@YAXPAUOink@@@Z",      "void oink( PTR )",
		"void oink( const int * )",     "?oink@@YAXPBH@Z",            "void oink( PTR )",
		"void oink( int * const )",     "?oink@@YAXQAH@Z",             "void oink( PTR )",
		"void oink( const int & )",     "?oink@@YAXABH@Z",            "void oink( PTR )",
		"void oink( volatile int * )",  "?oink@@YAXPCH@Z",            "void oink( PTR )",
		"void oink( int c[4][1] )",     "?oink@@YAXQAY00H@Z",         "void oink( PTR )",
		"void oink( int c[4] )",        "?oink@@YAXQAH@Z",            "void oink( PTR )",
		"void oink( )",                 "?oink@@YAXXZ",               "void oink( void )",
		"void Boink::oink()",           "?oink@Boink@@QAEXXZ",        "void Boink::oink( void )",
		"void Boink::oink() const",     "?oink@Boink@@QBEXXZ",        "void Boink::oink( void )",
		"void Boink::oink() volatile",  "?oink@Boink@@QCEXXZ",        "void Boink::oink( void )",
		"virtual void Boink::oink()",   "?oink@Boink@@UAEXXZ",        "void Boink::oink( void )",
		"static void Boink::oink()",    "?oink@Boink@@SAXXZ",         "void Boink::oink( void )",
		"void oink( int c[4][2] )",     "?oink@@YAXQAY01H@Z",         "void oink( PTR )",
		"void oink( int c[5][2] )",     "?oink@@YAXQAY01H@Z",         "void oink( PTR )",
		"void oink( int c[5][2][1] )",  "?oink@@YAXQAY110H@Z",        "void oink( PTR )",
		"void oink( char, char )",      "?oink@@YAXDD@Z",             "void oink( char, char )",
		"void oink( Oink )",            "?oink@@YAXUOink@@@Z",        "void oink( STRUCT )",
		"Boink oink( )",                "?oink@@YA?AUBoink@@XZ",      "STRUCT oink( void )",
		"Boink *oink( )",               "?oink@@YAPAUBoink@@XZ",      "PTR oink( void )",
		"void oink( int [] )",          "?oink@@YAXQAH@Z",            "void oink( PTR )",
		"void oink( int, ... )",        "?oink@@YAXHZZ",              "void oink( int, ... )",
		"const int oink( )",            "?oink@@YA?BHXZ",             "int oink( void )",
		"void *oink( )",                "?oink@@YAPAXXZ",             "PTR oink( void )",
		"int &oink( )",                 "?oink@@YAAAHXZ",             "PTR oink( void )",
		"void oink( int * )",           "?oink@@YAXPAH@Z",            "void oink( PTR )",
		"void oink( int ** )",          "?oink@@YAXPAPAH@Z",          "void oink( PTR )",
		"void oink( int & )",           "?oink@@YAXAAH@Z",            "void oink( PTR )",
		"int oink( char *, char ** )",  "?oink@@YAHPADPAPAD@Z",       "int oink( PTR, PTR )",
		"char oink( )",                 "?oink@@YADXZ",               "char oink( void )",
		"unsigned char oink( )",        "?oink@@YAEXZ",               "uchar oink( void )",
		"short oink( )",                "?oink@@YAFXZ",               "short oink( void )",
		"unsigned short oink( )",       "?oink@@YAGXZ",               "ushort oink( void )",
		"int oink( )",                  "?oink@@YAHXZ",               "int oink( void )",
		"unsigned int oink( )",         "?oink@@YAIXZ",               "uint oink( void )",
		"float oink( )",                "?oink@@YAMXZ",               "float oink( void )",
		"double oink( )",               "?oink@@YANXZ",               "double oink( void )",
		"void oink( char )",            "?oink@@YAXD@Z",              "void oink( char )",
		"void oink( int )",             "?oink@@YAXH@Z",              "void oink( int )",
		// TODO: function ptrs, operators
		//"void oink( __int64 )",      "?oink@@YAX_J@Z",
		//"typedef void (*FPTR)();"
		//"void oink( FPTR )",        "?oink@@YAXP6AXXZ@Z",
		//"typedef void (*FPTR)(int);"
		//"void oink( FPTR )",        "?oink@@YAXP6AXH@Z@Z",
		//"int Boink::operator +(int a)",  "??HBoink@@QAEHH@Z",
		//"int Boink::operator -(int a)",  "??GBoink@@QAEHH@Z",
		//"Boink::operator char *()",       "??BBoink@@QAEPADXZ",
	};

	int count = sizeof(cases)/sizeof(cases[0]);
	for( int i=0; i<count; i+=3 ) {
		char args[100];
		int numArgs;
		char *str = zStackTraceDemangleMSDEVFuncName( cases[i+1], "", 1, args, 100, numArgs );
		printf( "%s = %s\nshould be:%s\n      got:%s\n\n", cases[i], cases[i+1], cases[i+2], str );
		if( strcmp( str, cases[i+2] ) ) {
			printf( "Regression failed i=%d\n", i );
		}
	}
}
#endif
