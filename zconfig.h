// @ZBS {
//		*MODULE_OWNER_NAME zconfig
// }

#ifndef ZCONFIG_H
#define ZCONFIG_H

#include "zhashtable.h"

void zConfigParse( char *info, ZHashTable& hash, int markAsChanged=0 );
	// Actually implements the parsing described below

int zConfigLoadFile( char *filename, ZHashTable& hash, int markAsChanged=0 );
	// Loads all of the key/values in filename into
	// globalOptions.  The "markAsChanged" is useful for
	// marking everything that comes out of options.cfg
	// as changed to ensure that it will get written
	// out again.
	//
	// The config files are key/value combinations that
	// are formated as follows:
	// 
	// key1 = value that is long
	// key2 = "value that is long"
	// "key 3" = vale
	// [key4]
	// This is a multiline
	// value that ends at the close square
	// or at the beginning of the next bracketed key
	// []
	// key5 = Oink
	// 
	// Comments are allowed with double slash

int zConfigSaveFile( char *filename, ZHashTable &table );

#endif

