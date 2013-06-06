// @ZBS {
//		*MODULE_OWNER_NAME zrgbhsv
// }

#ifndef ZRGBHSV_H
#define ZRGBHSV_H

void zRGBF_to_HSVF( float r, float g, float b, float &h, float &s, float &v );
void zHSVF_to_RGBF( float h, float s, float v, float &r, float &g, float &b );
	// All rgbhsv are measured in normalized 0..1 domain

int zHSVAF_to_RGBAI( float h, float s, float v, float a );

inline int zRGBAI( int r, int g, int b, int a ) { return (r&255)<<24 | (g&255)<<16 | (b&255)<<8 | (a&255); }
	// Encodes the 0.255 domain rgba to an encoded int
int zRGBAF_to_RGBAI( float r, float g, float b, float a );
	// Encodes the normalized 0..1 domain rgba to an encoded int
void zRGBAI_to_RGBAF( int rgba, float &r, float &g, float &b, float &a );
	// Converts normalized 0..1 domain RGBA into an int where r is MSB

void zRGB_to_HSV_Bitmap( struct ZBitmapDesc *rgb, struct ZBitmapDesc *hsv );
	// Converts a whole bitmap to hsv
void zHSV_to_RGB_Bitmap( struct ZBitmapDesc *hsv, struct ZBitmapDesc *rgb );
	// Converts a whole bitmap to rgb

#endif
