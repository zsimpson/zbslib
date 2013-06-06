// @ZBS {
//		*MODULE_OWNER_NAME zbird
// }

#ifndef ZBIRD2_H
#define ZBIRD2_H

void zbirdInit( int port, int numUnits, int transmitterAddress );

void zbirdClose();

void zbirdMode( int mode );
	#define zbirdMODE_POS (0)
	#define zbirdMODE_POSANGLES (1)
	#define zbirdMODE_POSMATRIX (2)
	#define zbirdMODE_POSQUATERNION (3)
	#define zbirdMODE_QUATERNION (4)
	#define zbirdMODE_MATRIX (5)
	#define zbirdMODE_ANGLES (6)

void zbirdSamplePos( float pos[][3] );

void zbirdSamplePosQuaternion( float pos[][3], float quat[][4] );

void zbirdHemisphere( int which );
	#define zbirdHEMI_FORWARD 0
	#define zbirdHEMI_REAR 1
	#define zbirdHEMI_UPPER 2
	#define zbirdHEMI_LOWER 3
	#define zbirdHEMI_LEFT 4
	#define zbirdHEMI_RIGHT 5

#endif

