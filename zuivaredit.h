#ifndef ZUIVAREDIT_H
#define ZUIVAREDIT_H

#include "zui.h"
#include "ztl.h"

// GUIVarEdit -- View and edit any variable listed in the VARS
//==================================================================================

struct ZUIVarEdit : ZUIPanel {
	ZUI_DEFINITION(ZUIVarEdit,ZUIPanel);

	int mouseOver;
	int selected;
	float selectX, selectY;
	float mouseX, mouseY;
	double startVal;
	int startScroll;
	int optMenuUp;
	int linMode;
	ZTLVec<char *> varsView;

	float getLineH();
		// Internal function for calculating the height of one line
	struct ZVarPtr *getSelectedVar( int selected );
		// Internal

	void clearView();
		// Frees all memory form the varsView array

	void addVar( char *name );
	void addVarsByRegExp( char *regexp );
		// Add vars to this view either directly or using
		// GUIVarEdit_Add name= regexp= 

	void sortViewByName();
	void sortViewByDeclOrder();
	void createOptionMenu( ZHashTable *args );

	ZUIVarEdit() : ZUIPanel() { }
	virtual void destruct();
	virtual void factoryInit();
	virtual void render();
	virtual void handleMsg( ZMsg *msg );
};

#endif
