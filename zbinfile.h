// @ZBS {
//		*MODULE_OWNER_NAME zglfont
// }

#ifndef ZBINFILE_H
#define ZBINFILE_H

void zbinFileWrite( char *filename, void *a, int len );
void *zbinFileRead( char *filename, void *into, int &len );
	// Read into to len, if into is null it will be allocated to file size. Use len==-1 to determine filesize

#endif
