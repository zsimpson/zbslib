// @ZBS {
//		*MODULE_DEPENDS tbutil.cpp tbutil.h
// }

// MODULE
#include "tbutil.h"

// ZBSLIB
#include "zrgbhsv.h"
#include "ztmpstr.h"
#include "zregexp.h"

// C
//#define MALLOC_DEBUG
	// you also have to link the mallocdebug_heap/linear.cpp module from zbslib for some fns
#ifdef MALLOC_DEBUG
	// see functions depending on this #define at bottom of file
	#include "stdarg.h"
	#include "string.h"
#endif
#include "ctype.h"
#include "stdio.h"

//-----------------------------------------------------------------------------------------
// dealing with char* whitespace
//-----------------------------------------------------------------------------------------

char * chompWhite( char *s ) {
	// like perl chomp, but remove all kinds of trailing whitespace
	// (not just \r, \n)
	char *p = s + strlen( s ) - 1;
	while( p>=s && isspace( *p ) ) {
		*p-- = 0x0;
	}
	return s;
}

char * chompWhiteLeading( char *s, int chompTabs ) {
	// usage: char *offsetIntoS = chompWhiteLeading( s );

	// WARNING: if s is result of malloc, then be sure to
	// free( s ), and not free the returned value, which
	// will not point to the starting malloc address!

	while( *s && isspace( *s ) && (chompTabs || *s!='\t')) {
		*s++;
			// note that we do not write 0x0 into the string:
			// we simply increment, and return the pointer to
			// the first non-space char; see warning.
	}
	return s;
}

char * chompWhiteLeadingTrailing( char *s ) {
	return chompWhiteLeading( chompWhite( s ) );
}

char * getTail( char *s, int len, int prependEllipses ) {
	static char result[255];
	int slen = strlen( s );
	if( slen <= len + 3 ) {
		// +3 fudge for potential ellipses; really this fn was written to
		// solve a problem that a font->getlength should be solving.
		return s;
	}
	assert( slen < 255 && len < 255 );
	*result=0;
	if( prependEllipses ) {
		strcpy( result, "..." );
	}
	int offset = slen - len - ( prependEllipses ? 3 : 0 );
	strcat( result, s + max( 0, offset ) );
	return result;
}

//-----------------------------------------------------------------------------------------
// field validation
//-----------------------------------------------------------------------------------------
ZRegExp numericValue( "^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?$" );
int isNumericValue( char *text, double *val ) {
	if( text && *text ) {
		if( numericValue.test( text )) {
			if( val ) {
				*val = atof( text );
			}
			return 1;
		}
	}
	return 0;
}


/* Replaced 9 june 2013
int isNumericValue( char *text, double *val ) {
	// Is the passed text a valid numeric value?  That is, the text
	// must be nonempty, and each character valid as part of a numeric string.
	// Note that we don't validate the order, so actually it is possible
	// to call something like "EEEEe.---+" a numeric value.
	
	// ZRegExp floatNum( "" );
	// @TODO: regex for actually validating a string as numeric value
	
	if( text && *text ) {
		^[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?$
		char *p = text;
		while( *p ) {
			if( !isdigit( *p ) && *p!='.' && *p!='-' && *p!='+' && *p!='e' && *p!='E' ) {
				return 0;
			}
			p++;
		}
		if( val ) {
			*val = atof( text );
		}
		return 1;
	}
	return 0;
	
}
*/

//-----------------------------------------------------------------------------------------
// format a floating-point value for display with a given number of significant digits
// WARNING: a rotating list of FF_BUFFERCOUNT static buffers is used, so calls beyond this
// start overwriting the results of previous calls.
//
// This is a pretty slow routine, intended for non-perf-critical display/output type use
// See kinutil.h for default values: only val need be specified, which results in a string
// with 3 significant digits formatted with %g, with no padding/justification.

#define FF_BUFFERCOUNT 4
#define FF_BUFFERSIZE  64 

int FF_USETABS=0;
	// a global flag that can be set for program-wide or selective use of 
	// tab chars instead of spaces for padding.

char * formatFloat( double val, int sig, char formatCode, int width, bool leftJustify  ) {
	static char buffer[FF_BUFFERCOUNT][FF_BUFFERSIZE];
	static int whichBuffer=0;
	char *currentBuf  = buffer[ whichBuffer++ ];
	if( whichBuffer >= FF_BUFFERCOUNT ) {
		whichBuffer = 0;
	}

	double displayValue = val;
	if( sig > 0 ) {
		ZTmpStr formatter( "%%%d.%de", sig, sig-1 );
		displayValue = atof( ZTmpStr( formatter.s , val ) );
	}
	const char *formatDouble = formatCode == 'g' ? "%g" : formatCode == 'e' ? "%e" : "%lf";
	sprintf( currentBuf, formatDouble, displayValue );
	
	// PAD with spaces if a fixed width was requested.  Note we don't chop,
	// only increase the width if necessary.
	assert( width < FF_BUFFERSIZE && "Increase FF_BUFFERSIZE in formatFloat!" );
	if( width > 0 && width < FF_BUFFERSIZE ) {
		if( FF_USETABS ) {
			strcat( currentBuf, "\t" );
		}
		else {
			char workBuffer[FF_BUFFERSIZE];
			ZTmpStr formatter( "%%%s%ds", leftJustify ? "-" : "", width );
			sprintf( workBuffer, formatter, currentBuf );
			strcpy( currentBuf, workBuffer );
		}
	}
	return currentBuf;
}

//-----------------------------------------------------------------------------------------

char * replaceExtension( char *filepath, char *newExt, bool lastOnly ) {
	// SET or REPLACE extension in filepath. This will replace multitple
	// extenstions in cases like myfile.ext.ext
	// RETURNS pointer to this static: (be careful!)
	static char replaceExtensionResults[512];
	assert( newExt && filepath && strlen( filepath ) + strlen( newExt ) < 512 );

	strcpy( replaceExtensionResults, filepath );
	char *p = replaceExtensionResults + strlen( filepath );
	while( --p > replaceExtensionResults && *p != '\\' && *p != '/' ) {
		if( *p == '.' ) {
			*p = 0;
			if( lastOnly ) {
				break;
			}
		}
	}
	if( newExt[0] != '.' ) {
		strcat( replaceExtensionResults, "." );
	}
	strcat( replaceExtensionResults, newExt );
/*  OLD WAY (pre 05aug2008):
	ZFileSpec fs( filepath );
	char *ext = fs.getExt();
	if( !*ext ) {
		sprintf( replaceExtensionResults, "%s.%s", fs.get(), newExt );
	}
	else if( !strcmp( ext, newExt ) ) {
		strcpy( replaceExtensionResults, filepath );
	}
	else {
		char extWithPeriodRegEx[16];
		char newExtWithPeriod[16];
		sprintf( extWithPeriodRegEx, "\\.%s", ext );
		sprintf( newExtWithPeriod, ".%s", newExt );
		ZStr fname( fs.get() );
		zStrReplace( &fname, extWithPeriodRegEx, newExtWithPeriod );	
		strcpy( replaceExtensionResults, fname.getS() );
	}
*/

	return replaceExtensionResults;
}

//-----------------------------------------------------------------------------------------
// like lc in perl

char * lowerString( char *s ) {
	static char buffer[256];
	assert( strlen( s ) < 256 );
	
	char *src = s;
	char *dst = buffer;
	while( *src ) {
		*dst++ = tolower(*src++);
	}
	return buffer;
}

//-----------------------------------------------------------------------------------------
// Color utility for mapping [0,1] to "spectral" rgb color value as seen in many false
// color plots of scientific data.  Naive approach.  ;)  

FVec3 *spectralColors=0;
int    spectralColorsCount=0;

void getSpectralRGB( float val, FVec3 &color, float range1BoundaryValue, int grayscale, int boundaryGap, int colorBandSize ) {
	// range1BoundaryValue is an optional parameter used to create a discontinuity 
	// in the color space for better visualization of values above and below
	// the prescribed value: if provided, all values falling below the boundaryValue
	// will return a color in the first range, and all values falling above will 
	// return a color chosen from the subsequent ranges.

	// init r=255, g=0, b=0
	// g ramps up to 255  -> reds to orange to yellow 
	// r ramps dn to 0    -> yellow to green
	// b ramps up to 255  -> green to cyan
	// g ramps down to 0  -> cyan to blue
	// r ramps up to 255  -> blue to violet

	// do this with structs describing nsteps to allow us to expand/compress
	// ranges to taste.

	struct ramp {
		int steps;		// n steps
		int component;	// 0,1,2 -> r,g,b
		int direction;  // -1,1
	};

	/*
	ramp _ramps[5] = {
		{ 52, 1, +1	},	// ramp up green (range goes from red to yellow)
		{ 52, 0, -1	},	// ramp dn red   (range goes from yellow to green)
		{ 52, 2, +1 },  // ramp up blue	 (range goes from green to cyan)
		{ 52, 1, -1 },  // ramp dn green (range goes from cyan to blue)	
		{ 51, 0, +1 },  // ramp up red   (range goes from blue to purple)
			// nsteps may be arbitrary values; this setting produces a 256 color palette.
	};
	*/

	// version that goes from blue to red
	ramp _ramps[4] = {
		{ 52, 1, +1	},	// ramp up green
		{ 52, 0, -1	},	// ramp dn red
		{ 52, 2, +1 },  // ramp up blue
		{ 52, 1, -1 },  // ramp dn green
		//{ 51, 0, +1 },  // ramp up red
			// nsteps may be arbitrary values; this setting produces a 256 color palette.
	};


	if( !spectralColors ) {
		int nramps = sizeof(_ramps) / sizeof( ramp );
		int paletteSize=0;
		int i;
		for( i=0; i<nramps; i++ ) {
			paletteSize += _ramps[i].steps;
		}
		if( paletteSize ) {
			spectralColors = new FVec3[ paletteSize ];
		}
		if( spectralColors ) {
			spectralColorsCount = paletteSize;						
			float r=1.f;
			float g=0.f;
			float b=0.f;
			int index=0;
			for( i=0; i<nramps; i++ ) {
				float delta = 1.f / (_ramps[i].steps - 1);
				for( int j=0; j<_ramps[i].steps; j++, index++ ) {
					spectralColors[index].r = r;
					spectralColors[index].g = g;
					spectralColors[index].b = b;
					r += _ramps[i].component == 0 ? delta * _ramps[i].direction : 0.f;
					g += _ramps[i].component == 1 ? delta * _ramps[i].direction : 0.f;
					b += _ramps[i].component == 2 ? delta * _ramps[i].direction : 0.f;
					r = min( r, 1.f ); r = max( r, 0.f);
					g = min( g, 1.f ); g = max( g, 0.f);
					b = min( b, 1.f ); b = max( b, 0.f);
				}
			}
		}

		// Temp util: write out these files to a text file so that I can copy them
		// into the python code for use when exporting spectra as EPS.
		/*
		FILE *f = fopen( "/tmp/spec.txt", "wt" );
		fprintf( f, "plotColorSpectraCount = %d\n", spectralColorsCount );
		fprintf( f, "plotColorSpectra = [\n" );
		for( int i=0; i<spectralColorsCount; i++ ) {
			fprintf( f, "\t[ %.3f, %.3f, %.3f ],\n", spectralColors[i].r, spectralColors[i].g, spectralColors[i].b );
		}
		fprintf( f, "]\n" );
		*/
	}

	if( spectralColors && spectralColorsCount ) {
		// we could lerp the colors.  but probably unnecessary.
		val = min( val, 1.f ); val = max( val, 0.f);
		int index = (int)(val * (spectralColorsCount-1));
		if( range1BoundaryValue > 0 ) {
			int pureYellow = _ramps[0].steps - 1;
				// the color that fits at the boundary should take on
			if( val <= range1BoundaryValue  * .95f ) {
				// color below pureyellow
				index = (int)( val/range1BoundaryValue * ( pureYellow - boundaryGap ) );
			}
			else if ( val >= range1BoundaryValue * 1.05f ) {
				// skip past yellow
				val = val - range1BoundaryValue;
				val = val / (1.f - range1BoundaryValue);

				int offset = pureYellow + boundaryGap;
				index = (int)( val * (spectralColorsCount - offset - 1 ) + offset );
			}
			else {
				index = pureYellow;
			}
		}

		index = ( index / colorBandSize ) * colorBandSize;
			// granulate for better visibility

		if( grayscale ) {
			float f = 1.f - (float)index / (float)spectralColorsCount;
			color = FVec3( f, f, f );
		}
		else {
			color = spectralColors[ index ];
		}
	}
}

#include "wingl.h"
// SDK includes:
#ifdef __APPLE__
#include "OpenGL/gl.h"
#else
#include "GL/gl.h"
#endif
#include "zglfont.h"
void spectralLegend( float _x, float _y, float width, float height, float minVal, float maxVal, int invert  ) {
	// SPECTRAL COLOR LEGEND in 2d

	glBegin( GL_QUAD_STRIP );
	FVec3 colorq;
	float stepy = height / 64.f;
	for( int q=0; q<=64; q++ ) {
		float val = q/64.f;
		if( invert ) { val = 1.f - val; }
		getSpectralRGB( val, colorq );
		glColor3fv( colorq );
		glVertex2f( _x + 0.f, _y + stepy * q );
		glVertex2f( _x + width, _y + stepy * q );
	}
	glEnd();
	
	#define LEGEND_COUNT 5
	float dstQuart = (maxVal - minVal) / 4.f;
	float legendValues[LEGEND_COUNT] = { minVal, minVal + dstQuart, minVal + dstQuart*2.f, minVal + dstQuart*3.f, maxVal };
	ZTmpStr label;
	stepy = height / ( LEGEND_COUNT - 1 );
	for( int i=0; i<LEGEND_COUNT; i++ ) {
		double val = ( legendValues[ i ] - minVal ) / ( maxVal - minVal );
		if( invert ) { val = 1.0 - val; }

		label.set( "%s", formatFloat( legendValues[ i ], 4 ) );

		getSpectralRGB( (float)val, colorq );
		glColor3fv( colorq ); 
		zglFontPrint( label.s, _x + width, _y + stepy*i - 7, "controls" );
			// -7 is to roughly center the text on the legend value
	}
}

//-----------------------------------------------------------------
// standard plot colors: by tfb, originall for kinexp

const unsigned char plotColors[PLOTCOLORS_COUNT][3] = {
	{0xFF,0x00,0x00},
	{0x00,0xFF,0x00},
	{0x00,0x00,0xFF},
	{0xFF,0xFF,0x00},
	{0x00,0xFF,0xFF},
	{0xFF,0x00,0xFF},
	{0x00,0xAA,0x00},
	{0x99,0x66,0xFF},
	{0xFF,0x66,0x00},
	{0x33,0x99,0xFF},
	{0xFF,0x99,0xFF},
	{0x00,0x99,0x66},
};

const unsigned char plotColorsDark[PLOTCOLORS_COUNT][3] = {
	{0xBB,0x00,0x00},
	{0x00,0xBB,0x00},
	{0x00,0x00,0xBB},
	{0xBB,0xBB,0x00},
	{0x00,0xBB,0xBB},
	{0xBB,0x00,0xBB},
	{0x00,0x66,0x00},
	{0x66,0x00,0xBB},
	{0xBB,0x22,0x00},
	{0x00,0x66,0xCC},
	{0xFF,0x33,0xFF},
	{0x00,0x66,0x66},
};

const unsigned char * getPlotColorPtr( int index, int dark ) {
	index %= PLOTCOLORS_COUNT;
	if( dark ) {
		return plotColorsDark[index];
	}
	else {
		return plotColors[index];
	}
}

int getPlotColorI( int index, int _alpha, int dark ) {
	index %= PLOTCOLORS_COUNT;
	if( dark ) {
		return zRGBAI( plotColorsDark[index][0], plotColorsDark[index][1], plotColorsDark[index][2], _alpha );
	}
	else {
		return zRGBAI( plotColors[index][0], plotColors[index][1], plotColors[index][2], _alpha );
	}
}

float * getPlotColor3fv( int index, int dark ) {
	index %= PLOTCOLORS_COUNT;
	static float color3fv[3];
	if( dark ) {
		color3fv[0] = plotColorsDark[index][0] / 255.f;
		color3fv[1] = plotColorsDark[index][1] / 255.f;
		color3fv[2] = plotColorsDark[index][2] / 255.f;
	}
	else {
		color3fv[0] = plotColors[index][0] / 255.f;
		color3fv[1] = plotColors[index][1] / 255.f;
		color3fv[2] = plotColors[index][2] / 255.f;
	}
	return color3fv;
}

void getPlotColorFVec4( int index, FVec4 & color, float _alpha, int dark ) {
	index %= PLOTCOLORS_COUNT;
	if( dark ) {
		color.r = plotColorsDark[index][0]/255.f;
		color.g = plotColorsDark[index][1]/255.f;
		color.b = plotColorsDark[index][2]/255.f;
	}
	else {
		color.r = plotColors[index][0]/255.f;
		color.g = plotColors[index][1]/255.f;
		color.b = plotColors[index][2]/255.f;
	}
	color.a = _alpha;
}

//-----------------------------------------------------------------------------------------
// The below is a hack for getting some interim display results onscreen when in a single-
// threaded model while a long computation is taking place.

/*
#include "GL/glfw.h"
#include "ztime.h"
#include "zfilespec.h"
extern void render();
void asyncRender() {
	// RENDER
	::render();	
		// from main.cpp
	
	// REFRESH
	glFlush();
	glfwSwapBuffers();
	zTimeSleepMils( 1 );
}
*/

//----------------------------------------------------------------------------------
// memory debug util
// enable at top of file with #define (C headers are required)
#ifdef MALLOC_DEBUG
#include "mallocdebug.h"
extern void heapStatusEx( int &allocCount, int &allocBytes, int &freeCount, int &freeBytes );

int allocCount, allocBytes, beginCount, beginBytes, heapMark, lastCount, lastBytes;
int freeCount, freeCountBegin, freeCountLast, freeBytes, freeBytesLast, freeBytesBegin;
int iteration=-1;

void printHeap( char *msg, bool resetCounters ) {
	
	//heapCheck( 1 );
	heapStatusEx( allocCount, allocBytes, freeCount, freeBytes  );

	if( iteration == -1 /*|| resetCounters*/ ) {
		resetCounters = true;
		lastCount = beginCount = allocCount;
		lastBytes = beginBytes = allocBytes;
		freeCountLast = freeCountBegin  = freeCount;
	}
	else {
		resetCounters=false;
	}

	FILE *heapFile = fopen( "printHeap.txt", ++iteration==0 ? "wt" : "at" );
	heapMark = heapGetMark();
	if( resetCounters ) {
		printf( "%04d %-20s  <reset counters>\n", iteration, msg ? msg : "<no msg>" );
		fprintf( heapFile, "%04d  %-20s  <reset counters>\n", iteration, msg ? msg : "<no msg>"  );
	}
	else {
		printf( "-----------------------------------------------------------------------------\n" );
		printf( "%04d %-20s  Totals: allocs=%04d frees=%04d atebytes=%d\n", iteration, msg ? msg : "<no msg>", allocCount-beginCount, freeCount-freeCountBegin, allocBytes-beginBytes );
		printf( "     %-20s  LastOp: allocs=%04d frees=%04d atebytes=%d\n", "", allocCount-lastCount, freeCount-freeCountLast, allocBytes-lastBytes );
		
		fprintf( heapFile, "---------------------------------------------------------------------------------\n" );
		fprintf( heapFile, "%04d %-20s  Totals: allocs=%04d frees=%04d atebytes=%d\n", iteration, msg ? msg : "<no msg>", allocCount-beginCount, freeCount-freeCountBegin, allocBytes-beginBytes );
		fprintf( heapFile, "     %-20s  LastOp: allocs=%04d frees=%04d atebytes=%d\n", "", allocCount-lastCount, freeCount-freeCountLast, allocBytes-lastBytes );
	}
	//heapDump( 0, heapFile, heapMark );
	
	lastCount = allocCount;
	lastBytes = allocBytes;
	freeCountLast = freeCount;
	fclose ( heapFile );
}

// sample use:
/*
	printHeap( "Begin op1" );

	void *p[100];
	for( int i=0;i<100;i++) {
		p[i] = malloc( 1000 );
	}


	printHeap( "Begin op2" );
	void *q[100];
	for( i=0;i<100;i++) {
		q[i] = malloc( 1000 );
		free(q[i]);
	}
	malloc( 1000);
		// memory leak!
	printHeap( "Done op2" );


	for( i=0;i<100;i++) {
		delete p[i];
	}
	printHeap( "Done op1" );
	return;
 
 

*/
#endif




