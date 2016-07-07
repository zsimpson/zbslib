// @ZBS {
//		*MODULE_NAME ZIntegrator
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			An abstraction of every type of integrator with a lot of
//			emphasis on keeping everything to minimal dependiencies
//		}
//		*PORTABILITY win32 unix macosx
//		*REQUIRED_FILES zintegrator.cpp zintegrator.h
//		*VERSION 1.0
//		+HISTORY {
//			2008 Original version by ZBS during work on KinTek 2.0
//		}
//		+TODO {
//			Separate dependiencies into separate files
//		}
//		*PUBLISH yes
// }

// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "assert.h"
#include "float.h"
#include "math.h"
#include "memory.h"
#include "stdio.h"
#ifdef WIN32
	#include "malloc.h"
#else
	#include "stdlib.h"
#endif
// MODULE includes:
#include "zintegrator.h"
// ZBSLIB includes:

#ifndef NO_GSL
// @ZBSIF !configDefined( "NO_GSL" )
// the above is for the perl-parsing of files for dependencies; we don't
// want the dependency builder to see these includes if NO_GSL is defined.
	#include "zmat_gsl.h"
// @ZBSENDIF
#endif

#ifdef KIN
// This is here to prevent a dependency on the Eigen linear alebgra library
// which is employed by KinTek to replace dependency on GSL.  Eigen LA is 
// more comprehensive than GSL, and perhaps should be the default for LA.
// GSL is GPL'd, so KinTek can't ship it.  (tfb july 2016)
// @ZBSIF configDefined( "KIN" )
// the above is for the perl-parsing of files for dependencies.  
	#include "zmat_eigen.h"
// @ZBSENDIF
#endif




// ZIntegrator
//-----------------------------------------------------------------------------------------------------------------------------

ZIntegrator::ZIntegrator( int _dim, double *_initCond, double _x0, double _x1, double _errAbs, double _errRel, double _stepInit, double _stepMin, int _storeOut, CallbackDeriv _deriv, CallbackJacob _jacob, void *_callbackUserParams ) {
	dim = _dim;
	x = _x0;
	y = (double *)malloc( dim * sizeof(double) );
	dydx = (double *)malloc( dim * sizeof(double) );
	memcpy( y, _initCond, sizeof(double) * dim );
	x0 = _x0;
	x1 = _x1;
	errAbs = _errAbs;
	errRel = _errRel;
	stepInit = _stepInit;
	stepMaxCount = 100000;
	deriv = _deriv;
	jacob = _jacob;
	outColCount = 0;
	outColAlloc = 0;
	outX = 0;
	outY = 0;
	outF = 0;
	callbackUserParams = _callbackUserParams;
	stepLast = stepInit;
	firstEvolve = 1;
	storeOut = _storeOut;
	error = 0;
}

ZIntegrator::~ZIntegrator() {
	if( y ) {
		free( y );
		y = 0;
	}
	if( dydx ) {
		free( dydx );
		dydx = 0;
	}
	if( outX ) {
		free( outX );
		outX = 0;
	}
	if( outY ) {
		free( outY );
		outY = 0;
	}
	if( outF ) {
		free( outF );
		outF = 0;
	}
}

void ZIntegrator::outAdd() {
	if( storeOut ) {
		if( outColAlloc == outColCount ) {
			outColAlloc *= 4;
			outColAlloc = outColAlloc < 128 ? 128 : outColAlloc;
			outX = (double *)realloc( outX, outColAlloc * sizeof(double) );
			outY = (double *)realloc( outY, dim * outColAlloc * sizeof(double) );
			outF = (double *)realloc( outF, dim * outColAlloc * sizeof(double) );
		}
		outX[outColCount] = x;
		memcpy( &outY[outColCount*dim], y, dim * sizeof(double) );
		memcpy( &outF[outColCount*dim], dydx, dim * sizeof(double) );
		outColCount++;
	}
}

int ZIntegrator::evolve() {
#ifdef MKNESS
	static int iii=0;
	iii += 1;
	if ((iii % 100) == 0) {
		printf ("x=%g, x0=%g, x1=%g\n", x, x0, x1);
		iii = 0;
	}
#endif
	if( x >= x1 && x1 >= x0 ) {
		// Past the end, done
		return ZI_DONE_SUCCESS;
	}

	if( firstEvolve ) {

		// ADD the initial point
		(*deriv)( x, y, dydx, callbackUserParams );
		outAdd();

		firstEvolve = ZI_NONE;
		return ZI_WORKING;
	}

	(*deriv)( x, y, dydx, callbackUserParams );

	// BOUND clamp the end step
	if( x + stepLast*1.0001 > x1 && x1 > x0 ) {
		stepLast = x1 - x;
	}

	double stepNext;
	error = stepper( stepNext );
	if( error ) {
		return error;
	}
	// Step a little further if the last step placed us arbitrarily close to 
	// the endpoint. 
	double diff = fabs( x1 - x );
	if( diff > 0 && diff < 100.0*fabs(x1)*DBL_EPSILON ) {
		//printf( "\nZIntegrator: skip point at %g in favor of endpoint at %g\n", x, x1 );
		x = x1;
	}

	// @TODO: Dense

	outAdd();

	if( x >= x1 && x1 >= x0 ) {

		// ADD the last point if it is different enough
		if( storeOut ) {
			if( fabs(outX[outColCount-1]-x1) > 100.0*fabs(x1)*DBL_EPSILON ) {
				outAdd();
			}
		}

		// RETURN 1 here ensures that the last point gets picked up
		return ZI_WORKING;
	}

	if( fabs(stepNext) <= 4.0*DBL_EPSILON ) {
		error = ZI_STEP_UNDERFLOW;
		return error;
	}

	stepLast = stepNext;

//printf( "%g  ", stepNext );

	return ZI_WORKING;
}

int ZIntegrator::step( double deltaX ) {
	(*deriv)( x, y, dydx, callbackUserParams );

	x1 = x + deltaX;
	while( x < x1 ) {

		// BOUND clamp the end step
		if( x + stepLast*1.0001 > x1 && x1 > x0 ) {
			stepLast = x1 - x;
		}

		double stepNext;
		int err = stepper( stepNext );

		if( fabs(stepNext) <= 4.0*DBL_EPSILON ) {
			error = ZI_STEP_UNDERFLOW;
			return error;
		}

		stepLast = stepNext;
	}

	outAdd();

	return ZI_WORKING;
}

int ZIntegrator::integrate() {

	(*deriv)( x, y, dydx, callbackUserParams );

	outAdd();

	// @TODO dense

	for( int stepCount=0; stepCount<stepMaxCount; stepCount++) {

		// BOUND clamp the end step
		if( x + stepLast*1.0001 > x1 && x1 > x0 ) {
			stepLast = x1 - x;
		}
	
		double stepNext;
		int err = stepper( stepNext );
			// Stepper is expected to setup dydx which is somewhat opaque
	
		// @TODO: Dense

		outAdd();

		if( x >= x1 && x1 >= x0 ) {

			// ADD the last point if it is different enough
			if( fabs(outX[outColCount-1]-x1) > 100.0*fabs(x1)*DBL_EPSILON ) {
				outAdd();
			}

			// SUCCESS
			return ZI_DONE_SUCCESS;
		}

		if( fabs(stepNext) <= 4.0*DBL_EPSILON ) {
			error = ZI_STEP_UNDERFLOW;
			return error;
		}

		stepLast = stepNext;
	}

	error = ZI_STEP_OVERCOUNT;
	return error;
}




// ZIntegratorRosenbrockStifflyStable
//-----------------------------------------------------------------------------------------------------------------------------

ZIntegratorRosenbrockStifflyStable::ZIntegratorRosenbrockStifflyStable(
	int _dim, double *_initCond, double _x0, double _x1, double _errAbs, double _errRel, double _stepInit, double _stepMin, int _storeOut, 
	CallbackDeriv _deriv, CallbackJacob _jacob, void *_callbackUserParams
)
: ZIntegrator( _dim, _initCond, _x0, _x1, _errAbs, _errRel, _stepInit, _stepMin, _storeOut, _deriv, _jacob, _callbackUserParams )
{
	// Originally part of controller
	reject = 0;
	stepIsFirst = 1;
	errOld = 0.0;
	stepOld = 0.0;

	dfdy = (double *)malloc( dim * dim * sizeof(double) );
	dfdx = (double *)malloc( dim * sizeof(double) );
	k1 = (double *)malloc( dim * sizeof(double) );
	k2 = (double *)malloc( dim * sizeof(double) );
	k3 = (double *)malloc( dim * sizeof(double) );
	k4 = (double *)malloc( dim * sizeof(double) );
	k5 = (double *)malloc( dim * sizeof(double) );
	k6 = (double *)malloc( dim * sizeof(double) );
	cont1 = (double *)malloc( dim * sizeof(double) );
	cont2 = (double *)malloc( dim * sizeof(double) );
	cont3 = (double *)malloc( dim * sizeof(double) );
	cont4 = (double *)malloc( dim * sizeof(double) );
	a = (double *)malloc( dim * dim * sizeof(double) );
	dydxnew = (double *)malloc( dim * sizeof(double) );
	ytemp = (double *)malloc( dim * sizeof(double) );
	yerr = (double *)malloc( dim * sizeof(double) );
	yout = (double *)malloc( dim * sizeof(double) );

	#ifdef KIN
		#ifdef KIN_DEV
			// in the DEV version of KinTek, allow options for comparison.
			extern int Kin_simLuGSL;
			if( Kin_simLuGSL ) {
				luSolver = new ZMatLUSolver_GSL( a, _dim );
			}
			else {
				luSolver = new ZMatLUSolver_Eigen( a, _dim, 0 );
			}
		#else
			// in shipping versions of KinTek software, never use GSL.
			luSolver = new ZMatLUSolver_Eigen( a, _dim, 0 );
		#endif

	#else
		// outside of KinTek plugins, always use GSL linear algebra
		luSolver = new ZMatLUSolver_GSL( a, _dim );
	#endif
}

ZIntegratorRosenbrockStifflyStable::~ZIntegratorRosenbrockStifflyStable() {
	free( dfdy );
	free( dfdx );
	free( k1 );
	free( k2 );
	free( k3 );
	free( k4 );
	free( k5 );
	free( k6 );
	free( cont1 );
	free( cont2 );
	free( cont3 );
	free( cont4 );
	free( a );
	free( dydxnew );
	free( ytemp );
	free( yerr );
	free( yout );
}

int ZIntegratorRosenbrockStifflyStable::stepper( double &stepNext ) {
	const double RC_c2 = 0.386;
	const double RC_c3 = 0.21;
	const double RC_c4 = 0.63;
	const double RC_bet2p = 0.0317;
	const double RC_bet3p = 0.0635;
	const double RC_bet4p = 0.3438;
	const double RC_d1 = 0.2500000000000000e+00;
	const double RC_d2 = -0.1043000000000000e+00;
	const double RC_d3 = 0.1035000000000000e+00;
	const double RC_d4 = -0.3620000000000023e-01;
	const double RC_a21 = 0.1544000000000000e+01;
	const double RC_a31 = 0.9466785280815826e+00;
	const double RC_a32 = 0.2557011698983284e+00;
	const double RC_a41 = 0.3314825187068521e+01;
	const double RC_a42 = 0.2896124015972201e+01;
	const double RC_a43 = 0.9986419139977817e+00;
	const double RC_a51 = 0.1221224509226641e+01;
	const double RC_a52 = 0.6019134481288629e+01;
	const double RC_a53 = 0.1253708332932087e+02;
	const double RC_a54 = -0.6878860361058950e+00;
	const double RC_c21 = -0.5668800000000000e+01;
	const double RC_c31 = -0.2430093356833875e+01;
	const double RC_c32 = -0.2063599157091915e+00;
	const double RC_c41 = -0.1073529058151375e+00;
	const double RC_c42 = -0.9594562251023355e+01;
	const double RC_c43 = -0.2047028614809616e+02;
	const double RC_c51 = 0.7496443313967647e+01;
	const double RC_c52 = -0.1024680431464352e+02;
	const double RC_c53 = -0.3399990352819905e+02;
	const double RC_c54 = 0.1170890893206160e+02;
	const double RC_c61 = 0.8083246795921522e+01;
	const double RC_c62 = -0.7981132988064893e+01;
	const double RC_c63 = -0.3152159432874371e+02;
	const double RC_c64 = 0.1631930543123136e+02;
	const double RC_c65 = -0.6058818238834054e+01;
	const double RC_gam = 0.2500000000000000e+00;
	const double RC_d21 = 0.1012623508344586e+02;
	const double RC_d22 = -0.7487995877610167e+01;
	const double RC_d23 = -0.3480091861555747e+02;
	const double RC_d24 = -0.7992771707568823e+01;
	const double RC_d25 = 0.1025137723295662e+01;
	const double RC_d31 = -0.6762803392801253e+00;
	const double RC_d32 = 0.6087714651680015e+01;
	const double RC_d33 = 0.1643084320892478e+02;
	const double RC_d34 = 0.2476722511418386e+02;
	const double RC_d35 = -0.6594389125716872e+01;
	const double RC_safe = 0.9;
	const double RC_fac1 = 5.0;
	const double RC_fac2 = 0.166666666666666666666666666666666666666;

	// Originally part of stepper
	int i, j;

	double step = stepLast;

	(*jacob)( x, y, dfdy, dfdx, callbackUserParams );

	int luSign = 0;
	size_t *permutation = (size_t *)alloca( dim * sizeof(size_t) );

	while( 1 ) {

		// WHAT WAS DY in NR3
		{

			// LOAD mat A with neative of the jacobian, offset the diagonal
			double recipGam = 1.0 / ( RC_gam * step );
			for( i=0; i<dim; i++ ) {
				for( j=0; j<dim; j++ ) {
					a[j*dim+i] = -dfdy[i*dim+j];
				}
				a[i*dim+i] += recipGam;
			}

			// LU Demopose
			//(*luDecompose)( a, dim, permutation, &luSign );
			luSolver->decompose();


			// SOLVE for k1
			for( i=0; i<dim; i++) {
				ytemp[i] = dydx[i] + step*RC_d1*dfdx[i];
			}
			//(*luSolve)( a, dim, permutation, ytemp, k1 );
			luSolver->solve( ytemp, k1 );


			// SOLVE for k2
			for( i=0; i<dim; i++) {
				ytemp[i] = y[i] + RC_a21 * k1[i];
			}
			(*deriv)( x+RC_c2*step, ytemp, dydxnew, callbackUserParams );
			for( i=0; i<dim; i++) {
				ytemp[i] = dydxnew[i] + step*RC_d2*dfdx[i] + RC_c21*k1[i]/step;
			}
			//(*luSolve)( a, dim, permutation, ytemp, k2 );
			luSolver->solve( ytemp, k2 );


			// SOLVE for k3
			for( i=0; i<dim; i++) {
				ytemp[i] = y[i] + RC_a31*k1[i] + RC_a32*k2[i];
			}
			(*deriv)( x + RC_c3*step, ytemp, dydxnew, callbackUserParams );
			for( i=0; i<dim; i++) {
				ytemp[i] = dydxnew[i] + step*RC_d3*dfdx[i] + ( RC_c31*k1[i] + RC_c32*k2[i] ) / step;
			}
			//(*luSolve)( a, dim, permutation, ytemp, k3 );
			luSolver->solve( ytemp, k3 );


			// SOLVE for k4
			for( i=0; i<dim; i++) {
				ytemp[i] = y[i] + RC_a41*k1[i] + RC_a42*k2[i] + RC_a43*k3[i];
			}
			(*deriv)( x+RC_c4*step, ytemp, dydxnew, callbackUserParams );
			for( i=0; i<dim; i++) {
				ytemp[i] = dydxnew[i] + step*RC_d4*dfdx[i] + ( RC_c41*k1[i] + RC_c42*k2[i] + RC_c43*k3[i] ) / step;
			}
			//(*luSolve)( a, dim, permutation, ytemp, k4 );
			luSolver->solve( ytemp, k4 );


			// SOLVE for k5
			for( i=0; i<dim; i++) {
				ytemp[i] = y[i] + RC_a51*k1[i] + RC_a52*k2[i] + RC_a53*k3[i] + RC_a54*k4[i];
			}
			double xph = x + step;
			(*deriv)( xph, ytemp, dydxnew, callbackUserParams );
			for( i=0; i<dim; i++) {
				k6[i] = dydxnew[i] + ( RC_c51*k1[i] + RC_c52*k2[i] + RC_c53*k3[i] + RC_c54*k4[i] ) / step;
			}
			//(*luSolve)( a, dim, permutation, k6, k5 );
			luSolver->solve( k6, k5 );

			// SOLVE for yerr
			for( i=0; i<dim; i++) {
				ytemp[i] += k5[i];
			}
			(*deriv)( xph, ytemp, dydxnew, callbackUserParams );
			for( i=0; i<dim; i++) {
				k6[i] = dydxnew[i] + ( RC_c61*k1[i] + RC_c62*k2[i] + RC_c63*k3[i] + RC_c64*k4[i] + RC_c65*k5[i] ) / step;
			}
			//(*luSolve)( a, dim, permutation, k6, yerr );
			luSolver->solve( k6, yerr );

			// UPDATE yout
			for( i=0; i<dim; i++) {
				yout[i] = ytemp[i] + yerr[i];
			}
		}

		// WHAT WSA ERR in NR3
		double err = 0.0;
		{
			for( i=0; i<dim; i++) {
				double fabsy = fabs( y[i] );
				double fabsyout = fabs( yout[i] );
				double sk = errAbs + errRel * ( fabsy > fabsyout ? fabsy : fabsyout );
				double sqr = yerr[i] / sk;
				sqr *= sqr;
				err += sqr;
			}

			err = sqrt( err / (double)dim );
		}

		// WHAT was success in NR3
		int success = 0;
		{

			double pp = pow(err,0.25) / RC_safe;
			double mm = RC_fac1 < pp ? RC_fac1 : pp;
			double fac = RC_fac2 > mm ? RC_fac2 : mm;
			double stepNew = step / fac;
			if( err <= 1.0 ) {
				if( !stepIsFirst ) {
					double facpred = (stepOld/step) * pow( err * err / errOld, 0.25 ) / RC_safe;

					double mmm = RC_fac1 < facpred ? RC_fac1 : facpred;
					facpred = RC_fac2 > mmm ? RC_fac2 : mmm;

					fac = fac > facpred ? fac : facpred;
					stepNew=step/fac;
				}
				stepIsFirst = 0;

				stepOld = step;

				errOld = 0.01 > err ? 0.01 : err;

				if( reject ) {
					double minn = stepNew < step ? stepNew : step;
					double maxn = stepNew > step ? stepNew : step;
					stepNew = step >= 0.0 ? minn : maxn;
				}
				stepNext = stepNew;
				reject = 0;
				success = 1;
			}
			else {
				step = stepNew;
				reject = 1;
				success = 0;
			}
		}

		if( success ) {
			break;
		}

		if( fabs(step) <= fabs(x)*DBL_EPSILON ) {
			return ZI_STEP_UNDERFLOW;
		}
		if( step != step ) {
			// step is NaN or other non-real.  I've seen this in a very underconstrained system
			// with rate voltage-dependency (just to remind myself later).  I saw err above become non-real,
			// due to some element of yout[] being non-real.
			// I did not track down further, just return error condition here.  Not sure if
			// we need some new type of error here.  (tfb dec 2011)
			return ZI_STEP_UNDERFLOW;
		}
	}

	(*deriv)( x+step, yout, dydxnew, callbackUserParams );

	// @TODO: PREPARE dense

	memcpy( dydx, dydxnew, dim * sizeof(double) );
	memcpy( y, yout, dim * sizeof(double) );
	x += step;

	return ZI_NONE;
}




