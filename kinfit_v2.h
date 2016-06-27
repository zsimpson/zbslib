#ifndef KINFIT_V2_H
#define KINFIT_V2_H

// This exists because KineticExperiment contains a member fitDataContext.  This can go away
// once the transition to v3 fitting is complete.  It has been pulled into zbslib only
// because all of the KinExperiment stuff was pulled into KineticExperiment.

#include "fitdata.h"

#define FDC_FITOBSERVABLES_MAX	 (32)	// must be >= KINEXPERIMENT_OBSERVABLES_MAX
#define FDC_FITOUTPUTFACTORS_MAX (32)	
#define FDC_FITSTEPS_MAX		 (2048)	// must be >= KINEXPERIMENT_STEPS_MAX

struct KineticTrace;
struct KineticExperiment;

struct fitDataContext {
/*
	A fitDataContext points to an experiment whose experimental data, or simulated
	observable traces, are being fit.  The members consist mainly of pointers to
	various data used during the fit to allow other routines to easily access
	the data needed for fitting, without poking around inside the experiment
	looking for data based on various fitting options.

*/

	int observablesCount;
		// added in process of removing exp pointer: how many possible observables?
	int fitObservable[ FDC_FITOBSERVABLES_MAX ];
		// which observables to fit?  (1==fit, 0==not)
	int fitOutputFactor[ FDC_FITOUTPUTFACTORS_MAX ];
		// which output factors (observable constants) from exp should be fit?
	int	numSteps[ FDC_FITOBSERVABLES_MAX ];
		// how many time steps exist for this data to fit to.
	int stepOffset[ FDC_FITOBSERVABLES_MAX ];
		// experimental: offset into time/data mats to handle fitting subset of data (mixsteps)
	double time0;
		// 01.18.10 : the fit functions really need to know the time at which the mixstep
		// begins - the step offset above tells us where to find the first datapoint, but
		// it will often not be exactly at t=0 for the mixstep - there is usually some dead time
	double time1;
		// similarly, then end of the section of the function we are fitting - though the last
		// datapoint may occur before this.
	
	//=========================================================================================
	// This section tells which which data will be fit -- from here we can retrieve the time
	// and data value for the data associated with any observable (whether sim or measured)

	// In v1, you were pointing the ZMat* for time and data to either the simTime/simOBs trace
	// within the experiment (when fitting to simulated traces) or to the expRefData matrix
	// for the given obs.  

	// In v2, the calling code could directly reference either KinExperiment2::observables
	// for a simulated fit, or KinExperiment2::measured for fit to measured data.
	// But a more straightforward approach for the porting would be:

KineticTrace * dataToFit[ FDC_FITOBSERVABLES_MAX ];
int dataRow[ FDC_FITOBSERVABLES_MAX ];
	// we still actually need this row index because in the context of some 
	// fitting functions we don't really know if the dataToFit is measured data, in which
	// case it is only in row 0, or if it is simulated data, in which case all observables
	// are in the same KineticTrace...

	//=========================================================================================
	// output: the result of integrating with the current parameter values is stored here.
	// The render thread looks at this to display the best fit traces.
	
	KineticTrace * evalTraces[ FDC_FITOBSERVABLES_MAX ];

//	double functionTrace[ FDC_FITOBSERVABLES_MAX ][ FDC_FITSTEPS_MAX ];								
		// the curves that were generated using the best fit parameters

//	ZMat aFitToSimulatedObsTimeTrace;
		// to support twiddling a model and still rendering fits of this type accurately
		// while doing so, the time trace of the experiment is copied here in DataFitComplete.

	// All of the above could be replaced by:
	// KineticTrace functionTrace;
	
	fitDataContext() {
		memset( evalTraces, 0, sizeof( evalTraces ) );
		clear();
	}
	
	void clearEvalTraces();
	
	void clear( int clearFitOutputFactors=1 ) {
		observablesCount = 0;
		memset( numSteps, 0, sizeof(numSteps) );
		memset( stepOffset, 0, sizeof(stepOffset) );
		time0 = 0.0;
		time1 = 0.0;
		memset( fitObservable, 0, sizeof( fitObservable ) );
		if( clearFitOutputFactors )
			memset( fitOutputFactor, 0, sizeof( fitOutputFactor ) );
		memset( dataToFit, 0, sizeof(dataToFit) );
		memset( dataRow, 0, sizeof( dataRow ) );
		clearEvalTraces();
		//		memset( functionTrace, 0, sizeof( functionTrace ) );
//		aFitToSimulatedObsTimeTrace.clear();
	}

	int setupFitForExperiment( int typeOfDataToFit, KineticExperiment *e );
		// setup our data members based on the passed experiment, return total
		// number of data points to fit for this experiment (for all fitted observables)
};

//---------------------------------------------------------------------------------------

struct fitDataContextSingle {
	// This struct created to simplify port of analytic GSL callbacks.
	// It allows the analytic functions to operate as before on a single
	// curve, and avoid all of the complexity associated with global fit
	// and multiple observables, which aren't really applicable in their case.

	int	numSteps;
		// how many time steps exist for this data to fit to.
	int stepOffset;
		// offset into time/data mats to allow fitting a subset of data (mixsteps)
	double time0;
		// 01.18.10 : the fit functions really need to know the time at which the mixstep
		// begins - the step offset above tells us where to find the first datapoint, but
		// it will often not be exactly at t=0 for the mixstep - there is usually some dead time
	double time1;
	FitData *fd;
		// this is a hack so we have access to any forced normalization type
		// for the fit.
	int observableIndex;
		// easy access to the experiment observable index we're fitting
	
	KineticTrace *dataToFit;
	int dataRow;

	KineticTrace *results;
		// where to store results of best fit trace
	
	void *errVec;
		// Added for Simplex-based fitting using the gsl minimizer framework in which
		// we need to provide an error vector to the existing fitting function callbacks.
		// This gets alloc'd and used only by this minimizer-based fitting. tfb jan2013

	void *fitterJacobian;
		// Added to keep a handy pointer into fitter-allocated workspace, e.g. Levmar.  This
		// is not owned by us, is not copied, and its use depends on implementation of the
		// jacobian callback for the particular fitter being used.
	
	int evalFuncOnly;
		// if non-zero, fit-function will only evaluate itself at given params and fill
		// results, but not compute errors etc...  Used to compute a fitted trace with
		// potentially more steps than actual data points.


	private: fitDataContextSingle() {}
		// only allow construction from a fitDataContext.

	public: fitDataContextSingle( fitDataContext &fdc, int _steps, FitData *_fd ) {
		evalFuncOnly = 0;
		numSteps = _steps;
		fd = _fd;
		time0 = fdc.time0;
		time1 = fdc.time1;
		int i;
		for( i=0; i<FDC_FITOBSERVABLES_MAX; i++ ) {
			if( fdc.fitObservable[i] ) {
				stepOffset = fdc.stepOffset[i];
				dataToFit  = fdc.dataToFit[i];
				dataRow    = fdc.dataRow[i];
				results    = fdc.evalTraces[i];
				observableIndex = i;
				break;
			}
		}
		assert( i<FDC_FITOBSERVABLES_MAX );
	}
};

#endif
