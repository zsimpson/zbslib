#ifndef GUIATTREDIT_H
#define GUIATTREDIT_H

#include "gui.h"
#include "ztl.h"

// GUIAttrEdit -- View and edit any variable listed in a hash table
//==================================================================================

struct GUIAttrEdit : GUIPanel {
	GUI_TEMPLATE_DEFINTION(GUIAttrEdit);

	int mouseOver;
	int selected;
	float selectX, selectY;
	float mouseX, mouseY;
	double startVal;
	int startScroll;
	int optMenuUp;
	int linMode;
	double linIncrement;

	ZTLVec<char *> view;
		// The sorted list of the variables in the hash that we are viewing

	// The hashPtr is stored as an int pointer in the attr table

	float getLineH();
		// Internal function for calculating the height of one line
//	struct ZVarPtr *getSelectedVar( int selected );
		// Internal

	void clearView();
		// Frees all of the view array

	void addVar( char *name );
	void addAllVars();
	void addVarsByRegExp( char *regexp );
		// Add vars to this view either directly or using
		// GUIVarEdit_Add name= regexp= 

	void sortViewByName();
//	void sortViewByDeclOrder();
	void createOptionMenu( ZHashTable *args );

	GUIAttrEdit() : GUIPanel() { }
	virtual ~GUIAttrEdit();
	virtual void init();
	virtual void render();
	virtual void handleMsg( ZMsg *msg );
};

#endif

