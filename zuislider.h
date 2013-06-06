#ifndef ZUISLIDER_H
#define ZUISLIDER_H

#include "zui.h"

// GUIVarEdit -- View and edit any variable listed in the VARS
//==================================================================================

struct ZUISlider : ZUIPanel {
	ZUI_DEFINITION(ZUISlider,ZUIPanel);

	ZUISlider() : ZUIPanel() { }
	virtual ZUILayoutCell *layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly );
	virtual void factoryInit();
	virtual void render();
	virtual void handleMsg( ZMsg *msg );
};

#endif
