#ifndef GUIFILEPICK_H
#define GUIFILEPICK_H

#include "guitextlist.h"

// GUIFilePick -- View and select a list of files
//==================================================================================

struct GUIFilePick : GUITextList {
	GUI_TEMPLATE_DEFINTION(GUIFilePick);

	void populate( char *path );

	GUIFilePick() : GUITextList() { }
	virtual void handleMsg( ZMsg *msg );
};

#endif

