// @ZBS {
//		*COMPONENT_NAME zuibutton
//		*MODULE_OWNER_NAME zui
//		*MODULE_DEPENDS zuitexteditor.cpp
//		*GROUP modules/zui
//		*REQUIRED_FILES zuibutton.cpp
// }

// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#include "GL/gl.h"
// STDLIB includes:
#include "assert.h"
#include "string.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"
// MODULE includes:
#include "zui.h"
#include "zuitexteditor.h"
// ZBSLIB includes:
#include "zglfont.h"
#include "zmsg.h"
#include "zvars.h"

// ZUIButton
//--------------------------------------------------------------------------------------------------------------------------------

struct ZUIVarTextEditor : ZUITextEditor {
	ZUI_DEFINITION( ZUIVarTextEditor, ZUITextEditor );

	// SPECIAL ATTRIBUTES:
	//  ptr : a pointer to the value to be modified expressed as an integer
	//  ptrType : int | short | float | double
	//  value : a double value used if ptr and ptrType are not set
	//  lBound : lower bound [none]
	//  uBound : upper bound [none]
	//  sendMsg: What to send when the variable changes value
	// SPECIAL MESSAGES:

	double origVal;

	double getToDouble();
	void setFromDouble( double v );

	virtual void factoryInit();
	virtual void update();
	virtual void handleMsg( ZMsg *msg );
	virtual ZUI::ZUILayoutCell *layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly );
};

ZUI_IMPLEMENT( ZUIVarTextEditor, ZUITextEditor );

void ZUIVarTextEditor::factoryInit() {
	setUpdate( 1 );
}

double ZUIVarTextEditor::getToDouble() {
	void *ptr = (void *)getI("ptr");
	if( ptr ) {
		char *ptrType = getS( "ptrType" );
		if( !strcmp(ptrType,"int") ) {
			return (double)*(int *)ptr + 0.4; 
		}
		else if( !strcmp(ptrType,"short") ) {
			return (double)*(short *)ptr + 0.4;
		}
		else if( !strcmp(ptrType,"float") ) {
			return (double)*(float *)ptr;
		}
		else if( !strcmp(ptrType,"double") ) {
			return (double)*(double *)ptr;
		}
		return 0.0;
	}
	// Else
	return getD( "value" );
}

void ZUIVarTextEditor::setFromDouble( double v ) {
	if( has("lBound") ) {
		v = max( getD( "lBound" ), v );
	}
	if( has("uBound") ) {
		v = min( getD( "uBound" ), v );
	}

	void *ptr = (void *)getI("ptr");
	if( ptr ) {
		char *ptrType = getS( "ptrType" );
		if( !strcmp(ptrType,"int") ) {
			*(int *)ptr = (int)v;
		}
		else if( !strcmp(ptrType,"short") ) {
			*(short *)ptr = (short)v;
		}
		else if( !strcmp(ptrType,"float") ) {
			*(float *)ptr = (float)v;
		}
		else if( !strcmp(ptrType,"double") ) {
			*(double *)ptr = (double)v;
		}
	}
	else {
		putD( "value", v );
	}
}

void ZUIVarTextEditor::update() {
	if( !isEditting() ) {
		if( has("fmt") ) {
			double d = getToDouble();
			char buf[256];
			sprintf( buf, getS("fmt"), d );
			putS( "text", buf );
		}
	}
}

void ZUIVarTextEditor::handleMsg( ZMsg *msg ) {
	if( !strcmp(zmsgS(fromZUI),name) ) {
		double d = getToDouble();
		if( zmsgIs(type,TextEditor_Begin) ) {
			origVal = d;
		}
		else if( zmsgIs(type,TextEditor_Enter) ) {
			setFromDouble( atof(getS("text")) );
		}
		else if( zmsgIs(type,TextEditor_Escape) ) {
			setFromDouble( origVal );
		}
	}
	ZUITextEditor::handleMsg( msg );
}

ZUI::ZUILayoutCell *ZUIVarTextEditor::layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly ) {
	float d;
	printSize( has("sizeString")?getS("sizeString"):(char *)"0.0000", w, h, d );
	return 0;
}
