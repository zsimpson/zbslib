// @ZBS {
//		*MODULE_NAME KineticBase
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A chemical kinetics simulator, base system
//		}
//		*PORTABILITY win32 unix maxosx
//		*REQUIRED_FILES kineticbase.cpp kineticbase.h
//		*VERSION 1.0
//		+HISTORY {
//			1.0 ZBS Jul 2008, based on earlier prototypes
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }

// OPERATING SYSTEM specific includes:
// SDK includes:
#include "gsl/gsl_odeiv.h"
#include "gsl/gsl_vector.h"
#include "gsl/gsl_linalg.h"
// STDLIB includes:
#include "string.h"
#include "math.h"
#include "stdlib.h"
#include "float.h"
#ifdef WIN32
	#include "malloc.h"
#endif
// MODULE includes:
#include "kineticbase.h"
// ZBSLIB includes:
#include "zhashtable.h"
#include "zrand.h"
#include "zmathtools.h"
#include "zprof.h"
#include "zintegrator_gsl.h"
#include "ztmpstr.h"
#include "zendian.h"
#include "zregexp.h"
#include "zstr.h"

//#define VM_DISASSEMBLE
	// define this to cause VM disassembly to debug files at various checkpoints

#ifndef max
	#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min
	#define min(a,b) ((a)<(b)?(a):(b))
#endif

#ifdef WIN32
	extern unsigned __int64 zprofTick();
	#define TICK tick = zprofTick();
#endif

extern void trace( char *fmt, ... );

// KineticTrace
//------------------------------------------------------------------------------------------------------------------------------------

KineticTrace::KineticTrace() {
	rows = 0;
	cols = 0;
	colsAlloced = 0;
	last = 0;
	time = 0;
	rowCoords = 0;
	data = 0;
	sigma = 0;
	derivs = 0;
}

KineticTrace::KineticTrace( int _rows, int _cols ) {
	rows = 0;
	cols = 0;
	colsAlloced = 0;
	last = 0;
	time = 0;
	rowCoords = 0;
	data = 0;
	sigma = 0;
	derivs = 0;
	init( _rows, _cols );
}

KineticTrace::~KineticTrace() {
	clear();
}

double KineticTrace::getSigma( int i, int j, normalizeType nt, double defaultVal ) {
	static int verbose = 0;
	double returnVal = defaultVal;
	switch( nt ) {
		case NT_None:
			returnVal = defaultVal;
			break;
			
		case NT_Sigma:
			if( !sigma ) {
				if( verbose ) trace( "WARNING: requested NT_Sigma normalization on trace without sigma\n" );
				returnVal = defaultVal;
			}
			else {
				returnVal = sigma[ i * rows + j ];
			}
			break;
			
		case NT_SigmaAvg: {
			// compute the average starting at i and j given for this j; note that
			// we probably also need to know the count to average instead of all remaining
			// it may be we want to provide a separate "computeAverageSigma" fn and just
			// lookup the cached result here.
			if( !sigma ) {
				if( verbose ) trace( "WARNING: requested NT_SigmaAvg normalization on trace without sigma\n" );
				returnVal = defaultVal;
			}
			else {
				returnVal = getSigmaAvg( i, j, -1 );
			}
			break;
		}
			
		case NT_SigmaAvgExternal:
			returnVal = properties.getD( "sigmaAvgExternal", defaultVal );
			break;
			
		case NT_SigmaAvgAFit:
			returnVal = properties.getD( "sigmaAvgAFit", defaultVal );
			break;
			
		case NT_SigmaAvgAFitFamily:
			returnVal = properties.getD( "sigmaAvgAFitFamily", defaultVal );
			break;
			
		case NT_MaxData:
			double minData, maxData;
			getBoundsForRow( 0, minData, maxData, 1 );
			returnVal = maxData;
			break;

	}
	if( returnVal == 0.0 ) {
		// prevent divide by zero errors since sigma is used to normalize
		// data in fitting etc.  This should never happen, as kintek import
		// routines screen sigma data for 0.0
		returnVal = 1.0;
		if( verbose ) trace( "WARNING: a sigma of 0.0 was replaced by 1.0\n" );
	}
	return returnVal;
}

double KineticTrace::getSigmaAvg( int i, int j, int count ) {
	assert( sigma );
	if( count <= 0 ) {
		count = cols - i;
	}	
	double sum = 0.0;
	int c;
	for( c=0; c<count; c++ ) {
		sum += sigma[ (i+c) * rows + j ]; 
	}
	return sum / (double)count;
}

normalizeType KineticTrace::getSigmaBestNormalizeTypeAvailable() {
	normalizeType bestType = NT_None;
	if( sigma ) {
		bestType = NT_Sigma;
	}
	else if( properties.has( "sigmaAvgExternal" ) ) {
		bestType = NT_SigmaAvgExternal;
	}
	else if( properties.has( "sigmaAvgAFitFamily" ) ) {
		bestType = NT_SigmaAvgAFitFamily;
		// Ken wants this used by default before the per-trace afit below 2.28.10b
	}
	else if( properties.has( "sigmaAvgAFit" ) ) {
		bestType = NT_SigmaAvgAFit;
	}
	return bestType;
}

void KineticTrace::addCol( double *vec ) {
	// Assumes that vec[0] is time
	assert( rows && "addCol called with 0 rows!" );
	if( cols >= colsAlloced ) {
		colsAlloced = max( colsAlloced*2, 1024 );
		time = (double *)realloc( time, colsAlloced * sizeof(double) );
		data = (double *)realloc( data, rows * colsAlloced * sizeof(double) );
		derivs = (double *)realloc( derivs, rows * colsAlloced * sizeof(double) );
	}
	time[cols] = vec[0];
	memcpy( &data[cols*rows], &vec[1], rows * sizeof(double) );
	cols++;
}

void KineticTrace::add( double _time, double *vec, double *_derivs, double *_sigma ) {
	// vec has rows in it
	assert( rows && "add called with 0 rows!" );
	if( cols >= colsAlloced ) {
		colsAlloced = max( colsAlloced*2, 1024 );
		time = (double *)realloc( time, colsAlloced * sizeof(double) );
		data = (double *)realloc( data, rows * colsAlloced * sizeof(double) );
		if( _sigma ) {
			sigma = (double *)realloc( sigma, rows * colsAlloced * sizeof(double) );
		}
		derivs = (double *)realloc( derivs, rows * colsAlloced * sizeof(double) );
	}
	time[cols] = _time;
	memcpy( &data[cols*rows], &vec[0], rows * sizeof(double) );
	if( _sigma ) {
		memcpy( &sigma[cols*rows], &_sigma[0], rows * sizeof(double) );
	}
	if( _derivs ) {
		memcpy( &derivs[cols*rows], &_derivs[0], rows * sizeof(double) );
	}
	cols++;
}

int KineticTrace::insertRange( KineticTrace &t, double t0, double t1, double dst0, double scale, double trans, double epsilon ) {
	// insert from t the data in the range [t0,t1)
	// place it into out data at dst0
	// it is assumed that t is of the same dimension (same # rows)
	// it is assumed that the time entries of t are in ascending order (this could be changed)

	// return the index of the first inserted value

	int i,count = 0;
	int copyBegin = -1;
	for( i=0; i<t.cols ; i++ ) {
		if( t.time[i] + epsilon >= t0 && t.time[i] - epsilon < t1 ) {
			if( copyBegin == -1 ) {
				copyBegin = i;
			}
			count++;
		}
	}

	if( !rows ) {
		rows = t.rows;
	}
	assert( rows == t.rows && "rows is wrong dimension in ::insertRange!" );

	int insertedBegin = insertGapForRange( count, dst0, dst0+t1-t0, epsilon );

	memcpy( time + insertedBegin, t.time + copyBegin, count * sizeof( time[0] ) );
	memcpy( data + insertedBegin * rows, t.data + copyBegin * rows, count * rows * sizeof( data[0] ) );
	if( sigma && t.sigma ) {
		memcpy( sigma + insertedBegin * rows, t.sigma + copyBegin * rows, count * rows * sizeof( sigma[0] ) );
	}
		// I did it this way before I supported translate/scale; perhaps doing it all in the 
		// loops below is faster now.  tfb

	double delta = dst0 - t0;
	if( fabs( delta ) > epsilon ) {
		// translate in time
		for( i=insertedBegin; i<insertedBegin+count; i++ ) {
			time[ i ] += delta;
		}
	}
	if( fabs( scale - 1.0 ) > epsilon || trans != 0.0  ) {
		// scale/translate
		for( i=insertedBegin; i<insertedBegin+count; i++ ) {
			for( int j=0; j<rows; j++ ) {
				data[i*rows+j] *= scale;
				data[i*rows+j] += trans;
				if( sigma ) {
					sigma[i*rows+j] *= scale;
						// is this math right?
				}
			}
		}
	}
	return insertedBegin;
}

int KineticTrace::insertGapForRange( int count, double t0, double t1, double epsilon ) {
	// A naive approach to keep the logic simple:
	// 1. alloc enough room for current data + count to be added
	// 2. walk along current cols, copying a col at a time until new range encountered
	// 3. note & skip past the insert range
	// 4. copy remaining data

	rows = max( rows, 1 );
		// if insertRange is called on an empty Kinetic trace, default to 1 row

	// 1.
	colsAlloced = cols + count;
	double * newTime = (double *)malloc( colsAlloced * sizeof(double) );
	memset( newTime, 0, colsAlloced * sizeof(double) );

	double * newData = (double *)malloc( rows * colsAlloced * sizeof(double) );
	memset( newData, 0, rows * colsAlloced * sizeof(double) );

	double *newDerivs = (double *)malloc( rows * colsAlloced * sizeof(double) );
	memset( newDerivs, 0, rows * colsAlloced * sizeof(double) );

	double *newSigma = 0;
	if( sigma ) {
		newSigma = (double *)malloc( rows * colsAlloced * sizeof(double) );
		double *s = newSigma;
		for( int i=0; i< rows * colsAlloced; i++, s++ ) {
			*s = 1.0;
				// sigma is often used as a divisor to scale magnitudes into
				// units of standard deviations, so 1.0 is a handy default.
		}
	}

	// 2.
	int i;
	for( i=0; i<cols && time[i] + epsilon < t0; i++ ) {
		newTime[i] = time[i];
		memcpy( newData + i*rows, data + i*rows, rows * sizeof(double) );
		if( sigma ) {
			memcpy( newSigma + i*rows, sigma + i*rows, rows * sizeof(double) );
		}
	}

	// 3.
	int rangeBegin = i;
	int destCols   = rangeBegin + count;
	

	// 4.
	int keep=0;
	for( ; i<cols; i++ ) {
		if( time[i] - epsilon > t1 ) {
			newTime[ destCols + keep ] = time[i];
			memcpy( newData + (destCols+keep)*rows, data + i*rows, rows * sizeof(double) );
			if( sigma ) {
				memcpy( newSigma + (destCols+keep)*rows, sigma + i*rows, rows * sizeof(double) );
			}
			keep++;
		}
	}

	cols = destCols + keep;

	free(time);
	free(data);
	free(derivs);
	if( sigma ) {
		free( sigma );
	}

	time = newTime;
	data = newData;
	derivs = newDerivs;
	sigma = newSigma;

	return rangeBegin;
}

void KineticTrace::set( int timeIndex, int reagentIndex, double val ) {
	assert( timeIndex >=0 && timeIndex < cols );
	data[timeIndex*rows+reagentIndex] = val;
}

void KineticTrace::set( int timeIndex, int reagentIndex, double _time, double val, double *_sigma ) {
	assert( timeIndex >=0 && timeIndex < cols );
	time[timeIndex] = _time;
	data[timeIndex*rows+reagentIndex] = val;
	if( _sigma ) {
		assert( sigma );
		sigma[timeIndex*rows+reagentIndex] = *_sigma;
	}
}

double *KineticTrace::getColPtr( int timeIndex ) {
	return &data[timeIndex*rows];
}

double *KineticTrace::getLastColPtr() {
	return &data[(cols-1)*rows];
}

double *KineticTrace::getDerivColPtr( int timeIndex ) {
	return &derivs[timeIndex*rows];
}

void KineticTrace::init( int _rows, int _cols /*=-1*/, int withSigma /*=0*/, int withRowCoords /*=0*/ ) {
	clear();

	rows = _rows;
	colsAlloced = _cols > 0 ? _cols : 1024;

	time = (double *)malloc( colsAlloced * sizeof(double) );
	memset( time, 0, colsAlloced * sizeof(double) );

	data = (double *)malloc( rows * colsAlloced * sizeof(double) );
	memset( data, 0, rows * colsAlloced * sizeof(double) );

	derivs = (double *)malloc( rows * colsAlloced * sizeof(double) );
	memset( derivs, 0, rows * colsAlloced * sizeof(double) );

	if( withSigma ) {
		sigma = (double *)malloc( rows * colsAlloced * sizeof(double) );
		double *s = sigma;
		for( int i=0; i< rows * colsAlloced; i++, s++ ) {
			*s = 1.0;
				// sigma is often used as a divisor to scale magnitudes into
				// units of standard deviations, so 1.0 is a handy default.
		}
	}
	if( withRowCoords ) {
		rowCoords = (double *)malloc( rows * sizeof(double) );
	}
}

void KineticTrace::zeroInit( int reagentCount ) {
	init( reagentCount );
	double *zeroVec = (double *)alloca( sizeof(double) * reagentCount );
	memset( zeroVec, 0, sizeof(double) * reagentCount );
	add( 0.0, zeroVec );
}

void KineticTrace::clear() {
	properties.clear();
	rows = 0;
	cols = 0;
	colsAlloced = 0;
	last = 0;
	if( time ) {
		free( time );
		time = 0;
	}
	if( rowCoords ) {
		free( rowCoords );
		rowCoords = 0;
	}
	if( data ) {
		free( data );
		data = 0;
	}
	if( sigma ) {
		free( sigma );
		sigma = 0;
	}
	if( derivs ) {
		free( derivs );
		derivs = 0;
	}
	polyMat.clear();
}

void KineticTrace::copy( KineticTrace &t ) {
	clear();
	properties.copyFrom( t.properties );
	rows = t.rows;
	cols = t.cols;
	colsAlloced = t.colsAlloced;
	last = 0;
	if( colsAlloced ) {
		time = (double *)malloc( colsAlloced * sizeof(double) );
		data = (double *)malloc( rows * colsAlloced * sizeof(double) );
		memcpy( time, t.time, colsAlloced * sizeof(double) );
		memcpy( data, t.data, rows * colsAlloced * sizeof(double) );
		if( t.sigma ) {
			sigma = (double *)malloc( rows * colsAlloced * sizeof(double) );
			memcpy( sigma, t.sigma, rows * colsAlloced * sizeof(double) );
		}
	}
	if( t.rowCoords ) {
		rowCoords = (double *)malloc( rows * sizeof(double) );
		memcpy( rowCoords, t.rowCoords, rows * sizeof(double) );
	}

	derivs = 0;
	if( t.derivs ) {
		derivs = (double *)malloc( rows * colsAlloced * sizeof(double) );
		memcpy( derivs, t.derivs, rows * colsAlloced * sizeof(double) );
	}
	if( t.polyMat.count ) {
		polyFit();
			// it's probably faster to copy the ZMats from t, but...
	}
}

void KineticTrace::copyRange( KineticTrace &t, double t0, double t1 ) {
	int count=0;
	int first=0;
	int i;
	for( i=0; i<t.cols; i++ ) {
		if( t.time[i] < t0 ) {
			first = i+1;
		}
		else if ( t.time[i] < t1 ) {
			count++;
		}
	}
	clear();
	properties.copyFrom( t.properties );
	if( count ) {
		rows = t.rows;
		cols = count;
		colsAlloced = count;
		last=0;
		if( colsAlloced ) {
			time = (double *)malloc( colsAlloced * sizeof(double) );
			data = (double *)malloc( rows * colsAlloced * sizeof(double) );
			memcpy( time, t.time + first, colsAlloced * sizeof(double) );
			memcpy( data, t.data + t.rows * first, rows * colsAlloced * sizeof(double) );
			if( t.sigma ) {
				sigma = (double *)malloc( rows * colsAlloced * sizeof(double) );
				memcpy( sigma, t.sigma + t.rows * first, rows * colsAlloced * sizeof(double) );
			}
		}
		if( t.rowCoords ) {
			rowCoords = (double *)malloc( rows * sizeof(double) );
			memcpy( rowCoords, t.rowCoords, rows * sizeof(double) );
		}

		derivs = 0;
		if( t.derivs ) {
			derivs = (double *)malloc( rows * colsAlloced * sizeof(double) );
			memcpy( derivs, t.derivs + t.rows * first, rows * colsAlloced * sizeof(double) );
		}

		// experiment: does it matter to polyFit if the times are 0-based?
		// (yes, it does.  this fixed the very strange anomolies in HIV_NNRTI.mec)
		double startTime = time[0];
		time[0] = 0.0;
		for( i=1; i<cols; i++ ) {
			time[i] -= startTime;
		}
	}
}

void KineticTrace::copyRow( KineticTrace &src, int srcRow, int dstRow, int copyTime ) {
	assert( dstRow < rows );
	assert( src.cols == cols );
	if( copyTime ) {
		memcpy( time, src.time, sizeof( time[0] ) * colsAlloced );
	}
	for( int i=0; i<src.cols; i++ ) {
		set( i, dstRow, src.getData( i, srcRow ) );
	}
}

void KineticTrace::copyDerivsToData( int row ) {
	for( int i=0; i<cols; i++ ) {
		set( i, row, getDeriv( i, row ) );
	}
	properties.putI( ZTmpStr("row%d_derivative_only",row), 1 );
}

void KineticTrace::stealOwnershipFrom( KineticTrace &src ) {
	clear();

	rows = src.rows;
	cols = src.cols;
	colsAlloced = src.colsAlloced;
	last = src.last;
	time = src.time;
	rowCoords = src.rowCoords;
	data = src.data;
	sigma = src.sigma;
	derivs = src.derivs;

	src.rows = 0;
	src.cols = 0;
	src.colsAlloced = 0;
	src.last = 0;
	src.time = 0;
	src.rowCoords = 0;
	src.data = 0;
	src.sigma = 0;
	src.derivs = 0;

	polyMat.stealOwnershipFrom( src.polyMat );
}

int KineticTrace::findClosestTimeCol( double _time ) {
	// Find the index into the vector for time
	// It caches the previous result so if you march through in
	// ascending time then it avoids a binary search
	// This function assumes that mostly you query in ascending order
	// by caching the last place you looked.
	// @TODO if this isn't the case a binary search is called for.
	assert( last >= 0 && last < cols );

	if( _time < time[last] ) {
		// If looking for a time which is earlier than the last, restart search at beginning
		last = 0;
	}

	while( last < cols-1 && _time >= time[last+1] ) {
		last++;
	}

	assert( last < cols );
	return last;
}

double KineticTrace::getElemLERP( double _time, int row ) {
	if( cols == 0 ) {
		// Special case: an empty trace always returns 0
		return 0.0;
	}

	int col = findClosestTimeCol( _time );
	if( col >= 0 ) {
		if( col == cols-1 ) {
			return data[ col*rows + row ];
		}
		else {
			// LERP
			double startTime = time[col+0];
			double deltaTime = time[col+1] - startTime;
			double startVal  = data[(col+0)*rows+row];
			double deltaVal  = data[(col+1)*rows+row] - startVal;
			return (_time - startTime) * deltaVal / deltaTime + startVal;
		}
	}

	assert( ! "getElemLERP out of bounds" );
	return 0.0;
}

void KineticTrace::getColLERP( double _time, double *_vector ) {
	if( cols == 0 ) {
		assert( 0 );
			// ??? Not sure what to do here
	}

	int col = findClosestTimeCol( _time );
	if( col >= 0 ) {
		if( col == cols-1 ) {
			memcpy( _vector, &data[col*rows], sizeof(double) * rows );
		}
		else {
			// LERP
			double startTime = time[col+0];
			double deltaTime = time[col+1] - startTime;
			double timeMinusStartTime = _time - startTime;

			for( int i=0; i<rows; i++ ) {
				double startVal = data[(col+0)*rows+i];
				double deltaVal = data[(col+1)*rows+i] - startVal;
				_vector[i] = timeMinusStartTime * deltaVal / deltaTime + startVal;
			}
		}
	}
	else {
		assert( ! "getColLERP out of bounds" );
	}
}

void KineticTrace::mul( ZMat &a, KineticTrace &b ) {
	// Creates a new matrix that is the transform of b by a, take the time from b
	assert( a.cols == b.rows+1 );
		// The matrix a is assumed to be homogenous so its dimension is one larger than b
		// and we will assume a value of 1 for the last row of b

	clear();
	rows = a.rows;
	cols = b.cols;
	colsAlloced = cols;
	last = 0;
	time = (double *)malloc( cols * sizeof(double) );
	data = (double *)malloc( rows * cols * sizeof(double) );
	memcpy( time, b.time, cols * sizeof(double) );

	int dim = a.cols -1;
		// -1 because we hand code the last homogenous row

	for( int r=0; r<rows; r++ ) {
		for( int c=0; c<cols; c++ ) {
			double sum = 0.0;

			int i;
			for( i=0; i<dim; i++ ) {
				sum += a.getD(r,i) * b.data[c*b.rows+i];
			}

			// PICKUP the last col because assumed homogenous a
			sum += a.getD(r,i);

			data[c*rows+r] = sum;
		}
	}
}

double KineticTrace::scaleData( double scale ) {
	int count = rows * cols;
	double *p = data;
	double *s = sigma;
	for( int i=0; i<count; i++, p++ ) {
		*p = *p * scale;
		if( s ) {
			*s = *s * scale;
			s++;
				// is this correct math?  sigma *s is the standard deviation of the datapoint *p
		}
	}
	double previousScale = properties.getD( "scaleData", 1.0 );
	double newScale = previousScale * scale;
	properties.putD( "scaleData", newScale );
	return newScale;
}

void KineticTrace::cubicFit() {
	// This is the analog to polyFit, but uses a 3rd order instead of 5th,
	// two points at time instead of three.
	int r,c;
	if( cols < 2 ) {
		return;
	}
	
	// ALLOCATE polyMats
	polyMat.clear();
	for( r=0; r<rows; r++ ) {
		int _rows=2, _cols=cols-1;
		if( properties.getI( ZTmpStr("row%d_derivative_only") )) {
			_rows = _cols = 0;
		}
		polyMat.add( new ZMat(_rows, _cols, zmatF64) );
	}

	double y0,y1,d0,d1,a,b,x0,x1;
	for( c=0; c<cols-1; c++ ) {
		x0 = getTime(c+0);
		x1 = getTime(c+1);
		for( r=0; r<rows; r++ ) {
			y0 = getColPtr(c+0)[r];
			y1 = getColPtr(c+1)[r];
			d0 = getDerivColPtr(c+0)[r];
			d1 = getDerivColPtr(c+1)[r];
			a =  d0 * (x1-x0) - (y1-y0);
			b = -d1 * (x1-x0) + (y1-y0);
			polyMat[r]->putD( 0, c, a );
			polyMat[r]->putD( 1, c, b );
		}
	}
}

double KineticTrace::getElemCubic( double _time, int row ) {
	if( rows == 0 || cols == 0 ) {
		return 0.0;
	}
	if( polyMat.count != rows || polyMat[row]->cols != cols-1 || polyMat[row]->rows != 2 ) {
		return getElemLERP( _time, row );
	}
	assert( polyMat.count > 0 );
	
	int col = findClosestTimeCol( _time );
	assert( col >= 0 && col < cols );

	if( col == cols - 1 ) {
		return data[col*rows+row];
	}

	double t,t0,t1,oneMinusT;
	t0 = getTime( col );
	t1 = getTime( col + 1 );
	t = ( _time - t0 ) / ( t1 - t0 );
	oneMinusT = 1.0 - t;
	
	double y0,y1;
	y0 = getColPtr(col+0)[row];
	y1 = getColPtr(col+1)[row];
	
	double a,b;
	a = polyMat[row]->getD( 0, col );
	b = polyMat[row]->getD( 1, col );
	
	return (oneMinusT * y0) + (t * y1) + t * oneMinusT * (a * oneMinusT + b * t);
}

void KineticTrace::getColCubic( double _time, double *_vector ) {
	if( polyMat.count == 0 ) {
		getColLERP( _time, _vector );
		return;
	}
	
	int col = findClosestTimeCol( _time );
	assert( col >= 0 && col < cols );
	
	if( col == cols - 1 ) {
		memcpy( _vector, &data[col*rows], sizeof(double) * rows );
		// or with one.
		return;
	}
	
	double t,t0,t1,oneMinusT;
	t0 = getTime( col );
	t1 = getTime( col + 1 );
	t = ( _time - t0 ) / ( t1 - t0 );
	oneMinusT = 1.0 - t;
	
	double *y0,*y1;
	y0 = getColPtr(col+0);
	y1 = getColPtr(col+1);
	
	for( int i=0; i<rows; i++ ) {
		double a,b;
		a = polyMat[i]->getD( 0, col );
		b = polyMat[i]->getD( 1, col );
		_vector[i] = (oneMinusT * y0[i]) + (t * y1[i]) + t * oneMinusT * (a * oneMinusT + b * t);
		
	}
}

void KineticTrace::polyFit() {
	if( Kin_simSmoothing3 ) return cubicFit();
	int r, c;

	// @TODO: I think that this can be significantly optimized
	// by just pre-computing the polynomial inverted matrix
	// and hard-coding the matrix product instead of using LU.


	// @TODO: What to do if there's only two points?
	if( cols < 3 ) {
		return;
	}

	// ALLOCATE the polyMats;
	polyMat.clear();
	for( r=0; r<rows; r++ ) {
		int _rows=6, _cols=cols-2;
		if( properties.getI( ZTmpStr("row%d_derivative_only") )) {
			_rows = _cols = 0;
		}
		polyMat.add( new ZMat(_rows, _cols, zmatF64) );
	}

	gsl_vector *gslX = gsl_vector_alloc( 6 );
	gsl_permutation *gslP = gsl_permutation_alloc( 6 );

	ZMat powerMat( 5, cols, zmatF64 );
	for( c=0; c<cols; c++ ) {
		double tc = time[c];
		double t = time[c];
		powerMat.putD( 0, c, t );
		t *= tc;
		powerMat.putD( 1, c, t );
		t *= tc;
		powerMat.putD( 2, c, t );
		t *= tc;
		powerMat.putD( 3, c, t );
		t *= tc;
		powerMat.putD( 4, c, t );
	}

	ZMat m( 6, 6, zmatF64 );

	for( c=0; c<cols-2; c++ ) {

		// LOAD the polynomial matrix
		// GSL uses row major matricies so I've transposed everything below
		m.putD( 0, 0, 1.0 );
		m.putD( 1, 0, powerMat.getD(0,c+0) );
		m.putD( 2, 0, powerMat.getD(1,c+0) );
		m.putD( 3, 0, powerMat.getD(2,c+0) );
		m.putD( 4, 0, powerMat.getD(3,c+0) );
		m.putD( 5, 0, powerMat.getD(4,c+0) );
		
		m.putD( 0, 1, 1.0 );
		m.putD( 1, 1, powerMat.getD(0,c+1) );
		m.putD( 2, 1, powerMat.getD(1,c+1) );
		m.putD( 3, 1, powerMat.getD(2,c+1) );
		m.putD( 4, 1, powerMat.getD(3,c+1) );
		m.putD( 5, 1, powerMat.getD(4,c+1) );
		
		m.putD( 0, 2, 1.0 );
		m.putD( 1, 2, powerMat.getD(0,c+2) );
		m.putD( 2, 2, powerMat.getD(1,c+2) );
		m.putD( 3, 2, powerMat.getD(2,c+2) );
		m.putD( 4, 2, powerMat.getD(3,c+2) );
		m.putD( 5, 2, powerMat.getD(4,c+2) );
		
		m.putD( 0, 3, 0.0 );
		m.putD( 1, 3, 1.0 );
		m.putD( 2, 3, 2.0 * powerMat.getD(0,c+0) );
		m.putD( 3, 3, 3.0 * powerMat.getD(1,c+0) );
		m.putD( 4, 3, 4.0 * powerMat.getD(2,c+0) );
		m.putD( 5, 3, 5.0 * powerMat.getD(3,c+0) );
		
		m.putD( 0, 4, 0.0 );
		m.putD( 1, 4, 1.0 );
		m.putD( 2, 4, 2.0 * powerMat.getD(0,c+1) );
		m.putD( 3, 4, 3.0 * powerMat.getD(1,c+1) );
		m.putD( 4, 4, 4.0 * powerMat.getD(2,c+1) );
		m.putD( 5, 4, 5.0 * powerMat.getD(3,c+1) );
		
		m.putD( 0, 5, 0.0 );
		m.putD( 1, 5, 1.0 );
		m.putD( 2, 5, 2.0 * powerMat.getD(0,c+2) );
		m.putD( 3, 5, 3.0 * powerMat.getD(1,c+2) );
		m.putD( 4, 5, 4.0 * powerMat.getD(2,c+2) );
		m.putD( 5, 5, 5.0 * powerMat.getD(3,c+2) );

		// SOLVE by LU. 
		gsl_matrix_view gslM = gsl_matrix_view_array( (double *)m.mat, 6, 6 );

		int s;
		int err = gsl_linalg_LU_decomp( &gslM.matrix, gslP, &s );
		
		for( r=0; r<rows; r++ ) {
			if( polyMat[r]->rows==0 ) {
				continue;
					// can't polyfit this row, see "row%d_derivative_only"
			}
			double b[6];
			b[0] = getColPtr(c+0)[r];
			b[1] = getColPtr(c+1)[r];
			b[2] = getColPtr(c+2)[r];
			b[3] = getDerivColPtr(c+0)[r];
			b[4] = getDerivColPtr(c+1)[r];
			b[5] = getDerivColPtr(c+2)[r];
			gsl_vector_view gslB = gsl_vector_view_array( b, 6 );

			err = gsl_linalg_LU_solve( &gslM.matrix, gslP, &gslB.vector, gslX );

			// STORE the coefficients into the polyMat
			for( int i=0; i<6; i++ ) {
				polyMat[r]->putD( i, c, gsl_vector_get(gslX, i) );
			}
		}
	}

	gsl_permutation_free( gslP );
	gsl_vector_free( gslX );
}

double KineticTrace::getElemSLERP( double _time, int row ) {
	if( Kin_simSmoothing3 ) return getElemCubic( _time, row );
	
//	assert( polyMat.count > 0 );
		// Everything should now slerp and this requires that the poly mat was setup
		// Eventually this assert can go away to deal with the case that
		// there is only 2 data points in some degenerate trace
	if( rows == 0 || cols == 0 ) {
		return 0.0;
	}
	if( polyMat.count != rows || polyMat[row]->cols != cols-2 || polyMat[row]->rows != 6 ) {
		return getElemLERP( _time, row );
	}

	assert( polyMat.count > 0 );

	int col = findClosestTimeCol( _time );
	if( col == cols - 2 ) {
		return getElemLERP( _time, row );
			// can't slerp with only two points
	}
	else if( col == cols - 1 ) {
		return data[col*rows+row];
			// or with one.
	}
	
	double t1, t2, t3, t4, t5;
	t1 = _time;
	t2 = t1 * _time;
	t3 = t2 * _time;
	t4 = t3 * _time;
	t5 = t4 * _time;

	if( col >= 0  && col < cols-2 ) {
		// SLERP
		assert( polyMat.count == rows && polyMat[row]->cols == cols-2 && polyMat[row]->rows == 6 );

		double startTime = time[col+0];
		double deltaTime = time[col+1] - startTime;
		double timeMinusStartTime = _time - startTime;

		// The last two points don't have their own coefficients
		//col = min( col, col-3 );
		col = min( col, cols-3 );

		// @TODO: Thre are actually overlapping answers to this
		// should I take the mean of the two approximations?

		ZMat &coefs = *polyMat[row];
		double a0 = coefs.getD( 0, col );
		double a1 = coefs.getD( 1, col );
		double a2 = coefs.getD( 2, col );
		double a3 = coefs.getD( 3, col );
		double a4 = coefs.getD( 4, col );
		double a5 = coefs.getD( 5, col );

		// @TODO: Factor all these multiplies
		return a0 + a1*t1 + a2*t2 + a3*t3 + a4*t4 + a5*t5;
	}

	assert( ! "getElemSLERP out of bounds" );
	return 0.0;
}

void KineticTrace::getColSLERP( double _time, double *_vector ) {
	if( Kin_simSmoothing3 ) return getColCubic(_time, _vector );
	
	if( polyMat.count == 0 ) {
		// There was not enough points to polyfit so we revert to LERP
		getColLERP( _time, _vector );
		return;
	}

	assert( polyMat.count > 0 );

	int col = findClosestTimeCol( _time );
	if( col == cols - 2 ) {
		return getColLERP( _time, _vector );
			// can't slerp with only two points
	}
	else if( col == cols - 1 ) {
		memcpy( _vector, &data[col*rows], sizeof(double) * rows );
			// or with one.
		return;
	}

	double t1, t2, t3, t4, t5;
	t1 = _time;
	t2 = t1 * _time;
	t3 = t2 * _time;
	t4 = t3 * _time;
	t5 = t4 * _time;

	if( col >= 0 && col < cols-2 ) {
		// SLERP
		assert( polyMat.count == rows && polyMat[0]->cols == cols-2 && polyMat[0]->rows == 6 );

		double startTime = time[col+0];
		double deltaTime = time[col+1] - startTime;
		double timeMinusStartTime = _time - startTime;

		// The last two points don't have their own coefficients
		col = max( 0, min( col, cols-3 ) );

		for( int i=0; i<rows; i++ ) {
			// @TODO: Thre are actually overlapping answers to this
			// should I take the mean of the two approximations?

			ZMat &coefs = *polyMat[i];
			double a0 = coefs.getD( 0, col );
			double a1 = coefs.getD( 1, col );
			double a2 = coefs.getD( 2, col );
			double a3 = coefs.getD( 3, col );
			double a4 = coefs.getD( 4, col );
			double a5 = coefs.getD( 5, col );

			// @TODO: Factor all these multiplies
			_vector[i] = a0 + a1*t1 + a2*t2 + a3*t3 + a4*t4 + a5*t5;
		}
	}
	else {
		assert( ! "getColLERP out of bounds" );
	}
}

int KineticTrace::getMinNonzeroTime( double &minT, int initBounds ) {
	if( initBounds ) {
		minT = +DBL_MAX;
	}
	for( int c=0; c<cols; c++ ) {
		if( time[c] != 0.0 ) {
			minT = min( minT, time[c] );
		}
	}
	return cols > 0;
}


int KineticTrace::getBounds( double &minT, double &maxT, double &minY, double &maxY, int initBounds ) {
	if( initBounds ) {
		minT = +DBL_MAX;
		maxT = -DBL_MAX;
		minY = +DBL_MAX;
		maxY = -DBL_MAX;
	}
	double *d = data;
	for( int c=0; c<cols; c++ ) {
		minT = min( minT, time[c] );
		maxT = max( maxT, time[c] );

		for( int r=0; r<rows; r++ ) {
			minY = min( minY, *d );
			maxY = max( maxY, *d );
			d++;
		}
	}
	return cols > 0;
}

int KineticTrace::getBoundsForRow( int row, double &minY, double &maxY, int initBounds ) {
	if( initBounds ) {
		minY = +DBL_MAX;
		maxY = -DBL_MAX;
	}
	double *d = data + row;
	for( int c=0; c<cols; c++ ) {
		minY = min( minY, *d );
		maxY = max( maxY, *d );
		d += rows;
	}
	return cols > 0;
}

int KineticTrace::getBoundsForRange( int row, double &minY, double &maxY, double t0, double t1, int initBounds, int derivBounds ) {
	if( initBounds ) {
		minY = +DBL_MAX;
		maxY = -DBL_MAX;
	}
	int rbegin, rend;
	if( row < 0 ) {
		// examine all rows
		rbegin = 0;
		rend   = rows;
	}
	else {
		// examine a single row
		rbegin = row;
		rend   = row + 1;
	}
	int foundData=0;
	for( int r=rbegin; r<rend; r++ ) {
		double *pval = derivBounds ? (derivs + r) : (data + r); 
		for( int i=0; i<cols; i++, pval+=rows ) {
			if( time[i] >= t0 && time[i] <= t1 || doubleeq(time[i],t0) || doubleeq(time[i],t1) ) {
				// the above could be optimized if we assume the time values to be in order
				maxY = max( maxY, *pval );
				minY = min( minY, *pval );
				foundData=1;
			}
		}
	}
	return foundData;
}

int KineticTrace::getBoundsRowCoords( double &minRC, double &maxRC, int initBounds ) {
	int retval = 0;
	if( rowCoords ) {
		retval = 1;
		if( initBounds ) {
			minRC = +DBL_MAX;
			maxRC = -DBL_MAX;
		}
		for( int r=0; r<rows; r++ ) {
			minRC = min( minRC, rowCoords[ r ] );
			maxRC = max( maxRC, rowCoords[ r ] );
		}
	}
	return retval;
}


int KineticTrace::sameTimeEntries( KineticTrace &t ) {
	
	if( t.cols != cols ) {
		return 0;
	}
	for( int i=0; i<cols; i++ ) {
		if( !doubleeq( *(time+i), *(t.time+i) ) ) {
			return 0;
		}
	}
	return 1;
}


double KineticTrace::getMinStepSize() {
	double minDT = DBL_MAX;
	for( int i=1; i<cols; i++ ) {
		double dt = time[i] - time[i-1];
		minDT = min( dt, minDT );
	}
	return minDT;
}

const int KineticTrace_Version = 20100919;
int KineticTrace::loadBinary( FILE *f, int byteswap ) {
	int version;
	freadEndian( &version, sizeof(version), 1, f, byteswap );

	switch( version ) {
		case 20081016: {
			int len;
			freadEndian( &len, sizeof(len), 1, f, byteswap );
			char *d = (char*)alloca( len ); if(!d) return 0;
			fread( d, len, 1, f );	
			
			int _rows, _cols;
			freadEndian( &_rows, sizeof(_rows), 1, f, byteswap );
			freadEndian( &_cols, sizeof(_cols), 1, f, byteswap );

			init( _rows, _cols );
			cols = _cols;

			if( d && len ) {
				properties.putS( "desc", d );
					// this is awkwardly placed after the call to init since
					// init clears properties.
			}


			freadEndian( time, sizeof(double), _cols, f, byteswap );
			freadEndian( data, sizeof(double), _rows*_cols, f, byteswap );
		}
		break;

		case 20090324: {
			int _rows, _cols;
			freadEndian( &_rows, sizeof(_rows), 1, f, byteswap );
			freadEndian( &_cols, sizeof(_cols), 1, f, byteswap );

			init( _rows, _cols );
			cols = _cols;

			freadEndian( time, sizeof(double), _cols, f, byteswap );
			freadEndian( data, sizeof(double), _rows*_cols, f, byteswap );

			zHashTableLoad( f, properties );
			double scale = properties.getD( "scaleData", 1.0 );
		}
		break;

		case 20100106: {
			int _rows, _cols, hasSigma;
			freadEndian( &_rows, sizeof(_rows), 1, f, byteswap );
			freadEndian( &_cols, sizeof(_cols), 1, f, byteswap );
			freadEndian( &hasSigma, sizeof(hasSigma), 1, f, byteswap );

			init( _rows, _cols, hasSigma );
			cols = _cols;

			freadEndian( time, sizeof(double), _cols, f, byteswap );
			freadEndian( data, sizeof(double), _rows*_cols, f, byteswap );

			if( hasSigma ) {
				freadEndian( sigma, sizeof(double), _rows*_cols, f, byteswap );
			}

			zHashTableLoad( f, properties );
			double scale = properties.getD( "scaleData", 1.0 );
		}
		break;

		case 20100919: {
			int _rows, _cols, hasSigma, hasRowCoords;
			freadEndian( &_rows, sizeof(_rows), 1, f, byteswap );
			freadEndian( &_cols, sizeof(_cols), 1, f, byteswap );
			freadEndian( &hasSigma, sizeof(hasSigma), 1, f, byteswap );
			freadEndian( &hasRowCoords, sizeof(hasRowCoords), 1, f, byteswap );

			init( _rows, _cols, hasSigma, hasRowCoords );
			cols = _cols;

			freadEndian( time, sizeof(double), _cols, f, byteswap );
			freadEndian( data, sizeof(double), _rows*_cols, f, byteswap );

			if( hasSigma ) {
				freadEndian( sigma, sizeof(double), _rows*_cols, f, byteswap );
			}
			if( hasRowCoords ) {
				freadEndian( rowCoords, sizeof(double), _rows, f, byteswap );
			}

			zHashTableLoad( f, properties );
			double scale = properties.getD( "scaleData", 1.0 );
		}
		break;



		default:
			return 0;
	}
	return 1;
}

int KineticTrace::saveBinary( FILE *f ) {
	fwrite( &KineticTrace_Version, sizeof(KineticTrace_Version), 1, f );

	fwrite( &rows, sizeof(rows), 1 ,f );
	fwrite( &cols, sizeof(cols), 1 ,f );

	int hasSigma = sigma ? 1 : 0;
	fwrite( &hasSigma, sizeof( int ), 1, f );

	int hasRowCoords = rowCoords ? 1 : 0;
	fwrite( &hasRowCoords, sizeof( int ), 1, f );

	fwrite( time, sizeof(double)*cols, 1, f );
	fwrite( data, sizeof(double)*rows*cols, 1, f );

	if( hasSigma ) {
		fwrite( sigma, sizeof(double)*rows*cols, 1, f );
	}

	if( rowCoords ) {
		fwrite( rowCoords, sizeof(double)*rows, 1, f );
	}

	zHashTableSave( properties, f );

	// for now don't store derivatives or polyMat fit matrix; these can be computed.

	return 1;
}

void KineticTrace::saveText( char *filename, int row, double stepSize ) {
	// simple, for debuggging; this does not print any sigma values
	int start=0;
	int end=rows;
	if( row >= 0 ) {
		start = row;
		end = row + 1;
	}

	char buf[64];
	FILE *f = fopen( filename, "w" );
	if( f ) {
		for( int r=start; r<end; r++ ) {
			if( stepSize > 0.0 ) {
				for( double t=time[0]; t<=time[cols-1]; t += stepSize ) {
					ZTmpStr time("%g",t);
					ZTmpStr val("%g",getElemSLERP( t, r ) );
					sprintf( buf, "%15s%15s\n", time.s, val.s  );
					fprintf( f, "%s", buf );
				}
			}
			else {
				for( int c=0; c<cols; c++ ) {
					ZTmpStr t("%g",time[c]);
					ZTmpStr v("%g",getData( c, r ) );
					sprintf( buf, "%15s%15s\n", t.s, v.s  );
					fprintf( f, "%s", buf );
				}
			}
			fprintf( f, "\n\n" );
		}
		fclose( f );
	}


}

// KineticParameterInfo
//------------------------------------------------------------------------------------------------------------------------------------

void KineticParameterInfo::dump( KineticSystem *s ) {
	char buf[256];
	int buflen=256;
	trace( "KineticParamterInfo [%X] : %s, %s\n", this, name, type==PI_REACTION_RATE ? "Reaction Rate" : type==PI_OBS_CONST ? "Obs Const" : type == PI_INIT_COND ? "Initial Cond" : "BAD TYPE" );
	trace( "\tqIndex = %d pIndex = %d\n", qIndex, pIndex );
	trace( "\texperiment: %d (%s)\n", experiment, experiment>=0 ? s->experiments.count > experiment ? s->experiments[experiment]->viewInfo.getS( "name", "unnamed", buf, &buflen ) : "(null)" : "all" );
	trace( "\tmixstep: %d\n", mixstep );
	trace( "\tReagent: %d (%s)\n", reagent, reagent>=0 ? s->reagentGetName(reagent) : "n/a" );
	trace( "\tReaction: %d (%s)\n", reaction, reaction>=0 ? s->reactionGetString(reaction) : "n/a" );
	trace( "\tObsConst: %d \n", obsConst );
	trace( "\tGroup: %d\n", group );
	trace( "\tFitflag: %d\n", fitFlag );
	trace( "\tValue: %g\n", value );
}

// SavedKineticParameterInfo
//------------------------------------------------------------------------------------------------------------------------------------

void SavedKineticParameterInfo::byteSwap() {
	::BYTESWAP( value );
	::BYTESWAP( group );
	::BYTESWAP( fitFlag );
}

// KineticExperiment
//------------------------------------------------------------------------------------------------------------------------------------

KineticExperiment::KineticExperiment( KineticSystem *_system ) {
	system = 0;
	vmoc = 0;
	vmod = 0;
	vmog = 0;
	viewInfo.setThreadSafe( 1 );
	clear();
	ownsMeasured = 1;
	system = _system;
	id = _system->experimentNextUniqueId++;
}

KineticExperiment::~KineticExperiment() {
	clear();
}

int KineticExperiment::reactionCount() {
	return system->reactionCount();
}

int KineticExperiment::reagentCount() {
	return system->reagentCount();
}

int KineticExperiment::getExperimentIndex() {
	for( int i=0; i<system->experiments.count; i++ ) {
		if( system->experiments[i] == this ) {
			return i;
		}
	}
	assert( 0 );
	return -1;
}

int KineticExperiment::getMasterExperimentOrdinal() {
	// The strange name is meant to cause you to notice
	// that this does NOT return the index of this experiments
	// "master" experiment.  It counts the master experiments,
	// and returns the 1-based 'count' associated with the master
	// of this experiment.  E.g. if this is the 3rd master
	// experiment in the system, or is a slave to the 3rd master, 
	// we will return 3.
	

	int masterIndex = slaveToExperimentId >= 0 ? system->getExperimentById( slaveToExperimentId )->getExperimentIndex() : getExperimentIndex();
	int masterOrdinal = 0;
	for( int i=0; i<=masterIndex; i++ ) {
		if( system->experiments[i]->slaveToExperimentId < 0 ) {
			masterOrdinal++;
		}
	}
	return masterOrdinal;
}

void KineticExperiment::clear( int retainMeasured /*=0*/) {
	system					= 0;
	slaveToExperimentId		= -1;
	reagentIndexForSeries	= -1;
	mixstepIndexForSeries	= -1;
	mixstepDilutionTime		= 1e-5;
	mixstepDomain			= -1;
	mixstepNextUniqueId		= 0;
	mixstepCount			= 0;
	newMixstep( 0, 1.0, 1.0 );
	mixstepDataRefsDirty	= 0;

	equilibriumExperiment = 0;
	equilibriumVariance.clear();
	traceC.clear();
	traceG.clear();
	observableInstructions.clear();
	observableError.clear();
	traceOC.clear();
//	traceOG.clear();
	if( vmoc ) delete vmoc;
	if( vmod ) delete vmod;
	if( vmog ) delete vmog;
	vmoc = 0;
	vmod = 0;
	vmog = 0;
	observableResidualFFTMat.clear();
	if( !retainMeasured ) {
		measuredClear();
	}
	
	titrationProfile.clear();

	render_measured.clear();
	render_traceC.clear();
	render_traceG.clear();
	render_traceOC.clear();
//	render_traceOG.clear();
	render_traceDebug.clear();
}

void KineticExperiment::Mixstep::clear() {
	id=-1;
	duration = 1.0;
	dilution = 1.0;
	dataRefs.clear();
	traceOCForMixstep.clear();
	traceOCExtraForMixstep.clear();
}

void KineticExperiment::Mixstep::steal( Mixstep& toSteal ) {
	clear();
	id = toSteal.id;
	duration = toSteal.duration;
	dilution = toSteal.dilution;
	dataRefs.stealOwnershipFrom( toSteal.dataRefs );
	traceOCForMixstep.stealOwnershipFrom( toSteal.traceOCForMixstep );
	traceOCExtraForMixstep.clear();
	for( int i=0; i<toSteal.traceOCExtraForMixstep.count; i++ ) {
		traceOCExtraForMixstep.add( toSteal.traceOCExtraForMixstep.get( i ) );
		toSteal.traceOCExtraForMixstep.setWithoutDelete( i, 0 );
	}
	toSteal.traceOCExtraForMixstep.clear();
}

void KineticExperiment::mixstepInfo( int mixstep, double &startTime, double &endTime, double &dilution ) {
	assert( mixstep < mixstepCount );
	startTime=0.0;
	for( int i=0; i<mixstep; i++ ) {
		startTime += mixsteps[ i ].duration;
	}
	endTime  = startTime + mixsteps[ mixstep ].duration;
	dilution = mixsteps[ mixstep ].dilution;
}

void KineticExperiment::newMixstep( double *ics, double dilution, double duration ) {
	assert( mixstepCount < MAX_MIX_STEPS );
	mixsteps[ mixstepCount ].dilution = dilution;
	mixsteps[ mixstepCount ].duration = duration;
	mixsteps[ mixstepCount ].id       = mixstepNextUniqueId++;

	mixstepCount++;

	// system has been reparameterized: parameterInfo needs updating!
}

void KineticExperiment::delMixstep( int mixstep ) {
	// Wack the current mixstep, and translate any remaining; this DOES NOT
	// manage the deletion of series experiments in the case that the given 
	// mixstep defined the series; this should be handled before this call.
	if( mixstepCount==0 ) return;

	mixsteps[ mixstep ].clear();
		// clear the deleted mixstep

	// Shift the array contents if necessary
	if( mixstep < mixstepCount-1 ) {
		for( int i=mixstep; i<mixstepCount-1; i++ ) {
			mixsteps[i].steal( mixsteps[i+1] );
		}
		mixsteps[0].dilution = 1.0;
	}

	// Shift the mixstepIndexForSeries if necessary: the series should have 
	// been deleted already if this mixstep defined a series:
	assert( mixstep != mixstepIndexForSeries );
	if( mixstep < mixstepIndexForSeries ) {
		mixstepIndexForSeries--;
	}

	mixstepCount--;

	if( mixstepDomain >= mixstepCount ) {
		mixstepDomain--;
	}

	mixstepDataRefsDirty = 1;

	// System has been reparameterized: parameterInfo needs updating!
}
	
void KineticExperiment::simulate( struct KineticVMCodeD *vmd, double *pVec, int steps ) {
	zprofBeg( exp_sim );

	double errTolerance = viewInfo.getD( "integratorErrorTolerance" );
		// this value is used for both absolute and relative tolerance if it exists;
		// if not, the 0 we pass will cause defaults to get used.
	KineticIntegrator *integrator = new KineticIntegrator( KI_ROSS, vmd, errTolerance, errTolerance );
	
	double *dilutedC = (double*)alloca( reagentCount() * sizeof(double) );

	double *rrPlusOtherParams = (double*)alloca( vmd->pCount * sizeof(double) );
		// TFB: I changed things related to pCount during my voltage work; I changed pCount
		// such that it means *all* params, that is reactionRates plus any obsconsts plus
		// voltage/temp/press parameters, because this makes the most sense.  But there are
		// places in which historically we've passed only rates as P, and now I need to take
		// care to pass P vectors that are always as long as pCount lest memacess fault occur.

	int experimentIndex = getExperimentIndex();
	int singleMolecule  = viewInfo.getI( "singleMolecule" );
	int voltageDepends		= system->isDependent( KineticSystem::DT_Volt );
	int	temperatureDepends	= system->isDependent( KineticSystem::DT_Temp );
	int	concentrationDepends = system->isDependent( KineticSystem::DT_Conc );

	int i,j,hasFixed=0;
	ZMat fixedConc( reagentCount(), mixstepCount, zmatS32 );
	fixedConc.zero();
	for( i=0; i<fixedConc.rows; i++ ) {
		for( j=0; j<fixedConc.cols; j++ ) {
			if( viewInfo.has( ZTmpStr( "fixedReagentMix%d%s", j, system->reagentGetName( i ) ) ) ) {
				fixedConc.putF( i, j, 1 );
				hasFixed = 1;
			}
		}
	}

	double startTime = 0.0;
	for( i=0; i<mixstepCount; i++ ) {
		// CALCULATE startTime/endTime for this step

		double endTime = startTime + mixsteps[i].duration;
		if( endTime <= startTime ) {
			endTime = simulationDuration();
		}

		if( i < mixstepCount-1 ) {
			endTime -= mixstepDilutionTime;
				// if there are subsequent steps, allow enough time at the end of 
				// this step to apply a dilution, s.t. next step starts "on time"
		}

		int icCount=0, rrCount=0;
		double *ic = system->paramGetVector( PI_INIT_COND, &icCount, experimentIndex, i );
		double *rr = system->paramGetVector( PI_REACTION_RATE, &rrCount );

		if( voltageDepends ) {
			system->updateVoltageDependentRates( experimentIndex, i, rr );
				// NOTE: I am updating the computed rate per experiment/mixstep in the rr vector.
				// I am NOT modifying the pVec passed to us, which is used to computeOC() below.
				// Since the rates are not used in computing the observables, this should be ok.
		}
		else if( temperatureDepends ) {
			system->updateTemperatureDependentRates( experimentIndex, i , rr );
				// see comment above
		}
		else if( concentrationDepends ) {
			system->updateConcentrationDependentRates( experimentIndex, i, rr );
		}

		if( singleMolecule ) {
			for( int smRate=0; smRate<rrCount; smRate++ ) {
				if( viewInfo.has( ZTmpStr( "singleMoleculeRate%d", smRate ) ) ) {
					rr[ smRate ] = 0;
				}
			}
		}

		memcpy( rrPlusOtherParams, rr, rrCount * sizeof( double ) );
			// rrPlusOther... will be the P vector we pass to the integrator, and	
			// we've allocated it to be long enough such that memcpy of pCount 
			// length will not fault... 

		// CALC dilution if not step 0
		if( i > 0 ) {
			integrator->getDilutedC( mixsteps[i].dilution, ic, dilutedC );

			// Here we may want to purge a reagent -- i.e. dilutedC[reagent]=0.0
			//
			for( int r=0; r<reagentCount(); r++ ) {
				if( viewInfo.has( ZTmpStr( "purgeReagentMix%d%s", i, system->reagentGetName( r ) ) ) ) {
					dilutedC[ r ] = 0.0;
				}
			}
		}

		zprofBeg( integrateD );
		if( steps > 0 ) {
			// STEP integrate for fixed step size
			integrator->prepareD( i==0 ? ic : dilutedC, rrPlusOtherParams, endTime, startTime );
			double stepTime  = ( endTime - startTime ) / double( steps - 1);
			for( int t=0; t<steps-1; t++ ) {
				integrator->stepD( stepTime );
			}
		}
		else {
			// INTEGRATE the current chunk
			integrator->integrateD( i==0 ? ic : dilutedC, rrPlusOtherParams, endTime, startTime, 0.0,
								   hasFixed ? fixedConc.getPtrI( 0, i ) : 0 );
		}
		zprofEnd(); // integrateD 

		delete ic;
		delete rr;

		startTime = endTime + mixstepDilutionTime;
	}

	zprofBeg( steal );
	traceC.stealOwnershipFrom( integrator->traceC );
	traceG.stealOwnershipFrom( integrator->traceG );
	zprofEnd();
	delete integrator;

	assert( simulationDuration() > 0.0 );

	// COMPUTE the observables: note that we lock a mutex during observable-related computation
	// since this involves the alloc/dealloc of data that is rendered by the client app 
	TRACEOC_LOCK(this); 
	zprofBeg( polyfit );
	traceC.polyFit();	
	zprofEnd();

	zprofBeg( computeOC );
	if( mixstepCount==1 || (!voltageDepends && !temperatureDepends && !concentrationDepends) ) {
		computeOC( pVec );
	}
	else {
		// We will supply multiple pVec, one for each mixstep, since voltage
		// or temperature will typically change at each step.
		int pVecSize  = system->getPLength() * sizeof(double);
		double *pVecs = (double*)alloca( mixstepCount * pVecSize );
		double *pv = pVecs;
		for( int i=0; i<mixstepCount; i++, pv += system->getPLength() ) {
			memcpy( pv, pVec, pVecSize );
				// copy original pVec
			if( i>0 ) {
				// replace the params that likely are changing; the voltage and related
				// are at the end of the pVec, and we can deduce the index based on what
				// we can find out about regIndices...
				int rp0 = vmd->regIndexP( 0 );
				KineticParameterInfo *pi = system->paramGet( PI_VOLTAGE, 0, experimentIndex, i );
				if( pi ) {
					int rv = vmd->regIndexVolt();
					pv[ rv - rp0 ] = pi->value;
				}
				
				pi = system->paramGet( PI_TEMPERATURE, 0, experimentIndex, i );
				if( pi ) {
					int rt = vmd->regIndexTemp();
					pv[ rt - rp0 ] = pi->value;
				}

				pi = system->paramGet( PI_PRESSURE, 0, experimentIndex, i );
				if( pi ) {
					int rp = vmd->regIndexPres();
					pv[ rp - rp0 ] = pi->value;
				}

				pi = system->paramGet( PI_SOLVENTCONC, 0, experimentIndex, i );
				if( pi ) {
					int rp = vmd->regIndexSConc();
					pv[ rp - rp0 ] = pi->value;
				}

				// Initial Conc (constant value)
				pi = system->paramGet( PI_INIT_COND, 0, experimentIndex, i );
				int rc = vmd->regIndexConc();
				pv[ rc - rp0 ] = reagentIndexForSeries == -1 ? 1.0 : pi[ reagentIndexForSeries ].value;
			}
		}
		computeOCMixsteps( pVecs, system->getPLength() );
	}
	zprofEnd();
	
	// Potentially copy derivative info into data section on a per-row basis
	// if the [DERIV] command is present in the observable expression.
	for( i=0; i<observableInstructions.count; i++ ) {
		char *obs = observableInstructions[ i ];
		if( !obs ) {
			trace( "ERROR: observable instruction %d is NULL!\n", i );
		}
		if( obs && !strncmp( obs, "[DERIV]", 7 ) ) {
			traceOC.copyDerivsToData( i );
		}
		else {
			traceOC.properties.del( ZTmpStr("row%d_derivative_only",i) );
		}
	}

	// DO a polyFit on the observable trace such that SLERPS can be performed
	// whereever desired.  Should this be an option?  Is it expensive?
	zprofBeg( slerpPrep );
	if( viewInfo.getI( "Kin_fitUseSLERP", 1 ) ) {
		if( mixstepCount == 1 ) {
			traceOC.polyFit();	
		}
		else {
			// We have to do multiple polyFits due to discontinuities (!)
			double startTime, endTime, dilution;
			for( int step=0; step<mixstepCount; step++ ) {
				mixstepInfo( step, startTime, endTime, dilution );
				mixsteps[step].traceOCForMixstep.copyRange( traceOC, startTime, endTime + (step==mixstepCount-1 ? 1.0 : 0.0) );
				mixsteps[step].traceOCForMixstep.polyFit();
			}
		}
	}
	zprofEnd();

	TRACEOC_UNLOCK(this);

	SIMSTATE_LOCK( system );
	if( simulateState == KineticExperiment::KESS_Simulating ) {
		// check is necessary in case this experiment was dirtied during sim
		simulateState = KineticExperiment::KESS_Completed;
	}
	SIMSTATE_UNLOCK( system );
	zprofEnd(); // exp_sim
}

int KineticExperiment::reagentsEmpty() {
	if( viewInfo.getI( "isEquilibrium" ) ) {
		double eqStart = viewInfo.getD( "eqStart" );
		double eqEnd   = viewInfo.getD( "eqEnd" );
		double eqStep  = viewInfo.getD( "eqStep" );
		if( (eqStart != 0 || eqEnd != 0) && eqStep != 0 ) {
			return 0;
		}
		return 1;
	}

	int icCount=0;
	KineticParameterInfo *kpi = system->paramGet( PI_INIT_COND, &icCount, this->getExperimentIndex(), 0 );
	assert( icCount == system->reagentCount() );
	int r;
	for( r=0; r<icCount; r++) {
		if( kpi[r].value != 0 ) {
			break;
		}
	}
	if( r == icCount ) {
		return 1;
	}
	return 0;
}

void KineticExperiment::measuredClear() {
	if( ownsMeasured ) {
		for( int i=0; i<measured.count; i++ ) {
			if( measured[i] ) {
				delete measured[i];
				measured[i] = 0;
			}
		}
	}
	measured.clear();
}

double KineticExperiment::measuredDuration() {
	double duration = 0.0;

	for( int i=0; i<measured.count; i++ ) {
		int count = measured[i]->cols;
		for( int j=0; j<count; j++ ) {
			duration = max( duration, measured[i]->time[j] );
		}
	}

	return duration;
}

void KineticExperiment::measuredCreateFake( int numSteps, double variance ) {
	int i;

	if( equilibriumExperiment ) {
		return;
	}

	// CLEAR the measured data of the experiment
	measuredClear();

	// @TODO: Deal with mix steps
	assert( mixstepCount == 1 );

	assert( traceOC.cols > 0 );

	for( i=0; i<observablesCount(); i++ ) {
		KineticTrace *m = new KineticTrace();
		measured.add( m );
		m->init( 1 );
	}

	double start = observablesDomainBegin();
	double end = observablesDomainEnd();
	double duration = end - start;
	double step = duration / 1000.0;
	double *obsVector = (double *)alloca( sizeof(double) * traceOC.rows );
	for( double t=start; t<end; t+=step ) {
		// FETCH the lerped values for the observables at each time
		traceOC.getColSLERP( t, obsVector );

		// ADD noise (skipping over the time)
 		for( i=0; i < observablesCount(); i++ ) {
			obsVector[i] += zrandGaussianF() * variance;
			measured[i]->add( t, &obsVector[i] );
		}
	}
}

void KineticExperiment::measuredCreateFakeForMixsteps( int N, double sigma, double offset, int logTime, double **timeRefs ) {
	// Create synthetic experimental data for fit analysis etc, for the current mixstepDomain.
	measuredCreateFakeForMixsteps( measured, N, sigma, offset, logTime, timeRefs );
}
#ifdef KIN_DEV
// temporary for testing
extern int Kin_genDataVariatedSigma;
extern double Kin_genDataVariation;
#endif
void KineticExperiment::measuredCreateFakeForMixsteps( ZTLVec< KineticTrace* > &genTraces, int N, double sigma, double offset, int logTime, double **timeRefs ) {
	// Create synthetic experimental data for fit analysis etc, for the current mixstepDomain.

	int isEquilibrium = viewInfo.getI( "isEquilibrium" );
	int isPulseChase  = viewInfo.getI( "isPulseChase" );

	for( int i=0; i<observablesCount(); i++ ) {
		if( viewInfo.getI( ZTmpStr("obs%d-plot",i) ) ) {
						
			// CALCULATE time boundaries for data generation,taking into account any mixstepDomain
			double dataTime = observablesDomainBegin(); //0;	
				// what time to start generating data from
			double endTime  = observablesDomainEnd();
			double duration = endTime - dataTime; //simulationDuration();
				// how long to generate data
			double dataToSimulationOffset = 0.0;
				// in the case of a single mixstep, for which we will generate data starting at t=0,
				// what is the actual offset to the simulation at that point (when does the mixstep start?)
			int mixstepsToCreateDataFor = mixstepCount;
				// how many mixsteps will be create data for?  We want to generate N points for each mixstep.
			if( mixstepDomain >= 0 ) {
				double unused;
				mixstepInfo( mixstepDomain, dataToSimulationOffset, unused, unused );
				duration = mixsteps[ mixstepDomain ].duration;
				if( mixstepDomain+1 < mixstepCount ) {
					// note: this is only strictly necessary in the case that 
					// duration/N < epsilon (used below in call to zmatInsertRange)
					duration -= mixstepDilutionTime;
				}
				mixstepsToCreateDataFor = 1;
			}

			if( isEquilibrium ) {
				// This is a special case in which the observable output has nothing to
				// do with the simulation time or mixtep durations (its domain is concentration).
				duration = endTime - dataTime;
				mixstepsToCreateDataFor = 1;
			}
			
			int dataCreateCount = N * mixstepsToCreateDataFor;

			// REALLOC the matrix if required, preserving any data outside of the current domain.
			if( !genTraces[i] ) {
				genTraces.set( i, new KineticTrace() );
				genTraces[i]->init( 1, dataCreateCount, 1 );
					// for newly gen'd data, last param says alloc space for stored sigma as well
			}
			double epsilon = max( DBL_EPSILON, (( genTraces[i]->getLastTime() ) / 10000.0) );
			int dataBeginIndex = genTraces[i]->insertGapForRange( dataCreateCount, dataTime, dataTime+duration, epsilon );
#ifdef USE_KINFIT_V2
			memset( &fdc.numSteps, 0, sizeof( fdc.numSteps ) );
				// ensure no-one tries to ref old data in fdc, since we just gen'd new data
#endif
		
			// GENERATE the data.
			// note: data will be generated in the range [ dataTime, dataTime+duration )
			#ifndef KIN_DEV
			zrandSeedFromTime();
			int seedOffset = zrandI( 0, 20 );
			zrandSeed( 1969 + 13 * seedOffset );
				// always same 20 seeds; creates 'signature' for synthetic data to discourage fraud.
			#endif
			genTraces[i]->properties.putS( "source", ZTmpStr( "Generated: (sigma %.3lf, N=%d)", sigma, dataCreateCount ) );
			dataTime += offset;
			duration -= offset;
				// these are applied *after* the determination of dataBeginIndex such that any
				// data in this trace (or part of trace) will be overwritten by the new data.
			
			int n;
			for( int m=0; m<mixstepsToCreateDataFor; m++ ) {
				if( mixstepsToCreateDataFor > 1 ) {
					double unused;
					mixstepInfo( m, dataTime, unused, unused );
					duration = mixsteps[ m ].duration;				
					if( m+1 < mixstepCount ) {
						// note: this is only strictly necessary in the case that 
						// duration/N < epsilon (used below in call to zmatInsertRange)
						duration -= mixstepDilutionTime;
					}
					if( m == 0 ) {
						dataTime += offset;
						duration -= offset;
					}
				}
				double stepTime = duration/(N-1);

				if( logTime ) {
					// we'd like to gen data such that each range corresponding to a power of 10
					// gets some data.
					if( dataTime == 0.0 ) {
						traceOC.getMinNonzeroTime( dataTime, 1 );
							// at how small a time should we start the data gen?
						double minLogTime = pow( 10, (double)viewInfo.getI( "minLogscaleExponentX", -3 ) );
						dataTime = min( dataTime, minLogTime );
					}
					double logStart = log10( dataTime );
					double logEnd = log10( dataTime + duration );
					dataTime = logStart;
					stepTime = ( logEnd - logStart ) / (N-1);
				}
				for( n=dataBeginIndex; n < dataBeginIndex + N ; n++ ) {
					double time = logTime ? pow( 10, dataTime ) : dataTime;
					if( timeRefs && timeRefs[i] ) {
						// we should use the time reference passed to us; this is to allow
						// data to be generated with exactly the same time stamps as some other
						// existing trace.
						time = timeRefs[i][n];
					}

					double simulationTime = time + dataToSimulationOffset;

					// testing different sigma per datapoint:
					double useSigma = sigma;
					#ifdef KIN_DEV
						if( Kin_genDataVariatedSigma ) {
							double sigmaVariation = zrandGaussianF() * sigma * Kin_genDataVariation;
							useSigma = sigma + sigmaVariation;
						}
					#endif
					
					double val = getTraceOCSLERP( simulationTime, i, isEquilibrium || isPulseChase );
					genTraces[i]->set( n, 0, time, val + zrandGaussianF() * useSigma, genTraces[i]->sigma ? &useSigma : 0 );
						// note that we store the sigma per datapoint if this trace is alloc'd to hold sigma,
						// which will always be true if we just created.  We just don't want to try to store sigma
						// in a trace that already existed for which sigma was not available.  All or nothing.
					dataTime += stepTime;
				}
				dataBeginIndex = n;
					// for next mixstep
			}
		}
	}
}

int KineticExperiment::measuredHasAnyData() {
	for( int i=0; i<measured.count; i++ ) {
		if( measured[i] && measured[i]->cols > 0 ) {
			return 1;
		}
	}
	return 0;
}

int KineticExperiment::measuredDataHasSigma() {
	for( int i=0; i<measured.count; i++ ) {
		if( measured[i] && measured[i]->cols>0 && !measured[i]->sigma ) {
			return 0;
		}
	}
	return 1;
}

#ifdef USE_KINFIT_V2
void KineticExperiment::resetFitContext( int resetFitOutputFactors ) {
	// RESET the member fitDataContext as well as those in series conc experiments.

	// 27 jan 2010: only reset for this experiment, not any series.  I looked at
	// all uses of this fn, and they are all explicity calling on series 
	// experiments already.
	/*
	ZTLVec< KineticExperiment* > series;
	int count = getSeries( series );
	for( int i=0; i<count; i++ ) {
		series[i]->fdc.clear( resetFitOutputFactors );
	}
	*/
	fdc.clear( resetFitOutputFactors );
}

void KineticExperiment::enableObservableFit( int which, int updateSeries ) {
	// SET this observable up for fitting, optionally in all series conc experiments

	if( updateSeries ) {
		ZTLVec< KineticExperiment* > series;
		int count = getSeries( series );
		for( int i=0; i<count; i++ ) {
			series[i]->fdc.fitObservable[which] = 1;
		}
	}
	else {
		fdc.fitObservable[ which ] = 1;
	}
}
#endif

int KineticExperiment::getObservableConstantsForExperiment( ZTLVec< KineticParameterInfo* > &vec ) {
	vec.clear();
	int count;
	KineticParameterInfo *kpi = system->paramGet( PI_OBS_CONST, &count );
	for( int i=0; i<count; i++, kpi++ ) {
		if( obsConstsByName.getI( kpi->name, -1 ) != -1 ) {
			vec.add( kpi );
		}
	}
	return vec.count;
}

int KineticExperiment::seriesCount( int includeEqOrPC ) {
	int count = 1;
		// the "degenerate" series: the master

	if( includeEqOrPC || ( !viewInfo.getI( "isEquilibrium" ) && !viewInfo.getI( "isPulseChase" ) ) ) {
		int seriesMasterId = slaveToExperimentId >= 0 ? slaveToExperimentId : id;
		for( int i=0; i<system->experiments.count; i++ ) {
			if( system->experiments[i]->slaveToExperimentId == seriesMasterId ) {
				count++;
			}
		}
	}
	return count;
}

int KineticExperiment::getSeries( ZTLVec<KineticExperiment*> &vec, int includeEqOrPC ) {
	int seriesMasterId = slaveToExperimentId >= 0 ? slaveToExperimentId : id;

	vec.clear();
	vec.add( system->getExperimentById( seriesMasterId ) );
	int count = 1;
		// series master is always first (potentially only) in the list

	if( includeEqOrPC || ( !viewInfo.getI( "isEquilibrium" ) && !viewInfo.getI( "isPulseChase" ) ) ) {
		for( int i=0; i<system->experiments.count; i++ ) {
			if( system->experiments[i]->slaveToExperimentId == seriesMasterId ) {
				vec.add( system->experiments[i] );
				count++;
			}
		}
	}
	return count;
}

KineticParameterInfo * KineticExperiment::getSeriesParameterInfo() {
	KineticParameterInfo *pi = 0;

	int seriesType = viewInfo.getI( "seriesType" );
	switch( seriesType ) {
		case SERIES_TYPE_REAGENTCONC: {
			if( reagentIndexForSeries == -1 ) {
				return 0;
			}
			pi = system->paramGet( PI_INIT_COND, 0, getExperimentIndex(), mixstepIndexForSeries );
			assert( pi );
			pi += reagentIndexForSeries;
			break;
		}
		case SERIES_TYPE_VOLTAGE: {
			pi = system->paramGet( PI_VOLTAGE, 0, getExperimentIndex(), mixstepIndexForSeries );
			assert( pi );
			break;
		}
		case SERIES_TYPE_TEMPERATURE: {
			pi = system->paramGet( PI_TEMPERATURE, 0, getExperimentIndex(), mixstepIndexForSeries );
			assert( pi );
			break;
		}
		case SERIES_TYPE_SOLVENTCONC: {
			pi = system->paramGet( PI_SOLVENTCONC, 0, getExperimentIndex(), mixstepIndexForSeries );
			assert( pi );
			break;
		}
	}
	return pi;
}

double KineticExperiment::getSeriesConcentration() {
	if( viewInfo.getI( "seriesType" ) == SERIES_TYPE_REAGENTCONC ) {
		KineticParameterInfo *pi = getSeriesParameterInfo();
		if( pi ) {
			return pi->value;
		}
	}
	return -1.0;
}

double KineticExperiment::getSeriesVoltage() {
	if( viewInfo.getI( "seriesType" ) == SERIES_TYPE_VOLTAGE ) {
		KineticParameterInfo *pi = getSeriesParameterInfo();
		if( pi ) {
			return pi->value;
		}
	}
	return -1.0;
}

double KineticExperiment::getSeriesTemperature() {
	if( viewInfo.getI( "seriesType" ) == SERIES_TYPE_TEMPERATURE ) {
		KineticParameterInfo *pi = getSeriesParameterInfo();
		if( pi ) {
			return pi->value;
		}
	}
	return -1.0;
}

double KineticExperiment::getSeriesSolventConcentration() {
	if( viewInfo.getI( "seriesType" ) == SERIES_TYPE_SOLVENTCONC ) {
		KineticParameterInfo *pi = getSeriesParameterInfo();
		if( pi ) {
			return pi->value;
		}
	}
	return -1.0;
}

void KineticExperiment::getStatsForSeries( int &simulationStepsMin, int &simulationStepsMax, int &measuredCountMin, int &measuredCountMax ) {
	simulationStepsMin = 100000;
	simulationStepsMax = 0;
	measuredCountMin   = 100000;
	measuredCountMax   = 0;
	ZTLVec< KineticExperiment* > vec;
	int count = getSeries( vec );
	for( int i=0; i<count; i++ ) {
		simulationStepsMin = min( simulationStepsMin, vec[i]->simulationSteps() );
		simulationStepsMax = max( simulationStepsMax, vec[i]->simulationSteps() );
		int mcount = vec[i]->measured.count;
		for( int j=0; j<vec[i]->measured.count; j++ ) {
			measuredCountMin = min( measuredCountMin, vec[i]->measuredCount( j ) );
			measuredCountMax = max( measuredCountMax, vec[i]->measuredCount( j ) );
		}
	}
}

void KineticExperiment::updateSeriesFromMaster() {
	ZTLVec< KineticExperiment* > series;
	int count = getSeries( series, 1 );
	for( int i=1; i<count; i++ ) {
		series[i]->updateFromMaster( *series[0] );
	}
}

void KineticExperiment::mixstepSetDuration( int mixstep, double duration ) {
	mixsteps[ mixstep ].duration = duration;
	mixstepDataRefsDirty = 1;
	for( int i=0; i<system->experiments.count; i++ ) {
		if( system->experiments[i]->slaveToExperimentId == id ) {
			system->experiments[i]->mixsteps[ mixstep ].duration = duration;
			system->experiments[i]->mixstepDataRefsDirty = 1;
		}
	}
}

void KineticExperiment::mixstepSetDilution( int mixstep, double dilution ) {
	mixsteps[ mixstep ].dilution = dilution;
	for( int i=0; i<system->experiments.count; i++ ) {
		if( system->experiments[i]->slaveToExperimentId == id ) {
			system->experiments[i]->mixsteps[ mixstep ].dilution = dilution;
		}
	}
}

void KineticExperiment::copy( KineticExperiment &copyFrom, int copyMeasured, int copyFitContext ) {
	int i;

	clear( !copyMeasured );

	system = copyFrom.system;
	id = copyFrom.id;
	mixstepCount = copyFrom.mixstepCount;
	mixstepDilutionTime = copyFrom.mixstepDilutionTime;
	mixstepDomain = copyFrom.mixstepDomain;
	mixstepNextUniqueId = copyFrom.mixstepNextUniqueId;

	slaveToExperimentId = copyFrom.slaveToExperimentId;
	reagentIndexForSeries = copyFrom.reagentIndexForSeries;
	mixstepIndexForSeries = copyFrom.mixstepIndexForSeries;

	for( i=0; i<MAX_MIX_STEPS; i++ ) {
		mixsteps[i].id = copyFrom.mixsteps[i].id;
		mixsteps[i].duration = copyFrom.mixsteps[i].duration;
		mixsteps[i].dilution = copyFrom.mixsteps[i].dilution;
		mixsteps[i].traceOCForMixstep.copy( copyFrom.mixsteps[i].traceOCForMixstep );
		// NOTE: we will deal with the mixtep dataRefs below when dealing
		// with measured data.
	}

	equilibriumExperiment = copyFrom.equilibriumExperiment;
	equilibriumVariance.copy( copyFrom.equilibriumVariance );
	traceC.copy( copyFrom.traceC );
	traceG.clear();
	for( i=0; i<traceG.count; i++ ) {
		KineticTrace *t = new KineticTrace;
		t->copy( *copyFrom.traceG[i] );
		traceG.add( t );
	}
	TRACEOC_LOCK( this );
	traceOC.copy( copyFrom.traceOC );
	TRACEOC_UNLOCK( this );
//	traceOG.copy( copyFrom.traceOG );
	observableInstructions.copyStrings( copyFrom.observableInstructions );
	observableError.copy( copyFrom.observableError );
	observableResidualFFTMat.copy( copyFrom.observableResidualFFTMat );

	if( copyMeasured ) {
		ownsMeasured = copyFrom.ownsMeasured;
		measured.setCount( copyFrom.measured.count );
		for( i=0; i<copyFrom.measured.count; i++ ) {
			if( copyFrom.measured[i] ) {
				KineticTrace *copyTrace;
				if( ownsMeasured ) {
					copyTrace = new KineticTrace;
					copyTrace->copy( *copyFrom.measured[i] );
				}
				else {
					copyTrace = copyFrom.measured[i];
				}
				measured.set( i, copyTrace );
			}
		}
		for( i=0; i<MAX_MIX_STEPS; i++ ) {
			mixsteps[i].dataRefs.copy( copyFrom.mixsteps[i].dataRefs ); 
		}
		mixstepDataRefsDirty = copyFrom.mixstepDataRefsDirty;
	}

	#ifdef USE_KINFIT_V2
	if( copyFitContext ) {
		//copyFrom.fdc.clearEvalTraces();
			// commented out 11/12/2010 for jacSystem work; trying to get away from evalTraces in fitting code
		fdc.clearEvalTraces();

		//copyFrom.fdc.aFitToSimulatedObsTimeTrace.clear();
		//fdc.aFitToSimulatedObsTimeTrace.clear();
			// ensure pointer data inside ZMat not copied.  This stores time  trace
			// to support rendering fit traces for aFit to Simulated observables while
			// the model is being twiddled, and should never need to be copied, or
			// be important to keep in any context the exp is being copied.
		memcpy( &fdc, &copyFrom.fdc, sizeof( fitDataContext ) );
		fd.copy( copyFrom.fd );
	}
	#endif

	viewInfo.clear();
	viewInfo.copyFrom( copyFrom.viewInfo );
	obsConstsByName.clear();
	obsConstsByName.copyFrom( copyFrom.obsConstsByName );
}

void KineticExperiment::updateFromMaster( KineticExperiment &master) {
	// Special copy for the case of slaved experiments in which we know we
	// don't want to copy things like uniqueID etc...

	int i;

	mixstepCount		= master.mixstepCount;
	mixstepDilutionTime = master.mixstepDilutionTime;
	mixstepDomain		= master.mixstepDomain;

	for( i=0; i<MAX_MIX_STEPS; i++ ) {
		mixsteps[i].id = master.mixsteps[i].id;
		mixsteps[i].duration = master.mixsteps[i].duration;
		mixsteps[i].dilution = master.mixsteps[i].dilution;
	}

	equilibriumExperiment = master.equilibriumExperiment;

	int hasScale  = master.viewInfo.getI( "fitSeriesWithScaling" );
	int hasOffset = master.viewInfo.getI( "fitSeriesWithOffset" );
	if(  hasScale || hasOffset ) {
		if( !copyObservableInstructionsPreservingSeriesFactors( master.observableInstructions, hasScale, hasOffset ) ) {
			observableInstructions.copyStrings( master.observableInstructions );
			master.viewInfo.putI( "fitSeriesWithScaling", 0 );
			master.viewInfo.putI( "fitSeriesWithOffset", 0 );
				// the user has edited the automatic scale vars, so remove the
				// flag to use these, and subsequent series exps will also just
				// get a copy of the master exp per normal
		}
	}
	else {
		observableInstructions.copyStrings( master.observableInstructions );
	}

	char name[64];
	int nameLen = 64;
	viewInfo.getS( "name", "", name, &nameLen );
	viewInfo.clear();
	viewInfo.copyFrom( master.viewInfo );
	viewInfo.putS( "name", name );

	compileOC();
}

// @TODO: this functionality should get moved to KintekExperiment
int addSeriesOffsetToExpression( ZTmpStr &expr, char *suffix ) {
//	assert( ! removeSeriesOffsetFromExpression( expr ) && "attempt add double offset to expr!" );
	ZTmpStr withOffset( "%s + offset_%s", expr.s, suffix );
	expr.set( withOffset.s );
	return 1;
}

int removeSeriesOffsetFromExpression( ZTmpStr &expr ) {
	ZRegExp removeOffset( "^(.*\\S)\\s*\\+\\s*offset_\\d+.\\s*$" );
	if( removeOffset.test( expr.s ) ) {
		expr.set( removeOffset.get( 1 ) );
		return 1;
	}
	return 0;
}

int addSeriesScaleToExpression( ZTmpStr &expr, char *suffix ) {
	//
	// To scale we need to remove any offset, apply scale, and reapply offset
	//
	int hasOffset = removeSeriesOffsetFromExpression( expr );
	ZTmpStr withScale( "scale_%s*(%s)", suffix, expr.s );
	if( hasOffset ) {
		withScale.set( "%s+offset_%s", withScale.s, suffix );
	}
	expr.set( withScale.s );
	return 1;
}

int removeSeriesScaleFromExpression( ZTmpStr &expr ) {
	ZRegExp removeScale( "^\\s*scale_\\d+.\\s*\\*\\s*\\(\\s*(.+)\\s*\\)\\s*(\\+\\s*offset_\\d+.)*\\s*$" );
	if( removeScale.test( expr ) ) {
		char *mainExpr = removeScale.get( 1 );
		char *offset   = removeScale.get( 2 );
		expr.set( "%s%s", mainExpr, offset ? offset : "" );
		return 1;
	}
	return 0;
}

int KineticExperiment::copyObservableInstructionsPreservingSeriesFactors( ZTLPVec<char> & obsInstructions, int hasScale, int hasOffset ) {
	// this is crazy.  series may have individual scaling/offset factors for each experiment, so
	// we must carefully extract the expression without the scaling factor from the given instructions,
	// and apply our own scaling factor (if any) to the expression.  Return 1 if we have sucessfully
	// removed/added factors where it should have been possible, or 0 otherwise.

	observableInstructions.clear();
	char buffer[256];
	int buflen=256;
	char *expName = viewInfo.getS( "name", 0, buffer, &buflen );
	if( strlen( expName ) < 13 ) {
		return 0;
	}
		// we utilitze the suffix of the experiment name for scale/offset factors,
		// and rely on those names being formed as "Experiment 1a" etc...

	int i;
	for( i=0; i<obsInstructions.count; i++ ) {
		ZTmpStr master = obsInstructions.get( i );
		int scaleSuccess  = !hasScale  || removeSeriesScaleFromExpression( master );
		int offsetSuccess = !hasOffset || removeSeriesOffsetFromExpression( master );

		if( !scaleSuccess || !offsetSuccess ) {
			return 0;
		}

		
		char *suffix = viewInfo.getS( "name", 0, buffer, &buflen ) + strlen( "Experiment " );
		if( hasScale ) {
			addSeriesScaleToExpression( master, suffix );
		}
		if( hasOffset ) {
			addSeriesOffsetToExpression( master, suffix );
		}
		observableInstructions.add( strdup( master.s ) );
	}
	return 1;
}

const int KineticExperiment_Version = 20100504;
int KineticExperiment::loadBinary( FILE *f, int byteswap ) {

	KineticSystem *s = system;
	clear();
	system = s;

	int version,i,j;
	freadEndian( &version, sizeof(version), 1, f, byteswap );

	switch( version ) {
		case 20090310:
				// no format change here: only to identify the save date to determine if
				// we need to fix the mixstepNextUniqueId: see below.
		case 20081016: {
			// general
			freadEndian( &id, sizeof(id), 1, f, byteswap );
			freadEndian( &slaveToExperimentId, sizeof(slaveToExperimentId), 1, f, byteswap );
			freadEndian( &equilibriumExperiment, sizeof(equilibriumExperiment), 1, f, byteswap );
			freadEndian( &reagentIndexForSeries, sizeof(reagentIndexForSeries), 1, f, byteswap );
			freadEndian( &mixstepIndexForSeries, sizeof(mixstepIndexForSeries), 1, f, byteswap );

			// mixsteps
			freadEndian( &mixstepNextUniqueId, sizeof(mixstepNextUniqueId), 1, f, byteswap );
			freadEndian( &mixstepDilutionTime, sizeof(mixstepDilutionTime), 1, f, byteswap );
			freadEndian( &mixstepCount, sizeof(mixstepCount), 1, f, byteswap );
			freadEndian( &mixstepDomain, sizeof(mixstepDomain), 1, f, byteswap );
			for( i=0; i<mixstepCount; i++ ) {
				freadEndian( &mixsteps[i].id, sizeof(mixsteps[i].id), 1, f, byteswap );
				freadEndian( &mixsteps[i].duration, sizeof(mixsteps[i].duration), 1, f, byteswap );
				freadEndian( &mixsteps[i].dilution, sizeof(mixsteps[i].dilution), 1, f, byteswap );
			}
			if( version == 20081016 ) {
				// correct a potential problem with mixstep ids
				int idUsed[128]; memset( idUsed, 0, sizeof(idUsed) );
				int maxId = 0;
				for( int m=0; m<mixstepCount; m++ ) {
					if( idUsed[ mixsteps[m].id ] ) {
						for( int n=1; n<128; n++ ) {
							if( idUsed[n] == 0 ) {
								mixsteps[m].id = n;
								break;
							}
						}
					}
					idUsed[ mixsteps[m].id ] = 1;
					maxId = max( maxId, mixsteps[m].id );
				}
				mixstepNextUniqueId = maxId + 1;
			}

			// observables
			int count;
			freadEndian( &count, sizeof(count), 1, f, byteswap );
			for( i=0; i<count; i++ ) {
				int len;
				freadEndian( &len, sizeof(len), 1, f, byteswap );
				char *expr = (char*)alloca( len );
				fread( expr, len, 1, f );
				/////////////////////////////////////////// new oct 3 2009
				char *p = expr;
				while( *p ) {
					if( *p < 32 ) *p = 32;
						// get rid of any newlines etc...
					p++;
				}
				//////////////////////////////////////////// end new
				observableInstructions.add( strdup( expr ) );
			}

			// measured
			freadEndian( &count, sizeof(count), 1, f, byteswap );
			measured.setCount( count );
			for( i=0; i<count; i++ ) {
				int mcount;
				freadEndian( &mcount, sizeof(mcount), 1, f, byteswap );
				if( mcount ) {
					measured.set( i, new KineticTrace() );
					measured[ i ]->loadBinary( f, byteswap );
				}
			}

			// viewInfo
			zHashTableLoad( f, viewInfo );
		}
		break;

		case 20090629: {
			// general
			zHashTableLoad( f, viewInfo );
			freadEndian( &id, sizeof(id), 1, f, byteswap );
			freadEndian( &slaveToExperimentId, sizeof(slaveToExperimentId), 1, f, byteswap );
			freadEndian( &equilibriumExperiment, sizeof(equilibriumExperiment), 1, f, byteswap );
			freadEndian( &reagentIndexForSeries, sizeof(reagentIndexForSeries), 1, f, byteswap );
			freadEndian( &mixstepIndexForSeries, sizeof(mixstepIndexForSeries), 1, f, byteswap );
			
			// mixsteps
			freadEndian( &mixstepNextUniqueId, sizeof(mixstepNextUniqueId), 1, f, byteswap );
			freadEndian( &mixstepDilutionTime, sizeof(mixstepDilutionTime), 1, f, byteswap );
			freadEndian( &mixstepCount, sizeof(mixstepCount), 1, f, byteswap );
			freadEndian( &mixstepDomain, sizeof(mixstepDomain), 1, f, byteswap );
			for( i=0; i<mixstepCount; i++ ) {
				freadEndian( &mixsteps[i].id, sizeof(mixsteps[i].id), 1, f, byteswap );
				freadEndian( &mixsteps[i].duration, sizeof(mixsteps[i].duration), 1, f, byteswap );
				freadEndian( &mixsteps[i].dilution, sizeof(mixsteps[i].dilution), 1, f, byteswap );
			}

			// observables
			int count;
			freadEndian( &count, sizeof(count), 1, f, byteswap );
			for( i=0; i<count; i++ ) {
				int len;
				freadEndian( &len, sizeof(len), 1, f, byteswap );
				char *expr = (char*)alloca( len );
				fread( expr, len, 1, f );
				/////////////////////////////////////////// new oct 3 2009
				char *p = expr;
				while( *p ) {
					if( *p < 32 ) *p = 32;
						// get rid of any newlines etc...
					p++;
				}
				//////////////////////////////////////////// end new
				observableInstructions.add( strdup( expr ) );
			}

			// measured
			freadEndian( &ownsMeasured, sizeof(ownsMeasured), 1, f, byteswap );
			if( ownsMeasured ) {
				freadEndian( &count, sizeof(count), 1, f, byteswap );
				measured.setCount( count );
				for( i=0; i<count; i++ ) {
					int mcount;
					freadEndian( &mcount, sizeof(mcount), 1, f, byteswap );
					if( mcount ) {
						measured.set( i, new KineticTrace() );
						measured[ i ]->loadBinary( f, byteswap );
					}
				}
			}
		}
		break;

		case 20091208: {
			// general
			zHashTableLoad( f, viewInfo );
			freadEndian( &id, sizeof(id), 1, f, byteswap );
			freadEndian( &slaveToExperimentId, sizeof(slaveToExperimentId), 1, f, byteswap );
			freadEndian( &equilibriumExperiment, sizeof(equilibriumExperiment), 1, f, byteswap );
			freadEndian( &reagentIndexForSeries, sizeof(reagentIndexForSeries), 1, f, byteswap );
			freadEndian( &mixstepIndexForSeries, sizeof(mixstepIndexForSeries), 1, f, byteswap );
			
			// mixsteps
			freadEndian( &mixstepNextUniqueId, sizeof(mixstepNextUniqueId), 1, f, byteswap );
			freadEndian( &mixstepDilutionTime, sizeof(mixstepDilutionTime), 1, f, byteswap );
			freadEndian( &mixstepCount, sizeof(mixstepCount), 1, f, byteswap );
			freadEndian( &mixstepDomain, sizeof(mixstepDomain), 1, f, byteswap );
			for( i=0; i<mixstepCount; i++ ) {
				freadEndian( &mixsteps[i].id, sizeof(mixsteps[i].id), 1, f, byteswap );
				freadEndian( &mixsteps[i].duration, sizeof(mixsteps[i].duration), 1, f, byteswap );
				freadEndian( &mixsteps[i].dilution, sizeof(mixsteps[i].dilution), 1, f, byteswap );
				// new dec 08
				int dataRefsCount;
				freadEndian( &dataRefsCount, sizeof(dataRefsCount), 1, f, byteswap );
				mixsteps[i].dataRefs.setCount( dataRefsCount );
				for( j=0; j<dataRefsCount; j++ ) {
					int flag;
					freadEndian( &flag, sizeof(flag), 1, f, byteswap );
					if( flag ) {
						mixsteps[i].dataRefs.set( j, new KineticExperiment::Mixstep::MixstepDataRef() );
						freadEndian( &mixsteps[i].dataRefs[j]->dataId, sizeof(mixsteps[i].dataRefs[j]->dataId), 1, f, byteswap );
						freadEndian( &mixsteps[i].dataRefs[j]->offset, sizeof(mixsteps[i].dataRefs[j]->offset), 1, f, byteswap );
						freadEndian( &mixsteps[i].dataRefs[j]->scale, sizeof(mixsteps[i].dataRefs[j]->scale), 1, f, byteswap );
					}
				}
			}

			// observables
			int count;
			freadEndian( &count, sizeof(count), 1, f, byteswap );
			for( i=0; i<count; i++ ) {
				int len;
				freadEndian( &len, sizeof(len), 1, f, byteswap );
				char *expr = (char*)alloca( len );
				fread( expr, len, 1, f );
				/////////////////////////////////////////// new oct 3 2009
				char *p = expr;
				while( *p ) {
					if( *p < 32 ) *p = 32;
						// get rid of any newlines etc...
					p++;
				}
				//////////////////////////////////////////// end new
				observableInstructions.add( strdup( expr ) );
			}

			// measured
			freadEndian( &ownsMeasured, sizeof(ownsMeasured), 1, f, byteswap );
			if( ownsMeasured ) {
				freadEndian( &count, sizeof(count), 1, f, byteswap );
				measured.setCount( count );
				for( i=0; i<count; i++ ) {
					int mcount;
					freadEndian( &mcount, sizeof(mcount), 1, f, byteswap );
					if( mcount ) {
						measured.set( i, new KineticTrace() );
						measured[ i ]->loadBinary( f, byteswap );
					}
				}
			}
		}
		break;

		case 20100504:
		case 20101112: {
			// note: 20101112 was a temporary format, backed out.  See end of this block for the 
			// data it stored which is no longer stored.

			// general
			zHashTableLoad( f, viewInfo );
			freadEndian( &id, sizeof(id), 1, f, byteswap );
			freadEndian( &slaveToExperimentId, sizeof(slaveToExperimentId), 1, f, byteswap );
			freadEndian( &equilibriumExperiment, sizeof(equilibriumExperiment), 1, f, byteswap );
			freadEndian( &reagentIndexForSeries, sizeof(reagentIndexForSeries), 1, f, byteswap );
			freadEndian( &mixstepIndexForSeries, sizeof(mixstepIndexForSeries), 1, f, byteswap );
			
			// mixsteps
			freadEndian( &mixstepNextUniqueId, sizeof(mixstepNextUniqueId), 1, f, byteswap );
			freadEndian( &mixstepDilutionTime, sizeof(mixstepDilutionTime), 1, f, byteswap );
			freadEndian( &mixstepCount, sizeof(mixstepCount), 1, f, byteswap );
			freadEndian( &mixstepDomain, sizeof(mixstepDomain), 1, f, byteswap );
			for( i=0; i<mixstepCount; i++ ) {
				freadEndian( &mixsteps[i].id, sizeof(mixsteps[i].id), 1, f, byteswap );
				freadEndian( &mixsteps[i].duration, sizeof(mixsteps[i].duration), 1, f, byteswap );
				freadEndian( &mixsteps[i].dilution, sizeof(mixsteps[i].dilution), 1, f, byteswap );
				// new dec 08
				int dataRefsCount;
				freadEndian( &dataRefsCount, sizeof(dataRefsCount), 1, f, byteswap );
				mixsteps[i].dataRefs.setCount( dataRefsCount );
				for( j=0; j<dataRefsCount; j++ ) {
					int flag;
					freadEndian( &flag, sizeof(flag), 1, f, byteswap );
					if( flag ) {
						mixsteps[i].dataRefs.set( j, new KineticExperiment::Mixstep::MixstepDataRef() );
						freadEndian( &mixsteps[i].dataRefs[j]->dataId, sizeof(mixsteps[i].dataRefs[j]->dataId), 1, f, byteswap );
						freadEndian( &mixsteps[i].dataRefs[j]->offset, sizeof(mixsteps[i].dataRefs[j]->offset), 1, f, byteswap );
						freadEndian( &mixsteps[i].dataRefs[j]->scale, sizeof(mixsteps[i].dataRefs[j]->scale), 1, f, byteswap );
						freadEndian( &mixsteps[i].dataRefs[j]->translate, sizeof(mixsteps[i].dataRefs[j]->translate), 1, f, byteswap );
					}
				}
			}

			// observables
			int count;
			freadEndian( &count, sizeof(count), 1, f, byteswap );
			for( i=0; i<count; i++ ) {
				int len;
				freadEndian( &len, sizeof(len), 1, f, byteswap );
				char *expr = (char*)alloca( len );
				fread( expr, len, 1, f );
				/////////////////////////////////////////// new oct 3 2009
				char *p = expr;
				while( *p ) {
					if( *p < 32 ) *p = 32;
						// get rid of any newlines etc...
					p++;
				}
				//////////////////////////////////////////// end new
				observableInstructions.add( strdup( expr ) );
			}

			// measured
			freadEndian( &ownsMeasured, sizeof(ownsMeasured), 1, f, byteswap );
			if( ownsMeasured ) {
				freadEndian( &count, sizeof(count), 1, f, byteswap );
				measured.setCount( count );
				for( i=0; i<count; i++ ) {
					int mcount;
					freadEndian( &mcount, sizeof(mcount), 1, f, byteswap );
					if( mcount ) {
						measured.set( i, new KineticTrace() );
						measured[ i ]->loadBinary( f, byteswap );
					}
				}
			}
			
			if( version == 20101112 ) {
				// measuredTransform: a temporary (unreleased) dev feature that was backed out and not used.
				int hasMeasuredTransform;
				freadEndian( &hasMeasuredTransform, sizeof(hasMeasuredTransform), 1, f, byteswap );
				if( hasMeasuredTransform ) {
					ZMat unused;
					unused.loadBin( f );
				}
			}
		}
		break;


		default:
			return 0;
				// unrecognized version!
	}
	return 1;
}

int KineticExperiment::saveBinary( FILE *f ) {

	int i,j;

	fwrite( &KineticExperiment_Version, sizeof(KineticExperiment_Version), 1, f );

	// general
	zHashTableSave( viewInfo, f );
	fwrite( &id, sizeof(id), 1, f );
	fwrite( &slaveToExperimentId, sizeof(slaveToExperimentId), 1, f );
	fwrite( &equilibriumExperiment, sizeof(equilibriumExperiment), 1, f );
	fwrite( &reagentIndexForSeries, sizeof(reagentIndexForSeries), 1, f );
	fwrite( &mixstepIndexForSeries, sizeof(mixstepIndexForSeries), 1, f );

	// mixsteps
	fwrite( &mixstepNextUniqueId, sizeof(mixstepNextUniqueId), 1, f );
	fwrite( &mixstepDilutionTime, sizeof(mixstepDilutionTime), 1, f );
	fwrite( &mixstepCount, sizeof(mixstepCount), 1, f );
	fwrite( &mixstepDomain, sizeof(mixstepDomain), 1, f );
	for( i=0; i<mixstepCount; i++ ) {
		fwrite( &mixsteps[i].id, sizeof(mixsteps[i].id), 1, f );
		fwrite( &mixsteps[i].duration, sizeof(mixsteps[i].duration), 1, f );
		fwrite( &mixsteps[i].dilution, sizeof(mixsteps[i].dilution), 1, f );
		// new dec 08
		fwrite( &mixsteps[i].dataRefs.count, sizeof(mixsteps[i].dataRefs.count), 1, f );
		for( j=0; j<mixsteps[i].dataRefs.count; j++ ) {
			int flag = mixsteps[i].dataRefs[j] ? 1 : 0;
			fwrite( &flag, sizeof(flag), 1, f );
			if( flag ) {
				fwrite( &mixsteps[i].dataRefs[j]->dataId, sizeof(mixsteps[i].dataRefs[j]->dataId), 1, f );
				fwrite( &mixsteps[i].dataRefs[j]->offset, sizeof(mixsteps[i].dataRefs[j]->offset), 1, f );
				fwrite( &mixsteps[i].dataRefs[j]->scale, sizeof(mixsteps[i].dataRefs[j]->scale), 1, f );
				fwrite( &mixsteps[i].dataRefs[j]->translate, sizeof(mixsteps[i].dataRefs[j]->translate), 1, f );
			}
		}
	}

	// observables
	assert( observableInstructionsValid() );
	if( !observableInstructionsValid() ) {
		return 0;
	}
	fwrite( &observableInstructions.count, sizeof(observableInstructions.count), 1, f );
	for( i=0; i<observableInstructions.count; i++ ) {
		assert( observableInstructions[i] );
		int len = strlen( observableInstructions[i] ) + 1;  // +1: write terminating 0 to disk
		fwrite( &len, sizeof(len), 1, f );
		fwrite( observableInstructions[i], len, 1, f );
	}

	// measured
	fwrite( &ownsMeasured, sizeof(ownsMeasured), 1, f );
	if( ownsMeasured ) {
			// subclasses may not own the data, and their load fns will fixup refs
		fwrite( &measured.count, sizeof(measured.count), 1, f );
		for( i=0; i<measured.count; i++ ) {
			int mcount = measuredCount( i );
			fwrite( &mcount, sizeof(mcount), 1, f );
				// this is in case some measured traces are NULL
			if( mcount ) {
				measured[ i ]->saveBinary( f );
			}
		}
	}
	return 1;
}

void KineticExperiment::dump() {
	char buffer[256];
	int buflen=256;
	trace( "KineticExperiment: %X, id=%d, %s\n", this, id, viewInfo.getS( "name", "(unnamed)", buffer, &buflen ) );
	trace( "\tid: %d\n", id );
	trace( "\tslaveToExperimentId: %d\n", slaveToExperimentId );
	trace( "\treagentIndexForSeries, mixstepIndexForSeries = %d, %d\n", reagentIndexForSeries, mixstepIndexForSeries );
	trace( "\tequilibriumExperiment: %d\n", equilibriumExperiment );
	trace( "\tKintek isEquilibrium: %d\n", viewInfo.getI( "isEquilibrium" ) );
	trace( "\tKintek isPulseChase: %d\n", viewInfo.getI( "isPulseChase" ) );

	trace( "\tmixstepDilutionTime: %g\n", mixstepDilutionTime );
	trace( "\tmixstepDomain: %d\n", mixstepDomain );
	trace( "\tmixstepCount: %d\n", mixstepCount );
	int i;
	for( i=0; i<mixstepCount; i++ ) {
		trace( "\t\t%d: id(%d) dilution(%g) duration(%g)\n", i, mixsteps[i].id, mixsteps[i].dilution, mixsteps[i].duration );
	}
	trace( "\tObservable Instructions:\n" );
	for( i=0; i<observableInstructions.count; i++ ) {
		trace( "\t\t%s : error=%d\n", observableInstructions[i], observableError[i] );
	}
	trace( "\tMeasured: (count=%d)\n", measured.count );
	for( i=0; i<measured.count; i++ ) {
		trace( "\t\t%d: [KineticTrace %X] %d entries\n", i, measured[i], measuredCount( i ) );
	}
}

int KineticExperiment::observableInstructionsValid() {
	// ASSUMES observablesCompile() has been called to parse instructions.
	int count = observablesCount();
	assert( count == observableError.count );
	for( int j=0; j<count; j++ ) {
		if ( observableError[j] ) {
			return 0;
		}
	}
	return 1;
}

double KineticExperiment::getTraceOCSLERP( double time, int row, int ignoreMixsteps ) {
	if( mixstepCount == 1 || ignoreMixsteps ) {
		return traceOC.getElemSLERP( time, row );
	}
	double accum = 0.0;
	for( int i=0; i<mixstepCount; i++ ) {
		accum += mixsteps[i].duration;
		if( time < accum || i==mixstepCount-1 ) {
			return mixsteps[i].traceOCForMixstep.getElemSLERP( time - accum + mixsteps[i].duration, row );
		}
	}
	return -1.0;
}

double KineticExperiment::getTraceOCExtraSLERP( int index, double time, int row ) {
	if( mixstepCount == 1 ) {
		return traceOCExtra[index]->getElemSLERP( time, row );
	}
	double accum = 0.0;
	for( int i=0; i<mixstepCount; i++ ) {
		accum += mixsteps[i].duration;
		if( time < accum || i==mixstepCount-1 ) {
			return mixsteps[i].traceOCExtraForMixstep[index]->getElemSLERP( time - accum + mixsteps[i].duration, row );
		}
	}
	return -1.0;
}

void KineticExperiment::compileOC() {
	observableError.setCount( observableInstructions.count );;
	if( vmoc ) {
		delete vmoc;
	}
	vmoc = new KineticVMCodeOC( this, system->getPLength() - system->reagentCount() );
	// CALL the compile function explicitly becuase apparently 
	// you can't be polymorphic from inside a constructor
	// TFF jan 2012: subtract reagentCount() for the pCount passed in; pCount 
	// would not appear to include the reagent concentrations, which are referred
	// to inside of VMD as "RC"
	vmoc->compile();

	#ifdef VM_DISASSEMBLE
		vmoc->disassemble();
	#endif

	if( vmod ) {
		delete vmod;
	}
	vmod = new KineticVMCodeOD( this, system->getPLength() - system->reagentCount() );
		// tfb jan 2012: subtract reagentCount because vmd does not include these
		// in pCount; see definition of PC and RC within VMD

	// CALL the compile function explicitly becuase apparently 
	// you can't be polymorphic from inside a constructor
	vmod->compile();
	#ifdef VM_DISASSEMBLE
		vmod->disassemble();
	#endif
}

void KineticExperiment::compileOG() {
	if( vmog ) {
		delete vmog;
	}

	vmog = new KineticVMCodeOG( this, system->getPLength() );

	// CALL the compile function explicitly becuase apparently 
	// you can't be polymorphic from inside a constructor
	vmog->compile();
	#ifdef VM_DISASSEMBLE
		vmog->disassemble( "og" );
	#endif
}

void KineticExperiment::computeOC( double *P ) {
	if( !equilibriumExperiment && observablesCount() ) {

		traceOC.init( observablesCount(), traceC.cols );

		double *OC = (double *)alloca( observablesCount() * sizeof(double) );
		double *OD = (double *)alloca( observablesCount() * sizeof(double) );
		
		// FOREACH data point in C
		for( int i=0; i<traceC.cols; i++ ) {
			double *C = traceC.getColPtr( i );
			vmoc->execute_OC( C, P, OC );

			double *D = traceC.getDerivColPtr( i );
			vmod->execute_OD( C, P, D, OD );

			traceOC.add( traceC.getTime(i), OC, OD );
		}
	}
}

void KineticExperiment::computeOCMixsteps( double *P, int stride ) {
	if( !equilibriumExperiment && observablesCount() ) {

		traceOC.init( observablesCount(), traceC.cols );

		double *OC = (double *)alloca( observablesCount() * sizeof(double) );
		double *OD = (double *)alloca( observablesCount() * sizeof(double) );
		
		// To compute observable expressions which make use of symbolic names
		// like VOLT, TEMP, or PRES (which means the voltage, temperature, or
		// pressure at the current time) then we need to either used fixed
		// values per mixstep and subsitute those into P (or use distinct P
		// per mixstep), or we need to pass some time/column information
		// to execute_OC such that the voltage etc may be looked up based
		// on this time.  The latter sounds much slower.
		
		double startTime, endTime, dilution;
		int currentMixstep = 0;
		mixstepInfo( currentMixstep, startTime, endTime, dilution );

		// FOREACH data point in C
		for( int i=0; i<traceC.cols; i++ ) {
			double *C = traceC.getColPtr( i );
			double time = traceC.getTime( i );
			
			if( time > endTime && currentMixstep < mixstepCount-1 ) {
				currentMixstep++;
				mixstepInfo( currentMixstep, startTime, endTime, dilution );
				P += stride;
			}

			vmoc->execute_OC( C, P, OC );

			double *D = traceC.getDerivColPtr( i );
			vmod->execute_OD( C, P, D, OD );

			traceOC.add( time, OC, OD );
		}
	}
}


void KineticExperiment::observablesGetMinMax( double &minVal, double &maxVal, int bIncludeMeasured, int bVisibleOnly, int bIncludeDeriv, int minObs, int maxObs ) {
	minVal = +1e300;
	maxVal = -1e300;
	int obsCount = observablesCount();
	if( maxObs < 0 || maxObs >= obsCount ) {
		maxObs = obsCount - 1;
	}

	// Data examined will be limited to appropriate time domain
	double timeBegin = 0.0;
	double timeEnd   = simulationDuration();
	if( mixstepDomain >= 0 ) {
		double unused;
		mixstepInfo( mixstepDomain, timeBegin, timeEnd, unused );
		if( mixstepDomain+1 < mixstepCount ) {
			timeEnd -= mixstepDilutionTime;
				// if subsequent mixsteps exist, don't include endpoint
		}
	}
	if( viewInfo.getI( "isEquilibrium" ) ) {
		timeEnd = traceOC.getLastTime();
			// because the traceOC.time array contains *concentrations* which we plot against...
	}

	ZTLVec< KineticExperiment* > series;
		// this fn returns the min max across the series
	
	// mask traces/data that are not being plotted in the experiment if desired.
	int *excludeObservable=0;
	if( bVisibleOnly ) {
		excludeObservable = (int*)alloca( obsCount * sizeof(int) );
		memset( excludeObservable, 0, obsCount * sizeof( int ) );
		for( int obs=minObs; obs<=maxObs; obs++ ) {
			if( ! viewInfo.getI( ZTmpStr( "obs%d-plot",obs ) ) ) {
				excludeObservable[ obs ] = 1;
			}
		}
	}
	
	int count = getSeries( series );
	for( int i=0; i<count; i++ ) {
		KineticExperiment *e = series[i];
		double eMax,eMin;
		for( int obs=minObs; obs<=maxObs; obs++ ) {
			if( !excludeObservable ||  !excludeObservable[ obs ] ) {
				if( e->traceOC.getBoundsForRange( obs, eMin, eMax, timeBegin, timeEnd ) ) {
					minVal = min( minVal, eMin );
					maxVal = max( maxVal, eMax );
				}
				if( bIncludeDeriv ) {
					if( e->traceOC.getBoundsForRange( obs, eMin, eMax, timeBegin, timeEnd, 1, 1 ) ) {
						minVal = min( minVal, eMin );
						maxVal = max( maxVal, eMax );
					}
				}
			}
		}
		if( bIncludeMeasured ) {
			for( int m=minObs; m<=maxObs && m<e->measured.count; m++ ) {
				if( e->measured[ m ] && ( !excludeObservable || !excludeObservable[m] ) ) {
					if( e->measured[ m ]->getBoundsForRange( -1, eMin, eMax, timeBegin, timeEnd ) ) {
						minVal = min( minVal, eMin );
						maxVal = max( maxVal, eMax );
					}
				}
			}
		}
	}

	if( minVal > +1e299 ) minVal = 0;
	if( maxVal < -1e299 ) maxVal = 0;
}

void KineticExperiment::computeSSEPerObservable( normalizeType _normalize ) {
	TRACEOC_LOCK( this );
	ssePerObservable.setCount( traceOC.rows );
	normalizeTypeUsedForSSE.setCount( traceOC.rows );

	traceOCResiduals.clear();
	for( int i=0; i<traceOC.rows; i++ ) {
		KineticTrace *res = 0;
		int count = measuredCount( i );
		if( count ) {
			res = new KineticTrace( 1, count );
		}
		traceOCResiduals.add( res );
	}

	double normVal = 0.0;
	normalizeType normalize;
	for( int i=0; i<traceOC.rows; i++ ) {
		ssePerObservable.set( i, 0.0 );
		if( !measuredCount( i ) ) {
			normalizeTypeUsedForSSE.set( i, NT_Invalid );
			continue;
		}
		//
		// First determine how to normalize.  The user may have forced us by setting _normalize, otherwise
		// we will take either a specified type for this data, or what we deem to be the best available.
		//
		if( _normalize < NT_NumTypes ) {
			normalize = _normalize;
		}
		else {
			int ntype = measured[i]->properties.getI( "sigmaNormalizeTypeForFit", -1 );
			if( ntype == -1 ) {
				normalize = measured[i]->getSigmaBestNormalizeTypeAvailable();
			}
			else {
				normalize = (normalizeType) ntype;
			}
		}
		normalizeTypeUsedForSSE.set( i, normalize );
		//
		// Now get the value given the type of normalization
		//
		if( normalize != NT_None ) {
			if( normalize == NT_MaxData ) {
				double minData;
				measured[i]->getBoundsForRow( 0, minData, normVal, 1 );
			}
			else if( normalize == NT_SigmaAvg ) {
				if( measured[i]->sigma ) {
					normVal = measured[i]->getSigmaAvg( 0, 0 );
						// note in measured data, there is always only a single row of data (row 0)
				}
				else {
					normVal = 1.0;
					normalizeTypeUsedForSSE.set( i, NT_None );
				}
			}
			else if( normalize == NT_SigmaAvgExternal ) {
				normVal = measured[i]->properties.getD( "sigmaAvgExternal", -1.0 );
				if( normVal <= 0) {
					normVal = 1.0;
					normalizeTypeUsedForSSE.set( i, NT_None );
				}
			}
			else if( normalize == NT_SigmaAvgAFit ) {
				normVal = measured[i]->properties.getD( "sigmaAvgAFit", -1.0 );
				if( normVal <= 0) {
					normVal = 1.0;
					normalizeTypeUsedForSSE.set( i, NT_None );
				}
			}
			else if( normalize == NT_SigmaAvgAFitFamily ) {
				normVal = measured[i]->properties.getD( "sigmaAvgAFitFamily", -1.0 );
				if( normVal <= 0) {
					normVal = 1.0;
					normalizeTypeUsedForSSE.set( i, NT_None );
				}
			}
		}
		int mcount = measuredCount( i );
		double sum = 0.0;
		for( int j=0; j<mcount; j++ ) {
			double m = measured[i]->getData( j, 0 );
			double t = measured[i]->getTime( j );
			double s = getTraceOCSLERP( t, i );
			double err = ( m - s );
			if( normalize != NT_None ) {
				if( normalize == NT_Sigma ) {
					if( measured[i]->sigma ) {
						normVal = measured[i]->getSigma( j, 0, NT_Sigma );
					}
					else {
						normVal = 1.0;
						normalizeTypeUsedForSSE.set( i, NT_None );
					}
				}
				err /= normVal;
			}
			traceOCResiduals[ i ]->add( t, &err );
			sum += err * err;
		}
		ssePerObservable.set( i, sum );
	}
	TRACEOC_UNLOCK( this );
}

double KineticExperiment::getExpSSE() {
	double sum=0.0;
	for( int i=0; i<ssePerObservable.count; i++ ) {
		sum += ssePerObservable[i];
	}
	return sum;
}

double KineticExperiment::getSeriesSSE() {
	ZTLVec< KineticExperiment* > series;
	int count = getSeries( series );
	double sum=0.0;
	for( int i=0; i<count; i++ ) {
		sum += series[i]->getExpSSE();
	}
	return sum;
}

void KineticExperiment::copyRenderVariables() {
	int i;

	render_measured.clear();
	render_traceC.clear();
	render_traceG.clear();
	render_traceOC.clear();
//	render_traceOG.clear();
	render_traceDebug.clear();

	render_traceC.copy( traceC );
	render_traceOC.copy( traceOC );
	render_traceDebug.copy( traceDebug );
	for( i=0; i<measured.count; i++ ) {
		KineticTrace *t = new KineticTrace;
		t->copy( *measured[i] );
		render_measured.add( t );
	}
	for( i=0; i<traceG.count; i++ ) {
		KineticTrace *t = new KineticTrace;
		t->copy( *traceG[i] );
		render_traceG.add( t );
	}
//	for( i=0; i<traceOG.count; i++ ) {
//		KineticTrace *t = new KineticTrace;
//		t->copy( *traceOG[i] );
//		render_traceOG.add( t );
//	}
}


// KineticSystem
//------------------------------------------------------------------------------------------------------------------------------------

int KineticSystem::rateDependCoefTypes[4] = { PI_VOLTAGE_COEF, PI_TEMPERATURE_COEF, PI_PRESSURE_COEF, PI_SOLVENTCONC_COEF };
int KineticSystem::rateDependInitCondTypes[4] = { PI_VOLTAGE, PI_TEMPERATURE, PI_PRESSURE, PI_SOLVENTCONC };

KineticSystem::KineticSystem() {
	vmd = 0;
	vmh = 0;
	viewInfo.setThreadSafe( 1 );
	clear();
}

KineticSystem::~KineticSystem() {
	clear();
}

void KineticSystem::copy( KineticSystem &k, int copyExperiments /*=1*/, int copyData /*=1*/ ) {
	clear();
	reagents.copyStrings( k.reagents );
	reactions.copy( k.reactions );

	if( copyExperiments ) {
		for( int i=0; i<k.experiments.count; i++ ) {
			copyExperiment( k.experiments[i], 0, copyData );
				// by default the new experiments in the new system will have the same ids
				// as those in this system (ids are unique *within* systems)
		}
	}
	viewInfo.copyFrom( k.viewInfo );
	parameterInfo.copy( k.parameterInfo );
}

void KineticSystem::stealTracesToExtra( KineticSystem &from ) {
	// STEAL the passed traces into our 'extra' traces
	for( int e=0; e<experiments.count; e++ ) {
		KineticExperiment *exp = experiments[e];
		KineticExperiment *trialExp = from.experiments[e];

		KineticTrace *kt = new KineticTrace();
		kt->stealOwnershipFrom( trialExp->traceOC );
		exp->traceOCExtra.add( kt );

		if( exp->mixstepCount > 1 ) {
			for( int ms=0; ms<exp->mixstepCount; ms++ ) {
				KineticTrace *kt = new KineticTrace();
				kt->stealOwnershipFrom( trialExp->mixsteps[ms].traceOCForMixstep );
				exp->mixsteps[ms].traceOCExtraForMixstep.add( kt );
			}
		}
	}
}

void KineticSystem::clearExtraTraces() {
	// CLEAR all current 'extra' traces
	for( int e=0; e<experiments.count; e++ ) {
		experiments[e]->traceOCExtra.clear();
		for( int ms=0; ms<experiments[e]->mixstepCount; ms++ ) {
			experiments[e]->mixsteps[ms].traceOCExtraForMixstep.clear();
		}
	}
}

void KineticSystem::clear() {
	reagents.clear();
	reactions.clear();
	experiments.clear();
	experimentNextUniqueId = 0;
	parameterInfo.clear();
	viewInfo.clear();

	if( vmd ) {
		delete vmd;
		vmd = 0;
	}

	if( vmh ) {
		delete vmh;
		vmh = 0;
	}
}

const int KineticSystem_Version = 20090225;
int KineticSystem::loadBinary( FILE *f ) {
	// Check 32 vs 64 bit issues, endianness, etc.
	int byteswap, dataLongSize;
	if( !loadBinaryCompatData( f, byteswap, dataLongSize ) ) {
		return 0;
	}

	int version;
	freadEndian( &version, sizeof(version), 1, f, byteswap );
	

	int size;
	switch( version ) {
		case 20090225:
		case 20081016: {
			// Read viewInfo, this contains reactionText as entered by user; parse to 
			// reagents and reactions
			viewInfo.clear();
			zHashTableLoad( f, viewInfo );

			char *rText   = (char*)alloca( 1024 );
			int  rTextLen = 1024;
			viewInfo.getS( "reactionText", 0, rText, &rTextLen );
			if( rTextLen > 1024 ) {
				rText = (char*)alloca( rTextLen );
				viewInfo.getS( "reactionText", 0, rText, &rTextLen );
			}

			if( !rText ) return 0;
			if( !reactionsAddByFullText( rText ) ) {
				return 0;
			}

			// correct a problem of saved reaction coordinates in an earlier version:
			if( version == 20081016 ) {
				FVec2 pos;
				for( int i=reactions.count-2; i>=0; i-=2 ) {
					pos.x = viewInfo.getF( ZTmpStr("react-%d-x", i/2) );
					pos.y = viewInfo.getF( ZTmpStr("react-%d-y", i/2) );
					viewInfo.putF( ZTmpStr("react-%d-x", i), pos.x );
					viewInfo.putF( ZTmpStr("react-%d-y", i), pos.y );
					viewInfo.del( ZTmpStr("react-%d-x", i+1 ) );
					viewInfo.del( ZTmpStr("react-%d-y", i+1 ) );
				}
			}

			// Read experiments
			freadEndian( &experimentNextUniqueId, sizeof(experimentNextUniqueId), 1, f, byteswap );
			freadEndian( &size, sizeof(size), 1, f, byteswap );
			for( int i=0; i<size; i++ ) {
				KineticExperiment *e = newExperiment();
				if( !e->loadBinary( f, byteswap ) ) {
					return 0;
				}
			}
			experimentNextUniqueId -= size;
				// all of the KineticExperiment constructor calls above inc'd this; reset to what it should be.

			// Read KineticParamterInfo: the values in the hash are of type zhBIN, so they can't
			// get automatically byte-swapped like other hashtable entries.  Do it manually if necessary.
			ZHashTable savedParams;
			zHashTableLoad( f, savedParams );
			if( byteswap ) {
				for( ZHashWalk p( savedParams ); p.next(); ) {
					SavedKineticParameterInfo *skpi = (SavedKineticParameterInfo*)p.val;
					skpi->byteSwap();
				}
			}

			// Rename Voltage-related params from Vamplitude to Vo and k+1_Vcharge to z+1
			// Actually I just duplicate the entries using the new names so they'll be found
			// when allocParameterInfo is called with this hashtable.
            // TODO: This is probably already old enough to be removed since this feature is a DEV-only feature,
            // but we'll leave it in for a bit longer.  May2012.
			for( int i=0; i<reactionCount(); i++ ) {
				char *rateName = reactionGetRateName( i );
				SavedKineticParameterInfo *skpi = (SavedKineticParameterInfo*)savedParams.getS( ZTmpStr( "%s_Vamplitude", rateName ) );
				if( skpi ) {
					savedParams.putB( ZTmpStr( "%s_Vo", rateName ), (char*)skpi, sizeof( SavedKineticParameterInfo ) );
				}
				skpi = (SavedKineticParameterInfo*)savedParams.getS( ZTmpStr( "%s_Vcharge", rateName ) );
				if( skpi ) {
					rateName[0] = 'z';
					savedParams.putB( rateName, (char*)skpi, sizeof( SavedKineticParameterInfo ) );
				}
			}
            
            // may2012: rename voltage/temp-related coefs to be keyed by unique strings based on the reaction string
            // instead of the rate name, to be robust against edits to the mechanism.
            // TODO: remove this once some reasonable amount of time has passed.  This is a DEV-only feature at this point.
            for( int i=0; i<reactionCount(); i++ ) {
				char *rateName = reactionGetRateName( i );
                char *reactionString = reactionGetUniqueString( i );
                
                // Fixup temp-related
                //
                SavedKineticParameterInfo *skpi = (SavedKineticParameterInfo*)savedParams.getS( ZTmpStr( "%s_To", rateName ) );
				if( skpi ) {
					savedParams.putB( ZTmpStr( "%s_To", reactionString ), (char*)skpi, sizeof( SavedKineticParameterInfo ) );
				}
                skpi = (SavedKineticParameterInfo*)savedParams.getS( ZTmpStr( "%s_Ea", rateName ) );
				if( skpi ) {
					savedParams.putB( ZTmpStr( "%s_Ea", reactionString ), (char*)skpi, sizeof( SavedKineticParameterInfo ) );
				}
                
                // Fixup voltage-related (last because we modify rateName below)
                //
				skpi = (SavedKineticParameterInfo*)savedParams.getS( ZTmpStr( "%s_Vo", rateName ) );
				if( skpi ) {
					savedParams.putB( ZTmpStr( "%s_Vo", reactionString ), (char*)skpi, sizeof( SavedKineticParameterInfo ) );
				}
                rateName[0] = 'z';
				skpi = (SavedKineticParameterInfo*)savedParams.getS( rateName );
				if( skpi ) {
					savedParams.putB( ZTmpStr( "%s_z", reactionString ), (char*)skpi, sizeof( SavedKineticParameterInfo ) );
				}
			}

			// Setup the system parameters using saved values
			allocParameterInfo( &savedParams );
			compile();
		}
		break;

		default:
			return 0;
				// unrecognized version!
	}
	return 1;
}


int KineticSystem::saveBinary( FILE *f ) {
	// Write binary compat data (writes intsize, endianness etc)
	saveBinaryCompatData( f );

	fwrite( &KineticSystem_Version, sizeof( KineticSystem_Version ), 1, f );

	// Write viewInfo hash; this contains user-entered "reactionText" which
	// can be used to reconstruct reagent/reaction arrays.
	// and reactions arrays.
	zHashTableSave( viewInfo, f );

	// Write experiments
	fwrite( &experimentNextUniqueId, sizeof( experimentNextUniqueId ), 1, f );
	fwrite( &experiments.count, sizeof( experiments.count ), 1, f );
	for( int i=0; i<experiments.count; i++ ) {
		experiments[i]->saveBinary( f );
	}

	// Write KineticParamterInfo
	ZHashTable savedParams;
	saveParameterInfo( savedParams );
	zHashTableSave( savedParams, f );
	
	return 1;
}

void KineticSystem::dump() {
	trace( "===================================== KineticSystem::dump()\n" );
	trace( "KineticSystem [%X]\n", this );
	char buffer[2048];
	int buflen=2048;
	trace( "\tReaction Text: %s\n", viewInfo.getS( "reactionText", "(none)", buffer, &buflen ) );
	int i;
	for( i=0; i<reactions.count; i++ ) {
		trace( "Reaction %d: %s\n", i, reactionGetString( i ) );
	}
	for( i=0; i<parameterInfo.count; i++ ) {
		trace( "\n" );
		parameterInfo[i].dump( this );
	}
	for( i=0; i<experiments.count; i++ ) {
		trace( "\n" );
		experiments[i]->dump();
		if( experiments[i]->vmoc ) {
			experiments[i]->vmoc->disassemble( ZTmpStr( "exp%d", i ) );
		}
	}
}

void KineticSystem::reagentsSet( ZTLPVec<char>& newReagents ) {
	// Find out which reagents were deleted if any, and fixup the
	// experiments' reagentIndexForSeries if necessary.

	ZHashTable mapOldIndexToNewIndex;
	for( int i=0; i<reagents.count; i++ ) {
		for( int j=0; j<newReagents.count; j++ ) {
			if( !strcmp( reagents[i], newReagents[j] ) ) {
				// the previous index for reagent was i, and is now j
				mapOldIndexToNewIndex.bputI( &i, sizeof( i ), j );
				break;
			}
		}
	}
	for( int i=0; i<experiments.count; i++ ) {
		KineticExperiment *e = experiments[i];
		if( e->slaveToExperimentId==-1 && e->reagentIndexForSeries != -1 ) {
			int newIndex = mapOldIndexToNewIndex.bgetI( &(e->reagentIndexForSeries), sizeof(e->reagentIndexForSeries),  -1 );
			if( newIndex == -1 ) {
				e->system->delSeriesExperiments( e->getExperimentIndex() );
					// this sets the index to -1;
			}
			else if( e->reagentIndexForSeries != newIndex ) {
				ZTLVec< KineticExperiment* > series;
				int scount = e->getSeries( series, 1 );
				for( int j=0; j<scount; j++ ) {
					series[j]->reagentIndexForSeries = newIndex;
				}
			}
		}
	}
	reagents.copyStrings( newReagents );
}

int KineticSystem::reagentFindByName( char *r ) {
	if( !r ) {
		return -1;
	}
	for( int i=0; i<reagents.count; i++ ) {
		if( !strcmp(reagents[i],r) ) {
			return i;
		}
	}
	return -1;
}

void KineticSystem::reactionAddByNames( char *in0, char *in1, char *out0, char *out1 ) {
	Reaction reaction;
	reaction.in0 = reagentFindByName( in0 );
	reaction.in1 = reagentFindByName( in1 );
	reaction.out0 = reagentFindByName( out0 );
	reaction.out1 = reagentFindByName( out1 );
	reactions.add( reaction );
}

void KineticSystem::reactionAddByNamesTwoWay( char *in0, char *in1, char *out0, char *out1 ) {
	Reaction reaction0;
	reaction0.in0 = reagentFindByName( in0 );
	reaction0.in1 = reagentFindByName( in1 );
	reaction0.out0 = reagentFindByName( out0 );
	reaction0.out1 = reagentFindByName( out1 );
	reactions.add( reaction0 );

	Reaction reaction1;
	reaction1.in0 = reagentFindByName( out0 );
	reaction1.in1 = reagentFindByName( out1 );
	reaction1.out0 = reagentFindByName( in0 );
	reaction1.out1 = reagentFindByName( in1 );
	reactions.add( reaction1 );
}

//==================================================================================================
// Parsing reagents and reactions from a full text description of reaction(s)
// note: use of 'static' for file-local scope

static char *parsePtr;
static ZStr *parseToken;

static ZRegExp parseAnything( "^\\s*\\S+\\s*" );
int parseIsEmpty() {
	if( parseAnything.test( parsePtr ) ) {
		return 0;
	}
	return 1;
}
	
static ZRegExp parseSymbolRegEx( "^\\s*([A-Za-z_.~$#:?][A-Za-z_0-9.~$#:?]*)" );
static void parseSymbol() {
	if( parseSymbolRegEx.test(parsePtr) ) {
		zStrPrependS( parseToken, parseSymbolRegEx.get(1) );
		parsePtr += parseSymbolRegEx.getPos(0) + parseSymbolRegEx.getLen(0);
	}
	else {
		zStrPrependS( parseToken, "" );
	}
}

static ZRegExp parseReactionOperatorRegEx( "^[ \t]*([+=\\n}])*" );
static void parseReactionOperator() {
	if( parseReactionOperatorRegEx.test(parsePtr) ) {
		zStrPrependS( parseToken, parseReactionOperatorRegEx.get(1) );
		parsePtr += parseReactionOperatorRegEx.getPos(0) + parseReactionOperatorRegEx.getLen(0);
	}
	else {
		zStrPrependS( parseToken, "" );
	}
}

int isReservedWord( char *symbol ) {
	static char *reserved[] = { "log", "ln", "pow", "exp", "VOLT", "TEMP", "PRES", "CONC", "SCONC", "DERIV" };
	int count = sizeof( reserved ) / sizeof( reserved[0] );
	for( int i=0; i<count; i++ ) {
		if( !strcmp( symbol, reserved[i] ) ) {
			return 1;
		}
	}
	return 0;
}

char * KineticSystem::preprocessReactions( char *text ) {
	ZRegExp expandChainReaction( "(.*)\\{\\s*(\\S+)=\\s*(\\S+)\\s*\\}(\\d+)(.*)" );
	viewInfo.del( "preprocessedLinkedReactionCount" );

	if( expandChainReaction.test( text ) ) {
		int lastlen = strlen( expandChainReaction.get( 5 ) );
			// ZRegxp uses a set of 4 static buffers, so we can't "get" all 5 without overwriting
		char *first = expandChainReaction.get( 1 );
		char *a0	= expandChainReaction.get( 2 );
		char *a1	= expandChainReaction.get( 3 );
		int count	= expandChainReaction.getI( 4 );
		
		int len = strlen( first ) + (strlen(a0) + strlen(a1) + 3) * count + lastlen;
		char *newText = (char *)malloc( len * 2 );
		
		sprintf( newText, first );
		strcat( newText, ZTmpStr( "%s=%s", a0, a1 ) );

		for( int i=0; i<count-1; i++ ) {
			strcat( newText, ZTmpStr( "=%s%d", a1, i+1 ) );
		}
		char *last	= expandChainReaction.get( 5 );
		strcat( newText, last );

		viewInfo.putI( "preprocessedLinkedReactionCount", count );
		return newText;
	}
	return 0;
}

int KineticSystem::reactionsAddByFullText( char *_text ) {
	// This function may be called repeatedly to build up reagents and reactions
	// in a system.  Each may may specify one or more reactions.  Note that
	// even a single "A=B" results in two reactions, since we always add forward
	// and backward reactions to the system.  Multiple reactions may be defined
	// in a single call, as in "A + B = C = D = E + F", or individual reactions
	// may be delimted by newlines: "A+B=C\nC=D\nD=E+F"
	
	ZStr reactionStr;
		// the entire text of all reaction strings is built up into this;
		// it ends up begin just like _text, but with some formatting to make it prettier
	KineticSystem::Reaction react1, react2;
		// we build reactions as we parse the symbols in reaction text; we are potentially
		// building two reactions at a time for cases like A = B = C, since B participates in 2 reactions.
	int *react1Reagent=0, *react2Reagent=0;
		// pointers into the Reactions above; these are the "write" pointers for where to store the reagent index;
	int reagentCount;
		// ensure user doesn't enter too many reagents on one side of equation (only two allowed)

	// Use a state machine to parse the reactions
	enum parseState { psSyntaxError=-1, psIncomplete=0, psDone, psReactionsSetup, psReactionExpectingSymbol, psReactionExpectingOperator, psReactionDone };
		// To provide better feedback to the caller, we will distinguish between an erroneous statement, and
		// one that is incomplete, but as yet contains no syntax error.

	// Experimental: pre-process _text to expand instances of {A=B}3 = X to A=B=C=D = X etc and set viewInfo information
	// to indicate groups for these reactions.  This is a shorthand to allow the easy definition of reactions with a huge
	// number of steps.
	char *processedText = preprocessReactions( _text );
	if( processedText ) {
		_text = processedText;
	}
	
	// PREPARE the tokenizer
	parsePtr = _text;
	parseToken = 0;
	ZTLStack<int> parseStateStack;
	parseStateStack.push( psDone );
	int state = psReactionsSetup;

	while ( state != psSyntaxError && state != psIncomplete && state != psDone ) {
		switch( state ) {
			case psReactionsSetup: {
				react1.reset();
				react2.reset();
				react1Reagent = &react1.in0;
				react2Reagent = &react2.in0;
				reagentCount  = 0;
				state = psReactionExpectingSymbol;
				break;
			}

			case psReactionExpectingSymbol: {
				parseSymbol();
				if( ! parseToken->isFirst("") ) {
					char *reagent = parseToken->getS();
					if( isReservedWord( reagent ) ) {
						state = psSyntaxError;
						break;
					}
					reactionStr.appendS( reagent );
					int reagentIndex;
					if( (reagentIndex = reagentFindByName( reagent )) == -1 ) {
						reagentIndex = reagents.add( strdup(reagent) );
					}
					*react1Reagent++ = reagentIndex;
					*react2Reagent++ = reagentIndex;
					reagentCount++;
					state = psReactionExpectingOperator;
				}
				else if ( !parseIsEmpty() ) {
					state = psSyntaxError;
				}
				else {
					if( react1.in0 == -1 && react2.in0 == -1 && reactions.count ) {
						state = psReactionDone;
							// this is a case that a newline was entered after a valid mechanism, but
							// no additional symbol has been entered yet
					}
					else {
						state = psIncomplete;
					}
				}
				break;
			}

			case psReactionExpectingOperator: {
				parseReactionOperator();
				if( parseToken->isFirst("+") ) {
					if( reagentCount < 2 /* not too many reagents */ ) {
						reactionStr.appendS( " + " );
						state = psReactionExpectingSymbol;
					}
					else {
						state = psSyntaxError;
					}
				}
				else if( parseToken->isFirst("=") ) {
					reactionStr.appendS( " = " );
					// react2 is always the reaction we are finishing, and react1 always the one that is beginning
					if( react2.out0 != -1 ) {
						// add the completed reaction: we always add forward and reverse
						reactionAdd( react2.in0, react2.in1, react2.out0, react2.out1 );
						reactionAdd( react2.out0, react2.out1, react2.in0, react2.in1 );
					}
					// setup react2 to complete the reaction underway, and init react1
					memcpy( &react2, &react1, sizeof( react1 ) );
					react1.reset();
					react1Reagent = &react1.in0;
					react2Reagent = &react2.out0;
					reagentCount  = 0;
					state = psReactionExpectingSymbol;
				}
				else if( parseToken->isFirst("\n") ) {
					// newline signifies end of reaction; much like the case above but we don't continue to
					// build a new reaction from the last reagent
					reactionStr.appendS( "\n" );
					if( react2.out0 != -1 ) {
						// add the completed reaction: we always add forward and reverse
						reactionAdd( react2.in0, react2.in1, react2.out0, react2.out1 );
						reactionAdd( react2.out0, react2.out1, react2.in0, react2.in1 );
					}
					react1.reset();
					react2.reset();
					react1Reagent = &react1.in0;
					react2Reagent = &react2.in0;
					reagentCount  = 0;
					state = psReactionExpectingSymbol;
						// note; the above 6 lines could be replaced by setting state to psReactionSetup,
						// but it feels a bit bug-prone, so repeat the code here.
				}
				else if( parseToken->isFirst("") ) {
					if( !parseIsEmpty() ) {
						state = psSyntaxError;
					}
					else if( react2.out0 != -1 ) {
						// add the completed reaction: we always add forward and reverse
						int index = reactionAdd( react2.in0, react2.in1, react2.out0, react2.out1 );
						index = reactionAdd( react2.out0, react2.out1, react2.in0, react2.in1 );
						state = psReactionDone;
					}
					else {
						state = psIncomplete;
					}
				}
				else {
					state = psSyntaxError;
				}
				break;
			}

			case psReactionDone: {
				viewInfo.putS( "reactionText", reactionStr.getS() );
				state = psDone;
				break;
			}
		}
	}
	return state;
}

// end reaction string parser
//==================================================================================================

char * KineticSystem::reactionGetString( int r ) {
	static char reactionString[128];
		// return pointer to this static

	reactionString[0]=0;
	Reaction *re = &reactions[ r ];
	if( re->in0 >= 0 ) {
		strcat( reactionString, reagents[re->in0] );
	}
	if( re->in1 >= 0 ) {
		strcat( reactionString, " + " );
		strcat( reactionString, reagents[re->in1] );
	}
	if( re->out0 >= 0 ) {
		strcat( reactionString, " = " );
		strcat( reactionString, reagents[re->out0] );
	}
	if( re->out1 >= 0 ) {
		strcat( reactionString, " + " );
		strcat( reactionString, reagents[re->out1] );
	}
	return reactionString;
}

int KineticSystem::reactionGetReagents( int reaction, char **in0, char **in1, char **out0, char **out1 ) {
	int count=0;
	Reaction *r = &reactions[ reaction ];
	if( in0 ) {
		*in0 = 0;
		if( r->in0 != -1 ) {
			*in0 = reagents[ r->in0 ];
			count++;
		}
	}
	if( in1 ) {
		*in1 = 0;
		if( r->in1 != -1 ) {
			*in1 = reagents[ r->in1 ];
			count++;
		}
	}
	if( out0 ) {
		*out0 = 0;
		if( r->out0 != -1 ) {
			*out0 = reagents[ r->out0 ];
			count++;
		}
	}
	if( out1 ) {
		*out1 = 0;
		if( r->out1 != -1 ) {
			*out1 = reagents[ r->out1 ];
			count++;
		}
	}
	return count;
}

int KineticSystem::reactionGetFromReagents( char *_in0, char *_in1, char *_out0, char *_out1 ) {
	// return which reaction matches the ins/outs (any order) if found.
	int in0 = reagentFindByName( _in0 );
	int in1 = reagentFindByName( _in1 );
	int out0 = reagentFindByName( _out0 );
	int out1 = reagentFindByName( _out1 );
	
	for( int i=0; i<reactions.count; i++ ) {
		Reaction &r = reactions[i];
		if( (r.in0 == in0 && r.in1 == in1) || (r.in0 == in1 && r.in1 == in0) ) {
			if( (r.out0 == out0 && r.out1 == out1) || (r.out0 == out1 && r.out1 == out0) ) {
				return i;
			}
		}
	}
	return -1;
}


char * KineticSystem::reactionGetRateName( int reaction ) {
	// This function assumes that forward and backward reactions occur in matched
	// pairs, one after the other.  So reaction 0 returns "k+1", reaction 1 returns "k-1",
	// and so on.

	static char rateNameString[16];
		// returns pointer to this static

	rateNameString[0] = 'k';
	rateNameString[1] = reaction & 1 ? '-' : '+';
	sprintf( rateNameString+2, "%d", reaction/2 + 1 );

	return rateNameString;
}

KineticExperiment * KineticSystem::newExperiment() {
	KineticExperiment *e = new KineticExperiment( this );
	experiments.add( e );
	return e;
}

KineticExperiment * KineticSystem::getExperimentById( int _id ) {
	for( int i=0; i<experiments.count; i++ ) {
		if( experiments[i]->id == _id ) {
			return experiments[i];
		}
	}
	return 0;
}

KineticExperiment * KineticSystem::copyExperiment( KineticExperiment *e, int newUniqueId /* =1 */, int copyMeasured /* = 1*/ ) {
	KineticExperiment *eCopy = newExperiment();
	eCopy->copy( *e, copyMeasured );

	eCopy->system = this;
	eCopy->id = experimentNextUniqueId-1;
		// restore members overwritten in copy

	if( !newUniqueId ) {
		eCopy->id = e->id;

		// SET next unique id to valid value
		experimentNextUniqueId=0;
		for( int i=0; i<experiments.count; i++ ) {
			experimentNextUniqueId = max( experimentNextUniqueId, (experiments[i]->id + 1) );
		}
	}
	return eCopy;
}

void KineticSystem::copyInitialConditions( KineticExperiment *from, KineticExperiment *to ) {
	// After a copyExperiment, the caller will typically allocParamterInfo and then compile.
	// But the allocParameterInfo will not find the initial conditions for the new experiment
	// when it tries to lookup the saved values, since the unique id is different.
	// So  *after* the allocParamterInfo, this should be called to set those initial conditions.

	int fcount=0, tcount=0;
	int fIndex = from->getExperimentIndex();
	int tIndex = to->getExperimentIndex();

	// There are a number of kinds of "initial conditions" now, not just concentrations.
	// We also may have voltage, temperature, or pressure conditions.
	#define NUM_PARAMTYPES 9
	int paramTypes[ NUM_PARAMTYPES ] = { 
		PI_INIT_COND,
		PI_VOLTAGE,
		PI_VOLTAGE_COEF,
		PI_TEMPERATURE,
		PI_TEMPERATURE_COEF,
		PI_PRESSURE,
		PI_PRESSURE_COEF,
		PI_SOLVENTCONC,
		PI_SOLVENTCONC_COEF
	};

	for( int pt=0; pt<NUM_PARAMTYPES; pt++ ) {
		int type = paramTypes[ pt ];
		KineticParameterInfo *fpi = paramGet( type, &fcount, fIndex, -1 );
		KineticParameterInfo *tpi = paramGet( type, &tcount, tIndex, -1 );
		if( fpi ) {
			assert( tpi );
			assert( fcount == tcount );

			// we can loopp over fcount or tcount; or we can just check that we're
			// still looking at parameter-type for the appropriate experiment.
			while( fpi->experiment == fIndex && fpi->type == type ) {
				assert( tpi->experiment == tIndex && tpi->type == type );
				tpi->value = fpi->value;
				fpi++;
				tpi++;
			}
		}
	}
}

void KineticSystem::delExperiment( KineticExperiment *e ) {
	ZTLVec< KineticExperiment* > series;
	int scount = e->getSeries( series, 1 );
	for( int i=0; i<scount; i++ ) {
		experiments.del( series[i] );
	}
}

void KineticSystem::delExperiment( int eIndex ) {
	delExperiment( experiments[eIndex] );
}

void KineticSystem::delSeriesExperiments( int masterExpIndex ) {
	ZTLVec< KineticExperiment* > series;
	int scount = experiments[ masterExpIndex ]->getSeries( series, 1 );
	for( int i=1; i<scount; i++ ) {
		experiments.del( series[i] );
	}
	series[0]->reagentIndexForSeries = -1;
	series[0]->mixstepIndexForSeries = -1;
	series[0]->viewInfo.del( "seriesType" );
		// note that we did not delete series[0], which is the 'master' 
}

KineticExperiment * KineticSystem::addSeriesExperiment( int masterExpIndex, int reagentIndex /*=-1*/, int mixstepIndex /*=-1*/, int type/*=0*/ ) {
	KineticExperiment *master = experiments[masterExpIndex];
	int masterID = master->id;
	int currentSeriesType = master->viewInfo.getI( "seriesType", -1 );
	assert( currentSeriesType == -1 || currentSeriesType == type );
	master->viewInfo.putI( "seriesType", type );
	KineticExperiment *s = copyExperiment( master, 1, 0 );
	s->slaveToExperimentId = masterID;
	if( reagentIndex != -1 ) {
		master->reagentIndexForSeries = reagentIndex;
		s->reagentIndexForSeries = reagentIndex;
	}
	if( mixstepIndex != -1 ) {
		master->mixstepIndexForSeries = mixstepIndex;
		s->mixstepIndexForSeries = mixstepIndex;
	}
	return s;
}

void KineticSystem::paramInitPQIndex() {
	int i;
	int paramCount = 0;
	KineticParameterInfo *paramInfo = 0;

	int qi = 0;
	int pi = 0;

	// REACTION RATES
	paramInfo = paramGet( PI_REACTION_RATE, &paramCount );
	for( i=0; i<paramCount; i++ ) {
		paramInfo[i].qIndex = qi++;
		paramInfo[i].pIndex = pi++;
	}

	// OBSERVABLE CONSTANTS
	paramInfo = paramGet( PI_OBS_CONST, &paramCount );
	for( i=0; i<paramCount; i++ ) {
		paramInfo[i].qIndex = qi++;
		paramInfo[i].pIndex = pi++;
	}
	
	int piBeforeIC = pi;

	// ICs FOR EACH EXPERIMENT
	for( int e=0; e<eCount(); e++ ) {
		paramInfo = paramGet( PI_INIT_COND, &paramCount, e );
		pi = 0;
		for( i=0; i<paramCount; i++ ) {
			paramInfo[i].qIndex = qi++;
			paramInfo[i].pIndex = piBeforeIC + pi;
			pi++;
		}
	}
}

void KineticSystem::getQ( ZMat &qVec ) {
	int e, i;
	int paramCount = 0;
	KineticParameterInfo *paramInfo = 0;

	paramInfo = paramGet( PI_OBS_CONST, &paramCount );
	int voltTempPress = eCount() * 4;
		// a term for voltage, temperature, pressure, and solvent concentration are included for each experiment.
	int qCount = reactionCount() + paramCount + reagentCount() * eCount() + voltTempPress;
	qVec.alloc( qCount, 1, zmatF64 );

	int qi = 0;

	// REACTION RATES
	paramInfo = paramGet( PI_REACTION_RATE, &paramCount );
	for( i=0; i<paramCount; i++ ) {
		qVec.setD( qi, 0, paramInfo[i].value );
		qi++;
	}

	// OBSERVABLE CONSTANTS
	paramInfo = paramGet( PI_OBS_CONST, &paramCount );
	for( i=0; i<paramCount; i++ ) {
		qVec.setD( qi, 0, paramInfo[i].value );
		qi++;
	}
	
	// VOLTAGE/TEMP/PRESSURE FOR EACH EXPERIMENT
	// This is specifically to allow observable expressions to include keywords
	// like "VOLT" which should be substituted with the voltage for the given 
	// experiment. Likewise for TEMP, PRESS, CONC, (and now SCONC).  Since KineticSystem machinery
	// is setup to expect the P length to be the same for each experiment, then
	// we must insert default values for each experiment if *ANY* experiment
	// has one of these.  Therefore, it actually seems simplest to always include
	// a single term for each experiment for VOLT, TEMP, PRES.
	// NOTE: since we are not using ZFItter with volt,temp,pres experiments,
	// it may not be necessary to have these params in the Q vector,
	// since in getP we are pulling the values directly from paramInfo and not Q.
	double value;
	for( e=0; e<eCount(); e++ ) {
		// TODO: should I include mixsteps here as well?  If I want to pull the
		// P vector from the values of Q, then I need to, but that is only used
		// in fitting with ZFitter I presume, which is not supported for V,T,P anyway.
		value = 0.0;
		paramInfo = paramGet( PI_VOLTAGE, &paramCount, e );
		if( paramInfo ) {
			value = paramInfo->value;
		}
		qVec.setD( qi, 0, value );
		qi++;

		value = 1.0;
		paramInfo = paramGet( PI_TEMPERATURE, &paramCount, e );
		if( paramInfo ) {
			value = paramInfo->value;
		}
		qVec.setD( qi, 0, value );
		qi++;


		value = 1.0;
		paramInfo = paramGet( PI_PRESSURE, &paramCount, e );
		if( paramInfo ) {
			value = paramInfo->value;
		}
		qVec.setD( qi, 0, value );
		qi++;

		value = 1.0;
		paramInfo = paramGet( PI_SOLVENTCONC, &paramCount, e );
		if( paramInfo ) {
			value = paramInfo->value;
		}
		qVec.setD( qi, 0, value );
		qi++;
	}


	// ICs FOR EACH EXPERIMENT
	for( e=0; e<eCount(); e++ ) {
		paramInfo = paramGet( PI_INIT_COND, &paramCount, e );
		for( i=0; i<paramCount; i++ ) {
			qVec.setD( qi, 0, paramInfo[i].value );
			qi++;
		}
	}
}

void KineticSystem::getP( ZMat q, int forExperiment, ZMat &pVec, int forMixstep ) {
	// P is the local to experiment parameter vector.  So P is like Q, but has only ICs for the local experiment

	int i;

	int pCount = getPLength();
//printf( "pCount alloc %d\n", pCount );
	pVec.alloc( pCount, 1, zmatF64 );

	int pi = 0;

	// REACTION RATES
	int paramCount = 0;
	KineticParameterInfo *paramInfo = paramGet( PI_REACTION_RATE, &paramCount );
	for( i=0; i<paramCount; i++ ) {
		pVec.setD( pi, 0, q.getD(paramInfo[i].qIndex,0) );
		pi++;
	}

	// OBSERVABLE CONSTANTS
	paramInfo = paramGet( PI_OBS_CONST, &paramCount );
	for( i=0; i<paramCount; i++ ) {
		pVec.setD( pi, 0, q.getD(paramInfo[i].qIndex,0) );
		pi++;
	}
	
	// VOLTAGE/TEMP/PRESSURE
	// See comment in ::getQ; note that below we pull the values directly from paramInfo
	// rather than from Q, since Q does not hold values for all mixsteps, and the voltages
	// or other will typically be different at each mixstep.
	double value=0.0;
	paramInfo = paramGet( PI_VOLTAGE, &paramCount, forExperiment, forMixstep );
	pVec.setD( pi, 0, paramInfo ? paramInfo->value : value );
	pi++;
	
	value=1.0;
	paramInfo = paramGet( PI_TEMPERATURE, &paramCount, forExperiment, forMixstep );
	pVec.setD( pi, 0, paramInfo ? paramInfo->value : value );
	pi++;
	
	value=1.0;
	paramInfo = paramGet( PI_PRESSURE, &paramCount, forExperiment, forMixstep );
	pVec.setD( pi, 0, paramInfo ? paramInfo->value : value );
	pi++;

	value=1.0;
	paramInfo = paramGet( PI_SOLVENTCONC, &paramCount, forExperiment, forMixstep );
	pVec.setD( pi, 0, paramInfo ? paramInfo->value : value );
	pi++;

	// CONC - like above, this is a fixed value per mixstep - the initial concentration
	// of a concentration series.  This is in a register of its own, because the C registers
	// which will be loaded next are updated each step with the new concentrations, whereas
	// here we want the constant value of the intial conc.
	value = 1.0;
	int reagentIndex = experiments[ forExperiment ]->reagentIndexForSeries;
	if( reagentIndex != -1 ) {
		paramInfo = paramGet( PI_INIT_COND, &paramCount, forExperiment, forMixstep );
		if( paramInfo ) {
			// It may be this is NULL in the case of fitting series exp to analytic fns
			value = paramInfo[ reagentIndex ].value;
		}
	}
	pVec.setD( pi, 0, value );
	pi++;

	
	// NOTE: compileOC etc relies on the voltage/temp/pressure params being
	// at end of params (before IC), since they are referenced by symbolic
	// name like "VOLT" instead of paramInfo name like voltage_e0_m1"
	// see e.g. vmd->regIndexVolt()

	// ICs FOR THIS EXPERIMENT
	paramInfo = paramGet( PI_INIT_COND, &paramCount, forExperiment );
	for( i=0; i<paramCount; i++ ) {
		pVec.setD( pi, 0, q.getD(paramInfo[i].qIndex,0) );
		pi++;
	}
}

void KineticSystem::putQ( ZMat qVec ) {
	int e, i;

	int paramCount = 0;
	KineticParameterInfo *paramInfo = 0;

	int qi = 0;

	// REACTION RATES
	paramInfo = paramGet( PI_REACTION_RATE, &paramCount );
	for( i=0; i<paramCount; i++ ) {
		paramInfo[i].value = qVec.getD( qi, 0 );
		qi++;
	}

	// OBSERVABLE CONSTANTS
	paramInfo = paramGet( PI_OBS_CONST, &paramCount );
	for( i=0; i<paramCount; i++ ) {
		paramInfo[i].value = qVec.getD( qi, 0 );
		qi++;
	}
	
	// VOLTAGE/TEMP/PRESSURE:
	// ZFitter does not fit these, which is the use of this fn.
	qi += eCount() * 3;
		// skip past all voltage, temperature, pressure params

	// ICs FOR EACH EXPERIMENT
	for( e=0; e<eCount(); e++ ) {
		paramInfo = paramGet( PI_INIT_COND, &paramCount, e );
		for( i=0; i<paramCount; i++ ) {
			paramInfo[i].value = qVec.getD( qi, 0 );
			qi++;
		}
	}
}

int KineticSystem::getPLength() {
	int obsCount = 0;
	paramGet( PI_OBS_CONST, &obsCount );

	int voltTempPressConc = 5;	
		// a term for voltage, temperature, pressure, solventConc, initConc are included
		// for each experiment.
	
	return reactionCount() + obsCount + voltTempPressConc + reagentCount(); 
}


double *KineticSystem::paramGetVector( int type, int *count, int experimentIndex, int mixstep ) {
	int _count = 0;
	KineticParameterInfo *paramInfo = paramGet( type, &_count, experimentIndex, mixstep );
	assert( paramInfo );
	double *vec = new double[ _count ];
	for( int i=0; i<_count; i++ ) {
		vec[i] = paramInfo[i].value;
	}

	if( count ) {
		*count = _count;
	}
	return vec;
}

KineticParameterInfo *KineticSystem::paramGet( int type, int *count, int experimentIndex, int mixstep ) {
	KineticParameterInfo *first = 0;

	int _count = 0;
	for( int i=0; i<parameterInfo.count; i++ ) {
		KineticParameterInfo *k = &parameterInfo[i];
		if( k->type == type && ( k->experiment==experimentIndex || experimentIndex==-1 ) && ( k->mixstep==mixstep || mixstep==-1 ) ) {
			if( !first ) {
				first = &parameterInfo[i];
			}
			_count++;
		}
	}

	if( count ) {
		*count = _count;
	}

	return first;
}

KineticParameterInfo *KineticSystem::paramGetByName( char *name ) {
	for( int i=0; i<parameterInfo.count; i++ ) {
		if( !strcmp( parameterInfo[i].name, name ) ) {
			return &parameterInfo[i];
		}
	}
	return 0;
}

double KineticSystem::paramGetReactionRate( int r ) {
	KineticParameterInfo *pi = paramGet( PI_REACTION_RATE );
	return pi[r].value;
}

void KineticSystem::paramSetReactionRate( int r, double v ) {
	KineticParameterInfo *pi = paramGet( PI_REACTION_RATE );
	pi[r].value = v;
}

double KineticSystem::paramGetIC( int e, int r ) {
	KineticParameterInfo *pi = paramGet( PI_INIT_COND, 0, e );
	return pi[r].value;
}

void KineticSystem::paramSetIC( int e, int r, double v ) {
	KineticParameterInfo *pi = paramGet( PI_INIT_COND, 0, e );
	pi[r].value = v;
}

int KineticSystem::isDependent( KineticSystem::DependType type ) {
	int result = viewInfo.getI( "rateDependType", -1 ) == type;
	return result;
}

int KineticSystem::isRateDependent( int rate, int coefType, int includeLinkedRates ) {
	// Is the specific rate dependent on a certain parameter type?
	// If coefType == -1, it means all types.
	// If this rate is dependent, return the coefType it is dependent on
	int isDependent=0;

	int hasParam = 0;
	if( coefType == -1 ) {
		for( int i=0; i<4; i++ ) {
			if( paramGetRateCoefs( rate, rateDependCoefTypes[i] ) ) {
				coefType = rateDependCoefTypes[i];
				break;
			}
		}
	}
	if( coefType != -1 ) {
		if( includeLinkedRates ) {
			KineticParameterInfo *pi = paramGet( PI_REACTION_RATE );
			if( pi[ rate ].group > 1 ) { // a "linkage group"
				isDependent = rateGroupIsDependent( pi[ rate ].group, coefType );
			}
		}
		else {
			KineticParameterInfo *kpi = paramGetRateCoefs( rate, coefType );
			if( kpi ) {
				if( kpi[1].value != 0.0 ) {
					isDependent = 1;
						// because if the 2nd coef, which is the frequency term in the exponent, is 0, then
						// the param is not actually dependent on the quantity (e.g. voltage, activation energy etc)
						// that is multiplied by it in the exponential term.
				}
			}
		}
	}
	return isDependent ? coefType : 0;

}

int KineticSystem::rateGroupIsDependent( int group, int coefType, int *whichRate ) {
	// NOTE: previously it was the case that only one rate in a group was explicity dependent
	// on voltage, temperature, etc.  But now it is the case that if ANY rats in a group are
	// dependent in this way, then ALL will -- so if this group is dependent, we will always
	// return the first rate as whichRate below.
	int count;
	KineticParameterInfo *pi = paramGet( PI_REACTION_RATE, &count );
	for( int i=0; i<count; i++ ) {
		if( pi[i].group == group ) {
			int depType;
			if( (depType = isRateDependent( i, coefType )) != 0 ) {
				if( whichRate ) {
					*whichRate = i;
				}
				return depType;
			}
		}
	}
	return 0;
}

KineticParameterInfo* KineticSystem::paramGetRateCoefs( int rate, int coefType ) {
	// for the exponential expressions that may control rate: there are
	// always two such params per rate if the rate has them.
	int count;
	KineticParameterInfo *pi = paramGet( coefType, &count );
	for( int i=0; i<count; i++ ) {
		if( pi[i].reaction == rate ) {
			assert( i+1<count );
			return pi + i;
		}
	}
	return 0;
}

void KineticSystem::updateVoltageDependentRates( int eIndex, int mixstep, double *rates ) {

	//
	// Get the voltage particular to this experiment/mixstep, or use a reference voltage
	//
	double voltage, temperature;
	KineticParameterInfo *pi;
	if( eIndex >= 0 ) {
		pi = paramGet( PI_VOLTAGE, 0, eIndex, mixstep );
		assert( pi );
		voltage = pi->value;
		pi = paramGet( PI_TEMPERATURE, 0, eIndex, mixstep );
		assert( pi );
		temperature = pi->value;
	}
	else {
		voltage = viewInfo.getD( "referenceVoltage", -100 );
		temperature = viewInfo.getD( "referenceTemperature", 298.0 );
	}

	//
	// Update the value of any rate that is dependent on voltage
	//
	int count;
	pi = paramGet( PI_REACTION_RATE, &count );
	for( int i=0; i<count; i++ ) {
		KineticParameterInfo *vpi = paramGetRateCoefs( i, PI_VOLTAGE_COEF );
		if( vpi && vpi[1].value != 0.0 ) {
				// != 0.0 because those that have 0.0 for charge are mathematically not dependent on the voltage
			double amp  = vpi[0].value;
			double freq = vpi[1].value;
			if( i & 1 ) {
				freq = -freq;
					// reverse rates get negated charge in exponential term
					// TODO: convince myself this wouldn't negate the derivative in the Jacobian during fitting?
			}
			double rateVal = amp * exp( voltage * freq * FARADAY_CONST / ( GAS_CONST_JOULES * temperature ) );
			rates ? rates[ i ] = rateVal : pi[ i ].value = rateVal;
		}
	}
}

void KineticSystem::updateTemperatureDependentRates( int eIndex, int mixstep, double *rates ) {
	// This is called when Ea changes (or the experiment temp changes) and new rates should be computed
	// based on the rate at the reference temperature.  This is done during fitting of Ea, for example.

	//
	// Get the temperature for this experiment/mixstep, and the reference temperature
	//
	double expTemp, refTemp;
	KineticParameterInfo *pi = paramGet( PI_TEMPERATURE, 0, eIndex, mixstep );
	assert( pi );
	expTemp = pi->value;
	refTemp = viewInfo.getD( "referenceTemperature", 298.0 );
	
	//
	// Update the value of any rate that is dependent on temperature.  Note that
	// the rates at the reference temp are given by the PI_REACTION_RATE params.
	//
	int count;
	pi = paramGet( PI_REACTION_RATE, &count );
	for( int i=0; i<count; i++ ) {
		KineticParameterInfo *vpi = paramGetRateCoefs( i, PI_TEMPERATURE_COEF );
		if( vpi && vpi[1].value != 0.0) {
//			double amp  = vpi[0].value;
				// no longer used and eventually will go away
			double Ea = vpi[1].value;
			double rateVal = pi[i].value * exp( (-Ea/GAS_CONST_KJOULES)*(1.0/expTemp - 1.0/refTemp) );
			rates[ i ] = rateVal;
		}
	}
}

void KineticSystem::updateTemperatureDependentRatesAtRefTemp( double oldRefTemp, double newRefTemp ) {
	//
	// This is called to update the rates of this system which are based on a reference temperature.
	// When the reference temperature changes, we need to recompute all of the rates.
	// 
	int count;
	KineticParameterInfo *pi = paramGet( PI_REACTION_RATE, &count );
	for( int i=0; i<count; i++ ) {
		KineticParameterInfo *vpi = paramGetRateCoefs( i, PI_TEMPERATURE_COEF );
		if( vpi && vpi[1].value != 0.0) {
			//double amp  = vpi[0].value;
				// this is now unused and will eventually go away
			double Ea = vpi[1].value;
			double oldRate = pi[i].value;
			double newRate = oldRate * exp( (-Ea/GAS_CONST_KJOULES)*(1.0/newRefTemp - 1.0/oldRefTemp) );
			pi[ i ].value = newRate;
		}
	}
}

void KineticSystem::updateConcentrationDependentRates( int eIndex, int mixstep, double *rates ) {
	//
	// The current rates are those at the reference temperature and concentration.  Update
	// the rates taking into account the value of m, and any change in T and C.
	//
	double expConc, expTemp, refConc, refTemp;
	
	KineticParameterInfo *pi = paramGet( PI_SOLVENTCONC, 0, eIndex, mixstep );
	assert( pi );
	expConc = pi->value;

	pi = paramGet( PI_TEMPERATURE, 0, eIndex, mixstep );
	assert( pi );
	expTemp = pi->value;
	
	refConc = viewInfo.getD( "referenceConcentration", 0.0 );
	refTemp = viewInfo.getD( "referenceTemperature", 298.0 );

	//
	// Update the value of any rate that is dependent on concentration.  Note that
	// the rates at the reference conc are given by the PI_REACTION_RATE params.
	//
	int count;
	pi = paramGet( PI_REACTION_RATE, &count );
	for( int i=0; i<count; i++ ) {
		KineticParameterInfo *vpi = paramGetRateCoefs( i, PI_SOLVENTCONC_COEF );
		if( vpi && vpi[1].value != 0.0) {
			double m = vpi[1].value;
			// if( i & 1 ) {
			// 	m = -m;
			// 		// Not 
			// }
			double rateVal = pi[i].value * exp( m*(expConc/expTemp - refConc/refTemp)/GAS_CONST_KJOULES );
			rates[ i ] = rateVal;
		}
	}
}

void KineticSystem::updateConcentrationDependentRatesAtRefTemp( double oldRefTemp, double newRefTemp ) {
	//
	// Update system rates based on changing reference temperature
	//
	double refConc = viewInfo.getD( "referenceConcentration" );
	int count;
	KineticParameterInfo *pi = paramGet( PI_REACTION_RATE, &count );
	for( int i=0; i<count; i++ ) {
		KineticParameterInfo *vpi = paramGetRateCoefs( i, PI_SOLVENTCONC_COEF );
		if( vpi && vpi[1].value != 0.0) {
			double m = vpi[1].value;
			double oldRate = pi[i].value;
			double newRate = oldRate * exp( (m * refConc / GAS_CONST_KJOULES)*(1.0/newRefTemp - 1.0/oldRefTemp) );
			pi[ i ].value = newRate;
		}
	}
}

void KineticSystem::updateConcentrationDependentRatesAtRefConc( double oldRefConc, double newRefConc ) {
	//
	// Update system rates based on changing reference concentration
	//
	double refTemp = viewInfo.getD( "referenceTemperature", 298.0 );
	int count;
	KineticParameterInfo *pi = paramGet( PI_REACTION_RATE, &count );
	for( int i=0; i<count; i++ ) {
		KineticParameterInfo *vpi = paramGetRateCoefs( i, PI_SOLVENTCONC_COEF );
		if( vpi && vpi[1].value != 0.0) {
			double m = vpi[1].value;
			double oldRate = pi[i].value;
			double newRate = oldRate * exp( (m * (newRefConc - oldRefConc) / (GAS_CONST_KJOULES * refTemp)) );
			pi[ i ].value = newRate;
		}
	}
}


#define VALID_SYMBOLCHAR( c ) ( c == '_' || c == '.' || c == '~' || c == '$' || c == '#' || c == ':' || c == '?' || \
			(c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') )
	// These were allowed (_.~$#:) in v1 and will be allowed until we invent fancy symbology, if that happens.
	// NOTE: you must ensure this matches the regex defined int the reaction parser parseSymbolRegEx
	

int isReservedWord( char *symbolBegin, int len ) {
	char *fns[]  = { "log", "ln", "pow", "exp", "VOLT", "TEMP", "PRES", "CONC", "SCONC", "DERIV" };
	int fnLens[] = {3, 2, 3, 3, 4, 4, 4, 4, 5, 5};
	int count = sizeof( fns ) / sizeof( fns[0] );
	for( int i=0; i<count; i++ ) {
		if( len == fnLens[i] && !strncmp( fns[i], symbolBegin, len ) ) {
			return 1;
		}
	}
	return 0;
}


int KineticSystem::findValidSymbol( char *stringIn, char* &symbolBegin, char* &symbolEnd ) {
	// Look through stringIn for next valid symbol, and point to first and last char
	// of symbol with begin and end.  Return 1 if a symbol was found, 0 otherwise.
	symbolBegin = 0;
	for( char *c=stringIn; c && *c; c++ ) {
		if( VALID_SYMBOLCHAR( *c ) ) {
			if( !symbolBegin ) {
				symbolBegin = c;
			}
			symbolEnd = c;
		}
		// if the current char is not a valid symbol char, and we have built a valid symbol, return it,
		// otherwise keep looking for a symbol 
		//
		else if( symbolBegin ) {
			int len = symbolEnd - symbolBegin + 1;
			if( isReservedWord( symbolBegin, len ) ) {
				symbolBegin=0;
					// keep looking
			}
			else {
				return 1;
					// symbolBegin/End presently mark a valid symbol
			}
		}
	}
	// We must check once more for reserved work in case this is the last
	// symbol in the string:
	int len = symbolEnd - symbolBegin + 1;
	if( symbolBegin && isReservedWord( symbolBegin, len ) ) {
		symbolBegin = 0;
	}
	return ( symbolBegin != 0 );
}


void KineticSystem::observableConstSymbolExtract( char *instructions, ZHashTable &currentConsts, ZHashTable &totalConsts ) {
	char *symbolBegin, *symbolEnd, symbol[64];
	symbolBegin = instructions;
	while( findValidSymbol( symbolBegin, symbolBegin, symbolEnd ) ) {
		int len = symbolEnd - symbolBegin + 1;
		assert( len < 64 && "symbolLen too long!" );
		memcpy( symbol, symbolBegin, len );
		symbol[ len ] = 0;

		symbolBegin = symbolEnd + 1;
			// point to the next location to start searching for a symbol

		int r = reagentFindByName( symbol );
		if( r == -1 ) {
			// We are tracking the order of occurence, so only insert if not already exist
			if( currentConsts.getI( symbol, -1 ) == -1 ) {
				currentConsts.putI( symbol, currentConsts.activeCount() );
			}
			if( totalConsts.getI( symbol, -1 ) == -1 ) {
				totalConsts.putI( symbol, totalConsts.activeCount() );
			}
		}
	}
}


void KineticSystem::saveParameterInfo( ZHashTable &savedParameters ) {
	// This function extracts the current contents of the parameterInfo
	// vector and saves it in a ZHashTable indexed by unique strings.
	// This can be used to saved parameter values where possible across
	// edits to the system.  It could also be used to save/load the parameter
	// information to disk via the binary pack/unpack of ZHashTable.

	savedParameters.clear();
	int i;
	SavedKineticParameterInfo save;

	// Save reaction rates: these are keyed by a unique reaction string
	int paramCount=0;
	KineticParameterInfo *kpi = paramGet( PI_REACTION_RATE, &paramCount );
	for( i=0; i<paramCount; i++ ) {
		char *unique = reactionGetUniqueString( i );
		if( unique ) {
			savedParameters.putB( unique, save.set( kpi[i] ), sizeof( save ) );
		}
	}

	// Save observable constants: these have unique names already
	paramCount=0;
	kpi = paramGet( PI_OBS_CONST, &paramCount );
	for( i=0; i<paramCount; i++ ) {
		savedParameters.putB( kpi[i].name, save.set( kpi[i] ), sizeof( save ) );
	}

	// Save initial concentrations per experiment, per mixstep.
	for( i=0; i<experiments.count; i++ ) {
		for( int ms=0; ms<experiments[i]->mixstepCount; ms++ ) {
			paramCount=0;
			kpi = paramGet( PI_INIT_COND, &paramCount, i, ms );
			if( kpi ) {
				for( int j=0; j<paramCount; j++ ) {
					ZTmpStr unique( "%s_e%d_m%d", kpi[j].name, experiments[i]->id, experiments[i]->mixsteps[ms].id );
					savedParameters.putB( unique.s, save.set( kpi[j] ), sizeof( save ) );
				}
			}
		}
	}

	// Save voltage, temperature, pressure, concentration coefs if present
	//
	char prefix[4][16] = { "voltage", "temperature", "pressure", "concentration" };
    char suffix[8][3]  = { "Vo", "z", "To", "Ea", "P1", "P2", "k0", "m" };
		// this order should match rateDependCoefTypes; the suffixes are 2 per coef type
	for( int j=0; j<4; j++ ) {
		if( viewInfo.getI( "rateDependType", -1 ) == j ) {
			// We can only have coefficnets for one type of dependency, given by j
			//
			for( i=0; i<reactionCount(); i++ ) {
				KineticParameterInfo *kpi = paramGetRateCoefs( i, rateDependCoefTypes[j]  );
				if( kpi ) {
                    char *reactionString = reactionGetUniqueString( i );
                    ZTmpStr unique( "%s_%s", reactionString, suffix[ j*2 ] );
					savedParameters.putB( unique, save.set( kpi[0] ), sizeof( save ) );
					unique.set( "%s_%s", reactionString, suffix[ j*2+1 ] );
					savedParameters.putB( unique, save.set( kpi[1] ), sizeof( save ) );
				}
			}
			// But we may have initial conditions for more than one, since voltage for example
			// also makes use of a temperature initial condition.  (Really these are not "initial"
			// conditions, they are constant conditions, but may vary by experiment.)
			//
			for( int k=0; k<4; k++ ) {
				for( i=0; i<experiments.count; i++ ) {
					for( int ms=0; ms<experiments[i]->mixstepCount; ms++ ) {
						paramCount=0;
						kpi = paramGet( rateDependInitCondTypes[k], &paramCount, i, ms );
						if( kpi ) {
							for( int p=0; p<paramCount; p++ ) {
								ZTmpStr unique( "%s_e%d_m%d", prefix[k], experiments[i]->id, experiments[i]->mixsteps[ms].id );
								savedParameters.putB( unique.s, save.set( kpi[p] ), sizeof( save ) );
							}
						}
					}
				}
			}
			break;
				// we can only have 1 type of rate dependency in a system
		}
	}
}

void KineticSystem::loadParameterValues( ZHashTable &loadValues ) {
	SavedKineticParameterInfo *saved;
	int i;

	// Load reaction rates: these are keyed by a unique reaction string
	int paramCount=0;
	KineticParameterInfo *kpi = paramGet( PI_REACTION_RATE, &paramCount );
	for( i=0; i<paramCount; i++ ) {
		char *unique = reactionGetUniqueString( i );
		saved = (SavedKineticParameterInfo*)loadValues.getS( unique, 0 );
		if( saved ) {
			kpi[i].value = saved->value;
		}
	}

	// Load observable constants: these have unique names already
	paramCount=0;
	kpi = paramGet( PI_OBS_CONST, &paramCount );
	for( i=0; i<paramCount; i++ ) {
		saved = (SavedKineticParameterInfo*)loadValues.getS( kpi[i].name );
		if( saved ) {
			kpi[i].value = saved->value;
		}
	}

	// Load initial concentrations per experiment, per mixstep.
	for( i=0; i<experiments.count; i++ ) {
		for( int ms=0; ms<experiments[i]->mixstepCount; ms++ ) {
			paramCount=0;
			kpi = paramGet( PI_INIT_COND, &paramCount, i, ms );
			if( kpi ) {
				for( int j=0; j<paramCount; j++ ) {
					ZTmpStr unique( "%s_e%d_m%d", kpi[j].name, experiments[i]->id, experiments[i]->mixsteps[ms].id );
					saved = (SavedKineticParameterInfo*)loadValues.getS( unique );
					if( saved ) {
						kpi[j].value = saved->value;
					}
				}
			}
		}
	}

	// Load voltage, temperature, pressure, concentration coefs
	//
	char prefix[4][16] = { "voltage", "temperature", "pressure", "concentration" };
    char suffix[8][3]  = { "Vo", "z", "To", "Ea", "P1", "P2", "k0", "m" };
		// this order should match rateDependCoefTypes
	for( int j=0; j<4; j++ ) {
		if( viewInfo.getI( "rateDependType", -1 ) == j ) {
			// We can only have coefficnets for one type of dependency, given by j
			//
			for( i=0; i<reactionCount(); i++ ) {
				KineticParameterInfo *kpi = paramGetRateCoefs( i, rateDependCoefTypes[j]  );
				assert( kpi );
                
                char *reactionString = reactionGetUniqueString( i );
                ZTmpStr unique( "%s_%s", reactionString, suffix[ j*2 ] );
				saved = (SavedKineticParameterInfo*)loadValues.getS( unique, 0 );
				if( saved ) {
					kpi[0].value = saved->value;
				}
                
                unique.set( "%s_%s", reactionString, suffix[ j*2+1 ] );
				saved = (SavedKineticParameterInfo*)loadValues.getS( unique, 0 );
				if( saved ) {
					kpi[1].value = saved->value;
				}
			}
			// But we may have initial conditions for more than one, since voltage for example
			// also makes use of a temperature initial condition.  Really these are constant
			// conditions, though they may vary per experiment.
			//
			for( int k=0; k<4; k++ ) {
				for( i=0; i<experiments.count; i++ ) {
					for( int ms=0; ms<experiments[i]->mixstepCount; ms++ ) {
						paramCount=0;
						kpi = paramGet( rateDependInitCondTypes[k], &paramCount, i, ms );
						if( kpi ) {
							for( int p=0; p<paramCount; p++ ) {
								ZTmpStr unique( "%s_e%d_m%d", prefix[k], experiments[i]->id, experiments[i]->mixsteps[ms].id );
								saved = (SavedKineticParameterInfo*)loadValues.getS( unique );
								if( saved ) {
									kpi[p].value = saved->value;
								}
							}
						}
					}
				}
			}
			break;
				// we can only have 1 type of rate dependency in a system
		}
	}
}

void KineticSystem::allocParameterInfo( ZHashTable *paramValues ) {
	int e,i;

	// SAVE current paramter settings:
	//   The caller may pass a hash-table which contains parameter values keyed
	//   by unique strings (constructed in ::savedParameterInfo above).  If not,
	//   we'll try to save anything that is currently setup.
	// NOTE: There are operations on the KineticSystem that require the user to
	// save params *before* performing the operation, perform operation, and then
	// allocParameterInfo passing the previously saved hashtable.  For example,
	// operations like editing reactions, or deleting a mixstep; (e.g. in the latter
	// case we loop over all misteps saving params by eId and msId, but if mixstep 0
	// has been deleted, then the values for mixstep 1 will not be saved, and be
	// unavailable when the new system is parameterized (though the old ms1 is now 0).

	ZHashTable defaultValues;
	if( !paramValues ) {
		saveParameterInfo( defaultValues );
		paramValues = &defaultValues;
	}
	parameterInfo.clear();
	SavedKineticParameterInfo *saved;

	// REACTION RATES
	int q = 0;
	int p = 0;
	
	int preprocessedLinkedReactionCount = viewInfo.getI( "preprocessedLinkedReactionCount" ) * 2;
	for( int r=0; r<reactionCount(); r++ ) {
		KineticParameterInfo info;
		info.type = PI_REACTION_RATE;
		info.reaction = r;
		strcpy( info.name, reactionGetRateName( r ) );
		char *unique = reactionGetUniqueString( r );
		saved        = unique ? (SavedKineticParameterInfo*)paramValues->getS( unique, 0 ) : 0;
		info.value   = saved ? saved->value   : r&1 ? 1.0 : 10.0;
		info.group   = saved ? saved->group   : 0;
		info.fitFlag = saved ? saved->fitFlag : 1;
		info.qIndex = q++;
		info.pIndex = p++;

		if( r < preprocessedLinkedReactionCount ) {
			info.group = r & 1 ? 3 : 2;
		}

		parameterInfo.add( info );
	}

	// EXTRACT Observable Constants into a hash for counting; note that we process 
	// experiments by series such that irrespective of the ordering in the experiments
	// vector, the consants will be grouped by series in the parameters vector
	ZHashTable obsConstHash;
	ZTLVec< char * > orderedObsConsts;
	for( e=0; e<eCount(); e++ ) {
		KineticExperiment *ee = experiments[e];
		if( ee->slaveToExperimentId == -1 ) {
			ZTLVec< KineticExperiment* > series;
			int scount = ee->getSeries( series, 1 );
			for( int s=0; s<scount; s++ ) {
				ee = series[ s ];
				ee->obsConstsByName.clear();
				for( int o=0; o<ee->observableInstructions.count; o++ ) {
					observableConstSymbolExtract( ee->observableInstructions[o], ee->obsConstsByName, obsConstHash );
				}
			}
		}
	}
	orderedObsConsts.setCount( obsConstHash.activeCount() );
	for( i=0; i<obsConstHash.size(); i++ ) {
		char *key = obsConstHash.getKey( i );
		if( key ) {
			orderedObsConsts.set( obsConstHash.getValI( i ), key );
		}
	}


	// ALLOCATE observable constants; we do this in two passes such that 
	// scaling factors for series experiments can be placed at the end
	// of the list of constants for cosmetic reasons.
	int o = 0;
	ZRegExp scaleFactor( "scale_\\d+[a-z]" );
	ZRegExp numericValue( "^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?$" );
	for( int pass=0; pass<2; pass++ ) {
		for( i=0; i<orderedObsConsts.count; i++ ) {
			char *name = orderedObsConsts.get( i );
			assert( name );
			if( pass == 0 && scaleFactor.test( name ) ) {
				continue;
			}
			else if( pass == 1 && !scaleFactor.test( name ) ) {
				continue;
			}
			KineticParameterInfo info;
			strcpy( info.name, name );
			saved=(SavedKineticParameterInfo*)paramValues->getS( info.name, 0 );
			if( numericValue.test( info.name ) ) {
				info.value = atof( info.name );
				info.fitFlag = 0;
			}
			else {
				info.value   = saved ? saved->value   : !strncmp( info.name, "offset_", 7 ) ? 0.001 : 1.0;
				info.fitFlag = saved ? saved->fitFlag : 1;
			}
			info.group   = saved ? saved->group   : 0;
			info.type = PI_OBS_CONST;
			info.obsConst = o;
			info.qIndex = q++;
			info.pIndex = p++;
			parameterInfo.add( info );
			o++;
		}
	}
	
	int pBeforeIC = p;
	int maxQ = reactionCount() + o + reagentCount() * eCount() - 1;
		// mixsteps are not handled correctly for the p/q vectors or ZFitter.  So we 
		// clamp qIndex to maxQ to avoid asserts elsewhere.  This only affects experiments
		// with mixsteps, or those with voltage/temperature dependent rates, none of which
		// can be fit with ZFitter anyway.


	// ICs: note that we do extra work to process members of a series sequentially which
	// causes concentrations related to a series to be grouped sequentially for easier
	// parsing etc.
	for( e=0; e<eCount(); e++ ) {
		KineticExperiment *ee = experiments[e];
		if( ee->slaveToExperimentId == -1 ) {
			ZTLVec< KineticExperiment* > series;
			int seriesCount = ee->getSeries( series, 1 );
			for( int s=0; s<seriesCount; s++ ) {
				KineticExperiment *ke = series[s];
				for( int ms=0; ms<experiments[e]->mixstepCount; ms++ ) {
					// CREATE IC for this mix step
					for( int i=0; i<reagentCount(); i++ ) {
						KineticParameterInfo info;
						info.type = PI_INIT_COND;
						info.experiment = ke->getExperimentIndex();
						info.reagent = i;
						info.mixstep = ms;
						ZTmpStr unique( "%s_e%d_m%d", reagentGetName(i), ke->id, ke->mixsteps[ms].id );
						strcpy( info.name, reagentGetName(i) );
						saved=(SavedKineticParameterInfo*)paramValues->getS( unique.s, 0 );

						if( !saved && ke->slaveToExperimentId != -1 ) {
							// Look to our master for the IC if we don't have it ourselves, such that experiments that
							// were saved with SERIES_EXP_FULL_IC off will be setup correctly; this is only necessary
							// when loading older files that do not have all initial conditions for each experiment.
							unique.set( "%s_e%d_m%d", reagentGetName(i), ke->slaveToExperimentId, ke->mixsteps[ms].id );
							saved=(SavedKineticParameterInfo*)paramValues->getS( unique.s, 0 );
						}

						info.value   = saved ? saved->value   : ms==0 ? (i<2 ? 1.0 : 0.0) : 0;
						info.group   = saved ? saved->group   : 0;
						info.fitFlag = saved ? saved->fitFlag : 1;
						info.qIndex = q++;
						info.qIndex = min( info.qIndex, maxQ );
							// see note above at definition of maxQ, and comment directly below

						info.pIndex = pBeforeIC + e*reagentCount() + i;
							// @TODO: ZBS says: I'm not dealing with mixsteps properly in the fitter 3.0 yet so I'm not
							// sure what's going to happen to qIndex when we deal with that. (23 May 2009)
						parameterInfo.add( info );
					}
				}
			}
		}
	}

	// Voltage-related parameters only when at least 1 rate is dependent on voltage.
	// There are 2 kinds of voltage parameters:
	//
	// PI_VOLTAGE     : indentical in concept to "initial condition" for reagents - this is the
	//					voltage set for a given experiment, and as such there will be precisely
	//					M of these paramters for an experiment that has M mixsteps
	//
	// PI_VOLTAGE_COEF: these are the amplitude and frequency coefficients that with the voltage
	//					above define a give rate via an exponential expression k = A * exp( V * q )
	//					There is an 'A' and a 'q' term for each rate that is voltage-dependent.
	//					None, some, or all rates may be voltage dependent.
	//
	//					NOTE: we are moving toward NOT using the amplitude parameters for most 
	//					of these special rate dependencies.  E.g. temp, concentration create them,
	//					but they are not used by the program, because we compute new rates relative
	//					to old rates given a new reference temp or concentration, and the amplitude
	//					terms "fall out" and are not required for the computation.  They are still
	//					created here in case we want to utilize them, and to keep things symmetric
	//					since all of these dependencies share the same exponential form.
	// 
	//
	double refVoltage = viewInfo.getD( "referenceVoltage", -100 );
	double refTemperature = viewInfo.getD( "referenceTemperature", 298.0 );
	double refConcentration = viewInfo.getD( "referenceConcentration", 0.0 );
	if( isDependent( DT_Volt ) ) {
		// PI_VOLTAGE:
		//
		for( e=0; e<eCount(); e++ ) {
			KineticExperiment *ee = experiments[e];
			if( ee->slaveToExperimentId == -1 ) {
				ZTLVec< KineticExperiment* > series;
				int seriesCount = ee->getSeries( series, 1 );
				for( int s=0; s<seriesCount; s++ ) {
					KineticExperiment *ke = series[s];
					for( int ms=0; ms<experiments[e]->mixstepCount; ms++ ) {
						// CREATE Voltage param for this mix step
						//
						KineticParameterInfo info;
						info.type = PI_VOLTAGE;
						info.experiment = ke->getExperimentIndex();
						info.mixstep = ms;
						ZTmpStr unique( "voltage_e%d_m%d", ke->id, ke->mixsteps[ms].id );
						strcpy( info.name, unique );
						saved=(SavedKineticParameterInfo*)paramValues->getS( unique.s, 0 );

						if( !saved && ke->slaveToExperimentId != -1 ) {
							// Look to our master for the voltage if we don't have it ourselves
							unique.set( "voltage_e%d_m%d", ke->slaveToExperimentId, ke->mixsteps[ms].id );
							saved=(SavedKineticParameterInfo*)paramValues->getS( unique.s, 0 );
						}

						info.value   = saved ? saved->value   : ms==0 ? (-100) : 100;
						info.fitFlag = saved ? saved->fitFlag : 1;
						//info.qIndex = maxQ;
							// see note at definition of maxQ; this value will not be used;
						//info.pIndex = 0;
							// this will not be used; not sure if this will need to change;
						parameterInfo.add( info );
					}
				}
			}
		}
		// PI_VOLTAGE_COEF:
		//
		for( int r=0; r<reactionCount(); r++ ) {
			KineticParameterInfo info;
			info.type = PI_VOLTAGE_COEF;
			info.reaction = r;
			strcpy( info.name, ZTmpStr( "%s_Vo", reactionGetRateName( r ) ) );
            char *reactionString = reactionGetUniqueString( r );
            ZTmpStr unique( "%s_Vo", reactionString );
			saved        = (SavedKineticParameterInfo*)paramValues->getS( unique, 0 );
			info.value   = saved ? saved->value   : 1.0;
			info.fitFlag = saved ? saved->fitFlag : 1;
			info.group   = saved ? saved->group : !info.fitFlag;
			//info.qIndex = 0;
			//info.pIndex = 0;
			int index = parameterInfo.add( info );

			info.type = PI_VOLTAGE_COEF;
			info.reaction = r;
			strcpy( info.name, reactionGetRateName( r ) );
			info.name[0] = 'z';
				// e.g. 'z+1' for charge term for rate k+1
            unique.set( "%s_z", reactionString );
			saved        = (SavedKineticParameterInfo*)paramValues->getS( unique, 0 );
			info.value   = saved ? saved->value   : 0.0;
			info.fitFlag = saved ? saved->fitFlag : 1;
			info.group   = saved ? saved->group : !info.fitFlag;

			//info.qIndex = 0;
			//info.pIndex = 0;
			parameterInfo.add( info );

			// Now set the amplitude param based on the charge, voltage, and rate.
			KineticParameterInfo *ampParam = &parameterInfo[ index ];
			double charge  = info.value;
			double rate = paramGetReactionRate( r );
			if( r & 1 ) {
				charge = -charge;
					// we always store and display positive charges, but in our exponential expression charges for
					// reverse rates are negated
			}
			ampParam->value = rate / exp( refVoltage * charge * FARADAY_CONST / ( GAS_CONST_JOULES * refTemperature ) );
		}
	}
	if( isDependent( DT_Conc ) ) {
		for( e=0; e<eCount(); e++ ) {
			KineticExperiment *ee = experiments[e];
			if( ee->slaveToExperimentId == -1 ) {
				ZTLVec< KineticExperiment* > series;
				int seriesCount = ee->getSeries( series, 1 );
				for( int s=0; s<seriesCount; s++ ) {
					KineticExperiment *ke = series[s];
					for( int ms=0; ms<experiments[e]->mixstepCount; ms++ ) {
						// CREATE Concentration param for this mix step
						//
						KineticParameterInfo info;
						info.type = PI_SOLVENTCONC;
						info.experiment = ke->getExperimentIndex();
						info.mixstep = ms;
						ZTmpStr unique( "concentration_e%d_m%d", ke->id, ke->mixsteps[ms].id );
						strcpy( info.name, unique );
						saved=(SavedKineticParameterInfo*)paramValues->getS( unique.s, 0 );

						if( !saved && ke->slaveToExperimentId != -1 ) {
							// Look to our master for the concentration if we don't have it ourselves
							unique.set( "concentration_e%d_m%d", ke->slaveToExperimentId, ke->mixsteps[ms].id );
							saved=(SavedKineticParameterInfo*)paramValues->getS( unique.s, 0 );
						}

						info.value   = saved ? saved->value   : 0;
						info.fitFlag = saved ? saved->fitFlag : 1;
						parameterInfo.add( info );
					}
				}
			}
		}
		// PI_SOLVENTCONC_COEF:
		//
		for( int r=0; r<reactionCount(); r++ ) {
			KineticParameterInfo info;
			info.type = PI_SOLVENTCONC_COEF;
			info.reaction = r;
			strcpy( info.name, ZTmpStr( "%s_k0", reactionGetRateName( r ) ) );
            char *reactionString = reactionGetUniqueString( r );
            ZTmpStr unique( "%s_k0", reactionString );
            saved        = (SavedKineticParameterInfo*)paramValues->getS( unique, 0 );
			info.value   = saved ? saved->value   : 1.0;
			info.fitFlag = saved ? saved->fitFlag : 1;
			info.group   = saved ? saved->group : !info.fitFlag;

			int index = parameterInfo.add( info );

			info.type = PI_SOLVENTCONC_COEF;
			info.reaction = r;
			strcpy( info.name, ZTmpStr( "m%s", reactionGetRateName( r )+1 ) );
            unique.set( "%s_m", reactionString );
            saved        = (SavedKineticParameterInfo*)paramValues->getS( unique, 0 );
			info.value   = saved ? saved->value   : 0.0;
			info.fitFlag = saved ? saved->fitFlag : 1;
			info.group   = saved ? saved->group : !info.fitFlag;

			parameterInfo.add( info );

			// Now set the amplitude param based on the solvent accessibility constant, temperature, and rate.
			KineticParameterInfo *ampParam = &parameterInfo[ index ];
			double m    = info.value;
			double rate = paramGetReactionRate( r );
			ampParam->value = rate / exp( m / ( GAS_CONST_KJOULES * refTemperature ) );
		}
	}
	if( isDependent( DT_Volt ) || isDependent( DT_Conc) || isDependent( DT_Temp ) ) {
		// PI_TEMPERATURE: note that if this system is voltage or conc dependent, then it will also have
		// the PI_TEMPERATURE parameters for each experiment because voltage dependency implies
		// a temperature dependency in addition...
		//
		for( e=0; e<eCount(); e++ ) {
			KineticExperiment *ee = experiments[e];
			if( ee->slaveToExperimentId == -1 ) {
				ZTLVec< KineticExperiment* > series;
				int seriesCount = ee->getSeries( series, 1 );
				for( int s=0; s<seriesCount; s++ ) {
					KineticExperiment *ke = series[s];
					for( int ms=0; ms<experiments[e]->mixstepCount; ms++ ) {
						// CREATE Temperature param for this mix step
						//
						KineticParameterInfo info;
						info.type = PI_TEMPERATURE;
						info.experiment = ke->getExperimentIndex();
						info.mixstep = ms;
						ZTmpStr unique( "temperature_e%d_m%d", ke->id, ke->mixsteps[ms].id );
						strcpy( info.name, unique );
						saved=(SavedKineticParameterInfo*)paramValues->getS( unique.s, 0 );

						if( !saved && ke->slaveToExperimentId != -1 ) {
							// Look to our master for the voltage if we don't have it ourselves
							unique.set( "temperature_e%d_m%d", ke->slaveToExperimentId, ke->mixsteps[ms].id );
							saved=(SavedKineticParameterInfo*)paramValues->getS( unique.s, 0 );
						}

						info.value   = saved ? saved->value   : 298;
						info.fitFlag = saved ? saved->fitFlag : 1;
						parameterInfo.add( info );
					}
				}
			}
		}
		// PI_TEMPERATURE_COEF:
		//
		if( isDependent( DT_Temp ) ) {
			for( int r=0; r<reactionCount(); r++ ) {
				KineticParameterInfo info;
				info.type = PI_TEMPERATURE_COEF;
				info.reaction = r;
				strcpy( info.name, ZTmpStr( "%s_To", reactionGetRateName( r ) ) );
                char *reactionString = reactionGetUniqueString( r );
                ZTmpStr unique( "%s_To", reactionString );
                saved        = (SavedKineticParameterInfo*)paramValues->getS( unique, 0 );
				info.value   = saved ? saved->value   : 1.0;
				info.fitFlag = saved ? saved->fitFlag : 1;
				info.group   = saved ? saved->group : !info.fitFlag;

				int index = parameterInfo.add( info );

				info.type = PI_TEMPERATURE_COEF;
				info.reaction = r;
				strcpy( info.name, ZTmpStr( "Ea%s", reactionGetRateName( r )+1 ) );
                unique.set( "%s_Ea", reactionString );
                saved        = (SavedKineticParameterInfo*)paramValues->getS( unique, 0 );
				info.value   = saved ? saved->value   : 0.0;
				info.fitFlag = saved ? saved->fitFlag : 1;
				info.group   = saved ? saved->group : !info.fitFlag;

				parameterInfo.add( info );

				// Now set the amplitude param based on the activation energy, temperature, and rate.
				KineticParameterInfo *ampParam = &parameterInfo[ index ];
				double Ea   = info.value;
				double rate = paramGetReactionRate( r );
				ampParam->value = rate / exp( -Ea / ( GAS_CONST_KJOULES * refTemperature ) );
			}
		}
	}
}

int KineticSystem::fittingParametersCount() {
	int count = 0;
	for( int q=0; q<parameterInfo.count; q++ ) {
		if( parameterInfo[q].fitFlag ) {
			count++;
		}
	}
	return count;
}

void KineticSystem::clrFitFlags( int typeMask, int experiment ) {
	for( int q=0; q<parameterInfo.count; q++ ) {
		if( (parameterInfo[q].type & typeMask) && (experiment == -1 || experiment==parameterInfo[q].experiment) ) {
			parameterInfo[q].fitFlag = 0;
		}
	}
}

void KineticSystem::setFitFlags( int typeMask, int experiment ) {
	for( int q=0; q<parameterInfo.count; q++ ) {
		if( (parameterInfo[q].type & typeMask) && (experiment == -1 || experiment==parameterInfo[q].experiment) ) {
			parameterInfo[q].fitFlag = 1;
		}
	}
}

void KineticSystem::simulate( int forceAllExp /* =1 */, int steps /* =-1 */) {
	assert( vmd );
	ZMat q;

	for( int i=0; i<experiments.count; i++ ) {
		KineticExperiment *e = experiments[ i ];
		SIMSTATE_LOCK( this );
		if( forceAllExp || e->simulateState == KineticExperiment::KESS_Requested ) {
			getQ( q );
				// this has been moved inside the loop because experiments can need
				// simulation independent of one another, and they can become dirty
				// while we are in this loop, so we want to get the parameter values
				// at the time we decide an experiment needs to be simulated.
			e->simulateState = KineticExperiment::KESS_Simulating;
			SIMSTATE_UNLOCK( this );

			ZMat p;
			getP( q, i, p, 0 );
            
			if( e->viewInfo.getI( "isEquilibrium" ) ) {
				e->simulateEquilibriumKintek( vmd, p.getPtrD() );
					// this is not related to zbs equilibrium experiments.
			}
			else if( e->viewInfo.getI( "isPulseChase" ) ) {
				experiments[i]->simulatePulseChaseKintek( vmd, p.getPtrD() );
			}
			else {
				experiments[i]->simulate( vmd, p.getPtrD(), steps );
			}
		}
		else {
			SIMSTATE_UNLOCK( this );
		}
	}

}

int KineticSystem::getMasterExperiments( ZTLVec< KineticExperiment* > &exps, int plottedOnly ) {
	int count = 0;
	for( int i=0; i<experiments.count; i++ ) {
		if( experiments[i]->slaveToExperimentId == -1 ) {
			if( !plottedOnly || experiments[i]->viewInfo.getI("expPlot",1) ) {
				exps.add( experiments[i] );
				count++;
			}
		}
	}
	return count;
}
	

void KineticSystem::compile() {

	int e;
	if( vmd ) {
		delete vmd;
	}

//	vmd = new KineticVMCodeD( this, reactionCount() );
	vmd = new KineticVMCodeD( this, getPLength() - reagentCount() );
		// tfb jan 2012: because pCount inside of vmd should include
		// rate constants, observable constants, and volt/temp/press params

	// TELL each of the experiments to compile
	ZMat q;
	getQ( q );
	for( e=0; e<eCount(); e++ ) {
		ZMat p;
		getP( q, e, p );
		experiments[e]->compileOC();
	}
}

int KineticSystem::getBalance( int reaction, int reagent )  {
	// is the reagent gained or lost?

	// written to ease the porting of balanceMat v1 to new structure of v2
	// It is assumed that reactions are always added in pairs: that is, one 
	// forward and one backward, even if the backward rate is 0;
	// Note that new types of reactions possible in v2 are not represented well
	// by this function. (e.g. reagent may be on both sides of equation, or show
	// up twice on one side)

	Reaction *r = &reactions[ reaction ];
	if( reagent == r->in0 || reagent == r->in1 ) {
		return -1;
	}
	else if( reagent == r->out0 || reagent == r->out1 ) {
		return 1;
	}
	return 0;
}

int qsort_cmp( const void *v1, const void *v2 ) {
	return strcmp( (char*)v1, (char*)v2 ); 
	
}

char * KineticSystem::reactionGetUniqueString( int reaction ) {

	static char reactionString[256];
	reactionString[0] = 0;

	#define MAX_REACTANTS 256
	char reactantNames[MAX_REACTANTS][32];
//	assert(reagents.count<MAX_REACTANTS);
	if( reagents.count >= MAX_REACTANTS ) {
		return 0;
	}

	//
	// build a string containing the concatenated reaction names with their balance
	// values; the names should be in alphabetical order.
	//
	// eg:  b + a = c   will give reactionString == "a-1b-1c1"
	//

	// GET all reactant names and sort them
	int count=0;
	Reaction *r = &reactions[ reaction ];
	strcpy( reactantNames[ count++ ], ZTmpStr( "%s%d", reagents[ r->in0 ], -1 ) );
	if( r->in1 != -1 ) {
		strcpy( reactantNames[ count++ ], ZTmpStr( "%s%d", reagents[ r->in1 ], -1 ) );
	}
	strcpy( reactantNames[ count++ ], ZTmpStr( "%s%d", reagents[ r->out0 ], +1 ) );
	if( r->out1 != -1 ) {
		strcpy( reactantNames[ count++ ], ZTmpStr( "%s%d", reagents[ r->out1 ], +1 ) );
	}

	qsort( reactantNames, count, 32, qsort_cmp );

	// CONCAT into reactionString
	for( int i=0; i<count; i++ )  {
		strcat( reactionString, reactantNames[i] );		
	}

	return reactionString;
}

// KineticVM
//------------------------------------------------------------------------------------------------------------------------------------

KineticVM::KineticVM( KineticSystem *_system, int _pCount ) {
	system = _system;
	experiment = 0;
	pCount = _pCount;
	registers.setCount( regIndexCount() );
	registers[0] =  0.0;
	registers[1] =  1.0;
	registers[2] = -1.0;
	registers[3] =  2.0;
	registers[4] = -2.0;
	registers[5] = log(10.0);
}

KineticVM::~KineticVM() {
}

int KineticVM::regLookupC( char *name ) {
	int i;

	// FIND reagent name
	for( i=0; i<system->reagents.count; i++ ) {
		if( !strcmp(system->reagents[i],name) ) {
			return regIndexC( i );
		}
	}

	// FIND observable constant?
	KineticParameterInfo *paramInfo = system->paramGetByName( name );
	if( !paramInfo ) {
		return 0;
	}
	assert( paramInfo && paramInfo->type == PI_OBS_CONST );
	return regIndexP( paramInfo->pIndex );
}

char *KineticVM::regName( int i ) {
	static char buf[128];

	int rc = system->reagentCount();
	int pc = pCount;

	if( i == regIndexConstZero() ) {
		return "constZero";
	}
	if( i == regIndexConstOne() ) {
		return "constOne";
	}
	if( i == regIndexConstMinusOne() ) {
		return "constMinusOne";
	}
	if( i == regIndexConstTwo() ) {
		return "constTwo";
	}
	if( i == regIndexConstMinusTwo() ) {
		return "constMinusTwo";
	}
	if( i == regIndexConstLn10() ) {
		return "constLn10";
	}
	if( i >= regIndexP(0) && i < regIndexP(pc) ) {
		i -= regIndexP(0);
		sprintf( buf, "p[%d]", i );
	}
	if( i >= regIndexC(0) && i < regIndexC(rc) ) {
		i -= regIndexC(0);
		sprintf( buf, "C[%d('%s')]", i, system->reagentGetName(i) );
	}
	if( i >= regIndexD(0) && i < regIndexD(rc) ) {
		i -= regIndexD(0);
		sprintf( buf, "D[%d('%s')]", i, system->reagentGetName(i) );
	}
	if( i >= regIndexG(0,0) && i < regIndexG(rc,pc) ) {
		i -= regIndexG(0,0);
		int parameter = i / rc;
		int reagent   = i % rc;
		sprintf( buf, "G[%d,%d(d[%s]/d_p%d)]", reagent, parameter, system->reagentGetName(reagent), parameter );
	}
	if( i == regIndexWRT() ) {
		return "WRT";
	}
	return buf;
}

void KineticVM::regLoadP( double *P ) {
	memcpy( regP(0), P, sizeof(double) * pCount );
}

void KineticVM::regLoadC( double *C ) {
	memcpy( regC(0), C, sizeof(double) * system->reagentCount() );
}

void KineticVM::regLoadD( double *D ) {
	memcpy( regD(0), D, sizeof(double) * system->reagentCount() );
}

void KineticVM::regLoadG( int wrtParameter, double *G ) {
	int reagentCount = system->reagentCount();
	for( int i=0; i<reagentCount; i++ ) {
		// SETUP the register value for this parameter / reagent pair from the G vector
		*regG( i, wrtParameter ) = G[i];
	}
}

void KineticVM::executeMacBlock( int *codeStart, int blockStart, int count, double *output ) {
	int *index = codeStart + blockStart;

	for( int i=0; i<count; i++ ) {
		double outReg = 0.0;

		int *code = codeStart + *index;
		if( *code ) {
			// EXECUTE multiply and accumulate 4-tuple machine
			int len = *code++;
			for( int i=0; i<len; i+=4 ) {
				outReg += registers[ code[i+0] ] * registers[ code[i+1] ] * registers[ code[i+2] ] * registers[ code[i+3] ];
			}
		}

		*output++ = outReg;
		index++;
	}
}

void KineticVM::executeExpressionBlock( int *codeStart, int blockStart, int count, double *output ) {
	int *index = codeStart + blockStart;

	ZTLStack<double> stack( 64, 64 );

	for( int i=0; i<count; i++ ) {

		int *code = codeStart + *index;
		if( *code ) {
			int len = *code++;
			int *start = code;
			while( code - start < len ) {
				Ops opcode = (Ops)*code++;

				switch( opcode ) {
					case PUSH: {
						stack.push( registers[ *code ] );
						code++;
						break;
					}
					
					case PUSH_TIMEREF_C:
					case PUSH_TIMEREF_D:
					case PUSH_TIMEREF_G:
					{
						int reagent = *code++;
						double timeRef = *(double *)code;
						code++;
						code++;
						assert( experiment );
						double lastTime = experiment->traceC.getLastTime();
						if( timeRef < 0.0 || timeRef > lastTime ) {
							timeRef = lastTime;
						}
						double val = 0.0;
						if( opcode == PUSH_TIMEREF_C ) {
							val = experiment->traceC.getElemSLERP( timeRef, reagent );
						}
						else if( opcode == PUSH_TIMEREF_D ) {
							stack.push( 0.0 );
						}
						else if( opcode == PUSH_TIMEREF_G ) {
							int pIndex = (int)registers[ regIndexWRT() ];
							val = experiment->traceG[pIndex]->getElemSLERP( timeRef, reagent );
						}
						stack.push( val );
						break;
					}
					
					case LN: {
						double s0 = stack.pop();
						stack.push( log(s0) );
						break;
					}

					case LOG10: {
						double s0 = stack.pop();
						stack.push( log10(s0) );
						break;
					}

					case EXP: {
						double s0 = stack.pop();
						stack.push( exp(s0) );
						break;
					}

  					case POW10: {
						double s0 = stack.pop();
						stack.push( pow( 10.0, s0 ) );
						break;
					}

					case ADD: {
						double s0 = stack.pop();
						double s1 = stack.pop();
						stack.push( s0+s1 );
						break;
					}

					case SUB: {
						double s0 = stack.pop();
						double s1 = stack.pop();
						stack.push( s1-s0 );
						break;
					}

					case MUL: {
						double s0 = stack.pop();
						double s1 = stack.pop();
						stack.push( s0*s1 );
						break;
					}

					case DIV: {
						double s0 = stack.pop();
						double s1 = stack.pop();
						if( s0 == 0.0 ) {
							stack.push( 0.0 );
						}
						else {
							stack.push( s1/s0 );
						}
						break;
					}

					case CMP_WRT: {
						int pIndex = *code++;
						if( pIndex == (int)registers[ regIndexWRT() ] ) {
							stack.push( 1.0 );
						}
						else {
							stack.push( 0.0 );
						}
						break;
					}

					default: {
						assert( !"Invalid opcode" );
						break;
					}
				}
			}
			*output++ = stack.pop();
			assert( stack.top == 0 );
		}
		else {
			*output++ = 0.0;
		}

		index++;
	}
}

void KineticVM::disassembleMacDump( FILE *f, int *code ) {
	int count = *code++;
	if( !count ) {
		fprintf( f, "0" );
	}
	for( int z=0; z<count; z+=4 ) {
		char *a = strdup( regName( code[z+0] ) );
		char *b = strdup( regName( code[z+1] ) );
		char *c = strdup( regName( code[z+2] ) );
		char *d = strdup( regName( code[z+3] ) );
		fprintf( f, "%s*%s*%s*%s + ", a, b, c, d );
		free( a ); free( b ); free( c ); free( d );
	}
	fprintf( f, "\n" );
}

void KineticVM::disassembleExpressionDump( FILE *f, int *code ) {
	int len = *code++;
	fprintf( f, "(len=%d) ", len );

	int *start = code;
	while( code - start < len ) {
		Ops opcode = (Ops)*code++;
		switch( opcode ) {
			case PUSH: {
				char *a = strdup( regName( *code ) );
				fprintf( f, "%s ", a );
				free( a );
				code++;
				break;
			}
			case ADD:
				fprintf( f, "+ " );
				break;
			case SUB:
				fprintf( f, "- " );
				break;
			case MUL:
				fprintf( f, "* " );
				break;
			case DIV:
				fprintf( f, "/ " );
				break;
			case LN:
				fprintf( f, "ln " );
				break;
			case LOG10:
				fprintf( f, "log " );
				break;
			case EXP:
				fprintf( f, "exp " );
				break;
			case POW10:
				fprintf( f, "pow " );
				break;
			case CMP_WRT: {
				int wrt = *code++;
				fprintf( f, "(WRT %d) ", wrt );
				break;
			}
			default: {
				fprintf( f, "Unknown opcode %d\n", opcode );
				break;
			}
		}
	}
	fprintf( f, "\n" );
}

// KineticVMCode
//------------------------------------------------------------------------------------------------------------------------------------

KineticVMCode::KineticVMCode( KineticSystem *_system, int _pCount )
 : KineticVM( _system, _pCount )
{
	system = _system;
	pCount = _pCount;
}

KineticVMCode::~KineticVMCode() {
}

void KineticVMCode::emitStart( int index ) {
	bytecode[index] = bytecode.count;
	int zero = 0;
	bytecode.add( zero );
}

void KineticVMCode::emitStop( int index ) {
	bytecode[ bytecode[index] ] = bytecode.count - bytecode[index] - 1;
}

void KineticVMCode::emit( int value ) {
	bytecode.add( value );
}

void KineticVMCode::emit( double value ) {
	int *ptr = (int *)&value;
	bytecode.add( *ptr );
	ptr++;
	bytecode.add( *ptr );
}

int KineticVMCode::hasTimeRef( char *name, int &reagent, int &parameter, double &timeRef, int &error ) {
	error = 0;
	char *dup = strdup( name );
	timeRef = 0.0;
	int foundTimeRef = 0;

	char *square0 = strchr( dup, '[' );
	if( square0 ) {
		*square0 = 0;
		char *square1 = strchr( &square0[1], ']' );
		if( square1 ) {
			*square1 = 0;

			if(
				( square0[1] == 'e' || square0[1] == 'E' ) &&
				( square0[2] == 'n' || square0[2] == 'N' ) &&
				( square0[3] == 'd' || square0[3] == 'D' )
			) {
				timeRef = -1.0;
			}
			else {
				timeRef = atof( &square0[1] );
			}
			foundTimeRef = 1;
		}
		else {
			error = 1;
		}
		
	}

	reagent = -1;
	parameter = -1;
	
	reagent = system->reagentFindByName(dup);

	KineticParameterInfo *p = system->paramGetByName(dup);
	if( p ) {
		parameter = p->pIndex;
	}

	free( dup );
	return foundTimeRef;
}


// KineticVMCodeD
//------------------------------------------------------------------------------------------------------------------------------------

KineticVMCodeD::KineticVMCodeD( KineticSystem *_system, int _pCount, int includeJacobian )
 : KineticVMCode( _system, _pCount )
{
	blockStart_D = 0;
	blockStart_dD_dC = 0;
	compile( includeJacobian );
	#ifdef VM_DISASSEMBLE
		disassemble();
	#endif
}

void KineticVMCodeD::compile( int includeJacobian ) {
	int i, j;
		// index reagents
	int r;
		// index reactions

	// @TODO: Consider moving this lookup table into KineticBase if I use this again on the H system
	// Don't forget the deletes below
	ZTLVec<int> *reagentInMap = new ZTLVec<int>[ system->reagentCount() ];
	ZTLVec<int> *reagentOutMap = new ZTLVec<int>[ system->reagentCount() ];
	for( i=0; i<system->reactions.count; i++ ) {
		if( system->reactions[i].in0 != -1 ) {
			reagentInMap[ system->reactions[i].in0 ].addUniq( i );
		}
		if( system->reactions[i].in1 != -1 ) {
			reagentInMap[ system->reactions[i].in1 ].addUniq( i );
		}
		if( system->reactions[i].out0 != -1 ) {
			reagentOutMap[ system->reactions[i].out0 ].addUniq( i );
		}
		if( system->reactions[i].out1 != -1 ) {
			reagentOutMap[ system->reactions[i].out1 ].addUniq( i );
		}
	}

	bytecode.reset();
	bytecode.grow = 1024;

	bytecode.setCount( bcIndexCount() );

	// COMPUTE all the lookups to the start of each block
	blockStart_D     = bcIndex_D(0);
	blockStart_dD_dC = bcIndex_dD_dC( 0, 0 );

	int reactionCount = system->reactionCount();
	int reagentCount = system->reagentCount();

	int one = regIndexConstOne();
	int minusOne = regIndexConstMinusOne();
	int two = regIndexConstTwo();
	int minusTwo = regIndexConstMinusTwo();
	int ln10 = regIndexConstLn10();

	// COMPILE D
	//--------------------------------------------------------------------------------------------------

	for( i=0; i<reagentCount; i++ ) {
		emitStart( bcIndex_D(i) );

		int *reacts[2] = { (int *)reagentInMap[i], (int *)reagentOutMap[i] };
		int reactCounts[2] = { reagentInMap[i].count, reagentOutMap[i].count };

		// CHECK each side of the reaction to see if reagent i is involved
		for( int side=0; side<2; side++ ) {
			for( int ri=0; ri<reactCounts[side]; ri++ ) {
				r = reacts[side][ri];
				int in0 = system->reactionGet(r,0,0);
				int in1 = system->reactionGet(r,0,1);

				int dimer = system->reactionGet(r,side,0) == system->reactionGet(r,side,1);
				int signConstant = dimer ? ( side==0?minusTwo:two ) : ( side==0?minusOne:one );

				emit( signConstant );
				emit( regIndexP(r) );
				emit( in0 == -1 ? one : regIndexC( in0 ) );
				emit( in1 == -1 ? one : regIndexC( in1 ) );
			}
		}

		emitStop( bcIndex_D(i) );
	}


	// COMPILE dD_dC
	//--------------------------------------------------------------------------------------------------
	//ZMat debugMat( reagentCount, reagentCount, zmatF64 );
	if( includeJacobian ) {
		for( j=0; j<reagentCount; j++ ) {
			// j is the index of the reagent we are differenting with respect to.

			for( i=0; i<reagentCount; i++ ) {
			
				// @TODO: Optimize this so that for very sparse systems (liek the amorphous computing)
				// this would not write zeros into all of the zero'd places but would rather encode
				// the destination address for each block (which would make it a little less efficent for
				// dense systems but probably a lot faster for sparse ones.)
				// Also, potentially allowing sprase format for the matrix, but I don't know if the
				// GSL or NR integrator code has a sprase format.  So, this is a good place for the
				// students to work on.
				
				// These two arrays are optimizations so that we don't have to look through every
				// reaction looking for those that include the reagents.  The input and output
				// reactions are in a list by reagent using the above lookup maps.
				int *reacts[2] = { (int *)reagentInMap[i], (int *)reagentOutMap[i] };
				int reactCounts[2] = { reagentInMap[i].count, reagentOutMap[i].count };

				emitStart( bcIndex_dD_dC(i,j) );

				// CHECK each side of the reaction to see if reagent i is involved
				for( int side=0; side<2; side++ ) {
					for( int ri=0; ri<reactCounts[side]; ri++ ) {
						r = reacts[side][ri];
						int in0 = system->reactionGet(r,0,0);
						int in1 = system->reactionGet(r,0,1);

						// Reagent i is involved on this side of the reaction

						int dimer = system->reactionGet(r,side,0) == system->reactionGet(r,side,1);
						int signConstant = dimer ? ( side==0?minusTwo:two ) : ( side==0?minusOne:one );
							// For input, sign constant == -1 (register 2), for output sign constant == +1 (register 1)
							
						int emittedSomething = 0;

						// CASE 1: Two inputs to reaction r
						if( in0 != -1 && in1 != -1 ) {
							// Here we use the "Two Input Triplet Rule":
							// Start with: rate * in0 * in1
							// j indexes reagent which we are differentiating with respect to
							// Differentiates to: [rate * in0 * (d_in1/d_reagent[j])] + [rate * (d_in0/d_reagent[j]) * in1] + [(d_rate/d_reagent[j])*in0*in1]
							
							// Term 1 of the square brackets above which is non-zero only in case that in1 == j
							if( in1 == j ) {
								emit( signConstant );
								emit( regIndexP(r) );
								emit( regIndexC( in0 ) );
								emit( one );
								emittedSomething++;
							}

							// Term 2 of the square brackets above which is non-zero only in case that in0 == j
							if( in0 == j ) {
								emit( signConstant );
								emit( regIndexP(r) );
								emit( regIndexC(in1) );
								emit( one );
								emittedSomething++;
							}

							// Term 3 of the square brackets above is always zero
						}

						// CASE 2: Only one input to reaction r
						else {
							// Here we use the "One Input Rule"
							// Start with: rate * in0
							// j indexes reagent which we are differentiating with respect to
							// Differentiates to [rate * (d_in0/d_reagent[j])] + [d_rate/d_reagent[j] * in0]

							assert( in0 != -1 && !(in0 == -1 && in1 != -1) );
								// To simplify the code if there is only one input it must be in slot 0

							// Term 1 of the square brackets above which is non-zero only when in0 == j
							if( in0 == j ) {
								emit( signConstant );
								emit( regIndexP(r) );
								emit( one );
								emit( one );
								emittedSomething++;
							}

							// Term 2 of the square brackets above is always zero
						}
						
						if( emittedSomething ) {
							//debugMat.putD( i, j, 1.0 );
						}
					}
				}

				emitStop( bcIndex_dD_dC(i,j) );
			}
		}
	}

	#ifdef VM_DISASSEMBLE
		disassemble();
	#endif

	delete [] reagentInMap;
	delete [] reagentOutMap;
	
	//zmatSave_Matlab( debugMat, "jacob_view.mat", "J" );
	
}

void KineticVMCodeD::execute_D( double *C, double *dC_dt ) {
	int reagentCount = system->reagentCount();
	regLoadC( C );
	executeMacBlock( &bytecode[0], blockStart_D, reagentCount, dC_dt );
}

void KineticVMCodeD::execute_dD_dC( double *C, double *dD_dC ) {
	int reagentCount = system->reagentCount();
	regLoadC( C );
	executeMacBlock( &bytecode[0], blockStart_dD_dC, reagentCount*reagentCount, dD_dC );
}

void KineticVMCodeD::disassemble( char *ext ) {
	// WRITE out a trace for debugging
	int i, j;
	int reagentCount = system->reagentCount();
	if( !ext ) {
		ext = "d";
	}

	FILE *f = fopen( ZTmpStr("aode_disassemble_%s.txt",ext), "wb" );

	for( i=0; i<reagentCount; i++ ) {
		fprintf( f, "D[i=%d] = ", i );
		int *code = &bytecode[ bytecode[ blockStart_D+i ] ];
		disassembleMacDump( f, code );
	}

	for( i=0; i<reagentCount; i++ ) {
		for( j=0; j<reagentCount; j++ ) {
			fprintf( f, "dD_dC[i=%d j=%d] = ", i, j );
			int *code = &bytecode[ bytecode[ blockStart_dD_dC+i*reagentCount+j ] ];
			disassembleMacDump( f, code );
		}
	}

	fclose( f );
}

// KineticVMCodeH
//------------------------------------------------------------------------------------------------------------------------------------

KineticVMCodeH::KineticVMCodeH( KineticSystem *_system, int _pCount )
 : KineticVMCode( _system, _pCount )
{
	blockStart_H = 0;
	blockStart_dH_dG = 0;
	blockStart_dH_dt = 0;
	compile();
}

void KineticVMCodeH::compile() {
	#if !defined( KIN ) && !( defined(__llvm__) && defined(__APPLE__) )
	// because the llvm-g++ compiler that is default on osx 10.7.2 (Lion)
	// will segfault when compiling this routine if optimizations are turned
	// on to -O2 or -O3 -- and we don't need this routine for Kintek software
	// since we don't expose the ZFitter in the commercial version.
	// Note that I ensure with KIN that no version of KintekExp will compile
	// this code so that all platforms are built the same.

	int i, j;
		// index reagents
	int p;
		// index parameters
	int r;
		// index reactions

	bytecode.reset();
	bytecode.grow = 1024;

	bytecode.setCount( bcIndexCount() );

	// COMPUTE all the lookups to the start of each block
	blockStart_H     = bcIndex_H( 0, 0 );
	blockStart_dH_dG = bcIndex_dH_dG( 0, 0, 0 );
	blockStart_dH_dt = bcIndex_dH_dt( 0, 0 );

	int reactionCount = system->reactionCount();
	int reagentCount = system->reagentCount();
	int one = regIndexConstOne();
	int minusOne = regIndexConstMinusOne();
	int two = regIndexConstTwo();
	int minusTwo = regIndexConstMinusTwo();
	int ln10 = regIndexConstLn10();

	// COMPILE H
	//--------------------------------------------------------------------------------------------------

	// P is to local address of Q.  See notes in KineticParameterInfo in kineticbase.h

	for( p=0; p<pCount; p++ ) {
		// p is the paramter that we are differenting with respect to.

		for( i=0; i<reagentCount; i++ ) {

			emitStart( bcIndex_H(i,p) );

			// ASSEMBLE reagent i's rate equation by accounting for gains and losses in each reaction
			for( r=0; r<reactionCount; r++ ) {
				int in0 = system->reactionGet(r,0,0);
				int in1 = system->reactionGet(r,0,1);

				// CHECK each side of the reaction to see if reagent i is involved
				for( int side=0; side<2; side++ ) {
					if( system->reactionGet(r,side,0) == i || system->reactionGet(r,side,1) == i ) {
						// Reagent i is involved on this side of the reaction

						int dimer = system->reactionGet(r,side,0) == system->reactionGet(r,side,1);
						int signConstant = dimer ? ( side==0?minusTwo:two ) : ( side==0?minusOne:one );
							// For input, sign constant == -1, for output sign constant == +1

						// CASE 1: Two inputs to reaction r
						if( in0 != -1 && in1 != -1 ) {
							// Here we use the "Two Input Triplet Rule":
							// Start with: rate * in0 * in1
							// j indexes reagent which we are differentiating with respect to
							// Differentiates to: [rate * in0 * (d_in1/d_param[p])] + [rate * in1 * (d_in0/d_param[p])] + [(d_rate/d_param[p])*in0*in1]

							// Term 1 of the square brackets above
							emit( signConstant );
							emit( regIndexP(r) );
							emit( regIndexC(in0) );
							emit( regIndexG(in1,p) );

							// Term 2 of the square brackets above
							emit( signConstant );
							emit( regIndexP(r) );
							emit( regIndexC(in1) );
							emit( regIndexG(in0,p) );

							// Term 3 of the square brackets above (non-zero only when p == r)
							if( p == r ) {
								emit( signConstant );
								emit( one );
								emit( regIndexC(in0) );
								emit( regIndexC(in1) );
							}
						}

						// CASE 2: Only one input to reaction r
						else {
							// Here we use the "One Input Rule"
							// Start with: rate * in0
							// j indexes reagent which we are differentiating with respect to
							// Differentiates to [rate * (d_in0/d_param[p])] + [d_rate/d_param[p] * in0]

							assert( in0 != -1 && !(in0 == -1 && in1 != -1) );
								// To simplify the code if there is only one input it must be in slot 0

							// Term 1 of the square brackets above
							emit( signConstant );
							emit( one );
							emit( regIndexP(r) );
							emit( regIndexG(in0,p) );
							
							// Term 2 of the square brackets above which in non-zero only when p == r
							if( p == r ) {
								emit( signConstant );
								emit( one );
								emit( one );
								emit( regIndexC(in0) );
							}
						}
					}
				}
			}

			emitStop( bcIndex_H(i,p) );
		}
	}


	// COMPILE dH_dG
	//--------------------------------------------------------------------------------------------------
	for( p=0; p<pCount; p++ ) {
		// p indexes the parameter that we are differentiating with respect to in the following derivative variable

		// dH[i]_dG[j]
		for( j=0; j<reagentCount; j++ ) {
			for( i=0; i<reagentCount; i++ ) {
				
				emitStart( bcIndex_dH_dG(p,i,j) );

				// ASSEMBLE reagent i's rate equation by accounting for gains and losses in each reaction
				for( r=0; r<reactionCount; r++ ) {
					int in0 = system->reactionGet(r,0,0);
					int in1 = system->reactionGet(r,0,1);
			
					// CHECK each side of the reaction to see if reagent i is involved
					for( int side=0; side<2; side++ ) {
						if( system->reactionGet(r,side,0) == i || system->reactionGet(r,side,1) == i ) {
							// Reagent i is involved on this side of the reaction

							int dimer = system->reactionGet(r,side,0) == system->reactionGet(r,side,1);
							int signConstant = dimer ? ( side==0?minusTwo:two ) : ( side==0?minusOne:one );
								// For input, sign constant == -1, for output sign constant == +1
				
							// CASE 1: Two inputs to reaction r
							if( in0 != -1 && in1 != -1 ) {
								// Here we use the "Two Input Triplet Rule":
								// Start with: rate * in0 * in1
								// Differentiates to: [rate * in0 * (d_in1/d_param[p])] + [rate * (d_in0/d_param[p]) * in1] + [(d_rate/d_param[p])*in0*in1]
								// We're thinking of "d_in1/d_param[p]" as an individual variable
								// So we emit when we find the case tht in1==k

								// Term 1 of the square brackets above
								if( in1==j ) {
									emit( signConstant );
									emit( regIndexP(r) );
									emit( regIndexC(in0) );
									emit( one );
								}

								// Term 2 of the square brackets above
								if( in0==j ) {
									emit( signConstant );
									emit( regIndexP(r) );
									emit( regIndexC(in1) );
									emit( one );
								}

								// Term 3 of the square brackets above is always zero
							}

							// CASE 2: Only one input to reaction r
							else {
								// Here we use the "One Input Rule"
								// Start with: rate * in0
								// j indexes reagent which we are differentiating with respect to
								// Differentiates to [rate * (d_in0/d_param[p])] + [d_rate/d_param[p] * in0]
								// We're thinking of "d_in0/d_param[p]" as an individual variable
								// So we emit when we find the case that in0==k
		
								assert( in0 != -1 && !(in0 == -1 && in1 != -1) );
									// To simplify the code if there is only one input it must be in slot 0
			
								// Term 1 in square brackets above
								if( in0==j ) {
									emit( signConstant );
									emit( one );
									emit( regIndexP(r) );
									emit( one );
								}

								// Second term in square brackets above is always zero
							}
						}
					}
				}

				emitStop( bcIndex_dH_dG(p,i,j) );
			}
		}
	}


	// COMPILE dH_dt
	//--------------------------------------------------------------------------------------------------
	for( p=0; p<pCount; p++ ) {
		// p indexes the parameter that we are differentiating with respect to in the following derivative variable

		for( i=0; i<reagentCount; i++ ) {
			
			emitStart( bcIndex_dH_dt(p,i) );

			// ASSEMBLE reagent i's rate equation by accounting for gains and losses in each reaction
			for( r=0; r<reactionCount; r++ ) {
				int in0 = system->reactionGet(r,0,0);
				int in1 = system->reactionGet(r,0,1);
		
				// CHECK each side of the reaction to see if reagent i is involved
				for( int side=0; side<2; side++ ) {
					if( system->reactionGet(r,side,0) == i || system->reactionGet(r,side,1) == i ) {
						// Reagent i is involved on this side of the reaction

						int dimer = system->reactionGet(r,side,0) == system->reactionGet(r,side,1);
						int signConstant = dimer ? ( side==0?minusTwo:two ) : ( side==0?minusOne:one );
							// For input, sign constant == -1, for output sign constant == +1
			
						// CASE 1: Two inputs to reaction r
						if( in0 != -1 && in1 != -1 ) {
							// Here we use the "Two Input Triplet Rule":
							// Start with: rate * in0 * in1
							// Differentiates to: [rate * in0 * (d_in1/d_param[p])] + [rate * (d_in0/d_param[p]) * in1] + [(d_rate/d_param[p])*in0*in1]
							// We're thinking of "d_in1/d_param[p]" as an individual variable
							// Now differentiate wrt time we get the following 3 terms:
							// [ rate * d_in0/dt * d_in1/d_param[p]) ]
							// [ rate * d_in1/dt * d_in0/d_param[p]) ]
							// [ 0 ]

							// Term 1 of the square brackets above
							emit( signConstant );
							emit( regIndexP(r) );
							emit( regIndexD(in0) );
							emit( regIndexG(in1,p) );

							// Term 2 of the square brackets above
							emit( signConstant );
							emit( regIndexP(r) );
							emit( regIndexD(in1) );
							emit( regIndexG(in0,p) );

							// Term 3 of the square brackets above
							if( p==r ) {
								emit( signConstant );
								emit( one );
								emit( regIndexC(in0) );
								emit( regIndexD(in1) );

								emit( signConstant );
								emit( one );
								emit( regIndexD(in0) );
								emit( regIndexC(in1) );
							}
						}

						// CASE 2: Only one input to reaction r
						else {
							// Here we use the "One Input Rule"
							// Start with: rate * in0
							// Differentiates to [rate * (d_in0/d_param[p])] + [d_rate/d_param[p] * in0]
							// We're thinking of "d_in0/d_param[p]" as an individual variable
							// Now differentiate wrt time we get the term:
							// [ rate * d_in0/dt * d_in0/d_param[p]) ]
	
							assert( in0 != -1 && !(in0 == -1 && in1 != -1) );
								// To simplify the code if there is only one input it must be in slot 0
							// Term 1 is always zero
							// Term 2 in square brackets above
							if( p==r ) {
								emit( signConstant );
								emit( one );
								emit( regIndexD(in0) );
								emit( one );
							}
							// Second term in square brackets above is always zero
						}
					}
				}
			}

			emitStop( bcIndex_dH_dt(p,i) );
		}
	}
	#endif
}

void KineticVMCodeH::execute_H( int wrtParameter, double *C, double *G, double *dG_dt ) {
	int reagentCount = system->reagentCount();

	regLoadG( wrtParameter, G );
	regLoadC( C );

	// EXECUTE bytecode for H
	executeMacBlock( &bytecode[0], blockStart_H+reagentCount*wrtParameter, reagentCount, dG_dt );
}

void KineticVMCodeH::execute_dH_dG( int wrtParameter, double *C, double *D, double *G, double *dH_dG, double *dH_dt ) {
	int reagentCount = system->reagentCount();

	regLoadG( wrtParameter, G );
	regLoadC( C );
	regLoadD( D );

	// EXECUTE bytecode for dH_dG
	executeMacBlock( &bytecode[0], blockStart_dH_dG + reagentCount*reagentCount*wrtParameter, reagentCount*reagentCount, dH_dG );

	// EXECUTE bytecode for dH_dt
	executeMacBlock( &bytecode[0], blockStart_dH_dt + reagentCount*wrtParameter, reagentCount, dH_dt );
}

void KineticVMCodeH::disassemble( char *ext ) {
	// WRITE out a trace for debugging
	int i, j, p;
	int reagentCount = system->reagentCount();
	if( !ext ) {
		ext = "h";
	}

	FILE *f = fopen( ZTmpStr("aode_disassemble_%s.txt",ext), "wb" );

	for( i=0; i<reagentCount; i++ ) {
		for( p=0; p<pCount; p++ ) {
			fprintf( f, "H[i=%d p=%d] = ", i, p );
			int *code = &bytecode[ bytecode [ blockStart_H + reagentCount*p + i ] ];
			disassembleMacDump( f, code );
		}
	}

	for( p=0; p<pCount; p++ ) {
		for( i=0; i<reagentCount; i++ ) {
			for( j=0; j<reagentCount; j++ ) {
				fprintf( f, "dH_dG[p=%d i=%d j=%d] = ", p, i, j );
				int *code = &bytecode[ bytecode[ blockStart_dH_dG + reagentCount*reagentCount*p + reagentCount*i + j ] ];
				disassembleMacDump( f, code );
			}
		}
	}

	for( p=0; p<pCount; p++ ) {
		for( i=0; i<reagentCount; i++ ) {
			fprintf( f, "dH_dt[p=%d i=%d] = ", p, i );
			int *code = &bytecode[ bytecode[ blockStart_dH_dt + reagentCount*p + i ] ];
			disassembleMacDump( f, code );
		}
	}

	fclose( f );
}

// KineticVMCodeOC
//------------------------------------------------------------------------------------------------------------------------------------

KineticVMCodeOC::KineticVMCodeOC( KineticExperiment *_experiment, int _pCount )
 : KineticVMCode( _experiment->system, _pCount )
{
	experiment = _experiment;
}

/*
int KineticVMCodeOC::parenAwareSearcher( char *str, char lookingFor ) {
	// Finds the character "lookingFor" in "str" and returns the offset of the position or -1 if not found
	int parenDepth = 0;

	int i = 0;
	for( char *c=str; *c; c++, i++ ) {
		if( *c == '(' ) {
			parenDepth++;
		}
		else if( *c == ')' ) {
			parenDepth--;
		}
		else if( *c == lookingFor && parenDepth==0 ) {
			return i;
		}
	}
	return -1;
}
*/
int KineticVMCodeOC::parenAwareSearcher( char *str, char lookingFor ) {
	// Finds the character "lookingFor" in "str" and returns the offset of the position or -1 if not found
	int parenDepth = 0;

	int i = strlen( str ) - 1;
	for( char *c=&str[i]; c>=str; c--, i-- ) {
		if( *c == '(' ) {
			parenDepth++;
		}
		else if( *c == ')' ) {
			parenDepth--;
		}
		else if( *c == lookingFor && parenDepth==0 ) {
			return i;
		}
	}
	return -1;
}

int KineticVMCodeOC::parenAwareSearcherStr( char *str, char *lookingFor ) {
	// Finds the string "lookingFor" in "str" and returns the offset of the position or -1 if not found
	int parenDepth = 0;

	int i = strlen( str ) - 1;
	int lenLookingFor = strlen( lookingFor );
	char *endStr = str + i;
	for( char *c=&str[i]; c>=str; c--, i-- ) {
		if( *c == '(' ) {
			parenDepth++;
		}
		else if( *c == ')' ) {
			parenDepth--;
		}
		else if( parenDepth == 0 && 
			( c+lenLookingFor>endStr || !VALID_SYMBOLCHAR( *(c+lenLookingFor) ) ) &&
				// don't look past end of str
			( c==str || !VALID_SYMBOLCHAR( *(c-1) ) ) && !strncmp(c,lookingFor,lenLookingFor) ) {
			// the VALID_SYMBOLCHAR logic is to ensure we find exactly what we are lookingFor,
			// and not a substring within a longer symbol
			return i;
		}
	}
	return -1;
}


int KineticVMCodeOC::splitter( char *str, int index, char *&lft, char *&rgt ) {
	int lftLen = index;
	int rgtLen = strlen( str ) - index - 1;
	int lftIndex = 0;
	int rgtIndex = index + 1;

	lft = (char *)malloc( lftLen + 1 );
	rgt = (char *)malloc( rgtLen + 1 );

	// STRIP parens if the first and last paren are matching
	if( str[0] == '(' && str[index-1] == ')' ) {
		int strip = 1;
		int depth = 0;
		for( char *c=&str[0]; c<=&str[index-1]; c++ ) {
			if( *c == '(' ) {
				depth++;
			}
			else if( *c == ')' ) {
				depth--;
				if( depth == 0 && c<&str[index-1] ) {
					strip = 0;
					break;
				}
			}
		}
		if( strip ) {
			lftIndex++;
			lftLen-=2;
		}
	}

	if( str[index+1] == '(' && str[rgtIndex+rgtLen-1] == ')' ) {
		int strip = 1;
		int depth = 0;
		for( char *c=&str[index+1]; c<=&str[rgtIndex+rgtLen-1]; c++ ) {
			if( *c == '(' ) {
				depth++;
			}
			else if( *c == ')' ) {
				depth--;
				if( depth == 0 && c<&str[rgtIndex+rgtLen-1] ) {
					strip = 0;
					break;
				}
			}
		}
		if( strip ) {
			rgtIndex++;
			rgtLen-=2;
		}
	}

	memcpy( lft, &str[lftIndex], lftLen );
	lft[lftLen] = 0;

	memcpy( rgt, &str[rgtIndex], rgtLen );
	rgt[rgtLen] = 0;

	return 0;
}

int KineticVMCodeOC::recurseCompile( char *str ) {
	int error = 0;

	char *lft = 0;
	char *rgt = 0;


	int addI = parenAwareSearcher( str, '+' );
	if( addI >= 0 ) {
		error += splitter( str, addI, lft, rgt );
		if( !lft || !*lft ) {
			emit( PUSH );
			emit( regIndexConstZero() );
		}
		else {
			error += KineticVMCodeOC::recurseCompile( lft );
		}	
		error += KineticVMCodeOC::recurseCompile( rgt );
		emit( ADD );
	}
	else {
		int minI = parenAwareSearcher( str, '-' );
		if( minI >= 0 ) {
			error += splitter( str, minI, lft, rgt );
			if( !lft || !*lft ) {
				emit( PUSH );
				emit( regIndexConstZero() );
			}
			else {
				error += KineticVMCodeOC::recurseCompile( lft );
			}
			error += KineticVMCodeOC::recurseCompile( rgt );
			emit( SUB );
		}
		else {
			int mulI = parenAwareSearcher( str, '*' );
			if( mulI >= 0 ) {
				error += splitter( str, mulI, lft, rgt );
				if( !lft ) {
					return 1; // error
				}
				error += KineticVMCodeOC::recurseCompile( lft );
				error += KineticVMCodeOC::recurseCompile( rgt );
				emit( MUL );
			}
			else {
				int divI = parenAwareSearcher( str, '/' );
				if( divI >= 0 ) {
					error += splitter( str, divI, lft, rgt );
					if( !lft ) {
						return 1; // error
					}
					error += KineticVMCodeOC::recurseCompile( lft );
					error += KineticVMCodeOC::recurseCompile( rgt );
					emit( DIV );
				}
				else {
					int logI = parenAwareSearcherStr( str, "log" );
					if( logI >= 0 ) {
						error += splitter( str, logI+2, lft, rgt );
						error += KineticVMCodeOC::recurseCompile( rgt );
						emit( LOG10 );
					}
					else {
						int lnI = parenAwareSearcherStr( str, "ln" );
						if( lnI >= 0 ) {
							error += splitter( str, lnI+1, lft, rgt );
							error += KineticVMCodeOC::recurseCompile( rgt );
							emit( LN );
						}
						else {
							int powI = parenAwareSearcherStr( str, "pow" );
							if( powI >= 0 ) {
								error += splitter( str, powI+2, lft, rgt );
								error += KineticVMCodeOC::recurseCompile( rgt );
								emit( POW10 );
							}
							else {
								int expI = parenAwareSearcherStr( str, "exp" );
								if( expI >= 0 ) {
									error += splitter( str, expI+2, lft, rgt );
									error += KineticVMCodeOC::recurseCompile( rgt );
									emit( EXP );
								}
								else {
									// VOLT, TEMP, AND PRES are shorthand to mean the voltage (or other)
									// for the current experiment, current mixstep.  Thus the name of 
									// the param will vary, but as long as getP is called at the start
									// of each mixstep, the appropriate values will get loaded into the
									// P vector, and we can reference them as below.
									if( !strcmp( "VOLT", str ) ) {
										emit( PUSH );
										emit( regIndexVolt() );
									}
									else if( !strcmp( "TEMP", str ) ) {
										emit( PUSH );
										emit( regIndexTemp() );
									}
									else if( !strcmp( "PRES", str ) ) {
										emit( PUSH );
										emit( regIndexPres() );
									}
									else if( !strcmp( "SCONC", str ) ) {
										emit( PUSH );
										emit( regIndexSConc() );
//										int r = regIndexSConc();
//										double *v = regP( r );
//										printf( "SCONC pushing %g\n", *v );
										
									}
									else if( !strcmp( "CONC", str ) ) {
										emit( PUSH );
										emit( regIndexConc() );
									}
									else {
										int reagent = -1, parameter = -1;
										double timeRef = 0.0;
										int _error = 0;
										
										int hasTime = hasTimeRef( str, reagent, parameter, timeRef, _error );
										error += _error;
										if( hasTime ) {
											emit( PUSH_TIMEREF_C );
											if( reagent >= 0 ) {
												emit( reagent );
											}
											else {
												error++;
											}
											
											emit( timeRef );
										}
										else {
											if( reagent >= 0 ) {
												emit( PUSH );
												emit( regIndexC(reagent) );
											}
											else if( parameter >= 0 ) {
												emit( PUSH );
												emit( regIndexP(parameter) );
											}
											else {
												error++;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if( lft ) {
		free( lft );
	}
	if( rgt ) {
		free( rgt );
	}

	return error;
}

int KineticVMCodeOC::compile() {
	bytecode.reset();
	bytecode.grow = 1024;

	bytecode.setCount( experiment->observablesCount() );

	int combinedErr = 0;

	for( int o=0; o<experiment->observableInstructions.count; o++ ) {
		emitStart( o );

		char *src = experiment->observableInstructions[o];

		// ELIM all whitespace
		int err = 0;
		char *dst = (char *)malloc( strlen(src) + 1 );
		char *d = dst;
		char *s = src;
		if( !strncmp( s, "[DERIV]", 7 ) ) {
			s += 7;
			// skip special command to use time-derivative of this obs
		}
		while( *s && !err ) {
			if( *s != ' ' && *s != '\t' && *s != '\r' && *s != '\n' && (*s != '>' || s != src) ) {
					// (*s != '>' || s != src) : > is legal as first char only, but not compiled
				*d++ = *s;
			}
			s++;
		}
		*d = 0;
		
		err = recurseCompile( dst );
		free( dst );

		experiment->observableError[o] = err;
		combinedErr |= err;

		emitStop( o );
	}
	return combinedErr;
}

void KineticVMCodeOC::disassemble( char *ext ) {
	// WRITE out a trace for debugging
	int i;
	int reagentCount = system->reagentCount();
	if( !ext ) {
		ext = "oc";
	}

	FILE *f = fopen( ZTmpStr("aode_disassemble_%s.txt",ext), "wb" );

	for( i=0; i<experiment->observablesCount(); i++ ) {
		fprintf( f, "O[i=%d] = ", i );
		int *code = &bytecode[ bytecode[ i ] ];
		disassembleExpressionDump( f, code );
	}

	fclose( f );
}

void KineticVMCodeOC::execute_OC( double *C, double *P, double *OC ) {
	regLoadC( C );
	regLoadP( P );

	// EXECUTE bytecode for O
	executeExpressionBlock( &bytecode[0], 0, experiment->observablesCount(), OC );
}


// KineticVMCodeOG
//------------------------------------------------------------------------------------------------------------------------------------

int KineticVMCodeOG::recurseCompile( char *str ) {
	int error = 0;

	char *lft = 0;
	char *rgt = 0;


	int addI = parenAwareSearcher( str, '+' );
	if( addI >= 0 ) {
		error += splitter( str, addI, lft, rgt );
		if( !lft ) {
			emit( PUSH );
			emit( regIndexConstZero() );
		}
		error += KineticVMCodeOG::recurseCompile( lft );
		error += KineticVMCodeOG::recurseCompile( rgt );
		emit( ADD );
	}
	else {
		int minI = parenAwareSearcher( str, '-' );
		if( minI >= 0 ) {
			error += splitter( str, minI, lft, rgt );
			if( !lft ) {
				emit( PUSH );
				emit( regIndexConstZero() );
			}
			error += KineticVMCodeOG::recurseCompile( lft );
			error += KineticVMCodeOG::recurseCompile( rgt );
			emit( SUB );
		}
		else {
			int mulI = parenAwareSearcher( str, '*' );
			if( mulI >= 0 ) {
				error += splitter( str, mulI, lft, rgt );
				if( !lft ) {
					return 1; // error
				}

				error += KineticVMCodeOC::recurseCompile( lft );
				error += KineticVMCodeOG::recurseCompile( rgt );
				emit( MUL );

				error += KineticVMCodeOC::recurseCompile( rgt );
				error += KineticVMCodeOG::recurseCompile( lft );
				emit( MUL );

				emit( ADD );
			}
			else {
				int divI = parenAwareSearcher( str, '/' );
				if( divI >= 0 ) {
					error += splitter( str, divI, lft, rgt );
					if( !lft ) {
						return 1; // error
					}
					error += KineticVMCodeOC::recurseCompile( rgt );
					error += KineticVMCodeOG::recurseCompile( lft );
					emit( MUL );

					error += KineticVMCodeOC::recurseCompile( lft );
					error += KineticVMCodeOG::recurseCompile( rgt );
					emit( MUL );

					emit( SUB );

					error += KineticVMCodeOC::recurseCompile( rgt );
					error += KineticVMCodeOC::recurseCompile( rgt );
					emit( MUL );
					emit( DIV );
				}
				else {
					int reagent = -1, parameter = -1;
					double timeRef = 0.0;
					int _error = 0;
					
					int hasTime = hasTimeRef( str, reagent, parameter, timeRef, _error );
					error += _error;
					if( hasTime ) {
						emit( PUSH_TIMEREF_G );
						if( reagent >= 0 ) {
							emit( reagent );
						}
						else {
							error++;
						}
						
						emit( timeRef );
					}
					else {
						if( reagent >= 0 ) {
							// If the the symbol is a reagent name then we want to lookup
							// the G value of that reagent wrt to current parameter.
							// Because this code is shared wrt every parameter, the
							// execute code puts the current G vector into bank 0 hence the 0
							// in regIndexG below.
							emit( PUSH );
							emit( regIndexG(reagent,0) );
						}
						else if( parameter >= 0 ) {
							// If the symbol in the name of a parameter other than a reagent then things are more complicated
							// If wrt is this symbol then 1 otherwise 0 which is implemented by the CMP_WRT instruction
							emit( CMP_WRT );
							emit( parameter );
						}
						else {
							error++;
						}
					}
				}
			}
		}
	}

	if( lft ) {
		free( lft );
	}
	if( rgt ) {
		free( rgt );
	}

	return error;
}

KineticVMCodeOG::KineticVMCodeOG( KineticExperiment *_experiment, int _pCount )
 : KineticVMCodeOC( _experiment, _pCount )
{
	experiment = _experiment;
	
	// Note: it would be nice to call compile here but since compile requires
	// a virtualized recurse function it doesn't work since apparently
	// you can't be polymorphic in a constructor.
}


int KineticVMCodeOG::compile() {
	// This is mostly duplicate code from above but refactoring the code to eliminate this
	// smal redundancy seems uncalled for at the moment.

	bytecode.reset();
	bytecode.grow = 1024;

	bytecode.setCount( experiment->observablesCount() );

	int combinedErr = 0;

	for( int o=0; o<experiment->observableInstructions.count; o++ ) {
		emitStart( o );

		char *src = experiment->observableInstructions[o];

		// ELIM all whitespace
		int err = 0;
		char *dst = (char *)malloc( strlen(src) + 1 );
		char *d = dst;
		char *s = src;
		if( !strncmp( s, "[DERIV]", 7 ) ) {
			s += 7;
				// skip special command to use time-derivative of this obs
		}
		while( *s ) {
			if( *s != ' ' && *s != '\t' && *s != '\r' && *s != '\n' && (*s != '>' || s != src) ) {
				// (*s != '>' || s != src) : > is legal as first char only, but not compiled
				*d++ = *s;
			}
			s++;
		}
		*d = 0;

		err = recurseCompile( dst );
		free( dst );

		experiment->observableError[o] |= err;
			// note that we |= here instead of set; this preserves errors set in VMCodeOC::compile
		combinedErr |= err;

		emitStop( o );
	}

	return combinedErr;
}

void KineticVMCodeOG::execute_OG( int obs, int wrt, double *C, double *P, double *G, double *OG ) {
	regLoadC( C );
	regLoadP( P );

	// LOAD G into bank zero because this is always compiled against bank zero
	regLoadG( 0, G );

	*regWRT() = (double)wrt;

	// EXECUTE bytecode for OG
	executeExpressionBlock( &bytecode[0], obs, 1, OG );
}

void KineticVMCodeOG::disassemble( char *ext ) {
	KineticVMCodeOC::disassemble( ext ? ext : (char*)"og" );
}

// KineticVMCodeOD
//------------------------------------------------------------------------------------------------------------------------------------

KineticVMCodeOD::KineticVMCodeOD( KineticExperiment *_experiment, int _pCount )
 : KineticVMCodeOG( _experiment, _pCount )
{
}

int KineticVMCodeOD::recurseCompile( char *str ) {
	// Note this whole function except for the few lines at the 
	// end dealing with the reagentName of parameter name lookups
	// is otherwise identical to the KineticVMCodeOG code

	int error = 0;

	char *lft = 0;
	char *rgt = 0;

	int addI = parenAwareSearcher( str, '+' );
	if( addI >= 0 ) {
		error += splitter( str, addI, lft, rgt );
		if( !lft || !*lft ) {
			emit( PUSH );
			emit( regIndexConstZero() );
		}
		else {
			error += KineticVMCodeOD::recurseCompile( lft );
		}
		error += KineticVMCodeOD::recurseCompile( rgt );
		emit( ADD );
	}
	else {
		int minI = parenAwareSearcher( str, '-' );
		if( minI >= 0 ) {
			error += splitter( str, minI, lft, rgt );
			if( !lft || !*lft ) {
				emit( PUSH );
				emit( regIndexConstZero() );
			}
			else {
				error += KineticVMCodeOD::recurseCompile( lft );
			}
			error += KineticVMCodeOD::recurseCompile( rgt );
			emit( SUB );
		}
		else {
			int mulI = parenAwareSearcher( str, '*' );
			if( mulI >= 0 ) {
				error += splitter( str, mulI, lft, rgt );
				if( !lft ) {
					return 1; // error
				}

				error += KineticVMCodeOC::recurseCompile( lft );
				error += KineticVMCodeOD::recurseCompile( rgt );
				emit( MUL );

				error += KineticVMCodeOC::recurseCompile( rgt );
				error += KineticVMCodeOD::recurseCompile( lft );
				emit( MUL );

				emit( ADD );
			}
			else {
				int divI = parenAwareSearcher( str, '/' );
				if( divI >= 0 ) {
					error += splitter( str, divI, lft, rgt );
					if( !lft ) {
						return 1; // error
					}
					error += KineticVMCodeOC::recurseCompile( rgt );
					error += KineticVMCodeOD::recurseCompile( lft );
					emit( MUL );

					error += KineticVMCodeOC::recurseCompile( lft );
					error += KineticVMCodeOD::recurseCompile( rgt );
					emit( MUL );

					emit( SUB );

					error += KineticVMCodeOC::recurseCompile( rgt );
					error += KineticVMCodeOC::recurseCompile( rgt );
					emit( MUL );
					emit( DIV );
				}
				else {
					int logI = parenAwareSearcherStr( str, "log" );
					if( logI >= 0 ) {
						error += splitter( str, logI+2, lft, rgt );

						emit( PUSH );
						emit( regIndexConstOne() );

						emit( PUSH );
						emit( regIndexConstLn10() );

						error += KineticVMCodeOC::recurseCompile( rgt );
						emit( MUL );
						emit( DIV );

						error += KineticVMCodeOD::recurseCompile( rgt );
						emit( MUL );
					}
					else {
						int lnI = parenAwareSearcherStr( str, "ln" );
						if( lnI >= 0 ) {
							error += splitter( str, lnI+1, lft, rgt );

							emit( PUSH );
							emit( regIndexConstOne() );
							error += KineticVMCodeOC::recurseCompile( rgt );
							emit( DIV );

							error += KineticVMCodeOD::recurseCompile( rgt );
							emit( MUL );
						}
						else {
							int powI = parenAwareSearcherStr( str, "pow" );
							if( powI >= 0 ) {
								error += splitter( str, powI+2, lft, rgt );

								emit( PUSH );
								emit( regIndexConstLn10() );

								error += KineticVMCodeOC::recurseCompile( str );
								emit( MUL );

								error += KineticVMCodeOD::recurseCompile( rgt );
								emit( MUL );
							}
							else {
								int expI = parenAwareSearcherStr( str, "exp" );
								if( expI >= 0 ) {
									error += splitter( str, expI+2, lft, rgt );

									error += KineticVMCodeOC::recurseCompile( str );
									error += KineticVMCodeOD::recurseCompile( rgt );
									emit( MUL );
								}
								else {
									if( !strcmp( "VOLT", str ) ) {
										emit( PUSH );
										emit( regIndexConstZero() );
									}
									else if( !strcmp( "TEMP", str ) ) {
										emit( PUSH );
										emit( regIndexConstZero() );
									}
									else if( !strcmp( "PRES", str ) ) {
										emit( PUSH );
										emit( regIndexConstZero() );
									}
									else if( !strcmp( "SCONC", str ) ) {
										emit( PUSH );
										emit( regIndexConstZero() );
									}
									else if( !strcmp( "CONC", str ) ) {
										emit( PUSH );
										emit( regIndexConstZero() );
									}
									else {
										int reagent = -1, parameter = -1;
										double timeRef = 0.0;
										int _error = 0;
										
										int hasTime = hasTimeRef( str, reagent, parameter, timeRef, _error );
										error += _error;
										if( hasTime ) {
											emit( PUSH );
											emit( regIndexConstZero() );
										}
										else {
											if( reagent >= 0 ) {
												emit( PUSH );
												emit( regIndexD(reagent) );
											}
											else if( parameter >= 0 ) {
												emit( PUSH );
												emit( regIndexConstZero() );
											}
											else {
												error++;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if( lft ) {
		free( lft );
	}
	if( rgt ) {
		free( rgt );
	}

	return error;
}

void KineticVMCodeOD::execute_OD( double *C, double *P, double *D, double *OD ) {
	regLoadC( C );
	regLoadP( P );
	regLoadD( D );

	// EXECUTE bytecode for OD
	executeExpressionBlock( &bytecode[0], 0, experiment->observablesCount(), OD );
}

void KineticVMCodeOD::disassemble( char *ext ) {
	KineticVMCodeOC::disassemble( "od" );
}

// KineticIntegrator
//------------------------------------------------------------------------------------------------------------------------------------

KineticIntegrator::KineticIntegrator( int _whichIntegrator, KineticVMCodeD *_vmd, double _errAbs, double _errRel, KineticVMCodeH *_vmh ) {
	vmd = _vmd;
	vmh = _vmh;
	errAbs = _errAbs != 0 ? _errAbs : KI_DEFAULT_ERRABS;
	errRel = _errRel != 0 ? _errRel : KI_DEFAULT_ERRREL;
	wrtParameter = -1;
	whichIntegrator = _whichIntegrator;
	zIntegrator = 0;
}

KineticIntegrator::~KineticIntegrator() {
	if( zIntegrator ) {
		delete zIntegrator;
	}
}

// LU Adaptors
//----------------------------------

int KineticIntegrator::luDecomposeGSLAdaptor( double *matA, int dim, size_t *permutation, int *signNum ) {
	gsl_matrix_view gslMatA = gsl_matrix_view_array( matA, dim, dim );

	gsl_permutation gslPerm;
	gslPerm.size = dim;
	gslPerm.data = permutation;

	int err = gsl_linalg_LU_decomp( &gslMatA.matrix, &gslPerm, signNum );

	return err == GSL_SUCCESS ? 1 : 0;
}

int KineticIntegrator::luSolveGSLAdaptor( double *matU, int dim, size_t *permutation, double *B, double *X ) {
	gsl_matrix_view gslMatA = gsl_matrix_view_array( matU, dim, dim );

	gsl_permutation gslPerm;
	gslPerm.size = dim;
	gslPerm.data = permutation;

	gsl_vector_view gslB = gsl_vector_view_array( B, dim );
	gsl_vector_view gslX = gsl_vector_view_array( X, dim );

	int err = gsl_linalg_LU_solve( &gslMatA.matrix, &gslPerm, &gslB.vector, &gslX.vector );

	return err == GSL_SUCCESS ? 1 : 0;
}

// Adaptors
//----------------------------------
int KineticIntegrator::adapator_compute_D( double t, double *C, double *dC_dt, void *params ) {
	KineticIntegrator *_this = (KineticIntegrator *)params;
	return _this->compute_D( t, C, dC_dt );
}

int KineticIntegrator::adapator_compute_dD_dC( double t, double *C, double *dD_dC, double *dD_dx, void *params ) {
	KineticIntegrator *_this = (KineticIntegrator *)params;
	return _this->compute_dD_dC( t, C, dD_dC, dD_dx );
}

int KineticIntegrator::adapator_compute_H( double t, double *G, double *dG_dx, void *params ) {
	KineticIntegrator *_this = (KineticIntegrator *)params;
	return _this->compute_H( t, G, dG_dx );
}

int KineticIntegrator::adapator_compute_dH_dG( double t, double *G, double *dH_dG, double *dH_dx, void *params ) {
	KineticIntegrator *_this = (KineticIntegrator *)params;
	return _this->compute_dH_dG( t, G, dH_dG, dH_dx );
}

// Integrate functions
//----------------------------------

void KineticIntegrator::prepareD( double *icC, double *P, double endTime, double startTime ) {
	assert( vmd->isCompiled() );
	int reagentCount = vmd->system->reagentCount();

	// LOAD initial conditions & params if integrating from t=0
	if( startTime == 0.0 ) {
		traceC.clear();
		traceC.init( reagentCount );
	}

	// LOAD P as this may actually change across calls even if time is not zero
	// in the case of rate parameters dependent on changing temperatures or voltages.
	vmd->regLoadP( P );

	// STORE the first point
	double *D = (double *)alloca( sizeof(double) * reagentCount );
	compute_D( startTime, icC, D );
	traceC.add( startTime, icC, D );

	// FIND largest IC, INIT tolerances & step sizes
	double largestIC = 0.0;
	for( int i=0; i<reagentCount; i++ ) {
		largestIC = max( largestIC, icC[i] );
	}
	double errAbsAdjusted = max( 10.0*DBL_EPSILON, errAbs * largestIC );
	double initStep = ( endTime - startTime ) / 1000.0;
	double minStep  = ( endTime - startTime ) * 1e-10;

	// CREATE the integrator
	if( zIntegrator ) {
		delete zIntegrator;
	}
	switch( whichIntegrator ) {
		case KI_GSLRK45:
			zIntegrator = new ZIntegratorGSLRK45( 
				reagentCount, icC, startTime, endTime, errAbsAdjusted, errRel, initStep, minStep, 0,
				adapator_compute_D, this
			);
			break;

		case KI_GSLBS:
			zIntegrator = new ZIntegratorGSLBulirschStoerBaderDeuflhard( 
				reagentCount, icC, startTime, endTime, errAbsAdjusted, errRel, initStep, minStep, 0,
				adapator_compute_D, adapator_compute_dD_dC, this
			);
			break;

		case KI_ROSS:
			zIntegrator = new ZIntegratorRosenbrockStifflyStable( 
				reagentCount, icC, startTime, endTime, errAbsAdjusted, errRel, initStep, minStep, 0,
				adapator_compute_D, adapator_compute_dD_dC, this, 
				luDecomposeGSLAdaptor, luSolveGSLAdaptor
			);
			break;

		default:
			assert( !"Unsupported integator type" );
	}

	zIntegrator->firstEvolve = 0;
		// Because this prepare function always adds the first point,
		// we clear this flag so that the evolve won't also add the first point

}

int KineticIntegrator::stepD( double deltaTime ) {
	// TODO: alloc D in prepare D to avoid the stack alloc each time here?
	int reagentCount = vmd->system->reagentCount();
	double *D = (double *)alloca( sizeof(double) * reagentCount );
	int err = zIntegrator->step( deltaTime );
	compute_D( zIntegrator->x, zIntegrator->y, D );
	traceC.add( zIntegrator->x, zIntegrator->y, D );
	return err;
}

void KineticIntegrator::getDilutedC( double dilution, double *addC, double *dilutedC ) {
	int reagentCount = vmd->system->reagentCount();
	for( int i=0; i<reagentCount; i++ ) {
		dilutedC[i] = getC()[i] * dilution + addC[i];
	}
}

int KineticIntegrator::integrateD( double *icC, double *P, double endTime, double startTime, double stopIfStepSmallerThan, int *icFixed ) {
	assert( vmd->isCompiled() );
	int reagentCount = vmd->system->reagentCount();

	prepareD( icC, P, endTime, startTime );
	double *D = (double *)alloca( sizeof(double) * reagentCount );

	while( zIntegrator->evolve() == ZI_WORKING ) {
		// @TODO: handle errors from integrator

		compute_D( zIntegrator->x, zIntegrator->y, D );

		// There is likely a better way to hold concentrations fixed during integration,
		// but the parameterization infrastructure is complicated enough that reseting
		// to the icC value here is a reasonable approach to experiment with this feature.
		// tfb may 2013
		if( icFixed ) {
			for( int i=0; i<reagentCount; i++ ) {
				if( icFixed[i] ) {
					zIntegrator->y[i] = icC[i];
					D[i] = 0;
				}
			}
		}

		traceC.add( zIntegrator->x, zIntegrator->y, D );
		
		if( stopIfStepSmallerThan > 0.0 && zIntegrator->stepLast < stopIfStepSmallerThan ) {
			return 0;
		}
	}
	return 1;
}

void KineticIntegrator::allocTraceG() {
	traceG.clear();
	int pCount = vmh->pCount;
	int rCount = vmh->system->reagentCount();
	for( int p=0; p<pCount; p++ ) {
		KineticTrace *tg = new KineticTrace( rCount );
		traceG.add( tg );
	}
}

int hackDumpFlag = 0;

void KineticIntegrator::integrateH( int _wrtParameter, double *icG, double *P, double endTime ) {
	assert( vmh );
	assert( vmh->isCompiled() );

	int reagentCount = vmh->system->reagentCount();
	wrtParameter = _wrtParameter;

	assert( traceC.rows == reagentCount && traceC.cols > 0 );
		// The integrateD must have been called already

	traceC.polyFit();
// @TODO: Confirm that it hasn't alrady been polyfit?

	// LOAD parameters
	vmh->regLoadP( P );

	// STORE the first point
	double *H = (double *)alloca( sizeof(double) * reagentCount );

	switch( whichIntegrator ) {
		case KI_GSLRK45:
			zIntegrator = new ZIntegratorGSLRK45( 
				reagentCount, icG, 0.0, endTime, 1e-6, 1e-6, 1e-3, 1e-6, 0,
// @TODO: I need to set these error terms to more reasonable scales like we did in D
				adapator_compute_H, this
			);
			break;

		case KI_GSLBS:
			zIntegrator = new ZIntegratorGSLBulirschStoerBaderDeuflhard( 
				reagentCount, icG, 0.0, endTime, 1e-6, 1e-6, 1e-3, 1e-6, 0,
				adapator_compute_H, adapator_compute_dH_dG, this
			);
			break;

		case KI_ROSS:
			zIntegrator = new ZIntegratorRosenbrockStifflyStable( 
				reagentCount, icG, 0.0, endTime, 1e-6, 1e-6, 1e-3, 1e-6, 0,
				adapator_compute_H, adapator_compute_dH_dG, this, 
				luDecomposeGSLAdaptor, luSolveGSLAdaptor
			);
			break;

		default:
			assert( !"Unsupported integator type" );
	}

	while( zIntegrator->evolve() == ZI_WORKING ) {
		compute_H( zIntegrator->x, zIntegrator->y, H );

//if( hackDumpFlag ) {
//	static FILE *hackDumpFile = 0;
//	if( !hackDumpFile) {
//		hackDumpFile = fopen( "aode_hack_gh.txt", "wt" );
//	}
//	fprintf( hackDumpFile, "%lf %lf %lf\n", zIntegrator->x, zIntegrator->y[0], H[0] );
//	fflush( hackDumpFile );
//}

		traceG[wrtParameter]->add( zIntegrator->x, zIntegrator->y, H );
	}
}

// Compute functions
//----------------------------------

int KineticIntegrator::compute_D( double t, double *C, double *dC_dt ) {
	vmd->execute_D( C, dC_dt );
	return 0;
}

int KineticIntegrator::compute_dD_dC( double t, double *C, double *dD_dC, double *dD_dt ) {
	int reagentCount = vmd->system->reagentCount();
	vmd->execute_dD_dC( C, dD_dC );
	memset( dD_dt, 0, sizeof(double) * reagentCount );
		// dD_dt is the partial derivative of dD_dC wrt t and it is zero in this case
		// because the base sytem is invariant on t
	return 0;
}

int KineticIntegrator::compute_H( double t, double *G, double *dG_dt ) {
	int reagentCount = vmh->system->reagentCount();
	double *slerpedC = (double *)alloca( sizeof(double) * reagentCount );
	traceC.getColSLERP( t, slerpedC );
	vmh->execute_H( wrtParameter, slerpedC, G, dG_dt );
	return 0;
}

int KineticIntegrator::compute_dH_dG( double t, double *G, double *dH_dG, double *dH_dt ) {
	// INTERPOLATE traceC to solve for the reagent concentrations at this time
	int reagentCount = vmh->system->reagentCount();
	double *slerpedC = (double *)alloca( sizeof(double) * reagentCount );
	traceC.getColSLERP( t, slerpedC );

	double *D = (double *)alloca( sizeof(double) * reagentCount );
	vmd->execute_D( slerpedC, D );

	vmh->execute_dH_dG( wrtParameter, slerpedC, D, G, dH_dG, dH_dt );

	return 0;
}


/*
// Test code for simple analytic system

int KineticIntegrator::compute_D( double t, double *C, double *dC_dt ) {
	dC_dt[0] = - 10.0 * C[0];
	dC_dt[1] = + 10.0 * C[0];

	return 1;
}


int KineticIntegrator::compute_dD_dC( double t, double *C, double *dD_dC, double *dD_dt ) {
	dD_dC[ 0*2 + 0 ] = -10.0;
	dD_dC[ 0*2 + 1 ] = +10.0;
	dD_dC[ 1*2 + 0 ] =   0.0;
	dD_dC[ 1*2 + 1 ] =   0.0;

	dD_dt[0] = 0.0;
	dD_dt[1] = 0.0;

	return 1;
}


int KineticIntegrator::compute_H( double t, double *G, double *dG_dt ) {
	double C[2];
	C[0] = 0.0 + exp( - 10.0 * t );

	dG_dt[0] = -C[0] - 10.0 * G[0];
	dG_dt[1] = +C[0] + 10.0 * G[0];

	return 1;
}


int KineticIntegrator::compute_dH_dG( double t, double *G, double *dH_dG, double *dH_dt ) {
	dH_dG[ 0*2 + 0 ] = -10.0;
	dH_dG[ 0*2 + 1 ] = +10.0;
	dH_dG[ 1*2 + 0 ] =   0.0;
	dH_dG[ 1*2 + 1 ] =   0.0;

	double D[2];
	D[0] = -10.0 * exp( -10.0 * t );

	dH_dt[0] = -D[0];
	dH_dt[1] = +D[0];

	return 1;
}

*/
