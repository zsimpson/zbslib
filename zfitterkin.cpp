// @ZBS {
//		*MODULE_NAME ZFitter
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Am SVD gradient descent parameter fitter
//		}
//		*PORTABILITY win32 unix maxosx
//		*REQUIRED_FILES zfitterkin.cpp zfitterkin.h
//		*VERSION 1.0
//		+HISTORY {
//			1.0 ZBS Jul 2008, based on earlier prototypes
//			2.0 ZBS Jun 2009, Refactoring to separate the fitter into an abstract class
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "math.h"
#include "float.h"
// MODULE includes:
#include "zfitterkin.h"
// ZBSLIB includes:
#include "fftgsl.h"
#include "kineticbase.h"
#include "ztime.h"

ZFitterKin::ZFitterKin() {
	totalObsCount = 0;
	vmd = 0;
	vmh = 0;
	integrator = 0;
	minStepSizeC.clear();
	minStepSizeG.clear();

	slerpC = 0;
	slerpG = 0;
	icG = 0;
	dC_dt = 0;
	OG = 0;
	equallySpacedResidual = 0;
}

void ZFitterKin::clear() {
	ZFitter::clear();

	system.clear();

	totalObsCount = 0;
	if( vmd ) delete vmd; vmd = 0;
	if( vmh ) delete vmh; vmh = 0;
	if( integrator ) delete integrator; integrator = 0;
	minStepSizeC.clear();
	minStepSizeG.clear();

	if( slerpC ) delete slerpC; slerpC = 0;
	if( slerpG ) delete slerpG; slerpG = 0;
	if( icG ) delete icG; icG = 0;
	if( dC_dt ) delete dC_dt; dC_dt = 0;
	if( OG ) delete OG; OG = 0;
	if( equallySpacedResidual ) delete equallySpacedResidual; equallySpacedResidual = 0;
}

void ZFitterKin::start( int qCount, double *qStart, int *qFitFlags ) {
	assert( !"Unable to start a ZFitterKin by the polymorphic method. Call start with KineticSystem" );
}

void ZFitterKin::start( KineticSystem &_system ) {
	system.copy( _system );
	
	int reagentCount = system.reagentCount();

	// ALLOCATE minStepSize
	minStepSizeC.alloc( system.eCount(), 1, zmatF64 );
	minStepSizeG.alloc( system.eCount(), 1, zmatF64 );

	// COUNT the total number of observables for all experiments and COMPILE observable expresisons
	totalObsCount = 0;
	for( int e=0; e<system.eCount(); e++ ) {
		totalObsCount += system.experiments[e]->observablesCount();
		system.experiments[e]->compileOC();
		system.experiments[e]->compileOG();
	}
	
	// FETCH the initial parameters from the system into currQ & propQ
	system.paramInitPQIndex();
	system.getQ( origQ );
	system.getQ( propQ );
	
	system.getQ( origQ );
	int qCount = origQ.rows;
	ZMat fitFlags( qCount, 1, zmatS32 );
	for( int q=0; q<qCount; q++ ) {
		fitFlags.putI( q, 0, system.parameterInfo[q].fitFlag ? 1 : 0 );
	}

	// CREATE virtual machines and integrator. This requires knowing the length of the P vector
	ZMat pVec;
	system.getP( origQ, 0, pVec );
	vmd = new KineticVMCodeD( &system, system.reactionCount() );
	vmh = new KineticVMCodeH( &system, pVec.rows );
	vmh->disassemble( "h" );
	integrator = new KineticIntegrator( KI_ROSS, vmd, 0, 0, vmh );
		// Note that the integrator is SHARED by all experiments.
		// So when the integrator is called, the Q vector has to be mapped to the P vector

	// ALLOC workspace	
	slerpC = new double[ reagentCount ];
	slerpG = new double[ reagentCount ];
	icG = new double[ reagentCount ];
	dC_dt = new double[ reagentCount ];
	OG = new double[ qCount ];
	equallySpacedResidual = new double[ 128 ];

	ZFitter::start( qCount, origQ.getPtrD(), fitFlags.getPtrI() );
}

void ZFitterKin::stop() {
	clear();
}

int ZFitterKin::step() {
	int ret = ZFitter::step();
	system.putQ( propQ );
	return ret;
}

void ZFitterKin::copy( ZFitter *srcFitter ) {
	ZFitter::copy( srcFitter );
	ZFitterKin *zfk = (ZFitterKin *)srcFitter;
	system.copy( zfk->system );
}


void ZFitterKin::propEval( ZMat &in_q, double &out_sse, int &out_offTheRails ) {
	out_sse = 0.0;
	out_offTheRails = 0;

	int stackRow = 0;
		// Indexes into the testGrad and testHess for stacking (see "subtle issues" above)

	int eo = 0;
		// eo tracks the experiment and observable index. Eg (ex 0 has 1 obs and ex 1 has 2 obs then eo goes:0,1,2)
		
	int eCount = system.eCount();
	int reactionCount = system.reactionCount();

	for( int e=0; e<eCount; e++ ) {
		// @TODO: All of this code should go into a function and then this code
		// should launch a thread for each experiment doing the following
		// when all of those threads are complete then it sums up the errors from each into propE
	
		int exIsEquilibrium = system.experiments[e]->equilibriumExperiment;
		int exObsCount = system.experiments[e]->observablesCount();
			// The number of observables that THIS experiment has

		// CONVERT to local address space on the parameters (FETCH P[e] from Q)
		ZMat expP;
		system.getP( in_q, e, expP );

		// @TODO Deal with multiple mixsteps
		if( exIsEquilibrium ) {
			// FORCE the integrator to only evaluate the initial condition
			// so that we can get the dC_dt at time 0
			system.experiments[e]->mixsteps[0].duration = DBL_EPSILON;
		}

		// FETCH the initial conditions
		int obsConstCount = 0;
		system.paramGet( PI_OBS_CONST, &obsConstCount, -1, 0 );
		double *ic = expP.getPtrD(reactionCount+obsConstCount,0);
		double minStep = 0.01 * minStepSizeC.getD( e, 0 );
			// minStep tells the integrator to stop if the step is 1/100th of the smallest value
			// seen during the first run of the integrator.  This is a sanity check to prevent tings from going haywire.
		
		// INTEGRATE D to get traceC
		int ok = integrator->integrateD( ic, expP.getPtrD(), system.experiments[e]->simulationDuration(0), 0.0, minStep );
		if( !ok ) {
			// The integration was stopped because the min step was exceeded
			// Skip straight to aborting this testQ
			out_offTheRails = 1;
			return;
		}
		
		// STORE the minimum step size on the first time through
		minStepSizeC.putD( e, 0, integrator->getMinStepSizeC() );
		
		// MOVE the integrator's traces over to the fitter system
		system.experiments[e]->traceC.copy( integrator->traceC );
		
		// POLYFIT for later slerping
		system.experiments[e]->traceC.polyFit();

		// COMPUTE OC which will fill in traceOC
		system.experiments[e]->computeOC( expP.getPtrD() );

		// POLYFIT in preparation for slerping below
		system.experiments[e]->traceOC.polyFit();

		// FOREACH observable...
		for( int o=0; o<exObsCount; o++ ) {
			int dataCount = system.experiments[e]->measured[o]->cols;

			double testEForThisEO = 0.0;

			// FOREACH time sample...
			for( int i=0; i<dataCount; i++ ) {
				double time = system.experiments[e]->measured[o]->getTime(i);
				double data = system.experiments[e]->measured[o]->getData(i,0);
				
				// FETCH the observable value at time by interpolation 
				double obsVal = system.experiments[e]->traceOC.getElemSLERP( time, o );

				// ACCUM e (square error)
				double residual = obsVal - data;
				out_sse += residual * residual;
			}
		}
	}
}


void ZFitterKin::origEval( ZMat &in_q, ZMat &out_origGrad, ZMat &out_origHess, ZMat &out_origCovr ) {
	ZTime startTime = zTimeNow();

	int i, q, q0, q1;

	// COMPUTE all the expensive terms for the gradient, hessian, and covariance for this new origin
	// Returns 1 if we're converged
	
	int qCount = in_q.rows;

	out_origGrad.alloc( totalObsCount * qCount, 1, zmatF64 );
	out_origHess.alloc( totalObsCount * qCount, qCount, zmatF64 );
	out_origCovr.alloc( totalObsCount * qCount, totalObsCount * qCount, zmatF64 );
		// The gradient, hessian, and covariance for current evaluation point

	int stackRow = 0;
		// Indexes into the testGrad and testHess for stacking (see "subtle issues" above)

	int eo = 0;
		// eo tracks the experiment and observable index. Eg (ex 0 has 1 obs and ex 1 has 2 obs then eo goes:0,1,2)

	int eCount = system.eCount();
	int reagentCount = system.reagentCount();

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
	for( int e=0; e<eCount; e++ ) {
		int exIsEquilibrium = system.experiments[e]->equilibriumExperiment;
		int exObsCount = system.experiments[e]->observablesCount();
			// The number of observables that THIS experiment has

		// CONVERT to local address space on the parameters (FETCH P[e] from Q)
		ZMat expP;
		system.getP( in_q, e, expP );

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

			if( system.parameterInfo[q].isRelevantToExperiment(e) ) {
				if( system.parameterInfo[q].fitFlag && system.parameterInfo[q].type!=PI_OBS_CONST ) {

					if( exIsEquilibrium ) {
						// FORCE integrator to only evalulate initial conditions
						system.experiments[e]->mixsteps[0].duration = DBL_EPSILON;
					}

					// SETUP the initial conditions of G
					for( int j=0; j<reagentCount; j++ ) {
						icG[j] = 0.0;
						if( system.parameterInfo[q].type == PI_INIT_COND && system.parameterInfo[q].experiment == e && system.parameterInfo[q].reagent == j ) {
							icG[j] = 1.0;
						}
					}
					
					// @TODO Deal with multiple mixsteps
					// @TODO: Track minStep have an offtherails here					
					integrator->integrateH( system.parameterInfo[q].pIndex, icG, expP.getPtrD(), system.experiments[e]->simulationDuration(0) );
				}
				else {
					// For parameters that are not being fit or are observable constants, G is all zero
					integrator->traceG[ system.parameterInfo[q].pIndex ]->zeroInit( reagentCount );
				}
			}
		}

		// COPY the integrators's traceG into the experiment traceG
		system.experiments[e]->traceG.clear();
		for( q=0; q<qCount; q++ ) {
			if( system.parameterInfo[q].isRelevantToExperiment(e) ) {
				KineticTrace *kt = new KineticTrace;
				kt->copy( *integrator->traceG[ system.parameterInfo[q].pIndex ] );
				system.experiments[e]->traceG.add( kt );
			}
		}

		// ALLOC the matrix which will hold the FFT results of the residuals
		system.experiments[e]->observableResidualFFTMat.alloc( exObsCount, 64, zmatF64 );
		
		double expMeasuredDuration = system.experiments[e]->measuredDuration();
		KineticVMCodeOG *vmog = system.experiments[e]->vmog;
		
		// FOREACH observable in experiment e
		for( int o=0; o<exObsCount; o++ ) {
			int dataCount = system.experiments[e]->measured[o]->cols;

			// FOREACH time sample...
			for( i=0; i<dataCount; i++ ) {
				double time = system.experiments[e]->measured[o]->getTime(i);
				double data = system.experiments[e]->measured[o]->getData(i,0);
				
				// FETCH the observable value at time by interpolation 
				double obsVal = system.experiments[e]->traceOC.getElemSLERP( time, o );

				// ACCUM e (square error)
				double residual = obsVal - data;

				// COMPUTE OG at this point
				memset( OG, 0, sizeof(double) * qCount );
				for( q=0; q<qCount; q++ ) {
					// SLERP to get C at this point
					system.experiments[e]->traceC.getColSLERP( time, slerpC );
					
					// SLERP to get G at this point
					if( system.parameterInfo[q].isRelevantToExperiment(e) && system.parameterInfo[q].fitFlag ) {
						system.experiments[e]->traceG[ system.parameterInfo[q].pIndex ]->getColSLERP( time, slerpG );
					}
					else {
						memset( slerpG, 0, sizeof(double) * reagentCount );
					}

					// EVALUATE OG at this point
					int actualObsCount = system.experiments[e]->observableInstructions.count;
					if( exIsEquilibrium && o >= actualObsCount ) {
						OG[q] = 0.0;
						if( system.parameterInfo[q].isRelevantToExperiment(e) ) {
							// The OG for an equilibrium experiment is really H at time 0
							double *H_atTime0 = integrator->traceG[ system.parameterInfo[q].pIndex ]->getDerivColPtr( 0 );
							OG[q] = H_atTime0[ o - actualObsCount ];
						}
					}
					else {
						if( system.parameterInfo[q].fitFlag ) {
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
				variance = system.experiments[e]->equilibriumVariance.getD(o,0);
			}
			else {
				// SAMPLE the function at 128 evenly spaced intervals for FFT
				double t = 0.0;
				double tStep = expMeasuredDuration / 128.0;
				for( i=0; i<128; i++, t += tStep ) {
					equallySpacedResidual[i] = system.experiments[e]->traceOC.getElemSLERP( t, o ) - system.experiments[e]->measured[o]->getElemSLERP( t, 0 );
				}

				// FFT this 128 real valued trace
				FFTGSL fft;
				fft.fftD( equallySpacedResidual, 128 );

				// COMPUTE the power spectrum of the fft (the whole thing for viewing)
				for( i=0; i<64; i++ ) {
					double real, imag;
					fft.getComplexD( i, real, imag );

					// STORE the power spectrum into the observableResidualFFTMat for later observation
					system.experiments[e]->observableResidualFFTMat.putD( o, i, sqrt( real*real + imag*imag ) );

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
			out_origGrad.setD( stackRow, 0, expGrad.getD( q0, 0 ) );

			// STACK the hessian with the observables from this experiment
			for( q1=0; q1 < qCount; q1++ ) {
				out_origHess.setD( stackRow, q1, expHess.getD( q0, q1 ) );
			}

			// STACK the covariaince matrix in offet blocks (tricky diagonal block structure)
			for( q1=0; q1 < qCount; q1++ ) {
				out_origCovr.setD( stackRow, q1+beginStackRow, expCovr.getD( q0, q1 ) );
			}

			stackRow++;
		}
	}
}

