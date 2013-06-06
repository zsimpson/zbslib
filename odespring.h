#ifndef ODESPRING_H
#define ODESPRING_H

#include "ode/ode.h"
#include "zvec.h"

struct ODESpring {
	int type;
		#define ODESPRING_TYPE_HOOKE (1)
		#define ODESPRING_TYPE_INV_SQUARE (2)
		#define ODESPRING_TYPE_CONSTANT (3)
	int active;
		// True if this is a valid active spring
	int disabled;
		// True if this is a spring is temporarily disabled
	dBodyID dBody[2];
		// The bodies this spring is connected to
	FVec3 offset[2];
		// The offset in body coords of that connection
	float naturalLen;
		// The natural len of the spring
	float breakLen;
		// If this is > 0 then above this length, the spring will disable
	float formLen;
		// If this is > 0 then below this length the spring will enable
	float maximumForce;
		// If this is > 0 then no force greater than this will be applied
	float constant;
		// The control constant

	float *naturalLenPtr;
	float *breakLenPtr;
	float *formLenPtr;
	float *maximumForcePtr;
	float *constantPtr;
		// These are the same as above except as pointer.
		// If they are non zero then they will be used in place of the above

	ODESpring() {
		memset( this, 0, sizeof(*this) );
	}
};

void odeSpringsReset();
ODESpring *odeSpringCreate( dBodyID b0, dBodyID b1, FVec3 off0, FVec3 off1, int type=ODESPRING_TYPE_HOOKE, int computeNatLen=1 );
void odeSpringApplyForce( dBodyID b0, dBodyID b1, FVec3 off0, FVec3 off1, FVec3 forceVec, float damping );
void odeSpringUpdate( float damping = 1.f );
void odeSpringRender();

void odeBodyDamp( dBodyID dBody, float friction );


#endif