#ifndef ZMATCLAPACK_H
#define ZMATCLAPACK_H

#include "zmat.h"

void zmatSVD_CLAPACK( ZMat &inputMat, ZMat &uMat, ZMat &sVec, ZMat &vMat );

int zmatSolve_CLAPACK( ZMat &inputMat, ZMat &inputVec, ZMat &solutionVec );
	// Returns 1 on success, 0 on singular

void zmatEigen_CLAPACK( ZMat &input, ZMat &eigVecs, ZMat &eigVals );

#endif
