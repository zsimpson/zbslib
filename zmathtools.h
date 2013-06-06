// @ZBS {
//		*MODULE_OWNER_NAME zmathtools
// }

#ifndef ZMATHTOOLS_H
#define ZMATHTOOLS_H

#include "zvec.h"
#include "float.h"  // for DBL_EPSILON
#include "math.h"  // for fabs

// Math and statistics
//=================================================================================

#ifndef PI
	#define PI (3.14159265358979323846)
#endif

#ifndef PI2
	#define PI2 (2.0 * 3.14159265358979323846)
#endif

#ifndef PIF
	#define PIF (3.14159265358979323846f)
#endif

#ifndef PI2F
	#define PI2F (2.f * 3.14159265358979323846f)
#endif

#ifndef min
	#define min(a,b) ( ( (a) < (b) ) ? (a) : (b) )
#endif
#ifndef max
	#define max(a,b) ( ( (a) > (b) ) ? (a) : (b) )
#endif

#ifndef doubleeq
	#define doubleeq(a,b    ) ( ( fabs( (a) - (b) ) < DBL_EPSILON ) ? 1 : 0 )
#endif

#ifndef doubleeqeps
	#define doubleeqeps(a,b,eps) ( ( fabs( (a) - (b) ) < eps         ) ? 1 : 0 )
		// are floating point values equal to within eps tolerance?
#endif

const float sin64F[65] = {
	 0.000000000f,  0.098017140f,  0.195090322f,  0.290284677f,  0.382683432f,  0.471396737f,  0.555570233f,  0.634393284f,
	 0.707106781f,  0.773010453f,  0.831469612f,  0.881921264f,  0.923879533f,  0.956940336f,  0.980785280f,  0.995184727f,
	 1.000000000f,  0.995184727f,  0.980785280f,  0.956940336f,  0.923879532f,  0.881921264f,  0.831469612f,  0.773010453f,
	 0.707106781f,  0.634393284f,  0.555570233f,  0.471396737f,  0.382683432f,  0.290284677f,  0.195090322f,  0.098017140f,
	 0.000000000f, -0.098017141f, -0.195090322f, -0.290284678f, -0.382683433f, -0.471396737f, -0.555570233f, -0.634393284f,
	-0.707106781f, -0.773010454f, -0.831469613f, -0.881921265f, -0.923879533f, -0.956940336f, -0.980785280f, -0.995184727f,
	-1.000000000f, -0.995184727f, -0.980785280f, -0.956940336f, -0.923879532f, -0.881921264f, -0.831469612f, -0.773010453f,
	-0.707106781f, -0.634393284f, -0.555570233f, -0.471396736f, -0.382683432f, -0.290284677f, -0.195090321f, -0.098017140f,
	 0.000000000f,
};

const float cos64F[65] = {
	 1.000000000f,  0.995184727f,  0.980785280f,  0.956940336f,  0.923879532f,  0.881921264f,  0.831469612f,  0.773010453f,
	 0.707106781f,  0.634393284f,  0.555570233f,  0.471396737f,  0.382683432f,  0.290284677f,  0.195090322f,  0.098017140f,
	 0.000000000f, -0.098017140f, -0.195090322f, -0.290284677f, -0.382683433f, -0.471396737f, -0.555570233f, -0.634393284f,
	-0.707106781f, -0.773010454f, -0.831469612f, -0.881921264f, -0.923879533f, -0.956940336f, -0.980785280f, -0.995184727f,
	-1.000000000f, -0.995184727f, -0.980785280f, -0.956940336f, -0.923879532f, -0.881921264f, -0.831469612f, -0.773010453f,
	-0.707106781f, -0.634393284f, -0.555570233f, -0.471396736f, -0.382683432f, -0.290284677f, -0.195090322f, -0.098017140f,
	 0.000000000f,  0.098017141f,  0.195090322f,  0.290284678f,  0.382683433f,  0.471396737f,  0.555570233f,  0.634393285f,
	 0.707106782f,  0.773010454f,  0.831469613f,  0.881921265f,  0.923879533f,  0.956940336f,  0.980785281f,  0.995184727f,
	 1.000000000f,
};

const float teraF = 1e12f;
const float gigaF = 1e9f;
const float megaF = 1e6f;
const float kiloF = 1e3f;;
const float deciF = 1e-1f;
const float centiF = 1e-2f;
const float milliF = 1e-3f;
const float microF = 1e-6f;
const float nanoF = 1e-9f;
const float picoF = 1e-12f;

int doubleCompare( const void *a, const void *b );
int floatCompare( const void *a, const void *b );
int intCompare( const void *a, const void *b );
int intCompareDescending( const void *a, const void *b );
	// handy util fns for qsort; inline

int zmathIsPow2( int x );
int zmathNextPow2( int x );

double zmathDegToRadD( double d );
double zmathRadToDegD( double d );
float  zmathDegToRadF( float d );
float  zmathRadToDegF( float d );

int zmathRoundToInt( double x );
int zmathRoundToInt( float x );

inline float zmathFractF( float x ) { return x - (int)x; }
inline double zmathFractD( double x ) { return x - (int)x; }
	// Returns the fractional part.  This is probably not the right algorithm as
	// it won't deal with large integer portions. TODO: What's the right solution?

double zmathMedianI( int *hist, int w, int numSamples = -1 );
	// Given a histogram array of size w, find the median.
	// If the number of samples is already known, it can
	// be passed in.  Otherwise, it will be calculated.

double zmathEnclosedMeanI( int *hist, int w, int med, double percent );
	// Given a histogram array of size w and a median,
	// find the mean of all the values within percent on either
	// side of the median.

int zmathLinearRegression2f( FVec2 *samples, int sampleCount, float &b, float &m, float &r );
	// Given a list of samples, finds b, m, and r=correlation coefficient

inline int zmathBoundI( int &a, int _min, int _max ) {
	if( a < _min ) a = _min; else if( a > _max ) a = _max; return a;
}

inline float zmathBoundF( float n, float a, float b ) {
	if( n < a ) n = a; 	else if( n > b ) n = b; return n;
}

inline double zmathBoundD( double n, double a, double b ) {
	if( n < a ) n = a; 	else if( n > b ) n = b; return n;
}

void zmathStatsD( double *series, double *sorted, int count, double *mean, double *median, double *stddev );
void zmathStatsF( float *series, float *sorted, int count, float *mean, float *median, float *stddev );
void zmathStatsI( int *series, int *sorted, int count, float *mean, float *median, float *stddev );
void zmathStatsUB( unsigned char *series, int *sorted, int count, float *mean, float *median, float *stddev );
void zmathStatsSB( signed char *series, int *sorted, int count, float *mean, float *median, float *stddev );
void zmathStatsUS( unsigned short *series, int *sorted, int count, float *mean, float *median, float *stddev );
void zmathStatsSS( signed short *series, int *sorted, int count, float *mean, float *median, float *stddev );
	// Computes the requested stats.  It won't compute the parts for which the pointer
	// is null so it is fast to use this even if you don't want all of the information
	// It internally sorts the list by making a copy for the median so the series isn't touched
	// If you give a pointer to sorted then it will stuff the sorted version into that
	// array for use outside of the function.  sorted and series must be the same size

double zmathPearsonI( int *seriesX, int *seriesY, int count );
double zmathPearsonD( double *seriesX, double *seriesY, int count );
float zmathPearsonF( float *seriesX, float *seriesY, int count );
	// Find the pearson correlation between the two series

inline int zmathSignII( int x ) { return x < 0 ? -1 : ( x > 0 ? 1 : 0); }
inline float zmathSignFF( float x ) { return x < 0.f ? -1.f : ( x > 0.f ? 1.f : 0.f); }
inline int zmathSignFI( float x ) { return x < 0.f ? -1 : ( x > 0.f ? 1 : 0); }
inline double zmathSignDD( double x ) { return x < 0.0 ? -1.0 : ( x > 0.0 ? 1.0 : 0.0); }
inline int zmathSignDI( double x ) { return x < 0.0 ? -1 : ( x > 0.0 ? 1 : 0); }

DVec2 zmathWorldCoordToNormalizedScreenD( DMat4 &projMat, DVec3 world );
FVec2 zmathWorldCoordToNormalizedScreenF( FMat4 &projMat, FVec3 world );
	// given a world coord and a projection matrix stolen from the renderer
	// get a screen position
	// see balls2.cpp for an example of how to get the projection matrix

FVec3 zmathLinePlaneIntersect( FVec3 p0, FVec3 norm, FVec3 p1, FVec3 p2 );
	// p0 is a point on the plane and norm is the plane's normal
	// p1 and p2 are points on the line.
	// This could result in infinite or singularities which are
	// currently not trapped. @TODO

FVec3 zmathClosestApproach( int n, FVec3 _p[], int pStrideInBytes, FVec3 _d[], int dStrideInBytes );
	// Given a set of lines in 3 space defined by p, p a point on the
	// line and d, a normalized directional vector, this function
	// returns the point which is the closest approach of the lines
	// The stride values allow packing.  A stride of sizeof(FVec3) would
	// mean that they were tightly packed.

float zmathAngleNormalizeF( float a );
double zmathAngleNormalizeD( double a );
	// These ensure that angle is between 0.0 and 2.0 * PI

float zmathPointPlaneCollF( FVec3 &p, FVec3 &p0, FVec3 &n );
double zmathPointPlaneCollD( DVec3 &p, DVec3 &p0, DVec3 &n );
	// @TODO: Convert to sphere since point is just degenerate sphere
	// p is the collision test point
	// p0 is a point on the plane
	// n is the plane normal which must be unit length
	// returns the penetration depth or zero if no collision

double zmathSpherePolyCollD(
	DVec3 &_pos, DVec3 &p0, DVec3 &n, double radius, 
	int edgeCount, DVec3 *edgesP0, DVec3 *edgesN, DVec3 &collNormOUT
	// @TODO: add a stride into these arrays
	// @TODO: Untested
);

FVec3 zmathPointToLineSegment( FVec3 p0, FVec3 dir, FVec3 p );
	// Finds the vector of closest approach between the line segment
	// defined by p0 (one end point), dir(from p0 to the other endpoint) and the point p

DMat4 zmathODERotPosToDMat4( double *rot, double *pos );
	// Converts a rotation and position returned by the ODE SDK
	// into an OpenGL compatible DMat4

FMat4 zmathODERotPosToFMat4( double *rot, double *pos );
	// Converts a rotation and position returned by the ODE SDK
	// into an OpenGL compatible DMat4

void zmathFMat4ToODE( FMat4 &m, double r[4*3] );
	// Converts an open GL compatible transform into
	// a matrix compatible with the ODE SDK

void zmathDMat4ToODE( DMat4 &m, double r[4*3] );
	// Converts an open GL compatible transform into
	// a matrix compatible with the ODE SDK

FQuat zmathODEQuatToFQaut( double *odeQuat );
	// ODE puts their quaternions in w, v format instead of v, w format

void zmath2DCubicSpline( FVec3 p1, FVec3 p2, FVec3 p1d, FVec3 p2d, FVec2 result[4] );
	// Solve for the 2D spline coeficients.  The z value of the verts is ignored.

void zmathQuatPosToMat4( float quat[4], float pos[3], FMat4 &mat );
void zmathQuatToMat3( float quat[4], FMat3 &mat );

float zmathDistancePointToLine( FVec3 p0, FVec3 p1, FVec3 p );
	// Given two points on a line p0 and p1 find the distance
	// to point p.

float zmathAngleNormalize( float angRad );
	// Given some angle bring it to between 0 and 2 pi

int zmathCircleFromPoints( FVec2 p[3], FVec2 &center, float &radius );
	// Given three non-co-linear points on a plane, find the circle
	// that passes thru all 3.  Returns zero if they are colinear and 1 on success

int zmathConvexHull2D( FVec2 * P, int n, FVec2 * H );
	// Computes the convex hull of points P, array length n. H must be as long as n

void zmathTransformVectorArray( float *input, float *output, float *mat4x4, int numVerts, int inputStepInBytes=sizeof(float)*3, int outputStepInBytes=sizeof(float)*3 );
	// Transform a linear (but not necessarily tightly packed) array of verts by a single 4x4 transform matrix
	// input array and output array may be identical (the transform may be done in place) as the function
	// checks for this and uses a temp register in case of in-place transform
	// The defaults for stepinbytes is tighly packed floats

void zmathDiffusion( float *src, float *dst, int w, int h, float rate );
	// Compute the diffusion on a map of floats

int zmathAreaConvex2DPolygonI( IVec2 *pts, int count );
	// As it sounds

float zmathSigmoid( float x );
	// return 1.f / ( 1.f + expf(-x) );

float zmathSigmoidFittedCurve( float t, float x0, float y0, float x1, float y1, float s1, float z1 );
	// finds y-value of x on a sigmoid, with endpoints  with vertical error of 
	// less than 1- sigmoid (z1) away from (x0,y0),
	// and (x1,y1), and 0.f < s1 < 1.f  controlling where the inflection point is,
	// as a percentage of x1 - x0. 

float zmathBumpFunction(float x, float y, float k, float a, float b, float e);
	// returns the function k * expf( -( (x-a) * (x-a) + e * (y-b) * (y-b) ) )
	// which is a bump function with elliptical level curves, centered at (a, b).

void zmathActivateFPExceptions();
	// Activates a fault on all floating point exceptions
void zmathDeactivateFPExceptions();
	// Deactivates a fault on all floating point exceptions

void zmathFourierPower( float *src, int len, float *dst );
	// Compute a slow (non FFT) Fourier on src, this is useful
	// when you have a non-power of two trace and don't want to do smoothing

int zmathLineSegmentIntersect( float a0x, float a0y, float a1x, float a1y, float b0x, float b0y, float b1x, float b1y, float &x, float &y );
	// Computes the intersection between two line segments, returning 1 on intersection 0 otherwise
	// if there is an intersection, x and y contain it.

int zmathLineLineIntersect( FVec2 p0, FVec2 pd0, FVec2 p1, FVec2 pd1, float &k0, float &k1 );
	// Computes the line to line intersection where p0 is a point on line 0 and pd0 is in the direction of line 0
	// k0 and k1 are the scalars of pd0 and pd1 if there is a solution. Returns 1 if there is a solution

double zmathMaxVecD( double *vec, int count, int bytePitch=sizeof(double) );
	// Given a potentially sparse vector of doubles, find the max value.


#endif

