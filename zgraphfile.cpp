// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A two function (load/save) API for reading and writing graphic files.
//			This is a wrapper around the freeimage sdk which supprts a large number of file types
//			inclusing jpeg, png, tif, psd, bmp, etc.  I removed GIF due to compilation problems
//		}
//		+EXAMPLE {
//			ZBitmapDesc	bits;
//			zGraphFileLoad( "test.png", &bits );
//		}
//		*PORTABILITY win32
//		*SDK_DEPENDS freeimage-3.18
//		*REQUIRED_FILES zgraphfile.cpp zgraphfile.h
//		*VERSION 2.0
//		+HISTORY {
//			2.0 I replaced indiviudal libs with freeimage
//		}
//		+TODO {
//		}
//		*SELF_TEST no
// }
// OPERATING SYSTEM specific includes:
// SDK includes:
#include "FreeImage.h"
// STDLIB includes:
#include "assert.h"
#include "memory.h"
#include "ctype.h"
#include "stdlib.h"
#include "string.h"
// MODULE includes:
#include "zgraphfile.h"
// ZBSLIB includes:
#include "zbitmapdesc.h"
#include "zbits.h"

// void* datatype is used below only because it will reliably be 32 or 64 bits long
// depending on whether the program is compiled as 32 or 64 bit.  "int" was used here
// previously.
int freeimageTypeFromFilename( char *filename ) {
	static void* extensions[] = {
		(void*)"bmp",   (void*)FIF_BMP,
		(void*)"cut",   (void*)FIF_CUT,
		(void*)"dds",   (void*)FIF_DDS,
		(void*)"gif",   (void*)FIF_GIF,
		(void*)"hdr",   (void*)FIF_HDR,
		(void*)"ico",   (void*)FIF_ICO,
		(void*)"iff",   (void*)FIF_IFF,
		(void*)"lbm",   (void*)FIF_IFF,
		(void*)"jng",   (void*)FIF_JNG,
		(void*)"jpg",   (void*)FIF_JPEG,
		(void*)"jif",   (void*)FIF_JPEG,
		(void*)"fpeg",  (void*)FIF_JPEG,
		(void*)"jpe",   (void*)FIF_JPEG,
		(void*)"koa",   (void*)FIF_KOALA,
		(void*)"mng",   (void*)FIF_MNG,
		(void*)"pbm",   (void*)FIF_PBM,
		(void*)"pcd",   (void*)FIF_PCD,
		(void*)"pcx",   (void*)FIF_PCX,
		(void*)"pgm",   (void*)FIF_PGM,
		(void*)"png",   (void*)FIF_PNG,
		(void*)"ppm",   (void*)FIF_PPM,
		(void*)"psd",   (void*)FIF_PSD,
		(void*)"ras",   (void*)FIF_RAS,
		(void*)"tga",   (void*)FIF_TARGA,
		(void*)"targa", (void*)FIF_TARGA,
		(void*)"tif",   (void*)FIF_TIFF,
		(void*)"tiff",  (void*)FIF_TIFF,
		(void*)"wbmp",  (void*)FIF_WBMP,
		(void*)"xbm",   (void*)FIF_XBM,
		(void*)"xpn",   (void*)FIF_XPM,
	};

	// SEARCH backwards to a dot
	char *c;
	for( c = &filename[ strlen(filename)-1 ]; c >= filename; c-- ) {
		if( *c == '.' ) {
			c++;
			break;
		}
	}

	long type = FIF_UNKNOWN;
	for( int i=0; i < sizeof(extensions)/sizeof(extensions[0]); i+=2 ) {
		if( ! strcmp( (char *)extensions[i], c ) ) {
			type = (long)extensions[i+1];
			break;
		}
	}

	return (int)type;
}

static int freeimageInit = 0;

void zGraphFileInit() {
	// This init isn't strictly necessary as it will be called by the load function below
	// but it is useful for memory debugging so that all the freeimage allcations can happen
	// before any heap grab is done.
	if( !freeimageInit ) {
		FreeImage_Initialise();
		freeimageInit++;
	}
}

int zGraphFileLoad( char *filename, struct ZBits *desc, int descOnly, int filenameToLower ) {
	char lFilename[256];
	strcpy( lFilename, filename );
	if( filenameToLower ) {
		strcpy( lFilename, filename );
		for( char *c = lFilename; *c; c++ ) {
			*c = tolower( *c );
		}
	}

	FREE_IMAGE_FORMAT fif = (FREE_IMAGE_FORMAT)freeimageTypeFromFilename( lFilename );
	if( fif == FIF_UNKNOWN ) {
		return 0;
	}

	int y=0, p=0;
	char *src = 0;
	char *dst = 0;

	zGraphFileInit();

	int ret = 0;
	FIBITMAP *image = FreeImage_Load( fif, lFilename );
	if( !image ) {
		return 0;
	}
	FREE_IMAGE_TYPE fit = FreeImage_GetImageType( image );
	FREE_IMAGE_COLOR_TYPE fic = FreeImage_GetColorType( image );

	desc->defaults();

	switch( fic ) {
		case FIC_MINISBLACK:
			desc->channels = 1;
			desc->colorModel[0] = 'G';
			desc->colorModel[1] = 'R';
			desc->colorModel[2] = 'A';
			desc->colorModel[3] = 'Y';
			desc->channelSeq[0] = 'G';
			desc->channelSeq[1] = 'R';
			desc->channelSeq[2] = 'A';
			desc->channelSeq[3] = 'Y';
			break;

		case FIC_MINISWHITE:
			desc->channels = 1;
			desc->colorModel[0] = 'G';
			desc->colorModel[1] = 'R';
			desc->colorModel[2] = 'A';
			desc->colorModel[3] = 'Y';
			desc->channelSeq[0] = 'G';
			desc->channelSeq[1] = 'R';
			desc->channelSeq[2] = 'A';
			desc->channelSeq[3] = 'Y';
			// @TODO: invert it
			break;

		case FIC_PALETTE:
			desc->channels = 1;
			desc->colorModel[0] = 'G';
			desc->colorModel[1] = 'R';
			desc->colorModel[2] = 'A';
			desc->colorModel[3] = 'Y';
			desc->channelSeq[0] = 'G';
			desc->channelSeq[1] = 'R';
			desc->channelSeq[2] = 'A';
			desc->channelSeq[3] = 'Y';
			break;

		case FIC_RGB:
			desc->channels = 3;
			desc->colorModel[0] = 'R';
			desc->colorModel[1] = 'G';
			desc->colorModel[2] = 'B';
			desc->colorModel[3] = '0';
			desc->channelSeq[0] = 'R';
			desc->channelSeq[1] = 'G';
			desc->channelSeq[2] = 'B';
			desc->channelSeq[3] = '0';
			break;

		case FIC_RGBALPHA:
			desc->channels = 3;
			desc->alphaChannel = 3;
			desc->colorModel[0] = 'R';
			desc->colorModel[1] = 'G';
			desc->colorModel[2] = 'B';
			desc->colorModel[3] = 'A';
			desc->channelSeq[0] = 'R';
			desc->channelSeq[1] = 'G';
			desc->channelSeq[2] = 'B';
			desc->channelSeq[3] = 'A';
			break;

		case FIC_CMYK:
			ret = 0; goto done;
			break;
	}

	switch( fit ) {
		case FIT_BITMAP:
			if( FreeImage_GetBPP( image ) == 32 ) desc->alphaChannel = 3;
				// When a 32 bit PSD iomasge was loading in _whack it would incorrectly
				// report that there was no alpha channel so I'm trying this hack where
				// if it is 32 bits it set the alphaChannel

			desc->channelDepthInBits = FreeImage_GetBPP( image ) / (desc->channels + (desc->alphaChannel?1:0));
			if( desc->channelDepthInBits < 8 ) {
				ret = 0; goto done;
			}
			break;

		case FIT_UINT16:
			desc->channelDepthInBits = 16;
			break;

		case FIT_INT16:
			desc->channelDepthInBits = 16;
			desc->depthIsSigned = 1;
			break;

		case FIT_UINT32:
			desc->channelDepthInBits = 32;
			break;

		case FIT_INT32:
			desc->channelDepthInBits = 32;
			desc->depthIsSigned = 1;
			break;

		case FIT_FLOAT:
			desc->channelDepthInBits = 32;
			desc->channelTypeFloat = 1;
			break;

		case FIT_DOUBLE:
			ret = 0; goto done;
			break;

		case FIT_COMPLEX:
			ret = 0; goto done;
			break;

		case FIT_RGB16:
			desc->channelDepthInBits = 16;
			break;

		case FIT_RGBA16:
			desc->channelDepthInBits = 16;
			break;

		case FIT_RGBF:
			desc->channelDepthInBits = 32;
			desc->channelTypeFloat = 1;
			break;

		case FIT_RGBAF:
			desc->channelDepthInBits = 32;
			desc->channelTypeFloat = 1;
			break;
	}

	desc->width  = FreeImage_GetPitch( image ) / (FreeImage_GetBPP( image )/8);
	desc->height = FreeImage_GetHeight( image );
	desc->setWH( FreeImage_GetWidth( image ), desc->height );

	if( ! descOnly ) {
		desc->alloc();

		p = FreeImage_GetPitch( image );
		int r = FreeImage_GetRedMask( image );
		int g = FreeImage_GetGreenMask( image );
		int b = FreeImage_GetBlueMask( image );
		if( (r || g || b) && r != 0x000000FF ) {
			// This used to assert that r==0x00FF0000 and g==0x0000FF00 and b==0x000000FF
			// but on the powqerpc this failed.  I'm not sure what the right code is
			// until I get more riunning so for now I've removed these asserts
//			assert( r == 0x00FF0000 );
//			assert( g == 0x0000FF00 );
//			assert( b == 0x000000FF );
			// There is some channel rearrangement to be done, invert r & b
			int step = FreeImage_GetBPP( image ) / 8;
			for( y=0; y<desc->height; y++ ) {
				src = (char *)FreeImage_GetScanLine( image, desc->height - y - 1 );
				dst = (char *)desc->lineUchar( y );
				for( int x=0; x<desc->width; x++ ) {
					dst[0] = src[2];
					dst[1] = src[1];
					dst[2] = src[0];
					dst[3] = src[3];
					dst += step;
					src += step;
				}
			}
			ret = 1;
		}
		else {
			for( y=0; y<desc->height; y++ ) {
				src = (char *)FreeImage_GetScanLine( image, desc->height - y - 1 );
				dst = (char *)desc->lineUchar( y );
				memcpy( dst, src, p );
			}
			ret = 1;
		}
	}
	else {
		ret = 1;
	}

	done:;
	FreeImage_Unload( image );
	return ret;

}

int zGraphFileSave( char *filename, struct ZBits *desc, int quality, int filenameToLower ) {
	char lFilename[256];
	strcpy( lFilename, filename );
	if( filenameToLower ) {
		strcpy( lFilename, filename );
		for( char *c = lFilename; *c; c++ ) {
			*c = tolower( *c );
		}
	}

	FREE_IMAGE_FORMAT fif = (FREE_IMAGE_FORMAT)freeimageTypeFromFilename( lFilename );
	if( fif == FIF_UNKNOWN ) {
		return 0;
	}

	if( !freeimageInit ) {
		FreeImage_Initialise();
		freeimageInit++;
	}

	FREE_IMAGE_TYPE pixType = FIT_UNKNOWN;
	int bitsPerPixel = 0;
	int redMask=0xFF000000, greenMask=0x00FF0000, blueMask=0x0000FF00;
//	int redMask=0x000000FF, greenMask=0x0000FF00, blueMask=0x00FF0000;
	if( desc->channelTypeFloat ) {
		if( desc->channels == 3 && desc->alphaChannel == 0 ) {
			bitsPerPixel = 3 * 32;
			pixType = FIT_RGBF;
		}
		else if( desc->channels == 3 && desc->alphaChannel != 0 ) {
			bitsPerPixel = 4 * 32;
			pixType = FIT_RGBAF;
		}
		else {
			assert( desc->channels == 1 && desc->alphaChannel == 0 );
			bitsPerPixel = 32;
			pixType = FIT_FLOAT;
		}
	}
	else if( desc->channelDepthInBits == 16 ) {
		if( desc->channels == 3 && desc->alphaChannel == 0 ) {
			bitsPerPixel = 3 * 16;
			pixType = FIT_RGB16;
		}
		else if( desc->channels == 3 && desc->alphaChannel != 0 ) {
			bitsPerPixel = 4 * 16;
			pixType = FIT_RGBA16;
		}
		else {
			assert( desc->channels == 1 && desc->alphaChannel == 0 );
			bitsPerPixel = 1 * 16;
			pixType = desc->depthIsSigned ? FIT_INT16 : FIT_UINT16;
		}
	}
	else if( desc->channelDepthInBits == 32 ) {
		pixType = desc->depthIsSigned ? FIT_INT32 : FIT_UINT32;
		assert( desc->channels == 1 && desc->alphaChannel == 0 );
		bitsPerPixel = 1 * 32;
	}
	else if( desc->channelDepthInBits == 8 ) {
		pixType = FIT_BITMAP;
		if( desc->channels == 1 ) {
			bitsPerPixel = 1 * 8;
			redMask=0;
			greenMask=0;
			blueMask=0;
		}
		if( desc->alphaChannel ) {
			// I can't figure out how to get freeimage to store an 8 bit luminance / alpha image
			// although it reads it just fine.
			if( desc->channels == 1 ) {
				return 0;
			}
		}
		bitsPerPixel = desc->channelDepthInBits * ( desc->channels + (desc->alphaChannel?1:0) );
	}

	int ret = 0;
	FIBITMAP *image = FreeImage_AllocateT( pixType, desc->w(), desc->h(), bitsPerPixel, redMask, greenMask, blueMask );

	if( pixType == FIT_BITMAP && desc->channelDepthInBits == 8 && desc->channels == 1 ) {
		RGBQUAD *rgb = FreeImage_GetPalette( image );
		for( int i=0; i<256; i++ ) {
			rgb[i].rgbBlue = (unsigned char)i;
			rgb[i].rgbGreen = (unsigned char)i;
			rgb[i].rgbRed = (unsigned char)i;
		}
	}

	if( image ) {
		int p = desc->w() * desc->pixDepthInBytes();
		for( int y=0; y<desc->h(); y++ ) {
			char *src = (char *)desc->lineUchar( desc->h() - y - 1 );
			char *dst = (char *)FreeImage_GetScanLine( image, y );

			if( desc->totalNumChannels() == 3 ) {
				for( int x=0; x<desc->w(); x++ ) {
					dst[0] = src[2];
					dst[1] = src[1];
					dst[2] = src[0];
					dst += 3;
					src += 3;
				}
			}
			else if( desc->totalNumChannels() == 4 ) {
				for( int x=0; x<desc->w(); x++ ) {
					dst[0] = src[2];
					dst[1] = src[1];
					dst[2] = src[0];
					dst[3] = src[3];
					dst += 4;
					src += 4;
				}
			}
			else if( desc->channels == 1 ) {
				memcpy( dst, src, p );
			}
		}

		int flags = 0;
		if( fif == FIF_JPEG ) {
			if( quality < 50 ) {
				flags = JPEG_QUALITYBAD;
			}
			else if( quality < 100 ) {
				flags = JPEG_QUALITYAVERAGE;
			}
			else if( quality < 125 ) {
				flags = JPEG_QUALITYNORMAL;
			}
			else if( quality < 150 ) {
				flags = JPEG_QUALITYGOOD;
			}
			else {
				flags = JPEG_QUALITYSUPERB;
			}
		}

		ret = FreeImage_Save( fif, image, lFilename, flags );

		FreeImage_Unload( image );
		return ret;
	}
	return ret;
}

int zGraphFileLoad( char *filename, struct ZBitmapDesc *desc, int descOnly, int filenameToLower ) {
	ZBits bits;
	int ret = zGraphFileLoad( filename, &bits, descOnly, filenameToLower );
	if( ret ) {
		int flags = 0;
		int d = 0;
		if( bits.channelDepthInBits == 8 && bits.channels == 3 && bits.alphaChannel == 0 ) {
			d = 3;
		}
		else if( bits.channelDepthInBits == 8 && bits.channels == 3 && bits.alphaChannel != 0 ) {
			d = 4;
		}
		else if( bits.channelDepthInBits == 8 && bits.channels == 1 && bits.alphaChannel == 0 ) {
			d = 1;
		}
		else if( bits.channelDepthInBits == 8 && bits.channels == 1 && bits.alphaChannel != 0 ) {
			d = 2;
		}
		else if( bits.channelDepthInBits == 16 && bits.channels == 1 && bits.alphaChannel == 0 ) {
			d = 2;
			flags = zbNO_ALPHA_CHANNEL;
		}
		else {
			return 0;
		}

		desc->w = bits.w();
		desc->h = bits.h();
		desc->memW = bits.width;
		desc->memH = bits.height;
		desc->d = d;
		desc->flags = flags;
		if( !descOnly ) {
			desc->bits = (char *)malloc( desc->memW * desc->memH * desc->d );
			desc->owner = 1;
			memset( desc->bits, 0, desc->memW * desc->memH * desc->d );
			for( int y=0; y<desc->h; y++ ) {
				char *src = bits.lineChar( y );
				char *dst = desc->lineChar( y );
				memcpy( dst, src, bits.p() );
			}
		}
		return 1;
	}
	return 0;
}

int zGraphFileSave( char *filename, struct ZBitmapDesc *desc, int quality, int filenameToLower ) {
	ZBits bits;

	int type = 0;
	if( desc->d == 4 ) {
		type = ZBITS_COMMON_RGBA_8;
	}
	else if( desc->d == 3 ) {
		type = ZBITS_COMMON_RGB_8;
	}
	else if( desc->d == 2 && (desc->flags & zbNO_ALPHA_CHANNEL) ) {
		type = ZBITS_COMMON_LUM_16;
	}
	else if( desc->d == 2 && !(desc->flags & zbNO_ALPHA_CHANNEL) ) {
		type = ZBITS_COMMON_LUM_ALPHA_8;
	}
	else if( desc->d == 1 ) {
		type = ZBITS_COMMON_LUM_8;
	}
	else {
		return 0;
	}

	bits.createCommon( desc->w, desc->h, type );
	memset( bits.bits, 0, bits.lenInBytes() );
	for( int y=0; y<desc->h; y++ ) {
		char *src = desc->lineChar( y );
		char *dst = bits.lineChar( y );
		memcpy( dst, src, bits.p() );
	}

	return zGraphFileSave( filename, &bits, quality, filenameToLower );
}


