// @ZBS {
//		*MODULE_NAME zviewpoint
//		+DESCRIPTION {
//			Implements the UI for generic 2D and 3D motion around a space
//		}
//		*PORTABILITY win32
//		*REQUIRED_FILES zviewpoint.cpp zviewpoint.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#include "GL/gl.h"
#include "GL/glu.h"
// STDLIB includes:
#include "stdio.h"
#include "string.h"
#include "math.h"
#include "float.h"
// MODULE includes:
#include "zviewpoint.h"
// ZBSLIB includes:
#include "zmousemsg.h"
#include "zmsg.h"
#include "zvec.h"
#include "zmathtools.h"

FVec3 zviewpointTrans;
FQuat zviewpointRotQuat;
float zviewpointScale = 1.f;
DMat4 zviewpointReferenceModel;
DMat4 zviewpointReferenceProj;
int zviewpointReferenceViewport[4];

int zviewpointPermitRotX = 1;
int zviewpointPermitRotY = 1;
int zviewpointPermitRotZ = 1;
int zviewpointPermitTransX = 1;
int zviewpointPermitTransY = 1;
int zviewpointPermitTransZ = 1;
int zviewpointPermitScale = 1;

int zviewpointRotateButton    = 'M';
int zviewpointTranslateButton = 'R';
int zviewpointRotating = 0;
int zviewpointTranslating = 0;

int zviewpointMode = ZVIEWPOINTMODE_TRACKBALL;

void zviewpointSetMode( int mode ) {
	zviewpointMode = mode;
}

FVec3 zviewpointProjOnPlane( int scnX, int scnY, FVec3 p0, FVec3 planeNrm ) {
/*
	DMat4 modl = zviewpointReferenceModel;
    DMat4 _s = scale3D( DVec3(zviewpointScale, zviewpointScale, zviewpointScale));
	modl.cat( _s );
    DMat4 _t = trans3D( DVec3(zviewpointPermitTransX ? zviewpointTrans[0] : 0.f, zviewpointPermitTransY ? zviewpointTrans[1] : 0.f, zviewpointPermitTransZ ? zviewpointTrans[2] : 0.f));
	modl.cat( _t );
    FMat4 _v = zviewpointRotQuat.mat(); 
	DMat4 rot = DMat4( _v );
	rot.transpose();
	modl.cat( rot );

	if( zviewpointReferenceViewport[2]==0 || zviewpointReferenceViewport[3]==0 ) {
		// PROTECT against exception
		return FVec2( (float)0.f, (float)0.f );
	}
*/
	int viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );

	DMat4 projMat;
	glGetDoublev( GL_PROJECTION_MATRIX, (GLdouble *)projMat.m );

	DMat4 modelMat;
	glGetDoublev( GL_MODELVIEW_MATRIX, (GLdouble *)modelMat.m );

	double world[2][3] = {0,};
	gluUnProject( scnX, scnY, 0.f, modelMat, projMat, viewport, &world[0][0], &world[0][1], &world[0][2] );
	gluUnProject( scnX, scnY, 1.f, modelMat, projMat, viewport, &world[1][0], &world[1][1], &world[1][2] );
	FVec3 wp0 = zviewpointLinePlaneIntersect(
		p0,
		planeNrm,
		FVec3((float)world[0][0],(float)world[0][1],(float)world[0][2]), 
		FVec3((float)world[1][0],(float)world[1][1],(float)world[1][2])
	);
	return wp0;

}

FVec3 zviewpointProjMouseOnZ0Plane( ) {
	return zviewpointProjOnPlane( zMouseMsgX, zMouseMsgY, FVec3::Origin, FVec3::ZAxis );
}

FVec3 zviewpointLinePlaneIntersect( FVec3 p0, FVec3 norm, FVec3 p1, FVec3 p2 ) {
	FVec3 p0minusp1 = p0;
	p0minusp1.sub( p1 );
	FVec3 p2Minusp1 = p2;
	p2Minusp1.sub( p1 );
	float numerator   = norm.dot( p0minusp1 );
	float denominator = norm.dot( p2Minusp1 );
	// @todo: num or denom could be zero
	if( fabsf(denominator) > FLT_EPSILON ) {
		p2Minusp1.mul( numerator / denominator );
	}
	else {
		return FVec3::Origin;
	}
	p2Minusp1.add( p1 );
	return p2Minusp1;
}

void zviewpointSetPermitRot( int xOnOff, int yOnOff, int zOnOff ) {
	if( xOnOff != zviewpointPermitRotX || yOnOff != zviewpointPermitRotY || zOnOff != zviewpointPermitRotZ ) {
		zviewpointRotQuat.identity();
	}
	zviewpointPermitRotX = xOnOff;
	zviewpointPermitRotY = yOnOff;
	zviewpointPermitRotZ = zOnOff;
}

void zviewpointSetPermitTrans( int xOnOff, int yOnOff, int zOnOff ) {
	zviewpointPermitTransX = xOnOff;
	zviewpointPermitTransY = yOnOff;
	zviewpointPermitTransZ = zOnOff;
}

void zviewpointSetPermitScale( int onOff ) {
	zviewpointPermitScale = onOff;
}

void zviewpointSetButtons( char rotateButton, char translateButton ) {
	zviewpointRotateButton = rotateButton;
	zviewpointTranslateButton = translateButton;
}

void zviewpointSetup2DView( float l, float t, float r, float b ) {
	float extraW = 0.f, extraH = 0.f;
	if( r-l > t-b ) {
		zviewpointScale = (float)zviewpointReferenceViewport[2] / (r-l);
		extraH = ((zviewpointReferenceViewport[3] - (t-b)*zviewpointScale ) / 2.f) / zviewpointScale;
	}
	else {
		zviewpointScale = (float)zviewpointReferenceViewport[3] / (t-b);
		extraW = ((zviewpointReferenceViewport[2] - (r-l)*zviewpointScale ) / 2.f) / zviewpointScale;
	}
	zviewpointTrans.x = -l + extraW;
	zviewpointTrans.y = -b + extraH;
}

void zviewpointSetupView() {
	if( !zviewpointPermitScale ) {
		zviewpointScale = 1.f;
	}
	if( !zviewpointPermitTransX ) {
		zviewpointTrans.x = 0.f;
	}
	if( !zviewpointPermitTransY ) {
		zviewpointTrans.y = 0.f;
	}
	if( !zviewpointPermitTransZ ) {
		zviewpointTrans.z = 0.f;
	}

	// GRAB the reference matrix
	glGetDoublev( GL_MODELVIEW_MATRIX, zviewpointReferenceModel );
	glGetDoublev( GL_PROJECTION_MATRIX, zviewpointReferenceProj );
	glGetIntegerv( GL_VIEWPORT, zviewpointReferenceViewport );

	glScalef( zviewpointScale, zviewpointScale, zviewpointScale );
	glTranslatef( zviewpointPermitTransX ? zviewpointTrans[0] : 0.f, zviewpointPermitTransY ? zviewpointTrans[1] : 0.f, zviewpointPermitTransZ ? zviewpointTrans[2] : 0.f );
	FMat4 rot = zviewpointRotQuat.mat();
	rot.transpose();
	glMultMatrixf( rot );
}

void zviewpointDumpInCFormat( char *filename ) {
	glPushMatrix();
	glLoadIdentity();

	if( !zviewpointPermitScale ) {
		zviewpointScale = 1.f;
	}
	if( !zviewpointPermitTransX ) {
		zviewpointTrans.x = 0.f;
	}
	if( !zviewpointPermitTransY ) {
		zviewpointTrans.y = 0.f;
	}
	if( !zviewpointPermitTransZ ) {
		zviewpointTrans.z = 0.f;
	}

	glScalef( zviewpointScale, zviewpointScale, zviewpointScale );
	glTranslatef( zviewpointPermitTransX ? zviewpointTrans[0] : 0.f, zviewpointPermitTransY ? zviewpointTrans[1] : 0.f, zviewpointPermitTransZ ? zviewpointTrans[2] : 0.f );
	FMat4 rot = zviewpointRotQuat.mat();
	rot.transpose();
	glMultMatrixf( rot );

	float m[16];
	glGetFloatv( GL_MODELVIEW_MATRIX, m );
	glPopMatrix();

	FILE *file = fopen( filename, "wt" );
	fprintf( file, "float _m[] = { %ff, %ff, %ff, %ff, %ff, %ff, %ff, %ff, %ff, %ff, %ff, %ff, %ff, %ff, %ff, %ff };\n",
		m[0], m[1], m[2], m[3],
		m[4], m[5], m[6], m[7],
		m[8], m[9], m[10], m[11],
		m[12], m[13], m[14], m[15]
	);
	fclose( file );
}

void zviewpointReset() {
	memset( &zviewpointTrans, 0, sizeof(zviewpointTrans) );
	zviewpointRotQuat.fromAxisAngle( FVec3::XAxis, 0.f );
	zviewpointScale = 1.f;

	// This was commented out but it being commented out caused a bug in that when one
	// plugin clears the rotation and the you switch to another then it can't rotate
	// It makes sense that startup should clear all this.
	zviewpointPermitRotX = 1;
	zviewpointPermitRotY = 1;
	zviewpointPermitRotZ = 1;
	zviewpointPermitTransX = 1;
	zviewpointPermitTransY = 1;
	zviewpointPermitTransZ = 1;
	zviewpointPermitScale = 1;
}

void zviewpointHandleMsgTrackball( ZMsg *msg ) {
	static float viewpointMouseLast[2];
	static int dragging = 0;

	float newzviewpointScale = 0.f;
	int scaleChange = 0;

	if( (zmsgIs(type,ZUIMouseClickOn) || zmsgIs(type,MouseClick)) && zmsgIs(dir,D) && (zmsgIs(which,R) || zmsgIs(which,M)) && zmsgI(shift) && zmsgI(ctrl) && zmsgI(alt) ) {
		// RESET
		memset( &zviewpointTrans, 0, sizeof(zviewpointTrans) );
		zviewpointRotQuat.fromAxisAngle( FVec3::XAxis, 0.f );
		zviewpointScale = 1.f;
	}

	char button = msg->getS( "which", "X" ) [0]; 

//	if( (zmsgIs(type,ZUIMouseClickOn) || zmsgIs(type,MouseClick)) && zmsgIs(dir,D) && (  ( zmsgIs(which,M) && (zviewpointPermitRotX||zviewpointPermitRotY||zviewpointPermitRotZ) ) || zmsgIs(which,R)  ) ) {
	if( (zmsgIs(type,ZUIMouseClickOn) || zmsgIs(type,MouseClick)) && zmsgIs(dir,D) && (  ( button==zviewpointRotateButton && (zviewpointPermitRotX||zviewpointPermitRotY||zviewpointPermitRotZ) ) || button==zviewpointTranslateButton  ) ) {

		// I added the checks on permitrot but I think that this is probably temporay.  I probably need to put in
		// some kind of better mapping system so that calling code can assign the mappings that they want

		// I had commented out the M button for some reason probably because
		// it conflicted with something.  But I need it in Jarle.  So this probably
		// needs to turn into a option but I will leave it on until
		// I see what it is that needs it turned off.  Of course the
		// higher level thing could trap it and "zmsgUsed" to make ti disappear

		viewpointMouseLast[0] = zmsgF(x);
		viewpointMouseLast[1] = zmsgF(y);

		dragging = zMouseMsgRequestExclusiveDrag( "type=Viewpoint_MouseDrag" );
		if( dragging ) {
			zviewpointRotating = button == zviewpointRotateButton;
			zviewpointTranslating = button == zviewpointTranslateButton;
		}
		zMsgUsed();
	}
	else if( (zmsgIs(type,Key) && zmsgIs(which,wheelforward)) || (zmsgIs(type,Key) && !strcmp(msg->getS("which"),",") ) ) {
		newzviewpointScale = zviewpointScale * 0.8f;
		scaleChange = 1;
		zMsgUsed();
	}
	else if( (zmsgIs(type,Key) && zmsgIs(which,wheelbackward)) || (zmsgIs(type,Key) && !strcmp(msg->getS("which"),".") ) ) {
		newzviewpointScale = zviewpointScale * 1.2f;
		scaleChange = 1;
		zMsgUsed();
	}

	if( scaleChange && zviewpointPermitScale ) {
		float x = (float)zMouseMsgX;
		float y = (float)zMouseMsgY;

		// I want the world coord at which the mouse is pointing before the zoom 
		// to be the same as the world coord at which the mouse is pointing after the zoom
		// @TODO: convert his mess into a single function
		DMat4 preScale = zviewpointReferenceModel;
        DMat4 preScaleMat( scale3D( DVec3(zviewpointScale, zviewpointScale, zviewpointScale) ) );
		preScale.cat( preScaleMat );

		DVec3 pre0, pre1;
		gluUnProject( x, y, 0.f, preScale, zviewpointReferenceProj, zviewpointReferenceViewport, &pre0.x, &pre0.y, &pre0.z );
		gluUnProject( x, y, 1.f, preScale, zviewpointReferenceProj, zviewpointReferenceViewport, &pre1.x, &pre1.y, &pre1.z );
		FVec3 wp0 = zviewpointLinePlaneIntersect( FVec3::Origin, FVec3::ZAxis, FVec3((float)pre0.x,(float)pre0.y,(float)pre0.z), FVec3((float)pre1.x,(float)pre1.y,(float)pre1.z) );

		DMat4 postScale = zviewpointReferenceModel;
        DMat4 postScaleMat = ( scale3D( DVec3(newzviewpointScale, newzviewpointScale, newzviewpointScale) ) );
		postScale.cat( postScaleMat );
		
		DVec3 post0, post1;
		gluUnProject( x, y, 0.f, postScale, zviewpointReferenceProj, zviewpointReferenceViewport, &post0.x, &post0.y, &post0.z );
		gluUnProject( x, y, 1.f, postScale, zviewpointReferenceProj, zviewpointReferenceViewport, &post1.x, &post1.y, &post1.z );
		FVec3 wp1 = zviewpointLinePlaneIntersect( FVec3::Origin, FVec3::ZAxis, FVec3((float)post0.x,(float)post0.y,(float)post0.z), FVec3((float)post1.x,(float)post1.y,(float)post1.z) );

		wp1.sub( wp0 );
		zviewpointTrans.add( wp1 );

		zviewpointScale = newzviewpointScale;
	}


	if( zmsgIs(type,Viewpoint_MouseDrag) ) {
		if( zmsgI(releaseDrag) ) {
			zMouseMsgCancelExclusiveDrag();
			zviewpointRotating = zviewpointTranslating = 0;
		}
		else {
//			if( zmsgI(m) ) {
			if( zviewpointRotating ) {
				// ROTATE

				// COMPUTE the world axises about which we are spinning (i.e. the screen axis)
				FMat4 ref( zviewpointReferenceModel.m );
				FMat4 rot = zviewpointRotQuat.mat();
				rot.transpose();
				ref.cat( rot );
				ref.setTrans( FVec3::Origin );
				ref.orthoNormalize();
				ref.inverse();
				FVec3 xEye = ref.mul( FVec3::XAxis );
				FVec3 yEye = ref.mul( FVec3::YAxis );
				FVec3 zEye = ref.mul( FVec3::ZAxis );

				FQuat deltaX( xEye, -0.01f * ( viewpointMouseLast[1] - zmsgF(y) ) );
				if( !zviewpointPermitRotX ) deltaX.identity();
				FQuat deltaY( yEye,  0.01f * ( viewpointMouseLast[0] - zmsgF(x) ) );
				if( !zviewpointPermitRotY ) deltaY.identity();
				FQuat deltaZ;
				if( zmsgI(shift) ) {
					deltaX.identity();
					deltaY.identity();
					float side = 1.f;
					if( zmsgF(localX) < zmsgF(w)/2 ) {
						// If we're on the left or right side, spin the opposite the other
						side = -1.f;
					}
					deltaZ.fromAxisAngle( zEye, side * 0.01f * ( viewpointMouseLast[1] - zmsgF(y) ) );
				}
				if( !zviewpointPermitRotZ ) deltaZ.identity();

				// QUATERNION multiply the delta by the origial to get the new
				deltaX.mul( zviewpointRotQuat );
				deltaY.mul( deltaX );
				deltaZ.mul( deltaY );
				zviewpointRotQuat = deltaZ;

				viewpointMouseLast[0] = zmsgF(x);
				viewpointMouseLast[1] = zmsgF(y);
				zMsgUsed();
			}
//			else if( zmsgI(r) ) {
			else if( zviewpointTranslating ) {
				// COMPUTE how big is one pixel at the world z-plane of interest
				// For now, this plane normal to the camera and passes-though the origin
				// Do this by unprojecting rays at the mouse current and last and then
				// solving for the world position at the intersection of that plane

				// CAT in tranforms which take place before the translate
				DMat4 m = zviewpointReferenceModel;
                DMat4 _m( scale3D( DVec3(zviewpointScale, zviewpointScale, zviewpointScale) ) );
				m.cat( _m );
                DMat4 _m1( trans3D( DVec3(zviewpointTrans )) );
				m.cat( _m1 );

				DVec3 p0, p1, p2, p3;
				gluUnProject( zmsgF(x), zmsgF(y), 0.f, m, zviewpointReferenceProj, zviewpointReferenceViewport, &p0.x, &p0.y, &p0.z );
				gluUnProject( zmsgF(x), zmsgF(y), 1.f, m, zviewpointReferenceProj, zviewpointReferenceViewport, &p1.x, &p1.y, &p1.z );
				gluUnProject( viewpointMouseLast[0], viewpointMouseLast[1], 0.f, m, zviewpointReferenceProj, zviewpointReferenceViewport, &p2.x, &p2.y, &p2.z );
				gluUnProject( viewpointMouseLast[0], viewpointMouseLast[1], 1.f, m, zviewpointReferenceProj, zviewpointReferenceViewport, &p3.x, &p3.y, &p3.z );
				FVec3 wp0 = zviewpointLinePlaneIntersect( FVec3::Origin, FVec3::ZAxis, FVec3((float)p0.x,(float)p0.y,(float)p0.z), FVec3((float)p1.x,(float)p1.y,(float)p1.z) );
				FVec3 wp1 = zviewpointLinePlaneIntersect( FVec3::Origin, FVec3::ZAxis, FVec3((float)p2.x,(float)p2.y,(float)p2.z), FVec3((float)p3.x,(float)p3.y,(float)p3.z) );
				wp0.sub( wp1 );

				FVec3 delta( wp0.x, wp0.y, 0.f );

				if( zmsgI(shift) ) {
					delta.x = 0.f;
					delta.y = 0.f;
					delta.z = wp0.y * 4.f;
				}

				zviewpointTrans.add( delta );

				viewpointMouseLast[0] = zmsgF(x);
				viewpointMouseLast[1] = zmsgF(y);
				zMsgUsed();
			}
		}
	}
}

void zviewpointHandleMsgFly( ZMsg *msg ) {
	static float viewpointMouseLast[2];
	static int dragging = 0;

	float newzviewpointScale = 0.f;
	int scaleChange = 0;

	if( (zmsgIs(type,ZUIMouseClickOn) || zmsgIs(type,MouseClick)) && zmsgIs(dir,D) && (zmsgIs(which,R) || zmsgIs(which,M)) && zmsgI(shift) && zmsgI(ctrl) && zmsgI(alt) ) {
		// RESET
		memset( &zviewpointTrans, 0, sizeof(zviewpointTrans) );
		zviewpointRotQuat.fromAxisAngle( FVec3::XAxis, 0.f );
		zviewpointScale = 1.f;
	}

	char button = msg->getS( "which", "X" ) [0]; 

//	if( (zmsgIs(type,ZUIMouseClickOn) || zmsgIs(type,MouseClick)) && zmsgIs(dir,D) && zmsgIs(which,M) ) {
	if( (zmsgIs(type,ZUIMouseClickOn) || zmsgIs(type,MouseClick)) && zmsgIs(dir,D) && button==zviewpointRotateButton ) {
		dragging = zMouseMsgRequestExclusiveDrag( "type=Viewpoint_MouseDrag" );
	}

	if( zmsgIs(type,Viewpoint_MouseDrag) ) {
		if( zmsgI(releaseDrag) ) {
			zMouseMsgCancelExclusiveDrag();
		}
		else {
			int _deltaX = zMouseMsgX - zMouseMsgLastX;
			int _deltaY = zMouseMsgY - zMouseMsgLastY;

			FQuat deltaX( FVec3::XAxis, -0.01f * _deltaX );
			FQuat deltaY( FVec3::YAxis, +0.01f * _deltaY );
			FQuat deltaZ;

			deltaX.mul( zviewpointRotQuat );
			deltaY.mul( deltaX );
			deltaZ.mul( deltaY );
			zviewpointRotQuat = deltaZ;
		}
	}
}

void zviewpointHandleMsg( ZMsg *msg ) {
	if( zviewpointMode == ZVIEWPOINTMODE_TRACKBALL ) {
		zviewpointHandleMsgTrackball( msg );
		return;
	}
	if( zviewpointMode == ZVIEWPOINTMODE_FLY ) {
		zviewpointHandleMsgFly( msg );
		return;
	}
}

