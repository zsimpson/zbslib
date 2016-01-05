// @ZBS {
//		*COMPONENT_NAME zuivar
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zuivar.cpp zuitexteditor.cpp
// }

// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#ifdef __APPLE__
#include "OpenGL/gl.h"
#else
#include "GL/gl.h"
#endif
// STDLIB includes:
#include "assert.h"
#include "string.h"
#include "math.h"
#include "stdlib.h"
// MODULE includes:
#include "zuivar.h"
// ZBSLIB includes:
#include "zglfont.h"
#include "zvars.h"
#include "ztmpstr.h"

// STOPFLOW timing
#include "sfmod.h"

// ZUIVar
//==================================================================================

ZUI_IMPLEMENT(ZUIVar,ZUIPanel);

ZVAR( float, Zuiplay_testVar, 1 );

void ZUIVar::factoryInit() {
	ZUIPanel::factoryInit();

	#define defaultEpilson 1e-3
	putD( "epsilon", defaultEpilson );

	#define defaultPreciseTwiddleFactor 20.0
	putD( "preciseTwiddleFactor", defaultPreciseTwiddleFactor );

	ZUI *z = factory( 0, "ZUITextEditor" );
	z->attachTo( this );
	z->hidden = 1;
	container = 0;
}

enum { zvTypeUnknown=0, zvTypeInt, zvTypeFloat, zvTypeDouble, zvTypeString };

int ZUIVar::getType() {
	char *typeStr = getS( "type" );
	if( !typeStr || !*typeStr ) {
		ZVarPtr *zvar = zVarsLookup( getS("zvar") );
		if( zvar ) {
			switch( zvar->type ) {
				case zVarTypeINT: return zvTypeInt;
				case zVarTypeFLOAT: return zvTypeFloat;
				case zVarTypeDOUBLE: return zvTypeDouble;
				default:
					assert( 0 );
			}
		}
		else {
			assert( 0 );
		}
	}
	char *str = "";
	switch( typeStr[0] ) {
		case 's': return zvTypeString;
		case 'i': return zvTypeInt;
		case 'f': return zvTypeFloat;
		case 'd': return zvTypeDouble;
	}
	return zvTypeUnknown;
}

double ZUIVar::getDouble() {
	void *valPtr = 0;
	ZVarPtr *zvar = 0;
	double val = 0.0;
	valPtr = getp( "valPtr" );
	if( !valPtr ) {
		zvar = zVarsLookup( getS("zvar") );
		if( !zvar ) {
			val = getD( "val" );
		}
	}
	
	switch( getType() ) {
		case zvTypeString:
			return 0.0;

		case zvTypeInt:
			if( valPtr ) {
				return (double) *(int *)valPtr;
			}
			else if( zvar ) {
				assert( zvar->type == zVarTypeINT );
				return (double) *(int *)zvar->ptr;
			}
			return val;

		case zvTypeFloat:
			if( valPtr ) {
				return (float) *(float *)valPtr;
			}
			else if( zvar ) {
				assert( zvar->type == zVarTypeFLOAT );
				return (double) *(float *)zvar->ptr;
			}
			return val;

		case zvTypeDouble:
			if( valPtr ) {
				return (double) *(double *)valPtr;
			}
			else if( zvar ) {
				assert( zvar->type == zVarTypeDOUBLE );
				return *(double *)zvar->ptr;
			}
			return val;
	}
	return 0.0;
}

double ZUIVar::setDouble( double d ) {

	if( has( "rangeLow" ) ) {
		double rangeLow = getD( "rangeLow" );
		d = d < rangeLow ? rangeLow : d;
	}
	if( has( "rangeHigh" ) ) {
		double rangeHigh = getD( "rangeHigh" );
		d = d > rangeHigh ? rangeHigh : d;
	}

	void *valPtr = 0;
	ZVarPtr *zvar = 0;
	double val = 0.0;
	valPtr = getp( "valPtr" );
	if( !valPtr ) {
		zvar = zVarsLookup( getS("zvar") );
		if( !zvar ) {
			val = getD( "val" );
		}
	}
	
	switch( getType() ) {
		case zvTypeString:
			return d;

		case zvTypeInt:
			if( valPtr ) {
				*(int *)valPtr = (int)d;
			}
			else if( zvar ) {
				assert( zvar->type == zVarTypeINT );
				zvar->setFromDouble( (double)(int)d );
			}
			else {
				putI( "val", (int)d );
			}
			break;

		case zvTypeFloat:
			if( valPtr ) {
				*(float *)valPtr = (float)d;
			}
			else if( zvar ) {
				assert( zvar->type == zVarTypeFLOAT );
				zvar->setFromDouble( d );
			}
			else {
				putF( "val", (float)d );
			}
			break;

		case zvTypeDouble:
			if( valPtr ) {
				*(double *)valPtr = d;
			}
			else if( zvar ) {
				assert( zvar->type == zVarTypeDOUBLE );
				zvar->setFromDouble( d );
			}
			else {
				putD( "val", d );
			}
			break;
	}
	return d;
}

char * ZUIVar::getDoubleStr()
{
	// This factored out of ::render to facilitate setting an appropriate string in
	// the TextEditor when a user clicks to edit the value by hand. (tfb)
	static char buffer[128];	
		// returned char * points to this!

	// SETUP the value
	void *valPtr = 0;
	ZVarPtr *zvar = 0;
	double val = 0.0;
	valPtr = getp( "valPtr" );
	if( !valPtr ) {
		zvar = zVarsLookup( getS("zvar") );
		if( !zvar ) {
			val = getD( "val" );
		}
	}

	char *typeStr = getS( "type" );
	char *str = "";

	double storedValue = getDouble();
	double displayedValue = storedValue;

	// SUPPORT ranged-used of standard notation
	// @TODO: the %.4f below really should be something like %Xf where X==stdNotationPowLimit
	char *formatF_std = "%.4f", *formatF_e = "%3.2e", *formatD_std="%.4lf", *formatD_e="%3.2le";
	char *formatF=formatF_e,*formatD=formatD_e;
	int powLimit=getI( "stdNotationPowLimit", -1 );
	if( powLimit != -1 ) {
		double val = fabs( storedValue );
		if( val==0.0 || fabs( log10( val ) ) <= powLimit ) {
			formatF = formatF_std;
			formatD = formatD_std;
		}
	}
	// SUPPORT "formatShortest"
	if( getI( "formatShortest", 0 ) ) {
		formatF_std = "%g";
		formatF_e   = "%g";
		formatD_std = "%g";
		formatD_e   = "%g";
		formatF     = formatF_e;
		formatD		= formatD_e;
	}
	// SUPPORT "significantDigits"
	int sig;
	if( sig=getI( "significantDigits", 0 ) ) {
		ZTmpStr formatter( "%%%d.%de", sig, sig-1 );
		displayedValue = atof( ZTmpStr( formatter.s , storedValue ) );
	}

	// SUPPORT "formatString" which explicitly supplies the formatting string.
	// It is assumed that whomever set this understands the appropriate type etc. and did this correctly.
	char *formatI = "%d";     // can override the int formatting if you like
	char *formatString = getS( "formatString", NULL );
	if( formatString != NULL ) {
		// use the explicitly supplied format for all types
		formatI = formatString;
		formatF = formatString;
		formatD = formatString;
	}

	switch( getType() ) {
		case zvTypeString:
			if( valPtr ) {
				str = ZTmpStr( formatI, *(int *)valPtr );
			}
			else if( has("val") ) {
				str = getS("val");
			}
			break;

		case zvTypeInt:
			if( valPtr ) {
				str = ZTmpStr( formatI, *(int *)valPtr );
			}
			else if( zvar ) {
				assert( zvar->type == zVarTypeINT );
				str = ZTmpStr( formatI, *(int *)zvar->ptr );
			}
			else {
				str = ZTmpStr( formatI, (int)val );
			}
			break;

		case zvTypeFloat:
			/*
			if( valPtr ) {
				str = ZTmpStr( formatF, *(float *)valPtr );
			}
			else if( zvar ) {
				assert( zvar->type == zVarTypeFLOAT );
				str = ZTmpStr( formatF, *(float *)zvar->ptr );
			}
			else {
				str = ZTmpStr( formatD, val );
			}
			*/
			str = ZTmpStr( formatF, (float)displayedValue );
			break;

		case zvTypeDouble:
			/*
			if( valPtr ) {
				str = ZTmpStr( formatD, *(double *)valPtr );
			}
			else if( zvar ) {
				assert( zvar->type == zVarTypeDOUBLE );
				str = ZTmpStr( formatD, *(double *)zvar->ptr );
			}
			else {
				str = ZTmpStr( formatD, val );
			}
			*/
			str = ZTmpStr( formatD, displayedValue );
			break;
	}
	strcpy( buffer, str );
	return buffer;
}

void ZUIVar::render() {
	ZUIPanel::render();

	setupColor( (char*)(getI( "disabled" ) ? "varTextColorDisabled" : "varTextColor") );
	if( getI( "inEditMode" ) || !headChild->getI( "hidden" ) ) {
		// the two conditions above are probably supposed to be equivalent, but they
		// appear not to be.  headChild is the textEditor.  If it is visible, we should
		// not render our own text, lest it show thru an invisible textEditor panel.
		return;
	}

//	float arr = 3.f;
//	float hei = h - 1.f;
//	glBegin( GL_LINES );
//		glVertex2f( arr, 3.f );
//		glVertex2f( arr, hei );
//
//		glVertex2f( arr, 3.f );
//		glVertex2f( 0.f, 3.f+arr );
//
//		glVertex2f( arr, 3.f );
//		glVertex2f( arr+arr, 3.f+arr );
//
//		glVertex2f( arr, hei );
//		glVertex2f( 0.f, hei-arr );
//
//		glVertex2f( arr, hei );
//		glVertex2f( arr+arr, hei-arr );
//
//	glEnd();
//	float offset = 10.f;
	float offset = 0.f;

//	scaleAlphaMouseOver();

	char *str = getDoubleStr();
	void *fontPtr = zglFontGet( getS("font") );
	char *prefix = getS( "prefix", "" );
	float _x=0.f, _y=0.f;
	ZTmpStr text("%s%s",prefix,str);
	if( getI( "formatCenter", 0 ) ) {
		float _w,_h,_d;
		zglFontGetDim( text, _w, _h, _d, 0, fontPtr, -1 );
		_x = ( w - _w ) / 2.f;
		_y = ( h - _h ) / 2.f;
	}
	zglFontPrint( text, _x+offset, _y, 0, 1, fontPtr );

	if( getI( "dragging" ) && !getI( "disableDrag" ) ) {
		float startX = getF( "startX" );
		float startY = getF( "startY" );
		float mouseX = getF( "mouseX" );
		float mouseY = getF( "mouseY" );

#ifdef SFFAST
		// mmk - This does not reset gl states completely yet without this push, which is unfortunately very expensive.  Problem occurs only with dirty rect on.
		//glPushAttrib( GL_ALL_ATTRIB_BITS );
		glPushAttrib( GL_SCISSOR_BIT );  // SSFAST_TFBMOD: only this seems necessary
#endif

		glDisable( GL_SCISSOR_TEST );

		glDisable( GL_BLEND );
		glDisable( GL_DEPTH_TEST );
		glLineWidth( 2.f );
		glBegin( GL_LINES );
			glColor3ub( 0, 0, 0 );
			for( int i=0; i<2; i++ ) {
				// DRAW arrow shaft
				glVertex2f( startX, startY );
				glVertex2f( startX, mouseY );

				// DRAW arrow head
				float sign = mouseY > startY ? -1.f : 1.f;
				glVertex2f( startX, mouseY );
				glVertex2f( startX-5, mouseY + sign*5 );
				glVertex2f( startX, mouseY );
				glVertex2f( startX+5, mouseY + sign*5 );
				setupColor( "varArrowColor" );
				startX -= 1.f;
				startY += 1.f;
				mouseY += 1.f;
			}
		glEnd();
		
#ifdef SFFAST
		// mmk - This does not reset gl states completely yet without this push, which is unfortunately very expensive. Problem occurs only with dirty rect on.
		glPopAttrib ();
#endif
	}
}

ZUI::ZUILayoutCell *ZUIVar::layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &_w, float &_h, int sizeOnly ) {
	float _d;
	zglFontGetDim( getDoubleStr(), _w, _h, _d, getS("font") );
//	_w += 10;
	return 0;
}


void ZUIVar::dirty() {
	ZUI::dirty();

	float wx, wy;
	getWindowXY( wx, wy );

	float startX = getF( "startX" ) + wx;
	float startY = getF( "startY" ) + wy;
	float mouseX = getF( "mouseX" ) + wx;
	float mouseY = getF( "mouseY" ) + wy;

	if( mouseY > startY ) {
		dirtyRectAdd( (int)(startX-8.f), (int)startY-2, 16, (int)(mouseY-startY)+8 );
	}
	else {
		dirtyRectAdd( (int)(startX-8.f), (int)mouseY-2, 16, (int)(startY-mouseY)+10 );
	}
}

void ZUIVar::handleMsg( ZMsg *msg ) {
	if( zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L) ) {
		if( !getI( "disabled" )  ) {
			if( getType() != zvTypeString ) {
				dirty();
				putD( "startVal", getDouble() );
				putF( "startX", zmsgF(localX) );
				putF( "startY", zmsgF(localY) );
				putF( "mouseX", zmsgF(localX) );
				putF( "mouseY", zmsgF(localY) );
				requestExclusiveMouse( 1, 1 );
				putI( "dragging", 1 );
				putI( "wasDragged", 0 );
				focus( 1 );
					// need focus to receive ESCAPE message before app does
			}
		}
		zMsgUsed();
	}
	else if( zmsgIs(type,ZUIMouseReleaseDrag) || zmsgIs(type,ZUITakeFocus) ) {
		dirty();
		if( ! getI( "wasDragged" ) ) {
			putD( "startVal", getDouble() );
				// this is only needed if the message is ZUITakeFocus, since in this case
				// we did not get a ZUIMouseClickOn so startVal has not been set, and hitting ESC
				// would set the value to 0.
			putI( "inEditMode", 1 );
			assert( headChild );
			headChild->layoutIgnore = 1;
			headChild->x = 0.f;
			headChild->y = -3.f;
			headChild->goalX = 0.f;
			headChild->goalY = -3.f;
			headChild->w = w;
			headChild->h = h+6.f;
				// We had to rtaise this to 6 from 4 to avoid a probable rouding bug int the
				// zuitexteditor that caused it to woggle up and down as you typed
				// This is pretty hacky as it probably won't work on different fonts.

		
			char *styleName = getS( "style", 0 );
			if( styleName ) {
				headChild->putS( "style", getS( "style", "" ) );
				ZMsg styleMsg;
				styleMsg.putEncoded( ZTmpStr( "type=ZUIStyleChanged which=%s toZUI=%s", styleName, headChild->name ) );
				headChild->ZUI::handleMsg( &styleMsg );
					// so that following calls setting colors etc will reflect style settings
			}
			headChild->putS( "panelColor", getS( "varPanelColorEditMode" ) );
			headChild->putS( "panelFrame", getS( "varPanelFrameEditMode" ) );
			headChild->putS( "textEditTextColor", getS( "varTextColorEditMode" ) );
			headChild->putS( "textEditMarkColor", getS( "varTextEditMarkColor", "0xFF0000FF") );
	
			headChild->hidden = 0;
			headChild->dirty();
			headChild->putS( "sendMsg", ZTmpStr("type=ZUIVar_EditCallback toZUI=%s", name) );
			headChild->putS( "sendMsgOnCancel", ZTmpStr("type=ZUIVar_EditCancelCallback toZUI=%s", name) );
			headChild->putS( "text", getDoubleStr() );
			headChild->putI( "acceptEditOnClickOutside", 1 );
				// loss of focus will cause this TextEditor to act as if Enter was pressed.
			zMsgQueue( "type=ZUITakeFocus toZUI=%s", headChild->name );

		}
		else {
			double oldVal = getD( "startVal" );
			double newVal = getDouble();
			putD( "startVal", newVal );
			if( has( "sendMsgOnEditComplete" ) ) {
				zMsgQueue( "%s key=%s val=%e oldval=%e twiddleComplete=1", getS("sendMsgOnEditComplete"), name, newVal, oldVal );
			}
		}
		putI( "dragging", 0 );
		requestExclusiveMouse( 1, 0 );
		focus( 0 );
		zMsgUsed();
	}
	else if( zmsgIs(type,ZUIExclusiveMouseDrag) ) {
		if( getI( "disableDrag") ) {
			return;
				// I want all the various bounding logic etc, but I want to force the user
				// to make individual edits to the value.
		}
		dirty();
		double startVal = getD( "startVal" );
		double epsilon = getD( "epsilon" );
		float localY = zmsgF( localY );
		float startY = getF( "startY" );

		if( startVal == 0.0 ) {
			startVal = localY < startY ? -epsilon : epsilon;
		}

		double newVal;
		if( getType() == zvTypeInt && (getD( "rangeLow" ) * getD( "rangeHigh" ) < 0)  ) {
			newVal = startVal + ( localY - startY ) / 10.0;
				// special case to handle bounds spanning + & - ints
		}
		else {
			double divisor = 50.0;
			if( zmsgI( alt ) ) {
#ifdef KIN_DEV
				// remove this after Ken plays with values & decides on a nice default for KinExp
				extern double Kin_preciseTwiddleFactor;
				divisor *= Kin_preciseTwiddleFactor;
#else
				divisor *= getD( "preciseTwiddleFactor", 1.0 );
#endif
			}
			newVal = startVal * exp( (localY - startY)  / divisor );
		}

		// REDUCE precision to significantDigits for storage.
		// @TODO: come up with a more consistent design for "significatDigits".
		// It is used in getDoubleStr() also to format.
		int sig;
		if( sig=getI( "significantDigits", 0 ) ) {
			ZTmpStr formatter( "%%%d.%de", sig, sig-1 );
			newVal = atof( ZTmpStr( formatter.s , newVal ) );
				// limit significant digits 
		}
		double oldVal = getDouble();
		newVal = setDouble( newVal );
		putI( "wasDragged", 1 );
		putF( "mouseX", zmsgF(localX) );
		putF( "mouseY", zmsgF(localY) );
		if( has( "sendMsgOnVarChanging" ) ) {
			zMsgQueue( "%s key=%s oldval=%e val=%e", getS("sendMsgOnVarChanging"), name, oldVal, getDouble() );
		}
		zMsgUsed();
		dirty();
	}
	else if( zmsgIs(type,ZUIVar_EditCallback) ) {
		dirty();
		double newVal = atof( zmsgS(text) ); 
		double oldVal = getDouble();
		// mkness - now always sending message when value changed. This used to be skipped if they were the same.
		newVal = setDouble( newVal );
		if( has( "sendMsgOnEditComplete" ) ) {
			int changed = fabs( newVal - oldVal ) < 1e-10  ? 0 : 1 ;
			zMsgQueue( "%s changed=%d key=%s val=%e oldval=%e", getS("sendMsgOnEditComplete"), changed, name, newVal, oldVal );
		}
		putI( "inEditMode", 0 );
		headChild->hidden = 1;
		headChild->dirty();
		zMsgUsed();
	}
	else if( zmsgIs(type,ZUIVar_EditCancelCallback) ) {
		dirty();
		headChild->hidden = 1;
		headChild->dirty();
		putI( "inEditMode", 0 );
		setDouble( getD( "startVal" ) );
		zMsgUsed();
	}
	else if( zmsgIs(type,Key) && zmsgIs(which,escape) ) {
		dirty();
		// CANCEL any editing & restore startVal
		if( getI( "dragging" ) ) {
			putI( "dragging", 0 );
			requestExclusiveMouse( 1, 0 );
			double oldVal = getDouble();
			setDouble( getD( "startVal" ) );
			focus( 0 );
			// mkness - do NOT send message on escape. This used to send one.
			// tfb - not so fast, the app needs to know about this, because the value is in fact changing! reinstating...
			// mkness - tweaked this to not send this message if the ZUI had a 'noMsgOnEscape' entry.
			// This makes it less tedious to use the way I want. If you don't like my convention, just do not define any 'noMsgOnEscape'.
			if( has( "sendMsgOnEditComplete" ) && !has( "noMsgOnEscape" ) )
				zMsgQueue( "%s changed=0 key=%s val=%e oldval=%e fromEscape=1", getS("sendMsgOnEditComplete"), name, getDouble(), oldVal );
					// even though a 'cancel', need to inform any client of change of value
			zMsgUsed();
		}
	}

	if( !zMsgIsUsed() ) {
		ZUIPanel::handleMsg( msg );
			// new may4, 2009: don't forward to ZUIPanel if used!
	}
}
