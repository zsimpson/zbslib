// @ZBS {
//		*COMPONENT_NAME guitextlist
//		*MODULE_OWNER_NAME glgui
//		*GROUP modules/gui
//		*REQUIRED_FILES guitextlist.h
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
#include "guitextlist.h"
// ZBSLIB includes:
#include "ztmpstr.h"

#ifdef __USE_GNU
#define stricmp strcasecmp
#endif

// GUITextList
//==================================================================================

GUI_TEMPLATE_IMPLEMENT(GUITextList);

void GUITextList::init() {
	GUIPanel::init();
	mouseOver = -1;
	setAttrS( "mouseRequest", "over" );
	setAttrI( "noScissor", 1 );
}

GUITextList::~GUITextList() {
	clearView();
}

void GUITextList::clearView() {
	int i;
	for( i=0; i<lines.count; i++ ) {
		free( lines.vec[i] );
	}
	lines.clear();

	for( i=0; i<sorted.count; i++ ) {
		free( sorted.vec[i] );
	}
	sorted.clear();
}

static int nameCompare(const void *a, const void *b) {
	return stricmp( *(char**)a, *(char**)b );
}

void GUITextList::sortByName() {
	int i;
	for( i=0; i<sorted.count; i++ ) {
		free( sorted.vec[i] );
	}
	sorted.clear();

	for( i=0; i<lines.count; i++ ) {
		char *l = strdup( lines[i] );
		sorted.add( l );
	}

	qsort( sorted.vec, sorted.count, sizeof(char*), nameCompare );
	useSorted = 1;
}

void GUITextList::sortByOrder() {
	useSorted = 0;
}

void GUITextList::addLine( char *name ) {
	int len = strlen( name );
	char *dup = (char *)malloc( len + 2 );
		// +1 for the nul terminator and another +1 for the selection flag
	*dup = 0;
	strcpy( dup+1, name );
	lines.add( dup );
}

float GUITextList::getLineH() {
	float w, h, d;
	printSize( "W", w, h, d );
	return h;
}

void GUITextList::render() {
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
	for( int i=0; i<lines.count; i++ ) {
		char *l = lines[i];
		int highlighted = count == mouseOver && mouseWasInside;
	
		if( highlighted ) {
			setColorI( highlightedTextColor );
		}
		else if( l[0] ) {
			setColorI( selectedTextColor );
		}
		else {
			setColorI( textColor );
		}

		if( count >= scroll ) {
			print( l+1, 12, y );
				// +1 because the first byte contains the selection flag
			y -= lineH;
		}
		count++;

		if( y <= 0 ) {
			break;
		}
	}
}

void GUITextList::handleMsg( ZMsg *msg ) {
	float lineH = getLineH();

	int scroll = getAttrI( "scroll" );

	float x = zmsgF(localX);
	float y = zmsgF(localY);

	if( zmsgIs(type,Key) && zmsgIs(which,wheelforward) ) {
		if( contains(x,y) ) {
			scroll-=2;
			scroll = max( -10, scroll );
			scroll = min( lines.count-5, scroll );
			setAttrI( "scroll", scroll );
			mouseOver = (int)( (myH-y) / lineH );
			mouseOver += scroll;
			mouseOver = max( 0, min( lines.count-1, mouseOver ) );
			zMsgUsed();
		}
	}
	else if( zmsgIs(type,Key) && zmsgIs(which,wheelbackward) ) {
		if( contains(x,y) ) {
			scroll+=2;
			scroll = max( -10, scroll );
			scroll = min( lines.count-5, scroll );
			setAttrI( "scroll", scroll );
			mouseOver = (int)( (myH-y) / lineH );
			mouseOver += scroll;
			mouseOver = max( 0, min( lines.count-1, mouseOver ) );
			zMsgUsed();
		}
	}
	else if( zmsgIs(type,MouseMove) ) {
		mouseOver = (int)( (myH-y) / lineH );
		mouseOver += scroll;
	}
	else if( zmsgIs(type,MouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L) ) {
		selected = (int)( (myH-y) / lineH );
		selected += scroll;
		selected = max( 0, min( lines.count-1, selected ) );
		requestExclusiveMouse( 1, 1 );
		zMsgUsed();
		return;
	}
	else if( zmsgIs(type,MouseReleaseDrag) && zmsgIs(which,L) ) {
		int _selected = (int)( (myH-y) / lineH );
		_selected += scroll;
		_selected = max( 0, min( lines.count-1, _selected ) );
		if( selected == _selected ) {
			if( ! getAttrI("multiselect") ) {
				// CLEAR all of the other entries
				for( int i=0; i<lines.count; i++ ) {
					if( lines[i][0] ) {
						zMsgQueue( "%s __line__='%s'", getAttrS("sendMsgOnUnselect"), &lines[i][1] );
					}
					lines[i][0] = 0;
				}
			}
			lines[selected][0] = !lines[selected][0];
			if( lines[selected][0] ) {
				zMsgQueue( "%s __line__='%s'", getAttrS("sendMsgOnSelect"), &lines[selected][1] );
			}
			else {
				zMsgQueue( "%s __line__='%s'", getAttrS("sendMsgOnUnselect"), &lines[selected][1] );
			}
		}
		requestExclusiveMouse( 1, 0 );
		zMsgUsed();
		return;
	}
	else if( zmsgIs(type,MouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,R) ) {
		selectX = (int)x;
		selectY = (int)y;
		startScroll = scroll;
		requestExclusiveMouse( 1, 1 );
		zMsgUsed();
		return;
	}
	else if( zmsgIs(type,MouseReleaseDrag) && zmsgIs(which,R) ) {
		requestExclusiveMouse( 1, 0 );
		zMsgUsed();
		return;
	}
	else if( zmsgIs(type,MouseDrag) && zmsgI(r) ) {
		scroll = (int)( startScroll + ( y - selectY ) / lineH );
		scroll = max( -10, scroll );
		scroll = min( lines.count-5, scroll );
		setAttrI( "scroll", scroll );
	}
	else if( zmsgIs(type,GUITextList_Clear) ) {
		clearView();
	}
	else if( zmsgIs(type,GUITextList_Add) ) {
		if( zmsgHas(line) ) {
			addLine( zmsgS(line) );
		}
	}
	else if( zmsgIs(type,GUITextList_Sort) ) {
		if( zmsgIs(which,name) ) {
			sortByName();
		}
		else if( zmsgIs(which,order) ) {
			sortByOrder();
		}
	}
	GUIPanel::handleMsg( msg );
}

