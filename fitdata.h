// @ZBS {
//		*MODULE_OWNER_NAME fitdata
// }

#ifndef FITDATA_H
#define FITDATA_H

#include "ztl.h"
#include "zmat.h"
#include "zhashtable.h"
#include "zhash2d.h"
#ifdef FITSET_MULTITHREAD
	#include "pmutex.h"
#endif

#include "gsl_vector_double.h"
	// for gsl_vector support by FitData
#include "gsl_matrix_double.h"
	// for gsl_matrix in Levmar experiments with FitData


struct KineticSystem;
struct KineticExperiment;

//-----------------------------------------------------------------------------------------
//
// This file declares the ParamInfo, FitData, FitSet, and GridSet structs
//
// ParamInfo: all information about a single parameter, including fitting information
//
// FitData  : a collection of ParamInfos & specs that define a fit, as well as the
//            results of the fit once complete.
//
// FitSet   : a sortable/queryable collection of FitData - a collection of fits
//
// GridSet  : an extension to FitSet that supports 2d spatial hashing of FitData
//            by coordinate pair.
//
//------------------------------------------------------------------------------------------

enum paramType { PT_ANY=-1, PT_GENERIC=0, PT_RATE, PT_OUTPUTFACTOR, PT_VOLTCOEF, PT_TEMPCOEF, PT_PRESCOEF, PT_CONCCOEF, PT_NUMTYPES };
	// NOTE: If you add new types, add them to the END!  Numeric equivalents of these
	// types are saved to disk and when loaded you'll mess things up otherwise.

	// the type of a param.  Rates are kinetic rates for the model.  Output
	// Factors are multipliers applied in experiment observable equations.
	// We may add initial concentrations, etc.  Generic is used for analytic 
	// fit params that are amplitudes, etc.

enum constraintType { CT_NONE=0, CT_NONNEGATIVE, CT_RATIO, CT_FIXED, CT_NONPOSITIVE };	
	// NOTE: If you add new types, add them to the END!  Numeric equivalents of these
	// types are saved to disk and when loaded you'll mess things up otherwise.

	// How the parameter is contrained, if at all. These may need to become bitfield flags
	// to allow multiple constraints, but for now each entry implies all other information.
	//
	// CT_NONE: the parameter is free to take on any value
	// CT_FIXED: the parameter is held fixed (i.e. not fit at all)
	// CT_NONNEGATIVE: the parameter must be non-negative ( >= 0 )
	// CT_NONPOSITIVE: the parameter must be non-postiive ( <= 0 )
	// CT_RATIO: the parameter must remain in constant ratio with other parameter(s).
	//			 This is currently implemented by holding CT_RATIO params fixed,
	//			 and computing their value aglebraicly after the fit completes using
	//			 the bestFitValue of a single fitted "master" parameter (which may be constrained, but not CT_RATIO)

struct ParamInfo {
	// An attempt to generify the treatment of various kinds of parameters
	// used during a fit.  Rather than maintaining unique arrays and hashes
	// for the different kinds of params (rates, output factors, etc), and
	// unique structs about contraints, we'd like to have a single hash of
	// all parameters that contains all pertinent info for each.
	//
	// We'll also store results of the fit here, to hopefully simplify 
	// operations that deal with inputs and outputs of fits.
	//

	// The parameter name will be the key for a hash.  The key will map to
	// this struct.

	#define PARAMNAME_MAXLEN 16
	char paramName[ PARAMNAME_MAXLEN ];


	// Input to the fitting routines:
	//-------------------------------

	double initialValue;
		// the initial value for the param going into a fit

	paramType type;
		// what type of parameter is this (e.g. rate, outputfactor)

	constraintType constraint;
		// how is this parameter constrained during fitting?  Note that
		// CT_FIXED means the parameter is not actually fit at all. 

	char ratioMasterParamName[PARAMNAME_MAXLEN];
	double ratio;
		// in the case of CT_RATIO constrained params, the above tells us what param
		// we are held in ratio to, and what the ratio is..

	int fitIndex;
		// the index of this parameter in the list of parameters that are 
		// actually being fit.  For example, the index into the array of 
		// parameters used by gsl fitting routines.  More generally, says
		// this is the nth param based on whatever ordering is imposed by
		// the fitting system.

	// Output from the fitting routines:
	//----------------------------------

	double bestFitValue;
		// the result of the last fitting operation; would be useful to update
		// this as fitting routines iterate for the purposes of tracking the route
		// to the final bestFitValue.

	double bestFitValueLast;
		// a copy of the above from the last iteration of the fit, allowing  calculations
		// to have knowledge of the movement of params from step to step during the fit.

	double covarStdError2x;
		// the error estimate on the bestFitValue as computed from the covariance
		// matrix of fitted parameters (via sqrt of diagonal elements).  times 2.

	// Member Functions
	//-----------------

	ParamInfo();
	ParamInfo( char *name, double initVal, paramType pt=PT_GENERIC, constraintType ct=CT_NONE );
	ParamInfo( ParamInfo &toCopy );

	int loadBinary( FILE *f, int byteswap );
	void saveBinary( FILE *f );

	void clear();
	void copy( ParamInfo &toCopy );

	int usedByFit();
		// based on the constraintType, is this param used by the fitter?

	void setRatioMaster( ParamInfo *master );
		// setup this param to be computed as a fixed ratio w.r.t master

	/*
	enum paramType { PT_BESTFIT, PT_ERROR2X };
	char * paramVal( paramType pt=PT_BESTFIT, int sigDigits=3 );
		// return formatted string of value
	*/
};


//---------------------------------------------------------------------------------------
// FitData


enum dataType { DT_NONE=0, DT_SIMULATED, DT_EMPIRICAL };
	// DT_SIMULATED: data points generated by the integrator based on the model
	// DT_EMPIRICAL: data points imported to the program, or generated synthetically
	//               to approximate real-world data collected via experiments.

enum funcType { FT_NONE=0, FT_LINEAR, FT_EXP1, FT_EXP2, FT_EXP3, FT_EXP4, FT_BURST1, FT_BURST2, FT_BURST3,
				FT_POLY, FT_HYPERBOLA, FT_QUADRATIC, FT_HILL, FT_MODEL };
	// Which mathematical model to fit data to:
	//			K*t+C
	//			a1*e^(-k1*t)+c
	//			a1*e^(-k1*t)+a2*e^(-k2*t)+c
	//			a1*e^(-k1*t)+a2*e^(-k2*t)+a3*e^(-k3*t)+c
	//			a1*e^(-k1*t)+a2*e^(-k2*t)+a3*e^(-k3*t)+a4*e^(-k4*t)+c
	//			a1*e^(-k1*t)+k2*t+c
	//			a1*e^(-k1*t)+a2*e^(-k2*t)+k3*t+c
	//			a1*e^(-k1*t)+a2*e^(-k2*t)+a3*e^(-k3*t)+k4*t+c
	//			polynomial
	//			Hyperbola: y(t) = a * ( t / (t+Kd) ) + c
	//			Quadratic: y(t) = a * quadratic(Kd,E0,t) + c 
	//					   where quadratic = ((Kd+E0+t) - sqrt((Kd+E0+t)^2 - 4*E0*t)) / (2*E0)
	//			Hill equation: y(t) = a * t^n/(Kd^n+t^n) + c 
	//          (fit using model -- see func_model_piecewise in kinfit.cpp)

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
char *getNormalizeTypeString( normalizeType nt );
char *getNormalizeTypeString( ZTLVec< normalizeType > &normTypes, int *pcount=0 );


struct FitData {
	// This structure holds the definition for a fit as well as results from that fit.
	// It specifies what data to fit, what parameters are available, what kind of
	// constraints to apply to each parameter, and so on.  Once the fit is complete,
	// bestfit values and errors are avail for each param, as well as statistics
	// for the overall fit.

	int fitInProgress;
		// semaphore; prevent recursion.  In the case of singleton-type use of 
		// FitData, this flag is the central location to look to determine
		// if a fit is in progress.

	KineticSystem * fitSystem;
		// The system, which contains experiments, that will be fit.  Each Experiment
		// contains a local FitDataContext which holds specific information pertaining
		// to the experiment, such as pointers to actual data points, buffers for 
		// holding computed best fit curves for experiment-local observables, etc.
		// BUT THIS WILL CHANGE 

	KineticSystem *jacSystem;
		// a workspace used during fitting to make fitting thread-safe;
		// it will be based on fitSystem, so we only ensure it is
		// initialized to 0, and that if fitSystem is destroyed or 
		// copied from somewhere, jacSystem is destroyed.

	bool bOwnsFitSystem;
		// do we own the KineticSystem pointed to above?

	funcType fitFuncType;
		// which mathematical model to fit data to; see enum funcType

	dataType fitDataType;
		// which type of data will be fit; see enum dataType
	
	normalizeType fitNormalizeType;
		// what kind of normalization will be applied during error/residual calculation
		// in the case that forceHomogenousNormalization is turned on.  Otherwise the 
		// individual traces of data specifying what kind of normalization should be applied.
	int forceHomogenousNormalization;
		// should all fitted data be normalized using the same type of sigma as 
		// specified in fitNormalizeType above?
	
	ZHashTable params;
		// parameterName->ParamInfo mapping for all parameters exposed by the model
		// and experiments.  Each ParamInfo tells if the param is fit, what constraints
		// to apply, etc, and will as well receive output from the fit such as 
		// bestFitValue, error estimate, etc...  See struct ParamInfo.

	ZHashTable orderedParams;
		// convenient for lookup of ParamInfo by order of insertion

	ZHashTable fitIndexToParamInfo;
		// convenient lookup of ParamInfo by fitIndex.  See ParamInfo::fitIndex and
		// FitData::createGslParamVectorFromParams().

	int nDataPointsToFit;
		// how many data points exist for currently defined fit?  This is computed in
		// setupExpFitDataContexts()

	int hasBeenSetup;		// this is a hack for ntrials analytic fits see fitAnalytic() and fix this!
		// flag which says whether setupExpFitDataContexts has been called on us
		// to appropriate setup internal pointers to data etc...  The internal
		// pointers actually live inside FitDataContext of each experiment
		// in fitExps.
		
	ZHashTable properties;
		// added to store general misc attributes

	// Stats about completed fit:
	//---------------------------
		// previously in struct fitResults

	double chi2;
		// chi^2 error term for the overall fit (sum square of residuals==error terms)

	int dof;	
		// degrees of freedom ( = num data points - num params )

	double gammaQ; 
		// incomplete gamma fn  (see NumericalRecipies gammq)
		// [a measure of 'goodness of fit' > .001 == believeable fit]

	double sigma;
		// calculated sigma (stddev) of dataset wrt to bestfit curve(s)
	
	ZMat covar;
		// covariance matrix for parameters ACTUALLY FIT.  That is, params whose constraint
		// type is CT_FIXED or CT_RATIO (see enum constrainType) are not included in this
		// matrix.
	
	void *errVec;
		// Added for Simplex-based fitting using the gsl minimizer framework in which
		// we need to provide an error vector to the existing fitting function callbacks.
		// This gets alloc'd and used only by this minimizer-based fitting. tfb jan2013

	// Experimental for Levmar fitter:
	//--------------------------------

	double gslFuncSSE;
	gsl_vector *gslParams;
	gsl_vector *gslFunEval;
	gsl_vector *gslErrTermsOut;
	gsl_matrix *gslJacTermsOut;
		// The Levmar fitter uses plain vectors of double to store function output
		// and jacobian computations.  To allow the use of all the function and jacobian
		// callbacks that were written for use with GSL, we're going to maintain gsl
		// structures per fit for use by an adaptor function in kin_fit_levmar.cpp

	int nFunEval;
	int nJacEval;
		// how many calls ?



	// Member Functions
	//-----------------

	FitData();
	FitData( FitData &_copy );
	~FitData();

	int loadBinary( FILE *f, int byteswap );
	void saveBinary( FILE *f );

	void clear();
	void clearLevmarData();
	void clearParams();
	void clearFitSystem();

	void copy( FitData &toCopy, KineticSystem *fit=0, KineticSystem *jac=0, bool bOwnedSystems=false );
	void setupKineticSystems( FitData *initFrom, KineticSystem *fit, KineticSystem *jac, bool bOwnedSystems=false );
	void copyParams( FitData &toCopy );

	void addParameter( char *name, double initialValue, paramType pt=PT_GENERIC, constraintType ct=CT_NONE );

	ParamInfo * paramByName( char *name );
	ParamInfo * paramByFitIndex( int index );
	ParamInfo * paramByOrder( int index );
		// handy lookups in our hashes

	int paramCount( paramType type=PT_ANY, int bFittedOnly=0, int bIncludeRatioConstrained=0 );
		// return total params by type, including possibly all, only fitted, or only fitted plus
		// those whose value is computed as a multiple of one that was fitted (CT_RATIO)

	void swapParamDataPreservingNames( ParamInfo *p1, ParamInfo *p2 );
		// swap information between param entries, preserving name & fitIndex

	void copyBestFitValuesToInitialValues();
		// for all params, copy the current bestFitValues to initialValues;
		// used to start a new fit where the last one left off.

	void copyValuesToKineticSystem( KineticSystem &system, int useBestFitValues );
	void copyBestFitValuesToKineticSystem( KineticSystem &system );
	void copyInitialValuesToKineticSystem( KineticSystem &system );
		// copy values from FitData.params to the kineticSystem

	void resetBestFitValueLast();
		// set all bestFitValueLast in ParamInfo to 0

	void clearConstraintType( constraintType ct );
		// clear any constraints of the given type; they become CT_NONE

	void setupRateRatioConstraintsFromSystem( KineticSystem &model );
		// setup rate ratio constraint information in our params based on the settings
		// of the passed model.

	int createGslParamVectorFromParams( gsl_vector **pv, int useBestFitValue=0 );
		// allocate and populate a gsl vector with initial/bestfit vals for the
		// params that will be fit given our current settings.  This also
		// sets the fitIndex member of each paramInfo involved in the fit,
		// and creates the working hash used to later lookup params by this
		// fitIndex.  Returns #params allocated in vector.
	
	void updateParamsFromGslParamVector( const gsl_vector *v );
		// update the current bestFitValue of our params from the values
		// contained in the passed gsl_vector, keep tracking also of the last value.

	int createParamVectorFromParams( double **pv, int useBestFitValue=0 );
	//void updateParamsFromParamVector( const double *v );
		// these two are exactly analagous to the two above, but operate on
		// standard arrays of double rather than gsl_vector, for levmar experiments.
		
	void computeRatioParamValues();
		// update any params that are held in fixed ratio to a master param

	double *getDataToFit( double *vec=0 );
		// utility written for levmar experiment to return an array of double
		// that holds the total data to be fit. (nDataPointsToFit length)
	
	void print();
		// debug printf
};

//---------------------------------------------------------------------------------------
// FitSet

#define FITSET_DEFAULTSIZE 16
struct FitSet {
	// FitSet is a collection of FitData that represent fits computed with
	// various (usually related) starting conditions.  This is used to store
	// related results when exploring "Fit Space"

	ZHashTable properties;
		// different kinds of fitsets may have different properties; some are 
		// common to all:
		//
		//	name		(char*)
		//  tolerance	(double)
		// 
		//  gridsteps	(int)	only for grid FitSets
		//
		//  etc.

	FitData ** ppFitData;
		// We manage FitData alloc/dealloc, and maintain a vector of 
		// pointers to them that we sort in various orders on demand.

	int	arraySize;
		// how many FitData pointers can ppFitData point to
	int fitCount;
		// how many actual FitData have been allocated and added to the array

	char sortKey[32];
		// used by the qsort callback to know which parameter to sort on

	FitSet();
	~FitSet();

	int loadBinary( FILE *f, int byteswap );
	void saveBinary( FILE *f );

	void clear( bool bDeallocFitData=true );

	FitData * allocFit( FitData &initFromResults );
	void addFit( FitData *fd, int alreadyLocked=0 );
		// alreadyLocked is provided as a parameter to allow cases in which 
		// a larger operation on this fitset has already obtained the lock

	// Sorting: the FitData pointers may be sorted by named quantity
	//-------------------------------------------------------------------------

	void sortByValue( char *name );
		// current valid names are "Chi2" or a param name.

	void sort( FitData **data, int count );
		// recursive quicksort native to FitSet for member access etc, called
		// by sortByValue().  Clients should typically not call this directly.

	double getSortValue( int index, FitData **data=0 );
		// from the FitData* at index, return the value by current sortKey
		// data param is typically only used by sort() above for recursion.


	// Ordered access / statistics based on sort
	//------------------------------------------------------------------------

	double getMinValue();
	double getMaxValue();
		// get min or max value according to current order in ppFitData and
		// current sortKey to identify param.

	FitData * getNearestByValue( double val );
		// return the FitData whose sortKey value is closest to val.

	void getHistogram( int *bins, int count );
		// fill user-supplied array of count length by examining the key value
		// in each FitData based on the current sortKey and getMinValue/MaxValue.

	// Optional Threading
	//------------------------------------------------------------------------
	#ifdef FITSET_MULTITHREAD
		PMutex fitDataMutex;
		void lock() { fitDataMutex.lock(); }
		void unlock() { fitDataMutex.unlock(); }
	#else
		void lock() {}
		void unlock() {}
	#endif

	// Misc
	//------------------------------------------------------------------------
	void dump( char *sortBy );
		// debug aid
};

//---------------------------------------------------------------------------------------
// GridSet: a specialization of FitSet that adds 2D spatial hashing for coordinate-
//          based FitData access; intended for sampling regular 2d grids of params.

struct GridNode : public ZHash2DElement {
	// allow use of ZHash2D for spatial hashing for grids
	FitData *fd;
		// this FitData lives in and is owned by a FitSet/GridSet
	GridNode( FitData * _fd ) : fd(_fd) {}
};

struct GridSet : public FitSet {
	ZHash2D spatialToFitData;
		// lookup a GridNode->fd from FitSet by coordinate pair

	FitSet lowerBoundsDetailP1;
	FitSet lowerBoundsDetailP2;
		// experimental 'detail strips' : optional samples to place lower bounds
		// of p1,p2 in context visually

	GridSet();
	~GridSet();

	int loadBinary( FILE *f, int byteswap );
	void saveBinary( FILE *f );

	void clear( bool bDeallocFitData=true );
	void clearSpatialHash();
	void initSpatialHash( int bins, double xmin, double xmax, double ymin, double ymax );

	void addFit( FitData *fd, double p1, double p2 );
		// insert into spatial hash then call FitSet::addFit( fd );

	FitData *fitFromCoords( double p1, double p2 );
		// retrieve from spatial hash

	bool hasEmptyNeighbor( FitData *seed, FitData **ppNeighbor=0 );
		// does seed have neighbor that needs sampling?  If ppNeighbor nonNull, return
		// newly allocated empty neighbor, already setup for fitting.

	bool isComplete( double chi2SeedThreshold, double lBoundDetailThreshold );
		// have we sampled all locations meeting threshold criteria?

	bool lowerBoundsDetailCriteria( double chi2Threshold );
		// does our grid meet criteria for sampling lower bound detail?

	GridSet * createShadowGridWithBoundaryFeatures( FitSet *bounds1, FitSet *bounds2 );
		// allocate a new gridset which is a merging of the current grid with
		// the boundary sets for each param of this grid: for a given grid location,
		// the sample with the lowest chi2 is taken.  Note that the returned GridSet
		// DOES NOT own the FitData samples it references, so take care when deallocating.
};

#endif
