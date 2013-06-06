#ifndef ZUIVAR_H
#define ZUIVAR_H

#include "zui.h"

// GUIVar -- View and edit a variable
//==================================================================================

struct ZUIVar : ZUIPanel {
	ZUI_DEFINITION(ZUIVar,ZUIPanel);

	int getType();
	double getDouble();
	double setDouble( double d );
		// set, and return the value that was set, since this can get modified
		// due to range checks.

	char * getDoubleStr();

	ZUIVar() : ZUIPanel() { }
	virtual void dirty();
	virtual void factoryInit();
	virtual void render();
	virtual ZUILayoutCell *layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly=1 );
	virtual void handleMsg( ZMsg *msg );
};

#endif
