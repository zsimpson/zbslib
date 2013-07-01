

#ifndef TBUTIL_H
#define TBUTIL_H

#include "zvec.h"
#include "ztl.h"

// Various utility code tfb uses in his plugs; perhaps factor to better homes later.

//-----------------------------------------------------------------------------

#ifndef min
	#define min(a,b) ( ( (a) < (b) ) ? (a) : (b) )
#endif
#ifndef max
	#define max(a,b) ( ( (a) > (b) ) ? (a) : (b) )
#endif

//-----------------------------------------------------------------------------
// ZMsg helper macro

#define STRING_TO_PTR(hash,key) (void*)( atol( hash->getS( #key, "0" ) ) )
	// this is used such that you can more easily pass pointers in messages
	// and extract them (with this macro) in handlers
	//	
	// zMsgQueue( "type=TestPointer ptr=%ld", (long)this );
			// this gets encoded as a base10 string
	//
	// ZMSG_HANDLER( TestPointer ) {
	//	    void* ptr = STRING_TO_PTR( msg, ptr );
	//  }

//-----------------------------------------------------------------------------
// string hanlding

char *chompWhite( char *s );
	// remove trailing whitespace by overwriting with 0x0
char *chompWhiteLeading( char *s, int chompTabs=1 );
	// return a char* that is offset into passed s to skip leading whitespace
	// See warning in function definition; flag allows tabs to be retained for
	// tab-delimited files, in which columns of data may require leading tabs
char *chompWhiteLeadingTrailing( char *s );
	// skip/remove leading and trailing whitespace using above

char * getTail( char *s, int len, int prependEllipses=1 );
	// get last len chars of s, with optional prepended ...

char * formatFloat( double val, int sig=3, char formatCode='g', int width=-1, bool leftJustify=true );
	// format floating point value for display with specified significant
	// digits and display format.  formatCode is the printf code used to
	// format the output: f=fixed, e=scientific, g=shortest, etc.
	// width may be specified to pad with spaces to given width.
	// justification only applies when width > 0
extern int FF_USETABS;
	// a global flag controlling formatFloat space/tab usage;


char * replaceExtension( char *filepath, char *newExt, bool lastOnly=1 );
	// replace last or multiple extensions with new string. 

char *lowerString( char *s );
	// return static that holds lc version of string




//-----------------------------------------------------------------------------
// field validation

int isNumericValue( char *text, double *val=0 );

//-----------------------------------------------------------------------------
// color utils

struct FVec3;
void getSpectralRGB( float val, FVec3 &color, float range1ColorBoundary=-1, int grayscale=0, int boundaryGap=1, int colorBandSize=1 );
	// in: val [0,1] 
	// in: range1ColorBoundary - where the discontinuity from reds to greens is
	// in: grayscale -- if 1, return grayscale equivalent
	// in: boundaryGap
	// in: colorBandSize
	// out: color -- color in rgb space (for spectral plots)

void spectralLegend( float _x, float _y, float width, float height, float minVal, float maxVal, int invert=1  );
	// SPECTRAL COLOR LEGEND in 2d




#define PLOTCOLORS_COUNT 12
extern const unsigned char plotColors[PLOTCOLORS_COUNT][3];
	// some standard plot colors

const unsigned char * getPlotColorPtr( int index, int dark=0 );
	// standard plot colors, returns ptr to a series of 3 bytes
int getPlotColorI( int index, int _alpha=255, int dark=0 );
	// standard plot colors in packed int format
float * getPlotColor3fv( int index, int dark=0 );
	// standard plot colors, 3fv format
void getPlotColorFVec4( int index, FVec4 & color, float _alpha=1.f, int dark=0 );
	// init FVec4 with standard plot color

//------------------------------------------------------------------------------
// memory debug
void printHeap( char *msg, bool resetCounters=false );

#endif