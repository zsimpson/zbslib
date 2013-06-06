#ifndef GUIPLUGINVIEW_H
#define GUIPLUGINVIEW_H

#include "gui.h"

struct GUIPluginView : GUIPanel {
	GUI_TEMPLATE_DEFINTION( GUIPluginView );

	GUIPluginView() : GUIPanel() { }
	virtual void render();
	virtual void update();
	virtual void handleMsg( ZMsg *msg );
};

#endif

