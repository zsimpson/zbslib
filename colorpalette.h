// @ZBS {
//		*MODULE_OWNER_NAME colorpalette
// }

#ifndef COLORPALETTE_H
#define COLORPALETTE_H

extern float colorPaletteBlueToRedF256[256][4];
extern unsigned char colorPaletteBlueToRedUB256[256][4];

extern float colorPaletteBlueToRedBoundedF256[256][4];
extern unsigned char colorPaletteBlueToRedBoundedUB256[256][4];

extern float colorPaletteWhiteToBlackF256[256][4];
extern unsigned char colorPaletteWhiteToBlackUB256[256][4];

extern float colorPaletteBlackToWhiteF256[256][4];
extern unsigned char colorPaletteBlackToWhiteUB256[256][4];

void colorPaletteInit();

#endif


