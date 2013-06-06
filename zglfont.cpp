// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A font printing class for OpenGL that uses the Freetype SDK
//		}
//		*PORTABILITY win32
//		*SDK_DEPENDS freetype
//		*REQUIRED_FILES zglfont.cpp zglfont.h
//		*VERSION 1.3
//		+HISTORY {
//			1.2 Chris Hecker converted to use the gr intermediate file for DX / GL independence
//			1.3 Eliminated gr.h dependeny
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }
// OPERATING SYSTEM specific includes:

#include "wingl.h"
#include "GL/gl.h"
//#include "GL/glu.h"
// SDK includes:
#include "freetype.h"
// STDLIB includes:
#include "assert.h"
#include "stdlib.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "math.h"

// MODULE includes:
#include "zglfont.h"
// ZBSLIB includes:
#include "zbitmapdesc.h"
#include "zhashtable.h"

// For debugging:
//#include "zgraphfile.h"
//#include "zfilespec.h"
//#include "ztmpstr.h"

// STOPFLOW timing
#include "sfmod.h"

static ZHashTable zglFontHash;
static char *zglFontSearchPath = 0;

// ZGLFont
///////////////////////////////////////////////////////////////////////////////////

void ZGLFont::getDim( unsigned char c, int &w, int &h, int &d ) {
	w = advance[c];
	h = height;
	d = descender;
}

void ZGLFont::getDimF( unsigned char c, float &w, float &h, float &d ) {
	w = (float)advance[c];
	h = (float)height;
	d = (float)descender;
}

void ZGLFont::getDim( char *string, float &w, float &h, float &d, int len ) {
	if( !string ) {
		w = 0.f;
		h = 0.f;
		d = 0.f;
		return;
	}
	h = (float)height;
	d = (float)descender;

	int iw = 0;
	for( unsigned char *c = (unsigned char *)string; *c && len; c++, len-- ) {
		iw += advance[*c];
	}
	w = (float)iw;
}

int zglFontRoundToInt( float x ) {
	float f = (float)floor(x);
	if( x - f < 0.5f ) return (int)f;
	return (int)f + 1;
}


void ZGLFont::print( unsigned char c, int x, int y, int inverted ) {
	float s0=texCoords[c][0][0], t0=texCoords[c][0][1];
	float s1=texCoords[c][1][0], t1=texCoords[c][1][1];

	int hx = hotspot[c][0], hy = hotspot[c][1];
	int x0 = x            , y0 = y + ascender;
	int x1 = x + width[c] , y1 = y - descender;
	
	x0 -= hx;
	x1 -= hx;
	
	glBegin(GL_QUADS);
		if( inverted ) {
			glTexCoord2f(s0,t1); glVertex2i(x0,y0);
			glTexCoord2f(s0,t0); glVertex2i(x0,y1);
			glTexCoord2f(s1,t0); glVertex2i(x1,y1);
			glTexCoord2f(s1,t1); glVertex2i(x1,y0);
		}
		else {
			glTexCoord2f(s0,t0); glVertex2i(x0,y0);
			glTexCoord2f(s0,t1); glVertex2i(x0,y1);
			glTexCoord2f(s1,t1); glVertex2i(x1,y1);
			glTexCoord2f(s1,t0); glVertex2i(x1,y0);
		}
	glEnd();
}

// mmk - glPushAttrib() and glPopAttrib() are *extremely* expensive on my home machine.
// So for stopflow, these are removed and replaced with only the attributes needed.
// For other zlab apps, these continue to be used.

void ZGLFont::print( char *string, float x, float y, int len, int inverted ) {
	if( !string ) {
		return;
	}
	SFTIME_START (PerfTime_ID_Zlab_render_glfont, PerfTime_ID_Zlab_render_render);
	int ix = zglFontRoundToInt(x);
	int iy = zglFontRoundToInt(y);

#ifndef SFFAST
	glPushAttrib( GL_TEXTURE_BIT | GL_ENABLE_BIT );
#endif
    glBindTexture( GL_TEXTURE_2D, texture );
    glEnable( GL_TEXTURE_2D );

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
#ifndef SFFAST
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
#endif
	int count = 0;
	for( unsigned char *c = (unsigned char *)string; *c; c++ ) {
		if( len != -1 && count >= len ) {
			break;
		}
		count++;
		print( *c, ix, iy, inverted );
		ix += advance[*c];
	}
#ifdef SFFAST
	glDisable( GL_BLEND );
    glDisable( GL_TEXTURE_2D );
#else
	glPopAttrib();
#endif
	SFTIME_END (PerfTime_ID_Zlab_render_glfont);
}


// zgl Interface
///////////////////////////////////////////////////////////////////////////////////

int zglFontFileExists( char *f ) {
	FILE *ff = fopen( f, "rb" );
	if( ff ) {
		fclose( ff );
		return 1;
	}
	return 0;
}


void zglFontLoadBitmap( char *string, char *ttfFilename, float size, ZBitmapDesc *masterDesc, ZGLFont *font ) {
	// This function is meant to only run infrequently (like at init time)
	// It builds a big bitmap of all the letters in the font and stores
	// the texture coordinates for each one.  It recreates the 
	// FreeType engine on every call for simplicity.  (It may be
	// that this will be too slow and it will need to be cached.)
	assert( font );

	TT_Engine engine;
	int error = TT_Init_FreeType( &engine );
	assert( !error );

	// OPEN the TTF File
	TT_Face face;

	char *c;
	char buf[256];
	strcpy( buf, ttfFilename );
	char *path = zglFontSearchPath;
	while( !zglFontFileExists( buf ) ) {
		for( c=path; c ; c++ ) {
			if( *c == ';' || *c == 0 ) {
				strncpy( buf, path, c-path );
				strcpy( &buf[c-path], ttfFilename );
				path=c+1;
				break;
			}
			if( !*c ) break;
		}
		if( !c || !*c ) break;
	}
	error = TT_Open_Face( engine, buf, &face );
	assert( !error );

	// CREATE an instance (a point-size).
	TT_Instance instance;
	error = TT_New_Instance( face, &instance );
	assert( !error );

	// SET the point size
	error = TT_Set_Instance_CharSize( instance, (int)(size * 64.f) );
	assert( !error );

	// CREATE a glyph container
	TT_Glyph glyph;
	error = TT_New_Glyph( face, &glyph );
	assert( !error );

	TT_Face_Properties	properties;
	TT_Get_Face_Properties( face, &properties );
	int monoSpaced = properties.postscript->isFixedPitch;

	// FETCH the metrics
	TT_Instance_Metrics metrics;
	error = TT_Get_Instance_Metrics( instance, &metrics );
	assert( !error );

	// BUILD a character mapping for ascii
	int numMaps = properties.num_CharMaps;
	TT_CharMap charMap;
	int i;
	for( i = 0; i < numMaps; i++ ) {
		unsigned short platform = 0;
		unsigned short encoding = 0;
		TT_Get_CharMap_ID( face, i, &platform, &encoding );
		if( (platform == 3 && encoding == 1) || (platform == 0 && encoding == 0) ) {
			TT_Get_CharMap( face, i, &charMap );
			break;
		}
	}

	assert( i < numMaps );
		// Apparently there is not simple mapping.	
		// Figure out what the right thing to do here is.

	TT_UShort index[256] = {0,};
	for( int code= 0; code < 256; code++ ) {
		int glyphI = TT_Char_Index( charMap, code );
		if( glyphI < 0 ) {
			glyphI = 0;
		}
		index[code] = glyphI;
	}

	memset( font->advance, 0, sizeof(font->advance) );

	// LOAD each glyph and figure out the sizes so
	// that we can allocate an appropriate bitmap for all of them
	// We are computing the maximum and minimium bounds of the set

	//int charSpacing = 2;
		// This is the spacing as it is in the bitmap.  It doesn't
		// have anything to do with the spacing of the font itself
		// which is determined by the advance

	int monoSpacing = 0;
	if( monoSpaced ) {
		error = TT_Load_Glyph( instance, glyph, index['W'], TTLOAD_SCALE_GLYPH | TTLOAD_HINT_GLYPH );
		assert( !error );
		TT_Big_Glyph_Metrics gMetrics;
		error = TT_Get_Glyph_Big_Metrics( glyph, &gMetrics );
		monoSpacing = int( (float)gMetrics.horiAdvance / 64.f );
		assert( !error );
	}

	//////////////////////////////////////////////////////////////////////////
	// PASS 1: Accumulate widths and line height
	//////////////////////////////////////////////////////////////////////////

	// ACCUMULATE the list of widths and the maximum line height
	unsigned char *uc;
	int totalW = 0;
	TT_Outline outline;
	TT_BBox bbox, largest_bbox;
	TT_Raster_Map pixmap;
	TT_Big_Glyph_Metrics gMetrics;
	TT_Byte palette[5] = { 0, 0x40, 0x80, 0xC0, 0xFF };

	largest_bbox.xMin = 0;
	largest_bbox.xMax = 0;
	largest_bbox.yMin = 0;
	largest_bbox.yMax = 0;

	for( uc=(unsigned char *)string; *uc; uc++ ) {
		if( index[*uc] == 0 ) {
			continue;
		}

		TT_Load_Glyph( instance, glyph, index[*uc], TTLOAD_SCALE_GLYPH | TTLOAD_HINT_GLYPH );
		TT_Get_Glyph_Big_Metrics( glyph, &gMetrics );
		TT_Get_Glyph_Outline( glyph, &outline );
		TT_Get_Outline_BBox( &outline, &bbox );

		// GRID fit it, convert to pixels from 1/64ths of a pixels
		bbox.xMin = (int)ceilf( float(bbox.xMin & -64) / 64.f );
		//bbox.xMin = min(0,bbox.xMin);
		bbox.yMin = (int)ceilf( float(bbox.yMin & -64) / 64.f );
		bbox.xMax = (int)ceilf( float((bbox.xMax + 63) & -64) / 64.f );
		bbox.yMax = (int)ceilf( float((bbox.yMax + 63) & -64) / 64.f );

		largest_bbox.xMin = min( bbox.xMin, largest_bbox.xMin );
		largest_bbox.xMax = max( bbox.xMax, largest_bbox.xMax );
		largest_bbox.yMin = min( bbox.yMin, largest_bbox.yMin );
		largest_bbox.yMax = max( bbox.yMax, largest_bbox.yMax );

		int w = (bbox.xMax - bbox.xMin) + 1;
			// @HACK: I don't understand this +1, see below regarding verdanda 10 'i'
		int h = bbox.yMax - bbox.yMin;
		totalW += w + 2;
			// ZBS tried acdding a bit here to avoid rounding problems.
			// It seems to help a lot in cases where you are printing a font
			// after a scale so that it is not one to one pixel
			// see below also
	}

	//////////////////////////////////////////////////////////////////////////
	// ALLOCATE the bitmap so that it is 512 wide by some other power of 2 high
	//////////////////////////////////////////////////////////////////////////
	int lineH =   largest_bbox.yMax - largest_bbox.yMin;
	int bitmapW = totalW;
	int bitmapH = lineH;

	while( bitmapW > 512 ) {
		bitmapW /= 2;
		bitmapH *= 2;
	}
	bitmapH += lineH*4;
		// I add another couple of line's worth of height into the texture map
		// because of potential rounding problems... That is, the 
		// calculation of dividing the line in half over and over
		// assumes that you can cut letters in half which you
		// can't so sometimes a little extra room is needed and thus
		// this extra line is added

	masterDesc->allocP2( bitmapW, bitmapH, 2 );
	memset( masterDesc->bits, 0, masterDesc->bytes() );

	//////////////////////////////////////////////////////////////////////////
	// PASS 2: Write the fonts into the bitmap
	//////////////////////////////////////////////////////////////////////////

	int x = 0, y = 0;
	for( uc=(unsigned char *)string; *uc; uc++ ) {

//if( *uc=='m' ) {
//int a = 1;
//}

		TT_Load_Glyph( instance, glyph, index[*uc], TTLOAD_SCALE_GLYPH | TTLOAD_HINT_GLYPH );
		TT_Get_Glyph_Outline( glyph, &outline );
		TT_Get_Glyph_Big_Metrics( glyph, &gMetrics );
		TT_Get_Outline_BBox( &outline, &bbox );

		if( index[*uc] == 0 ) {
			memset( &bbox, 0, sizeof(bbox) );
		}
		else {
			// GRID fit it, convert to pixels from 1/64ths of a pixels
			bbox.xMin = (int)ceilf( float(bbox.xMin & -64) / 64.f );
//			bbox.xMin = min(0,bbox.xMin);
			bbox.yMin = (int)ceilf( float(bbox.yMin & -64) / 64.f );
			bbox.xMax = (int)ceilf( float((bbox.xMax + 63) & -64) / 64.f );
			bbox.yMax = (int)ceilf( float((bbox.yMax + 63) & -64) / 64.f );
		}

		int width = bbox.xMax - min(bbox.xMin,0);
		width++;
			// @HACK: I don't understand why this needs to be +1 here
			// Why does it need to be plus one here?
			// Verdana 10 point 'i' doesn't show up unless this is +1

		if( *uc == ' ' ) {
			width = gMetrics.horiAdvance / 64;
		}

		// WRAP if needed
		if( x + width > masterDesc->w ) {
			x = 0;
			y += lineH;
		}
	
		// ALLOCATE the memory for one character's pixmap
		ZBitmapDesc desc;
		desc.allocP2( width, lineH, 1 );

		// SETUP the FreeType Pixmap
		pixmap.rows   = desc.h;
		pixmap.cols   = desc.memW; // pitch
		pixmap.width  = desc.w; // width
		pixmap.flow   = TT_Flow_Down;
		pixmap.size   = 0;
		pixmap.bitmap = desc.bits;

		TT_Set_Raster_Gray_Palette( engine, palette );

		TT_Translate_Outline( &outline, -bbox.xMin*64, -largest_bbox.yMin*64 );
			// This shifts the character over int the desc such that
			// the bottom row will have the the bottom most descnder
			// So the base line is moved up by the descender height
			// It is also moved over right by the x left cutin (for example: 'j')
			// has a slightly negative val

		TT_Get_Outline_Pixmap( engine, &outline, &pixmap );

		// COPY into the master bitmap
		for( int _y=0; _y<desc.h; _y++ ) {
			unsigned char *src = desc.lineUchar( _y );
			unsigned char *dst = &masterDesc->lineUchar( _y + y )[ x * masterDesc->d ];
			for( int _x=0; _x<desc.w; _x++ ) {
				*dst++ = 0xff;
				*dst++ = *src++;
			}
		}

		// SETUP the font size information for this char
		font->texCoords[*uc][0][0] = (float)x / (float)masterDesc->memW;
		font->texCoords[*uc][0][1] = (float)y / (float)masterDesc->memH;
		font->texCoords[*uc][1][0] = (float)(x+width) / (float)masterDesc->memW;
		font->texCoords[*uc][1][1] = (float)(y+lineH) / (float)masterDesc->memH;

		// In the case that the char has a negative xMin we need to 
		// add a extra space onto the right because it would have
		// been expected to draw the character slightly to the left
		// of where we will really draw it since we don't allow
		// negative xMins
		int extraW = (bbox.xMin < 0 ? -bbox.xMin : 0);
		font->width[*uc] = width + extraW;

		font->hotspot[*uc][0] = -bbox.xMin;
		font->hotspot[*uc][1] = -bbox.yMin;

		//font->advance[*uc] = monoSpaced ? monoSpacing : int( (float)gMetrics.horiAdvance / 64.f );
		font->advance[*uc] = monoSpaced ? monoSpacing : int( (float)gMetrics.horiAdvance / 64.f ) + extraW;

		// INCEREMENT (Wrap is done above after next w is calc'd)
		x += width + 2;
			// ZBS tried acdding a bit here to avoid rounding problems.
			// It seems to help a lot in cases where you are printing a font
			// after a scale so that it is not one to one pixel
			// see above also
	}

//ZFileSpec spec( ttfFilename );
//zGraphFileSave( ZTmpStr("%s-%f%s",spec.getFile(0),size,".png"), masterDesc );
// the cap O is 15 by 16 in the output file
// the little w is 11 x 9
// There's 2 pixels between N and O

	assert( masterDesc->d == 2 );
	font->texture = 0;
	glGenTextures( 1, (unsigned int *)&font->texture );
	glBindTexture( GL_TEXTURE_2D, font->texture );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0 );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0 );
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	glBindTexture( GL_TEXTURE_2D, font->texture );
	glTexImage2D( GL_TEXTURE_2D, 0, masterDesc->d, masterDesc->memW, masterDesc->memH, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, masterDesc->bits );
//	gluBuild2DMipmaps( GL_TEXTURE_2D, GL_LUMINANCE_ALPHA, masterDesc->memW, masterDesc->memH, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, masterDesc->bits );
//	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
//	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );

	font->descender = -largest_bbox.yMin;
	font->height = lineH;
	font->ascender = font->height - font->descender;
	font->bitmapW = masterDesc->memW;
	font->bitmapH = masterDesc->memH;

	TT_Close_Face( face );
	error = TT_Done_FreeType( engine );
	assert( !error );
}

void zglFontLoad( char *fontid, char *ttfFilename, float size, int minChar, int maxChar ) {
	ZBitmapDesc masterDesc;
	ZGLFont *font = new ZGLFont();
	assert( maxChar-minChar < 256 );
	unsigned char fullSet[257] = {0,};
    int i;
	for( i=minChar; i<=maxChar; i++ ) {
		fullSet[i] = i == 0 ? 1 : i;
	}
	fullSet[0] = 1;
	fullSet[i] = 0;
	zglFontLoadBitmap( (char *)fullSet, ttfFilename, size, &masterDesc, font );
	zglFontHash.putP( fontid, font );
}

void *zglFontGet( char *fontid ) {
	return (void *)zglFontHash.getp( fontid );
}

void zglFontPrint( char *string, float x, float y, char *fontid, int addDesc, void *fontPtr, int len ) {
	ZGLFont *font = (ZGLFont *)fontPtr;
	if( !fontPtr ) {
		font = (ZGLFont *)zglFontHash.getp( fontid );
		if( !font ) return;
	}

	font->print( string, x, y+(addDesc?font->descender:0.f), len );
}

void zglFontPrintInverted( char *string, float x, float y, char *fontid, int addDesc, void *fontPtr, int len ) {
	ZGLFont *font = (ZGLFont *)fontPtr;
	if( !fontPtr ) {
		font = (ZGLFont *)zglFontHash.getp( fontid );
		if( !font ) return;
	}

	font->print( string, x, y+(addDesc?font->descender:0.f), len, 1 );
}



/*
void zglFontPrintCenter( char *string, float x, float y, char *fontid, int addDesc, void *fontPtr, int len) {
	ZGLFont *font = (ZGLFont *)fontPtr;
    float new_x = 0.f;
	if( !fontPtr ) {
		font = (ZGLFont *)zglFontHash.getp( fontid );
		if( !font ) return;
	}
    float w, h, d;
    font->getDim( string, w, h, d );
    new_x -= w/2;
    printf("OLDX: %f, NEW: %f", x, new_x);
    font->print( string, new_x, y+(addDesc?font->descender:0.f), len );

    // ROTATED TEXT ?? yet to implement ??
//    
//        if (degrees != 0.f) {
//            // Rotate the matrix
//            glPushMatrix();
//            // Move the point to the origin and rotate
//            glTranslatef(new_x, y, 0);
//            //glRotatef(degrees,0.f,0.f,1.0f);
//            font->print( string, 0.f+(addDesc?font->descender:0.f), 0.f );
//            glPopMatrix();
//        }
//  
}
*/

void zglFontGetDimChar( unsigned char c, float &w, float &h, float &d, char *fontid, void *fontPtr ) {
	ZGLFont *font = (ZGLFont *)fontPtr;
	if( !fontPtr ) {
		font = (ZGLFont *)zglFontHash.getp( fontid );
		if( !font ) return;
	}
	font->getDimF( c, w, h, d );
}

void zglFontGetDim( char *string, float &w, float &h, float &d, char *fontid, void *fontPtr, int len ) {
	ZGLFont *font = (ZGLFont *)fontPtr;
	if( !fontPtr ) {
		font = (ZGLFont *)zglFontHash.getp( fontid );
		w = 0.f; h=0.f; d=0.f;
		if( !font ) return;
	}
	font->getDim( string, w, h, d, len );
}

float zglFontGetLineH( char *fontid, void *fontPtr ) {
	ZGLFont *font = (ZGLFont *)fontPtr;
	if( !fontPtr ) {
		font = (ZGLFont *)zglFontHash.getp( fontid );
		if( !font ) return 0.f;
	}
	return (float)font->height;
}

void zglFontSetTTFSearchPaths( char *path ) {
	if( zglFontSearchPath ) free( zglFontSearchPath );
	zglFontSearchPath = strdup( path );
}

void zglFontPrintWordWrap( char *m, float l, float t, float w, float h, char *fontid, void *fontPtr ) {
	float segmentW = 0.f, segmentH = 0.f;
	char temp;
	char *lastSpace = NULL;

	ZGLFont *font = (ZGLFont *)fontPtr;
	if( !fontPtr ) {
		font = (ZGLFont *)zglFontHash.getp( fontid );
		if( !font ) return;
	}

	int len = strlen( m ) + 1;
	char *copyOfM = (char *)malloc( len );
	memcpy( copyOfM, m, len );
	char *segmentStart = copyOfM;
	float descender;
	zglFontGetDim( "W", segmentW, segmentH, descender, 0, font );
	float y = t - segmentH;

	// Calculate word-wrap
	for( char *c = copyOfM; ; c++ ) {
		if( *c == '\n' || *c == 0 ) {
			temp = *c;
			*c = 0;
			zglFontPrint( segmentStart, l, y, 0, 1, font, -1 );
			*c = temp;
			y -= segmentH;
			segmentStart = c+1;
			segmentW = 0.0;
			if( *c == 0 ) {
				break;
			}
		}
		else {
			char buf[2];
			buf[0] = *c;
			buf[1] = 0;
			float charW = 0.f;
			zglFontGetDim( buf, charW, segmentH, descender, 0, font );
			segmentW += charW;

			if( segmentW >= w ) {
				if( !lastSpace ) {
					lastSpace = c;
				}
				temp = *lastSpace;
				*lastSpace = 0;
				zglFontPrint( segmentStart, l, y, 0, 1, font, -1 );
				*lastSpace = temp;
				y -= segmentH;
				segmentStart = *lastSpace==' ' ? lastSpace+1 : lastSpace;
				c = lastSpace;
				lastSpace = NULL;
				segmentW = 0.0;
			}
			else if( *c == ' ' ) {
				lastSpace = c;
			}
		}
	}
    free( copyOfM );
}

