// @ZBS {
//		*MODULE_OWNER_NAME zcovar
// }


// This code was adapted from Jon Blow's code and I think that it is a 
// little wozzly.  I think it would be preferable to make a set of simpler
// functions like:
// FVec2 vectorArrayMean( FVec2 *vectorArray, int count ) 
// FMat2 vectorArrayCovariance( FVec2 *vectorArray, int count, FVec2 mean ) 
// void eigen2f( FMat2 mat, FVec2 &vals, FMat2 &vecs ) 



#ifndef ZCOVAR_H
#define ZCOVAR_H

#include "zvec.h"

struct ZCovarMat2 {
    float a, b, c;

    void reset();

    ZCovarMat2 add( ZCovarMat2 &other );
    ZCovarMat2 inverse();

	void rotate( float theta );
    void scale( float factor );
	
    void moveToGlobalCoords( ZCovarMat2 *dest, float x, float y );
    void moveToLocalCoords( ZCovarMat2 *dest, float x, float y );

    int findEigenvectors( float eigenvalues[2], FVec2 eigenvectors[2] );
    int findEigenvalues( float eigenvalues[2] );
};

struct ZCovarBody2 {
    ZCovarMat2 covariance;
    FVec2 mean;
    float mass;
	FVec2 axis[2];
	float lambda[2];

	FVec2 sum;
	int count;
		// These two are used for accumulating the mean

	ZCovarBody2();
    void reset();
    void accumulate(float x, float y, float mass);
	void computeMean();
    void normalize();
	void scale( float s );
	void computeAxises();
	FVec2 getMaxAxis();
	FVec2 getMinAxis();
	float aspectRatio();
		// Always returns >= 1.0
};

int zCovarSolveQuadraticNonNegDet( float a, float b, float c, float solutions[2] );
	// Find the solutions, returned in solutions array.  Returns number of solutions (0,1,2)

ZCovarBody2 zCovarComputeBody( FVec2 *points, int numPoints );

#endif

