// @ZBS {
//		*MODULE_NAME zprofglgui
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			GL User interface for the zprof profile system
//		}
//		*PORTABILITY win32
//		*SDK_DEPENDS freetype
//		*REQUIRED_FILES zprofglgui.cpp zprofglgui.h
//		*VERSION 1.0
//		+HISTORY {
//			1.0 19 July 2005 ZBS Created
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH yes
// }

// OPERATING SYSTEM specific includes:
#include "wingl.h"
#ifdef __APPLE__
#include "OpenGL/gl.h"
#else
#include "GL/gl.h"
#endif
// SDK includes:
// STDLIB includes:
#include "stdio.h"
#include "string.h"
// MODULE includes:
#include "zglfont.h"
// ZBSLIB includes:
#include "zprof.h"

int zprofGLGUIVisible = 0;
void *zprofGLGUIFontPtr = 0;
float zprofGLGUIFontW = 0.f;
float zprofGLGUIFontH = 0.f;

#if defined(ZPROF) && defined(WIN32)

void zprofGLGUIToggle() {
	if( ! zprofGLGUIVisible ) {
		zprofSortTree();
	}
	zprofGLGUIVisible = ! zprofGLGUIVisible;
}

void zprofGLGUIDumpRecurse( ZProf *r, int depth, float &y, ZProf *root, int which ) {
	char line[256] = {0};

	__int64 percentOfTotal_0 = 0;
	__int64 percentOfTotal_1 = 0;
	if( root && root->accumTick[0] && root->accumTick[1] ) {
		percentOfTotal_0 = 10000 * r->accumTick[0] / root->accumTick[0];
		percentOfTotal_1 = 10000 * r->accumTick[1] / root->accumTick[1];
	}
	if( which & 1 ) {
		sprintf( line+strlen(line), "%03.0lf ", (double)percentOfTotal_0 / 100.0 );
	}
	if( which & 2 ) {
		sprintf( line+strlen(line), "%03.0lf ", (double)percentOfTotal_1 / 100.0 );
	}

	__int64 percentOfParent_0 = 0;
	__int64 percentOfParent_1 = 0;
	if( r->parent && r->parent->accumTick[0] && r->parent->accumTick[1] ) {
		percentOfParent_0 = 10000 * r->accumTick[0] / r->parent->accumTick[0];
		percentOfParent_1 = 10000 * r->accumTick[1] / r->parent->accumTick[1];
	}
	if( which & 1 ) {
		sprintf( line+strlen(line), "%03.0lf ", (double)percentOfParent_0 / 100.0);
	}
	if( which & 2 ) {
		sprintf( line+strlen(line), "%03.0lf ", (double)percentOfParent_1 / 100.0);
	}

	__int64 percentChange_0 = 0;
	__int64 percentChange_1 = 0;
	if( r->count[0] && r->count[1] && r->refTick[0] && r->refTick[1] ) {
		percentChange_0 = 10000 * r->accumTick[0]/r->count[0] / r->refTick[0];
		percentChange_1 = 10000 * r->accumTick[1]/r->count[1] / r->refTick[1];
	}
	if( which & 1 ) {
		sprintf( line+strlen(line), "%05.0lf ", (double)percentChange_0 / 100.0 );
	}
	if( which & 2 ) {
		sprintf( line+strlen(line), "%05.0lf ", (double)percentChange_1 / 100.0 );
	}

	sprintf( line+strlen(line), "%I64d ", r->count[0] );

	strcat( line, r->identString );

	zglFontPrint( line, (float)depth * 10.f, y, 0, 1, zprofGLGUIFontPtr );
	y -= zprofGLGUIFontH;

	for( ZProf *i = r->chldHead; i; i=i->siblNext ) {
		zprofGLGUIDumpRecurse( i, depth + 1, y, root, which );
	}
}

void zprofGLGUIRender( int which ) {
	if( ! zprofGLGUIVisible ) return;

	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();

	// SETUP pixel matrix first quadrant
	int viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );
	glTranslatef( -1.0f, -1.0f, 0.f );
	glScalef( 2.0f/(float)viewport[2], 2.0f/(float)viewport[3], 1.f );

	glPushAttrib( GL_ALL_ATTRIB_BITS );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_BLEND );

	zprofGLGUIFontPtr = zglFontGet( "zprofglgui" );
	if( !zprofGLGUIFontPtr ) {
		zglFontLoad( "zprofglgui", "verdana.ttf", 10.f );
		zprofGLGUIFontPtr = zglFontGet( "zprofglgui" );
		float d;
		zglFontGetDim( "Q", zprofGLGUIFontW, zprofGLGUIFontH, d, "zprofglgui" );
	}

	// FIND the root
	ZProf *root = 0;
	ZProf *i;
	for( i = ZProf::heapHead; i; i=i->heapNext ) {
		if( !strcmp(i->identString,"root") ) {
			root = i;
			break;
		}
	}

	float y = viewport[3] - zprofGLGUIFontH;
	for( i = ZProf::heapHead; i; i=i->heapNext ) {
		if( ! i->parent ) {
			zprofGLGUIDumpRecurse( i, 0, y, root, which );
		}
	}

	for( int j=0; j<ZProf::extraLineCount; j++ ) {
		zglFontPrint( ZProf::extraLines[j], 0.f, y, 0, 1, zprofGLGUIFontPtr );
		y -= zprofGLGUIFontH;
	}

	glPopAttrib();
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();

}

#else

void zprofGLGUIToggle() { }
void zprofGLGUIDumpRecurse( struct ZProf *r, int depth, float &y, ZProf *root, int which ) { }
void zprofGLGUIRender( int which ) { }

#endif
