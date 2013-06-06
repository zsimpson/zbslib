// @ZBS {
//		*MODULE_NAME zcovar
//		+DESCRIPTION {
//			Implements a covariance body.
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zcovar.cpp zcovar.h
//		*OPTIONAL_FILES
//		*VERSION 1.0
//		+HISTORY {
//			1.0 Adapted from original code by Jon Blow
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "assert.h"
#include "math.h"
// MODULE includes:
#include "zcovar.h"
// ZBSLIB includes:

// ZCovarMat2
////////////////////////////////////////////////////////////////////////////

void ZCovarMat2::reset() {
	a = b = c = 0;
}

ZCovarMat2 ZCovarMat2::inverse() {
    float det = a*c - b*b;
    float factor = 1.0f / det;

    ZCovarMat2 result;
    result.a = c * factor;
    result.b = -b * factor;
    result.c = a * factor;

    return result;
}

ZCovarMat2 ZCovarMat2::add( ZCovarMat2 &other ) {
    ZCovarMat2 result;
    result.a = a + other.a;
    result.b = b + other.b;
    result.c = c + other.c;

    return result;
}

void ZCovarMat2::scale( float factor ) {
	a *= factor;
	b *= factor;
	c *= factor;
}

void ZCovarMat2::rotate( float theta ) {
	float s = sinf(theta);
	float t = cosf(theta);

	float a_prime = a*t*t + b*2*s*t + c*s*s;
	float b_prime = -a*s*t + b*(t*t - s*s) + c*s*t;
	float c_prime = a*s*s - b*2*s*t + c*t*t;

	a = a_prime;
	b = b_prime;
	c = c_prime;
}


int ZCovarMat2::findEigenvalues( float eigenvalues[2] ) {
    float qa, qb, qc;
    qa = 1;
    qb = -(a + c);
    qc = a * c - b * b;

	float solutions[2];
    int numSolutions = zCovarSolveQuadraticNonNegDet( qa, qb, qc, solutions );

    // If there's only one solution, explicitly state it as a
    // double eigenvalue.
    if( numSolutions == 1 ) {
        solutions[1] = solutions[0];
        numSolutions = 2;
    }

    eigenvalues[0] = solutions[0];
    eigenvalues[1] = solutions[1];
    return numSolutions;
}

int ZCovarMat2::findEigenvectors( float eigenvalues[2], FVec2 eigenvectors[2] ) {
    int num_eigenvalues = findEigenvalues(eigenvalues);
    assert(num_eigenvalues == 2);

    // Now that we have the quadratic coefficients, find the eigenvectors.
    const float VANISHING_EPSILON = 1.0e-5f;
    const float SAMENESS_LOW = 0.9999f;
    const float SAMENESS_HIGH = 1.0001f;

    bool punt = false;
    const float A_EPSILON = 0.0000001f;
    if( a < A_EPSILON ) {
        punt = true;
    }
	else {
        float ratio = (float)fabs(eigenvalues[1] / eigenvalues[0]);
        if ((ratio > SAMENESS_LOW) && (ratio < SAMENESS_HIGH)) punt = true;
    }

    if( punt ) {
        eigenvalues[0] = a;
        eigenvalues[1] = a;

        eigenvectors[0] = FVec2(1, 0);
        eigenvectors[1] = FVec2(0, 1);
        num_eigenvalues = 2;
        return num_eigenvalues;
    }

    int j;
    for( j = 0; j < num_eigenvalues; j++ ) {
        float lambda = eigenvalues[j];

        FVec2 result1, result2;
        result1 = FVec2(-b, a - lambda);
        result2 = FVec2(-(c - lambda), b);

        FVec2 result;
        if (result1.mag2() > result2.mag2()) {
            result = result1;
        } else {
            result = result2;
        }

        result.normalize();
        eigenvectors[j] = result;
    }

    return num_eigenvalues;
}

void ZCovarMat2::moveToGlobalCoords(ZCovarMat2 *dest, float x, float y) {
    dest->a = a + x*x;
    dest->b = b + x*y;
    dest->c = c + y*y;
}

void ZCovarMat2::moveToLocalCoords(ZCovarMat2 *dest, float x, float y) {
    dest->a = a - x*x;
    dest->b = b - x*y;
    dest->c = c - y*y;
}

// ZCovarBody2
////////////////////////////////////////////////////////////////////////////

ZCovarBody2::ZCovarBody2() {
	reset();
}

void ZCovarBody2::reset() {
    mass = 0;
    mean.x = 0;
    mean.y = 0;
    covariance.reset();
	sum.origin();
	count = 0;
}

void ZCovarBody2::accumulate(float x, float y, float pointMass) {
    mass += pointMass;

    float cx = x * pointMass;
    float cy = y * pointMass;
    covariance.a += cx * cx;
    covariance.b += cx * cy;
    covariance.c += cy * cy;

	sum.add( FVec2(x, y) );
	count++;
}

void ZCovarBody2::computeMean() {
	mean = sum;
	mean.div( (float)count );
}

void ZCovarBody2::normalize() {
    if (mass == 0) return;
    float imass = 1.0f / mass;
    covariance.scale(imass);
    mass = 1.0f;
}

void ZCovarBody2::scale( float s ) {
    covariance.scale(s);
    mass *= s;
}

void ZCovarBody2::computeAxises() {
	covariance.findEigenvectors( lambda, axis );
}

float ZCovarBody2::aspectRatio() {
	if( lambda[0] > lambda[1] ) {
		return (float)sqrt(lambda[0]) / (float)sqrt(lambda[1]);
	}
	return (float)sqrt(lambda[1]) / (float)sqrt(lambda[0]);
}

FVec2 ZCovarBody2::getMaxAxis() {
	if( lambda[0] > lambda[1] ) {
		return axis[0];
	}
	return axis[1];
}

FVec2 ZCovarBody2::getMinAxis() {
	if( lambda[0] < lambda[1] ) {
		return axis[0];
	}
	return axis[1];
}


// Free functions
////////////////////////////////////////////////////////////////////////////

ZCovarBody2 zCovarComputeBody( FVec2 *points, int numPoints ) {
	int i;

    // First start with an empty body.
	ZCovarBody2 body;
    body.reset();

	float sumX = 0.f, sumY = 0.f;

	FVec2 sum( 0, 0 );
	for( i=0; i<numPoints; i++ ) {
		sum.add( points[i] );
	}
    body.mean = sum;
	if( numPoints > 0 ) {
		body.mean.div( (float)numPoints );
	}

	for( i=0; i<numPoints; i++ ) {
        body.accumulate( points[i].x-body.mean.x, points[i].y-body.mean.y, 1.f );
	}

	return body;
}

int zCovarSolveQuadraticNonNegDet( float a, float b, float c, float solutions[2] ) {
    if( a == 0.f ) {
		// Then bx + c = 0; thus, x = -c / b
        if( b == 0.f ) {
            return 0;
        }

        solutions[0] = -c / b;
        return 1;
    }

    float discriminant = b * b - 4 * a * c;
    if( discriminant < 0.f ) {
		discriminant = 0.f;
	}

    float sign_b = 1.0f;
    if( b < 0.f ) {
		sign_b = -1.f;
	}

    int nroots = 0;
    float q = -0.5f * ( b + sign_b * sqrtf(discriminant) );
  
    nroots++;
    solutions[0] = q / a;

    if( q != 0.f ) {
        float solution = c / q;
        if( solution != solutions[0] ) {
            nroots++;
            solutions[1] = solution;
        }
    }

    return nroots;
}

