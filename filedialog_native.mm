#include "zmsg.h"
#include "zfilespec.h"
#import <Cocoa/Cocoa.h>

void GetOpenFileFromUser( char *title, char *path, char *file, char *ext, char *okMsg, char *cancelMsg ) {
	
	
	int i;
	NSOpenPanel* openDlg = [NSOpenPanel openPanel];
	
	NSString *nsPath = [NSString stringWithUTF8String:path];
	NSString *nsFile = [NSString stringWithUTF8String:file];
	NSString *nsExt  = [NSString stringWithUTF8String:ext];
	
	[openDlg setCanChooseFiles:YES];
	[openDlg setResolvesAliases: true];
	[openDlg setAllowsMultipleSelection: false];
	[openDlg setCanChooseDirectories:NO];
	[openDlg setAllowedFileTypes:[NSArray arrayWithObjects:nsExt, nil]];
	[openDlg setDirectoryURL:[NSURL fileURLWithPath:nsPath]];

	// Display the dialog.  If the OK button was pressed,
	// process the files.
	if ( [openDlg runModal] == NSOKButton ) {
	    // Get an array containing the full filenames of all
	    // files and directories selected.
		NSArray* files = [openDlg filenames];
		
	    // Loop through all the files and process them: (for future possibility of
		// multiple selection, which is disabled above)
	    for( i = 0; i < [files count]; i++ ) {
        	NSString* fileName = [files objectAtIndex:i];
			if( okMsg ) {
				const char *utf8 = [fileName fileSystemRepresentation];
				zMsgQueue( "%s osx=1 filespec='%s'", okMsg, escapeQuotes( (char*)utf8 ) );
					// on OSX, the confirm to overwrite existing files happens as part of the native dialog
			}
		}
	}
	else {
		if( cancelMsg ) {
			zMsgQueue( cancelMsg );
		}
	}
}


void GetSaveFileFromUser( char *title, char *path, char *file, char *ext, char *okMsg, char *cancelMsg ) {
	
	int i;
	NSSavePanel* saveDlg = [NSSavePanel savePanel];
	
	NSString *nsPath = [NSString stringWithUTF8String:path];
	NSString *nsFile = [NSString stringWithUTF8String:file];
	NSString *nsExt  = [NSString stringWithUTF8String:ext];

	[saveDlg setAllowedFileTypes:[NSArray arrayWithObjects:nsExt, nil]];
	[saveDlg setNameFieldStringValue:nsFile];

	if ( [saveDlg runModal] == NSOKButton ) {
	    // Get an array containing the full filenames of all
	    // files and directories selected.
        NSArray* files = [saveDlg filenames];
        if( okMsg ) {
            NSString* fileName = [files objectAtIndex:0];
            const char *utf8 = [fileName fileSystemRepresentation];
			zMsgQueue( "%s overwriteExisting=1 osx=1 filespec='%s'", okMsg, escapeQuotes( (char*)utf8 ) );
				// on OSX, the confirm to overwrite existing files happens as part of the native dialog
		}
	}
	else {
		if( cancelMsg ) {
			zMsgQueue( cancelMsg );
		}
	}
}

void chooseFileNative( char *title, char *path, int openingFile, char *okMsg, char *cancelMsg ) {
	
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSWindow *keyWindow = [NSApp keyWindow];
	NSOpenGLContext *gl = [NSOpenGLContext currentContext];
	
	ZFileSpec fs( path );
	
	char dir[256];
	strncpy( dir, fs.getDir(), 255 );
	
	if( openingFile ) {
		GetOpenFileFromUser( title, dir, fs.getFile(), fs.getExt(), okMsg, cancelMsg );
	}
	else {
		GetSaveFileFromUser( title, dir, fs.getFile(), fs.getExt(), okMsg, cancelMsg );
	}
	
	[gl makeCurrentContext];
	[keyWindow makeKeyWindow];
	[[NSCursor arrowCursor] set];
	[pool release];
}
