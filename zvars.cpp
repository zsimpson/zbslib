// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A method for declaring global variables and stuffing them into a hash
//			which allows them to be modified by name at run-time.
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zvars.cpp zvars.h
//		*VERSION 2.0
//		+HISTORY {
//			2.0 Changed to new naming convention
//		}
//		+TODO {
//		}
//		*SELF_TEST yes console
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "math.h"
#include "assert.h"
#ifdef WIN32
#include "malloc.h"
#endif
// MODULE includes:
#include "zvars.h"
// ZBSLIB includes:
#include "ztmpstr.h"
#include "zhashtable.h"

#ifndef min
	#define min(a,b) ( ( (a) < (b) ) ? (a) : (b) )
#endif
#ifndef max
	#define max(a,b) ( ( (a) > (b) ) ? (a) : (b) )
#endif

static ZHashTable *zVarsHash = 0;
	// This hash holds all the ZVarPtr pointers

static int varsRoundToInt( double x ) {
	double f = floor(x);
	if( x - f < 0.5 ) return (int)f;
	return (int)f + 1;
}

// ZVarPtr Methods
//----------------------------------------------------------------------

double ZVarPtr::getDouble() {
	switch( type ) {
		case zVarTypeINT:
			return (double) *(int *)ptr;
			break;
		case zVarTypeSHORT:
			return (double) *(short *)ptr;
			break;
		case zVarTypeFLOAT:
			return (double) *(float *)ptr;
			break;
		case zVarTypeDOUBLE:
			return (double) *(double *)ptr;
			break;
	}
	return 0.0;
}

void ZVarPtr::setFromDouble( double val ) {
	switch( type ) {
		case zVarTypeINT:
			*(int *)ptr = (int)varsRoundToInt(val);
			if( bounds ) {
				if( *(int *)ptr < (int)minB ) *(int *)ptr = (int)minB;
				if( *(int *)ptr > (int)maxB ) *(int *)ptr = (int)maxB;
			}
			break;
		case zVarTypeSHORT:
			*(short *)ptr = (short)varsRoundToInt(val);
			if( bounds ) {
				if( *(short *)ptr < (short)minB ) *(short *)ptr = (short)minB;
				if( *(short *)ptr > (short)maxB ) *(short *)ptr = (short)maxB;
			}
			break;
		case zVarTypeFLOAT:
			*(float *)ptr = (float)val;
			if( bounds ) {
				if( *(float *)ptr < (float)minB ) *(float *)ptr = (float)minB;
				if( *(float *)ptr > (float)maxB ) *(float *)ptr = (float)maxB;
			}
			break;
		case zVarTypeDOUBLE:
			*(double *)ptr = (double)val;
			if( bounds ) {
				if( *(double *)ptr < (double)minB ) *(double *)ptr = (double)minB;
				if( *(double *)ptr > (double)maxB ) *(double *)ptr = (double)maxB;
			}
			break;
	}
}

void ZVarPtr::deltaFromDouble( double val ) {
	switch( type ) {
		case zVarTypeINT:
			*(int *)ptr += (int)varsRoundToInt(val);
			if( bounds ) {
				if( *(int *)ptr < (int)minB ) *(int *)ptr = (int)minB;
				if( *(int *)ptr > (int)maxB ) *(int *)ptr = (int)maxB;
			}
			break;
		case zVarTypeSHORT:
			*(short *)ptr += (short)varsRoundToInt(val);
			if( bounds ) {
				if( *(short *)ptr < (short)minB ) *(short *)ptr = (short)minB;
				if( *(short *)ptr > (short)maxB ) *(short *)ptr = (short)maxB;
			}
			break;
		case zVarTypeFLOAT:
			*(float *)ptr += (float)val;
			if( bounds ) {
				if( *(float *)ptr < (float)minB ) *(float *)ptr = (float)minB;
				if( *(float *)ptr > (float)maxB ) *(float *)ptr = (float)maxB;
			}
			break;
		case zVarTypeDOUBLE:
			*(double *)ptr += (double)val;
			if( bounds ) {
				if( *(double *)ptr < (double)minB ) *(double *)ptr = (double)minB;
				if( *(double *)ptr > (double)maxB ) *(double *)ptr = (double)maxB;
			}
			break;
	}
}

void ZVarPtr::setWithinBounds() {
	if( bounds ) {
		switch( type ) {
			case zVarTypeINT:
				if( *(int *)ptr < (int)minB ) *(int *)ptr = (int)minB;
				if( *(int *)ptr > (int)maxB ) *(int *)ptr = (int)maxB;
				break;
			case zVarTypeSHORT:
				if( *(short *)ptr < (short)minB ) *(short *)ptr = (short)minB;
				if( *(short *)ptr > (short)maxB ) *(short *)ptr = (short)maxB;
				break;
			case zVarTypeFLOAT:
				if( *(float *)ptr < (float)minB ) *(float *)ptr = (float)minB;
				if( *(float *)ptr > (float)maxB ) *(float *)ptr = (float)maxB;
				break;
			case zVarTypeDOUBLE:
				if( *(double *)ptr < (double)minB ) *(double *)ptr = (double)minB;
				if( *(double *)ptr > (double)maxB ) *(double *)ptr = (double)maxB;
				break;
		}
	}
}

int ZVarPtr::copiesAreDifferent() {
	if( !copyPtr ) return 0;
	switch( type ) {
		case zVarTypeINT:
			return *(int *)ptr != *(int *)copyPtr;
		case zVarTypeSHORT:
			return *(short *)ptr != *(short *)copyPtr;
		case zVarTypeFLOAT:
			return *(float *)ptr != *(float *)copyPtr;
		case zVarTypeDOUBLE:
			return *(double *)ptr != *(double *)copyPtr;
	}
	return 0;
}

void ZVarPtr::updateCopy() {
	if( !copyPtr ) return;
	switch( type ) {
		case zVarTypeINT:
			*(int *)copyPtr = *(int *)ptr;
		case zVarTypeSHORT:
			*(short *)copyPtr = *(short *)ptr;
		case zVarTypeFLOAT:
			*(float *)copyPtr = *(float *)ptr;
		case zVarTypeDOUBLE:
			*(double *)copyPtr = *(double *)ptr;
	}
}

void ZVarPtr::makeTabLine( char *buf ) {
	strcpy( buf, ZTmpStr("%s\t%d\t%d\t%le\t%le\t%d\t",
		name, type, bounds, minB, maxB, serialNumber
	));

	if( type == zVarTypeINT && ptr ) {
		strcat( buf, ZTmpStr("%d",*(int *)ptr) );
	}
	else if( type == zVarTypeSHORT && ptr ) {
		strcat( buf, ZTmpStr("%d",*(short *)ptr) );
	}
	else if( type == zVarTypeFLOAT && ptr ) {
		strcat( buf, ZTmpStr("%e",*(float *)ptr) );
	}
	else if( type == zVarTypeDOUBLE && ptr ) {
		strcat( buf, ZTmpStr("%le",*(double *)ptr) );
	}
	strcat( buf, "\r\n" );
}

void ZVarPtr::exponentialIncrement( int increment ) {
	switch( type ) {
		case zVarTypeINT:
			if( *(int *)ptr == 0 ) {
				*(int *)ptr += ( increment < 0 ? -1 : ( increment > 0 ? 1 : -0 ) );
			}
			else {
				double ee = exp((double)*(int *)ptr);
				*(int *)ptr = varsRoundToInt( (double)*(int *)ptr + ee * (increment < 0 ? -1.0 : 1.0) );
			}
			if( bounds ) {
				if( *(int *)ptr < (int)minB ) *(int *)ptr = (int)minB;
				if( *(int *)ptr > (int)maxB ) *(int *)ptr = (int)maxB;
			}
			break;
		case zVarTypeSHORT:
			if( *(short *)ptr == 0 ) {
				*(short *)ptr += ( increment < 0 ? -1 : ( increment > 0 ? 1 : -0 ) );
			}
			else {
				double ee = exp((double)*(int *)ptr);
				*(short *)ptr = varsRoundToInt( (double)*(short *)ptr + ee * (increment < 0 ? -1.0 : 1.0) );
			}
			if( bounds ) {
				if( *(short *)ptr < (short)minB ) *(short *)ptr = (short)minB;
				if( *(short *)ptr > (short)maxB ) *(short *)ptr = (short)maxB;
			}
			break;
		case zVarTypeFLOAT:
			*(float *)ptr = *(float *)ptr + (float)exp(*(float *)ptr) / 50.0f * (increment < 0 ? -1.0f : 1.0f);
			if( bounds ) {
				if( *(float *)ptr < (float)minB ) *(float *)ptr = (float)minB;
				if( *(float *)ptr > (float)maxB ) *(float *)ptr = (float)maxB;
			}
			break;
		case zVarTypeDOUBLE:
			*(double *)ptr = *(double *)ptr + exp(*(double *)ptr) / 50.0 * (increment < 0 ? -1.0 : 1.0);
			if( bounds ) {
				if( *(double *)ptr < (double)minB ) *(double *)ptr = (double)minB;
				if( *(double *)ptr > (double)maxB ) *(double *)ptr = (double)maxB;
			}
			break;
	}
}

void ZVarPtr::resetDefault() {
	switch( type ) {
		case zVarTypeINT:
			*(int *)ptr = (int)defaultVal;
			break;
		case zVarTypeSHORT:
			*(short *)ptr = (short)defaultVal;
			break;
		case zVarTypeFLOAT:
			*(float *)ptr = (float)defaultVal;
			break;
		case zVarTypeDOUBLE:
			*(double *)ptr = (double)defaultVal;
			break;
	}
}

int ZVarPtr::getPtrSize() {
	switch( type ) {
		case zVarTypeINT:
			return sizeof(int);
		case zVarTypeSHORT:
			return sizeof(short);
		case zVarTypeFLOAT:
			return sizeof(float);
		case zVarTypeDOUBLE:
			return sizeof(double);
	}
	return 0;
}


// Interface
//--------------------------------------------------------------------------

ZHashTable * zVarsGet() {
	return zVarsHash;
}

int zVarsCount() {
	if( zVarsHash ) {
		return zVarsHash->activeCount();
	}
	return 0;
}

int zVarsEnum( int &last, ZVarPtr *&varPtr ) {
	last++;
	while( zVarsHash && last < zVarsHash->size() ) {
		char *key = zVarsHash->getKey( last );
		if( key ) {
			varPtr = (ZVarPtr *)zVarsHash->getValp( last );
			return 1;
		}
		last++;
	}
	return 0;
}

void zVarsAdd( ZVarPtr *v ) {
	if( !zVarsHash ) {
		zVarsHash = new ZHashTable();
	}

	static int serialNum = 0;
	v->serialNumber = serialNum++;
		// Useful for sorting by declartion order
	zVarsHash->putP( v->name, v );
}

ZVarPtr *zVarsLookup( char *name ) {
	if( !zVarsHash ) return 0;
	ZVarPtr *p = (ZVarPtr *)zVarsHash->getp( name );
	if( !p ) return 0;
	return p;
}

void zVarsSprintValOnly( char *buf, char *var ) {
	ZVarPtr *v = zVarsLookup( var );
	if( v ) {
		if( v->type == zVarTypeINT && v->ptr ) {
			strcat( buf, ZTmpStr("%d",*(int *)v->ptr) );
		}
		else if( v->type == zVarTypeSHORT && v->ptr ) {
			strcat( buf, ZTmpStr("%d",*(short *)v->ptr) );
		}
		else if( v->type == zVarTypeFLOAT && v->ptr ) {
			strcat( buf, ZTmpStr("%4.2e",*(float *)v->ptr) );
		}
		else if( v->type == zVarTypeDOUBLE && v->ptr ) {
			strcat( buf, ZTmpStr("%4.2le",*(double *)v->ptr) );
		}
	}
}

static int __varsOrderCompare(const void *a, const void *b) {
	ZVarPtr *_a = zVarsLookup( *(char **)a );
	ZVarPtr *_b = zVarsLookup( *(char **)b );
	return _a->serialNumber > _b->serialNumber ? 1 : (_a->serialNumber < _b->serialNumber ? -1 : 0);
}

void zVarsSave( char *filename, int asC ) {
	if( !zVarsHash ) return;
	FILE *out = fopen( filename, "w+t" );
	if( out ) {
		zVarsSave( out, asC );
		fclose( out );
	}
}

void zVarsSave( FILE *out, int asC ) {
	if( !zVarsHash ) return;
	char **sorted = (char **)alloca( sizeof(char*) * zVarsHash->activeCount() );

	int count = 0;
	int last = -1;
	ZVarPtr *v;
	while( zVarsEnum( last, v ) ) {
		sorted[count++] = strdup(v->name);
	}
	qsort( sorted, count, sizeof(char*), __varsOrderCompare );

	if( out ) {
		for( int i=0; i<count; i++ ) {
			ZVarPtr *v = zVarsLookup( sorted[i] );
			if( asC ) {
				char valBuf[256]; valBuf[0] = 0;
				zVarsSprintValOnly( valBuf, v->name );

				char *types[5] = { "", "int", "short", "float", "double" };
				if( v->bounds ) {
					char *minVal;
					if( v->type == zVarTypeINT && v->ptr ) minVal = ZTmpStr("%d",(int)v->minB);
					else if( v->type == zVarTypeSHORT && v->ptr ) minVal = ZTmpStr("%d",(short)v->minB);
					else if( v->type == zVarTypeFLOAT && v->ptr ) minVal = ZTmpStr("%4.2e",(float)v->minB);
					else if( v->type == zVarTypeDOUBLE && v->ptr ) minVal = ZTmpStr("%4.2le",(double)v->minB);

					char *maxVal;
					if( v->type == zVarTypeINT && v->ptr ) maxVal = ZTmpStr("%d",(int)v->maxB);
					else if( v->type == zVarTypeSHORT && v->ptr ) maxVal = ZTmpStr("%d",(short)v->maxB);
					else if( v->type == zVarTypeFLOAT && v->ptr ) maxVal = ZTmpStr("%4.2e",(float)v->maxB);
					else if( v->type == zVarTypeDOUBLE && v->ptr ) maxVal = ZTmpStr("%4.2le",(double)v->maxB);

					fprintf( out, "ZVARB( %s, %s, %s, %s, %s );\n", types[v->type], v->name, valBuf, minVal, maxVal );
				}
				else {
					fprintf( out, "ZVAR( %s, %s, %s );\n", types[v->type], v->name, valBuf );
				}
			}
			else {
				char buf[256];
				strcpy( buf, ZTmpStr("%s = ",v->name) );
				zVarsSprintValOnly( buf, v->name );
				fprintf( out, "%s\n", buf );
			}
		}
	}
	for( int i=0; i<count; i++ ) {
		free( sorted[i] );
	}
}

void zVarsLoad( char *filename ) {
	FILE *in = fopen( filename, "rt" );
	if( in ) {
		zVarsLoad( in );
		fclose( in );
	}
}

void zVarsLoad( FILE *in, char *EOTmarker ) {
//	static RegExp assign( "([a-zA-Z0-9_:]+)[^=]*=[ \t]*([0-9.e+\\-]+)" );
	if( in ) {
		char buf[256];
		while( fgets( buf, 256, in ) ) {
			// DETECT end of vars
			if( EOTmarker && !strncmp( EOTmarker, buf, strlen( EOTmarker ) ) ) {
				break;
			} 

			// SPLIT
			char *c=buf, *key=buf, *endKey=0, *equals=0, *val=0, *endVal=0;

			// LOOKING for end of key
			for( ; *c; c++ ) {
				if( *c == '\t' || *c == ' ' || *c == '=' ) {
					endKey = c;
					break;
				}
			}

			// LOOKING for equals
			for( ; *c; c++ ) {
				if( *c == '=' ) {
					equals = c;
					c++;
					break;
				}
			}

			// LOOKING for start of val
			for( ; *c; c++ ) {
				if( *c != '\t' && *c != ' ' ) {
					val = c;
					break;
				}
			}

			// LOOKING for end of val
			for( ; *c; c++ ) {
				if( !((*c >= '0' && *c <= '9') || *c=='.' || *c=='e' || *c=='+' || *c=='-') ) {
					endVal = c;
					break;
				}
			}
				
			if( key && endKey && equals && val && endVal ) {
				char _val[80], _key[80];
				memcpy( _val, val, endVal-val );
				_val[endVal-val] = 0;
				memcpy( _key, key, endKey-key );
				_key[endKey-key] = 0;
				double __val = atof( _val );
				ZVarPtr *v = zVarsLookup( _key );
				if( v ) {
					if( __val < v->minB ) __val = v->minB;
					if( __val > v->maxB ) __val = v->maxB;
					if( v->type == zVarTypeINT ) *(int *)v->ptr = (int)__val;
					if( v->type == zVarTypeSHORT ) *(short *)v->ptr = (short)__val;
					if( v->type == zVarTypeFLOAT ) *(float *)v->ptr = (float)__val;
					if( v->type == zVarTypeDOUBLE ) *(double *)v->ptr = __val;
				}
			}
		}
	}
}

void zVarsSetFromDouble( char *name, double value ) {
	ZVarPtr *v = zVarsLookup( name );
	if( v ) {
		v->setFromDouble( value );
	}
}

static int __varsCompare( const void *arg1, const void *arg2 ) {
	char *a = ((ZVarPtr*)arg1)->name;
	char *b = ((ZVarPtr*)arg2)->name;
	return strcmp( a, b );
}

char *zVarsFindMatch( char *str, int which ) {
	// Pull out everything which matches, used for tab expansion
	int size = zVarsHash->activeCount();
	ZVarPtr **list = (ZVarPtr **)alloca( size * sizeof(ZVarPtr*) );

	int count = 0;
	int last = -1;
	ZVarPtr *v;
	while( zVarsEnum( last, v ) ) {
		if( !strncmp( v->name, str, strlen(str) ) ) {
			list[count++] = v;
		}
	}

	qsort( list, count, sizeof(ZVarPtr*), __varsCompare );

	// And return
	if( count == 0 ) {
		return NULL;
	}
	return list[which % count]->name;
}

int zVarsResetMatching( char *str ) {
	// Reset each variable that partially matches to its original value.
	int count = 0;
	int last = -1;
	ZVarPtr *v;
	while( zVarsEnum( last, v ) ) {
		if( !strncmp( v->name, str, strlen(str) ) ) {
			v->resetDefault();
			++count;
		}
	}
	return count;
}

void zVarsAdd( char *name, short *value, double minB, double maxB ) {
	ZVarPtr *v = new ZVarPtr;
	v->name = strdup( name );
	v->type = zVarTypeSHORT;
	v->ptr  = value;
	v->copyPtr = malloc(sizeof(short));
	v->minB  = minB;
	v->maxB  = maxB;
	v->bounds = 1; if( maxB == ZVARS_MAXVAL_SHORT && minB == -ZVARS_MAXVAL_SHORT ) v->bounds = 0;
	v->defaultVal = (double)*value;
	v->linIncrement = 1.0;
	v->expIncrement = 50.0;
	v->linearTwiddleMode = 0;
	zVarsAdd( v );
	v->setFromDouble( (short)*value );
}

void zVarsAdd( char *name, int *value, double minB, double maxB ) {
	ZVarPtr *v = new ZVarPtr;
	v->name = strdup( name );
	v->type = zVarTypeINT;
	v->ptr  = value;
	v->copyPtr = malloc(sizeof(int));
	v->minB  = minB;
	v->maxB  = maxB;
	v->bounds = 1; if( maxB == ZVARS_MAXVAL_INT && minB == -ZVARS_MAXVAL_INT ) v->bounds = 0;
	v->defaultVal = (double)*value;
	v->linIncrement = 1.0;
	v->expIncrement = 50.0;
	v->linearTwiddleMode = 0;
	zVarsAdd( v );
	v->setFromDouble( (double)*value );
}

void zVarsAdd( char *name, float *value, double minB, double maxB ) {
	ZVarPtr *v = new ZVarPtr;
	v->name = strdup( name );
	v->type = zVarTypeFLOAT;
	v->ptr  = value;
	v->copyPtr = malloc(sizeof(float));
	v->minB  = minB;
	v->maxB  = maxB;
	v->bounds = 1; if( maxB == ZVARS_MAXVAL_FLOAT && minB == -ZVARS_MAXVAL_FLOAT ) v->bounds = 0;
	v->defaultVal = (double)*value;
	v->linIncrement = 1.0;
	v->expIncrement = 50.0;
	v->linearTwiddleMode = 0;
	zVarsAdd( v );
	v->setFromDouble( (float)*value );
}

void zVarsAdd( char *name, double *value, double minB, double maxB ) {
	ZVarPtr *v = new ZVarPtr;
	v->name = strdup( name );
	v->type = zVarTypeDOUBLE;
	v->ptr  = value;
	v->copyPtr = malloc(sizeof(double));
	v->minB  = minB;
	v->maxB  = maxB;
	v->bounds = 1; if( maxB == ZVARS_MAXVAL_DOUBLE && minB == -ZVARS_MAXVAL_DOUBLE ) v->bounds = 0;
	v->defaultVal = (double)*value;
	v->linIncrement = 1.0;
	v->expIncrement = 50.0;
	v->linearTwiddleMode = 0;
	zVarsAdd( v );
	v->setFromDouble( *value );
}

void zVarsDel( char *prefix ) {
	// remove all keys that begin with prefix. (tfb, may 16 2009)
	if( zVarsHash ) {
		for( ZHashWalk w( *zVarsHash ); w.next(); ) {
			char *name = w.key;
			if( !strncmp( prefix, name, strlen( prefix ) ) ) {
				zVarsHash->del( name );
			}
		}
	}
}

void zVarsBoundAll() {
	int count = 0;
	int last = -1;
	ZVarPtr *v;
	while( zVarsEnum( last, v ) ) {
		v->setWithinBounds();
	}
}

ZHashTable zVarsChangedHash;

int zVarsChanged( char *varListSpaceDelimited ) {
	int len = strlen( varListSpaceDelimited ) + 1;
	char *tmp = (char *)alloca( len );
	memcpy( tmp, varListSpaceDelimited, len );

	char *tok = strtok( tmp, " " );

	int ret = 0;
	while( tok ) {
		ZVarPtr *varPtr = zVarsLookup( tok );
		assert( varPtr );
		double curVal = varPtr->getDouble();
		if( zVarsChangedHash.lookup(tok)==0 || zVarsChangedHash.getd(tok) != curVal ) {
			zVarsChangedHash.putD( tok, curVal );
			ret = 1;
		}
		tok = strtok( NULL, " " );
	}

	return ret;
}

void zVarsChangedResetAll() {
	zVarsChangedHash.clear();
}

void zVarsSetAdjustLinMode( char *varName, double step ) {
	ZVarPtr *v = zVarsLookup( varName ) ;
	if( v ) {
		v->linearTwiddleMode = 1;
		v->linIncrement = step;
	}
}

void zVarsSetAdjustExpMode( char *varName, double step ) {
	ZVarPtr *v = zVarsLookup( varName ) ;
	if( v ) {
		v->linearTwiddleMode = 0;
		v->expIncrement = step;
	}
}

#ifdef SELF_TEST

ZVAR( int, runPlugin, 1 );
	// You can make a global variable and add it to the editable table with this macro:

double someGlobalDouble = 0.0;

void main( int argc, char **argv ) {
	ZTelnetServer srv( 2020, zVarsTelnetHandler );
		// Create the telnet server and point it to the standard varsTelnetHandler
		// which lives in the vars module.

	varAdd( someGlobalDouble );
		// Alternatively, you can add the variable in like this

	while( 1 ) {
		srv.poll();
			// You have to poll the server since it doesn't have its own thread
			// This is a non-blocking call.
	}
}
#endif

