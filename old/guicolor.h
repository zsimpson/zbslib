// GUIColor
//==================================================================================

#include "gui.h"

struct GUIColor : GUIObject {
	GUI_TEMPLATE_DEFINTION(GUIColor);

	// SPECIAL ATTRIBUTES:
	// h : hue (0..1)
	// s : saturation (0..1)
	// v : value (0..1)
	// a : alpha (0..1)
	// sendMsg : what to send when changes.  h,s,v,r,g,b,a are all added on to the message
	// GUIColor_SetColor : send to it to set a color in either rgba or hsva

	int draggingV;
	int draggingA;

	void sendMsg();

	virtual void init();
	virtual void render();
	virtual void queryOptSize( float &w, float &h );
	virtual void handleMsg( ZMsg *msg );
};


