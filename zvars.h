// @ZBS {
//		*MODULE_OWNER_NAME zvars
// }

#ifndef ZVARS_H
#define ZVARS_H

#include "stdio.h"
	// for FILE*; see zVarLoad/Save declarations

struct ZVarPtr {
	char *name;
	int type;
		#define zVarTypeINT (1)
		#define zVarTypeSHORT (2)
		#define zVarTypeFLOAT (3)
		#define zVarTypeDOUBLE (4)
	void *ptr;
	void *copyPtr;
	int bounds;
	double minB;
	double maxB;
	double defaultVal;
	int serialNumber;
		// Auto incremented number useful for sorting by declartion order
	double linIncrement;
	double expIncrement;
		// The values by which to twiddle this in lin and exp mode
	int linearTwiddleMode;
		// 0 = (default) exponential
		// 1 = linear

	double getDouble();
	void setFromDouble( double x );
	void deltaFromDouble( double x );
	int copiesAreDifferent();
	void updateCopy();
	void makeTabLine( char *buf );
	void exponentialIncrement( int increment );
	void resetDefault();
	int getPtrSize();
	void setWithinBounds();
};

#define ZVARS_MAXVAL_SHORT  (0x7FFE)
#define ZVARS_MAXVAL_INT    (0x7FFFFFFE)
#define ZVARS_MAXVAL_FLOAT  (1.0e38)
#define ZVARS_MAXVAL_DOUBLE (1.0e38)

void zVarsAdd( char *name, int *value, double minB=-ZVARS_MAXVAL_INT, double maxB=ZVARS_MAXVAL_INT );
void zVarsAdd( char *name, short *value, double minB=-ZVARS_MAXVAL_SHORT, double maxB=ZVARS_MAXVAL_SHORT );
void zVarsAdd( char *name, float *value, double minB=-ZVARS_MAXVAL_FLOAT, double maxB=ZVARS_MAXVAL_FLOAT );
void zVarsAdd( char *name, double *value, double minB=-ZVARS_MAXVAL_DOUBLE, double maxB=ZVARS_MAXVAL_DOUBLE );
void zVarsDel( char *prefix );

#define zvarAdd(n) _addVar(#n,&n)
#define zvarAddB(n,minB,maxB) zVarsAdd(#n,&n,minB,maxB)

// The following is used for initalization of globals.  Use the ADDVAR macro
struct ZVarsAutoConstruct {
	ZVarsAutoConstruct( char *_name, int *val, double minB=-ZVARS_MAXVAL_INT, double maxB=ZVARS_MAXVAL_INT ) {
		zVarsAdd( _name, val, minB, maxB );
	}
	ZVarsAutoConstruct( char *_name, short *val, double minB=-ZVARS_MAXVAL_SHORT, double maxB=ZVARS_MAXVAL_SHORT ) {
		zVarsAdd( _name, val, minB, maxB );
	}
	ZVarsAutoConstruct( char *_name, float *val, double minB=-ZVARS_MAXVAL_FLOAT, double maxB=ZVARS_MAXVAL_FLOAT ) {
		zVarsAdd( _name, val, minB, maxB );
	}
	ZVarsAutoConstruct( char *_name, double *val, double minB=-ZVARS_MAXVAL_DOUBLE, double maxB=ZVARS_MAXVAL_DOUBLE ) {
		zVarsAdd( _name, val, minB, maxB );
	}
};

#define ZVAR(type,name,val) type name = (type)val; ZVarsAutoConstruct __##name( #name, &name )
#define ZVARB(type,name,val,minB,maxB) type name = (type)val; ZVarsAutoConstruct __##name( #name, &name, minB, maxB )
#define ZVARLABEL(label) static short _##label = 0; ZVarsAutoConstruct __##label( #label, &_##label )

int zVarsEnum( int &last, ZVarPtr *&varPtr );
	// Example usage:
	// int last = -1;
	// ZVarPtr *v;
	// while( zVarsEnum( last, v ) ) {

int zVarsCount();
	// How many vars are there in the hash table

class ZHashTable;
ZHashTable *zVarsGet();
	// returns hash; do not alter!

ZVarPtr *zVarsLookup( char *name );
	// Lookup the varptr from the key name

char *zVarsFindMatch( char *str, int which );
	// Find a match based on a partial string

int zVarsResetMatching( char *str );
	// Reset all variables matching a partial string to their default values

void zVarsSave( FILE *f, int asC=0 );
	// Save to open file
void zVarsSave( char *filename, int asC=0 );
	// Save to filename

void zVarsLoad( FILE *f, char *EOTmarker=0 );
	// Load from open file, optional marker for end of vars segment
void zVarsLoad( char *filename );
	// Load from filename; does not clear list first

void zVarsSprintValOnly( char *buf, char *var );
	// strcats the value only into the buf

void zVarsSetFromDouble( char *name, double value );
	// Sets and bound the current value of that variable

void zVarsBoundAll();
	// Goes through all the vars that have bounds and makes sure they are in bounds

int zVarsChanged( char *varListSpaceDelimited );
	// Goes through all the vars that have bounds and makes sure they are in bounds

void zVarsChangedResetAll();
	// Clears the hash of changes

void zVarsSetAdjustLinMode( char *varName, double step );
void zVarsSetAdjustExpMode( char *varName, double step );
	// Sets the default adjust mode and step

#endif

