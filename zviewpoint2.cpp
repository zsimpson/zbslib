// @ZBS {
//		*MODULE_NAME zviewpoint2
//		+DESCRIPTION {
//			zviewpoint encapsulated in a class
//		}
//		*PORTABILITY win32 osx linux
//		*REQUIRED_FILES zviewpoint2.cpp zviewpoint2.h
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
#ifdef __APPLE__
#include "OpenGL/gl.h"
#include "OpenGL/glu.h"
#else
#include "GL/gl.h"
#include "GL/glu.h"
#endif
// STDLIB includes:
#include "stdio.h"
#include "string.h"
#include "math.h"
#include "float.h"
// MODULE includes:
#include "zviewpoint2.h"
// ZBSLIB includes:
#include "zmousemsg.h"
#include "zmsg.h"
#include "zvec.h"
#include "zmathtools.h"

static FVec3 Ones( 1.f, 1.f, 1.f );


FVec2 ZViewpoint::zviewpointProjOnPlane( int scnX, int scnY, FVec3 p0, FVec3 planeNrm ) {

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
	int viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );

	DMat4 projMat;

	double world[2][3] = {0,};
	gluUnProject( scnX, scnY, 0.f, modl, projMat, viewport, &world[0][0], &world[0][1], &world[0][2] );
	gluUnProject( scnX, scnY, 1.f, modl, projMat, viewport, &world[1][0], &world[1][1], &world[1][2] );
	FVec3 wp0 = zviewpointLinePlaneIntersect( FVec3::Origin, FVec3::ZAxis, FVec3((float)world[0][0],(float)world[0][1],(float)world[0][2]), FVec3((float)world[1][0],(float)world[1][1],(float)world[1][2]) );
	return FVec2( (float)wp0.x, (float)wp0.y );
}

FVec2 ZViewpoint::zviewpointProjMouseOnZ0Plane( ) {
	return zviewpointProjOnPlane( zMouseMsgX, zMouseMsgY, FVec3::Origin, FVec3::ZAxis );
}

FVec3 ZViewpoint::zviewpointLinePlaneIntersect( FVec3 p0, FVec3 norm, FVec3 p1, FVec3 p2 ) {
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

void ZViewpoint::zviewpointSetPermitRot( int xOnOff, int yOnOff, int zOnOff ) {
	if( xOnOff != zviewpointPermitRotX || yOnOff != zviewpointPermitRotY || zOnOff != zviewpointPermitRotZ ) {
		zviewpointRotQuat.identity();
	}
	zviewpointPermitRotX = xOnOff;
	zviewpointPermitRotY = yOnOff;
	zviewpointPermitRotZ = zOnOff;
}

void ZViewpoint::zviewpointSetPermitTrans( int xOnOff, int yOnOff, int zOnOff ) {
	zviewpointPermitTransX = xOnOff;
	zviewpointPermitTransY = yOnOff;
	zviewpointPermitTransZ = zOnOff;
}

void ZViewpoint::zviewpointSetPermitScale( int onOff ) {
	zviewpointPermitScale = onOff;
}

void ZViewpoint::zviewpointSetButtons( char rotateButton, char translateButton ) {
	zviewpointRotateButton = rotateButton;
	zviewpointTranslateButton = translateButton;
}

void ZViewpoint::zviewpointSetup2DView( float l, float t, float r, float b ) {
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

void ZViewpoint::zviewpointSetupView() {
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

	// The scene being viewed may not live at the origin.
	// So I want to translate to it, do my rotations,
	// translate back, do my scale, and then the client
	// draws his stuff.

	FVec3 transWithScale( zviewpointTransAxes.x * zviewpointScaleAxes.x,
						  zviewpointTransAxes.y * zviewpointScaleAxes.y,
						  zviewpointTransAxes.z * zviewpointScaleAxes.z );

	glTranslatef( transWithScale.x + (zviewpointPermitTransX ? zviewpointTrans[0] : 0.f),
				  transWithScale.y + (zviewpointPermitTransY ? zviewpointTrans[1] : 0.f),
				  transWithScale.z + (zviewpointPermitTransZ ? zviewpointTrans[2] : 0.f) );

	FMat4 rot = zviewpointRotQuat.mat();
	rot.transpose();
	glMultMatrixf( rot );

	glTranslatef( -( transWithScale.x + (zviewpointPermitTransX ? zviewpointTrans[0] : 0.f )),
				  -( transWithScale.y + (zviewpointPermitTransY ? zviewpointTrans[1] : 0.f )),
				  -( transWithScale.z + (zviewpointPermitTransZ ? zviewpointTrans[2] : 0.f )) );

	glScalef( zviewpointScale * zviewpointScaleAxes.x, 
			  zviewpointScale * zviewpointScaleAxes.y,
			  zviewpointScale * zviewpointScaleAxes.z );
}

void ZViewpoint::zviewpointDumpInCFormat( char *filename ) {
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

	// @TODO: do I want to dump the per-axes data-determined scale/trans?  The following
	// is the scale/trans as adjusted by the user via the mouse.
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

void ZViewpoint::zviewpointReset() {
	memset( &zviewpointTrans, 0, sizeof(zviewpointTrans) );
	zviewpointRotQuat.fromAxisAngle( FVec3::XAxis, 0.f );
	zviewpointScale = 1.f;
	zviewpointScaleAxes = FVec3( 1, 1, 1 );
	zviewpointTransAxes = FVec3::Origin;
	zviewpointTarget = FVec3::Origin;

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

void ZViewpoint::zviewpointRotateTrackball( float dx, float dy, float side ) {
	// ROTATE: this was originally in the message handler for trackball mode
	// reponse to mouse drag.  I factored out to here so that I can call this
	// code in response to arrow keys to step-rotate the object about a given 
	// axis. (tfb)

	// COMPUTE the world axises about which we are spinning (i.e. the screen axis)
	FMat4 ref( zviewpointReferenceModel.m );
	FMat4 rot = zviewpointRotQuat.mat();
	rot.transpose();
	ref.cat( rot );
	ref.setTrans( FVec3::Origin );
	ref.orthoNormalize();
	ref.inverse();
		// ref is now the transform that will take a point in eye-coordinates and
		// transform it to its original pre-viewing-transform world coordinates. tfb

	FVec3 xEye = ref.mul( FVec3::XAxis );
	FVec3 yEye = ref.mul( FVec3::YAxis );
	FVec3 zEye = ref.mul( FVec3::ZAxis );

	FQuat deltaX( xEye, dy );
	if( !zviewpointPermitRotX ) deltaX.identity();
	FQuat deltaY( yEye,  dx );
	if( !zviewpointPermitRotY ) deltaY.identity();
	FQuat deltaZ;
	if( side != 0 ) {
		deltaX.identity();
		deltaY.identity();
		deltaZ.fromAxisAngle( zEye, side * -dy );
	}
	if( !zviewpointPermitRotZ ) deltaZ.identity();

	// QUATERNION multiply the delta by the origial to get the new
	deltaX.mul( zviewpointRotQuat );
	deltaY.mul( deltaX );
	deltaZ.mul( deltaY );
	zviewpointRotQuat = deltaZ;
}

void ZViewpoint::zviewpointHandleMsgTrackball( ZMsg *msg ) {
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
	else if( zmsgIs( type, KeyDown ) ) {
		if( zmsgIs( which, down ) )  {
			zviewpointRotateTrackball( 0.f, -zviewpointRotateStep );
			zMsgUsed();
		}
		if( zmsgIs( which, up ) )  {
			zviewpointRotateTrackball( 0.f, +zviewpointRotateStep );
			zMsgUsed();
		}
		if( zmsgIs( which, left ) )  {
			zviewpointRotateTrackball( +zviewpointRotateStep, 0.f );
			zMsgUsed();
		}
		if( zmsgIs( which, right ) )  {
			zviewpointRotateTrackball( -zviewpointRotateStep, 0.f );
			zMsgUsed();
		}
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
			if( zviewpointRotating ) {
				float dx = +0.01f * ( viewpointMouseLast[0] - zmsgF(x) );
				float dy = -0.01f * ( viewpointMouseLast[1] - zmsgF(y) );
				float side = zmsgI( shift ) ? ( (zmsgF(localX) < zmsgF(w)/2) ? -1.f : 1.f ) : 0.f;
					// note: the side doesn't really work if the object is being rendered in a ZUI because the
					// w here refers to the screen, and not the ZUI.
				zviewpointRotateTrackball( dx, dy, side );
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

void ZViewpoint::zviewpointHandleMsgFly( ZMsg *msg ) {
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

void ZViewpoint::zviewpointHandleMsg( ZMsg *msg ) {
	if( zviewpointMode == ZVIEWPOINTMODE_TRACKBALL ) {
		zviewpointHandleMsgTrackball( msg );
		return;
	}
	if( zviewpointMode == ZVIEWPOINTMODE_FLY ) {
		zviewpointHandleMsgFly( msg );
		return;
	}
}

