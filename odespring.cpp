// @ZBS {
//		*MODULE_NAME odespring
//		+DESCRIPTION {
//			Springs for the ODE physics system
//		}
//		*PORTABILITY win32
//		*REQUIRED_FILES odespring.cpp odespring.h
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
#include "assert.h"
// MODULE includes:
#include "odespring.h"
// ZBSLIB includes:
#include "zmathtools.h"

#define ODESPRINGS_MAX (1024)
ODESpring odeSprings[ODESPRINGS_MAX];

void odeSpringsReset() {
	for( int i=0; i<ODESPRINGS_MAX; i++ ) {
		odeSprings[i].active = 0;
	}
}

ODESpring *odeSpringCreate( dBodyID b0, dBodyID b1, FVec3 off0, FVec3 off1, int type, int computeNatLen ) {
	for( int i=0; i<ODESPRINGS_MAX; i++ ) {
		if( ! odeSprings[i].active ) {
			odeSprings[i].type = type;
			odeSprings[i].active = 1;
			odeSprings[i].disabled = 0;
			odeSprings[i].dBody[0] = b0;
			odeSprings[i].dBody[1] = b1;
			odeSprings[i].offset[0] = off0;
			odeSprings[i].offset[1] = off1;
			odeSprings[i].naturalLen = 0.f;
			odeSprings[i].breakLen = 0.f;
			odeSprings[i].maximumForce = 0.f;
			odeSprings[i].constant = 1.f;
			odeSprings[i].naturalLenPtr = 0;
			odeSprings[i].breakLenPtr = 0;
			odeSprings[i].maximumForcePtr = 0;
			odeSprings[i].constantPtr = 0;

			if( computeNatLen ) {
				FVec3 p0 = odeSprings[i].offset[0];
				if( odeSprings[i].dBody[0] ) {
					dReal *pos0 = (dReal *)dBodyGetPosition( odeSprings[i].dBody[0] );
					dReal *rot0 = (dReal *)dBodyGetRotation( odeSprings[i].dBody[0] );
					FMat4  mat0 = zmathODERotPosToFMat4( rot0, pos0 );
					p0 = mat0.mul( odeSprings[i].offset[0] );
				}

				FVec3 p1 = odeSprings[i].offset[1];
				if( odeSprings[i].dBody[1] ) {
					dReal *pos1 = (dReal *)dBodyGetPosition( odeSprings[i].dBody[1] );
					dReal *rot1 = (dReal *)dBodyGetRotation( odeSprings[i].dBody[1] );
					FMat4  mat1 = zmathODERotPosToFMat4( rot1, pos1 );
					p1 = mat1.mul( odeSprings[i].offset[1] );
				}

				p0.sub( p1 );
				odeSprings[i].naturalLen = p0.mag();
			}

			return &odeSprings[i];
		}
	}
	assert( 0 );
	return 0;
}

void odeSpringApplyForce( dBodyID b0, dBodyID b1, FVec3 off0, FVec3 off1, FVec3 forceVec, float damping ) {
	dReal zero[3] = { 0.0, 0.0, 0.0 };
	const dReal *vel0 = (const dReal *)&zero;
	const dReal *vel1 = (const dReal *)&zero;

	if( b0 ) {
		vel0 = dBodyGetLinearVel( b0 );
	}
	if( b1 ) {
		vel1 = dBodyGetLinearVel( b1 );
	}
	FVec3 relVel( (float)vel0[0]-(float)vel1[0], (float)vel0[1]-(float)vel1[1], (float)vel0[2]-(float)vel1[2] );
	float a = relVel.dot( forceVec );
	float b = forceVec.dot( forceVec );
	FVec3 dampingVec = forceVec;
	if( fabsf(b) > 1e-5f ) {
		dampingVec.mul( -damping * a / b );
	}
	forceVec.add( dampingVec );
	if( b0 ) {
		dBodyAddForceAtRelPos( b0,  forceVec.x,  forceVec.y,  forceVec.z, off0.x, off0.y, off0.z );
	}
	if( b1 ) {
		dBodyAddForceAtRelPos( b1, -forceVec.x, -forceVec.y, -forceVec.z, off1.x, off1.y, off1.z );
	}
}

void odeSpringUpdate( float damping ) {
	// APPLY spring forces
	for( int i=0; i<ODESPRINGS_MAX; i++ ) {
		if( odeSprings[i].active ) {

			FVec3 p0;
			if( odeSprings[i].dBody[0] ) {
				dReal *pos0 = (dReal *)dBodyGetPosition( odeSprings[i].dBody[0] );
				dReal *rot0 = (dReal *)dBodyGetRotation( odeSprings[i].dBody[0] );
				FMat4  mat0 = zmathODERotPosToFMat4( rot0, pos0 );
				p0 = mat0.mul( odeSprings[i].offset[0] );
			}
			else {
				p0 = odeSprings[i].offset[0];
			}

			FVec3 p1;
			if( odeSprings[i].dBody[1] ) {
				dReal *pos1 = (dReal *)dBodyGetPosition( odeSprings[i].dBody[1] );
				dReal *rot1 = (dReal *)dBodyGetRotation( odeSprings[i].dBody[1] );
				FMat4  mat1 = zmathODERotPosToFMat4( rot1, pos1 );
				p1 = mat1.mul( odeSprings[i].offset[1] );
			}
			else {
				p1 = odeSprings[i].offset[1];
			}

			FVec3 springVec = p1;
			springVec.sub( p0 );
			float len = springVec.mag();

			float formLen = odeSprings[i].formLenPtr ? *odeSprings[i].formLenPtr : odeSprings[i].formLen;
			if( len < formLen && formLen > 0.f ) {
				odeSprings[i].disabled = 0;
			}

			if( len > 0.00001f && ! odeSprings[i].disabled ) {
				springVec.div( len );

				float naturalLen = odeSprings[i].naturalLenPtr ? *odeSprings[i].naturalLenPtr : odeSprings[i].naturalLen;
				float breakLen = odeSprings[i].breakLenPtr ? *odeSprings[i].breakLenPtr : odeSprings[i].breakLen;
				float maximumForce = odeSprings[i].maximumForcePtr ? *odeSprings[i].maximumForcePtr : odeSprings[i].maximumForce;
				float constant = odeSprings[i].constantPtr ? *odeSprings[i].constantPtr : odeSprings[i].constant;

				if( len > breakLen && breakLen > 0.f ) {
					odeSprings[i].disabled = 1;
					continue;
				}

				len -= naturalLen;
				switch( odeSprings[i].type ) {
					case ODESPRING_TYPE_HOOKE:
						springVec.mul( constant * len );
						break;
					case ODESPRING_TYPE_INV_SQUARE:
						springVec.mul( constant / (len*len) );
						break;
					case ODESPRING_TYPE_CONSTANT:
						springVec.mul( constant );
						break;
				}

				float l = springVec.mag();
				if( maximumForce > 0.f && l > maximumForce ) {
					springVec.div( l );
					springVec.mul( maximumForce );
				}

				odeSpringApplyForce( odeSprings[i].dBody[0], odeSprings[i].dBody[1], odeSprings[i].offset[0], odeSprings[i].offset[1], springVec, damping );
			}
		}
	}
}

void odeSpringRender() {
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glPointSize( 4.f );
	for( int i=0; i<ODESPRINGS_MAX; i++ ) {
		if( odeSprings[i].active ) {
			if( odeSprings[i].disabled ) {
				continue;
				glColor4ub( 120, 120, 120, 128 );
			}
			else {
				glColor4ub( 120, 120, 230, 255 );
			}

			FVec3 p0;
			if( odeSprings[i].dBody[0] ) {
				dReal *pos0 = (dReal *)dBodyGetPosition( odeSprings[i].dBody[0] );
				dReal *rot0 = (dReal *)dBodyGetRotation( odeSprings[i].dBody[0] );
				FMat4  mat0 = zmathODERotPosToFMat4( rot0, pos0 );
				p0 = mat0.mul( odeSprings[i].offset[0] );
			}
			else {
				p0 = odeSprings[i].offset[0];
			}

			FVec3 p1;
			if( odeSprings[i].dBody[1] ) {
				dReal *pos1 = (dReal *)dBodyGetPosition( odeSprings[i].dBody[1] );
				dReal *rot1 = (dReal *)dBodyGetRotation( odeSprings[i].dBody[1] );
				FMat4  mat1 = zmathODERotPosToFMat4( rot1, pos1 );
				p1 = mat1.mul( odeSprings[i].offset[1] );
			}
			else {
				p1 = odeSprings[i].offset[1];
			}

			glBegin( GL_LINES );
				glVertex3fv( p0 );
				glVertex3fv( p1 );
			glEnd();

			glBegin( GL_POINTS );
				FVec3 hack = p1;
				hack.sub( p0 );
				hack.normalize();
				hack.mul( odeSprings[i].naturalLen );
				hack.add( p0 );
				glVertex3fv( hack );
			glEnd();

		}
	}
}

void odeBodyDamp( dBodyID dBody, float friction ) {
	dReal *lv = (dReal *)dBodyGetLinearVel ( dBody );

	dReal lv1[3];
	lv1[0] = lv[0] * -friction;
	lv1[1] = lv[1] * -friction;
	lv1[2] = lv[2] * -friction;
	dBodyAddForce( dBody, lv1[0], lv1[1], lv1[2] );

	dReal *av = (dReal *)dBodyGetAngularVel( dBody );
	dReal av1[3];
	av1[0] = av[0] * -friction;
	av1[1] = av[1] * -friction;
	av1[2] = av[2] * -friction;
	dBodyAddTorque( dBody, av1[0], av1[1], av1[2] );
}
