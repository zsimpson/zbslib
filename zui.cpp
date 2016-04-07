// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A GL gui
//		}
//		*PORTABILITY win32 unix mac
//		*REQUIRED_FILES zui.cpp zui.h
//		*GROUP modules/zui
//		*VERSION 1.0
//		+HISTORY {
//			Based on older "gui" system
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }

/*
Here is some LIMITED ZUI documentation
=======================================

type=ZUISet key=value toZUI=destination		// Sets a variable in a ZUI element
type=ZUIHide								// Hides a zui element
type=ZUIShow								// Stops hiding a zui element
type=ZUIDirty or type=ZUIDirtyAll			// Force a zui element to re-draw
type=ZUIExecuteFile file=myFileName.zui		// Creates a zui from the file specified
type=ZUIDie									// kills a zui
type=ZUILayout								// causes all zui's to re-layout themselves
type=ZUIStyleChanged						// Changes a zui's style (fonts and such)

-- For ZUI Buttons --
ZUIEnable, ZUIDisable, ZUISelect, ZUIDeselect



*/

// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#ifdef __APPLE__
#include "OpenGL/gl.h"
#include "OpenGL/glu.h"
#else
#include "GL/gl.h"
#include "GL/glu.h"
#endif
// STDLIB includes:
#include "assert.h"
#include "string.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"
#include "float.h"
#include "stdarg.h"
#include "ctype.h"
#ifdef WIN32
#include "malloc.h"
#endif
// MODULE includes:
#include "zui.h"
// ZBSLIB includes:
#include "zglfont.h"
#include "zmsg.h"
#include "zmousemsg.h"
#include "zregexp.h"
#include "zstr.h"
#include "zprof.h"
#include "ztmpstr.h"

#include "zgltools.h"
#include "ztime.h"
#include "zvec.h"
	// last three added for drag & drop

// STOPFLOW timing
#include "sfmod.h"

#ifdef __USE_GNU
#define stricmp strcasecmp
#endif

extern void trace( char *fmt, ... );


int lastLayoutFrame;

ZHashTable ZUI::zuiVarLookup;

ZUI_VAR_BEGIN()
	ZUI_VAR(F,x)
	ZUI_VAR(F,y)
	ZUI_VAR(F,w)
	ZUI_VAR(F,h)
	ZUI_VAR(I,layoutManual)
	ZUI_VAR(I,layoutIgnore)
	ZUI_VAR(I,hidden)
	ZUI_VAR(I,container)
	ZUI_VAR(S,name)
	ZUI_VAR(F,scrollX)
	ZUI_VAR(F,scrollY)
	ZUI_VAR(F,translateX)
	ZUI_VAR(F,translateY)
	ZUI_VAR(F,goalX)
	ZUI_VAR(F,goalY)
ZUI_VAR_END()

void *ZUI::getVar( char *memberName, char &type ) {
	int off = ZUI::zuiVarLookup.geti( memberName );
	if( off ) {
		type = (off & 0xFF000000) >> 24;
		return (char *)this + (off & 0x00FFFFFF);
	}
	type = 0;
	return 0;
}


// STYLE & PROPERTIES
//--------------------------------------------------------------------------------------------------------------------------------

ZUI_IMPLEMENT(ZUI,NONE);

void ZUI::styleUpdate() {
	// FIND the style and suck in everthing from it and its ancestors
	// as long as there isn't a local overload
	ZUI *style = 0;
	static char *styleStr = "*style";
	int offset = 1;

	for( ZUI *p = this; p; p=p->parent ) {
		style = (ZUI *)ZUI::zuiByNameHash.getp( p->ZHashTable::getS(&styleStr[offset]) );
		if( style ) {
			// CREATE a list of all the ancestors of this style then go through it backwards
			ZUI *ancestors[128];
			int ancestorCount = 0;
			for( p = style; p; p=p->parent ) {
				ancestors[ancestorCount++] = p;
			}
			for( int i=ancestorCount-1; i>=0; i-- ) {
				// PULL from the style sheet unless there is a local overload
				for( int j=0; j<ancestors[i]->size(); j++ ) {
					ZHashRecord *p = ancestors[i]->hashTable[j];
					if( p ) {
						if( !(p->flags & zhDELETED) ) {
							// CHECK for local overload

							int key_offset = 0;
							if (p->key[0] == '*') {
								key_offset = 1;
							}
							// MMK - Added key_offset to apply only for leading '*'.
							// Original code used p->key[1] instead of p->key[key_offset].

							char inheritedStyle[256];
							strcpy( inheritedStyle, "inherit_style_" );
							strcat( inheritedStyle, &p->key[key_offset] );

							char inheritedProp[256];
							strcpy( inheritedProp, "inherit_prop_" );
							strcat( inheritedProp, &p->key[key_offset] );

							if( ! ZHashTable::has( p->key, p->keyLen ) || ZHashTable::has( inheritedStyle ) || ZHashTable::has( inheritedProp ) ) {
								// ZBS substitiued following code for this during reorg where findKey no longer looks up the heirarchy
								//char *hat = (char *)alloca( p->keyLen + 2 );
								//hat[0] = '^';
								//memcpy( &hat[1], p->key, p->keyLen );
								//bputB( hat, p->keyLen+1, &p->key[p->keyLen], p->valLen, p->type, p->flags );

								bputB( p->key, p->keyLen, &p->key[p->keyLen], p->valLen, p->type, p->flags );
								int t = 1;
								ZHashTable::bputB( inheritedStyle, strlen(inheritedStyle)+1, (char*)&t, sizeof(t) );
							}
						}
					}
				}
			}
			return;
		}
		offset = 0;
	}
}

void ZUI::propertiesInherit() {
	// FOREACH starred prop
	ZUI *z = this;
	ZUI *a = z->parent;

	while( a ) {
		for( ZHashWalk p( *a ); p.next(); ) {
			char *prop = p.key;
			char *a = p.val;
			
			if( prop[0] == '*' ) {
				char inheritedProp[256];
				strcpy( inheritedProp, "inherit_prop_" );
				strcat( inheritedProp, &p.key[1] );

if( !strcmp(inheritedProp,"inherit_prop_buttonTextColor") ) {
int b = 1;
}

				if( ! z->ZHashTable::has( &prop[1] ) || z->ZHashTable::has( inheritedProp ) ) {
					z->ZHashTable::bputB( p.key+1, p.keyLen-1, p.val, p.valLen, p.type, p.flags );
					int t = 1;
					z->ZHashTable::bputB( inheritedProp, strlen(inheritedProp)+1, (char*)&t, sizeof(t) );
				}
			}
			
		}
		a = a->parent;
	}
}

void ZUI::propertiesRunInheritOnAll() {
	// FOREACH zui node
	for( ZHashWalk n( zuiByNameHash ); n.next(); ) {
		// FOREACH starred prop
		ZUI *z = *(ZUI **)n.val;
		z->propertiesInherit();
	}
}



// ZHASHTABLE OVERLOADS
//--------------------

void *ZUI::findBinKey( char *key, int *valLen, int *valType ) {
//	zprofScope( _zui_find_bin_key );
	char type;
	void *bin = ZUI::getVar( key, type );
	if( bin ) {
		// Found a binary version, use that
		switch( type ) {
			case 'F':
				*valLen = sizeof(float);
				*valType = zhFLT;
				return bin;
			case 'I':
				*valLen = sizeof(int);
				*valType = zhS32;
				return bin;
			case 'S':
				*valLen = sizeof(char*);
				*valType = zhSTR;
				return *(char **)bin;
		}
		assert( 0 );
	}
	return 0;
}

void *ZUI::findKey( char *key, int *valLen, int *valType ) {
	void *val;
	int keyLen = strlen( key )+1;

	// SEARCH up the superclass chain looking for a binary overload
	val = findBinKey( key, valLen, valType );
	if( val ) {
		return val;
	}

	// CHECK for local hash copy
//	zprofBeg( _findkey_find_local_hash );
	val = this->bgetb( key, keyLen, valLen, valType, 0 );
//	zprofEnd();
	if( val ) {
		return val;
	}
	return 0;
}

int ZUI::has( char *key ) {
	int valLen, valType;
	void *val = findKey( key, &valLen, &valType );
	if( val ) {
		return 1;
	}
	return 0;
}

int ZUI::isS( char *key, char *cmp ) {
	char *val = getS( key );
	if( val && cmp ) {
		return !strcmp( val, cmp );
	}
	return 0;
}

char *ZUI::getS( char *key, char *onEmpty ) {
	clearLast();
	int valLen, valType;
	void *val = findKey( key, &valLen, &valType );
	if( val ) {
		if( valType != zhSTR ) {
			ZHashTable::convert( valType, (char *)val, valLen, zhSTR, &last, &lastLen );
			return (char*)last;
		}
		return (char*)val;
	}
	return onEmpty;
}

int ZUI::getI( char *key, int onEmpty ) {
	clearLast();
	int valLen, valType;
	void *val = findKey( key, &valLen, &valType );
	if( val ) {
		if( valType != zhS32 ) {
			ZHashTable::convert( valType, (char *)val, valLen, zhS32, &last, &lastLen );
			return *(int *)last;
		}
		return *(int *)val;
	}
	return onEmpty;
}

unsigned int ZUI::getU( char *key, unsigned int onEmpty ) {
	clearLast();
	int valLen, valType;
	void *val = findKey( key, &valLen, &valType );
	if( val ) {
		if( valType != zhU32 ) {
			ZHashTable::convert( valType, (char *)val, valLen, zhU32, &last, &lastLen );
			return *(unsigned int *)last;
		}
		return *(unsigned int *)val;
	}
	return onEmpty;
}

S64 ZUI::getL( char *key, S64 onEmpty ) {
	clearLast();
	int valLen, valType;
	void *val = findKey( key, &valLen, &valType );
	if( val ) {
		if( valType != zhS64 ) {
			ZHashTable::convert( valType, (char *)val, valLen, zhS64, &last, &lastLen );
			return *(S64 *)last;
		}
		return *(S64 *)val;
	}
	return onEmpty;
}

float ZUI::getF( char *key, float onEmpty ) {
	clearLast();
	int valLen, valType;
	void *val = findKey( key, &valLen, &valType );
	if( val ) {
		if( valType != zhFLT ) {
			ZHashTable::convert( valType, (char *)val, valLen, zhFLT, &last, &lastLen );
			return *(float *)last;
		}
		return *(float *)val;
	}
	return onEmpty;
}

double ZUI::getD( char *key, double onEmpty ) {
	clearLast();
	int valLen, valType;
	void *val = findKey( key, &valLen, &valType );
	if( val ) {
		if( valType != zhDBL ) {
			ZHashTable::convert( valType, (char *)val, valLen, zhDBL, &last, &lastLen );
			return *(double *)last;
		}
		return *(double *)val;
	}
	return onEmpty;
}

void *ZUI::getp( char *key, void *onEmpty ) {
	clearLast();
	int valLen, valType;
	void *val = findKey( key, &valLen, &valType );
	if( val ) {
		if( valType != zhPTR && valType != zhPFR ) {
			ZHashTable::convert( valType, (char *)val, valLen, zhPTR, &last, &lastLen );
			return *(void* *)last;
		}
		return *(void* *)val;
	}
	return onEmpty;
}

int ZUI::geti( char *key, int onEmpty ) {
	assert( 0 );
	return 0;
}

unsigned int ZUI::getu( char *key, unsigned int onEmpty ) {
	assert( 0 );
	return 0;
}

S64 ZUI::getl( char *key, S64 onEmpty ) {
	assert( 0 );
	return 0;
}

float ZUI::getf( char *key, float onEmpty ) {
	assert( 0 );
	return 0;
}

double ZUI::getd( char *key, double onEmpty ) {
	assert( 0 );
	return 0;
}

void ZUI::putB( char *key, char *val, int valLen ) {
	assert( 0 );
}

void ZUI::putS( char *key, char *val, int valLen ) {
	int _valLen, _valType;
	void *_val = findBinKey( key, &_valLen, &_valType );
	if( _val ) {
		if( _valType == zhSTR ) {
			free( *(char **)_val );
			*(char **)_val = strdup( val );
		}
		else {
			char *dstVal; int dstValLen;
			ZHashTable::convert( zhSTR, (char *)val, valLen, _valType, &dstVal, &dstValLen );
			memcpy( _val, dstVal, dstValLen );
			free( dstVal );
		}
	}
	else {
		ZHashTable::putS( key, val, valLen );
	}
	checkNotifyMsg( key );
}

void ZUI::putS( const char *key, char *val, int valLen ) {
	putS( (char*)key, (char*)val, valLen );
}

void ZUI::putS( char *key, const char *val, int valLen ) {
	putS( (char*)key, (char*)val, valLen );
}

void ZUI::putS( const char *key, const char *val, int valLen ) {
	putS( (char*)key, (char*)val, valLen );
}


void ZUI::putI( char *key, int val ) {
	int _valLen, _valType;
	void *_val = findBinKey( key, &_valLen, &_valType );
	if( _val ) {
		// @TODO: add convert
		assert( _valType == zhS32 );
		*(int *)_val = val;
	}
	else {
		ZHashTable::putI( key, val );
	}
	checkNotifyMsg( key );
}

void ZUI::putI( const char *key, int val ) {
	putI( (char*)key, val );
}

void ZUI::putU( char *key, unsigned int val ) {
	int _valLen, _valType;
	void *_val = findBinKey( key, &_valLen, &_valType );
	if( _val ) {
		// @TODO: add convert
		assert( _valType == zhU32 );
		*(unsigned int *)_val = val;
	}
	else {
		ZHashTable::putU( key, val );
	}
	checkNotifyMsg( key );
}

void ZUI::putL( char *key, S64 val ) {
	int _valLen, _valType;
	void *_val = findBinKey( key, &_valLen, &_valType );
	if( _val ) {
		// @TODO: add convert
		assert( _valType == zhS64 );
		*(S64 *)_val = val;
	}
	else {
		ZHashTable::putL( key, val );
	}
	checkNotifyMsg( key );
}

void ZUI::putF( char *key, float val ) {
	int _valLen, _valType;
	void *_val = findBinKey( key, &_valLen, &_valType );
	if( _val ) {
		// @TODO: add convert
		assert( _valType == zhFLT );
		*(float *)_val = val;
	}
	else {
		ZHashTable::putF( key, val );
	}
	checkNotifyMsg( key );
}

void ZUI::putD( char *key, double val ) {
	int _valLen, _valType;
	void *_val = findBinKey( key, &_valLen, &_valType );
	if( _val ) {
		if( _valType == zhDBL ) {
			*(double *)_val = val;
		}
		else {
			if( _valType == zhSTR ) {
				free( *(char **)_val );
				// this will have to clean
			}
			char *dstVal; int dstValLen;
			ZHashTable::convert( zhDBL, (char *)&val, sizeof(double), _valType, &dstVal, &dstValLen );
			memcpy( _val, dstVal, dstValLen );
			free( dstVal );
		}
	}
	else {
		ZHashTable::putD( key, val );
	}
	checkNotifyMsg( key );
}

void ZUI::putP( char *key, void *ptr, int freeOnReplace, int delOnNul ) {
	int _valLen, _valType;
	void *_val = findBinKey( key, &_valLen, &_valType );
	if( _val ) {
		// @TODO: add convert
		assert( _valType == zhPTR || _valType == zhPFR );
		*(void* *)_val = ptr;
	}
	else {
		ZHashTable::putP( key, ptr, freeOnReplace, delOnNul );
	}
	checkNotifyMsg( key );
}

void ZUI::putKeyStr( char *key, char *fmt, ... ) {
	static char tempString[512];
	{
		va_list argptr;
		va_start( argptr, fmt );
		vsprintf( tempString, fmt, argptr );
		va_end( argptr );
		assert( strlen(tempString) < 512 );
	}

	putS( key, tempString );
	checkNotifyMsg( key );
}

char *ZUI::dumpToString() {
	char *s = ZHashTable::dumpToString();
	char extraInfo[512] = {0};
	sprintf( extraInfo, "parent=%s\n", parent ? parent->name : "" );
	char *string = new char[ strlen(s) + strlen(extraInfo) + 1 ];
	strcpy( string, s );
	strcat( string, extraInfo );
	delete s;
	return string;
}


// COLOR
//--------------------

ZHashTable ZUI::colorPaletteHash;

void ZUI::setColorI( int color ) {
	if( (color&0x000000FF) != 0xFF ) {
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	}
	else {
		glDisable( GL_BLEND );
	}
	glColor4ub( (color&0xFF000000)>>24, (color&0x00FF0000)>>16, (color&0x0000FF00)>>8, color&0x000000FF );
}

void ZUI::setColorS( char *colorName ) {
	static int colorPaletteHashInit = 0;
	if( ! colorPaletteHashInit ) {
		// INITIALIZE the colorPalette
		colorPaletteHashInit++;
		// void* is used as a datatype below to allow compilation as either 32 or 64bit, and have the
		// datatype change size accordingly (as the char* for "red" will).
		void* colors[] = {
			(void*)"red", (void*)0xFF0000FF, (void*)"green", (void*)0x00FF00FF, (void*)"blue", (void*)0x0000FFFF,
			(void*)"white", (void*)0xFFFFFFFF, (void*)"black", (void*)0x000000FF,
			(void*)"yellow", (void*)0xFFFF00FF,
			(void*)"plum", (void*)0xFF01CCFF, (void*)"purple", (void*)0xBA01FFFF, (void*)"cyan", (void*)0xFF00FFFF,
			(void*)"orange", (void*)0xFAAB00FF, (void*)"rose", (void*)0xFF594CFF, (void*)"lavendar", (void*)0xC64CFFFF,
			(void*)"brown", (void*)0x523210FF, (void*)"tan", (void*)0x89643DFF, (void*)"gray", (void*)0x808080FF,
		};
		for( int i=0; i<sizeof(colors)/sizeof(colors[0]); i+=2 ) {
			#if defined(__LP64__) || defined(_Wp64)
			colorPaletteHash.putU( (char *)colors[i], (long long)colors[i+1] );
			#else
			colorPaletteHash.putU( (char *)colors[i], (unsigned int)colors[i+1] );
			#endif
		}
	}
	int color = colorPaletteHash.getU( colorName );
	setColorI( color );
}

void ZUI::setupColor( char *key ) {
	if( !has(key) ) {
		setColorI( 0xFFFFFFFF );
	}
	else {
		char *colorName = getS( key );
		if( colorName && (colorName[0] >= '0' && colorName[0] <= '9') || colorName[0]=='-' ) {
			int color = getU( key );
			setColorI( color );
		}
		else {
			setColorS( colorName );
		}
	}
}

int ZUI::byteOrderReverse( int i ) {
	unsigned char temp[4];
	unsigned char *src = (unsigned char *)&i;
	temp[0] = src[3];
	temp[1] = src[2];
	temp[2] = src[1];
	temp[3] = src[0];
	return *(int *)&temp;
}

int ZUI::getColorI( char *key ) {
	if( !has(key) ) {
		return 0xFFFFFFFF;
	}
	else {
		char *colorName = getS( key );
		if( colorName && (colorName[0] >= '0' && colorName[0] <= '9') || colorName[0]=='-' ) {
			int color = getU( key );
			//color = byteOrderReverse( color );
				// removed by tfb April 2015.  Seems wrong.  Note that setColorI/S etc
				// never do bytereversal.
			return color;
		}
		else {
			int color = colorPaletteHash.getU( colorName );
			//color = byteOrderReverse( color );
				// removed by tfb April 2015.  Seems wrong.  Note that setColorI/S etc
				// never do bytereversal.
			return color;
		}
	}
}

void ZUI::scaleAlpha( float scale ) {
	float color[4];
	glGetFloatv( GL_CURRENT_COLOR, color );
	color[3] *= scale;
	glColor4fv( color );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
}

// POSITION / SIZE
//--------------------

void ZUI::getXY( float &x, float &y ) {
	x = this->x;
	y = this->y;
}

void ZUI::getWH( float &w, float &h ) {
	w = this->w;
	h = this->h;
}

void ZUI::getXYWH( float &x, float &y, float &w, float &h ) {
	x = this->x;
	y = this->y;
	w = this->w;
	h = this->h;
}

void ZUI::getWindowXY( float &x, float &y ) {
	x = 0.f;
	y = 0.f;
	for( ZUI *o=this; o; o=o->parent ) {
		float _x, _y;
		o->getXY( _x, _y );
		float sx = o->scrollX;
		float sy = o->scrollY;
		float tx = o->translateX;
		float ty = o->translateY;
		if( o==this ) {
			// A scroll applies to the children of a panel, not the panel itself
			sx = 0.f;
			sy = 0.f;
		}
		x += _x + sx + tx;
		y += _y + sy + ty;
	}
}

void ZUI::getLocalXY( float &_x, float &_y ) {
	for( ZUI *o=this; o; o=o->parent ) {
		float sx = o->scrollX;
		float sy = o->scrollY;
		float tx = o->translateX;
		float ty = o->translateY;
		if( o==this ) {
			// A scroll applies to the children of a panel, not the panel itself
			sx = 0.f;
			sy = 0.f;
		}
		_x -= o->x + sx + tx;
		_y -= o->y + sy + ty;
	}
}

int ZUI::containsLocal( float _x, float _y ) {
	return( _x >= 0.f && _x < w && _y >= 0.f && _y < h );
}

void ZUI::move( float _x, float _y ) {
	if( _x != x || _y != y ) {
		zMsgQueue( "type=ZUIMoved oldX=%f oldY=%f newX=%f newY=%f toZUI='%s'", x, y, _x, _y, name );
		x = _x;
		y = _y;
	}
}

void ZUI::moveNear( char *who, char *where, float xOff, float yOff ) {
	float _x=0.f, _y=0.f;
	float myW, myH;
	getWH( myW, myH );
	ZUI *o = zuiFindByName( who );
	if( o ) {
		float x=0.f, y=0.f;
		o->getWindowXY( x, y );
		if( *where == 'B' ) {
			_x = x;
			_y = y - myH;
		}
		else if( *where == 'T' ) {
			_x = x;
			float otherW, otherH;
			o->getWH( otherW, otherH );
			_y = y + otherH;
		}
		_x += xOff;
		_y += yOff;
		int viewport[4];
		glGetIntegerv( GL_VIEWPORT, viewport );
		_x = max( 0, min( viewport[2], _x ) );
		_y = max( 0, min( viewport[3], _y ) );
		getLocalXY( _x, _y );
		move( _x, _y );
	}
}

void ZUI::resize( float _w, float _h, int layout ) {
	if( w != _w || h != _h ) {
		zMsgQueue( "type=ZUIResized oldW=%f oldH=%f newW=%f newH=%f toZUI='%s'", w, h, _w, _h, name );
		w = _w;
		h = _h;
		if( layout ) {
			layoutRequest();	
		}
	}
}

void ZUI::orderAbove( ZUI *toMoveZUI, ZUI *referenceZUI ) {
	// If the toMoveZUI occurs before the referenceZUI then we need
	// to move the toMoveZUI otherwise it is already above so we don't do anything
	
	// tfb addition: if referenceZUI is null, order above everything
	if( !referenceZUI ) {
		ZUI *tail;
		for( tail=toMoveZUI->parent->headChild; tail->nextSibling; tail=tail->nextSibling );
		if( toMoveZUI == tail ) {
			return;
		}
		referenceZUI = tail;
	}

	ZUI *first = NULL;
	for( ZUI *c=toMoveZUI->parent->headChild; c; c=c->nextSibling ) {
		if( !first && c==toMoveZUI ) {
			first = c;
			break;
		}
		else if( !first && c==referenceZUI ) {
			first = c;
			break;
		}
	}

	if( first == toMoveZUI ) {
		// RELINK so that toMoveZUI is next after the referenceZUI
		ZUI *movePrev = toMoveZUI->prevSibling;
		ZUI *moveNext = toMoveZUI->nextSibling;
		ZUI *refPrev = referenceZUI->prevSibling;
		ZUI *refNext = referenceZUI->nextSibling;

		// UNLINK the moving one from where it is
		if( movePrev ) {
			movePrev->nextSibling = moveNext;
		}
		if( moveNext ) {
			moveNext->prevSibling = movePrev;
		}

		// LINK it to where it is moving
		referenceZUI->nextSibling = toMoveZUI;
		toMoveZUI->prevSibling = referenceZUI;
		toMoveZUI->nextSibling = refNext;
		if( refNext ) {
			refNext->prevSibling = toMoveZUI;
		}

		// UPDATE the headlink
		if( toMoveZUI == toMoveZUI->parent->headChild ) {
			toMoveZUI->parent->headChild = moveNext;
		}
	}
}

void ZUI::orderBelow( ZUI *toMoveZUI, ZUI *referenceZUI ) {
	// If the toMoveZUI occurs before the referenceZUI then we need
	// to move the toMoveZUI otherwise it is already above so we don't do anything
	ZUI *first = NULL;
	for( ZUI *c=toMoveZUI->parent->headChild; c; c=c->nextSibling ) {
		if( !first && c==toMoveZUI ) {
			first = c;
			break;
		}
		else if( !first && c==referenceZUI ) {
			first = c;
			break;
		}
	}

	if( first == referenceZUI ) {
		// RELINK so that toMoveZUI is next before the referenceZUI
		ZUI *movePrev = toMoveZUI->prevSibling;
		ZUI *moveNext = toMoveZUI->nextSibling;
		ZUI *refPrev = referenceZUI->prevSibling;
		ZUI *refNext = referenceZUI->nextSibling;

		// UNLINK the moving one from where it is
		if( movePrev ) {
			movePrev->nextSibling = moveNext;
		}
		if( moveNext ) {
			moveNext->prevSibling = movePrev;
		}

		// LINK it to where it is moving
		referenceZUI->prevSibling = toMoveZUI;
		toMoveZUI->nextSibling = referenceZUI;
		toMoveZUI->prevSibling = refPrev;
		if( refPrev ) {
			refPrev->nextSibling = toMoveZUI;
		}

		// UPDATE the headlink
		if( referenceZUI == toMoveZUI->parent->headChild ) {
			toMoveZUI->parent->headChild = toMoveZUI;
		}
	}
}

ZUI *ZUI::findByCoord( float winX, float winY ) {
	// CHECK for children first
	for( ZUI *o=headChild; o; o=o->nextSibling ) {
		ZUI *found = o->findByCoord( winX, winY );
		if( found ) {
			return found;
		}
	}

	getLocalXY( winX, winY );
		// winX, winY now in local coords

	if( containsLocal(winX,winY) ) {
		return this;
	}
	return 0;
}

ZUI *ZUI::findByCoordWithAttribute( float winX, float winY, char *key, char *val ) {
	if( !strcmp( name, "zubzub0" ) ) {
		int d= 1;
	}
	// CHECK for children first
	for( ZUI *o=headChild; o; o=o->nextSibling ) {
		ZUI *found = o->findByCoordWithAttribute( winX, winY, key, val );
		if( found ) {
			return found;
		}
	}

	getLocalXY( winX, winY );
		// winX, winY now in local coords

	if( containsLocal(winX,winY) && !strcmp( getS( key, "key" ), val ) ) {
		return this;
	}
	return 0;
}


// HIERARCHY
//--------------------

void ZUI::attachTo( ZUI *_parent ) {
	assert( _parent );
	if( parent ) {
		detachFrom();
	}
	parent = _parent;

	this->styleUpdateFlag = 1;
	//styleUpdate();
	//propertiesInherit();

	// REQUEST layouts
	layoutRequest();

	// ADD to the end of the list
	nextSibling = prevSibling = 0;
	ZUI *tail = 0;
	for( ZUI *o = _parent->headChild; o; o=o->nextSibling ) {
		tail = o;
	}
	if( tail ) {
		tail->nextSibling = this;
		prevSibling = tail;
	}
	else {
		parent->headChild = this;
	}
}

void ZUI::detachFrom() {
	if( parent ) {
		layoutRequest();

		if( prevSibling ) {
			prevSibling->nextSibling = nextSibling;
		}
		if( nextSibling ) {
			nextSibling->prevSibling = prevSibling;
		}
		if( parent->headChild == this ) {
			parent->headChild = nextSibling;
		}
		parent = NULL;
		prevSibling = NULL;
		nextSibling = NULL;
	}
}

int ZUI::isMyChild( char *_name ) {
	for( ZUI *o=headChild; o; o=o->nextSibling ) {
		if( !strcmp( name, _name ) ) {
			return 1;
		}
	}
	return 0;
}

int ZUI::isMyAncestor( ZUI *_parent ) {
	for( ZUI* p=parent; p; p=p->parent ) {
		if ( p == _parent )
			return 1;
	}
	return 0;
}

int ZUI::childCount() {
	int count  = 0;
	ZUI *child = headChild;
	while( child ) {
		count++;
		child = child->nextSibling;
	}
	return count;
}

// DEATH
//--------------------

ZUI *ZUI::deadHead = 0;

void ZUI::zuiGarbageCollect() {
	ZUI *o = deadHead;
	ZUI *next = NULL;
	while( o ) {
		next = o->nextSibling;
		o->destruct();
		delete o;
		o = next;
	}
	deadHead = 0;
}

void ZUI::killChildren() {
	ZUI *next = NULL;
	for( ZUI *o = headChild; o; o=next ) {
		next = o->nextSibling;
		o->die();
	}
}

void ZUI::die() {
	if( getI( "immortal" ) ) {
		// This is a usefuil attribute that keeps code generated
		// objects alive so that they can be reattached to
		// dynamically generated content
		detachFrom();
		return;
	}

	// MARK this and all children as dead.  They will 
	// be synchronously garbage collected by the update function.

	ZUI *next = NULL;
	for( ZUI *o = headChild; o; o=next ) {
		next = o->nextSibling;
		o->die();
	}
	if( ZUI::modalStackTop > 0 ) {
		if( ZUI::modalStack[ZUI::modalStackTop-1] == this ) {
			modal( 0 );
		}
	}
	if( ZUI::focusStackTop > 0 ) {
		if( ZUI::focusStack[ZUI::focusStackTop-1] == this ) {
			focus( 0 );
		}
	}


	// REMOVE the name from the hash
	zuiByNameHash.del( name );

	// REMOVE from zuiWantUpdates
	ZUI* t = (ZUI*)this;
	ZUI::zuiWantUpdates.del( (char*)&t, sizeof(ZUI*) );

	// ADD into the dead list
	putI( "dead", 1 );
	detachFrom();
	nextSibling = deadHead;
	deadHead = this;
}

// MODALITY & FOCUS
//--------------------

ZUI *ZUI::modalStack[ZUI_STACK_SIZE];
int ZUI::modalStackTop = 0;
ZUI *ZUI::focusStack[ZUI_STACK_SIZE];
int ZUI::focusStackTop = 0;

void ZUI::modal( int onOff ) {
	if( onOff ) {
		assert( ZUI::modalStackTop < ZUI_STACK_SIZE );
		ZUI::modalStack[ ZUI::modalStackTop++ ] = this;
	}
	else {
		ZUI::modalStackTop--;
		assert(ZUI::modalStackTop>=0);
		assert(ZUI::modalStack[ ZUI::modalStackTop ] == this);
		ZUI::modalStack[ ZUI::modalStackTop ] = 0;
	}
}

void ZUI::focus( int onOff ) {
	// TFB: test having a single focus zui, rather than a stack, to avoid
	// complications such as ZUIs dieing that are down the stack somewhere.
	// The changes below are the only mods for this new paradigm.

	if( onOff ) {
		ZUI::focusStack[ 0 ] = this;
		ZUI::focusStackTop   = 1;
	}
	else {
		if( focusStack[ 0 ] == this ) {
			ZUI::focusStack[ 0 ] = 0;
			ZUI::focusStackTop   = 0;
		}
	}
}

// DRAG AND DROP
//--------------

int ZUI::dragTextureId = -1;
ZUI *ZUI::dragZuiSource = 0;
ZUI *ZUI::dragZuiTarget = 0;
float ZUI::dragOffsetX = 0;
float ZUI::dragOffsetY = 0;
double ZUI::cancelTime = -1.0;
float ZUI::cancelX = 0;
float ZUI::cancelY = 0;

void ZUI::pixelsToTexture( int texture ) {
	// assumes we have been rendered on the screen buffer
	float wx,wy;
	getWindowXY( wx, wy );

	int imageWidth = (int)w;
	int imageHeight = (int)h;

	// ALLOC pixel buffer & fill with screen data
	#define BYTES_PER_PIXEL 4
		// @TODO: this is always the zlab default, but we should read this from glfw util fn?
	unsigned char * pixels = new unsigned char[ imageWidth * imageHeight * BYTES_PER_PIXEL ];
	glReadPixels( (int)wx, (int)wy, imageWidth, imageHeight, GL_RGBA,  GL_UNSIGNED_BYTE, pixels);

	// COPY pixel data to texture
	glBindTexture( GL_TEXTURE_2D, texture );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
}

void ZUI::beginDragAndDrop( float startx, float starty ) {
	//trace( "ZUI::beginDragAndDrop on % s:  %g, %g ----------------------- \n", name, startx, starty );
	putI( "dragAndDropping", 1 );
	dragOffsetX = w/2.f; // startx;
	dragOffsetY = h/2.f; // starty;
		// setting to the center of the dragged zui removes problems at low framerate or really fast
		// drag speed when the mouse arrow can then be outside the dragged item, which may be confusing
		// to the user in terms of where to drop - when the mouse is over the target, or when the zui is?
		// This way the dragged item just gets centered on the mouse.
	dragZuiSource = this;
	dragZuiTarget = 0;

	// generate a texture, and fill it with our pixels.
	int texture = zglGenTexture();
	pixelsToTexture( texture );
	dragTextureId = texture;
		// a non-negative texture id tells the zui system to draw the dragged object at the mouse

	char *sendMsg = getS( "sendMsgOnDragInit" );
	zMsgQueue( "%s", sendMsg );
}

void ZUI::renderDragAndDrop() {
	if( dragZuiTarget && dragZuiSource ) {
		// hilight the target
		float tx,ty,tw,th;
		dragZuiTarget->getWindowXY( tx, ty );
		dragZuiTarget->getWH( tw, th );
		dirtyRectAdd( (int)tx, (int)ty, (int)tw, (int)th );
		glColor4ub( 255, 255, 255, 200 );
		glLineWidth( 2.f );
		tx += 2; 
		ty += 2;
		tw -= 4;
		th -= 4;
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glBegin( GL_LINE_LOOP );
			glVertex2f( tx, ty );
			glVertex2f( tx + tw-1, ty );
			glVertex2f( tx + tw-1, ty + th-1 );
			glVertex2f( tx, ty + th-1 );
		glEnd();
	}
	if( dragZuiSource && dragTextureId >= 0 ) {
		// render a quad with the texture of the object being dragged.
		float x0,x1,y0,y1;
		if( cancelTime < 0 ) {
			x0 = zMouseMsgX - dragOffsetX;	
			y0 = zMouseMsgY - dragOffsetY;
		}
		else {
			// a positive cancelTime means the drag was canceled at some point, 
			// and we want to animate it popping back to where it started.
			#define DRAG_CANCEL_ACCEL 10000.0
			float homeX, homeY;
			dragZuiSource->getWindowXY( homeX, homeY );
			homeX += dragOffsetX;
			homeY += dragOffsetY;
			FVec2 toHome( homeX - cancelX, homeY - cancelY );
			float totalDist = toHome.mag();
			toHome.normalize();
			float time = float(zTimeNow() - cancelTime);
			float dist = 1000.f * time + float( DRAG_CANCEL_ACCEL * time * time ) / 2.f;
			if( dist >= totalDist ) {
				resetDragAndDrop();
				return;
			}
			toHome.mul( dist );
			toHome.add( FVec2( cancelX, cancelY ) );
			x0 = toHome.x - dragOffsetX;
			y0 = toHome.y - dragOffsetY;
		}
		x1 = x0 + dragZuiSource->w;
		y1 = y0 + dragZuiSource->h;
	
		dragZuiTarget ? glColor4ub( 255, 255, 255, 220 ) : glColor4ub( 255, 200, 200, 128 );
			// reddish hue when not over a valid drop target
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glEnable( GL_TEXTURE_2D );
		glBindTexture( GL_TEXTURE_2D, dragTextureId );
		zglTexRect( x0, y0, x1, y1, 0, 0, 1.0, 1.0 );
		dirtyRectAdd( (int)x0, (int)y0, (int)(x1-x0), (int)(y1-y0) );
	}
}

void ZUI::updateDragAndDrop() {
	// find out who we're over and set state information to allow the render to
	// indicate to the user whether or not we're over a valid drop location.
	ZUI *root = ZUI::zuiFindByName( "root" );
	ZUI *target = 0;
	if( root ) {
		dragZuiTarget = root->findByCoordWithAttribute( (float)zMouseMsgX, (float)zMouseMsgY, "dragAcceptObjectType", getS( "dragEmitObjectType" ) );
			// find a zui under the mouse that accepts the type of object we emit
	}
}

void ZUI::endDragAndDrop() {
	// find out who we're over, and see if they'll accept the object.
	int cancel = 1;
	ZUI *root = ZUI::zuiFindByName( "root" );
	ZUI *target = 0;
	if( root ) {
		target = root->findByCoordWithAttribute( (float)zMouseMsgX, (float)zMouseMsgY, "dragAcceptObjectType", getS( "dragEmitObjectType" ) );
			// find a zui under the mouse that accepts the type of object we emit
	}
	if( target && target != this ) {
		// @TODO: we need more validation here (or elsewhere?)  if we were to do it here, we probably need a callback fn
		// that can be called with the data in question, as well as src->dst, so transfer can be validated somehow, and
		// we can get the result of that validation so we might choose to set a flag that causes the render to look different
		// to give feedback to the user that the drop location is valid or not.
		char *sendMsg = target->getS( "sendMsgOnDragAccept" );
		//trace( "endDragAndDrop: an accepting zui was found: %s\n", target->name );
		zMsgQueue( "%s zuiAccept=%s dragAcceptObjectType=%s dragAcceptObject=%ld", sendMsg, target->name, getS( "dragEmitObjectType" ), long( getp( "dragEmitObject" ) ) ); 
		resetDragAndDrop();
		cancel = 0;
	}
	if( cancel ) {
		// rubber-band the rendered object back to it's origin; ZUI system will do this.
		cancelTime = zTimeNow();
		cancelX = (float)zMouseMsgX;
		cancelY = (float)zMouseMsgY;
		//trace( "endDragAndDrop: NO accepting zui was found! (canceled)\n" );
	}
	char *sendMsg = getS( "sendMsgOnDragEnd", 0 );
	if( sendMsg ) {
		zMsgQueue( "%s cancel=%d", sendMsg, cancel );
	}
	putI( "dragAndDropping", 0 );
	//trace( "ZUI::endDragAndDrop finished: dragZuiSource = %X\n", dragZuiSource );
}

void ZUI::resetDragAndDrop() {
	// cleanup & prevent further render of dragged object
	if( dragTextureId >= 0 ) {
		glDeleteTextures( 1, (GLuint*)&dragTextureId );
		dragTextureId = -1;
	}
	dragZuiSource = 0;
	cancelTime = -1;
	//trace( "ZUI::resetDragAndDrop finished: dragZuiSource = %X\n", dragZuiSource );
}

// DIRTY RECTS
//--------------------

int ZUI::dirtyRectCount = 0;

int ZUI::dirtyRects[DIRTY_RECTS_MAX][4];

void ZUI::dirtyRectAdd( int x, int y, int w, int h ) {
	// Walk list of rects, noting rects that we enclose.  If
	// we find a rect that encloses us, early out.

	if( w<=0 || h<=0 ) {
		return;
	}

	#define ISCONTAINED(_x,_y,_w,_h,_i) ( dirtyRects[_i][0] <= _x && dirtyRects[_i][0] + dirtyRects[_i][2] >= _x+_w && \
									      dirtyRects[_i][1] <= _y && dirtyRects[_i][1] + dirtyRects[_i][3] >= _y+_h )
		// rect def'd by _x,_y,_w,_h is contained by dirtyRects[_i]

	#define CONTAINS(_x,_y,_w,_h,_i) ( dirtyRects[_i][0] >= _x && dirtyRects[_i][0] + dirtyRects[_i][2] <= _x+_w && \
					  				   dirtyRects[_i][1] >= _y && dirtyRects[_i][1] + dirtyRects[_i][3] <= _y+_h )
		// rect def'd by _x,_y,_w,_h contains dirtyRects[_i]

	ZHashTable removeRects;
	for( int i=0; i<dirtyRectCount; i++ ) {
		if( ISCONTAINED( x, y, w, h, i ) ) {
			// early out, the passed rect is already contained in an existing rect
			return;
		}
		else if( CONTAINS( x, y, w, h, i ) ) {
			// dirtyRect[i] should be removed - fully contained in the passed rect
			removeRects.bputI( &i, sizeof(i), 1 );
		}
	}

	if( removeRects.activeCount() ) {
		// remove the rects that will be consumed by adding the passed rect 
		int oldRects[DIRTY_RECTS_MAX][4];
		int oldRectCount = dirtyRectCount;
		memcpy( oldRects, dirtyRects, sizeof( dirtyRects ) );
		for( int i=0, dirtyRectCount=0; i<oldRectCount; i++ ) {
			if( !removeRects.bgetI( &i, sizeof(i), 0 ) ) {
				dirtyRects[dirtyRectCount][0] = oldRects[i][0];
				dirtyRects[dirtyRectCount][1] = oldRects[i][1];
				dirtyRects[dirtyRectCount][2] = oldRects[i][2];
				dirtyRects[dirtyRectCount][3] = oldRects[i][3];
				dirtyRectCount++;
			}
		}
	}

	if( dirtyRectCount < DIRTY_RECTS_MAX ) {
		// add the passed rect (note if we removed any, above condition always true)
		dirtyRects[dirtyRectCount][0] = x;
		dirtyRects[dirtyRectCount][1] = y;
		dirtyRects[dirtyRectCount][2] = w;
		dirtyRects[dirtyRectCount][3] = h;
		dirtyRectCount++;
	}
	else {
		// Invalidate the whole screen
		dirtyRectCount = 1;
		dirtyRects[0][0] = 0;
		dirtyRects[0][1] = 0;
		dirtyRects[0][2] = 100000;
		dirtyRects[0][3] = 100000;
	}
}

void ZUI::dirtyAll() {
	// Invalidate the whole screen
	dirtyRectCount = 1;
	dirtyRects[0][0] = 0;
	dirtyRects[0][1] = 0;
	dirtyRects[0][2] = 100000;
	dirtyRects[0][3] = 100000;
}

void ZUI::dirty( int el, int er, int et, int eb ) {
	float xwin, ywin;
	getWindowXY( xwin, ywin );

	int xi = int(xwin - 0.5f) - el;
	int yi = int(ywin - 0.5f) - eb;
	int wi = int(w + 1.f) + el + er;
	int hi = int(h + 1.f) + et + eb;
	xi = max( 0, min( (int)rootW, xi ) );
	yi = max( 0, min( (int)rootH, yi ) );

	int r = xi + wi;
	int t = yi + hi;
	r = max( xi, min( (int)rootW, r ) );
	t = max( yi, min( (int)rootH, t ) );

	wi = r - xi;
	hi = t - yi;

	dirtyRectAdd( xi, yi, wi, hi );
}

void ZUI::scissorIntersect( int _x, int _y, int _w, int _h ) {
	// glScissor using the argument intersected with
	// the current scissor box.
	int scissorBox[4];
	glGetIntegerv( GL_SCISSOR_BOX, scissorBox );
	int _r = min( _x + _w, scissorBox[0] + scissorBox[2] );
	int _t = min( _y + _h, scissorBox[1] + scissorBox[3] );
	_x = max( _x, scissorBox[0] ); 
	_y = max( _y, scissorBox[1] );
	_w = _r - _x;
	_h = _t - _y;
	glScissor( _x, _y, _w, _h );
}


// RENDER
//--------------------

//ZHashTable zuiProfHash;

// mmk - glPushAttrib() and glPopAttrib() are *extremely* expensive on my home machine.
// So for stopflow, these are removed and replaced with only the attributes needed.
// For other zlab apps, these continue to be used.

void traceRecurseRenderChain( ZUI *z ) {
	static int level=0;

	level++;

	static char indent[512];
	memset( indent, 0, 512 );
	memset( indent, 32, level );

	trace( "%s%s %s\n", indent, z->name, z->getS( "class", "unknown" ) );
	if( level == 1 ) {
		char *zuiContents = z->dumpToString();
		trace( " zui dump for top of recurse:\n%s\n", zuiContents );
	}
	for( ZUI *o=z->headChild; o; o=o->nextSibling ) {
		traceRecurseRenderChain( o );
	}

	level--;
}

#ifdef _DEBUG
char zuiLastRender[512];
#endif
void ZUI::recurseRender( int whichDirtyRect ) {
	if( hidden ) {
		return;
	}

	float speed = 6.f * float( zuiTime - zuiLastTime );
	speed = min( 1.f, speed );
	this->x += speed * (this->goalX - this->x);
	this->y += speed * (this->goalY - this->y);

	float _x, _y;
	getWindowXY( _x, _y );

	int li = (int)(_x-0.5f);
	int bi = (int)(_y-0.5f);
	int ri = (int)(_x + w + 0.5f );
	int ti = (int)(_y + h + 0.5f );

	// EARLY out if no intersection with the dirty rect
	if( 
		whichDirtyRect >= 0 && whichDirtyRect < dirtyRectCount && (
			   li > dirtyRects[whichDirtyRect][0] + dirtyRects[whichDirtyRect][2]
			|| ri < dirtyRects[whichDirtyRect][0]
			|| bi > dirtyRects[whichDirtyRect][1] + dirtyRects[whichDirtyRect][3]
			|| ti < dirtyRects[whichDirtyRect][1]
		)
	) {
		return;
	}

	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();

	glTranslatef( floorf(x+translateX), floorf(y+translateY), 0.0f );

	int viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );

	#ifdef _DEBUG
	strcpy( zuiLastRender, name );
	#endif

	//unsigned __int64 start = zprofTick();

#ifndef SFFAST
	SFTIME_START (PerfTime_ID_Zlab_getI, PerfTime_ID_Zlab_render_recurse);
	int clipToBounds = getI( "clipToBounds" );
	SFTIME_END (PerfTime_ID_Zlab_getI);
	if( clipToBounds ) {
		glPushAttrib( GL_SCISSOR_BIT );
		glEnable( GL_SCISSOR_TEST );
		scissorIntersect( (int)_x, (int)_y, (int)w, (int)h );
			// zuis may choose to clip themselves and chilren
	}
	glPushAttrib( GL_ALL_ATTRIB_BITS );
#else
	// SFFAST_TFBMOD: we must respect clipToBounds, which is a property of some kinds of
	// zuis.  We needn't push/pop all the attrib bits, but we must do scissoring.  If you
	// do not use ZUIs that make use of clipToBounds (e.g. listboxes) then you'll never
	// push/pop these bits.
	int clipToBounds = getI( "clipToBounds" );
	if( clipToBounds ) {
		glPushAttrib( GL_SCISSOR_BIT );
		glEnable( GL_SCISSOR_TEST );
		scissorIntersect( (int)_x, (int)_y, (int)w, (int)h );
			// zuis may choose to clip themselves and childen
	}
#endif


	SFTIME_START (PerfTime_ID_Zlab_render_render, PerfTime_ID_Zlab_render_recurse);

#ifdef KIN_DEMO
	int matDepth1=0, matDepth2=0;
	glGetIntegerv( GL_MODELVIEW_STACK_DEPTH, &matDepth1 );
#endif

	render();
	SFTIME_END (PerfTime_ID_Zlab_render_render);

#ifdef KIN_DEMO
	glGetIntegerv( GL_MODELVIEW_STACK_DEPTH, &matDepth2 );
	if( matDepth1 != matDepth2 ) {
		trace( "matDepth problem: matDepth1=%d, matDepth2=%d", matDepth1, matDepth2 );
		trace( "zui rendered was %s\n", this->name );
		char *zuiContents = this->dumpToString();
		trace( "zui dump:\n%s\n", zuiContents );
		assert( false && "aborting render" );
	}

#endif

#ifndef SFFAST
	glPopAttrib();
#endif

	//unsigned __int64 stop = zprofTick();
	//__int64 ticks = stop - start;
	//zuiProfHash.putD( name, (double)ticks );

	// @TODO: Scrolls needs to coords into the window coords and the scissoring
	glTranslatef( scrollX, scrollY, 0.f );

	for( ZUI *o=headChild; o; o=o->nextSibling ) {
		int attDepth1=0, attDepth2=0;
		int matDepth1, matDepth2;

		glGetIntegerv( GL_ATTRIB_STACK_DEPTH,    &attDepth1 );
		glGetIntegerv( GL_MODELVIEW_STACK_DEPTH, &matDepth1 );
		
		o->recurseRender( whichDirtyRect );

		glGetIntegerv( GL_ATTRIB_STACK_DEPTH,    &attDepth2 );
		glGetIntegerv( GL_MODELVIEW_STACK_DEPTH, &matDepth2 );
		
		if( matDepth1 != matDepth2 ) {
			trace( "matDepth problem: matDepth1=%d, matDepth2=%d, recurseRender chain follows:\n", matDepth1, matDepth2 );
			traceRecurseRenderChain( o );
		}
		if( attDepth1 != attDepth2 ) {
			trace( "attDepth problem: attDepth1=%d, attDepth2=%d\n", attDepth1, attDepth2 );
		}

		assert( matDepth1 == matDepth2 );
		assert( attDepth1 == attDepth2 );
	}

#ifndef SFFAST
	if( clipToBounds ) {
		glPopAttrib( /*GL_SCISSOR_BIT*/ );
	}
#else
	// SFFAST_TFBMOD: pop the scissor bit if we it pushed earlier
	if( clipToBounds ) {
		glPopAttrib();
	}
#endif

	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();
}

int useDirtyRects = 0;
int drawDirtyRects = 0;
	// moved to global scope to allowing toggling via UI in other plugins
#include "zgltools.h"
void ZUI::zuiRenderTree( ZUI *root ) {
	int i;

	// Recursively render all of the zui objects
	if( !root ) root = zuiFindByName( "root" );
	if( !root ) return;

	// TURN OFF all effects
	glPushAttrib( GL_ALL_ATTRIB_BITS );    // mmk - would like to remove this also, but there is still some visual effect
	glDisable( GL_LIGHTING );
	glDisable( GL_COLOR_MATERIAL );
	glDisable( GL_CULL_FACE );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_BLEND );
	glDisable( GL_TEXTURE_2D );
	glDisable( GL_FOG );

	// Reset both matricies to gives us simple 2D coordinates
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glPushMatrix();
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glPushMatrix();

	// PIXEL matrix, first quadrant
	float viewport[4];
	glGetFloatv( GL_VIEWPORT, viewport );
	glTranslatef( -1.0f, -1.0f, 0.f );
	if( viewport[2] != 0.f && viewport[3] != 0.f ) {
		glScalef( 2.0f/viewport[2], 2.0f/viewport[3], 1.f );
	}

	SFTIME_START (PerfTime_ID_Zlab_render_recurse, PerfTime_ID_Zlab_render_tree);

#ifdef SFFAST
	// Enable states that we will keep constant, as the performance impact of changing them is large.
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );    // this one is still being set when blending is activated
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
#endif

	// TRAVERSE the dirty rects rendering each one
	if( useDirtyRects && !postLayoutRender ) {
		glEnable( GL_SCISSOR_TEST );
		for( i=0; i<dirtyRectCount; i++ ) {
			glScissor( dirtyRects[i][0], dirtyRects[i][1], dirtyRects[i][2], dirtyRects[i][3] );
			root->recurseRender( i );
		}
		glDisable( GL_SCISSOR_TEST );

		// DRAW the dirtyrects
		if( drawDirtyRects ) {
			glColor3ub( rand()%50+200, rand()%50+200, rand()%50+200 );
//			glColor3ub( rand()%50, rand()%50, rand()%50 );
			for( i=0; i<dirtyRectCount; i++ ) {
				glBegin( GL_LINE_LOOP );
				glVertex2i( dirtyRects[i][0], dirtyRects[i][1] );
				glVertex2i( dirtyRects[i][0] + dirtyRects[i][2], dirtyRects[i][1] );
				glVertex2i( dirtyRects[i][0] + dirtyRects[i][2], dirtyRects[i][1] + dirtyRects[i][3] );
				glVertex2i( dirtyRects[i][0], dirtyRects[i][1] + dirtyRects[i][3] );
				glEnd();
			}
		}
		dirtyRectCount = 0;
	}
	else {
		root->recurseRender( -1 );
		postLayoutRender = 0;
	}
	SFTIME_END (PerfTime_ID_Zlab_render_recurse);
	renderDragAndDrop();
		// we render a dragged object after all render passes to avoid the object
		// being occluded based on zui depth.  The same could apply to twiddler arrows.


	glPopAttrib();    // mmk - would like to remove this also, but there is still some visual effect
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();
}

void ZUI::printSize( char *m, float &w, float &h, float &d, int len ) {
	void *fontPtr = zglFontGet( getS("font") );
	zglFontGetDim( m, w, h, d, 0, fontPtr, len );
}

void ZUI::print( char *m, float l, float t, int len ) {
	float trashW;
	void *fontPtr = zglFontGet( getS("font") );
	float segmentH, d;
	zglFontGetDimChar( 'W', trashW, segmentH, d, 0, fontPtr );
	float startY = h - t - segmentH;
	zglFontPrint( m, l, t-segmentH+d, 0, 0, fontPtr, len );
}

void ZUI::printWordWrap( char *m, float l, float t, float w, float h, float *computeW, float *computeH ) {
	float segmentW = 0.f, segmentH = 0.f;
	char temp;
	char *lastSpace = NULL;

	void *fontPtr = zglFontGet( getS("font") );

	int len = strlen( m ) + 1;
	char *copyOfM = (char *)alloca( len );
	memcpy( copyOfM, m, len );
	char *segmentStart = copyOfM;
	float descender;
	float trashW;
	zglFontGetDimChar( 'W', trashW, segmentH, descender, 0, fontPtr );
	float startY = h - t - segmentH;
	float y = startY;
	float maxW = 0.f;

	// Calculate word-wrap
	for( char *c = copyOfM; ; c++ ) {
		if( *c == '\n' || *c == 0 ) {
			temp = *c;
			*c = 0;
			if( !computeH ) {
				zglFontPrint( segmentStart, l, y+descender, 0, 0, fontPtr );
			}
			else {
				zglFontGetDim( segmentStart, segmentW, segmentH, descender, 0, fontPtr );
			}
			*c = temp;
			y -= segmentH;
			segmentStart = c+1;
			maxW = max( maxW, segmentW );
			segmentW = 0.0;
			if( *c == 0 ) {
				break;
			}
		}
		else {
			float charW;
			zglFontGetDimChar( *c, charW, segmentH, descender, 0, fontPtr );
			segmentW += charW;

			if( segmentW > w ) {
				if( !lastSpace || *c == ' ' ) {
					lastSpace = c;
				}
				temp = *lastSpace;
				*lastSpace = 0;
				if(!computeH) {
					zglFontPrint( segmentStart, l, y+descender, 0, 0, fontPtr );
				}
				else {
					zglFontGetDim( segmentStart, segmentW, segmentH, descender, 0, fontPtr );
				}
				*lastSpace = temp;
				y -= segmentH;
				segmentStart = *lastSpace==' ' ? lastSpace+1 : lastSpace;
				c = lastSpace;
				lastSpace = NULL;
				maxW = max( maxW, segmentW );
				segmentW = 0.0;
			}
			else if( *c == ' ' ) {
				lastSpace = c;
			}
		}
	}
	if( computeW ) {
		*computeW = maxW;
	}
	if( computeH ) {
		*computeH = -(y - startY);
	}
}

void ZUI::begin3d( bool setViewport ) {
//	glMatrixMode( GL_TEXTURE );
//	glPushMatrix();
//	glLoadIdentity();
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();
//	glPushAttrib( GL_ALL_ATTRIB_BITS );
	
	if( setViewport ) {	
		putI( "setViewport", 1 );
		glPushAttrib( GL_VIEWPORT_BIT );
		float _x,_y;
		getWindowXY( _x, _y );
		glViewport( (int)_x, (int)_y, (int)w, (int)h );
	}
}

void ZUI::end3d() {
	if( getI( "setViewport" ) ) {
		del( "setViewport" );
		glPopAttrib();
	}
//	glPopAttrib();
//	glMatrixMode( GL_TEXTURE );
//	glPopMatrix();
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();
}

// REGISTRATION
//--------------------

ZHashTable ZUI::zuiByNameHash;
ZHashTable *ZUI::factoryTablePtrs = 0;
ZHashTable *ZUI::factoryTableSize = 0;
ZHashTable *ZUI::factoryTableSuperclass = 0;
	// These have to be dynamically allocated because
	// we can't ensure the startup order of the globals.
	// So we allocate them on demand in zuiFactoryRegisterClass

void ZUI::registerAs( char *newName ) {
	assert( ! zuiByNameHash.getp( newName ) );
		// Duplicate name

	char *currName = name;
	if( currName && *currName ) {
		zuiByNameHash.del( currName );
	}

	static int defaultNameCounter = 1;
	char defaultName[32];
	if( !newName || (newName && !*newName) ) {
		sprintf( defaultName, "zui-%d", defaultNameCounter++ );
		newName = defaultName;
	}
	if( newName && *newName ) {
		assert( zuiByNameHash.getS( newName ) == 0 );
		zuiByNameHash.putP( newName, this );
		name = strdup( newName );
	}
}

void ZUI::factoryRegisterClass( void *ptr, int size, char *newName, char *superclass ) {
	if( !factoryTablePtrs ) {
		factoryTablePtrs = new ZHashTable();
		factoryTableSize = new ZHashTable();
		factoryTableSuperclass = new ZHashTable();
	}
	factoryTablePtrs->putP( newName, ptr );
	factoryTableSize->putI( newName, size );
	factoryTableSuperclass->putS( newName, superclass );
}

ZUI *ZUI::factory( char *newName, char *type ) {
	assert( factoryTablePtrs );

	int size = factoryTableSize->getI( type );
	ZUI *templateCopy = (ZUI *)factoryTablePtrs->getp( type );
	if( zuiErrorTrapping ) {
		if( !(size && templateCopy) ) {
			char buffer[512];
			sprintf( buffer, "ZUI Error: Factory failed to find type=%s\n", type );
			int newLen = (zuiErrorMsg?strlen(zuiErrorMsg):0) + strlen( buffer ) + 1;
			char *_errorMsg = (char *)malloc( newLen );
			*_errorMsg = 0;
			strcat( _errorMsg, zuiErrorMsg?zuiErrorMsg:"" );
			strcat( _errorMsg, buffer );
			free( zuiErrorMsg );
			zuiErrorMsg = _errorMsg;

			// CREATE a crap stub
			size = factoryTableSize->getI( "ZUI" );
			templateCopy = (ZUI *)factoryTablePtrs->getp( "ZUI" );
		}
	}
	else {
		assert( size && templateCopy );
	}

	ZUI *o = (ZUI *)malloc( size );
	memcpy( o, templateCopy, size );
	o->ZHashTable::init();
	o->putS( "class", type );
	o->registerAs( newName );
	o->factoryInit();
	o->styleUpdateFlag = 1;
	o->appendNotifyMsg( "hidden", "type=ZUIDirty toZUI=$this; type=ZUINotifyHiddenChanged toZUI=$this" );
	
	return o;
}

ZUI *ZUI::zuiFindByName( char *_name ) {
	char *slash = strchr( _name, '/' );
	if( slash ) {
		const int STACKSIZE = 1024;
		int stackTop = 0;
		ZUI *stack[ STACKSIZE ];

		// USE local addressing
		ZUI *found = 0;
		char *dupName = strdup( _name );
		dupName[ slash - _name ] = 0;
		ZUI *container = (ZUI *)zuiByNameHash.getp( dupName );
		char *localName = slash+1;
		if( container ) {
			// RECURSIVE search the children looking for the local name
			stack[ stackTop++ ] = container;
			while( stackTop > 0 ) {
				ZUI *s = stack[ --stackTop ];
				for( ZUI *o=s->headChild; o; o=o->nextSibling ) {
					if( o->isS("localName",localName) ) {
						found = o;
						goto done;
					}
					stack[stackTop++] = o;
					assert( stackTop < STACKSIZE );
				}
			}
			done:;
		}
		free( dupName );
		return found;
	}
	return (ZUI *)zuiByNameHash.getp( _name );	
}

void ZUI::init() {
	ZHashTable::init();
	factoryInit();
}

// CLONE
//--------------------

ZUI *ZUI::clone( ZUI *root, char *newName ) {
	// RECURSIVE clone 

	ZUI *myClone = root->cloneMe(newName);
	for( ZUI *o=root->headChild; o; o=o->nextSibling ) {
		ZUI::clone( o )->attachTo( myClone );
	}
	return myClone;
}

ZUI *ZUI::cloneMe(char *newName) {
	ZUI *copy = factory( newName, getS( "class" ) );
	copy->clear();
		// clear our hash first; note binary properties (e.g. name) not cleared or copied via hash
	copy->copyFrom( *this );
		// copy hash from *this
	copy->layoutManual = layoutManual;
	copy->layoutIgnore = layoutIgnore;
	copy->hidden = hidden;
	copy->container = container;
		// not sure if other binary properties need be copied?  We don't want all of
		// them, for example "name"
	return copy;
}

// LAYOUT
//--------------------

void ZUI::layoutRequest() {
//	trace( "[%04d] ::layoutRequest\n", zuiFrameCounter );
	if( zuiFrameCounter == lastLayoutFrame+1 ) {
		int a =1 ;
	}
	zuiLayoutRequest++;
}

int ZUI::canLayout() {
	return !hidden && !layoutManual && !layoutIgnore;
}

float ZUI::layoutParseInstruction( char *str, float maxW, float maxH, float _w, float _h ) {
	if( !str ) return 0.f;
	float stack[10];
	int top=0;
	#define PUSH(x) (stack[top++]=x)
	#define POP() (stack[--top])

	const int START = 1;
	const int NUMBER = 2;
	const int DONE = 3;
	int state = START;
	float constant = 0.f;
	float decimal = 0.f;
	float a, b, d;
	int negative = 0;

	for( char *c=str; state != DONE; c++ ) {
		switch( state ) {
			case START:
				if(
					( c[0] == '-' && c[1] >= '0' && c[1] <='9' )
					|| ( (*c >= '0' && *c <='9') || *c == '.' )
				) {
					c--;
					decimal = 0.f;
					constant = 0.f;
					negative = 0;
					state = NUMBER;
				}
				else {
					switch( *c ) {
						case 0: state = DONE; break;
						case ' ': break;

						case 'W': PUSH( maxW ); break;
						case 'H': PUSH( maxH ); break;
						case 'w': PUSH( _w ); break;
						case 'h': PUSH( _h ); break;

						case '+': a=POP(); b=POP(); PUSH( a + b ); break;
						case '*': a=POP(); b=POP(); PUSH( a * b ); break;
						case '-': a=POP(); b=POP(); PUSH( b - a ); break;
						case '/': a=POP(); b=POP(); PUSH( b / a ); break;

						case '>': a=POP(); b=POP(); PUSH( a > b ); break;
						case '<': a=POP(); b=POP(); PUSH( a < b ); break;

						case '^': a=POP(); b=POP(); PUSH( max(a,b) ); break;
						case 'v': a=POP(); b=POP(); PUSH( min(a,b) ); break;

						case '?': a=POP(); b=POP(); d=POP(); PUSH( a ? b : d ); break;

						default:
							assert( 0 );
								// Illegal instruction
					}
				}
				break;

			case NUMBER:
				if( *c == '-' ) {
					negative = 1;
				}
				if( *c == '.' ) {
					assert( decimal == 0.f );
					decimal = 1.f;
				}
				if( *c >= '0' && *c <= '9' ) {
					if( decimal != 0.f ) {
						decimal /= 10.f;
						constant += (*c - '0') * decimal;
					}
					else {
						constant *= 10.f;
						constant += *c - '0';
					}
				}
				if( *c == ' ' || *c == 0 ) {
					c--;
					PUSH( negative ? -constant : constant );
					state = START;
				}
				if( *c == 0 ) {
					state = DONE;
				}
				break;
		}
	}
	if( top > 0 ) {
		return floorf( POP() );
	}
	return 0.f;
}

ZUI::ZUILayoutCell *ZUI::layoutDispatch( float maxW, float maxH, float parentW, float parentH, float &_w, float &_h, int sizeOnly ) {
	ZUILayoutCell *cells = 0;
	int foundLayout = 0;
	if( container && headChild ) {
		_w = 0.f, _h = 0.f;
		if( isS("layout","pack") ) {
			cells = layoutPack( maxW, maxH, _w, _h, sizeOnly );
			foundLayout++;
		}
		if( isS("layout","border") ) {
			cells = layoutBorder( maxW, maxH, _w, _h, sizeOnly );
			foundLayout++;
		}
		if( isS("layout","table") ) {
			cells = layoutTable( maxW, maxH, _w, _h, sizeOnly );
			foundLayout++;
		}
		if( !foundLayout ) {
			// Default to pack
			cells = layoutPack( maxW, maxH, _w, _h, sizeOnly );
		}
	}
	else {
		return layoutQueryNonContainer( maxW, maxH, parentW, parentH, _w, _h, sizeOnly );
	}

	return cells;
}

ZUI::ZUILayoutCell *ZUI::layoutQuery( float maxW, float maxH, float parentW, float parentH, float &_w, float &_h, int sizeOnly ) {
	// This function distributes layout to the appropriate manager
	// Note that if the sizeOnly flag is set, no layout routine is
	// expected or allowed to allocate a cell array
	ZUILayoutCell *cells = 0;

	// For debug trapping
	float requestW = 0.f, requestH = 0.f;
	char *forceInstructionW = strdup( getS( "layout_forceW", "" ) );
	char *forceInstructionH = strdup( getS( "layout_forceH", "" ) );
	if( *forceInstructionW || *forceInstructionH ) {
		// Run the layout only for the purposes of determining the requestW and H so
		// that the layoutParseInstruction can know what the values of little 'w' and 'h' are
		// We have to store off these values because they can change during the second pass
		if( sizeOnly ) {
			cells = layoutDispatch( maxW, maxH, parentW, parentH, requestW, requestH, 1 );
			assert( cells == 0 );
			putF( "_requestW", requestW );
			putF( "_requestH", requestH );
		}
		else {
			// We need the second pass to be the same as the first so we just extract it
			requestW = getF( "_requestW" );
			requestH = getF( "_requestH" );
		}

		if( *forceInstructionW ) {
			maxW = layoutParseInstruction( forceInstructionW, parentW, parentH, requestW, requestH );
		}
		if( *forceInstructionH ) {
			maxH = layoutParseInstruction( forceInstructionH, parentW, parentH, requestW, requestH );
		}
	}

	_w = 0.f, _h = 0.f;

	if( getI( "permitScrollX" ) ) {
		maxW = max( maxW, 10000 );
			// @TODO: replace the occurances of 10000 with params like scrollXMax or some such.
			// I don't know this is the best place to do this; the idea is that if a panel can 
			// be scrolled, then we needn't abide by the maxW passed in to us -- we can scroll
			// within that area and allow our children to size as they desire.
	}
	cells = layoutDispatch( maxW, maxH, parentW, parentH, _w, _h, sizeOnly );

	if( *forceInstructionW ) {
		_w = layoutParseInstruction( forceInstructionW, parentW, parentH, requestW, requestH );
	}
	if( *forceInstructionH ) {
		_h = layoutParseInstruction( forceInstructionH, parentW, parentH, requestW, requestH );
	}

	free( forceInstructionW );
	free( forceInstructionH );

	// ENFORCE that no cells are allowed to be wider or taller than max
	if( cells ) {
		int cellI = 0;
		for( ZUI *c = headChild; c; c=c->nextSibling ) {
			if( ! c->canLayout() ) continue;
			cells[cellI].w = min( cells[cellI].w, maxW );
			cells[cellI].h = min( cells[cellI].h, maxH );
		}
	}

	_w = min( _w, maxW );
	_h = min( _h, maxH );

	// @TODO / WARNING: I realized debugging something else that requestH and requestW only get
	// set when there is some kind of "force" instruction (see above).  So panels may not scroll
	// properly unless you force their width or similar.
	float maxScroll;
	if( getI( "permitScrollY" ) ) {
		maxScroll = max( requestH-_h, _h-parentH );
		putF( "maxScrollY", maxScroll );
		if( maxScroll <= 0 ) {
			scrollY = getF( "scrollYHome" );
		}
	}
	if( getI( "permitScrollX" ) ) {
		maxScroll = max( requestW-_w, _w-parentW );
		putF( "maxScrollX", maxScroll );
		if( maxScroll <= 0 ) {
			scrollX = getF( "scrollXHome" );
		}
	}
	return cells;
}

ZUI::ZUILayoutCell *ZUI::layoutPack( float maxW, float maxH, float &_w, float &_h, int sizeOnly ) {
	// DETERMINE the side

	int resizeHeightToFitChildren=0;
	float actualMaxH = maxH;
	if( getI( "permitScrollY" ) ) {
		resizeHeightToFitChildren=1;
		maxH = 1e6f;
	}

	char side = 0;
	char *_side = getS( "pack_side" );
	if( _side ) side = toupper(_side[0]);
	if( !side ) side = 'L';

	// COUNT children, MAKE a child list, ALLOCATE cells
	int numCells = 0;
	ZUI *c;
	for( c = headChild; c; c=c->nextSibling ) {
		if( ! c->canLayout() ) continue;
		numCells++;
	}

	// ALLOCATE cells if needed
	ZUILayoutCell *cells = new ZUILayoutCell[numCells];

	float layout_padX = getF( "layout_padX" );
	float layout_padY = getF( "layout_padY" );
	int pack_fillOpposite = getI( "pack_fillOpposite" );
	int pack_fillLast = getI( "pack_fillLast" );
	int pack_wrapFlow = getI( "pack_wrapFlow" );

	int layout_sizeToMaxW = strchr( getS( "layout_sizeToMax", "" ), 'w' ) || strchr( getS( "layout_sizeToMax", "" ), 'W' ) ? 1 : 0;
	int layout_sizeToMaxH = strchr( getS( "layout_sizeToMax", "" ), 'h' ) || strchr( getS( "layout_sizeToMax", "" ), 'H' ) ? 1 : 0;

	float posX=0.f, posY=0.f;
	int lastRow=0, lastCol=0;
	float cellW=0.f, cellH=0.f;
	int row=0, col=0;
	int cellI=0;

	float maxRowH[256] = {0,};
	float maxColW[256] = {0,};
	assert( numCells < sizeof(maxRowH)/sizeof(maxRowH[0]) );
	if( side == 'L' || side == 'R' ) {
		// ACCUMULATE the width of each child, creating new rows as needed
		float remainW = maxW - layout_padX;
		row = 0; col = 0;
		for( c = headChild, cellI=0; c; c=c->nextSibling ) {
			if( ! c->canLayout() ) continue;

			// DISCOUNT the padding from the remaining width
			remainW -= layout_padX;
			
			// QUERY the child for size only given the remaining width
			c->layoutQuery( remainW, maxH-layout_padY*2.f, remainW, maxH-layout_padY*2.f, cellW, cellH );
			cells[cellI].requestW = cellW;
			cells[cellI].requestH = cellH;

			if( cellW > remainW ) {
				if( col != 0 ) {
					// We have exceeded the space available for this row so we 
					// advance to the next row unless this is the first column of this row
					remainW = maxW - layout_padX;
					row++;
					col = 0;
				}

				// The cell is asking for more than the max, so it is given the
				// chance to restate its demands given the new width.
				c->layoutQuery( remainW-layout_padX, maxH-layout_padY*2.f, remainW-layout_padX, maxH-layout_padY*2.f, cellW, cellH );
				cells[cellI].requestW = cellW;
				cells[cellI].requestH = cellH;
			}

			maxRowH[row] = max( maxRowH[row], cellH );

			cells[cellI].w = cellW;
			cells[cellI].h = 0.f;
			cells[cellI].x = (float)col;
			cells[cellI].y = (float)row;
				// x and y only represent row and col, not the actually the x position until later

			if( !sizeOnly && pack_fillLast && cellI == numCells-1 ) {
				// This is a special rule which allows the last cell in a pack
				// to simply gobble up all the remaining space.
				cells[cellI].w = remainW;
			}

			remainW -= cellW;
			col++;
			cellI++;
		}

		int numRows = row+1;
		_w = 0.f;
		_h = layout_padY;
		posX = side == 'R' ? 0.f : layout_padX;
		posY = maxH;
		for( c = headChild, cellI=0, lastRow=-1; c; c=c->nextSibling ) {
			if( ! c->canLayout() ) continue;

			int row = (int)cells[cellI].y;
			assert( row < numCells );
			if( row != lastRow ) {
				posX = side == 'R' ? maxW : layout_padX;
				// I reversed the following two ifs when I found a bug when an object was packed l,r and used fillOpposite it ended up outside of box
				if( pack_fillOpposite && row == numRows-1 && !sizeOnly ) {
					maxRowH[row] = maxH - layout_padY*2.f;
				}
				if( (side == 'R' && col != 0) || side == 'L' ) {
					posY -= maxRowH[row] + layout_padY;
				}
				_h += maxRowH[row] + layout_padY;
				lastRow = row;
			}

			// Reverse flow (R,T) have to pre-increment
			if( side == 'R' ) posX -= cells[cellI].w + layout_padX;

			cells[cellI].x = posX;
			cells[cellI].y = posY;
			cells[cellI].w = cells[cellI].w;
			cells[cellI].h = maxRowH[row];

			if( side == 'R' ) _w = max( _w, maxW - posX + layout_padX );
			else _w = max( _w, posX + cells[cellI].w + layout_padX );

			// Normal flow (L,B) have to post-increment
			if( side == 'L' ) posX += cells[cellI].w + layout_padX;
			cellI++;
		}
		//if( side == 'R' ) h -= layout_padY;
	}

	else if( side == 'T' || side == 'B' ) {
		// ACCUMULATE the width of each child, creating new rows as needed
		float remainH = maxH - layout_padY;
		row = 0; col = 0;
		for( c = headChild, cellI=0; c; c=c->nextSibling ) {
			if( ! c->canLayout() ) continue;

			// DISCOUNT the padding from the remaining height
			remainH -= layout_padY;
			
			// QUERY the child for size only given the remaining width
			c->layoutQuery( maxW-layout_padX*2.f, remainH, maxW-layout_padX*2.f, remainH, cellW, cellH );
			cells[cellI].requestW = cellW;
			cells[cellI].requestH = cellH;

			if( cellH > remainH ) {
				if( row != 0 && pack_wrapFlow ) {
					// We have exceeded the space available for this row so we 
					// advance to the next row unless this is the first column of this row
					remainH = maxH - layout_padY;
					col++;
					row = 0;
				}

				// The cell is asking for more than the max, so it is given the
				// chance to restate its demands given the new width.
				c->layoutQuery( maxW-layout_padX*2.f, remainH-layout_padY, maxW-layout_padX*2.f, remainH-layout_padY, cellW, cellH );
				cells[cellI].requestW = cellW;
				cells[cellI].requestH = cellH;
			}

			maxColW[col] = max( maxColW[col], cellW );

			cells[cellI].w = 0.f;
			cells[cellI].h = cellH;
			cells[cellI].x = (float)col;
			cells[cellI].y = (float)row;
				// x and y only represent row and col, not the actually the x position until later

			if( !sizeOnly && pack_fillLast && cellI == numCells-1 ) {
				// This is a special rule which allows the last cell in a pack
				// to simply gobble up all the remaining space.
				cells[cellI].h = remainH;
			}

			remainH -= cellH;
			row++;
			cellI++;
		}
		if( resizeHeightToFitChildren ) {
			maxH = actualMaxH;
		}


		int numCols = col+1;
		_w = layout_padX;
		_h = 0.f;
		posY = 0.f;
		posX = layout_padX;
		for( c = headChild, cellI=0, lastCol=-1; c; c=c->nextSibling ) {
			if( ! c->canLayout() ) continue;

			int col = (int)cells[cellI].x;
			assert( col < numCells );
			if( col != lastCol ) {
				posY = side == 'T' ? maxH : layout_padY;
				// I reversed the following two ifs when I found a bug when an object was packed l,r and used fillOpposite it ended up outside of box
				if( pack_fillOpposite && col == numCols-1 && !sizeOnly ) {
					maxColW[col] = maxW - layout_padX*2.f;
				}
				if( col != 0 ) {
					posX += maxColW[row] + layout_padX;
				}
				_w += maxColW[col] + layout_padX;
				lastCol = col;
			}

			// Reverse flow (R,T) have to pre-increment
			if( side == 'T' ) posY -= cells[cellI].h + layout_padY;

			cells[cellI].x = posX;
			cells[cellI].y = posY;
			cells[cellI].h = cells[cellI].h;
			cells[cellI].w = maxColW[col];

			if( side == 'T' ) _h = max( _h, maxH - posY + layout_padY );
			else _h = max( _h, posY + cells[cellI].h + layout_padY );

			// Normal flow (L,B) have to post-increment
			if( side == 'B' ) posY += cells[cellI].h + layout_padY;
			cellI++;
		}
	}

	if( layout_sizeToMaxW ) {
		_w = maxW;
	}
	if( layout_sizeToMaxH ) {
		_h = maxH;
	}

	if( sizeOnly ) {
		delete cells;
		cells = 0;
	}

	return cells;
}

ZUI::ZUILayoutCell *ZUI::layoutBorder( float maxW, float maxH, float &width, float &height, int sizeOnly ) {
	
	// COUNT children, MAKE a child list, ALLOCATE cells
	int numCells = 0;
	ZUI *child;
	for( child = headChild; child; child=child->nextSibling ) {
		if( ! child->canLayout() ) continue;
		numCells++;
	}

	// ALLOCATE cells if needed
	ZUILayoutCell *cells = new ZUILayoutCell[numCells];

	int layout_sizeToMaxW = strchr( getS( "layout_sizeToMax", "" ), 'w' ) || strchr( getS( "layout_sizeToMax", "" ), 'W' ) ? 1 : 0;
	int layout_sizeToMaxH = strchr( getS( "layout_sizeToMax", "" ), 'h' ) || strchr( getS( "layout_sizeToMax", "" ), 'H' ) ? 1 : 0;
	float layout_padX = getF( "layout_padX" );
	float layout_padY = getF( "layout_padY" );

	float colW[3] = { 0, };
	float rowH[3] = { 0, };
		// These are the widths and heights respectively of the three cols and rows

	static char *sides[] = { "n", "s", "nw", "ne", "sw", "se", "w", "e", "c" };
	enum sideEnum { N, S, NW, NE, SW, SE, W, E, C };
	const int numSides = 9;

	float extents[9][2] = {0,};
	float quadExtents[9][2] = {0,};
	width = 0.f, height = 0.f;

	// PASS 1, accumulate requested extents
	int side;
	for( side=0; side<numSides; side++ ) {
		int cellI = 0;
		for( child = headChild; child; child=child->nextSibling ) {
			if( ! child->canLayout() ) continue;

			float cellW, cellH;
			char *border_side = child->getS( "border_side", 0 );
			if( !border_side ) {
				border_side = "n";
			}
			float _w, _h;
			if( !strcmp(sides[side],border_side) ) {
				switch( side ) {
					case N:
						assert( ! extents[NW][0] && ! extents[NE][0] && ! extents[E][0] && ! extents[W][0] );
						_w = maxW-layout_padX*2.f;
						_h = maxH-quadExtents[N][1]-quadExtents[S][1]-layout_padY;
						child->layoutQuery( _w, _h, _w, _h, cellW, cellH );
						cells[cellI].requestW = cellW;
						cells[cellI].requestH = cellH;
						cellW += layout_padX*2.f;
						cellH += layout_padY;
						extents[N][1] += cellH;
						extents[N][0] = max( extents[N][0], cellW );
						quadExtents[N][1] += cellH;
						quadExtents[N][0] = max( quadExtents[N][0], cellW );
						break;
					case NW:
						assert( ! extents[N][0] && ! extents[W][0] );
						_w = maxW-extents[NW][0]-extents[NE][0]-layout_padX;
						_h = maxH-quadExtents[N][1]-quadExtents[S][1]-layout_padY;
						child->layoutQuery( _w, _h, _w, _h, cellW, cellH );
						cells[cellI].requestW = cellW;
						cells[cellI].requestH = cellH;
						cellW += layout_padX;
						cellH += layout_padY;
						extents[NW][0] += cellW;
						extents[NW][1] = max( extents[NW][1], cellH );
						quadExtents[N][1] = max( quadExtents[N][1], cellH );
						break;
					case NE:
						assert( ! extents[N][0] && ! extents[E][0] );
						_w = maxW-extents[NW][0]-extents[NE][0]-layout_padX;
						_h = maxH-quadExtents[N][1]-quadExtents[S][1]-layout_padY;
						child->layoutQuery( _w, _h, _w, _h, cellW, cellH );
						cells[cellI].requestW = cellW;
						cells[cellI].requestH = cellH;
						cellW += layout_padX;
						cellH += layout_padY;
						extents[NE][0] += cellW;
						extents[NE][1] = max( extents[NE][1], cellH );
						quadExtents[N][1] = max( quadExtents[N][1], cellH );
						break;
					case S:
						assert( ! extents[SW][0] && ! extents[SE][0] && ! extents[E][0] && ! extents[W][0] );
						_w = maxW - layout_padX*2.f;
						_h = maxH-quadExtents[N][1]-quadExtents[S][1]-layout_padY;
						child->layoutQuery( _w, _h, _w, _h, cellW, cellH );
						cells[cellI].requestW = cellW;
						cells[cellI].requestH = cellH;
						cellW += layout_padX*2.f;
						cellH += layout_padY;
						extents[S][1] += cellH;
						extents[S][0] = max( extents[S][0], cellW );
						quadExtents[S][1] += cellH;
						quadExtents[S][0] = max( quadExtents[S][0], cellW );
						break;
					case SW:
						assert( ! extents[S][0] && ! extents[W][0] );
						_w = maxW-extents[SW][0]-extents[SE][0]-layout_padX;
						_h = maxH-quadExtents[N][1]-quadExtents[S][1]-layout_padY;
						child->layoutQuery( _w, _h, _w, _h, cellW, cellH );
						cells[cellI].requestW = cellW;
						cells[cellI].requestH = cellH;
						cellW += layout_padX;
						cellH += layout_padY;
						extents[SW][0] += cellW;
						extents[SW][1] = max( extents[SW][1], cellH );
						quadExtents[S][1] = max( quadExtents[S][1], cellH );
						break;
					case SE:
						assert( ! extents[S][0] && ! extents[E][0] );
						_w = maxW-extents[SW][0]-extents[SE][0]-layout_padX;
						_h = maxH-quadExtents[N][1]-quadExtents[S][1]-layout_padY;
						child->layoutQuery( _w, _h, _w, _h, cellW, cellH );
						cells[cellI].requestW = cellW;
						cells[cellI].requestH = cellH;
						cellW += layout_padX;
						cellH += layout_padY;
						extents[SE][0] += cellW;
						extents[SE][1] = max( extents[SE][1], cellH );
						quadExtents[S][1] = max( quadExtents[S][1], cellH );
						break;
					case W:
						assert( ! extents[C][0] );
						_w = maxW-extents[W][0]-extents[E][0]-layout_padX;
						_h = maxH-quadExtents[N][1]-quadExtents[S][1]-/*quadExtents[c][1]-*/layout_padY;
							// I removed the quadExtents[c] subtraction when I realized tht if I created
							// an east and west simultaneously it failed.  Same below
						child->layoutQuery( _w, _h, _w, _h, cellW, cellH );
						cells[cellI].requestW = cellW;
						cells[cellI].requestH = cellH;
						cellW += layout_padX;
						cellH += layout_padY;
						extents[W][0] += cellW;
						extents[W][1] = max( extents[W][1], cellH );
						quadExtents[C][0] += cellW;
						quadExtents[C][1] = max( quadExtents[C][1], cellH );
						break;
					case E:
						assert( ! extents[C][0] );
						_w = maxW-extents[W][0]-extents[E][0]-layout_padX;
						_h = maxH-quadExtents[N][1]-quadExtents[S][1]-/*quadExtents[c][1]-*/layout_padY;
						child->layoutQuery( _w, _h, _w, _h, cellW, cellH );
						cells[cellI].requestW = cellW;
						cells[cellI].requestH = cellH;
						cellW += layout_padX;
						cellH += layout_padY;
						extents[E][0] += cellW;
						extents[E][1] = max( extents[E][1], cellH );
						quadExtents[C][0] += cellW;
						quadExtents[C][1] = max( quadExtents[C][1], cellH );
						break;
					case C:
						assert( ! extents[W][0] && ! extents[E][0] );
						_w = maxW-layout_padX*2.f;
						_h = maxH-quadExtents[N][1]-quadExtents[S][1]-/*quadExtents[c][1]-*/layout_padY;
						child->layoutQuery( _w, _h, _w, _h, cellW, cellH );
						cells[cellI].requestW = cellW;
						cells[cellI].requestH = cellH;
						cellW += layout_padX*2.f;
						cellH += layout_padY;
						extents[C][0] = max( extents[C][0], cellW );
						extents[C][1] += cellH;
						quadExtents[C][0] = max( quadExtents[C][0], cellW );
						quadExtents[C][1] += cellH;
						break;
				}
			}
			cellI++;
		}
	}

	// PASS 2, position the cells
	width = 0.f;
	width = max( width, extents[N][0] );
	width = max( width, extents[S][0] );
	width = max( width, extents[NW][0] + extents[NE][0] );
	width = max( width, extents[SW][0] + extents[SE][0] );
	width = max( width, extents[W][0] + extents[E][0] );
	width = max( width, extents[C][0] );

	height = quadExtents[N][1] + quadExtents[S][1] + quadExtents[C][1] + layout_padY;

	if( layout_sizeToMaxW ) {
		width  = maxW;
	}
	if( layout_sizeToMaxH ) {
		height = maxH;
	}

	for( side=0; side<numSides; side++ ) {
		float accumW = 0.f, accumH = 0.f;
		int cellI = 0;
		for( child = headChild; child; child=child->nextSibling ) {
			if( ! child->canLayout() ) continue;

			char *border_side = child->getS( "border_side", 0 );
			if( !border_side ) border_side = "n";
			if( !strcmp(sides[side],border_side) ) {
				switch( side ) {
					case N:
						cells[cellI].w = width - layout_padX*2.f;
						cells[cellI].h = cells[cellI].requestH;
						cells[cellI].x = layout_padX;
						cells[cellI].y = maxH - accumH - cells[cellI].requestH - layout_padY;
						accumH += cells[cellI].requestH + layout_padY;
						break;
					case NW:
						cells[cellI].w = cells[cellI].requestW;
						cells[cellI].h = cells[cellI].requestH;
						cells[cellI].x = accumW + layout_padX;
						cells[cellI].y = maxH - cells[cellI].requestH - layout_padY;
						accumW += cells[cellI].requestW + layout_padX;
						break;
					case NE:
						cells[cellI].w = cells[cellI].requestW;
						cells[cellI].h = cells[cellI].requestH;
						cells[cellI].x = maxW - accumW - cells[cellI].requestW - layout_padX;
						cells[cellI].y = maxH - cells[cellI].requestH - layout_padY;
						accumW += cells[cellI].requestW + layout_padX;
						break;
					case S:
						cells[cellI].w = width - layout_padX*2.f;
						cells[cellI].h = cells[cellI].requestH;
						cells[cellI].x = layout_padX;
						cells[cellI].y = accumH + layout_padY;
						accumH += cells[cellI].requestH + layout_padY;
						break;
					case SW:
						cells[cellI].w = cells[cellI].requestW;
						cells[cellI].h = cells[cellI].requestH;
						cells[cellI].x = accumW + layout_padX;
						cells[cellI].y = layout_padY;
						accumW += cells[cellI].requestW + layout_padX;
						break;
					case SE:
						cells[cellI].w = cells[cellI].requestW;
						cells[cellI].h = cells[cellI].requestH;
						cells[cellI].x = maxW - accumW - cells[cellI].requestW - layout_padX;
						cells[cellI].y = layout_padY;
						accumW += cells[cellI].requestW + layout_padX;
						break;
					case W:
						cells[cellI].w = cells[cellI].requestW;
						cells[cellI].h = cells[cellI].requestH;
						cells[cellI].x = accumW + layout_padX;
						cells[cellI].y = maxH - quadExtents[N][1] - quadExtents[C][1] - layout_padY;
						accumW += cells[cellI].requestW + layout_padX;
						break;
					case E:
						cells[cellI].w = cells[cellI].requestW;
						cells[cellI].h = cells[cellI].requestH;
						cells[cellI].x = maxW - accumW - cells[cellI].requestW - layout_padX;
						cells[cellI].y = maxH - quadExtents[N][1] - quadExtents[C][1] - layout_padY;
						accumW += cells[cellI].requestW + layout_padX;
						break;
					case C:
						cells[cellI].w = width - layout_padX*2.f;
						cells[cellI].h = cells[cellI].requestH;
						cells[cellI].x = layout_padX;
						cells[cellI].y = maxH - quadExtents[N][1] - accumH - cells[cellI].requestH - layout_padY;
						accumH += cells[cellI].requestH + layout_padY;
						break;
				}
			}
			cellI++;
		}
	}

	if( sizeOnly ) {
		delete cells;
		cells = 0;
	}

	return cells;
}

ZUI::ZUILayoutCell *ZUI::layoutTable( float maxW, float maxH, float &width, float &height, int sizeOnly ) {
	//  table_cols
	//  table_rows
	//  table_colWeightN (where n is 1-numCols) = NUMBER
	//  table_colDistributeEvenly = [0] | 1
	//  table_rowDistributeEvenly = [0] | 1
	//  table_calcHFromW = (child option) scalar: multiply derived w by scalar to get h

	// COUNT children, MAKE a child list, ALLOCATE cells
	int numCells = 0;
	ZUI *child;
	for( child = headChild; child; child=child->nextSibling ) {
		if( ! child->canLayout() ) continue;
		numCells++;
	}

	// ALLOCATE cells if needed
	ZUILayoutCell *cells = new ZUILayoutCell[numCells];

	int layout_sizeToMaxW = strchr( getS( "layout_sizeToMax", "" ), 'w' ) || strchr( getS( "layout_sizeToMax", "" ), 'W' ) ? 1 : 0;
	int layout_sizeToMaxH = strchr( getS( "layout_sizeToMax", "" ), 'h' ) || strchr( getS( "layout_sizeToMax", "" ), 'H' ) ? 1 : 0;
	float layout_padX = getF( "layout_padX" );
	float layout_padY = getF( "layout_padY" );

	float *colW = (float *)alloca( sizeof(float) * numCells );
	float *rowH = (float *)alloca( sizeof(float) * numCells );
	memset( colW, 0, sizeof(float) * numCells );
	memset( rowH, 0, sizeof(float) * numCells );
		// These are the widths and heights respectively of the rows and cols
		// Allocate the maximum possible would be all children in one row or one column

	// DETERMINE number of rows and cols
	int cols = getI( "table_cols" );
	int rows = getI( "table_rows" );
	if( !rows ) {
		assert( cols );
		rows = (numCells/cols) + ((numCells%cols)!=0?1:0);
	}
	if( !cols ) {
		assert( rows );
		cols = (numCells/rows) + ((numCells%rows)!=0?1:0);
	}

	// PASS 1, accumulate requested extents
	int i = 0;
	int cellI = 0;
	for( child = headChild; child; child=child->nextSibling ) {
		if( ! child->canLayout() ) continue;

		float cellW, cellH;
		float _w, _h;

		_w = maxW - layout_padX*2.f;
		_h = maxH - layout_padY*2.f;
		child->layoutQuery( _w, _h, _w, _h, cellW, cellH );
		cells[cellI].requestW = cellW;
		cells[cellI].requestH = cellH;
		cells[cellI].w = cellW + layout_padX;
		cells[cellI].h = cellH + layout_padY;
		cellI++;
	}

	// FIND the widest and the tallest in each column and row
	float *colSizes = (float *)alloca( sizeof(float) * cols );
	memset( colSizes, 0, sizeof(float)*cols );
	float *rowSizes = (float *)alloca( sizeof(float) * rows );
	memset( rowSizes, 0, sizeof(float)*rows );
	cellI = 0;
	for( child = headChild; child; child=child->nextSibling ) {
		if( ! child->canLayout() ) continue;
		colSizes[cellI%cols] = max( colSizes[cellI%cols], cells[cellI].w );
		rowSizes[cellI/cols] = max( rowSizes[cellI/cols], cells[cellI].h );
		cellI++;
	}

	// The requested totals could exceed the available space in which case we have to
	// assign the col widths as best we can and then ask each element again
	// to see if they want to resize their heights based on this given width

	// DISTRIBUTE the columns in one of three manners: 
	// by maximum (default), evenly (option), or by weight (option)
	if( getI("table_colDistributeEvenly") ) {
		// DISTRIBUTE evenly
		for( i=0; i<cols; i++ ) {
			colSizes[i] = (maxW-(cols+1)*layout_padX) / cols;
		}
	}
	else if( getI("table_colWeight1") ) {
		// DISTRIBUTE by weight
		float sum = 0.f;
		for( i=0; i<cols; i++ ) {
			char buf[128];
			sprintf( buf, "table_colWeight%d", i+1 );
			sum += getF( buf );
		}
		for( i=0; i<cols; i++ ) {
			char buf[128];
			sprintf( buf, "table_colWeight%d", i+1 );
			colSizes[i] = (maxW-(cols+1)*layout_padX) * getF( buf ) / sum;
		}
	}
	else {
		// DISTRIBUTE by maximum request
		float totalW = 0.f;
		for( i=0; i<cols; i++ ) {
			totalW += colSizes[i];
		}
		if( totalW > 0 && totalW > maxW-layout_padX ) {
			// The total requested is more than is available and
			// there are not colWeights specified so we have to make
			// it up based on the reqests
			for( i=0; i<cols; i++ ) {
				colSizes[i] = (maxW-(cols+1)*layout_padX) * colSizes[i] / totalW;
			}
		}
	}

	// DERIVE cell heights for special table_calcHFromW option
	cellI=0;
	for( child = headChild; child; child=child->nextSibling ) {
		if( ! child->canLayout() ) continue;
		if( child->has("table_calcHFromW") ) {
			cells[cellI].h = child->getF("table_calcHFromW") * colSizes[cellI%cols];
			rowSizes[cellI/cols] = max( rowSizes[cellI/cols], cells[cellI].h );
			cellI++;
		}
	}

	// DISTRIBUTE rows also according to two options
	if( getI( "table_rowDistributeEvenly" ) ) {
		// DISTRIBUTE evenly
		for( i=0; i<rows; i++ ) {
			rowSizes[i] = (maxH-(rows+1)*layout_padY) / rows;
		}
	}
	else {
		// The col sizes could have changed since the query so now we
		// have to ask all of the children again what they want for height
		// on the now established col widths

		int cellI = 0;
		for( child = headChild; child; child=child->nextSibling ) {
			if( ! child->canLayout() ) continue;

			float cellW, cellH;
			float _w, _h;

			_w = colSizes[cellI%cols];
			_h = maxH - layout_padY*2.f;
			child->layoutQuery( _w, _h, _w, _h, cellW, cellH );
			cells[cellI].requestW = cellW;
			cells[cellI].requestH = cellH;
			cells[cellI].w = cellW;
			cells[cellI].h = cellH;
			cellI++;
		}
		cellI = 0;
		for( child = headChild; child; child=child->nextSibling ) {
			if( ! child->canLayout() ) continue;
			rowSizes[cellI/cols] = max( rowSizes[cellI/cols], cells[cellI].h );
			cellI++;
		}
	}

	// DETERMINE the total width and height
	width = layout_padX*(cols+1);
	for( i=0; i<cols; i++ ) {
		width += colSizes[i];
	}
	height = layout_padY*(rows+1);
	for( i=0; i<rows; i++ ) {
		height += rowSizes[i];
	}

	// PASS 2, position the cells
	float *colPos = (float *)alloca( sizeof(float) * cols );
	float *rowPos = (float *)alloca( sizeof(float) * rows );

	float y = maxH - rowSizes[0] - layout_padY;
	for( i=0; i<rows; i++ ) {
		rowPos[i] = y;
		y -= rowSizes[i+1] + layout_padY;
	}
	float x = layout_padX;
	for( i=0; i<cols; i++ ) {
		colPos[i] = x;
		x += colSizes[i] + layout_padX;
	}

	cellI = 0;
	for( child = headChild; child; child=child->nextSibling ) {
		if( ! child->canLayout() ) continue;
		cells[cellI].w = colSizes[cellI%cols];
		cells[cellI].h = rowSizes[cellI/cols];
		cells[cellI].x = colPos[cellI%cols];
		cells[cellI].y = rowPos[cellI/cols];
		cellI++;
	}

	if( sizeOnly ) {
		delete cells;
		cells = 0;
	}

	return cells;
}

void ZUI::layout( float maxW, float maxH ) {
//	zprofBeg( layout );

	// This function is the public interface to layout. It performs a multi-pass
	// layout.  First it calls layoutQuery which ends up returning a single
	// list of cells for this ZUI.  Each of these cells is filled and aligned
	// according to the child's wishes and then it recursively calls layout
	// for each of these children.

	// DEBUG trapping	
	if( !strcmp(name,"zuitestContainer") ) {
		int a = 1;
	}

	float indentL = getF( "layout_indentL" );
	float indentT = getF( "layout_indentT" );
	float indentR = getF( "layout_indentR" );
	float indentB = getF( "layout_indentB" );

	maxW -= indentL + indentR;
	maxH -= indentT + indentB;
	
	float parentW = parent ? parent->w : w;
	float parentH = parent ? parent->h : h;

	float _w, _h;
	ZUILayoutCell *cells = layoutQuery( maxW, maxH, parentW, parentH, _w, _h, 0 );
		// The final argument here indicates that we want the cells back.
		// I.e. this is not a test

	// PLACE children into cells according to their fill and align wishes
	int i = 0;
	for( ZUI *c=headChild; c; c=c->nextSibling ) {
		float cellW, cellH;
		if( c->layoutManual ) {
			float _x, _y;
			cellW = ZUI::layoutParseInstruction( c->getS("layoutManual_w"), maxW, maxH, maxW, maxH );
			cellH = ZUI::layoutParseInstruction( c->getS("layoutManual_h"), maxW, maxH, maxW, maxH );
			_x = ZUI::layoutParseInstruction( c->getS("layoutManual_x"), maxW, maxH, cellW, cellH );
			_y = ZUI::layoutParseInstruction( c->getS("layoutManual_y"), maxW, maxH, cellW, cellH );
			c->x = _x;
			c->y = _y;
			c->resize( cellW, cellH, 0 );
			c->layout( cellW, cellH );
		}
		else if( c->canLayout() ) {
			assert( cells );
			cellW = cells[i].requestW;
			cellH = cells[i].requestH;

			// FILL the child into the cell if requested
			char *fillOpt = c->getS( "layout_cellFill", "" );
			if( *fillOpt ) {
				for( char *s = fillOpt; s && *s; *s++=tolower(*s) );
				if( strchr( fillOpt, 'w' ) ) {
					cellW = cells[i].w;
				}
				if( strchr( fillOpt, 'h' ) ) {
					cellH = cells[i].h;
				}
			}

			cellW = min( cells[i].w, cellW );
			if( !c->parent || !c->parent->getI( "permitScrollY" ) ) {
				// @TODO: the whole scrolling thing needs cleaning up when ZUI is rebuilt.
				// This little hack is so that the child of a scrolling panel will display 
				// properly when the window height is too small for the panel to fit in.
				// To see this behavior, cause the following line to always execute, and look
				// at KinExp, making the window very short, and note the Model Editor Panel
				// does not layout correctly -- because it is drawn at negative offset, but
				// it's height has been also reduced, so a gap appears above the panel.
				cellH = min( cells[i].h, cellH );
			}
			
			c->resize( cellW, cellH, 0 );

			// RECURSIVE layout of this child's children
			c->layout( cellW, cellH );

			// ALIGN the child into the cell if requested
			char *alignOpt = c->getS( "layout_cellAlign", "" );
			float x = 0.f, y = cells[i].h - min( cellH, cells[i].h );
			
			if( *alignOpt ) {
				for( char *u = alignOpt; u && *u; *u++=tolower(*u) );
					 if( !strcmp(alignOpt,"n" ) ) { x = (cells[i].w - cellW) / 2.f; y = cells[i].h - cellH; }
				else if( !strcmp(alignOpt,"nw") ) { x = 0.f; y = cells[i].h - cellH; }
				else if( !strcmp(alignOpt,"w" ) ) { x = 0.f; y = (cells[i].h - cellH) / 2.f; }
				else if( !strcmp(alignOpt,"sw") ) { x = 0.f; y = 0.f; }
				else if( !strcmp(alignOpt,"s" ) ) { x = (cells[i].w - cellW) / 2.f; y = 0.f; }
				else if( !strcmp(alignOpt,"se") ) { x = cells[i].w - cellW; y = 0.f; }
				else if( !strcmp(alignOpt,"e" ) ) { x = cells[i].w - cellW; y = (cells[i].h - cellH) / 2.f; }
				else if( !strcmp(alignOpt,"ne") ) { x = cells[i].w - cellW; y = cells[i].h - cellH; }
				else if( !strcmp(alignOpt,"c" ) ) { x = (cells[i].w - cellW) / 2.f; y = (cells[i].h - cellH) / 2.f; }
			}
			c->move( indentL + cells[i].x + x, indentB + cells[i].y + y );
			i++;
		}
		c->goalX = c->x;
		c->goalY = c->y;
		c->dirty();
	}

	free( cells );
	dirty();
	
//	zprofEnd();
}

// UPDATE
//--------------------

ZHashTable ZUI::zuiWantUpdates;
int ZUI::zuiFrameCounter = 0;
double ZUI::zuiTime = 0;
double ZUI::zuiLastTime = 0;
int ZUI::zuiLayoutRequest = 0;
int ZUI::postLayoutRender = 0;

void ZUI::setUpdate( int flag ) {
	putI( "update", flag );
	ZUI* t = (ZUI*)this;
	ZUI::zuiWantUpdates.bputI( (char*)&t, sizeof(ZUI*), flag );
}

void ZUI::zuiUpdate( double time ) {
	zuiLastTime = zuiTime;
	zuiTime = time;
	zuiFrameCounter++;

//	zprofBeg( zui_garbage_collect );
	zuiGarbageCollect();
//	zprofEnd();

	for( int i=0; i<ZUI::zuiByNameHash.size(); i++ ) {
		ZUI *val = (ZUI *)ZUI::zuiByNameHash.getValp(i);
		if( val ) {
			if( val->styleUpdateFlag ) {
				val->styleUpdate();
				val->propertiesInherit();
				val->styleUpdateFlag = 0;
			}
		}
	}

//	zprofBeg( zui_update_indv );
	for( ZHashWalk walk(zuiWantUpdates); walk.next(); ) {
		int *v = (int*)walk.val;
		ZUI **o = (ZUI**)walk.key;
		if( o && v && *v ) {
			assert( ! (*o)->getI("dead") );
			if( !(*o)->hidden || (*o)->getI("updateWhenHidden") ) {
				(*o)->update();
			}
		}
	}
//	zprofEnd();

	if( zuiLayoutRequest ) {
//		zprofBeg( zui_process_layout_request );

		ZUI *root = (ZUI *)zuiByNameHash.getp( "root" );
		if( root ) {
			root->w = rootW;
			root->h = rootH;

//zprofReset(0);
			root->layout( rootW, rootH );
//zprofDumpToFile( "layoutprof.txt", "layout" );
		}
		
		//trace( "[%05d] ZUILayout (%d requests)\n", zuiFrameCounter, zuiLayoutRequest );
		zuiLayoutRequest = 0;
		lastLayoutFrame = zuiFrameCounter;

		postLayoutRender = 1;
			// tells the renderer that a full layout has just occured.  The renderer
			// will ignore dirty rects for this frame, and do a single non-scissored render.
			// This ensures that any resource hits or additional triggered layouts 
			// occur synchronously rather than being delayed until some UI becomes
			// non-clipped.  (tfb - set the above to 0 and note the "chug" when scrolling
			// the UI panel of kintek after the app has initially started.  This can
			// probably go away once we solve the recursive layout problem).

//		zprofEnd();
	}
}

// MESSAGES
//--------------------

float ZUI::rootW = 0.f;
float ZUI::rootH = 0.f;

void ZUI::zuiReshape( float w, float h ) {
	::zMsgQueue( "type=ZUIReshape oldW=%f oldH=%f newW=%%f newH=%f", rootW, rootH, w, h );
	rootW = w;
	rootH = h;
	layoutRequest();
	dirtyAll();
}

void ZUI::recurseVisibilityNotify( int state ) {
	ZMsg *notifyShow = zHashTable( "type=ZUINotifyShow" );
	ZMsg *notifyHide = zHashTable( "type=ZUINotifyHide" );
	for( ZUI *o=headChild; o; o=o->nextSibling ) {
		if( !o->hidden ) {
			o->handleMsg( state ? notifyShow : notifyHide );
				// potential problem: you may get multiple notifyShow messages
				// in a single frame, and will get them even if already visible.
			o->recurseVisibilityNotify( state );
		}
	}
	delete notifyShow;
	delete notifyHide;
}

void ZUI::handleMsg( ZMsg *msg ) {
	if( zmsgIs( type, ZUINotifyHiddenChanged ) ) {
		// DETERMINE actual visibility 
		int parentVis = 1;
		for( ZUI *p=parent; p && parentVis; p=p->parent ) {
			parentVis = parentVis && !p->hidden;
		}
		if( parentVis ) {
			recurseVisibilityNotify( !hidden );
		}
	}
	else if( zmsgIs(type,ZUISet) ) {
		char *key  = zmsgS(key);
		if( !key || !*key ) return;
		int hasToggle = zmsgHas(toggle);
		int hasDelta = zmsgHas(delta);
		int hasScale = zmsgHas(scale);
		char *val = zmsgS(val);
		char *end;
		double valD = 0.0;
		
		unsigned int intVal = strtoul( val, &end, 0 );
		valD = (double)intVal;
		if( *end != 0 ) {
			// It was not an int
			valD = strtod( val, &end );
		}

		if( end != val || hasToggle || hasDelta || hasScale ) {
			// This is a number
			double oldVal = getD( key );
			double newVal = valD;

			if( hasToggle ) {
				newVal = (double) ! (int)oldVal;
			}
			else if( hasDelta ) {
				newVal = oldVal + zmsgD(delta);
			}
			else if( hasScale ) {
				newVal = oldVal * zmsgD(delta);
			}

			if( zmsgHas(max) ) {
				newVal = min( zmsgD(max), newVal );
			}
			if( zmsgHas(min) ) {
				newVal = max( zmsgD(min), newVal );
			}

			putD( key, newVal );
		}
		else {
			putS( key, val );
		}
	}
	else if( zmsgIs(type,ZUISetI) ) {
		char *key  = zmsgS(key);
		if( !key || !*key ) return;
		putI( key, zmsgI(val) );
	}
	else if( zmsgIs(type,ZUIHide) ) {
		if( !(zmsgHas(toZUI) || zmsgHas(toZUIGroup)) ) {
			return;
		}
		hidden = 1;
		checkNotifyMsg( "hidden" );
			// note that putI( "hidden", 1 ) will accomplish both lines above
		layoutRequest();
	}
	else if( zmsgIs(type,ZUIShow) ) {
		if( !(zmsgHas(toZUI) || zmsgHas(toZUIGroup)) ) {
			return;
		}
		hidden = 0;
		checkNotifyMsg( "hidden" );
			// note that putI( "hidden", 0 ) will accomplish both lines above
		if( zmsgI( modal ) ) {
			assert( zmsgHas(toZUI) );
			modal( 1 );
				// is it problematic to go modal in the middle of message processing?
				// this effectively happened before in message handlers; now I'm adding
				// this functionality here.  15 feb 2008 tfb
		}
		layoutRequest();
	}
	else if( zmsgIs(type,ZUIMoveNear) ) {
		moveNear( zmsgS(who), zmsgS(where), zmsgF(x), zmsgF(y) );
	}
	else if( zmsgIs(type,ZUIDie) ) {
		die();
	}
	else if( zmsgIs(type,ZUIDirty) ) {
		dirty();
	}
	else if( zmsgIs(type,ZUIDirtyAll) ) {
		dirtyAll();
	}
	else if( zmsgIs(type, ZUIStyleChanged ) ) {
			// new 2009 may 22 (tfb) - support dynamic style changes (e.g. fonts)
		char* stylename=0;
		if( zmsgHas( which ) ) {
			stylename = zmsgS( which );
		}
		else if( zmsgHas( toZUI ) ) {
			stylename = getS( "style", 0 );
		}
		else {
			assert( !"must provide either a stylename or a toZUI for ZUIStyleChanged!" );
		}
		if( !strcmp( getS( "style", "" ), stylename ) ) {
			//trace( "-------------------- style %s changed for zui %s", stylename, name );
			ZUI *z = ZUI::zuiFindByName( stylename );
			if( z ) {
				ZRegExp setStyles( "font|color|Color" );
				for( ZHashWalk w(*z); w.next(); ) {
					if( setStyles.test( w.key ) ) {
						char *val = z->getS( w.key );

						putS( w.key, val );
						//trace( "set style key %s to value %s\n", w.key, val );
					}
				}
			}
		}
	}
}

ZMSG_HANDLER( ZUIDumpAtMouse ) {
	ZUI *root = ZUI::zuiFindByName( "root" );
	if( root ) {
		ZUI *found = root->findByCoord( (float)zMouseMsgX, (float)zMouseMsgY );
		if( found ) {
			found->putD( "debugFlash", ZUI::zuiTime );
			char *string = found->dumpToString();
			char *sendTo = zmsgS(sendTo);
			ZUI *sendToZUI = ZUI::zuiFindByName( sendTo );
			if( sendToZUI ) {
				sendToZUI->putS( "text", string );
			}
			delete string;
		}
	}
}

// NOTIFICATIONS
//--------------------

void ZUI::checkNotifyMsg( char *key ) {
	char keyBuf[256];
	strcpy( keyBuf, "notify-" );
	strcat( keyBuf, key );
	char *sendNotify = getS( keyBuf, 0 );
	if( sendNotify ) {
		zMsgQueue( sendNotify );
	}
}

void ZUI::appendNotifyMsg( char *key, char *msg ) {
	char keyBuf[256];
	strcpy( keyBuf, "notify-" );
	strcat( keyBuf, key );

	char *sendNotify = getS( keyBuf );
	char *buffer = (char *)alloca( strlen(sendNotify) + strlen(msg) + 2 );
	buffer[0] = 0;
	strcat( buffer, sendNotify );
	if( ! ( sendNotify[strlen(sendNotify)-1] == ';' || msg[0]==';' ) ) {
		strcat( buffer, ";" );
	}
	strcat( buffer, msg );

	putS( keyBuf, buffer );
}

void ZUI::setNotifyMsg( char *key, char *msg ) {

	char keyBuf[256];
	strcpy( keyBuf, "notify-" );
	strcat( keyBuf, key );

	putS( keyBuf, msg );
}

char *ZUI::getNotifyMsg( char *key ) {
	char keyBuf[256];
	strcpy( keyBuf, "notify-" );
	strcat( keyBuf, key );

	return getS( keyBuf );
}

// KEYBOARD AND MOUSE
//--------------------

ZHashTable ZUI::globalKeyBindings;
void ZUI::zuiBindKey( char *key, char *sendMsg ) {
	globalKeyBindings.putS( key, strdup(sendMsg) );
}

ZMSG_HANDLER( ZUIBindKey ) {
	ZUI::zuiBindKey( zmsgS(key), zmsgS(sendMsg) );
}

int ZUI::requestExclusiveMouse( int anywhere, int onOrOff ) {
	if( onOrOff ) {
		char buffer[128];
		sprintf( buffer, "type=ZUIExclusiveMouseDrag toZUI='%s'", name );
		int gotExclusive = zMouseMsgRequestExclusiveDrag( buffer );
		putI( "mouseExclusiveDrag", gotExclusive );
	}
	else {
		if( getI( "mouseExclusiveDrag" ) ) {
			zMouseMsgCancelExclusiveDrag();
		}
	}
	return 1;
}

void ZUI::getMouseLocalXY( float &x, float &y ) {
	x = (float)zMouseMsgX;
	y = (float)zMouseMsgY;
	getLocalXY( x, y );
}

struct ZUIModalKeyBinding {
	ZUI *o;
	ZUIModalKeyBinding *next;
	ZUIModalKeyBinding *prev;
};
static ZUIModalKeyBinding *headModalKeyBinding = 0;

void ZUI::modalKeysClear() {
	ZUIModalKeyBinding *next;
	for( ZUIModalKeyBinding *o = headModalKeyBinding; o; o=next ) {
		next = o->next;
		delete o;
	}
	headModalKeyBinding = 0;
	::zMsgQueue( "type=ZUIModalKeysClear" );
}

void ZUI::modalKeysBind( ZUI *o ) {
	ZUIModalKeyBinding *b = new ZUIModalKeyBinding;
	b->o = o;
	b->next = headModalKeyBinding;
	b->prev = 0;
	if( headModalKeyBinding ) {
		headModalKeyBinding->prev = b;
	}
	headModalKeyBinding = b;
}

void ZUI::modalKeysUnbind( ZUI *o ) {
	for( ZUIModalKeyBinding *b = headModalKeyBinding; b; b=b->next ) {
		if( b->o == o ) {
			if( b->next ) {
				b->next->prev = b->prev;
			}
			if( b->prev ) {
				b->prev->next = b->next;
			}
			if( headModalKeyBinding == b ) {
				headModalKeyBinding = b->next;
			}
			delete b;
			break;
		}
	}
}

// There's a bug in here which is that it is sending mouse ZUIMouseReleaseDrag
// to everyone when it should only send to whoever has it locked.
// Press the top button and the up event gets eaten by the text
void ZUI::dispatch( ZMsg *msg ) {
	if( zmsgI(__used__) ) {
		return;
	}
	if( ZUI::focusStackTop > 0 && focusStack[ZUI::focusStackTop-1] == this && zmsgI(__focusedDispatched__) ) {
		// Focused zuis already got their chance, don't give
		return;
	}

	if( !hidden || zmsgI(__sendDirect__) ) {
		float _x, _y;

		if( getI( "clipMessagesToBounds" ) ) {
			// if the message has coords and those coords are outside of
			// our bounds, do not respond to the message, including any
			// children.  (tfb, may3, 2009 - scrollable listbox)
			_x = msg->getF( "x", -1.f );
			if( _x >= 0.f ) {
				_y = msg->getF( "y", -1.f );
				float winX, winY;
				getWindowXY( winX, winY );
				if( !( _x>=winX && _x<winX+w && _y>=winY && _y<winY+h ) ) {
					return;
				}
			}
		}

		// experimental: parent intercept of messages (tfb, may1 2009)
		int allowChildren = 1;
		int parentHandled = 0;

		/*
		if( getI( "parentHandler" ) ) {
			// do we intercept messages before allowing children a chance?
			allowChildren = 0;
			if( getI( "parentHandled" ) ) {
				// have we already been given a chance to intercept? parent should
				// set this after handling.
				parentHandled = 1;
				allowChildren = 1;
				putI( "parentHandled", 0 );
			}
		}
		*/
		
		// RECURSE otherwise
		if( ! zmsgHas(__sendDirect__) && !zmsgI(__used__) && allowChildren ) {
			ZUI *tail;
			for( tail=headChild; tail && tail->nextSibling; tail=tail->nextSibling );

			ZUI *prev = NULL;
			for( ZUI *_o=tail; _o; _o=prev ) {
				prev = _o->prevSibling;
					// We have to hold on to this because the call
					// to handleMsg could kill the object
				_o->dispatch( msg );
				if( zmsgI(__used__) ) {
					return;
				}
			}
		}
		if( zmsgI(__used__) || parentHandled ) {
			// parentHandled: avoid sending to parent again in the
			// case parent called dispatch() to send to children 
			// (see may1 2009 comment above)
			return;
		}

		if( zmsgHas(x) && zmsgHas(y) ) {
			// Convert global coords to local coords
			_x = zmsgF(x);
			_y = zmsgF(y);
			getLocalXY( _x, _y );
			msg->putF( "localX", _x );
			msg->putF( "localY", _y );
			msg->putI( "on", containsLocal(_x,_y) );
		}

		// CONVERT special ZUI mouse messages: ZUIMouseClickOn, ZUIMouseEnter, ZUIMouseLeave, ZUIMouseReleaseDrag
		if( zmsgIs(type,MouseClick) ) {
			if( containsLocal( _x, _y ) ) {
				putI( "mouseWasInside", 1 );
				ZMsg temp;
				temp.copyFrom( *msg );
				temp.putS( "type", "ZUIMouseClickOn" );
				temp.putS( "__serial__", "-1" );
					// Force the handler into believing that the serial number is unique

				handleMsg( &temp );
				if( getI("dead") ) {
					return;
				}
				if( temp.getI("__used__") ) {
					zMsgUsed();
				}
			}
			else {
				ZMsg temp;
				temp.copyFrom( *msg );
				temp.putS( "type", "ZUIMouseClickOutside" );
				temp.putS( "__serial__", "-1" );
					// Force the handler into believing that the serial number is unique

				handleMsg( &temp );
				if( getI("dead") ) {
					return;
				}
				if( temp.getI("__used__") ) {
					zMsgUsed();
				}
			}
		}
		else if( zmsgIs(type,ZUIMouseMove) || zmsgIs(type,ZUIExclusiveMouseDrag) ) {
			ZMsg temp;
			temp.copyFrom( *msg );

			if( zmsgI(releaseDrag) ) {
				temp.putS( "type", "ZUIMouseReleaseDrag" );
			}
			temp.putS( "__serial__", "-1" );
				// Force the handler into believing that the serial number is unique

			handleMsg( &temp );
			if( getI("dead") ) {
				return;
			}
			if( temp.getI("__used__") ) {
				zMsgUsed();
			}

			// MONITOR enter / exit and send message on state change
			if( containsLocal( _x, _y ) && !getI("mouseWasInside") ) {
				// REMEMBER the state for next frame
				putI( "mouseWasInside", 1 );

				// GENERATE a ZUIMouseEnter
				ZMsg temp;
				temp.putS( "type", "ZUIMouseEnter" );
				temp.putI( "dragging", zmsgIs(type,ZUIExclusiveMouseDrag) ? 1 : 0 );
				temp.putS( "__serial__", "-1" );
					// Force the handler into believing that the serial number is unique
				handleMsg( &temp );
			}
			else if( !containsLocal( _x, _y ) && getI("mouseWasInside") ) {
				// REMEMBER the state for next frame
				putI( "mouseWasInside", 0 );

				// GENERATE a ZUIMouseLeave
				ZMsg temp;
				temp.putS( "type", "ZUIMouseLeave" );
				temp.putI( "dragging", zmsgIs(type,ZUIExclusiveMouseDrag) ? 1 : 0 );
				temp.putS( "__serial__", "-1" );
					// Force the handler into believing that the serial number is unique
				handleMsg( &temp );
			}
		}
		else {
			handleMsg( msg );
		}

	}
}

int zuiKeyEvent=0;
void ZUI::zuiDispatchTree( ZMsg *msg ) {
	// Messages that are directed to a specific ZUI or
	// group are sent directly without a recursion and
	// without checking hidden flag.
	// Otherwise, they are sent recursively though the heirarchy
	// and the hidden flag is respected.

	if( zmsgI(__used__) ) {
		return;
	}

	if( zmsgIs(type,ZUILayout) ) {
		// Layout is a special message that goes to all ZUIs and must be accepted
		// even if everything is hidden
		layoutRequest();
	}

	if( zmsgIs(type, Key) ) {
		// Increment a counter that can be externed by plugins as a cheap way
		// of knowing about any user activity (combined with zMouseMsgX etc)
		zuiKeyEvent++;
	}

	char *toZUI = zmsgS(toZUI);
	if( *toZUI ) {
		ZUI *dst = zuiFindByName( toZUI );
		if( dst ) {
			msg->putI( "__sendDirect__", 1 );
			dst->dispatch( msg );
				// dispatch has a special rule that shortcuts 
				// the recursion and ignores "hidden" if __sendDirect__ set set.
		}
		return;
	}

	char *toZUIGroup = zmsgS(toZUIGroup);
	char *exceptToZUI = zmsgS(exceptToZUI);
	if( *toZUIGroup ) {
		// TRAVERSE all the objects and send to any that have this group identification.
		msg->putI( "__sendDirect__", 1 );

		for( ZHashWalk walk(zuiByNameHash); walk.next(); ) {
			if( *exceptToZUI && !strcmp(exceptToZUI,walk.key) ) continue;

			ZUI *o = *(ZUI **)walk.val;
			if( o->isS( "group", toZUIGroup ) ) {
				o->dispatch( msg );
			}
		}

		return;
	}

	int toZUISiblings = zmsgI(toZUISiblings);
	if( toZUISiblings ) {
		// TRAVERSE all the objects and send to any that have this group identification.
		ZUI *_this = ZUI::zuiFindByName( zmsgS("fromZUI") );
		if( _this ) {
			msg->putI( "__sendDirect__", 1 );
			for( ZUI *o = _this->parent; o; o=o->nextSibling ) {
				o->dispatch( msg );
				if( zMsgIsUsed() ) {
					return;
				}
			}
			return;
		}
	}

	int toZUIParents = zmsgI(toZUIParents);
	if( toZUIParents ) {
		// TRAVERSE all the objects and send to any that have this group identification.
		ZUI *_this = ZUI::zuiFindByName( zmsgS("fromZUI") );
		if( _this ) {
			msg->putI( "__sendDirect__", 1 );
			for( ZUI *o = _this->parent; o; o=o->parent ) {
				o->dispatch( msg );
				if( zMsgIsUsed() ) {
					return;
				}
			}
			return;
		}
	}

	assert( !zmsgIs(type,ZUISet) );
		// If it gets here it wasn't a toZUI or toZUIGroup
		// because it is so easy to forget, we enfore a rule
		// here that you can't send a ZUISet without a toZUI because
		// otherwise it sets the flag in every ZUI in the world!

	if( ZUI::modalStackTop > 0 ) {
		// @TODO: deal with focus/modality combination more elegantly?
		ZUI *modalZUI = ZUI::modalStack[ZUI::modalStackTop-1];
		ZUI *focusZUI = ZUI::focusStackTop>0 ? ZUI::focusStack[ZUI::focusStackTop-1] : 0;
		if(  focusZUI && ( focusZUI==modalZUI || focusZUI->isMyAncestor( modalZUI ) ) ) {
			focusStack[ZUI::focusStackTop-1]->dispatch( msg );
			if( zMsgIsUsed() ) {
				return;
			}
			msg->putI( "__focusedDispatched__", 1 );
				// This tells the dispatcher during recursion below
				// not to handle the msg again.
		}
		modalZUI->dispatch( msg );
		if( zMsgIsUsed() ) {
			return;	
		}
	}

	// SPECIAL ZUI Messages: we process these even when in a modal state
	if( zmsgIs(type,ZUIOrderAbove) ) {
		ZUI *toMoveZUI = zuiFindByName( zmsgS(toMove) );
		ZUI *referenceZUI = zmsgHas(reference) ? zuiFindByName( zmsgS(reference) ) : 0;
		if( toMoveZUI /*&& referenceZUI*/ ) {
			if( !referenceZUI || toMoveZUI->parent == referenceZUI->parent ) {
				toMoveZUI->parent->orderAbove( toMoveZUI, referenceZUI );
			}
		}
		return;
	}
	else if( zmsgIs(type,ZUIOrderBelow) ) {
		ZUI *toMoveZUI = zuiFindByName( zmsgS(toMove) );
		ZUI *referenceZUI = zuiFindByName( zmsgS(reference) );
		if( toMoveZUI && referenceZUI ) {
			if( toMoveZUI->parent == referenceZUI->parent ) {
				toMoveZUI->parent->orderBelow( toMoveZUI, referenceZUI );
			}
		}
		return;
	}

	//
	// The check for modalStackTop was moved here to allow the special zui
	// messages above to be processed even when in a modal state.  Note that
	// we need to allow ZUIShow to pass through such that additional modal
	// dialogs may be displayed when already in a modal state.
	//
	if( ZUI::modalStackTop > 0  && !zmsgIs( type, ZUIShow ) ) {
		return;
	}

	// GIVE focus first chance
	if( ZUI::focusStackTop > 0 ) {
		focusStack[ZUI::focusStackTop-1]->dispatch( msg );
		if( zMsgIsUsed() ) {
			return;
		}
		msg->putI( "__focusedDispatched__", 1 );
			// This tells the dispatcher during recursion below
			// not to handle the msg again.
	}

	if( zmsgIs(type,ZUIExecuteFile) ) {
		char *file = zmsgS(file);
		if( file && *file ) {
			zuiExecuteFile( file );
		}
		return;
	}
	else if( zmsgIs(type,Key) ) {
		if( headModalKeyBinding ) {
			ZUIModalKeyBinding *next = 0;
			for( ZUIModalKeyBinding *b = headModalKeyBinding; b; b=next ) {
				next = b->next;
					// Keep a next as the handleMsg can cause a delete
				b->o->handleMsg( msg );
				if( zMsgIsUsed() ) {
					return;
				}
			}
			return;
		}
		else {
			char *sendMsg = globalKeyBindings.getS( zmsgS(which), 0 );
			if( sendMsg ) {
				::zMsgQueue( sendMsg );
				zMsgUsed();
				return;
			}
		}
	}

	ZUI *root = zuiFindByName( "root" );
	if( root ) {
		root->dispatch( msg );
	}
}

void ZUI::zMsgQueue( char *msg, ... ) {
	// OPverlaoded so that the fromZUI paramter can be set
	static char tempString[512];
	{
		va_list argptr;
		va_start( argptr, msg );
		vsprintf( tempString, msg, argptr );
		va_end( argptr );
		assert( strlen(tempString) < 512 );
	}

	// Search for semi-colon separated strings
	// Must be outside of quote contexts
	char *msgStart = tempString;
	int sQuote = 0, dQuote = 0;

	for( char *c=tempString; *c; c++ ) {
		if( *c == '\'' && !sQuote ) sQuote++;
		else if( *c == '\'' && sQuote ) sQuote--;
		else if( *c == '\"' && !dQuote ) dQuote++;
		else if( *c == '\"' && dQuote ) dQuote--;
		if( !dQuote && !sQuote && *c == ';' ) {
			*c = 0;
			ZHashTable *hash = zHashTable( msgStart );
			char *toZUI = hash->getS("toZUI");
			if( toZUI && ! strcmp(toZUI,"$this") ) {
				hash->putS( "toZUI", getS("name") );
			}
			hash->putS( "fromZUI", name );
			::zMsgQueue( hash );
			msgStart = c+1;
		}
	}

	if( msgStart && *msgStart ) {
		ZHashTable *hash = zHashTable( msgStart );
		hash->putS( "fromZUI", name );
		char *toZUI = hash->getS("toZUI");
		if( toZUI && ! strcmp(toZUI,"$this") ) {
			hash->putS( "toZUI", getS("name") );
		}
		::zMsgQueue( hash );
	}

}

// SCRIPTS
//--------------------

static char *zuiTrimChars( char *str, char *chars ) {
	int len = strlen( str );
	char *tail = &str[len-1];
	while( tail > str ) {
		if( strchr( chars, *tail ) ) {
			*tail=0;
			tail--;
		}
		else {
			break;
		}
	}
	char *head = str;
	while( *head ) {
		if( strchr( chars, *head ) ) {
			head++;
		}
		else {
			break;
		}
	}
	return head;
}

static char *zuiTrimQuotes( char *str ) {
	int len = strlen( str );
	char *tail = &str[len-1];
	while( tail > str ) {
		if( strchr( "'\"", *tail ) ) {
			*tail=0;
			tail--;
		}
		else {
			break;
		}
	}
	char *head = str;
	while( *head ) {
		if( strchr( "'\"", *head ) ) {
			head++;
		}
		else {
			break;
		}
	}
	return head;
}

static void zuiEscapeDecode( char *s ) {
	if( !s ) return;
	char *read = s;
	char *write = s;
	while( *read ) {
		if( *read == '\\' ) {
			switch( *(read+1) ) {
				case 'n': *write = '\n'; read++; break;
				case 't': *write = '\t'; read++; break;
				case 's': *write = ' '; read++; break;
				case '\\': *write = '\\'; read++; break;
			}
		}
		else {
			*write = *read;
		}
		write++;
		read++;
	}
	*write = 0;
}

void ZUI::zuiExecuteFile( char *filename, ZHashTable *args ) {
	ZStr *lines = zStrReadFile( filename );
	if( lines ) {
		zuiExecute( lines, args );
	}
	else {
		trace( "ERROR: failed to read .zui file '%s'\n", filename );
	}
}

void ZUI::zuiExecute( char *script, ZHashTable *args ) {
	ZStr *lines = zStrSplit( "\\n+", script );
	zuiExecute( lines, args );
}

void ZUI::zuiExecute( ZStr *lines, ZHashTable *args ) {
	static ZRegExp var1Test( "\\$\\((\\w+)\\)" );
	static ZRegExp var2Test( "\\$(\\w+)" );
	static ZRegExp comdTest( "^\\s*!\\s*(\\S+)\\s*([^\\n\\r\\t]*)" );
	static ZRegExp com1Test( "^\\s*#" );
	static ZRegExp com2Test( "^\\s*//" );
	static ZRegExp comOTest( "^\\s*/\\*" );
	static ZRegExp comCTest( "^\\s*\\*/" );
	static ZRegExp nameTest( "^\\s*:\\s*([^ \\t\\r\\n=]*)\\s*=?\\s*([^\\s{}]+)?" );
	static ZRegExp qattTest( "^\\s*'([\\+\\*]?[^']+)'\\s*=\\s*([^\\n\\r\\t]*)" );
	static ZRegExp attrTest( "^\\s*([\\+\\*]?\\S+)\\s*=\\s*([^\\n\\r\\t]*)" );
	static ZRegExp grDfTest( "^\\s*@(\\S+)\\s*=" );
	static ZRegExp grupTest( "^\\s*@(\\S+)\\s*$" );
	static ZRegExp closTest( "}\\s*$" );
	static ZRegExp openTest( "{\\s*$" );

	ZStr *line = lines;
	ZStr *includeStackOld[32] = {0,};
	ZStr *includeStackLines[32] = {0,};
	int includeStackDepth = 0;

	int inComment = 0;
	int inMultiline = 0;
	char multilineKey[256];
	char multilineAccumulator[4096*4] = {0,};
	int multilineIgnoreWhitespace = 0;

	const int STACK_SIZE = 20;
	int stackTop = 0;
	ZUI *current = NULL;
	ZUI *objStack[STACK_SIZE];

	char inGroup[80] = {0,};
	ZHashTable groups;
	ZUI groupAccumulator;
		// This is for accumulating attributes into a group block

	while( 1 ) {
		// While there is something on the include stack
		int endFile = 0;
		for( ; !endFile && line; line=line->next ) {
			// Do variable substitutions

			char *a = (char *)*line; // simplify debug
			while( !inComment && (char *)*line ) {
				if( var1Test.test( (char *)*line ) ) {
					char *subs = 0;
					if( args ) {
						subs = args->getS( var1Test.get(1) );
					}
					if( !stricmp("this",var1Test.get(1)) ) {
						subs = objStack[stackTop-1]->name;
					}
					else if( !stricmp("parent",var1Test.get(1)) ) {
						subs = objStack[stackTop-2]->name;
					}
					else if( !stricmp("gparent",var1Test.get(1)) ) {
						subs = objStack[stackTop-3]->name;
					}
					if( subs ) {
						int subsLen = strlen( subs );
						int foundPos = var1Test.getPos(1);
						int foundLen = var1Test.getLen(1);
						int size = strlen( (char *)*line ) + subsLen + 1;
						char *repl = (char *)malloc( size );
						memset( repl, 0, size );
						memcpy( &repl[0], (char *)*line, foundPos );
						memcpy( &repl[foundPos], subs, subsLen );
						memcpy( &repl[foundPos+subsLen], &((char *)*line)[foundPos+foundLen], strlen((char *)*line)-(foundPos+foundLen) );
						line->set( repl );
					}
				}
				else if( var2Test.test( (char *)*line ) ) {
					char *subs = 0;
					if( args ) {
						subs = args->getS( var2Test.get(1) );
					}
					if( !stricmp("this",var2Test.get(1)) ) {
						subs = objStack[stackTop-1]->name;
					}
					else if( !stricmp("parent",var2Test.get(1)) ) {
						subs = objStack[stackTop-2]->name;
					}
					else if( !stricmp("gparent",var2Test.get(1)) ) {
						subs = objStack[stackTop-3]->name;
					}
					if( subs ) {
						int subsLen = strlen( subs );
						int foundPos = var2Test.getPos(1);
						int foundLen = var2Test.getLen(1);
						int size = strlen( (char *)*line ) + subsLen + 1;
						char *repl = (char *)malloc( size );
						memset( repl, 0, size );
						memcpy( &repl[0], (char *)*line, foundPos-1 );
						memcpy( &repl[foundPos-1], subs, subsLen );
						memcpy( &repl[foundPos-1+subsLen], &((char *)*line)[foundPos+foundLen], strlen((char *)*line)-(foundPos+foundLen) );
						line->set( repl );
					}
				}
				else {
					break;
				}
			}

			if( !inComment && inMultiline ) {
				if( comdTest.test( (char *)*line ) && !stricmp( comdTest.get( 1 ), "endmultiline" ) ) {
					objStack[stackTop-1]->putS( multilineKey, multilineAccumulator );
					inMultiline = 0;
				}
				else {
					char *a = (char *)malloc( strlen((char *)*line) * 2 );
					strcpy( a, (char *)*line );
					char *b = a;
					if( multilineIgnoreWhitespace ) {
						b = zuiTrimChars( a, " \t\r\n" );
					}
					zuiEscapeDecode( b );
					strcat( multilineAccumulator, b );
					int lastChar = strlen(b)-1;
					if( b[lastChar] != '\n' && b[lastChar] != '\r' ) {
						strcat( multilineAccumulator, " " );
					}
				}
			}

			else if( comOTest.test( (char *)*line ) ) {
				inComment = 1;
			}
			else if( comCTest.test( (char *)*line ) ) {
				inComment = 0;
			}
			else if( inComment ) {
				continue;
			}
			else if( com1Test.test( (char *)*line ) ) {
				// Skip # comments
			}
			else if( com2Test.test( (char *)*line ) ) {
				// Skip '//' comments
			}
			else if( comdTest.test( (char *)*line ) ) {
				char *action = comdTest.get( 1 );
				if( !stricmp( action, "delete" ) ) {
					char *obj = comdTest.get( 2 );
					ZUI *o = zuiFindByName( obj );
					if( o ) {
						o->die();
					}
					zuiGarbageCollect();
				}
				else if( !stricmp( action, "endfile" ) ) {
					endFile = 1;
				}
				else if( !stricmp( action, "sendMsg" ) ) {
					char *msg = comdTest.get( 2 );
					::zMsgQueue( msg );
				}
				else if( !stricmp( action, "sendMsgToThis" ) ) {
					char *msg = comdTest.get( 2 );
					::zMsgQueue( "%s toZUI='%s'", msg, objStack[stackTop-1]->name );
				}
				else if( !stricmp( action, "sendMsgNow" ) ) {
					ZMsg *msg = new ZMsg();
					zStrHashSplit( comdTest.get( 2 ), msg );
					zMsgDispatchNow( msg );
				}
				else if( !stricmp( action, "createfont" ) ) {
					ZHashTable argTable;
					argTable.putEncoded( comdTest.get( 2 ) );
					zglFontLoad( argTable.getS("name"), argTable.getS("file"), argTable.getF("size") );
						// @TODO: This should cope with changes in type size
						// Right now I think that if it already exists, nothing will happen
				}
				else if( !stricmp( action, "execute" ) ) {
					zuiExecuteFile( comdTest.get( 2 ), args );
				}
				else if( !stricmp( action, "palette" ) ) {
					ZHashTable argTable;
					argTable.putEncoded( comdTest.get( 2 ) );
					ZUI::colorPaletteHash.putU( argTable.getS("key"), argTable.getU("val") );
				}
				else if( !stricmp( action, "multiline" ) ) {
					strcpy( multilineKey, comdTest.get( 2 ) );
					multilineIgnoreWhitespace = 0;
					inMultiline = 1;
					multilineAccumulator[0] = 0;
				}
				else if( !stricmp( action, "multilineIgnoreWhitespace" ) ) {
					strcpy( multilineKey, comdTest.get( 2 ) );
					multilineIgnoreWhitespace = 1;
					inMultiline = 1;
					multilineAccumulator[0] = 0;
				}
				else if( !stricmp( action, "include" ) ) {
					includeStackOld[includeStackDepth] = line->next;
						// We store off the next line because we don't want to
						// re-read the !include statement when we pop.

					ZStr *_lines = zStrReadFile( comdTest.get( 2 ) );
					ZStr *stubLine = new ZStr( "" );
					stubLine->next = _lines;
					includeStackLines[includeStackDepth] = stubLine;
					line = stubLine;

					includeStackDepth++;
					continue;
				}
			}
			else if( grDfTest.test( (char *)*line ) ) {
				strcpy( inGroup, grDfTest.get(1) );
				current = &groupAccumulator;
			}
			else if( grupTest.test( (char *)*line ) ) {
				ZHashTable *hash = (ZHashTable *)groups.getI( grupTest.get(1) );
				if( hash ) {
					assert( stackTop > 0 );
					char *_name = strdup( objStack[stackTop-1]->name );
					objStack[stackTop-1]->copyFrom( *hash );
					objStack[stackTop-1]->putS( "name", _name );
					free( _name );
				}
			}
			else if( nameTest.test( (char *)*line ) ) {
				// CREATE the object from a factory
				char *name = nameTest.get( 1 );
				char *type = nameTest.get( 2 );
				if( type && *type ) {
					current = factory( name, type );
				}
				else {
					current = zuiFindByName( name );
					assert( current );
				}

				// ATTACH to parent
				if( stackTop > 0 ) {
					current->attachTo( objStack[stackTop-1] );
				}
			}
			else if( qattTest.test( (char *)*line ) ) {
				// STUFF attributes into appropriate tables
				char *key = qattTest.get(1);
				char *_val = qattTest.get(2);
				char *dupVal = strdup( _val );
				zuiEscapeDecode( dupVal );
				char *val = dupVal;
				// TRIM leading and trailing spaces
				key = zuiTrimChars( key, " \t" );
				val = zuiTrimChars( val, " \t" );
				key = zuiTrimQuotes( key );
				val = zuiTrimQuotes( val );

				assert( stackTop > 0 );
				if( key[0] == '+' ) {
					objStack[stackTop-1]->putS( &key[1], val );
					key[0] = '*';
					objStack[stackTop-1]->putS( &key[0], val );
				}
				else {
					objStack[stackTop-1]->putS( &key[0], val );
				}

				if( !strcmp(key,"parent") ) {
					ZUI *p = zuiFindByName( val );
					if( p ) {
						objStack[stackTop-1]->detachFrom();
						objStack[stackTop-1]->attachTo( p );
					}
				}
				free( dupVal );
			}
			else if( attrTest.test( (char *)*line ) ) {
				// STUFF attributes into appropriate tables
				char *key = attrTest.get(1);
				char *_val = attrTest.get(2);
				char *dupVal = strdup( _val );
				zuiEscapeDecode( dupVal );
				char *val = dupVal;

				// TRIM leading and trailing spaces
				key = zuiTrimChars( key, " \t" );
				val = zuiTrimChars( val, " \t" );
				key = zuiTrimQuotes( key );
				val = zuiTrimQuotes( val );

				assert( stackTop > 0 );
				if( key[0] == '+' ) {
					objStack[stackTop-1]->putS( &key[1], val );
					key[0] = '*';
					objStack[stackTop-1]->putS( &key[0], val );
				}
				else {
					objStack[stackTop-1]->putS( &key[0], val );
				}

				if( !strcmp(key,"parent") ) {
					ZUI *p = zuiFindByName( val );
					if( p ) {
						objStack[stackTop-1]->detachFrom();
						objStack[stackTop-1]->attachTo( p );
					}
				}
				free( dupVal );
			}
			if( openTest.test( (char *)*line ) ) {
				// INCREMENT stack
				objStack[stackTop] = current;
				stackTop++;
				assert( stackTop < STACK_SIZE );
			}
			if( closTest.test( (char *)*line ) ) {
				if( *inGroup ) {
					ZHashTable *dup = new ZHashTable();
					dup->copyFrom( groupAccumulator );
					groups.putP( inGroup, dup );
					*inGroup = 0;
				}

				// DECREMENT stack
				stackTop--;
				assert( stackTop >= 0 );
			}
		}

		if( includeStackDepth > 0 ) {
			--includeStackDepth;
			zStrDelete( includeStackLines[includeStackDepth] );
			line = includeStackOld[includeStackDepth];
		}
		else {
			break;
		}
	}

	for( int i=0; i<groups.size(); i++ ) {
		char *key = groups.getKey(i);
		ZHashTable *val = (ZHashTable *)groups.getValp(i);
		if( val ) {
			delete val;
		}
	}
	groups.clear();

	//propertiesRunInheritOnAll();

	zStrDelete( lines );
	layoutRequest();
}

void ZUI::zuiSetErrorTrap( int state ) {
	if( zuiErrorMsg ) {
		free( zuiErrorMsg );
		zuiErrorMsg = 0;
	}
	zuiErrorTrapping = state;
}

int ZUI::zuiErrorTrapping = 0;
char *ZUI::zuiErrorMsg = 0;

// CONSTRUCT / DESTRUCT
//--------------------

ZUI::ZUI() {
	parent = headChild = nextSibling = prevSibling = 0;
}

//-----------------------------------------------------------------------------------------------
// Generic DialogBox type functions for zlab.  These make use of zuiDialog in main.zui
//-----------------------------------------------------------------------------------------------

void zuiNamedDialog( char *name, char *type, char *title, int modal, char *okMsg, char *cancelMsg, char *dlgParams, char *controlParams,
			    char *initMsg, char *okText, char *cancelText, int clonedInterior ) {
	// type is the name of a zui that defines the interior controls for the dialog
	// okMsg and cancelMsg are optional.  If no cancelMsg, dialog will not have cancel button.
	// dlgParams/controlParams if non-null may be of form: "text='Hello World' temperature=98.6 myval=5"
	// these are used to configure the dialog and it's controls (e.g. panelColor)
	// initMsg specifies a msg to be generated before the dialog is displayed.  This can be used
	// to programatically initialize interior controls of the dialog.

	ZHashTable *msg = new ZHashTable();
	msg->putS( "name", name );
	msg->putS( "dlgControlsName", type );
	msg->putS( "type", "zuiDialogOpen" );
	msg->putS( "title", title );
	if( modal ) msg->putI( "modal", modal );
	if( okMsg && *okMsg ) msg->putS( "okMsg", okMsg );
	if( cancelMsg /*&& *cancelMsg*/ ) msg->putS( "cancelMsg", cancelMsg );
		// I want the cancel button to show up even if the cancelMsg is ""
	if( dlgParams ) msg->putS( "dlgParams", dlgParams );
	if( controlParams ) msg->putS( "controlParams", controlParams );
	if( initMsg ) msg->putS( "initMsg", initMsg );
	if( !okText || *okText ) msg->putS( "dlgOKText", okText ? okText : (char*)"OK" );
	if( !cancelText || *cancelText ) msg->putS( "dlgCancelText", cancelText ? cancelText : (char*)"Cancel" );
	msg->putI( "clonedInterior", clonedInterior );
	zMsgQueue( msg );
}

void zuiDialog( char *type, char *title, int modal, char *okMsg, char *cancelMsg, char *dlgParams, char *controlParams,
			    char *initMsg, char *okText, char *cancelText, int clonedInterior ) {
	zuiNamedDialog(0,type,title,modal,okMsg,cancelMsg,dlgParams,controlParams,initMsg,okText,cancelText,clonedInterior);
}

void zuiMessageBox( char *title, char *msg, int colorError, char *okMsg, char *cancelMsg, char *okText, char *cancelText ) {
	// many params optional; see declaration; for now default to modal messagebox
	static char dlgParams[256];
	static char ctlParams[256];
	dlgParams[0] = ctlParams[0] = 0;
	if( colorError ) {
		sprintf( dlgParams, "panelColor=zuiMessageDlgHilight" );
		sprintf( ctlParams, "panelColor=zuiMessagePanelHilight" );
			// color the dialog panel as well as the panel of the client controls 'hilight' color
	}
	else {
		sprintf( dlgParams, "panelColor=zuiMessageDlgDefault" );
		sprintf( ctlParams, "panelColor=zuiMessagePanelDefault" );
	}
	
	const char *dlgType = strlen(msg) > 512 || strstr( msg, "Big Dialog" ) ? "messageBoxTextDialogBig" : "messageBoxText";

	ZTmpStr initMsg( "type=zuiMsgBoxInit zuiTextName='%s' text='%s'", dlgType, escapeQuotes( msg, 1 ) );
		// the init message that will allow our messagebox to get initialized with user-passed information
	
	zuiNamedDialog( 0, (char*)dlgType, title, 1, okMsg, cancelMsg, dlgParams[0] ? dlgParams : 0, ctlParams[0] ? ctlParams : 0, initMsg, okText, cancelText );
}

void zuiProgressBox( char *name, char *title, char *msg, char *cancelMsg, char *cancelText ) {
	ZTmpStr initMsg( "type=zuiMsgBoxInit text='%s'", escapeQuotes( msg, 1 ) );
	zuiNamedDialog(name,(char*)"progressBoxInsert",title,1,cancelMsg,"",0,0,initMsg.s,(char*)(cancelText ? cancelText : "Cancel","") );
}


ZMSG_HANDLER( zuiMsgBoxInit ) {
	ZUI *z = ZUI::zuiFindByName( ZTmpStr( "%s/%s", zmsgS( dlgName ), zmsgS( zuiTextName ) ) );
		// use local addressing to find the ZUIText within the current dialog
	if( z ) {
		z->putS( "text", zmsgS( text ) );
	}
}

ZMSG_HANDLER( zuiDialogInit ) {
	if( zmsgHas( dlgParams ) ) {
		ZUI::zuiFindByName( zmsgS( dlgName ) )->putEncoded( zmsgS( dlgParams ) );
	}
	if( zmsgHas( controlParams ) ) {
		ZUI::zuiFindByName( zmsgS( dlgInteriorName ) )->putEncoded( zmsgS( controlParams ) );
			// note that we place any parameters for the client controls into the panel which
			// contains them.  controls may therefore inherit such attributes, or access
			// them in an initMsg via parent->parent zui (multiple controls will be contained
			// in a client ZUIPanel, which will in turn be included in dlgInterior).
	}
}

int dialogsOpen = 0;
ZMSG_HANDLER( zuiDialogOpen ) {
	ZUI *dlg = ZUI::zuiFindByName( (char*)(strstr( zmsgS(dlgControlsName), "DialogBig" ) ? "root/zuiDialogBig" : "root/zuiDialog") );
	ZUI *dlgControls = ZUI::zuiFindByName( ZTmpStr( "root/%s", zmsgS( dlgControlsName ) ) );
		// local addressing to find our zuiDialog template (defined in main.zui) and the
		// template for the user-defined controls, both of which live under root
	if( !dlgControls ) {
		dlgControls = ZUI::zuiFindByName( zmsgS( dlgControlsName ) );
	}

	if( dlg && dlgControls ) {
		char *newDlgName = 0;
		if( zmsgHas( name ) ) {
			newDlgName = zmsgS( name );
		}

		ZUI *newDlg = ZUI::clone( dlg, newDlgName );
		ZUI *dlgInterior = ZUI::zuiFindByName( ZTmpStr( "%s/%s", newDlg->name, "dlgPanel" ) );

		// allow client to force size of parent dlg
		int dW = dlgControls->getI( "dlgWidth" );
		int dH = dlgControls->getI( "dlgHeight" );
		if( dW ) {
			newDlg->putI( "layoutManual_w", dW );
		}
		if( dH ) {
			newDlg->putI( "layoutManual_h", dH );
		}

		if( dlgInterior ) {
			ZUI *newControls = dlgControls;
			if( zmsgI(clonedInterior) ) {
				newControls = ZUI::clone( dlgControls );
				newControls->hidden = 0;
			}
			newControls->attachTo( dlgInterior );

			//
			// SET dlg title
			//
			ZUI::zuiFindByName( ZTmpStr( "%s/%s", newDlg->name, "dlgTitle" ) )->putS( "text", zmsgS( title ) );

			//
			// SET OK/Cancel button attributes
			//
			ZUI *ok = ZUI::zuiFindByName( ZTmpStr( "%s/%s", newDlg->name, "dlgOK" ) );
			ZUI *cancel = ZUI::zuiFindByName( ZTmpStr( "%s/%s", newDlg->name, "dlgCancel" ) );

			ok->putS( "text", zmsgS( dlgOKText ) );
			ok->putS( "sendMsg", ZTmpStr( "type=zuiDialogClose die=1 dialogName=%s", newDlg->name ) );
			if( zmsgHas( okMsg ) ) {
				ok->putS( "sendMsg", ZTmpStr( "%s;%s", ok->getS( "sendMsg" ), zmsgS( okMsg ) ) );
			}

			cancel->putS( "text", zmsgS( dlgCancelText ) );
			cancel->putI( "hidden", !zmsgHas( cancelMsg ) );
			cancel->putS( "sendMsg", ZTmpStr( "type=zuiDialogClose die=1 dialogName=%s", newDlg->name ) );
			if( zmsgHas( cancelMsg ) ) {
				cancel->putS( "sendMsg", ZTmpStr( "%s;%s", cancel->getS( "sendMsg" ), zmsgS( cancelMsg ) ) );
			}

			// 
			// SET arbitrary params passed to us in an "internal" init message (as 
			// opposed to the init message the a user may specify).  We used to set 
			// these params directly on the newDlg or dlgInterior, but attachTo()
			// downstream will set the styleUpdate flag, and later the styles/properties
			// will get updated, so we need to have these set *after* that occurs. (!)
			//
			//
			/* OLD
			if( zmsgHas( dlgParams ) ) {
				newDlg->putEncoded( zmsgS( dlgParams ) );
			}
			if( zmsgHas( controlParams ) ) {
				dlgInterior->putEncoded( zmsgS( controlParams ) );
					// note that we place any parameters for the client controls into the panel which
					// contains them.  controls may therefore inherit such attributes, or access
					// them in an initMsg via parent->parent zui (multiple controls will be contained
					// in a client ZUIPanel, which will in turn be included in dlgInterior).
			}
			*/
			// NEW
			ZTmpStr msgbuf;
			if( zmsgHas( dlgParams ) && zmsgHas( controlParams ) ) {
				msgbuf.set( "dlgParams='%s' controlParams='%s'", zmsgS( dlgParams ), zmsgS( controlParams ) );
			}
			else if( zmsgHas( dlgParams ) ) {
				msgbuf.set( "dlgParams='%s'", zmsgS( dlgParams ) );
			}
			else if( zmsgHas( controlParams ) ) {
				msgbuf.set( "controlParams='%s'", zmsgS( controlParams ) );
			}
			if( msgbuf.s[0] ) {
				zMsgQueue( "type=zuiDialogInit dlgName=%s dlgInteriorName=%s %s __countdown__=1", newDlg->name, dlgInterior->name, (char*)msgbuf );
			}

			//
			// Attach and queue messages for init/display
			//
			newDlg->attachTo( ZUI::zuiFindByName( "root" ) );
			if( zmsgHas( initMsg ) ) {
				zMsgQueue( ZTmpStr( "%s dlgName=%s", zmsgS( initMsg ), newDlg->name ) );
			}
			zMsgQueue( "type=ZUIOrderAbove toMove=%s", newDlg->name );
			zMsgQueue( "type=ZUIShow modal=%d toZUI=%s", zmsgI(modal), newDlg->name );
			if( dialogsOpen++ > 0 ) {
				zMsgQueue( "type=ZUISet toZUI=%s key=translateX val=%d", newDlg->name, dialogsOpen * 20 );
				zMsgQueue( "type=ZUISet toZUI=%s key=translateY val=%d", newDlg->name, dialogsOpen * 20 );
					// offset the dialog 
			}
		}
		else {
			newDlg->die();
		}
	}
	else {
		if( !dlg ) {
			trace( "ERROR: root/zuiDialog UI not found in zuiDialogOpen\n" );
		}
		if( !dlgControls ) {
			trace( "ERROR: user-defined dialog controls %s not found in zuiDialogOpen\n", zmsgS( dlgControlsName ) );
		}
	}
}

ZMSG_HANDLER( zuiDialogClose ) {
	ZUI *dlg = ZUI::zuiFindByName( zmsgS( dialogName ) );
	if( dlg ) {
		if( ZUI::modalStackTop > 0 ) {
			if( ZUI::modalStack[ZUI::modalStackTop-1] == dlg ) {
				dlg->modal( 0 );
			}
			else {
				assert( ! "Dialog closing that is not top of modal stack!" );
			}
		}
		if( zmsgI( die ) ) {
			zMsgQueue( "type=ZUIDie toZUI='%s'", dlg->name );
				// this was added when I added the clone() functionality
				// to support multiple dialog boxes using zuiDialogOpen msg
		}
		else {
			zMsgQueue( "type=ZUIHide toZUI='%s'", dlg->name );
				// TODO: should all dialog close cause a Die?  And if you
				// are reusing a custom dialog, then set the immortal attribute
				// and manually attach to root when you display the dialog? (tfb)
		}
		dialogsOpen--;
	}
}

