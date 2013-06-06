// @ZBS {
//		*MODULE_OWNER_NAME zhashtable
// }

#ifndef ZPLUGIN_H
#define ZPLUGIN_H

#include "zhashtable.h"

extern ZHashTable *zPlugins;
	// This contains the names of each plugin and
	// a pointer to their property hash

struct ZPluginRegister {
	ZPluginRegister( char *name, ZHashTable *propertyTable );
};

struct ZPluginExportRegister {
	ZPluginExportRegister( char *symbol, void* ptr, ZHashTable *propertyTable );
	ZPluginExportRegister( char *key, char *val, ZHashTable *propertyTable );
};

// MACROS used to declare plugins
//-----------------------------------------------------------------------

#define ZPLUGIN_BEGIN( name ) \
	static ZHashTable __##name##PropertyTable; \
	static ZPluginRegister __##name##__( #name, &__##name##PropertyTable ); \
	namespace name { \
	ZHashTable *propertyTable = &__##name##PropertyTable; \
	ZPluginExportRegister __name( "name", #name, propertyTable ); \
	static char *thisPluginName = #name;

#define ZPLUGIN_END }


#define ZPLUGIN_EXPORT_SYMBOL( symbol ) \
	ZPluginExportRegister __##symbol( #symbol, (void*)symbol, propertyTable );


#define ZPLUGIN_EXPORT_PROPERTY( key, val ) \
	ZPluginExportRegister __##key( #key, val, propertyTable );


// Helper functions used to get/typecast plugin properties
//-----------------------------------------------------------------------

int zPluginEnum( int &last, ZHashTable *&plugin );
	//EXAMPLE:
	// int last = -1;
	// ZHashTable *plugin = 0;
	// while( pluginEnum( last, plugin ) ) {
	//   printf( "%s\n", plugin->getS("name") );
	// }

inline ZHashTable *zPluginGetPropertyTable( char *name ) {
	if( zPlugins ) {
		return (ZHashTable *)zPlugins->getp( name );
	}
	return 0;
}

inline void *zPluginGetP( char *pluginName, char *propertyKey ) {
	ZHashTable *plugin = zPluginGetPropertyTable( pluginName );
	if( plugin ) {
		return (void *)plugin->getp( propertyKey );
	}
	return 0;
}

inline int zPluginGetI( char *pluginName, char *propertyKey ) {
	ZHashTable *plugin = zPluginGetPropertyTable( pluginName );
	if( plugin ) {
		return (int)plugin->getI( propertyKey );
	}
	return 0;
}

/*
//EXAMPLE USAGE:
//
//  Somewhere you wish to declare a plugin with a specific set of
//properties which might include variables and function pointers.
//  You agree on conventions for these interfaces in some way outside
//the scope of this module.
//  For example, imagine we have agreed on an interface called: "oinkInterface"
//and that this will have "oinkInterface = 1" to declare itself as well as
//two function pointers.  One called "void (*startup)()" the other "void (*update)(int)"
//  So, in some file you declare the plugin like:

PLUGIN_BEGIN( myPlugin );
  // Note that this creates a "myPlugin" namespace so you don't have to worry about collisions

void startup() {
	printf( "We're in startup!\n" );
}

void update( int i ) {
	printf( "We're in update %d!\n", i );
}

PLUGIN_EXPORT_SYMBOL( startup );
PLUGIN_EXPORT_SYMBOL( update );
PLUGIN_EXPORT_PROPERTY( oinkInterface, "1" );
PLUGIN_END;

//  Now, in some module where you wish to access this plugin
//you can get a pointer to the property table and then a 
//pointer to the symbols as this example demonstrates:

ZHashTable *plugin = pluginGetPlugin( "myPlugin" );
assert( plugin );

int hasOinkInterface = plugin->getI( "hasOinkInterface" );
assert( hasOinkInterface );

typedef void (*StartupFnPtr)();
StartupFnPtr startup = (StartupFnPtr)plugin->getI( "startup" );
(*startup)();

typedef void (*UpdateFnPtr)( int );
UpdateFnPtr update = (UpdateFnPtr)plugin->getI( "update" );
(*update)( 1 );

*/

// Shadow Garden Interface:
	//shadowGardenInterface = 1
	//void (*startup)();
	//void (*shutdown)();
	//void (*update)();
	//void (*render)();
	//void (*setupView)();
	//void (*handleMsg)( TMsg * );


#endif

