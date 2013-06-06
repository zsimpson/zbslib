// @ZBS {
//		*COMPONENT_NAME guitwiddle
//		*MODULE_OWNER_NAME glgui
//		*GROUP modules/gui
//		*REQUIRED_FILES guitwiddle.h
// }
// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#include "GL/gl.h"
// STDLIB includes:
#include "string.h"
#include "math.h"
#include "stdlib.h"
// MODULE includes:
#include "guitwiddle.h"
// ZBSLIB includes:
#include "zvars.h"
#include "ztmpstr.h"
#include "zmathtools.h"

double GUIVarTwiddler_getToDouble( GUIObject *gui ) {
	void *ptr = (void *)gui->getAttrI("ptr");
	if( ptr ) {
		char *ptrType = gui->getAttrS( "ptrType" );
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
	return gui->getAttrD( "value" );
}

void GUIVarTwiddler_setFromDouble( GUIObject *gui, double v ) {
	if( gui->hasAttr("lBound") ) {
		v = max( gui->getAttrD( "lBound" ), v );
	}
	if( gui->hasAttr("uBound") ) {
		v = min( gui->getAttrD( "uBound" ), v );
	}

	void *ptr = (void *)gui->getAttrI("ptr");
	if( ptr ) {
		char *ptrType = gui->getAttrS( "ptrType" );
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
		gui->setAttrD( "value", v );
	}
}


GUI_TEMPLATE_IMPLEMENT(GUIVarTwiddler);

void GUIVarTwiddler::init() {
	GUIObject::init();
	selected = 0;
	mouseOver = -1;
	setAttrS( "mouseRequest", "over" );
	setAttrS( "noScissor", "1" );
}

void GUIVarTwiddler::render() {
	glColor3ub( 255, 0, 0 );

	float cenX = myW / 2.f;
	float cenY = myH / 2.f;
	float rad = cenX;

	if( selected ) {
		glBegin( GL_TRIANGLE_FAN );
		glVertex2f( cenX, cenY );
	}
	else {
		glBegin( GL_POINTS );
	}

	float t=0.f, td = PI2F/16.f;
	for( int i=0; i<=16; i++, t+=td ) {
		glVertex2f( cenX + rad*(float)cos(t), cenY + rad*(float)sin(t) );
	}

	glEnd();

	if( selected ) {
		glBegin( GL_LINES );
			glVertex2f( selectX, selectY );
			glVertex2f( selectX, mouseY );

			float sign = mouseY > selectY ? -1.f : 1.f;
			glVertex2f( selectX, mouseY );
			glVertex2f( selectX-5, mouseY + sign*5 );

			glVertex2f( selectX, mouseY );
			glVertex2f( selectX+5, mouseY + sign*5 );
		glEnd();
	}
}

void GUIVarTwiddler::handleMsg( ZMsg *msg ) {
	float x = zmsgF(localX);
	float y = zmsgF(localY);

	if( zmsgIs(type,MouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L) ) {
		setAttrI( "cursor", -1 );
		selected = 1;
		selectX = x;
		selectY = y;
		mouseX = x;
		mouseY = y;
		startVal = GUIVarTwiddler_getToDouble( this );
		if( startVal == 0.0 ) startVal = 1e-1;
		requestExclusiveMouse( 1, 1 );
		zMsgUsed();
		return;
	}
	else if( zmsgIs(type,MouseReleaseDrag) ) {
		selected = 0;
		requestExclusiveMouse( 1, 0 );
		zMsgUsed();
		return;
	}
	else if( zmsgIs(type,MouseDrag) ) {
		mouseX = x;
		mouseY = y;
		if( isAttrS("twiddle","linear") ) {
			double delta = (mouseY-selectY) / 5.0;
			GUIVarTwiddler_setFromDouble( this, startVal + delta );
		}
		else {
			double scale = exp( (mouseY-selectY) / 50.0 );
			double val = startVal * scale;
			if( val == 0.0 ) val = 1e-1;
			GUIVarTwiddler_setFromDouble( this, val );
		}
	}
	GUIObject::handleMsg( msg );
}

void GUIVarTwiddler::queryOptSize( float &w, float &h ) {
	w = 10;
	h = 10;
}

// GUIVarTextEditor
//=============================================================================

GUI_TEMPLATE_IMPLEMENT( GUIVarTextEditor );

void GUIVarTextEditor::update() {
	if( !isEditting() ) {
		if( hasAttr("fmt") ) {
			double d = GUIVarTwiddler_getToDouble( this );
			setAttrS( "text", ZTmpStr(getAttrS("fmt"),d) );
		}
	}
}

void GUIVarTextEditor::handleMsg( ZMsg *msg ) {
	if( !strcmp(zmsgS(fromGUI),getAttrS("name")) ) {
		double d = GUIVarTwiddler_getToDouble( this );
		if( zmsgIs(type,TextEditor_Begin) ) {
			origVal = d;
		}
		else if( zmsgIs(type,TextEditor_Enter) ) {
			GUIVarTwiddler_setFromDouble( this, atof(getAttrS("text")) );
		}
		else if( zmsgIs(type,TextEditor_Escape) ) {
			GUIVarTwiddler_setFromDouble( this, origVal );
		}
	}
	GUITextEditor::handleMsg( msg );
}

void GUIVarTextEditor::queryOptSize( float &w, float &h ) {
	float d;
	printSize( hasAttr("sizeString")?getAttrS("sizeString"):(char *)"0.0000", w, h, d );
}
