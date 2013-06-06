#ifndef GUITWIDDLE_H
#define GUITWIDDLE_H

#include "gui.h"

struct GUIVarTwiddler : GUIObject {
	GUI_TEMPLATE_DEFINTION(GUIVarTwiddler);

	// SPECIAL ATTRIBUTES:
	//  ptr : a pointer to the value to be modified expressed as an integer
	//  ptrType : int | short | float | double
	//  value : a double value used if ptr and ptrType are not set
	//  lBound : lower bound [none]
	//  uBound : upper bound [none]
	//  twiddle : linear | exponential
	//  sendMsg: What to send when the variable changes value
	// SPECIAL MESSAGES:

	int mouseOver;
	int selected;
	float selectX, selectY;
	float mouseX, mouseY;
	double startVal;

	virtual void init();
	virtual void render();
	virtual void handleMsg( ZMsg *msg );
	virtual void queryOptSize( float &w, float &h );
};

struct GUIVarTextEditor : GUITextEditor {
	GUI_TEMPLATE_DEFINTION(GUIVarTextEditor);

	// SPECIAL ATTRIBUTES:
	//  ptr : a pointer to the value to be modified expressed as an integer
	//  ptrType : int | short | float | double
	//  value : a double value used if ptr and ptrType are not set
	//  lBound : lower bound [none]
	//  uBound : upper bound [none]
	//  sendMsg: What to send when the variable changes value
	// SPECIAL MESSAGES:

	double origVal;

	virtual void update();
	virtual void handleMsg( ZMsg *msg );
	virtual void queryOptSize( float &w, float &h );
};

#endif



