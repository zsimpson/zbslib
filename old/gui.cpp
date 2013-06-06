// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A GL gui
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES gui.cpp gui.h
//		*OPTIONAL_FILES guibitmappainter.cpp guicolor.cpp guigraph.cpp guilight.cpp guitwiddle.cpp guivaredit.cpp
//		*GROUP modules/gui
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//			Rename to zbs standard conventions
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
#ifdef WIN32
#include "wintiny.h"
#endif
#include "wingl.h"
// SDK includes:
#include "GL/gl.h"
// STDLIB includes:
#include "assert.h"
#include "math.h"
#include "stdio.h"
#include "string.h"
#include "stdarg.h"
#include "stdlib.h"
#include "ctype.h"
#ifdef WIN32
#include "malloc.h"
#endif
// MODULE includes:
#include "gui.h"
// ZBSLIB includes:
#include "zregexp.h"
#include "ztmpstr.h"
#include "zglfont.h"
#include "zmousemsg.h"
#include "zvars.h"
#include "zplatform.h"

int guiWinW, guiWinH;
double guiTime;
ZHashTable guiFaceHash, guiFontHash, guiNameToObjectTable;


// GUIObject
//==================================================================================

GUI_TEMPLATE_IMPLEMENT(GUIObject);

GUIObject *GUIObject::modalStack[GUI_MODAL_STACK_SIZE];
int GUIObject::modalStackTop = 0;

GUIObject::GUIObject() {
	myX = 0.0f;
	myY = 0.0f;
	myW = 1.0f;
	myH = 1.0f;
	parent = headChild = nextSibling = prevSibling = NULL;
	dead = 0;
	attributeTable = NULL;
	boundMsgHead = NULL;
}

GUIObject::~GUIObject() {
//	if( exclusiveMouseRequestObject == this ) {
//		exclusiveMouseRequestObject = NULL;
//	}
	if( exclusiveKeyboardRequestObject == this ) {
		exclusiveKeyboardRequestObject = NULL;
	}
	if( attributeTable ) {
		guiNameToObjectTable.del( getAttrS("name") );
		delete attributeTable;
	}

	for( char *head = boundMsgHead; head; ) {
		char *next = *(char **)head;
		free( head );
		head = next;
	}
}

void GUIObject::init() {
	attributeTable = new GUIAttributeTable();
	registerAs("");
		// Assigns a default name to this object
}

void GUIObject::modal( int onOff ) {
	if( onOff ) {
		GUIObject::modalStack[GUIObject::modalStackTop++] = this;
		setAttrI( "modal", 1 );
	}
	else {
		GUIObject::modalStackTop--;
		assert( GUIObject::modalStackTop >= 0 );
		setAttrI( "modal", 0 );
	}
}

int GUIObject::hasAttr( char *key ) {
	char *k = key;
	char _key[256];
	for( GUIObject *p = this; p; p=p->parent ) {
		char *val = p->attributeTable->getS( k );
		if( val ) {
			return 1;
		}
		// Now that we are searching up the ancestors
		// we have to prefix a "*" to the names to see
		// if we should inherit them or not
		if( k == key ) {
			_key[0] = '*';
			strcpy( &_key[1], key );
			k = _key;
		}
	}
	return 0;
}

static int getI( char *s ) {
	if( !s ) return (int)0;
	if( s[0]=='0' && s[1]=='x' ) {
        unsigned int a = strtoul (s, NULL, 16 );
		return a;
	}
	return strtol( s, NULL, 10 );
}

static float getF( char *s ) {
	if( !s ) return (float)0;
	return (float)atof( s );
}

static double getD( char *s ) {
	if( !s ) return (double)0;
	return (double)atof( s );
}

static int isChar( char c ) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

void *GUIObject::getAttrp( char *key ) {
	char *k = key;
	char _key[256];
	for( GUIObject *p = this; p; p=p->parent ) {
		void *val = p->attributeTable->getp( k );
		if( val ) {
			return val;
		}
		// Now that we are searching up the ancestors
		// we have to prefix a "*" to the names to see
		// if we should inherit them or not
		if( k == key ) {
			_key[0] = '*';
			strcpy( &_key[1], key );
			k = _key;
		}
	}
	return 0;
}

int GUIObject::getAttrI( char *key ) {
	char *k = key;
	char _key[256];
	for( GUIObject *p = this; p; p=p->parent ) {
		char *val = p->attributeTable->getS( k );
		if( val ) {
			if( isChar(*val) ) {
				ZVarPtr *v = zVarsLookup( val );
				if( v ) {
					return (int)( v->getDouble() );
				}
				return 0;
			}
			return getI( val );
		}
		// Now that we are searching up the ancestors
		// we have to prefix a "*" to the names to see
		// if we should inherit them or not
		if( k == key ) {
			_key[0] = '*';
			strcpy( &_key[1], key );
			k = _key;
		}
	}
	return 0;
}

float GUIObject::getAttrF( char *key ) {
	char *k = key;
	char _key[256];
	for( GUIObject *p = this; p; p=p->parent ) {
		char *val = p->attributeTable->getS( k );
		if( val ) {
			if( isChar(*val) ) {
				ZVarPtr *v = zVarsLookup( val );
				if( v ) {
					return (float)( v->getDouble() );
				}
				return 0;
			}
			return getF( val );
		}
		// Now that we are searching up the ancestors
		// we have to prefix a "*" to the names to see
		// if we should inherit them or not
		if( k == key ) {
			_key[0] = '*';
			strcpy( &_key[1], key );
			k = _key;
		}
	}
	return 0.f;
}

double GUIObject::getAttrD( char *key ) {
	char *k = key;
	char _key[256];
	for( GUIObject *p = this; p; p=p->parent ) {
		char *val = p->attributeTable->getS( k );
		if( val ) {
			if( isChar(*val) ) {
				ZVarPtr *v = zVarsLookup( val );
				if( v ) {
					return v->getDouble();
				}
				return 0;
			}
			return getD(val);
		}
		// Now that we are searching up the ancestors
		// we have to prefix a "*" to the names to see
		// if we should inherit them or not
		if( k == key ) {
			_key[0] = '*';
			strcpy( &_key[1], key );
			k = _key;
		}
	}
	return 0.0;
}

char *GUIObject::getAttrS( char *key ) {
	char *k = key;
	char _key[256];
	for( GUIObject *p = this; p; p=p->parent ) {
		char *s = p->attributeTable->getS( k );
		if( s ) {
			return s;
		}
		// Now that we are searching up the ancestors
		// we have to prefix a "*" to the names to see
		// if we should inherit them or not
		if( k == key ) {
			_key[0] = '*';
			strcpy( &_key[1], key );
			k = _key;
		}
	}
	return "";
}

int GUIObject::isAttrI( char *key, int val ) {
	return getAttrI( key ) == val;
}

int GUIObject::isAttrF( char *key, float val ) {
	return getAttrF( key ) == val;
}

int GUIObject::isAttrD( char *key, double val ) {
	return getAttrD( key ) == val;
}

int GUIObject::isAttrS( char *key, char *val ) {
	return !strcmp( getAttrS( key ), val );
}

void GUIObject::setAttr( char *fmt, ... ) {
	static char temp[512];
	{
		va_list argptr;
		va_start( argptr, fmt );
		vsprintf( temp, fmt, argptr );
		va_end( argptr );
		assert( strlen(temp) < 512 );
	}

	ZHashTable h(32);
	h.putEncoded( temp );

	for( int i=0; i<h.size(); i++ ) {
		char *key = h.getKey(i);
		char *val = h.getValS(i);
		if( key ) {
			setAttrS( key, val );
		}
	}
}

void GUIObject::setAttrp( char *key, void *val ) {
	attributeTable->putp( key, val );
}

void GUIObject::setAttrI( char *key, int val ) {
	char *v = attributeTable->getS( key );
	if( v && isChar(*v) ) {
		// This might be a pointer to a global
		ZVarPtr *_v = zVarsLookup( v );
		if( _v ) {
			_v->setFromDouble( (double)val );
			return;
		}
	}
	char buf[16];
	sprintf( buf, "%d", val );
	setAttrS( key, buf );
}

void GUIObject::setAttrF( char *key, float val ) {
	char *v = attributeTable->getS( key );
	if( v && isChar(*v) ) {
		// This might be a pointer to a global
		ZVarPtr *_v = zVarsLookup( v );
		if( _v ) {
			_v->setFromDouble( (double)val );
			return;
		}
	}
	char buf[256];
	sprintf( buf, "%f", val );
	setAttrS( key, buf );
}

void GUIObject::setAttrD( char *key, double val ) {
	char *v = attributeTable->getS( key );
	if( v && isChar(*v) ) {
		// This might be a pointer to a global
		ZVarPtr *_v = zVarsLookup( v );
		if( _v ) {
			_v->setFromDouble( (double)val );
			return;
		}
	}
	char buf[256];
	sprintf( buf, "%lf", val );
	setAttrS( key, buf );
}

void GUIObject::setAttrS( char *key, char *val ) {
	if( !memcmp(key,"sendMsgOn:",10) ) {
		// This is a special attribute which actually
		// gets stuffed into a linked list to accelerate queries
		// @TODO: Deal with deleteing bindings
		// The linked is is formatted as follows:
		// 4 bytes: next pointer
		// 4 bytes: count of following 
		//   nul-terminated key
		//   nul-terminated val
		// nul-terminated message to send
		char *_m = (char *)malloc( strlen(key) + strlen(val) + sizeof(int) + sizeof(char *) );
		char *m = _m;
		*(char **)m = boundMsgHead;
		boundMsgHead = m;
		m += sizeof(char *);

		ZHashTable hash;
		hash.putEncoded( key+10 );
			// +10 from length of "sendMsgOn:"
		*(int *)m = hash.activeCount();
		m += sizeof(int);
		for( int i=0; i<hash.size(); i++ ) {
			char *_key = hash.getKey( i );
			char *_val = hash.getValS( i );
			if( _key ) {
				strcpy( m, _key );
				m += strlen( _key ) + 1;
				assert( _val );
				strcpy( m, _val );
				m += strlen( _val ) + 1;
			}
		}
		strcpy( m, val );
		return;
	}
	else if( strcmp(key,"hooks") ) {
		// If this is NOT a hooks message...
		// we send a change message to anyone who has hooked this GUIObject's messages
		char *hooks = getAttrS( "hooks" );
		while( *hooks ) {
			char *space = strchr( hooks, ' ' );
			if( !space ) {
				space = strchr( hooks, 0 );
			}
			if( space ) {
				char hookName[128];
				memcpy( hookName, hooks, space-hooks );
				hookName[space-hooks] = 0;
				if( hookName[0]=='0' && hookName[1]=='x' ) {
					GUIObject *who = (GUIObject *)strtoul( hookName, NULL, 16 );
					strcpy( hookName, who->getAttrS("name") );
				}
				zMsgQueue( "type=GUIAttrChanged key='%s' val='%s' toGUI='%s'", key, val, hookName );
			}
			hooks += space - hooks;
			if( !*space ) {
				break;
			}
		}

		if( !strcmp(key,"hidden") ) {
			postLayout();
		}
	}
	attributeTable->putS( key, val );
}

void GUIObject::registerAs( char *name ) {
	char *currName = getAttrS( "name" );
	if( currName && *currName ) {
		guiNameToObjectTable.del( currName );
	}

	static int defaultNameCounter = 1;
	char defaultName[32];
	if( !name || (name && !*name) ) {
		sprintf( defaultName, "obj%d", defaultNameCounter++ );
		name = defaultName;
	}
	if( name && *name ) {
		assert( guiNameToObjectTable.getS( name ) == NULL );
		guiNameToObjectTable.putI( name, (int)this );
		setAttrS( "name", name );
	}
}

void GUIObject::dump( int toStdout ) {
	attributeTable->dump( toStdout );
}

void GUIObject::orderAbove( GUIObject *toMoveGUI, GUIObject *referenceGUI ) {
	// If the toMoveGUI occurs before the referenceGUI then we need
	// to move the toMoveGUI otherwise it is already above so we don't do anything
	GUIObject *first = NULL;
	for( GUIObject *c=headChild; c; c=c->nextSibling ) {
		if( !first && c==toMoveGUI ) {
			first = c;
			break;
		}
		else if( !first && c==referenceGUI ) {
			first = c;
			break;
		}
	}

	if( first == toMoveGUI ) {
		// RELINK so that toMoveGUI is next after the referenceGUI
		GUIObject *movePrev = toMoveGUI->prevSibling;
		GUIObject *moveNext = toMoveGUI->nextSibling;
		GUIObject *refPrev = referenceGUI->prevSibling;
		GUIObject *refNext = referenceGUI->nextSibling;

		// UNLINK the moving one from where it is
		if( movePrev ) {
			movePrev->nextSibling = moveNext;
		}
		if( moveNext ) {
			moveNext->prevSibling = movePrev;
		}

		// LINK it to where it is moving
		referenceGUI->nextSibling = toMoveGUI;
		toMoveGUI->prevSibling = referenceGUI;
		toMoveGUI->nextSibling = refNext;
		if( refNext ) {
			refNext->prevSibling = toMoveGUI;
		}

		// UPDATE the headlink
		if( toMoveGUI == headChild ) {
			headChild = moveNext;
		}
	}
}

void GUIObject::orderBelow( GUIObject *toMoveGUI, GUIObject *referenceGUI ) {
	// If the toMoveGUI occurs before the referenceGUI then we need
	// to move the toMoveGUI otherwise it is already above so we don't do anything
	GUIObject *first = NULL;
	for( GUIObject *c=headChild; c; c=c->nextSibling ) {
		if( !first && c==toMoveGUI ) {
			first = c;
			break;
		}
		else if( !first && c==referenceGUI ) {
			first = c;
			break;
		}
	}

	if( first == referenceGUI ) {
		// RELINK so that toMoveGUI is next before the referenceGUI
		GUIObject *movePrev = toMoveGUI->prevSibling;
		GUIObject *moveNext = toMoveGUI->nextSibling;
		GUIObject *refPrev = referenceGUI->prevSibling;
		GUIObject *refNext = referenceGUI->nextSibling;

		// UNLINK the moving one from where it is
		if( movePrev ) {
			movePrev->nextSibling = moveNext;
		}
		if( moveNext ) {
			moveNext->prevSibling = movePrev;
		}

		// LINK it to where it is moving
		referenceGUI->prevSibling = toMoveGUI;
		toMoveGUI->nextSibling = referenceGUI;
		toMoveGUI->prevSibling = refPrev;
		if( refPrev ) {
			refPrev->nextSibling = toMoveGUI;
		}

		// UPDATE the headlink
		if( toMoveGUI == headChild ) {
			headChild = moveNext;
		}
	}
}

void GUIObject::attachTo( GUIObject *_parent ) {
	assert( _parent );
	if( parent ) {
		detachFrom();
	}
	parent = _parent;
	_parent->postLayout();
	postLayout();

	// ADD to the end of the list
	nextSibling = prevSibling = NULL;
	GUIObject *tail = NULL;
	for( GUIObject *c = _parent->headChild; c; c=c->nextSibling ) {
		tail = c;
	}
	if( tail ) {
		tail->nextSibling = this;
		prevSibling = tail;
	}
	else {
		parent->headChild = this;
	}
}

void GUIObject::detachFrom() {
	if( parent ) {
		postLayout();
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

int GUIObject::isMyChild( char *name ) {
	for( GUIObject *o=headChild; o; o=o->nextSibling ) {
		if( o->isAttrS( "name", name ) ) {
			return 1;
		}
	}
	return 0;
}

void GUIObject::killChildren() {
	GUIObject *next = NULL;
	for( GUIObject *c = headChild; c; c=next ) {
		next = c->nextSibling;
		c->die();
	}
}

void GUIObject::die() {
	if( getAttrI( "immortal" ) ) {
		// This is a usefuil attribute that keeps code generated
		// objects alive so that they can be reattached to
		// dynamically generated content
		detachFrom();
		return;
	}

	// MARK this and all children as dead.  They will 
	// be synchronously garbage collected by the update function.
	registerAs( NULL );
		// This frees up the name in the database.
		// The garbage collector will get around to
		// pulling all of this out regardless of name
	dead = 1;
	detachFrom();
	GUIObject *next = NULL;
	for( GUIObject *c = headChild; c; c=next ) {
		next = c->nextSibling;
		c->die();
	}
	if( GUIObject::modalStackTop >= 0 ) {
		if( GUIObject::modalStack[GUIObject::modalStackTop-1] == this ) {
			GUIObject::modalStackTop--;
		}
	}
}

GUIObject *GUIObject::childFromLocation( float x, float y ) {
	for( GUIObject *c=headChild; c; c=c->nextSibling ) {
		if( x>=c->myX && x<=c->myX+c->myW && y>=c->myY && y<=c->myY+c->myH ) {
			return c;
		}
	}
	return 0;
}

void GUIObject::getLocalCoords( float &x, float &y ) {
	for( GUIObject *c=this; c; c=c->parent ) {
		x -= c->myX;
		y -= c->myY;
	}
}

void GUIObject::getWindowCoords( float &x, float &y ) {
	for( GUIObject *c=this; c; c=c->parent ) {
		x += c->myX;
		y += c->myY;
	}
}

int GUIObject::contains( float _x, float _y ) {
	return( _x >= 0 && _x < myW && _y >= 0 && _y < myH );
}

void GUIObject::move( float _x, float _y ) {
	zMsgQueue( "type=GUIMoved oldX=%f oldY=%f newX=%f newY=%f toGUI='%s'", myX, myY, _x, _y, getAttrS("name") );
	myX = _x;
	myY = _y;
}

void GUIObject::moveNear( char *who, char *where, float xOff, float yOff ) {
	float _x=0.f, _y=0.f;
	GUIObject *o = guiFind( who );
	if( o ) {
		float x=0.f, y=0.f;
		o->getWindowCoords( x, y );
		if( *where == 'B' ) {
			_x = x;
			_y = y - myH;
		}
		else if( *where == 'T' ) {
			_x = x;
			_y = y + o->myH;
		}
		_x += xOff;
		_y += yOff;
		_x = max( 0, min( guiWinW, _x ) );
		_y = max( 0, min( guiWinH, _y ) );
		getLocalCoords( _x, _y );
		move( _x, _y );
	}
}

void GUIObject::resize( float _w, float _h ) {
	zMsgQueue( "type=GUIResized oldW=%f oldH=%f newW=%f newH=%f toGUI='%s'", myW, myH, _w, _h, getAttrS("name") );
	myW = _w;
	myH = _h;
	layout();
}

void GUIObject::print( char *m, float x, float y ) {
	zglFontPrint( m, x, y, getAttrS("font") );
}

void GUIObject::printSize( char *m, float &x, float &y, float &d ) {
	zglFontGetDim( m, x, y, d, getAttrS("font") );
}

void GUIObject::printWordWrap( char *m, float l, float t, float w, float h ) {
	float segmentW = 0.f, segmentH = 0.f;
	char temp;
	char *lastSpace = NULL;

	void *font = zglFontGet( getAttrS("font") );

	int len = strlen( m ) + 1;
	char *copyOfM = (char *)alloca( len );
	memcpy( copyOfM, m, len );
	char *segmentStart = copyOfM;
	float descender;
	zglFontGetDim( "W", segmentW, segmentH, descender, 0, font );
	float y = myH - t - segmentH;

	// Calculate word-wrap
	for( char *c = copyOfM; ; c++ ) {
		if( *c == '\n' || *c == 0 ) {
			temp = *c;
			*c = 0;
			print( segmentStart, l, y );
			*c = temp;
			y -= segmentH;
			segmentStart = c+1;
			segmentW = 0.0;
			if( *c == 0 ) {
				break;
			}
		}
		else {
			char buf[2];
			buf[0] = *c;
			buf[1] = 0;
			float charW = 0.f;
			zglFontGetDim( buf, charW, segmentH, descender, 0, font );
			segmentW += charW;

			if( segmentW >= w ) {
				if( !lastSpace ) {
					lastSpace = c;
				}
				temp = *lastSpace;
				*lastSpace = 0;
				print( segmentStart, l, y );
				*lastSpace = temp;
				y -= segmentH;
				segmentStart = *lastSpace==' ' ? lastSpace+1 : lastSpace;
				c = lastSpace;
				lastSpace = NULL;
				segmentW = 0.0;
			}
			else if( *c == ' ' ) {
				lastSpace = c;
			}
		}
	}
}

struct GUIModalKeyBinding {
	GUIObject *o;
	GUIModalKeyBinding *next;
	GUIModalKeyBinding *prev;
};
static GUIModalKeyBinding *headModalKeyBinding = 0;

void GUIObject::modalKeysClear() {
	GUIModalKeyBinding *next;
	for( GUIModalKeyBinding *o = headModalKeyBinding; o; o=next ) {
		next = o->next;
		delete o;
	}
	headModalKeyBinding = 0;
	::zMsgQueue( "type=GUIModalKeysClear" );
}

void GUIObject::modalKeysBind( GUIObject *o ) {
	GUIModalKeyBinding *b = new GUIModalKeyBinding;
	b->o = o;
	b->next = headModalKeyBinding;
	b->prev = 0;
	if( headModalKeyBinding ) {
		headModalKeyBinding->prev = b;
	}
	headModalKeyBinding = b;
}

void GUIObject::modalKeysUnbind( GUIObject *o ) {
	for( GUIModalKeyBinding *b = headModalKeyBinding; b; b=b->next ) {
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

void GUIObject::queryOptSize( float &w, float &h ) {
	w = 1;
	h = 1;
	if( hasAttr("optW") ) w = getAttrF("optW");
	if( hasAttr("optH") ) h = getAttrF("optH");
}

void GUIObject::queryMinSize( float &w, float &h ) {
	w = 1;
	h = 1;
}

void GUIObject::queryMaxSize( float &w, float &h ) {
	w = 10000;
	h = 10000;
}

void GUIObject::update() {
}

// the problem: imagine that there is a container (child) that is inside of another conater (parent)
// the parent is laying out stuff in pack and wants to know the size of the child
// it asks the child and the child can determine it's opt and min sizes but it
// may turn out that these are too big in which case the parent will want to 
// change the size but then the child will need to re-layout because it has
// been granted less space which can then trickle down
// 
// seems like it needs to passes.  One is, recursively figure out
// the optimal size.  This is probably too big so then from 
// top down the parent's begin to restrict the size and each container
// is forced to make due with what it gets.

void GUIObject::packQueryOptSize( float &w, float &h ) {
	if( isAttrS( "layoutManager", "pack" ) ) {
		w = 0.f;
		h = 0.f;
		int layoutDir = 0;
		if( isAttrS( "pack_side", "top" ) || isAttrS( "pack_side", "bottom" ) ) {
			layoutDir = 1;
		}

		for( GUIObject *c = headChild; c; c=c->nextSibling ) {
			float _w, _h;
			c->recurseQueryOptSize( _w, _h );

			float padX = c->getAttrF( "packChildOption_padX" );
			float padY = c->getAttrF( "packChildOption_padY" );

			padX *= 2.f;
			padY *= 2.f;

			if( layoutDir == 0 ) {
				w += _w + padX;
				h = max( h, _h + padY );
			}
			else {
				h += _h + padY;
				w = max( w, _w + padX );
			}
		}
	}
}

void GUIObject::tableQueryOptSize( float &w, float &h ) {
	w = 0.f;
	h = 0.f;

	int numChildren = 0;
	GUIObject *c;
	for( c = headChild; c; c=c->nextSibling ) {
		numChildren++;
	}
	int cols = getAttrI( "table_cols" );
	int rows = getAttrI( "table_rows" );
	if( cols && !rows ) {
		rows = numChildren / cols + (numChildren % cols > 0 ? 1 : 0);
	}
	else if( rows && !cols ) {
		cols = numChildren / rows + (numChildren % rows > 0 ? 1 : 0);
	}
	if( !cols ) {
		cols = 1;
	}
	if( !rows ) {
		rows = numChildren / cols + (numChildren % cols > 0 ? 1 : 0);
	}
	cols = cols > 0 ? cols : 1;
	rows = rows > 0 ? rows : (numChildren / cols + (numChildren % cols > 0 ? 1 :  0) );
	float *colSizes = (float *)alloca( sizeof(float) * cols );
	memset( colSizes, 0, sizeof(float)*cols );
	float *rowSizes = (float *)alloca( sizeof(float) * rows );
	memset( rowSizes, 0, sizeof(float)*rows );
	int i = 0;
	for( c = headChild; c; c=c->nextSibling, i++ ) {
		float _w, _h;
		c->recurseQueryOptSize( _w, _h );

		float padX = c->getAttrF( "tableChildOption_padX" );
		float padY = c->getAttrF( "tableChildOption_padY" );
		padX *= 2.f;
		padY *= 2.f;

		colSizes[i%cols] = max( colSizes[i%cols], _w + padX );
		rowSizes[i/cols] = max( rowSizes[i/cols], _h + padY );
	}

	w = 0.f; h = 0.f;
	for( i=0; i<cols; i++ ) {
		w += colSizes[i];
	}
	for( i=0; i<rows; i++ ) {
		h += rowSizes[i];
	}
}

void GUIObject::placeQueryOptSize( float &w, float &h ) {
	w = getAttrF( "place_optW" );
	h = getAttrF( "place_optH" );
}

void GUIObject::recurseQueryOptSize( float &w, float &h ) {
	if( getAttrI("hidden") || getAttrI("nolayout") ) {
		w = 0.f;
		h = 0.f;
	}

	if( !headChild ) {
		// This is a leaf node so we can just get the optimal size
		// We don't have to recurse anymore.
		queryOptSize( w, h );
		return;
	}

	if( isAttrS( "layoutManager", "pack" ) ) {
		packQueryOptSize( w, h );
	}
	else if( isAttrS( "layoutManager", "table" ) ) {
		tableQueryOptSize( w, h );
	}
	else if( isAttrS( "layoutManager", "place" ) ) {
		placeQueryOptSize( w, h );
	}
	else {
		packQueryOptSize( w, h );
	}
}

void GUIObject::pack() {
	char *_side = getAttrS( "pack_side" );
	int side = toupper(_side[0]);
	if( !side ) side = 'L';
	char *_anchor = getAttrS( "pack_anchor" );
	int anchor = toupper(_anchor[0]);
	if( !anchor ) anchor = 'C';

	// COUNT children
	int numChildren = 0;
	GUIObject *c;
	for( c = headChild; c; c=c->nextSibling ) {
		if( !c->getAttrI("hidden") && !c->getAttrI("nolayout") ) {
			numChildren++;
		}
	}

	float *optSizes = (float *)alloca( sizeof(float) * 2 * (numChildren+1) );

	// QUERY size of each non-hidden child
	int i = 0;
	for( c = headChild; c; c=c->nextSibling ) {
		if( !c->getAttrI("hidden") && !c->getAttrI("nolayout") ) {
			i++;
			c->recurseQueryOptSize( optSizes[i*2+0], optSizes[i*2+1] );
			float _optW = c->getAttrF( "packChildOption_optW" );
			float _optH = c->getAttrF( "packChildOption_optH" );
			if( _optW > 0.f ) optSizes[2*i+0] = _optW;
			if( _optH > 0.f ) optSizes[2*i+1] = _optH;
			// @TODO: deal with non-optimal sizes
		}
	}

	float posX = ( side == 'R' ) ? myW : 0.f;
	float posY = ( side == 'T' ) ? myH : 0.f;
	i = 0;

	float remainW = myW;
	float remainH = myH;
	for( c = headChild; c; c=c->nextSibling ) {
		if( c->getAttrI("hidden") || c->getAttrI("nolayout") ) {
			continue;
		}
		i++;
		float padX = c->getAttrF( "packChildOption_padX" );
		float padY = c->getAttrF( "packChildOption_padY" );
		float _w = optSizes[i*2+0];
		float _h = optSizes[i*2+1];

		if( side == 'L' ) _w = min( _w, myW-posX-padX*2.f );
		if( side == 'R' ) _w = min( _w, posX-padX*2.f );
		if( side == 'T' ) _h = min( _h, posY-padY*2.f );
		if( side == 'B' ) _h = min( _h, myH-posY-padY*2.f );

		char *fill = c->getAttrS( "packChildOption_fill" );

		// EXPAND w/h if fill is on
		if( (i==numChildren || (side=='T' || side == 'B')) && (!stricmp(fill,"x") || !stricmp(fill,"both")) ) {
			_w = remainW - padX*2.f;	
		}
		if( (i==numChildren || (side=='L' || side == 'R')) && (!stricmp(fill,"y") || !stricmp(fill,"both")) ) {
			_h = remainH - padY*2.f;
		}
		_w = min( parent->myW, _w );
		_h = min( parent->myH, _h );

		c->resize( _w, _h );

		// PRE-INCREMENT the pos on top and right layouts
		if( side == 'R' ) {
			posX -= _w + padX*2;
			remainW -= _w + padX*2;
		}
		if( side == 'T' ) {
			posY -= _h + padY*2;
			remainH -= _h + padY*2;
		}

		float x = posX + padX;
		float y = posY + padY;
		if( side == 'L' || side == 'R' ) {
			if( anchor == 'B' ) y = 0.f;
			if( anchor == 'T' ) y = myH - _h;
			if( anchor == 'C' ) y = (myH - _h) / 2.f;
		}
		if( side == 'T' || side == 'B' ) {
			if( anchor == 'L' ) x = 0.f;
			if( anchor == 'R' ) x = myW - _w;
			if( anchor == 'C' ) x = (myW - _w) / 2.f;
		}
		c->move( x, y );

		// POST-INCREMENT on top and right layouts
		if( side == 'L' ) {
			posX += optSizes[i*2+0] + padX*2;
			remainW -= optSizes[i*2+0] + padX*2;
		}
		if( side == 'B' ) {
			posY += optSizes[i*2+1] + padY*2;
			remainH -= optSizes[i*2+1] + padY*2;
		}
	}
}

void GUIObject::table() {
	// COUNT children
	int numChildren = 0;
	GUIObject *c;
	for( c = headChild; c; c=c->nextSibling ) {
		if( (!c->getAttrI("hidden") && !c->getAttrI("nolayout")) ) {
			numChildren++;
		}
	}

	// CALC number of rows and cols
	int cols = getAttrI( "table_cols" );
	int rows = getAttrI( "table_rows" );
	if( cols && !rows ) {
		rows = numChildren / cols + (numChildren % cols > 0 ? 1 : 0);
	}
	else if( rows && !cols ) {
		cols = numChildren / rows + (numChildren % rows > 0 ? 1 : 0);
	}
	if( !cols ) {
		cols = 1;
	}
	if( !rows ) {
		rows = numChildren / cols + (numChildren % cols > 0 ? 1 : 0);
	}

	float *optSizes = (float *)alloca( sizeof(float) * 2 * numChildren );
	memset( optSizes, 0, sizeof(float) * 2 * numChildren );

	// QUERY size of each child
	int i = 0;
	for( c = headChild; c; c=c->nextSibling ) {
		if( !c->getAttrI("hidden") && !c->getAttrI("nolayout") ) {
			c->recurseQueryOptSize( optSizes[i*2+0], optSizes[i*2+1] );

			float padX = c->getAttrF( "tableChildOption_padX" );
			float padY = c->getAttrF( "tableChildOption_padY" );
			padX *= 2.f;
			padY *= 2.f;

			optSizes[i*2+0] += padX;
			optSizes[i*2+1] += padY;

			i++;
			// @TODO: deal with non-optimal sizes
		}
	}

	// FIND the widest and the tallest in each column and row
	float *colSizes = (float *)alloca( sizeof(float) * cols );
	memset( colSizes, 0, sizeof(float)*cols );
	float *rowSizes = (float *)alloca( sizeof(float) * rows );
	memset( rowSizes, 0, sizeof(float)*rows );
	i = 0;
	for( c = headChild; c; c=c->nextSibling ) {
		if( !c->getAttrI("hidden") && !c->getAttrI("nolayout") ) {
			rowSizes[i/cols] = max( rowSizes[i/cols], optSizes[i*2+1] );
			colSizes[i%cols] = max( colSizes[i%cols], optSizes[i*2+0] );
			i++;
		}
	}

	// CALCULATE the row and col positions
	if( getAttrF( "table_colDistributeEvenly" ) ) {
		for( i=0; i<cols; i++ ) {
			colSizes[i] = myW / cols;
		}
	}
	else if( getAttrF( "table_colWeight1" ) ) {
		// The col is weight-based, so override the col sizes
		float sum = 0.f;
		for( i=0; i<cols; i++ ) {
			char buf[128];
			sprintf( buf, "table_colWeight%d", i+1 );
			sum += getAttrF( buf );
		}
		for( i=0; i<cols; i++ ) {
			char buf[128];
			sprintf( buf, "table_colWeight%d", i+1 );
			colSizes[i] = myW * getAttrF( buf ) / sum;
		}
	}

	i=0;
	for( c = headChild; c; c=c->nextSibling ) {
		if( !c->getAttrI("hidden") && !c->getAttrI("nolayout") && c->hasAttr("tableChildOption_calcHFromW") ) {
			optSizes[i*2+1] = c->getAttrF("tableChildOption_calcHFromW") * colSizes[i%cols];
			rowSizes[i/cols] = max( rowSizes[i/cols], optSizes[i*2+1] );
			i++;
		}
	}

	if( getAttrF( "table_rowDistributeEvenly" ) ) {
		for( i=0; i<rows; i++ ) {
			rowSizes[i] = myH / rows;
		}
	}
	else if( getAttrF( "table_rowWeight1" ) ) {
		// The row is weight-based, so override the row sizes
		float sum = 0.f;
		for( i=0; i<rows; i++ ) {
			char buf[128];
			sprintf( buf, "table_rowWeight%d", i+1 );
			sum += getAttrF( buf );
		}
		for( i=0; i<rows; i++ ) {
			char buf[128];
			sprintf( buf, "table_rowWeight%d", i+1 );
			rowSizes[i] = myH * getAttrF( buf ) / sum;
		}
	}

	float *colPos = (float *)alloca( sizeof(float) * cols );
	float *rowPos = (float *)alloca( sizeof(float) * rows );
	float y = myH - rowSizes[0];
	for( i=0; i<rows; i++ ) {
		rowPos[i] = y;
		y -= rowSizes[i+1];
	}
	float x = 0.f;
	for( i=0; i<cols; i++ ) {
		colPos[i] = x;
		x += colSizes[i];
	}

	// RESIZE the children and position them
	float pos[2] = { 0, };
	i = 0;
	for( c = headChild; c; c=c->nextSibling ) {
		if( !c->getAttrI("hidden") && !c->getAttrI("nolayout") ) {
			float pad[2];
			pad[0] = c->getAttrF( "tableChildOption_padX" );
			pad[1] = c->getAttrF( "tableChildOption_padY" );

			float cellW = colSizes[i%cols];
			float cellH = rowSizes[i/cols];

			float _w = min( optSizes[i*2+0]-pad[0]*2.f, cellW );
			float _h = min( optSizes[i*2+1]-pad[1]*2.f, cellH );
			char *fill = c->getAttrS( "tableChildOption_fill" );
			if( (!stricmp(fill,"x") || !stricmp(fill,"both")) ) {
				_w = cellW - pad[0]*2.f;	
			}
			if( (!stricmp(fill,"y") || !stricmp(fill,"both")) ) {
				_h = cellH - pad[1]*2.f;	
			}

			c->resize( _w, _h );
				// This will cause a layout of this new size

			float xx = colPos[i%cols] + pad[0];
			char *hanchor = c->getAttrS( "tableChildOption_hAnchor" );
			switch( *hanchor ) {
				case 'c':
					xx += (cellW - _w) / 2.f;
					break;
				case 'r':
					xx = cellW - pad[0] - _w;
					break;
			}
			float yy = rowPos[i/cols] + pad[1];
			char *vanchor = c->getAttrS( "tableChildOption_vAnchor" );
			switch( *vanchor ) {
				case 'c':
					yy += (cellH - _h) / 2.f;
					break;
				case 't':
					yy = cellH - pad[1] - _h;
					break;
			}

			c->move( xx, yy );
			i++;
		}
	}
}

float GUIObject::placeParseInstruction( char *str ) {
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
	float a, b;

	float optW, optH;
	recurseQueryOptSize( optW, optH );

	for( char *c=str; state != DONE; c++ ) {
		switch( state ) {
			case START:
				if( (*c >= '0' && *c <='9') || *c == '.' ) {
					c--;
					decimal = 0.f;
					constant = 0.f;
					state = NUMBER;
				}
				else {
					switch( *c ) {
						case 0: state = DONE; break;
						case ' ': break;

						case 'L': PUSH( parent->myX ); break;
						case 'T': PUSH( parent->myY + parent->myH ); break;
						case 'R': PUSH( parent->myX + parent->myW ); break;
						case 'B': PUSH( parent->myY ); break;
						case 'W': PUSH( parent->myW ); break;
						case 'H': PUSH( parent->myH ); break;
						case 'w': PUSH( optW ); break;
						case 'h': PUSH( optH ); break;

						case '+': a=POP(); b=POP(); PUSH( a + b ); break;
						case '*': a=POP(); b=POP(); PUSH( a * b ); break;
						case '-': a=POP(); b=POP(); PUSH( b - a ); break;
						case '/': a=POP(); b=POP(); PUSH( b / a ); break;

						default:
							assert( 0 );
								// Illegal instruction
					}
				}
				break;

			case NUMBER:
				if( *c == '.' ) {
					assert( decimal == 0.0 );
					decimal = 1.0;
				}
				if( *c >= '0' && *c <= '9' ) {
					if( decimal != 0.0 ) {
						decimal /= 10.0;
						constant += (*c - '0') * decimal;
					}
					else {
						constant *= 10.0;
						constant += *c - '0';
					}
				}
				if( *c == ' ' || *c == 0 ) {
					c--;
					PUSH( constant );
					state = START;
				}
				if( *c == 0 ) {
					state = DONE;
				}
				break;
		}
	}
	if( top > 0 ) {
		return POP();
	}
	return 0.f;
}

void GUIObject::place() {
	for( GUIObject *c = headChild; c; c=c->nextSibling ) {
		if( !c->getAttrI("hidden") && !c->getAttrI("nolayout") ) {
			char *l = c->getAttrS( "placeChildOption_l" );
			char *r = c->getAttrS( "placeChildOption_r" );
			char *t = c->getAttrS( "placeChildOption_t" );
			char *b = c->getAttrS( "placeChildOption_b" );
			char *w = c->getAttrS( "placeChildOption_w" );
			char *h = c->getAttrS( "placeChildOption_h" );

			// You must specify either both sides or one side and a dimension
			assert( (*l && *r && !*w) || (*l && !*r && *w) || (!*l && *r && *w) );
			assert( (*t && *b && !*h) || (*t && !*b && *h) || (!*t && *b && *h) );

			float _l = c->placeParseInstruction( l );
			float _t = c->placeParseInstruction( t );
			float _r = c->placeParseInstruction( r );
			float _b = c->placeParseInstruction( b );
			float _w = c->placeParseInstruction( w );
			float _h = c->placeParseInstruction( h );

			float __w = _w ? _w : _r - _l;
			float __h = _h ? _h : _t - _b;
			//assert( __w >= 0.f && __h >= 0.f );

			float x = *l ? _l : _r - __w;
			float y = *b ? _b : _t - __h;
			//assert( x >= 0.f && y >= 0.f );

			c->resize( __w, __h );
			c->move( x, y );
		}
	}
}

void GUIObject::postLayout() {
	setAttrS( "relayout", "1" );
	if( parent ) {
		parent->setAttrI( "relayout", 1 );
	}
}

void GUIObject::layout() {
//char *name = getAttrS("name");
//if( !stricmp(name,"_filt") ) {
//int a = 1;
//}
	if( getAttrI("layoutExpandToChildren") ) {
		float optW, optH;
		recurseQueryOptSize( optW, optH );
		myW = optW;
		myH = optH;
	}

	if( isAttrS( "layoutManager", "pack" ) ) {
		pack();
	}
	else if( isAttrS( "layoutManager", "table" ) ) {
		table();
	}
	else if( isAttrS( "layoutManager", "place" ) ) {
		place();
	}
	else {
		pack();
	}
	setAttrS( "relayout", "0" );
	zMsgQueue( "type=GUILayoutComplete toGUI=%s", getAttrS("name") );
}

GUIObject *GUIObject::exclusiveKeyboardRequestObject = NULL;
//GUIObject *GUIObject::exclusiveMouseRequestObject = NULL;
//int GUIObject::exclusiveMouseRequestAnywhere = 0;

int GUIObject::requestExclusiveMouse( int anywhere, int onOrOff ) {
//	if( onOrOff ) {
//		if( exclusiveMouseRequestObject ) {
//			// You can not have an exclusive lock.
//			return 0;
//		}
//		exclusiveMouseRequestAnywhere = anywhere;
//		exclusiveMouseRequestObject = this;
//	}
//	else {
//		if( exclusiveMouseRequestObject != this ) {
//			// You can not undo someone else's lock
//			return 0;
//		}
//		exclusiveMouseRequestAnywhere = 0;
//		exclusiveMouseRequestObject = NULL;
//	}

	if( onOrOff ) {
		int gotExclusive = zMouseMsgRequestExclusiveDrag( ZTmpStr("type=GUIExclusiveMouseDrag toGUI='%s'", getAttrS("name")) );
		setAttrI( "mouseExclusiveDrag", gotExclusive );
	}
	else {
		if( getAttrI( "mouseExclusiveDrag" ) ) {
			zMouseMsgCancelExclusiveDrag();
		}
	}
	return 1;
}

int GUIObject::requestExclusiveKeyboard( int onOrOff ) {
	if( onOrOff ) {
		if( exclusiveKeyboardRequestObject ) {
			// You can not have an exclusive lock.
			return 0;
		}
		zMsgQueue( "type=KeyboardFocus which=Gained toGUI='%s'", getAttrS( "name" ) );
		exclusiveKeyboardRequestObject = this;
	}
	else {
		if( exclusiveKeyboardRequestObject != this ) {
			// You can not undo someone else's lock
			return 0;
		}
		zMsgQueue( "type=KeyboardFocus which=Lost toGUI='%s'", getAttrS( "name" ) );
		exclusiveKeyboardRequestObject = NULL;
	}
	return 1;
}

void GUIObject::zMsgQueue( char *msg, ... ) {
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
			hash->putS( "fromGUI", getAttrS("name") );
			::zMsgQueue( hash );
			msgStart = c+1;
		}
	}

	if( msgStart && *msgStart ) {
		ZHashTable *hash = zHashTable( msgStart );
		hash->putS( "fromGUI", getAttrS("name") );
		::zMsgQueue( hash );
	}
}

void GUIObject::addHook( GUIObject *hook ) {
	char hookAddressAsStr[16];
	sprintf( hookAddressAsStr, "0x%X", hook );
	char *hooks = getAttrS( "hooks" );
	char * _hooks = (char *)alloca( strlen(hooks) + strlen(hookAddressAsStr) + 1 );
	strcpy( _hooks, hooks );
	strcat( _hooks, hookAddressAsStr );
	setAttrS( "hooks", _hooks );
}

void GUIObject::addHook( char *hook ) {
	char *hooks = getAttrS( "hooks" );
	char * _hooks = (char *)alloca( strlen(hooks) + strlen(hook) + 1 );
	strcpy( _hooks, hooks );
	strcat( _hooks, hook );
	setAttrS( "hooks", _hooks );
}

void GUIObject::handleMsg( ZMsg *msg ) {
	// PATTERN MATCH the current message against the sendMsgOn attribute
	char *head = boundMsgHead;
	while( head ) {
		char *next = *(char **)head;
		char *m = head + sizeof(char *);
		int count = *(int *)m;
		m += sizeof(int);
		int found = 1;
		for( int j=0; j<count; j++ ) {
			char *val = msg->getS( m );
			m += strlen( m ) + 1;
			if( !val ) {
				found = 0;
				break;
			}
			else {
				if( strcmp( val, m ) ) {
					found = 0;
					break;
				}
				m += strlen( m ) + 1;
			}
		}
		if( count && found ) {
			char fromGUI[256];
			sprintf( fromGUI, " fromGUI=%s fromBinding=1", getAttrS("name") );
			char *temp = (char *)alloca( strlen(m) + strlen(fromGUI) + 4 );
			strcpy( temp, m );
			strcat( temp, fromGUI );
			zMsgQueue( temp );
			zMsgUsed();
		}

		head = next;
	}

	if( zmsgIs(type,GUISetAttr) ) {
		char *key  = zmsgS(key);
		if( !*key ) {
			return;
		}

		char *val  = zmsgS(val);
		char _val[64];
		strcpy( _val, val );

		if( zmsgHas(toggle) ) {
			int toggle = getAttrI(key);
			toggle = !toggle;
			_val[0] = toggle ? '1' : '0';
			_val[1] = 0;
		}
		else if( zmsgHas(delta) ) {
			float v = getAttrF(key);
			v += zmsgF(delta);
			sprintf( _val, "%f", v );
		}
		else if( zmsgHas(scale) ) {
			float v = getAttrF(key);
			v *= zmsgF(scale);
			sprintf( _val, "%f", v );
		}

		if( zmsgHas(max) ) {
			float v = (float)atof( _val );
			v = min( zmsgF(max), v );
			sprintf( _val, "%f", v );
		}

		if( zmsgHas(min) ) {
			float v = (float)atof( _val );
			v = max( zmsgF(min), v );
			sprintf( _val, "%f", v );
		}

		setAttrS( key, _val );
	}
	else if( zmsgIs(type,GUIAddHook) ) {
		addHook( zmsgS(who) );
		// @TODO: Add ability to remove a hook
	}
	else if( zmsgIs(type,GUIHide) ) {
		setAttrI( "hidden", 1 );
	}
	else if( zmsgIs(type,GUIShow) ) {
		setAttrI( "hidden", 0 );
	}
	else if( zmsgIs(type,GUIMoveNear) ) {
		moveNear( zmsgS(who), zmsgS(where), zmsgF(x), zmsgF(y) );
	}
	else if( zmsgIs(type,GUILayoutNow) ) {
		layout();
	}
	else if( zmsgIs(type,GUILayout) ) {
		postLayout();
	}
	else if( zmsgIs(type,GUISetModal) ) {
		modal( zmsgI(val) );
	}
	else if( zmsgIs(type,GUIDie) ) {
		die();
	}
}

void GUIObject::setColorI( int color ) {
	if( (color&0x000000FF) != 0xFF ) {
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	}
	else {
		glDisable( GL_BLEND );
	}
	glColor4ub( (color&0xFF000000)>>24, (color&0x00FF0000)>>16, (color&0x0000FF00)>>8, color&0x000000FF );
}

// GUIPanel
//==================================================================================

GUI_TEMPLATE_IMPLEMENT(GUIPanel);

GUIPanel::GUIPanel() : GUIObject() {
	mouseOver = 0;
}

void GUIPanel::render() {
	int color = getAttrI( "panelColor" );
	if( !color ) color = getAttrI( "color" );
	if( mouseOver ) {
		int mouseOverColor = getAttrI( "mouseOverColor" );
		int mouseOverAlpha = getAttrI( "mouseOverAlpha" );
		if( !mouseOverColor ) {
			mouseOverColor = color;
			mouseOverColor &= 0xFFFFFF00;
			mouseOverColor |= mouseOverAlpha;
		}
		if( mouseOverAlpha ) {
			color = mouseOverColor;
		}
	}

	if( color ) {
		setColorI( color );
		glBegin( GL_QUADS );
			glVertex2f( 0.f, 0.f );
			glVertex2f( 0.f, myH );
			glVertex2f( myW, myH );
			glVertex2f( myW, 0.f );
		glEnd();
	}
}

void GUIPanel::handleMsg( ZMsg *msg ) {
	if( zmsgIs( type, MouseEnter ) ) {
		mouseOver = 1;
	}
	else if( zmsgIs( type, MouseLeave ) ) {
		mouseOver = 0;
	}
	if( zmsgIs(type,MouseClickOn) ) {
		mouseOver = 0;
	}
	GUIObject::handleMsg( msg );
}

// GUIButton
//==================================================================================

GUI_TEMPLATE_IMPLEMENT(GUIButton);

GUIButton::GUIButton() {
	pressed = 0;
}

void GUIButton::init() {
	GUIObject::init();
	addHook( this );
		// buttons hook their own attribute changes
		// so as to monitor the changes in selected
}

char *GUIButton::text() {
	char *textPtr = (char *)getAttrI("textPtr");
	if( textPtr ) return textPtr;
	return getAttrS( "text" );
}

int GUIButton::isSelected() {
	int s = getAttrI( "selected" );

	char *val = attributeTable->getS( "selectedPtr" );
	if( val ) {
		ZVarPtr *v = zVarsLookup( val );
		if( v ) {
			int _s = (int)( v->getDouble() );
//			if( _s != s ) {
//				setAttrI( "selected", s );
//			}
			return _s;
		}
	}

	int *ptr = (int *)getAttrI("selectedPtr");
	if( ptr ) {
		if( *ptr != s ) {
//			setAttrI( "selected", *ptr );
			s = *ptr;
		}
	}
	return s;
}

void GUIButton::setSelected( int s ) {
	setAttrI( "selected", s );
	int readOnly = getAttrI("selectedPtrReadOnly");
	char *val = attributeTable->getS( "selectedPtr" );
	if( val && !readOnly ) {
		ZVarPtr *v = zVarsLookup( val );
		if( v ) {
			v->setFromDouble( s );
			return;
		}
	}
	int *ptr = (int *)getAttrI("selectedPtr");
	if( ptr && !readOnly ) {
		*ptr = s;
	}
}

void GUIButton::render() {
	char *t = text();
	float curveW = min( 8.f, myW * 0.10f );

	int color = getAttrI("buttonColor");
	if( !color ) color = getAttrI("color");
	if( isSelected() ) color = getAttrI( "buttonSelectedColor" );
	if( pressed ) color = getAttrI( "buttonPressedColor" );

	setColorI( color );

	glBegin( GL_QUADS );
		glVertex2f( curveW, 0.f );
		glVertex2f( curveW, myH );
		glVertex2f( myW-curveW-1, myH );
		glVertex2f( myW-curveW-1, 0.f );
	glEnd();

	glBegin( GL_TRIANGLE_FAN );
		glVertex2f( curveW, myH/2.0f );
		glVertex2f( curveW, myH );
		glVertex2f( curveW/2.0f, myH-myH*0.1f );
		glVertex2f( curveW*0.1f, myH-myH*0.3f );
		glVertex2f( 0.0f, myH/2.0f );
		glVertex2f( curveW*0.1f, myH*0.3f );
		glVertex2f( curveW/2.0f, myH*0.1f );
		glVertex2f( curveW, 0.0f );
	glEnd();

	glBegin( GL_TRIANGLE_FAN );
		glVertex2f( myW-curveW-1.f, myH/2.0f );
		glVertex2f( myW-curveW-1.f, myH );
		glVertex2f( myW-curveW/2.0f-1.f, myH-myH*0.1f );
		glVertex2f( myW-curveW*0.1f-1.f, myH-myH*0.3f );
		glVertex2f( myW-0.0f-1.f, myH/2.0f );
		glVertex2f( myW-curveW*0.1f-1.f, myH*0.3f );
		glVertex2f( myW-curveW/2.0f-1.f, myH*0.1f );
		glVertex2f( myW-curveW-1.f, 0.0f );
	glEnd();

	double dwellTime = getAttrD("dwellTime");
	double enterTime = getAttrD("enterTime");
	if( dwellTime > 0.0 && enterTime > 0.0 ) {
		int _color = getAttrI( "buttonPressedColor" );
		if( !_color ) _color = color;
		setColorI( _color );
		float elpased = float( (guiTime - enterTime) / dwellTime );
		glBegin( GL_TRIANGLE_FAN );
		glVertex2f( myW/2.f, myH/2.f );
		float t = 0.f, tStep = elpased * 2.0f * 3.14159265f / 64.f;
		for( int i=0; i<=64; i++, t += tStep ) {
			glVertex2f( myW/2.f+myW/2.f*cosf(t), myH/2.f+myH/2.f*sinf(t) );
		}
		glEnd();
	}

	float winX=0.f, winY=0.f;
	getWindowCoords( winX, winY );
	glEnable(GL_SCISSOR_TEST);
	glScissor( (int)winX+2, (int)winY, (int)myW-6, (int)myH );

	float tw, th, td;
	printSize( t, tw, th, td );
	float kw=0.f, kh=0.f, kd=0.f;
	char key[64];
	char *s = getAttrS( "keyBinding" );
	char *d = &key[2];
	for( ; *s; s++ ) {
		if( *s != '.' ) {
			*d++ = *s;
		}
	}
	*d = 0;
	if( key[2] ) {
		key[0] = ' ';
		key[1] = '(';
		int len = strlen(key);
		key[len] = ')';
		key[len+1] = 0;
		assert( len < sizeof(key)-2 );
		printSize( key, kw, kh, kd );
	}
	float x = (myW-(tw+kw))/2.0f;
	float y = ((myH-th)/2.0f)-1.f;
	char textAlign = *getAttrS("textAlign");
	if( textAlign == 'L' ) {
		x = 4.f;
	}
	else if( textAlign == 'R' ) {
		x = myW-(tw+kw)-4.f;
	}

	int textColor = getAttrI("textColor");

	setColorI( textColor );
	print( t, x, y );

	if( key[1] ) {
		if( getAttrI("keyState") ) {
			setColorI( 0xFF0000FF );
		}
		else {
			setColorI( textColor );
		}
		print( key, x+tw, y );
	}

	if( pressed ) {
		setColorI( textColor );
		print( t, x+1.f, y );

		if( key[1] ) {
			print( key, x+1.f+tw, y );
		}
	}
}

void GUIButton::queryOptSize( float &w, float &h ) {
	assert( !headChild );
	float _d;
	printSize( text(), w, h, _d );
	w += 8;
	h += 8;

	float kw=0.f, kh=0.f, kd=0.f;
	char key[64];
	char *s = getAttrS( "keyBinding" );
	char *d = &key[2];
	for( ; *s; s++ ) {
		if( *s != '.' ) {
			*d++ = *s;
		}
	}
	*d = 0;
	if( key[2] ) {
		key[0] = ' ';
		key[1] = '(';
		int len = strlen(key);
		key[len] = ')';
		key[len+1] = 0;
		assert( len < sizeof(key)-2 );
		printSize( key, kw, kh, kd );
		w += kw;
	}
}

void GUIButton::handleMsg( ZMsg *msg ) {
	float dwellTime = getAttrF("dwellTime");
	if( dwellTime > 0.f ) {
		// This is a dwell type button which means that it is activated
		// by the mouse passing over it and is activated when the mouse
		// has dwelled over it for the given period.  It is useful
		// when a light source like a laser is spoofing the mouse
		// and you want to control UI

		if( zmsgIs(type,MouseEnter) ) {
			setAttrD( "enterTime", guiTime );
			zMsgQueue( "type=GUIButton_DwellCheck toGUI=%s", getAttrS("name") );
		}
		else if( zmsgIs(type,MouseClick) && zmsgIs(dir,U) ) {
			setAttrD( "enterTime", 0.0 );
		}
		else if( zmsgIs(type,MouseLeave) ) {
			setAttrD( "enterTime", 0.0 );
		}
		else if( zmsgIs(type,GUIButton_DwellCheck) ) {
			double enterTime = getAttrD( "enterTime" );
			if( enterTime > 0.0 ) {
				if( guiTime - enterTime > dwellTime ) {
					if( getAttrI( "toggle" ) ) {
						setSelected( !isSelected() );
					}
					zMsgQueue( "%s", getAttrS( "sendMsg" ) );
					setAttrD( "enterTime", 0.0 );
				}
				else {
					zMsgQueue( "type=GUIButton_DwellCheck toGUI=%s __countdown__=1", getAttrS("name") );
				}
			}
		}
		GUIObject::handleMsg( msg );
		return;
	}

	if( zmsgIs(type,MouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L) ) {
		zMsgUsed();
		requestExclusiveMouse( 0, 1 );
		pressed = 1;
		return;
	}
	else if( zmsgIs(type,MouseEnter) && zmsgI(dragging) ) {
		pressed = 1;
	}
	else if( zmsgIs(type,MouseLeave) && zmsgI(dragging) ) {
		pressed = 0;
	}
	else if( zmsgIs(type,MouseReleaseDrag) ) {
		if( pressed ) {
			if( getAttrI( "toggle" ) ) {
				setSelected( !isSelected() );
			}
			zMsgQueue( "%s", getAttrS( "sendMsg" ) );
		}
		pressed = 0;
		requestExclusiveMouse( 0, 0 );
	}
	else if( zmsgIs(type,GUIButton_SetSelectNoPropagate) ) {
		// This is a special message which means clear the select
		// and do propagate this message with a sendMsgOnUnselect
		// It is used when there are some buttons which act togther
		// and when one is pressed you want to clear the others
		// without getting into recursive override mess.
		attributeTable->putI( "selected", zmsgI(val) );
			// Not using setAttr here so that we don't issue a GUIAttrChanged message
	}
	else if( zmsgIs(type,GUIAttrChanged) && zmsgIs(key,selected) && !strcmp(zmsgS(fromGUI),getAttrS("name")) ) {
		if( isSelected() ) {
			zMsgQueue( "%s", getAttrS("sendMsgOnSelect") );
		}
		else {
			zMsgQueue( "%s", getAttrS("sendMsgOnUnselect") );
		}
	}
	else if( zmsgIs(type,GUIModalKeysClear) ) {
		setAttrI( "keyState", 0 );
	}
	else if( zmsgIs(type,Key) ) {
		if( hasAttr("keyBinding") ) {
			char *key = zmsgS( which );
			char *keyBinding = strdup( getAttrS( "keyBinding" ) );
			if( strchr( keyBinding, '.' ) ) {
				// When a partial selection has happened, for example, imagine
				// that the keybinding is "5-6".  When the "5" is pressed
				// the keyState is set to 1 to mean that the first level
				// has been selected.
				// This is accompanied by a call to the static GUI function
				// which tracks who has a pending binding.  The Gui dispatcher
				// will then only send Key events to those who have
				// a pending call.
				// All GUIs must release if they don't match

				// PARSE the keyBinding and match it to the key depending on the keyState
				int keyState = getAttrI("keyState");

				int found = 0;
				int count = 0;
				char *split = strtok( keyBinding, "." );
				while( split ) {
					if( count++ == keyState ) {
						if( !stricmp(split,key) ) {
							// We match this key, so bind to the modal key events
							found = 1;
						}
					}
					split = strtok( NULL, "." );
				}

				// At this point count tells us how many elems there were, for example "1-2" there are 2

				if( found ) {
					if( keyState == count-1 ) {
						// We are the matching signal, send message
						pressed = 1;
						zMsgQueue( "type=GUIButton_clearPressed toGUI=%s __countdown__=1", getAttrS("name") );
						if( getAttrI( "toggle" ) ) {
							setSelected( !isSelected() );
						}
						zMsgQueue( "%s", getAttrS( "sendMsg" ) );
						modalKeysClear();
						setAttrI( "keyState", 0 );
						zMsgUsed();
					}
					else {
						modalKeysBind( this );
						setAttrI( "keyState", keyState+1 );
					}
				}
				else {
					if( keyState > 0 ) {
						setAttrI( "keyState", 0 );
						modalKeysUnbind( this );
					}
				}

				free( keyBinding );
			}

			else if( !stricmp(keyBinding,key) ) {
				pressed = 1;
				zMsgQueue( "type=GUIButton_clearPressed toGUI=%s __countdown__=1", getAttrS("name") );
				if( getAttrI( "toggle" ) ) {
					setSelected( !isSelected() );
				}
				zMsgQueue( "%s", getAttrS( "sendMsg" ) );
			}
		}
	}
	else if( zmsgIs(type,GUIButton_clearPressed) ) {
		pressed = 0;
	}
	GUIObject::handleMsg( msg );
}

// GUIRadioButton
//==================================================================================

GUI_TEMPLATE_IMPLEMENT(GUIRadioButton);

void GUIRadioButton::init() {
	GUIObject::init();
	addHook( this );
		// buttons hook their own attribute changes
		// so as to monitor the changes in selected
}

char *GUIRadioButton::text() {
	char *textPtr = (char *)getAttrI("textPtr");
	if( textPtr ) {
		return (char *)textPtr;
	}
	return getAttrS( "text" );
}

void GUIRadioButton::render() {
	int selectedTextColor = hasAttr("selectedTextColor") ? getAttrI("selectedTextColor") : getAttrI("textColor");
	int unselectedTextColor = hasAttr("unselectedTextColor") ? getAttrI("unselectedTextColor") : getAttrI("textColor");

	float cenX = 5.f;
	float cenY = myH / 2.f;
	float rad = cenX;

	int selected = getAttrI( "selected" );
	if( selected ) {
		setColorI( selectedTextColor );
		glBegin( GL_TRIANGLE_FAN );
		glVertex2f( cenX, cenY );
	}
	else {
		setColorI( unselectedTextColor );
		glPointSize( 1.f );
		glBegin( GL_POINTS );
	}

	float t=0.f, td = 2.f*3.1415926539f/16.f;
	for( int i=0; i<=16; i++, t+=td ) {
		glVertex2f( cenX + rad*(float)cos(t), cenY + rad*(float)sin(t) );
	}

	glEnd();

	if( getAttrI("wordWrap") ) {
		printWordWrap( text(), 12.0, 0.0, myW, myH );
	}
	else {
		float w, h, d;
		printSize( "Q", w, h, d );
		print( text(), 12.0, myH-h );
	}
}

void GUIRadioButton::queryOptSize( float &w, float &h ) {
	float d;
	printSize( text(), w, h, d );
	w += 12;
	h -= d;
}

void GUIRadioButton::handleMsg( ZMsg *msg ) {
	if( zmsgIs(type,MouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L) ) {
		zMsgUsed();
		if( hasAttr("group") ) {
			zMsgQueue( "type=GUISetAttr key=selected val=1 toGUI='%s'", getAttrS("name") );
		}
	}
	else if( zmsgIs(type,GUIAttrChanged) && zmsgIs(key,selected) ) {
		if( getAttrI("selected") ) {
			zMsgQueue( "type=GUISetAttr key=selected val=0 toGUIGroup='%s' exceptToGUI='%s'", getAttrS("group"), getAttrS("name") );
			zMsgQueue( "%s", getAttrS("sendMsgOnSelect") );
		}
		else {
			zMsgQueue( "%s", getAttrS("sendMsgOnUnselect") );
		}
	}
	GUIObject::handleMsg( msg );
}

// GUITabButton
//==================================================================================

GUI_TEMPLATE_IMPLEMENT(GUITabButton);

void GUITabButton::render() {
	int selectedState = isSelected();

	float curveW = min( 8.f, myW * 0.10f );

	int color = getAttrI( "tabButtonColor" );
	if( !color ) color = getAttrI( "buttonColor" );
	if( !color ) color = getAttrI( "color" );
	if( selectedState ) {
		color = getAttrI( "buttonSelectedColor" );
	}

	setColorI( color );

	glBegin( GL_QUADS );
		glVertex2f( curveW, 0.f );
		glVertex2f( curveW, myH );
		glVertex2f( myW-curveW-1, myH );
		glVertex2f( myW-curveW-1, 0.f );
	glEnd();

	glBegin( GL_TRIANGLE_FAN );
		glVertex2f( curveW, myH/2.0f );
		glVertex2f( curveW, myH );
		glVertex2f( curveW/2.0f, myH-myH*0.1f );
		glVertex2f( curveW*0.1f, myH-myH*0.3f );
		glVertex2f( 0.0f, myH/2.0f );
	glEnd();

	glBegin( GL_TRIANGLE_FAN );
		glVertex2f( myW-curveW-1.f, myH/2.0f );
		glVertex2f( myW-curveW-1.f, myH );
		glVertex2f( myW-curveW/2.0f-1.f, myH-myH*0.1f );
		glVertex2f( myW-curveW*0.1f-1.f, myH-myH*0.3f );
		glVertex2f( myW-0.0f-1.f, myH/2.0f );
	glEnd();

	glBegin( GL_TRIANGLE_FAN );
		glVertex2f( curveW, 0.0f);
		glVertex2f( curveW, myH*0.5f);
		glVertex2f( 0.0f, myH*0.5f);
		glVertex2f( 0.0f, 0.0f );
	glEnd();

	glBegin( GL_TRIANGLE_FAN );
		glVertex2f( myW-curveW-1.f, 0.0f );
		glVertex2f( myW-curveW-1.f, myH*0.5f );
		glVertex2f( myW-0.0f-1.f, myH*0.5f );
		glVertex2f( myW-0.0f-1.f, 0.0f );
	glEnd();

	setColorI( getAttrI("textColor") );
	float w, h, d;
	char *t = text();
	printSize( t, w, h, d );
	print( t, (myW-w)/2.0f, ((myH-h-d)/2.0f) );
	if( selectedState ) {
		print( t, (myW-w)/2.0f+1.0f, ((myH-h-d)/2.0f) );
	}
}

void GUITabButton::handleMsg( ZMsg *msg ) {
	if( zmsgIs(type,GUITabButton_Select) || (zmsgIs(type,MouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L)) ) {
		zMsgQueue( "type=GUISetAttr key=selected val=0 toGUIGroup='%s'", getAttrS("group") );
		zMsgQueue( "type=GUISetAttr key=selected val=1 toGUI='%s'", getAttrS("name") );
		zMsgQueue( "%s", getAttrS( "sendMsg" ) );
		zMsgUsed();
		return;
	}
	GUIButton::handleMsg( msg );
}

// GUIText
//==================================================================================

GUI_TEMPLATE_IMPLEMENT(GUIText);

char *GUIText::text() {
	char *textPtr = (char *)getAttrI("textPtr");
	if( textPtr ) {
		return (char *)textPtr;
	}
	return getAttrS( "text" );
}

void GUIText::render() {
	GUIPanel::render();

	setColorI( getAttrI( "textColor" ) );
		
	if( getAttrI("wordWrap") ) {
		printWordWrap( text(), 0.0, 0.0, myW, myH );
	}
	else {
		float w, h, d;
		printSize( "Q", w, h, d );
		print( text(), 0.0, myH-h );
	}
}

void GUIText::queryOptSize( float &w, float &h ) {
	float d;
	printSize( text(), w, h, d );
}

// GUIHorizLine
//==================================================================================

GUI_TEMPLATE_IMPLEMENT(GUIHorizLine);

void GUIHorizLine::render() {
	int color = getAttrI( "horizLineColor" );
	if( !color ) color = getAttrI( "color" );
	setColorI( color );
	float thickness = getAttrF( "thickness" );
	thickness = max( thickness, 1.f );
	glBegin( GL_QUADS );
		glVertex2f( 0.f, 1.f );
		glVertex2f( myW, 1.f );
		glVertex2f( myW, 1.f+thickness );
		glVertex2f( 0.f, 1.f+thickness );
	glEnd();
}

void GUIHorizLine::queryOptSize( float &w, float &h ) {
	float thickness = getAttrF( "thickness" );
	thickness = max( thickness, 2.f );
	w = 1;
	h = thickness;
}

// GUIMenu
//==================================================================================

GUI_TEMPLATE_IMPLEMENT(GUIMenu);

void GUIMenu::init() {
	GUIObject::init();

	setAttrS( "*packChildOption_fill", "x" );
	setAttrI( "*packChildOption_padX", 4 );
	setAttrI( "*packChildOption_padY", 2 );
	setAttrI( "nolayout", 1 );
	setAttrS( "layoutManager", "pack" );
	setAttrS( "pack_side", "top" );
	setAttrI( "layoutExpandToChildren", 1 );
	setAttrI( "color", 0x907070c0 );
	setAttrI( "*mouseOverColor", 0x707090ff );
}

void GUIMenu::handleMsg( ZMsg *msg ) {
	if( getAttrI("modal") ) {
		if( zmsgIs(type,MouseClick) && !contains( zmsgF(localX), zmsgF(localY) ) ) {
			die();
			return;
		}
		if( zmsgIs(type,Key) ) {
			die();
			return;
		}
	}
	if( zmsgIs( type, MenuItemSelected ) && isMyChild( zmsgS(fromGUI) ) ) {
		zMsgQueue( "type=MenuItemSelected fromGUI=%s", getAttrS("name") );
		die();
		return;
	}
	else if( zmsgIs( type, Menu_ClearItems ) ) {
		killChildren();
	}
	else if( zmsgIs( type, Menu_AddItem ) ) {
		GUIMenuItem *mi = new GUIMenuItem();
		mi->init();
		mi->attachTo( this );
		mi->setAttrS( "text", zmsgS(label) );
		mi->setAttrS( "sendMsg", zmsgS(sendMsg) );
	}
	GUIPanel::handleMsg( msg );
}

// GUIMenuItem
//==================================================================================

GUI_TEMPLATE_IMPLEMENT(GUIMenuItem);

GUIMenuItem::GUIMenuItem() : GUIPanel() {
	mouseOver = 0;
}

void GUIMenuItem::init() {
	GUIPanel::init();
	setAttrS( "mouseRequest", "over" );
}

void GUIMenuItem::render() {
	if( mouseOver ) {
		int mouseOverColor = getAttrI("mouseOverColor");
		int parentColor = parent->getAttrI( "color" );
		int color = mouseOverColor ? mouseOverColor : parentColor | 0x000000FF;
		setAttrI( "color", color );
		GUIPanel::render();
	}
	else {
		setAttrI( "color", 0 );
		GUIPanel::render();
	}

	setColorI( getAttrI( "textColor" ) );

	float w, h, d;
	printSize( "Q", w, h, d );
	print( getAttrS("text"), 0.f, 0.f/*-d+1*/ );
}

void GUIMenuItem::queryOptSize( float &w, float &h ) {
	float d;
	printSize( getAttrS("text"), w, h, d );
//	h -= d;
//	h += 4;
}

void GUIMenuItem::handleMsg( ZMsg *msg ) {
	if( zmsgIs( type, MouseEnter ) ) {
		mouseOver = 1;
	}
	else if( zmsgIs( type, MouseLeave ) ) {
		mouseOver = 0;
	}

	if( zmsgIs(type,MouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L) ) {
		zMsgUsed();
		zMsgQueue( getAttrS( "sendMsg" ) );
		zMsgQueue( "type=MenuItemSelected fromGUI=%s", getAttrS("name") );
	}

	GUIPanel::handleMsg( msg );
}

// GUITextEditor
//==================================================================================

GUI_TEMPLATE_IMPLEMENT(GUITextEditor);

char *GUITextEditor::text() {
	char *textPtr = (char *)getAttrI("textPtr");
	if( textPtr ) return (char *)textPtr;
	return getAttrS( "text" );
}

int GUITextEditor::isEditting() {
	return hasAttr( "cursor" ) && getAttrI("cursor") >= 0;
}

void GUITextEditor::render() {
	float w, h, d, markBegW, markEndW, cursorW;

	GUIPanel::render();

	setColorI( getAttrI( "textColor" ) );
		
	if( getAttrI("wordWrap") ) {
		printWordWrap( text(), 0.0, 0.0, myW, myH );
	}
	else {
		printSize( "Q", w, h, d );
		print( text(), 0.0, myH-h-d );
	}

	if( !isEditting() ) {
		return;
	}

	int markBeg = getAttrI( "markBeg" );
	int markEnd = getAttrI( "markEnd" );
	int cursor  = getAttrI( "cursor" );
	char *t = text();
	int len = strlen( t );
	
	if( getAttrI( "focused" ) ) {
		assert( markBeg <= markEnd );
		char *temp = (char *)alloca( len+1 );
		strncpy( temp, t, markBeg );
		temp[markBeg] = 0;
		printSize( temp, markBegW, h, d );

		strncpy( temp, t, markEnd );
		temp[markEnd] = 0;
		printSize( temp, markEndW, h, d );

		strncpy( temp, t, cursor );
		temp[cursor] = 0;
		printSize( temp, cursorW, h, d );

		// DRAW the mark
		int color = getAttrI( "markColor" );
		if( !color ) color = 0xFFFFFF80;
		setColorI( color );

		glBegin( GL_QUADS );
			glVertex2f( markBegW, 0.f );
			glVertex2f( markBegW, myH );
			glVertex2f( markEndW, myH );
			glVertex2f( markEndW, 0.f );
		glEnd();

		// DRAW the blinking cursor 
		if( guiTime - lastBlinkTime > 0.15 ) {
			blinkState = !blinkState;
			lastBlinkTime = guiTime;
		}
		int alpha = blinkState ? 100 : 0;
		glColor4ub( 255, 255, 255, alpha );
		glBegin( GL_QUADS );
			glVertex2f( cursorW, 0.f );
			glVertex2f( cursorW, myH );
			glVertex2f( cursorW+2, myH );
			glVertex2f( cursorW+2, 0.f );
		glEnd();
		glDisable( GL_BLEND );
	}
}

void GUITextEditor::queryOptSize( float &w, float &h ) {
	float d;
	printSize( text(), w, h, d );
	h += d;
//	h += 4;
}

int GUITextEditor::textPosFromLocalX( float x ) {
	char *t = text();
	int len = strlen( t );
	char *temp = (char *)alloca( len+1 );

	int i;
	for( i=0; i<len; i++ ) {
		temp[i] = t[i];
		temp[i+1] = 0;
		float w, h, d;
		printSize( temp, w, h, d );
		if( w > x ) {
			return i;
		}
	}
	return i;
}

void GUITextEditor::handleMsg( ZMsg *msg ) {
	int markBeg = getAttrI( "markBeg" );
	int markEnd = getAttrI( "markEnd" );
	int cursor  = getAttrI( "cursor" );

	if( zmsgIs(type,KeyboardFocus) ) {
		setAttrI( "focused", zmsgIs(which,Gained) ? 1 : 0 );
	}
	else if( zmsgIs(type,MouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L) ) {
		zMsgUsed();
		requestExclusiveMouse( 1, 1 );
		requestExclusiveKeyboard( 1 );
		cursor = markBeg = markEnd = textPosFromLocalX( zmsgF(localX) );
		setAttrI( "markBeg", markBeg );
		setAttrI( "markEnd", markEnd );
		setAttrI( "cursor", cursor );
		setAttrI( "dragStart", cursor );
		zMsgQueue( "type=TextEditor_Begin fromGUI=%s", getAttrS("name") );
	}
	else if( zmsgIs(type,MouseClick) && isEditting() ) {
		if( !contains(zmsgF(localX),zmsgF(localY)) ) {
			requestExclusiveMouse( 1, 0 );
			requestExclusiveKeyboard( 0 );
			setAttrI( "cursor", -1 );
			ZMsg *_msg = new ZMsg;
			_msg->copyFrom( *msg );
			::zMsgQueue( _msg );
		}
	}
	else if( zmsgIs(type,MouseDrag) ) {
		int dragStart  = getAttrI( "dragStart" );
		int x = textPosFromLocalX( zmsgF(localX) );
		cursor = x;
		if( x > dragStart ) {
			markBeg = dragStart;
			markEnd = x;
		}
		else {
			markBeg = x;
			markEnd = dragStart;
		}
		setAttrI( "markBeg", markBeg );
		setAttrI( "markEnd", markEnd );
		setAttrI( "cursor", cursor );
	}
	else if( zmsgIs(type,MouseReleaseDrag) ) {
		requestExclusiveMouse( 1, 0 );
	}
	else if( zmsgIs(type,Key) ) {
		char *src = text();
		int len = strlen(src);
		char *dst = (char *)alloca( len + 2 );
		strcpy( dst, src );
		int which = zMsgTranslateCharacter( zmsgS(which) );

		markBeg = max( 0, min( len, markBeg ) );
		markEnd = max( 0, min( len, markEnd ) );
		int markCount = markEnd - markBeg;
		cursor  = max( 0, min( len, cursor  ) );

		if( zmsgIs(which,left) ) {
			if( zmsgI(shift) ) {
				if( cursor == markBeg ) {
					markBeg--;
				}
				else {
					markEnd--;
				}
				cursor--;
			}
			else {
				cursor--;
				markBeg = cursor;
				markEnd = cursor;
			}
		}
		else if( zmsgIs(which,right) ) {
			if( zmsgI(shift) ) {
				if( cursor == markEnd ) {
					markEnd++;
				}
				else {
					markBeg++;
				}
				cursor++;
			}
			else {
				cursor++;
				markBeg = cursor;
				markEnd = cursor;
			}
		}
		else if( zmsgIs(which,home) ) {
			if( zmsgI(shift) ) {
				if( cursor > markBeg ) {
					markEnd = markBeg;
				}
				cursor = 0;
				markBeg = 0;
			}
			else {
				cursor = 0;
				markBeg = 0; 
				markEnd = 0;
			}
 		}
		else if( zmsgIs(which,end) ) {
			if( zmsgI(shift) ) {
				if( cursor < markEnd ) {
					markBeg = markEnd;
				}
				cursor = strlen(text());
				markEnd = cursor;
			}
			else {
				cursor = strlen(text()); 
				markBeg = cursor;
				markEnd = markBeg;
			}
 		}
		else if( which == '\b' ) {
			if( markEnd > markBeg ) {
				if( markBeg > 0 ) {
					memcpy( dst, src, markBeg-1 );
				}
				memcpy( &dst[markBeg], &src[markEnd], len-markEnd );
				dst[markBeg + (len-markEnd)] = 0;
				len -= markEnd - markBeg;
				cursor = markBeg;
				markEnd = markBeg;
			}
			else if( cursor > 0 ) {
				memcpy( dst, src, cursor-1 );
				memcpy( &dst[cursor-1], &src[markEnd], len-markEnd );
				dst[markBeg-1 + (len-markEnd)] = 0;
				len -= markEnd - markBeg;
				cursor--;
				markBeg = cursor;
				markEnd = cursor;
			}
			else {
				strcpy( dst, src );
			}
		}
		else if( zmsgIs(which,delete) ) {
			if( markEnd == markBeg && len > 0 ) {
				memcpy( &dst[0], &src[0], markBeg );
				memcpy( &dst[markBeg], &src[markBeg+1], len-markBeg-1 );
				dst[len-1] = 0;
				len--;
			}
			else {
				if( markBeg > 0 ) {
					memcpy( dst, src, markBeg-1 );
				}
				memcpy( &dst[markBeg], &src[markEnd], len-markEnd );
				dst[markBeg + (len-markEnd)] = 0;
				len -= markEnd - markBeg;
				cursor = markBeg;
				markEnd = markBeg;
			}
		}
		else if( which == 27 ) {
			setAttrI( "cursor", -1 );
			requestExclusiveMouse( 1, 0 );
			requestExclusiveKeyboard( 0 );
			zMsgQueue( "type=TextEditor_Escape fromGUI=%s", getAttrS("name") );
			return;
		}
		else if( which == '\r' || which == '\n' ) {
			requestExclusiveMouse( 1, 0 );
			requestExclusiveKeyboard( 0 );
			setAttrI( "cursor", -1 );
			zMsgQueue( "type=TextEditor_Enter fromGUI=%s", getAttrS("name") );
			if( hasAttr("sendMsg") ) {
				zMsgQueue( "%s fromGUI=%s", getAttrS("sendMsg"), getAttrS("name") );
			}
			return;
		}
		else if( which >= ' ' && which < 127 ) {
			// Insert a plain-old key, deleteing the mark if non-zero length
			memcpy( dst, src, markBeg );
			dst[markBeg] = which;
			memcpy( &dst[markBeg + 1], &src[markEnd], len-markEnd );
			dst[markBeg + 1 + (len-markEnd)] = 0;
			markBeg++;
			markEnd = markBeg;
			len++;
			cursor = markBeg;
		}

		cursor  = max( 0, min( len, cursor ) );
		markBeg = max( 0, min( len, markBeg ) );
		markEnd = max( 0, min( len, markEnd ) );
		if( strcmp( dst, src ) ) {
			zMsgQueue( "type=TextEditor_TextChanged fromGUI=%s", getAttrS("name") );
		}
		setAttrS( "text", dst );
		setAttrI( "markBeg", markBeg );
		setAttrI( "markEnd", markEnd );
		setAttrI( "cursor",  cursor );
	}
	GUIPanel::handleMsg( msg );
}

// GUIPicker
//==================================================================================

GUI_TEMPLATE_IMPLEMENT(GUIPicker);

void GUIPicker::render() {
	GUIPanel::render();

	float lineH = zglFontGetLineH( getAttrS("font") );
	int scroll = getAttrI( "scroll" );

	glDisable( GL_BLEND );
	float y = myH - lineH;
	for( int extra=scroll; extra<0; extra++ ) {
		y -= lineH;
		if( y < 0 ) break;
	}

	int textColor = getAttrI( "textColor" );
	int selectedTextColor = getAttrI( "selectedTextColor" );
	int highlightedTextColor = getAttrI( "highlightedTextColor" );
	int mouseOver = getAttrI( "mouseOver" );
	int count = 0;
	for( int i=0; ; i++ ) {
		char *v = getAttrS( ZTmpStr( "key%d", i ) );

		int highlighted = count == mouseOver && mouseWasInside;

		if( 0 ) { }
		else if( count == getAttrI("selected") ) {
			setColorI( selectedTextColor );
		}
		else if( highlighted ) {
			setColorI( highlightedTextColor );
		}
		else {
			setColorI( textColor );
		}

		if( count >= scroll ) {
			print( v, 12, y );
			y -= lineH;
		}
		count++;

		if( y <= 0 ) {
			break;
		}
	}
}

void GUIPicker::queryOptSize( float &w, float &h ) {
	w = 100.f;
	h = 100.f;
	GUIObject::queryOptSize( w, h );
}

void GUIPicker::handleMsg( ZMsg *msg ) {
	float lineH = zglFontGetLineH( getAttrS("font") );

	int scroll = getAttrI( "scroll" );

	float x = zmsgF(localX);
	float y = zmsgF(localY);

	if( zmsgIs(type,Key) && zmsgIs(which,wheelforward) ) {
		if( contains(x,y) ) {
			scroll-=2;
			scroll = max( -10, scroll );
			//scroll = min( count-5, scroll );
			setAttrI( "scroll", scroll );
			int mouseOver = (int)( (myH-y) / lineH );
			mouseOver += scroll;
			setAttrI( "mouseOver", mouseOver );
			//mouseOver = selected > zVarsCount()-1 ? zVarsCount()-1 : mouseOver;
			zMsgUsed();
		}
	}
	else if( zmsgIs(type,Key) && zmsgIs(which,wheelbackward) ) {
		if( contains(x,y) ) {
			scroll+=2;
			scroll = max( -10, scroll );
			//scroll = min( count-5, scroll );
			setAttrI( "scroll", scroll );
			int mouseOver = (int)( (myH-y) / lineH );
			mouseOver += scroll;
			setAttrI( "mouseOver", mouseOver );
			//mouseOver = selected > zVarsCount()-1 ? zVarsCount()-1 : mouseOver;
			zMsgUsed();
		}
	}
	else if( zmsgIs(type,MouseMove) ) {
		int mouseOver = (int)( (myH-y) / lineH );
		mouseOver += scroll;
		setAttrI( "mouseOver", mouseOver );
		//mouseOver = selected > zVarsCount()-1 ? zVarsCount()-1 : mouseOver;
	}

	else if( zmsgIs(type,MouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L) ) {
		int selected = (int)( (myH-y) / lineH );
		selected += scroll;
		//selected = selected > zVarsCount()-1 ? zVarsCount()-1 : selected;
		setAttrI( "selected", selected );
		if( hasAttr("sendMsg") ) {
			char *key = getAttrS( ZTmpStr("key%d",selected) );
			zMsgQueue( "%s key=%s", getAttrS("sendMsg"), key );
		}
		zMsgUsed();
		return;
	}
	else if( zmsgIs(type,MouseReleaseDrag) ) {
		requestExclusiveMouse( 1, 0 );
		setAttrI( "selected", -1 );
		zMsgUsed();
		return;
	}
	else if( zmsgIs(type,MouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,R) ) {
		setAttrI( "selectX", (int)x );
		setAttrI( "selectY", (int)y );
		setAttrI( "startScroll", scroll );
		requestExclusiveMouse( 1, 1 );
		zMsgUsed();
		return;
	}
	else if( zmsgIs(type,MouseDrag) && zmsgI(r) ) {
		scroll = (int)( getAttrI("startScroll") + ( y - getAttrF("selectY") ) / lineH );
		scroll = max( -10, scroll );
		scroll = min( zVarsCount()-5, scroll );
		setAttrI( "scroll", scroll );
	}
	else if( zmsgIs(type,GUIPicker_Clear) ) {
		for( int i=0; ; i++ ) {
			ZTmpStr key( "key%d", i );
			char *v = getAttrS( key );
			if( v ) {
				attributeTable->del( key );
			}
			else {
				break;
			}
		}
	}
	GUIPanel::handleMsg( msg );
}

// GUI Execute
//==================================================================================

char *trimChars( char *str, char *chars ) {
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

char *trimQuotes( char *str ) {
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

void guiExecuteFile( char *filename, ZHashTable *args ) {
	FILE *f = fopen( filename, "rb" );
	if( f ) {
		fseek( f, 0, SEEK_END );
		int len = ftell( f );
		fseek( f, 0, SEEK_SET );
		char *buf = (char *)malloc( len+1 );
		fread( buf, len, 1, f );
		buf[len] = 0;
		fclose( f );
		guiExecute( buf, args );
		free( buf );
	}
}

void guiEscapeDecode( char *s ) {
	if( !s ) return;
	char *read = s;
	char *write = s;
	while( *read ) {
		if( *read == '\\' ) {
			switch( *(read+1) ) {
				case 'n': *write = '\n'; read++; break;
				case 't': *write = '\t'; read++; break;
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

void guiExecute( char *script, ZHashTable *args ) {
	static ZRegExp var1Test( "\\$\\((\\w+)\\)" );
	static ZRegExp var2Test( "\\$(\\w+)" );
	static ZRegExp comdTest( "^\\s*!\\s*(\\S+)\\s+([^\\n\\r\\t]*)" );
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

	ZStr *lines = zStrSplit( "\\n+", script );
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
	GUIObject *current = NULL;
	GUIObject *objStack[STACK_SIZE];

	char inGroup[80] = {0,};
	ZHashTable groups;
	GUIObject groupAccumulator;
	groupAccumulator.init();
		// This is just for accumulating attributes into a group block

	while( 1 ) {
		// While there is something on the include stack

		for( ; line; line=line->next ) {
			// Do variable substitutions

			char *a = (char *)*line; // simplify debug
			while( (char *)*line ) {
				if( var1Test.test( (char *)*line ) ) {
					char *subs = 0;
					if( args ) {
						subs = args->getS( var1Test.get(1) );
					}
					if( !stricmp("this",var1Test.get(1)) ) {
						subs = objStack[stackTop-1]->getAttrS("name");
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
						subs = objStack[stackTop-1]->getAttrS("name");
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

			if( inMultiline ) {
				if( comdTest.test( (char *)*line ) && !stricmp( comdTest.get( 1 ), "endmultiline" ) ) {
					objStack[stackTop-1]->setAttrS( multilineKey, multilineAccumulator );
					inMultiline = 0;
				}
				else {
					char *a = (char *)*line;
					if( multilineIgnoreWhitespace ) {
						a = trimChars( (char *)*line, " \t\r\n" );
					}
					guiEscapeDecode( a );
					strcat( multilineAccumulator, a );
					strcat( multilineAccumulator, " " );
				}
			}

			if( comOTest.test( (char *)*line ) ) {
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
					GUIObject *o = guiFind( obj );
					if( o ) {
						o->die();
					}
					guiGarbageCollect();
				}
				else if( !stricmp( action, "sendMsg" ) ) {
					char *msg = comdTest.get( 2 );
					zMsgQueue( msg );
				}
				else if( !stricmp( action, "sendMsgToThis" ) ) {
					char *msg = comdTest.get( 2 );
					zMsgQueue( "%s toGUI='%s'", msg, objStack[stackTop-1]->getAttrS("name") );
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
					guiExecuteFile( comdTest.get( 2 ), args );
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

					FILE *f = fopen( comdTest.get( 2 ), "rb" );
					if( f ) {
						fseek( f, 0, SEEK_END );
						int len = ftell( f );
						fseek( f, 0, SEEK_SET );
						char *buf = (char *)malloc( len+1 );
						fread( buf, len, 1, f );
						buf[len] = 0;
						fclose( f );

						ZStr *_lines = zStrSplit( "\\n+", buf );
						// Create a fake single line so that the
						// Line increment at the top of this loop will
						// skip over it and thus we will read the first line
						ZStr *stubLine = new ZStr( "" );
						stubLine->next = _lines;
						includeStackLines[includeStackDepth] = stubLine;
						line = stubLine;

						free( buf );
					}

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
					char *name = strdup( objStack[stackTop-1]->attributeTable->getS("name") );
					objStack[stackTop-1]->attributeTable->copyFrom( *hash );
					objStack[stackTop-1]->attributeTable->putS( "name", name );
					free( name );
				}
			}
			else if( nameTest.test( (char *)*line ) ) {
				// CREATE the object from a factory
				char *name = nameTest.get( 1 );
				char *type = nameTest.get( 2 );
				//if( !strcmp("calibWiz_nav3",name) ) {
				//int a = 1;
				//}
				if( type && *type ) {
					current = guiFactory( name, type );
					current->setAttrS( "layoutManager", "pack" );
				}
				else {
					current = guiFind( name );
					assert( current );
				}

				// ATTACH to parent
				if( stackTop > 0 ) {
					current->attachTo( objStack[stackTop-1] );
				}
				current->postLayout();
			}
			else if( qattTest.test( (char *)*line ) ) {
				// STUFF attributes into appropriate tables
				char *key = qattTest.get(1);
				char *val = qattTest.get(2);
				// TRIM leading and trailing spaces
				key = trimChars( key, " \t" );
				val = trimChars( val, " \t" );
				key = trimQuotes( key );
				val = trimQuotes( val );

				assert( stackTop > 0 );
				if( key[0] == '+' ) {
					objStack[stackTop-1]->setAttrS( &key[1], val );
					key[0] = '*';
					objStack[stackTop-1]->setAttrS( &key[0], val );
				}
				else {
					objStack[stackTop-1]->setAttrS( &key[0], val );
				}

				if( !strcmp(key,"parent") ) {
					GUIObject *p = guiFind( val );
					if( p ) {
						objStack[stackTop-1]->detachFrom();
						objStack[stackTop-1]->attachTo( p );
					}
				}
			}
			else if( attrTest.test( (char *)*line ) ) {
				// STUFF attributes into appropriate tables

				char *key = attrTest.get(1);
				char *val = attrTest.get(2);

				// TRIM leading and trailing spaces
				key = trimChars( key, " \t" );
				val = trimChars( val, " \t" );
				key = trimQuotes( key );
				val = trimQuotes( val );

				assert( stackTop > 0 );
				if( key[0] == '+' ) {
					objStack[stackTop-1]->setAttrS( &key[1], val );
					key[0] = '*';
					objStack[stackTop-1]->setAttrS( &key[0], val );
				}
				else {
					objStack[stackTop-1]->setAttrS( &key[0], val );
				}

				if( !strcmp(key,"parent") ) {
					GUIObject *p = guiFind( val );
					if( p ) {
						objStack[stackTop-1]->detachFrom();
						objStack[stackTop-1]->attachTo( p );
					}
				}
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
					dup->copyFrom( *groupAccumulator.attributeTable );
					groups.putI( inGroup, (int)dup );
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
		ZHashTable *val = (ZHashTable *)groups.getValI(i);
		if( val ) {
			delete val;
		}
	}
	groups.clear();

	zStrDelete( lines );
}


// GUI Factory Interface
//==================================================================================

ZHashTable *factoryTablePtrs = NULL;
ZHashTable *factoryTableSize = NULL;
	// These have to be dynamically allocated because
	// we can't ensure the startup order of the globals.
	// So we allocate them on demand in guiFactoryRegisterClass

void guiFactoryRegisterClass( void *ptr, int size, char *name ) {
	if( !factoryTablePtrs ) {
		factoryTablePtrs = new ZHashTable();
		factoryTableSize = new ZHashTable();
	}
	factoryTablePtrs->putI( name, (int)ptr );
	factoryTableSize->putI( name, size );
}

GUIObject *guiFactory( char *name, char *type ) {
	assert( factoryTablePtrs );

	int size = factoryTableSize->getI( type );
	GUIObject *templateCopy = (GUIObject *)factoryTablePtrs->getI( type );
	assert( size && templateCopy );

	GUIObject *o = (GUIObject *)malloc( size );
	memcpy( o, templateCopy, size );
	o->init();

	o->registerAs( name );
	return o;
}

GUIObject *guiFind( char *name ) {
	return (GUIObject *)guiNameToObjectTable.getI( name );	
}

// GUI Update
//==================================================================================

void guiRecurseUpdate( GUIObject *o ) {
	if( o->getAttrI( "relayout" ) ) {
		if( guiFind("root") == o ) {
			o->resize( (float)guiWinW, (float)guiWinH );
		}
		o->layout();
	}

	if( !o->getAttrI("hidden") || o->getAttrI("updateWhenHidden") ) {
		o->update();
		for( GUIObject *_o=o->headChild; _o; _o=_o->nextSibling ) {
			guiRecurseUpdate( _o );
		}
	}
}

int guiFrameCounter = 0;

void guiGarbageCollect() {
	// @TODO: Make a separate list the traversal would be faster
	// GARBAGE COLLECT any objects which are marked as dead
	int size = guiNameToObjectTable.size();
	for( int i=0; i<size; i++ ) {
		GUIObject *o = (GUIObject *)guiNameToObjectTable.getValI( i );
		if( o && o->isDead() ) {
			delete o;
		}
	}
}

void guiUpdate( double time ) {
	guiTime = time;
	guiFrameCounter++;

	guiGarbageCollect();

	GUIObject *root = guiFind( "root" );
	if( root ) {
		guiRecurseUpdate( root );
	}
}

// GUI Rendering
//==================================================================================

void guiRecurseRender( GUIObject *o ) {
	if( !o ) return;

	if( !o->getAttrI("hidden") ) {
		glMatrixMode( GL_MODELVIEW );
		glPushMatrix();

		glPushAttrib( GL_ALL_ATTRIB_BITS );
		glDisable( GL_LIGHTING );
		glDisable( GL_COLOR_MATERIAL );
		glDisable( GL_CULL_FACE );
		glDisable( GL_DEPTH_TEST );
		glDisable( GL_BLEND );
		glDisable( GL_TEXTURE_2D );
		glDisable( GL_FOG );

		glTranslatef( floorf(o->myX), floorf(o->myY), 0.0f );

		float scrollX = o->getAttrF( "scrollX" );
		float scrollY = o->getAttrF( "scrollY" );
		glTranslatef( scrollX, scrollY, 0.0 );

		int useScissor = !o->getAttrI("noScissor");
		if( useScissor ) {
			glEnable(GL_SCISSOR_TEST);
			float x=0.f, y=0.f;
			o->getWindowCoords( x, y );
			glScissor( (int)x-1, (int)y-1, (int)ceilf(o->myW)+2, (int)ceilf(o->myH)+2 );
		}
		
		o->render();
		
		if( useScissor ) {
			glDisable(GL_SCISSOR_TEST);
		}

		for( GUIObject *_o=o->headChild; _o; _o=_o->nextSibling ) {
			int depth1, depth2;
			glGetIntegerv( GL_MODELVIEW_STACK_DEPTH, &depth1 );
			
			guiRecurseRender( _o );

			glGetIntegerv( GL_MODELVIEW_STACK_DEPTH, &depth2 );
			assert( depth1 == depth2 );
		}

		glPopAttrib();

		glMatrixMode( GL_MODELVIEW );
		glPopMatrix();
	}
}

void guiRender( GUIObject *root ) {
	// Recursively render all of the gui objects
	if( !root ) root = guiFind( "root" );

	// TURN OFF all effects
	glPushAttrib( GL_ALL_ATTRIB_BITS );

    glDisable( GL_LIGHTING );
	glDisable( GL_COLOR_MATERIAL );
	glDisable( GL_CULL_FACE );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_TEXTURE_2D );
	glDisable( GL_FOG );

	// Reset both matricies to gives us simple 2D coordinates
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glPushMatrix();
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glPushMatrix();

	// MAKE the lower left 0, 0 and scale = 1.0 = 1pixel
	glTranslatef( -1.0f, -1.0f, 0.f );
	if( guiWinW != 0 && guiWinH != 0 ) {
		glScalef( 2.0f/(float)guiWinW, 2.0f/(float)guiWinH, 1.f );
	}

	guiRecurseRender( root );

	// Turn back on all selected attributes
	glPopAttrib();

	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();
}


// GUI Key
//==================================================================================

ZHashTable guiKeyBindings;

void guiBindKey( char *key, char *sendMsg ) {
	guiKeyBindings.putS( key, strdup(sendMsg) );
}


ZMSG_HANDLER( GUIBindKey ) {
	guiBindKey( zmsgS(key), zmsgS(sendMsg) );
}

// GUI Dispatching
//==================================================================================

void guiRecurseDispatch( GUIObject *o, ZMsg *msg ) {
	if( !o ) return;

	if( zmsgI(__sendDirect__) || !o->getAttrI("hidden") ) {
		float _x, _y;

		if( zmsgHas(x) && zmsgHas(y) ) {
			_x = zmsgF(x);
			_y = zmsgF(y);
			o->getLocalCoords( _x, _y );
			msg->putF( "localX", _x );
			msg->putF( "localY", _y );
			msg->putI( "on", o->contains(_x,_y) );
		}

		if( !zmsgI(__used__) ) {
			// ZBS added this if during a fix to the wheel response in ascan
			// where it would conitue to process.  I think it is the correct
			// behavior but seems like a dangerous fix

			// ZBS added the capture of other gui's handlers
			if( !zmsgI("__redirect_override__") && o->hasAttr("redirectMsgHandlerToGUI") ) {
				// This gui has it's handler redirected to another GUI
				char *otherGUI = o->getAttrS( "redirectMsgHandlerToGUI" );
				GUIObject *other = guiFind( otherGUI );
				if( other ) {
					msg->putS( "__redirected__", o->getAttrS("name") );
					other->handleMsg( msg );
					if( msg->getI("__redirect_override__") ) {
						// The redirection gui says that it should be handled by the default gui
						o->handleMsg( msg );
						int a = 1;
					}
				}
			}
			else {
				o->handleMsg( msg );
				int a = 1;
			}
		}

		// The handleMsg could have killed the object so bail on children if so.
		if( o->isDead() ) {
			return;
		}
		if( o && !zmsgI(__used__) ) {
			// ADD special GUI mouse messages: MouseClickOn, MouseEnter, MouseLeave, MouseReleaseDrag
			if( zmsgIs(type,MouseClick) ) {
				if( o->contains( _x, _y ) ) {
					o->mouseWasInside = 1;
					ZMsg temp;
					temp.copyFrom( *msg );
					temp.putS( "type", "MouseClickOn" );
					temp.putS( "__serial__", "-1" );
						// Force the handler into believing that the serial number is unique
					o->handleMsg( &temp );

					if( o->isDead() ) {
						return;
					}
					if( temp.getI("__used__") ) {
						zMsgUsed();
					}
				}
			}
			else if( zmsgIs(type,GUIMouseMove) || zmsgIs(type,GUIExclusiveMouseDrag) ) {
				ZMsg temp;
				temp.copyFrom( *msg );

				if( zmsgI(releaseDrag) ) {
					temp.putS( "type", "MouseReleaseDrag" );
				}
				else if( zmsgIs(type,GUIMouseMove) ) {
					temp.putS( "type", "MouseMove" );
				}
				else {
					temp.putS( "type", "MouseDrag" );
				}
				temp.putS( "__serial__", "-1" );
					// Force the handler into believing that the serial number is unique
				o->handleMsg( &temp );

				if( o->isDead() ) {
					return;
				}
				if( temp.getI("__used__") ) {
					zMsgUsed();
				}


				if( o->contains( _x, _y ) && !o->mouseWasInside ) {
					o->mouseWasInside = 1;
					ZMsg temp;
					temp.putS( "type", "MouseEnter" );
					temp.putI( "dragging", zmsgIs(type,GUIExclusiveMouseDrag) ? 1 : 0 );
					temp.putS( "__serial__", "-1" );
						// Force the handler into believing that the serial number is unique
					o->handleMsg( &temp );
				}
				else if( !o->contains( _x, _y ) && o->mouseWasInside ) {
					o->mouseWasInside = 0;
					ZMsg temp;
					temp.putS( "type", "MouseLeave" );
					temp.putI( "dragging", zmsgIs(type,GUIExclusiveMouseDrag) ? 1 : 0 );
					temp.putS( "__serial__", "-1" );
						// Force the handler into believing that the serial number is unique
					o->handleMsg( &temp );
				}
			}

			if( zmsgHas(__sendDirect__) ) {
				// Do not decend into recurstion if we are just sending
				// to this one GUIObject
				return;
			}

			// RECURSE otherwise
			if( o && !zmsgI(__used__) ) {
				GUIObject *tail;
				for( tail=o->headChild; tail && tail->nextSibling; tail=tail->nextSibling )
					;

				GUIObject *prev = NULL;
				for( GUIObject *_o=tail; _o; _o=prev ) {
					prev = _o->prevSibling;
						// We have to hold on to this because the call
						// to handleMsg could kill the object
					guiRecurseDispatch( _o, msg );
				}
			}
		}
	}
}

void guiDispatch( ZMsg *msg ) {
	// Messages that are directed to a specific GUIObject or
	// group are sent directly without a recursion and
	// without checking the deaf or hidden flags.
	// @TODO: get rid of deaf
	// Otherwise, they are sent recursively though the heirarchy
	// and the hidden flag is respected.

	if( zmsgI(__used__) ) {
		return;
	}

	char *toGUI = zmsgS(toGUI);
	if( *toGUI ) {
		GUIObject *dst = guiFind( toGUI );
		if( dst ) {
			msg->putI( "__sendDirect__", 1 );
			guiRecurseDispatch( dst, msg );
			// guiRecurseDispatch has a special rule that shortcuts 
			// the recursion and ignores "hidden" if __sendDirect__ set set.
		}
		return;
	}

	char *toGUIGroup = zmsgS(toGUIGroup);
	char *exceptToGUI = zmsgS(exceptToGUI);
	if( *toGUIGroup ) {
		// TRAVERSE all the objects and send to any that
		// have this group identification.
		msg->putI( "__sendDirect__", 1 );
		for( int i=0; i<guiNameToObjectTable.size(); i++ ) {
			char *key = guiNameToObjectTable.getKey( i );
			if( key ) {
				if( *exceptToGUI && !strcmp(exceptToGUI,key) ) {
					continue;
				}
				GUIObject *obj = (GUIObject *)guiNameToObjectTable.getValI( i );
				if( obj->isAttrS( "group", toGUIGroup ) ) {
					guiRecurseDispatch( obj, msg );
				}
			}
		}
		return;
	}

	// If it gets here it wasn't a toGUI or toGUIGroup
	// because it is so easy to forget, we enfore a rule
	// here that you can't send a GUISetAttr without a toGUI because
	// otherwise it sets the flag in every GUI in the world!
	assert( !zmsgIs(type,GUISetAttr) );

	if( GUIObject::modalStackTop > 0 ) {
		guiRecurseDispatch( GUIObject::modalStack[GUIObject::modalStackTop-1], msg );
		return;
	}

	// SPECIAL GUI Messages:
	//------------------------------------------------------
	if( zmsgIs(type,GUIOrderAbove) ) {
		GUIObject *toMoveGUI = guiFind( zmsgS(toMove) );
		GUIObject *referenceGUI = guiFind( zmsgS(reference) );
		if( toMoveGUI && referenceGUI ) {
			if( toMoveGUI->parent == referenceGUI->parent ) {
				toMoveGUI->parent->orderAbove( toMoveGUI, referenceGUI );
			}
		}
		return;
	}
	else if( zmsgIs(type,GUIOrderBelow) ) {
		GUIObject *toMoveGUI = guiFind( zmsgS(toMove) );
		GUIObject *referenceGUI = guiFind( zmsgS(reference) );
		if( toMoveGUI && referenceGUI ) {
			if( toMoveGUI->parent == referenceGUI->parent ) {
				toMoveGUI->parent->orderBelow( toMoveGUI, referenceGUI );
			}
		}
		return;
	}
	else if( zmsgIs(type,GUIExecuteFile) ) {
		char *file = zmsgS(file);
		if( file && *file ) {
			guiExecuteFile( file );
		}
		return;
	}
	else if( zmsgIs(type,Key) ) {
		if( GUIObject::exclusiveKeyboardRequestObject ) {
			GUIObject::exclusiveKeyboardRequestObject->handleMsg( msg );
			return;
		}
		else {
			if( headModalKeyBinding ) {
				GUIModalKeyBinding *next = 0;
				for( GUIModalKeyBinding *b = headModalKeyBinding; b; b=next ) {
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
				char *sendMsg = guiKeyBindings.getS( zmsgS(which), 0 );
				if( sendMsg ) {
					zMsgQueue( sendMsg );
					return;
				}
			}
		}
	}
//	else if( zmsgIs(type,MouseClick) ) {
//		if( GUIObject::exclusiveMouseRequestObject ) {
//			if( zmsgHas(x) && zmsgHas(y) ) {
//				float _x = zmsgI(x);
//				float _y = zmsgI(y);
//				GUIObject::exclusiveMouseRequestObject->getLocalCoords( _x, _y );
//				msg->putF( "localX", _x );
//				msg->putF( "localY", _y );
//			}
//			GUIObject::exclusiveMouseRequestObject->handleMsg( msg );
//			return;
//		}
//	}
		
	GUIObject *root = guiFind( "root" );
	guiRecurseDispatch( root, msg );
}

void guiReshape( int _winW, int _winH ) {
	guiWinW = _winW;
	guiWinH = _winH;
	GUIObject *root = guiFind( "root" );
	if( root ) {
		root->postLayout();
	}
}

// @TODO: These need to become function pointers to make this non-Windows dependent
void guiHideMouse() {
	#ifdef WIN32
	while( ShowCursor(0) >= -1 );
	while( ShowCursor(1) < -1 );
	#endif
}

void guiShowMouse() {
	#ifdef WIN32
	while( ShowCursor(1) <= 0 );
	while( ShowCursor(0) > 0 );
	#endif
}

void guiDieByRegExp( char *regExp ) {
	ZRegExp exp( regExp );
	int size = guiNameToObjectTable.size();
	for( int i=0; i<size; i++ ) {
		char *key = (char *)guiNameToObjectTable.getKey( i );
		GUIObject *val = (GUIObject *)guiNameToObjectTable.getValI( i );
		if( key && exp.test(key) ) {
			val->die();
		}
	}
}

