// @ZBS {
//		*MODULE_NAME ZFitter
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Am SVD gradient descent parameter fitter
//		}
//		*PORTABILITY win32 unix maxosx
//		*REQUIRED_FILES zfitter.cpp zfitter.h
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
#include "zfitter.h"
// ZBSLIB includes:
#include "ztime.h"
#include "zmat_gsl.h"

ZFitter::ZFitter() {
	clear();
}

int ZFitter::propCount() {
	int count = 0;
	for( int i=0; i<stateHistory.cols; i++ ) {
		count += stateHistory.getI(0,i) ? 0 : 1;
	}
	return count;
}

int ZFitter::origCount() {
	int count = 0;
	for( int i=0; i<stateHistory.cols; i++ ) {
		count += stateHistory.getI(0,i) ? 1 : 0;
	}
	return count;
}

void ZFitter::start( int qCount, double *qStart, int *qFitFlags ) {
	state = ZF_RUNNING;
	fitFlags.copyI( qFitFlags, qCount, 1 );
	origQ.copyD( qStart, qCount, 1 );
	propQ.copy( origQ );
	propEigenRejectFlags.alloc( origQ.rows, 1, zmatS32 );
	propBackoffStepsOnLastEigenValue = 0;

	// COUNT how many parameters are being fit
	qBeingFitCount = 0;
	for( int q=0; q<origQ.rows; q++ ) {
		if( fitFlags.getI(q,0) ) {
			qBeingFitCount++;
		}
	}

	stateHistory.alloc( 1, 1 );
	stateHistory.putD( 0, 0, 1.0 );
	timingPropEvalAccum = 0.0;
	timingOrigNewAccum = 0.0;
	propEval( propQ, propE, propOffTheRails );

	// GROW history
	qHistory.growCol( propQ );
	eHistory.growCol( propE );

	// CALL origNew for first time
	origNew();
	
	origLastHistoryIndex = eHistory.cols - 1;
	propCountSinceOrig = 1;
}

void ZFitter::clear() {
	state = ZF_STOPPED;

	fitFlags.clear();
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
	propCountSinceOrig = 0;
	origLastHistoryIndex = 0;
	qBeingFitCount = 0;

	stateHistory.clear();
	qHistory.clear();
	eHistory.clear();

	timingPropEvalAccum = 0.0;
	timingOrigNewAccum = 0.0;
}

void ZFitter::copy( ZFitter *c ) {
	state = c->state;

	fitFlags.copy( c->fitFlags );
	origE = c->origE;
	origQ.copy( c->origQ );
	origVtt.copy( c->origVtt );
	origSignal.copy( c->origSignal );
	origNoise.copy( c->origNoise );
	origSNR.copy( c->origSNR );
	origGrad.copy( c->origGrad );
	origHess.copy( c->origHess );
	origCovr.copy( c->origCovr );
	origU.copy( c->origU );
	origUt.copy( c->origUt );
	origS.copy( c->origS );
	origVt.copy( c->origVt );
	origUtCovr.copy( c->origUtCovr );
	origCovrInEigenSpace.copy( c->origCovrInEigenSpace );
	origGradInEigenSpace.copy( c->origGradInEigenSpace );
	origGradInEigenSpaceScaledByS.copy( c->origGradInEigenSpaceScaledByS );
	
	propE = c->propE;
	propQ.copy( c->propQ );
	propEigenRejectFlags.copy( c->propEigenRejectFlags );
	propOffTheRails = c->propOffTheRails;
	propBackoffStepsOnLastEigenValue = c->propBackoffStepsOnLastEigenValue;
	propCountSinceOrig = c->propCountSinceOrig;
	origLastHistoryIndex = c->origLastHistoryIndex;
	qBeingFitCount = c->qBeingFitCount;

	stateHistory.copy( c->stateHistory );
	qHistory.copy( c->qHistory );
	eHistory.copy( c->eHistory );

	timingPropEvalAccum = c->timingPropEvalAccum;
	timingOrigNewAccum = c->timingOrigNewAccum;
}

void ZFitter::stop() {
	clear();
}

int ZFitter::step() {
	if( state != ZF_RUNNING ) {
		// Call after xconvergence
		return state;
	}

	// GENERATE a new proposal
	int propNewRet = propNew();

	// RESOLVE four possible outcomes
	switch( propNewRet ) {
		case PN_CONVERGED:
			//  Converged: There are no more proposals to be made. The last orig is final
			state = ZF_DONE_CONVERGED;
			break;

		case PN_TRAPPED:
			//  Trapped: There are no more proposals to be made, but we don't belive that the last orig has converged
			state = ZF_DONE_TRAPPED;
			break;

		case PN_PROPOSE: {
			//  Propse: This is a new proposal, we don't move orig
			ZTime startTime = zTimeNow();
			propEval( propQ, propE, propOffTheRails );
			timingPropEvalAccum += zTimeNow() - startTime;
			
			// GROW history
			qHistory.growCol( propQ );
			eHistory.growCol( propE );

			// GROW into the stateHistory that this was a plain-old proposition
			ZMat s( 1, 1 );
			s.putD( 0, 0, 0.0 );
			stateHistory.growCol( s );
			
			state = ZF_RUNNING;
			break;
		}

		case PN_ACCEPT: {
			//  Accept: the last proposal as the new orig
			origNew();

			// GROW into the stateHistory that this was a new origin
			ZMat s( 1, 1 );
			s.putD( 0, 0, 1.0 );
			stateHistory.growCol( s );

			state = ZF_RUNNING;
			break;
		}
	}

	return state;
}

int ZFitter::propNew() {
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
		if( ! fitFlags.getI(q,0) ) {
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

				ZTime startTime = zTimeNow();
				propEval( propQ, propE, propOffTheRails );
				timingPropEvalAccum += zTimeNow() - startTime;
					// Except in the special optimization case, we have to reset the traceC system --
					// it isn't enough to merely recover the old propE
			}
			propCountSinceOrig = 1;
				// This is 1 to include the new orig
			propBackoffStepsOnLastEigenValue = 0;
			origLastHistoryIndex = eHistory.cols-1;
			propEigenRejectFlags.zero();
			return PN_ACCEPT;
		}
	}

	// ... otherwise, this is a regular proposal	
	propCountSinceOrig++;
	return PN_PROPOSE;
}


void ZFitter::origNew() {
	ZTime startTime = zTimeNow();

	int q;

	// ACCEPT the propQ/propE as the new origQ/origE
	origQ.copy( propQ );
	origE = propE;

	// COMPUTE all the expensive terms for the gradient, hessian, and covariance for this new origin
	// Returns 1 if we're converged
	
	int qCount = origQ.rows;

	// CALL the virtual that implements the "heavy lifiting" of obtaining the gradient. hessian, covaiance
	origEval( origQ, origGrad, origHess, origCovr );

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
	
	int firstTime = eHistory.rows == 1 ? 1 : 0;
	
	// COMPUTE SNR
	origSNR.alloc( qCount, 1, zmatF64 );
	for( q=0; q<qCount; q++ ) {
		origSNR.putD(q,0,0.0);
		if( origS.getD(q,0) > 0.0 ) {
			double n2 = *origNoise.getPtrD(q,0) / ( origS.getD(q,0) * origS.getD(q,0) );
			origSNR.putD( q, 0, fabs( origSignal.getD(q,0) / n2 ) );
		}

		// REJECT all q's that are either not being fit or have < 1 SNR
		if(
			q < qBeingFitCount
			&& (
				   ( origSNR.getD(q,0) > 1.0 )
				|| ( firstTime && origS.getD(q,0) / origS.getD(0,0) > 1e-9 )
				|| ( firstTime && q==0 && origS.getD(0,0) > 1e-9 )
			)
		) {
			// The first time is a special case where we do not enforce the SNR condition
			// but do want to avoid float point exceptions
		
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

	timingOrigNewAccum += zTimeNow() - startTime;
}


double ZFitter::getSNRForRender( int q ) {
	double snr = origSNR.getD(q,0);
	if( propEigenRejectFlags.getI(q,0) ) {
		snr = 0.0;
	}
	return fabs( snr );
}

