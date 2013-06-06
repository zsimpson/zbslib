// @ZBS {
//		*MODULE_OWNER_NAME zfitterkin
// }

#ifndef ZFITTERKIN_H
#define ZFITTERKIN_H


#include "zfitter.h"
#include "kineticbase.h"

struct ZFitterKin : ZFitter {
	KineticSystem system;

	// Integrator, etc
	//------------------------------------------------------------ 
	int totalObsCount;
	KineticVMCodeD *vmd;
	KineticVMCodeH *vmh;
	KineticIntegrator *integrator;
	ZMat minStepSizeC;
	ZMat minStepSizeG;

	// Workspace
	//------------------------------------------------------------ 
	double *slerpC;
	double *slerpG;
	double *icG;
	double *dC_dt;
	double *OG;
	double *equallySpacedResidual;
	
	ZFitterKin();

	virtual void start( int qCount, double *qStart, int *qFitFlags );
		// Do not call this, use the following start.  This stub just asserts(0)
	void start( KineticSystem &_system );
	virtual void stop();
	virtual int step();

	virtual void copy( ZFitter *srcFitter );
	virtual void clear();

	virtual void propEval( ZMat &in_q, double &out_sse, int &out_offTheRails );
	virtual void origEval( ZMat &in_q, ZMat &out_origGrad, ZMat &out_origHess, ZMat &out_origCovr );
};

#endif


