// @ZBS {
//		*MODULE_NAME KineticFitter
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A chemical kinetics simulator, fitter system for fitting to parameters
//		}
//		*PORTABILITY win32 unix maxosx
//		*REQUIRED_FILES kineticfitter.cpp kineticfitter.h
//		*VERSION 1.0
//		+HISTORY {
//			1.0 ZBS Jul 2008, based on earlier prototypes
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "string.h"
#include "math.h"
#include "float.h"
#include "stdlib.h"
#ifdef WIN32
#include "malloc.h"
#endif
// MODULE includes:
#include "kineticfitter.h"
// ZBSLIB includes:
#include "zmsg.h"
#include "ztime.h"
#include "fftgsl.h"
#include "zmat_gsl.h"


/*
	NAMING CONVENTIONS
	-----------------------------------------------------------------------
	Q:     the global vector of parameters
	P:     the local-to-an-experiment vector of parameters. (P does not contain ICs for foreign experiments)
	C:     the concentration of the reagents
	D:	   the time derivatives of C (this is integrated to derive C)
	OC:    the observable values (some arbitrary function of C's)
	G[q]:  the derivative of C with respect to (wrt) parameter q
	H[q]:  the time derivative G[q] (this is integrated to derive G[q])
	OG[q]: the derivative of OC with respect to (wrt) parameter q
	prop*: variable which are part of a "proposition" to be evaluated.
	orig*: variable related to the last accepted point which is the "origin" of proceeding proposals
	origQ: the position in parameter space of last acceptance. The "origin" of proceeding proposals
	propQ: the position in parameter space of the current proposal to be evaluated
	origE: the value of the error function at the current origin
	propE: the value of the error function of the last proposal
	trace*:  A time trace of the given variable.  Each trace includes a time basis and derivatives for SLERPing
	
	SUBTLE ISSUES
	-----------------------------------------------------------------------
	Q vs P.
		The global parameters of the system that are being fit are called "q".
		The q parameters are ordered as:
			reaction rates
			observable constants (shared over all experiments)
			initial conditions for experiment 0
			initial conditions for experiment 1
			etc. for each experiment
		The "p" parameters are the parameters for any ONE given experiment.  The are ordered as:
			reaction rates, 
			observable constants (shared over all experiments)
			initial conditions for THAT experiment
		Thus, the p vector will be shorter than the q vector if there is more than one experiment
	
	Gradient, Hessian, and Co-variance
		* The gradient is the slope of the error surface. It describes how fast the
		  error function is changing wrt each direction in parameter space.
		* The hessian is the 2nd deriv of the error surface.  Each column vector
		  of the hessian is the rate of change of the gradient wrt to each direction in parameter space.
		* The co-variance describes the correlation of the parameters.
		  For a given observation, a large value in this matrix would indicate strong correlation between the two parameters.
		  In other words, if you moved one of these co-varying parameters, in order to hold the fit in place 
		  you'd also have to move the other.  A near-zero value, on the other hand, would indicate that movement
		  along one parameter would move the fit independent of the other parameter.
		  A large negative value would indicate that the two co-varying parameters have to move in opposite 
		  directions to continue to fit the data.
		  
	Zeroing
		Note that some parameters are either not fit or are not relevant to some given experiments 
		and therefore those rows/cols of the gradient, hessian, and co-variance are zeroed out.
		
	Stacking of the gradient, hessian, and co-variance
		For a given experiment, an element in the gradient vector is the rate of change of the error of an observable wrt a parameter.
		Therefore the size of the gradient vector for a single experiment is the number of parameters (Q) 
		times the number of observables for that experiment (Q * exObsCount).
		When all experiments are considered as a whole, this requires stacking the gradient of each experiment
		on top of each other.  Because there can be a different number of observables per experiment, 
		the stack is not necessarily evenly spaced.
		The size of the stack gradient becomes Q*(totalObsCount)
		This logic also applies to the hessian.  So the hessian becomes a stack of Q*exObsCount x Q matricies where exObsCount is appropriate to each experiemnt.
		The total size of the stacked hessian is Q*(totalObsCount) x Q
		The stacking of the co-variance is more complicated.  The co-variance is a set of diagonal QxQ blocks,
		one for each observable within each experiment.
		Therefore, the co-variance is a mostly sparse Q*(totalObsCount) x Q*(totalObsCount) matrix with dense diagonal QxQ blocks
		
	Outline of algorithm
		Accept current parameters as the "new origin".  origNew()
			origNew() does all the heavy lifting to build a parabolic fit of the n-dimensional
			parameter space.  First it integrates H to get traceG and traceOG. Then it
			constructs and stacks the gradient, hessian, and covariances of each experiment
			into one big system.
			Then it uses FFT to estimate the noise on the signal
			Then it uses SVD to deconstruct the hessian into an eigen basis which can then
			be scaled to reduce the steps in poorly determined directions.
		
		Call the step manager to make a "new proposal", propNew()
			propNew() makes proposals in parameter space.  Like Levenburg Marquadt, it uses a parabolic
			fit to estimate the next step.  Unlike LM, it uses the eigen basis to selectively pull
			back in eigen directions that are poorly determined.  It uses the ratio of the bottom
			two eigen values to determine how many pull back steps it should make in each poorly
			determined direction, progressively eliminating eigen directions until the system
			makes progress.
		
		Evaluate the proposal by integrating D to get C and evaluating OC.  propEval()
			give a proposal Q (propQ) this function integrates D to get traceC and traceOC
			and measures the error function to get propE.
*/


extern int hackDumpFlag;
extern void trace( char *fmt, ... );

// KineticFitter
//------------------------------------------------------------------------------------------

KineticFitter::~KineticFitter() {
	clear();
}

void KineticFitter::setSendMsg( char *msg ) {
	if( fitThreadSendMsg ) {
		free( fitThreadSendMsg );
	}
	fitThreadSendMsg = strdup( msg );
}

KineticFitter::KineticFitter( KineticSystem &systemToCopy ) {
	slerpC = 0;
	slerpG = 0;
	icG = 0;
	dC_dt = 0;
	OG = 0;
	equallySpacedResidual = 0;
	vmd = 0;
	vmh = 0;
	integrator = 0;
	fitThreadSendMsg = 0;
	clear();
	KineticSystem::copy( systemToCopy );
}

void KineticFitter::clear() {
	KineticSystem::clear();
	memset( &fitThreadID, 0, sizeof(fitThreadID) );
	fitThreadMonitorDeath = 0;
	fitThreadMonitorStep = 0;
	fitThreadDeathFlag = 0;
	fitThreadStepFlag = 0;
	fitThreadSimulateEachStep = 0;
	fitThreadLoopCount = 0;
	fitThreadStatus = 0;

	origE = 0.0;
	origQ.clear();
	origVtt.clear();
	origSignal.clear();
	origNoise.clear();
	origSNR.clear();
	origGrad.clear();
	origHess.clear();
	origCovr.clear();
	origU.clear();
	origUt.clear();
	origS.clear();
	origVt.clear();
	origUtCovr.clear();
	origCovrInEigenSpace.clear();
	origGradInEigenSpace.clear();
	origGradInEigenSpaceScaledByS.clear();

	propE = 0.0;
	propQ.clear();
	propEigenRejectFlags.clear();
	propOffTheRails = 0;
	propBackoffStepsOnLastEigenValue = 0;

	minStepSizeC.clear();
	minStepSizeG.clear();

	if( slerpC ) delete slerpC; slerpC = 0;
	if( slerpG ) delete slerpG; slerpG = 0;
	if( icG ) delete icG; icG = 0;
	if( dC_dt ) delete dC_dt; dC_dt = 0;
	if( OG ) delete OG; OG = 0;
	if( equallySpacedResidual ) delete equallySpacedResidual; equallySpacedResidual = 0;

	origCount = 0;
	propCount = 0;
	qBeingFitCount = 0;
	qHistory.clear();
	eHistory.clear();

	totalObsCount = 0;

	if( vmd ) delete vmd; vmd = 0;
	if( vmh ) delete vmh; vmh = 0;
	if( integrator ) delete integrator; integrator = 0;

	if( fitThreadSendMsg ) {
		free( fitThreadSendMsg );
		fitThreadSendMsg = 0;
	}
}

void *KineticFitter::threadMain( void *args ) {
	// This is used only by the kineticFitThreaded as an entry point, do not call directly
	KineticFitter *_this = (KineticFitter *)args;
	_this->fit();
	return 0;
}

void KineticFitter::threadStart() {
	// CREATE a mutex for renderable variables
	pthread_mutex_init( &renderVariablesMutex, 0 );

	// LAUNCH a thread to run this fit.
	pthread_attr_t attr;
	pthread_attr_init( &attr );
	pthread_attr_setstacksize( &attr, 1024*4096 );
	pthread_create( &fitThreadID, &attr, &threadMain, (void*)this );
	pthread_attr_destroy( &attr );
}

void KineticFitter::renderVariablesLock() {
	pthread_mutex_lock( &renderVariablesMutex );
}

void KineticFitter::renderVariablesUnlock() {
	pthread_mutex_unlock( &renderVariablesMutex );
}



//---------------------------------------------------------------------------------------------------------------------

double KineticFitter::getSNRForRender( int q ) {
	double snr = origSNR.getD(q,0);
	if( propEigenRejectFlags.getI(q,0) ) {
		snr = 0.0;
	}
	return fabs( snr );
}


void KineticFitter::origNew() {
	ZTime startTime = zTimeNow();

	int i, q, q0, q1;

	// ACCEPT the propQ/propE as the new origQ/origE
	origQ.copy( propQ );
	origE = propE;

	// COMPUTE all the expensive terms for the gradient, hessian, and covariance for this new origin
	// Returns 1 if we're converged
	
	int qCount = origQ.rows;

	origGrad.alloc( totalObsCount * qCount, 1, zmatF64 );
	origHess.alloc( totalObsCount * qCount, qCount, zmatF64 );
	origCovr.alloc( totalObsCount * qCount, totalObsCount * qCount, zmatF64 );
		// The gradient, hessian, and covariance for current evaluation point

	int stackRow = 0;
		// Indexes into the testGrad and testHess for stacking (see "subtle issues" above)

	int eo = 0;
		// eo tracks the experiment and observable index. Eg (ex 0 has 1 obs and ex 1 has 2 obs then eo goes:0,1,2)

/*
// COMPUTE OC to get traceOC
if( exIsEquilibrium ) {
	// For an equilibirum experiment, the dC_dt term is considered
	// an observable.  For consistency we insert the dC_dt into both
	// its normal "deriv" slot and also as the observable of this experiment
	// The additional observables for an equilibrium experiment is really D at time 0
	double *D_atTime0 = integrator->traceC.getDerivColPtr( 0 );
	experiments[e]->traceOC.add( 0.0, D_atTime0, D_atTime0 );
	for( int q=0; q<origQ.rows; q++ ) {
		experiments[e]->traceOG.add( new KineticTrace(reagentCount()) );
		KineticTrace *ogq = experiments[e]->traceOG[q];

		if( parameterInfo[q].isRelevantToExperiment(e) ) {
			// The OG for an equilibrium experiment is really H at time 0
			double *H_atTime0 = integrator->traceG[ parameterInfo[q].pIndex ]->getDerivColPtr( 0 );
			ogq->add( 0.0, H_atTime0 );
		}
		else {
			ogq->copy( zeroTrace );
		}
	}
}
else {
*/


	// For each experiment...
	for( int e=0; e<eCount(); e++ ) {
		int exIsEquilibrium = experiments[e]->equilibriumExperiment;
		int exObsCount = experiments[e]->observablesCount();
			// The number of observables that THIS experiment has

		// CONVERT to local address space on the parameters (FETCH P[e] from Q)
		ZMat expP;
		getP( origQ, e, expP );

		// ALLOCATE the experimental grad, hess, covr
		ZMat expGrad( exObsCount*qCount, 1, zmatF64 );
		ZMat expHess( exObsCount*qCount, qCount, zmatF64 );
		ZMat expCovr( exObsCount*qCount, exObsCount*qCount, zmatF64 );
			// The gradient, hessian, and covariance for THIS experiment (will later be stacked into testGrad, testHess, testCovr)

		integrator->allocTraceG();

		for( q=0; q<qCount; q++ ) {
			// INTEGRATE H wrt q
			// Note that integration is only necessary wrt to parameters that are actually
			// relevant to this experiment.  For example, experiment 1 can not change wrt to 
			// the initial conditions of experiment 1.

			// @TODO: All of this code should go into a function and then this code
			// should launch a thread for each e/o
			// when all of those threads are complete then it can continue
			// at which point then the traces can be copied into the experiments
			// PROFILE this one part of this.  This is probably the only thing that should be threaded?
			// we could go nuts and try to thread at experiemnt level but probably not worth it
			// I think this requres that we make an array of integrators and then preallocate
			// the threads and have each thread have its own integrator which in this code
			// we setup and then unstall the thread.

			if( parameterInfo[q].isRelevantToExperiment(e) ) {
				if( parameterInfo[q].fitFlag && parameterInfo[q].type!=PI_OBS_CONST ) {

					if( exIsEquilibrium ) {
						// FORCE integrator to only evalulate initial conditions
						experiments[e]->mixsteps[0].duration = DBL_EPSILON;
					}

					// SETUP the initial conditions of G
					for( int j=0; j<reagentCount(); j++ ) {
						icG[j] = 0.0;
						if( /*!exIsEquilibrium &&*/ parameterInfo[q].type == PI_INIT_COND && parameterInfo[q].experiment == e && parameterInfo[q].reagent == j ) {
							icG[j] = 1.0;
						}
					}
					
					// @TODO Deal with multiple mixsteps
					// @TODO: Track minStep have an offtherails here					
					integrator->integrateH( parameterInfo[q].pIndex, icG, expP.getPtrD(), experiments[e]->mixsteps[0].duration );
				}
				else {
					// For parameters that are not being fit or are observable constants, G is all zero
					integrator->traceG[ parameterInfo[q].pIndex ]->zeroInit( reagentCount() );
				}
			}
		}

		// COPY the integrators's traceG into the experiment traceG
		experiments[e]->traceG.clear();
		for( q=0; q<qCount; q++ ) {
			if( parameterInfo[q].isRelevantToExperiment(e) ) {
				KineticTrace *kt = new KineticTrace;
				kt->copy( *integrator->traceG[ parameterInfo[q].pIndex ] );
				experiments[e]->traceG.add( kt );
			}
		}

		// ALLOC the matrix which will hold the FFT results of the residuals
		experiments[e]->observableResidualFFTMat.alloc( exObsCount, 64, zmatF64 );
		
		double expMeasuredDuration = experiments[e]->measuredDuration();
		KineticVMCodeOG *vmog = experiments[e]->vmog;
		
		// FOREACH observable in experiment e
		for( int o=0; o<exObsCount; o++ ) {
			int dataCount = experiments[e]->measured[o]->cols;

			// FOREACH time sample...
			for( i=0; i<dataCount; i++ ) {
				double time = experiments[e]->measured[o]->getTime(i);
				double data = experiments[e]->measured[o]->getData(i,0);
				
				// FETCH the observable value at time by interpolation 
				double obsVal = experiments[e]->traceOC.getElemSLERP( time, o );

				// ACCUM e (square error)
				double residual = obsVal - data;

				// COMPUTE OG at this point
				memset( OG, 0, sizeof(double) * qCount );
				for( q=0; q<qCount; q++ ) {
					// SLERP to get C at this point
					experiments[e]->traceC.getColSLERP( time, slerpC );
					
					// SLERP to get G at this point
					if( parameterInfo[q].isRelevantToExperiment(e) && parameterInfo[q].fitFlag ) {
						experiments[e]->traceG[ parameterInfo[q].pIndex ]->getColSLERP( time, slerpG );
					}
					else {
						memset( slerpG, 0, sizeof(double) * reagentCount() );
					}

					// EVALUATE OG at this point
					int actualObsCount = experiments[e]->observableInstructions.count;
					if( exIsEquilibrium && o >= actualObsCount ) {
						OG[q] = 0.0;
						if( parameterInfo[q].isRelevantToExperiment(e) ) {
							// The OG for an equilibrium experiment is really H at time 0
							double *H_atTime0 = integrator->traceG[ parameterInfo[q].pIndex ]->getDerivColPtr( 0 );
							OG[q] = H_atTime0[ o - actualObsCount ];
						}
					}
					else {
						if( parameterInfo[q].fitFlag ) {
							vmog->execute_OG( o, q, slerpC, expP.getPtrD(), slerpG, &OG[q] );
						}
					}
				}

				// ACCUMULATE gradient, hessian, co-varaince
				for( q0=0; q0<qCount; q0++ ) {

					// ACCUMULATE the gradient
					double derivObsAtTimeWrtQ0 = OG[q0];
					assert( q0*exObsCount+o < expGrad.rows );
					*expGrad.getPtrD(q0+o*qCount,0) += 2.0 * residual * derivObsAtTimeWrtQ0;

					for( q1=0; q1 < qCount; q1++ ) {
						double derivObsAtTimeWrtQ1 = OG[q1];
						assert( q0*exObsCount+o < expHess.rows );
						assert( q1 < expHess.cols );

						// ACCUMULATE the hessian
						*expHess.getPtrD(q0+o*qCount,q1) += 2.0 * derivObsAtTimeWrtQ0 * derivObsAtTimeWrtQ1;

						// ACCUMULATE the unscaled co-variance
						*expCovr.getPtrD(q0+o*qCount,q1+o*qCount) += derivObsAtTimeWrtQ0 * derivObsAtTimeWrtQ1;
					}
				}
			}

			// FFT the residuals to compute noise specturm
			double variance = 0.0;

			if( exIsEquilibrium ) {
				variance = experiments[e]->equilibriumVariance.getD(o,0);
			}
			else {
				// SAMPLE the function at 128 evenly spaced intervals for FFT
				double t = 0.0;
				double tStep = expMeasuredDuration / 128.0;
				for( i=0; i<128; i++, t += tStep ) {
					equallySpacedResidual[i] = experiments[e]->traceOC.getElemSLERP( t, o ) - experiments[e]->measured[o]->getElemSLERP( t, 0 );
				}

				// FFT this 128 real valued trace
				FFTGSL fft;
				fft.fftD( equallySpacedResidual, 128 );

				// COMPUTE the power spectrum of the fft (the whole thing for viewing)
				for( i=0; i<64; i++ ) {
					double real, imag;
					fft.getComplexD( i, real, imag );

					// STORE the power spectrum into the observableResidualFFTMat for later observation
					experiments[e]->observableResidualFFTMat.putD( o, i, sqrt( real*real + imag*imag ) );

					// ACCUMULATE the variance of the top-half of the specturm for noise estimation
					if( i >= 31 ) {
						variance += real*real + imag*imag;
					}
				}

				// CONVERT to per-point variance
				variance /= 32.0;
			}

			// SCALE the covariance matrix by the variance
			for( q0=0; q0 < qCount; q0++ ) {
				for( q1=0; q1 < qCount; q1++ ) {
					*expCovr.getPtrD(q0+o*qCount,q1+o*qCount) *= variance;
				}
			}
		}

		// INCREMENT eo by the number of observables for this experiment 
		eo += exObsCount;

		// STACK the hessian and covariance of the experiments into one.  See "Stacking" in the SUBTLE ISSUES
		int beginStackRow = stackRow;
		for( q0=0; q0<qCount*exObsCount; q0++ ) {
			origGrad.setD( stackRow, 0, expGrad.getD( q0, 0 ) );

			// STACK the hessian with the observables from this experiment
			for( q1=0; q1 < qCount; q1++ ) {
				origHess.setD( stackRow, q1, expHess.getD( q0, q1 ) );
			}

			// STACK the covariaince matrix in offet blocks (tricky diagonal block structure)
			for( q1=0; q1 < qCount; q1++ ) {
				origCovr.setD( stackRow, q1+beginStackRow, expCovr.getD( q0, q1 ) );
			}

			stackRow++;
		}
	}
	
	// SVD the hessian and scale terms used for making proposals from this origin

	// CONVERT origGrad into the actual gradient by negating
	for( q=0; q<origGrad.rows; q++ ) {
		*origGrad.getPtrD(q) *= -1.0;
	}

	// SVD the Hessian: H = U * S * Vt
	zmatSVD_GSL( origHess, origU, origS, origVt );
	zmatTranspose( origU, origUt );

	// TRANSFORM the covariance into the eigen space
	zmatMul( origUt, origCovr, origUtCovr );
	zmatMul( origUtCovr, origU, origCovrInEigenSpace );
	assert( origCovrInEigenSpace.rows == qCount && origCovrInEigenSpace.cols == qCount );
	
	// EXTRACT the diagonal elements out of the eigen covariance to get the noise, store off in member variable fitNoise
	zmatDiagonalMatrixToVector( origCovrInEigenSpace, origNoise );

	// TRANSFORM the gradient vector into eigen space
	zmatMul( origUt, origGrad, origGradInEigenSpace );

	// For now assert that they are all positive
	for( q=0; q<qCount; q++ ) {
		if( origS.getD(q,0) < 0.0 ) {
			// @TODO surgery on the negative singular values?
			assert( 0 );
		}
		assert( origS.getD(q,0) >= 0.0 );
	}

	// TRANSFORM origQ into eigen space
	zmatMul( origVt, origQ, origSignal );

	// SCALE gradient by singular values
	assert( qCount == origS.rows && qCount == origGradInEigenSpace.rows );
	origGradInEigenSpaceScaledByS.copy( origGradInEigenSpace );

	// COMPUTE SNR
	origSNR.alloc( qCount, 1, zmatF64 );
	for( q=0; q<qCount; q++ ) {
		origSNR.putD(q,0,0.0);
		if( origS.getD(q,0) > 0.0 ) {
			double n2 = *origNoise.getPtrD(q,0) / ( origS.getD(q,0) * origS.getD(q,0) );
			origSNR.putD( q, 0, fabs( origSignal.getD(q,0) / n2 ) );
		}

		// REJECT all q's that are either not being fit or have < 1 SNR
		if( q < qBeingFitCount && origSNR.getD(q,0) > 1.0 ) {
			// Because the s vec is sorted it automatically means
			// that those q's which are not being fit are at the bottom
			// thus it is OK to compare q to qBeingFitCount
			*origGradInEigenSpaceScaledByS.getPtrD(q,0) /= origS.getD(q,0);
		}
		else {
			origGradInEigenSpaceScaledByS.setD(q,0,0.0);
			propEigenRejectFlags.setI( q, 0, 1 );
		}
	}

	origNewAccum += zTimeNow() - startTime;
}


void KineticFitter::propEval() {

	ZTime startTime = zTimeNow();

	// This take the member propQ and evaluates it rsulting in
	// propE being set as well as the traceC for all experiment being updated

	propE = 0.0;

	int stackRow = 0;
		// Indexes into the testGrad and testHess for stacking (see "subtle issues" above)

	int eo = 0;
		// eo tracks the experiment and observable index. Eg (ex 0 has 1 obs and ex 1 has 2 obs then eo goes:0,1,2)

	for( int e=0; e<eCount(); e++ ) {
		// @TODO: All of this code should go into a function and then this code
		// should launch a thread for each experiment doing the following
		// when all of those threads are complete then it sums up the errors from each into propE
	
		int offTheRails = 0;
			// This flag is set if the integrator or the error accumulator
			// gets wildly out of hand compared to the first time through.
			// It is a signal that testQ is far from reality and thus the step should be reduced.
		
		int exIsEquilibrium = experiments[e]->equilibriumExperiment;
		int exObsCount = experiments[e]->observablesCount();
			// The number of observables that THIS experiment has

		// CONVERT to local address space on the parameters (FETCH P[e] from Q)
		ZMat expP;
		getP( propQ, e, expP );

		// @TODO Deal with multiple mixsteps
		if( exIsEquilibrium ) {
			// FORCE the integrator to only evaluate the initial condition
			// so that we can get the dC_dt at time 0
			experiments[e]->mixsteps[0].duration = DBL_EPSILON;
		}

		// FETCH the initial conditions
		int obsConstCount = 0;
		paramGet( PI_OBS_CONST, &obsConstCount, -1, 0 );
		double *ic = expP.getPtrD(reactionCount()+obsConstCount,0);
		double minStep = 0.01 * minStepSizeC.getD( e, 0 );
			// minStep tells the integrator to stop if the step is 1/100th of the smallest value
			// seen during the first run of the integrator.  This is a sanity check to prevent tings from going haywire.
		
		// INTEGRATE D to get traceC
		int ok = integrator->integrateD( ic, expP.getPtrD(), experiments[e]->mixsteps[0].duration, 0.0, minStep );
		if( !ok ) {
			// The integration was stopped because the min step was exceeded
			// Skip straight to aborting this testQ
			propOffTheRails = 1;
			return;
		}
		
		// STORE the minimum step size on the first time through
		minStepSizeC.putD( e, 0, integrator->getMinStepSizeC() );
		
		// MOVE the integrator's traces over to the fitter system
		experiments[e]->traceC.copy( integrator->traceC );
		
		// POLYFIT for later slerping
		experiments[e]->traceC.polyFit();

		// COMPUTE OC which will fill in traceOC
		experiments[e]->computeOC( expP.getPtrD() );

		// POLYFIT in preparation for slerping below
		experiments[e]->traceOC.polyFit();

		// FOREACH observable...
		for( int o=0; o<exObsCount; o++ ) {
			int dataCount = experiments[e]->measured[o]->cols;

			double testEForThisEO = 0.0;

			// FOREACH time sample...
			for( int i=0; i<dataCount; i++ ) {
				double time = experiments[e]->measured[o]->getTime(i);
				double data = experiments[e]->measured[o]->getData(i,0);
				
				// FETCH the observable value at time by interpolation 
				double obsVal = experiments[e]->traceOC.getElemSLERP( time, o );

				// ACCUM e (square error)
				double residual = obsVal - data;
				propE += residual * residual;
			}
		}
	}

	qHistory.growCol( propQ );
	eHistory.growCol( propE );

	propEvalAccum += zTimeNow() - startTime;
}


int KineticFitter::propNew() {
	int i, q;
	
	int qCount = origQ.rows;

	// FIND the bottom two unrejected eigen values.
	// REJECT eigen values that have now been sufficiently backed off
	int b0 = -1;
	int b1 = -1;
	
	int countRejected = 0;
	for( i=0; i<qCount; i++ ) countRejected += propEigenRejectFlags.getI(i,0) ? 1 : 0;
	
	while( countRejected < qCount ) {

		for( q=qCount-1; q>=0; q-- ) {
			if( ! propEigenRejectFlags.getI(q,0) ) {
				b0 = q;
				break;
			}
		}
		for( q=b0-1; q>=0; q-- ) {
			if( ! propEigenRejectFlags.getI(q,0) ) {
				b1 = q;
				break;
			}
		}
		
		if( b0 < 0 ) {
			// All eigen values are rejected.
			return PN_TRAPPED;
		}

		// b0 and b1 now contain the last and penultimate unrejected eigen indicies
		
		propBackoffStepsOnLastEigenValue++;
		
		int maxSteps;
		if( b0 == 0 ) {
			// For the top eigen value we use a arbitrary number...
			maxSteps = 16;
		}
		else {
			// but for those below the top we use a ratio to the next higher eigen value
			double bottomRatio = origS.getD(b1,0) / origS.getD(b0,0);
			maxSteps = (int)ceil( log( bottomRatio ) / log( 2.0 ) );
			maxSteps = maxSteps < 2 ? 2 : maxSteps;
		}
		
		// CHECK if we've backed off too much
		if( propBackoffStepsOnLastEigenValue > maxSteps ) {
			// We've backed off enough in this direction to abandon it
			propEigenRejectFlags.putI( b0, 0, 1 );
			propBackoffStepsOnLastEigenValue = 0;
		}
		else {
			break;
		}

		countRejected = 0;
		for( i=0; i<qCount; i++ ) countRejected += propEigenRejectFlags.getI(i,0) ? 1 : 0;
	}

	if( b0 < 0 ) {
		// All eigen values are rejected.
		return PN_TRAPPED;
	}

	// ZERO out the rejected directions
	ZMat propGradInEigenSpaceScaledByS( origGradInEigenSpaceScaledByS );
	for( q=0; q<qCount; q++ ) {
		if( propEigenRejectFlags.getI(q,0) ) {
			propGradInEigenSpaceScaledByS.putD( q, 0, 0.0 );
		}
	}
	
	// The following logic is an optimization.
	// When the fitter is convering well, it is often the case that bottom of the parabaloid
	// will be the accepted point at this origin.  The noraml algoritm seaches for a local
	// minimum by asking if the currently backed-off value is worse than the previous.  When it
	// finds minima, it has to unwind the prop state back to the previous state and this is
	// somewhat costly as it involves another call to propEval().  So, for the common situation
	// where the parabaloid is going to be the accepted state, it makes sense to test it second
	// so that the prop state gets left in the correct state and no re-evaluation is needed.
	// Thus, the logic below inverts the first two props after a new origin and can short-ciruit
	// the acceptance logic.
	
	double s2t;
	if( propCountSinceOrig == 1 ) {
		// Invert the order to test the "backed-off" version before the bottom of the parabaloid
		s2t = pow( 2.0, (double)(1) );
	}
	else if( propCountSinceOrig == 2 ) {
		// Now we test the bottom of the parabaloid
		s2t = pow( 2.0, (double)(0) );
	}
	else {
		s2t = pow( 2.0, (double)(propBackoffStepsOnLastEigenValue-1) );
	}
		
	// SCALE the bottom direction
	propGradInEigenSpaceScaledByS.setD( b0, 0, origGradInEigenSpaceScaledByS.getD(b0,0) / s2t );
	
	// SOVLE for deltaQ
	ZMat deltaQ;
	zmatTranspose( origVt, origVtt );
	zmatMul( origVtt, propGradInEigenSpaceScaledByS, deltaQ );

	// ASSERT that parameters that are not being fit should have no deltaQ
	for( q=0; q<qCount; q++ ) {
		if( ! parameterInfo[q].fitFlag ) {
			assert( fabs(deltaQ.getD(q,0)) < 4.0 * FLT_EPSILON );
		}
	}

	// SOLVE for new propQ
	zmatAdd( origQ, deltaQ, propQ );

	// CHECK for convergence
	double qSquared = zmatDot( origQ, origQ );
	double deltaQSquared = zmatDot( deltaQ, deltaQ );
	if( deltaQSquared < 1e-14 * qSquared ) {
		// @TODO: What are the right values for these epsilons?
		propEigenRejectFlags.zero();
		return PN_CONVERGED;
	}

	// CHECK for acceptance because the last prop moved upwards and the prev is better than orig
	if( !propOffTheRails && propCountSinceOrig >= 3 ) {
		int lastIndex, currIndex;
		if( propCountSinceOrig == 3 ) {
			// INVERT the order on the optimizated step
			lastIndex = origLastHistoryIndex+propCountSinceOrig-1;
			currIndex = origLastHistoryIndex+propCountSinceOrig-2;
		}
		else {
			lastIndex = origLastHistoryIndex+propCountSinceOrig-2;
			currIndex = origLastHistoryIndex+propCountSinceOrig-1;
		}

		double lastPropE = eHistory.getD( 0, lastIndex );
		double currPropE = eHistory.getD( 0, currIndex );

		if( lastPropE < origE && currPropE > lastPropE ) {
			// Accept
			zmatGetCol( qHistory, lastIndex, propQ );

			if( propCountSinceOrig > 3 ) {
				// This "if" is here so that it will skip the re-eval during the special startup inversion optimization (see above notes)

				propEval();
					// Except in the special optimization case, we have to reset the traceC system --
					// it isn't enough to merely recover the old propE
			}
			propCountSinceOrig = 1;
				// This is 1 to include the new orig
			propBackoffStepsOnLastEigenValue = 0;
			origLastHistoryIndex = eHistory.cols-1;
			propEigenRejectFlags.zero();
			origCount++;
			return PN_ACCEPT;
		}
	}

	// ... otherwise, this is a regular proposal	
	propCountSinceOrig++;
	propCount++;
	return PN_PROPOSE;
}


void KineticFitter::fit( int takeOnlyOneStep ) {
	int convered = 0;
	int trapped = 0;
	
	minStepSizeC.alloc( eCount(), 1, zmatF64 );
	minStepSizeG.alloc( eCount(), 1, zmatF64 );
	
	fitThreadLoopCount = 0;
	
	// COUNT the total number of observables for all experiments and COMPILE observable expresisons
	totalObsCount = 0;
	for( int e=0; e<eCount(); e++ ) {
		totalObsCount += experiments[e]->observablesCount();
		experiments[e]->compileOC();
		experiments[e]->compileOG();
	}

	// FETCH the initial parameters from the system into currQ & propQ
	paramInitPQIndex();
	getQ( origQ );
	getQ( propQ );

	// COUNT how many parameters are being fit
	qBeingFitCount = 0;
	for( int q=0; q<origQ.rows; q++ ) {
		if( parameterInfo[q].fitFlag ) {
			qBeingFitCount++;
		}
	}

	// CREATE virtual machines and integrator. This requires knowing the length of the P vector
	ZMat pVec;
	getP( origQ, 0, pVec );
	vmd = new KineticVMCodeD( this, reactionCount() );
	vmh = new KineticVMCodeH( this, pVec.rows );
	vmh->disassemble( "h" );
	integrator = new KineticIntegrator( KI_ROSS, vmd, vmh );
		// Note that the integrator is SHARED by all experiments.
		// So when the integrator is called, the Q vector has to be mapped to the P vector

	// ALLOCATE workspace
	slerpC = new double[ reagentCount() ];
	slerpG = new double[ reagentCount() ];
	icG = new double[ reagentCount() ];
	dC_dt = new double[ reagentCount() ];
	OG = new double[ origQ.rows ];
	equallySpacedResidual = new double[ 128 ];

	// INIT the proposal count state
	propEigenRejectFlags.alloc( origQ.rows, 1, zmatS32 );
	propBackoffStepsOnLastEigenValue = 0;
	
	// CREATE the first origin at the starting place
	stateHistory.alloc( 1, 1 );
	stateHistory.putD( 0, 0, 1.0 );
	propEvalAccum = 0.0;
	origNewAccum = 0.0;
	propEval();
	origNew();
	propCount = 1;
	origCount = 1;
	origLastHistoryIndex = eHistory.cols - 1;
	propCountSinceOrig = 1;
		// This is init to one because the first origin in the history

	while( 1 ) {

		if( takeOnlyOneStep && fitThreadLoopCount == 1 ) {
			// For sampling the error surface, sometimes we only want to take a single step 
			fitThreadStatus = KE_DONE_SINGLE_STEP;
			goto done;
		}

		fitThreadLoopCount++;

		// MONITOR the breakpoint so the outside thread can watch each step and request pauses, etc
		if( fitThreadSendMsg ) {
			zMsgQueue( fitThreadSendMsg );
		}
		if( fitThreadMonitorStep ) {
			while( 1 ) {
				fitThreadStatus = KE_PAUSED;
				if( fitThreadStepFlag ) {
					// An outside thread has signalled to proceed
					fitThreadStepFlag = 0;
					fitThreadStatus = KE_RUNNING;
					break;
				}
				else if( fitThreadMonitorDeath && fitThreadDeathFlag ) {
					goto done;
				}
				else {
					zTimeSleepMils( 500 );
				}
			}
		}

		// GENERATE a new proposal
		int state = propNew();

		// RESOLVE four possible outcomes
		switch( state ) {
			case PN_CONVERGED:
				//  Converged: There are no more proposals to be made. The last orig is final
				fitThreadStatus = KE_DONE_CONVERGED;
				goto done;

			case PN_TRAPPED:
				//  Trapped: There are no more proposals to be made, but we don't belive that the last orig has converged
				fitThreadStatus = KE_DONE_TRAPPED;
				goto done;

			case PN_PROPOSE: {
				//  Propse: This is a new proposal, we don't move orig
				propEval();
				ZMat state( 1, 1 );
				state.putD( 0, 0, 0.0 );
				stateHistory.growCol( state );
				break;
			}

			case PN_ACCEPT: {
				//  Accept: the last proposal as the new orig
				origNew();
				ZMat state( 1, 1 );
				state.putD( 0, 0, 1.0 );
				stateHistory.growCol( state );
				break;
			}
		}

		renderVariablesLock();
		for( int e=0; e<eCount(); e++ ) {
			experiments[e]->copyRenderVariables();
		}		
		renderVariablesUnlock();
	}
	
	done:;
	
	delete slerpC; slerpC=0;
	delete slerpG; slerpG=0;
	delete icG; icG=0;
	delete dC_dt; dC_dt=0;
	delete OG; OG=0;
	delete equallySpacedResidual; equallySpacedResidual=0;

	delete vmd; vmd = 0;
	delete vmh; vmh = 0;
	delete integrator; integrator = 0;

	if( fitThreadSendMsg ) {
		zMsgQueue( fitThreadSendMsg );
	}
}








// Old code for reference
//--------------------------------------------------------------------------------------------------------------------------------------


#if 0

void KineticFitter::fit( int singleStep ) {


	/*
	
	NAMING CONVENTIONS
	-----------------------------------------------------------------------
	Q:     the global vector of parameters
	P:     the local to an experiment vector of parameters
	C:     the concentration of the reagents
	D:	   the time derivatives of C (integrated to derive C)
	OC:    the observable value (some arbitrary function of C's)
	G[q]:  the derivative of C with respect to (wrt) parameter q
	H[q]:  the time derivative G[q] (integrated to derive G[q])
	OG[q]: the derivative of OC with respect to (wrt) parameter q
	test*: the set of parameters and other state variables under test
	curr*: the set of parameters and other state variables last accepted as an improvement
	currQ: the position in parameter space of last acceptance
	testQ: the position in parameter space at the current evaluation
	currE: the value of the error function on last acceptance
	testE: the value of the error function at the current evaluation
	trace*:  A time trace of the given variable.  Each trace includes a time basis and possibly derivatives for SLERPing
	
	SUBTLE ISSUES
	-----------------------------------------------------------------------
	Q vs P.
		The global parameters of the system that are being fit are called "q".
		The q parameters are ordered as:
			reaction rates
			observable constants (shared over all experiments)
			initial conditions for experiment 0
			initial conditions for experiment 1
			etc. for each experiment
		The "p" parameters are the parameters for any ONE given experiment.  The are ordered as:
			reaction rates, 
			observable constants (shared over all experiments)
			initial conditions for THAT experiment
		Thus, the p vector may be shorter than the q vector if there is more than one experiment
	
	Gradient, Hessian, and Co-variance
		* The gradient is the slope of the error surface. It describes how fast the
		  error function is changing wrt each direction in parameter space.
		* The hessian is the 2nd deriv of the error surface.  Each column vector
		  of the hessian is the rate change of the gradient wrt to each direction in parameter space.
		* The co-variance describes the correlation of the parameters.
		  For a given observation, a large value in this matrix would indicate strong correlation between the two parameters.
		  In other words, in order to hold a solution to the fit in place if you moved one of these parameters
		  you'd also have to move the other.  A near-zero value, on the other hand, would indicate that movement
		  along one parameter would adjust the fit independent of the other parameter.
		  A large negative value would indicate that they have to move in opposite directions to continue to explain the data.
		  
	Zeroing
		Note that some parameters are not relevant to some given experiments and therefore those rows/cols
		of the gradient, hessian, and co-variance are zeroed out.
		
	Stacking of the gradient, hessian, and co-variance
		For a given experiment, an element in the gradient vector is the rate of change of the error of an observable wrt a parameter.
		Therefore the size of the gradient vector for a single experiment is number of parameters (Q) times the number of observables for that experiment (Q * exObsCount)
		When all experiments are considered as a whole, this requires stacking gradient of each experiment
		on top of each other.  Since there can be a different number of observables per experiment, 
		this means that the stack in the gradient is not necessarily evenly spaced.
		The size of the stack gradient becomes Q*(totalObsCount)
		This logic also applies to the hessian.  So the hessian becomes a stack of Q*exObsCount x Q matricies where exObsCount is appropriate to each experiemnt.
		The size of the stacked hessian is Q*(totalObsCount) x Q
		The stacking of the co-variance is more complicated.  The co-variance is a set of diagonal QxQ blocks,
		one for each observable within each experiment.
		Therefore, the co-variance is a mostly sparse Q*(totalObsCount) x Q*(totalObsCount) matrix with only diagonal QxQ blocks
		
	Progressive elimination of poor search directions
		@TODO Describe this tricky algorithm
		
	Computing Step Size with SVD
		@TODO Describe the decoposition


	OUTLINE of code  (see thse line numbers embedded in following code
	-----------------------------------------------------------------------

	01. while( !done ) {
	02. 	foreach experiment {
	03.		integrate D to get traceC
	04.		compute OC to get traceOC
	05.		evaluate the error to get testE
	06.		if test is worse than curr then the step was too big {
	07.			progressively eliminate lower SNR search direcitons
	08.		}
	09.		else if was better {
	10.			integrate H to get traceG[] for each parameter
	11.			for each observable {
	12.				for each data point {
	13.					compute OG at this point
	14.					accumulate gradient, hessian, co-varaince
	15.				}
	16.				fft the residuals to compute noise specturm
	17.			}
	18.			stack the hessian and covariance
	19.		}
	21.		copy variable to make them visible to renderer thread
	22.		test for acceptance and convergence
	23.		if( accepted ) {
	24.			currX = testX
	25.			compute new testX
	26.		}
	27.}
	

	*/


	int i;
		// Used to index data points
	int q, q0, q1;
		// Used to index fitting parameters
	int e;	
		// Used to index experiments
	int o;	
		// Used to index observables


	// INIT thread status
	fitThreadStatus = KE_RUNNING;
	fitThreadLoopCount = 0;
	qHistory.clear();
	
	// COUNT the total number of observables for all experiments and COMPILE observable expresisons
	int totalObsCount = 0;
	for( e=0; e<eCount(); e++ ) {
		totalObsCount += experiments[e]->observablesCount();
		experiments[e]->compileOC();
		experiments[e]->compileOG();
	}

	ZMat currMaxError( totalObsCount, 1, zmatF64 );
	ZMat testMaxError( totalObsCount, 1, zmatF64 );
		// These track the maximum error in each observable for detecting if the integrator goes off the rails

	ZMat testGradInEigenSpaceScaled;
	ZMat testGradInEigenSpaceRejectFlag( currQ.rows, 1, zmatU32 );
		// The algorithm that progressively eliminates weak SNR directions uses
		// these values to flag directions that it does wish to search anymore (until the next acceptance)

	// FETCH the initial parameters from the system into currQ & testQ
	getQ( currQ );
	int qCount = currQ.rows;
		// currQ is the parameters which were last accepted as good

	ZMat testQ;
	testQ.copy( currQ );
		// testQ is the parameters currently under consideration

	ZMat deltaQ( qCount, 1, zmatF64 );
		// deltaQ is the step in parameter space that will be taken
		// it's what we solve for with the hessian system

	// COUNT how many parameters are being fit
	qBeingFitCount = 0;
	for( q=0; q<qCount; q++ ) {
		if( parameterInfo[q].fitFlag ) {
			qBeingFitCount++;
		}
	}

	currE = DBL_MAX;
		// currE stores the error (sum square error) of the last accepted parameters
		// Starting at DBL_MAX forces acceptance of the first evaluation of the error function

	// CREATE a virtual machine and an integrator. This requires knowing the length of the P vector
	ZMat pVec;
	getP(currQ,0,pVec);
	KineticVMCodeD *vmd = new KineticVMCodeD( this, reactionCount() );
	KineticVMCodeH *vmh = new KineticVMCodeH( this, pVec.rows );
	KineticIntegrator *integrator = new KineticIntegrator( KI_ROSS, vmd, vmh );
		// Note that the integrator is SHARED by all experiments.
		// So when the integrator is called, the Q vector has to be mapped to the P vector
		
	ZMat minStepSize( eCount(), 1, zmatF64 );
		// This matrix stores the minimium step size used during the first integration. 
		// Used to determine if the parameters have gone off the rails

	// CLEAR timing variables
	timingTotal = 0.0;
	timingIntegrateDTotal = 0.0;
	timingIntegrateHTotal = 0.0;

	// ALLOCATE workspace
	double *slerpC = (double *)alloca( sizeof(double) * reagentCount() );
	double *slerpG = (double *)alloca( sizeof(double) * reagentCount() );
	double *icG = (double *)alloca( sizeof(double) * reagentCount() );
	double *dC_dt = (double *)alloca( sizeof(double) * reagentCount() );
	double *OG = (double *)alloca( sizeof(double) * qCount );
	double *equallySpacedResidual = (double *)alloca( sizeof(double) * 128 );


	// PREPARE all the equilibirum experiments with a fake
	// measured value of a zero row
/*
	KineticTrace zeroTrace;
	zeroTrace.zeroInit( reagentCount() );

	for( e=0; e<eCount(); e++ ) {
		int exIsEquilibrium = experiments[e]->equilibirumExperiment;
		if( exIsEquilibrium ) {
			experiments[e]->measuredClear();
			for( i=0; i<reagentCount(); i++ ) {
				KineticTrace *zeroCopy = new KineticTrace;
				zeroCopy->copy( zeroTrace );
				experiments[e]->measured.add( zeroCopy );
			}
		}
	}
*/

	//========================================================================================
	// OUTLINE 01.  Descend down fitting space until convergence or trap
	//========================================================================================

	int trapCount = 0;
	int converged = 0;
	int trapped = 0;
	int cancelled = 0;

	// CHECK for trivial rejection case
	if( qBeingFitCount <= 0 ) {
		cancelled = 1;
		// Nothing to do!
	}

	while( ! converged && ! trapped && ! cancelled ) {
		int worse = 0;
			// This flag is set when it is detected that testE > currE.
			// In other words, when the step was too large and needs to be reduced

		// UPDATE all the experiments with the current parameters
		putQ( testQ );

		if( singleStep && fitThreadLoopCount == 1 ) {
			// For sampling the error surface, sometimes we only want to take a single step 
			break;
		}

		fitThreadLoopCount++;

		// MONITOR the breakpoint so the outside thread can watch each step and request pauses, etc
		if( fitThreadSendMsg ) {
			zMsgQueue( fitThreadSendMsg );
		}
		if( fitThreadMonitorStep ) {
			while( 1 ) {
				fitThreadStatus = KE_PAUSED;
				timingCurrentLoopStart = 0.0;
				if( fitThreadStepFlag ) {
					// An outside thread has signalled to proceed
					fitThreadStepFlag = 0;
					fitThreadStatus = KE_RUNNING;
					break;
				}
				else if( fitThreadMonitorDeath && fitThreadDeathFlag ) {
					cancelled = 1;
					break;
				}
				else {
					zTimeSleepMils( 500 );
				}
			}
		}
		timingCurrentLoopStop = 0.0;
		timingCurrentLoopStart = zTimeNow();

		double testE = 0.0;
			// The error for current evaluation point
		ZMat testGrad( totalObsCount * qCount, 1, zmatF64 );
		ZMat testHess( totalObsCount * qCount, qCount, zmatF64 );
		ZMat testCovr( totalObsCount * qCount, totalObsCount * qCount, zmatF64 );
			// The gradient, hessian, and covariance for current evaluation point
		int stackRow = 0;
			// Indexes into the testGrad and testHess for stacking (see "subtle issues" above)
		int eo = 0;
			// eo tracks the experiment and observable index. Eg (ex 0 has 1 obs and ex 1 has 2 obs then eo goes:0,1,2)

		//========================================================================================
		// OUTLINE 02.  For each experiment...
		//========================================================================================
		for( e=0; e<eCount(); e++ ) {
			int offTheRails = 0;
				// This flag is set if the integrator or the error accumulator
				// gets wildly out of hand compared to the first time through.
				// It is a signal that testQ is far from reality and thus the step should be reduced.
			
			int exObsCount = experiments[e]->observablesCount();
				// The number of observables that THIS experiment has
			
			int exIsEquilibrium = experiments[e]->equilibirumExperiment;
			if( exIsEquilibrium ) {
				// In the special equilibirium experiments, the observables are each reagent
				exObsCount = reagentCount();
			}
				
			ZMat expGrad( exObsCount*qCount, 1, zmatF64 );
			ZMat expHess( exObsCount*qCount, qCount, zmatF64 );
			ZMat expCovr( exObsCount*qCount, exObsCount*qCount, zmatF64 );
				// The gradient, hessian, and covariance for THIS experiment (will later be stacked into testGrad, testHess, testCovr)

			//========================================================================================
			// OUTLINE 03.  integrate D to get traceC
			//========================================================================================								

			// LATCH timing
			timingIntegrateDCurrStop = 0.0;
			timingIntegrateDCurrStart = zTimeNow();

			// CONVERT to local address space on the parameters (FETCH P[e] from Q)
			ZMat expP;
			getP( testQ, e, expP );

			// @TODO Deal with multiple mixsteps
			if( exIsEquilibrium ) {
				// FORCE the integrator to only evaluate the initial condition
				// so that we can get the dC_dt at time 0
				experiments[e]->mixsteps[0].duration = DBL_EPSILON;
			}

			// FETCH the initial conditions
			double *ic = paramGetVector( PI_INIT_COND, 0, e, 0 );
			double minStep = fitThreadLoopCount > 1 ? 0.01 * minStepSize.getD( e, 0 ) : 0.0;
				// minStep tells the integrator to stop if the step is 1/100th of the smallest value
				// seen during the first run of the integrator.  This is a sanity check to prevent tings from going haywire.
			
			// INTEGRATE D to get traceC
			int ok = integrator->integrateD( ic, expP.getPtrD(), experiments[e]->mixsteps[0].duration, 0.0, minStep );
			delete ic;
			if( !ok ) {
				// The integration was stopped because the min step was exceeded
				// Skip straight to aborting this testQ
				offTheRails = 1;
				goto escape;
			}
			
			// STORE the minimum step size on the first time through
			if( fitThreadLoopCount == 1 ) {
				minStepSize.putD( e, 0, integrator->getMinStepSizeC() );
			}
			
			// LATCH timing
			timingIntegrateDCurrStop = zTimeNow();

			// MOVE the integrator's traces over to the fitter system
			experiments[e]->traceC.copy( integrator->traceC );
			
			// POLYFIT for later slerping
			experiments[e]->traceC.polyFit();

			//========================================================================================
			// OUTLINE 04.  compute OC to get traceOC
			//========================================================================================
			if( exIsEquilibrium ) {
/*
				// @TODO
				// For an equilibirum experiment, the dC_dt term is considered
				// an observable.  For consistency we the dC_dt into both
				// its normal "deriv" slot and also as the observable of this experiment
				// The observable for an equilibrium experiment is really D at time 0
				double *dC_dt_atTime0 = integrator->traceC.getDerivColPtr( 0 );
				experiments[e]->traceOC.add( 0.0, dC_dt_atTime0, dC_dt_atTime0 );
				for( q=0; q<qCount; q++ ) {
					experiments[e]->traceOG.add( new KineticTrace(reagentCount()) );
					KineticTrace *ogq = experiments[e]->traceOG[q];

					if( parameterInfo[q].isRelevantToExperiment(e) ) {
						// The OG for an equilibrium experiment is really H at time 0
						double *dG_dt_atTime0 = integrator->traceG[ parameterInfo[q].pIndex ]->getDerivColPtr( 0 );
						ogq->add( 0.0, dG_dt_atTime0 );
					}
					else {
						ogq->copy( zeroTrace );
					}
				}
*/
			}
			else {
				// COMPUTE OC which will fill in traceOC
				experiments[e]->computeOC( expP.getPtrD() );

				// POLYFIT in preparation for slerping below
				experiments[e]->traceOC.polyFit();
			}


			//========================================================================================
			// OUTLINE 05.  evaluate the error to get testE
			//========================================================================================
			// FOREACH observable...
			for( o=0; o<exObsCount; o++ ) {
				int dataCount = experiments[e]->measured[o]->cols;

				double testEForThisEO = 0.0;

				// FOREACH time sample...
				for( i=0; i<dataCount; i++ ) {
					double time = experiments[e]->measured[o]->getTime(i);
					double data = experiments[e]->measured[o]->getData(i,0);
					
					// FETCH the observable value at time by interpolation 
					double obsVal = experiments[e]->traceOC.getElemSLERP( time, o );

					// ACCUM e (square error)
					double residual = obsVal - data;
					testE += residual * residual;
					testEForThisEO += residual * residual;

					// Note the subtle "eo+o" in the next line.  @TODO Document this better
					testMaxError.setD( eo+o, 0, testEForThisEO );
					if( fitThreadLoopCount > 1 && testEForThisEO > currMaxError.getD(eo+o,0) * 100.0 ) {
						// Something has caused the integrator to go so far off track
						// that the error term is way off compared to the maximum error in the last iteration
						// Therefore we wish to invoke the deltaQ halving logic by setting testE = DBL_MAX
						offTheRails = 1;
						
						// break 2 levels with goto
						goto escape;
					}
				}
			}

			escape:;
			
			if( offTheRails || testE >= currE ) {
				worse = 1;

				//========================================================================================
				// OUTLINE 06.  if test is worse than curr then the step was too big
				// OUTLINE 07.	progressively eliminate lower SNR search direcitons
				//========================================================================================

				// See "Progressive elimination of poor search directions" in Subtle issues above
				// @TODO Document this tricky code. For example, why 512.0 below?
				for( q=qCount-1; q>=0; ) {

					if(
						testGradInEigenSpaceRejectFlag.getI(q) == 0 
						&& (
							q==qCount-1
							|| testGradInEigenSpaceRejectFlag.getI(q+1) == 1 
						)
					) {

						double s2t = currS.getD(q,0) * pow( 2.0, (double)(trapCount+1) );

						if( s2t > 512.0 * currS.getD(q,0) ) {
							testGradInEigenSpaceScaled.putD( q, 0, 0.0 );
							testGradInEigenSpaceRejectFlag.putI( q, 0, 1 );
							trapCount = 0;
							q--;
							if( q < 0 ) {
								trapped = 1;
								break;
							}
							continue;
						}
						testGradInEigenSpaceScaled.setD( q, 0, currGradInEigenSpace.getD(q,0) / s2t );
						break;
					}
					q--;
				}

				zmatMul( currVtt, testGradInEigenSpaceScaled, deltaQ );
				zmatAdd( currQ, deltaQ, testQ );

				// CHECK for trap 
				const int trapEscape = 20;
				if( trapCount++ > trapEscape ) {
					trapped = 1;
				}
				
				// BREAK out of the experiment loop
				goto out_of_experiments_loop;
			}
			else {
				//========================================================================================
				// OUTLINE 09.  otherwise test was better than curr
				// OUTLINE 10.  integrate H to get traceG[] for each parameter
				//========================================================================================

				timingIntegrateHCurrStop = 0.0;
				timingIntegrateHCurrStart = zTimeNow();

				integrator->allocTraceG();

				for( q=0; q<qCount; q++ ) {
					// INTEGRATE H wrt q
					// Note that integration is only necessary wrt to parameters that are actually
					// relevant to this experiment.  For example, experiment 1 can not change wrt to 
					// the initial conditions of experiment 1.

					if( parameterInfo[q].isRelevantToExperiment(e) ) {
						if( parameterInfo[q].fitFlag && parameterInfo[q].type!=PI_OBS_CONST ) {
							if( exIsEquilibrium ) {
								// FORCE integrator to only evalulate initial conditions
								experiments[e]->mixsteps[0].duration = DBL_EPSILON;
							}
							
							// SETUP the initial conditions of G
							for( int j=0; j<reagentCount(); j++ ) {
								icG[j] = 0.0;
								if( parameterInfo[q].type == PI_INIT_COND && parameterInfo[q].experiment == e && parameterInfo[q].reagent == j ) {
									icG[j] = 1.0;
								}
							}

							// @TODO Deal with multiple mixsteps
							integrator->integrateH( parameterInfo[q].pIndex, icG, expP.getPtrD(), experiments[e]->mixsteps[0].duration );
						}
						else {
							// For parameters that are not being fit or are observable constants, G is all zero
							integrator->traceG[ parameterInfo[q].pIndex ]->zeroInit( reagentCount() );
						}
					}
				}

				// COPY the integrators's traceG into the experiment traceG
				experiments[e]->traceG.clear();
				for( q=0; q<qCount; q++ ) {
					if( parameterInfo[q].isRelevantToExperiment(e) ) {
						KineticTrace *kt = new KineticTrace;
						kt->copy( *integrator->traceG[ parameterInfo[q].pIndex ] );
						experiments[e]->traceG.add( kt );
					}
				}

				// LATCH timers
				timingIntegrateHCurrStop = zTimeNow();
				timingIntegrateDTotal += timingIntegrateDCurrStop - timingIntegrateDCurrStart;
				timingIntegrateHTotal += timingIntegrateHCurrStop - timingIntegrateHCurrStart;

				// ALLOC the matrix which will hold the FFT results of the residuals
				experiments[e]->observableResidualFFTMat.alloc( exObsCount, 64, zmatF64 );
				
				double expMeasuredDuration = experiments[e]->measuredDuration();
				KineticVMCodeOG *vmog = experiments[e]->vmog;
				
				//========================================================================================
				// OUTLINE 11.  for each observable ...
				//========================================================================================
				for( o=0; o<exObsCount; o++ ) {
					int dataCount = experiments[e]->measured[o]->cols;

					//========================================================================================
					// OUTLINE 12.  for each data point ...
					//========================================================================================

					// FOREACH time sample...
					for( i=0; i<dataCount; i++ ) {
						double time = experiments[e]->measured[o]->getTime(i);
						double data = experiments[e]->measured[o]->getData(i,0);
						
						// FETCH the observable value at time by interpolation 
						double obsVal = experiments[e]->traceOC.getElemSLERP( time, o );

						// ACCUM e (square error)
						double residual = obsVal - data;

						//========================================================================================
						// OUTLINE 13. compute OG at this point
						//========================================================================================
						memset( OG, 0, sizeof(double) * qCount );
						for( q=0; q<qCount; q++ ) {
							// SLERP to get C at this point
							experiments[e]->traceC.getColSLERP( time, slerpC );
							
							// SLERP to get G at this point
							if( parameterInfo[q].isRelevantToExperiment(e) && parameterInfo[q].fitFlag ) {
								experiments[e]->traceG[ parameterInfo[q].pIndex ]->getColSLERP( time, slerpG );
							}
							else {
								memset( slerpG, 0, sizeof(double) * reagentCount() );
							}

							// EVALUATE OG at this point
							if( parameterInfo[q].fitFlag ) {
								vmog->execute_OG( o, q, slerpC, expP.getPtrD(), slerpG, &OG[q] );
							}
						}

						//========================================================================================
						// OUTLINE 14.  accumulate gradient, hessian, co-varaince
						//========================================================================================
						for( int q0=0; q0<qCount; q0++ ) {

							// ACCUMULATE the gradient
							double derivObsAtTimeWrtQ0 = OG[q0];
							assert( q0*exObsCount+o < expGrad.rows );
							*expGrad.getPtrD(q0+o*qCount,0) += 2.0 * residual * derivObsAtTimeWrtQ0;

							for( q1=0; q1 < qCount; q1++ ) {
								double derivObsAtTimeWrtQ1 = OG[q1];
								assert( q0*exObsCount+o < expHess.rows );
								assert( q1 < expHess.cols );

								// ACCUMULATE the hessian
								*expHess.getPtrD(q0+o*qCount,q1) += 2.0 * derivObsAtTimeWrtQ0 * derivObsAtTimeWrtQ1;

								// ACCUMULATE the unscaled co-variance
								*expCovr.getPtrD(q0+o*qCount,q1+o*qCount) += derivObsAtTimeWrtQ0 * derivObsAtTimeWrtQ1;
							}
						}
					}

					//========================================================================================
					// OUTLINE 16.  fft the residuals to compute noise specturm
					//========================================================================================
					double variance = 0.0;

					if( exIsEquilibrium ) {
						// See note above that the deriv and values of equlibirum are the same
						double dO_dt = experiments[e]->traceOC.getDerivColPtr(0)[o];
						variance = dO_dt * dO_dt;
					}
					else {
						// SAMPLE the function at 128 evenly spaced intervals for FFT
						double t = 0.0;
						double tStep = expMeasuredDuration / 128.0;
						for( i=0; i<128; i++, t += tStep ) {
							equallySpacedResidual[i] = experiments[e]->traceOC.getElemSLERP( t, o ) - experiments[e]->measured[o]->getElemSLERP( t, 0 );
						}

						// FFT this 128 real valued trace
						FFTGSL fft;
						fft.fftD( equallySpacedResidual, 128 );

						// COMPUTE the power spectrum of the fft (the whole thing for viewing)
						for( i=0; i<64; i++ ) {
							double real, imag;
							fft.getComplexD( i, real, imag );

							// STORE the power spectrum into the observableResidualFFTMat for later observation
							experiments[e]->observableResidualFFTMat.putD( o, i, sqrt( real*real + imag*imag ) );

							// ACCUMULATE the variance of the top-half of the specturm for noise estimation
							if( i >= 31 ) {
								variance += real*real + imag*imag;
							}
						}

						// CONVERT to per-point variance
						variance /= 32.0;
					}

					// SCALE the covariance matrix by the variance
					for( q0=0; q0 < qCount; q0++ ) {
						for( q1=0; q1 < qCount; q1++ ) {
							*expCovr.getPtrD(q0+o*qCount,q1+o*qCount) *= variance;
						}
					}
				}

				// INCREMENT eo by the number of observables for this experiment 
				eo += exObsCount;

				//========================================================================================
				// OUTLINE 18.  stack the hessian and covariance.  See "Stacking" in the SUBTLE ISSUES
				//========================================================================================
				int beginStackRow = stackRow;
				for( q0=0; q0<qCount*exObsCount; q0++ ) {
					testGrad.setD( stackRow, 0, expGrad.getD( q0, 0 ) );

					// STACK the hessian with the observables from this experiment
					for( q1=0; q1 < qCount; q1++ ) {
						testHess.setD( stackRow, q1, expHess.getD( q0, q1 ) );
					}

					// STACK the covariaince matrix in offet blocks (tricky diagonal block structure)
					for( q1=0; q1 < qCount; q1++ ) {
						testCovr.setD( stackRow, q1+beginStackRow, expCovr.getD( q0, q1 ) );
					}

					stackRow++;
				}
			}
		}
		
		out_of_experiments_loop:;
		
		//========================================================================================
		// OUTLINE 21.	copy all the variables to make them visible to renderer thread
		//========================================================================================
		renderVariablesLock();
		for( e=0; e<eCount(); e++ ) {
			experiments[e]->copyRenderVariables();
		}		
		renderVariablesUnlock();

		//========================================================================================
		// OUTLINE 22.	test for acceptance and convergence
		//========================================================================================
		double qSquared = zmatDot( currQ, currQ );
		double deltaQSquared = zmatDot( deltaQ, deltaQ );

		// TEST for convergence
		fitThreadWorse = worse;
		if( fitThreadLoopCount > 1 && deltaQSquared < 1e-14 * qSquared ) {
			// @TODO: What are the right values for these epsilons?
			converged = 1;
		}
		else if( !worse && !cancelled ) {
			//========================================================================================
			// OUTLINE 23.	if( accepted ) {
			// OUTLINE 24.		currX = testX
			//========================================================================================

			currQ.copy( testQ );
			currE = testE;
			currMaxError.copy( testMaxError );
			currGrad.copy( testGrad );
			currHess.copy( testHess );

			// ACCUMULTE the trajectory into qHistory
			ZMat errorMat( 1, 1, zmatF64 );
			errorMat.putD( 0, 0, currE );
			ZMat trajectory( currQ );
			trajectory.growRow( errorMat );
			qHistory.growCol( trajectory );
			
			// RESET trapCount
			trapCount = 0;

			//========================================================================================
			// OUTLINE 25.  compute new testX.  See "Computing Step with SVD" in SUBTLE ISSUES
			//========================================================================================

			// CONVERT testGrad into the actual gradient by negating
			for( q=0; q<testGrad.rows; q++ ) {
				*testGrad.getPtrD(q) *= -1.0;
			}

			// SVD the Hessian: H = U * S * Vt
			zmatSVD_GSL( testHess, currU, currS, currVt );
			zmatTranspose( currU, currUt );

			// TRANSFORM the covariance into the eigen space
			zmatMul( currUt, testCovr, currUtCovr );
			zmatMul( currUtCovr, currU, currCovrInEigenSpace );
			assert( currCovrInEigenSpace.rows == qCount && currCovrInEigenSpace.cols == qCount );
			
			// EXTRACT the diagonal elements out of the eigen covariance to get the noise, store off in member variable fitNoise
			zmatDiagonalMatrixToVector( currCovrInEigenSpace, currNoise );

			// TRANSFORM the gradient vector into eigen space
			zmatMul( currUt, testGrad, currGradInEigenSpace );

			// For now assert that they are all positive
			for( q=0; q<qCount; q++ ) {
				if( currS.getD(q,0) < 0.0 ) {
					// @TODO surgery on the negative singular values?
					assert( 0 );
				}
				assert( currS.getD(q,0) >= 0.0 );
			}

			// TRANSFORM currQ into eigen space
			zmatMul( currVt, currQ, currSignal );

			// SCALE gradient by singular values
			assert( qCount == currS.rows && qCount == currGradInEigenSpace.rows );
			currGradInEigenSpaceScaled.copy( currGradInEigenSpace );
			testGradInEigenSpaceRejectFlag.alloc( currGradInEigenSpaceScaled.rows, 1, zmatU32 );
			currSNR.alloc( qCount, 1, zmatF64 );
			for( q=0; q<qCount; q++ ) {
				currSNR.putD(q,0,0.0);
				if( currS.getD(q,0) > 0.0 ) {
					*currNoise.getPtrD(q,0) /= currS.getD(q,0) * currS.getD(q,0);
					currSNR.putD( q, 0, fabs( currSignal.getD(q,0) / currNoise.getD(q,0) ) );
				}
				else {
					// @TODO: Not clear this is needed.  The renderer should prbably start using the new variable currSNR
					*currNoise.getPtrD(q,0) = 0.0;
				}

				if( q < qBeingFitCount && currSNR.getD(q,0) > 1.0 ) {
					// Because the s vec is sorted it automatically means
					// that those q's which are not being fit are at the bottom
					// thus it is OK to compare q to qBeingFitCount
					*currGradInEigenSpaceScaled.getPtrD(q,0) /= currS.getD(q,0);
				}
				else {
					testGradInEigenSpaceRejectFlag.putI( q, 0, 1 );
					*currGradInEigenSpaceScaled.getPtrD(q,0) = 0.0;
					*currNoise.getPtrD(q,0) = -1.0;
						// -1 is a flag to say that the noise is out of bounds
				}
			}
			
			testGradInEigenSpaceScaled.copy( currGradInEigenSpaceScaled );

			// TRANSFORM the scaled gradient from eigen space back into parameter space to get deltaQ
			zmatTranspose( currVt, currVtt );
			zmatMul( currVtt, currGradInEigenSpaceScaled, deltaQ );

			// ADD deltaQ to currQ to get the new test point testQ
			zmatAdd( currQ, deltaQ, testQ );

			// ASSERT that parameters that are not being fit should have no deltaQ
			for( q=0; q<qCount; q++ ) {
				if( ! parameterInfo[q].fitFlag ) {
					assert( fabs(deltaQ.getD(q,0)) < 4.0 * FLT_EPSILON );
				}
			}
		}

		timingCurrentLoopStop = zTimeNow();
		timingTotal += timingCurrentLoopStop - timingCurrentLoopStart;
	}

	delete integrator;
	delete vmd;
	delete vmh;

	if( cancelled ) {
		fitThreadStatus = KE_DONE_CANCELLED;
	}
	else if( converged ) {
		fitThreadStatus = KE_DONE_CONVERGED;
	}
	else if( trapped ) {
		fitThreadStatus = KE_DONE_TRAPPED;
	}
	else if( singleStep && fitThreadLoopCount == 1 ) {
		fitThreadStatus = KE_DONE_SINGLE_STEP;
	}
	else {
		assert( ! "Fit thread Unknown stop reason" );
	}

	if( fitThreadSendMsg ) {
		zMsgQueue( fitThreadSendMsg );
	}
}


#endif


