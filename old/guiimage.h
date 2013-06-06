#ifndef GUIIMAGE_H
#define GUIIMAGE_H

#include "gui.h"

// GUIImage
//==================================================================================

struct GUIImage : GUIObject {
	GUI_TEMPLATE_DEFINTION(GUIImage);

	int imageW, imageH;
	int texture;

	// SPECIAL ATTRIBUTES:
	//   color: color


	GUIImage() : GUIObject() { }
	virtual ~GUIImage();
	virtual void init();
	virtual void render();
	virtual void queryOptSize( float &w, float &h );
	virtual void handleMsg( TMsg *msg );
};

#endif

