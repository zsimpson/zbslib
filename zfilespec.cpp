// @ZBS {
//		*MODULE_NAME File Specificaiton Parser
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A portable filename & path parser
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zfilespec.cpp zfilespec.h
//		*VERSION 2.0
//		+HISTORY {
//			2.0 A compete revision using a real parser instead of the hacked up mess from before
//		}
//		+TODO {
//		}
//		*SELF_TEST SELFTEST_ZFILESPEC
//		*PUBLISH yes
// }
// OPERATING SYSTEM specific includes:
#ifdef WIN32
	#include "wintiny.h"
	#include "direct.h"
	#include "ctype.h"
#else
	#include "unistd.h"
#endif
// SDK includes:
// STDLIB includes:
#include "string.h"
#include "stdarg.h"
#include "sys/stat.h"
#include "assert.h"
// MODULE includes:
#include "zfilespec.h"
// ZBSLIB includes:




#ifdef WIN32
	static char dirChar = '\\';
	static char *dirString = "\\";
	static char *dirNormal = ".\\";
#else
	static char dirChar = '/';
	static char *dirString = "/";
	static char *dirNormal = "./";
#endif

static char *flipSlashes( char *_s ) {
	char *s = _s;
	while( *s ) {
		if( *s == '\\' ) {
			*s = '/';
		}
		else if( *s == '/' ) {
			*s = '\\';
		}
		s++;
	}
	return _s;
}

char * convertSlashes( char *s, char _dirChar=0 ) {
	char *p = s;
	if( _dirChar == 0 ) {
		_dirChar = dirChar;
	}
	while( *s ) {
		if( *s == '\\' || *s == '/' ) {
			*s = _dirChar;
		}
		s++;
	}
	return p;
}

static char *stripLeadingSlashes( char *s ) {
	while( *s && *s == dirChar ) {
		s++;
	}
	return s;
}

char *stripTrailingSlashes( char *s ) {
	int l = strlen( s );
	char *t = &s[l-1];
	while( t > s && *t == dirChar ) {
		*t-- = 0;
	}
	return s;
}

static char *stripLeadingDots( char *s ) {
	while( *s && *s == '.' ) {
		s++;
	}
	return s;
}

static char *stripTrailingDots( char *s ) {
	int l = strlen( s );
	char *t = &s[l-1];
	while( t > s && *t == '.' ) {
		*t-- = 0;
	}
	return s;
}

static char *stripLastExt( char *s ) {
	int l = strlen( s );
	char *t = &s[l-1];
	while( t > s ) {
		if( *t == '.' ) {
			*t = 0;
			return s;
		}
		if( *t == dirChar || *t == ':' ) {
			return s;
		}
		t--;
	}
	return s;
}

static char *stripAllExt( char *s ) {
	int l = strlen( s );
	char *t = &s[l-1];
	while( t > s ) {
		if( *t == '.' ) {
			*t = 0;
		}
		if( *t == dirChar || *t == ':' ) {
			return s;
		}
		t--;
	}
	return s;
}

void resolveDoubleDots(char *path) {
	char *s = path;
	if( *s && *(s+1) ) {
		while( *(s+2) ) {
			if( *s == '.' && *(s+1) == '.' && *(s+2) == '/' ) {
				char *b = s;
				while( b > path+1 && *b != '/' ) --b;
				--b;
				while( b > path && *b != '/' ) --b;
				strcpy( b, s+2 );
			}
			++s;
		}
	}
}


char *zFileSpecMake( int firstToken, ... ) {
	va_list argptr;
	va_start( argptr, firstToken );
	char __s[256];
	char *s;
	char *d = ZFileSpec::getNextWorkBuffer();

	int firstDir = 1;

	int lastToken = 0;
	int token = firstToken;
	while( token ) {
		char *_s = va_arg( argptr, char * );
		strcpy( __s, _s );
		s = __s;
			// I added this extra copy due to recent default
			// changes in GNU which apprently make it illegal
			// to write into a static, or perhapse it is
			// merging strings and then making it illegal.
			// Anyways, this helps

		convertSlashes(s);
		int l = strlen(s);
		switch( token ) {
			case FS_DRIVE:
				if( lastToken != 0 ) {
					return NULL;
				}
				s = stripTrailingSlashes(s);
				l = strlen(s );
				strcat( d, s );
				if( l>0 && s[0] != dirChar ) {
					// If its not a UNC dir then ensure we have a colon
					if( s[l-1] != ':' ) {
						strcat( d, ":" );
					}
				}
				break;

			case FS_DIR:
				if( lastToken == FS_FILE || lastToken == FS_EXT ) {
					return NULL;
				}
				if( !firstDir ) {
					s = stripLeadingSlashes(s );
				}
				firstDir=0;
				s = stripTrailingSlashes(s );
				l = strlen(s );
				if( lastToken == 0 ) {
					// if first token, add a directory normalization if
					// there is no other slash in the string
					// Added a check for ':' as well; this was failing if this dir was
					// already fully qualified. -TNB
					if( !strchr(s, dirChar) && !strchr( s, ':') ) {
						strcat( d, dirNormal );
					}
				}
				else if( strlen(d)>0 && d[strlen(d)-1]!=dirChar && *s!=dirChar ) {
					strcat( d, dirString );
				}
				strcat( d, s );
				if( d[strlen(d)-1]!=dirChar ) {
					strcat( d, dirString );
				}
				break;

			case FS_FILE:
				if( lastToken == FS_FILE || lastToken == FS_EXT ) {
					return NULL;
				}
				s = stripLeadingSlashes(s );
				s = stripTrailingSlashes(s );
				l = strlen(s );
				if( lastToken == 0 ) {
					// if first token, add a directory normalization if
					// there is no other slash in the string
					if( !strchr(s, dirChar) ) {
						strcat( d, dirNormal );
					}
				}
				else if( d[strlen(d)-1]!=dirChar ) {
					strcat( d, dirString );
				}
				strcat( d, s );
				break;

			case FS_EXT:
				if( lastToken == FS_DRIVE || lastToken == FS_DIR || lastToken == 0 ) {
					return NULL;
				}
				s = stripLeadingDots(s );
				s = stripTrailingDots(s );
				l = strlen(s );
				if( d[strlen(d)-1]!='.' ) {
					strcat( d, "." );
				}
				strcat( d, s );
				break;

			default:
				return NULL;
				
		}

		lastToken = token;
		token = va_arg( argptr, int );
	}

	va_end( argptr );
	return d;
}

int ZFileSpec::workBuffer = 0;
char ZFileSpec::workBuffers[ZFILESPEC_WORK_BUFFER_COUNT][ZFILESPEC_PATH];

char *ZFileSpec::getNextWorkBuffer() {
	workBuffer = (++workBuffer)%ZFILESPEC_WORK_BUFFER_COUNT;
	char *p = workBuffers[workBuffer];
	memset( p, 0, ZFILESPEC_PATH );
	return p;
}

ZFileSpec::ZFileSpec() {
	memset( path, 0, sizeof(path) );
	memset( drive, 0, sizeof(drive) );
	memset( dir, 0, sizeof(dir) );
	memset( dirs, 0, sizeof(dirs) );
	memset( file, 0, sizeof(file) );
	memset( ext, 0, sizeof(ext) );
}

ZFileSpec::ZFileSpec( char *_path, int standardizeSlashses ) {
	set( _path, standardizeSlashses );
}

void ZFileSpec::set( char *_path, int standardizeSlashses ) {
	if( _path == path )	// Without this it clears itself if you pass it to itself.
		return;
	memset( path, 0, sizeof(path) );
	memset( drive, 0, sizeof(drive) );
	memset( dir, 0, sizeof(dir) );
	memset( dirs, 0, sizeof(dirs) );
	memset( file, 0, sizeof(file) );
	memset( ext, 0, sizeof(ext) );
	parse( _path, standardizeSlashses );
}

int ZFileSpec::isRelative() {
	return getDrive()[0] == 0 && getDir(0)[0] != '/' && getDir(0)[0] != '\\';
}

int ZFileSpec::isAbsolute() {
	return getDrive()[0] != 0 || getDir(0)[0] == '/' || getDir(0)[0] == '\\';
}

void ZFileSpec::makeRelative() {
	#ifdef WIN32
		char currentDir[256];
		GetCurrentDirectory( 256, currentDir );
	#else
	#endif
}

void ZFileSpec::makeAbsolute(int _resolveDoubleDots) {
	if( isRelative() ) {
		char currentDir[256];

		#ifdef WIN32
			GetCurrentDirectory( 256, currentDir );
		#else
			getcwd( currentDir, 256 );
		#endif

		char _path[256];
		strcpy( _path, currentDir );
		strcat( _path, "/" );
		strcat( _path, path );
		strcpy( path, _path );
		parse( path, 1 );

		if( _resolveDoubleDots ) {
			resolveDoubleDots(&path[0]);
		}
	}
}

void ZFileSpec::getCwd() {
	char currentDir[256];
	#ifdef WIN32
		GetCurrentDirectory( 256, currentDir );
	#else
		getcwd( currentDir, 256 );
	#endif
	set( currentDir );
}


void ZFileSpec::parse( char *__path, int standardizeSlashses ) {
	char _path[256];
	strncpy( _path, __path, 255 );
	if( standardizeSlashses ) {
		convertSlashes( _path, '/' );
	}
	strncpy( path, _path, 255 );

	enum {
		START=0,
		COMPUTER_NAME_START,
		COMPUTER_NAME,
		FIRST_DIR,
		SCANNING_NAME,
		END,
	};

	memset( drive, 0, sizeof(drive) );
	memset( dir, 0, sizeof(dir) );
	memset( dirs, 0, sizeof(dirs) );
	memset( file, 0, sizeof(file) );
	memset( ext, 0, sizeof(ext) );

	int state = START;
	int dirCount = 0;
	char *lastStart = path;
	char *c = path;
	while( state != END ) {
		switch( state ) {
			case START: {
				if( c[0] == '/' || c[0] == '\\' ) {
					// Possible root ref or beginning of a computer name
					if( c[1] == c[0] ) {
						// This is beginning of a computer name
						state = COMPUTER_NAME_START;
					}
					else {
						c--;
						state = FIRST_DIR;
					}
				}
				else if( c[0] == 0 ) {
					// Empty
					state = END;
				}
				else {
					state = SCANNING_NAME;
					lastStart = c-1;
				}
				break;
			}

			case COMPUTER_NAME_START: {
				lastStart = c+1;
				state = COMPUTER_NAME;
				break;
			}

			case COMPUTER_NAME: {
				if( c[0] == '/' || c[1] == '\\' ) {
					memcpy( drive, lastStart, c-lastStart );
					state = FIRST_DIR;
				}
				else if( c[0] == 0 ) {
					memcpy( drive, lastStart, c-lastStart );
					state = END;
				}
				break;
			}

			case FIRST_DIR: {
				dirs[0][0] = c[0];
				dirCount++;
				lastStart = c;
				state = SCANNING_NAME;
				break;
			}

			case SCANNING_NAME: {
				if( c[0] == '/' || c[0] == '\\' ) {
					// The last thing we read was a directory, copy it

					memcpy( dirs[dirCount], lastStart+1, c-lastStart );
					dirCount++;
					lastStart = c;
				}
				else if( c[0] == ':' ) {
					memcpy( drive, lastStart+1, c-lastStart );
					lastStart = c;
					state = SCANNING_NAME;
				}
				else if( c[0] == 0 ) {
					// The last thing we read was the file
					memcpy( file, lastStart+1, c-lastStart );
					state = END;
				}
			}
		}
		
		c++;
	}

	// SEARCH backward in the file for an extension and remove it
	for( char *s = file + strlen( file ); s >= file; s-- ) {
		if( s[0] == '.' ) {
			strcpy( ext, s+1 );
			s[0] = 0;
			break;
		}
	}
}

int ZFileSpec::getNumDirs() {
	for( int i=0; i<sizeof(dirs)/sizeof(dirs[0]); i++ ) {
		if( dirs[i][0] == 0 ) {
			return i;
		}
	}
	return 0;
}

char *ZFileSpec::getDrive() {
	return drive;
}

char *ZFileSpec::getDir( int n ) {
	if( n==-1 ) {
		dir[0] = 0;
		for( int i=0; i<sizeof(dirs)/sizeof(dirs[0]); i++ ) {
			strcat( dir, dirs[i] );
		}
		return dir;
	}
	else if( n < sizeof(dirs)/sizeof(dirs[0]) ) {
		return dirs[n];
	}
	return "";
}

char *ZFileSpec::getDirUpTo( int n ) {
	dir[0] = 0;
	for( int i=0; i<=n; i++ ) {
		strcat( dir, getDir(i) );
	}
	return dir;
}

char *ZFileSpec::getFile( int withExtention ) {
	if( withExtention ) {
		dir[0] = 0;
		strcpy( dir, file );
		if( ext[0] ) {
			strcat( dir, "." );
			strcat( dir, ext );
		}
		return dir;
	}
	return file;
}

char *ZFileSpec::getExt() {
	return ext;
}

unsigned int ZFileSpec::getTime() {
	struct stat s;
	int a = stat( path, &s );
	return a==-1 ? 0 : (unsigned int)s.st_mtime;
}

S64 ZFileSpec::getSize() {
	struct stat s;
	int a = stat( path, &s );
	return a==-1 ? 0 : s.st_size;
}

int zFileSpecChdir( char *dir ) {
	#ifdef WIN32
		ZFileSpec spec( dir );
		if( spec.getDrive()[0] ) {
			_chdrive( toupper( spec.getDrive()[0] ) - 'A' + 1  );
		}
	#endif
	return chdir( dir );
}

void zFileSpecUnescapeHTTPURL( char *url ) {
	char *r = url;
	char *w = url;
	while( *r ) {
		if( *r == '%' ) {
			int val = 0;
			r++;

			if( r[0] >= 'a' && r[0] <= 'f' ) val |= ((r[0]-'a')+10) << 4;
			else if( r[0] >= 'A' && r[0] <= 'F' ) val |= ((r[0]-'A')+10) << 4;
			else if( r[0] >= '0' && r[0] <= '9' ) val |= (r[0]-'0') << 4;

			if( r[1] >= 'a' && r[1] <= 'f' ) val |= ((r[1]-'a')+10);
			else if( r[1] >= 'A' && r[1] <= 'F' ) val |= ((r[1]-'A')+10);
			else if( r[1] >= '0' && r[1] <= '9' ) val |= (r[1]-'0');

			*w++ = (unsigned char)val;
			r += 2;
		}
		else {
			*w++ = *r++;
		}
	}
	*w = 0;
}


#ifdef SELFTEST_ZFILESPEC

#include "assert.h"

void main() {
	char *s;
	
	s = zFileSpecMake( 
		FS_DRIVE, "c:", FS_DIR, "\\oink\\", FS_DIR, "\\boink", FS_DIR, "spoink\\",
		FS_DIR, "kerploink", FS_FILE, "file.blowme", FS_EXT, "hard", FS_END
	);
	assert( !stricmp(s,
		"c:\\oink\\boink\\spoink\\kerploink\\file.blowme.hard"
	) );

	s = zFileSpecMake( 
		FS_DRIVE, "\\\\horton\\", FS_DIR, "c\\oink\\", FS_DIR, "\\boink", FS_DIR, "spoink\\",
		FS_DIR, "kerploink", FS_FILE, "file.blowme", FS_EXT, "hard", FS_END
	);
	assert( !stricmp(s,
		"\\\\horton\\c\\oink\\boink\\spoink\\kerploink\\file.blowme.hard"
	) );

	s = zFileSpecMake( 
		FS_DIR, "boink", FS_FILE, "filename", FS_EXT, "ext", FS_END
	);
	assert( !stricmp(s,
		".\\boink\\filename.ext"
	) );

	s = zFileSpecMake( 
		FS_FILE, "filename", FS_END
	);
	assert( !stricmp(s,
		".\\filename"
	) );

	s = zFileSpecMake( 
		FS_FILE, "c:\\dir\\filename", FS_EXT, "ext", FS_END
	);
	assert( !stricmp(s,
		"c:\\dir\\filename.ext"
	) );

	s = zFileSpecMake( 
		FS_DIR, "c:\\dir\\dir\\", FS_FILE, "\\filename", FS_END
	);
	assert( !stricmp(s,
		"c:\\dir\\dir\\filename"
	) );


	// getDrive
	s = ZFileSpec( "c:/foo/bar.cpp" ).getDrive();
	assert( !stricmp("c:",s) );

	s = ZFileSpec( "/foo/bar.cpp" ).getDrive();
	assert( !stricmp("",s) );

	s = ZFileSpec( "//computer/foo/bar.cpp" ).getDrive();
	assert( !stricmp("computer",s) );

	// getDir
	s = ZFileSpec( "c:/foo/bar/file1.txt" ).getDir( -1 );
	assert( !stricmp("/foo/bar/",s) );

	s = ZFileSpec( "c:/foo/bar/file1.txt" ).getDir( 0 );
	assert( !stricmp("/",s) );

	s = ZFileSpec( "c:/foo/bar/file1.txt" ).getDir( 1 );
	assert( !stricmp("foo/",s) );

	s = ZFileSpec( "c:/foo/bar/file1.txt" ).getDir( 2 );
	assert( !stricmp("bar/",s) );

	s = ZFileSpec( "c:/foo/bar/file1.txt" ).getDir( 3 );
	assert( !stricmp("",s) );

	s = ZFileSpec( "./foo/bar/file1.txt" ).getDir( 0 );
	assert( !stricmp("./",s) );

	s = ZFileSpec( "../foo/bar/file1.txt" ).getDir( 0 );
	assert( !stricmp("../",s) );

	s = ZFileSpec( "foo/bar/file1.txt" ).getDir( 0 );
	assert( !stricmp("foo/",s) );

	s = ZFileSpec( "foo/bar/file1.txt" ).getDir( 1 );
	assert( !stricmp("bar/",s) );

	s = ZFileSpec( "foo/bar/file1.txt" ).getDir( 2 );
	assert( !stricmp("",s) );

	s = ZFileSpec( "/foo/bar/file1.txt" ).getDir( -1 );
	assert( !stricmp("/foo/bar/",s) );

	s = ZFileSpec( "/foo/bar/file1.txt" ).getDir( 0 );
	assert( !stricmp("/",s) );

	s = ZFileSpec( "/foo/bar/file1.txt" ).getDir( 1 );
	assert( !stricmp("foo/",s) );

	s = ZFileSpec( "/foo/bar/file1.txt" ).getDir( 2 );
	assert( !stricmp("bar/",s) );


	// getDirUpTo
	s = ZFileSpec( "c:/foo/bar/file1.txt" ).getDirUpTo( 0 );
	assert( !stricmp("/",s) );

	s = ZFileSpec( "c:/foo/bar/file1.txt" ).getDirUpTo( 1 );
	assert( !stricmp("/foo/",s) );

	s = ZFileSpec( "c:/foo/bar/file1.txt" ).getDirUpTo( 2 );
	assert( !stricmp("/foo/bar/",s) );

	s = ZFileSpec( "c:/foo/bar/file1.txt" ).getDirUpTo( 3 );
	assert( !stricmp("/foo/bar/",s) );


	// getFile
	s = ZFileSpec( "c:/foo/bar/file1.txt" ).getFile( 1 );
	assert( !stricmp("file1.txt",s) );

	s = ZFileSpec( "c:/foo/bar/" ).getFile( 1 );
	assert( !stricmp("",s) );

	s = ZFileSpec( "c:/foo/bar/file1" ).getFile( 1 );
	assert( !stricmp("file1",s) );

	s = ZFileSpec( "c:/foo/bar/file1.txt" ).getFile( 0 );
	assert( !stricmp("file1",s) );

	s = ZFileSpec( "c:/foo/bar/file1.txt.dat" ).getFile( 0 );
	assert( !stricmp("file1.txt",s) );

	s = ZFileSpec( "c:/foo/bar/file1.txt.dat" ).getFile( 1 );
	assert( !stricmp("file1.txt.dat",s) );


	// getExt
	s = ZFileSpec( "c:/foo/bar/file1.txt" ).getExt();
	assert( !stricmp("txt",s) );

	s = ZFileSpec( "c:/foo/bar/" ).getExt();
	assert( !stricmp("",s) );

	s = ZFileSpec( "c:/foo/bar/file1" ).getExt();
	assert( !stricmp("",s) );

	s = ZFileSpec( "c:/foo/bar/file1.txt" ).getExt();
	assert( !stricmp("txt",s) );

	s = ZFileSpec( "c:/foo/bar/file1.txt.dat" ).getExt();
	assert( !stricmp("dat",s) );
}

#endif


