#ifndef GUITEXTLIST_H
#define GUITEXTLIST_H

#include "gui.h"
#include "ztl.h"

// GUITextList -- View and select a list of text elements
//==================================================================================

struct GUITextList : GUIPanel {
	GUI_TEMPLATE_DEFINTION(GUITextList);

	int mouseOver;
	int startScroll;
	int useSorted;
	int selectX, selectY;
	int selected;

	ZTLVec<char *> lines;
		// In the order they were added
	ZTLVec<char *> sorted;
		// In sorted order

	float getLineH();
		// Internal function for calculating the height of one line
	struct ZVarPtr *getSelectedVar( int selected );
		// Internal

	void clearView();
		// Frees all memory form the varsView array

	void addLine( char *name );

	void sortByName();
	void sortByOrder();

	GUITextList() : GUIPanel() { }
	virtual ~GUITextList();
	virtual void init();
	virtual void render();
	virtual void handleMsg( ZMsg *msg );
};

#endif

