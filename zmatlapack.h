#ifndef ZMATLAPACK_H
#define ZMATLAPACK_H

#include "zmat.h"

void zmatEigenDecomposition( ZMat &mat, ZMat &resVecs, ZMat &resVals );
	// Decompose mat into eigen vectors (resVecs) and values a 1xN matrix

#endif