// @ZBS {
//		*MODULE_OWNER_NAME zglfont
// }

#ifndef ZGLFONT_H
#define ZGLFONT_H

// ZGLFont
///////////////////////////////////////////////////////////////////////////////////

struct ZGLFont {
	int texture;
	float texCoords[256][2][2];
		// 256 chracters, bottom right and top left texture coordinates
	int hotspot[256][2];
	int width[256]; // integer width
	int height;
	int advance[256];
	int descender;
	int ascender;
	int bitmapW;
	int bitmapH;

	ZGLFont() { texture = 0; }
		// The are constructed by the zglFontLoad free function only

	void getDim( unsigned char c, int &w, int &h, int &d );
	void getDimF( unsigned char c, float &w, float &h, float &d );
	void getDim( char *string, float &w, float &h, float &d, int len=-1 );
	void print( unsigned char c, int x, int y, int inverted=0 );
	void print( char *string, float x, float y, int len=-1, int inverted=0 );
};

void zglFontLoadBitmap( char *string, char *ttfFilename, float size, struct ZBitmapDesc *masterDesc, struct ZGLFont *font=0 );
	// Create a bitmap of the string

void zglFontLoad( char *fontid, char *ttfFilename, float size, int minChar=0, int maxChar=127 );
	// Loads a true type font, renders it with the given size,
	// and stores a pointer to it in a hash table under the name fontid

void *zglFontGet( char *fontid );
	// Returns a pointer to the font which can be passed to
	// the fontPtr field below for speed improvement

void zglFontPrint( char *string, float x, float y, char *fontid, int addDesc=1, void *fontPtr=0, int len=-1 );
	// Render the string at x, y
	// You can pass in either a fontid string or a fontPtr.
	// If you pass only a fontid it will lookup the fontptr in the hashtable
	// If addDesc, it adds in the descender so that the baseline is the bottom of the text

void zglFontPrintInverted( char *string, float x, float y, char *fontid, int addDesc=1, void *fontPtr=0, int len=-1 );
	// Exactly like zglFontPrint but each character is inverted

void zglFontPrintWordWrap( char *m, float l, float t, float w, float h, char *fontid, void *fontPtr=0 );
	// This prints with a word wrap, it doesn't attempt to cache the word wrapping

void zglFontGetDimChar( unsigned char c, float &w, float &h, float &d, char *fontid, void *fontPtr=0 );

void zglFontGetDim( char *string, float &w, float &h, float &d, char *fontid, void *fontPtr=0, int len=-1 );
	// Fetches the width of the specified string,
	// the height and descender of the font *in general*
	// that is, the height and descender always return the same value
	// for a given font.  Units are pixels

float zglFontGetLineH( char *fontid, void *fontPtr=0 );
	// Fetch the height of the line, includes the descender

void zglFontSetTTFSearchPaths( char *path );
	// Set a semi-colon deliminted search path

#endif


