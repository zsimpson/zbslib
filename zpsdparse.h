// @ZBS {
//		*MODULE_OWNER_NAME zpsdparse
// }

#ifndef ZPSDPARSE_H
#define ZPSDPARSE_H

#include "zbitmapdesc.h"

struct ZPsdMonoBitmap {
   int w,h;
   int channels;  // number of channels in file for query
   unsigned char *bits;
};

extern int zPsdRead( ZPsdMonoBitmap *map, char *fileName, int channel );
	// read a monochome channel from a PSD
	//   pass in Channel = -1 to query the w,h, and channels fields

extern void zPsdFree( ZPsdMonoBitmap *data );
	// free up the above channel

extern char *zPsdRead4( char *file, int r, int g, int b, int a, int *w, int *h );
	// load a bitmap from a PSD file:
	//      r,g,b,a:
	//      =======
	//      0,1,2,3    loads an RGBA image
	//      0,1,2,-1   loads an RGB image with 0xFF as the alpha channel
	//      4,5,6,7    loads an RGBA image entirely out of alpha channels
	//      c0,c1,d2-256,d3-256
	//                 loads R from channel c0
	//                 loads G from channel c1
	//                 sets  B to d2
	//                 sets  A to d3


#endif
