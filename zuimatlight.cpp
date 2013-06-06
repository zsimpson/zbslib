// @ZBS {
//		*COMPONENT_NAME zuibutton
//		*MODULE_OWNER_NAME zui
//      *MODULE_DEPENDS zuicolor.cpp zuiradiobutton.cpp zuivartexteditor.cpp zuivartwiddler.cpp zui3ddirectionarrow.cpp
//		*GROUP modules/zui
//		*REQUIRED_FILES zuimatlight.cpp zuimatlight.h
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
// ZBSLIB includes:
#include "zglfont.h"
#include "zmsg.h"
#include "zvars.h"
#include "zglmatlight.h"
#include "zregexp.h"
#include "zvec.h"
#include "zuimenu.h"

// Light Editor UI Callback
//-----------------------------------------------------------------------------------------------------------------------------------------

static int __cdecl stringCompare(const void *a, const void *b) {
	return stricmp( (char*)a, (char*)b );
}

char lightSelected[256];

ZMSG_HANDLER( Light_Save ) {
	zglLightSaveAll( "lights.txt" );
}

ZMSG_HANDLER( Light_ActiveChange ) {
	ZGLLight *l = zglLightFetch( lightSelected );
	if( l ) {
		l->active = zmsgI( active );
	}
}

ZMSG_HANDLER( Light_AmbientChange ) {
	ZGLLight *l = zglLightFetch( lightSelected );
	if( l ) {
		l->ambient[0] = zmsgF(r);
		l->ambient[1] = zmsgF(g);
		l->ambient[2] = zmsgF(b);
	}
}

ZMSG_HANDLER( Light_DiffuseChange ) {
	ZGLLight *l = zglLightFetch( lightSelected );
	if( l ) {
		l->diffuse[0] = zmsgF(r);
		l->diffuse[1] = zmsgF(g);
		l->diffuse[2] = zmsgF(b);
	}
}

ZMSG_HANDLER( Light_SpecularChange ) {
	ZGLLight *l = zglLightFetch( lightSelected );
	if( l ) {
		l->specular[0] = zmsgF(r);
		l->specular[1] = zmsgF(g);
		l->specular[2] = zmsgF(b);
	}
}

ZMSG_HANDLER( Light_Select ) {
	zMsgQueue( "type=ZUISet toZUI=lightEditor_selectedButton key=text val='%s'", zmsgS(name) );

	strcpy( lightSelected, zmsgS(name) );
	ZGLLight *l = zglLightFetch( lightSelected );
	if( l ) {
		// SETUP UI:

		// ...active
		zMsgQueue( "type=ZUISet toZUI=lightEditor_active key=selected val=%d", l->active );

		zMsgQueue( "type=ZUIColor_SetColor toZUI=lightEditor_ambient r=%f g=%f b=%f", l->ambient[0], l->ambient[1], l->ambient[2] );
		zMsgQueue( "type=ZUIColor_SetColor toZUI=lightEditor_diffuse r=%f g=%f b=%f", l->diffuse[0], l->diffuse[1], l->diffuse[2] );
		zMsgQueue( "type=ZUIColor_SetColor toZUI=lightEditor_specular r=%f g=%f b=%f", l->specular[0], l->specular[1], l->specular[2] );

		// ...type
		char *type = l->isPositional() ? "lightEditor_posToggle" : (l->isDirectional() ? "lightEditor_dirToggle" : "lightEditor_spotToggle");
		zMsgQueue( "type=ZUISet toZUI=%s key=selected val=1", type );

		// ...position
		zMsgQueue( "type=ZUISet toZUI=lightEditor_posXEditor key=ptr val=0x%x", (int)&l->pos[0] );
		zMsgQueue( "type=ZUISet toZUI=lightEditor_posXTwiddler key=ptr val=0x%x", (int)&l->pos[0] );
		zMsgQueue( "type=ZUISet toZUI=lightEditor_posYEditor key=ptr val=0x%x", (int)&l->pos[1] );
		zMsgQueue( "type=ZUISet toZUI=lightEditor_posYTwiddler key=ptr val=0x%x", (int)&l->pos[1] );
		zMsgQueue( "type=ZUISet toZUI=lightEditor_posZEditor key=ptr val=0x%x", (int)&l->pos[2] );
		zMsgQueue( "type=ZUISet toZUI=lightEditor_posZTwiddler key=ptr val=0x%x", (int)&l->pos[2] );

		zMsgQueue( "type=ZUISet toZUI=lightEditor_constantAttenEditor key=ptr val=0x%x", (int)&l->constantAttenuation );
		zMsgQueue( "type=ZUISet toZUI=lightEditor_constantAttenTwiddler key=ptr val=0x%x", (int)&l->constantAttenuation );
		zMsgQueue( "type=ZUISet toZUI=lightEditor_linearAttenEditor key=ptr val=0x%x", (int)&l->linearAttenuation );
		zMsgQueue( "type=ZUISet toZUI=lightEditor_linearAttenTwiddler key=ptr val=0x%x", (int)&l->linearAttenuation );
		zMsgQueue( "type=ZUISet toZUI=lightEditor_quadraticAttenEditor key=ptr val=0x%x", (int)&l->quadraticAttenuation );
		zMsgQueue( "type=ZUISet toZUI=lightEditor_quadraticAttenTwiddler key=ptr val=0x%x", (int)&l->quadraticAttenuation );

		// ...spot
		zMsgQueue( "type=ZUISet toZUI=lightEditor_spotExponentEditor key=ptr val=0x%x", (int)&l->spotExponent );
		zMsgQueue( "type=ZUISet toZUI=lightEditor_spotExponentTwiddler key=ptr val=0x%x", (int)&l->spotExponent );
		zMsgQueue( "type=ZUISet toZUI=lightEditor_spotCutoffEditor key=ptr val=0x%x", (int)&l->spotCutoff );
		zMsgQueue( "type=ZUISet toZUI=lightEditor_spotCutoffTwiddler key=ptr val=0x%x", (int)&l->spotCutoff );

		// ...dir
		zMsgQueue( "type=SetDir toZUI=lightEditor_dirArrow x=%f y=%f z=%f", l->dir[0], l->dir[1], l->dir[2] );
	}
}

ZMSG_HANDLER( Light_CreateSelectMenu ) {
	// @TODO: Consolidate these menu calls, make it more convenient to make menus
	char sortedLightList[256][256];
	int sortedLightListCount = 0;
	int i;

	ZUI *selectLightMenu = ZUI::factory( "selectLightMenu", "ZUIMenu" );
	selectLightMenu->attachTo( ZUI::zuiFindByName("root") );

	ZRegExp *filterRegExp = 0;
	if( zmsgHas(filter) ) {
		filterRegExp = new ZRegExp( zmsgS(filter) );
	}

	for( i=0; i<zglLightHash.size(); i++ ) {
		char *k = zglLightHash.getKey( i );
		if( k ) {
			if( filterRegExp && filterRegExp->test(k) ) {
				strcpy( sortedLightList[sortedLightListCount++], k );
			}
		}
	}
	qsort( sortedLightList, sortedLightListCount, 256, stringCompare );

	for( i=0; i<sortedLightListCount; i++ ) {
		zMsgQueue( "type=Menu_AddItem label='%s' sendMsg='type=Light_Select name=%s' toZUI=selectLightMenu", sortedLightList[i], sortedLightList[i] );
	}
//	zMsgQueue( "type=ZUILayout toZUI=selectLightMenu" );
//	selectLightMenu->putI( "nolayout", 1 );

	// @TODO: I've got to fix this layout mess for menus
	// Right now I've just got to get the lights working and don't have time!
	selectLightMenu->putI( "layoutManual", 1 );
	selectLightMenu->putI( "layoutManual_x", 100 );
	selectLightMenu->putI( "layoutManual_y", 100 );
	selectLightMenu->putI( "layoutManual_w", 100 );
	selectLightMenu->putI( "layoutManual_h", 100 );

//	selectLightMenu->resize( 300, 300 );
	selectLightMenu->layout( 300, 300 );
//	selectLightMenu->move( 100, 100 );
//	zMsgQueue( "type=ZUIMoveNear who='%s' where=B toZUI=selectLightMenu", zmsgS(fromZUI) );
//	zMsgQueue( "type=ZUISetModal toZUI=selectLightMenu val=1" );

	if( filterRegExp ) {
		delete filterRegExp;
	}
}

ZMSG_HANDLER( Light_TypeChange ) {
	ZGLLight *l = zglLightFetch( lightSelected );
	if( l ) {
		if( zmsgIs(which,directional) ) {
			l->makeDirectional();
		}
		else if( zmsgIs(which,positional) ) {
			l->makePositional();
		}
		else if( zmsgIs(which,spot) ) {
			l->makeSpot();
		}
	}
}

ZMSG_HANDLER( Light_DirChange ) {
	ZGLLight *l = zglLightFetch( lightSelected );
	if( l ) {
		FMat4 *matPtr = (FMat4 *)zmsgI( matPtr );
		if( l->isPositional() ) {
			FVec3 dir = matPtr->mul( FVec3::XAxisMinus );
			l->pos[0] = dir.x;
			l->pos[1] = dir.y;
			l->pos[2] = dir.z;
		}
		else if( l->isDirectional() ) {
			FVec3 dir = matPtr->mul( FVec3::XAxisMinus );
			l->dir[0] = dir.x;
			l->dir[1] = dir.y;
			l->dir[2] = dir.z;
		}
		else if( l->isSpot() ) {
			FVec3 dir = matPtr->mul( FVec3::XAxis );
			l->spotDir[0] = dir.x;
			l->spotDir[1] = dir.y;
			l->spotDir[2] = dir.z;
		}
	}
}

ZMSG_HANDLER( Light_Exclusive ) {
}

// Material Editor UI Callback
//-----------------------------------------------------------------------------------------------------------------------------------------

char materSelected[256];

ZMSG_HANDLER( Mater_Save ) {
	zglMaterialSaveAll( "materials.txt" );
}

ZMSG_HANDLER( Mater_CreateSelectMenu ) {
	// @TODO: Consolidate these menu calls, make it more convenient to make menus
	char sortedList[256][256];
	int sortedListCount = 0;
	int i;

	ZUI *selectMaterMenu = ZUI::factory( "selectMaterMenu", "ZUIMenu" );
	selectMaterMenu->attachTo( ZUI::zuiFindByName("root") );

	ZRegExp *filterRegExp = 0;
	if( zmsgHas(filter) ) {
		filterRegExp = new ZRegExp( zmsgS(filter) );
	}

	for( i=0; i<zglMaterialHash.size(); i++ ) {
		char *k = zglMaterialHash.getKey( i );
		if( k ) {
			if( filterRegExp && filterRegExp->test(k) ) {
				strcpy( sortedList[sortedListCount++], k );
			}
		}
	}
	qsort( sortedList, sortedListCount, 256, stringCompare );

	for( i=0; i<sortedListCount; i++ ) {
		ZUIMenuItem *mi = new ZUIMenuItem();
		zMsgQueue( "type=Menu_AddItem label='%s' sendMsg='type=Mater_Select name=%s' toZUI=selectMaterMenu", sortedList[i], sortedList[i] );
	}
	zMsgQueue( "type=ZUILayout toZUI=selectMaterMenu" );
	zMsgQueue( "type=ZUIMoveNear who=%s where=B toZUI=selectMaterMenu", zmsgS(fromZUI) );
	zMsgQueue( "type=ZUISetModal toZUI=selectMaterMenu val=1" );

	if( filterRegExp ) {
		delete filterRegExp;
	}
}

ZMSG_HANDLER( Mater_Select ) {
	zMsgQueue( "type=ZUISet toZUI=materEditor_selectedButton key=text val='%s'", zmsgS(name) );

	strcpy( materSelected, zmsgS(name) );
	ZGLMaterial *l = zglMaterialFetch( materSelected );
	if( l ) {
		// SETUP UI:

		// ...active
		zMsgQueue( "type=ZUISet toZUI=materEditor_active key=selected val=%d", l->active );

		zMsgQueue( "type=ZUIColor_SetColor toZUI=materEditor_ambient r=%f g=%f b=%f", l->ambient[0], l->ambient[1], l->ambient[2] );
		zMsgQueue( "type=ZUIColor_SetColor toZUI=materEditor_diffuse r=%f g=%f b=%f", l->diffuse[0], l->diffuse[1], l->diffuse[2] );
		zMsgQueue( "type=ZUIColor_SetColor toZUI=materEditor_specular r=%f g=%f b=%f", l->specular[0], l->specular[1], l->specular[2] );
		zMsgQueue( "type=ZUIColor_SetColor toZUI=materEditor_emission r=%f g=%f b=%f", l->emission[0], l->emission[1], l->emission[2] );
		zMsgQueue( "type=ZUISet toZUI=materEditor_shininessEditor key=ptr val=0x%x", (int)&l->shininess );
		zMsgQueue( "type=ZUISet toZUI=materEditor_shininessTwiddler key=ptr val=0x%x", (int)&l->shininess );
	}
}

ZMSG_HANDLER( Mater_ActiveChange ) {
	ZGLMaterial *l = zglMaterialFetch( materSelected );
	if( l ) {
		l->active = zmsgI( active );
	}
}

ZMSG_HANDLER( Mater_AmbientChange ) {
	ZGLMaterial *l = zglMaterialFetch( materSelected );
	if( l ) {
		l->ambient[0] = zmsgF(r);
		l->ambient[1] = zmsgF(g);
		l->ambient[2] = zmsgF(b);
	}
}

ZMSG_HANDLER( Mater_DiffuseChange ) {
	ZGLMaterial *l = zglMaterialFetch( materSelected );
	if( l ) {
		l->diffuse[0] = zmsgF(r);
		l->diffuse[1] = zmsgF(g);
		l->diffuse[2] = zmsgF(b);
	}
}

ZMSG_HANDLER( Mater_SpecularChange ) {
	ZGLMaterial *l = zglMaterialFetch( materSelected );
	if( l ) {
		l->specular[0] = zmsgF(r);
		l->specular[1] = zmsgF(g);
		l->specular[2] = zmsgF(b);
	}
}

ZMSG_HANDLER( Mater_EmissionChange ) {
	ZGLMaterial *l = zglMaterialFetch( materSelected );
	if( l ) {
		l->emission[0] = zmsgF(r);
		l->emission[1] = zmsgF(g);
		l->emission[2] = zmsgF(b);
	}
}
