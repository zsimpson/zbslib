// @ZBS {
//		*MODULE_OWNER_NAME zpython
// }

#ifndef ZPYTHON_H
#define ZPYTHON_H

#ifdef _DEBUG
#pragma message( "Turning off debug so python links." ) 
#define _ZPYTHON_DEBUG
#undef _DEBUG
#define NDEBUG
#endif

#include "python.h"
#include "methodobject.h"

#ifdef _ZPYTHON_DEBUG
#define _DEBUG
#undef NDEBUG
#pragma message( "Turning on debug" ) 
#endif


struct PythonRegister {
	unsigned char data[16];
	PythonRegister( char *_moduleName, char *_methodName, PyCFunction _method );
};

typedef void StdoutCallback(char *);

struct PythonInterpreter {
	void startup( StdoutCallback *stdoutCallback );
	int isBusy();
	void halt();
	void run( char *pythonCode );
	void runFile( char *pythonFile );
	void shutdown();
};

extern PythonInterpreter pythonInterpreter;

class ZHashTable;
extern void pythonConsoleStart(ZHashTable *_config, char *defaultDirectory);
extern void pythonCheckForUpdatedFile();
extern void pythonSetKeystroke( int keystroke );
#endif
