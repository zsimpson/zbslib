// @ZBS {
//		*MODULE_NAME ztrackball
//		+DESCRIPTION {
//			Implements the UI for spinning something in 3d with the mouse
//		}
//		*PORTABILITY win32
//		*REQUIRED_FILES ztrackball.cpp ztrackball.h
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
// MODULE includes:
#include "ztrackball.h"
// ZBSLIB includes:
#include "zvec.h"
#include "zmousemsg.h"

int zTrackballHasMat = 1;
FMat4 zTrackballMat;
FVec2 zTrackballStartDrag;
FMat4 zTrackballStartDragMat;

extern int requestExclusiveMouse( int anywhere, int onOrOff );
	// @TODO: This needs to be converted into somekind
	// of message so that we don't need this awful hack

void zTrackballReset() {
	zTrackballHasMat = 0;
}

void zTrackballSetupView() {
	if( !zTrackballHasMat ) {
		zTrackballHasMat = 1;
		FMat4 mat;
		glGetFloatv( GL_MODELVIEW_MATRIX, mat );
		mat.setTrans( FVec3::Origin );
		zTrackballMat = mat;
	}
	else {
		glMultMatrixf( zTrackballMat );
	}
}

int zTrackballSave( char *filename ) {
	FILE *f = fopen( filename, "wb" );
	if( f ) {
		fprintf( f, "%f %f %f %f\n", zTrackballMat.m[0][0], zTrackballMat.m[1][0], zTrackballMat.m[2][0], zTrackballMat.m[3][0] );
		fprintf( f, "%f %f %f %f\n", zTrackballMat.m[0][1], zTrackballMat.m[1][1], zTrackballMat.m[2][1], zTrackballMat.m[3][1] );
		fprintf( f, "%f %f %f %f\n", zTrackballMat.m[0][2], zTrackballMat.m[1][2], zTrackballMat.m[2][2], zTrackballMat.m[3][2] );
		fprintf( f, "%f %f %f %f\n", zTrackballMat.m[0][3], zTrackballMat.m[1][3], zTrackballMat.m[2][3], zTrackballMat.m[3][3] );
		fclose( f );
		return 1;
	}
	return 0;
}


int zTrackballLoad( char *filename ) {
	FILE *f = fopen( filename, "rt" );
	if( f ) {
		fscanf( f, "%f %f %f %f", &zTrackballMat.m[0][0], &zTrackballMat.m[1][0], &zTrackballMat.m[2][0], &zTrackballMat.m[3][0] );
		fscanf( f, "%f %f %f %f", &zTrackballMat.m[0][1], &zTrackballMat.m[1][1], &zTrackballMat.m[2][1], &zTrackballMat.m[3][1] );
		fscanf( f, "%f %f %f %f", &zTrackballMat.m[0][2], &zTrackballMat.m[1][2], &zTrackballMat.m[2][2], &zTrackballMat.m[3][2] );
		fscanf( f, "%f %f %f %f", &zTrackballMat.m[0][3], &zTrackballMat.m[1][3], &zTrackballMat.m[2][3], &zTrackballMat.m[3][3] );
		fclose( f );
		return 1;
	}
	return 0;
}


void zTrackballHandleMsg( ZMsg *msg ) {
	static int dragging = 0;
//	if( zmsgIs(type,Key) && zmsgIs(which,wheelforward) ) {
	if( zmsgIs(type,Key) && zmsgIs(which,.) ) {
//		FVec3 trans = zTrackballMat.getTrans();
//		trans.z *= 1.1f;
//		zTrackballMat.setTrans( trans );
		FMat4 s = scale3D( FVec3(1.1f,1.1f,1.1f ) );
		zTrackballMat.cat( s );
		zMsgUsed();
	}
//	else if( zmsgIs(type,Key) && zmsgIs(which,wheelbackward) ) {
	else if( zmsgIs(type,Key) && zmsgIs(which,/) ) {
//		FVec3 trans = zTrackballMat.getTrans();
//		trans.z *= 0.8f;
//		zTrackballMat.setTrans( trans );
		FMat4 s = scale3D(FVec3(0.9f,0.9f,0.9f));
		zTrackballMat.cat( s );
		zMsgUsed();
	}

	if( zmsgIs(type,MouseClick) && zmsgIs(dir,D) && (zmsgIs(which,R) || zmsgIs(which,M) ) ) {
		// I had commented out the M button for some reason probably because
		// it conflicted with something.  But I need it in Jarle.  So this probably
		// needs to turn into a option but I will leave it on until
		// I see what it is that needs it turned off.  Of course the
		// higher level thing could trap it and "zmsgUsed" to make ti disappear

		zTrackballStartDrag = FVec2( zmsgF(localX), zmsgF(localY) );
		zTrackballStartDragMat = zTrackballMat;
		dragging = zMouseMsgRequestExclusiveDrag( "type=Trackball_MouseDrag" );
		zMsgUsed();
	}
	else if( zmsgIs(type,Trackball_MouseDrag) ) {
		if( zmsgI(releaseDrag) ) {
			zMouseMsgCancelExclusiveDrag();
		}
		else {
			FVec2 mouseDelta;
			if( zmsgHas(localX) ) {
				mouseDelta = FVec2( zmsgF(localX), zmsgF(localY) );
			}
			else {
				mouseDelta = FVec2( zmsgF(x), zmsgF(y) );
			}
			mouseDelta.sub( zTrackballStartDrag );
			
			if( zmsgI(r) ) {
				mouseDelta.mul( 0.01f );
				zTrackballMat = zTrackballStartDragMat;
				FMat4 eye = zTrackballMat;
				eye.setTrans( FVec3::Origin );
				eye.inverse();
				FVec3 yEye = eye.mul( FVec3::YAxis );
				FVec3 xEye = eye.mul( FVec3::XAxisMinus );
				FMat4 r1 = rotate3D( yEye, mouseDelta.x );
				FMat4 r2 = rotate3D( xEye, mouseDelta.y );
				zTrackballMat.cat( r1 );
				zTrackballMat.cat( r2 );
				zMsgUsed();
			}
			else if( zmsgI(m) ) {
				mouseDelta.mul( 0.2f );
					// Come up with a way to scale this correctly.  Need to convert
					// mouse coords into world coords or something
				FVec3 trans = zTrackballStartDragMat.getTrans();
				trans.add( FVec3(mouseDelta.x,mouseDelta.y,0.f) );
				zTrackballMat.setTrans( trans );
				zMsgUsed();
			}
		}
	}
	else if( zmsgIs(type,ZTrackball_Save) ) {
		zTrackballSave( zmsgS(filename) );
	}
	else if( zmsgIs(type,ZTrackball_Load) ) {
		zTrackballLoad( zmsgS(filename) );
	}
}
