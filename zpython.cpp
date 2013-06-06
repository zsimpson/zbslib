// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			The python interface
//		}
//		*PORTABILITY win32
//		*REQUIRED_FILES zpython.cpp zpython.h
//		*GROUP modules/zpython
//		*VERSION 1.0
//		+HISTORY {
//			Added by Ken July 2012
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }

#include "zpython.h"
#include "pthread.h"
#include "assert.h"
#include "stdio.h"
#include "float.h"
#include "stdlib.h"
#include "new.h"
#ifdef WIN32
	#include "malloc.h"
#endif
#include "ztmpstr.h"
#include "ztime.h"
#include "zfilespec.h"
#include "zmsg.h"
#include "zui.h"
#include "mainutil.h"

static char *INTERNAL_PYTHON_MODULE = "zlab";
static int gDebug = 0;

// fileRead()
//----------------------------------------------------------------
char *fileRead( const char *fileName ) {
	FILE * pFile;
	long lSize;
	char * buffer;
	size_t result;

	pFile = fopen ( fileName , "rb" );
	if (pFile==NULL) {
		return 0;
	}

	// obtain file size:
	fseek (pFile , 0 , SEEK_END);
	lSize = ftell (pFile);
	rewind (pFile);

	// allocate memory to contain the whole file:
	buffer = (char*) malloc (sizeof(char)*(lSize+1));
	if (buffer == NULL) {
		return 0;
	}

	// copy the file into the buffer:
	result = fread (buffer,1,lSize,pFile);
	if (result != lSize) {
		return 0;
	}

	/* the whole file is now loaded in the memory buffer. */

	// terminate
	fclose (pFile);
	buffer[lSize] = '\0';

	return buffer;
}

// PythonReg
//----------------------------------------------------------------
struct PythonReg {
	static PythonReg *root;

	PythonReg *next;
	char *moduleName;
	char *methodName;
	PyCFunction method;

	PythonReg( char *_moduleName, char *_methodName, PyCFunction _method ) {
		assert( _moduleName && _methodName && _method );
		next = root;
		root = this;
		moduleName = _moduleName;
		methodName = _methodName;
		method = _method;
	}

	static void getModuleList(ZHashTable &hash) {
		PythonReg *reg = root;
		while( reg ) {
			if( !hash.has( reg->moduleName ) ) {
				hash.putS( reg->moduleName, reg->moduleName );
			}
			reg = reg->next;
		}
	}

	static void registerAll() {
		ZHashTable moduleList;
		getModuleList(moduleList);

		for( ZHashWalk n( moduleList ); n.next(); ) {
			char *currentModule = n.key;
			int count = 0;
			PythonReg *reg = root;
			while( reg ) {
				if( !strcmp(currentModule,reg->moduleName) )
					count += 1;
				reg = reg->next;
			}

			PyMethodDef *methodList = new PyMethodDef[count+1];
			memset( methodList, 0, sizeof( PyMethodDef ) * (count+1) );

			reg = root;
			int index = 0;
			while( reg ) {
				if( !strcmp(currentModule,reg->moduleName) ) {
					methodList[index].ml_meth = reg->method;
					methodList[index].ml_name = reg->methodName;
					methodList[index].ml_flags = METH_VARARGS;
					methodList[index].ml_doc = 0;
					assert( index < count );
					index += 1;
				}
				reg = reg->next;
			}

			if( gDebug )
				trace( "Registering %d methods in %s\n", count, currentModule );
			Py_InitModule( currentModule, methodList );
		}
	}
};
PythonReg *PythonReg::root = 0;

PythonRegister::PythonRegister( char *_moduleName, char *_methodName, PyCFunction _method ) {
	assert( sizeof(PythonRegister) == sizeof(PythonReg) );
	new ( (void*)&this->data[0] ) PythonReg( _moduleName, _methodName, _method );
}

void *pythonThreadMain( void *args );
int pythonThreadHalt(void *);

// PythonRunner
// A safe class with margins to make sure we aren't getting corrupted...
//----------------------------------------------------------------
class PythonRunner {
	static const int MARGIN = 32;
	static const int PATTERN_CHAR = 0x5E;
	static const int PATTERN_INT = 0x5E5E5E5E;
	char *head;
	char *code;
	char *tail;
	pthread_t threadID;
	int running;

public:	
	PythonRunner() {
		head = 0;
		code = 0;
		tail = 0;
		running = 0;
	}

	int isRunning() {
		return running;
	}

	int waitForCompletion() {
		int reps = 5000 / 100;	// five seconds
		while( reps>0 && isRunning() ) {
			zTimeSleepMils( 100 );
			if( gDebug )
				trace("*");
			reps--;
		}
		if( isRunning() ) {
			trace("ERROR: Python code is already running.  Wait for it to finish.");
			return 0;
		}
		return 1;
	}

	int set(char *_pythonCode) {
		if( !waitForCompletion() )
			return 0;

		assert( head == 0 );
		head = new char[MARGIN+strlen(_pythonCode)+1+MARGIN];
		code = head + MARGIN;
		tail = head + MARGIN + strlen(_pythonCode)+1;
		memset( head, PATTERN_CHAR, MARGIN );
		memset( tail, PATTERN_CHAR, MARGIN );
		memcpy( code,_pythonCode,strlen(_pythonCode)+1);
		validate(head);
		validate(tail);
		return 1;
	}

	void validate(char *_ptr) {
		int *ptr = (int*)_ptr;
		int *end = ptr + (MARGIN * sizeof(char) / sizeof(int));
		while( ptr != end ) {
			assert( *ptr == PATTERN_INT );
			ptr++;
		}
	}

	void halt() {
		if( !isRunning() )
			return;
		PyGILState_STATE state = PyGILState_Ensure();
		Py_AddPendingCall(&pythonThreadHalt, NULL);
		PyGILState_Release(state);
	}

	void run() {
		if( !waitForCompletion() )
			return;

		assert( code != 0 );
		running = 1;
		enable(0,0,0,1);

		validate(head);
		validate(tail);
		if( gDebug && strlen(code)<128 ) 
			traceNoFormat( ZTmpStr(" PY: %s", code) );

		// LAUNCH a thread to open the file as it can take a long time.
		pthread_attr_t attr;
		pthread_attr_init( &attr );
		pthread_attr_setstacksize( &attr, 16*4096 );
		pthread_create( &threadID, &attr, &pythonThreadMain, code );
		pthread_attr_destroy( &attr );
	}

	void enable(int o, int c, int r, int h) {
		zMsgQueue( "type=%s toZUI=pythonOpenButton", o ? "ZUIEnable" : "ZUIDisable" );
		zMsgQueue( "type=%s toZUI=pythonClearButton", c ? "ZUIEnable" : "ZUIDisable" );
		zMsgQueue( "type=%s toZUI=pythonRunButton", r ? "ZUIEnable" : "ZUIDisable" );
		zMsgQueue( "type=%s toZUI=pythonHaltButton", h ? "ZUIEnable" : "ZUIDisable" );
	}

	void cleanup() {
//		if( gDebug )
//			trace("Cleaning up...");
		validate(head);
		validate(tail);
		delete head;
		head = tail = code = 0;
		running = 0;
		enable(1,1,1,0);
//		if( gDebug )
//			trace("done");
	}
};
PythonRunner pythonRunner;

int pythonThreadHalt(void *) {
    PyErr_SetInterrupt();
	return -1;
}

void *pythonThreadMain( void *args ) {
	char *code = (char *)args;
	PyRun_SimpleString( code );
	pythonRunner.cleanup();
	return 0;
}



// PythonInterpreter
//----------------------------------------------------------------
static StdoutCallback *stdoutCallback = 0;


void PythonInterpreter::startup(StdoutCallback *_stdoutCallback) {
	stdoutCallback = _stdoutCallback;

	gDebug = options.getI("pythonDebug");

	Py_Initialize();
	PythonReg::registerAll();

	ZFileSpec path( pluginPath("") );
	path.makeAbsolute(true);

	run( ZTmpStr( "zlabRunning=1\n" ).s );

	run( ZTmpStr( "from sys import path as _pyPath\n_pyPath.append(\"%s\")\n", path.get() ).s );

	run( ZTmpStr( "from %s import zlabHookedPrint\n", INTERNAL_PYTHON_MODULE ).s );

/*
	ZHashTable moduleList;
	PythonReg::getModuleList(moduleList);
	for( ZHashWalk n( moduleList ); n.next(); ) {
		if( strcmp(n.key,INTERNAL_PYTHON_MODULE) )
			run( ZTmpStr("import %s\n",n.key).s );
	}
*/
	char *pythonStartupFile = zlabCorePath( "main.py" );
	runFile( pythonStartupFile );

	zMsgQueue( "type=ZUISet key=text val='%s' toZUI=pythonDebugNote", gDebug ? "(DEBUGGING)" : "" );
}

int PythonInterpreter::isBusy() {
	return pythonRunner.isRunning();
}

void PythonInterpreter::halt() {
	if( gDebug  ) 
		traceNoFormat( " PY: halting..." );
	pythonRunner.halt();
	if( gDebug  ) 
		traceNoFormat( " done\n" );
}

void PythonInterpreter::run( char *_pythonCode ) {
	if( pythonRunner.set(_pythonCode) )
		pythonRunner.run();
}

void PythonInterpreter::runFile( char *pythonFile ) {
	char *script = fileRead( pythonFile );
	int ok = pythonRunner.set(script);
	delete script;
	if( ok )
		pythonRunner.run();
}

void PythonInterpreter::shutdown() {
	if( pythonRunner.isRunning() )
		pythonRunner.halt();
	Py_Finalize();
}

PythonInterpreter pythonInterpreter;

// PythonConfig
//----------------------------------------------------------------
static ZHashTable *pythonConfig = 0;


// Python Methods callable from python script, and registered as part of module 'zlab'
//----------------------------------------------------------------

// --- zlabHookedPrint -------------------
// Hooks the python print function (stdout)
static PyObject *ZLabModule_zlabHookedPrint(PyObject *self, PyObject *args) {
	char *val = 0;
	PyArg_ParseTuple(args,"s",&val);
	if( val ) {
		if( stdoutCallback ) {
			stdoutCallback(val);
		}
		else {
			traceNoFormat( val );
		}
	}

	return Py_BuildValue("s", val ? val : "");
}

PythonRegister register_print( INTERNAL_PYTHON_MODULE,"zlabHookedPrint",&ZLabModule_zlabHookedPrint);

// --- printConsole -------------------
static PyObject *ZLabModule_printConsole(PyObject *self, PyObject *args) {
	char *val = 0;
	PyArg_ParseTuple(args,"s",&val);
	if( val ) {
		traceNoFormat( val );
		zMsgQueue( "type=ZUISet key=text val='%s' toZUI=pythonOutput", val );
	}

	return Py_BuildValue("s", val ? val : "");
}

PythonRegister register_printConsole( INTERNAL_PYTHON_MODULE, "printConsole", &ZLabModule_printConsole);

// --- message -------------------
static PyObject *ZLabModule_message(PyObject *self, PyObject *args) {
	char *val = 0;
	PyArg_ParseTuple(args,"s",&val);
	if( val ) {
		traceNoFormat( val );
		zMsgQueue( val );
		return Py_BuildValue("i", 1);
	}

	return Py_BuildValue("i", 0);
}

PythonRegister register_message( INTERNAL_PYTHON_MODULE, "message", &ZLabModule_message);

// --- optionGet -------------------
static PyObject *ZLabModule_optionGet(PyObject *self, PyObject *args) {
	char *optionKey = 0;
	char result[256];
	result[0] = '\0';

	PyArg_ParseTuple(args,"s",&optionKey);
	if( !optionKey || strlen(optionKey) == 0 ) {
		trace( "ERROR: no key specified in call to zlab.optionGet()\n" );
	}
	else {
		char *optionValue = options.getS(optionKey);
		if( !optionValue ) {
			if( gDebug )
				trace( "Warning: No value set in options.cfg for key '%s'\n", optionKey );
		}
		else {
			strcpy( result, options.getS(optionKey) );
		}
	}
	return Py_BuildValue("s", result);
}

PythonRegister register_optionGet( INTERNAL_PYTHON_MODULE, "optionGet", &ZLabModule_optionGet);

// --- configGet -------------------
static PyObject *ZLabModule_configGet(PyObject *self, PyObject *args) {
	char *configKey = 0;
	char configValue[256];
	configValue[0] = '\0';

	PyArg_ParseTuple(args,"s",&configKey);
	if( pythonConfig ) {
		if( !configKey || strlen(configKey) == 0 ) {
			trace( "ERROR: no key specified in call to zlab.configGet()\n" );
		}
		else {
			char *configValue = pythonConfig->getS(configKey);
			if( !configValue ) {
				if( gDebug )
					trace( "Warning: No value set in config file for key '%s'\n", configKey );
			}
			else {
				strcpy( configValue, pythonConfig->getS(configKey) );
			}
		}
	}
	return Py_BuildValue("s", configValue);
}

PythonRegister register_configGet( INTERNAL_PYTHON_MODULE, "configGet", &ZLabModule_configGet);

// --- configPut -------------------
static PyObject *ZLabModule_configPut(PyObject *self, PyObject *args) {
	char *configKey = 0;
	char *configValue = 0;

	PyArg_ParseTuple(args,"ss",&configKey,&configValue);
	if( pythonConfig ) {
		if( configKey && configValue ) {
			pythonConfig->putS(configKey,configValue);
		}
		else {
			trace( "Warning: Illegal key and value in configPut()" );
		}
	}
	return Py_BuildValue("i", 1);
}

PythonRegister register_configPut( INTERNAL_PYTHON_MODULE, "configPut", &ZLabModule_configPut);

// --- zgetch -------------------
static int gLastKeystroke = 0;

void pythonSetKeystroke( int keystroke ) {
	gLastKeystroke = keystroke;
}

static PyObject *ZLabModule_getch(PyObject *self, PyObject *args) {
	while( gLastKeystroke == 0 ) {
		zTimeSleepMils( 20 );
	}
	
	int k = gLastKeystroke;
	gLastKeystroke = 0;

	return Py_BuildValue("c", k);
}

PythonRegister register_getch( INTERNAL_PYTHON_MODULE, "getch", &ZLabModule_getch);

static PyObject *ZLabModule_kbhit(PyObject *self, PyObject *args) {
	return Py_BuildValue("i", gLastKeystroke == 0 ? 0 : 1);
}

PythonRegister register_kbhit( INTERNAL_PYTHON_MODULE, "kbhit", &ZLabModule_kbhit);


// Running Python
//----------------------------------------------------------------
ZMSG_HANDLER( PythonRun ) {
	char *code = 0;
	ZUI *z = ZUI::zuiFindByName( "pythonTextEditor" );
	if( z ) {
		code = z->getS( "text", "" );
	}
	if( code ) {
		pythonInterpreter.run(code);
	}
}

ZMSG_HANDLER( PythonClear ) {
	ZUI *z = ZUI::zuiFindByName( "pythonTextEditor" );
	if( z ) {
		z->putS( "text", "" );
		zMsgQueue( "type=ZUITextEditor_LinesUpdate" );	// Required when you update the text manually
		zMsgQueue( "type=ZUIEnable toZUI=pythonTextEditor" );
	}
}

ZMSG_HANDLER( PythonHalt ) {
	pythonInterpreter.halt();
}


struct PythonConsole {
	ZTimer timer;
	unsigned int lastFileTime;
	static const float TIME_BETWEEN_FILE_CHECKS;
	ZFileSpec fileSpec;
	char defaultDir[256];

	PythonConsole() {
		lastFileTime = 0;
		defaultDir[0] = '\0';
	}

	void start(char *_defaultDir) {
		if( _defaultDir )
			strcpy(defaultDir,_defaultDir);

		if( pythonConfig ) {
			if( pythonConfig->getS("lastPythonLoaded") ) {
				zMsgQueue( "type=PythonLoad filespec='%s'", pythonConfig->getS("lastPythonLoaded") );
			}
		}
	}

	void loadFile( char *_fileSpec ) {
		fileSpec.set(_fileSpec);
		char *content = *fileSpec.get() ? fileRead( fileSpec.get() ) : 0;
		if( content ) {
			if( pythonConfig )
				pythonConfig->putS( "lastPythonLoaded", fileSpec.get() );

			zMsgQueue( "type=ZUISet key=text val='%s' toZUI=pythonFileName", fileSpec.get() );
			zMsgQueue( "type=ZUIDirty toZUI=pythonFileName" );

			lastFileTime = fileSpec.getTime();

			ZUI *z = ZUI::zuiFindByName( "pythonTextEditor" );
			if( z ) {
				// WARNING! Do not use ZUITextEditor_SetText here because the message passing buffer can only handle 2048 bytes.
				z->putS( "text", content );
				zMsgQueue( "type=ZUITextEditor_LinesUpdate" );	// Required when you update the text manually
				zMsgQueue( "type=ZUIDisable toZUI=pythonTextEditor" );
			}
		}
	}

	void pickFile() {
		zuiChooseFile( "Open File", defaultDir, true, "type=PythonLoad", 0, true );
	}


	void checkForUpdatedFile() {
		if( !timer.isRunning() || timer.isDone() ) {
			if( *fileSpec.get() && (lastFileTime == 0 || lastFileTime < fileSpec.getTime()) ) {
				loadFile(fileSpec.get());
				lastFileTime = fileSpec.getTime();
			}
			timer.start( TIME_BETWEEN_FILE_CHECKS );
		}
	}
};
const float PythonConsole::TIME_BETWEEN_FILE_CHECKS = 2.0f;

PythonConsole pythonConsole;

void pythonConsoleStart(ZHashTable *_config, char *defaultDirectory) {
	// WARNING! Very important to assign config before calling pythonConsole.start since it USES the config info.
	pythonConfig = _config;
	pythonConsole.start(defaultDirectory);
}

void pythonCheckForUpdatedFile() {
	pythonConsole.checkForUpdatedFile();
}

ZMSG_HANDLER( PythonLoad ) {
	char *filespec = zmsgS(filespec);
	pythonConsole.loadFile(filespec);
}

ZMSG_HANDLER( PythonOpen ) {
	pythonConsole.pickFile();
}
