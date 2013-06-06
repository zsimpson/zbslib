#ifndef GLGUIVAREDIT_H
#define GLGUIVAREDIT_H

#include "gui.h"
#include "ztl.h"

// GUIVarEdit -- View and edit any variable listed in the VARS
//==================================================================================

struct GUIVarEdit : GUIPanel {
	GUI_TEMPLATE_DEFINTION(GUIVarEdit);

	int mouseOver;
	int selected;
	float selectX, selectY;
	float mouseX, mouseY;
	double startVal;
	int startScroll;
	int optMenuUp;
	int linMode;
	double linIncrement;
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

	GUIVarEdit() : GUIPanel() { }
	virtual ~GUIVarEdit();
	virtual void init();
	virtual void render();
	virtual void handleMsg( ZMsg *msg );
};

#endif

