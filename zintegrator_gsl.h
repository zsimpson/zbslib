// @ZBS {
//		*MODULE_OWNER_NAME zintegrator_gsl
// }

#ifndef ZINTEGRATOR_GSL
#define ZINTEGRATOR_GSL

#include "zintegrator.h"

#include "gsl/gsl_errno.h"
#include "gsl/gsl_odeiv.h"

struct ZIntegratorGSLRK45 : ZIntegrator {
	gsl_odeiv_system gslSystem;
	gsl_odeiv_evolve *gslEvolve;
	gsl_odeiv_control *gslControl;
	gsl_odeiv_step *gslStep;
	double gslStepSize;

	virtual int evolve();
	virtual int step( double deltaX );
	virtual int integrate();

	ZIntegratorGSLRK45(
		int _dim, double *_initCond, double _x0, double _x1, double _errAbs, double _errRel, double _stepInit, double _stepMin, int _storeOut,
		CallbackDeriv _deriv, void *_callbackUserParams
	);
	virtual ~ZIntegratorGSLRK45();
};

struct ZIntegratorGSLBulirschStoerBaderDeuflhard : ZIntegrator {
	gsl_odeiv_system gslSystem;
	gsl_odeiv_evolve *gslEvolve;
	gsl_odeiv_control *gslControl;
	gsl_odeiv_step *gslStep;
	double gslStepSize;

	virtual int evolve();
	virtual int step( double deltaX );
	virtual int integrate();

	ZIntegratorGSLBulirschStoerBaderDeuflhard(
		int _dim, double *_initCond, double _x0, double _x1, double _errAbs, double _errRel, double _stepInit, double _stepMin, int _storeOut,
		CallbackDeriv _deriv, CallbackJacob _jacob, void *_callbackUserParams
	);
	virtual ~ZIntegratorGSLBulirschStoerBaderDeuflhard();
};

#endif


