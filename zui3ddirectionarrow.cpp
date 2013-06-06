// @ZBS {
//		*COMPONENT_NAME zui3ddirectionarrow
//		*MODULE_OWNER_NAME zui
//		*GROUP modules/zui
//		*REQUIRED_FILES zui3ddirectionarrow.cpp
// }

// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#include "GL/gl.h"
#include "GL/glu.h"
// STDLIB includes:
#include "string.h"
#include "math.h"
// MODULE includes:
#include "zui.h"
// ZBSLIB includes:
#include "zmathtools.h"
#include "zgltools.h"
#include "zvec.h"

struct ZUI3DDirectionArrow : ZUI {
	ZUI_DEFINITION(ZUI3DDirectionArrow,ZUI);

	FVec2 startDrag;
	FMat4 startDragMat;
	FMat4 mat;

	void sendMsg();
	virtual void render();
	virtual ZUI::ZUILayoutCell *layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly );
	virtual void handleMsg( ZMsg *msg );
};


ZUI_IMPLEMENT(ZUI3DDirectionArrow,ZUI);

void ZUI3DDirectionArrow::render() {

	glPushAttrib( GL_ALL_ATTRIB_BITS );

	float winX=0.f, winY=0.f;
	getWindowXY( winX, winY );

	float myW, myH;
	getWH( myW, myH );

	int viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );

	glViewport( viewport[0]+(int)winX, viewport[1]+(int)winY, (int)myW, (int)myH );
	glScissor( viewport[0]+(int)winX, viewport[1]+(int)winY, (int)myW, (int)myH );

	// @TODO: Would be nice to put in normals and a light source on this
	glEnable( GL_LIGHTING );
	glEnable( GL_LIGHT0 );
	float pos0[4] = { -1.f, 0.5, 0.f, 0.f };
	float dif0[4] = { 0.3f, 0.3f, 0.6f, 1.f };
	glLightfv( GL_LIGHT0, GL_POSITION, pos0 );
	glLightfv( GL_LIGHT0, GL_DIFFUSE, dif0 );
	glEnable( GL_LIGHT1 );
	float pos1[4] = { 0.8f, 0.1f, 0.001f, 0.f };
	float dif1[4] = { 0.8f, 0.8f, 1.0f, 1.f };
	glLightfv( GL_LIGHT1, GL_POSITION, pos1 );
	glLightfv( GL_LIGHT1, GL_DIFFUSE, dif1 );

	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();
	gluPerspective( 45.f, 1.f, 0.1, 10.0 );
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();
	gluLookAt( 0.0, 1.2, 2.5, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0 );

	glMultMatrixf( (const float *)&mat );

	glClear( GL_DEPTH_BUFFER_BIT );
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_COLOR_MATERIAL );

	// DRAW the stick part
	{
		glColor3ub( 255, 255, 255 );
		glBegin( GL_QUAD_STRIP );
		float tf = 0.f, tfStep = PI2F / 16.f;
		for( int t=0; t<=16; t++, tf += tfStep ) {
			FVec3 a( 0.f, sinf(tf), cosf(tf) );
			FVec3 top = a;
			top.mul( 0.1f );
			top.x = 0.3f;
			FVec3 bot = a;
			bot.mul( 0.2f );
			bot.x = -1.f;
			glNormal3fv( a );
			glVertex3fv( top );
			glVertex3fv( bot );
		}
		glEnd();
	}

	// DRAW a cap on the stick part
	{
		glColor3ub( 255, 255, 255 );
		glBegin( GL_TRIANGLE_FAN );
		float tf = 0.f, tfStep = PI2F / 16.f;
		glNormal3f( -1.f, 0.f, 0.f );
		glVertex3f( -1.f, 0.f, 0.f );
		for( int t=0; t<=16; t++, tf += tfStep ) {
			glVertex3f( -1.f, 0.2f*(float)sin(tf), 0.2f*(float)cos(tf) );
		}
		glEnd();
	}

	// DRAW the pointy part
	{
		glColor3ub( 255, 200, 200 );
		glBegin( GL_QUAD_STRIP );
		float tf = 0.f, tfStep = PI2F / 16.f;
		for( int t=0; t<=16; t++, tf += tfStep ) {
			FVec3 a( 0.f, (float)sin(tf), (float)cos(tf) );
			FVec3 top = a;
			top.mul( 0.0f );
			top.x = 1.f;
			FVec3 bot = a;
			bot.mul( 0.3f );
			bot.x = 0.3f;
			glNormal3fv( a );
			glVertex3fv( top );
			glVertex3fv( bot );
		}
		glEnd();
	}

	// DRAW a cap on the pointy part
	{
		glColor3ub( 255, 200, 200 );
		glBegin( GL_TRIANGLE_FAN );
		float tf = 0.f, tfStep = PI2F / 16.f;
		glNormal3f( -1.f, 0.f, 0.f );
		glVertex3f( 0.3f, 0.f, 0.f );
		for( int t=0; t<=16; t++, tf += tfStep ) {
			glVertex3f( 0.3f, 0.3f*(float)sin(tf), 0.3f*(float)cos(tf) );
		}
		glEnd();
	}

	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();
	glPopAttrib();
}

ZUI::ZUILayoutCell *ZUI3DDirectionArrow::layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly ) {
	w = 100.f;
	h = 100.f;
	return 0;
}

void ZUI3DDirectionArrow::sendMsg() {
	if( has( "sendMsg" ) ) {
		zMsgQueue( "%s matPtr=0x%x", getS("sendMsg"), &mat );
	}
}

void ZUI3DDirectionArrow::handleMsg( ZMsg *msg ) {
	if( zmsgIs(type,ZUIMouseClickOn) && zmsgIs(which,L) && zmsgIs(dir,D) ) {
		startDrag = FVec2( zmsgF(localX), zmsgF(localY) );
		startDragMat = mat;
		requestExclusiveMouse( 1, 1 );
		zMsgUsed();
		sendMsg();
	}
	else if( zmsgIs(type,ZUIMouseReleaseDrag) ) {
		requestExclusiveMouse( 1, 0 );
		zMsgUsed();
	}
	else if( zmsgIs(type,ZUIExclusiveMouseDrag) ) {
		FVec2 mouseDelta( zmsgF(localX), zmsgF(localY) );
		mouseDelta.sub( startDrag );
		mouseDelta.mul( 0.03f );
		mat = startDragMat;
		FMat4 eye = mat;
		eye.setTrans( FVec3::Origin );
		eye.inverse();
		FVec3 yEye = eye.mul( FVec3::YAxisMinus );
		//FVec3 yEye = eye.mul( FVec3::YAxis );
		FVec3 xEye = eye.mul( FVec3::XAxis );
		mat.cat( rotate3D( yEye, mouseDelta.x ) );
		mat.cat( rotate3D( xEye, mouseDelta.y ) );
		zMsgUsed();
		sendMsg();
	}
	else if( zmsgIs(type,SetDir) ) {
		FVec3 xaxis( zmsgF(x), zmsgF(y), zmsgF(z) );
		xaxis.mul(-1.f);
		FVec3 yaxis( 0.f, 1.f, 0.f );
		yaxis.cross( xaxis );
		FVec3 zaxis = yaxis;
		zaxis.cross( xaxis );
		xaxis.normalize();
		yaxis.normalize();
		zaxis.normalize();
		mat.m[0][0] = xaxis.x;
		mat.m[0][1] = xaxis.y;
		mat.m[0][2] = xaxis.z;
		mat.m[0][3] = 0.f;
		mat.m[1][0] = yaxis.x;
		mat.m[1][1] = yaxis.y;
		mat.m[1][2] = yaxis.z;
		mat.m[1][3] = 0.f;
		mat.m[2][0] = zaxis.x;
		mat.m[2][1] = zaxis.y;
		mat.m[2][2] = zaxis.z;
		mat.m[2][3] = 0.f;
		mat.m[3][0] = 0.f;
		mat.m[3][1] = 0.f;
		mat.m[3][2] = 0.f;
		mat.m[3][3] = 1.f;
	}
}
