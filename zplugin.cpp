// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A system for managing zPlugins.  It provides a way for zPlugins to
//			declare themselves and their interfaces into hash table that
//			can be searched by the master application.
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zplugin.cpp zplugin.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "stdlib.h"
#include "string.h"
// MODULE includes:
#include "zplugin.h"
// ZBSLIB includes:

ZHashTable *zPlugins = 0;

// These two functions are part of the auto-registeration
// system of the zPlugins.
// Plugins register themselves with macros that secretly create
// globals whose constructors stuff values into hastable

ZPluginRegister::ZPluginRegister( char *name, ZHashTable *propertyTable ) {
	if( !zPlugins ) {
		zPlugins = new ZHashTable;
	}
	zPlugins->putP( name, propertyTable );
}

ZPluginExportRegister::ZPluginExportRegister( char *symbol, void* ptr, ZHashTable *propertyTable ) {
	propertyTable->putP( symbol, ptr );
}

ZPluginExportRegister::ZPluginExportRegister( char *key, char *val, ZHashTable *propertyTable ) {
	propertyTable->putS( key, val );
}

int zPluginEnum( int &last, ZHashTable *&plugin ) {
    if( !zPlugins ) return 0;
	last++;
	if( last >= zPlugins->size() ) {
		return 0;
	}
	while( last < zPlugins->size() ) {
		char *key = zPlugins->getKey( last );
		ZHashTable * val = (ZHashTable*)zPlugins->getValp( last );
		if( key ) {
			plugin = val;
			return 1;
		}
		last++;
	}
	return 0;
}

