// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Allows modification of variables by telnet.  Uses the zvars
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zvarstelnet.cpp zvarstelnet.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "string.h"
#include "math.h"
// MODULE includes:
#include "zvarstelnet.h"
// ZBSLIB includes:
#include "zregexp.h"
#include "zvars.h"
#include "ztelnetserver.h"
#include "ztmpstr.h"

static interactiveMode = 1;
#ifndef min
	#define min(a,b) ( ( (a) < (b) ) ? (a) : (b) )
#endif
#ifndef max
	#define max(a,b) ( ( (a) > (b) ) ? (a) : (b) )
#endif

void zVarsTelnetCommand( char *cmd, class ZTelnetServerSession *s ) {
	static ZRegExp cmdParse( "\\.([a-zA-Z]+) *([^\n\r]*)" );
	if( cmdParse.test( cmd ) ) {
		char c[256];
		char o[256];
		cmdParse.get( 1, c, 256 );
		cmdParse.get( 2, o, 256 );
		if( !strcmp(c,"save") ) {
			zVarsSave( o );
		}
		else if( !strcmp(c,"load") ) {
			zVarsLoad( o );
		}
		else if( !strcmp(c,"noninteractive") ) {
			interactiveMode = 0;
		}
		else if( !strcmp(c,"interactive") ) {
			interactiveMode = 1;
		}
		else if( !strcmp(c,"sendall") ) {
			int last = -1;
			ZVarPtr *varPtr;
			while( zVarsEnum( last, varPtr ) ) {
				char buf[1024] = {0};
				varPtr->makeTabLine( buf );
				s->puts( buf );
			}
		}
		else if( !strcmp(c,"sendupdates") ) {
			int last = -1;
			ZVarPtr *varPtr;
			while( zVarsEnum( last, varPtr ) ) {
				if( varPtr->copiesAreDifferent() ) {
					char buf[1024] = {0};
					varPtr->makeTabLine( buf );
					s->puts( buf );
					varPtr->updateCopy();
				}
			}
		}
	}
}

int zVarsTelnetHandler( char *line, class ZTelnetServerSession *s ) {
	if( line == NULL ) {
		interactiveMode = 0;
		s->setCharMode();
		s->setEchoMode(0);
		return 1;
	}

	if( line == (char*)1 ) {
		//printf( "Dropped\n" );
		return 1;
	}

	static char input[256] = {0,};
	static char last[256] = {0,};
	static char tab[256]   = {0,};
	static int  tabCount = 0;
	static int  scaleCount = 10000;
	static double scaleBase = 1.0;

	char c =line[0];
	if( c != '\t' ) tabCount = 0;
	if( c != '*' && c != '/' ) scaleCount = 10000;
	switch( c ) {
		case 3: // ^C
			input[0]=0;
			if( interactiveMode ) s->puts( "\r" zTelnetServer_ANSI_CLEAR_TO_EOL );
			break;

		case '\b':
			input[max(0,strlen(input)-1)] = 0;
			if( interactiveMode ) s->puts( "\b \b" );
			break;

		case '\t': {
			if( interactiveMode ) {
				if( tabCount == 0 ) {
					strcpy( tab, input );
				}
				char *match = zVarsFindMatch( tab, tabCount );
				if( match ) {
					strcpy( input, ZTmpStr("%s = ",match) );
					zVarsSprintValOnly( input, match );

					s->puts( ZTmpStr("\r%s",zTelnetServer_ANSI_CLEAR_TO_EOL) );
					s->puts( input );

					tabCount++;
				}
			}
			break;
		}

		case '*':
		case '/':
		case ']':
		case '[':
		case '\n': {
			if( input[0] == '.' ) {
				zVarsTelnetCommand( input, s );
				input[0] = 0;
				if( interactiveMode ) s->putC( '\n' );
				break;
			}

			last[0] = 0;
			static ZRegExp assign( "([a-zA-Z0-9_:]+)[^=]*=[ \t]*([0-9.e+\\-]+)(%)?" );
			if( assign.test( input ) ) {
				int percent = assign.get(3) ? 1 : 0;
				double val = atof( assign.get(2) );
				char *name = assign.get(1);
				ZVarPtr *v = zVarsLookup( name );
				double oldVal = 0.0;
				if( v ) {
					if( v->type == zVarTypeINT ) oldVal = (double) *(int *)v->ptr;
					if( v->type == zVarTypeSHORT ) oldVal = (double) *(short *)v->ptr;
					if( v->type == zVarTypeFLOAT ) oldVal = *(float *)v->ptr;
					if( v->type == zVarTypeDOUBLE ) oldVal = *(double *)v->ptr;
					if( (c=='*'||c=='/') && scaleCount==10000 ) {
						scaleBase = oldVal;
						scaleCount = 0;
					}
					if( c=='*' ) scaleCount++;
					if( c=='/' ) scaleCount--;
					double scale = scaleCount==10000 ? 1.0 : exp( (double)scaleCount / 5.0 );
					if( c=='*' || c=='/' ) val = scale * scaleBase;
					if( c==']' ) val += 1.0;
					if( c=='[' ) val -= 1.0;
					if( percent ) val = (val/100.0) * oldVal;
					if( val < v->minB ) val = v->minB;
					if( val > v->maxB ) val = v->maxB;
					if( v->type == zVarTypeINT ) *(int *)v->ptr = (int)val;
					if( v->type == zVarTypeSHORT ) *(short *)v->ptr = (short)val;
					if( v->type == zVarTypeFLOAT ) *(float *)v->ptr = (float)val;
					if( v->type == zVarTypeDOUBLE ) *(double *)v->ptr = val;
					if( c!='\n' ) {
						strcpy( input, ZTmpStr("%s = ",name) );
						zVarsSprintValOnly( input, name );
						if( c=='*' || c=='/' ) {
							strcat( input, ZTmpStr("(%4.2lf %d)", scale, scaleCount) );
						}
						if( interactiveMode ) s->puts( ZTmpStr("\r%s",zTelnetServer_ANSI_CLEAR_TO_EOL) );
						if( interactiveMode ) s->puts( input );
					}
				}
			}
			if( c=='\n' ) {
				input[0] = 0;
				if( interactiveMode ) s->putC( '\n' );
			}
			break;
		}

		default:
			strcat( input, line );
			if( interactiveMode ) s->puts( line );
			break;
	}

	return 1;
}
