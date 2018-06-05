// @ZBS {
//		*MODULE_OWNER_NAME kineticbase
// }

#ifndef KINETICBASE_H
#define KINETICBASE_H

// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
// MODULE includes:
// ZBSLIB includes:
#include "zmat.h"
#include "zhashtable.h"
#include "ztl.h"
#include "zintegrator.h"
#ifdef KIN_MULTITHREAD
	#include "pmutex.h"
#endif
#ifdef USE_KINFIT_V2
// @ZBSIF extraDefines( 'USE_KINFIT_V2' )
// the above is for the perl-parsing of files for dependencies; we don't
// want the dependency builder to see these includes if USE_KINFIT_V2 is not
// defined.
	#include "kinfit_v2.h"		// for fitDataContext, until v2 fitting is replaced with v3
	#include "fitdata.h"		// for FitData, until v2 fitting is replaced with v3
								// these are both members of KineticExperiment!
	extern int Kin_simSmoothing3;
		// use 3rd order interpolation instead of 5th
// @ZBSENDIF
#else
// during fitting, residuals may be normalized in various ways:
enum normalizeType { NT_Invalid = -1,
					 NT_None, 				// no normalization
					 NT_Sigma,				// known sigma per datapoint
					 NT_SigmaAvg,			// use single computed average value based on known sigma per datapoint
					 NT_SigmaAvgExternal,	// use single average value as specified during data import (e.g. from raw data file)
					 NT_SigmaAvgAFit,		// use estimated single value based on best fit to some analytic fn
					 NT_SigmaAvgAFitFamily, // use average of estimated values across a collection of traces, e.g. a series
					 NT_MaxData,				// use the max y-value of the data being fit, i.e. normalize all data to [0,1]
					 NT_NumTypes
};
#endif


// We start with a vector of concentrations at some time ta.  This may be thought of 
// as an initial condition.  In fact, when we start we have to have this vector 
// as an initial condition.  It is not a function of time, but is associated with some specific time.
//
// In addition we have parameters, P, and the right hand side of the reaction equaitons. 
// So, given a ta, C(ta) and the parameters, P we can calculate these right hand sides of 
// the reaction equations at ta.  These calcuations are at a given time ta but the time is constant.
//
// We now replace a vector, which the integrator calls the derivative of C with respect to 
// time with D, but it is really not a function of time it is just a vector at ta.
//
// We now call the differential equation integrator.  The differential equation integrator performs 
// its magic and produces a new C, at some different time tb.
//
// We can now restart the process from the beginning.
//
// The sequence of C(ti), i=1..n form TraceC



// NAMING CONVENTIONS
// -------------------------------------------------------------------------------------------------------------------------------
// a-z                        : Lowercase letters are scalars
// A-Z                        : Uppercase letters are vectors
// _                          : Underscore represents a differential as in dx/dt := dx_dt
//
// VARIABLES
// -------------------------------------------------------------------------------------------------------------------------------
// t                          : Time
// P                          : Parameter vector for integration consists of the rate constants followed by the initial conditions
// p                          : An element of P
// Q                          : Parameter vector for fitting which is a complicated set of elements from the P vectors of each experiment
// C                          : Concentration vector at some t
//
// DIFFERENTIALS
// -------------------------------------------------------------------------------------------------------------------------------
// dC_dt                      : The deriv of the concentration vector wrt to time.  This is the "base system" aka "time deriv"
// dC_dp                      : The deriv of the concentration vector wrt a specific parameter. aka "param deriv"
// traceC                     : The function C(t) found by integrating the ODE system dC_dt and interpolating between points
//
// d(dC_dt)_dp = d(dC_dp)_dt  : This is "the trick" of the whole fitting scheme which involves swapping differentiation order
//                            : so that we can have numerical differential without finite differencing. 
//                            : Interchanging the differential order is ok as long as the functions are continuous which they are.
//                            : During fitting we need dC_dp to determine delta steps in parameter space
//                            : and we can obtain it by integrating the *time-differential* equation d(dC_dp)_dt
//
// dC_dt = D                  : D is a shorthand for dC_dt
// dC_dp = G                  : G is a shorthand for dC_dp
// dD_dp = dG_dt = H          : Restatement of "the trick" from above and given the shorthand H
// traceG                     : The result of integrating the ODE system H (factored one p at a time)
//
// JACOBIANS
// -------------------------------------------------------------------------------------------------------------------------------
// dD_dC                      : The Jacobian of the base system. (Used to be call "J0")
// dD_dt = 0                  : Consquence of time invariance of D (Used during jacobian callback of D)
// dH_dG                      : The Jacobian of the param deriv (Used to call "J2").
// dH_dt                      : Didn't have this before
//
// INDEXES
// -------------------------------------------------------------------------------------------------------------------------------
// r                          : index to a reaction
// i, j                       : index to a reagent
// p, q                       : index to a parameter (fitter index not nec. same as integrator parameter index)



// KineticTrace
//--------------------------------------------------------------------------------------------------------------------------

struct KineticTrace {
	// This holds a set of concentrations associated with a time series
	// It permits for interpolation between the time points

	ZHashTable properties;
		// various info about the data, such as textual desc, scaling parameters

	int rows;
		// The number of elements the vector (not counting time)

	int cols;
		// Number of records used (count * width == num of doubles used)

	int colsAlloced;
		// Number of columns allocated (always >= cols)

	int last;
		// Last found time in find used for cache

	double *time;
		// Array of time points associated with each column

	double *rowCoords;
		// optional coordinates associated with each row; added to
		// allow data to be visualized in 3 dimensions if rows correspond
		// to numeric values.

	double *data;
		// Stored in col-major order (columns are memory contiguous)

	double *sigma;
		// every data point may have an associated sigma (stdev); optional

	double *derivs;
		// Optional time derivatives of each reagents at column time (col major) used for pokynomial fitting

	ZTLPVec<ZMat> polyMat;
		// For each reagent there is a 6 by cols-2 matrix that stores the polynomial fit
		// For each mat, the columns store the polynomial fit coefficents for the data point at that col


	double getTime( int i ) { if( i<0 || i >= cols ) return -1.0; else return time[i]; }
		// Fetch time on column i

	double getData( int i, int j ) { return data[i*rows+j]; }
		// Fetch data on column i, row j

	double getDeriv( int i, int j ) { return derivs[i*rows+j]; }
		// Fetch deriv on column i, row j

	double getSigma( int i, int j, normalizeType nt, double defaultVal=1.0 );
		// Fetch sigma on column i, row j
	
	double getSigmaAvg( int i, int j, int count=-1 );
		// average the sigma values specified
	
	normalizeType getSigmaBestNormalizeTypeAvailable();
		// return the "best" type of sigma available for this data

	double getLastTime() { return cols ? time[cols-1] : 0; }
		// Fetch time on last column

	double getLastData( int row ) { return cols ? getData( cols-1, row ) : 0; }
		// Fetch data on last column

	void addCol( double *vector );
		// Add column, where vector[0] is time.  Length of vector should be rows+1 (1 for time)

	void add( double _time, double *vector, double *derivs=0, double *sigma=0 );
		// Add a column where time is in a separate variable. size of vector should be rows

	int insertRange( KineticTrace &t, double t0, double t1, double dst0, double scale, double trans, double epsilon );
		// insert the values from t[t0,t1) into ourself at dst0, scaled by scale.
		// See below insertGapForRange that we call to do most of the work.

	int insertGapForRange( int count, double t0, double t1, double epsilon );
		// prepare the trace to receive count new data points in the time range [t0,t1].  Existing
		// data points that fall into this range will be lost.  This involves realloc of data, time.
		// Return the index (col) of the first location for the new data range.

	void init( int _rows, int _cols=-1, int withSigma=0, int withRowCoords=0 );
		// Init given rows (expandable later); _cols may be used to specify initial cols
		// or use default of 1024

	void zeroInit( int reagentCount );
		// Init but add a single col of all zeros

	void clear();
		// Free all memory, etc.

	void copy( KineticTrace &t );
		// Copy evertyhing, loses current data, reallocates

	void copyTime( KineticTrace &t );
		// Copies time only, as many entries as will fit in our cols.

	void copyRange( KineticTrace &t, double t0, double t1 );
		// Copy only values for time in range [t0,t1): loses current data, reallocates

	void copyRow(  KineticTrace &src, int srcRow, int dstRow, int copyTime );
		// Copy row from src to our data, optionally copying time.
		// We should be prealloc to match col-dims of src
	
	void copyDerivsToData( int row );
		// copies our own deriv to data and sets property to prevent polyFit on
		// that row.

	void stealOwnershipFrom( KineticTrace &src );
		// Copies over the data from src and replaces it with zeros

	int findClosestTimeCol( double time );
		// Used for interpolation, finds the closest column given the time
		// Bounded so it will return the last or first if you are later or earlier respecitvely.

	double getElemLERP( double time, int row );
		// Linear interpolation of reagent row at time

	void getColLERP( double time, double *_vector );
		// Linear interpolation of an entire column at time into the _vector array (must be rows in length)

	double *getColPtr( int timeIndex );
		// Fetch a pointer into the data at timeIndex

	double *getLastColPtr();
		// Fetch a pointer into the data at last time

	double *getDerivColPtr( int timeIndex );
		// Fetch a pointer into the derivs at timeIndex

	void set( int timeIndex, int reagentIndex, double val );
		// Explicitly change the value with val

	void set( int timeIndex, int reagentIndex, double _time, double val, double *_sigma=0 );
		// Explicitly change the time with _time, value with val, optional sigma

	void mul( ZMat &a, KineticTrace &b );
		// Left multiply a times b.
		// Matrix a is homoegnous so it has one mosre column than b has rows

	double scaleData( double scale );
		// scale the data by the passed value; return the cumulative scale,
		// which is also stored in our properties hash.

	void cubicFit();
	double getElemCubic( double time, int row );
	void getColCubic( double time, double *_vector );
		// analogs for polyFit using a cubic instead
	
	void polyFit();
		// Compute the coefficients for each triplet set of points col-2 total

	double getElemSLERP( double time, int row );
		// Spline interpolate of reagent row

	void getColSLERP( double time, double *_vector );
		// Spline interpolate for entire column of reagents

	int getMinNonzeroTime( double &minT, int initBounds );
		// minimum time value that is not 0; returns 1 if data exists

	int getBounds( double &minT, double &maxT, double &minY, double &maxY, int initBounds=1 );
		// Get the bounds of the data. Set initBounds to 0 if you want to use this call several times to get a global min / max.  Be sure to set the min to DBL_MAX and max to -DBL_MAX
		// returns 1 if there was in fact data.

	int getBoundsForRow( int row, double &minY, double &maxY, int initBounds=1 );
		// Get bounds as above but only for single row.  returns 1 if there was data 

	int getBoundsForRange( int row, double &minY, double &maxY, double t0, double t1, int initBounds=1, int derivBounds=0 );
		// As above, but only within time range [t0,t1].  if row>=0, bounds are only for that
		// row; if row<0, all rows are considered. returns 1 if there was data.

	int getBoundsRowCoords( double &min, double &max, int initBounds=1 );
		// max/min for rowCoords

	int getCountInRange( double t0, double t1, int *i0=0, int *i1=0 );
		// how many columns exist in the range [t0,t1]?
		// if i0, i1 are (either) non-NULL, return first/last indices of range.

	int sameTimeEntries( KineticTrace &t );
		// does t have same entries for time as we do?

	double getMinStepSize();
		// Get the smallest step

	int loadBinary( FILE *f, int byteswap );
	int saveBinary( FILE *f );

	void saveText( char *filename, int row=-1, double stepSize=0.0 );

	KineticTrace();
	KineticTrace( int _rows, int _cols=-1 );
	~KineticTrace();
};

// KineticParameterInfo
//------------------------------------------------------------------------------------------

// Understanding the P and Q vectors.
// The Q Vector is the list of all parameters than could be fit. It is arranged as follows:
//   Rates rates, Observable Constants, ICs of each experiment
// Note that these parameters are not necessarily all fit not are they all necessarily independent
// Fitting flags and constraints and so for are all stored in the KineticParameterInfo structure
// 
// The P Vector is an abbreviated Q vector...
// Because the fitter compiles an H VM that is SHARED between all experiments,
// the ICs for one experiment don't belong in another and therefore there is a "local" mapping which is as follows
//   Rates rates, Observable Constants, ICs OF ONE EXPERIMENT
// When the fitter calls the integrateH function on an experiment it has to load 
// this abbreviated parameter vector which is called "P"

struct KineticSystem;
struct KineticParameterInfo {
	int type;
		#define PI_REACTION_RATE (1)
		#define PI_OBS_CONST (2)
		#define PI_INIT_COND (4)
		#define PI_VOLTAGE (8)
		#define PI_VOLTAGE_COEF (16)
		#define PI_TEMPERATURE (32)
		#define PI_TEMPERATURE_COEF (64)
		#define PI_PRESSURE (128)
		#define PI_PRESSURE_COEF (256)
		#define PI_SOLVENTCONC (512)
		#define PI_SOLVENTCONC_COEF (1024)
		#define PI_ALL (0xFFFF)

	int experiment;
		// If type == PI_INIT_COND then this field tells which experiment of the system

	int reagent;
		// If type == PI_INIT_COND then this field tells which reagent of the system

	int mixstep;
		// If type == PI_INIT_COND, which mixstep this is for 

	int reaction;
		// If type == PI_REACTION_RATE then this field tells which reaction of the system

	int obsConst;
		// if type == PI_OBS_CONST this is the fields that tells which whichObsConst

	int qIndex;
		// When the global list of parameters is made for fitting, this is the index into that compact vector
		// The "q" vector does not have parameters which are not being fit

	int pIndex;
		// The index wrt to the local space.

	int fitFlag;
		// Is this parameter fit

	double value;
		// General purpose

	char name[64];
		// General name (@TODO naming convention)

	int group;
		// Parameters may be part of a 'group' which implies relationship: 
		// 0: no group, the parameter is freely/indepently floating
		// 1: fixed group, a holdover from KinTek v1, means this param is fixed; deprecated; use 'fitFlag' now.
		// >1: fixed ratio group, e.g. all params with group 2 when fit are held in constant ratio.  For v2, this
		//     is accomplished by actually only fitting one of the params, and solving for the others algebraicly 
		//     after the fit.

	// @TODO: This will also contain ratio information, etc

	int isRelevantToExperiment( int e ) {
		return type == PI_REACTION_RATE || type == PI_OBS_CONST || ( type == PI_INIT_COND && experiment == e );
			// @TODO
			// This logic is a source of confusion to me.  To my thinking, an OBS_CONST is not relevant to an experiment
			// if that experiment contains no observable expressions which use the OBS_CONST.  Put another way, the 
			// observable traces for a given experiment will be unaltered for varying values of some OBS_CONST if that
			// OBS_CONST is not used in the expressions.  But the above logic says all OBS_CONSTS are relevant to
			// every experiment.  This is used by zbs for his fitter, I believe.  For my v2 fitter, I do extra work
			// elsewhere to track which OBS_CONSTs are used per experiment, such that I know when they are not used
			// the derivatives needed by the fitter are 0 wrt this OBS_CONST, and I early out of some work. (tfb)
	}

	int isICOfExperiment( int e ) {
		return type == PI_INIT_COND && experiment == e;
	}

	KineticParameterInfo()  {
		type = 0;
		experiment = -1;
		reagent = -1;
		mixstep = 0;
		reaction = -1;
		obsConst = -1;
		qIndex = -1;
		pIndex = -1;
		fitFlag = 1;
		value = 0.0;
		name[0] = 0;
		group = 0;
	}

	char *suffixedFriendlyName( int masterOrdinal, int slaveOrdinal, int mixstepCount );
		// name displayed in ui to user

	void dump( KineticSystem *s );
		// debug

	static char * paramTypeName( int type );
		// debug
};

struct SavedKineticParameterInfo {
	double value;
	int group;
	int fitFlag;
		// these are the items that are NOT computed in an allocParamterInfo, and want to
		// be saved at times.  This struct makes storing them in a hash handy.
	
	char * set( KineticParameterInfo &p ) {
		// handy util specifically to aid poking this struct into a hashtable
		value   = p.value;
		group   = p.group;
		fitFlag = p.fitFlag;
		return (char*)this;
	}

	void byteSwap();
		// byteswap individual members
};



// KineticExperiment
//--------------------------------------------------------------------------------------------------------------------------

struct KineticExperiment {

	// Basics
	//------------------------------------------------------------

	struct KineticSystem *system;
		// Un-owned reference to the container system

	int id;
		// an id that is unique within the context of a single KineticSystem;
		// relies on constructor taking a system, or a system being called to create us.

	int getExperimentIndex();
		// The index of this experiment in the system's list; note that when experiments
		// are deleted, an experiment's index may change.  The system's paramterInfo array
		// can still use this, since the array is rebuilt anytime the system is reparameterized.
		// But anyone wishing to track an experiment reliably across deletions etc... should
		// reference the id (above) instead.

	int getMasterExperimentOrdinal( int *slaveOrdinal=0 );
		// Series experiments consist of a single master experiment and one or more "slave"
		// experiments.  A non-series experiment is also considered a master.  This returns 
		// the 'ordinal' of our master - telling the caller that the master in question is
		// the 1st, 2nd, or 3rd master in the list, and so on.  This value cannot be used to
		// index into any array maintained by the system.
		// If slaveOrdinal is non-NULL, set it to the 1-based ordinal indicating which position
		// this experiment occupies in the series.  Set it to 0 if we are not part of a series.

	int reactionCount();
	int reagentCount();


	// Series Experiments: if an experiments are identical except
	// for one quantity, it is useful to treat them as a "series".
	// Series may be concentration (most common), voltage, or
	// temperature.  THe last two added dec2011.
	//------------------------------------------------------------

	#define SERIES_TYPE_REAGENTCONC (0)
	#define SERIES_TYPE_VOLTAGE (1)
	#define SERIES_TYPE_TEMPERATURE (2)
	#define SERIES_TYPE_SOLVENTCONC (3)
		// the seriesType is stored in the viewInfo hashtable of properties.
		// default is REAGENTCONC which refers to concentrations of a reagent.
		// SOLVENTCONC refers to concentration of a solvent, e.g. urea which
		// acts as a denaturant for proteins and affects folding rates.

	int slaveToExperimentId;
		// When this is set to non-negative, we are part of a series exeperiment
		// and this id points to our master.

	int mixstepIndexForSeries;
		// specifies the mixstep for which the series is defined.

	int reagentIndexForSeries;
		// concentration series only:
		// when slaved to a master, this index is the reagent index for which the
		// concentration series is defined.  

	int isMaster() { return slaveToExperimentId == -1; }
	int getSeries( ZTLVec<KineticExperiment*> &vec, int includeEqOrPC=0 );
		// return the pointers to experiments in a series; master is always first.
		// returns count in series (1 if not a series)

	int seriesCount( int includeEqOrPC=0 );
		// return count in this series (1 if not a series at all)

	char *getAttributeNameForParamType( int paramType );
		// return string for parameter type to be used as keys in hashtables.

	char *getNameForSeriesType( int abbreviated );
		// return a string naming the type of series, or NULL of not a series.

	KineticParameterInfo * getSeriesParameterInfo( int type, int mixstepIndex, int paramIndex=0 );
		// return the paramterInfo for the series parameter if it exists

	KineticParameterInfo * getPrimarySeriesParameterInfo();
		// Now that any reagent, voltage, temp, or solvent conc can vary in any mixstep of
		// a series, we need an easy way to ask for the 'primary' variable for the series,
		// for the purpose of plotting rate vs concentration, among other things.

	double getSeriesConcentration( int mixstepIndex, int paramIndex );
		// return the reagent concentration for this exp in a series or -1 if not a conc series.

	double getSeriesVoltage( int mixstepIndex );
		// return the voltage for this exp in a series or -1 if not a volt series.

	double getSeriesTemperature( int mixtepIndex );
		// return the temperature for this exp in a series or -1 if not a temp series.

	double getSeriesSolventConcentration( int mixstepIndex );
		// return the solvent conc for this exp in a series or -1 if not a solventconc series.

	double getSeriesValue( int type, int mixstepIndex, int paramIndex=0 );
		// return the value for the series, whatever kind it is

	void getStatsForSeries( int &simulationStepsMin, int &simulationStepsMax, int &measuredCountMin, int &measuredCountMax, int includeZeroData=1 );
		// useful stats for a collection of experiments in a concentration series

	int seriesParamVaries( int type, int mixstepIndex, int paramIndex=0 );
		// does the reagent/temp/voltage/solvent-conc vary on this mixstep for this series?
		// If mixstep is -1, does this series param vary on any mixstep at all?

	int seriesParamVariesAndExclusively( int type, int mixstepIndex, int paramIndex=0 );
		// does this seriesParam vary at this mixstep, and is it the ONLY param that
		// varies across all mixsteps?  i.e. is this the reason this is a series?

	void updateSeriesFromMaster();
		// update the series with information from the master; typically UI writes to the master
		// only with regard to concentrations, mixstepDomains, etc...  SO prior to simulation,
		// we must copy the master to the slaves.

	// Mixsteps
	//------------------------------------------------------------

	int mixstepNextUniqueId;
		// int unique id for the next mixstep created in this experiment

	double mixstepDilutionTime;
		// How long the dilution takes

	int mixstepCount;
		// The number of mixsteps

	int mixstepDomain;
		// if not -1, then operations on the experiment should apply to the specified mixstep only

	struct Mixstep {
		int id;
			// id that is unique within the context of an experiment, so that
			// specific mixsteps may be found by id even after add/del shifting.

		//private:  // I am making this private, at least temporarily, to find all places that ref
				  // this directly and instead have them ref simulationDuration( int mixstep )
			friend struct KineticExperiment;
			double duration;
				// length of step, in seconds
		public:
			double *getDurationPtr() { return &duration; }

		double dilution;
			// dilution of this step

		KineticTrace traceOCForMixstep;
			// the observable trace for this mixstep only.  This is created specifically
			// to allow polyFit to be performed per mixstep.  Ugly.

		ZTLPVec< KineticTrace > traceOCExtraForMixstep;
			// experimental: analagously, we must maintain these extra traces per mixstep.
			// These are currently not copied as part of copy()

		// New dec 2009: mixsteps reference external data per mixstep rather
		// than using the measured arrays.  The measured arrays may still be used
		// to composite the mixstep references which also hold info about offset
		// and scale to be applied.
		struct MixstepDataRef {
			int	dataId;	
				// a unique id for the data (refers to a KineticTrace)
			double offset;
				// the offset into that data at which our data starts
			double scale;
				// scaling that should be applied to the data
			double translate;
				// additive value to apply to data
			MixstepDataRef() {
				dataId = -1;
				offset = 0.0;
				scale  = 1.0;
				translate = 0.0;
			}
		};
		ZTLPVec< MixstepDataRef > dataRefs;
			// This vector will have as many entries as there are observables in this experiment,
			// but some entries may ne NULL if there is no data for this mixstep/observable.

		void clear();
		void steal( Mixstep& toSteal );
	};
	int mixstepDataRefsDirty;
		// indicates some aspect of a MixstepData was modified, such that any composite
		// trace that may have been constructed from the vector of mixteps should be recomputed.
		// see: int createMeasuredFromMixstepDataRefs()

	#define MAX_MIX_STEPS (12)
	Mixstep mixsteps[MAX_MIX_STEPS];

	void mixstepInfo( int mixstep, double &startTime, double &endTime, double &dilution );
		// Get information about the mixstep extracted from the mixsteps array

	void newMixstep( double *ics, double dilution, double duration );
		// create new mixstep entry, and add parameters to the system that describe
		// the initial concentrations for the step

	void delMixstep( int mixstep );
		// delete the specified mixstep entry

	void mixstepSetDuration( int mixstep, double duration );
		// set mixstep duration for this exp and any slaved to it

	void mixstepSetDilution( int mixstep, double duration );
		// set mixstep dilution for this exp and any slaved to it

	int getReagentFixedMatrix( ZMat &fixedConc, ZTLVec<int> *ignoreReagents );
		// fills a matrix that indicates which reagents/mixsteps have fixed concentrations

	// Equilibirum flags
	//------------------------------------------------------------
	//
	// WARNING: train wreck approaching...  These equilibrium vars
	// added by zbs are unrelated to the "isEquilibrium" flag added
	// by tfb (in viewInfo).  We each have some idea of what an
	// "equilibrium experiment" is, but they are different ideas.

	ZMat equilibriumVariance;
		// This is the variance for each observable
	int equilibriumExperiment;
		// If this is set then this is a special "equilibrium experiment"
		// When set, we interpret the initCond after fitting as the equilibirum values

	// Simulation
	//------------------------------------------------------------

	double simulationDuration( int _mixstepDomain = -1 ) {
		double duration = 0.0;
		for( int i=0; i<mixstepCount; i++ ) {
			if( _mixstepDomain == -1 || _mixstepDomain == i ) {
				duration += mixsteps[ i ].duration;
			}
		}
		return duration;
	}

	KineticTrace traceC;
		// The results of the integration of base system of equations

	ZTLPVec<KineticTrace> traceG;
		// There are parameterCount traceG's

	enum KESimulationState { 
		KESS_None,		// experiment idle, nothing required to do
		KESS_Need,		// material changes have been made to the system & this exp needs to be simulated
		KESS_Requested,	// a simulation has been requested; sim thread will set this to KESS_Completed when done
		KESS_Simulating,// sim pending or in progress
		KESS_Completed,	// the requested simulation has completed; calling thread will reset this to None
	} simulateState;

	void simulate( struct KineticVMCodeD *vmd, double *pVec, int steps=-1 );
		// Run the integrator on this experiment; if steps>0, a fixed-stepsize will be used
		// to result in the specified number of integration points (including initial values),
		// otherwise the step-size is dynamically adjusted to achieve desired precision.

	virtual void simulateEquilibriumKintek( struct KineticVMCodeD *vmd, double *pVec ) {}
		// unrelated to zbs equilibrium exp; overloaded in kintek source code

	virtual void simulatePulseChaseKintek( struct KineticVMCodeD *vmd, double *pVec ) {}
		// overloaded in kintek source code

	int simulationSteps() { return traceC.rows ? traceC.cols : 0; }

	int reagentsEmpty();
		// Returns 1 if experiment has all 0 initial concs for mixstep 0
		// This condition will cause the integrator to never return...

	// Observables
	//------------------------------------------------------------
	ZTLPVec<char> observableInstructions;
		// The instruction strings for the observables, in infix notation

	ZTLVec<int> observableError;
		// Flag set if there's an error in the observable expression (it's row vector will be set to all zero)

	int observableInstructionsValid();
		// True if there are no errors

	int observablesCount() { return equilibriumExperiment ? reagentCount()+observableInstructions.count : observableInstructions.count; }
		// Number of observable rules

	KineticTrace traceOC;
		// The computed observables

	ZTLPVec< KineticTrace > traceOCExtra;
		// Computed observables at various parameter set values.  This is experimental; it is not
		// saved nor is it copied by the various copy() functions.  The idea is to compute observables
		// at a number of differing parameter sets, and save the results here for comparison to the
		// best fit param set etc...  This is used to plot these traces in the experiment plot callback of kintek.

	ZTLPVec< KineticTrace > traceOCResiduals;
		// residuals between traceOC and measured data; as above, not copied, or saved.

	double getTraceOCSLERP( double time, int row, int ignoreMixsteps=0 );
		// get the SLERP value at time; provided to overload functionality when mixstepCount>1

	double getTraceOCExtraSLERP( int index, double time, int row );
		// get the SLERP value at time for the extra trace at index computed at parameter bounds: experimental

	struct KineticVMCodeOC *vmoc;
		// Virtual machine that can compute the observables given the concentrations and observable constants
		// But the map of observable constants has to be known when this is made

	struct KineticVMCodeOD *vmod;
		// Virtual machine that can compute the time deriv of the observables given the concentrations and observable constants
		// But the map of observable constants has to be known when this is made

	struct KineticVMCodeOG* vmog;
		// Virtual machine that can compute the observables deriviates given the concentrations and observable constants
		// But the map of observable constants has to be known when this is made

	void compileOC();
		// Creates the observable VM and compiles it

	void computeOC( double *pVec );
		// Executes the observables VM using the parameters
		
	void computeOCMixsteps( double *pVec, int stride );
		// Same as above, but allowing a distict parameter set to be used at each mixstep.
		// Params are given in sets that are each "stride" doubles long.

	void compileOG();
		// Creates the observable VM WRT each parameter and compiles it

	ZMat observableResidualFFTMat;
		// Holds the computed FFT of the residual on the last fit point. Size = observableCount x 64 matrix

	void observablesGetMinMax( double &minVal, double &maxVal, int bIncludeMeasuredData=1, int bVisibleOnly=0, int bIncludeDeriv=0,
							   int minObs=0, int maxObs=-1 );
		// get min/max data values for this exp + series, respecting both the simulation
		// duration and the mixstepDomain.  Optionally included measured, option exclude hidden observables.

	double observablesDomainBegin() { return traceOC.getTime( 0 );	}
	double observablesDomainEnd() {	return traceOC.getLastTime();	}


	ZTLVec<double> ssePerObservable;
		// observablesCount entries giving the sse for the measured vs. simulated data.
		// This is computed after each simulation of the experiment, in ::simulate().
	ZTLVec<normalizeType> normalizeTypeUsedForSSE;
		// which normalizeType was used for each observable in computing the above sse
	
	void computeSSEPerObservable( normalizeType normalize = NT_NumTypes, ZTLVec< double > *sigmas=0 );
		// compute sse for each observable, optionally normalized.  Note that the default
		// value of normalize here is not a valid type, and results in the normalization
		// using the per-trace sigma information that is available (perhaps none).

	double getExpSSE();
		// compute SSE for the current experiment based on ssePerObservable above.

	double getSeriesSSE();
		// compute total SSE for all experiments in our series.

	KineticTrace traceDebug;
		// For debugging to visualize valuses during fits

	// Measured
	//------------------------------------------------------------

	int ownsMeasured;
		// This was added 06/16/2009 by tfb, and the measured member was
		// changed to a ZTLVec instead of a ZTLPVec.  ownsData is 1 by
		// default to allow current users that expect this to remain unaffected,
		// as the KineticExperiment destructor will delete traces if owned.

	ZTLVec< KineticTrace* > measured;
		// Was a ZTLPVec: see directly above

	void measuredClear();
		// Clears the measured data traces: this has been changed to
		// delete if we own the data, and just clear the ZTLVec otherwise.

	double measuredDuration();
		// Finds the maximum time point in the measured data

	void measuredCreateFake( int numSteps, double variance );
		// Create pretend data for the experiment

	int measuredCreateFakeForMixsteps( int n, double sigma, double offset, int logTime, double **timeRefs=0, int errType=0, double errP1=0, double errP2=0 );
		// create synthetic data, respecting mixstep settings

	int measuredCreateFakeForMixsteps( ZTLVec< KineticTrace* > &traces, int n, double sigma, double offset, int logTime, double **timeRefs=0, int errType=0, double errP1=0, double errP2=0 );
		// create synthetic data, respecting mixstep settings

	int measuredCount( int i ) { return i<measured.count ? (measured[i] ? measured[i]->cols : 0) : 0; }
		// each data associated with an observable has arbitrary # steps
	
	int measuredHasAnyData();
		// True if any of the measured traces have data
	
	int measuredDataHasSigma();
		// True if all measured traces have known sigma
	
	// Titration Profile
	//------------------------------------------------------------
	// TODO - I ran across this when adding weighting profile below
	// and wondered about it, and I'm not sure this is actually used
	// in any useful way.  It could perhaps be removed. tfb oct 2015
	KineticTrace titrationProfile;
	virtual int loadTitrationProfile( char *filespec ) { return 1; }

	// Optional Threading
	//------------------------------------------------------------

	#ifdef KIN_MULTITHREAD
		PMutex traceOCMutex;
		PMutex traceFWMutex;
		PMutex measuredMutex;
		#define TRACEOC_LOCK(x)    x->traceOCMutex.lock()
		#define TRACEOC_UNLOCK(x)  x->traceOCMutex.unlock()
		#define TRACEFW_LOCK(x)    x->traceFWMutex.lock()
		#define TRACEFW_UNLOCK(x)  x->traceFWMutex.unlock()
		#define MEASURED_LOCK(x)    x->measuredMutex.lock()
		#define MEASURED_UNLOCK(x)  x->measuredMutex.unlock()
	#else
		#define TRACEOC_LOCK(x)    
		#define TRACEOC_UNLOCK(x)  
		#define TRACEFW_LOCK(x)    
		#define TRACEFW_UNLOCK(x)  
		#define MEASURED_LOCK(x)
		#define MEASURED_UNLOCK(x)
	#endif


	// KintekExplorer-V2-specific Fitting interface
	//------------------------------------------------------------
	#ifdef USE_KINFIT_V2
		// Temporary: related to fitting in v2.  Will go away when we
		// move from v2->v3.

		fitDataContext fdc;
			// context info for fitting data in this exp to some function or model

		FitData fd;
			// this is used for special-case fitting of series to analytic functions!

		void resetFitContext( int resetFitOutputFactors=1 );
			// reset fit context for this and all series conc child experiments

		void enableObservableFit( int which, int updateSeries=0 );
			// enable this observable for fitting (inc. all series conc)
	#endif

	// Fit Weighting Profile
	//------------------------------------------------------------

	KineticTrace fitWeightingProfile;
		// This trace holds an arbitrary function which serves as a 
		// weighting profile when fitting the current experiment.
		// experimental, oct 2015
	void setWeightingProfileExp1( double A, double b, double C );	
		// In practice this is a single exponential decay, and is used
		// to force a fit to better model the early/fast transient.
	



	//experiment:
	ZHashTable obsConstsByName;
		// track the observable constants used by this experiment; this is used
		// by the v2 fitter to avoid computation of derivatives when some fitted
		// constant is NOT used by any of the expressions in the experiment.
		// I would like to use the promisingly-named "isRelevantToExperiment" of
		// KineticParameterInfo, but see the comment there about it not being 
		// what it seems to be.  (tfb)

	int getObservableConstantsForExperiment( ZTLVec< KineticParameterInfo* > &vec );
		// based on the above, get a list of the obs const parameterInfo structs
		// for those used by this experiment.

	// Hashtables for general parameterization.  See detailed notes in KineticSystem
	//------------------------------------------------------------------------------

	ZHashTable viewInfo;

	// Renderable copies
	//------------------------------------------------------------
	// For the purposes of keeping the render thread and the fitting thread
	// separated, after each interation of fitting, the fitting thread makes a copy
	// of all these traces into these separate buffers.
	// Any call to copyRenderVariables is expected to be mutexted with the render thread
	void copyRenderVariables();
	ZTLPVec<KineticTrace> render_measured;
	KineticTrace render_traceC;
	ZTLPVec<KineticTrace> render_traceG;
	KineticTrace render_traceOC;
//	ZTLPVec<KineticTrace> render_traceOG;
	KineticTrace render_traceDebug;
	
	// @TODO: Remove all these render_X variables once I'm done refactoring to ZFitter

	// Housekeeping
	//------------------------------------------------------------
	void clear( int retainMeasured=0 );
		// Free everything

	void copy( KineticExperiment &copyFrom, int copyMeasured=1, int copyFitContext=1 );
		// clear and then copy by reallocating everything

	void updateFromMaster( KineticExperiment &master );
		// specialized copy for slaved experiments

	int copyObservableInstructionsPreservingSeriesFactors( ZTLPVec<char> & obsInstructions, int hasScale, int hasOffset );
		// util used by updateFromMaster()

	virtual int loadBinary( FILE *f, int byteswap );
		// load the experiment from disk; typically done in the context of KineticSystem::loadBinary

	virtual int saveBinary( FILE *f );
		// save the experiment to disk; typically done in the context of KineticSystem::saveBinary

	void adaptToModel( KineticSystem &oldSystem, KineticSystem &newSystem );
		// adapt this experiment to new system if possible, marking obs errors where they occur
		// this is used when a KineticSystem has been edited and we wish to preserve whatever experiment
		// settings we can.

	void dump();
		// debug output

	KineticExperiment( KineticSystem *_system );
	virtual ~KineticExperiment();

	private:
		KineticExperiment();
			// prevent use of null constructor; if this is needed, then the same
			// init should be done as in the constructor above.  An experiment 
			// without a system is ill-defined.
};

// KineticSystem
//------------------------------------------------------------------------------------------

struct KineticSystem {

	// Reagents
	//----------------------------------------
	ZTLPVec<char> reagents;
		// Holds reagent names
	
	ZTLVec<int> *systemFixedReagents;
		// temp, non-saved, set during ::simulate

	ZTLPVec<char> reagentPartials;
		// experiment to track derivative of reagent concentration due
		// to particular reactions.  To start we'll append a single reaction
		// index to the reagent name, so reagent FS may have partial
		// FS0, which means rate of change of FS due to reaction 0+1 (forward+reverse)

	int reagentCount() { return reagents.count; }

	int reagentAdd( char *name ) {
		char *dup = strdup( name );
		reagents.add( dup );
		return reagents.count - 1;
	}

	void reagentsSet( ZTLPVec<char>& newReagents );
		// Sets a new set of reagent names, fixes-up the experiment
		// reagentIndexForSeries if necessary.

	char *reagentGetName( int i ) {
		return reagents[i];
	}
	
	int reagentFindByName( char *r );
		// Returns -1 if not found

	int reagentPartialFindByName( char *r );
		// Returns -1 if not found

	int reagentIsUsedByReaction( int reagentIndex, int reactionIndex );

	int reagentsGetFixedFlag( ZTLVec<int> &fixedReagents );
		// which reagents can be held fixed at the system level (across all experiments/mixsteps)

	int reagentGetSecondOrderComponents( char *reagent, char *&compA, char *&compB );
		// if reagent is a product of two other reagents, return 1, and return
		// names of reagents in compA and compB.
	
	// Reactions
	//----------------------------------------
	struct Reaction {
		// Reactions are unidirectional and consist of 1 or 2 inputs and 1 or 2 outputs
		// -1 represents no reactant.
		union {
			int react[2][2];
			struct {
				int in0, in1, out0, out1;
			};
		};
		int labeled;
			// if this reaction is the unlabeled-member of a pair of reactions (e.g. radiolabeled, flourescent-tracer)
			// then this is the index of the corresponding labeled reaction.
		Reaction() { reset(); }
		void reset() { in0=in1=out0=out1=-1; labeled=-1; }
	};

	ZTLVec<Reaction> reactions;
		// Holds reactions in the form in0, in1, out0, out1 (indexed into reagents array where -1 is none)

	int reactionCount() { return reactions.count; }

	int reagentCountForReaction( int r ) {
		int count = 0;
		if( reactions[r].in0 != -1 ) count++;
		if( reactions[r].in1 != -1 ) count++;
		if( reactions[r].out0 != -1 ) count++;
		if( reactions[r].out1 != -1 ) count++;
		return count;
	}

	int reactionAdd( int in0, int in1, int out0, int out1 ) {
		Reaction reaction;
		reaction.in0 = in0;
		reaction.in1 = in1;
		reaction.out0 = out0;
		reaction.out1 = out1;
		reactions.add( reaction );
		return reactions.count-1;
	}

	void reactionAddByNames( char *in0, char *in1, char *out0, char *out1 );
	void reactionAddByNamesTwoWay( char *in0, char *in1, char *out0, char *out1 );

	char * expandChainReaction( char *text );
	char * expandRepeatedReaction( char *text );
	char * preprocessReactions( char *text );
		// experimental; see comments at definition

	int reactionsAddByFullText( char *reactionText );
		// this last variant can be used to setup reagents *and* reactions from a full text description
		// of the reactions (e.g. "E + S = EI = E + P") and two-way reactions are assumed.  Newlines may
		// appear between reactions as well (e.g "A+B=C\nC=D"); see more comments in fn definition.
		// Returns 1 on success; note that reagents/reactions are overwritten even in the case of a 
		// failed parse!

	int reactionsAddUnlabeled();
	int updateGroupAndRateValueForUnlabeledPairings();
		// both experimental: see comments at definition

	int reactionGet( int which, int side, int ab ) {
		return reactions[which].react[side][ab];
	}

	char *reactionGetString( int reaction, int withSpaces=1 );
		// return a string that describes the reaction, e.g. "A + B = C"

	char *reactionGetUniqueString( int reaction );
		// return a reagent-sorted unique string used to identify equivalent reactions

	int reactionGetReagents( int reaction, char **in0, char **in1, char **out0, char **out1 );
		// point passed values to names of reagents, or null if reagent slot == -1.

	int reactionGetFromReagents( char *in0, char *in1, char *out0, char *out1 );
		// return reaction index if reaction is found matching reagents.

	int reactionIspH( int reaction );
		// is this reaction a binding to protons, i.e. H+, meaning a reagent is named "pH"

	static char *reactionGetRateName( int reaction );
		// return a string that names the rate which controls this reaction, e.g. "k+1"





	// Observable Constants
	//----------------------------------------

	static int findValidSymbol( char *stringIn, char* &symbolBegin, char* &symbolEnd );
		// helper fn to find the next valid symbol in a string
	void observableConstSymbolExtract( char *instructions, ZHashTable &currHash, ZHashTable &accumHash );
		// Given an observable instruction, extract out alpha-numberic symbols that are observable consts
		// currHash receives the obs const names that are part of passed instructions.
		// accumHash does as well, but can be used to build a hash of names over multiple instructions.

	
	// Experiments
	//----------------------------------------

	int experimentNextUniqueId;
		// for assigning unique ids to new experiments: unique in the context of THIS system

	ZTLPVec<KineticExperiment> experiments;
		// Owned pointers to the experiments

	KineticExperiment *getExperimentById( int _id );
		// return experiment with given unique id

	virtual KineticExperiment *newExperiment();
		// New's up an experiment, adds it to experiments, and returns the pointer

	KineticExperiment *copyExperiment( KineticExperiment *e, int newUniqueId=1, int copyMeasured=1 );
		// Copies the passed experiment and adds it to the list.

	void copyInitialConditions( KineticExperiment *from, KineticExperiment *to );
		// Copies the initial conditions of 'from' to 'to'; needs to be done after
		// a copyExperiment that resulted in a new experiment with new unique id.  Must
		// be done *after* allocParameterInfo has been called (after the copy).

	void delExperiment( KineticExperiment *e );
		// Removes the passed experiment + series from the list, and deletes it.

	void delExperiment( int eIndex );
		// Removes the experiment at eIndex + series from the list, and deletes it.

	void delSeriesExperiments( int masterExpIndex );
		// Remove all concentration series for masterExpIndex, leaving only the master

	KineticExperiment * addSeriesExperiment( int masterExpIndex, int reagentIndex=-1, int mixstepIndex=-1, int type=0 );
		// Add a series experiment to masterExpIndex.  

	void updateSlavedExperiments();
		// copies contents of master exp to any slaves necessary for simulation

	void simulate( int forceAllExp=1, int steps=-1 );
		// Run simulate on all experiments; steps>0 implies fixed step-size.
		// forceAllExp is a temp hack since zack prob doesn't want to mess
		// with the 'dirty' kind of flags in KineticExperiment.

	KineticExperiment *operator [](int i) { return i < experiments.count ? experiments[i] : 0; }
		// Convenient shorthand for then ith experiment (used frequently for traversal)

	int eCount() { return experiments.count; }
		// count of experiments

	int getMasterExperiments( ZTLVec< KineticExperiment* > &exps, int plottedOnly=0 );
		// populate list of the master experiments (those not slaved to
		// another experiment, returning count added to list.

	void updateSystemReagentHoldFixedForExperiments();
		// this is a hack to see if we can cause the system to hold a reagent fixed at the
		// system level, which is possible if all experiments hold the same reagent fixed.
		// If this is possible, we can cause the reagent to hold fixed by adjusting 
		// the compilation of KineticVMCodeD; otherwise, we have to reset the reagent value
		// at each step inside of integrateD.  tfb oct 2016
	
	// Hashtables used for general information storage. viewInfo started as a place to store things
	// used by the render thread, and grew to hold all kinds of stuff.  So viewInfo is used with
	// the threadSafe feature turned on.
	//----------------------------------------

	ZHashTable viewInfo;

	
	// Virtual machine caches
	//----------------------------------------

	struct KineticVMCodeD *vmd;
	struct KineticVMCodeH *vmh;
		// Owned cached of the virtual machine codes, common to all experiments of a given model

	void compile();
		// This creates all of the virtual machines as well as extracts the 
		// observable consts out of the experiments

	
	// Parameter fetch and put
	//----------------------------------------
	ZTLVec<KineticParameterInfo> parameterInfo;
		// A list of all the parameter infos for everything parameter
		// Note that this is sorted when getQ is called and therefore acts as a map

	void paramInitPQIndex();
		// Sets up the p / q map

	void getQ( ZMat &qVec );
		// Given the parameterInfo about which parameters are fit and which aren't

	void getP( ZMat q, int forExperiment, ZMat &pVec, int forMixstep=-1 );
		// Make the abbreviated list of "local" parameters given the global parameters q.  (See notes above.)

	void putQ( ZMat q );
		// Load the values in the q vector into the appropriate places in the experiments and system

	int getPLength();
		// Gets the length of the p vector

	void saveParameterInfo( ZHashTable &savedParameters );
		// Save parameter info for subsequent lookup by unique string ids

	void allocParameterInfo( ZHashTable *paramValues=0 );
		// Create the parameter info for every parameter; user may pass optional
		// hash containing param values keyed by unique strings as setup by saveParameterInfo

	void loadParameterValues( ZHashTable &loadValues );
		// Load parameter values from the hash - this was written to allow updating the
		// values from some saved table without having to realloc the param vector, such
		// that pointers into this vector will remain valid

	int fittingParametersCount( int typeMask=PI_ALL );
		// Count how many parameters are set to fit

	void clrFitFlags( int typeMask=PI_ALL, int experiment=-1 );
	void setFitFlags( int typeMask=PI_ALL, int experiment=-1 );

	KineticParameterInfo *paramGet( int type, int *count=0, int experimentIndex=-1, int mixstep=0 );
	KineticParameterInfo *paramGetByName( char *name );

	double paramGetReactionRate( int r );
	void paramSetReactionRate( int r, double v );
	double paramGetIC( int e, int r );
	void paramSetIC( int e, int r, double v );

	double *paramGetVector( int type, int *count=0, int experimentIndex=-1, int mixstep=0 );
		// This new's an array for the doubles so be sure to delete when you're done

	// Voltage and Temperature Dependence 
	//-----------------------------------------------------------
	// I had thought to put all voltage/temperature dependence functionality in
	// the KintekSystem subclass, but the parameterization of the system is 
	// maintained here (allocParamInfo etc)
	#define GAS_CONST_JOULES 8.3144621						// in  J/K/mol
	#define GAS_CONST_KJOULES (GAS_CONST_JOULES / 1000.0)	// in kJ/K/mol
	#define GAS_CONST_KCAL 0.00198720413					// in kcal/K/mol

	#define FARADAY_CONST 0.0964853365						// in kJ/mv/gram

	enum DependType { DT_Volt, DT_Temp, DT_Pres, DT_Conc };
	static int rateDependCoefTypes[4];
	static int rateDependInitCondTypes[4];
	int isDependent( KineticSystem::DependType type );
		// the above only tells if the system has or should be parameterized for
		// this kind of dependency; it may be that if some of the parameters are
		// zero, for example, that the system is not actually dependent.
		// Further, we intend here these names to be mutually exclusive options,
		// so if a system depends on voltage, then isDependent( DT_Volt ) will 
		// be TRUE, and isDependent( DT_Temp ) will be FALSE, even though all
		// voltage-dependent system implicity depend on temperature as well.

		// The functions below will actually check param values to see if the
		// specific rate as parameterized is actually dependent:

	int isRateDependent( int rate, int coefType, int includeLinkedRates=0 );
		// coefType is one of the types of params that a rate may depend on,
		// such as PI_VOLTAGE_COEF, PI_TEMPERATURE_COEF, etc.
		// rate is a specific rate index
		// includeLinkedRates means is this rate, or any of the rates in the same group, v-dependent...
		// This checks even the value of the 2nd coef, since if it is 0, then 
		// the rate is by definition not depending on the voltage etc.

	KineticParameterInfo* paramGetRateCoefs( int rate, int coefType );
		// get a point to two consecutive param structs for ampltude & frequency terms
		// for exponential expression that controls rate. Returns 0 if this rate has
		// no such params.  coefType is something like PI_VOLTAGE_COEF or similar.

	int rateGroupIsDependent( int group, int coefType, int *whichRate=0 );
		// is the specified rate group dependent on dependType?  We must
		// ensure that only one rate in a group is dependent in this way.
		// Return voltage-dependet rate in whichRate if appropriate.

	void updateTemperatureDependentRates( int eIndex, int mixstep, double *rates );		
	void updateTemperatureDependentRatesAtRefTemp( double Told, double Tnew );
		// system rates are the rates at a reference temperature.  Rates for experiments
		// at other temperatures are computed from the rate at ref temp as well as 
		// the Ea coefficient for temp-dependent rates.

	void updateVoltageDependentRates( int eIndex, int mixstep, double *rates );
	void updateVoltageDependentRatesAtRefTemp( double Told, double Tnew );
    void updateVoltageDependentRatesAtRefVolt( double Vold, double Vnew );
    	// Just like temp, but Charge is in the exponential.  Note that voltage-dependence
    	// implies also a dependence on a temperature, but this is not the same as 
    	// temp-dependence above, which includes an activation energy (Ea) term.
    
    void updateConcentrationDependentRates( int eIndex, int mixstep, double *rates );
	void updateConcentrationDependentRatesAtRefTemp( double Told, double Tnew );
    void updateConcentrationDependentRatesAtRefConc( double Cold, double Cnew );
    	// Just like voltage, but for solvent concentration

	// TODO: 
	// void updatePressureDependentRates( int eIndex, int mixstep, double *rates );


	// Optional Threading
	//------------------------------------------------------------

	#ifdef KIN_MULTITHREAD
		PMutex systemMutex;
			// we will probably want to get more granular than this, but for now
			// this replace the mutex that locks the system while it simulates.
		#define SYSTEM_LOCK(x)     (x)->systemMutex.lock()
		#define SYSTEM_UNLOCK(x)   (x)->systemMutex.unlock()

		PMutex simStateMutex;
			// this protects the simulateState of experiments, but since we often 
			// want to update this for multiple experiments as an atomic operation,
			// we prefer a single system-level mutex for this.
		#define SIMSTATE_LOCK(x)   (x)->simStateMutex.lock()
		#define SIMSTATE_UNLOCK(x) (x)->simStateMutex.unlock()
	#else
		#define SYSTEM_LOCK(x)   
		#define SYSTEM_UNLOCK(x) 
		#define SIMSTATE_LOCK(x)
		#define SIMSTATE_UNLOCK(x)
	#endif


	// House keeping
	//----------------------------------------
	void copy( KineticSystem &k, int copyExperiments = 1, int copyData = 1 );
		// copy everything, loses current data, reallocates

	void stealTracesToExtra( KineticSystem &from );
		// copy the observable traces from the passed KineticSystem to our "Extra" traces for plotting etc...

	void clearExtraTraces();
		// clear all 'extra' traces (traceOCExtra etc) in all experiments

	void clear();
		// Frees all memory

	virtual int loadBinary( FILE *f );
		// load a KineticSystem from disk

	virtual int saveBinary( FILE *f );
		// save the KineticSystem to disk

	void dump();
		// debug output

	KineticSystem();
	virtual ~KineticSystem();

	// Temporary Porting work: this section is created to get things working with
	// the various refactors.  This stuff will change as we go from v2 to v3
	//-----------------------------------------------------------------------

	int getBalance( int reaction, int reagent );
		// is reagent gained or lost?  this is used by layout code for the flow view,
		// and represetns the v1 way of looking at reactions.  In v2, really the balance
		// can be both +1 and -1 since a reagent may be gained and lost in a reaction.
};

// KineticIntegratorVM
//------------------------------------------------------------------------------------------

struct KineticVM {
	// The KineticVM is a virtual machine which has a bank of register addresses used
	// by the various parts of code that have to be excuted.  It has a stack-based
	// opcode inerpreter as well as an optimized 4-tuple MAC

	KineticSystem *system;
	KineticExperiment *experiment;
		// These are non-owned reference to the system and experiment
		// May be null in some cases
		
	int pCount;
		// Number of parameters.  If integrating only D0 then this is reactionCount

	ZTLVec<double> registers;
		// Register map (see accsessor funcs below)
		// constant 0
		// constant 1
		// constant -1
		// constant 2
		// constant -2

		// rate constants (P)
		// concentrations (C)
		// time differentials (D)
		// parameter derivatives (G)

	#define PC (pCount)
	#define RC (system->reagentCount())
	#define RPC ( system->reagentPartials.count )
		// RPC is experimenteal for reagentPartials - I'm not adding this to VMCodeH etc
		// as those are not used by KinTek software.

	int regIndexConstZero()                { return 0; }
	int regIndexConstOne()                 { return 1; }
	int regIndexConstMinusOne()            { return 2; }
	int regIndexConstTwo()                 { return 3; }
	int regIndexConstMinusTwo()            { return 4; }
	int regIndexConstLn10()                { return 5; }
	
	int regIndexP( int p )                 { return 6+p; }
	int regIndexVolt()					   { return 6+PC-5; }
	int regIndexTemp()					   { return 6+PC-4; }
	int regIndexPres()					   { return 6+PC-3; }
	int regIndexSConc()					   { return 6+PC-2; }
	int regIndexConc()					   { return 6+PC-1; }
	
	int regIndexC( int i )                 { return 6+PC+i; }
	int regIndexD( int i )                 { return 6+PC+RC+RPC+i; }
	int regIndexG( int i, int p )          { return 6+PC+RC+RC+RPC+RPC+  p*RC+i; }
	int regIndexWRT()                      { return 6+PC+RC+RC+RPC+RPC+ PC*RC; }
	int regIndexCount()                    { return 6+PC+RC+RC+RPC+RPC+ PC*RC + 1; }

	double *regConstZero()                 { return &registers.vec[regIndexConstZero()]; }
	double *regConstOne()                  { return &registers.vec[regIndexConstOne()]; }
	double *regConstMinusOne()             { return &registers.vec[regIndexConstMinusOne()]; }
	double *regConstTwo()                  { return &registers.vec[regIndexConstTwo()]; }
	double *regConstMinusTwo()             { return &registers.vec[regIndexConstMinusTwo()]; }
	double *regP( int p )                  { return &registers.vec[regIndexP(p)]; }
	double *regC( int i )                  { return &registers.vec[regIndexC(i)]; }
	double *regD( int i )                  { return &registers.vec[regIndexD(i)]; }
	double *regG( int i, int p )           { return &registers.vec[regIndexG(i,p)]; }
	double *regWRT()                       { return &registers.vec[regIndexWRT()]; }

	char *regName( int regIndex );

	void regLoadP( double *P );
	void regLoadC( double *C );
	void regLoadD( double *D );
	void regLoadG( int wrtParameter, double *G );

	int regLookupD( char *name );
	int regLookupC( char *name );
		// If a name is a reagent then return the regC otherwise if its a parameter return from regP
		
	enum Ops {
		NOP = 0,
		ADD,
		SUB,
		MUL,
		DIV,
		CMP_WRT,
		PUSH,
		PUSH_TIMEREF_C,
		PUSH_TIMEREF_D,
		PUSH_TIMEREF_G,
		LN,
		LOG10,
		EXP,
		POW10
	};

	void disassembleMacDump( FILE *f, int *code );
	void disassembleExpressionDump( FILE *f, int *code );

	void executeMacBlock( int *codeStart, int blockStart, int count, double *output );
	void executeExpressionBlock( int *codeStart, int blockStart, int count, double *output );
		// codeStart is the start of the opcodes and blockStart is an index into this
		// buffer where a list of count indicies (indicies relative to codeStart) where
		// the code is for each output variable

	KineticVM( KineticSystem *_system, int _parameterCount );
	~KineticVM();
};

struct KineticVMCode : KineticVM {
	// KineticVMCode is the base class of all "executable" collections of bytecode
	// that will run on the VM
	// The bytecode and various indicies are all allocated in one big memory buffer (a resizable vec)
	// The map of the indicies are maintained by the various sub-classes of this class

	// Bytecode
	//----------------------------------------
	ZTLVec<int> bytecode;
		// All of the bytecode is stored in one big buffer.  Where each block is a size followed by 
		// 4-tuple register references for a multiply and accumulate (MAC) virtual machine

	void emitStart( int index );
	void emitStop( int index );
		// Mark start and stop of emit block, use bcIndex funcs for index

	void emit( int value );
		// Emits value into bytecodeBuffer at bytecodeBufferWrite. Reallocates buffer if needed
	void emit( double value );
		// Emits a double value into bytecodeBuffer at bytecodeBufferWrite. Reallocates buffer if needed

	int hasTimeRef( char *name, int &reagent, int &parameter, double &timeRef, int &error );
		// If the name is formatted with a square bracket then it returns the time ref in the 
		// Returns 1 if it has a [] in which case returns the value in timeRef
		// Returns 0 if it is a naked symbol 
	
	int isCompiled() { return bytecode.count > 0; }

	KineticVMCode( KineticSystem *_system, int _parameterCount );
	~KineticVMCode();
};

struct KineticVMCodeD : KineticVMCode {
	int blockStart_D;
	int blockStart_dD_dC;

	int bcIndex_D( int i )                           { return i; }
	int bcIndex_dD_dC( int i, int j )                { return RC+RPC+j*(RC+RPC)+i; }
	int bcIndexCount()                               { return RC+RPC+(RC+RPC)*(RC+RPC); }

	void compile( int includeJacobian=1 );
	void disassemble( char *ext=0 );

	void execute_D( double *C, double *dC_dt );
	void execute_dD_dC( double *C, double *dD_dC );

	KineticVMCodeD( KineticSystem *_system, int _parameterCount, int includeJacobian=1 );
};

struct KineticVMCodeH : KineticVMCode  {
	int blockStart_H;
	int blockStart_dH_dG;
	int blockStart_dH_dt;

	int bcIndex_H( int i, int p )                    { return  p*RC+i; }
	int bcIndex_dH_dG( int p, int i, int j )         { return PC*RC+ p*RC*RC+j*RC+i; }
	int bcIndex_dH_dt( int p, int i )                { return PC*RC+PC*RC*RC     +p*RC+i; }
	int bcIndexCount()                               { return PC*RC+PC*RC*RC+PC*RC; }

	void compile();
	void disassemble( char *ext=0 );

	void execute_H( int wrtParameter, double *C, double *G, double *dG_dt );
	void execute_dH_dG( int wrtParameter, double *C, double *D, double *G, double *dH_dG, double *dH_dt );

	KineticVMCodeH( KineticSystem *_system, int _parameterCount );
};

struct KineticVMCodeOC : KineticVMCode {
	// Parser functions:
	int parenAwareSearcher( char *str, char lookingFor );
	int parenAwareSearcherStr( char *str, char *lookingFor );
	int splitter( char *str, int index, char *&lft, char *&rgt );
	virtual int recurseCompile( char *str );

	int compile();
	void disassemble( char *ext=0 );

	void execute_OC( double *C, double *P, double *OC );

	KineticVMCodeOC( KineticExperiment *_experiment, int _parameterCount );
};

struct KineticVMCodeOG : KineticVMCodeOC {
	virtual int recurseCompile( char *str );
	int compile();
	void execute_OG( int obs, int wrt, double *C, double *P, double *G, double *OG );
	void disassemble( char *ext=0 );
	KineticVMCodeOG( KineticExperiment *_experiment, int _parameterCount );
};

struct KineticVMCodeOD : KineticVMCodeOG {
	KineticVMCodeOD( KineticExperiment *_experiment, int _parameterCount );
	virtual int recurseCompile( char *str );
	void disassemble( char *ext=0 );
	void execute_OD( double *C, double *P, double *D, double *OD );
};

// KineticIntegrator
//------------------------------------------------------------------------------------------

struct KineticIntegrator {

	KineticVMCodeD *vmd;
		// Un-owned reference to the vmd
	KineticVMCodeH *vmh;
		// Un-owned reference to the vmh

	double errAbs;
	double errRel;
		// absolute and relative error tolerances influence dynamic step size
					
	KineticTrace traceC;
		// The results of the integration of base system of equations

	ZTLPVec<KineticTrace> traceG;
		// There are parameterCount traceG's

	int whichIntegrator;
		#define KI_GSLRK45  (0)
		#define KI_GSLBS    (1)
		#define KI_ROSS     (2)

	static int luDecomposeGSLAdaptor( double *matA, int dim, size_t *permutation, int *signNum );
	static int luSolveGSLAdaptor( double *matU, int dim, size_t *permutation, double *B, double *X );

	ZIntegrator *zIntegrator;

	static int adapator_compute_D( double t, double *C, double *dC_dt, void *params );
	static int adapator_compute_dD_dC( double t, double *C, double *dD_dC, double *dD_dt, void *params );
	static int adapator_compute_H( double t, double *G, double *dG_dt, void *params );
	static int adapator_compute_dH_dG( double t, double *G, double *dH_dG, double *dH_dt, void *params );

	int compute_D( double t, double *C, double *dC_dt );
	int compute_dD_dC( double t, double *C, double *dD_dC, double *dD_dt );
	int compute_H( double t, double *G, double *dG_dt );
	int compute_dH_dG( double t, double *G, double *dH_dG, double *dH_dt );
		// These all store the matricies in col-major order
		// Return 0 for success

	int wrtParameter;
		// When integrating H, this is which parameter we are integrating with respect to

	public:

	double getTime() { return zIntegrator->x; }

	// Step integration...
	void prepareD( double *icC, double *P, double endTime, double startTime=0.0 );
		// Prepare D to be stepped using stepD, or evolved using integrateContinuedD 

	int stepD( double deltaTime );
		// Returns err from zintegrator is any
		// Step D forward deltaTime resulting in a new value for C

	void setC( int row, double val ) { zIntegrator->loadY( row, val ); }
		// Useful for when you want to force a change to the concentration such as due to diffusion

	double *getC() { return zIntegrator->y; }

	void getDilutedC( double dilution, double *addC, double *dilutedC );
		// store values for each reagent in dilultedC based on the current C,
		// the passed dilution and addC.  This does not modify C.

	// Full integration...
	int integrateD( double *icC, double *P, double endTime, double startTime=0.0, double stopIfStepSmallerThan=0.0, int *icFixed=0 );
		// Evolve D forward to endTime (intermediate results from the integrator are stored)
		// Starting with initial conditions icC and using parameters P (which are the reaction rates)
		// The integrator will stop if stopIfStepSmallerThan>0.0 && step size < stopIfStepSmallerThan
		// icFixed is optional array of flags that says whether a given ic should be held fixed.

	void allocTraceG();
		// The traceG must be allocated explicitly before any calls to integrateH

	void integrateH( int _wrtParameter, double *icG, double *P, double endTime );
		// Evolve H with regard to parameter forward to endTime (intermediate results from the integrator are stored)
		// Starting with initial conditions icG and using parameters P
		
	double getMinStepSizeC() { return traceC.getMinStepSize(); }

	KineticIntegrator( int _whichIntegrator, KineticVMCodeD *vmd, double _errAbs, double _errRel, KineticVMCodeH *vmh=0 );
		// passing 0 for error tolerances will result in default values used:
		#define KI_DEFAULT_ERRABS 1e-7
		#define KI_DEFAULT_ERRREL 1e-7
	~KineticIntegrator();
};



#endif



