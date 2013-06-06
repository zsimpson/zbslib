
#ifndef ZVIEWPOINT2_H
#define ZVIEWPOINT2_H

#include "zvec.h"
#include "zmsg.h"

struct ZViewpoint {
	FVec3 zviewpointTarget;
	FVec3 zviewpointTransAxes;
	FVec3 zviewpointTrans;
	FQuat zviewpointRotQuat;
	FVec3 zviewpointScaleAxes;
	float zviewpointScale;
		// I'd added the TransAxes and ScaleAxes to allow a static translation and scale to
		// be applied external to the user-controlled scale/translate applied via the mouse.
		// This is to allow an initial scale/trans to be applied per dimension for the purpose
		// of viewing 3d data.  Should this be applied in the plotter instead of in the viewpoint?
		// I think perhaps the viewpoint will want to know about these quantities and so to start
		// I have placed them in here.  Perhaps move them out to ZUIPlot3d.

	DMat4 zviewpointReferenceModel;
	DMat4 zviewpointReferenceProj;
	int zviewpointReferenceViewport[4];

	int zviewpointPermitRotX;
	int zviewpointPermitRotY;
	int zviewpointPermitRotZ;
	int zviewpointPermitTransX;
	int zviewpointPermitTransY;
	int zviewpointPermitTransZ;
	int zviewpointPermitScale;

	int zviewpointRotateButton;
	int zviewpointTranslateButton;

	float zviewpointRotateStep;
		// used for stepped rotatations via arrow keys

	int zviewpointRotating;
	int zviewpointTranslating;

	float viewpointMouseLast[2];
	int dragging;

	int zviewpointMode;
		#define ZVIEWPOINTMODE_TRACKBALL (0)
		#define ZVIEWPOINTMODE_FLY (1)

	ZViewpoint() {
		zviewpointRotateButton    = 'M';
		zviewpointTranslateButton = 'R';
		zviewpointRotateStep =  float( 3.14159265358979 / 20.0 );
		zviewpointRotating = 0;
		zviewpointTranslating = 0;
		dragging = 0;
		zviewpointMode = ZVIEWPOINTMODE_TRACKBALL;
		zviewpointReset();
	}

	void zviewpointSetMode( int mode ) {
		zviewpointMode = mode;
	}

	FVec2 zviewpointProjOnPlane( int scnX, int scnY, FVec3 p0, FVec3 planeNrm );
		// Extracts the current matrix and projects the point onto the give plane

	FVec2 zviewpointProjMouseOnZ0Plane( );
		// Extracts the current matrix and projects the mouse on to the z=0 plane

	FVec3 zviewpointLinePlaneIntersect( FVec3 p0, FVec3 norm, FVec3 p1, FVec3 p2 );
	void zviewpointSetPermitRot( int xOnOff, int yOnOff, int zOnOff );
	void zviewpointSetPermitTrans( int xOnOff, int yOnOff, int zOnOff );
	void zviewpointSetPermitScale( int onOff );
	void zviewpointSetButtons( char rotateButton, char translateButton );
		// expects 'L', 'M', 'R' as are passed by mouse messages

	void zviewpointSetup2DView( float l, float t, float r, float b );
	void zviewpointSetupView();
	void zviewpointDumpInCFormat( char *filename );
		// Dump the current matrix to the filename

	void zviewpointReset();
	void zviewpointRotateTrackball( float dx, float dy, float side=0.f );
	void zviewpointHandleMsgTrackball( ZMsg *msg );
	void zviewpointHandleMsgFly( ZMsg *msg );
	void zviewpointHandleMsg( ZMsg *msg );
		//	Typical usage:
		//	zviewpointHandleMsg( msg );
		//	if( zMsgIsUsed() ) return;

};

#endif



