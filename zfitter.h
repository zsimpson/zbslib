// @ZBS {
//		*MODULE_OWNER_NAME zfitter
// }

#ifndef ZFITTER_H
#define ZFITTER_H

// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
// MODULE includes:
// ZBSLIB includes:
#include "zmat.h"

struct ZFitter {
	int state;
		#define ZF_STOPPED        (0)
		#define ZF_DONE_CONVERGED (1)
		#define ZF_DONE_TRAPPED   (2)
		#define ZF_RUNNING        (3)
		
	ZMat fitFlags;

	double origE;
	ZMat origQ;
	ZMat origVtt;
	ZMat origSignal;
	ZMat origNoise;
	ZMat origSNR;
	ZMat origGrad;
	ZMat origHess;
	ZMat origCovr;
	ZMat origU;
	ZMat origUt;
	ZMat origS;
	ZMat origVt;
	ZMat origUtCovr;
	ZMat origCovrInEigenSpace;
	ZMat origGradInEigenSpace;
	ZMat origGradInEigenSpaceScaledByS;
	
	double propE;
	ZMat propQ;
	ZMat propEigenRejectFlags;
	int propOffTheRails;
	int propBackoffStepsOnLastEigenValue;
	int propCountSinceOrig;
	int origLastHistoryIndex;
	
	int qBeingFitCount;

	ZMat stateHistory;
	ZMat qHistory;
	ZMat eHistory;
		// A row vector of the e val at each prop
	int propCount();
	int origCount();

	double timingPropEvalAccum;
	double timingOrigNewAccum;

	double getSNRForRender( int q );


	// EXAMPLE USAGE:
	//
	// ZFitterSubclass fit;
	//     // A concrete sub-class of ZFitter is instanciated
	// fit.start( qCount, startQ, fitFlags );
	//     // Fitter is started
	// while( fit.step() == ZF_STATE_RUNNING );
	//     // Run it until converge or trapped
	// fit.stop();


	// ALTERNATIVELY, USING THE THREADS
	//
	// ZFitterSubclass renderFit;
	//     // The read-only copy used by the renderer
	//
	// threadMain() {
	//     ZMat startQ;
	//     ZFitterSubclass fit;
	//     fit.start( qCount, startQ, fitFlags );
	//     while( fit.step() == ZF_STATE_RUNNING ) {
	//         renderMutexLock();
	//         renderFit.copy( fit );
	//         renderMutexUnlock();
	//         // The renderFit is now safe place to be rendered by the rendering thread
	//         // without fear that the fitting thread is going to manipulate something during render
	//     }
	//     fit.stop();
	// }

	virtual void start( int qCount, double *qStart, int *qFitFlags );
	virtual void stop();
	virtual int step();
		// Runs one step of the fitter.  Returns the "state" defined above
	
	virtual void copy( ZFitter *srcFitter );
	virtual void clear();

	int propNew();
		// This function is independent of any specific model.
		// It uses the origX variables and the results from the last propEval
		// to make a new proposal. Returns one of the following
		#define PN_CONVERGED (1)
		#define PN_TRAPPED (2)
		#define PN_PROPOSE (3)
		#define PN_ACCEPT (4)

	void origNew();
		// This funciton is independent of any specific model.
		// It uses the current proposition propQ to make a new origin
		// It calls the virtual origEval to get the gradient, hessian, and covariance
		// and then does SVD on the hessian which is then used by the propNew
		// to make new propositions

	virtual void propEval( ZMat &in_q, double &out_sse, int &out_offTheRails ) = 0;
		// Given the parameter vector in_q, this virtual is expected to evaluate
		// the error function returning the SSE in out_sse and setting out_offTheRails
		// if the system has gone into an error state.

	virtual void origEval( ZMat &in_q, ZMat &out_origGrad, ZMat &out_origHess, ZMat &out_origCovr ) = 0;
		// Given the parameter vector in_q, this virtual fills in the gradient, hessian, and covaraiance
		
	ZFitter();
		
};

#endif


