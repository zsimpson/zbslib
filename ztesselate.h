// @ZBS {
//		*MODULE_OWNER_NAME ztesselate
// }

#ifndef ZTESSELATE_H
#define ZTESSELATE_H

#include "zvec.h"

int zTesselatePointInPoly( int npol, FVec2 *points, FVec2 point );
float zTesselatePlanarPoly( FVec3 *verts, int count, FVec3 normal, int *tris, int &triCount );
	// Given a 2D planar set of verts and a normal to the plane,
	// this gives you back a triangle list and triangle count
	// returns area
float zTesselatePlanarPoly( FVec2 *verts, int count, int *tris, int &triCount );
	// Same for 2D

#endif