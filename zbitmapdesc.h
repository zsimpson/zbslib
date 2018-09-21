// @ZBS {
//		*MODULE_OWNER_NAME zbitmapdesc
// }

#ifndef ZBITMAPDESC_H
#define ZBITMAPDESC_H

// If you want to use the IPLImage library, be sure to
// include ipl.h BEFORE this file in the include list!

struct ZBitmapDesc {
	char *bits;
	int owner;
		// Is memory owned by this class
		int w, h, d;
		// width, height, depth
	int memW, memH;
		// These are the actually allocated w & h, often the next power of 2 for texture map loading
	void *iplImage;
		// If using the Intel IPL Library, this is a cached pointer to an IplImage
	void *bitmapInfoHeader;
		// If using the Windows library, this is a cached pointer to a BITMAPINO
	int flags;
		#define zbNO_ALPHA_CHANNEL (1)
			// This overrides the default assumption that a depth of 2 or 4 means that
			// there is an alpha channel.  Used when creating ipl images
	
	ZBitmapDesc();
	ZBitmapDesc( ZBitmapDesc &copy );
		// Copy's over the description ONLY; not the bits, owner, nor iplImage

	~ZBitmapDesc();

	int p() { return memW * d; }
		// Get the pitch

	void           *lineVoid  ( int y ) const { return (void *)&bits[y*memW*d]; }
	char           *lineChar  ( int y ) const { return (char *)&bits[y*memW*d]; }
	unsigned short *lineUshort( int y ) const { return (unsigned short *)&bits[y*memW*d]; }
	short          *lineShort ( int y ) const { return (short *)&bits[y*memW*d]; }
	unsigned char  *lineUchar ( int y ) const { return (unsigned char *)&bits[y*memW*d]; }
	int            *lineInt   ( int y ) const { return (int *)&bits[y*memW*d]; }
	unsigned int   *lineUint  ( int y ) const { return (unsigned int *)&bits[y*memW*d]; }

	char           getChar   ( int x, int y ) const { return *(char *)&bits[y*memW*d + x*d]; }
	unsigned char  getUchar  ( int x, int y ) const { return *(unsigned char *)&bits[y*memW*d + x*d]; }
	short          getShort  ( int x, int y ) const { return *(short *)&bits[y*memW*d + x*d]; }
	unsigned short getUshort ( int x, int y ) const { return *(unsigned short *)&bits[y*memW*d + x*d]; }
	int            getInt    ( int x, int y ) const { return *(int *)&bits[y*memW*d + x*d]; }
	unsigned int   getUint   ( int x, int y ) const { return *(unsigned int *)&bits[y*memW*d + x*d]; }
	unsigned int   getTriplet( int x, int y ) const { return (0x00FFFFFF & *(unsigned int *)&bits[y*memW*d + x*d]); }

	void setChar   ( int x, int y, char a ) { *(char *)&bits[y*memW*d + x*d] = a; }
	void setUchar  ( int x, int y, unsigned char a ) { *(unsigned char *)&bits[y*memW*d + x*d] = a; }
	void setShort  ( int x, int y, short a ) { *(short *)&bits[y*memW*d + x*d] = a; }
	void setUshort ( int x, int y, unsigned short a ) { *(unsigned short *)&bits[y*memW*d + x*d] = a; }
	void setInt    ( int x, int y, int a ) { *(int *)&bits[y*memW*d + x*d] = a; }
	void setUint   ( int x, int y, unsigned int a ) { *(unsigned int *)&bits[y*memW*d + x*d] = a; }
	void setTriplet( int x, int y, unsigned int a ) { unsigned int b = *(unsigned int *)&bits[y*memW*d + x*d]; b &= 0xFF000000; b |= (a & 0x00FFFFFF); *(unsigned int *)&bits[y*memW*d + x*d] = b; }

	void copy( ZBitmapDesc &src );
		// Makes a copy of the description as well as the bits if there are any

	int bytes() { return memW * memH * d; }

	void init      ( int _w, int _h, int _d, char *_bits );
	void initP2    ( int _w, int _h, int _d, char *_bits );
		// w and h are made powers of two
	void initP2W   ( int _w, int _h, int _d, char *_bits );
		// w is made a pow 2, h is not
	void initSP2   ( int _w, int _h, int _d, char *_bits );
		// w and h are made powers of two and same, i.e. square
	void initQWord ( int _w, int _h, int _d, char *_bits );
		// init on quad word boundaries, which the IPL likes

	void alloc     ( int _w, int _h, int _d );
	void allocP2   ( int _w, int _h, int _d );
	void allocP2W  ( int _w, int _h, int _d );
		// W is made a pow2, h is not
	void allocSP2  ( int _w, int _h, int _d );
	void allocQWord( int _w, int _h, int _d );
		// alloc on quad word boundaries, which the IPL likes
	
	void _clear();
		// clear calls _clear to do any work that is not IPL-dependent
		// we do this so that all IPL code is in the header, which allows
		// you to compile by just putting ipl.h first in your include list, 
		// without and special #defines
		
	void fill( int x );

	void clear() {
		#ifdef IPLAPI
		if( iplImage ) {
			((IplImage*)iplImage)->imageData = NULL;
			if( ((IplImage*)iplImage)->roi ) {
				iplDeleteROI( ((IplImage*)iplImage)->roi );
			}
			iplDeallocate( (IplImage*)iplImage, IPL_IMAGE_HEADER );
		}
		#endif

		#ifdef _WINDOWS_
		if( bitmapInfoHeader ) {
			free( bitmapInfoHeader );
			bitmapInfoHeader = NULL;
		}
		#endif

		_clear();
	}

	float  memWRatioF() { return (float) w / (float) memW; }
	double memWRatioD() { return (double)w / (double)memW; }
	float  memHRatioF() { return (float) h / (float) memH; }
	double memHRatioD() { return (double)h / (double)memH; }

	#ifdef IPLAPI
		// If you include the IPLAPI before this header, we know that you are 
		// using the Intel libraries and it is safe to include this conversion
		// See above.

		IplImage *getIplImage() {
			if( !iplImage ) {
				static char *colorTypes[5] = { "", "GRAY", "GRAY", "RGB", "RGBA" };

				IplROI *roi = NULL;
				if( w != memW || h != memH ) {
					roi = iplCreateROI( 0, 0, 0, w, h );		
				}

				int alignFlag = IPL_ALIGN_DWORD;
				if( !(memW % 8) ) {
					alignFlag = IPL_ALIGN_QWORD;
				}
				else if( memW % 4 ) {
					assert( 0 ); // ipl insists on dword or qword scanline alignment
				}

				int nChannels = d==2 ? 1 : (d<=3?d:3);
				int alphaChannel = (d==2) ? 1 : (d==4 ? 3 : 0);
				char *colorType = colorTypes[d];
				int depth = IPL_DEPTH_8U;

				if( flags & zbNO_ALPHA_CHANNEL ) {
					nChannels = d==2 ? 1 : d;
					alphaChannel = 0;
					colorType = colorTypes[d];
					depth = d==2 ? IPL_DEPTH_16U : IPL_DEPTH_8U;
				}

				iplImage = iplCreateImageHeader(
					nChannels,
					alphaChannel,
					depth,//int depth,
					d<=2 ? (char*)"GRAY" : (char *)"RGB",//char* colorModel,
					colorType,//char* channelSeq,
					IPL_DATA_ORDER_PIXEL,//int dataOrder,
					IPL_ORIGIN_TL,//int origin,
					alignFlag,//int align,
					memW,//int width,
					memH,//int height,
					roi,//IplROI* roi,
					NULL,//IplImage* maskROI,
					NULL,//void* imageID,
					NULL//IplTileInfo* tileInfo
				);
				((IplImage *)iplImage)->imageData = (char *)bits;
			}
			return (IplImage *)iplImage;
		}
	#endif

	#ifdef _WINDOWS_H
		// If you include the windows.h before this header, we know that you are 
		// using the Windows libraries and it is safe to include this conversion
		// Note that this will not deal with palettes
		//
		// TFB changed _WINDOWS_ to _WINDOWS_H to explicity look for windows.h
		// inclusion, since _WINDOWS_ is defined in FreeImage.h for ALL platforms
		// in FreeImage 3.18

		BITMAPINFO *getBitmapInfo() {
			if( !bitmapInfoHeader ) {
				if( memW != w ) __asm int 3;
				bitmapInfoHeader = malloc( sizeof(BITMAPINFOHEADER) );
				BITMAPINFO *head = (BITMAPINFO *)bitmapInfoHeader;
				head->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
				head->bmiHeader.biWidth = w;
				head->bmiHeader.biHeight = h;
				head->bmiHeader.biPlanes = 1;
				head->bmiHeader.biBitCount = d * 8;
				head->bmiHeader.biCompression = 0;
				head->bmiHeader.biSizeImage = 0;
				head->bmiHeader.biXPelsPerMeter = 0;
				head->bmiHeader.biYPelsPerMeter = 0;
				head->bmiHeader.biClrUsed = 0;
				head->bmiHeader.biClrImportant = 0;
			}
			return (BITMAPINFO *)bitmapInfoHeader;
		}
	#endif

};

void zBitmapDescInvertLines( ZBitmapDesc &src );
	// Swaps lines

void zBitmapDescSetChannel( ZBitmapDesc &src, int channel, int value );
	// Sets all bits of a given channel (0-3) to the specificied value

void zBitmapDescInvertBGR( ZBitmapDesc &src );
	// Flips the byte order of the bits in a bitmap of depth 3 or 4

#define bdANY -1
void zBitmapDescSetTransparent( ZBitmapDesc &src, int keyColor, int alphaValue=0 );
	// Any RGB which matches keyColor is set to the alphaValue; (R = MSB)
	// if keyColor is bdANY then all values are changed

void zBitmapDescConvert( ZBitmapDesc &src, ZBitmapDesc &dst );
	// Convert from one desc to another.  If bits are not allocated
	// on the dst, they are allocated and owner is set

void zBitmapDescBrightnessAdjust( ZBitmapDesc &bd, int lowerBound, int upperBound );
	// Brightness adjusts the bitmap so that all values are shifted between the lower and upper bound
	// There is no initial historgam normalization so calling this on an image without a full
	// contrast range might have unintended results.

void zBitmapDescWriteInto( ZBitmapDesc &src, ZBitmapDesc &dst, int x, int y );
	// Writes the contents of src into dst at x, y in dst coords and clipping to dst

#endif

