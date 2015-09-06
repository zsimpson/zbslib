// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Common vector classes of various types and sizes
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zvec.cpp zvec.h
//		*VERSION 2.0
//		+HISTORY {
//		}
//		+TODO {
//			Convert to standard naming convention (ZFVec2, etc.)
//			Cleanup
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "math.h"
#include "memory.h"
// MODULE includes:
#include "zvec.h"
// ZBSLIB includes:

#ifndef min
	#define min(a,b) ( ( (a) < (b) ) ? (a) : (b) )
#endif
#ifndef max
	#define max(a,b) ( ( (a) > (b) ) ? (a) : (b) )
#endif

void IRect::unionRect( IRect o ) {
	// Union with zero sized rects is meaningless
	if( _w > 0 && o._w > 0 ) {
		int __l = min( _l, o._l );
		_w = max( 0, max( _l+_w, o._l+o._w ) - __l );
		_l = __l;
		int __t = min( _t, o._t );
		_h = max( 0, max( _t+_h, o._t+o._h ) - __t );
		_t = __t;
	}
}

void IRect::unionPoint( int x, int y ) {
	int __l = min( _l, x );
	_w = max( 0, _l + _w - __l );
	_l = __l;
	int __t = min( _t, y );
	_h = max( 0, _t + _h - __t );
	_t = __t;
}

void IRect::clipTo( IRect c ) {
	int __l = max( _l, c._l );
	_w = max( 0, min( _l+_w, c._l+c._w ) - __l );
	_l = __l;
	int __t = max( _t, c._t );
	_h = max( 0, min( _t+_h, c._t+c._h ) - __t );
	_t = __t;
}

void IRect::deltaSize( int dx, int dy ) {
	_w += dx; _h += dy; _w = max(0,_w); _h = max(0,_h);
}


void FRect::unionRect( FRect o ) {
	// Union with zero sized rects is meaningless
	if( _w > 0.0f && o._w > 0.0f ) {
		float __l = min( _l, o._l );
		_w = max( 0.0f, max( _l+_w, o._l+o._w ) - __l );
		_l = __l;
		float __t = min( _t, o._t );
		_h = max( 0.0f, max( _t+_h, o._t+o._h ) - __t );
		_t = __t;
	}
}

void FRect::unionPoint( float x, float y ) {
	float __l = min( _l, x );
	_w = max( 0.0f, _l + _w - __l );
	_l = __l;
	float __t = min( _t, y );
	_h = max( 0.0f, _t + _h - __t );
	_t = __t;
}

void FRect::clipTo( FRect c ) {
	float __l = max( _l, c._l );
	_w = max( 0.0f, min( _l+_w, c._l+c._w ) - __l );
	_l = __l;
	float __t = max( _t, c._t );
	_h = max( 0.0f, min( _t+_h, c._t+c._h ) - __t );
	_t = __t;
}

void FRect::deltaSize( float dx, float dy ) {
	_w += dx; _h += dy; _w = max(0.0f,_w); _h = max(0.0f,_h);
}

void DRect::unionRect( DRect o ) {
	// Union with zero sized rects is meaningless
	if( _w > 0.0 && o._w > 0.0 ) {
		double __l = min( _l, o._l );
		_w = max( 0.0f, max( _l+_w, o._l+o._w ) - __l );
		_l = __l;
		double __t = min( _t, o._t );
		_h = max( 0.0f, max( _t+_h, o._t+o._h ) - __t );
		_t = __t;
	}
}

void DRect::unionPoint( double x, double y ) {
	double __l = min( _l, x );
	_w = max( 0.0, _l + _w - __l );
	_l = __l;
	double __t = min( _t, y );
	_h = max( 0.0, _t + _h - __t );
	_t = __t;
}

void DRect::clipTo( DRect c ) {
	double __l = max( _l, c._l );
	_w = max( 0.0, min( _l+_w, c._l+c._w ) - __l );
	_l = __l;
	double __t = max( _t, c._t );
	_h = max( 0.0, min( _t+_h, c._t+c._h ) - __t );
	_t = __t;
}

void DRect::deltaSize( double dx, double dy ) {
	_w += dx; _h += dy; _w = max(0.0,_w); _h = max(0.0,_h);
}


double SVec2::mag() {
	return sqrt((double)x*(double)x + (double)y*(double)y);
}

double IVec2::mag() {
	return sqrt((double)x*(double)x + (double)y*(double)y);
}

float FVec2::mag() {
	return (float)sqrt(x*x + y*y);
}

double DVec2::mag() {
	return sqrt(x*x + y*y);
}

double SVec3::mag() {
	return sqrt((double)x*(double)x + (double)y*(double)y + (double)z*(double)z);
}

double IVec3::mag() {
	return sqrt((double)x*(double)x + (double)y*(double)y + (double)z*(double)z);
}

float FVec3::mag() {
	return (float)sqrt(x*x + y*y + z*z);
}

float FVec4::mag() {
	return (float)sqrt(x*x + y*y + z*z + w*w);
}

double DVec3::mag() {
	return sqrt(x*x + y*y + z*z);
}

double DVec4::mag() {
	return sqrt(x*x + y*y + z*z + w*w);
}

float FVec3::dist( FVec3 v1, FVec3 v2 ) {
	float dx = v1.x - v2.x;
	float dy = v1.y - v2.y;
	float dz = v1.z - v2.z;

	return ( dx * dx + dy * dy + dz * dz );
}

float FVec3::dist2( FVec3 v1, FVec3 v2 ) {
	float dx = v1.x - v2.x;
	float dy = v1.y - v2.y;
	float dz = v1.z - v2.z;

	return (float)sqrt( dx * dx + dy * dy + dz * dz );
}

void FVec3::abs() {
	x = (float)fabs(x); y = (float)fabs(y); z = (float)fabs(z);
}

void DVec3::abs() {
	x = fabs(x); y = fabs(y); z = fabs(z);
}

void FVec3::project( FVec3 a ) {
	float d0 = dot( a );
	float d1 = dot( *this );
	if( d1 != 0.f ) {
		mul( d0/d1 );
	}
	else {
		x = 0.f;
		y = 0.f;
	}
}


void IVec2::project( IVec2 a ) {
	int d0 = dot( a );
	int d1 = dot( *this );
	if( d1 != 0 ) {
		mul( d0/d1 );
	}
	else {
		x = 0;
		y = 0;
	}
}

void SVec2::project( SVec2 a ) {
	int d0 = dot( a );
	int d1 = dot( *this );
	if( d1 != 0 ) {
		mul( d0/d1 );
	}
	else {
		x = 0;
		y = 0;
	}
}

void FVec2::project( FVec2 a ) {
	float d0 = dot( a );
	float d1 = dot( *this );
	if( d1 != 0 ) {
		mul( d0/d1 );
	}
	else {
		x = 0.f;
		y = 0.f;
	}
}

void DVec2::project( DVec2 a ) {
	double d0 = dot( a );
	double d1 = dot( *this );
	if( d1 != 0 ) {
		mul( d0/d1 );
	}
	else {
		x = 0.0;
		y = 0.0;
	}
}

void FVec2::reflectAbout( FVec2 a ) {
	FVec2 proj( a );
	proj.project( *this );
	FVec2 perp = proj;
	perp.sub( *this );
	perp.add( proj );
	x = perp.x;
	y = perp.y;
}

void DVec2::reflectAbout( DVec2 a ) {
	DVec2 proj( a );
	proj.project( *this );
	DVec2 perp = proj;
	perp.sub( *this );
	perp.add( proj );
	x = perp.x;
	y = perp.y;
}

FVec2 &FVec2::operator =(FVec3 v) {
	x = v.x;
	y = v.y;
	return *this;
}


FMat2::FMat2() { identity(); }

FMat2::FMat2( float b[2][2] ) { 
	memcpy( m, b, sizeof(float) * 4 );
}

FMat2::FMat2( float b[4] ) {
	memcpy( m, b, sizeof(float) * 4 );
}

void FMat2::add( FMat2 &b ) {
	m[0][0] += b.m[0][0];
	m[1][0] += b.m[1][0];
	m[0][1] += b.m[0][1];
	m[1][1] += b.m[1][1];
}

void FMat2::sub( FMat2 &b ) {
	m[0][0] -= b.m[0][0];
	m[1][0] -= b.m[1][0];
	m[0][1] -= b.m[0][1];
	m[1][1] -= b.m[1][1];
}

void FMat2::mul( float b ) {
	m[0][0] *= b;
	m[1][0] *= b;
	m[0][1] *= b;
	m[1][1] *= b;
}

void FMat2::div( float s ) {
	m[0][0] /= s;
	m[1][0] /= s;
	m[0][1] /= s;
	m[1][1] /= s;
}

void FMat2::mul( FMat2 &b ) {
	float t[2][2];
	t[0][0] = m[0][0]*b.m[0][0] + m[1][0]*b.m[0][1];
	t[0][1] = m[0][1]*b.m[0][0] + m[1][1]*b.m[0][1];
	t[1][0] = m[0][0]*b.m[1][0] + m[1][0]*b.m[1][1];
	t[1][1] = m[0][1]*b.m[1][0] + m[1][1]*b.m[1][1];
	memcpy( m, t, sizeof(float) * 4 );
}

float FMat2::determinant() {
	return m[0][0] * m[1][1] - m[0][1] * m[1][0];	
}

int FMat2::inverse() {
	float det = determinant();
	if( fabsf(det) < 0.00000001f ) {
		return 0;
	}
	det = 1.f / det;

	FMat2 temp;
	temp.m[0][0] =  m[1][1] * det;  temp.m[1][0] = -m[1][0] * det;
	temp.m[0][1] = -m[0][1] * det;  temp.m[1][1] =  m[0][0] * det;

	memcpy( m, &temp.m[0][0], sizeof(m) );

	return 1;
}


DMat2::DMat2() { identity(); }
DMat2::DMat2( double b[2][2] ) {
	memcpy( m, b, sizeof(double) * 4 );
}

DMat2::DMat2( double b[4] ) {
	memcpy( m, b, sizeof(double) * 4 );
}

void DMat2::add( DMat2 &b ) {
	m[0][0] += b.m[0][0];
	m[1][0] += b.m[1][0];
	m[0][1] += b.m[0][1];
	m[1][1] += b.m[1][1];
}

void DMat2::sub( DMat2 &b ) {
	m[0][0] -= b.m[0][0];
	m[1][0] -= b.m[1][0];
	m[0][1] -= b.m[0][1];
	m[1][1] -= b.m[1][1];
}

void DMat2::mul( double b ) {
	m[0][0] *= b;
	m[1][0] *= b;
	m[0][1] *= b;
	m[1][1] *= b;
}

void DMat2::div( double s ) {
	m[0][0] /= s;
	m[1][0] /= s;
	m[0][1] /= s;
	m[1][1] /= s;
}


void DMat2::mul( DMat2 &b ) {
	double t[2][2];
	t[0][0] = m[0][0]*b.m[0][0] + m[1][0]*b.m[0][1];
	t[0][1] = m[0][1]*b.m[0][0] + m[1][1]*b.m[0][1];
	t[1][0] = m[0][0]*b.m[1][0] + m[1][0]*b.m[1][1];
	t[1][1] = m[0][1]*b.m[1][0] + m[1][1]*b.m[1][1];
	memcpy( m, t, sizeof(float) * 4 );
}

double DMat2::determinant() {
	return m[0][0] * m[1][1] - m[0][1] * m[1][0];	
}

int DMat2::inverse() {
	double det = determinant();
	if( fabs(det) < 0.00000001 ) {
		return 0;
	}
	det = 1.0 / det;

	DMat2 temp;
	temp.m[0][0] =  m[1][1] * det;  temp.m[1][0] = -m[1][0] * det;
	temp.m[0][1] = -m[0][1] * det;  temp.m[1][1] =  m[0][0] * det;

	memcpy( m, &temp.m[0][0], sizeof(m) );

	return 1;
}



IVec3 IVec3::Origin( 0, 0, 0 );
IVec3 IVec3::XAxis ( 1, 0, 0 );
IVec3 IVec3::YAxis ( 0, 1, 0 );
IVec3 IVec3::ZAxis ( 0, 0, 1 );
IVec3 IVec3::XAxisMinus ( -1, 0, 0 );
IVec3 IVec3::YAxisMinus ( 0, -1, 0 );
IVec3 IVec3::ZAxisMinus ( 0, 0, -1 );

SVec3 SVec3::Origin( 0, 0, 0 );
SVec3 SVec3::XAxis ( 1, 0, 0 );
SVec3 SVec3::YAxis ( 0, 1, 0 );
SVec3 SVec3::ZAxis ( 0, 0, 1 );
SVec3 SVec3::XAxisMinus ( -1, 0, 0 );
SVec3 SVec3::YAxisMinus ( 0, -1, 0 );
SVec3 SVec3::ZAxisMinus ( 0, 0, -1 );

FVec3 FVec3::Origin( 0.0f, 0.0f, 0.0f );
FVec3 FVec3::XAxis ( 1.0f, 0.0f, 0.0f );
FVec3 FVec3::YAxis ( 0.0f, 1.0f, 0.0f );
FVec3 FVec3::ZAxis ( 0.0f, 0.0f, 1.0f );
FVec3 FVec3::XAxisMinus ( -1.0f, 0.0f, 0.0f );
FVec3 FVec3::YAxisMinus ( 0.0f, -1.0f, 0.0f );
FVec3 FVec3::ZAxisMinus ( 0.0f, 0.0f, -1.0f );

DVec3 DVec3::Origin( 0.0, 0.0, 0.0 );
DVec3 DVec3::XAxis ( 1.0, 0.0, 0.0 );
DVec3 DVec3::YAxis ( 0.0, 1.0, 0.0 );
DVec3 DVec3::ZAxis ( 0.0, 0.0, 1.0 );
DVec3 DVec3::XAxisMinus ( -1.0, 0.0, 0.0 );
DVec3 DVec3::YAxisMinus ( 0.0, -1.0, 0.0 );
DVec3 DVec3::ZAxisMinus ( 0.0, 0.0, -1.0 );

FMat4 FMat4::Identity;
DMat4 DMat4::Identity;
//FMat2 FMat2::Identity;
FMat3 FMat3::Identity;
DMat3 DMat3::Identity;


//////////////////////////////////////////////////////////////////////////////////
// FMat2
//////////////////////////////////////////////////////////////////////////////////

/*
FMat2::FMat2() {
	m[0][1] = 0.f;
	m[1][0] = 0.f;
	m[0][0] = 1.0f;
	m[1][1] = 1.0f;
}

FMat2::FMat2( float b[2][2] ) {
	memcpy( m, b, sizeof(float) * 4 );
}

FMat2::FMat2( float b[4] ) {
	memcpy( m, b, sizeof(float) * 4 );
}

void FMat3::cat( FMat3 &b ) {
	float t[2][2];
	t[0][0] = m[0][0]*b.m[0][0] + m[1][0]*b.m[0][1];
	t[0][1] = m[0][1]*b.m[0][0] + m[1][1]*b.m[0][1];

	t[1][0] = m[0][0]*b.m[1][0] + m[1][0]*b.m[1][1];
	t[1][1] = m[0][1]*b.m[1][0] + m[1][1]*b.m[1][1];

	memcpy( m, t, sizeof(float) * 4 );
}
																	   
FVec3 FMat3::mul( FVec2 v ) {										   
	FVec2 t;
	t.x = v.x*m[0][0] + v.y*m[1][0];
	t.y = v.x*m[0][1] + v.y*m[1][1];
	return t;
}
																	   
void FMat2::identity() {
	m[0][1] = 0.f;
	m[1][0] = 0.f;
	m[0][0] = 1.0f;
	m[1][1] = 1.0f;
}

void FMat3::transpose() {
	FMat2 temp;
	memcpy( temp.m, m, sizeof(float) * 4 );
	m[0][0] = temp.m[0][0];
	m[0][1] = temp.m[1][0];

	m[1][0] = temp.m[0][1];
	m[1][1] = temp.m[1][1];
}

void FMat3::add( FMat3 b ) {
	m[0][0] += b.m[0][0];
	m[1][0] += b.m[1][0];
	m[2][0] += b.m[2][0];

	m[0][1] += b.m[0][1];
	m[1][1] += b.m[1][1];
	m[2][1] += b.m[2][1];

	m[0][2] += b.m[0][2];
	m[1][2] += b.m[1][2];
	m[2][2] += b.m[2][2];
}

void FMat3::sub( FMat3 b ) {
	m[0][0] -= b.m[0][0];
	m[1][0] -= b.m[1][0];
	m[2][0] -= b.m[2][0];

	m[0][1] -= b.m[0][1];
	m[1][1] -= b.m[1][1];
	m[2][1] -= b.m[2][1];

	m[0][2] -= b.m[0][2];
	m[1][2] -= b.m[1][2];
	m[2][2] -= b.m[2][2];
}

void FMat3::mul( float s ) {
	float *p = &m[0][0];
	for( int i=0; i<9; i++, p++ ) {
		*p *= s;
	}
}


float FMat3::determinant() {
	return (
		  (m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]))
		- (m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2]))
		+ (m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]))
	);
}

void FMat3::adjoint() {
	FMat3 temp = *this;

	m[0][0] = temp.m[1][1] * temp.m[2][2] - temp.m[2][1] * temp.m[1][2];
	m[1][0] = temp.m[2][0] * temp.m[1][2] - temp.m[1][0] * temp.m[2][2];
	m[2][0] = temp.m[1][0] * temp.m[2][1] - temp.m[2][0] * temp.m[1][1];

	m[0][1] = temp.m[2][1] * temp.m[0][2] - temp.m[0][1] * temp.m[2][2];
	m[1][1] = temp.m[0][0] * temp.m[2][2] - temp.m[2][0] * temp.m[0][2];
	m[2][1] = temp.m[2][0] * temp.m[0][1] - temp.m[0][0] * temp.m[2][1];

	m[0][2] = temp.m[0][1] * temp.m[1][2] - temp.m[1][1] * temp.m[0][2];
	m[1][2] = temp.m[1][0] * temp.m[0][2] - temp.m[0][0] * temp.m[1][2];
	m[2][2] = temp.m[0][0] * temp.m[1][1] - temp.m[1][0] * temp.m[0][1];
}																	   

int FMat3::inverse() {												   
	// Note: This is not tested yet.
	float det = determinant();
	if( det < 0.001f ) return 0;
	adjoint();
	div( det );
	return 1;
}

void FMat3::div( float s ) {
	float *p = &m[0][0];
	for( int i=0; i<9; i++, p++ ) {
		*p /= s;
	}
}

void FMat3::skewSymetric( FVec3 cross ) {
    m[0][0] =  0.0f;
	m[1][0] = -cross.z;
	m[2][0] =  cross.y;
    
	m[0][1] =  cross.z;
	m[1][1] =  0.0f;
	m[2][1] = -cross.x;
    
	m[0][2] = -cross.y;
	m[1][2] =  cross.x;
	m[2][2] =  0.0f;
}
*/

//////////////////////////////////////////////////////////////////////////////////
// FMat3
//////////////////////////////////////////////////////////////////////////////////

FMat3::FMat3() {
	memset( m, 0, sizeof(float) * 9 );
	m[0][0] = 1.0f;
	m[1][1] = 1.0f;
	m[2][2] = 1.0f;
}

FMat3::FMat3( float b[3][3] ) {
	memcpy( m, b, sizeof(float) * 9 );
}

FMat3::FMat3( float b[9] ) {
	memcpy( m, b, sizeof(float) * 9 );
}

void FMat3::cat( const FMat3 &b ) {
	float t[3][3];
	t[0][0] = m[0][0]*b.m[0][0] + m[1][0]*b.m[0][1] + m[2][0]*b.m[0][2];
	t[0][1] = m[0][1]*b.m[0][0] + m[1][1]*b.m[0][1] + m[2][1]*b.m[0][2];
	t[0][2] = m[0][2]*b.m[0][0] + m[1][2]*b.m[0][1] + m[2][2]*b.m[0][2];

	t[1][0] = m[0][0]*b.m[1][0] + m[1][0]*b.m[1][1] + m[2][0]*b.m[1][2];
	t[1][1] = m[0][1]*b.m[1][0] + m[1][1]*b.m[1][1] + m[2][1]*b.m[1][2];
	t[1][2] = m[0][2]*b.m[1][0] + m[1][2]*b.m[1][1] + m[2][2]*b.m[1][2];

	t[2][0] = m[0][0]*b.m[2][0] + m[1][0]*b.m[2][1] + m[2][0]*b.m[2][2];
	t[2][1] = m[0][1]*b.m[2][0] + m[1][1]*b.m[2][1] + m[2][1]*b.m[2][2];
	t[2][2] = m[0][2]*b.m[2][0] + m[1][2]*b.m[2][1] + m[2][2]*b.m[2][2];
																	   
	memcpy( m, t, sizeof(float) * 9 );								   
}																	   
																	   
FVec3 FMat3::mul( FVec3 v ) {										   
	FVec3 t;
	t.x = v.x*m[0][0] + v.y*m[1][0] + v.z*m[2][0];
	t.y = v.x*m[0][1] + v.y*m[1][1] + v.z*m[2][1];
	t.z = v.x*m[0][2] + v.y*m[1][2] + v.z*m[2][2];
	return t;
}
																	   
void FMat3::identity() {
	memset( m, 0, sizeof(float) * 9 );
	m[0][0] = 1.0f;
	m[1][1] = 1.0f;
	m[2][2] = 1.0f;
}

void FMat3::transpose() {
	FMat3 temp;
	memcpy( temp.m, m, sizeof(float) * 9 );
	m[0][0] = temp.m[0][0];
	m[0][1] = temp.m[1][0];
	m[0][2] = temp.m[2][0];

	m[1][0] = temp.m[0][1];
	m[1][1] = temp.m[1][1];
	m[1][2] = temp.m[2][1];

	m[2][0] = temp.m[0][2];
	m[2][1] = temp.m[1][2];
	m[2][2] = temp.m[2][2];
}

void FMat3::orthoNormalize() {
	FVec3 x( m[0][0], m[0][1], m[0][2] );
	FVec3 y( m[1][0], m[1][1], m[1][2] );
	FVec3 z;
	x.normalize();

	// z = x cross y
	z = x;
	z.cross( y );
	z.normalize();

	// y = z cross x
	y = z;
	y.cross( x );
	y.normalize();

	m[0][0] = x.x;
	m[1][0] = y.x;
	m[2][0] = z.x;
	m[0][1] = x.y;
	m[1][1] = y.y;
	m[2][1] = z.y;
	m[0][2] = x.z;
	m[1][2] = y.z;
	m[2][2] = z.z;
}

void FMat3::add( FMat3 b ) {
	m[0][0] += b.m[0][0];
	m[1][0] += b.m[1][0];
	m[2][0] += b.m[2][0];

	m[0][1] += b.m[0][1];
	m[1][1] += b.m[1][1];
	m[2][1] += b.m[2][1];

	m[0][2] += b.m[0][2];
	m[1][2] += b.m[1][2];
	m[2][2] += b.m[2][2];
}

void FMat3::sub( FMat3 b ) {
	m[0][0] -= b.m[0][0];
	m[1][0] -= b.m[1][0];
	m[2][0] -= b.m[2][0];

	m[0][1] -= b.m[0][1];
	m[1][1] -= b.m[1][1];
	m[2][1] -= b.m[2][1];

	m[0][2] -= b.m[0][2];
	m[1][2] -= b.m[1][2];
	m[2][2] -= b.m[2][2];
}

void FMat3::mul( float s ) {
	float *p = &m[0][0];
	for( int i=0; i<9; i++, p++ ) {
		*p *= s;
	}
}


float FMat3::determinant() {
	return (
		  (m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]))
		- (m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2]))
		+ (m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]))
	);
}

void FMat3::adjoint() {
	FMat3 temp = *this;

	m[0][0] = temp.m[1][1] * temp.m[2][2] - temp.m[2][1] * temp.m[1][2];
	m[1][0] = temp.m[2][0] * temp.m[1][2] - temp.m[1][0] * temp.m[2][2];
	m[2][0] = temp.m[1][0] * temp.m[2][1] - temp.m[2][0] * temp.m[1][1];

	m[0][1] = temp.m[2][1] * temp.m[0][2] - temp.m[0][1] * temp.m[2][2];
	m[1][1] = temp.m[0][0] * temp.m[2][2] - temp.m[2][0] * temp.m[0][2];
	m[2][1] = temp.m[2][0] * temp.m[0][1] - temp.m[0][0] * temp.m[2][1];

	m[0][2] = temp.m[0][1] * temp.m[1][2] - temp.m[1][1] * temp.m[0][2];
	m[1][2] = temp.m[1][0] * temp.m[0][2] - temp.m[0][0] * temp.m[1][2];
	m[2][2] = temp.m[0][0] * temp.m[1][1] - temp.m[1][0] * temp.m[0][1];
}																	   

int FMat3::inverse() {												   
	// Note: This is not tested yet.
	float det = determinant();
	if( det < 0.001f ) return 0;
	adjoint();
	div( det );
	return 1;
}

void FMat3::div( float s ) {
	float *p = &m[0][0];
	for( int i=0; i<9; i++, p++ ) {
		*p /= s;
	}
}

void FMat3::skewSymetric( FVec3 cross ) {
    m[0][0] =  0.0f;
	m[1][0] = -cross.z;
	m[2][0] =  cross.y;
    
	m[0][1] =  cross.z;
	m[1][1] =  0.0f;
	m[2][1] = -cross.x;
    
	m[0][2] = -cross.y;
	m[1][2] =  cross.x;
	m[2][2] =  0.0f;
}

//////////////////////////////////////////////////////////////////////////////////
// DMat3
//////////////////////////////////////////////////////////////////////////////////

DMat3::DMat3() {
	memset( m, 0, sizeof(double) * 9 );
	m[0][0] = 1.0;
	m[1][1] = 1.0;
	m[2][2] = 1.0;
}

DMat3::DMat3( double b[3][3] ) {
	memcpy( m, b, sizeof(double) * 9 );
}

DMat3::DMat3( double b[9] ) {
	memcpy( m, b, sizeof(double) * 9 );
}

void DMat3::cat( const DMat3 &b ) {
	double t[3][3];
	t[0][0] = m[0][0]*b.m[0][0] + m[1][0]*b.m[0][1] + m[2][0]*b.m[0][2];
	t[0][1] = m[0][1]*b.m[0][0] + m[1][1]*b.m[0][1] + m[2][1]*b.m[0][2];
	t[0][2] = m[0][2]*b.m[0][0] + m[1][2]*b.m[0][1] + m[2][2]*b.m[0][2];

	t[1][0] = m[0][0]*b.m[1][0] + m[1][0]*b.m[1][1] + m[2][0]*b.m[1][2];
	t[1][1] = m[0][1]*b.m[1][0] + m[1][1]*b.m[1][1] + m[2][1]*b.m[1][2];
	t[1][2] = m[0][2]*b.m[1][0] + m[1][2]*b.m[1][1] + m[2][2]*b.m[1][2];

	t[2][0] = m[0][0]*b.m[2][0] + m[1][0]*b.m[2][1] + m[2][0]*b.m[2][2];
	t[2][1] = m[0][1]*b.m[2][0] + m[1][1]*b.m[2][1] + m[2][1]*b.m[2][2];
	t[2][2] = m[0][2]*b.m[2][0] + m[1][2]*b.m[2][1] + m[2][2]*b.m[2][2];
																	   
	memcpy( m, t, sizeof(double) * 9 );								   
}																	   
																	   
DVec3 DMat3::mul( DVec3 v ) {										   
	DVec3 t;
	t.x = v.x*m[0][0] + v.y*m[1][0] + v.z*m[2][0];
	t.y = v.x*m[0][1] + v.y*m[1][1] + v.z*m[2][1];
	t.z = v.x*m[0][2] + v.y*m[1][2] + v.z*m[2][2];
	return t;
}
																	   
void DMat3::adjoint() {
	DMat3 temp = *this;

	m[0][0] = temp.m[1][1] * temp.m[2][2] - temp.m[2][1] * temp.m[1][2];
	m[1][0] = temp.m[2][0] * temp.m[1][2] - temp.m[1][0] * temp.m[2][2];
	m[2][0] = temp.m[1][0] * temp.m[2][1] - temp.m[2][0] * temp.m[1][1];

	m[0][1] = temp.m[2][1] * temp.m[0][2] - temp.m[0][1] * temp.m[2][2];
	m[1][1] = temp.m[0][0] * temp.m[2][2] - temp.m[2][0] * temp.m[0][2];
	m[2][1] = temp.m[2][0] * temp.m[0][1] - temp.m[0][0] * temp.m[2][1];

	m[0][2] = temp.m[0][1] * temp.m[1][2] - temp.m[1][1] * temp.m[0][2];
	m[1][2] = temp.m[1][0] * temp.m[0][2] - temp.m[0][0] * temp.m[1][2];
	m[2][2] = temp.m[0][0] * temp.m[1][1] - temp.m[1][0] * temp.m[0][1];
}																	   

double DMat3::determinant() {
	return (
		  (m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]))
		- (m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2]))
		+ (m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]))
	);
}

int DMat3::inverse() {
	// Note: This is not tested yet.
	double det = determinant();
	if( det < 0.0001 ) return 0;
	adjoint();
	div( det );
	return 1;
}																	   

void DMat3::identity() {
	memset( m, 0, sizeof(double) * 9 );
	m[0][0] = 1.0;
	m[1][1] = 1.0;
	m[2][2] = 1.0;
}

void DMat3::transpose() {
	DMat3 temp;
	memcpy( temp.m, m, sizeof(double) * 9 );
	m[0][0] = temp.m[0][0];
	m[0][1] = temp.m[1][0];
	m[0][2] = temp.m[2][0];

	m[1][0] = temp.m[0][1];
	m[1][1] = temp.m[1][1];
	m[1][2] = temp.m[2][1];

	m[2][0] = temp.m[0][2];
	m[2][1] = temp.m[1][2];
	m[2][2] = temp.m[2][2];
}

void DMat3::orthoNormalize() {
	DVec3 x( m[0][0], m[0][1], m[0][2] );
	DVec3 y( m[1][0], m[1][1], m[1][2] );
	DVec3 z;
	x.normalize();

	// z = x cross y
	z = x;
	z.cross( y );
	z.normalize();

	// y = z cross x
	y = z;
	y.cross( x );
	y.normalize();

	m[0][0] = x.x;
	m[1][0] = y.x;
	m[2][0] = z.x;
	m[0][1] = x.y;
	m[1][1] = y.y;
	m[2][1] = z.y;
	m[0][2] = x.z;
	m[1][2] = y.z;
	m[2][2] = z.z;
}

void DMat3::add( DMat3 &b ) {
	m[0][0] += b.m[0][0];
	m[1][0] += b.m[1][0];
	m[2][0] += b.m[2][0];

	m[0][1] += b.m[0][1];
	m[1][1] += b.m[1][1];
	m[2][1] += b.m[2][1];

	m[0][2] += b.m[0][2];
	m[1][2] += b.m[1][2];
	m[2][2] += b.m[2][2];
}

void DMat3::sub( DMat3 &b ) {
	m[0][0] -= b.m[0][0];
	m[1][0] -= b.m[1][0];
	m[2][0] -= b.m[2][0];

	m[0][1] -= b.m[0][1];
	m[1][1] -= b.m[1][1];
	m[2][1] -= b.m[2][1];

	m[0][2] -= b.m[0][2];
	m[1][2] -= b.m[1][2];
	m[2][2] -= b.m[2][2];
}

void DMat3::mul( double s ) {
	double *p = &m[0][0];
	for( int i=0; i<9; i++, p++ ) {
		*p *= s;
	}
}

void DMat3::div( double s ) {
	double *p = &m[0][0];
	for( int i=0; i<9; i++, p++ ) {
		*p /= s;
	}
}

void DMat3::skewSymetric( DVec3 cross ) {
    m[0][0] =  0.0;
	m[1][0] = -cross.z;
	m[2][0] =  cross.y;
    
	m[0][1] =  cross.z;
	m[1][1] =  0.0;
	m[2][1] = -cross.x;
    
	m[0][2] = -cross.y;
	m[1][2] =  cross.x;
	m[2][2] =  0.0;
}


//////////////////////////////////////////////////////////////////////////////////
// FMat4
//////////////////////////////////////////////////////////////////////////////////

FMat4::FMat4() {
	memset( m, 0, sizeof(float) * 16 );
	m[0][0] = 1.0f;
	m[1][1] = 1.0f;
	m[2][2] = 1.0f;
	m[3][3] = 1.0f;
}

FMat4::FMat4( float b[4][4] ) {
	memcpy( m, b, sizeof(float) * 16 );
}

FMat4::FMat4( float b[16] ) {
	memcpy( m, b, sizeof(float) * 16 );
}

FMat4::FMat4( double b[4][4] ) {
	for( int i=0; i<4; i++ ) {
		for( int j=0; j<4; j++ ) {
			m[i][j] = (float)b[i][j];
		}
	}
}

FMat4::FMat4( FMat3 orient, FVec3 pos ) {
	m[0][0] = orient.m[0][0];
	m[0][1] = orient.m[0][1];
	m[0][2] = orient.m[0][2];
	m[0][3] = 0.0f;

	m[1][0] = orient.m[1][0];
	m[1][1] = orient.m[1][1];
	m[1][2] = orient.m[1][2];
	m[1][3] = 0.0f;

	m[2][0] = orient.m[2][0];
	m[2][1] = orient.m[2][1];
	m[2][2] = orient.m[2][2];
	m[2][3] = 0.0f;

	m[3][0] = pos.x;
	m[3][1] = pos.y;
	m[3][2] = pos.z;
	m[3][3] = 1.0f;
}

void FMat4::add( FMat4 &b ) {
	m[0][0] += b.m[0][0];
	m[0][1] += b.m[0][1];
	m[0][2] += b.m[0][2];
	m[0][3] += b.m[0][3];

	m[1][0] += b.m[1][0];
	m[1][1] += b.m[1][1];
	m[1][2] += b.m[1][2];
	m[1][3] += b.m[1][3];

	m[2][0] += b.m[2][0];
	m[2][1] += b.m[2][1];
	m[2][2] += b.m[2][2];
	m[2][3] += b.m[2][3];

	m[3][0] += b.m[3][0];
	m[3][1] += b.m[3][1];
	m[3][2] += b.m[3][2];
	m[3][3] += b.m[3][3];
}

void FMat4::mul( float s ) {
	m[0][0] *= s;
	m[0][1] *= s;
	m[0][2] *= s;
	m[0][3] *= s;

	m[1][0] *= s;
	m[1][1] *= s;
	m[1][2] *= s;
	m[1][3] *= s;

	m[2][0] *= s;
	m[2][1] *= s;
	m[2][2] *= s;
	m[2][3] *= s;

	m[3][0] *= s;
	m[3][1] *= s;
	m[3][2] *= s;
	m[3][3] *= s;

}

void FMat4::set( FMat3 &orient, FVec3 pos ) {
	m[0][0] = orient.m[0][0];
	m[0][1] = orient.m[0][1];
	m[0][2] = orient.m[0][2];
	m[0][3] = 0.0f;

	m[1][0] = orient.m[1][0];
	m[1][1] = orient.m[1][1];
	m[1][2] = orient.m[1][2];
	m[1][3] = 0.0f;

	m[2][0] = orient.m[2][0];
	m[2][1] = orient.m[2][1];
	m[2][2] = orient.m[2][2];
	m[2][3] = 0.0f;

	m[3][0] = pos.x;
	m[3][1] = pos.y;
	m[3][2] = pos.z;
	m[3][3] = 1.0f;
}

void FMat4::cat( const FMat4 &b ) {
	float t[4][4];
	t[0][0] = m[0][0]*b.m[0][0] + m[1][0]*b.m[0][1] + m[2][0]*b.m[0][2] + m[3][0]*b.m[0][3];
	t[0][1] = m[0][1]*b.m[0][0] + m[1][1]*b.m[0][1] + m[2][1]*b.m[0][2] + m[3][1]*b.m[0][3];
	t[0][2] = m[0][2]*b.m[0][0] + m[1][2]*b.m[0][1] + m[2][2]*b.m[0][2] + m[3][2]*b.m[0][3];
	t[0][3] = m[0][3]*b.m[0][0] + m[1][3]*b.m[0][1] + m[2][3]*b.m[0][2] + m[3][3]*b.m[0][3];

	t[1][0] = m[0][0]*b.m[1][0] + m[1][0]*b.m[1][1] + m[2][0]*b.m[1][2] + m[3][0]*b.m[1][3];
	t[1][1] = m[0][1]*b.m[1][0] + m[1][1]*b.m[1][1] + m[2][1]*b.m[1][2] + m[3][1]*b.m[1][3];
	t[1][2] = m[0][2]*b.m[1][0] + m[1][2]*b.m[1][1] + m[2][2]*b.m[1][2] + m[3][2]*b.m[1][3];
	t[1][3] = m[0][3]*b.m[1][0] + m[1][3]*b.m[1][1] + m[2][3]*b.m[1][2] + m[3][3]*b.m[1][3];

	t[2][0] = m[0][0]*b.m[2][0] + m[1][0]*b.m[2][1] + m[2][0]*b.m[2][2] + m[3][0]*b.m[2][3];
	t[2][1] = m[0][1]*b.m[2][0] + m[1][1]*b.m[2][1] + m[2][1]*b.m[2][2] + m[3][1]*b.m[2][3];
	t[2][2] = m[0][2]*b.m[2][0] + m[1][2]*b.m[2][1] + m[2][2]*b.m[2][2] + m[3][2]*b.m[2][3];
	t[2][3] = m[0][3]*b.m[2][0] + m[1][3]*b.m[2][1] + m[2][3]*b.m[2][2] + m[3][3]*b.m[2][3];

	t[3][0] = m[0][0]*b.m[3][0] + m[1][0]*b.m[3][1] + m[2][0]*b.m[3][2] + m[3][0]*b.m[3][3];
	t[3][1] = m[0][1]*b.m[3][0] + m[1][1]*b.m[3][1] + m[2][1]*b.m[3][2] + m[3][1]*b.m[3][3];
	t[3][2] = m[0][2]*b.m[3][0] + m[1][2]*b.m[3][1] + m[2][2]*b.m[3][2] + m[3][2]*b.m[3][3];
	t[3][3] = m[0][3]*b.m[3][0] + m[1][3]*b.m[3][1] + m[2][3]*b.m[3][2] + m[3][3]*b.m[3][3];

	memcpy( m, t, sizeof(float) * 16 );
}

FVec3 FMat4::mul( FVec2 v ) {
	FVec3 t;
	t.x = v.x*m[0][0] + v.y*m[1][0] + m[3][0];
	t.y = v.x*m[0][1] + v.y*m[1][1] + m[3][1];
	t.z = v.x*m[0][2] + v.y*m[1][2] + m[3][2];
	return t;
}

FVec3 FMat4::mul( FVec3 v ) {
	FVec3 t;
	t.x = v.x*m[0][0] + v.y*m[1][0] + v.z*m[2][0] + m[3][0];
	t.y = v.x*m[0][1] + v.y*m[1][1] + v.z*m[2][1] + m[3][1];
	t.z = v.x*m[0][2] + v.y*m[1][2] + v.z*m[2][2] + m[3][2];
	return t;
}

FVec4 FMat4::mul( FVec4 v ) {
	FVec4 t;
	t.x = v.x*m[0][0] + v.y*m[1][0] + v.z*m[2][0] + v.w*m[3][0];
	t.y = v.x*m[0][1] + v.y*m[1][1] + v.z*m[2][1] + v.w*m[3][1];
	t.z = v.x*m[0][2] + v.y*m[1][2] + v.z*m[2][2] + v.w*m[3][2];
	t.w = v.x*m[0][3] + v.y*m[1][3] + v.z*m[2][3] + v.w*m[3][3];
	return t;
}

void FMat4::mul( float v[4] ) {
	float _v[4];

	_v[0] = v[0]*m[0][0] + v[1]*m[1][0] + v[2]*m[2][0] + v[3]*m[3][0];
	_v[1] = v[0]*m[0][1] + v[1]*m[1][1] + v[2]*m[2][1] + v[3]*m[3][1];
	_v[2] = v[0]*m[0][2] + v[1]*m[1][2] + v[2]*m[2][2] + v[3]*m[3][2];
	_v[3] = v[0]*m[0][3] + v[1]*m[1][3] + v[2]*m[2][3] + v[3]*m[3][3];

	memcpy( v, _v, sizeof(float)*4 );
}

void FMat4::mul( float v[4], float o[4] ) {
	o[0] = v[0]*m[0][0] + v[1]*m[1][0] + v[2]*m[2][0] + v[3]*m[3][0];
	o[1] = v[0]*m[0][1] + v[1]*m[1][1] + v[2]*m[2][1] + v[3]*m[3][1];
	o[2] = v[0]*m[0][2] + v[1]*m[1][2] + v[2]*m[2][2] + v[3]*m[3][2];
	o[3] = v[0]*m[0][3] + v[1]*m[1][3] + v[2]*m[2][3] + v[3]*m[3][3];
}

void FMat4::identity() {
	memset( m, 0, sizeof(float) * 16 );
	m[0][0] = 1.0;
	m[1][1] = 1.0;
	m[2][2] = 1.0;
	m[3][3] = 1.0;
}

void FMat4::transpose() {
	FMat4 temp;
	memcpy( temp.m, m, sizeof(float) * 16 );
	m[0][0] = temp.m[0][0];
	m[0][1] = temp.m[1][0];
	m[0][2] = temp.m[2][0];
	m[0][3] = temp.m[3][0];

	m[1][0] = temp.m[0][1];
	m[1][1] = temp.m[1][1];
	m[1][2] = temp.m[2][1];
	m[1][3] = temp.m[3][1];

	m[2][0] = temp.m[0][2];
	m[2][1] = temp.m[1][2];
	m[2][2] = temp.m[2][2];
	m[2][3] = temp.m[3][2];

	m[3][0] = temp.m[0][3];
	m[3][1] = temp.m[1][3];
	m[3][2] = temp.m[2][3];
	m[3][3] = temp.m[3][3];
}

int FMat4::inverse() {
	// Stolen from bump.c - David G Yu, SGI
	float tmp[4][4];
    float aug[5][4];
    int h, i, j, k;

    for( h=0; h<4; ++h ) {
        for( i=0; i<4; i++ ) {
            aug[0][i] = m[0][i];
            aug[1][i] = m[1][i];
            aug[2][i] = m[2][i];
            aug[3][i] = m[3][i];
            aug[4][i] = (h == i) ? 1.0F : 0.0F;
        }

        for( i=0; i<3; ++i ) {
            float pivot = 0.0F;
		    int pivotIndex;
            for( j=i; j<4; ++j ) {
                float temp = aug[i][j] > 0.0F ? aug[i][j] : -aug[i][j];
                if( pivot < temp ) {
                    pivot = temp;
                    pivotIndex = j;
                }
            }
            if( pivot == 0.0F ) {
				identity();
				return 0;
		    }

            if( pivotIndex != i ) {
                for( k=i; k<5; ++k ) {
                    float temp = aug[k][i];
                    aug[k][i] = aug[k][pivotIndex];
                    aug[k][pivotIndex] = temp;
                }
            }

            for( k=i+1; k<4; ++k ) {
                float q = -aug[i][k] / aug[i][i];
                aug[i][k] = 0.0F;
                for( j=i+1; j<5; ++j ) {
                    aug[j][k] = q * aug[j][i] + aug[j][k];
                }
            }
        }

        if( aug[3][3] == 0.0F ) {
			identity();
			return 0;
		}

        tmp[h][3] = aug[4][3] / aug[3][3];

        for( k=1; k<4; ++k ) {
            float q = 0.0F;
            for( j=1; j<=k; ++j ) {
                q = q + aug[4-j][3-k] * tmp[h][4-j];
            }
            tmp[h][3-k] = (aug[4][3-k] - q) / aug[3-k][3-k];
        }
    }

	memcpy( m, tmp, sizeof(float) * 16 );
	return 1;
}																	   

void FMat4::pivot( FVec3 axis, float angleRad ) {
	float t[4];
	t[0] = m[3][0];
	t[1] = m[3][1];
	t[2] = m[3][2];
	t[3] = m[3][3];
	m[3][0] = 0.f;
	m[3][1] = 0.f;
	m[3][2] = 0.f;
	m[3][3] = 0.f;

	FMat4 r = rotate3D( axis, angleRad );
	cat( r );

	m[3][0] = t[0];
	m[3][1] = t[1];
	m[3][2] = t[2];
	m[3][3] = t[3];
}

void FMat4::orthoNormalize() {
	FVec3 x( m[0][0], m[0][1], m[0][2] );
	FVec3 y( m[1][0], m[1][1], m[1][2] );
	FVec3 z;
	x.normalize();

	// z = x cross y
	z = x;
	z.cross( y );
	z.normalize();

	// y = z cross x
	y = z;
	y.cross( x );
	y.normalize();

	m[0][0] = x.x;
	m[1][0] = y.x;
	m[2][0] = z.x;
	m[0][1] = x.y;
	m[1][1] = y.y;
	m[2][1] = z.y;
	m[0][2] = x.z;
	m[1][2] = y.z;
	m[2][2] = z.z;
}

//////////////////////////////////////////////////////////////////////////////////
// DMat4
//////////////////////////////////////////////////////////////////////////////////

DMat4::DMat4() {
	memset( m, 0, sizeof(double) * 16 );
	m[0][0] = 1.0f;
	m[1][1] = 1.0f;
	m[2][2] = 1.0f;
	m[3][3] = 1.0f;
}

DMat4::DMat4( double b[4][4] ) {
	memcpy( m, b, sizeof(double) * 16 );
}

DMat4::DMat4( double b[16] ) {
	memcpy( m, b, sizeof(double) * 16 );
}

DMat4::DMat4( DMat3 &orient, DVec3 pos ) {
	m[0][0] = orient.m[0][0];
	m[0][1] = orient.m[0][1];
	m[0][2] = orient.m[0][2];
	m[0][3] = 0.0;

	m[1][0] = orient.m[1][0];
	m[1][1] = orient.m[1][1];
	m[1][2] = orient.m[1][2];
	m[1][3] = 0.0;

	m[2][0] = orient.m[2][0];
	m[2][1] = orient.m[2][1];
	m[2][2] = orient.m[2][2];
	m[2][3] = 0.0;

	m[3][0] = pos.x;
	m[3][1] = pos.y;
	m[3][2] = pos.z;
	m[3][3] = 1.0;
}

DMat4::DMat4( FMat4& fm )
{
	m[0][0] = fm.m[0][0];
	m[0][1] = fm.m[0][1];
	m[0][2] = fm.m[0][2];
	m[0][3] = fm.m[0][3];

	m[1][0] = fm.m[1][0];
	m[1][1] = fm.m[1][1];
	m[1][2] = fm.m[1][2];
	m[1][3] = fm.m[1][3];

	m[2][0] = fm.m[2][0];
	m[2][1] = fm.m[2][1];
	m[2][2] = fm.m[2][2];
	m[2][3] = fm.m[2][3];

	m[3][0] = fm.m[3][0];
	m[3][1] = fm.m[3][1];
	m[3][2] = fm.m[3][2];
	m[3][3] = fm.m[3][3];
}


void DMat4::set( DMat3 &orient, DVec3 pos ) {
	m[0][0] = orient.m[0][0];
	m[0][1] = orient.m[0][1];
	m[0][2] = orient.m[0][2];
	m[0][3] = 0.0;

	m[1][0] = orient.m[1][0];
	m[1][1] = orient.m[1][1];
	m[1][2] = orient.m[1][2];
	m[1][3] = 0.0;

	m[2][0] = orient.m[2][0];
	m[2][1] = orient.m[2][1];
	m[2][2] = orient.m[2][2];
	m[2][3] = 0.0;

	m[3][0] = pos.x;
	m[3][1] = pos.y;
	m[3][2] = pos.z;
	m[3][3] = 1.0;
}

void DMat4::cat( const DMat4 &b ) {
	double t[4][4];
	t[0][0] = m[0][0]*b.m[0][0] + m[1][0]*b.m[0][1] + m[2][0]*b.m[0][2] + m[3][0]*b.m[0][3];
	t[0][1] = m[0][1]*b.m[0][0] + m[1][1]*b.m[0][1] + m[2][1]*b.m[0][2] + m[3][1]*b.m[0][3];
	t[0][2] = m[0][2]*b.m[0][0] + m[1][2]*b.m[0][1] + m[2][2]*b.m[0][2] + m[3][2]*b.m[0][3];
	t[0][3] = m[0][3]*b.m[0][0] + m[1][3]*b.m[0][1] + m[2][3]*b.m[0][2] + m[3][3]*b.m[0][3];

	t[1][0] = m[0][0]*b.m[1][0] + m[1][0]*b.m[1][1] + m[2][0]*b.m[1][2] + m[3][0]*b.m[1][3];
	t[1][1] = m[0][1]*b.m[1][0] + m[1][1]*b.m[1][1] + m[2][1]*b.m[1][2] + m[3][1]*b.m[1][3];
	t[1][2] = m[0][2]*b.m[1][0] + m[1][2]*b.m[1][1] + m[2][2]*b.m[1][2] + m[3][2]*b.m[1][3];
	t[1][3] = m[0][3]*b.m[1][0] + m[1][3]*b.m[1][1] + m[2][3]*b.m[1][2] + m[3][3]*b.m[1][3];

	t[2][0] = m[0][0]*b.m[2][0] + m[1][0]*b.m[2][1] + m[2][0]*b.m[2][2] + m[3][0]*b.m[2][3];
	t[2][1] = m[0][1]*b.m[2][0] + m[1][1]*b.m[2][1] + m[2][1]*b.m[2][2] + m[3][1]*b.m[2][3];
	t[2][2] = m[0][2]*b.m[2][0] + m[1][2]*b.m[2][1] + m[2][2]*b.m[2][2] + m[3][2]*b.m[2][3];
	t[2][3] = m[0][3]*b.m[2][0] + m[1][3]*b.m[2][1] + m[2][3]*b.m[2][2] + m[3][3]*b.m[2][3];

	t[3][0] = m[0][0]*b.m[3][0] + m[1][0]*b.m[3][1] + m[2][0]*b.m[3][2] + m[3][0]*b.m[3][3];
	t[3][1] = m[0][1]*b.m[3][0] + m[1][1]*b.m[3][1] + m[2][1]*b.m[3][2] + m[3][1]*b.m[3][3];
	t[3][2] = m[0][2]*b.m[3][0] + m[1][2]*b.m[3][1] + m[2][2]*b.m[3][2] + m[3][2]*b.m[3][3];
	t[3][3] = m[0][3]*b.m[3][0] + m[1][3]*b.m[3][1] + m[2][3]*b.m[3][2] + m[3][3]*b.m[3][3];

	memcpy( m, t, sizeof(double) * 16 );
}

DVec3 DMat4::mul( DVec2 v ) {
	DVec3 t;
	t.x = v.x*m[0][0] + v.y*m[1][0] + m[3][0];
	t.y = v.x*m[0][1] + v.y*m[1][1] + m[3][1];
	t.z = v.x*m[0][2] + v.y*m[1][2] + m[3][2];
	return t;
}

DVec3 DMat4::mul( DVec3 v ) {
	DVec3 t;
	t.x = v.x*m[0][0] + v.y*m[1][0] + v.z*m[2][0] + m[3][0];
	t.y = v.x*m[0][1] + v.y*m[1][1] + v.z*m[2][1] + m[3][1];
	t.z = v.x*m[0][2] + v.y*m[1][2] + v.z*m[2][2] + m[3][2];
	return t;
}

DVec4 DMat4::mul( DVec4 v ) {
	DVec4 t;
	t.x = v.x*m[0][0] + v.y*m[1][0] + v.z*m[2][0] + v.w*m[3][0];
	t.y = v.x*m[0][1] + v.y*m[1][1] + v.z*m[2][1] + v.w*m[3][1];
	t.z = v.x*m[0][2] + v.y*m[1][2] + v.z*m[2][2] + v.w*m[3][2];
	t.w = v.x*m[0][3] + v.y*m[1][3] + v.z*m[2][3] + v.w*m[3][3];
	return t;
}

void DMat4::identity() {
	memset( m, 0, sizeof(double) * 16 );
	m[0][0] = 1.0;
	m[1][1] = 1.0;
	m[2][2] = 1.0;
	m[3][3] = 1.0;
}

void DMat4::transpose() {
	DMat4 temp;
	memcpy( temp.m, m, sizeof(double) * 16 );
	m[0][0] = temp.m[0][0];
	m[0][1] = temp.m[1][0];
	m[0][2] = temp.m[2][0];
	m[0][3] = temp.m[3][0];

	m[1][0] = temp.m[0][1];
	m[1][1] = temp.m[1][1];
	m[1][2] = temp.m[2][1];
	m[1][3] = temp.m[3][1];

	m[2][0] = temp.m[0][2];
	m[2][1] = temp.m[1][2];
	m[2][2] = temp.m[2][2];
	m[2][3] = temp.m[3][2];

	m[3][0] = temp.m[0][3];
	m[3][1] = temp.m[1][3];
	m[3][2] = temp.m[2][3];
	m[3][3] = temp.m[3][3];
}

int DMat4::inverse() {
	// Stolen from bump.c - David G Yu, SGI
	double tmp[4][4];
    double aug[5][4];
    int h, i, j, k;

    for( h=0; h<4; ++h ) {
        for( i=0; i<4; i++ ) {
            aug[0][i] = m[0][i];
            aug[1][i] = m[1][i];
            aug[2][i] = m[2][i];
            aug[3][i] = m[3][i];
            aug[4][i] = (h == i) ? 1.0 : 0.0;
        }

        for( i=0; i<3; ++i ) {
            double pivot = 0.0;
		    int pivotIndex;
            for( j=i; j<4; ++j ) {
                double temp = aug[i][j] > 0.0 ? aug[i][j] : -aug[i][j];
                if( pivot < temp ) {
                    pivot = temp;
                    pivotIndex = j;
                }
            }
            if( pivot == 0.0 ) {
				identity();
				return 0;
		    }

            if( pivotIndex != i ) {
                for( k=i; k<5; ++k ) {
                    double temp = aug[k][i];
                    aug[k][i] = aug[k][pivotIndex];
                    aug[k][pivotIndex] = temp;
                }
            }

            for( k=i+1; k<4; ++k ) {
                double q = -aug[i][k] / aug[i][i];
                aug[i][k] = 0.0;
                for( j=i+1; j<5; ++j ) {
                    aug[j][k] = q * aug[j][i] + aug[j][k];
                }
            }
        }

        if( aug[3][3] == 0.0 ) {
			identity();
			return 0;
		}

        tmp[h][3] = aug[4][3] / aug[3][3];

        for( k=1; k<4; ++k ) {
            double q = 0.0;
            for( j=1; j<=k; ++j ) {
                q = q + aug[4-j][3-k] * tmp[h][4-j];
            }
            tmp[h][3-k] = (aug[4][3-k] - q) / aug[3-k][3-k];
        }
    }

	memcpy( m, tmp, sizeof(double) * 16 );
	return 1;
}																	   

void DMat4::pivot( DVec3 axis, double angleRad ) {
	double t[4];
	t[0] = m[3][0];
	t[1] = m[3][1];
	t[2] = m[3][2];
	t[3] = m[3][3];
	m[3][0] = 0.f;
	m[3][1] = 0.f;
	m[3][2] = 0.f;
	m[3][3] = 0.f;

	DMat4 r = rotate3D( axis, angleRad );
	cat( r );

	m[3][0] = t[0];
	m[3][1] = t[1];
	m[3][2] = t[2];
	m[3][3] = t[3];
}

void DMat4::orthoNormalize() {
	DVec3 x( m[0][0], m[0][1], m[0][2] );
	DVec3 y( m[1][0], m[1][1], m[1][2] );
	DVec3 z;
	x.normalize();

	// z = x cross y
	z = x;
	z.cross( y );
	z.normalize();

	// y = z cross x
	y = z;
	y.cross( x );
	y.normalize();

	m[0][0] = x.x;
	m[1][0] = y.x;
	m[2][0] = z.x;
	m[0][1] = x.y;
	m[1][1] = y.y;
	m[2][1] = z.y;
	m[0][2] = x.z;
	m[1][2] = y.z;
	m[2][2] = z.z;
}

//////////////////////////////////////////////////////////////////////////////////
// Quaternion
//////////////////////////////////////////////////////////////////////////////////
	  
FMat4 FQuat::mat() {
	FMat4 mat;

	// Quat to matrix code taken from http://www.gamasutra.com/features/19980703/quaternions_01.htm
	float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;
	x2 = q[0] + q[0];
	y2 = q[1] + q[1];
	z2 = q[2] + q[2];
	xx = q[0] * x2; xy = q[0] * y2; xz = q[0] * z2;
	yy = q[1] * y2; yz = q[1] * z2; zz = q[2] * z2;
	wx = q[3] * x2; wy = q[3] * y2; wz = q[3] * z2;

	mat.m[0][0] = 1.0f - (yy + zz);
	mat.m[1][0] = xy - wz;
	mat.m[2][0] = xz + wy;
	mat.m[3][0] = 0.f;

	mat.m[0][1] = xy + wz;
	mat.m[1][1] = 1.0f - (xx + zz);
	mat.m[2][1] = yz - wx;
	mat.m[3][1] = 0.f;

	mat.m[0][2] = xz - wy;
	mat.m[1][2] = yz + wx;
	mat.m[2][2] = 1.0f - (xx + yy);
	mat.m[3][2] = 0.f;

	mat.m[0][3] = 0.f;
	mat.m[1][3] = 0.f;
	mat.m[2][3] = 0.f;
	mat.m[3][3] = 1.f;

	return mat;
}

void FQuat::fromAxisAngle( FVec3 &axis, float radians ) {
	float c = cosf( radians / 2.f );
	float s = sinf( radians / 2.f );
	q[0] = axis.x * s;
	q[1] = axis.y * s;
	q[2] = axis.z * s;
	q[3] = c;
}

void FQuat::mul( FQuat &o ) {
	float temp[4];
	temp[0] = q[3]*o.q[0] + q[0]*o.q[3] + q[1]*o.q[2] - q[2]*o.q[1];
	temp[1] = q[3]*o.q[1] - q[0]*o.q[2] + q[1]*o.q[3] + q[2]*o.q[0];
	temp[2] = q[3]*o.q[2] + q[0]*o.q[1] - q[1]*o.q[0] + q[2]*o.q[3];
	temp[3] = q[3]*o.q[3] - q[0]*o.q[0] - q[1]*o.q[1] - q[2]*o.q[2];
	q[0] = temp[0];
	q[1] = temp[1];
	q[2] = temp[2];
	q[3] = temp[3];
}

FQuat::FQuat( FMat4 &mat ) {
	// From http://www.gamasutra.com/features/19980703/quaternions_01.htm
	// untested
	float tr, s, _q[4];
	int i, j, k;
	int nxt[3] = {1,2,0};
	tr = mat.m[0][0] + mat.m[1][1] + mat.m[2][2];
	if( tr > 0.f ) {
		s = sqrtf( tr + 1.f );
		q[3] = s / 2.f;
		s = 0.5f / s;
		q[0] = (mat.m[1][2] - mat.m[2][1]) * s;
		q[1] = (mat.m[2][0] - mat.m[0][2]) * s;
		q[2] = (mat.m[0][1] - mat.m[1][0]) * s;
	}
	else {
		i=0;
		if( mat.m[1][1] > mat.m[0][0] ) {
			i = 1;
		}
		if( mat.m[2][2] > mat.m[i][i] ) {
			i = 2;
		}
		j = nxt[i];
		k = nxt[j];
		s = sqrtf( (mat.m[i][i] - (mat.m[j][j] + mat.m[k][k])) + 1.f );
		_q[i] = s * 0.5f;
		if( s != 0.f ) {
			s = 0.5f / s;
		}
		_q[3] = ( mat.m[j][k] - mat.m[k][j] ) * s;
		_q[j] = ( mat.m[i][j] + mat.m[j][i] ) * s;
		_q[k] = ( mat.m[i][k] + mat.m[k][i] ) * s;

		q[0] = _q[0];
		q[1] = _q[1];
		q[2] = _q[2];
		q[3] = _q[3];
	}
}

void FQuat::conjugate() {
	q[0] = -q[0];
	q[1] = -q[1];
	q[2] = -q[2];
}

float FQuat::mag() {
	return sqrtf(q[3]*q[3] + q[0]*q[0] + q[1]*q[1] + q[2]*q[2] );
}

void FQuat::normalize() {
	float m = mag();
	q[0] /= m;
	q[1] /= m;
	q[2] /= m;
	q[3] /= m;
}

void FQuat::toAxisAngle( FVec3 &axis, float &radians ) {
	normalize();
    float cos_a = q[3];
    radians = acosf( cos_a ) * 2.f;
    float sin_a = sqrtf( 1.0f - cos_a * cos_a );
    if ( fabsf( sin_a ) < 0.0005 ) sin_a = 1.f;
    axis.x = q[0] / sin_a;
    axis.y = q[1] / sin_a;
    axis.z  = q[2] / sin_a;
}

void FQuat::fromEulerAngles( float x, float y, float z ) {
	FQuat xq( FVec3::XAxis, x );
	FQuat yq( FVec3::YAxis, y );
	FQuat zq( FVec3::ZAxis, z );
	xq.mul( yq );
	xq.mul( zq );
	*this = xq;
}


//////////////////////////////////////////////////////////////////////////////////
// rotation and scaling
//////////////////////////////////////////////////////////////////////////////////

DMat4 rotate3D( DVec3 axis, double angleRad ) {
	double c = cos( angleRad );
	double s = sin( angleRad );
	double t = 1.0 - c;

	axis.normalize();
	DMat4 mat;
	mat.m[0][0] = t * axis.x * axis.x + c;
	mat.m[1][0] = t * axis.x * axis.y - s * axis.z;
	mat.m[2][0] = t * axis.x * axis.z + s * axis.y;
	mat.m[3][0] = 0.0;

	mat.m[0][1] = t * axis.x * axis.y + s * axis.z;
	mat.m[1][1] = t * axis.y * axis.y + c;
	mat.m[2][1] = t * axis.y * axis.z - s * axis.x;
	mat.m[3][1] = 0.0;

	mat.m[0][2] = t * axis.x * axis.z - s * axis.y;
	mat.m[1][2] = t * axis.y * axis.z + s * axis.x;
	mat.m[2][2] = t * axis.z * axis.z + c;
	mat.m[3][2] = 0.0;

	mat.m[0][3] = 0.0;
	mat.m[1][3] = 0.0;
	mat.m[2][3] = 0.0;
	mat.m[3][3] = 1.0;
	return mat;
}

FMat4 rotate3D( FVec3 axis, float angleRad ) {
	float c = (float)cosf( angleRad );
	float s = (float)sinf( angleRad );
	float t = 1.0f - c;

	axis.normalize();
	FMat4 mat;
	mat.m[0][0] = t * axis.x * axis.x + c;
	mat.m[1][0] = t * axis.x * axis.y - s * axis.z;
	mat.m[2][0] = t * axis.x * axis.z + s * axis.y;
	mat.m[3][0] = 0.0f;

	mat.m[0][1] = t * axis.x * axis.y + s * axis.z;
	mat.m[1][1] = t * axis.y * axis.y + c;
	mat.m[2][1] = t * axis.y * axis.z - s * axis.x;
	mat.m[3][1] = 0.0f;

	mat.m[0][2] = t * axis.x * axis.z - s * axis.y;
	mat.m[1][2] = t * axis.y * axis.z + s * axis.x;
	mat.m[2][2] = t * axis.z * axis.z + c;
	mat.m[3][2] = 0.0f;

	mat.m[0][3] = 0.0f;
	mat.m[1][3] = 0.0f;
	mat.m[2][3] = 0.0f;
	mat.m[3][3] = 1.0f;
	return mat;
}

FMat4 rotate3D( FVec3 axis, FVec3 dir)
{
	float c = dir.x;
	float s = dir.y;
	float t = 1.0f - c;

	axis.normalize();
	FMat4 mat;
	mat.m[0][0] = t * axis.x * axis.x + c;
	mat.m[1][0] = t * axis.x * axis.y - s * axis.z;
	mat.m[2][0] = t * axis.x * axis.z + s * axis.y;
	mat.m[3][0] = 0.0f;

	mat.m[0][1] = t * axis.x * axis.y + s * axis.z;
	mat.m[1][1] = t * axis.y * axis.y + c;
	mat.m[2][1] = t * axis.y * axis.z - s * axis.x;
	mat.m[3][1] = 0.0f;

	mat.m[0][2] = t * axis.x * axis.z - s * axis.y;
	mat.m[1][2] = t * axis.y * axis.z + s * axis.x;
	mat.m[2][2] = t * axis.z * axis.z + c;
	mat.m[3][2] = 0.0f;

	mat.m[0][3] = 0.0f;
	mat.m[1][3] = 0.0f;
	mat.m[2][3] = 0.0f;
	mat.m[3][3] = 1.0f;
	return mat;
}

DMat4 scale3D( DVec3 s ) {
	DMat4 d;
	d.m[0][0] = s.x;
	d.m[1][1] = s.y;
	d.m[2][2] = s.z;
	return d;
}

FMat4 scale3D( FVec3 s ) {
	FMat4 d;
	d.m[0][0] = s.x;
	d.m[1][1] = s.y;
	d.m[2][2] = s.z;
	return d;
}

DMat3 rotate3D_3x3( DVec3 axis, double angleRad ) {
	double c = cos( angleRad );
	double s = sin( angleRad );
	double t = 1.0 - c;

	axis.normalize();
	DMat3 mat;
	mat.m[0][0] = t * axis.x * axis.x + c;
	mat.m[1][0] = t * axis.x * axis.y - s * axis.z;
	mat.m[2][0] = t * axis.x * axis.z + s * axis.y;

	mat.m[0][1] = t * axis.x * axis.y + s * axis.z;
	mat.m[1][1] = t * axis.y * axis.y + c;
	mat.m[2][1] = t * axis.y * axis.z - s * axis.x;

	mat.m[0][2] = t * axis.x * axis.z - s * axis.y;
	mat.m[1][2] = t * axis.y * axis.z + s * axis.x;
	mat.m[2][2] = t * axis.z * axis.z + c;

	return mat;
}

FMat3 rotate3D_3x3( FVec3 axis, float angleRad ) {
	float c = (float)cos( (double)angleRad );
	float s = (float)sin( (double)angleRad );
	float t = 1.0f - c;

	axis.normalize();
	FMat3 mat;
	mat.m[0][0] = t * axis.x * axis.x + c;
	mat.m[1][0] = t * axis.x * axis.y - s * axis.z;
	mat.m[2][0] = t * axis.x * axis.z + s * axis.y;

	mat.m[0][1] = t * axis.x * axis.y + s * axis.z;
	mat.m[1][1] = t * axis.y * axis.y + c;
	mat.m[2][1] = t * axis.y * axis.z - s * axis.x;

	mat.m[0][2] = t * axis.x * axis.z - s * axis.y;
	mat.m[1][2] = t * axis.y * axis.z + s * axis.x;
	mat.m[2][2] = t * axis.z * axis.z + c;

	return mat;
}

/*

This code is totally wrong, it is somehow handed differently
than OpenGL but I haven't had tiome to figure out how.

FMat3 rotate3D_3x3_EulerAngles( float ax, float ay, float az ) {


	FMat3 mat;
	float sinx, siny, sinz, cosx, cosy, cosz, sysz, cxcz, sxcz;

	sinx = (float)sin(ax); cosx = (float)cos(ax);
	siny = (float)sin(ay); cosy = (float)cos(ay);
	sinz = (float)sin(az); cosz = (float)cos(az);
	
	sysz = siny*sinz;
	cxcz = cosx*cosz;
	sxcz = sinx*cosz;

	mat.m[0][0] = sinx * sysz + cxcz;
	mat.m[0][1] = cosy * sinz;
	mat.m[0][2] = sxcz - cosx * sysz;
	mat.m[1][0] = sxcz * siny - cosx * sinz;
	mat.m[1][1] = cosy * cosz;
	mat.m[1][2] = -cxcz * siny - sinx * sinz;
	mat.m[2][0] = -sinx * cosy;
	mat.m[2][1] = siny;
	mat.m[2][2] = cosx * cosy;

	return mat; 
}
*/


// Airplane. The airplane is facing down the positive Z axis with up as Y
// Rotation 1 is about Y and is called yaw
// Rotation 2 is about X and is called pitch
// Rotation 3 is about Z and is called roll

FMat3 rotate3D_3x3_EulerAirplane( float yaw, float pitch, float roll ) {
	float cy = cosf( yaw );
	float sy = sinf( yaw );
	float cp = cosf( pitch );
	float sp = sinf( pitch );
	float cr = cosf( roll );
	float sr = sinf( roll );
	FMat3 mat;
	mat.m[0][0] = cy*cr + sy*sp*sr;    mat.m[1][0] = -sr*cy + sy*sp*cr;    mat.m[2][0] = sy*cp;
	mat.m[0][1] = cp*sr;               mat.m[1][1] = cp*cr;                mat.m[2][1] = -sp;
	mat.m[0][2] = -cr*sy + cy*sp*sr;   mat.m[1][2] = sr*sy+cy*sp*cr;       mat.m[2][2] = cy*cp;
	return mat;
}

void mat3ToEulerAirplane( FMat3 mat, float &yaw, float &pitch, float &roll ) {
	// Note that in above matrix:
	// tan(yaw) = sy/cy = m[2][0] / m[2][2]
	// tan(roll)    = sr/cr = m[0][1] / m[1][1]
	// -sin(pitch)  = -sp   = m[2][1];
	yaw   = atan2f( mat.m[2][0], mat.m[2][2] );
	roll  = atan2f( mat.m[0][1], mat.m[1][1] );
	pitch = asinf( -mat.m[2][1] );
}


// Euler Angles Max
// Rotation 1 is about Y and called yaw. -PI to PI
// Rotation 2 is about Z and called pitch -PI to PI
// Rotation 3 is about X and is called roll. -PI to PI

FMat4 rotate3D_EulerAnglesMax( float yaw, float pitch, float roll ) {
	float cy = cosf( yaw );
	float sy = sinf( yaw );
	float cp = cosf( pitch );
	float sp = sinf( pitch );
	float cr = cosf( roll );
	float sr = sinf( roll );
	FMat4 mat;
	mat.m[0][0] = cp*cy;               mat.m[1][0] = -cr*cy*sp+sr*sy;      mat.m[2][0] = cy*sp*sr+cr*sy;
	mat.m[0][1] = sp;                  mat.m[1][1] = cp*cr;                mat.m[2][1] = -cp*sr;
	mat.m[0][2] = cp*sy;               mat.m[1][2] = cy*sr-cr*sp*sy;       mat.m[2][2] = cr*cy+sp*sr*sy;
	mat.orthoNormalize();
	return mat;
}

void mat4ToEulerAnglesMax( FMat4 mat, float &yaw, float &pitch, float &roll ) {
	// Note that in above matrix:
	// tan(yaw) = sy/cy = [0][2] / [0][0]
	// tan(roll) = sr/cr = -[2][1] / [1][1]
	// sin(pitch) = [0][1]
	yaw   = atan2f( mat.m[0][2], mat.m[0][0] );
	roll  = atan2f( -mat.m[2][1], mat.m[1][1] );
	pitch = asinf( mat.m[0][1] );
}



DMat3 scale3D_3x3( DVec3 s ) {
	DMat3 d;
	d.m[0][0] = s.x;
	d.m[1][1] = s.y;
	d.m[2][2] = s.z;
	return d;
}

FMat3 scale3D_3x3( FVec3 s ) {
	FMat3 d;
	d.m[0][0] = s.x;
	d.m[1][1] = s.y;
	d.m[2][2] = s.z;
	return d;
}

FMat3 trans2D_3x3( FVec2 tr ) {
	FMat3 m;
	m.m[2][0] = tr.x;
	m.m[2][1] = tr.y;
	return m;
}

FMat3 trans2D_3x3( float x, float y ) {
	FMat3 m;
	m.m[2][0] = x;
	m.m[2][1] = y;
	return m;
}

DMat3 trans2D_3x3( DVec2 tr ) {
	DMat3 m;
	m.m[2][0] = tr.x;
	m.m[2][1] = tr.y;
	return m;
}

DMat3 trans2D_3x3( double x, double y ) {
	DMat3 m;
	m.m[2][0] = x;
	m.m[2][1] = y;
	return m;
}


FMat4 trans3D( float x, float y, float z ) {
	FMat4 m;
	m.setTrans( FVec3(x,y,z) );
	return m;
}

DMat4 trans3D( double x, double y, double z ) {
	DMat4 m;
	m.setTrans( DVec3(x,y,z) );
	return m;
}

FMat4 trans3D( FVec3 trans ) {
	FMat4 m;
	m.setTrans( trans );
	return m;
}

DMat4 trans3D( DVec3 trans ) {
	DMat4 m;
	m.setTrans( trans );
	return m;
}

FMat3 orientFromHomogenous( FMat4 mat ) {
	FMat3 orient;
	orient.m[0][0] = mat.m[0][0];
	orient.m[0][1] = mat.m[0][1];
	orient.m[0][2] = mat.m[0][2];
	orient.m[1][0] = mat.m[1][0];
	orient.m[1][1] = mat.m[1][1];
	orient.m[1][2] = mat.m[1][2];
	orient.m[2][0] = mat.m[2][0];
	orient.m[2][1] = mat.m[2][1];
	orient.m[2][2] = mat.m[2][2];
	return orient;
}
