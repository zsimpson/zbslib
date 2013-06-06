// @ZBS {
//		*MODULE_OWNER_NAME zmat_matlab
// }

#include "zmat.h"

#ifndef ZMAT_MATLAB_H
#define ZMAT_MATLAB_H

int zmatLoad_Matlab( ZMat &loadInto, char *filename, char *varName=0 );
int zmatSave_Matlab( ZMat &saveFrom, char *filename, char *varName );

#endif

