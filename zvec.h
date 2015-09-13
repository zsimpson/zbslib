// @ZBS {
//		*MODULE_OWNER_NAME zvec
// }

// This is a set of vector classes for various types and sizes
// In many cases it is much easier to use these specifically typed
// and sized vectors in perfence to a more gneral purpose system
// such as a templatized or n-vector class.

#ifndef ZVEC_H
#define ZVEC_H

struct SVec2;
struct IVec2;
struct FVec2;
struct DVec2;
struct DMat2;
struct IRect;
struct FRect;
struct DRect;
struct SVec3;
struct IVec3;
struct FVec3;
struct DVec3;
struct DVec4;
struct FMat2;
struct FMat3;
struct DMat3;
struct FMat4;
struct DMat4;


struct SVec2 {
	short x, y;

	SVec2() { x = y = (short)0; }
	SVec2( int _x, int _y ) { x = _x; y = _y; }
	SVec2( double _x, double _y ) { x = (short)_x; y = (short)_y; }

	void add( SVec2 v ) { x += v.x; y += v.y; }
	void sub( SVec2 v ) { x -= v.x; y -= v.y; }
	void mul( int c ) { x *= c; y *= c; }
	void div( int c ) { x /= c; y /= c; }
	void bound( SVec2 v0, SVec2 v1 ) {
		x = x<v0.x ? v0.x : (x>v1.x?v1.x:x);
		y = y<v0.y ? v0.y : (y>v1.y?v1.y:y);
	}

	double mag();
	int mag2() { return x*x + y*y; }
	int dot( SVec2 v ) { return x*v.x + y*v.y; }

	void normalize() { int m=(int)mag(); if(m>0){ x/=m; y/=m; } }
	int equals( SVec2 v ) { return x==v.x && y==v.y; }
	void perp() { short t=x; x=-y; y=t; }
	int perpDot( SVec2 a ) { return x*a.y - y*a.x; }
	void project( SVec2 a );
		// Projects a onto this vector
	void swap() { short t=x; x=y; y=t; }

	void origin() { x = y = 0; }
	void xAxis() { x = 1; y = 0; }
	void yAxis() { x = 0; y = 1; }

	operator short *() { return (short *)this; }
};

struct IVec2 {
	int x, y;

	IVec2() { x = y = 0; }
	IVec2( int _x, int _y ) { x = _x; y = _y; }
	IVec2( float _x, float _y ) { x = (int)_x; y = (int)_y; }
	IVec2( double _x, double _y ) { x = (int)_x; y = (int)_y; }

	void add( IVec2 v ) { x += v.x; y += v.y; }
	void sub( IVec2 v ) { x -= v.x; y -= v.y; }
	void mul( int c ) { x *= c; y *= c; }
	void div( int c ) { x /= c; y /= c; }
	void bound( IVec2 v0, IVec2 v1 ) {
		x = x<v0.x ? v0.x : (x>v1.x?v1.x:x);
		y = y<v0.y ? v0.y : (y>v1.y?v1.y:y);
	}

	double mag();
	int mag2() { return x*x + y*y; }
	int dot( IVec2 v ) { return x*v.x + y*v.y; }

	void normalize() { int m=(int)mag(); if(m>0){ x/=m; y/=m; } }

	int equals( IVec2 v ) { return x==v.x && y==v.y; }
	void perp() { int t=x; x=-y; y=t; }
	int perpDot( IVec2 a ) { return x*a.y - y*a.x; }
	void project( IVec2 a );
		// Projects 'a' onto this vector
	void swap() { int t=x; x=y; y=t; }

	void origin() { x = y = 0; }
	void xAxis() { x = 1; y = 0; }
	void yAxis() { x = 0; y = 1; }

	operator int *() { return (int *)this; }
};

struct FVec2 {
	float x, y;

	FVec2() { x = y = 0.0; }
	FVec2( int _x, int _y ) { x = (float)_x; y = (float)_y; }
	FVec2( int *_x ) { x = (float)*(_x+0); y = (float)*(_x+1); }
	FVec2( float _x, float _y ) { x = _x; y = _y; }
	FVec2( double _x, double _y ) { x = (float)_x; y = (float)_y; }
	FVec2( float *_x ) { x = *(_x+0); y = *(_x+1); }
	FVec2( IVec2 v ) { x = (float)v.x; y = (float)v.y; }
	FVec2( SVec2 v ) { x = (float)v.x; y = (float)v.y; }

	void add( FVec2 v ) { x += v.x; y += v.y; }
	void sub( FVec2 v ) { x -= v.x; y -= v.y; }
	void mul( float c ) { x *= c; y *= c; }
	void div( float c ) { x /= c; y /= c; }
	void bound( FVec2 v0, FVec2 v1 ) {
		x = x<v0.x ? v0.x : (x>v1.x?v1.x:x);
		y = y<v0.y ? v0.y : (y>v1.y?v1.y:y);
	}

	void boundLen( float l ) {
		float m = mag();
		if( m > l ) {
			normalize();
			mul( l );
		}
	}

	void complexMul( FVec2 a ) {
		float _x = x*a.x - y*a.y;
		float _y = y*a.x + x*a.y;
		x = _x;
		y = _y;
	}

	float mag();
	float mag2() { return x*x + y*y; }
	float dot( FVec2 v ) { return x*v.x + y*v.y; }

	void normalize() { float m=mag(); if(m>0){ x/=m; y/=m; } }
	int equals( FVec2 v ) { return x==v.x && y==v.y; }
	void perp() { float t=x; x=-y; y=t; }
	float perpDot( FVec2 a ) { return x*a.y - y*a.x; }
	void project( FVec2 a );
		// Projects a onto this vector
	void reflectAbout( FVec2 a );
	void swap() { float t=x; x=y; y=t; }

	void origin() { x = y = 0.f; }
	void xAxis() { x = 1.f; y = 0.f; }
	void yAxis() { x = 0.f; y = 1.f; }

	FVec2 &operator =(IVec2 v) { x=(float)v.x; y=(float)v.y; return *this; }
	FVec2 &operator =(FVec3 v);
	operator float *() { return (float *)this; }
};

struct DVec2 {
	double x, y;

	DVec2() { x = y = 0.0; }
	DVec2( int _x, int _y ) { x = _x; y = _y; }
	DVec2( double _x, double _y ) { x = _x; y = _y; }
	DVec2( IVec2 v ) { x = (double)v.x; y = (double)v.y; }
	DVec2( SVec2 v ) { x = (double)v.x; y = (double)v.y; }
	DVec2( FVec2 v ) { x = (double)v.x; y = (double)v.y; }
	DVec2( int *_x ) { x = (double)*(_x+0); y = (double)*(_x+1); }
	DVec2( double *_x ) { x = *(_x+0); y = *(_x+1); }

	void add( DVec2 v ) { x += v.x; y += v.y; }
	void sub( DVec2 v ) { x -= v.x; y -= v.y; }
	void mul( double c ) { x *= c; y *= c; }
	void div( double c ) { x /= c; y /= c; }
	void bound( DVec2 v0, DVec2 v1 ) {
		x = x<v0.x ? v0.x : (x>v1.x?v1.x:x);
		y = y<v0.y ? v0.y : (y>v1.y?v1.y:y);
	}

	void boundLen( double l ) {
		double m = mag();
		if( m > l ) {
			normalize();
			mul( l );
		}
	}

	double mag();
	double mag2() { return x*x + y*y; }
	double dot( DVec2 v ) { return x*v.x + y*v.y; }

	void normalize() { double m=mag(); if(m>0){ x/=m; y/=m; } }
	int equals( DVec2 v ) { return x==v.x && y==v.y; }
	void perp() { double t=x; x=-y; y=t; }
	double perpDot( DVec2 a ) { return x*a.y - y*a.x; }
	void project( DVec2 a );
		// Projects a onto this vector
	void reflectAbout( DVec2 a );
	void swap() { double t=x; x=y; y=t; }

	void origin() { x = y = 0.0; }
	void xAxis() { x = 1.0; y = 0.0; }
	void yAxis() { x = 0.0; y = 1.0; }

	DVec2 &operator =(IVec2 v) { x=(double)v.x; y=(double)v.y; return *this; }
	operator double *() { return (double *)this; }
};

struct FMat2 {
	float m[2][2];
	// This is in m[col][row] format to match gl

	FMat2();
	FMat2( float b[2][2] );
	FMat2( float b[4] );

	void mul( FMat2 &b );
	FVec2 mul( FVec2 v ) {
		FVec2 t; t.x=v.x*m[0][0] + v.y*m[1][0]; t.y=v.x*m[0][1] + v.y*m[1][1]; return t;
	}
	void add( FMat2 &b );
	void sub( FMat2 &b );
	void mul( float b );
	void div( float s );
	void identity() { m[0][0] = 1.f; m[0][1] = 0.f; m[1][0] = 0.f; m[1][1] = 1.f; }
	float determinant();
	int inverse();
	
	FVec2 colVec(int i) {
		return FVec2( m[i][0], m[i][1] );
	}
	FVec2 rowVec(int i) {
		return FVec2( m[0][i], m[1][i] );
	}
};

struct DMat2 {
	double m[2][2];
	// This is in m[col][row] form to match gl

	DMat2();
	DMat2( double b[2][2] );
	DMat2( double b[4] );

	void mul( DMat2 &b );
	DVec2 mul( DVec2 v ) {
		DVec2 t; t.x=v.x*m[0][0] + v.y*m[1][0]; t.y=v.x*m[0][1] + v.y*m[1][1]; return t;
	}
	void add( DMat2 &b );
	void sub( DMat2 &b );
	void mul( double b );
	void div( double s );
	void identity() { m[0][0] = 1.0; m[0][1] = 0.0; m[1][0] = 0.0; m[1][1] = 1.0; }
	double determinant();
	int inverse();

	DVec2 colVec(int i) {
		return DVec2( m[i][0], m[i][1] );
	}
	DVec2 rowVec(int i) {
		return DVec2( m[0][i], m[1][i] );
	}
};

struct IRect {
	int _l, _t, _w, _h;

	IRect() { _l=_t=_w=_h=0; }
	IRect( int __l, int __t, int __w, int __h ) { _l=__l; _t=__t; _w=__w; _h=__h; }
	IRect( IRect &r ) { _l=r._l; _t=r._t; _w=r._w; _h=r._h; }

	void set( int __l, int __t, int __w, int __h) { _l=__l; _t=__t; _w=__w; _h=__h; }
	void set( IRect r2 ) { _l=r2._l; _t=r2._t; _w=r2._w; _h=r2._h; }

	int l() { return _l; }
	int t() { return _t; }
	int r() { return _l+_w; }
	int b() { return _t+_h; }
	int w() { return _w; }
	int h() { return _h; }
	void setW( int __w ) { _w=__w; }
	void setH( int __h ) { _h=__h; }
	void setWH( int __w, int __h ) { _w=__w; _h=__h; }

	int rPixel() { return _l+_w-1; }
	int bPixel() { return _t+_h-1; }

	int includes( int x, int y ) { return x>=_l && x<_l+_w && y>=_t && y<_t+_h; }
	int includes( IVec2 v ) { return v.x>=_l && v.x<_l+_w && v.y>=_t && v.y<_t+_h; }
	int includes( SVec2 v ) { return v.x>=_l && v.x<_l+_w && v.y>=_t && v.y<_t+_h; }

	int equals( IRect a ) { return _l==a._l && _t==a._t && _w==a._w && _h==a._h; }

	int area() { return _w * _h; }

	int isValid() { return _w >= 0 && _h >= 0; }

	void unionRect( IRect o );
		// Union with zero sized rects is meaningless

	void unionPoint( int x, int y );
	void clipTo( IRect c );
	int intersects( IRect o ) {
		IRect temp( o );
		temp.clipTo( *this );
		return( temp._w > 0 || temp._h > 0 );
	}

	void scale( int factor ) { _w *= factor; _h *= factor; }

	void moveTo( int x, int y ) { _l=x; _t=y; }

	void translate( int dx, int dy ) { _l += dx; _t += dy; }

	void deltaSize( int dx, int dy );
};

struct FRect {
	float _l, _t, _w, _h;

	FRect() { _l=_t=_w=_h=0; }
	FRect( int __l, int __t, int __w, int __h ) { _l=(float)__l; _t=(float)__t; _w=(float)__w; _h=(float)__h; }
	FRect( float __l, float __t, float __w, float __h ) { _l=__l; _t=__t; _w=__w; _h=__h; }
	FRect( double __l, double __t, double __w, double __h ) { _l=(float)__l; _t=(float)__t; _w=(float)__w; _h=(float)__h; }
	FRect( IRect r ) { _l=(float)r._l; _t=(float)r._t; _w=(float)r._w; _h=(float)r._h; }
	FRect( FRect &r ) { _l=r._l; _t=r._t; _w=r._w; _h=r._h; }

	void set( int __l, int __t, int __w, int __h ) { _l=(float)__l; _t=(float)__t; _w=(float)_w; _h=(float)__h; }
	void set( float __l, float __t, float __w, float __h ) { _l=__l; _t=__t; _w=__w; _h=__h; }
	void set( double __l, double __t, double __w, double __h ) { _l=(float)__l; _t=(float)__t; _w=(float)__w; _h=(float)__h; }
	void set( IRect r ) { _l=(float)r._l; _t=(float)r._t; _w=(float)r._w; _h=(float)r._h; }
	void set( FRect r ) { _l=(float)r._l; _t=(float)r._t; _w=(float)r._w; _h=(float)r._h; }

	float l() { return _l; }
	float t() { return _t; }
	float r() { return _l+_w; }
	float b() { return _t+_h; }
	float w() { return _w; }
	float h() { return _h; }
	void setW( float __w ) { _w=__w; }
	void setH( float __h ) { _h=__h; }
	void setWH( float __w, float __h ) { _w=__w; _h=__h; }

	int includes( float x, float y ) { return x>=_l && x<_l+_w && y>=_t && y<_t+_h; }
	int includes( FVec2 v ) { return v.x>=_l && v.x<_l+_w && v.y>=_t && v.y<_t+_h; }

	int equals( FRect a ) { return _l==a._l && _t==a._t && _w==a._w && _h==a._h; }

	float area() { return _w * _h; }

	int isValid() { return _w >= 0.0f && _h >= 0.0f; }

	void unionRect( FRect o );
		// Union with zero sized rects is meaningless
	void unionPoint( float x, float y );
	void clipTo( FRect c );

	int intersects( FRect o ) {
		FRect temp( o );
		temp.clipTo( *this );
		return( temp._w > 0.0f || temp._h > 0.0f );
	}

	void scale( float factor ) { _w *= factor; _h *= factor; }

	void moveTo( float x, float y ) { _l=x; _t=y; }

	void translate( float dx, float dy ) { _l += dx; _t += dy; }

	void deltaSize( float dx, float dy );
};

struct DRect {
	double _l, _t, _w, _h;

	DRect() { _l=_t=_w=_h=0; }
	DRect( int __l, int __t, int __w, int __h ) { _l=(double)__l; _t=(double)__t; _w=(double)_w; _h=(double)__h; }
	DRect( float __l, float __t, float __w, float __h ) { _l=(double)__l; _t=(double)__t; _w=(double)__w; _h=(double)__h; }
	DRect( double __l, double __t, double __w, double __h ) { _l=(double)__l; _t=(double)__t; _w=(double)__w; _h=(double)__h; }
	DRect( IRect r ) { _l=(double)r._l; _t=(double)r._t; _w=(double)r._w; _h=(double)r._h; }
	DRect( FRect r ) { _l=(double)r._l; _t=(double)r._t; _w=(double)r._w; _h=(double)r._h; }
	DRect( DRect &r ) { _l=r._l; _t=r._t; _w=r._w; _h=r._h; }

	void set( int __l, int __t, int __w, int __h ) { _l=(double)__l; _t=(double)__t; _w=(double)_w; _h=(double)__h; }
	void set( float __l, float __t, float __w, float __h ) { _l=(double)__l; _t=(double)__t; _w=(double)__w; _h=(double)__h; }
	void set( double __l, double __t, double __w, double __h ) { _l=(double)__l; _t=(double)__t; _w=(double)__w; _h=(double)__h; }
	void set( IRect r ) { _l=(double)r._l; _t=(double)r._t; _w=(double)r._w; _h=(double)r._h; }
	void set( FRect r ) { _l=(double)r._l; _t=(double)r._t; _w=(double)r._w; _h=(double)r._h; }
	void set( DRect r ) { _l=(double)r._l; _t=(double)r._t; _w=(double)r._w; _h=(double)r._h; }

	double l() { return _l; }
	double t() { return _t; }
	double r() { return _l+_w; }
	double b() { return _t+_h; }
	double w() { return _w; }
	double h() { return _h; }
	void setW( double __w ) { _w=__w; }
	void setH( double __h ) { _h=__h; }
	void setWH( double __w, double __h ) { _w=__w; _h=__h; }

	int includes( double x, double y ) { return x>=_l && x<_l+_w && y>=_t && y<_t+_h; }
	int includes( DVec2 v ) { return v.x>=_l && v.x<_l+_w && v.y>=_t && v.y<_t+_h; }

	int equals( DRect a ) { return _l==a._l && _t==a._t && _w==a._w && _h==a._h; }

	double area() { return _w * _h; }

	int isValid() { return _w >= 0.0 && _h >= 0.0; }

	void unionRect( DRect o );
		// Union with zero sized rects is meaningless
	void unionPoint( double x, double y );
	void clipTo( DRect c );

	int intersects( DRect o ) {
		DRect temp( o );
		temp.clipTo( *this );
		return( temp._w > 0.0 || temp._h > 0.0 );
	}

	void scale( double factor ) { _w *= factor; _h *= factor; }

	void moveTo( double x, double y ) { _l=x; _t=y; }

	void translate( double dx, double dy ) { _l += dx; _t += dy; }

	void deltaSize( double dx, double dy );
};

struct SVec3 {
	short x, y, z;

	SVec3() { x = y = z = (short)0; }
	SVec3( int _x, int _y, int _z ) { x = _x; y = _y; z = _z; }
	SVec3( double _x, double _y, double _z ) { x = (short)_x; y = (short)_y; z = (short)_z; }
	SVec3( short *v ) { x = v[0]; y = v[1]; z = v[2]; }

	void add( SVec3 v ) { x += v.x; y += v.y; z += v.z; }
	void sub( SVec3 v ) { x -= v.x; y -= v.y; z -= v.z; }
	void mul( int c ) { x *= c; y *= c; z *= c; }
	void div( int c ) { x /= c; y /= c; z /= c; }
	void bound( SVec3 v0, SVec3 v1 ) {
		x = x<v0.x ? v0.x : (x>v1.x?v1.x:x);
		y = y<v0.y ? v0.y : (y>v1.y?v1.y:y);
		z = z<v0.z ? v0.z : (z>v1.z?v1.z:z);
	}
	void origin() { x = y = z = 0; }
	void xAxis() { x = 1; y = z = 0; }
	void yAxis() { y = 1; x = z = 0; }
	void zAxis() { z = 1; x = y = 0; }

	double mag();
	int mag2() { return x*x + y*y + z*z; }
	int dot( SVec3 v ) { return x*v.x + y*v.y + z*v.z; }

	void normalize() { int m=(int)mag(); if(m>0){ x/=m; y/=m; z/=m; } }
	int equals( SVec3 v ) { return x==v.x && y==v.y && z==v.z; }

	operator short *() { return (short *)this; }

	static SVec3 Origin;
	static SVec3 XAxis;
	static SVec3 YAxis;
	static SVec3 ZAxis;
	static SVec3 XAxisMinus;
	static SVec3 YAxisMinus;
	static SVec3 ZAxisMinus;
};

struct IVec3 {
	int x, y, z;

	IVec3() { x = y = z = 0; }
	IVec3( int _x, int _y, int _z ) { x = _x; y = _y; z = _z; }
	IVec3( double _x, double _y, double _z ) { x = (int)_x; y = (int)_y; z = (int)_z; }
	IVec3( int *v ) { x = v[0]; y = v[1]; z = v[2]; }

	void add( IVec3 v ) { x += v.x; y += v.y; z += v.z; }
	void sub( IVec3 v ) { x -= v.x; y -= v.y; z -= v.z; }
	void mul( int c ) { x *= c; y *= c; z *= c; }
	void div( int c ) { x /= c; y /= c; z /= c; }
	void bound( IVec3 v0, IVec3 v1 ) {
		x = x<v0.x ? v0.x : (x>v1.x?v1.x:x);
		y = y<v0.y ? v0.y : (y>v1.y?v1.y:y);
		z = z<v0.z ? v0.z : (z>v1.z?v1.z:z);
	}
	void origin() { x = y = z = 0; }
	void xAxis() { x = 1; y = z = 0; }
	void yAxis() { y = 1; x = z = 0; }
	void zAxis() { z = 1; x = y = 0; }

	double mag();
	int mag2() { return x*x + y*y + z*z; }
	int dot( IVec3 v ) { return x*v.x + y*v.y + z*v.z; }

	void normalize() { int m=(int)mag(); if(m>0){ x/=m; y/=m; z/=m; } }

	int equals( IVec3 v ) { return x==v.x && y==v.y && z==v.z; }

	operator int *() { return (int *)this; }

	static IVec3 Origin;
	static IVec3 XAxis;
	static IVec3 YAxis;
	static IVec3 ZAxis;
	static IVec3 XAxisMinus;
	static IVec3 YAxisMinus;
	static IVec3 ZAxisMinus;
};

struct FVec3 {
	union { float r, x; };
	union { float g, y; };
	union { float b, z; };

	FVec3() { x = y = z = 0.0f; }
	FVec3( int _x, int _y, int _z ) { x = (float)_x; y = (float)_y; z = (float)_z; }
	FVec3( float _x, float _y, float _z ) { x = _x; y = _y; z = _z; }
	FVec3( float *v ) { x = v[0]; y = v[1]; z = v[2]; }
	FVec3( IVec3 v ) { x = (float)v.x; y = (float)v.y; z = (float)v.z; }
	FVec3( SVec3 v ) { x = (float)v.x; y = (float)v.y; z = (float)v.z; }
	FVec3( FVec2 v ) { x = (float)v.x; y = (float)v.y; z = 0.f; }

	void add( FVec3 v ) { x += v.x; y += v.y; z += v.z; }
	void sub( FVec3 v ) { x -= v.x; y -= v.y; z -= v.z; }
	void mul( float c ) { x *= c; y *= c; z *= c; }
	void div( float c ) { x /= c; y /= c; z /= c; }
	void bound( FVec3 v0, FVec3 v1 ) {
		x = x<v0.x ? v0.x : (x>v1.x?v1.x:x);
		y = y<v0.y ? v0.y : (y>v1.y?v1.y:y);
		z = z<v0.z ? v0.z : (z>v1.z?v1.z:z);
	}
	void origin() { x = y = z = 0.f; }
	void xAxis() { x = 1.f; y = z = 0.f; }
	void yAxis() { y = 1.f; x = z = 0.f; }
	void zAxis() { z = 1.f; x = y = 0.f; }
	void abs();
	void project( FVec3 a );

	void boundLen( float l ) {
		float m = mag();
		if( m > l ) {
			normalize();
			mul( l );
		}
	}
	void cross(FVec3 b) {
		float _x = y*b.z - z*b.y;
		float _y = z*b.x - x*b.z;
		float _z = x*b.y - y*b.x;
		x = _x;
		y = _y;
		z = _z;
	}

	float mag();
	float mag2() { return x*x + y*y + z*z; }
	float dot( FVec3 v ) { return x*v.x + y*v.y + z*v.z; }

	void normalize() { float m=mag(); if(m>0.f){ x/=m; y/=m; z/=m; } }
	int equals( FVec3 v ) { return x==v.x && y==v.y && z==v.z; }

	FVec3 &operator =(IVec3 v) { x=(float)v.x; y=(float)v.y; z=(float)v.z; return *this; }

	operator float *() { return (float *)this; }

	static float dist( FVec3 v1, FVec3 v2 );
	static float dist2( FVec3 v1, FVec3 v2 );

	static FVec3 Origin;
	static FVec3 XAxis;
	static FVec3 YAxis;
	static FVec3 ZAxis;
	static FVec3 XAxisMinus;
	static FVec3 YAxisMinus;
	static FVec3 ZAxisMinus;
};

inline FVec3 operator * ( FVec3 a, float c ) {
	FVec3 result = a;
	result.mul(c);
	return result;	
}

inline FVec3 operator / ( FVec3 a, float c ) {
	FVec3 result = a;
	result.div(c);
	return result;
}

inline FVec3 operator + ( FVec3 a, FVec3 b ) {
	FVec3 result = a;
	a.add(b);
	return a;
}

inline FVec3 operator - ( FVec3 a, FVec3 b ) {
	FVec3 result = a;
	a.sub(b);
	return a;
}

struct FVec4 {
	union { float r, x; };
	union { float g, y; };
	union { float b, z; };
	union { float a, w; };

	FVec4() { x = y = z = w = 0.0f; }
	FVec4( float _x, float _y, float _z, float _w ) { x = _x; y = _y; z = _z; w = _w; }
	void set( FVec4 v ) { x = v.x; y = v.y; z = v.z; w = v.w; } 

	void add( FVec4 v ) { x += v.x; y += v.y; z += v.z; w += v.w; }
	void sub( FVec4 v ) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; }
	void mul( float c ) { x *= c; y *= c; z *= c; w *= c; }
	void div( float c ) { x /= c; y /= c; z /= c; w /= c; }

	float mag();
	float mag2() { return x*x + y*y + z*z + w*w; }
	float dot( FVec4 v ) { return x*v.x + y*v.y + z*v.z + w*v.w; }

	void normalize() { float m=mag(); if(m>0.f){ x/=m; y/=m; z/=m; w/=m; } }

	operator float *() { return (float *)this; }
};

struct DVec3 {
	union { double r, x; };
	union { double g, y, t; };
	union { double b, z, p; };

	DVec3() { x = y = z = 0.0; }
	DVec3( int _x, int _y, int _z ) { x = _x; y = _y; z = _z; }
	DVec3( double _x, double _y, double _z ) { x = _x; y = _y; z = _z; }
	DVec3( double *v ) { x = v[0]; y = v[1]; z = v[2]; }
	DVec3( IVec3 v ) { x = (double)v.x; y = (double)v.y; z = (double)v.z; }
	DVec3( SVec3 v ) { x = (double)v.x; y = (double)v.y; z = (double)v.z; }
	DVec3( FVec3 v ) { x = (double)v.x; y = (double)v.y; z = (double)v.z; }

	void add( DVec3 v ) { x += v.x; y += v.y; z += v.z; }
	void sub( DVec3 v ) { x -= v.x; y -= v.y; z -= v.z; }
	void mul( double c ) { x *= c; y *= c; z *= c; }
	void div( double c ) { x /= c; y /= c; z /= c; }
	void bound( DVec3 v0, DVec3 v1 ) {
		x = x<v0.x ? v0.x : (x>v1.x?v1.x:x);
		y = y<v0.y ? v0.y : (y>v1.y?v1.y:y);
		z = z<v0.z ? v0.z : (z>v1.z?v1.z:z);
	}
	void origin() { x = y = z = 0.0; }
	void xAxis() { x = 1.0; y = z = 0.0; }
	void yAxis() { y = 1.0; x = z = 0.0; }
	void zAxis() { z = 1.0; x = y = 0.0; }
	void abs();

	void boundLen( double l ) {
		double m = mag();
		if( m > l ) {
			normalize();
			mul( l );
		}
	}
	void cross(DVec3 b) {
		double _x = y*b.z - z*b.y;
		double _y = z*b.x - x*b.z;
		double _z = x*b.y - y*b.x;
		x = _x;
		y = _y;
		z = _z;
	}

	double mag();
	double mag2() { return x*x + y*y + z*z; }
	double dot( DVec3 v ) { return x*v.x + y*v.y + z*v.z; }

	void normalize() { double m=mag(); if(m>0.0){ x/=m; y/=m; z/=m; } }
	int equals( DVec3 v ) { return x==v.x && y==v.y && z==v.z; }

	DVec3 &operator =(IVec3 v) { x=(double)v.x; y=(double)v.y; z=(double)v.z; return *this; }

	operator double *() { return (double *)this; }

	static DVec3 Origin;
	static DVec3 XAxis;
	static DVec3 YAxis;
	static DVec3 ZAxis;
	static DVec3 XAxisMinus;
	static DVec3 YAxisMinus;
	static DVec3 ZAxisMinus;
};

struct DVec4 {
	union { double r, x; };
	union { double g, y; };
	union { double b, z; };
	union { double a, w; };

	DVec4() { x = y = z = w = 0.0; }
	DVec4( double _x, double _y, double _z, double _w ) { x = _x; y = _y; z = _z; w = _w; }

	void add( DVec4 v ) { x += v.x; y += v.y; z += v.z; w += v.w; }
	void sub( DVec4 v ) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; }
	void mul( double c ) { x *= c; y *= c; z *= c; w *= c; }
	void div( double c ) { x /= c; y /= c; z /= c; w /= c; }

	double mag();
	double mag2() { return x*x + y*y + z*z + w*w; }
	double dot( DVec4 v ) { return x*v.x + y*v.y + z*v.z + w*v.w; }

	void normalize() { double m=mag(); if(m>0.0){ x/=m; y/=m; z/=m; w/=m; } }

	operator double *() { return (double *)this; }
};

struct FMat3 {
	float m[3][3];
		// This is in m[col][row] format to match gl

	FMat3();
	FMat3( float b[3][3] );
	FMat3( float b[9] );

	void cat( const FMat3 &b );
	FVec3 mul( FVec3 v );

	float determinant();
	void adjoint();
	int inverse();
		// Uses Cramer's rule.  Return 0 on failure otherwise 1;

	void identity();
	void transpose();
	void orthoNormalize();
	void add( FMat3 b );
	void sub( FMat3 b );
	void mul( float s );
	void div( float s );
	void skewSymetric( FVec3 cross );

	static FMat3 Identity;
};

struct DMat3 {
	double m[3][3];
		// This is in m[col][row] format to match gl

	DMat3();
	DMat3( double b[3][3] );
	DMat3( double b[9] );

	void cat( const DMat3 &b );
	DVec3 mul( DVec3 v );

	double determinant();
	void adjoint();
	int inverse();

	void identity();
	void transpose();
	void orthoNormalize();
	void add( DMat3 &b );
	void sub( DMat3 &b );
	void mul( double b );
	void div( double s );
	void skewSymetric( DVec3 cross );

	static DMat3 Identity;
};

struct FMat4 {
	float m[4][4];
		// This is in m[col][row] format to match gl

	FMat4();
	FMat4( double b[4][4] );
	FMat4( float b[4][4] );
	FMat4( float b[16] );
	FMat4( FMat3 orient, FVec3 pos );

	void set( FMat3 &orient, FVec3 pos );
	void cat( const FMat4 &b );
	void add( FMat4 &b );
	FVec3 mul( FVec2 v );
	FVec3 mul( FVec3 v );
	FVec4 mul( FVec4 v );
	void mul( float v[4] );
	void mul( float v[4], float o[4] );
	void mul( float s );
	FVec3 getTrans() { return FVec3(m[3][0],m[3][1],m[3][2]); }
	void setTrans( FVec3 v ) { m[3][0]=v.x; m[3][1]=v.y; m[3][2]=v.z; }
	void trans( FVec3 v ) { m[3][0]+=v.x; m[3][1]+=v.y; m[3][2]+=v.z; }
	void identity();
	void pivot( FVec3 axis, float angleRad ); // remove translation, rotate, reapply
	int inverse(); // return 1 on success
	void transpose();
	void orthoNormalize();

	operator float *() { return &m[0][0]; }

	static FMat4 Identity;
};

struct DMat4 {
	double m[4][4];
	// This is in m[col][row] form to match gl

	DMat4();
	DMat4( double b[4][4] );
	DMat4( double b[16] );
	DMat4( DMat3 &orient, DVec3 pos );
	DMat4( FMat4 & );

	void set( DMat3 &orient, DVec3 pos );
	void cat( const DMat4 &b );
	DVec3 mul( DVec2 v );
	DVec3 mul( DVec3 v );
	DVec4 mul( DVec4 v );
	DVec3 getTrans() { return DVec3(m[3][0],m[3][1],m[3][2]); }
	void setTrans( DVec3 v ) { m[3][0]=v.x; m[3][1]=v.y; m[3][2]=v.z; }
	void trans( DVec3 v ) { m[3][0]+=v.x; m[3][1]+=v.y; m[3][2]+=v.z; }
	void identity();
	void pivot( DVec3 axis, double angleRad );	// remove translation, rotate, reapply
	int inverse();  // return 1 if success
	void transpose();
	void orthoNormalize();

	operator double *() { return &m[0][0]; }

	static DMat4 Identity;
};

struct FQuat {
	float q[4];
		// In x, y, z, w order

	FMat4 mat();
	void mul( FQuat &o );
	float mag();
	void normalize();
	void conjugate();

	void toAxisAngle( FVec3 &axis, float &radians );
	void fromAxisAngle( FVec3 &axis, float radians );
	void fromEulerAngles( float x, float y, float z );

	void identity() { q[0] = 0.f; q[1] = 0.f; q[2] = 0.f; q[3] = 1.f; }
	FQuat() { q[0] = 0.f; q[1] = 0.f; q[2] = 0.f; q[3] = 1.f; }
	FQuat( FVec3 &axis, float radians ) { fromAxisAngle( axis, radians ); }
	FQuat( FMat4 &mat );
		// Ignores the translation, converts 3x3 rotation matrix into a quaternion.
		// Expects that the mat is a orthonormal matrix
};

// Airplane. The airplane is facing down the positive Z axis with up as Y
// Rotation 1 is about Y and is called yaw.    -PI to PI
// Rotation 2 is about X and is called pitch.  -PI to PI
// Rotation 3 is about Z and is called roll.   -PI to PI
// Note that this means that this means that pitch is negative when
// the nose of the plane is pointing up toward the positive y axis.
// The matrix from this is equivilent to:
//		glRotatef( radToDegF(yaw), 0.f, 1.f, 0.f );
//		glRotatef( radToDegF(pitch), 1.f, 0.f, 0.f );
//		glRotatef( radToDegF(roll), 0.f, 0.f, 1.f );
FMat3 rotate3D_3x3_EulerAirplane( float yaw, float pitch, float roll );
void mat3ToEulerAirplane( FMat3 mat, float &yaw, float &pitch, float &roll );

// Euler Angles MAX.  Bones are facing down the X axis.
// Rotation 1 is about Y and called yaw. -PI to PI
// Rotation 2 is about Z and called pitch -PI to PI
// Rotation 3 is about X and is called roll. -PI to PI
FMat4 rotate3D_EulerAnglesMax( float yaw, float pitch, float roll );
void mat4ToEulerAnglesMax( FMat4 mat, float &yaw, float &pitch, float &roll );

extern FMat3 rotate3D_3x3( FVec3 axis, float angleRad );
extern DMat3 rotate3D_3x3( DVec3 axis, double angleRad );
extern FMat4 rotate3D( FVec3 axis, float angleRad );
extern DMat4 rotate3D( DVec3 axis, double angleRad );
extern FMat4 rotate3D( FVec3 axis, FVec3 dir);

extern FMat3 scale3D_3x3( FVec3 s );
extern DMat3 scale3D_3x3( DVec3 s );
extern FMat4 scale3D( FVec3 s );
extern DMat4 scale3D( DVec3 s );

extern FMat3 trans2D_3x3( FVec2 tr );
extern FMat3 trans2D_3x3( float x, float y );
extern DMat3 trans2D_3x3( DVec2 tr );
extern DMat3 trans2D_3x3( double x, double y );
extern FMat4 trans3D( float x, float y, float z );
extern FMat4 trans3D( FVec3 trans );
extern DMat4 trans3D( double x, double y, double z );
extern DMat4 trans3D( DVec3 trans );

extern FMat3 orientFromHomogenous( FMat4 mat );


#endif

