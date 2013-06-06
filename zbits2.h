// @ZBS {
//		*MODULE_OWNER_NAME zbits
// }

#ifndef ZBITS_H
#define ZBITS_H

struct ZBits {
	int format;
		#define ZBITS_U8_L         (0x00000008)
		#define ZBITS_S8_L         (0x00000108)
		#define ZBITS_U8_LA        (0x00001008)
		#define ZBITS_S8_LA        (0x00001108)
		#define ZBITS_U8_RGB       (0x00002008)
		#define ZBITS_S8_RGB       (0x00002108)
		#define ZBITS_U8_RGBA      (0x00003008)
		#define ZBITS_S8_RGBA      (0x00003108)

		#define ZBITS_U16_L        (0x00000010)
		#define ZBITS_S16_L        (0x00000110)
		#define ZBITS_U16_LA       (0x00001010)
		#define ZBITS_S16_LA       (0x00001110)
		#define ZBITS_U16_RGB      (0x00002010)
		#define ZBITS_S16_RGB      (0x00002110)
		#define ZBITS_U16_RGBA     (0x00003010)
		#define ZBITS_S16_RGBA     (0x00003110)

		#define ZBITS_U32_L        (0x00000020)
		#define ZBITS_S32_L        (0x00000120)
		#define ZBITS_U32_LA       (0x00001020)
		#define ZBITS_S32_LA       (0x00001120)
		#define ZBITS_U32_RGB      (0x00002020)
		#define ZBITS_S32_RGB      (0x00002120)
		#define ZBITS_U32_RGBA     (0x00003020)
		#define ZBITS_S32_RGBA     (0x00003120)

		#define ZBITS_F32_L        (0x00000220)
		#define ZBITS_F32_LA       (0x00001220)
		#define ZBITS_F32_RGB      (0x00002220)
		#define ZBITS_F32_RGBA     (0x00003220)

		#define ZBITS_DEPTH_MASK   (0x000000FF)
		#define ZBITS_TYPE_MASK    (0x00000F00)
			#define ZBITS_TYPE_U   (0x00000000)
			#define ZBITS_TYPE_S   (0x00000100)
			#define ZBITS_TYPE_F   (0x00000200)
		#define ZBITS_FMT_MASK     (0x0000F000)
			#define ZBITS_FMT_L    (0x00000000)
			#define ZBITS_FMT_LA   (0x00001000)
			#define ZBITS_FMT_RGB  (0x00002000)
			#define ZBITS_FMT_RGBA (0x00003000)

	int w;
	int h;
		// Size of image in pixels

	int memW;
	int memH;
		// Size of the buffer in pixels (must be >= w and h)

	void *appData;
		// Application controlled data

	int pixDepthInBytes;
		// Setup by create

	int owner;
		// Boolean if this instance owns the bits buffer
		// Default = 0 (Set to 1 by alloc)

	char *mallocBits;
		// Where the malloc was (potentially non-aligned)

	char *bits;
		// The first pixel (16 byte aligned inside of mallocBits frame)


	// Constructor
	//----------------------------------------------------------------------------------------------------------

	ZBits();
	ZBits( int w, int h, int fmt, int _alloc=1, int _pow2=0 );
	ZBits( ZBits *bitsToCopy, int _alloc=1, int _pow2=0, int _copyBits=0 );
		// Makes a copy of the DESCRIPTION but not bits themselves unless requested by _copyBits

	~ZBits();
		// Frees everything by calling clear()


	// Create, Destroy
	//----------------------------------------------------------------------------------------------------------

	void create( int w, int h, int fmt, int _alloc=1, int _pow2=0 );
		// Create the image using default rules, optionally allocate, optionally rounds up to a power of 2

	void create( ZBits2 *bitsToCopy, int _alloc=1, int _pow2=0, int _copyBits=0 );
		// Makes a copy of the DESCRIPTION but not bits themselves unless requested by _copyBits

	void transferOwnershipFrom( ZBits *src );
		// Without reallocing the bits in the original, transfer the ownership to this from src

	void alloc();
		// Allocates the bits based on the description.  create() (or non-default constructor) must have been called first. Does not fill

	void dealloc();
		// Deletes all the bits but leaves the description alone

	void clear();
		// Deallocs bits, resets all the description to default

	void fill( int value=0 );
		// memsets all the bits to this value

	// Utility
	//----------------------------------------------------------------------------------------------------------

	int channelCount() { return (((format & ZBITS_FMT_MASK) >> 12)+1); }

	int memSizeInBytes() { return memW * memH * pixDepthInBytes; }

	float tw() { return (float)w / (float)memW; }
	float th() { return (float)h / (float)memH; }
		// Normalized texture size

	int p() { return memW * pixDepthInBytes; }
		// pitch in bytes

	// Pixel ops
	//----------------------------------------------------------------------------------------------------------

	unsigned char  *lineU8 ( int y ) { return (unsigned char  *)&bits[pixDepthInBytes*memW*y]; }
	signed char    *lineS8 ( int y ) { return (  signed char  *)&bits[pixDepthInBytes*memW*y]; }
	unsigned short *lineU16( int y ) { return (unsigned short *)&bits[pixDepthInBytes*memW*y]; }
	signed short   *lineS16( int y ) { return (  signed short *)&bits[pixDepthInBytes*memW*y]; }
	unsigned int   *lineU32( int y ) { return (unsigned int   *)&bits[pixDepthInBytes*memW*y]; }
	signed int     *lineS32( int y ) { return (  signed int   *)&bits[pixDepthInBytes*memW*y]; }
	float          *lineF32( int y ) { return (float          *)&bits[pixDepthInBytes*memW*y]; }
								   
	unsigned char  *ptrU8 ( int x, int y ) { return (unsigned char  *)&bits[pixDepthInBytes*(memW*y+x)]; }
	signed char    *ptrS8 ( int x, int y ) { return (  signed char  *)&bits[pixDepthInBytes*(memW*y+x)]; }
	unsigned short *ptrU16( int x, int y ) { return (unsigned short *)&bits[pixDepthInBytes*(memW*y+x)]; }
	signed short   *ptrS16( int x, int y ) { return (  signed short *)&bits[pixDepthInBytes*(memW*y+x)]; }
	unsigned int   *ptrU32( int x, int y ) { return (unsigned int   *)&bits[pixDepthInBytes*(memW*y+x)]; }
	signed int     *ptrS32( int x, int y ) { return (  signed int   *)&bits[pixDepthInBytes*(memW*y+x)]; }
	float          *ptrF32( int x, int y ) { return (float          *)&bits[pixDepthInBytes*(memW*y+x)]; }
								   
	unsigned char  getU8 ( int x, int y, int channel ) { return ((unsigned char  *)&bits[pixDepthInBytes*(memW*y+x)])[channel]; }
	signed char    getS8 ( int x, int y, int channel ) { return ((  signed char  *)&bits[pixDepthInBytes*(memW*y+x)])[channel]; }
	unsigned short getU16( int x, int y, int channel ) { return ((unsigned short *)&bits[pixDepthInBytes*(memW*y+x)])[channel]; }
	signed short   getS16( int x, int y, int channel ) { return ((  signed short *)&bits[pixDepthInBytes*(memW*y+x)])[channel]; }
	unsigned int   getU32( int x, int y, int channel ) { return ((unsigned int   *)&bits[pixDepthInBytes*(memW*y+x)])[channel]; }
	signed int     getS32( int x, int y, int channel ) { return ((  signed int   *)&bits[pixDepthInBytes*(memW*y+x)])[channel]; }
	float          getF32( int x, int y, int channel ) { return ((float          *)&bits[pixDepthInBytes*(memW*y+x)])[channel]; }

	unsigned char  setU8 ( int x, int y, int channel, unsigned char  p ) { ((unsigned char  *)&bits[pixDepthInBytes*(memW*y+x)])[channel] = p; }
	signed char    setS8 ( int x, int y, int channel,   signed char  p ) { ((  signed char  *)&bits[pixDepthInBytes*(memW*y+x)])[channel] = p; }
	unsigned short setU16( int x, int y, int channel, unsigned short p ) { ((unsigned short *)&bits[pixDepthInBytes*(memW*y+x)])[channel] = p; }
	signed short   setS16( int x, int y, int channel,   signed short p ) { ((  signed short *)&bits[pixDepthInBytes*(memW*y+x)])[channel] = p; }
	unsigned int   setU32( int x, int y, int channel, unsigned int   p ) { ((unsigned int   *)&bits[pixDepthInBytes*(memW*y+x)])[channel] = p; }
	signed int     setS32( int x, int y, int channel,   signed int   p ) { ((  signed int   *)&bits[pixDepthInBytes*(memW*y+x)])[channel] = p; }
	float          setF32( int x, int y, int channel, float          p ) { ((float          *)&bits[pixDepthInBytes*(memW*y+x)])[channel] = p; }


	// RGB Pixel ops
	//----------------------------------------------------------------------------------------------------------

	struct ZBitsU8RGB  { unsigned char  r, g, b; };
	struct ZBitsS8RGB  {   signed char  r, g, b; };
	struct ZBitsU16RGB { unsigned short r, g, b; };
	struct ZBitsS16RGB {   signed short r, g, b; };
	struct ZBitsU32RGB { unsigned int   r, g, b; };
	struct ZBitsS32RGB {   signed int   r, g, b; };
	struct ZBitsF32RGB {   float        r, g, b; };

	ZBitsU8RGB  *lineU8RGB  ( int y ) { return (ZBitsU8RGB  *)&bits[pixDepthInBytes*memW*y]; }
	ZBitsS8RGB  *lineS8RGB  ( int y ) { return (ZBitsS8RGB  *)&bits[pixDepthInBytes*memW*y]; }
	ZBitsU16RGB *lineU16RGB ( int y ) { return (ZBitsU16RGB *)&bits[pixDepthInBytes*memW*y]; }
	ZBitsS16RGB *lineS16RGB ( int y ) { return (ZBitsS16RGB *)&bits[pixDepthInBytes*memW*y]; }
	ZBitsU32RGB *lineU32RGB ( int y ) { return (ZBitsU32RGB *)&bits[pixDepthInBytes*memW*y]; }
	ZBitsS32RGB *lineS32RGB ( int y ) { return (ZBitsS32RGB *)&bits[pixDepthInBytes*memW*y]; }
	ZBitsF32RGB *lineF32RGB ( int y ) { return (ZBitsF32RGB *)&bits[pixDepthInBytes*memW*y]; }

	ZBitsU8RGB  *ptrU8RGB  ( int x, int y ) { return (ZBitsU8RGB  *)&bits[pixDepthInBytes*(memW*y+x)]; }
	ZBitsS8RGB  *ptrS8RGB  ( int x, int y ) { return (ZBitsS8RGB  *)&bits[pixDepthInBytes*(memW*y+x)]; }
	ZBitsU16RGB *ptrU16RGB ( int x, int y ) { return (ZBitsU16RGB *)&bits[pixDepthInBytes*(memW*y+x)]; }
	ZBitsS16RGB *ptrS16RGB ( int x, int y ) { return (ZBitsS16RGB *)&bits[pixDepthInBytes*(memW*y+x)]; }
	ZBitsU32RGB *ptrU32RGB ( int x, int y ) { return (ZBitsU32RGB *)&bits[pixDepthInBytes*(memW*y+x)]; }
	ZBitsS32RGB *ptrS32RGB ( int x, int y ) { return (ZBitsS32RGB *)&bits[pixDepthInBytes*(memW*y+x)]; }
	ZBitsF32RGB *ptrF32RGB ( int x, int y ) { return (ZBitsF32RGB *)&bits[pixDepthInBytes*(memW*y+x)]; }


	// RGBA Pixel ops
	//----------------------------------------------------------------------------------------------------------

	struct ZBitsU8RGBA  { unsigned char  r, g, b, a; };
	struct ZBitsS8RGBA  {   signed char  r, g, b, a; };
	struct ZBitsU16RGBA { unsigned short r, g, b, a; };
	struct ZBitsS16RGBA {   signed short r, g, b, a; };
	struct ZBitsU32RGBA { unsigned int   r, g, b, a; };
	struct ZBitsS32RGBA {   signed int   r, g, b, a; };
	struct ZBitsF32RGBA {   float        r, g, b, a; };

	ZBitsU8RGBA  *lineU8RGBA  ( int y ) { return (ZBitsU8RGBA  *)&bits[pixDepthInBytes*memW*y]; }
	ZBitsS8RGBA  *lineS8RGBA  ( int y ) { return (ZBitsS8RGBA  *)&bits[pixDepthInBytes*memW*y]; }
	ZBitsU16RGBA *lineU16RGBA ( int y ) { return (ZBitsU16RGBA *)&bits[pixDepthInBytes*memW*y]; }
	ZBitsS16RGBA *lineS16RGBA ( int y ) { return (ZBitsS16RGBA *)&bits[pixDepthInBytes*memW*y]; }
	ZBitsU32RGBA *lineU32RGBA ( int y ) { return (ZBitsU32RGBA *)&bits[pixDepthInBytes*memW*y]; }
	ZBitsS32RGBA *lineS32RGBA ( int y ) { return (ZBitsS32RGBA *)&bits[pixDepthInBytes*memW*y]; }
	ZBitsF32RGBA *lineF32RGBA ( int y ) { return (ZBitsF32RGBA *)&bits[pixDepthInBytes*memW*y]; }

	ZBitsU8RGBA  *ptrU8RGBA  ( int x, int y ) { return (ZBitsU8RGBA  *)&bits[pixDepthInBytes*(memW*y+x)]; }
	ZBitsS8RGBA  *ptrS8RGBA  ( int x, int y ) { return (ZBitsS8RGBA  *)&bits[pixDepthInBytes*(memW*y+x)]; }
	ZBitsU16RGBA *ptrU16RGBA ( int x, int y ) { return (ZBitsU16RGBA *)&bits[pixDepthInBytes*(memW*y+x)]; }
	ZBitsS16RGBA *ptrS16RGBA ( int x, int y ) { return (ZBitsS16RGBA *)&bits[pixDepthInBytes*(memW*y+x)]; }
	ZBitsU32RGBA *ptrU32RGBA ( int x, int y ) { return (ZBitsU32RGBA *)&bits[pixDepthInBytes*(memW*y+x)]; }
	ZBitsS32RGBA *ptrS32RGBA ( int x, int y ) { return (ZBitsS32RGBA *)&bits[pixDepthInBytes*(memW*y+x)]; }
	ZBitsF32RGBA *ptrF32RGBA ( int x, int y ) { return (ZBitsF32RGBA *)&bits[pixDepthInBytes*(memW*y+x)]; }

};


// @TODO:

int *zbitsHistogramI( ZBits *src, int numBuckets, int subSample=1, int *sum=0 );
float *zbitsHistogramF( ZBits *src, int numBuckets, int subSample=1, int *sum=0 );
	// Alloc a histogram buffer of numBuckets and prcess the src
	// the src must be monochrome. subSample allows stepping the
	// sampling by that many units 

void zbitsConvert( ZBits *src, ZBits *dst );
	// This will do basic conversion from one channel count to another

void zbitsInvert( ZBits *src );
	// Flip vertically

void zbitsThresh( ZBits *src, int compare, int inverseLogic=0 );
	// Threshold

void zbitsAverageFrames( ZBits **arrayOfBitsToAvg, int count, ZBits *dstBits );
void zbitsFloodFill( ZBits *srcBits, ZBits *dstBits, int x, int y, int val, short *pixelList=0, int *count=0 );
	// The pixelList is a array of shorts x, y. The count is the number os PAIRS of x,ys thus the number of points not the total elements in the arraty
	// likewise, count must be passed as the maximum number of pairs, not the size of the arary

// Read & Writes a simple binary datafile of bits into the FILE * (passed as void *)
void zbitsWriteBinFile( ZBits *bits, void *filePtr );
void zbitsReadBinFile( ZBits *bits, void *filePtr );



#endif



