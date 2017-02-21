// @ZBS {
//		*MODULE_NAME Perl-like String Splitter and Manipulator
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A Perl-like string/list class.  Perl-like functions such as split are provided
//			Very handy when you are doing text file processing.
//		}
//		+EXAMPLE {
//			ZStr *zStr = zStrSplit( "\s+", "This is a test" );
//			printf( "%s", zStr->get(1) );
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zstr.cpp zstr.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST yes console
//		*PUBLISH yes
// }
// OPERATING SYSTEM specific includes:
// STDLIB includes:
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "assert.h"
#include "ctype.h"
// MODULE includes:
#include "zstr.h"
// ZBSLIB includes:
#include "zregexp.h"
#include "zhashtable.h"

#ifdef __USE_GNU
#define stricmp strcasecmp
#endif

void zStrDelete( ZStr *head ) {
	ZStr *next = NULL;
	while( head ) {
		next = head->next;
		delete head;
		head = next;
	}
}

ZStr::ZStr( char *_s, int len ) {
	s = NULL;
	next = NULL;
	set( _s, len );
}

ZStr::~ZStr() {
	set( NULL );
}

void ZStr::set( char *_s, int len ) {
	if( s ) delete s;
	if( len > 0 ) {
		// An explicit sub-string snip length is specified
		s = (char *)malloc( len+1 );
		memcpy( s, _s, len );
		s[len] = 0;
	}
	else if( !_s || len == 0 ) {
		s = NULL;
	}
	else {
		s = strdup( _s );
	}
}

ZStr::operator int() {
	if( !s ) return (int)0;
	if( s[0]=='0' && s[1]=='x' ) {
        unsigned int a = strtoul (s, NULL, 16 );
		return a;
	}
	return strtol( s, NULL, 10 );
}

ZStr::operator short() {
	if( !s ) return (short)0;
	if( s[0]=='0' && s[1]=='x' ) return (short)strtol( s, NULL, 16 );
	return (short)strtol( s, NULL, 10 );
}

ZStr::operator char() {
	if( !s ) return (char)0;
	if( s[0]=='0' && s[1]=='x' ) return (char)strtol( s, NULL, 16 );
	return (char)strtol( s, NULL, 10 );
}

ZStr::operator float() {
	if( !s ) return (float)0;
	return (float)atof( s );
}

ZStr::operator double() {
	if( !s ) return (double)0;
	return (double)atof( s );
}

ZStr::operator char *() {
	if( !s ) return "";
	return s;
}

ZStr *ZStr::get(int x) {
	ZStr *c;
	for( c=this; x && c; c=c->next, x-- );
	if(c) return c;
	return 0;
}

char *ZStr::getS(int x) {
	ZStr *c;
	for( c=this; x && c; c=c->next, x-- );
	if(c && c->s) {
		return c->s;
	}
	return "";
}

int ZStr::is(int i,char *compare) {
	char *s = getS(i);
	return !strcmp(s,compare);
}

int ZStr::contains(int i,char *compare) {
	char *s = getS(i);
	return strstr(s,compare) ? 1 : 0;
}

int ZStr::getAsInt(int x) {
	return getI(x);
}

int ZStr::getI(int x) {
	ZStr *c;
	for( c=this; x && c; c=c->next, x-- );
	if(c) return (int)*c;
	return 0;
}

float ZStr::getF(int x) {
	ZStr *c;
	for( c=this; x && c; c=c->next, x-- );
	if(c) return (float)*c;
	return 0;
}

double ZStr::getD(int x) {
	ZStr *c;
	for( c=this; x && c; c=c->next, x-- );
	if(c) return (double)*c;
	return 0;
}

void ZStr::appendS( char *str, int lenToAppend ) {
	if( lenToAppend < 0 ) {
		lenToAppend = strlen( str );
	}

	int oldLen = s ? strlen( s ) : 0;
	char *temp = (char *)malloc( oldLen + lenToAppend + 1 );
	memcpy( temp, s, oldLen );
	memcpy( temp+oldLen, str, lenToAppend );
	temp[lenToAppend+oldLen] = 0;
	if( s ) {
		free( s );
	}
	s = temp;
}

void ZStr::prependS( char *str, int lenToPrepend ) {
	if( lenToPrepend < 0 ) {
		lenToPrepend = strlen( str );
	}

	int oldLen = s ? strlen( s ) : 0;
	char *temp = (char *)malloc( oldLen + lenToPrepend + 1 );
	memcpy( temp, str, lenToPrepend );
	memcpy( temp+lenToPrepend, s, oldLen );
	temp[lenToPrepend+oldLen] = 0;
	if( s ) {
		free( s );
	}
	s = temp;
}

int ZStr::getLen() {
	if( !s ) return 0;
	return strlen( s );
}

void zStrChomp( char *str ) {
	if( str ) {
		int len = strlen( str );
		if( len <= 0 ) return;
		while( --len ) {
			if( str[len]=='\r' || str[len]=='\n' ) str[len] = 0;
			else break;
		}
	}
}

ZStr *zStrReplace( ZStr *lines, char *matchRegEx, char *replaceStr ) {
	ZRegExp regEx( matchRegEx );
	int lenOfReplace = strlen( replaceStr );
	for( ZStr *l=lines; l; l=l->next ) {	
		ZStr *replace = new ZStr;
		char *ll = l->getS();
		int readOff = 0;
		int foundOff = 0;
		int foundLen = 0;
		while( regEx.test(ll + readOff) ) {
			foundOff = regEx.getPos(0) + readOff;
			foundLen = regEx.getLen(0);
			replace->appendS( ll + readOff, foundOff-readOff );
			replace->appendS( replaceStr, lenOfReplace );
			readOff = foundOff + foundLen;
		}
		replace->appendS( ll + foundOff+foundLen, strlen(ll)-foundOff+foundLen );
		if( l->s ) {
			free( l->s );
		}
		l->s = replace->s;
	}
	return lines;
}

ZStr *zStrSplit( char *_regExp, char *text ) {
	ZRegExp regExp( _regExp );
	return zStrSplitByRegExpPtr( &regExp, text );
}

ZStr *zStrSplitByRegExpPtr( void *_regExp, char *text ) {
	ZRegExp *regExp = (ZRegExp *)_regExp;
	if( !text ) {
		return new ZStr( NULL );
	}
	char *c = text;
	ZStr *first = NULL;
	ZStr *last = NULL;

	while( regExp->test(c) ) {
		int pos = regExp->getPos(0);
		int len = regExp->getLen( 0 );
		if( !len ) break;
		ZStr *z = new ZStr( c, pos );
		if( last ) {
			last->next = z;
		}
		last = z;
		if( !first ) {
			first = z;
		}
		c += pos + len;
	}
	// Anything that is left over into a trailing zstr
	ZStr *z = new ZStr( c );
	if( last ) {
		last->next = z;
	}
	if( !first ) {
		first = z;
	}
	return first;
}

ZStr *zStrSplitByChar( char splitChar, char *text ) {
	ZStr *first = NULL;
	ZStr *last = NULL;

	char *lastStart = text;
	char *c = text;
	for( ; *c; c++ ) {
		if( *c == splitChar ) {
			ZStr *z = new ZStr( lastStart, c-lastStart );
			if( last ) {
				last->next = z;
			}
			last = z;
			if( !first ) {
				first = z;
			}
			lastStart = c+1;
		}
	}

	// Anything that is left over into a trailing zstr
	ZStr *z = new ZStr( lastStart, c-lastStart );
	if( last ) {
		last->next = z;
	}
	if( !first ) {
		first = z;
	}
	return first;
}

char *zStrJoin( char *pattern, ZStr *head ) {
	int len = 0;
	if( !pattern ) pattern = "";
	int lenOfPattern = strlen(pattern);
	ZStr *z;
	for( z=head; z; z=z->next ) {
		len += strlen(*z) + lenOfPattern;
	}
	char *buf = (char *)malloc( len+1 );
	char *c = buf;
	for( z=head; z; z=z->next ) {
		strcpy( c, *z );
		c += strlen( *z );
		if( z->next ) {
			// Don't add the pattern to the last one
			strcpy( c, pattern );
			c += lenOfPattern;
		}
	}
	return buf;
}

int zStrCount( ZStr *head ) {
	int count = 0;
	for( ZStr *z=head; z; z=z->next, count++ );
	return count;
}

char *zStrEscapeQuote( char *text ) {
	int len = strlen( text );
	char *_text = (char *)malloc( len * 2 + 1 );
	char *s = text;
	char *d = _text;
	while( *s ) {
		if( *s == '\'' || *s == '\"' ) {
			*d++ = '\\';
		}
		*d++ = *s++;
	}
	*d = 0;
	return _text;
}

void zStrHashSplit( char *text, class ZHashTable *hashTable ) {
	assert( hashTable );
	int state = -1;
	char key[1024];
	char val[1024];
	int keyLen = 0, valLen = 0;
	int whichQuote = -1;
		// Which quoting system was used to start this key or value

	for( char *c=text; ; c++ ) {
		assert( keyLen < 1024 && valLen < 1024 );
		switch( state ) {
			case -2:
				// Terminate
				return;

			case -1:
				// Looking for the start of a key
				switch( *c ) {
					case '\'': whichQuote = 1; state = 0; break;
					case '\"': whichQuote = 2; state = 0; break;
					case 0: state = -2; break;
					default:
						if( *c != ' ' ) {
							whichQuote = 0;
							state = 0;
							c--;
						}
				}
				break;

			case 0:
				// Building a key
				if( *c == '\\' && (*(c+1)=='\'' || *(c+1)=='\"') ) {
					key[keyLen++] = *(c+1);
					c++;
				}
				else if( whichQuote == 1 && *c == '\'' ) {
					state = 1;
				}
				else if( whichQuote == 2 && *c == '\"' ) {
					state = 1;
				}
				else if( whichQuote == 0 && *c == ' ' ) {
					state = 1;
				}
				else if( whichQuote == 0 && *c == '=' ) {
					state = 1;
					c--;
				}
				else if( *c == 0 ) {
					state = -2;
				}
				else {
					key[keyLen++] = *c;
				}
				break;

			case 1:
				// Looking for equals
				whichQuote = 0;
				if( *c == 0 ) {
					state = -2;
				}
				else if( *c == '=' ) {
					state = 2;
				}
				break;

			case 2:
				// Looking for the start of a value
				switch( *c ) {
					case '\'': whichQuote = 1; state = 3; break;
					case '\"': whichQuote = 2; state = 3; break;
					case 0: state = -2; break;
					default:
						if( *c != ' ' ) {
							whichQuote = 0;
							state = 3;
							c--;
						}
				}
				break;

			case 3:
				// Building a value
				if( *c == '\\' && (*(c+1)=='\'' || *(c+1)=='\"') ) {
					val[valLen++] = *(c+1);
					c++;
				}
				else if( whichQuote == 1 && *c == '\'' ) {
					state = 4;
				}
				else if( whichQuote == 2 && *c == '\"' ) {
					state = 4;
				}
				else if( whichQuote == 0 && *c == ' ' ) {
					state = 4;
				}
				else if( *c == 0 ) {
					c--;
					state = 4;
				}
				else {
					val[valLen++] = *c;
				}
				break;

			case 4:
				key[keyLen] = 0;
				val[valLen] = 0;
				hashTable->putS( key, val );
				keyLen = 0;
				valLen = 0;
				state = -1;
				if( *c ) c--; else return;
				break;
			
		}
	}
}

ZStr *zStrReadFile( char *file, char *eofTag /*=0*/ ) {
	FILE *f = fopen( file, "rt" );
	if( f ) {
		int bufSize = 0x10000;
		char *line = new char[ bufSize ];
			// @TODO: Make this not care about line size
	
		ZStr *first = 0;
		ZStr *last = 0;
		while( fgets(line,bufSize,f) ) {
			// For some reason on the Mac the fopen "rt" just isn't working here
			// and CRs are getting into the stream.  I have no idea why
			// so I'm just explicitly removing them (zbs)
			// tfb sez: this was probably a file generated on Mac OS9 or earlier
			// in which they used the AppleII/Commodore format of CR-only.
			for( char *c=line; *c; c++ ) {
				if( c[0] == 0x0d ) {
					c[0] = 0x0a;
					c[1] = 0;
					break;
				}
			}

			line[strlen(line)-1] = 0;
			if( eofTag && !strcmp( line, eofTag ) ) {
				// caller may pass string 'eofTag' which we treat as EOF, and do
				// not include in the ZStr returned.
				break;
			}

			ZStr *lineZStr = new ZStr( line );
			if( last ) {
				last->next = lineZStr;
			}
			if( !first ) {
				first = lineZStr;
			}
			last = lineZStr;
		}
		fclose( f );
		delete [] line;
		return first;
	}
	return 0;
}

ZStr *zStrAppendS( ZStr *&head, char *str, int len ) {
	ZStr *s = new ZStr( str, len );
	if( head ) {
		// SEEK to end
		ZStr *c = head;
		while( c->next ) {
			c = c->next;
		}
		c->next = s;
	}
	else {
		head = s;
	}
	return s;
}

ZStr *zStrPrependS( ZStr *&head, char *str, int len ) {
	ZStr *s = new ZStr( str, len );
	s->next = head;
	head = s;
	return s;
}

void zStrAppendList( ZStr *&head, ZStr *listToAppend ) {
	ZStr *l = head;
	while( l && l->next ) {
		l = l->next;
	}
	if( l ) {
		l->next = listToAppend;
	}
	else {
		head = listToAppend;
	}
}

void zStrPrependList( ZStr *&head, ZStr *listToAppend ) {
	ZStr *l = listToAppend;
	while( l && l->next ) {
		l = l->next;
	}
	if( l ) {
		l->next = head;
	}
	head = listToAppend;
}


int stricmpSort( const void *a, const void *b ) {
	return stricmp( (char *)a, (char *)b );
}

void zStrSortStrVectorIgnoreCase( char **vectorOfStrings, int count ) {
	qsort( vectorOfStrings, count, sizeof(char**), stricmpSort );
}

void zStrToLower( char *str ) {
	for( char *p = str; *p; p++ ) {
		*p = tolower( *p );
	}
}

void zStrStripWhitespaceFrontAndBack( char *str ) {
	char *src = str;
	char *dst = str;
	while( *src ) {
		switch( *src ) {
			case ' ':
			case '\t':
			case '\r':
			case '\n':
				src++;
				break;
			default:
				*dst++ = *src++;
		}
	}
}

void zStrPrint( ZStr *head, char *delim ) {
	while( head ) {
		printf( "%s%s", head->s ? head->s : "", delim ? delim : "" );
		head = head->next;
	}
}


#ifdef SELF_TEST

int main() {
	char *test1[] = {
		"A bunch of,stuff,divided by,commas",
		"A line with no commas",
		"A line with one field and an empty trailing field,",
		",A line with trailing field",
		",,A,B",
		",,,",
		",",
		"",
		NULL
	};
	int test1Lens[] = { 4, 1, 2, 2, 4, 4, 2, 1, 1 };

	char *test2[] = {
		"This is a test",
		"This   is   a test",
		"This   is   a test    ",
		"   ",
		"",
		NULL,
	};

	for( int i=0; i<sizeof(test1)/sizeof(test1[0]); i++ ) {
		ZStr *z = zStrSplit( ",", test1[i] );
		assert( zStrCount( z ) == test1Lens[i] );
		char *join = zStrJoin(",",z);
		assert( !test1[i] || !strcmp( join, test1[i] ) );
	}

	for( i=0; i<sizeof(test2)/sizeof(test2[0]); i++ ) {
		ZStr *z = zStrSplit( " +", test2[i] );
		switch( i ) {
			case 0:
			case 1:
				assert( z && !strcmp( *z, "This" ) ); z=z->next;
				assert( z && !strcmp( *z, "is" ) ); z=z->next;
				assert( z && !strcmp( *z, "a" ) ); z=z->next;
				assert( z && !strcmp( *z, "test" ) ); z=z->next;
				assert( !z );
				break;
			case 2:
				assert( z && !strcmp( *z, "This" ) ); z=z->next;
				assert( z && !strcmp( *z, "is" ) ); z=z->next;
				assert( z && !strcmp( *z, "a" ) ); z=z->next;
				assert( z && !strcmp( *z, "test" ) ); z=z->next;
				assert( z && !strcmp( *z, "" ) ); z=z->next;
				assert( !z );
				break;
			case 3:
				assert( z && !strcmp( *z, "" ) ); z=z->next;
				assert( z && !strcmp( *z, "" ) ); z=z->next;
				assert( !z );
				break;
			case 4:
			case 5:
				assert( z && !strcmp( *z, "" ) ); z=z->next;
				assert( !z );
				break;
		}
	}

	char *test3[] = {
		"key1=val1 key2=val2",
		"key1=val1",
		"",
		"=",
		"key1",
		"key1='val1'",
		"key1='val1' key2='val2' key3 = 'val3'",
		"key1='val1' key2=\"val2\" key3 = \"val3\"",
		"'key1'=val1 \"key2\"=\"val2\" key3 = \"val3\"",
	};

	for( i=0; i<sizeof(test3)/sizeof(test3[0]); i++ ) {
		ZHashTable hash;
		zStrHashSplit( test3[i], &hash );
		char *v1 = hash.get( "key1" );
		char *v2 = hash.get( "key2" );
		char *v3 = hash.get( "key3" );
		char *_v1, *_v2, *_v3;
		switch( i ) {
			case 0: _v1="val1"; _v2="val2"; _v3=""; break;
			case 1: _v1="val1"; _v2=""; _v3=""; break;
			case 2: _v1=""; _v2=""; _v3=""; break;
			case 3: _v1=""; _v2=""; _v3=""; break;
			case 4: _v1=""; _v2=""; _v3=""; break;
			case 5: _v1="val1"; _v2=""; _v3=""; break;
			case 6: _v1="val1"; _v2="val2"; _v3="val3"; break;
			case 7: _v1="val1"; _v2="val2"; _v3="val3"; break;
			case 8: _v1="val1"; _v2="val2"; _v3="val3"; break;
		}
		assert( (!v1 || !strcmp(v1,_v1)) && (!v2 || !strcmp(v2,_v2)) && (!v3 || !strcmp(v3,_v3)) );
	}

	return 0;
}
#endif
