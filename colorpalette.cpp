// @ZBS {
//		*MODULE_NAME Colorpalette
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			Common color palettes
//		}
//		+EXAMPLE {
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES colorpalette.cpp colorpalette.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//			I need a cleaner interface for this
//		}
//		*SELF_TEST no
//		*PUBLISH yes
// }


#include "colorpalette.h"
#include "zrgbhsv.h"
#include "memory.h"

float colorPaletteBlueToRedF256[256][4];
unsigned char colorPaletteBlueToRedUB256[256][4];

float colorPaletteBlueToRedBoundedF256[256][4];
unsigned char colorPaletteBlueToRedBoundedUB256[256][4];

float colorPaletteWhiteToBlackF256[256][4];
unsigned char colorPaletteWhiteToBlackUB256[256][4];

float colorPaletteBlackToWhiteF256[256][4];
unsigned char colorPaletteBlackToWhiteUB256[256][4];

void colorPaletteInit() {
	static int colorPaletteBuilt = 0;
	if( !colorPaletteBuilt ) {
		colorPaletteBuilt++;
		int i;

		// BLUE to RED
		for( i=0; i<256; i++ ) {
			float h = ( (i * 186 / 256 + 80) % 256 ) / 256.f;
			zHSVF_to_RGBF( 0.99f-h, 1.f, 1.f, colorPaletteBlueToRedF256[i][0], colorPaletteBlueToRedF256[i][1], colorPaletteBlueToRedF256[i][2] );
			colorPaletteBlueToRedF256[i][3] = 1.f;

			colorPaletteBlueToRedUB256[i][0] = (unsigned char)(colorPaletteBlueToRedF256[i][0] * 255.f);
			colorPaletteBlueToRedUB256[i][1] = (unsigned char)(colorPaletteBlueToRedF256[i][1] * 255.f);
			colorPaletteBlueToRedUB256[i][2] = (unsigned char)(colorPaletteBlueToRedF256[i][2] * 255.f);
			colorPaletteBlueToRedUB256[i][3] = 255;
		}

		memcpy( colorPaletteBlueToRedBoundedF256, colorPaletteBlueToRedF256, sizeof(colorPaletteBlueToRedF256) );
		memcpy( colorPaletteBlueToRedBoundedUB256, colorPaletteBlueToRedUB256, sizeof(colorPaletteBlueToRedUB256) );
		colorPaletteBlueToRedBoundedF256[0][0] = 0.f;
		colorPaletteBlueToRedBoundedF256[0][1] = 0.f;
		colorPaletteBlueToRedBoundedF256[0][2] = 0.f;
		colorPaletteBlueToRedBoundedF256[255][0] = 1.f;
		colorPaletteBlueToRedBoundedF256[255][1] = 1.f;
		colorPaletteBlueToRedBoundedF256[255][2] = 1.f;
		colorPaletteBlueToRedBoundedUB256[0][0] = 0;
		colorPaletteBlueToRedBoundedUB256[0][1] = 0;
		colorPaletteBlueToRedBoundedUB256[0][2] = 0;
		colorPaletteBlueToRedBoundedUB256[255][0] = 255;
		colorPaletteBlueToRedBoundedUB256[255][1] = 255;
		colorPaletteBlueToRedBoundedUB256[255][2] = 255;

		// WHITE to BLACK
		for( i=0; i<256; i++ ) {
			float c = float(256 - i) / 256.f;
			colorPaletteWhiteToBlackF256[i][0] = c;
			colorPaletteWhiteToBlackF256[i][1] = c;
			colorPaletteWhiteToBlackF256[i][2] = c;
			colorPaletteWhiteToBlackF256[i][3] = 1.f;

			unsigned char ub = (unsigned char)(c * 255.f);
			colorPaletteWhiteToBlackUB256[i][0] = ub;
			colorPaletteWhiteToBlackUB256[i][1] = ub;
			colorPaletteWhiteToBlackUB256[i][2] = ub;
			colorPaletteWhiteToBlackUB256[i][3] = 255;
		}

		// BLACK to WHITE
		for( i=0; i<256; i++ ) {
			float c = float(i) / 256.f;
			colorPaletteBlackToWhiteF256[i][0] = c;
			colorPaletteBlackToWhiteF256[i][1] = c;
			colorPaletteBlackToWhiteF256[i][2] = c;
			colorPaletteBlackToWhiteF256[i][3] = 1.f;

			unsigned char ub = (unsigned char)(c * 255.f);
			colorPaletteBlackToWhiteUB256[i][0] = ub;
			colorPaletteBlackToWhiteUB256[i][1] = ub;
			colorPaletteBlackToWhiteUB256[i][2] = ub;
			colorPaletteBlackToWhiteUB256[i][3] = 255;
		}
	}
}

