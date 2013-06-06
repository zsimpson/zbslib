
#ifndef ZVIEWPOINT_H
#define ZVIEWPOINT_H

#include "zvec.h"
#include "zmsg.h"

extern FVec3 zviewpointTrans;
extern FQuat zviewpointRotQuat;
extern float zviewpointScale;

void zviewpointSetMode( int mode=0 );
	#define ZVIEWPOINTMODE_TRACKBALL (0)
	#define ZVIEWPOINTMODE_FLY (1)

void zviewpointSetupView();
void zviewpointReset();

void zviewpointHandleMsg( ZMsg *msg );
//	Typical usage:
//	zviewpointHandleMsg( msg );
//	if( zMsgIsUsed() ) return;

void zviewpointSetup2DView( float l, float t, float r, float b );

void zviewpointSetPermitRot( int xOnOff, int yOnOff, int zOnOff );
void zviewpointSetPermitTrans( int xOnOff, int yOnOff, int zOnOff );
void zviewpointSetPermitScale( int onOff );

void zviewpointSetButtons( char rotateButton, char translateButton );
	// expects 'L', 'M', 'R' as are passed by mouse messages

FVec3 zviewpointProjOnPlane( int scnX, int scnY, FVec3 p0, FVec3 planeNrm );
	// Extracts the current matrix and projects the point onto the give plane

FVec3 zviewpointLinePlaneIntersect( FVec3 p0, FVec3 norm, FVec3 p1, FVec3 p2 );
FVec3 zviewpointProjMouseOnZ0Plane();
	// Extracts the current matrix and projects the mouse on to the z=0 plane

void zviewpointDumpInCFormat( char *filename );
	// Dump the current matrix to the filename

#endif



