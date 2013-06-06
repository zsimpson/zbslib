// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Native file open/save dialog functionality
//		}
//		*PORTABILITY win32 osx 
//		*REQUIRED_FILES filedialog_native.cpp filedialog_native.h
//		NOTE filedialog_native.cpp will be replaced by the Objective-C .mm version on OSX 10.5 and greater.
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//			Add linux functionality (GTK?)
//			Cleanup
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }

// OPERATING SYSTEM specific includes:
#ifdef WIN32
	#include "windows.h" 
	#include "direct.h"
	#include "GL/glfw.h" // to get at the hwnd - perhaps the hwnd should be stuffed into options in main()?
#endif

#ifdef __APPLE__
	#include <Carbon/Carbon.h>
#endif

// SDK includes:
// STDLIB includes:

// MODULE includes:
#include "filedialog_native.h"

// ZBSLIB includes:
#include "zmsg.h"
#include "zfilespec.h"





//=======================================================================================
// Windows
#ifdef WIN32

char * getWindowsFilterStringFromPath( char *path ) {
	ZFileSpec fs( path );
	char *ext = fs.getExt();

	static char *allFiles = "All Files (*.*)\0*.*\0";

	if( ! *ext ) {
		return allFiles;
	}

	// Special case any file types we want special names for
	if( !stricmp( ext, "txt" ) ) {
		return "Text Files (*.txt)\0*.txt\0" "All Files (*.*)\0*.*\0";
	}

	// For other extensions, we'll compose an upper case name, e.g. .pth will become "PTH Files (*.pth)"
	static char filterName[64];
	memset( filterName, 0, 64 );
	strcpy( filterName, ext );
	char *p = filterName;
	while( *p ) {
		*p = toupper( *p );
		p++;
	}
	sprintf( p, " Files (*.%s)", ext );
	while( *p++ );
	*p++ = '*';
	*p++ = '.';
	strcpy( p, ext );
	while( *p++ );

	// and we'll always add the option to see all files, because it's annoying when you cant...
	memcpy( p, allFiles, 20 );

	return filterName;
}

void chooseFileNative( char *title, char *path, int openingFile, char *okMsg, char *cancelMsg ) {
	char szFileName[MAX_PATH] = "";
	ZFileSpec fs( path );
	if( !openingFile ) {
		strcpy( szFileName, fs.getFile() );
	}

	// Windows changes the working folder when you choose a file to open or 
	// save.  This makes using relative paths in our apps a pain, so I'm going
	// to save the cwd and restore it to defeat this behavior.
	char cwd[1024];
	getcwd( cwd, 1024 );
	extern void trace( char *msg, ... );

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = (HWND)glfwZBSExt_GetHWND();
	ofn.lpstrFilter = getWindowsFilterStringFromPath( path );
	ofn.lpstrFile = szFileName;
	ofn.lpstrTitle = title;

	// Windows will ignore out initDir request unless we use backslashes, and
	// ensure there is no trailing slash...
	extern char * convertSlashes( char *, char c=0 );
	extern char * stripTrailingSlashes( char * );
		// these are in ZFileSPec, and were static, but I changed that use use it here. (tfb)
	ofn.lpstrInitialDir = stripTrailingSlashes( convertSlashes( fs.getDir(), '\\' ) );
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY;
	if( openingFile ) {
		ofn.Flags = ofn.Flags | OFN_FILEMUSTEXIST;
	}
	ofn.lpstrDefExt = fs.getExt();

	int status = 0;
	if( openingFile ) {
		status = GetOpenFileName( &ofn );
	}
	else {
		status = GetSaveFileName( &ofn );
	}
	if( status ) {
		if( okMsg ) {
			zMsgQueue( "%s filespec='%s'", okMsg, escapeQuotes( convertSlashes( szFileName, '/' ) ) );
		}
	}
	else if( cancelMsg ) {
		zMsgQueue( cancelMsg );
	}

	zFileSpecChdir( cwd );
	getcwd( cwd, 1024 );
}

#endif

//=======================================================================================
// OSX

#ifdef __APPLE__ 
#if defined( __LP64__ )
	// 64-bit Cocoa-based code is in filedialog_native.mm
#else
		// 32-bit Carbon-based dialogs for OSX: see filedialog_native.m for Cocoa
		// TODO: clean this up a bit - code grabbed from various examples online
		// TODO: this is a Carbon impl; this means 32bit only; when we move to
		// Cocoa-based GLFW we'll need to update this to use the Cocoa methods.

/*
struct NavDialogCreationOptions {
	UInt16  version;
	NavDialogOptionFlags optionFlags;
	Point   location;
	CFStringRef  clientName;
	CFStringRef  windowTitle;
	CFStringRef  actionButtonLabel;
	CFStringRef  cancelButtonLabel;
	CFStringRef  saveFileName;
	CFStringRef  message;
	UInt32  preferenceKey;
	CFArrayRef  popupExtension;
	WindowModality modality;
	WindowRef  parentWindow;
	char   reserved[16];
};
 */

void GetOpenFileFromUser( char *title, char *path, char *okMsg, char *cancelMsg ) {
	// Allow the user to choose an existing file
	
	NavDialogCreationOptions dialogOptions;
	NavDialogRef dialog;
	NavReplyRecord replyRecord;
	CFURLRef fileAsCFURLRef = NULL;
	FSRef fileAsFSRef;
	OSStatus status;
	UInt8 output_dir_name[1024];
	
	ZFileSpec fs( path );
	
	
	// Get the standard set of defaults
	status = NavGetDefaultDialogCreationOptions(&dialogOptions);
	require_noerr( status, CantGetNavOptions );
	
	// Make the window app-wide modal
	dialogOptions.modality = kWindowModalityAppModal;

	dialogOptions.windowTitle = CFStringCreateWithCString( NULL, title, 0 );
	dialogOptions.saveFileName = CFStringCreateWithCString( NULL, fs.getFile(), 0 ); 
	
	// Create the dialog
	status = NavCreateGetFileDialog(&dialogOptions, NULL, NULL, NULL, NULL, NULL, &dialog);
	require_noerr( status, CantCreateDialog );
	
	// Show it
	status = NavDialogRun(dialog);
	require_noerr( status, CantRunDialog );
	
	// Get the reply
	status = NavDialogGetReply(dialog, &replyRecord);
	require( ((status == noErr) || (status == userCanceledErr)), CantGetReply );
	
	// If the user clicked "Cancel", just bail
	if ( status == userCanceledErr ) {
		if( cancelMsg ) {
			zMsgQueue( cancelMsg );
			NavDialogDispose( dialog );
			return;
		}
	}
	
	// Get the file
	status = AEGetNthPtr(&(replyRecord.selection), 1, typeFSRef, NULL, NULL, &fileAsFSRef, sizeof(FSRef), NULL);
	require_noerr( status, CantExtractFSRef );
	
	FSRefMakePath( &fileAsFSRef, output_dir_name, 1024 );
	if( okMsg ) {
		zMsgQueue( "%s osx=1 filespec='%s'", okMsg, escapeQuotes( (char*)output_dir_name ) );
	}
	
	// Cleanup
CantExtractFSRef:
UserCanceled:
	verify_noerr( NavDisposeReply(&replyRecord) );
CantGetReply:
CantRunDialog:
	NavDialogDispose(dialog);
CantCreateDialog:
CantGetNavOptions:
	return;
}

void GetSaveFileFromUser( char *title, char *path, char *okMsg, char *cancelMsg ) {
	// Allow the user to specify a name to save a file - the file need not already
	// exist.
	
	NavDialogCreationOptions dialogOptions;
	NavDialogRef dialog;
	NavReplyRecord replyRecord;
	CFURLRef fileAsCFURLRef = NULL;
	FSRef fileAsFSRef;
	OSStatus status;
	UInt8 output_dir_name[1024];
	UniChar output_filename[255];
	CFIndex len = 255;
	int i,pathlen;
	
	ZFileSpec fs( path );
	//char *debug = fs.getFile();

	
	// Get the standard set of defaults
	status = NavGetDefaultDialogCreationOptions(&dialogOptions);
	require_noerr( status, CantGetNavOptions );
	
	// Make the window app-wide modal
	dialogOptions.modality = kWindowModalityAppModal;
	
	dialogOptions.windowTitle = CFStringCreateWithCString( NULL, title, 0 );
	dialogOptions.saveFileName = CFStringCreateWithCString( NULL, fs.getFile(), 0 ); 
	//dialogOptions.optionFlags |= kNavPreserveSaveFileExtension;
	dialogOptions.optionFlags |= kNavNoTypePopup;
	
	// Create the dialog
	status = NavCreatePutFileDialog(&dialogOptions, NULL, NULL, NULL, NULL, &dialog);
	require_noerr( status, CantCreateDialog );
	
	// Show it
	status = NavDialogRun(dialog);
	require_noerr( status, CantRunDialog );
	
	// Get the reply
	status = NavDialogGetReply(dialog, &replyRecord);
	require( ((status == noErr) || (status == userCanceledErr)), CantGetReply );
	
	// If the user clicked "Cancel", just bail
	if ( status == userCanceledErr ) {
		if( cancelMsg ) {
			zMsgQueue( cancelMsg );
			NavDialogDispose( dialog );
			return;
		}
	}
	
	// Get the file
	status = AEGetNthPtr(&(replyRecord.selection), 1, typeFSRef, NULL, NULL, &fileAsFSRef, sizeof(FSRef), NULL);
	require_noerr( status, CantExtractFSRef );
	
	// Get folder name
	FSRefMakePath( &fileAsFSRef, output_dir_name, 1024 );
	pathlen = strlen( (const char *)output_dir_name );
	output_dir_name[ pathlen++ ] = '/';
	
	
	// Get filename
	len = CFStringGetLength( replyRecord.saveFileName);
	for( i=0; i<len && pathlen < 1023; i++ ) {
		output_dir_name[ pathlen++ ] = CFStringGetCharacterAtIndex( replyRecord.saveFileName, i );
	}
	output_dir_name[ pathlen ] = 0;
	if( okMsg ) {
		zMsgQueue( "%s overwriteExisting=1 osx=1 filespec='%s'", okMsg, escapeQuotes( (char*)output_dir_name ) );
			// on OSX, the confirm to overwrite existing files happens as part of the native dialog 
	}
	
	
	// Cleanup
CantExtractFSRef:
	verify_noerr( NavDisposeReply(&replyRecord) );
CantGetReply:
CantRunDialog:
	NavDialogDispose(dialog);
CantCreateDialog:
CantGetNavOptions:
	return;
}

void chooseFileNative( char *title, char *path, int openingFile, char *okMsg, char *cancelMsg ) {
	if( openingFile ) {
		GetOpenFileFromUser( title, path, okMsg, cancelMsg ); 
	}
	else {
		GetSaveFileFromUser( title, path, okMsg, cancelMsg ); 
	}
	SetThemeCursor( 0 );
}
#endif  // !__LP64__

#endif	// __APPLE__


//=======================================================================================
// Other - using GTK?

#if !defined( WIN32 ) && !defined(__APPLE__)
/*

// something like this for GTK on linux

	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new ("Open File",
			NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL);

	if(gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		// do something with filename
		g_free (filename);
	}
	gtk_widget_destroy(dialog);
*/
#endif

