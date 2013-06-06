// @ZBS {
//		*MODULE_OWNER_NAME zcsvfile
// }

#ifndef ZCSVFILE_H
#define ZCSVFILE_H

char **zcsvFileParse( char *buf, int &rowCount, int &colCount, int tabMode=0 );
char **zcsvFileRead( char *filename, int &rowCount, int &colCount, int tabMode=0 );
	// In tab mode, uses tab for separator and does not remove quotes from strings
void zcsvFileFree( char **ptrs );

#endif

