// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Provides a varierty of random math tools
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zmathtools.cpp zmathtools.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "stdlib.h"
#include "math.h"
#include "string.h"
#include "float.h"
#ifdef WIN32
#include "malloc.h"
#endif
// MODULE includes:
// ZBSLIB includes:
#include "zvec.h"
#include "zmathtools.h"

int zmathIsPow2( int x ) {
	if( x == 0 ) return 1;
	int count = 0;
	for( int i=0; i<32; i++ ) {
		count += x&1;
		x >>= 1;
	}
	return( count == 1 );
}

int zmathNextPow2( int x ) {
	int p = 2;
	for( int i=1; i<31; i++ ) {
		if( p >= x ) {
			return p;
		}
		p *= 2;
	}
	return 0;
}

double zmathDegToRadD( double d ) {
	return PI2 * (d/360.0);
}

double zmathRadToDegD( double d ) {
	return (d / PI2) * 360.0;
}

float zmathDegToRadF( float d ) {
	return PI2F * (d/360.f);
}

float zmathRadToDegF( float d ) {
	return (d / PI2F) * 360.f;
}

int zmathRoundToInt( double x ) {
	double f = floor(x);
	if( x - f < 0.5 ) return (int)f;
	return (int)f + 1;
}

int zmathRoundToInt( float x ) {
	float f = (float)floor(x);
	if( x - f < 0.5f ) return (int)f;
	return (int)f + 1;
}

double zmathMedianI( int *hist, int w, int numSamples ) {
	if( numSamples == -1 ) {
		numSamples = 0;
		for( int x=0; x<w; x++ ) {
			numSamples += hist[x];
		}
	}

	double med = 0.0;
	double half = (double)numSamples / 2.0;
	for( int x=0; x<w; x++ ) {
		med += (double)hist[x];
		if( med >= half ) {
			// This sample contains the median; subdivide this sample accordingly
			return (double)x + ( ( half - (med - (double)hist[x]) ) / (double)hist[x] );
		}
	}
	return 0.0;
}

double zmathEnclosedMean( int *hist, int w, int med, double percent ) {
	int sampleSum = hist[med];
	int sum = med * hist[med];
	int samples = 0;

	int x;
	for( x=0; x<w; x++ ) {
		samples += hist[x];
	}

	int stopAt = (int)( (double)samples * percent );
		
	for( x=1; x<w; x++ ) {
		int l = med - x;
		int r = med + x;
		if( l >= 0 ) {
			sampleSum += hist[l];
			sum       += hist[l] * l;
		}
		if( r < w ) {
			sampleSum += hist[r];
			sum       += hist[r] * r;
		}
		if( sampleSum > stopAt ) {
			return (double)sum / (double)sampleSum;
		}
	}
	return 0.0;
}

int doubleCompare( const void *a, const void *b ) {
	double c = *(double *)a - *(double *)b;
	if( c < 0.0 ) return -1;
	if( c > 0.0 ) return 1;
	return 0;
}

int floatCompare( const void *a, const void *b ) {
	float c = *(float *)a - *(float *)b;
	if( c < 0.f ) return -1;
	if( c > 0.f ) return 1;
	return 0;
}

int intCompare( const void *a, const void *b ) {
	return *(int *)a - *(int *)b;
}

int intCompareDescending( const void *a, const void *b ) {
	return *(int *)b - *(int *)a;
}


void zmathStatsD( double *series, double *sorted, int count, double *mean, double *median, double *stddev ) {
	if( count == 0 ) {
		if( mean ) {
			*mean = 0;
		}
		if( median ) {
			*median = 0;
		}
		if( stddev ) {
			*stddev = 0;
		}
		return;
	}

	if( median || sorted ) {
		if( !sorted ) {
			sorted = (double *)alloca( count * sizeof(double) );
		}
		memcpy( sorted, series, count * sizeof(double) );
		qsort( sorted, count, sizeof(double), doubleCompare );
		if( count&1 ) {
			*median = (double)sorted[count/2];
		}
		else {
			*median = ((double)sorted[count/2] + (float)sorted[count/2-1]) / 2.0;
		}
	}

	double _mean;
	if( mean || stddev ) {
		double sum = 0.0;
		for( int i=0; i<count; i++ ) {
			sum += (double)series[i];
		}
		_mean = sum / (double)count;
		if( mean ) {
			*mean = (double)_mean;
		}
	}
	if( stddev ) {
		double sum = 0.0;
		for( int i=0; i<count; i++ ) {
			double d = (double)series[i] - _mean;
			sum += d*d;
		}
		sum /= count>1 ? count-1 : 1;
		*stddev = (double)sqrt( sum );
	}
}

void zmathStatsF( float *series, float *sorted, int count, float *mean, float *median, float *stddev ) {
	if( count == 0 ) {
		if( mean ) {
			*mean = 0;
		}
		if( median ) {
			*median = 0;
		}
		if( stddev ) {
			*stddev = 0;
		}
		return;
	}

	if( median || sorted ) {
		if( !sorted ) {
			sorted = (float *)alloca( count * sizeof(float) );
		}
		memcpy( sorted, series, count * sizeof(float) );
		qsort( sorted, count, sizeof(float), floatCompare );
		if( count&1 ) {
			*median = (float)sorted[count/2];
		}
		else {
			*median = ((float)sorted[count/2] + (float)sorted[count/2-1]) / 2.f;
		}
	}

	double _mean;
	if( mean || stddev ) {
		double sum = 0.0;
		for( int i=0; i<count; i++ ) {
			sum += (double)series[i];
		}
		_mean = sum / (double)count;
		if( mean ) {
			*mean = (float)_mean;
		}
	}
	if( stddev ) {
		double sum = 0.0;
		for( int i=0; i<count; i++ ) {
			double d = (double)series[i] - _mean;
			sum += d*d;
		}
		sum /= count>1 ? count-1 : 1;
		*stddev = (float)sqrt( sum );
	}
}

void zmathStatsI( int *series, int *sorted, int count, float *mean, float *median, float *stddev ) {
	if( count == 0 ) {
		if( mean ) {
			*mean = 0;
		}
		if( median ) {
			*median = 0;
		}
		if( stddev ) {
			*stddev = 0;
		}
	}

	if( median || sorted ) {
		if( !sorted ) {
			sorted = (int *)alloca( count * sizeof(int) );
		}
		memcpy( sorted, series, count * sizeof(int) );
		qsort( sorted, count, sizeof(int), intCompare );
		if( count&1 ) {
			*median = (float)sorted[count/2];
		}
		else {
			*median = ((float)sorted[count/2] + (float)sorted[count/2-1]) / 2.f;
		}
	}

	double _mean;
	if( mean || stddev ) {
		double sum = 0.0;
		for( int i=0; i<count; i++ ) {
			sum += (double)series[i];
		}
		_mean = sum / (double)count;
		if( mean ) {
			*mean = (float)_mean;
		}
	}
	if( stddev ) {
		double sum = 0.0;
		for( int i=0; i<count; i++ ) {
			double d = (double)series[i] - _mean;
			sum += d*d;
		}
		sum /= count>1 ? count-1 : 1;
		*stddev = (float)sqrt( sum );
	}
}

void zmathStatsUB( unsigned char *series, int *sorted, int count, float *mean, float *median, float *stddev ) {
	if( median || sorted ) {
		if( !sorted ) {
			sorted = (int *)alloca( count * sizeof(unsigned char) );
		}
		memcpy( sorted, series, count * sizeof(unsigned char) );
		qsort( sorted, count, sizeof(unsigned char), floatCompare );
		if( count&1 ) {
			*median = (float)sorted[count/2];
		}
		else {
			*median = ((float)sorted[count/2] + (float)sorted[count/2-1]) / 2.f;
		}
	}

	double _mean;
	if( mean || stddev ) {
		double sum = 0.0;
		for( int i=0; i<count; i++ ) {
			sum += (double)series[i];
		}
		_mean = sum / (double)count;
		if( mean ) {
			*mean = (float)_mean;
		}
	}
	if( stddev ) {
		double sum = 0.0;
		for( int i=0; i<count; i++ ) {
			double d = (double)series[i] - _mean;
			sum += d*d;
		}
		sum /= count>1 ? count-1 : 1;
		*stddev = (float)sqrt( sum );
	}
}

void zmathStatsSB( signed char *series, int *sorted, int count, float *mean, float *median, float *stddev ) {
	if( median || sorted ) {
		if( !sorted ) {
			sorted = (int *)alloca( count * sizeof(signed char) );
		}
		memcpy( sorted, series, count * sizeof(signed char) );
		qsort( sorted, count, sizeof(signed char), floatCompare );
		if( count&1 ) {
			*median = (float)sorted[count/2];
		}
		else {
			*median = ((float)sorted[count/2] + (float)sorted[count/2-1]) / 2.f;
		}
	}

	double _mean;
	if( mean || stddev ) {
		double sum = 0.0;
		for( int i=0; i<count; i++ ) {
			sum += (double)series[i];
		}
		_mean = sum / (double)count;
		if( mean ) {
			*mean = (float)_mean;
		}
	}
	if( stddev ) {
		double sum = 0.0;
		for( int i=0; i<count; i++ ) {
			double d = (double)series[i] - _mean;
			sum += d*d;
		}
		sum /= count>1 ? count-1 : 1;
		*stddev = (float)sqrt( sum );
	}
}

void zmathStatsUS( unsigned short *series, int *sorted, int count, float *mean, float *median, float *stddev ) {
	if( median || sorted ) {
		if( !sorted ) {
			sorted = (int *)alloca( count * sizeof(unsigned short) );
		}
		memcpy( sorted, series, count * sizeof(unsigned short) );
		qsort( sorted, count, sizeof(unsigned short), floatCompare );
		if( count&1 ) {
			*median = (float)sorted[count/2];
		}
		else {
			*median = ((float)sorted[count/2] + (float)sorted[count/2-1]) / 2.f;
		}
	}

	double _mean;
	if( mean || stddev ) {
		double sum = 0.0;
		for( int i=0; i<count; i++ ) {
			sum += (double)series[i];
		}
		_mean = sum / (double)count;
		if( mean ) {
			*mean = (float)_mean;
		}
	}
	if( stddev ) {
		double sum = 0.0;
		for( int i=0; i<count; i++ ) {
			double d = (double)series[i] - _mean;
			sum += d*d;
		}
		sum /= count>1 ? count-1 : 1;
		*stddev = (float)sqrt( sum );
	}
}

void zmathStatsSS( signed short *series, int *sorted, int count, float *mean, float *median, float *stddev ) {
	if( median || sorted ) {
		if( !sorted ) {
			sorted = (int *)alloca( count * sizeof(signed short) );
		}
		memcpy( sorted, series, count * sizeof(signed short) );
		qsort( sorted, count, sizeof(signed short), floatCompare );
		if( count&1 ) {
			*median = (float)sorted[count/2];
		}
		else {
			*median = ((float)sorted[count/2] + (float)sorted[count/2-1]) / 2.f;
		}
	}

	double _mean;
	if( mean || stddev ) {
		double sum = 0.0;
		for( int i=0; i<count; i++ ) {
			sum += (double)series[i];
		}
		_mean = sum / (double)count;
		if( mean ) {
			*mean = (float)_mean;
		}
	}
	if( stddev ) {
		double sum = 0.0;
		for( int i=0; i<count; i++ ) {
			double d = (double)series[i] - _mean;
			sum += d*d;
		}
		sum /= count>1 ? count-1 : 1;
		*stddev = (float)sqrt( sum );
	}
}

double zmathPearsonI( int *seriesX, int *seriesY, int count ) {
	double x = 0.0;
	double y = 0.0;
	double xy = 0.0;
	double xx = 0.0;
	double yy = 0.0;

	int *xi = seriesX;
	int *yi = seriesY;
	for( int i=0; i<count; i++ ) {
		double xid = (double)*xi;
		double yid = (double)*yi;

		x += xid;
		y += yid;

		xy += xid * yid;
		xx += xid * xid;
		yy += yid * yid;

		xi++;
		yi++;
	}

	double c = (double)count;
	double corr = (c * xy - x * y) / sqrt( (c * xx - x * x) * (c * yy - y * y) );
	return corr;
}

double zmathPearsonD( double *seriesX, double *seriesY, int count ) {
	double x = 0.0;
	double y = 0.0;
	double xy = 0.0;
	double xx = 0.0;
	double yy = 0.0;

	double *xi = seriesX;
	double *yi = seriesY;
	for( int i=0; i<count; i++ ) {
		x += *xi;
		y += *yi;

		xy += *xi * *yi;
		xx += *xi * *xi;
		yy += *yi * *yi;

		xi++;
		yi++;
	}

	double c = (double)count;
	double corr = (c * xy - x * y) / sqrt( (c * xx - x * x) * (c * yy - y * y) );
	return corr;
}


float zmathPearsonF( float *seriesX, float *seriesY, int count ) {
	float x = 0.f;
	float y = 0.f;
	float xy = 0.f;
	float xx = 0.f;
	float yy = 0.f;

	float *xi = seriesX;
	float *yi = seriesY;
	for( int i=0; i<count; i++ ) {
		x += *xi;
		y += *yi;

		xy += *xi * *yi;
		xx += *xi * *xi;
		yy += *yi * *yi;

		xi++;
		yi++;
	}

	float c = (float)count;
	float corr = (c * xy - x * y) / sqrtf( (c * xx - x * x) * (c * yy - y * y) );
	return corr;
}


DVec2 zmathWorldCoordToNormalizedScreenD( DMat4 &projMat, DVec3 world ) {
	DVec4 v0( world.x, world.y, world.z, 1.0 );
	v0 = projMat.mul( v0 );
	DVec2 v1 = DVec2( (double)v0.x/(double)v0.w, (double)v0.y/(double)v0.w );
	v1.x = ((double)v1.x+1.0) / 2.0;
	v1.y = ((double)v1.y+1.0) / 2.0;
	return v1;
}

FVec2 zmathWorldCoordToNormalizedScreenF( FMat4 &projMat, FVec3 world ) {
	FVec4 v0( world.x, world.y, world.z, 1.f );
	v0 = projMat.mul( v0 );
	FVec2 v1 = FVec2( (float)v0.x/(float)v0.w, (float)v0.y/(float)v0.w );
	v1.x = ((float)v1.x+1.0f) / 2.0f;
	v1.y = ((float)v1.y+1.0f) / 2.0f;
	return v1;
}

int zmathLinearRegression2f( FVec2 *samples, int sampleCount, float &b, float &m, float &r ) {
	float sx = 0.f, sy = 0.f, sxy = 0.f, sxx = 0.f, syy = 0.f;
	FVec2 *s = samples;
	for( int i=0; i<sampleCount; i++, s++ ) {
		sx  += s->x;
		sy  += s->y;
		sxy += s->x * s->y;
		sxx += s->x * s->x;
		syy += s->y * s->y;
	}
	if( sampleCount <= 0 || sx==0 || sxx==0 || sy==0 || syy==0 ) return 0;
		// I'm not sure if this is the correct error check or not.

	float n = (float)sampleCount;
	m = (n * sxy - sx*sy) / (n * sxx - sx*sx);
	b = sy / n - m * sx / n;
	r = ( n*sxy - sx*sy ) / ( (float)sqrt( n*sxx - sx*sx ) * (float)sqrt( n*syy - sy*sy ) );
	return 1;
}

FVec3 zmathLinePlaneIntersect( FVec3 p0, FVec3 norm, FVec3 p1, FVec3 p2 ) {
	FVec3 p0minusp1 = p0;
	p0minusp1.sub( p1 );
	FVec3 p2Minusp1 = p2;
	p2Minusp1.sub( p1 );
	float numerator   = norm.dot( p0minusp1 );
	float denominator = norm.dot( p2Minusp1 );
	// @todo: num or denom could be zero
	if( fabsf(denominator) > FLT_EPSILON ) {
		p2Minusp1.mul( numerator / denominator );
	}
	else {
		return FVec3::Origin;
	}
	p2Minusp1.add( p1 );
	return p2Minusp1;
}

FVec3 zmathClosestApproach( int n, FVec3 _p[], int pStrideInBytes, FVec3 _d[], int dStrideInBytes ) {
	float A=0.f, B=0.f, C=0.f, D=0.f, E=0.f, F=0.f, G=0.f, H=0.f, I=0.f;
	float J=0.f, K=0.f, L=0.f, O=0.f, P=0.f, R=0.f, S=0.f, T=0.f, U=0.f;

	FVec3 *p = _p;
	FVec3 *d = _d;

	for( int i=0; i<n; i++ ) {
		d->normalize();
		A += d->x * d->x;
		B += d->y * d->y;
		C += d->z * d->z;
		D += d->x * d->y;
		E += d->x * d->z;
		F += d->y * d->z;
		G += d->x * d->x * p->y;
		H += d->x * d->x * p->z;
		I += d->y * d->y * p->x;
		J += d->y * d->y * p->z;
		K += d->x * d->y * p->x;
		L += d->x * d->y * p->y;
		O += d->y * d->z * p->y;
		P += d->y * d->z * p->z;
		R += d->x * d->z * p->z;
		S += d->x * d->z * p->x;
		T += d->z * d->z * p->y;
		U += d->z * d->z * p->x;

		char *__d = (char *)d;
		__d += dStrideInBytes;
		d = (FVec3 *)__d;

		char *__p = (char *)p;
		__p += pStrideInBytes;
		p = (FVec3 *)__p;
	}

	float D2 = D*D;
	float E2 = E*E;
	float F2 = F*F;
	float n2 = (float)n*(float)n;

	float denom = -B*n2+n2*n+E2*B+F2*A-E2*n-D2*n+D2*C+B*A*n+B*n*C-2*D*F*E-B*A*C-n2*C-n2*A+n*A*C-F2*n;
	FVec3 s;
	s.x = -(-F2*L+F2*U+I*F2-F2*R-n2*U+n2*R+n2*L-I*n2-n*B*R+I*n*C+n*U*C-D*C*P+D*C*T+D*C*G-D*C*K-U*B*C+L*B*C+R*B*C-n*L*C-n*R*C-F*E*T-F*E*G+F*E*K+n*E*S+I*n*B+n*B*U-E*B*O+E*B*J+E*B*H-E*B*S-n*B*L+n*D*K+n*E*O-n*E*J-n*E*H+F*E*P-F*D*J-F*D*H+F*D*S+F*D*O-I*B*C+n*D*P-n
*D*T-n*D*G)/denom;
	s.y = (-A*C*K-A*C*P+A*C*T+A*C*G+n*C*K+I*D*n+A*F*O+A*n*P-A*n*T-A*n*G-A*F*J-A*F*H+A*F*S+n*C*P-n*C*T-n*C*G-U*D*C+R*D*C+L*D*C-n2*K+A*n*K+n*F*J+n*F*H-n*F*S-n*F*O+U*D*n-R*D*n-L*D*n+I*E*F+E*F*U-E*F*R-L*F*E+E*D*J+E*D*H-E*D*S-E*D*O-E2*G+E2*P-I*D*C-E2*T+n2*T+E2*K-
n2*P+n2*G)/denom;
	s.z = -(n2*O+n2*S-D2*S-n2*J-n2*H+D2*J-D2*O+D2*H+B*A*O-n*A*S-n*A*O+D*F*R+D*E*K+D*E*P-D*E*T-D*E*G+E*n*R+E*n*L-E*n*U+B*n*J+B*n*H+I*E*B-E*B*L+E*B*U-E*B*R-B*n*S-B*n*O+n*A*J+n*A*H+F*A*G+F*A*T-F*A*K-F*A*P-B*A*J-B*A*H+B*A*S+D*F*L-F*n*G-F*n*T+F*n*K+F*n*P-D*F*U-I*
E*n-I*D*F)/denom;
	return s;
}

float zmathAngleNormalizeF( float a ) {
	while( a > PI2F ) a -= PI2F;
	while( a < 0.0f ) a += PI2F;
	return a;
}

double zmathAngleNormalizeD( double a ) {
	while( a > PI2 ) a -= PI2;
	while( a < 0.0 ) a += PI2;
	return a;
}

float zmathPointPlaneCollF( FVec3 &p, FVec3 &p0, FVec3 &n ) {
	FVec3 d = p;
	d.sub( p0 );
	float dot = d.dot( n );
	if( dot < 0.f ) {
		return -dot;
	}
	return 0.f;
}

double zmathPointPlaneCollD( DVec3 &p, DVec3 &p0, DVec3 &n ) {
	DVec3 d = p;
	d.sub( p0 );
	double dot = d.dot( n );
	if( dot < 0.0 ) {
		return -dot;
	}
	return 0.0;
}

double zmathSpherePolyCollD(
	DVec3 &_pos, DVec3 &p0, DVec3 &n, double radius, 
	int edgeCount, DVec3 *edgesP0, DVec3 *edgesN, DVec3 &collNormOUT
) {
	// @TODO: Untested
	DVec3 pos = _pos;
	pos.sub( p0 );
	double d = pos.dot( n );
	double pen = d - radius;
	double r2 = radius * 2.0;
	int nearEdge = -1;
	if( pen < 0.0 && pen > -r2 ) {
		// We penetrate the plane, are we inside its bounds?
		for( int i=0; i<edgeCount; i++ ) {
			DVec3 p = _pos;
			p.sub( edgesP0[i] );
				// p is the vector from the edge vertex to the center of sphere
			double dot = p.dot( edgesN[i] );
			if( dot < -radius ) {
				// We are way far enough away from this edge that
				// we can't possibly be touching the polygon.
				return 0.0;
			}
			else if( dot < 0.0 ) {
				// The center is outside the polygon but we are still within
				// one radius so it is possible that we are touching.
				// Note this edge and come back to it once we are sure we're
				// not far outside of some other edge
				nearEdge = i;
			}
		}

		if( nearEdge >= 0 ) {
			// Near some edge, need to compute the distance between the edge 
			// and the sphere center
			DVec3 p = _pos;
			p.sub( edgesP0[nearEdge] );
				// p is the vector from the edge vertex to the center of sphere
			double pMag = p.mag();
			DVec3 normalizedP = p;
			normalizedP.div( pMag );
			DVec3 edge = edgesP0[(nearEdge+1)%edgeCount];
			edge.sub( edgesP0[nearEdge] );
			edge.normalize();
				// Edge is now a normalized vector in direction of edge
			double cosine = normalizedP.dot( edge );
				// The cosine between the edge vector and the p vector
			edge.mul( pMag * cosine );
				// Edge now a vector to the point along the edge which is closest to center of sphere
			p.sub( edge );
			pen = p.mag();
			p.div( pen );
			collNormOUT = p;
				// Now a normalized vector from nearest collision point to center of ball
			return pen - radius;
		}

		// Yes, fully inside of the polygon, collNorm is same as surface normal
		collNormOUT = n;
		return pen;
	}
	// Nowhere near this polygon
	return 0.0;
}


FVec3 zmathPointToLineSegment( FVec3 p0, FVec3 dir, FVec3 p ) {
	FVec3 pMinusP0 = p;
	pMinusP0.sub( p0 );
	float denom = dir.dot(dir);
//	if( fabs(denom) < 10.f*FLT_EPSILON ) {
//		return FVec3( 0.f, 0.f, 0.f );
//	}
	float t = dir.dot( pMinusP0 ) / denom;

	if( t < 0.f ) {
		p.sub( p0 );
		return p;
	}
	else if( t > 1.f ) {
		p0.add( dir );
		p.sub( p0 );
		return p;
	}

	dir.mul( t );
	dir.add( p0 );
	p.sub( dir );

	return p;
}


DMat4 zmathODERotPosToDMat4( double *rot, double *pos ) {
	DMat4 m;
	m.m[0][0] = rot[0];
	m.m[0][1] = rot[4];
	m.m[0][2] = rot[8];
	m.m[0][3] = 0.;

	m.m[1][0] = rot[1];
	m.m[1][1] = rot[5];
	m.m[1][2] = rot[9];
	m.m[1][3] = 0.;

	m.m[2][0] = rot[2];
	m.m[2][1] = rot[6];
	m.m[2][2] = rot[10];
	m.m[2][3] = 0.;

	m.m[3][0] = pos[0];
	m.m[3][1] = pos[1];
	m.m[3][2] = pos[2];
	m.m[3][3] = 1.;

	return m;
}

FMat4 zmathODERotPosToFMat4( double *rot, double *pos ) {
	FMat4 m;
	m.m[0][0] = (float)rot[0];
	m.m[0][1] = (float)rot[4];
	m.m[0][2] = (float)rot[8];
	m.m[0][3] = 0.f;

	m.m[1][0] = (float)rot[1];
	m.m[1][1] = (float)rot[5];
	m.m[1][2] = (float)rot[9];
	m.m[1][3] = 0.f;

	m.m[2][0] = (float)rot[2];
	m.m[2][1] = (float)rot[6];
	m.m[2][2] = (float)rot[10];
	m.m[2][3] = 0.f;

	m.m[3][0] = (float)pos[0];
	m.m[3][1] = (float)pos[1];
	m.m[3][2] = (float)pos[2];
	m.m[3][3] = 1.f;

	return m;
}

void zmathFMat4ToODE( FMat4 &m, double r[4*3] ) {
	r[0] = m.m[0][0];
	r[1] = m.m[1][0];
	r[2] = m.m[2][0];
	r[3] = 0.f;

	r[4] = m.m[0][1];
	r[5] = m.m[1][1];
	r[6] = m.m[2][1];
	r[7] = 0.f;

	r[8] = m.m[0][2];
	r[9] = m.m[1][2];
	r[10] = m.m[2][2];
	r[11] = 0.f;
}

void zmathDMat4ToODE( DMat4 &m, double r[4*3] ) {
	r[0] = m.m[0][0];
	r[1] = m.m[1][0];
	r[2] = m.m[2][0];
	r[3] = 0.f;

	r[4] = m.m[0][1];
	r[5] = m.m[1][1];
	r[6] = m.m[2][1];
	r[7] = 0.f;

	r[8] = m.m[0][2];
	r[9] = m.m[1][2];
	r[10] = m.m[2][2];
	r[11] = 0.f;
}

FQuat zmathODEQuatToFQaut( double *odeQuat ) {
	FQuat q;
	q.q[0] = (float)odeQuat[1];
	q.q[1] = (float)odeQuat[2];
	q.q[2] = (float)odeQuat[3];
	q.q[3] = (float)odeQuat[0];
	return q;
}

void zmath2DCubicSpline( FVec3 p1, FVec3 p2, FVec3 p1d, FVec3 p2d, FVec2 result[4] ) {
	// Solve for the 2D spline coef.  The z value of the verts is ignored.
	FVec2 a, b, c, d;
	a.x =   2*p1.x +  -2*p2.x +   1*p1d.x +   1*p2d.x;
	a.y =   2*p1.y +  -2*p2.y +   1*p1d.y +   1*p2d.y;

	b.x =  -9*p1.x +   9*p2.x +  -5*p1d.x +  -4*p2d.x;
	b.y =  -9*p1.y +   9*p2.y +  -5*p1d.y +  -4*p2d.y;

	c.x =  12*p1.x + -12*p2.x +   8*p1d.x +   5*p2d.x;
	c.y =  12*p1.y + -12*p2.y +   8*p1d.y +   5*p2d.y;

	d.x =  -4*p1.x +   5*p2.x +  -4*p1d.x +  -2*p2d.x;
	d.y =  -4*p1.y +   5*p2.y +  -4*p1d.y +  -2*p2d.y;

	result[0] = a;
	result[1] = b;
	result[2] = c;
	result[3] = d;
}

void zmathQuatPosToMat4( float quat[4], float pos[3], FMat4 &mat ) {
	// Quat to matrix code taken from http://www.gamasutra.com/features/19980703/quaternions_01.htm

	float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;
	x2 = quat[0] + quat[0];
	y2 = quat[1] + quat[1];
	z2 = quat[2] + quat[2];
	xx = quat[0] * x2; xy = quat[0] * y2; xz = quat[0] * z2;
	yy = quat[1] * y2; yz = quat[1] * z2; zz = quat[2] * z2;
	wx = quat[3] * x2; wy = quat[3] * y2; wz = quat[3] * z2;

	mat.m[0][0] = 1.0f - (yy + zz);
	mat.m[0][1] = xy - wz;
	mat.m[0][2] = xz + wy;
	mat.m[0][3] = 0.0f;

	mat.m[1][0] = xy + wz;
	mat.m[1][1] = 1.0f - (xx + zz);
	mat.m[1][2] = yz - wx;
	mat.m[1][3] = 0.0f;

	mat.m[2][0] = xz - wy;
	mat.m[2][1] = yz + wx;
	mat.m[2][2] = 1.0f - (xx + yy);
	mat.m[2][3] = 0.0f;

	mat.m[3][0] = (float)pos[0];
	mat.m[3][1] = (float)pos[1];
	mat.m[3][2] = (float)pos[2];
	mat.m[3][3] = 1;
}

void zmathQuatToMat3( float quat[4], FMat3 &mat ) {
	// Quat to matrix code taken from http://www.gamasutra.com/features/19980703/quaternions_01.htm

	float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;
	x2 = quat[0] + quat[0];
	y2 = quat[1] + quat[1];
	z2 = quat[2] + quat[2];
	xx = quat[0] * x2; xy = quat[0] * y2; xz = quat[0] * z2;
	yy = quat[1] * y2; yz = quat[1] * z2; zz = quat[2] * z2;
	wx = quat[3] * x2; wy = quat[3] * y2; wz = quat[3] * z2;

	mat.m[0][0] = 1.0f - (yy + zz);
	mat.m[0][1] = xy - wz;
	mat.m[0][2] = xz + wy;

	mat.m[1][0] = xy + wz;
	mat.m[1][1] = 1.0f - (xx + zz);
	mat.m[1][2] = yz - wx;

	mat.m[2][0] = xz - wy;
	mat.m[2][1] = yz + wx;
	mat.m[2][2] = 1.0f - (xx + yy);
}

float zmathDistancePointToLine( FVec3 p0, FVec3 p1, FVec3 p ) {
	FVec3 q0 = p1;
	q0.sub( p0 );

	FVec3 q1 = p0;
	q1.sub( p );

	FVec3 q2 = q0;
	q0.cross( q1 );

	return q0.mag() / q2.mag();
}

float zmathAngleNormalize( float angRad ) {
	while( angRad < 0.f ) {
		angRad += PI2F;
	}
	while( angRad > PI2F ) {
		angRad -= PI2F;
	}
	return angRad;
}

int zmathCircleFromPoints( FVec2 p[3], FVec2 &center, float &radius ) {
	FVec2 ab( p[1] );
	ab.sub( p[0] );

	FVec2 bc( p[2] );
	bc.sub( p[1] );

	FVec2 abP = ab;
	abP.perp();

	FVec2 bcP = bc;
	bcP.perp();

	FMat2 kMat;
	kMat.m[0][0] = abP.x;  kMat.m[1][0] = bcP.x;
	kMat.m[0][1] = abP.y;  kMat.m[1][1] = bcP.y;

	FVec2 abM = p[0];
	abM.add( p[1] );
	abM.div( 2.f );

	FVec2 bcM = p[1];
	bcM.add( p[2] );
	bcM.div( 2.f );

	FVec2 bcMabM = bcM;
	bcMabM.sub( abM );

	int suc = kMat.inverse();
	if( !suc ) return 0;

	FVec2 k = kMat.mul( bcMabM );

	center = abP;
	center.mul( k.x );
	center.add( abM );

	FVec2 r( center );
	r.sub( p[0] );
	radius = r.mag();

	return 1;
}

// The following code is a modified from code found at 
// http://softsurfer.com/Archive/algorithm_0109/algorithm_0109.htm
//
// Copyright 2001, softSurfer (www.softsurfer.com)
// This code may be freely used and modified for any purpose
// providing that this copyright notice is included with it.
// SoftSurfer makes no warranty for this code, and cannot be held
// liable for any real or imagined damage resulting from its use.
// Users of this code must verify correctness for their application.

// Assume that a class is already given for the object:

//   Point with coordinates {float x, y;}
//===================================================================

// isLeft(): tests if a point is Left|On|Right of an infinite line.

//   Input: three points P0, P1, and P2
//   Return: >0 for P2 left of the line through P0 and P1
//           =0 for P2 on the line
//           <0 for P2 right of the line
//   See: the January 2001 Algorithm on Area of Triangles
static inline float isLeft( FVec2 P0, FVec2 P1, FVec2 P2 )
{
   return (P1.x - P0.x)*(P2.y - P0.y) - (P2.x - P0.x)*(P1.y - P0.y);
}
//===================================================================

#include "zprof.h" 

float zmathSigmoid( float x ) {
	zprofScope( _sigmoid );
	return 1.f / ( 1.f + expf(-x) );
}

float zmathSigmoidFittedCurve( float t, float x0, float y0, float x1, float y1, float s, float z ){
	// s should be a percentage, so between 0 & 1. 
	zmathBoundF( s, 0.f, 1.f );

	float d = x1 - x0;
	float e = y1 - y0;
	float g1 = s * d;
	float g2 = ( 1.f - s ) * d;
	float g = min ( fabsf(g1), fabsf(g2) );
	float a = 0.f;


	float answer = y0 + (e / 2.f) + ( e / 2.f ) * zmathSigmoid ( z * (t - ( x0 + g1 ) ) / g )   ; 
	return answer;
} 

float zmathBumpFunction(float x, float y, float k, float a, float b, float e){
	return k * expf( -( (x-a) * (x-a) + e * (y-b) * (y-b) ) );
}

// chainHull_2D(): Andrew's monotone chain 2D convex hull algorithm
//    Input: P[] = an array of 2D points
//                  presorted by increasing x- and y-coordinates
//            n = the number of points in P[]
//    Output: H[] = an array of the convex hull vertices (max is n)
//    Return: the number of points in H[]

static int pointCompare( const void *a, const void *b ) {
	FVec2 *_a = (FVec2 *)a;
	FVec2 *_b = (FVec2 *)b;
	if( _a->x < _b->x ) return -1;
	if( _a->x > _b->x ) return 1;
	if( _a->y < _b->y ) return -1;
	if( _a->y > _b->y ) return 1;
	return 0;
}

int zmathConvexHull2D( FVec2 * P, int n, FVec2 * H )
{
	qsort( P, n, sizeof(*P), pointCompare );

   // the output array H[] will be used as the stack
   int   bot=0, top=(-1); // indices for bottom and top of the stack
   int   i;               // array scan index

   // Get the indices of points with min x-coord and min|max y-coord
   int minmin = 0, minmax;
   float xmin = P[0].x;
   for (i=1; i<n; i++)
       if (P[i].x != xmin) break;
   minmax = i-1;
   if (minmax == n-1) {      // degenerate case: all x-coords == xmin
       H[++top] = P[minmin];
       if (P[minmax].y != P[minmin].y) // a nontrivial segment
           H[++top] = P[minmax];
       H[++top] = P[minmin];          // add polygon endpoint
       return top+1;
   }

   // Get the indices of points with max x-coord and min|max y-coord
   int maxmin, maxmax = n-1;
   float xmax = P[n-1].x;
   for (i=n-2; i>=0; i--)
       if (P[i].x != xmax) break;
   maxmin = i+1;

   // Compute the lower hull on the stack H
   H[++top] = P[minmin];     // push minmin point onto stack
   i = minmax;
   while (++i <= maxmin)
   {
       // the lower line joins P[minmin] with P[maxmin]
       if (isLeft( P[minmin], P[maxmin], P[i]) >= 0 && i < maxmin)
           continue;         // ignore P[i] above or on the lower line

       while (top > 0)       // there are at least 2 points on the stack
       {
           // test if P[i] is left of the line at the stack top
           if (isLeft( H[top-1], H[top], P[i]) > 0)
               break;        // P[i] is a new hull vertex
           else
               top--;        // pop top point off stack
       }
       H[++top] = P[i];      // push P[i] onto stack
   }

   // Next, compute the upper hull on the stack H above the bottom hull
   if (maxmax != maxmin)     // if distinct xmax points
       H[++top] = P[maxmax]; // push maxmax point onto stack
   bot = top;                // the bottom point of the upper hull stack
   i = maxmin;
   while (--i >= minmax)
   {
       // the upper line joins P[maxmax] with P[minmax]
       if (isLeft( P[maxmax], P[minmax], P[i]) >= 0 && i > minmax)
           continue;         // ignore P[i] below or on the upper line

       while (top > bot)   // at least 2 points on the upper stack
       {
           // test if P[i] is left of the line at the stack top
           if (isLeft( H[top-1], H[top], P[i]) > 0)
               break;        // P[i] is a new hull vertex
           else
               top--;        // pop top point off stack
       }
       H[++top] = P[i];      // push P[i] onto stack
   }
   if (minmax != minmin)
       H[++top] = P[minmin]; // push joining endpoint onto stack

   return top+1;
}

void zmathTransformVectorArray( float *input, float *output, float *mat4x4, int numVerts, int inputStepInBytes, int outputStepInBytes ) {
	float *vi = input;
	float *vo = output;
	int i;

	if( input == output ) {
		// If the input and output arrays are the same, a temporary copy must be made during the multiply
		for( i=0; i<numVerts; i++ ) {
			float _0 = mat4x4[0*4+0] * vi[0]  +  mat4x4[1*4+0] * vi[1]  +  mat4x4[2*4+0] * vi[2]  +  mat4x4[3*4+0];
			float _1 = mat4x4[0*4+1] * vi[0]  +  mat4x4[1*4+1] * vi[1]  +  mat4x4[2*4+1] * vi[2]  +  mat4x4[3*4+1];
			float _2 = mat4x4[0*4+2] * vi[0]  +  mat4x4[1*4+2] * vi[1]  +  mat4x4[2*4+2] * vi[2]  +  mat4x4[3*4+2];

			vo[0] = _0;
			vo[1] = _1;
			vo[2] = _2;

			vi = (float *)( (char *)vi + inputStepInBytes );
			vo = (float *)( (char *)vo + outputStepInBytes );
		}
	}
	else {
		// The arrays can be traversed more efficiently with a temp copy
		for( i=0; i<numVerts; i++ ) {
			vo[0] = mat4x4[0*4+0] * vi[0]  +  mat4x4[1*4+0] * vi[1]  +  mat4x4[2*4+0] * vi[2]  +  mat4x4[3*4+0];
			vo[1] = mat4x4[0*4+1] * vi[0]  +  mat4x4[1*4+1] * vi[1]  +  mat4x4[2*4+1] * vi[2]  +  mat4x4[3*4+1];
			vo[2] = mat4x4[0*4+2] * vi[0]  +  mat4x4[1*4+2] * vi[1]  +  mat4x4[2*4+2] * vi[2]  +  mat4x4[3*4+2];

			vi = (float *)( (char *)vi + inputStepInBytes );
			vo = (float *)( (char *)vo + outputStepInBytes );
		}
	}
}

void zmathDiffusion( float *src, float *dst, int w, int h, float rate ) {
	// DIFUSE the mobile map (ASL). The rate of diffusion is proportional to the second derivative of the spatial change in concentration

	memcpy( dst, src, sizeof(float)*w*h );

	int x, y;
	
	float *der = (float *)alloca( max(w,h) * sizeof(float) );

	// HORIZONTAL diffusion
	for( y=0; y<h; y++ ) {

		// COMPUTE one horizontal line of first deriviative...
		float *src0 = &src[y*w + 0];
		float *src1 = &src[y*w + 1];
		for( x=0; x<w-1; x++ ) {
			der[x] = *src1 - *src0;
			src0++;
			src1++;
		}

		// Move material in proportion to the second derivative
		float *_dst = &dst[y*w + 1];
		for( x=1; x<w-1; x++ ) {
			*_dst += rate * (der[x] - der[x-1]);
			_dst++;
		}
	}

	// VERTICAL diffusion
	for( x=0; x<w; x++ ) {
		// COMPUTE one vert line of first deriviative...
		float *src0 = &src[x];
		float *src1 = &src[w + x];
		for( y=0; y<h-1; y++ ) {
			der[y] = *src1 - *src0;
			src0 += w;
			src1 += w;
		}

		// Move material in proportion to the second derivative
		float *_dst = &dst[w + x];
		for( y=1; y<h-1; y++ ) {
			*_dst += rate * (der[y] - der[y-1]);
			_dst += w;
		}
	}
}

int zmathAreaConvex2DPolygonI( IVec2 *pts, int count ) {
	// Using the determinate method. See http://www.mathwords.com/a/area_convex_polygon.htm
	int _count = count-1;
	int sum0 = 0, sum1 = 0;
	for( int i=0; i<_count; i++ ) {
		sum0 += pts[i].x * pts[i+1].y;
		sum1 += pts[i].y * pts[i+1].x;
	}
	sum0 += pts[count-1].x * pts[0].y;
	sum1 += pts[count-1].y * pts[0].x;

	return abs((sum0 - sum1)) / 2;
}

void zmathActivateFPExceptions() {
    #ifdef WIN32
	_clearfp();
	_controlfp( ~(_EM_INVALID|_EM_OVERFLOW|_EM_ZERODIVIDE), _MCW_EM );
		// Setting the mask HIDEs the exception
	#endif
}

void zmathDeactivateFPExceptions() {
    #ifdef WIN32
	_clearfp();
	_controlfp( 0xFFFFFFFF, _MCW_EM );
		// Clearing the mask permits the exception
	#endif
}


void zmathFourierPower( float *src, int len, float *dst ) {
	int timeCount  = len;
	int omegaCount = timeCount / 2;

	// PRECOMPUTE the  phasors
	static float *lookup = 0;
	static int lastTimeCount = 0;
	if( !lookup || lastTimeCount != timeCount ) {
		if( lookup ) {
			delete lookup;
		}
		lastTimeCount = timeCount;

		lookup = new float[ omegaCount * timeCount * 2 ];
		float ofStep = PI2F;
		float tfStep = 1.f / (float)timeCount;
		float of = 0.f;
		for( int o=0; o<omegaCount; o++, of += ofStep ) {
			float *l = &lookup[ o * timeCount * 2 ];
			float tf = 0.f;
			for( int t=0; t<timeCount; t++, tf += tfStep ) {
				float a = of * tf;
				*l++ = cosf( a );
				*l++ = sinf( a );
			}
		}
	}

	// COMPUTE the Fourier
	float of = 0.f;
	float ofStep = PI2F;
	float tfStep = 1.f / (float)timeCount;
	for( int o=0; o<omegaCount; o++, of += ofStep ) {
		float *l = &lookup[ o * timeCount * 2 ];
		float *tt = src;
		float tf = 0.f;
		float accumC = 0.f;
		float accumS = 0.f;
		for( int t=0; t<timeCount; t++, tf += tfStep ) {
			float a = *tt++;
			float c = *l++ * a;
			float s = *l++ * a;
			accumC += c;
			accumS += s;
		}
		dst[o] = accumC*accumC + accumS*accumS;
	}
}

int zmathLineSegmentIntersect( float a0x, float a0y, float a1x, float a1y, float b0x, float b0y, float b1x, float b1y, float &x, float &y ) {
	// Linear system
	// AV = [ a1 - a0 ]
	// BV = [ b1 - b0 ]
	//
	// [ AVx BVx ][ a ] = [ b0x - a0x ]
	// [ AVy BVy ][ b ] = [ b0y - a0y ]
	//
	// Using Cramer's rule for a 2d system we take three determinants
	// D = det( [AV BV] )
	// a = det( [ 

	float AVx = a1x - a0x;
	float AVy = a1y - a0y;
	float BVx = b1x - b0x;
	float BVy = b1y - b0y;

	float cx = b0x - a0x;
	float cy = b0y - a0y;

	// Cramer's rule: 
	//
	//   D = det( [AVx BVx ] 
	//            [AVy BVy ] )
	//
	//   a = det( [ cx BVx ] 
	//            [ cy BVy ] ) / D
	//
	//   b = det( [ AVx cx ] 
	//            [ AVy cy ] ) / D
	//
	float D = AVx * BVy - AVy * BVx;
	if( D == 0.f ) {
		return 0;
	}

	float a = (cx * BVy - cy * BVx) / D;
	float b = -(AVx * cy - AVy * cx) / D;
	if( a > 0.f && a < 1.f && b > 0.f && b < 1.f ) {
		x = AVx * a + a0x;
		y = AVy * a + a0y;
		return 1;
	}

	return 0;
}

int zmathLineLineIntersect( FVec2 p0, FVec2 pd0, FVec2 p1, FVec2 pd1, float &k0, float &k1 ) {
	FMat2 m;
	m.m[0][0] = pd0.x;
	m.m[0][1] = pd0.y;
	m.m[1][0] = pd1.x;
	m.m[1][1] = pd1.y;

	float det = m.m[0][0] * m.m[1][1] - m.m[0][1] * m.m[1][0];
	if( fabsf(det) < 0.00000001f ) {
		return 0;
	}

	det = 1.f / det;

	FMat2 temp;
	temp.m[0][0] =  m.m[1][1] * det;
	temp.m[0][1] = -m.m[0][1] * det;
	temp.m[1][0] = -m.m[1][0] * det;
	temp.m[1][1] =  m.m[0][0] * det;

	FVec2 b = p0;
	b.sub( p1 );
	temp.mul( b );

	k0 = temp.m[0][0] * b.x + temp.m[0][1] * b.y;
	k1 = temp.m[1][0] * b.x + temp.m[1][1] * b.y;

	return 1;
}

double zmathMaxVecD( double *vec, int count, int bytePitch ) {
	double largest = -DBL_MAX;
	char *p = (char *)vec;
	while( count-- ) {
		largest = largest > *(double*)p ? largest : *(double*)p;
		p += bytePitch;
	}
	return largest;
}
