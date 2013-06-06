#ifndef GUIBITMAPPAINTER_H
#define GUIBITMAPPAINTER_H

#include "gui.h"
//#include "bitmapdesc.h"

struct GUIBitmapPainter : public GUIPanel {
	GUI_TEMPLATE_DEFINTION(GUIBitmapPainter);

	// SPECIAL ATTRIBUTES:
	// paintMode = none | paint | erase
	// brushSize = pixelRadius
	// bitmapPtr = a pointer to the bitmap to paint into
	// bitmapOwner = 0 | 1: Does this gui own the bitmap memory
	// sendMsg = msg to send on change
	// sendMsgOnPaint = 0 | 1: should the sendMsg happen on every paint
	// paintColor = paint color
	// paintMask = bit mask when writing color
	//
	// SPEICAL MESSAGES:
	// GUIBitmapPainter_AllocBitmap
	// GUIBitmapPainter_Save
	// GUIBitmapPainter_Load
	// GUIBitmapPainter_Blur
	// GUIBitmapPainter_Clear


	//BitmapDesc bitmap;
	int texture;
	float mouseX, mouseY;
	int isEmpty;

	void freeBitmap();
	void sendMsg();
	void drawCircle( int x, int y, int radius, int colr, int mask );

	GUIBitmapPainter() : GUIPanel() { }
	virtual void init();
	virtual ~GUIBitmapPainter();
	virtual void render();
	virtual void handleMsg( ZMsg *msg );
};

#endif



