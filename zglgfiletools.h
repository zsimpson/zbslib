// @ZBS {
//		*MODULE_OWNER_NAME zglgfiletools
// }

#ifndef ZGLGFILETOOLS_H
#define ZGLGFILETOOLS_H

void zglLoadTexture( char *filename, int texture, struct ZBitmapDesc *desc=(struct ZBitmapDesc *)0, int vertFlip=0 );
	// Loads filename into specified texture

int zglGenTexture( char *filename, struct ZBitmapDesc *desc=(struct ZBitmapDesc *)0, int vertFlip=0 );
	// Calls zglGenTexture() and then loads the filename into this texture

void zglLoadTextureRGBA( char *filename, int texture, struct ZBitmapDesc *desc=(struct ZBitmapDesc *)0, int vertFlip=0 );
	// Like zglLoadTexture except expands the texture into RGBA

int zglGenTextureRGBA( char *filename, struct ZBitmapDesc *desc=(struct ZBitmapDesc *)0, int vertFlip=0 );
	// Like zglGenTexture except expands the texture into RGBA

void zglLoadTextureAndAlphaRGBANoinvert( char *filename, char *alphaFilename, int texture, struct ZBitmapDesc *desc=(struct ZBitmapDesc *)0, int vertFlip=0 );
	// Like zglLoadTextureRGBA except that it also loads another
	// alpha file and sets the alpha channel from that
	// In the alpha file, 0 = transparent, 255 = opaque.
void zglLoadTextureAndAlphaRGBAInvert( char *filename, char *alphaFilename, int texture, struct ZBitmapDesc *desc=(struct ZBitmapDesc *)0, int vertFlip=0 );
	// As above but 255 = transparent, 0 = opaque.

int zglGenTextureAndAlphaRGBANoinvert( char *filename, char *alphaFilename, struct ZBitmapDesc *desc=(struct ZBitmapDesc *)0, int vertFlip=0 );
int zglGenTextureAndAlphaRGBAInvert( char *filename, char *alphaFilename, struct ZBitmapDesc *desc=(struct ZBitmapDesc *)0, int vertFlip=0 );
	// Like zglLoadTextureAndAlphaRGBA excepts generates the texture




void zglLoadTextureZBits( char *filename, int texture, struct ZBits *desc=(struct ZBits *)0, int vertFlip=0 );
	// Loads filename into specified texture

int zglGenTextureZBits( char *filename, struct ZBits *desc=(struct ZBits *)0, int vertFlip=0 );
	// Calls zglGenTexture() and then loads the filename into this texture

void zglLoadTextureRGBAZBits( char *filename, int texture, struct ZBits *desc=(struct ZBits *)0, int vertFlip=0 );
	// Like zglLoadTexture except expands the texture into RGBA

int zglGenTextureRGBAZBits( char *filename, struct ZBits *desc=(struct ZBits *)0, int vertFlip=0 );
	// Like zglGenTexture except expands the texture into RGBA

void zglLoadTextureAndAlphaRGBANoinvertZBits( char *filename, char *alphaFilename, int texture, struct ZBits *desc=(struct ZBits *)0, int vertFlip=0 );
	// Like zglLoadTextureRGBA except that it also loads another
	// alpha file and sets the alpha channel from that
	// In the alpha file, 0 = transparent, 255 = opaque.
void zglLoadTextureAndAlphaRGBAInvertZBits( char *filename, char *alphaFilename, int texture, struct ZBits *desc=(struct ZBits *)0, int vertFlip=0 );
	// As above but 255 = transparent, 0 = opaque.

int zglGenTextureAndAlphaRGBANoinvertZBits( char *filename, char *alphaFilename, struct ZBits *desc=(struct ZBits *)0, int vertFlip=0 );
int zglGenTextureAndAlphaRGBAInvertZBits( char *filename, char *alphaFilename, struct ZBits *desc=(struct ZBits *)0, int vertFlip=0 );
	// Like zglLoadTextureAndAlphaRGBA excepts generates the texture



#endif

