// @ZBS {
//		*COMPONENT_NAME zuivaredit
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuivaredit.cpp
// }

// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#ifdef __APPLE__
#include "OpenGL/gl.h"
#else
#include "GL/gl.h"
#endif
#include "zglfont.h"
// STDLIB includes:
#include "assert.h"
#include "math.h"
#include "string.h"
#include "stdlib.h"
// MODULE includes:
#include "zuivaredit.h"
// ZBSLIB includes:
#include "ztmpstr.h"
#include "zvars.h"
#include "zregexp.h"
#include "zplatform.h"
#include "zhashtable.h"


// ZUIVarEdit
//==================================================================================

ZUI_IMPLEMENT(ZUIVarEdit,ZUIPanel);


void ZUIVarEdit::factoryInit() {
	ZUIPanel::factoryInit();
	selected = -1;
	mouseOver = -1;
	optMenuUp = 0;
	linMode = 0;
	putS( "mouseRequest", "over" );
	putI( "noScissor", 1 );
}

void ZUIVarEdit::destruct() {
	clearView();
}

void ZUIVarEdit::clearView() {
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

void ZUIVarEdit::sortViewByName() {
	qsort( varsView.vec, varsView.count, sizeof(char*), varsViewNameCompare );
}

void ZUIVarEdit::sortViewByDeclOrder() {
	qsort( varsView.vec, varsView.count, sizeof(char*), varsViewDeclOrderCompare );
}

void ZUIVarEdit::addVar( char *name ) {
	char *dup = strdup(name);
	varsView.add( dup );
}

void ZUIVarEdit::addVarsByRegExp( char *regexp ) {
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

float ZUIVarEdit::getLineH() {
	float w, h, d;
	zglFontGetDim( "W", w, h, d, getS("font") );
	return h /*- d*/;
}

void ZUIVarEdit::render() {
	ZUIPanel::render();

	float lineH = getLineH();
	int scroll = getI( "scroll" );

	glDisable( GL_BLEND );
	float w, h;
	getWH( w, h );
	float y = h - lineH;

	for( int extra=scroll; extra<0; extra++ ) {
		y -= lineH;
		if( y < 0 ) break;
	}

	int inside = getI("mouseWasInside");

	int count = 0;
	for( int i=0; i<varsView.count; i++ ) {
		ZVarPtr *v = zVarsLookup( varsView.vec[i] );

		int highlighted = count == mouseOver && inside;
	
		if( count == selected ) {
			setupColor( "selectedTextColor" );
		}
		else {
			setupColor( "textColor" );
		}

		char *s;
		switch( v->type ) {
			case zVarTypeDOUBLE: s=ZTmpStr("%s = %3.2le", v->name, *(double *)v->ptr); break;
			case zVarTypeFLOAT:  s=ZTmpStr("%s = %3.2e",  v->name, *(float *)v->ptr ); break;
			case zVarTypeINT:    s=ZTmpStr("%s = %d",     v->name, *(int *)v->ptr   ); break;
			case zVarTypeSHORT:  s=ZTmpStr("%s = %d",     v->name, *(short *)v->ptr ); break;
		}

		if( count >= scroll ) {
			zglFontPrint( s, 12, y, getS("font") );
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

ZVarPtr *ZUIVarEdit::getSelectedVar( int selected ) {
	int count = 0;
	for( int i=0; i<varsView.count; i++ ) {
		if( count == selected ) {
			return zVarsLookup( varsView.vec[i] );
		}
		count++;
	}
	return 0;
}

void ZUIVarEdit::createOptionMenu( ZHashTable *args ) {
	zuiExecute( "\n\
		:varEditOptMenu = ZUIPanel {\n\
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
			: = ZUIButton {\n\
				text = 'Done'\n\
				sendMsg = 'type=GUIVarEdit_CreateOptionMenuDone toGUI=$who; type=GUIDie toGUI=varEditOptMenu'\n\
			}\n\
			: = ZUIButton {\n\
				text = 'Reset'\n\
				sendMsg = 'type=SetVar key=$key reset=1'\n\
			}\n\
			: = ZUIButton {\n\
				text = 'Neg'\n\
				sendMsg = 'type=SetVar key=$key scale=-1'\n\
			}\n\
			: = ZUIButton {\n\
				text = '+'\n\
				sendMsg = 'type=SetVar key=$key delta=+1'\n\
			}\n\
			: = ZUIButton {\n\
				text = '-'\n\
				sendMsg = 'type=SetVar key=$key delta=-1'\n\
			}\n\
			: = ZUIButton {\n\
				text = '0'\n\
				sendMsg = 'type=SetVar key=$key val=0'\n\
			}\n\
			: = ZUIButton {\n\
				text = '1'\n\
				sendMsg = 'type=SetVar key=$key val=1'\n\
			}\n\
			: = ZUITextEditor {\n\
				text = $val\n\
				sendMsg = 'type=ZUIVarEdit_CreateOptionMenuDirectEntry toZUI=$who key=$key'\n\
			}\n\
			#@TODO: Add key binding to esacpe\n\
		}\n\
	", args);
}

void ZUIVarEdit::handleMsg( ZMsg *msg ) {
	float lineH = getLineH();

	int scroll = getI( "scroll" );

	float x = zmsgF(localX);
	float y = zmsgF(localY);
	float w, h;
	getWH( w, h );

	if( zmsgIs(type,Key) && zmsgIs(which,wheelforward) ) {
		if( containsLocal(x,y) ) {
			scroll-=2;
			scroll = max( -10, scroll );
			scroll = min( zVarsCount()-5, scroll );
			putI( "scroll", scroll );
			mouseOver = (int)( (h-y) / lineH );
			mouseOver += scroll;
			mouseOver = selected > zVarsCount()-1 ? zVarsCount()-1 : mouseOver;
			zMsgUsed();
		}
	}
	else if( zmsgIs(type,Key) && zmsgIs(which,wheelbackward) ) {
		if( containsLocal(x,y) ) {
			scroll+=2;
			scroll = max( -10, scroll );
			scroll = min( zVarsCount()-5, scroll );
			putI( "scroll", scroll );
			mouseOver = (int)( (h-y) / lineH );
			mouseOver += scroll;
			mouseOver = selected > zVarsCount()-1 ? zVarsCount()-1 : mouseOver;
			zMsgUsed();
		}
	}
	else if( zmsgIs(type,ZUIMouseMove) ) {
		mouseOver = (int)( (h-y) / lineH );
		mouseOver += scroll;
		mouseOver = selected > zVarsCount()-1 ? zVarsCount()-1 : mouseOver;
	}

	else if( zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) && zmsgI(ctrl) && zmsgI(shift) ) {
		//ADJUST the linIncrement

		int _selected = (int)( (h-y) / lineH );
		_selected += scroll;
		_selected = _selected > zVarsCount()-1 ? zVarsCount()-1 : _selected;
		ZVarPtr *selectVarPtr = getSelectedVar( _selected );
		if( selectVarPtr ) {
			if( zmsgIs(which,L) ) {
				selectVarPtr->linIncrement *= 2.f;
			}
			else if( zmsgIs(which,R) ) {
				selectVarPtr->linIncrement /= 2.f;
			}
		}
	}
	else if( zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L) ) {
		selected = (int)( (h-y) / lineH );
		selected += scroll;
		selected = selected > zVarsCount()-1 ? zVarsCount()-1 : selected;
		ZVarPtr *selectVarPtr = getSelectedVar( selected );
		if( selectVarPtr ) {
			if( zmsgI(shift) && !zmsgI(ctrl) ) {
				linMode = 0;
				zMsgQueue( "type=ZUIVarEdit_CreateOptionMenu key=%s val=%lf toZUI=%s", selectVarPtr->name, selectVarPtr->getDouble(), name );
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
	else if( zmsgIs(type,ZUIMouseReleaseDrag) ) {
		if( selected >= 0 ) {
			ZVarPtr *selectVarPtr = getSelectedVar( selected );
			if( selectVarPtr ) {
				zMsgQueue( "type=ZUIVarEdit_VarDoneChange key=%s val=%lf", selectVarPtr->name, selectVarPtr->getDouble() );
			}
		}

		requestExclusiveMouse( 1, 0 );
		selected = -1;
		zMsgUsed();
		return;
	}
	else if( zmsgIs(type,ZUIExclusiveMouseDrag) && zmsgI(l) ) {
		int last = -1;
		int count = 0;
		//while( count++ != selected+1 && zVarsEnum( last, selectVarPtr, regExp ) );
		ZVarPtr *selectVarPtr = getSelectedVar( selected );
		mouseX = x;
		mouseY = y;
		double val;
		if( selectVarPtr ) {
			// If this variable requests linear mode or the global linear mode is set, do things linearly.
			int _linMode = selectVarPtr->linearTwiddleMode || linMode;
			if( _linMode ) {
				val = startVal + (mouseY-selectY) * selectVarPtr->linIncrement;
			}
			else{
				val = startVal * exp( (mouseY-selectY) / selectVarPtr->expIncrement );
			}
			if( has("sendMsg") ) {
				zMsgQueue( getS("sendMsg" ) );
			}
			else {
				zMsgQueue( "type=ZUIVarEdit_VarChanging key=%s val=%lf", selectVarPtr->name, selectVarPtr->getDouble() );
			}
			selectVarPtr->setFromDouble( val );
		}
	}
	else if( zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,R) ) {
		selectX = x;
		selectY = y;
		startScroll = scroll;
		requestExclusiveMouse( 1, 1 );
		zMsgUsed();
		return;
	}
	else if( zmsgIs(type,ZUIExclusiveMouseDrag) && zmsgI(r) ) {
		scroll = (int)( startScroll + ( y - selectY ) / lineH );
		scroll = max( -10, scroll );
		scroll = min( zVarsCount()-5, scroll );
		putI( "scroll", scroll );
	}
	else if( zmsgIs(type,ZUIVarEdit_Clear) ) {
		clearView();
	}
	else if( zmsgIs(type,ZUIVarEdit_Add) ) {
		if( zmsgHas(regexp) ) {
			addVarsByRegExp( zmsgS(regexp) );
		}
		else if( zmsgHas(name) ) {
			addVar( zmsgS(name) );
		}
	}
	else if( zmsgIs(type,ZUIVarEdit_Sort) ) {
		if( zmsgIs(which,name) ) {
			sortViewByName();
		}
		else if( zmsgIs(which,order) ) {
			sortViewByDeclOrder();
		}
	}
	else if( zmsgIs(type,ZUIVarEdit_CreateOptionMenu) ) {
		ZHashTable hash;
		hash.putS( "who", name );
		hash.putS( "key", zmsgS(key) );
		hash.putS( "val", ZTmpStr("%1.2e", zmsgF(val)) );
		createOptionMenu( &hash );
		optMenuUp = 1;
		zMsgQueue( "type=ZUIMoveNear who=%s where=T x=4 y=%f toGUI=varEditOptMenu", name, -(mouseOver-scroll)*lineH );
		zMsgQueue( "type=ZUISetModal val=1 toZUI=varEditOptMenu" );
	}
	else if( zmsgIs(type,ZUIVarEdit_CreateOptionMenuDone) ) {
		optMenuUp = 0;
		selected = -1;
	}
	else if( zmsgIs(type,ZUIVarEdit_CreateOptionMenuDirectEntry) ) {
		ZUI *obj = zuiFindByName( zmsgS(fromZUI) );
		if( obj ) {
			zMsgQueue( "type=SetVar key=%s val=%f", zmsgS(key), atof(obj->getS("text")) );
		}
	}
	else if( zmsgIs(type,ZUIVarEdit_ResetAll) ) {
		for( int i=0; i<varsView.count; i++ ) {
			ZVarPtr *v = zVarsLookup( varsView.vec[i] );
			v->resetDefault();
		}
	}
	ZUIPanel::handleMsg( msg );
}
