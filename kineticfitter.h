// @ZBS {
//		*MODULE_OWNER_NAME kineticfitter
// }

#ifndef KINETICFITTER_H
#define KINETICFITTER_H

// OPERATING SYSTEM specific includes:
// SDK includes:
#include "pthread.h"
// STDLIB includes:
// MODULE includes:
#include "kineticbase.h"
// ZBSLIB includes:
#include "zmat.h"


// KineticFitter
//------------------------------------------------------------------------------------------

struct KineticFitter : public KineticSystem {

	// Thread management
	//------------------------------------------------------------
	pthread_t fitThreadID;
	int fitThreadMonitorDeath;
		// If set, the fit thread will monitor the fitThreadDeathFlag and kill itself if set
	int fitThreadMonitorStep;
		// If set, the fit thread will monitor the fitThreadStepFlag and only 
		// proceed once through the main loop per setting of this flag
	int fitThreadDeathFlag;
	int fitThreadStepFlag;
	int fitThreadSimulateEachStep;
		// Set this if you want to watch the fitting process
	int fitThreadLoopCount;
		// Incremented each pass of the fitter so progress can be plotted
	int fitThreadStatus;
		#define KE_UNSTARTED (0)
		#define KE_RUNNING (1)
		#define KE_DONE_CONVERGED (2)
		#define KE_DONE_TRAPPED (3)
		#define KE_DONE_CANCELLED (4)
		#define KE_DONE_SINGLE_STEP (5)
		#define KE_PAUSED (6)

	char *fitThreadSendMsg;
		// This message will be sent on each loop to update the status
		// Be sure to use a mutex protected zMsgQueue!
		
	pthread_mutex_t renderVariablesMutex;
	void renderVariablesLock();
	void renderVariablesUnlock();
		// These mutex the copies of all the viewable traces "render_traceXXX" in all the experiments

	// Origin: These are all the variables that relate to the last accepted position.
	//------------------------------------------------------------ 
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

	// Proposal: These are all the variables that relate to the currently propsed position
	//------------------------------------------------------------ 
	double propE;
	ZMat propQ;
	ZMat propEigenRejectFlags;
	int propOffTheRails;
	int propBackoffStepsOnLastEigenValue;
	int propCountSinceOrig;
	int origLastHistoryIndex;

	// Intergator and VM
	//------------------------------------------------------------ 
	int totalObsCount;
	KineticVMCodeD *vmd;
	KineticVMCodeH *vmh;
	KineticIntegrator *integrator;
	ZMat minStepSizeC;
	ZMat minStepSizeG;
		// These track the minimum steps that's ever been seen for each experiment
		// one for the D system and one for the H system

	// Timing
	//------------------------------------------------------------ 
	double propEvalAccum;
	double origNewAccum;

	// Workspace
	//------------------------------------------------------------ 
	double *slerpC;
	double *slerpG;
	double *icG;
	double *dC_dt;
	double *OG;
	double *equallySpacedResidual;
	
	int qBeingFitCount;
	int origCount;
	int propCount;
	ZMat qHistory;
	ZMat eHistory;
		// A row vector of the e val at each prop
	ZMat stateHistory;
		// A row vector 0 if it was a normal prop or 1 if it was accepted as an origin

	double getSNRForRender( int q );

	int propNew();
		#define PN_CONVERGED (1)
		#define PN_TRAPPED (2)
		#define PN_PROPOSE (3)
		#define PN_ACCEPT (4)
		// Contains the logic for the step manager to make new proposals
		
	void propEval();
		// Evaluates the system at the proposal by integrating D to get C

	void origNew();
		// Creates a new origin from which place new proposals will be made.
		// This is a heavy weight operation that integrates H, does FFT and SVD

	void fit( int singleStep=0 );
	void clear();

	void setSendMsg( char *msg );

	static void *threadMain( void *args );
	void threadStart();

	KineticFitter( KineticSystem &systemToCopy );
	~KineticFitter();
};


#endif


