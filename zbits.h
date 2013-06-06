// @ZBS {
//		*MODULE_OWNER_NAME zbits
// }

#ifndef ZBITS_H
#define ZBITS_H

struct ZBits {
	// This is an Intel Image IPLImage wrapper.
	// The goal is to hide a lot of the complexities of the IPL
	// system into the default values and to encapsulate
	// the conversion from the simpler ZBitmapDesc struct
	// into an IPL image

	int channels;
		// Number of color channels, not including alpha channels
		// Default = 1

	int alphaChannel;
		// Which channel is the alpha channels (0 means no alpha)
		// Default = 0

	int channelDepthInBits;
		// Channel depth in bits (only 1, 8, 16, 32) allowed
		// Default = 8

	int depthIsSigned;
		// Are the channels signed or unsigned
		// This is not an IPL function, it is merged into the depth bits
		// Default = 0

	int channelTypeFloat;
		// Is the channel a float.
		// This is not an IPL function
		// Default = 0

	char colorModel[4];
		// RGB, GRAY, HSV, HLS, LUV, XYZ, YCrCb, YUV
		// Default "GRAY"

	char channelSeq[4];
		// G, GRAY, BGR, BGRA, RGB, RGBA, HSV, HLS, XYZ, YUV, YCr, YCC, LUV		
		// Default = GRAY

	int planeOrder;
		// Is the data in pixel order (0) or plane order (1)
		// Default = 0

	int originBL;
		// Is the origin in the bottom left (normally assumed to be the top left)
		// Default = 0
		
	int align;
		// Alignment size in bytes (4, 8, 16, 32)
		// Default = 8

	int width;
		// Width in pixels of the image which may be bigger than the number of viz pixels for alignment
		// Default = 256

	int height;
		// Height in pixels
		// Default = 256

	int l, t, r, b;
		// The region of interest in pixels.
		// Default = (0, 0) - (width, height)

	void *appData;
		// The application may place whatever they want here
		// Note that the app must delete it if is an allocated buffer
		// Default = NULL

	int owner;
		// Boolean if this instance owns the bits buffer
		// Default = 0 (Set to 1 by alloc)

	char *bits;
		// The actual bits.  Set by alloc() or you can set it directly
		// but be sure that the description exactly matches, esp. alignment

	char *mallocBits;
		// Where the malloc was (potentially non-aligned)

  protected:
	void defaultDRules( int d );
		// Used to setup default rules

  public:
	ZBits();
		// Sets all the defaults but doesn't actually create the IplImage

	ZBits( int w, int h, int d, int _alloc=1, int _pow2=0 );
		// Creates using w and h and the following rules for d.
		// If these aren't what you want, then set the values directly and create()
		//   d == 1 : Image is GRAY
		//   d == 2 : Image is 8-bit GRAY with alpha
		//   d == 3 : Image is RGB 8-bit
		//   d == 4 : Image is RGBA 8-bit

	ZBits( struct ZBitmapDesc *desc, int _alloc=1, int _pow2=0 );
		// Creates using the desc as the basis.  Applies the same rules
		// as above in interpreting the data in the desc

	ZBits( ZBits *desc, int _alloc=1, int _pow2=0 );
		// Makes a copy of the header description but not
		// the bits themselves (use zbitsClone for that)
		// NOTE: This does not copy the roi, maskROI or tileInfo

	~ZBits();
		// Frees everything by calling clear()

	int specEquals( ZBits *compare );
		// Returns 1 if all of the spec is the same ( from the top to 'b' ... does not include the bits, etc. )

	void defaults();
		// Sets all values to default

	void createCommon( int _w, int _h, int _mode, int _alloc=1, int _pow2=0 );
		// Create the image based on a common format listed below
		#define ZBITS_COMMON_LUM_8 (1)
		#define ZBITS_COMMON_LUM_16 (2)
		#define ZBITS_COMMON_RGB_8 (3)
		#define ZBITS_COMMON_RGBA_8 (4)
		#define ZBITS_COMMON_LUM_ALPHA_8 (5)
		#define ZBITS_COMMON_LUM_FLOAT (6)
		#define ZBITS_COMMON_LUM_32S (7)

	void create( int w, int h, int d, int _alloc=1, int _pow2=0 );
		// Create the image using default rules, optionally allocate, optionally rounds up to a power of 2

	void create( struct ZBitmapDesc *desc, int _alloc=1, int _pow2=0 );
		// Creates using the desc as the basis.  Applies the same rules
		// as above in interpreting the data in the desc, optionally rounds up to a power of 2

	void create( ZBits *desc, int _alloc=1, int _pow2=0 );
		// Makes a copy of the header description but not
		// the bits themselves (use zbitsClone for that)
		// NOTE: This does not copy the roi, maskROI or tileInfo

	void clone( ZBits *desc );
		// Create an exact copy of the desc including reallocating the bits

	void transferOwnershipFrom( ZBits *src );
		// Without reallocing the bits in orig, transfer the ownership to this from src

	void upsizePow2();
		// Ups the width and height to a power of two and changes the roi to the relevant area

	void alloc();
		// Allocates the bits based on the desc.  Create() (or non-default constructor) must have been called first

	void fill( int value );
		// memsets all the bits to this value

	virtual void clear();
		// Deallocs bits

	virtual void setWH( int _w, int _h );
	virtual void setROIxywh( int _x, int _y, int _w, int _h );
	virtual void setROIltrb( int _l, int _t, int _r, int _b );
		// set the region of interest

	int pixDepthInBytes() { 
		return ((alphaChannel ? 1 : 0) + channels) * ( channelDepthInBits >> 3 );
	}

	unsigned int getDomainMax() {
		switch( channelDepthInBits ) {
			case  8: return depthIsSigned ? 0xFFU / 2 : 0xFFU; break;
			case 16: return depthIsSigned ? 0xFFFFU / 2 : 0xFFFFU; break;
			case 32: return depthIsSigned ? 0xFFFFFFFFU / 2 : 0xFFFFFFFFU;  break;
		}
		return 0;
	}

	// Note that the following are only useful when the data is byte-aligned
	// They cannot be used when the depthInBits is 1

	void           *lineVoid  ( int y ) { return (         void  *)&bits[y*width*pixDepthInBytes()]; }
	char           *lineChar  ( int y ) { return (         char  *)&bits[y*width*pixDepthInBytes()]; }
	unsigned char  *lineUchar ( int y ) { return (unsigned char  *)&bits[y*width*pixDepthInBytes()]; }
	short          *lineShort ( int y ) { return (  signed short *)&bits[y*width*pixDepthInBytes()]; }
	unsigned short *lineUshort( int y ) { return (unsigned short *)&bits[y*width*pixDepthInBytes()]; }
	int            *lineInt   ( int y ) { return (  signed int   *)&bits[y*width*pixDepthInBytes()]; }
	unsigned int   *lineUint  ( int y ) { return (unsigned int   *)&bits[y*width*pixDepthInBytes()]; }
	float          *lineFloat ( int y ) { return (         float *)&bits[y*width*pixDepthInBytes()]; }
	char           *lineS1    ( int y ) { return (         char  *)&bits[y*width*pixDepthInBytes()]; }
	unsigned char  *lineU1    ( int y ) { return (unsigned char  *)&bits[y*width*pixDepthInBytes()]; }
	short          *lineS2    ( int y ) { return (  signed short *)&bits[y*width*pixDepthInBytes()]; }
	unsigned short *lineU2    ( int y ) { return (unsigned short *)&bits[y*width*pixDepthInBytes()]; }
	int            *lineS4    ( int y ) { return (  signed int   *)&bits[y*width*pixDepthInBytes()]; }
	unsigned int   *lineU4    ( int y ) { return (unsigned int   *)&bits[y*width*pixDepthInBytes()]; }

	void           *ptrVoid  ( int x, int y ) { return (         void  *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	char           *ptrChar  ( int x, int y ) { return (         char  *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	unsigned char  *ptrUchar ( int x, int y ) { return (unsigned char  *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	short          *ptrShort ( int x, int y ) { return (  signed short *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	unsigned short *ptrUshort( int x, int y ) { return (unsigned short *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	int            *ptrInt   ( int x, int y ) { return (  signed int   *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	unsigned int   *ptrUint  ( int x, int y ) { return (unsigned int   *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	float          *ptrFloat ( int x, int y ) { return (         float *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	char           *ptrS1    ( int x, int y ) { return (         char  *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	unsigned char  *ptrU1    ( int x, int y ) { return (unsigned char  *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	short          *ptrS2    ( int x, int y ) { return (  signed short *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	unsigned short *ptrU2    ( int x, int y ) { return (unsigned short *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	int            *ptrS4    ( int x, int y ) { return (  signed int   *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	unsigned int   *ptrU4    ( int x, int y ) { return (unsigned int   *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }

	struct ZBitsS3 {   signed char r, g, b; };
	struct ZBitsU3 { unsigned char r, g, b; };
	ZBitsS3 *lineS3      ( int y ) { return (ZBitsS3 *)&bits[y*width*pixDepthInBytes()]; }
	ZBitsU3 *lineU3      ( int y ) { return (ZBitsU3 *)&bits[y*width*pixDepthInBytes()]; }

	char           getChar  ( int x, int y ) { return *(         char  *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	unsigned char  getUchar ( int x, int y ) { return *(unsigned char  *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	short          getShort ( int x, int y ) { return *(  signed short *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	unsigned short getUshort( int x, int y ) { return *(unsigned short *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	int            getInt   ( int x, int y ) { return *(  signed int   *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	unsigned int   getUint  ( int x, int y ) { return *(unsigned int   *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	float          getFloat ( int x, int y ) { return *(         float *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	char           getS1    ( int x, int y ) { return *(         char  *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	unsigned char  getU1    ( int x, int y ) { return *(unsigned char  *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	short          getS2    ( int x, int y ) { return *(  signed short *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	unsigned short getU2    ( int x, int y ) { return *(unsigned short *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	int            getS4    ( int x, int y ) { return *(  signed int   *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	unsigned int   getU4    ( int x, int y ) { return *(unsigned int   *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; }
	unsigned int   getTriplet(int x, int y ) { return (0x00FFFFFF & *(unsigned int *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]); }

	void setChar  ( int x, int y, char           v ) { *(         char  *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]=v; }
	void setUchar ( int x, int y, unsigned char  v ) { *(unsigned char  *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]=v; }
	void setShort ( int x, int y, short          v ) { *(  signed short *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]=v; }
	void setUshort( int x, int y, unsigned short v ) { *(unsigned short *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]=v; }
	void setInt   ( int x, int y, int            v ) { *(  signed int   *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]=v; }
	void setUint  ( int x, int y, unsigned int   v ) { *(unsigned int   *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]=v; }
	void setFloat ( int x, int y, float          v ) { *(         float *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]=v; }
	void setS1    ( int x, int y, char           v ) { *(         char  *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]=v; }
	void setU1    ( int x, int y, unsigned char  v ) { *(unsigned char  *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]=v; }
	void setS2    ( int x, int y, short          v ) { *(  signed short *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]=v; }
	void setU2    ( int x, int y, unsigned short v ) { *(unsigned short *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]=v; }
	void setS4    ( int x, int y, int            v ) { *(  signed int   *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]=v; }
	void setU4    ( int x, int y, unsigned int   v ) { *(unsigned int   *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]=v; }
	void setTriplet(int x, int y, unsigned int a ) { unsigned int b = *(unsigned int *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()]; b &= 0xFF000000; b |= (a & 0x00FFFFFF); *(unsigned int *)&bits[y*width*pixDepthInBytes()+x*pixDepthInBytes()] = b; }

	unsigned int getPixel( int x, int y );
	float getPixelF( int x, int y );
		// A slow function that looks up the depth on each call and casts the pixel to an unsigned int or float

	int w() { return r-l; }
	int h() { return b-t; }
	float tw() { return (float)w() / (float)width; }
	float th() { return (float)h() / (float)height; }
	int p() { return width * pixDepthInBytes(); }
	int totalNumChannels() { return channels + (alphaChannel ? 1 : 0); }

	operator   signed char *() { return (  signed char *)bits; }
	operator unsigned char *() { return (unsigned char *)bits; }

	void copyBitsFrom( struct ZBitmapDesc *desc );
	int lineInBytes() { return w() * pixDepthInBytes(); }
	int lenInBytes() { return width * height * pixDepthInBytes(); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////

int *zbitsHistogramI( ZBits *src, int numBuckets, int subSample=1, int *sum=0 );
float *zbitsHistogramF( ZBits *src, int numBuckets, int subSample=1, int *sum=0 );
	// Alloc a histogram buffer of numBuckets and prcess the src
	// the src must be monochrome. subSample allows stepping the
	// sampling by that many units 

void zbitsByteReorder( ZBits *src );
	// Reorders BGR
	
void zbitsConvert( ZBits *src, ZBits *dst );
	// This will do basic conversion from one channel count to another

void zbitsInvert( ZBits *src );
	// Flip vertically

void zbitsThresh( ZBits *src, int compare, int inverseLogic=0 );
	// Threshold

void zbitsAverageFrames( ZBits **arrayOfBitsToAvg, int count, ZBits *dstBits );
void zbitsFloodFill( ZBits *srcBits, ZBits *dstBits, int x, int y, int val, short *pixelList=0, int *count=0, int subsampleMask=0xFFFFFFFF );
	// The pixelList is a array of shorts x, y. The count is the number os PAIRS of x,ys thus the number of points not the total elements in the arraty
	// likewise, count must be passed as the maximum number of pairs, not the size of the arary

// Read & Writes a simple binary datafile of bits into the FILE * (passed as void *)
void zbitsWriteBinFile( ZBits *bits, void *filePtr );
void zbitsReadBinFile( ZBits *bits, void *filePtr );

////////////////////////////////////////////////////////////////////////////////////////////////////
// @TODO: Remove all this, replace with the _filt system
//extern ZBits *zbitsFilterLast;
//extern ZBits *zbitsFilterNext;
//
//void zbitsFilterStart( ZBits *src );
//void zbitsFilterStop();
//void zbitsFilterSetNext( ZBits *next );
//void zbitsFilterStart( ZBits *src );
//void zbitsFilterSetNext( ZBits *next );
//void zbitsFilterAllocNext();
//void zbitsFilterClear();


#endif



