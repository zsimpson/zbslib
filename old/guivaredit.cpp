// @ZBS {
//		*COMPONENT_NAME guivaredit
//		*MODULE_OWNER_NAME glgui
//		*GROUP modules/gui
//		*REQUIRED_FILES guivaredit.h
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
// MODULE includes:
#include "guivaredit.h"
// ZBSLIB includes:
#include "ztmpstr.h"
#include "zvars.h"
#include "zregexp.h"

#ifdef __USE_GNU
#define stricmp strcasecmp
#endif

// GUIVarEdit
//==================================================================================

GUI_TEMPLATE_IMPLEMENT(GUIVarEdit);


void GUIVarEdit::init() {
	GUIPanel::init();
	selected = -1;
	mouseOver = -1;
	optMenuUp = 0;
	linMode = 0;
	linIncrement = 1.0;
	setAttrS( "mouseRequest", "over" );
	setAttrI( "noScissor", 1 );
}

GUIVarEdit::~GUIVarEdit() {
	clearView();
}

void GUIVarEdit::clearView() {
	for( int i=0; i<varsView.count; i++ ) {
		free( varsView.vec[i] );
	}
	varsView.clear();
}

int varsViewNameCompare(const void *a, const void *b) {
	return stricmp( *(char**)a, *(char**)b );
}

int varsViewDeclOrderCompare(const void *a, const void *b) {
	ZVarPtr *_a = zVarsLookup( *(char **)a );
	ZVarPtr *_b = zVarsLookup( *(char **)b );
	return _a->serialNumber > _b->serialNumber ? 1 : (_a->serialNumber < _b->serialNumber ? -1 : 0);
}

void GUIVarEdit::sortViewByName() {
	qsort( varsView.vec, varsView.count, sizeof(char*), varsViewNameCompare );
}

void GUIVarEdit::sortViewByDeclOrder() {
	qsort( varsView.vec, varsView.count, sizeof(char*), varsViewDeclOrderCompare );
}

void GUIVarEdit::addVar( char *name ) {
	char *dup = strdup(name);
	varsView.add( dup );
}

void GUIVarEdit::addVarsByRegExp( char *regexp ) {
	ZRegExp regExp( regexp );
	int last = -1;
	ZVarPtr *v;
	while( zVarsEnum( last, v ) ) {
		if( regExp.test( v->name ) ) {
			char *dup = strdup(v->name);
			varsView.add( dup );
		}
	}
}

float GUIVarEdit::getLineH() {
	float w, h, d;
	printSize( "W", w, h, d );
	return h /*- d*/;
}

void GUIVarEdit::render() {
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

	int count = 0;
	for( int i=0; i<varsView.count; i++ ) {
		ZVarPtr *v = zVarsLookup( varsView.vec[i] );

		int highlighted = count == mouseOver && mouseWasInside;
	
		if( count == selected ) {
			setColorI( selectedTextColor );
		}
		else if( highlighted ) {
			setColorI( highlightedTextColor );
		}
		else {
			setColorI( textColor );
		}

		char *s;
		switch( v->type ) {
			case zVarTypeDOUBLE: s=ZTmpStr("%s = %3.2le", v->name, *(double *)v->ptr); break;
			case zVarTypeFLOAT:  s=ZTmpStr("%s = %3.2e",  v->name, *(float *)v->ptr ); break;
			case zVarTypeINT:    s=ZTmpStr("%s = %d",     v->name, *(int *)v->ptr   ); break;
			case zVarTypeSHORT:  s=ZTmpStr("%s = %d",     v->name, *(short *)v->ptr ); break;
		}

		if( count >= scroll ) {
			print( s, 12, y );
			y -= lineH;
		}
		count++;

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

ZVarPtr *GUIVarEdit::getSelectedVar( int selected ) {
	int count = 0;
	for( int i=0; i<varsView.count; i++ ) {
		if( count == selected ) {
			return zVarsLookup( varsView.vec[i] );
		}
		count++;
	}
	return 0;
}

void GUIVarEdit::createOptionMenu( ZHashTable *args ) {
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
				sendMsg = 'type=GUIVarEdit_CreateOptionMenuDone toGUI=$who; type=GUIDie toGUI=varEditOptMenu'\n\
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
				sendMsg = 'type=GUIVarEdit_CreateOptionMenuDirectEntry toGUI=$who key=$key'\n\
			}\n\
			#@TODO: Add key binding to esacpe\n\
		}\n\
	", args);
}

void GUIVarEdit::handleMsg( ZMsg *msg ) {
	float lineH = getLineH();

	int scroll = getAttrI( "scroll" );

	float x = zmsgF(localX);
	float y = zmsgF(localY);

	if( zmsgIs(type,Key) && zmsgIs(which,wheelforward) ) {
		if( contains(x,y) ) {
			scroll-=2;
			scroll = max( -10, scroll );
			scroll = min( zVarsCount()-5, scroll );
			setAttrI( "scroll", scroll );
			mouseOver = (int)( (myH-y) / lineH );
			mouseOver += scroll;
			mouseOver = selected > zVarsCount()-1 ? zVarsCount()-1 : mouseOver;
			zMsgUsed();
		}
	}
	else if( zmsgIs(type,Key) && zmsgIs(which,wheelbackward) ) {
		if( contains(x,y) ) {
			scroll+=2;
			scroll = max( -10, scroll );
			scroll = min( zVarsCount()-5, scroll );
			setAttrI( "scroll", scroll );
			mouseOver = (int)( (myH-y) / lineH );
			mouseOver += scroll;
			mouseOver = selected > zVarsCount()-1 ? zVarsCount()-1 : mouseOver;
			zMsgUsed();
		}
	}
	else if( zmsgIs(type,MouseMove) ) {
		mouseOver = (int)( (myH-y) / lineH );
		mouseOver += scroll;
		mouseOver = selected > zVarsCount()-1 ? zVarsCount()-1 : mouseOver;
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
		selected = selected > zVarsCount()-1 ? zVarsCount()-1 : selected;
		ZVarPtr *selectVarPtr = getSelectedVar( selected );
		if( selectVarPtr ) {
			if( zmsgI(shift) && !zmsgI(ctrl) ) {
				linMode = 0;
				zMsgQueue( "type=GUIVarEdit_CreateOptionMenu key=%s val=%lf toGUI=%s", selectVarPtr->name, selectVarPtr->getDouble(), getAttrS("name") );
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
		zMsgUsed();
		return;
	}
	else if( zmsgIs(type,MouseReleaseDrag) ) {
		if( selected >= 0 ) {
			ZVarPtr *selectVarPtr = getSelectedVar( selected );
			if( selectVarPtr ) {
				zMsgQueue( "type=GUIVarEdit_VarDoneChange key=%s val=%lf", selectVarPtr->name, selectVarPtr->getDouble() );
			}
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
		ZVarPtr *selectVarPtr = getSelectedVar( selected );
		mouseX = x;
		mouseY = y;
		double val;
		if( linMode ) {
			val = startVal + (mouseY-selectY) * linIncrement;
		}
		else{
			val = startVal * exp( (mouseY-selectY) / 50.0 );
		}
		if( selectVarPtr ) {
			zMsgQueue( "type=GUIVarEdit_VarChanging key=%s val=%lf", selectVarPtr->name, selectVarPtr->getDouble() );
			selectVarPtr->setFromDouble( val );
		}
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
		scroll = min( zVarsCount()-5, scroll );
		setAttrI( "scroll", scroll );
	}
	else if( zmsgIs(type,GUIVarEdit_Clear) ) {
		clearView();
	}
	else if( zmsgIs(type,GUIVarEdit_Add) ) {
		if( zmsgHas(regexp) ) {
			addVarsByRegExp( zmsgS(regexp) );
		}
		else if( zmsgHas(name) ) {
			addVar( zmsgS(name) );
		}
	}
	else if( zmsgIs(type,GUIVarEdit_Sort) ) {
		if( zmsgIs(which,name) ) {
			sortViewByName();
		}
		else if( zmsgIs(which,order) ) {
			sortViewByDeclOrder();
		}
	}
	else if( zmsgIs(type,GUIVarEdit_CreateOptionMenu) ) {
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
	else if( zmsgIs(type,GUIVarEdit_CreateOptionMenuDone) ) {
		optMenuUp = 0;
		selected = -1;
	}
	else if( zmsgIs(type,GUIVarEdit_CreateOptionMenuDirectEntry) ) {
		GUIObject *obj = guiFind( zmsgS(fromGUI) );
		if( obj ) {
			zMsgQueue( "type=SetVar key=%s val=%f", zmsgS(key), atof(obj->getAttrS("text")) );
		}
	}
	else if( zmsgIs(type,GUIVarEdit_ResetAll) ) {
		for( int i=0; i<varsView.count; i++ ) {
			ZVarPtr *v = zVarsLookup( varsView.vec[i] );
			v->resetDefault();
		}
	}
	GUIPanel::handleMsg( msg );
}

//void GUIVarEdit::queryOptSize( float &w, float &h ) {
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
