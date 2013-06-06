
#ifndef ZGRAPHFILE_H
#define ZGRAPHFILE_H

void zGraphFileInit();
	// It is not necessary to call this before you call the below but
	// it is nice at times for memory heap debugging as it gets all
	// of the freeimage calls done before any memory mark.

int zGraphFileLoad( char *filename, struct ZBits *desc, int descOnly=0, int filenameToLower=1 );
int zGraphFileSave( char *filename, struct ZBits *desc, int quality=100, int filenameToLower=1 );

int zGraphFileLoad( char *filename, struct ZBitmapDesc *desc, int descOnly=0, int filenameToLower=1 );
int zGraphFileSave( char *filename, struct ZBitmapDesc *desc, int quality=100, int filenameToLower=1 );
	// These are deprecated.  I need to remove all references to ZBitmapDesc

#endif
