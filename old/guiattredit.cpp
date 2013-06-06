// @ZBS {
//		*COMPONENT_NAME guiarreedit
//		*MODULE_OWNER_NAME glgui
//		*GROUP modules/gui
//		*REQUIRED_FILES guiattredit.h
// }
// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#include "GL/gl.h"
// STDLIB includes:
#include "assert.h"
#include "math.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
// MODULE includes:
#include "guiattredit.h"
// ZBSLIB includes:
#include "ztmpstr.h"
#include "zregexp.h"

#ifdef __USE_GNU
#define stricmp strcasecmp
#endif

// GUIAttrEdit
//==================================================================================

GUI_TEMPLATE_IMPLEMENT(GUIAttrEdit);


void GUIAttrEdit::init() {
	GUIPanel::init();
	selected = -1;
	mouseOver = -1;
	optMenuUp = 0;
	linMode = 0;
	linIncrement = 1.0;
	setAttrS( "mouseRequest", "over" );
	setAttrI( "noScissor", 1 );
}

GUIAttrEdit::~GUIAttrEdit() {
	clearView();
}

void GUIAttrEdit::clearView() {
	for( int i=0; i<view.count; i++ ) {
		free( view.vec[i] );
	}
	view.clear();
}

static int viewNameCompare( const void *a, const void *b ) {
	return stricmp( *(char**)a, *(char**)b );
}

void GUIAttrEdit::sortViewByName() {
	qsort( view.vec, view.count, sizeof(char*), viewNameCompare );
}

void GUIAttrEdit::addVar( char *name ) {
	char *dup = strdup(name);
	view.add( dup );
}

void GUIAttrEdit::addAllVars() {
	ZHashTable *hash = (ZHashTable *)getAttrp( "hashPtr" );
	if( !hash ) return;
	ZHashWalk w( *hash );
	while( w.next() ) {
		char *dup = strdup(w.key);
		view.add( dup );
	}
}

void GUIAttrEdit::addVarsByRegExp( char *regexp ) {
	ZRegExp regExp( regexp );
	int last = -1;
	ZHashTable *hash = (ZHashTable *)getAttrp( "hashPtr" );
	if( !hash ) return;
	ZHashWalk w( *hash );
	while( w.next() ) {
		if( regExp.test( w.key ) ) {
			char *dup = strdup(w.key);
			view.add( dup );
		}
	}
}

float GUIAttrEdit::getLineH() {
	float w, h, d;
	printSize( "W", w, h, d );
	return h;
}

void GUIAttrEdit::render() {
	GUIPanel::render();

	float lineH = getLineH();
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

	ZHashTable *hash = (ZHashTable *)attributeTable->getp( "hashPtr" );
	if( !hash ) return;

	for( int i=0; i<view.count; i++ ) {
		int highlighted = (i == mouseOver && mouseWasInside);
	
		if( i == selected ) {
			setColorI( selectedTextColor );
		}
		else if( highlighted ) {
			setColorI( highlightedTextColor );
		}
		else {
			setColorI( textColor );
		}

		if( i >= scroll ) {
			char buf[1024];
			char *val = hash->getS( view[i] );
			sprintf( buf, "%s = %s", view[i], val );
			print( buf, 12, y );
			y -= lineH;
		}

		if( y <= 0 ) {
			break;
		}
	}

	if( !optMenuUp && selected != -1 ) {
		glDisable( GL_LINE_SMOOTH );
		glLineWidth( 1.f );
		glColor3ub( 0xff, 0, 0 );
		glBegin( GL_LINES );
			glVertex2f( selectX, selectY );
			glVertex2f( selectX, mouseY );

			float sign = mouseY > selectY ? -1.f : 1.f;
			glVertex2f( selectX, mouseY );
			glVertex2f( selectX-5, mouseY + sign*5 );

			glVertex2f( selectX, mouseY );
			glVertex2f( selectX+5, mouseY + sign*5 );
		glEnd();
	}
}

void GUIAttrEdit::createOptionMenu( ZHashTable *args ) {
	guiExecute( "\n\
		:varEditOptMenu = GUIPanel {\n\
			+panelColor = 0x70709080\n\
			+buttonColor = 0x907090e0\n\
			+buttonPressedColor = 0x707090FF\n\
			+horizLineColor = 0xFFFFFFFF\n\
			+textColor = 0xFFFFFFFF\n\
			nolayout = 1\n\
			layoutExpandToChildren = 1\n\
			parent = root\n\
			layoutManager = pack\n\
			layout_side = left\n\
			*packChildOption_padX = 1\n\
			: = GUIButton {\n\
				text = 'Done'\n\
				sendMsg = 'type=GUIAttrEdit_CreateOptionMenuDone toGUI=$who; type=GUIDie toGUI=varEditOptMenu'\n\
			}\n\
			: = GUIButton {\n\
				text = 'Reset'\n\
				sendMsg = 'type=SetVar key=$key reset=1'\n\
			}\n\
			: = GUIButton {\n\
				text = 'Neg'\n\
				sendMsg = 'type=SetVar key=$key scale=-1'\n\
			}\n\
			: = GUIButton {\n\
				text = '+'\n\
				sendMsg = 'type=SetVar key=$key delta=+1'\n\
			}\n\
			: = GUIButton {\n\
				text = '-'\n\
				sendMsg = 'type=SetVar key=$key delta=-1'\n\
			}\n\
			: = GUIButton {\n\
				text = '0'\n\
				sendMsg = 'type=SetVar key=$key val=0'\n\
			}\n\
			: = GUIButton {\n\
				text = '1'\n\
				sendMsg = 'type=SetVar key=$key val=1'\n\
			}\n\
			: = GUITextEditor {\n\
				text = $val\n\
				sendMsg = 'type=GUIAttrEdit_CreateOptionMenuDirectEntry toGUI=$who key=$key'\n\
			}\n\
			#@TODO: Add key binding to esacpe\n\
		}\n\
	", args);
}

void GUIAttrEdit::handleMsg( ZMsg *msg ) {
	float lineH = getLineH();

	int scroll = getAttrI( "scroll" );

	float x = zmsgF(localX);
	float y = zmsgF(localY);

	if( zmsgIs(type,Key) && zmsgIs(which,wheelforward) ) {
		if( contains(x,y) ) {
			scroll-=2;
			scroll = max( -10, scroll );
			scroll = min( view.count-5, scroll );
			setAttrI( "scroll", scroll );
			mouseOver = (int)( (myH-y) / lineH );
			mouseOver += scroll;
			mouseOver = selected > view.count-1 ? view.count-1 : mouseOver;
			zMsgUsed();
		}
	}
	else if( zmsgIs(type,Key) && zmsgIs(which,wheelbackward) ) {
		if( contains(x,y) ) {
			scroll+=2;
			scroll = max( -10, scroll );
			scroll = min( view.count-5, scroll );
			setAttrI( "scroll", scroll );
			mouseOver = (int)( (myH-y) / lineH );
			mouseOver += scroll;
			mouseOver = selected > view.count-1 ? view.count-1 : mouseOver;
			zMsgUsed();
		}
	}
	else if( zmsgIs(type,MouseMove) ) {
		mouseOver = (int)( (myH-y) / lineH );
		mouseOver += scroll;
		mouseOver = selected > view.count-1 ? view.count-1 : mouseOver;
	}

	else if( zmsgIs(type,MouseClickOn) && zmsgIs(dir,D) && zmsgI(ctrl) && zmsgI(shift) ) {
		//ADJUST the linIncrement
		if( zmsgIs(which,L) ) {
			linIncrement *= 2.f;
		}
		else if( zmsgIs(which,R) ) {
			linIncrement /= 2.f;
		}
	}
	else if( zmsgIs(type,MouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L) ) {
		selected = (int)( (myH-y) / lineH );
		selected += scroll;
		selected = selected > view.count-1 ? view.count-1 : selected;

		ZHashTable *hash = (ZHashTable *)getAttrp( "hashPtr" );
		if( !hash ) return;
		char *val = hash->getS( view[selected] );

		char *end;
		double valD = strtod( val, &end );
		if( end != val ) {
			// This is a number that can be manipulated

//			if( zmsgI(shift) && !zmsgI(ctrl) ) {
//				linMode = 0;
//				zMsgQueue( "type=GUIAttrEdit_CreateOptionMenu key=%s val=%lf toGUI=%s", selectVarPtr->name, selectVarPtr->getDouble(), getAttrS("name") );
//			}
//			else {
				if( zmsgI(ctrl) && !zmsgI(shift) ) {
					linMode = 1;
				}
				else {
					linMode = 0;
				}
				selectX = x;
				selectY = y;
				mouseX = x;
				mouseY = y;
				startVal = valD;
				requestExclusiveMouse( 1, 1 );
//			}
		}
		else {
			selected = -1;
		}


/*
		ZVarPtr *selectVarPtr = getSelectedVar( selected );
		if( selectVarPtr ) {
			if( zmsgI(shift) && !zmsgI(ctrl) ) {
				linMode = 0;
				zMsgQueue( "type=GUIAttrEdit_CreateOptionMenu key=%s val=%lf toGUI=%s", selectVarPtr->name, selectVarPtr->getDouble(), getAttrS("name") );
			}
			else {
				if( zmsgI(ctrl) && !zmsgI(shift) ) {
					linMode = 1;
				}
				else {
					linMode = 0;
				}
				selectX = x;
				selectY = y;
				mouseX = x;
				mouseY = y;
				switch( selectVarPtr->type ) {
					case zVarTypeINT:    startVal = (double)*(int    *)selectVarPtr->ptr + 0.4; break;
					case zVarTypeSHORT:  startVal = (double)*(short  *)selectVarPtr->ptr + 0.4; break;
					case zVarTypeFLOAT:  startVal = (double)*(float  *)selectVarPtr->ptr; break;
					case zVarTypeDOUBLE: startVal = (double)*(double *)selectVarPtr->ptr; break;
				}
				requestExclusiveMouse( 1, 1 );
			}
		}
*/
		zMsgUsed();
		return;
	}
	else if( zmsgIs(type,MouseReleaseDrag) ) {
		if( selected >= 0 ) {
//			ZVarPtr *selectVarPtr = getSelectedVar( selected );
//			if( selectVarPtr ) {
//				zMsgQueue( "type=GUIAttrEdit_VarDoneChange key=%s val=%lf", selectVarPtr->name, selectVarPtr->getDouble() );
//			}
		}

		requestExclusiveMouse( 1, 0 );
		selected = -1;
		zMsgUsed();
		return;
	}
	else if( zmsgIs(type,MouseDrag) && zmsgI(l) ) {
		int last = -1;
		int count = 0;
		//while( count++ != selected+1 && zVarsEnum( last, selectVarPtr, regExp ) );

		ZHashTable *hash = (ZHashTable *)getAttrp( "hashPtr" );
		double val = hash->getD( view[selected] );
		mouseX = x;
		mouseY = y;
		if( linMode ) {
			val = startVal + (mouseY-selectY) * linIncrement;
		}
		else{
			val = startVal * exp( (mouseY-selectY) / 50.0 );
		}
		hash->putS( view[selected], ZTmpStr("%4.3le",val) );
	}
	else if( zmsgIs(type,MouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,R) ) {
		selectX = x;
		selectY = y;
		startScroll = scroll;
		requestExclusiveMouse( 1, 1 );
		zMsgUsed();
		return;
	}
	else if( zmsgIs(type,MouseDrag) && zmsgI(r) ) {
		scroll = (int)( startScroll + ( y - selectY ) / lineH );
		scroll = max( -10, scroll );
		scroll = min( view.count-5, scroll );
		setAttrI( "scroll", scroll );
	}
	else if( zmsgIs(type,GUIAttrEdit_Clear) ) {
		clearView();
	}
//	else if( zmsgIs(type,GUIAttrEdit_Add) ) {
//		if( zmsgHas(regexp) ) {
//			addVarsByRegExp( zmsgS(regexp) );
//		}
//		else if( zmsgHas(name) ) {
//			addVar( zmsgS(name) );
//		}
//	}
	else if( zmsgIs(type,GUIAttrEdit_Sort) ) {
		if( zmsgIs(which,name) ) {
			sortViewByName();
		}
//		else if( zmsgIs(which,order) ) {
//			sortViewByDeclOrder();
//		}
	}
	else if( zmsgIs(type,GUIAttrEdit_CreateOptionMenu) ) {
		ZHashTable hash;
		hash.putS( "who", getAttrS("name") );
		hash.putS( "key", zmsgS(key) );
		hash.putS( "val", ZTmpStr("%1.2e", zmsgF(val)) );
		createOptionMenu( &hash );
		optMenuUp = 1;
		zMsgQueue( "type=GUILayout toGUI=varEditOptMenu" );
		zMsgQueue( "type=GUIMoveNear who=%s where=T x=4 y=%f toGUI=varEditOptMenu", getAttrS("name"), -(mouseOver-scroll)*lineH );
		zMsgQueue( "type=GUISetModal val=1 toGUI=varEditOptMenu" );
	}
	else if( zmsgIs(type,GUIAttrEdit_CreateOptionMenuDone) ) {
		optMenuUp = 0;
		selected = -1;
	}
	else if( zmsgIs(type,GUIAttrEdit_CreateOptionMenuDirectEntry) ) {
		GUIObject *obj = guiFind( zmsgS(fromGUI) );
		if( obj ) {
			zMsgQueue( "type=SetVar key=%s val=%f", zmsgS(key), atof(obj->getAttrS("text")) );
		}
	}
//	else if( zmsgIs(type,GUIAttrEdit_ResetAll) ) {
//		for( int i=0; i<view.count; i++ ) {
//			ZVarPtr *v = zVarsLookup( varsView.vec[i] );
//			v->resetDefault();
//		}
//	}
	else if( zmsgIs(type,GUIAttrEdit_Clear) ) {
		clearView();
	}
	else if( zmsgIs(type,GUIAttrEdit_Add) ) {
		addVar( zmsgS(key) );
	}
	else if( zmsgIs(type,GUIAttrEdit_ViewAll) ) {
		if( zmsgI(clear) ) {
			clearView();
		}
		addAllVars();
	}
	GUIPanel::handleMsg( msg );
}

//void GUIAttrEdit::queryOptSize( float &w, float &h ) {
//	//w = 200;
//	w = 0;
//
//	int last = -1;
//	char *name;
//	void *ptr;
//	int type;
//	while( zVarsEnum( last, name, ptr, type, regExp ) ) {
//		char *s;
//		switch( type ) {
//			case zVarTypeDOUBLE: s=ZTmpStr("%s = %3.2le", name, *(double *)ptr); break;
//			case zVarTypeFLOAT:  s=ZTmpStr("%s = %3.2e",  name, *(float *)ptr ); break;
//			case zVarTypeINT:    s=ZTmpStr("%s = %d",     name, *(int *)ptr   ); break;
//			case zVarTypeSHORT:  s=ZTmpStr("%s = %d",     name, *(short *)ptr ); break;
//		}
//		float _w = 0.f, _h = 0.f;
//		printSize( s, _w, _h );
//		w = max( w, _w );
//	}
//
//	int c = zVarsCount() + 1;
//	c = c > 30 ? 30 : c;
//	if( optHInLines > 0 ) {
//		c = optHInLines;
//	}
//	h = lineH * (float)c + lineH/2;
//}
