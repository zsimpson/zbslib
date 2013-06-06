#ifndef ZMAT_GSL_H
#define ZMAT_GSL_H

#include "zmat.h"

void zmatMUL_GSL( ZMat &aMat, ZMat &bMat, ZMat &cVec );
	// C = A * B

void zmatSVD_GSL( ZMat &inputMat, ZMat &uMat, ZMat &sVec, ZMat &vtMat );

#endif
