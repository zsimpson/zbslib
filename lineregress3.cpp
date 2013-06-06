#include "vec3.h"
#include "f2clibs/f2c.h"
#include "clapack.h"
#include "assert.h"


int linearRegression3f( FVec3 *samples, int sampleCount, FVec3 &p, FVec3 &d, float &r ) {
	// CALCULATE all the partial sums
	FVec3 *s = samples;
	int n = sampleCount;
	float sx = 0.0f;
	float sy = 0.0f;
	float sz = 0.0f;
	float sx2 = 0.0f;
	float sy2 = 0.0f;
	float sz2 = 0.0f;
	float sxy = 0.0f;
	float sxz = 0.0f;
	float syz = 0.0f;

	for( int i=0; i<n; i++ ) {
		sx += s[i].x;
		sy += s[i].y;
		sz += s[i].z;
		sx2 += s[i].x * s[i].x;
		sy2 += s[i].y * s[i].y;
		sz2 += s[i].z * s[i].z;
		sxy += s[i].x * s[i].y;
		sxz += s[i].x * s[i].z;
		syz += s[i].y * s[i].z;
	}

	// MAKE a guess at the values of p and d.
	FVec3 avgP;
	for( int j=0; j<sampleCount; j++ ) {
		avgP.add( samples[j] );
	}
	avgP.div( (float)sampleCount );
	FVec3 avgPositive, avgNegative;
	int avgPositiveCnt=0, avgNegativeCnt=0;
	for( j=0; j<sampleCount; j++ ) {
		FVec3 a = samples[j];
		a.sub( avgP );
		float dot = a.dot( avgP );
		if( dot < 0.f ) {
			avgNegative.add( a );
			avgNegativeCnt++;
		}
		else {
			avgPositive.add( a );
			avgPositiveCnt++;
		}
	}
	if( avgPositiveCnt <= 0 ) {
		return 0;
	}
	if( avgNegativeCnt <= 0 ) {
		return 0;
	}
	avgPositive.div( avgPositiveCnt );
	avgNegative.div( avgNegativeCnt );

	avgNegative.mul( -1.f );
	avgPositive.add( avgNegative );
	avgPositive.normalize();

	FVec3 pp = avgPositive;
	float c = - avgP.dot( pp );
	pp.mul( c );
	pp.add( avgP );

	p = pp;
	d = avgPositive;

	// ITERATE the solution finder by:
	// - plug in the current guess at p and d into the objective function
	// - evaluate the constraint equations and the new jacobian at this place
	// - linsolve the jacobian Ax = B where B is the vector of constraint equations
	// @TODO: Need to make this more intelligent about solving.  Now just blindly does 10 interations
	for( i=0; i<10; i++ ) {
		// CALCULATE the current value of the objective function
		// @TODO: this will be used to stop the iteration
		float dx2 = d.x * d.x;
		float dx3 = dx2 * d.x;
		float dx4 = dx2 * dx2;
		float dy2 = d.y * d.y;
		float dy3 = dy2 * d.y;
		float dy4 = dy2 * dy2;
		float dz2 = d.z * d.z;
		float dz3 = dz2 * d.z;
		float dz4 = dz2 * dz2;
		float sum = 
			dx4*sx2               -2.0*dx2*sx2         +sx2                  -2.0*p.x*sx         -2.0*p.y*sy
			-2.0*dy2*sy2          +dy4*sy2             +sy2                  -2.0*p.z*sz         -2.0*dz2*sz2
			+dz4*sz2              +2.0*dz2*d.x*sxy*d.y +sz2                  +2.0*dx2*sx*p.x     +dx2*dz2*sz2
			+dx2*dy2*sy2          +2.0*dz2*sz*p.z      +dz2*dx2*sx2          +2.0*dy2*sy*p.y     +dy2*dx2*sx2
			+2.0*dx3*sxy*d.y      +2.0*dx3*sxz*d.z     +2.0*d.x*d.y*sy*p.x   -4.0*d.x*d.y*sxy    +2.0*d.x*d.z*sz*p.x
			-4.0*d.x*d.z*sxz      +2.0*dy3*d.x*sxy     +2.0*dy3*syz*d.z      +2.0*d.y*d.x*sx*p.y +2.0*d.y*d.z*sz*p.y
			-4.0*d.y*d.z*syz      +2.0*dz3*d.x*sxz     +2.0*dz3*d.y*syz      +2.0*d.z*d.x*sx*p.z +2.0*d.z*d.y*sy*p.z
			+2.0*dy2*d.x*d.z*sxz  +dy2*dz2*sz2         +2.0*dx2*d.y*d.z*syz  +dz2*dy2*sy2
		;
		r = (float)n * p.z*p.z + (float)n*p.x*p.x + (float)n * p.y*p.y + sum;



		float constraintValues[6];
		float jacobianValues[6][6];


		constraintValues[0] = d.x*d.x + d.y*d.y + d.z*d.z - 1.0;
		constraintValues[1] = d.x*p.x + d.y*p.y + d.z*p.z;
		constraintValues[2] = 2.0 * (
			dz2*d.x*sx2+dy2*d.x*sx2+2*dx3*sx2+2*d.x*sx*p.x+d.x*dz2*sz2-2*d.x*sx2+d.x*dy2*sy2
			+3*dx2*d.y*sxy+3*dx2*d.z*sxz+d.y*sy*p.x-2*d.y*sxy+d.z*sz*p.x-2*d.z*sxz+dy3*sxy
			+d.y*sx*p.y+dz3*sxz+d.z*sx*p.z+dy2*d.z*sxz+2*d.x*d.y*syz*d.z+dz2*d.y*sxy
		);
		constraintValues[3] = 2.0 * (
			d.y*dx2*sx2+2*dy3*sy2+2*d.y*sy*p.y-2*d.y*sy2+dx2*d.y*sy2+dx3*sxy+d.x*sy*p.x
			-2*d.x*sxy+3*dy2*d.x*sxy+3*dy2*d.z*syz+d.x*sx*p.y+d.z*sz*p.y-2*d.z*syz+dz3*syz
			+d.z*sy*p.z+2*d.y*d.x*d.z*sxz+d.y*dz2*sz2+dx2*d.z*syz+dz2*d.x*sxy+dz2*d.y*sy2
		);
		constraintValues[4] = 2*n*p.x+2*(dx2*sx-sx+d.x*d.y*sy+d.x*d.z*sz);
		constraintValues[5] = 2*n*p.y+2*(dy2*sy-sy+d.x*sx*d.y+d.y*d.z*sz);

		jacobianValues[0][0] = 2.0 * d.x;
		jacobianValues[0][1] = p.x;
		jacobianValues[0][2] = 2*(dz2*sx2 + dy2*sx2 + 6*dx2*sx2 + 2*p.x*sx + dz2*sz2 - 2*sx2 + dy2*sy2 + 6*d.x*d.y*sxy + 6*d.x*d.z*sxz + 2*d.y*d.z*syz );
		jacobianValues[0][3] = 2*(2*d.y*d.x*sx2 + 2*d.x*d.y*sy2 + 3*dx2*sxy + sy*p.x - 2*sxy + 3*dy2*sxy + sx*p.y + 2*d.y*sxz*d.z + 2*d.x*syz*d.z + dz2*sxy );
		jacobianValues[0][4] = 2*(2*d.x*sx+d.y*sy+d.z*sz);
		jacobianValues[0][5] = 2*(d.y*sx);
		jacobianValues[1][0] = 2.0 * d.y;
		jacobianValues[1][1] = p.y;
		jacobianValues[1][2] = 2*(2*d.y*d.x*sx2 + 2*d.x*d.y*sy2 + 3*dx2*sxy + sy*p.x - 2*sxy + 3*dy2*sxy + sx*p.y + 2*d.y*sxz*d.z + 2*d.x*syz*d.z + dz2*sxy );
		jacobianValues[1][3] = 2*(dx2*sx2 + 6*dy2*sy2 + 2*p.y*sy - 2*sy2 + dx2*sy2 + 6*d.x*d.y*sxy + 6*d.y*d.z*syz + 2*d.x*d.z*sxz + dz2*sz2 + dz2*sy2);
		jacobianValues[1][4] = 2*(d.x*sy);
		jacobianValues[1][5] = 2*(2*d.y*sy+d.x*sx+d.z*sz);
		jacobianValues[2][0] = 2.0 * d.z;
		jacobianValues[2][1] = p.z;
		jacobianValues[2][2] = 2*(2*d.z*d.x*sx2 + 2*d.x*d.z*sz2 + 3*dx2*sxz + sz*p.x - 2*sxz + 3*dz2*sxz + sx*p.z + dy2*sxz + 2*d.x*d.y*syz + 2*d.z*sxy*d.y );
		jacobianValues[2][3] = 2*(3*syz*dy2 + sz*p.y - 2*syz + 3*dz2*syz + sy*p.z + 2*sxz*d.x*d.y + 2*d.y*d.z*sz2 + syz*dx2 + 2*d.z*d.x*sxy + 2*d.z*d.y*sy2 );
		jacobianValues[2][4] = 2*(sz*d.x);
		jacobianValues[2][5] = 2*(d.y*sz);
		jacobianValues[3][0] = 0;
		jacobianValues[3][1] = d.x;
		jacobianValues[3][2] = 2*(2*d.x*sx+d.y*sy+d.z*sz);
		jacobianValues[3][3] = 2*(d.x*sy);
		jacobianValues[3][4] = 2 * n;
		jacobianValues[3][5] = 0;
		jacobianValues[4][0] = 0;
		jacobianValues[4][1] = d.y;
		jacobianValues[4][2] = 2*(d.y*sx);
		jacobianValues[4][3] = 2*(2*d.y*sy+d.x*sx+d.z*sz);
		jacobianValues[4][4] = 0;
		jacobianValues[4][5] = 2 * n;
		jacobianValues[5][0] = 0;
		jacobianValues[5][1] = d.z;
		jacobianValues[5][2] = 2*(d.z*sx);
		jacobianValues[5][3] = 2*(d.z*sy);
		jacobianValues[5][4] = 0;
		jacobianValues[5][5] = 0;

		// LINSOLVE the jacobian
		long c1 = 6;
		long c2 = 1;
		long pivot[6];
		long ok;
		sgesv_( &c1, &c2, (float *)jacobianValues, &c1, pivot, constraintValues, &c1, &ok );

		// constraintValues now holds the increment in the working values of d and p
		d.x -= constraintValues[0];
		d.y -= constraintValues[1];
		d.z -= constraintValues[2];
		p.x -= constraintValues[3];
		p.y -= constraintValues[4];
		p.z -= constraintValues[5];
	}
	return 1;
}
