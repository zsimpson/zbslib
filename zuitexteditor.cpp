// @ZBS {
//		*COMPONENT_NAME zuitexteditor
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuitexteditor.cpp
// }


// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#ifdef __APPLE__
#include "OpenGL/gl.h"
#else
#include "GL/gl.h"
#endif
// STDLIB includes:
#include "assert.h"
#include "string.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"
#include "ctype.h"
#ifdef WIN32
#include "malloc.h"
#endif
// MODULE includes:
#include "zui.h"
#include "zuitexteditor.h"
// ZBSLIB includes:
#include "zglfont.h"
#include "zmsg.h"
#include "zvars.h"
#include "zplatform.h"

// ZUITextEditor
//--------------------------------------------------------------------------------------------------------------------------------

ZUI_IMPLEMENT(ZUITextEditor,ZUIPanel);

void ZUITextEditor::factoryInit() {
	lines = 0;
	putI( "scaleAlphaMouseOver", 1 );
	putI( "restoreTextOnCancel", 1 );
	putF( "viewWindowPixelX", 0.f );
	putF( "viewWindowPixelY", 0.f );
	putI("disabled",0);
	setUpdate( 1 );
}

char *ZUITextEditor::text() {
//	char *textPtr = (char *)getI("textPtr");
//	if( textPtr ) return (char *)textPtr;
	char *t = getS( "text" );
	return t ? t : (char *)"";
}

int ZUITextEditor::isEditting() {
//	return has( "cursor" ) && getI("cursor") >= 0;
	return getI("focused");
}

void ZUITextEditor::getWH( float &w, float &h ) {
	ZUI::getWH( w, h );
	if( getI("multiline") ) {
		w = max( 0, w );
	}
}

void ZUITextEditor::getXYWH( float &x, float &y, float &w, float &h ) {
	ZUI::getXY( x, y );
	ZUI::getWH( w, h );
	if( getI("multiline") ) {
		w = max( 0, w );
	}
}

void ZUITextEditor::linesUpdate() {
	// ALLOCATE a local buffer for 16K lines.  A buffer of the right size will be allocated when the true count is known.
	#define TEMP_LINES_COUNT (1024*16)
	int *_lines = (int*)alloca( sizeof(int) * TEMP_LINES_COUNT * 2 );
	assert(_lines);
	lineCount = 0;

	// PARSE the text and find all the lines
	void *fontPtr = zglFontGet( getS("font") );

	float w, h, d;
	float myW, myH;
	getWH( myW, myH );

	char *textStart = text();

	if( ! getI( "multiline" ) ) {
		if( lines ) {
			free( lines );
		}
		lines = (int *)malloc( sizeof(int) * 3 );
		lineCount = 1;
		lines[0] = 0;
		lines[1] = strlen(textStart);
		return;
	}
	else {
		float lineW = 0.f;
		char *lastSpace = 0;
		char *lastStartLine = textStart;
		for( char *c = lastStartLine; ; c++ ) {
			char _c = *c;
			char *newLine = 0;

			if( _c == 0 ) {
				newLine = c;
			}
			else if( _c == '\n' || _c == '\r' ) {
				if( _c=='\r' && *(c+1)=='\n' ) {
					c++;
				}
				newLine = c+1;
			}
			else if( _c >= ' ' && _c < 127 ) {
				if( _c == ' ' ) {
					lastSpace = c;
				}
				zglFontGetDimChar( _c, w, h, d, 0, fontPtr );
				lineW += w;
				if( lineW >= myW ) {
					if( lastSpace ) {
						newLine = lastSpace+1;
					}
					else {
						if( lastStartLine == c ) {
							newLine = c+1;
						}
						else {
							newLine = c;
						}
					}
				}
			}

			if( newLine ) {
				_lines[lineCount*2+0] = lastStartLine - textStart;
				_lines[lineCount*2+1] = newLine - lastStartLine;
				lineCount++;
				assert( lineCount*2+1 < TEMP_LINES_COUNT );
				lastStartLine = newLine;
				lastSpace = 0;
				lineW = 0.f;
				c = newLine-1;
			}
			if( _c == 0 ) {
				break;
			}
		}

		if( lines ) {
			free( lines );
		}
		lines = (int*)malloc( sizeof(int) * (lineCount*2) );
		memcpy( lines, _lines, sizeof(int) * lineCount*2 );
	}
}

void ZUITextEditor::strPosToLineAndChar( int strPos, int &line, int &charX ) {
	strPos = max( 0, strPos );
	strPos = min( (int)strlen( text() ), strPos );

	for( line=0; line<lineCount; line++ ) {
		if( lines[line*2] <= strPos && strPos < lines[line*2]+lines[line*2+1] ) {
			charX = strPos - lines[line*2];
			return;
		}
	}

	// Past the end
	line = lineCount - 1;
	charX = lines[line*2+1];
}

int ZUITextEditor::lineAndCharToStrPos( int line, int charX ) {
	assert( line >= 0 && line < lineCount );

	int lineOff = lines[line*2+0];
	int lineLen = lines[line*2+1];

	// DEAL with charX being negative
	while( charX < 0 && line > 0 ) {
		// Go up a line and to the end
		line--;
		charX += lines[line*2+1];
	}

	// DEAL with charX being past end of line
	while( charX >= lines[line*2+1] && line < lineCount-1 ) {
		// Go down a line and to the beginning
		charX -= lines[line*2+1];
		line++;
	}

	// End of file boundary
	if( line == lineCount-1 ) {
		charX = min( charX, lines[line*2+1] );
	}
	if( line == 0 ) {
		charX = max( 0, charX );
	}

	assert( line >= 0 && line < lineCount && charX >= 0 && charX <= lines[line*2+1] );
	return lines[line*2+0]+charX;
}

const int borderSize = 3;

int ZUITextEditor::boundStrPos( int strPos ) {
	strPos = max( 0, strPos );
	strPos = min( (int)strlen( text() ), strPos );
	return strPos;
}

void ZUITextEditor::getViewWH( float &_w, float &_h ) {
	_w = w - 2*borderSize;
	_h = h - 2*borderSize;
}

void ZUITextEditor::strPosToPixelXY( int strPos, float &x, float &y ) {
	// Note that this does NOT do any bound checking

	float myW, myH;
	getViewWH( myW, myH );

	strPos = boundStrPos( strPos );

	int line, charX;
	strPosToLineAndChar( strPos, line, charX );

	float _w, _h, _d;
	char *textStart = text();
	printSize( &textStart[ lines[line*2] ], _w, _h, _d, charX );

	x = _w - getF( "viewWindowPixelX" );
	y = float(line)*_h - getF( "viewWindowPixelY" );
}

int ZUITextEditor::strPosFromPixelXY( float x, float y ) {
	// Returns a bounded strPos

	float myW, myH;
	getViewWH( myW, myH );

	x += getF( "viewWindowPixelX" );
	y += getF( "viewWindowPixelY" );

	void *fontPtr = zglFontGet( getS("font") );
	assert( fontPtr );

	float lineW, lineH, lineD;
	zglFontGetDimChar( 'Q', lineW, lineH, lineD, 0, fontPtr );

	// BOUND check line
	int line = (int)( y / lineH );
	if( line < 0 ) {
		return 0;
	}
	if( line > lineCount-1 ) {
		return (int)strlen( text() );
	}

	// FETCH the strpos on line at pos x by measuring line widths
	assert( line >= 0 && line < lineCount );
	char *textStart = text();
	char *lineText = &textStart[ lines[line*2+0] ];
	int lineLen = lines[line*2+1];

	float last = 0.f;
	for( int i=0; i<lineLen; i++ ) {
		float lineW, lineH, lineD;
		zglFontGetDim( lineText, lineW, lineH, lineD, 0, fontPtr, i );
		if( lineW >= x ) {
			// DETERMINE which is closer, previous character boundry or this one
			if( last > 0.f && x-last < lineW-x ) {
				// Last was closer
				return lines[line*2] + i - 1;
			}
			else {
				// Current is closer
				return lines[line*2] + i;
			}
		}
		last = lineW;
	}

	// We went past the end of the line so return
	// the last character on this line
	return lines[line*2+0] + lines[line*2+1];
}

void ZUITextEditor::cursorMove( int deltaX, int deltaY ) {
	float viewW, viewH;
	getViewWH( viewW, viewH );
	if( viewW <= 0.f || viewH <= 0.f ) {
		return;
	}

	int cursor = getI( "cursor" );

	// ADVANCE the vertical in character space
	int line, charX;
	strPosToLineAndChar( cursor, line, charX );
	line += deltaY;
	line = max( 0, min( lineCount-1, line ) );

	// ADVANCE the horizontal in character space
	charX += deltaX;
//	charX = max( 0, min( (int)strlen(text()), charX ) );

	cursor = lineAndCharToStrPos( line, charX );
	putI( "cursor", cursor );

	// Because charX could be neattive the last call could have changed the line and charX
	// therefore we have to fetch them again
	strPosToLineAndChar( cursor, line, charX );

	// DETERMINE if we need to change the view window
	// by looking at the pixel coords of the new cursor
	float x, y;
	strPosToPixelXY( cursor, x, y );
	if( x > viewW ) {
		putF( "viewWindowPixelX", getF( "viewWindowPixelX" ) + x-viewW + 4.f );
			// 4.f so that the cursor will be visible
	}
	else if( x < 0.f ) {
		float toMove;
		if( charX > 0 ) {
			// This is the case that you're backspace
			toMove = min( viewW/2.f, getF( "viewWindowPixelX" ) );
		}
		else {
			// This is the home case, trying to get to the beginning
			toMove = getF( "viewWindowPixelX" );
		}
		putF( "viewWindowPixelX", getF( "viewWindowPixelX" ) - toMove );
	}

	// Add code for scrolling y
	float lineW, lineH, lineD;
	printSize( "Q", lineW, lineH, lineD );
	float viewWindowPixelY = getF( "viewWindowPixelY" );
	float currYExtent = (line+1) * lineH - viewWindowPixelY;
	
	if( currYExtent > viewH ) {
		// INCREASE scroll
		viewWindowPixelY += currYExtent - viewH; 
	}
	else if ( currYExtent < lineH ) {
		// DECREASE scroll
		viewWindowPixelY = line * lineH; 
		viewWindowPixelY = max( 0, viewWindowPixelY );
	}

	putF( "viewWindowPixelY", viewWindowPixelY ); 

	// FETCH the pixel coords for the cursor so that render will have them
	strPosToPixelXY( cursor, x, y );
	putF( "cursorY", y );
	putF( "cursorX", x );
}

void ZUITextEditor::markPosUpdate( int prevCursor, int changing ) {
	int cursor = getI( "cursor" );
	int markBeg = getI( "markBeg" );
	int markEnd = getI( "markEnd" );
	int markStart = getI( "markStart" );

	if( !changing ) {
		markBeg = markEnd = cursor;
		putI( "markStart", -1 );
	}
	else {
		if( !has("markStart") || markStart == -1 ) {
			markStart = prevCursor;
			putI( "markStart", markStart );
		}
		if( cursor <= markStart ) {
			markBeg = cursor;
			markEnd = markStart;
		}
		if( cursor >= markStart ) {
			markBeg = markStart;
			markEnd = cursor;
		}
	}

	putI( "markBeg", markBeg );
	putI( "markEnd", markEnd );

	// UPDATE the markXY variables
	float x, y;
	strPosToPixelXY( markBeg, x, y );
	putF( "markBegX", x );
	putF( "markBegY", y );
	strPosToPixelXY( markEnd, x, y );
	putF( "markEndX", x );
	putF( "markEndY", y );
}

void ZUITextEditor::update() {
	if( isEditting() ) {
		if( zuiTime - lastBlinkTime > 0.15 ) {
			blinkState = !blinkState;
			lastBlinkTime = zuiTime;
			dirty();
		}
	}
}

void ZUITextEditor::render() {

	float myW, myH;
	getWH( myW, myH );		

	if( !lines ) {
		linesUpdate();
	}

	ZUIPanel::render();

	glPushMatrix();
	glTranslatef( (float)borderSize, -(float)borderSize, 0.f );
		// This is to make sure we don't render on top of the zuipanel border

	int noScissor = getI( "noScissor" );
		// because I need to be able to clip text editors to a larger area in some places. (tfb)
		// we'll also need to do with when using the dirty rect system.
	if( !noScissor ) {
		glPushAttrib( GL_ENABLE_BIT | GL_SCISSOR_BIT );
		int viewport[4];
		glGetIntegerv( GL_VIEWPORT, viewport );
		glEnable( GL_SCISSOR_TEST );
		float winX, winY;
		getWindowXY( winX, winY );
		scissorIntersect(
			viewport[0] + (int)winX-1 + borderSize,
			viewport[1] + (int)winY + borderSize,
			(int)ceilf(myW) - 2*borderSize,
			(int)ceilf(myH) - 2*borderSize
		);
	}

	float lineW, lineH, lineD;
	printSize( "Q", lineW, lineH, lineD );

	if( isEditting() ) {
		int markBeg = getI( "markBeg" );
		int markEnd = getI( "markEnd" );
		float markBegX = getF( "markBegX" );
		float markBegY = getF( "markBegY" );
		float markEndX = getF( "markEndX" );
		float markEndY = getF( "markEndY" );
		float cursorX = getF( "cursorX" );
		float cursorY = getF( "cursorY" );
		char *t = text();
		int len = strlen( t );
		
		if( getI( "focused" ) ) {
			// DRAW the mark
			setupColor( "textEditMarkColor" );

			int lineSpan = int( (markEndY-markBegY) / lineH ) + 1;
				// +1 because both lines measured from the top

			float middleBegY = lineSpan > 2 ? markBegY + lineH  : markBegY;
			float middleEndY = lineSpan > 2 ? markEndY          : markBegY;

			glBegin( GL_QUADS );
				if( lineSpan == 1 ) {
					glVertex2f( markBegX+1, myH-(markBegY+lineH) );
					glVertex2f( markEndX+1, myH-(markBegY+lineH) );
					glVertex2f( markEndX+1, myH-(markBegY) );
					glVertex2f( markBegX+1, myH-(markBegY) );
				}
				else {
					glVertex2f( markBegX, myH-(markBegY+lineH) );
					glVertex2f( myW, myH-(markBegY+lineH) );
					glVertex2f( myW, myH-(markBegY) );
					glVertex2f( markBegX, myH-(markBegY) );

					glVertex2f( 0.f, myH-(middleEndY) );
					glVertex2f( myW, myH-(middleEndY) );
					glVertex2f( myW, myH-(middleBegY) );
					glVertex2f( 0.f, myH-(middleBegY) );

					glVertex2f( 0.f, myH-(markEndY+lineH) );
					glVertex2f( markEndX, myH-(markEndY+lineH) );
					glVertex2f( markEndX, myH-(markEndY) );
					glVertex2f( 0.f, myH-(markEndY) );
				}
			glEnd();

			// DRAW the blinking cursor 
			float alpha = blinkState ? 0.9f : 0.1f;
			setupColor( "textEditCursorColor" );
			scaleAlpha( alpha );
			glBegin( GL_QUADS );
				glVertex2f( cursorX, myH - cursorY );
				glVertex2f( cursorX, myH - (cursorY+lineH) );
				glVertex2f( cursorX+2, myH - (cursorY+lineH) );
				glVertex2f( cursorX+2, myH - cursorY );
			glEnd();
			glDisable( GL_BLEND );
		}
	}

	setupColor( (char*)(!getI("disabled") ? "textEditTextColor" : "textEditTextColorDisabled") );
	char *t = (char *)text();

	glTranslatef( -getF("viewWindowPixelX"), getF("viewWindowPixelY"), 0.f );

//	glColor3ub( 255, 0, 0 );
//	glBegin( GL_QUADS );
//	glVertex2f( 0.f, myH-0.f );
//	glVertex2f( 10.f, myH-0.f );
//	glVertex2f( 10.f, myH-10.f );
//	glVertex2f( 0.f, myH-10.f );
//	glEnd();

	float _y = myH;
	for( int i=0; i<lineCount; i++ ) {
		print( (char *)&t[ lines[i*2+0] ], 0.f, _y, lines[i*2+1] );
		_y -= lineH;
		// @OPT: This could stop when it gets to the end of the visible rect and start at the beginning
	}

	glPopMatrix();

	if( !noScissor ) {
		glPopAttrib();
	}
}

ZUI::ZUILayoutCell *ZUITextEditor::layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly ) {
	float d;
	printSize( text(), w, h, d );
	w += 8;
	h += d + 4;
	return 0;
}

int ZUITextEditor::isAlphaNum( char c ) {
	return isalnum(c) || c=='_';
}

void ZUITextEditor::sendMsg( char *extraMsg, int fromCancel ) {
	if( fromCancel ) { 
		if( has("sendMsgOnCancel") ) {
			ZMsg *msg = zHashTable( "%s %s", getS("sendMsgOnCancel"), extraMsg );
			msg->putS( "fromZUI", name );
			msg->putS( "text", text() );
			msg->putS( "oldText", getS( "takeFocusContents" ) );
			msg->putI( "fromCancel", 1 );
			::zMsgQueue( msg );
		}
	}
	else {
		if( has("sendMsg") ) {
			ZMsg *msg = zHashTable( "%s %s", getS("sendMsg"), extraMsg );
			msg->putS( "fromZUI", name );
			msg->putS( "text", text() );
			::zMsgQueue( msg );
		}
	}
}

void ZUITextEditor::takeFocus( int strPos ) {
	int refocus = ZUI::focusStack[0] == this;
	focus( 1 );

	putI( "markBeg", strPos );
	putI( "markEnd", strPos );
	putI( "cursor", strPos );
	putI( "dragStart", strPos );
	putI( "focused", 1 );
	linesUpdate();
	cursorMove(0,0);
	putI( "scaleAlphaMouseOver", 0 );

	// SAVE the contents at begin of edit
	if( !refocus ) {
		putS( "takeFocusContents", getS( "text" ) );
	}

	// TELL anyone who cares that we took focus
	zMsgQueue( "type=ZUITextEditor_GainFocus fromZUI=%s", name );
}

void ZUITextEditor::cancelFocus() {
	focus( 0 );
	putI( "cursor", -1 );
	putI( "scaleAlphaMouseOver", 1 );
	putI( "focused", 0 );
	dirty();

	// TELL anyone who case that we are not in focus anymore
	zMsgQueue( "type=ZUITextEditor_LostFocus fromZUI=%s", name );
}

void ZUITextEditor::handleMsg( ZMsg *msg ) {
	int markBeg = getI( "markBeg" );
	int markEnd = getI( "markEnd" );
	int cursor  = getI( "cursor" );
	int prevCursor = cursor;


	if( zmsgIs(type,ZUIEnable) ) {
		if( !(zmsgHas(toZUI) || zmsgHas(toZUIGroup)) ) {
			return;
		}
		putI("disabled",0);
		layoutRequest();
	}

	if( zmsgIs(type,ZUIDisable) ) {
		if( !(zmsgHas(toZUI) || zmsgHas(toZUIGroup)) ) {
			return;
		}
		putI("disabled",1);
		layoutRequest();
	}

	// LOSE FOCUS
	if( zmsgIs(type,ZUIMouseClickOutside) && getI("focused") ) {
		// MANAGE behavior of "lostfocus during edit" via attributes:
		if( getI( "acceptEditOnClickOutside" ) ) {
			sendMsg( "fromClickOutside=1", 0 );
		}
		else if ( getI( "cancelEditOnClickOutside" ) ) {
			sendMsg( "fromClickOutside=1", 1 );
			if( getI( "restoreTextOnCancel" ) ) {
				putS( "text", getS( "takeFocusText" ) );
			}
		}
		
		cancelFocus();
		// Note that we are NOT using the message so that it may be handled by others
		return;
	}

	// GAIN FOCUS
	else if( zmsgIs(type,ZUITakeFocus) ) {
		// Sent by parents for example to force this text box into edit
		// For example, a filepicker as soon as it comes up wants
		// the cursor blinking in its textbox
		zMsgUsed();
		takeFocus( strlen( text() ) );
	}

	// LEFT CLICK
	else if( (zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L)) ) {
		dirty();
		zMsgUsed();
		takeFocus( strPosFromPixelXY( zmsgF(localX)-borderSize, h-zmsgF(localY)-borderSize ) );
		markPosUpdate( prevCursor, 0 );
			// This 1 tells it not to reset the mark so if this is 1
			// it will mark the whole field on entry, I'm not sure this
			// is the desired behavior, we could make it an option
		requestExclusiveMouse( 1, 1 );
	}

	// RIGHT CLICK
	else if( (zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,R)) && !getI( "disableTextScroll" ) ) {
		dirty();
		zMsgUsed();
		requestExclusiveMouse( 1, 1 );
		putI( "scrollStart", getI("textScrollY") );
		putF( "scrollStartPix", zmsgF(localY) );
	}

	// MOUSE DRAG
	else if( zmsgIs(type,ZUIExclusiveMouseDrag) ) {
		dirty();
		if( zmsgI(r) ) {
			zMsgUsed();
			float w, h, d;
			printSize( "Q", w, h, d );
			int textScrollY = getI("scrollStart") + int( (zmsgF(localY) - getF( "scrollStartPix" )) / h );
			textScrollY = max( 0, min( lineCount-1, textScrollY ) );
			putI( "textScrollY", textScrollY );
			if( ! isEditting() ) {
				putI( "tempDisableAlphaScale", 1 );
			}
			putI( "scaleAlphaMouseOver", 0 );
		}
		else if( zmsgI(l) ) {
			zMsgUsed();
			int dragStart  = getI( "dragStart" );
			int x = strPosFromPixelXY( zmsgF(localX)-borderSize, h-zmsgF(localY)-borderSize );
			cursor = x;
			if( x > dragStart ) {
				markBeg = dragStart;
				markEnd = x;
			}
			else {
				markBeg = x;
				markEnd = dragStart;
			}
			putI( "markBeg", markBeg );
			putI( "markEnd", markEnd );
			putI( "cursor", cursor );
			linesUpdate();
			cursorMove(0,0);
			markPosUpdate( prevCursor, 1 );
		}
	}
	else if( zmsgIs(type,ZUIMouseReleaseDrag) ) {
		dirty();
		zMsgUsed();
		requestExclusiveMouse( 1, 0 );
		putI( "markStart", -1 );

		if( getI( "tempDisableAlphaScale" ) ) {
			putI( "scaleAlphaMouseOver", 1 );
			putI( "tempDisableAlphaScale", 0 );
		}
	}

	// LINES UPDATE
	else if( zmsgIs(type,ZUITextEditor_LinesUpdate) ) {
		dirty();
		zMsgUsed();
		linesUpdate();
		//cursorMove((int)2000000000,0);
		markPosUpdate( 0, 0 );
	}

	// SET TEXT
	else if( zmsgIs(type,ZUITextEditor_SetText) ) {
		dirty();
		zMsgUsed();
		putS( "text", zmsgS(text) );
		linesUpdate();
		cursorMove((int)2000000000,0);
		markPosUpdate( 0, 0 );
	}

	// RESIZE
	else if( zmsgIs(type,ZUIResized) ) {
		putF( "viewWindowPixelX", 0.f );
		putF( "viewWindowPixelY", 0.f );
		dirty();
		linesUpdate();
		cursorMove( 0, 0 );
		markPosUpdate( 0, 0 );
	}


	// KEYBOARD--------------------------------------------------------------------
	else if( isEditting() && zmsgIs(type,Key) ) {
		dirty();
//extern void trace( char *fmt, ... );
//int _which = zMsgTranslateCharacter( zmsgS(which) );
//trace( "editor %d %c\n", _which, _which );
		// If you start typing and the cursor is not visible then
		// move the scroll to where it is visible
		float x, y;
		strPosToPixelXY( cursor, x, y );
		float myW, myH;
		getWH( myW, myH );
		float lineW, lineH, lineD;
		printSize( "Q", lineW, lineH, lineD );
		int line, charX;
		if( y >= myH-lineH-1.f ) {
			strPosToLineAndChar( cursor, line, charX );
			line = max( 0, line-1 );
//			putI( "textScrollY", line );
		}
		if( y <= lineH ) {
			strPosToLineAndChar( cursor, line, charX );
			line = min( lineCount-1, (int)(line - myH/lineH)+3 );
//			putI( "textScrollY", line );
		}
		char *src = text();
		int len = strlen(src);
		char *dst = (char *)alloca( len + 4 );
		strcpy( dst, src );
		int whichLen = strlen( zmsgS(which) );
		int which = zMsgTranslateCharacter( zmsgS(which) );

		if( zmsgIs(which,left) ) {
			zMsgUsed();
			if( zmsgI(ctrl) ) {
				char *textStart = text();
				int cursorLine, cursorCharX;
				strPosToLineAndChar( cursor, cursorLine, cursorCharX );
				// SEARCH for whitespace and then the first char that isn't whitespace

				// If you are sitting on non-alphanum, skip it
				char *c = &textStart[cursor-1];
				if( ! isAlphaNum( *c ) ) {
					for( ; c>textStart; c-- ) {
						if( isAlphaNum( *c ) ) {
							break;
						}
					}
				}
				for( ; c>textStart; c-- ) {
					if( ! isAlphaNum(*c) ) {
						c++;
						break;
					}
				}
				cursorMove( c-textStart-cursor, 0 );
			}
			else {
				cursorMove( -1, 0 );
			}
			markPosUpdate( prevCursor, zmsgI(shift) );
			// scrollX
//			if( !has( "multiline" )) {
//			if( getI("cursor") < getI( "textScrollX" ) )
//				putI( "textScrollX", getI( "cursor" ) );
//			}
			// end scrollX

		}

		else if( zmsgIs(which,right) ) {
			zMsgUsed();
			if( zmsgI(ctrl) ) {
				char *textStart = text();
				char *textEnd = textStart + strlen(textStart);
				int cursorLine, cursorCharX;
				strPosToLineAndChar( cursor, cursorLine, cursorCharX );
				// SEARCH for whitespace and then the first char that isn't whitespace

				// If you are sitting on non-alphanum, skip it
				char *c = &textStart[cursor];
				if( isAlphaNum( *c ) ) {
					for( ; c<textEnd; c++ ) {
						if( ! isAlphaNum( *c ) ) {
							break;
						}
					}
				}
				for( ; c<textEnd; c++ ) {
					if( isAlphaNum(*c) ) {
						break;
					}
				}
				cursorMove( c-textStart-cursor, 0 );
			}
			else {
				cursorMove( 1, 0 );
			}
			markPosUpdate( prevCursor, zmsgI(shift) );
		}

		else if( zmsgIs(which,up) ) {
			zMsgUsed();
			cursorMove( 0, -1 );
			markPosUpdate( prevCursor, zmsgI(shift) );
		}

		else if( zmsgIs(which,down) ) {
			zMsgUsed();
			cursorMove( 0, 1 );
			markPosUpdate( prevCursor, zmsgI(shift) );
		}

		else if( zmsgIs(which,pageup) ) {
			zMsgUsed();
			float w, h, d;
			printSize( "Q", w, h, d );
			float myW, myH;
			getWH( myW, myH );
			cursorMove( 0, -(int)( myH/h ) );
			markPosUpdate( prevCursor, zmsgI(shift) );
		}

		else if( zmsgIs(which,pagedown) ) {
			zMsgUsed();
			float w, h, d;
			printSize( "Q", w, h, d );
			float myW, myH;
			getWH( myW, myH );
			cursorMove( 0, (int)( myH/h ) );
			markPosUpdate( prevCursor, zmsgI(shift) );
		}

		else if( zmsgIs(which,home) ) {
			zMsgUsed();
			if( zmsgI(ctrl) ) {
				cursorMove( 0, -(int)2000000000 );
			}
			int cursorLine, cursorCharX;
			strPosToLineAndChar( cursor, cursorLine, cursorCharX );
			cursorMove( -cursorCharX, 0 );
			markPosUpdate( prevCursor, zmsgI(shift) );
		}

		else if( zmsgIs(which,end) ) {
			zMsgUsed();
			if( zmsgI(ctrl) ) {
				cursorMove( 0, (int)2000000000 );
			}
			int cursorLine, cursorCharX;
			strPosToLineAndChar( cursor, cursorLine, cursorCharX );
			char *_text = text();
			if( _text[ lines[cursorLine*2] + lines[cursorLine*2+1] - 1 ] == '\n' ) {
				cursorMove( lines[cursorLine*2+1] - cursorCharX - 1, 0 );
			}
			else {
				cursorMove( lines[cursorLine*2+1] - cursorCharX, 0 );
			}
			markPosUpdate( prevCursor, zmsgI(shift) );
 		}

		else if( which == '\b' ) {
			zMsgUsed();
			if( !getI("disabled") ) {
				if( markEnd > markBeg ) {
					memcpy( dst, src, markBeg );
					memcpy( &dst[markBeg], &src[markEnd], len-markEnd );
					dst[markBeg + (len-markEnd)] = 0;
					putS( "text", dst );
					linesUpdate();
					if( cursor > markBeg ) {
						putI( "cursor", getI("cursor")-(markEnd-markBeg) );
					}
					cursorMove(0,0);
					markPosUpdate( prevCursor, 0 );
				}
				else if( cursor > 0 ) {
					memcpy( dst, src, cursor-1 );
					memcpy( &dst[cursor-1], &src[markEnd], len-markEnd );
					dst[markBeg-1 + (len-markEnd)] = 0;
					putS( "text", dst );
					linesUpdate();
					putI( "cursor", getI("cursor")-1 );
					cursorMove(0,0);
					markPosUpdate( prevCursor, 0 );
				}
			}
		}

		else if( zmsgIs(which,ctrl_c) || zmsgIs(which,cmd_c) ) {
			zMsgUsed();
			// COPY the mark to clipboard
			int markLen = markEnd - markBeg;
			char *cut = (char *)alloca(markLen+1);
			memcpy( cut, &src[markBeg], markLen );
			cut[markLen] = 0;
			zplatformCopyToClipboard( cut, strlen(cut) );
		}

		else if( zmsgIs(which,ctrl_v) || zmsgIs(which,cmd_v) ) {
			zMsgUsed();
			if( !getI("disabled") ) {
				// COPY from clipboard
				char buffer[16384];
				zplatformGetTextFromClipboard( buffer, 16384 );
				dst = (char *)alloca( len + strlen(buffer) + 1 );
				memcpy( dst, src, markBeg );
				strcpy( &dst[markBeg], buffer );
				strcat( &dst[markBeg], &src[markEnd] );
				putS( "text", dst );
				linesUpdate();
				cursorMove(strlen(buffer),0);
				markPosUpdate( prevCursor, 0 );
			}
		}

		else if( zmsgIs(which,ctrl_x) || zmsgIs(which,cmd_x) || zmsgIs(which,delete) ) {
			zMsgUsed();
			if( !getI("disabled") ) {
				if( markBeg == markEnd && cursor < len ) {
					memcpy( &dst[0], &src[0], cursor );
					memcpy( &dst[cursor], &src[cursor+1], len-cursor-1 );
					dst[len-1] = 0;
					putS( "text", dst );
					linesUpdate();
					markPosUpdate( prevCursor, 0 );
				}
				else {
					if( zmsgIs(which,ctrl_x) || zmsgIs(which,cmd_x) ) {
						// COPY the mark to clipboard
						int markLen = markEnd - markBeg;
						char *cut = (char *)alloca(markLen+1);
						memset( cut, 0, markLen+1 );
						memcpy( cut, &src[markBeg], markLen );
						cut[markLen] = 0;
						zplatformCopyToClipboard( cut, strlen(cut) );
					}
					if( markBeg > 0 ) {
						memcpy( dst, src, markBeg-1 );
					}
					memcpy( &dst[markBeg], &src[markEnd], len-markEnd );
					dst[markBeg + (len-markEnd)] = 0;
					putS( "text", dst );
					linesUpdate();
					if( cursor > markBeg ) {
						putI( "cursor", getI("cursor")-(markEnd-markBeg) );
					}
					cursorMove(0,0);
					markPosUpdate( prevCursor, 0 );
				}
			}
		}

		else if( which == 27 ) {
			zMsgUsed();
			cancelFocus();
			sendMsg( "fromEscape=1", 1 );
			if( getI( "restoreTextOnCancel" ) ) {
				putS( "text", getS( "takeFocusContents" ) );
			}
			return;
		}

		else if( (which == '\r' || which == '\n') && ( !getI("multiline") || zmsgI(shift) ) ) {
			zMsgUsed();
			cancelFocus();
			sendMsg( "fromEnter=1", 0 );
			return;
		}

		else if( which == '\t' ) {
			const char *tabToKey = zmsgI(shift) ? "tabToZUIBack" : "tabToZUI";
			char *tabTo = getS( (char*)tabToKey, 0 );
			if( !tabTo && parent ) {
				// if our parent is a ZUIVar, it may have been setup with a "tabTo..."
				tabTo = parent->getS( (char*)tabToKey, 0 );
			}
			if( tabTo ) {
				zMsgUsed();
				cancelFocus();
				sendMsg( "fromEnter=1", 0 );
				zMsgQueue( "type=ZUITakeFocus toZUI=%s", tabTo );
					// NOTE: at present only ZUITextEditor responds to ZUITakeFocus, and this 
					// is utilized here to tab between texteditors;
			}
			return;
		}

		else if( whichLen==1 && (which >= ' ' && which < 127) || ( (which == '\r' || which == '\n') && getI("multiline") ) ) {
			// Insert a plain-old key, deleteing the mark if non-zero length
			zMsgUsed();
			if( !getI("disabled") ) {
				memcpy( dst, src, markBeg );
				dst[markBeg] = which;
				memcpy( &dst[markBeg + 1], &src[markEnd], len-markEnd );
				dst[markBeg + 1 + (len-markEnd)] = 0;
				putS( "text", dst );
				cursor = markBeg;
				putI( "cursor", cursor );
				linesUpdate();
				cursorMove(1,0);
				markPosUpdate( prevCursor, 0 );
			}
		}

		if( strcmp( dst, src ) && has("sendMsgOnChange") ) {
			ZHashTable *msg = zHashTable( "%s", getS("sendMsgOnChange") );
			msg->putS( "text", text() );
			msg->putS( "fromZUI", name );
			::zMsgQueue( msg );
		}
	}

	if( !zMsgIsUsed() ) {
		ZUIPanel::handleMsg( msg );
	}
}

