// @ZBS {
//		*COMPONENT_NAME zuifilepicker
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuifilepicker.cpp zuifilepicker.h zuitexteditor.cpp zuitexteditor.h
//		+TODO {
//			Lots of little bugs left.
//			Clicking on something does pass the full file spec back
//			Clicking on OK causes problems
//			Need scrolling of list area
//		}
// }

// OPERATING SYSTEM specific includes:
#include "wingl.h"
#include "wintiny.h"
// SDK includes:
#include "GL/gl.h"
// STDLIB includes:
#include "ctype.h"
#include "assert.h"
#include "string.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"
#ifdef WIN32
#include "malloc.h"
#endif
// MODULE includes:
#include "zui.h"
// ZBSLIB includes:
#include "zglfont.h"
#include "zmsg.h"
#include "zvars.h"
#include "ztmpstr.h"
#include "zwildcard.h"
#include "zfilespec.h"
#include "zuitexteditor.h"

// Optionally pull in the native file dialog code if defined for this zlab app
// (to define this for your app, use EXTRA_DEFINES macro in the ZBS header like this:
// *EXTRA_DEFINES < your other defines here > NATIVE_FILE_DIALOGS
#ifdef NATIVE_FILE_DIALOGS
// @ZBSIF extraDefines( 'NATIVE_FILE_DIALOGS' )
// the above is for the perl-parsing of files for dependencies; we don't
// want the dependency builder to see these includes if NATIVE_FILE_DIALOGS is not
// defined.
	#include "filedialog_native.h"
// @ZBSENDIF
#endif



struct FileLine {
	int indent;
	int displayStart;
	int displayLen;
	int isDir;
	int isOpen;
	int isSelected;
	char path[256];
};

struct ZUIFileList : ZUIPanel {
	ZUI_DEFINITION(ZUIFileList,ZUIPanel);

	int lineCount;
	int allocLineCount;
	FileLine *lines;
	void closeFolder( int line );
	void openFolder( int line );
	void openPath( char *path );
	char *getSelected();

	virtual void factoryInit();
	ZUIFileList() : ZUIPanel() {};
	virtual void render();
	virtual void update();
	virtual void handleMsg( ZMsg *msg );
};

ZUI_IMPLEMENT(ZUIFileList,ZUIPanel);


void ZUIFileList::factoryInit() {
	allocLineCount = 0;
	lineCount = 0;
	lines = 0;
}

int sorter( const void *a, const void *b ) {
	#ifdef __USE_GNU
	#define stricmp strcasecmp
	#endif
	FileLine *_a = (FileLine *)a;
	FileLine *_b = (FileLine *)b;
	if( _a->isDir && !_b->isDir ) {
		return -1;
	}
	if( !_a->isDir && _b->isDir ) {
		return +1;
	}
	return stricmp( &_a->path[_a->displayStart], &_b->path[_b->displayStart] );
}

void ZUIFileList::closeFolder( int line ) {
	assert( lines[line].isDir );
	if( ! lines[line].isOpen ) {
		return;
	}

	int indent = lines[line].indent;
	int i;
	for( i=line+1; i<lineCount; i++ ) {
		if( lines[i].indent <= indent ) {
			break;
		}
	}

	memmove( &lines[line+1], &lines[i], sizeof(FileLine) * (lineCount-i) );
	lineCount -= i-line-1;
	lines[line].isOpen = 0;
}

// This needs a lot of help.  It needs to understand wildcards which 
// is a pain because the ZWildcard doesn't have the concept of
// enumerating just directories for example.  The wildcard applies
// to the files bu not the dirs
// Meanwhile, under win32 it needs to have drives instead of a root
// also, relative paths will probably mess it u p like d:/oink/../boink.txt

void ZUIFileList::openFolder( int line ) {
	assert( lines[line].isDir );
	assert( ! lines[line].isOpen );
	lines[line].isOpen = 1;

	//printf( "      openFolder: %s\n", lines[line].path );

	char foldPattern[256];
	char filePattern[256];

	strcpy( foldPattern, lines[line].path );
	strcat( foldPattern, "*" );

	strcpy( filePattern, lines[line].path );
	char *pattern = getS( "pattern", "*" );
	strcat( filePattern, pattern );

	int skipWildcardSearch = 0;
	int dirLineCount = 0;
	int fileLineCount = 0;
	int localLineCount = 0;
	int skipStartSlash = 0;
	FileLine *localLines = 0;
	char *extraSlash = "";

	#ifdef WIN32
	if( line == 0 ) {
		char drives[32][3] = { 0, };

		int i;
		int driveMask = GetLogicalDrives();
		for( i=0; i<32; i++ ) {
			if( (1<<i) & driveMask ) {
				drives[dirLineCount][0] = 'a' + i;
				drives[dirLineCount][1] = ':';
				drives[dirLineCount][2] = 0;
				dirLineCount++;
			}
		}

		localLines = (FileLine *)malloc( sizeof(FileLine) * dirLineCount );
		for( i=0; i<dirLineCount; i++ ) {
			localLines[localLineCount].indent = 1;
			localLines[localLineCount].isDir = 1;
			localLines[localLineCount].isOpen = 0;
			localLines[localLineCount].isSelected = 0;
			strcpy( localLines[localLineCount].path, "/" );
			strcat( localLines[localLineCount].path, drives[i] );
			strcat( localLines[localLineCount].path, "/" );
			localLines[localLineCount].displayLen = strlen(drives[i])+1;
			localLines[localLineCount].displayStart = strlen(localLines[localLineCount].path) - localLines[localLineCount].displayLen;
			localLineCount++;
		}

		skipWildcardSearch = 1;
	}
	extraSlash = "/";
	skipStartSlash = 1;
	#endif

	if( ! skipWildcardSearch ) {

		// COUNT folders (disregard pattern)
		{
			ZWildcard w( foldPattern + skipStartSlash );
			while( w.next() ) {
				if( strcmp(w.getName(),".") && strcmp(w.getName(),"..") ) {
					char *fullName = w.getFullName();
					if( ! zWildcardFileIsHidden( fullName ) ) {
						if( zWildcardFileIsFolder( fullName ) ) {
							//printf( "      Found folder: %s\n", fullName );
							dirLineCount++;
						}
					}
				}
			}
		}

		// COUNT files that match pattern
		{
			ZWildcard w( filePattern + skipStartSlash );
			while( w.next() ) {
				if( strcmp(w.getName(),".") && strcmp(w.getName(),"..") ) {
					char *fullName = w.getFullName();
					if( ! zWildcardFileIsHidden( fullName ) ) {
						if( ! zWildcardFileIsFolder( fullName ) ) {
							//printf( "      Found matching file: %s\n", fullName );
							fileLineCount++;
						}
					}
				}
			}
		}

		int indent = lines[line].indent + 1;
		localLineCount = 0;
		localLines = (FileLine *)malloc( sizeof(FileLine) * (fileLineCount + dirLineCount) );
		{
			ZWildcard w( foldPattern + skipStartSlash );
			while( w.next() ) {
				if( strcmp(w.getName(),".") && strcmp(w.getName(),"..") ) {
					char *fullName = w.getFullName();
					if( ! zWildcardFileIsHidden( fullName ) ) {
						if( zWildcardFileIsFolder( fullName ) ) {
							localLines[localLineCount].indent = indent;
							localLines[localLineCount].isDir = 1;
							localLines[localLineCount].isOpen = 0;
							localLines[localLineCount].isSelected = 0;
							strcpy( localLines[localLineCount].path, extraSlash );
							strcat( localLines[localLineCount].path, w.getFullName() );
							strcat( localLines[localLineCount].path, "/" );
							localLines[localLineCount].displayLen = strlen(w.getName())+1;
							localLines[localLineCount].displayStart = strlen(localLines[localLineCount].path) - localLines[localLineCount].displayLen;
							localLineCount++;
						}
					}
				}
			}
		}

		{
			ZWildcard w( filePattern + skipStartSlash );
			while( w.next() ) {
				if( strcmp(w.getName(),".") && strcmp(w.getName(),"..") ) {
					char *fullName = w.getFullName();
					if( ! zWildcardFileIsHidden( fullName ) ) {
						if( ! zWildcardFileIsFolder( fullName ) ) {
							localLines[localLineCount].indent = indent;
							localLines[localLineCount].isDir = 0;
							localLines[localLineCount].isOpen = 0;
							localLines[localLineCount].isSelected = 0;
							strcpy( localLines[localLineCount].path, extraSlash );
							strcat( localLines[localLineCount].path, w.getFullName() );
							localLines[localLineCount].displayLen = strlen(w.getName());
							localLines[localLineCount].displayStart = strlen(localLines[localLineCount].path) - localLines[localLineCount].displayLen;
							localLineCount++;
						}
					}
				}
			}
		}
	}

	qsort( localLines, localLineCount, sizeof(FileLine), sorter );

	int newAllocLineCount = localLineCount+lineCount;
	if( newAllocLineCount > allocLineCount ) {
		lines = (FileLine *)realloc( lines, sizeof(FileLine) * newAllocLineCount );
	}
	memmove( &lines[line+localLineCount+1], &lines[line+1], sizeof(FileLine)*(lineCount-line-1) );
	memcpy( &lines[line+1], localLines, sizeof(FileLine)*localLineCount );
	lineCount += localLineCount;

	free( localLines );
}

void ZUIFileList::openPath( char *pattern ) {
	int i, j;
	
	//printf( "ZUIFileList::openPath with pattern: %s\n", pattern );

	// Open the folder in the path and show all of it folders
	// if the file of this pattern is valid then it will move the highlight to that file
	// if it is a wildcard then maybe it should lightlight all those or filter?
	ZFileSpec spec( pattern );
	int searchFrom = 0;

	spec.makeAbsolute();

	char *fileAndExt = spec.getFile(1);
	//printf( "fileAndExt = %s\n", fileAndExt );
	if( strchr(fileAndExt,'*') || strchr(fileAndExt,'?') ) {
		putS( "pattern", fileAndExt );
	}
	else {
		putS( "pattern", "*" );
	}

	char dirs[256][256] = {0,};
	int dirsCount = 0;
	char drive[8] = {0,};

	// All systems have a root
	strcpy( dirs[dirsCount], "/" );
	dirsCount++;

	// Only win32 has the drive specs
	#ifdef WIN32
		// Use the specified drive spec or current if not specified
		if( * spec.getDrive() ){
			strcat( dirs[dirsCount], "/" );
			strcat( dirs[dirsCount], spec.getDrive() );
			strcat( dirs[dirsCount], "/" );
			strcpy( drive, spec.getDrive() );
		}
		else {
			// EXTRACT the current drive
			// @TODO real code
			char currentDir[256];
			GetCurrentDirectory( 256, currentDir );
			dirs[dirsCount][0] = '/';
			dirs[dirsCount][1] = tolower( currentDir[0] );
			dirs[dirsCount][2] = ':';
			dirs[dirsCount][3] = '/';
			drive[0] = dirs[dirsCount][1];
			drive[1] = ':';
			drive[2] = 0;
		}
		dirsCount++;
	#endif

	// ADD the dirs skipping root already taken care of above
	int numDirs = spec.getNumDirs();
	//printf( "numDirs = %d\n", numDirs );
	for( i=1; i<numDirs; i++ ) {
		#ifdef WIN32
			strcpy( dirs[dirsCount], "/" );
			strcat( dirs[dirsCount], drive );
			strcat( dirs[dirsCount], spec.getDirUpTo(i) );
		#else
			strcpy( dirs[dirsCount], spec.getDirUpTo(i) );
		#endif
		dirsCount++;
	}

	for( i=0; i<dirsCount; i++ ) {
		char *dir = dirs[i];
		//printf("   Processing directory: %s\n", dir );

		// FIND this folder
		for( j=searchFrom; j<lineCount; j++ ) {
			if( lines[j].isDir && !stricmp(lines[j].path, dir) ) {
				closeFolder( j );
				openFolder( j );
				putI( "scroll", j );
				searchFrom = j;
				break;
			}
		}

		if( lineCount==0 ) {
			// Not found, if this is the first one then open root
			assert( lineCount == 0 );
			lineCount = 1;
			allocLineCount = 100;
			lines = (FileLine *)malloc( sizeof(FileLine) * allocLineCount );
			lines[0].isDir = 1;
			lines[0].indent = 0;
			strcpy( lines[0].path, "/" );
			lines[0].displayLen = 1;
			lines[0].displayStart = 0;
			lines[0].isOpen = 0;
			lines[0].isSelected = 0;
			openFolder( 0 );
		}
	}

	// CLEAR all the selected
	for( i=0; i<lineCount; i++ ) {
		lines[i].isSelected = 0;
	}

	// SEARCH for the given file
	for( j=searchFrom; j<lineCount; j++ ) {
		if( !stricmp(lines[j].path,pattern) ) {
			lines[j].isSelected = 1;
			break;
		}
	}
	dirty();
}

char *ZUIFileList::getSelected() {
	for( int i=0; i<lineCount; i++ ) {
		if( lines[i].isSelected ) {
			return lines[i].path;
		}
	}
	return "";
}

void ZUIFileList::render() {
	ZUIPanel::render();

	float lineW, lineH, lineD;
	printSize( "+ ", lineW, lineH, lineD );

	float colorFolder[4];
	setupColor( "fileListFolderColor" );
	glGetFloatv( GL_CURRENT_COLOR, colorFolder );

	float colorFile[4];
	setupColor( "fileListFileColor" );
	glGetFloatv( GL_CURRENT_COLOR, colorFile );

	float colorSelected[4];
	setupColor( "fileListSelectedColor" );
	glGetFloatv( GL_CURRENT_COLOR, colorSelected );

	float mx, my;
	getMouseLocalXY( mx, my );
	int mouseLine = int( (h - my) / lineH ) + getI( "scroll" );

	void *fontPtr = zglFontGet( getS("font") );

	int i = getI( "scroll" );
	for( float y = h; y > lineH && i < lineCount; y -= lineH, i++ ) {
		if( i == mouseLine ) {
			setupColor( "fileListMouseColor" );
			glBegin( GL_QUADS );
				glVertex2f( 0.f, y );
				glVertex2f( w, y );
				glVertex2f( w, y-lineH );
				glVertex2f( 0.f, y-lineH );
			glEnd();
		}
		float indent = float( (int)lines[i].indent * lineW );
		if( lines[i].isDir ) {
			glColor4fv( colorFolder );
			char *str = "+ ";
			if( lines[i].isOpen ) {
				str = "- ";
			}
			zglFontPrint( str, indent, y-lineH+lineD, 0, 0, fontPtr, 2 );
			indent += lineW;
		}
		else {
			glColor4fv( colorFile );
		}
		if( lines[i].isSelected ) {
			glColor4fv( colorSelected );
		}

		zglFontPrint( &lines[i].path[lines[i].displayStart], indent, y-lineH+lineD, 0, 0, fontPtr, lines[i].displayLen );
	}
}

void ZUIFileList::update() {
	// @TODO: for now we dirty the whole list since render
	// looks at mouse pos and expects to draw wherever it likes
	// disregarding dirty rects. For dirty rects, we want to trap
	// mouse moves and explicitly dirty the mouse lines that have
	// changed.  However, this means we need to requrest mouse move
	// messages, and thus need to know when we become visible; we can
	// respond to a ZUIShowNotify/ZUIHideNotify for this.
	dirty();
}

void ZUIFileList::handleMsg( ZMsg *msg ) {
	ZUIPanel::handleMsg( msg );

	if( zmsgIs(type,ZUIFileList_OpenPath) ) {
		openPath( zmsgS(text) );
	}
	else if( zmsgIs( type, ZUINotifyShow ) ) {
		setUpdate( 1 );
	}
	else if (zmsgIs( type, ZUINotifyHide ) ) {
		setUpdate( 0 );
	}
	else if( (zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L)) ) {
		float lineW, lineH, lineD;
		printSize( "Q", lineW, lineH, lineD );
		int line = getI( "scroll" ) + int( (h - zmsgF(localY) ) / lineH );
		if( line >= 0 && line < lineCount ) {
			for( int i=0; i<lineCount; i++ ) {
				lines[i].isSelected = 0;
			}
			if( lines[line].isDir ) {
				if( lines[line].isOpen ) {
					closeFolder( line );
				}
				else {
					openFolder( line );
				}
			}
			else {
				// CLEAR all the other selected and select this one
				lines[line].isSelected = 1;
			}
			char filespec[256];
			int offset = 0;
			#ifdef WIN32
				offset = 1;
			#endif
			strcpy( filespec, lines[line].path + offset );
			if( lines[line].isDir ) {
				if( has( "userEnteredFilename" ) ) {
					strcat( filespec, getS( "userEnteredFilename" ) );
				}
				else {
					strcat( filespec, getS("pattern") );
				}
			}
			zMsgQueue( "type=ZUIFileList_Selected filespec='%s' toZUIParents=1", escapeQuotes( filespec ) );
		}
		zMsgUsed();
		dirty();
	}
	else if( (zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,R)) ) {
		putF( "startMouseY", zmsgF(localY) );
		putI( "startScroll", getI("scroll") );
		requestExclusiveMouse( 1, 1 );
		zMsgUsed();
	}
	else if( zmsgIs(type,ZUIExclusiveMouseDrag) && zmsgI(r) ) {
		float lineW, lineH, lineD;
		printSize( "Q", lineW, lineH, lineD );
		int scroll = getI( "startScroll" ) + int( zmsgF(localY) - getF( "startMouseY" ) ) / (int)lineH;
		scroll = max( 0, scroll );
		putI( "scroll", scroll );
		zMsgUsed();
		dirty();
	}
	else if( zmsgIs(type,Key) ) {
		if( zmsgIs(which,wheelforward) ) {
			int scroll = max( 0, getI( "scroll" ) -5 );
			putI( "scroll", scroll );
			zMsgUsed();
			dirty();
		}
		else if( zmsgIs(which,wheelbackward) ) {
			int scroll = max( 0, getI( "scroll" ) +5 );
			putI( "scroll", scroll );
			zMsgUsed();
			dirty();
		}
	}

}

//-------------------------------------------------------------------------------------------------

struct ZUIFilePicker : ZUIPanel {
	ZUI_DEFINITION(ZUIFilePicker,ZUIPanel);

	ZUIFileList *findFileList();
	ZUITextEditor *findTextEditor();

	ZUIFilePicker() : ZUIPanel() { }
	virtual void handleMsg( ZMsg *msg );
};

ZUI_IMPLEMENT(ZUIFilePicker,ZUIPanel);

ZUIFileList *ZUIFilePicker::findFileList() {
	for( ZUI *c=headChild; c; c=c->nextSibling ) {
		if( c->isS("class","ZUIFileList") ) {
			return (ZUIFileList*)c;
		}
	}
	return 0;
}

ZUITextEditor *ZUIFilePicker::findTextEditor() {
	for( ZUI *c=headChild; c; c=c->nextSibling ) {
		if( c->isS("class","ZUITextEditor") ) {
			return (ZUITextEditor*)c;
		}
	}
	return 0;
}

void ZUIFilePicker::handleMsg( ZMsg *msg ) {

	// NOTE: the use of "sendMsg" was the original implementation for standalone 
	// ZUIFilePicker that would send this message on OK or CANCEL, and in both
	// cases end modal / hide the picker.  But some clients will imbed this control
	// in a larger dialog, which they'll manage the modal state etc themselves.  In
	// this new use, the specific okMsg and cancelMsg is used. The main reason for
	// embedding is to use a standard dialog model based on ZUIPanel which uses 
	// styleDialog, & gets some special functionality (e.g. left-click drag) (tfb)

	if( zmsgIs(type,ZUIFilePicker_OK) ) {
		char *text = ZUI::zuiFindByName( getS("textEditorZUI") )->getS( "text", "" );
		if( has("okMsg") ) {
			zMsgQueue( "%s filespec='%s'", getS( "okMsg" ), escapeQuotes( text ) );
		}
		else if( has( "sendMsg" ) ) {
			zMsgQueue( "%s filespec='%s'", getS( "sendMsg" ), escapeQuotes( text ) );
			// CLEAR and hide if it is modal
			if( ZUI::modalStackTop > 0 && modalStack[modalStackTop-1] == this ) {
				modal( 0 );
				hidden = 1;
			}
		}
		zMsgUsed();
	}
	else if( zmsgIs(type,ZUIFilePicker_Cancel) ) {
		if( has("cancelMsg") ) {
			zMsgQueue( getS( "cancelMsg", "" ) );
		}
		else if( has( "sendMsg" ) ) {
			char *text = ZUI::zuiFindByName( getS("textEditorZUI") )->getS( "text", "" );
			zMsgQueue( "%s cancel=1 filespec='%s'", getS( "sendMsg" ), escapeQuotes( text ) );
			// CLEAR and hide if it is modal
			if( ZUI::modalStackTop > 0 && modalStack[modalStackTop-1] == this ) {
				modal( 0 );
				hidden = 1;
			}
		}
		zMsgUsed();
	}

	else if( zmsgIs(type,ZUIFilePicker_SetPath) ) {
		ZUIFileList *list = ZUIFilePicker::findFileList();
		if( list ) {
			list->openPath( zmsgS(path) );
			list->del( "userEnteredFilename" );
				// clear the userEnteredFilename since a SetPath msg is received when the filebox is
				// initially opening, and we don't want to have an old filename around from the last
				// use of this picker dialog, since the file type may be inappropriate.
		}
		zMsgQueue( "type=ZUITextEditor_SetText text='%s' toZUI=%s", escapeQuotes( zmsgS(path) ), getS("textEditorZUI") );
	}

	else if( zmsgIs(type,ZUIFileList_Selected) ) {
		// Sent from the lister saying that something was selected with the mouse
		// UPDATE the text editor to have this value
		zMsgQueue( "type=ZUITextEditor_SetText text='%s' toZUI=%s", escapeQuotes( zmsgS(filespec) ), getS("textEditorZUI") );
	}

	else if( zmsgIs(type,ZUIFilePickerTextChange) ) {
		// This can be a problem when the path in question contains quite a few files.
		// For example, try c:\windows\system32\ and then start typing...you'll choke
		// things up nicely, and the app becomes unresponsive.  Especially since you'll
		// type fast enough that multiple messages will arrive and you'll fully parse
		// them all before a single render occurs, there is no visual feedback and things
		// seem really stuck.  A trial solution: similar to windows, only reparse
		// everything and open that path when enter is pressed.

		// implmented by removing this, and adding a message that the texteditor sends
		// when enter has been pressed (ZUIFilePickerTextEnter)

		//ZUIFileList *list = ZUIFilePicker::findFileList();
		//list->openPath( zmsgS(text) );

		// Now I will save the filename from the currently entered text so that
		// if the user then clicks a new folder, we can cleverly retain the filename
		// and append it to the new path that has been selected:
		char *enteredPath = zmsgS( text );
		ZFileSpec fs( enteredPath );
		char *filename = fs.getFile();
		ZUIFileList *list = ZUIFilePicker::findFileList();
		if( filename && *filename ) {
			list->putS( "userEnteredFilename", filename );			
		}
		else {
			list->del( "userEnteredFilename" );
		}
		// this 
	}

	else if( zmsgIs(type,ZUIFilePickerTextEnter) ) {
		// 04aug2008: see issue above.  our texteditor now sends this message
		// when the user presses enter, and that is when we parse.

		char *path = zmsgS(text);
		//printf( "open path on %s\n", path );
		ZUIFileList *list = ZUIFilePicker::findFileList();
		list->openPath( path );

		// texteditors call cancelFocus when enter is pressed, but in our case,
		// we'd like our texteditor to retain focus.
		ZUITextEditor *textedit = findTextEditor();  assert( textedit );
		textedit->takeFocus( strlen( textedit->getS( "text", "" ) ) );
	}

	else if( zmsgIs(type,ZUIFilePicker_SetTitle ) ) {
		zMsgQueue( "type=ZUISet key=text val='%s' toZUI=%s", zmsgS(text), getS("labelTextZUI") );
	}

	else if( zmsgIs(type,Key) && zmsgIs(which,escape) ) {
		if( ZUI::modalStackTop > 0 && modalStack[modalStackTop-1] == this ) {
			modal( 0 );
			hidden = 1;
		}
	}
}

//----------------------------------------------------------------------------------------------
// The following implemenents a simple way for a client to pick a file.  It utilizes a default
// definition of a file picker dialog box defined in main.zui, available to all gui plugins.
// The caller should provide at minimum the okMsg, in the form of:
//
//      "type=SomeMessagename"
//
// This message will be received if the user clicks OK, and the message parameter 'filespec' will
// contain a string of the filename picked.  An optional message for cancel may be specified, but
// the filespec is not returned in this case.


void zuiChooseFile( char *dialogTitle, char *path, int openingFile, char *okMsg, char *cancelMsg/*=0*/, bool slideIn/*=false*/, char *dlgName/*=0*/ ) {

#ifdef NATIVE_FILE_DIALOGS
	#if defined( WIN32 ) || defined( __APPLE__ )
		chooseFileNative( dialogTitle, path, openingFile, okMsg, cancelMsg );
		return;
	#endif
#endif
	
	if( !dlgName ) {

		ZUI *z = ZUI::zuiFindByName( "zuiFilePickerDialog" );
		if( !z ) {
			char *filePickerScript = " \n\
				:zuiFilePickerDialog = ZUIPanel { \n\
					style  = styleDialog \n\
					hidden = 1 \n\
					parent = root \n\
					\n\
					layoutManual = 1 \n\
					layoutManual_x = W w - 2 / \n\
					layoutManual_y = H h - \n\
					layoutManual_w = 600 \n\
					layoutManual_h = H \n\
					*layout_cellFill = wh \n\
					layout_sizeToMax = wh \n\
					\n\
					: = ZUIFilePicker { \n\
						\n\
						layoutManual = 1 \n\
						layoutManual_x = 10 \n\
						layoutManual_y = 10 \n\
						layoutManual_w = W 20 -  \n\
						layoutManual_h = H 20 - \n\
						clipToBounds = 1 \n\
						layout = border \n\
						*layout_cellFill = wh \n\
						layout_sizeToMax = wh \n\
						panelColor = 0 \n\
						\n\
						textEditorZUI = zlabFileDialogPickerTextEditor \n\
						labelTextZUI = zlabFileDialogLabelText \n\
						\n\
						:zlabFileDialogLabelText = ZUIText { \n\
							border_side = n \n\
							font = header \n\
							textColor = 0x000000FF \n\
							text = \"Open File\" \n\
						} \n\
						\n\
						: = ZUIFileList { \n\
							panelFrame = 2 \n\
							panelColor = 0x808080FF \n\
							border_side = c \n\
							layout_forceH = H 10 - \n\
							fileListFolderColor = 0xFFFF00FF \n\
							fileListFileColor = 0xFFFFFFFF \n\
							fileListMouseColor = 0x0000FF20 \n\
							fileListSelectedColor = 0x80E080FF \n\
						} \n\
						\n\
						: = ZUIPanel { \n\
							border_side = s \n\
							panelColor = 0 \n\
							\n\
							: = ZUIButton { \n\
								text = \"OK\" \n\
								sendMsg = \"type=ZUIFilePicker_OK toZUIParents=1\" \n\
							} \n\
							: = ZUIButton { \n\
								text = \"Cancel\" \n\
								sendMsg = \"type=ZUIFilePicker_Cancel toZUIParents=1\" \n\
							} \n\
						} \n\
						\n\
						:zlabFileDialogPickerTextEditor = ZUITextEditor { \n\
							style = styleTextEdit \n\
							border_side = s \n\
							sendMsgOnChange = \"type=ZUIFilePickerTextChange toZUIParents=1\" \n\
							sendMsg = \"type=ZUIFilePickerTextEnter toZUIParents=1\" \n\
						} \n\
					} \n\
				} \n\
			";


			ZUI::zuiExecute( filePickerScript, 0 );
		}
		dlgName = "zuiFilePickerDialog";
	}
	ZUI *dlg = ZUI::zuiFindByName( dlgName );
	assert( dlg );
	if( slideIn ) {
		dlg->x = 0.f;
		// Animate in from the left
	}

	// SETUP the ZUIFilePicker control
	ZUI *zui = dlg->headChild;
	zui->putS( "okMsg", ZTmpStr("type=zuiDialogClose dialogName='%s'; %s", dlgName, okMsg) );
	zui->putS( "cancelMsg", ZTmpStr("type=zuiDialogClose dialogName='%s'; %s", dlgName, cancelMsg ? cancelMsg : "") );
	zMsgQueue( "type=ZUIFilePicker_SetPath path='%s' toZUI='%s'", escapeQuotes( path ), zui->name  );
	zMsgQueue( "type=ZUIFilePicker_SetTitle text='%s' toZUI='%s'", dialogTitle, zui->name );

	// GO MODAL
	zMsgQueue( "type=ZUIOrderAbove toMove='%s'", dlgName );
	zMsgQueue( "type=ZUIShow modal=1 toZUI='%s'", dlgName );
}



