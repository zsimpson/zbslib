// @ZBS {
//		*MODULE_NAME Wildcard Enumerator
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A class for traversing filename wildcards.
//		}
//		+EXAMPLE {
//			ZWildcard w( "*.bmp" );
//			while( w.next() ) \{
//				printf( "%s\n", w.getName() );
//			\}
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zwildcard.cpp zwildcard.h
//		*VERSION 2.0
//		+HISTORY {
//			2.0 Created a Unix compatibility layer by emulating windows wildcards
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH yes
// }


#ifdef WIN32
//	#include "windows.h"
	// The following is the extracted code needed from windows.h.  If there is
	// a compile problem for some reason just comment out the below and uncomment
	// the above include

	extern "C" {
	#define INVALID_HANDLE_VALUE (void *)-1
	struct FILETIME {
		unsigned long dwLowDateTime;
		unsigned long dwHighDateTime;
	};
	#pragma pack(push,8)
	struct WIN32_FIND_DATA {
		unsigned long dwFileAttributes;
		FILETIME ftCreationTime;
		FILETIME ftLastAccessTime;
		FILETIME ftLastWriteTime;
		unsigned long nFileSizeHigh;
		unsigned long nFileSizeLow;
		unsigned long dwReserved0;
		unsigned long dwReserved1;
		char cFileName[ 260 ];
		char cAlternateFileName[ 14 ];
	};
	#pragma pack(pop)

	#define FILE_ATTRIBUTE_HIDDEN 2
	__declspec(dllimport) unsigned long __stdcall GetFileAttributesA( const char * );
	#define GetFileAttributes GetFileAttributesA
	__declspec(dllimport) int __stdcall FindClose( void *hFindFile );
	__declspec(dllimport) void *__stdcall FindFirstFileA( const char *lpFileName, WIN32_FIND_DATA *lpFindFileData );
	#define FindFirstFile FindFirstFileA
	__declspec(dllimport) int __stdcall FindNextFileA( void *hFindFile, WIN32_FIND_DATA *lpFindFileData );
	#define FindNextFile FindNextFileA
	}

	// End windows.h duplication
#else
#include "sys/types.h"
#include "dirent.h"
#include "fnmatch.h"
#endif

// OPERATING SYSTEM specific includes:
// STDLIB includes:
#include "sys/stat.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "ctype.h"
#include "assert.h"
// MODULE includes:
#include "zwildcard.h"
// ZBSLIB includes:

ZWildcard::ZWildcard( char *_pattern ) {
	flags = 0;
	if( _pattern[0] == '\'' ) {
		_pattern++;
	}
	pattern = strdup( _pattern );

	if( pattern[strlen(pattern)-1] == '\'' ) {
		pattern[strlen(pattern)-1] = 0;
	}

	shortPattern = 0;
	findFileData = 0;
	hFind = 0;

	#ifdef WIN32
		findFileData = malloc( sizeof(WIN32_FIND_DATA) );
		memset( findFileData, 0, sizeof(WIN32_FIND_DATA) );
		hFind = INVALID_HANDLE_VALUE;
	#endif
}

ZWildcard::~ZWildcard() {
	if( shortPattern ) {
		free( shortPattern );
		shortPattern = 0;
	}
	if( findFileData ) {
		#ifdef WIN32
			free( findFileData );
		#endif
		findFileData = 0;
	}
	if( pattern ) {
		free( pattern );
		pattern = 0;
	}

	#ifdef WIN32
	    FindClose( hFind );
	#else
		if( hFind ) {
			closedir( (DIR *)hFind );
		}
	#endif
}

int ZWildcard::next() {
	int ret = 0;

	#ifdef WIN32
		if( hFind == INVALID_HANDLE_VALUE ) {
			hFind = FindFirstFile( pattern, (WIN32_FIND_DATA *)findFileData );
			ret = (int)( hFind != INVALID_HANDLE_VALUE );
		}
		else {
			ret = FindNextFile( hFind, (WIN32_FIND_DATA *)findFileData );
		}
	#else
		if( flags == 2 ) {
			return 0;
		}

		if( hFind == 0 ) {
			char *star = strchr( pattern, '*' );
			char *ques = strchr( pattern, '?' );
			if( star || ques ) {
				// There is a wildcard so break the path into the non-filename part
				// and do an opendir on it...
				char path[256];
				strcpy( path, pattern );
				for( char *tail=&path[strlen(path)-1]; tail>=path; tail-- ) {
					if( *tail == '/' || *tail == '\\' ) {
						shortPattern = strdup( tail+1 );
						tail[1] = 0;
						break;
					}
				}

				hFind = (void *)opendir( path );
				if( hFind ) {
					flags = 1;
				}
				else {
					flags = 0;
					return 0;
				}
			}
			else {
				flags = 0;
			}
		}
		if( flags == 1 ) {
			// The opendir was successful
			// FIND the next file
			int match = 0;
			do {
				findFileData = readdir( (DIR *)hFind );
				if( ! findFileData ) {
					return 0;
				}
				match = ! fnmatch( shortPattern, ((struct dirent *)findFileData)->d_name, 0 );
			} while( !match );
			return match ? 1 : 0;
		}
		else if( flags == 0 ) {
			// This means that it had no wildcard char and should be returned as is if it exists
			struct stat s;
			if( stat(pattern,&s)==0 ) {
				flags = 2;
				return 1;
			}
			return 0;
		}
	#endif

	return ret;
}

char *ZWildcard::getName() {
	#ifdef WIN32
		return ((WIN32_FIND_DATA *)findFileData)->cFileName;
	#else
		if( flags == 2 ) {
			for( char *s = &pattern[strlen(pattern)-1]; s>=pattern; s-- ) {
				if( *s == '/' || *s == '\\' ) {
					return s+1;
				}
			}
			return pattern;
		}
		else {
			assert( findFileData );
			return ((struct dirent *)findFileData)->d_name;
		}
	#endif
}

char *ZWildcard::getFullName() {
	static char path[256];

	// Find the last slash in the pattern.  Everything after that is replaced
	strcpy( path, pattern );
	for( char *s = &path[strlen(path)-1]; s>=path; s-- ) {
		if( *s == '/' || *s == '\\' ) {
			*(s+1) = 0;
			break;
		}
	}

	#ifdef WIN32
		strcat( path, ((WIN32_FIND_DATA *)findFileData)->cFileName );
		return path;
	#else
		if( flags == 2 ) {
			strcpy( path, pattern );
		}
		else {
			strcat( path, ((struct dirent *)findFileData)->d_name );
		}
		return path;
	#endif
}

int zWildcardFileExists( char *name ) {
	struct stat s;
	if( stat(name,&s)==0 ) {
		return 1;
	}
	return 0;
}

int zWildcardOutOfDate( char *a, char *b ) {
	struct stat astat;
	int reta = stat( a, &astat );

	struct stat bstat;
	int retb = stat( b, &bstat );

	if( !reta && !retb ) {
		return astat.st_mtime > bstat.st_mtime ? 1 : 0;
	}
	else if( !reta && retb ) {
		return 1;
	}
	else if( reta && retb ) {
		return 1;
	}
	return 0;
}

void zWildcardTabExpand( char *partialString, char result[256] ) {
	char pattern[256];
	strcpy( pattern, partialString );
	strcat( pattern, "*" );
	ZWildcard w( pattern );
	result[0] = 0;
	while( w.next() ) {
		char *n = w.getFullName();
		if( !result[0] ) {
			strcpy( result, n );
		}
		else {
			// SHORTEN common to the shortest common substring
			int i;
			for( i=0; tolower(result[i])==tolower(n[i]); i++ ) ; 
			result[i] = 0;
		}
	}
}

int zWildcardFileIsFolder( char *name ) {
	struct stat buf;
	int s = stat( name, &buf );
	#ifdef WIN32
		return buf.st_mode & _S_IFDIR;
	#else
		return buf.st_mode & S_IFDIR;
	#endif
}

int zWildcardFileIsHidden( char *name ) {
	#ifdef WIN32
		return GetFileAttributes( name ) & FILE_ATTRIBUTE_HIDDEN;
	#else
		for( char *c=name+strlen(name); c>=name; c-- ) {
			if( c[0] == '/' ) {
				if( c[1] == '.' ) {
					return 1;
				}
				return 0;
			}
		}
		return 0;
	#endif
}

int zWildcardNextSerialNumber( char *pattern ) {
	int largest = -1;
	ZWildcard w( pattern );
	char *firstWildcardPtr = strpbrk( pattern, "*?" );
	if( firstWildcardPtr ) {
		int firstWildcardIndex = firstWildcardPtr - pattern;
		while( w.next() ) {
			char *ptr = &(w.getFullName())[firstWildcardIndex];
			char *end = ptr;
			for( ; *end >= '0' && *end <= '9'; end++ );
			*end = 0;
			int i = atoi(ptr);
			largest = largest > i ? largest : i;
		}
	}
	return largest > 0 ? largest+1 : 0;
}

