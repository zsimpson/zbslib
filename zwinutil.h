// @ZBS {
//		*MODULE_OWNER_NAME zwinutil
// }

#ifndef ZWINUTIL_H
#define ZWINUTIL_H

// Process and threads
void zWinUtilStartProcess( char *name, char *commandLine, int waitTillComplete );
int zWinUtilStartThread( void *startRoutine, int argument );
int zwinUtilLaunchURL(char *url);
int zWinUtilFindWindowFromProcessId( unsigned long processId );

// Clipboard
void zWinUtilGetTextFromClipboard( char *buffer, int size );
void zWinUtilCopyTextToClipboard( char *buffer, int size );

// Memory maps
void *zWinUtilMemoryMapFileAsReadonly( char *filename );
void zWinUtilUnmapFile( void *p );

// Registry Tools
//int winUtilGetRegKey( char *regPath, char *dest );
int zWinUtilCreateRegKey( char *regPath );
int zWinUtilDeleteRegKey( char *regPath );
int zWinUtilSetRegString( char *regPath, char *value );
int zWinUtilSetRegValue( char *regPath, void *value, int length );
int zWinUtilSetRegDWORD( char *regPath, unsigned int val );
int zWinUtilGetRegString( char *regPath, char *value, int max_length );
int zWinUtilGetRegValue( char *regPath, void *value, int max_length );
int zWinUtilGetRegValueSize( char *regPath );
int zWinUtilRegIsEmpty (char *regPath);

// Misc.
int zWinUtilGetWindowsDirectory (char *buffer, unsigned int size);
void zWinUtilMessageBox( char *buffer, char *title=0 );
	// If no title is given, "Warning" is default
int zWinUtilYesNoBox( char *buffer );
int zWinUtilCreateShortcut( char *lpszPathObj, char *lpszPathLink, char *lpszDesc, char *workDir, char *args );
int zWinUtilRebootSystem();
int zWinUtilGetCommonProgramsFolderPath( char *buffer );
int zWinUtilGetStartupFolderPath( char *buffer );
void zWinUtilGetScreenMetrics( int &w, int &h, int &d );
	// Fetch display dimensions

// Message spoofing
void zWinUtilSpoofMouseEvent( int whichEvent, int x, int y );
	#define ZWINUTIL_SPOOF_MOUSE_L_DOWN (0)
	#define ZWINUTIL_SPOOF_MOUSE_L_UP   (1)
	#define ZWINUTIL_SPOOF_MOUSE_R_DOWN (2)
	#define ZWINUTIL_SPOOF_MOUSE_R_UP   (3)
	#define ZWINUTIL_SPOOF_MOUSE_MOVE   (4)

#endif

