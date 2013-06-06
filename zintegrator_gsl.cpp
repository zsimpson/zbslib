// @ZBS {
//		*MODULE_NAME ZIntegrator_GSL
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Wrapper for GSL integrators into the ZIntegrator framework
//		}
//		*PORTABILITY win32 unix macosx
//		*REQUIRED_FILES zintegrator_gsl.cpp zintegrator_gsl.h
//		*SDK_DEPENDS gsl-1.8
//		*VERSION 1.0
//		+HISTORY {
//			2008 Original version by ZBS during work on KinTek 2.0
//		}
//		*PUBLISH yes
// }


// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
// MODULE includes:
#include "zintegrator_gsl.h"
// ZBSLIB includes:


// ZIntegratorGSLRK45
//-----------------------------------------------------------------------------------------------------------------------------

ZIntegratorGSLRK45::ZIntegratorGSLRK45(
	int _dim, double *_initCond, double _x0, double _x1, double _errAbs, double _errRel, double _stepInit, double _stepMin, int _storeOut,
	CallbackDeriv _deriv, void *_callbackUserParams
)
: ZIntegrator( _dim, _initCond, _x0, _x1, _errAbs, _errRel, _stepInit, _stepMin, _storeOut, _deriv, 0, _callbackUserParams )
{
	gslEvolve = gsl_odeiv_evolve_alloc( dim );
	gslControl = gsl_odeiv_control_standard_new( errAbs, errRel, 1.0, 1.0 );
	gslStep = gsl_odeiv_step_alloc( gsl_odeiv_step_rkf45, dim );
	gslSystem.dimension = dim;
	gslSystem.params = callbackUserParams;

	typedef int (*GSLDeriv)( double t, const double y[], double dydt[], void * params );
	typedef int (*GSLJacob)( double t, const double y[], double * dfdy, double dfdt[], void * params );

	gslSystem.function = (GSLDeriv)deriv;
	gslSystem.jacobian = (GSLJacob)jacob;
	gslStepSize = stepInit;
}

ZIntegratorGSLRK45::~ZIntegratorGSLRK45() {
	gsl_odeiv_evolve_free( gslEvolve );
	gsl_odeiv_control_free( gslControl );
	gsl_odeiv_step_free( gslStep );
}

int ZIntegratorGSLRK45::evolve() {
	if( firstEvolve ) {
		// ADD the initial point
		(*deriv)( x, y, dydx, callbackUserParams );
		outAdd();

		firstEvolve = ZI_NONE;
		return ZI_WORKING;
	}

	if( x >= x1 && error == GSL_SUCCESS ) {
		return ZI_DONE_SUCCESS;
	}

	int err = gsl_odeiv_evolve_apply( gslEvolve, gslControl, gslStep, &gslSystem, &x, x1, &gslStepSize, y );
	if( err == GSL_SUCCESS ) {
		return ZI_WORKING;
	}
	else {
		error = err;
		return ZI_ERROR;
	}
}

int ZIntegratorGSLRK45::step( double deltaX ) {
	double stepEndX = x + deltaX;
	while( x < stepEndX ) {
		int err = gsl_odeiv_evolve_apply( gslEvolve, gslControl, gslStep, &gslSystem, &x, x1, &gslStepSize, y );
		if( err != GSL_SUCCESS ) {
			error = err;
			return ZI_ERROR;
		}
	}
	return ZI_WORKING;
}

int ZIntegratorGSLRK45::integrate() {
	while( x < x1 ) {
		int err = gsl_odeiv_evolve_apply( gslEvolve, gslControl, gslStep, &gslSystem, &x, x1, &gslStepSize, y );
		if( err != GSL_SUCCESS ) {
			error = err;
			return ZI_ERROR;
		}

		(*deriv)( x, y, dydx, callbackUserParams );

		outAdd();
	}
	return ZI_DONE_SUCCESS;
}


// ZIntegratorGSLBulirschStoerBaderDeuflhard
//-----------------------------------------------------------------------------------------------------------------------------

ZIntegratorGSLBulirschStoerBaderDeuflhard::ZIntegratorGSLBulirschStoerBaderDeuflhard(
	int _dim, double *_initCond, double _x0, double _x1, double _errAbs, double _errRel, double _stepInit, double _stepMin, int _storeOut,
	CallbackDeriv _deriv, CallbackJacob _jacob, void *_callbackUserParams
)
: ZIntegrator( _dim, _initCond, _x0, _x1, _errAbs, _errRel, _stepInit, _stepMin, _storeOut, _deriv, _jacob, _callbackUserParams )
{
	gslEvolve = gsl_odeiv_evolve_alloc( dim );
	gslControl = gsl_odeiv_control_standard_new( errAbs, errRel, 1.0, 1.0 );
	gslStep = gsl_odeiv_step_alloc( gsl_odeiv_step_bsimp, dim );
	gslSystem.dimension = dim;
	gslSystem.params = callbackUserParams;

	typedef int (*GSLDeriv)( double t, const double y[], double dydt[], void * params );
	typedef int (*GSLJacob)( double t, const double y[], double * dfdy, double dfdt[], void * params );

	gslSystem.function = (GSLDeriv)deriv;
	gslSystem.jacobian = (GSLJacob)jacob;
	gslStepSize = stepInit;
}

ZIntegratorGSLBulirschStoerBaderDeuflhard::~ZIntegratorGSLBulirschStoerBaderDeuflhard() {
	gsl_odeiv_evolve_free( gslEvolve );
	gsl_odeiv_control_free( gslControl );
	gsl_odeiv_step_free( gslStep );
}

int ZIntegratorGSLBulirschStoerBaderDeuflhard::evolve() {
	if( firstEvolve ) {
		// ADD the initial point
		(*deriv)( x, y, dydx, callbackUserParams );
		outAdd();

		firstEvolve = ZI_NONE;
		return ZI_WORKING;
	}

	if( x >= x1 && error == GSL_SUCCESS ) {
		return ZI_DONE_SUCCESS;
	}

	int err = gsl_odeiv_evolve_apply( gslEvolve, gslControl, gslStep, &gslSystem, &x, x1, &gslStepSize, y );
	if( err == GSL_SUCCESS ) {
		return ZI_WORKING;
	}
	else {
		error = err;
		return ZI_ERROR;
	}
}

int ZIntegratorGSLBulirschStoerBaderDeuflhard::step( double deltaX ) {
	double stepEndX = x + deltaX;
	while( x < stepEndX ) {
		int err = gsl_odeiv_evolve_apply( gslEvolve, gslControl, gslStep, &gslSystem, &x, x1, &gslStepSize, y );
		if( err != GSL_SUCCESS ) {
			error = err;
			return ZI_ERROR;
		}
	}
	return ZI_DONE_SUCCESS;
}

int ZIntegratorGSLBulirschStoerBaderDeuflhard::integrate() {
	while( x < x1 ) {
		int err = gsl_odeiv_evolve_apply( gslEvolve, gslControl, gslStep, &gslSystem, &x, x1, &gslStepSize, y );
		if( err != GSL_SUCCESS ) {
			error = err;
			return ZI_ERROR;
		}

		(*deriv)( x, y, dydx, callbackUserParams );

		outAdd();
	}
	return ZI_DONE_SUCCESS;
}
