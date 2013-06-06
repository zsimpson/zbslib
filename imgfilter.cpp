// @ZBS {
//		*MODULE_NAME Image Filter
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A wrapper for doing sequential image filtering
//		}
//		*PORTABILITY win32
//		*REQUIRED_FILES imgfilter.cpp imgfilter.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//			This has not yet been ported to non-win32 but is intended
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }

#include "imgfilter.h"
#include "assert.h"
#include "memory.h"

struct ImgFilterNode {
	ImgFilterNode *next;
	int flags;
		#define ifOWNER (1)
	ZBitmapDesc *desc;
};

ImgFilterNode *imgFilterHead = 0;
ImgFilterNode *imgFilterTail = 0;

ZBitmapDesc *imgFilterLast = 0;
ZBitmapDesc *imgFilterNext = 0;

void imgFilterStart( ZBitmapDesc *src ) {
	imgFilterClear();

	imgFilterHead = new ImgFilterNode;
	imgFilterHead->flags = 0;
	imgFilterHead->next = 0;
	imgFilterHead->desc = src;
	imgFilterTail = imgFilterHead;
	imgFilterLast = imgFilterHead->desc;
	imgFilterNext = 0;
}

void imgFilterAllocNext() {
	ImgFilterNode *n = new ImgFilterNode;
	n->flags = ifOWNER;
	n->next = 0;
	n->desc = new ZBitmapDesc;
	if( imgFilterHead == imgFilterTail ) {
		imgFilterNext = imgFilterLast;
	}
	n->desc->allocQWord( imgFilterNext->w, imgFilterNext->h, imgFilterNext->d );
	n->desc->flags = imgFilterNext->flags;

	imgFilterLast = imgFilterNext;
	imgFilterTail->next = n;
	imgFilterTail = n;
	imgFilterNext = imgFilterTail->desc;
}

void imgFilterSetNext( ZBitmapDesc *next ) {
	ImgFilterNode *n = new ImgFilterNode;
	n->flags = 0;
	n->next = 0;
	n->desc = next;

	if( imgFilterHead == imgFilterTail ) {
		imgFilterNext = imgFilterLast;
	}
	imgFilterLast = imgFilterNext;
	imgFilterTail->next = n;
	imgFilterTail = n;
	imgFilterNext = imgFilterTail->desc;
}

void imgFilterClear() {
	for( ImgFilterNode *c = imgFilterHead; c;  ) {
		ImgFilterNode *n = c->next;
		if( c->desc && (c->flags & ifOWNER) ) {
			delete c->desc;
		}
		delete c;
		c = n;
	}
	imgFilterHead = 0;
	imgFilterTail = 0;
	imgFilterLast = 0;
	imgFilterNext = 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////

int imgFilterFloodForSize( ZBitmapDesc &floodDesc, int _x, int _y, unsigned char marker, int *offsets, int offsetsSize ) {
	int count = 0;

	int stackTop = 0;
	#define STACK_SIZE (16384)
	int stack[STACK_SIZE];

	#define PUSH(a,b) stack[stackTop] = ((a & 0xFFFF)<<16) | (b & 0xFFFF); stackTop++;
	#define POP stackTop--; x=(stack[stackTop]&0xFFFF0000)>>16; y=(stack[stackTop]&0xFFFF); 

	int x = _x;
	int y = _y;
	PUSH( _x, _y );

	while( stackTop > 0 ) {
		POP;

		floodDesc.setUchar( x, y, marker );

		if( count < offsetsSize ) {
			offsets[count] = y*floodDesc.memW+x;
			count++;
		}
		else {
			return count;
		}

		if( stackTop >= STACK_SIZE-4 ) {
			break;
		}

		if( y > 0 && floodDesc.getUchar(x,y-1)==255 ) {
			PUSH( x, y-1 );
		}
		if( y < floodDesc.h-1 && floodDesc.getUchar(x,y+1)==255 ) {
			PUSH( x, y+1 );
		}
		if( x > 0 && floodDesc.getUchar(x-1,y)==255 ) {
			PUSH( x-1, y );
		}
		if( x < floodDesc.w-1 && floodDesc.getUchar(x+1,y)==255 ) {
			PUSH( x+1, y );
		}

		// Diagonals
		if( x > 0 && y > 0 && floodDesc.getUchar(x-1,y-1)==255 ) {
			PUSH( x-1, y-1 );
		}
		if( x < floodDesc.w-1 && y > 0 && floodDesc.getUchar(x+1,y-1)==255 ) {
			PUSH( x+1, y-1 );
		}
		if( x > 0 && y < floodDesc.h-1 && floodDesc.getUchar(x-1,y+1)==255 ) {
			PUSH( x-1, y+1 );
		}
		if( x < floodDesc.w-1 && y < floodDesc.h-1 && floodDesc.getUchar(x+1,y+1)==255 ) {
			PUSH( x+1, y+1 );
		}

	}

	return count;
}

void imgFilterFloodLargePass( ZBitmapDesc *srcD, ZBitmapDesc *dstD, int minGroupCount ) {
	// ALLOCATE a temp buffer to hold the marked ones
	ZBitmapDesc tmpD;
	tmpD.copy( *srcD );

	assert( dstD->memW == dstD->w );
	memset( dstD->bits, 0, dstD->bytes() );

	// EXTRACT connected groups
	#define OFFSETS_SIZE (10000)
	int offsets[OFFSETS_SIZE];
	for( int y=0; y<tmpD.h; y++ ) {
		unsigned char *tmpLine = tmpD.lineUchar(y);
		for( int x=0; x<tmpD.w; x++ ) {
			if( *tmpLine == 255 ) {
				int count = imgFilterFloodForSize( tmpD, x, y, 0x80, offsets, sizeof(offsets)/sizeof(offsets[0]) );
				if( count > minGroupCount ) {
					for( int i=0; i<count; i++ ) {
						dstD->bits[offsets[i]] = (unsigned char)0xFF;
					}
				}
			}
			tmpLine++;
		}
	}
}


void imgFilterCrop( ZBitmapDesc *srcD, ZBitmapDesc *dstD, int l, int t, int w, int h ) {
	assert( l >= 0 && l < srcD->w );
	assert( t >= 0 && t < srcD->h );
	assert( l+w < srcD->w );
	assert( t+h < srcD->h );
	assert( dstD->w == w && dstD->h == h );
	assert( srcD->d == dstD->d );

	int d = dstD->d;
	int r = l + w;
	int b = t + h;
	for( int y=t; y<b; y++ ) {
		char *src = srcD->lineChar( y );
		char *dst = dstD->lineChar( y-t );
		memcpy( dst, &src[l*d], w*d );
	}
}

void imgFilterDarkToAlpha( ZBitmapDesc *srcD, ZBitmapDesc *dstD, unsigned char dstC ) {
	assert( srcD->w == dstD->w );
	assert( srcD->h == dstD->h );
	assert( srcD->d == 1 && dstD->d == 2 );

	int w = srcD->w;
	int h = srcD->h;

	for( int y=0; y<h; y++ ) {
		char *src = srcD->lineChar( y );
		char *dst = dstD->lineChar( y );
		for( int x=0; x<w; x++ ) {
			*dst++ = dstC;
			*dst++ = *src;
			src++;
		}
	}
}

