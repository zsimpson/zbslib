// @ZBS {
//		*MODULE_OWNER_NAME imgfilter
// }

#ifndef IMGFILTER_H
#define IMGFILTER_H

#include "zbitmapdesc.h"

extern ZBitmapDesc *imgFilterLast;
extern ZBitmapDesc *imgFilterNext;

void imgFilterStart( ZBitmapDesc *src );
void imgFilterSetNext( ZBitmapDesc *next );
void imgFilterAllocNext();
void imgFilterClear();

////////////////////////////////////////////////////////////////////////////////////////////////////

void imgFilterFloodLargePass( ZBitmapDesc *srcD, ZBitmapDesc *dstD, int minGroupCount );

void imgFilterCrop( ZBitmapDesc *srcD, ZBitmapDesc *dstD, int l, int t, int w, int h );
	// Note, the dstD must already be allocated to the correct size

void imgFilterDarkToAlpha( ZBitmapDesc *srcD, ZBitmapDesc *dstD, unsigned char dstC );
	// Given a one byte src and a two byte dst, copy the src
	// into the alpha and dstC into the color.  This makes it transparent according to brightness of src.

#endif

