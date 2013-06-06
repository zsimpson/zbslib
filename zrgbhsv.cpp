// @ZBS {
//		*MODULE_NAME RGB to HSV conversion
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Conversion from RGB space to HSV and vice-versa
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zrgbhsv.cpp zrgbhsv.h
//		*VERSION 1.1
//		+HISTORY {
//			1.1 Made the naming convension more consistent
//		}
//		+TODO {
//			consider isolating the bitmap function in this module
//		}
//		*SELF_TEST no
//		*PUBLISH yes
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
#include "assert.h"
#include "memory.h"
#include "math.h"
// MODULE includes:
#include "zrgbhsv.h"
// ZBSLIB includes:
#include "zbitmapdesc.h"

#ifndef min
	#define min(a,b) ( ( (a) < (b) ) ? (a) : (b) )
#endif
#ifndef max
	#define max(a,b) ( ( (a) > (b) ) ? (a) : (b) )
#endif

typedef unsigned char uchar;

// r,g,b values are from 0 to 1
// h = [0,1], s = [0,1], v = [0,1]
//		if s == 0, then h = -1 (undefined)

void zRGBF_to_HSVF( float r, float g, float b, float &h, float &s, float &v ) {
	float _min = min( r, min( g, b ) );
	float _max = max( r, max( g, b ) );
	v = _max;
	float delta = _max - _min;
	if( _max != 0 ) {
		s = delta / _max;
	}
	else {
		// r = g = b = 0; s = 0, v is undefined
		s = 0;
		h = 0;
		return;
	}
	if( delta == 0.f ) {
		// r = g = b; s = 0, h is undefined
		s = 0;
		h = 0;
		return;
	}
	if( r == _max ) h = ( g - b ) / delta;  // between yellow & magenta
	else if( g == _max ) h = 2 + ( b - r ) / delta;  // between cyan & yellow
	else h = 4 + ( r - g ) / delta;  // between magenta & cyan
	h *= 60;  // degrees
	if( h < 0 ) h += 360.f;
	h /= 360.f;
}

// HSV
void zHSVF_to_RGBF( float h, float s, float v, float &r, float &g, float &b ) {
	h *= 360.f;
	int i;
	float f, p, q, t;
	if( s == 0 ) {
		// achromatic (grey)
		r = g = b = v;
		return;
	}
	h /= 60.f;			// sector 0 to 5
	i = (int)floor( h );
	f = h - i;			// factorial part of h
	p = v * ( 1.f - s );
	q = v * ( 1.f - s * f );
	t = v * ( 1.f - s * ( 1.f - f ) );
	switch( i ) {
		case 0: r = v; g = t; b = p; break;
		case 1: r = q; g = v; b = p; break;
		case 2: r = p; g = v; b = t; break;
		case 3: r = p; g = q; b = v; break;
		case 4: r = t; g = p; b = v; break;
		default: r = v; g = p; b = q; break;
	}
}

int zHSVAF_to_RGBAI( float h, float s, float v, float a ) {
	float r, g, b;
	zHSVF_to_RGBF( h, s, v, r, g, b );
	return zRGBAF_to_RGBAI( r, g, b, a );
}

int zRGBAF_to_RGBAI( float r, float g, float b, float a ) {
	unsigned int rr = (unsigned int)( r * 255.f ) & 0xff;
	unsigned int gg = (unsigned int)( g * 255.f ) & 0xff;
	unsigned int bb = (unsigned int)( b * 255.f ) & 0xff;
	unsigned int aa = (unsigned int)( a * 255.f ) & 0xff;
	return rr<<24 | gg<<16 | bb<<8 | aa;
		// Changed to match gui
}

void zRGBAI_to_RGBAF( int rgba, float &r, float &g, float &b, float &a ) {
	unsigned int rr = (rgba&0xFF000000) >> 24;
	unsigned int gg = (rgba&0x00FF0000) >> 16;
	unsigned int bb = (rgba&0x0000FF00) >> 8;
	unsigned int aa = (rgba&0x000000FF) >> 0;
	r = (float)rr / 255.f;
	g = (float)gg / 255.f;
	b = (float)bb / 255.f;
	a = (float)aa / 255.f;
		// Changed to match gui
}

void zRGB_to_HSV_Bitmap( ZBitmapDesc *rgb, ZBitmapDesc *hsv ) {
	assert( rgb->d >= 3 && hsv->d >= 3 && rgb->w == hsv->w && rgb->h == hsv->h );
	for( int y=0; y<rgb->h; y++ ) {
		uchar *src = rgb->lineUchar( y );
		uchar *dst = hsv->lineUchar( y );
		for( int x=0; x<rgb->w; x++ ) {
			uchar r = *src++;
			uchar g = *src++;
			uchar b = *src++;
			float h, s, v;
			zRGBF_to_HSVF( (float)r / 255.f, (float)g / 255.f, (float)b / 255.f, h, s, v );
			*dst++ = (uchar)(h * 255.f);
			*dst++ = (uchar)(s * 255.f);
			*dst++ = (uchar)(v * 255.f);
		}
	}
}

void zHSV_to_RGB_Bitmap( ZBitmapDesc *hsv, ZBitmapDesc *rgb ) {
	assert( rgb->d >= 3 && hsv->d >= 3 && rgb->w == hsv->w && rgb->h == hsv->h );
	for( int y=0; y<rgb->h; y++ ) {
		uchar *src = hsv->lineUchar( y );
		uchar *dst = rgb->lineUchar( y );
		for( int x=0; x<rgb->w; x++ ) {
			uchar h = *src++;
			uchar s = *src++;
			uchar v = *src++;
			float r, g, b;
			zHSVF_to_RGBF( (float)h / 255.f, (float)s / 255.f, (float)v / 255.f, r, g, b );
			*dst++ = (uchar)(r * 255.f);
			*dst++ = (uchar)(g * 255.f);
			*dst++ = (uchar)(b * 255.f);
		}
	}
}

