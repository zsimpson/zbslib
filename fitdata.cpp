// @ZBS {
//		*MODULE_NAME FitData
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Context for fit data, by Thomas
//		}
//		*PORTABILITY win32 unix maxosx
//		*REQUIRED_FILES fitdata.cpp fitdata.h
//		*VERSION 1.0
//		+HISTORY {
//			1.0 13 May 2010 ZBS added module header
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }

#include "fitdata.h"
#include "zendian.h"
#include "kineticbase.h"
#include "zstr.h"
#include "ztmpstr.h"
#include "zmathtools.h"
#include "zregexp.h"
#include "zmat.h"

#include "string.h"
#include "math.h"
#include "float.h"
#ifndef __APPLE__
	#include "malloc.h"
#endif

//extern int Kin_fitspaceCalcLBoundDetail;
extern void trace( char *fmt, ... );

char* funcTypeParamNames[(int)FT_NUMAFUNCS][9]  = {
	{ "","","","","","","","","" }, // FT_NONE
	{ "b","C","","","","","","","" },// FT_LINEAR
	{ "A1","b1","C","","","","","","" },// FT_EXP1
	{ "A1","b1","A2","b2","C","","","","" },// FT_EXP2
	{ "A1","b1","A2","b2","A3","b3","C","","" },// FT_EXP3
	{ "A1","b1","A2","b2","A3","b3","A4","b4","C" },// FT_EXP4
	{ "A1","b1","b2","C","","","","","" },// FT_BURST1
	{ "A1","b1","A2","b2","b3","C","","","" },// FT_BURST2
	{ "A1","b1","A2","b2","A3","b3","b4","C","" },// FT_BURST3
	{ "","","","","","","","","" },// FT_POLY
	{ "a","Kd","c","","","","","","" },// FT_HYPERBOLA
	{ "kcat","Km","","","","","","","" },// FT_MM
	{ "kon","kcat","","","","","","","" },// FT_MM2
	{ "a","Kd","E","c","","","","","" },// FT_QUADRATIC
	{ "a","Kd","n","c","","","","","" },// FT_HILL
	{ "a","K1","K2","c","","","","","" },// FT_2SBINDING
	{ "a","K1","K2","K3","c","","","","" },// FT_3SBINDING
	{ "a","K1","K2","K3","K4","c","","","" },// FT_4SBINDING
};
int funcTypeNumParams[(int)FT_NUMAFUNCS]  = {
	0, 2, 3, 5, 7, 9, 4, 6, 8, 0, 3, 2, 2, 4, 4, 4, 5, 6
};



//--------------------------------------------------------------------------------------
// ParamInfo
//--------------------------------------------------------------------------------------

ParamInfo::ParamInfo() {
	clear();
}

//----------------------------------------

ParamInfo::ParamInfo( char *name, double initVal, paramType pt/*=PT_GENERIC*/, constraintType ct/*=CT_NONE*/ ) {
	clear();
	initialValue	= initVal;
	bestFitValue    = initVal;
	bestFitValueLast= 0;
	covarStdError = 0;
	type			= pt;
	constraint		= ct;

	assert( strlen(name) < PARAMNAME_MAXLEN );
	strncpy( paramName, name, PARAMNAME_MAXLEN );
	paramName[PARAMNAME_MAXLEN-1] = 0;
}

//----------------------------------------

ParamInfo::ParamInfo( ParamInfo &toCopy ) {
	clear();
	copy( toCopy );
}

//----------------------------------------
const int ParamInfo_Version = 20171024;
int ParamInfo::loadBinary( FILE *f, int byteswap ) {
	int version;
	freadEndian( &version, sizeof( version ), 1, f, byteswap );
	
	switch( version ) {
		case 20080407: {
			#define OLD_PARAMNAME_SIZE 16
				// before we doubled the size to 32; see header for comment.
			freadEndian( paramName, 1, OLD_PARAMNAME_SIZE, f, byteswap );
			freadEndian( &initialValue, sizeof( initialValue ), 1, f, byteswap );
			freadEndian( &type, sizeof( type ), 1, f, byteswap );
			freadEndian( &constraint, sizeof( constraint ), 1, f, byteswap );
			freadEndian( ratioMasterParamName, 1, OLD_PARAMNAME_SIZE, f, byteswap );
			freadEndian( &ratio, sizeof( ratio ), 1, f, byteswap );
			freadEndian( &fitIndex, sizeof( fitIndex ), 1, f, byteswap );
			freadEndian( &bestFitValue, sizeof( bestFitValue ), 1, f, byteswap );
			freadEndian( &covarStdError, sizeof( covarStdError ), 1, f, byteswap );
			covarStdError /= 2.0;
		}
		break;
			
		case 20151105: {
				// identical to above, but using sizeof(paramName) which is now 32.
			freadEndian( paramName, 1, sizeof(paramName), f, byteswap );
			freadEndian( &initialValue, sizeof( initialValue ), 1, f, byteswap );
			freadEndian( &type, sizeof( type ), 1, f, byteswap );
			freadEndian( &constraint, sizeof( constraint ), 1, f, byteswap );
			freadEndian( ratioMasterParamName, 1, sizeof( ratioMasterParamName ), f, byteswap );
			freadEndian( &ratio, sizeof( ratio ), 1, f, byteswap );
			freadEndian( &fitIndex, sizeof( fitIndex ), 1, f, byteswap );
			freadEndian( &bestFitValue, sizeof( bestFitValue ), 1, f, byteswap );
			freadEndian( &covarStdError, sizeof( covarStdError ), 1, f, byteswap );
			covarStdError /= 2.0;
		}
		break;

		case 20171024: {
			// identical to above, but no longer dividing covarStdError by 2 because it is
			// now computed/stored as 1x.
			freadEndian( paramName, 1, sizeof(paramName), f, byteswap );
			freadEndian( &initialValue, sizeof( initialValue ), 1, f, byteswap );
			freadEndian( &type, sizeof( type ), 1, f, byteswap );
			freadEndian( &constraint, sizeof( constraint ), 1, f, byteswap );
			freadEndian( ratioMasterParamName, 1, sizeof( ratioMasterParamName ), f, byteswap );
			freadEndian( &ratio, sizeof( ratio ), 1, f, byteswap );
			freadEndian( &fitIndex, sizeof( fitIndex ), 1, f, byteswap );
			freadEndian( &bestFitValue, sizeof( bestFitValue ), 1, f, byteswap );
			freadEndian( &covarStdError, sizeof( covarStdError ), 1, f, byteswap );
		}
    break;

		default:
			assert( false && "bad ParamInfo version!" );
			return 0;
	}

	return 1;
}

//----------------------------------------

void ParamInfo::saveBinary( FILE *f ) {
	fwrite( &ParamInfo_Version, sizeof( ParamInfo_Version ), 1, f );

	fwrite( paramName, sizeof( paramName ), 1, f );
	fwrite( &initialValue, sizeof( initialValue ), 1, f );
	fwrite( &type, sizeof( type ), 1, f );
	fwrite( &constraint, sizeof( constraint ), 1, f );
	fwrite( ratioMasterParamName, sizeof( ratioMasterParamName ), 1, f );
	fwrite( &ratio, sizeof( ratio ), 1, f );
	fwrite( &fitIndex, sizeof( fitIndex ), 1, f );
	fwrite( &bestFitValue, sizeof( bestFitValue ), 1, f );
	fwrite( &covarStdError, sizeof( covarStdError ), 1, f );
}

//----------------------------------------

void ParamInfo::clear() {
	paramName[0]			= 0;
	initialValue			= 0.0;
	type					= PT_GENERIC;
	constraint				= CT_NONE;
	ratioMasterParamName[0] = 0;
	ratio					= 1.0;
	fitIndex				= 0;
	bestFitValue			= 0.0;
	covarStdError			= 0.0;
}

//----------------------------------------

void ParamInfo::copy( ParamInfo &toCopy ) {
	memcpy( this, &toCopy, sizeof(*this) );
}

//----------------------------------------

int ParamInfo::usedByFit() {
	// TODO: a better name?
	// based on our constraint type, are we a param that will be varied during fitting
	// to find the bestfit parameter set?  
	return constraint != CT_FIXED && constraint != CT_RATIO;
		// note that CT_RATIO params are computed *algebraicly* after the actual fit
		// is complete.
}

//----------------------------------------

void ParamInfo::setRatioMaster( ParamInfo *master ) {
	constraint = CT_RATIO;
	ratio      = master->initialValue == 0.0 ? 0 : initialValue / master->initialValue;
	assert( ratio >= 0 );
	strncpy( ratioMasterParamName, master->paramName, PARAMNAME_MAXLEN );
	ratioMasterParamName[ PARAMNAME_MAXLEN-1 ] = 0;
}

//--------------------------------------------------------------------------------------
// FitData
//--------------------------------------------------------------------------------------

char ntString[8][32] = {
	"Invalid", "None", "Sigma", "SigmaAvg", "SigmaAvgExternal", "SigmaAvgAFit", "SigmaAvgAFitFamily", "MaxData"
};
char *getNormalizeTypeString( normalizeType nt ) {
	return ntString[ (int)nt + 1 ];
}
char *getNormalizeTypeString( ZTLVec< normalizeType > &normTypes, int *pcount ) {
	static char multipleTypes[512];
	multipleTypes[0] = 0;
	
	ZHashTable types;
	
	int count=0;
	for( int i=0; i<normTypes.count; i++ ) {
		if( normTypes[i] > NT_Invalid && normTypes[i] < NT_NumTypes ) {
			if( ! types.getI( ntString[ (int)normTypes[i] + 1 ] ) ) {
				if( count ) {
					strcat( multipleTypes, " " );
				}
				strcat( multipleTypes, ntString[ (int)normTypes[i] + 1 ] );
				types.putI( ntString[ (int)normTypes[i] + 1 ], 1 );
				count++;
			}
		}
		else if( normTypes[i] != NT_Invalid ) {
			strcat( multipleTypes, "bugbugbug" );
			count++;
		}
	}
	if( pcount ) {
		*pcount = count;
	}
	return multipleTypes;
}

FitData::FitData() {
	fitSystem = 0;
	jacSystem = 0;
	bOwnsFitSystem = false;

	clear();
}

//----------------------------------------

FitData::FitData( FitData &_copy ) {
	fitSystem = 0;
	jacSystem = 0;
	bOwnsFitSystem = false;

	copy( _copy );
}

//----------------------------------------

FitData::~FitData() {
	clear();
		// must dealloc objects in params hash
}

//----------------------------------------
const int FitData_Version = 20110523;
int FitData::loadBinary( FILE *f, int byteswap ) {
	// NOTE: detection of 32 vs 64 bit etc. handled at higher level.
	int version;
	freadEndian( &version, sizeof( version ), 1, f, byteswap );

	switch( version ) {
		case 20080407:
		case 20100108:
				// just adds fitUsesKnownDataSigma, so tack on to this code block
		{
			freadEndian( &fitFuncType, sizeof( fitFuncType ), 1, f, byteswap );
			freadEndian( &fitDataType, sizeof( fitDataType ), 1, f, byteswap );

			// LOAD params, and add to 3 hashtables.
			int count;
			freadEndian( &count, sizeof( count ), 1, f, byteswap );
			for( int i=0; i<count; i++ ) {
				ParamInfo *pi = new ParamInfo();
				pi->loadBinary( f, byteswap );

				// fix params names for double-digits reaction rates:
				if( i > 17 && pi->type == PT_RATE && pi->paramName[2] > '9' ) {
					strcpy( pi->paramName, KineticSystem::reactionGetRateName( i ) );
				}

				params.putP( pi->paramName, pi );
				orderedParams.bputP( &i, sizeof(i), pi );
				if( pi->fitIndex != -1 ) {
					fitIndexToParamInfo.bputP( &pi->fitIndex, sizeof(pi->fitIndex), pi );
				}
			}

			int normalizeData, fitUsesKnownDataSigma=0;
			freadEndian( &normalizeData, sizeof( normalizeData ), 1, f, byteswap );
			freadEndian( &nDataPointsToFit, sizeof( nDataPointsToFit ), 1, f, byteswap );
			freadEndian( &hasBeenSetup, sizeof( hasBeenSetup ), 1, f, byteswap );
			freadEndian( &chi2, sizeof( chi2 ), 1, f, byteswap );
			freadEndian( &dof, sizeof( dof ), 1, f, byteswap );
			freadEndian( &sigma, sizeof( sigma ), 1, f, byteswap );
			if( version == 20100108 ) {
				freadEndian( &fitUsesKnownDataSigma, sizeof( fitUsesKnownDataSigma ), 1, f, byteswap );
			}
			fitNormalizeType = normalizeData ? NT_MaxData : fitUsesKnownDataSigma ? NT_Sigma : NT_None;
			if( !covar.loadBin( f ) ) {
				return 0;
			}
		}
		break;
			
		case 20100119:
		case 20100208:
		case 20110523:
		{
			freadEndian( &fitFuncType, sizeof( fitFuncType ), 1, f, byteswap );
			freadEndian( &fitDataType, sizeof( fitDataType ), 1, f, byteswap );
			freadEndian( &fitNormalizeType, sizeof( fitNormalizeType ), 1, f, byteswap );
			forceHomogenousNormalization = 0;
			if( version > 20100119 ) {
				freadEndian( &forceHomogenousNormalization, sizeof( forceHomogenousNormalization ), 1, f, byteswap );
			}
			
			// LOAD params, and add to 3 hashtables.
			int count;
			freadEndian( &count, sizeof( count ), 1, f, byteswap );
			for( int i=0; i<count; i++ ) {
				ParamInfo *pi = new ParamInfo();
				pi->loadBinary( f, byteswap );
				
				// fix params names for double-digits reaction rates:
				if( i > 17 && pi->type == PT_RATE && pi->paramName[2] > '9' ) {
					strcpy( pi->paramName, KineticSystem::reactionGetRateName( i ) );
				}
				
				params.putP( pi->paramName, pi );
				orderedParams.bputP( &i, sizeof(i), pi );
				if( pi->fitIndex != -1 ) {
					fitIndexToParamInfo.bputP( &pi->fitIndex, sizeof(pi->fitIndex), pi );
				}
			}
			
			freadEndian( &nDataPointsToFit, sizeof( nDataPointsToFit ), 1, f, byteswap );
			freadEndian( &hasBeenSetup, sizeof( hasBeenSetup ), 1, f, byteswap );
			freadEndian( &chi2, sizeof( chi2 ), 1, f, byteswap );
			freadEndian( &dof, sizeof( dof ), 1, f, byteswap );
			freadEndian( &sigma, sizeof( sigma ), 1, f, byteswap );
			if( !covar.loadBin( f ) ) {
				return 0;
			}
			if( version == 20110523 ) {
				zHashTableLoad( f, properties );
			}
		}
		break;
			
			
		default:
			assert( false && "bad FitData version!" );
			return 0;
	}
	return 1;
}

//----------------------------------------

void FitData::saveBinary( FILE *f ) {
	// NOTE: detection of 32 vs 64 bit etc. handled at higher level.
	fwrite( &FitData_Version, sizeof( FitData_Version ), 1, f );
	fwrite( &fitFuncType, sizeof( fitFuncType ), 1, f );
	fwrite( &fitDataType, sizeof( fitDataType ), 1, f );
	fwrite( &fitNormalizeType, sizeof( fitNormalizeType ), 1, f );
	fwrite( &forceHomogenousNormalization, sizeof( forceHomogenousNormalization ), 1, f );

	// WRITE the params in the order they were inserted.
	int count = orderedParams.activeCount();
	fwrite( &count, sizeof( count ), 1, f );
	for( int i=0; i<count; i++ ) {
		ParamInfo *pi = paramByOrder( i );
		pi->saveBinary( f );
	}

	fwrite( &nDataPointsToFit, sizeof( nDataPointsToFit ), 1, f );
	fwrite( &hasBeenSetup, sizeof( hasBeenSetup ), 1, f );
	fwrite( &chi2, sizeof( chi2 ), 1, f );
	fwrite( &dof, sizeof( dof ), 1, f );
	fwrite( &sigma, sizeof( sigma ), 1, f );
	covar.saveBin( f );
	zHashTableSave( properties, f );
}

//----------------------------------------

void FitData::clear() {
	fitInProgress	= 0;
	clearFitSystem();
	fitFuncType		= FT_NONE;
	fitDataType		= DT_NONE;
	fitNormalizeType = NT_None;
	clearParams();
	nDataPointsToFit	= 0;
	hasBeenSetup		= 0;
	chi2				= 0.f;
	dof					= 0;
	gammaQ				= 0.f;
	sigma				= 0.f;
	covar.clear();
	properties.clear();

	params.clear();
	orderedParams.clear();
	fitIndexToParamInfo.clear();

	fitterJacobian = 0;
	fitOptionLnRates = 0;
	fitOptionNegErr = 0;
}

//----------------------------------------

void FitData::clearParams() {
	// We own the pointers inside the hash; delete
	for( ZHashWalk p( params ); p.next(); ) {
		delete( *((ParamInfo**)p.val) );
		*((ParamInfo**)p.val) = 0;
	}
 	params.clear();
	fitIndexToParamInfo.clear();
	orderedParams.clear();
}

//----------------------------------------

void FitData::clearFitSystem() {
	if( bOwnsFitSystem ) {
		delete fitSystem;
		delete jacSystem;
	}
	fitSystem = 0;
	jacSystem = 0;
	bOwnsFitSystem = false;
}

//----------------------------------------

void FitData::copy( FitData &toCopy, KineticSystem *fit, KineticSystem *jac, bool bOwnedSystems ) {
	clear();
		// includes dealloc of params hash, and clear of fitExps

	fitInProgress	= toCopy.fitInProgress;

	setupKineticSystems( &toCopy, fit, jac, bOwnedSystems );

	fitFuncType		= toCopy.fitFuncType;
	fitDataType		= toCopy.fitDataType;
	fitNormalizeType = toCopy.fitNormalizeType;
	forceHomogenousNormalization = toCopy.forceHomogenousNormalization;
	copyParams( toCopy );
		// copies 3 separate hashes; pointers are now uniquely owned
	nDataPointsToFit= toCopy.nDataPointsToFit;
	hasBeenSetup	= toCopy.hasBeenSetup;
	chi2			= toCopy.chi2;
	dof				= toCopy.dof;
	gammaQ			= toCopy.gammaQ;
	sigma			= toCopy.sigma;
	covar.copy( toCopy.covar );
	properties.copyFrom( toCopy.properties );

	fitOptionLnRates = toCopy.fitOptionLnRates;
	fitOptionNegErr = toCopy.fitOptionNegErr;
}

//----------------------------------------

int jacSystemHasData( KineticSystem *jac ) {
	for( int i=0; i<jac->experiments.count; i++ ) {
		if( jac->experiments[i]->measuredHasAnyData() ) {
			return 1;
		}
	}
	return 0;
}

void FitData::setupKineticSystems( FitData *initFrom, KineticSystem *fs, KineticSystem *js, bool bOwned ) {
	// this was formerly copyKineticSystem - but I want to be able to
	// deal in subclassed KintekSystems as well, so I removed the previous
	// logic that new'd up the systems, and now pass them in instead to avoid
	// subclassing FitData for this one func.
	
	if( bOwnsFitSystem ) {
		delete fitSystem;
		delete jacSystem;
	}
	fitSystem = 0;
	jacSystem = 0;

	if( !bOwned && initFrom ) {
		if( !fs ) { fs = initFrom->fitSystem; }
		if( !js ) { js = initFrom->jacSystem; }
	}

	fitSystem = fs;
	jacSystem = js;
	if( bOwned && initFrom ) {
		fs->copy( *initFrom->fitSystem );
		fs->compile();
		js->copy( *fs, 1, jacSystemHasData(initFrom->jacSystem) );
		js->compile();
	}

	bOwnsFitSystem = bOwned;
}

//----------------------------------------

void FitData::copyParams( FitData &toCopy ) {
	// make a deep copy of the passed hash of params.  This involves newing up
	// ParamInfo structs which are copied from the passed ones, such that the
	// pointers held by the two hashes after this call are unique and self-owned.
	clearParams();
	int paramCount=toCopy.params.activeCount();
	for( int i=0; i<paramCount; i++ ) {
		ParamInfo *pi    = toCopy.paramByOrder( i );
		ParamInfo *newpi = new ParamInfo( *pi );
		params.putP( newpi->paramName, newpi );
		orderedParams.bputP( &i, sizeof(i), newpi );
		if( newpi->fitIndex != -1 ) {
			fitIndexToParamInfo.bputP( &(newpi->fitIndex), sizeof(newpi->fitIndex), newpi );
		}
	}
}

//----------------------------------------

void FitData::addParameter( char *name, double initialValue, paramType pt/*=PT_GENERIC*/, constraintType ct/*=CT_NONE*/ ) {
	int count		= params.activeCount();
	ParamInfo *pi	= new ParamInfo( name, initialValue, pt, ct );
	params.putP( name, pi );
	orderedParams.bputP( &count, sizeof(count), pi );
}

//----------------------------------------

ParamInfo * FitData::paramByName( char *name ) {
	return (ParamInfo*)params.getp( name );
}

//----------------------------------------

ParamInfo * FitData::paramByFitIndex( int index ) {
	return (ParamInfo*)fitIndexToParamInfo.bgetp( &index, sizeof(index) );
}

//----------------------------------------

ParamInfo * FitData::paramByOrder( int index ) {
	return (ParamInfo*)orderedParams.bgetp( &index, sizeof(index) );
}

//----------------------------------------

int FitData::paramCount( paramType type, int bFittedOnly, int bIncludeRatioConstrained ) {
	// Count parameters of a given type (or all in case of PT_ANY), including:
	//   bFittedOnly = 0 : all paramters
	//   bFittedOnly = 1 :
	//			bIncludeRatioConstrained = 0: those actually employed by fitting routines
	//			bIncludeRatioConstrained = 1: those employed by fitter, plus those computed as ratios
	//
	// NOTE: Be sure routines that setup this structure have been
	// called (such as setupExpFitDataContexts) before relying on this value.

	// @TODO: is this a good idea?
	//
	//if( !hasBeenSetup ) {
	//	return -1;
	//}

	int count=0;
	ParamInfo *pi;
	for( ZHashWalk p( params ); p.next(); ) {
		pi = *((ParamInfo**)p.val);
		if( type==PT_ANY || pi->type==type ) {
			if( !bFittedOnly || ( pi->usedByFit() || ( bIncludeRatioConstrained && pi->constraint==CT_RATIO ) ) ) {
				count++;
			}
		}
	}
	return count;
}

//----------------------------------------

void FitData::swapParamDataPreservingNames( ParamInfo *p1, ParamInfo *p2 ) {
	// Exchange all data between ParamInfo structs, but preserve the original
	// paramName and fitIndex of each.

	// ALL hashes will remain valid since the pointer values held in hashes don't 
	// change.  Looking up a paramInfo by name, fitIndex, or orderadded will always
	// get back the same ParamInfo * and contain the same paramName and fitIndex,
	// but the bestFitValues etc... will have changed.
	 
	ParamInfo temp( *p1 );
	p1->copy( *p2 );
		// p1 now looks like p2
	strcpy( p1->paramName, temp.paramName );
	p1->fitIndex = temp.fitIndex;
		// but preserve p1 name and fitIndex

	ParamInfo temp2( *p2 );
	p2->copy( temp );
		// p2 looks like the original p1
	strcpy( p2->paramName, temp2.paramName );
	p2->fitIndex = temp2.fitIndex;
		// but preserve p2 name and fitIndex
}

//----------------------------------------

void FitData::copyBestFitValuesToInitialValues() {
	// Copy bestFitValue to initialValue for all parameters.
	ParamInfo *pi;
	for( ZHashWalk p( params ); p.next(); ) {
		pi = *((ParamInfo**)p.val);
		pi->initialValue = pi->bestFitValue;
	}
}

//----------------------------------------

void FitData::copyValuesToKineticSystem( KineticSystem & kSystem, int bestFit ) {
	// COPY the current bestFitValue for all params to the given model and experiments.
	// ALL params are copied; those held fixed will simply have bestFitValue == initialValue.
	//
	// experiments may be NULL
	ParamInfo *pi;
	KineticParameterInfo *kpi;

	// NEW: oct 2015 - just iterate over every parameter, and update anything that we 
	// find in the FitData parameters.  The old way is commeted out below.
	//

	for( int i=0; i<kSystem.parameterInfo.count; i++ ) {
		KineticParameterInfo *kpi = &kSystem.parameterInfo[i];
		ZTmpStr name( "%s", kpi->name );
		if( kpi->type == PI_INIT_COND ) {
			// Compose the name for the initial condition with series experiment and mixstep
			// information.  We rely on the fact that series experiments have their IC values
			// set as appropriate in this FitData, even if they are not being fit.
			KineticExperiment *e = kSystem.experiments[ kpi->experiment ];
			name.set( "%s_e%d_m%d", kpi->name, e->id, e->mixsteps[kpi->mixstep].id );
		}
		pi = paramByName( name );
		if( pi ) {
			kpi->value = bestFit ? pi->bestFitValue : pi->initialValue;
		}
	}	
}

void FitData::copyBestFitValuesToKineticSystem( KineticSystem & kSystem ) {
	copyValuesToKineticSystem( kSystem, 1 );
}

void FitData::copyInitialValuesToKineticSystem( KineticSystem & kSystem ) {
	copyValuesToKineticSystem( kSystem, 0 );
}


//----------------------------------------

void FitData::resetBestFitValueLast() {
	// reset all bestFitValueLast in ParamInfo to 0
	ParamInfo *pi;
	for( ZHashWalk p( params ); p.next(); ) {
		pi = *((ParamInfo**)p.val);
		pi->bestFitValueLast = 0;
			// reset state info used by jacobian
	}
}

//----------------------------------------

void FitData::clearConstraintType( constraintType ct ) {
	// reset any contraints of given type to CT_NONE
	ParamInfo *pi;
	for( ZHashWalk p( params ); p.next(); ) {
		pi = *((ParamInfo**)p.val);
		if( pi->constraint == ct ) {
			if( ct == CT_RATIO ) {
				// clear ratio-related
				pi->ratio					= 1.0;
				pi->ratioMasterParamName[0] = 0;
			}
			pi->constraint = CT_NONE;
		}
	}
}

//----------------------------------------

void setupRatioConstraints( ParamInfo **linkedParams, int linkCount, ParamInfo *basisParam=0 ) {
	if( linkCount ) {
		// The "basisParam" is the param on which other params will be based.  
		if( !basisParam ) {
			basisParam = linkedParams[ 0 ];
		}
		for( int f=0; f<linkCount; f++ ) {
			ParamInfo *pi = linkedParams[ f ];
			if( pi != basisParam ) {
				pi->setRatioMaster( basisParam );
					// set the constraint type, ratio info, etc.
			}
		}
	}
}

void FitData::setupParamRatioConstraintsFromSystem( KineticSystem &kSystem ) {
	// Setup the ratio contraints for our params based on the settings in
	// the passed model.  This requires that our params hash has been setup
	// with parameters to be used for the current fit, including initial values.
	// Typically accomplished by calling with initFitParams or setupExpFitDataContexts.

	clearConstraintType( CT_RATIO );

	int pCount = paramCount();
	int numGroups = 256;
		// group numbering: 0->no group; 1->"fixed", >=2 -> a ratio group
		// in practice, user-spec'd groups are 0-10, but we emply special
		// groups starting at 100 to link labeled/unlabeled pathways behind
		// the scenes.  So 256 is just an arbitrarily large number we're not 
		// ever likely to come close to.
	
	ParamInfo **linkedParams = (ParamInfo**)alloca( pCount * sizeof(ParamInfo*) );

	for( int g=2; g<numGroups; g++ ) {
		int linkCount = 0;
		memset( linkedParams, 0, pCount * sizeof(ParamInfo*) );

		// Add all params that are members of group g to the list, then setup 
		// the ratio constraints for that list.
		//
		
		// OLD WAY: order of params is arbitrary
		// for( ZHashWalk w(params); w.next(); ) {
		// 	char *name = (char*)w.key;
		// 	ParamInfo *pi = *((ParamInfo**)w.val);

		// NEW WAY: would like to process params in order added
		// by the user, so that ratio masters are earliest params.
		int count = orderedParams.activeCount();
		for( int i=0; i<count; i++ ) {
			ParamInfo *pi = paramByOrder( i );
			
			KineticParameterInfo *kpi = kSystem.paramGetByName( pi->paramName );

			if( kpi->group == g ) {
				linkedParams[ linkCount++ ] = pi;
			}
		}

		// Now, setup the constraints.
		//
		setupRatioConstraints( linkedParams, linkCount );
	}
}

//----------------------------------------

int FitData::createParamVectorFromParams( double **pv, int useBestFitValues /*=0*/ ) {
	// ALLOCATE & POPULATE a double* containing the inital parameter values 
	// for a fit by examining parameter specs.  We we also set the fitIndex for
	// each ParamInfo struct involved in the fit, allowing one to look up the
	// position in the double* parameter array by name via fd.params.

	// Returns: actual parameter count allocated & initialized.

	int vecsize   = paramCount( PT_ANY, 1 );
	if( !vecsize ) {
		return 0;
	}
	double *v = (double*)malloc( vecsize * sizeof(double) );

	int count = 0;
	ParamInfo *pi;
	int totalParams = params.activeCount();
	for( int i=0; i<totalParams; i++ ) {
		pi = paramByOrder( i );
		if( pi->usedByFit() ) {
			pi->fitIndex = count;
			fitIndexToParamInfo.bputP( &count, sizeof(count), pi );
			double value = useBestFitValues ? pi->bestFitValue : pi->initialValue;
			if( pi->constraint == CT_NONNEGATIVE ) {
				value = sqrt( value );
					// ye olde "fit the square root and square the output trick"
			}
			else if( pi->constraint == CT_NONPOSITIVE ) {
				value = sqrt( fabs(value) );
					// ye olde "fit the square root and square the output trick"
			}
			else if( pi->constraint == CT_BOX && pi->type == PT_RATE ) {
				if( fitOptionLnRates ) {
					if( value == 0.0 ) {
						value = 1e-15;
					}
					value = log( value );
				}
			}

			v[count++] = value;
		}
		else {
			pi->fitIndex = -1;
		}
	}
	assert( count == vecsize );
	
	*pv = v;
	return count;
}

//----------------------------------------

int FitData::createParamBoundsVectors( double **lb, double **ub ) {
	//
	// alloc and populate double* vectors that hold upper and lower bounds
	// for parameters that are being fit.
	//
	int vecsize   = paramCount( PT_ANY, 1 );
	if( !vecsize ) {
		return 0;
	}
	double *l = (double*)malloc( vecsize * sizeof(double) );
	double *u = (double*)malloc( vecsize * sizeof(double) );

	ZRegExp scaleFactor( "scale_(\\d+)[a-z]" );
	ZRegExp offsetFactor( "offset_(\\d+)[a-z]" );

	int count = 0;
	ParamInfo *pi;
	int totalParams = params.activeCount();
	for( int i=0; i<totalParams; i++ ) {
		pi = paramByOrder( i );
		if( pi->usedByFit() ) {

			#define KIN_MAXPARAMVALUE 1e15
				// duplicated from _kin.h

			l[ count ] = -KIN_MAXPARAMVALUE;
			u[ count ] = +KIN_MAXPARAMVALUE;

			if( pi->constraint == CT_BOX ) {
				// The only constraint-type that gets non-default bounds...  The user has 
				// Set the bounds via UI, or we get defaults based on the name-class of the param.
				if( scaleFactor.test( pi->paramName ) ) {
					l[ count ] = properties.getD( "scaleConstantL", 0.9 );
					u[ count ] = properties.getD( "scaleConstantU", 1.1 );
				}
				else if( offsetFactor.test( pi->paramName ) ) {
					l[ count ] = properties.getD( "offsetConstantL", -0.1 );
					u[ count ] = properties.getD( "offsetConstantU", +0.1 );
				}
				else {
					l[ count ] = properties.getD( ZTmpStr( "%sL", pi->paramName ), 0.0 );
					u[ count ] = properties.getD( ZTmpStr( "%sU", pi->paramName ), +KIN_MAXPARAMVALUE );					
					if( pi->type == PT_RATE && fitOptionLnRates ) {
						l[count] = log( max(1e-15, l[count]) );
						u[count] = log( u[count] );
					}
				}
				pi->lb = l[count];
				pi->ub = u[count];
					// NOTE: the pi-> members are used as a conenvience for debugging info.  The current
					// fitting routine (levmar) uses these arrays of double to read boundaries.
				printf( "Bounds for parameter %s: %g, %g\n", pi->paramName, l[count], u[count] );

			}
			count++;
		}
	}
	assert( count == vecsize );
	
	*lb = l;
	*ub = u;

	return count;
}

//----------------------------------------

void FitData::updateParamsFromParamVector( const double *v ) {
	// the double *v contains the fitter's current guess at best fits 
	// to attempt.  We want to store these values in our bestFitValue member
	// of each ParamInfo, while also moving the previous val to bestFitValueLast.

	int nParams = paramCount( PT_ANY );
	for( int i=0; i<nParams; i++ ) {
		ParamInfo *pi = paramByOrder( i );
		assert( pi );
		pi->bestFitValueLast = pi->bestFitValue;
		if( pi->usedByFit() ) {
			pi->bestFitValue = v[pi->fitIndex];
			if( pi->constraint == CT_NONNEGATIVE ) {
				pi->bestFitValue = pi->bestFitValue * pi->bestFitValue;
					// ye olde "fit the square root and square the output trick"
			}
			else if( pi->constraint == CT_NONPOSITIVE ) {
				pi->bestFitValue = -(pi->bestFitValue * pi->bestFitValue);
					// ye olde "fit the square root and square the output trick"
			}
			else if( pi->constraint == CT_BOX && pi->type == PT_RATE ) {
				if( fitOptionLnRates ) {
					pi->bestFitValue = exp( pi->bestFitValue );
				}
			}
		}
		else {
			// not used by fit, so bestFit is just initial
			pi->bestFitValue = pi->initialValue;
		}
	}
	computeRatioParamValues();
		// update ratio params to reflect new values
}

//----------------------------------------

void FitData::computeCovarFromJacobian( const double *v, const double *J, int rowMajor ) {
	// Compute the covariance matrix as (JT*J)^-1
	// This involves factoring chain-rule terms out of J before doing linear algebra.
	// These terms are based on the parameter values as utilized by the fitter, so
	// these are passed in v.   
	// 

	ZMat jac;
	int nParamsToFit = paramCount( PT_ANY, 1 );
	if( rowMajor ) {
		ZMat _jac;
		_jac.alloc( nParamsToFit, nDataPointsToFit, zmatF64 );
		_jac.copyDataD( (double*)J );
		zmatTranspose( _jac, jac );
	
	}
	else {
		jac.alloc( nDataPointsToFit, nParamsToFit, zmatF64 );
		jac.copyDataD( (double*)J );
	}

	// Divide out any chain terms that were applied during the jacobian calculation.
	// The params passed in v are those used by the fitter in computing the jacobian,
	// so for example may be the square or natural log of the "real" parameter value.
	// This is what creates the need for the chain terms which we now seek to remove,
	// so that errors reported on the parameters refer to the "real" parameters, and
	// are not instead errors on the composite function of the parameter.
	//
	for( int i=0; i<nParamsToFit; i++ ) {
		ParamInfo *pi = paramByFitIndex( i );
		if( pi->constraint == CT_BOX && pi->type == PT_RATE ) {
			if( fitOptionLnRates ) {
				double chainTerm = exp( v[i] );
				for( int n=0; n<nDataPointsToFit; n++ ) {
					jac.putD( n, i, jac.getD( n, i ) / chainTerm );
				}
			}
		}
		else if( pi->constraint == CT_NONNEGATIVE || pi->constraint == CT_NONPOSITIVE ) {
			double chainTerm = 2.0 * v[i];
			for( int n=0; n<nDataPointsToFit; n++ ) {
				jac.putD( n, i, jac.getD( n, i ) / chainTerm );
			}
		}
	}
	
	// Now get the covar from J.  Note that historically covar has been a zmatF32 matrix,
	// so we are retaining that format, even though jac is zmat64.
	extern double zmatPseudoInverseViaSVD( ZMat &in, ZMat &out );
	ZMat JT, JTJ, JTJinv;
	zmatTranspose( jac, JT );
	zmatMul( JT, jac, JTJ );
	zmatPseudoInverseViaSVD( JTJ, JTJinv );
	assert( JTJinv.cols == nParamsToFit && JTJinv.rows == nParamsToFit && "JTJinv bad dims" );
	covar.alloc( nParamsToFit, nParamsToFit, zmatF32 );
	for( int c=0; c<JTJinv.cols; c++ ) {
		for( int r=0; r<JTJinv.rows; r++ ) {
			double f = JTJinv.getD( r, c );
			if( !( f>DBL_MIN && f<DBL_MAX ) ) {
				f = 0.0;
					// this handles nan, inf
			}
			if( f > FLT_MAX ) {
				f = FLT_MAX;
					// until we convert the covar matrix to zmat64
			}
			covar.putF( r, c, (float)f );
		}
	}
}

//----------------------------------------

void FitData::updateParamErrorsFromCovar( double errScale ) {
	// set the error on fitted parameters as sqrt(diag(covar)) * errScale
	int nParams = paramCount( PT_ANY );
	for( int i=0; i<nParams; i++ ) {
		ParamInfo *pi = paramByOrder( i );
		assert( pi );
		if( pi->usedByFit() ) {
			pi->covarStdError = sqrt(covar.getF( pi->fitIndex, pi->fitIndex )) * errScale;
		}
		else {
			// not used by fit, so bestFit is just initial
			pi->covarStdError = 0.0;
		}
	}
}

//----------------------------------------

int FitData::fitIndexByParamName( char *name ) {
	// inefficient, but not a bottleneck.
	ParamInfo *pi;
	for( ZHashWalk p( params ); p.next(); ) {
		pi = *((ParamInfo**)p.val);
		if( !strcmp( name, pi->paramName) ) {
			return pi->fitIndex;
		}
	}
	return -2;
		// -1 means param name found, but not being fit. -2 means param name not found.
}

//----------------------------------------

void FitData::computeRatioParamValues() {
	for( ZHashWalk p( params ); p.next(); ) {
		ParamInfo *pi = *((ParamInfo**)p.val);
		if( pi->constraint == CT_RATIO ) {
			// compute param value as ratio of master param (that *is* fit)
			ParamInfo *master	= paramByName( pi->ratioMasterParamName );
			pi->bestFitValue	= pi->ratio * master->bestFitValue;
			pi->covarStdError = pi->ratio * master->covarStdError; 
		}
	}
}

//----------------------------------------

double* FitData::getDataToFit( double *vec /*=0*/ ) {
	assert( fitSystem && nDataPointsToFit && "no fitSystem or data in getDataToFit" );
	if( !vec ) {
		vec = (double*)malloc( sizeof(double) * nDataPointsToFit );
		assert( vec && "alloc failed in getDataToFit" );
	}
	int count=0;
	for( int n=0; n<fitSystem->experiments.count; n++ ) {
		KineticExperiment & trialExp = *(fitSystem->experiments[n]);
		fitDataContext * fdc = &trialExp.fdc;
		for( int j=0; j<fdc->observablesCount; j++ ) {
			if( !fdc->fitObservable[j] ) continue;	

			int row = fdc->dataRow[j];
			int col = fdc->stepOffset[j];

			for( int i=0; i<fdc->numSteps[j]; i++, col++ ) {
				vec[count++] = fdc->dataToFit[j]->getData( col, row );
			}
		}
	}
	assert( count == nDataPointsToFit && "wrong count in getDataToFit" );
	return vec;
}

//----------------------------------------

void FitData::print() {

	double chi			= sqrt( chi2 );
	double c			= max( 1, chi / sqrt((double)dof) );
		// if chi2/dof is >> 1, the error term from covar matrix 
		// will be too small.  In this case it is common to multiply the error
		// by chi/sqrt(dof) to compensate.  (according to gsl docs)
		// see example & docs in Nonlinear LeastSquare Fitting chapter.
	// 2016 May: if the fit was NOT normalized by known sigma, the chi2/dof metric
	// is meaningless.
	if( properties.getI( "errorsScaledBySigma" ) ) {
		c = 1.0;
	}

	int count = params.activeCount();
	trace( "FitData chi2=%g, c=%g, nData=%d, normalizeType=%s\n", chi2, c, nDataPointsToFit, getNormalizeTypeString(fitNormalizeType) );
	trace( " %c %-15s %-12s %-12s %-12s\n", ' ', "name", "initial", "bestfit", "stderr*c" );
	char initial[16], bestfit[16], err[16];
	for( int i=0; i<count; i++ ) {
		ParamInfo *pi = paramByOrder( i );
		double iVal = pi->initialValue;
		double bVal = pi->bestFitValue;
//		if( pi->constraint == CT_NONNEGATIVE ) {
//			iVal = iVal * iVal;
//			bVal = bVal * bVal;
//		}
		sprintf( initial, "%g", iVal );
		sprintf( bestfit, "%g", bVal );
		sprintf( err, "%g", pi->covarStdError * c );
		trace( " %c %-15s %-12s %-12s %-12s\n", pi->constraint==CT_FIXED ? 'X' : 
										pi->constraint==CT_NONNEGATIVE ? '+' :
										pi->constraint==CT_NONPOSITIVE ? '-' :
										pi->constraint==CT_RATIO ? 'R' :
										pi->constraint==CT_BOX ? 'B' : ' ',
										pi->paramName, initial, bestfit, err );
		if( pi->constraint == CT_RATIO ) {
			printf( " ( == %g * %s )", pi->ratio, pi->ratioMasterParamName );
		}
		if( pi->constraint == CT_BOX ) {
			printf( " [ %g, %g ]", pi->lb, pi->ub );	
		}
		printf( "\n" );
	}
}


//--------------------------------------------------------------------------------------
// FitSet
//--------------------------------------------------------------------------------------

FitSet::FitSet() {
	arraySize = FITSET_DEFAULTSIZE;
	ppFitData = (FitData**)malloc( arraySize * sizeof(FitData*) );
	assert( ppFitData );
	fitCount  = 0;
	sortKey[0]= 0;
}

//----------------------------------------

FitSet::~FitSet() {
	clear();
}

//----------------------------------------

const int FitSet_Version = 20080407;
int FitSet::loadBinary( FILE *f, int byteswap ) {
	int version;
	freadEndian( &version, sizeof( version ), 1, f, byteswap );

	clear( true );

	switch( version ) {
		case 20080407: {
//			unsigned int propSize;
//			freadEndian( &propSize, sizeof( propSize ), 1, f, byteswap );
//			char *prop = (char *)malloc( propSize );
//			fread( prop, propSize, 1, f );
//			properties.clear();
//			zHashTableUnpack( prop, properties );
//			free( prop );

			properties.clear();
			zHashTableLoad( f, properties );

			// convert to new tolerance format:
			double tolerance = properties.getD( "tolerance" );
			if( tolerance < 1 ) {
				properties.putD( "tolerance", tolerance + 1.0 );
			}
			
			int _fitCount;
			freadEndian( &_fitCount, sizeof( _fitCount ), 1, f, byteswap );
			for( int i=0; i<_fitCount; i++ ) {
				FitData *fd = new FitData();
				fd->loadBinary( f, byteswap );
				addFit( fd );
			}
			fread( sortKey, sizeof( sortKey ), 1, f );
		}
		break;

		default:
			assert( false && "bad FitSet version!" );
			return 0;
	}
	return 1;
}

//----------------------------------------

void FitSet::saveBinary( FILE *f ) {
	fwrite( &FitSet_Version, sizeof( FitSet_Version ), 1, f );

	// properties hash
	//unsigned int propSize;
	//char *prop = zHashTablePack( properties, &propSize );
	//fwrite( &propSize, sizeof( propSize ), 1, f );
	//fwrite( prop, propSize, 1, f );
	//free( prop );

	zHashTableSave( properties, f );


	// fitdata contained 
	fwrite( &fitCount, sizeof( fitCount ), 1, f );
	for( int i=0; i<fitCount; i++ ) {
		ppFitData[i]->saveBinary( f );
	}

	// misc
	fwrite( sortKey, sizeof( sortKey ), 1, f );
}

//----------------------------------------

void FitSet::clear( bool bDeallocFitData/*=true*/ ) {
	// Note that the flag controls only whether the individual pointers
	// to FitData are deallocated, since someone else may choose to own
	// these.  But we always deallocate ppFitData array, which is alloc'd
	// by us to hold these pointers.
	properties.clear();
	if( bDeallocFitData ) {
		for( int i=0; i<fitCount; i++ ) {
			if( ppFitData[i] ) {
				free( ppFitData[ i ] );
				ppFitData[ i ] = 0;
			}
		}
	}
	delete ppFitData;
	ppFitData = 0;
	arraySize = 0;
	fitCount  = 0;
	sortKey[0]= 0;
}
//----------------------------------------

// not sure if this will be useful...may not want to add until you know you
// have a good fit...or you could attempt until you get a good one.
FitData * FitSet::allocFit( FitData &initFromResults ) {
	// ALLOCATE and ADD to set
	FitData *fd = new FitData( initFromResults );
	fd->copyBestFitValuesToInitialValues();
	addFit( fd );
	return fd;
}

//----------------------------------------

// the thing I don't like about this one is that the client alloc'd the memory,
// but we want to own and free it.  Problematic typically in multiheap impl. eg dlls.
void FitSet::addFit( FitData *fd, int alreadyLocked ) {
	if( !alreadyLocked ) {
		lock();
	}
	if( !ppFitData ) {
		assert( !fitCount && !arraySize );
		arraySize = FITSET_DEFAULTSIZE;
		ppFitData = (FitData**)malloc( arraySize * sizeof(FitData*) );
		assert( ppFitData );
	}
	if( fitCount >= arraySize ) {
		FitData **newArray = (FitData**)realloc( ppFitData, arraySize * sizeof(FitData*) * 2 );
		assert( newArray );
		ppFitData = newArray;
		arraySize *= 2;
	}
	ppFitData[ fitCount++ ] = fd;
	if( !alreadyLocked ) {
		unlock();
	}
}

//----------------------------------------
// Sorting: the FitData pointers may be sorted by named quantity

void FitSet::sortByValue( char *name ) {
	// check for valid name to sort by.
	// current valid names are "Chi2" or a param name.
	if( fitCount ) {
		if( strcmp( name, "Chi2" ) ) {
			FitData *fd = ppFitData[0];
			if( !fd->paramByName( name ) ) {
				// doesn't match Chi2 or a param name.
				assert(false);
				return;
			}	
		}
		strcpy( sortKey, name );
		sort( ppFitData, fitCount );
		// debug;
		/*
		printf( "Sorted '%s' by %s:\n", properties.getS( "name", "unknown" ), name );
		for( int c=0; c<fitCount; c++ ) {
			FitData *fd = ppFitData[c];
			printf( "  ppFitData[%d] = %g\n", c, getSortValue( c ) );
		}
		*/
	}
	else {
		// assume name is good.
		strcpy( sortKey, name );
	}
}

void FitSet::sort( FitData **data, int count ) {
	// recursive quicksort; coded in favor of qsort to provide thread-safe
	// access to (non-static) member data.

	
	int i, j;
	FitData *t;

	if( count <= 1 ) {
		return;
	}

	// Partition elements
	double v = getSortValue( 0, data );
	i = 0;
	j = count;
	for(;;) {
		while( ++i < count && getSortValue( i, data ) < v ) {}
			// inc i until the ith entry value is less than v;
		while( --j >= 0 && getSortValue( j, data ) > v ) {}
			// dec j until the jth entry is greater than v
		if(i >= j) break;
			// if i and j overlap, 
		t = data[i]; data[i] = data[j]; data[j] = t;
	}
	t = data[i-1]; data[i-1] = data[0]; data[0] = t;
	sort( data, i-1 );
	sort( data+i, count-i );
}

//----------------------------------------

double FitSet::getSortValue( int index, FitData **data/*=0*/ ) {
	assert( index>=0 && index<fitCount && "index out of range" );

	// The optional data param is provided for use by the recursive sort
	if( !data ) {
		data = ppFitData;
	}

	// from the FitData* at index, return the value of the current sortKey
	if( sortKey[0] ) {
		if( !strcmp( sortKey, "Chi2" ) ) {
			return data[index]->chi2;
		}
		else {
			return data[index]->paramByName( sortKey )->bestFitValue;
		}
	}
	assert(false && "no sortKey defined" );
	return -1.0;
}

//----------------------------------------

double FitSet::getMinValue() {
	// assumes FitSet has been sorted via sortByValue()
	return getSortValue( 0 );
}

//----------------------------------------

double FitSet::getMaxValue() {
	// assumes FitSet has been sorted via sortByValue()
	return getSortValue( fitCount-1 );
}

//----------------------------------------

FitData * FitSet::getNearestByValue( double val ) {
	// assumes FitSet has been sorted via sortByValue() such that sortKey has
	// been set. return the FitData whose sortKey value is closest to val.
	FitData * result = 0;
	if( sortKey[0] ) {
		double mindist = DBL_MAX;
		for( int i=0; i<fitCount; i++ ) {
			double compareVal = getSortValue( i );
			double dist       = fabs( compareVal - val );
			if( dist < mindist ) {
				result	= ppFitData[ i ];
				mindist	= dist;
			}
		}
	}
	return result;
}

//----------------------------------------

void FitSet::getHistogram( int *bins, int count ) {
	// assumes FitSet has been sorted via sortByValue()
	// fill user-supplied array of count length by examining the key value
	// in each FitData based on the current sortKey and getMinValue/MaxValue.

	double maxv = getMaxValue();
	double minv = getMinValue();
	double diff = maxv - minv;
	double binsize = ( diff + .0001 ) / count;
	memset( bins, 0, count * sizeof(int) );
	double minval = getSortValue( 0 );
	for( int j=0; j<fitCount; j++ ) {
		int index = (int)( ( getSortValue( j ) - minval )  / binsize ); 
		assert( index <= count );
		if( index == count ) index = count-1;
			// due to precision issues, it is possible we step past bound
		bins[ index ]++;
	}
}

void FitSet::dump( char *sortBy ) {
	trace( "FitSet %s, %d fits, sorted by %s:\n", properties.getS( "name", "Unnamed" ), fitCount, sortBy );
	sortByValue( sortBy );
	int pcount = ppFitData[0]->paramCount();
	for( int i=0; i<fitCount; i++ ) {
		FitData *fd = ppFitData[ i ];
		for( int p=0; p<pcount; p++ ) {
			ParamInfo *pi = fd->paramByOrder( p );
			assert( pi );
			if( pi->paramName[0] == 'k' ) {
				trace( "%s=%.2lf\t", pi->paramName, pi->bestFitValue );
			}
		}
		trace( "chi2=%g\n", fd->chi2 );
	}
}

//--------------------------------------------------------------------------------------
// GridSet
//--------------------------------------------------------------------------------------

GridSet::GridSet() {
}

//----------------------------------------

GridSet::~GridSet() {
	clear();
}

//----------------------------------------
const int GridSet_Version = 20080617;
int GridSet::loadBinary( FILE *f, int byteswap ){ 
	int version;
	freadEndian( &version, sizeof( version ), 1, f, byteswap );

	switch( version ) {
		case 20080617: {
			assert( sizeof( spatialToFitData.xMin ) == sizeof( float ) );
			assert( sizeof( spatialToFitData.xBins ) == sizeof( int ) );
			float xMin,xMax,yMin,yMax;
			int   xBins,yBins;
			freadEndian( &xMin, sizeof( xMin ), 1, f, byteswap );
			freadEndian( &xMax, sizeof( xMax ), 1, f, byteswap );
			freadEndian( &yMin, sizeof( yMin ), 1, f, byteswap );
			freadEndian( &yMax, sizeof( yMax ), 1, f, byteswap );
			freadEndian( &xBins, sizeof( xBins ), 1, f, byteswap );
			freadEndian( &yBins, sizeof( yBins ), 1, f, byteswap );

			lowerBoundsDetailP1.loadBinary( f, byteswap );
			lowerBoundsDetailP2.loadBinary( f, byteswap );

			// FitSet loads most of our data members, including all FitData structs
			if( !FitSet::loadBinary( f, byteswap ) ) {
				return 0;
			}
			if( xMin == xMax ) {
				xMax = xMin + .001f;
			}
			if( yMin == yMax ) {
				yMax = yMin + .001f;
			}
			initSpatialHash( xBins, xMin, xMax, yMin, yMax );
		}
		break;

		default:
			assert( false && "bad GridSet version!" );
			return 0;

	}
	return 1;
}

//----------------------------------------

void GridSet::saveBinary( FILE *f ) {
	fwrite( &GridSet_Version, sizeof( GridSet_Version ), 1, f );

	// Save stats about binary hash to avoid recomputing stuff on load
	assert( sizeof( spatialToFitData.xMin ) == sizeof( float ) );
	assert( sizeof( spatialToFitData.xBins ) == sizeof( int ) );
	fwrite( &spatialToFitData.xMin, sizeof( spatialToFitData.xMin ), 1, f );
	fwrite( &spatialToFitData.xMax, sizeof( spatialToFitData.xMax ), 1, f );
	fwrite( &spatialToFitData.yMin, sizeof( spatialToFitData.yMin ), 1, f );
	fwrite( &spatialToFitData.yMax, sizeof( spatialToFitData.yMax ), 1, f );
	fwrite( &spatialToFitData.xBins, sizeof( spatialToFitData.xBins ), 1, f );
	fwrite( &spatialToFitData.yBins, sizeof( spatialToFitData.yBins ), 1, f );

	// Save detail fitsets for lower bound
	lowerBoundsDetailP1.saveBinary( f );
	lowerBoundsDetailP2.saveBinary( f );
	
	FitSet::saveBinary( f );
}

//----------------------------------------

void GridSet::clear( bool bDeallocFitData ) {
	FitSet::clear( bDeallocFitData);
	clearSpatialHash();
	lowerBoundsDetailP1.clear();
	lowerBoundsDetailP2.clear();
}

//----------------------------------------

void GridSet::clearSpatialHash() {
	// FREE nodes in spatial hash: you can't delete nodes while walking
	// via ZHash2DEnum because of how the enumerator works.
	for( int i=0; i<spatialToFitData.xBins; i++ ) {
		for( int j=0; j<spatialToFitData.yBins; j++ ) {
			GridNode *node = (GridNode*)*spatialToFitData.getBinByRowCol( j, i );
			while( node ) {
				GridNode *delnode = node;
				node = (GridNode*)node->next;
				delete delnode;
			}
		}
	}
	spatialToFitData.clear();
}

//----------------------------------------

void GridSet::addFit( FitData *fd, double p1, double p2 ) {
	// debug
	static int lastrow, lastcol;
	if( !fitCount ) {
		lastrow = -1;
		lastcol = -1;
	}
	// end debug
		
	FitSet::addFit( fd );
	GridNode *g = new GridNode( fd );
		
	int row, col;
	spatialToFitData.getBinRowCol( (float)p1, (float)p2, row, col );
	spatialToFitData.insert( g, (float)p1, (float)p2 );

	//debug:
	if( lastrow == row && lastcol == col ) {
		// printf( "inserted duplicate entry to spatial hash!\n"  );
		// assert(false && "inserted duplicate entry to spatial hash!" );
	}
	lastrow = row;
	lastcol = col;
	// end debug

	/*  verbose debug info
	printf( "  SpatialHash: insert ( %g, %g ) at row:col [ %d, %d ] -->\n", p1,p2,row+1,col+1 );
	int steps = properties.getI( "gridsteps" );
	for( int i=-1; i<steps; i++ ) {
		for( int j=-1; j<steps; j++ ) {
			if( i<0 && j>=0) {
				
				printf( "%s  %02d", j==0 ? "  " : "", j+1 );
					// col label
			}
			else if( i>=0 && j<0 ) {
				printf( "%02d  ", i+1 );
					// row label
			}
			else if( i>=0 && j>=0 ) {
				GridNode *node = (GridNode*)*spatialToFitData.getBinByRowCol( i, j );
				printf( " %c  ", node ? 'X' : ' ' );
					// hash entry
			}
			if( j==steps-1 ) {
				printf( "\n" );
			}
		} 
	}
	*/
}

//----------------------------------------

void GridSet::initSpatialHash( int bins, double xmin, double xmax, double ymin, double ymax ) {
	clearSpatialHash();
	spatialToFitData.init( bins, bins, (float)xmin, (float)xmax, (float)ymin, (float)ymax );

	// if this gridset already contains fits, add them to the spatial hash
	char *paramName1 = 	properties.getS( "paramName1" );
	char *paramName2 = 	properties.getS( "paramName2" );
	if( paramName1 && paramName2 && fitCount ) {
		for( int i=0; i<fitCount; i++ ) {
			GridNode *g = new GridNode( ppFitData[i] );
			spatialToFitData.insert( g, (float)ppFitData[i]->paramByName( paramName1 )->bestFitValue,
										(float)ppFitData[i]->paramByName( paramName2 )->bestFitValue );
		}
	}
}

//----------------------------------------

FitData * GridSet::fitFromCoords( double p1, double p2 ) {
	GridNode *node = (GridNode*)*spatialToFitData.getBin( (float)p1, (float)p2 );
	if( node ) {
		if( node->next ) {
			printf( "WARNING: more than one fit found in GridSet spatial hash bin!\n" );
		}
		return node->fd;
	}
	return 0;
}

//----------------------------------------

bool GridSet::hasEmptyNeighbor( FitData *seed, FitData **ppNeighbor ) {
	// does the seed have a neighbor that needs sampling?
	// if ppNeighbor is non-NULL, alloc a new FitData for the neighbor,
	// and set it up with params for it's fit.

	char *pname1  = properties.getS( "paramName1" );
	char *pname2  = properties.getS( "paramName2" );
	assert( pname1 && pname2 );

	ParamInfo *p1 = seed->paramByName( pname1 );
	ParamInfo *p2 = seed->paramByName( pname2 );

	int row, col, steps;
	spatialToFitData.getBinRowCol( (float)p1->bestFitValue, (float)p2->bestFitValue, row, col );
	steps = properties.getI( "gridsteps" );

	// If x is our seed, we have 8 neighbors on the grid we'll have a look at.
	// (is it advantagious to search in a specific direction?)
	// 
	//  1  2  3
	//  8  x  4
	//  7  6  5
	
	// clockwise circle including all neighbors:
	#define NEIGHBOR_COUNT 8
	// for neighbor:					   1   2   3   4   5   6   7   8
	int rowOffsets[NEIGHBOR_COUNT] = { -1, -1, -1,  0,  1,  1,  1,  0 };
	int colOffsets[NEIGHBOR_COUNT] = { -1,  0,  1,  1,  1,  0, -1, -1 };

	// exclude diagonals:
	//#define NEIGHBOR_COUNT 4
	// pick neighbors in this order: 4,8,2,6
	//int rowOffsets[NEIGHBOR_COUNT] = { 0,  0, -1, 1 };
	//int colOffsets[NEIGHBOR_COUNT] = { 1, -1,  0, 0 };

	for( int i=0; i<NEIGHBOR_COUNT; i++ ) {
		int _row = row + rowOffsets[i];
		int _col = col + colOffsets[i];
		if( _row < 0 || _row >= steps || _col < 0 || _col >= steps ) {
			continue;
		}
		GridNode *node = (GridNode*)*spatialToFitData.getBinByRowCol( _row, _col );
		if( !node ) {
			if( ppNeighbor ) {
				// ALLOC & SETUP a new fit
				*ppNeighbor = new FitData( *seed );
				(*ppNeighbor)->copyBestFitValuesToInitialValues();
				p1 = (*ppNeighbor)->paramByName( pname1 );
				p2 = (*ppNeighbor)->paramByName( pname2 ); 

				float cx, cy;
				spatialToFitData.getBinCentroid( _row, _col, cx, cy );
				p1->initialValue = cx;
				p2->initialValue = cy;

				// Force the sampled value to be non-negative if appropriate; this could be problematic
				// if it ends up forcing two sampled values into the same spatial hash bin.
				if( p1->initialValue < 0 && ( p1->type == PT_RATE || p1->type == PT_OUTPUTFACTOR ) ) {
					p1->initialValue = 0;
				}
				if( p2->initialValue < 0 && ( p2->type == PT_RATE || p2->type == PT_OUTPUTFACTOR ) ) {
					p2->initialValue = 0;
				}

				int newRow, newCol;
				spatialToFitData.getBinRowCol( (float)p1->initialValue, (float)p2->initialValue, newRow, newCol );
				if( newRow != _row || newCol != _col ) {
					assert( false && "spatial hash boundary issue..." );
					//spatialToFitData.getBinRowCol( p1->initialValue, p2->initialValue, newRow, newCol );
				}
//				printf( "    gridsample (%02d,%02d) %s=%g, %s=%g",
//							 _row+1, _col+1, pname1, p1->initialValue, pname2, p2->initialValue );
			}
			return true;
		}
	}
	return false;
}

bool GridSet::isComplete( double seedThreshold, double lBoundDetailThreshold ) {
	// seedThreshold is the chi2 value beyond which we will not use a sample as an initial condition,
	// so this potentially limits the locations on the grid we will actually sample.

	// lBoundDetailThreshold is the chi2 level for a "decent" fit, used to determine whether
	// this grid would benefit from sample the values below the lower bound for more context.

	int steps = properties.getI( "gridsteps", -1 );
	if( steps == -1 ) {
		return false;
	}
	if( 0/*Kin_fitspaceCalcLBoundDetail*/ ) {
		if( lowerBoundsDetailCriteria( lBoundDetailThreshold) && !lowerBoundsDetailP1.fitCount && !lowerBoundsDetailP2.fitCount ) {
			return false;
		}
	}
	if( fitCount == steps * steps ) {
		return true;
	}
	else if( fitCount ) {
		// it's possible we're complete even without steps * steps entries;
		// we quit a grid if all samples with empty neighbors are above a threshold
		// chi2; that is, when indications are that remaining samples are "badlands"
		int i;
		for( i=0; i<fitCount; i++ ) {
			if( ppFitData[i]->chi2 < seedThreshold ) {
				if( hasEmptyNeighbor( ppFitData[i] ) ) {
					return false;
						// this location is a valid seed for continued sampling!
				}
			}
		}
	}
	return true;
}

bool GridSet::lowerBoundsDetailCriteria( double chi2Threshold ) {
	GridNode *node = (GridNode*) *spatialToFitData.getBinByRowCol( 0, 0 );
	if( !node ) {
		return false;
			// can happen if 0,0 is in "badlands"; for now abort.
	}
	assert( node );

	FitData *fd = node->fd;
	if( fd->chi2 > chi2Threshold ) {
		// the fit at the 0,0 corner of the grid is bad; so we don't need more detail
		// @TODO: really perhaps we want more detail if *any* edge fit is good
		return false;
	}

	int steps = properties.getI( "gridsteps" );
	int row, col;

	//for now require a full column 0 of samples
	for( row=0; row<steps; row++ ) {
		if(! *spatialToFitData.getBinByRowCol( row, 0 ) ) {
			return false;
		}
	}
	//for now require a full row 0 of samples
	for( col=0; col<steps; col++ ) {
		if(! *spatialToFitData.getBinByRowCol( 0, col ) ) {
			return false;
		}
	}

	return true;
}

GridSet * GridSet::createShadowGridWithBoundaryFeatures( FitSet *fs1, FitSet *fs2 ) {

	// ALLOC new grid, copy properties hash, init spatial hash to match ours
	GridSet *gs = new GridSet();
	gs->properties.copyFrom( properties );
	#define GSH (spatialToFitData)
	gs->initSpatialHash( GSH.xBins, GSH.xMin, GSH.xMax, GSH.yMin, GSH.yMax );

	// GET properties necessary for algorithm
	char *paramName1 = properties.getS( "paramName1" );
	char *paramName2 = properties.getS( "paramName2" );
	double minp1     = properties.getD( "minp1" );
	double maxp1     = properties.getD( "maxp1" );
	double minp2     = properties.getD( "minp2" );
	double maxp2     = properties.getD( "maxp2" );

	// SETUP pointers to all fits that will contend for spots on the grid
	FitSet * fitSets[3];
	fitSets[0] = fs1;
	fitSets[1] = fs2;
	fitSets[2] = (FitSet*)this;

	for( int c=0; c<3; c++ ) {
		FitData ** ppFD = fitSets[c]->ppFitData;
		int count		= fitSets[c]->fitCount; 

		for( int i=0; i<count; i++ ) {
			double p1 = ppFD[i]->paramByName( paramName1 )->bestFitValue;
			double p2 = ppFD[i]->paramByName( paramName2 )->bestFitValue;
			if( true || (p1 >= minp1 && p1 <= maxp1 && p2 >= minp2 && p2 <= maxp2) ) {
				int row,col;
				spatialToFitData.getBinRowCol( (float)p1, (float)p2, row, col );
				GridNode *node = (GridNode*)*gs->spatialToFitData.getBinByRowCol( row, col );
				if( node ) {
					if( node->fd->chi2 > ppFD[i]->chi2 ) {
						node->fd = ppFD[i];
					}
				}
				else {
					gs->addFit( ppFD[i], p1, p2 );
				}
			}
			else {
				printf( "funny, shadowgrid found out-of-bounds param: %g[%g,%g] %g[%g,%g]\n", p1,minp1,maxp1,p2,minp2,maxp2);
					// sanity check;
					// this was only ever reached due to floating imprecision in compares of equal vals
			}
		}
	}
	return gs;
}








