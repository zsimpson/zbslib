// @ZBS {
//		*MODULE_OWNER_NAME zintegrator
// }

#ifndef ZINTEGRATOR_H
#define	ZINTEGRATOR_H

#include "stdio.h"	// to get size_t

class ZMatLinEqSolver;

typedef int (*CallbackDeriv)( double x, double *y, double *dYdx, void *params );
typedef int (*CallbackJacob)( double x, double *y, double *dFdY, double *dFdx, void *params );

struct ZIntegrator {
	int dim;
		// The dimension of the system
	double x;
		// The current value of the independent variable
	double *y;
		// The current values of the system
	double *dydx;
		// The current values of the system
	double x0, x1;
		// Start and stop of the integrator
	double stepInit;
		// initial step size
	double stepMin;
		// The minimium step size before giving up
	int stepMaxCount;
		// Maximum number of steps before abandoning
	double stepLast;
		// The last step taken by the integrator
	double errAbs;
	double errRel;
		// Error terms
	CallbackDeriv deriv;
		// The callback for the deriviatives
	CallbackJacob jacob;
		// The callback for the Jacobians
	void *callbackUserParams;
		// User callback variable

	int outColAlloc;
		// Buffer size
	int outColCount;
		// Buffer count
	double *outX;
		// The times of each out point
	double *outY;
		// The data at the point, a dim * outColCount matrix
	double *outF;
		// The dydx at the point, a dim * outColCount matrix
	int storeOut;
		// If set then it stores the output of every step
	int firstEvolve;
		// A flag set to detect the first call to evolove
	int error;
		#define ZI_NONE (0)
		#define ZI_WORKING (0)
			// Working and none are the same thing
		#define ZI_ERROR (1)
			// This is returned when a lower-level error flag has been put into error
		#define ZI_DONE_SUCCESS (2)
		#define ZI_STEP_UNDERFLOW (3)
		#define ZI_STEP_OVERCOUNT (4)

	double outGetX( int col ) { return outX[ col ]; }
	double outGetY( int row, int col ) { return outY[ col*dim+row ]; }
	double outGetF( int row, int col ) { return outF[ col*dim+row ]; }

	virtual void loadY( int row, double val ) { y[row] = val; }

	virtual int stepper( double &stepNext ) { return 0; }
		// If the ancestor class top-level integration logic is used, then this is the stepper function

	virtual void outAdd();

	virtual int evolve();
		// returns ZI_NONE on each step, ZI_DONE_SUCCESS on complete or error

	virtual int step( double deltaX );
		// returns error or zero

	virtual int integrate();
		// returns error or zero

	ZIntegrator(
		int _dim, double *_initCond, double _x0, double _x1, double _errAbs, double _errRel, double _stepInit, double _stepMin, int _storeOut, 
		CallbackDeriv _deriv, CallbackJacob _jacob, void *_callbackUserParams
	);
	virtual ~ZIntegrator();
};

struct ZIntegratorRosenbrockStifflyStable : ZIntegrator {
	// Based on the Numerical Recipes 3 Ross integrator

	// Workspace vectors are allocated all in the constructor to avoid lots of allocas in the core routine
	double *dfdy;
	double *dfdx;
	double *k1;
	double *k2;
	double *k3;
	double *k4;
	double *k5;
	double *k6;
	double *cont1;
	double *cont2;
	double *cont3;
	double *cont4;
	double *a;
	double *dydxnew;
	double *ytemp;
	double *yerr;
	double *yout;

	// Originally part of controller
	int reject;
	int stepIsFirst;
	double errOld;
	double stepOld;

	ZMatLinEqSolver *luSolver;

	virtual int stepper( double &stepNext );

	ZIntegratorRosenbrockStifflyStable(
		int _dim, double *_initCond, double _x0, double _x1, double _errAbs, double _errRel, double _stepInit, double _stepMin, int _storeOut,
		CallbackDeriv _deriv, CallbackJacob _jacob, void *_callbackUserParams
	);
	virtual ~ZIntegratorRosenbrockStifflyStable();
};

#endif


