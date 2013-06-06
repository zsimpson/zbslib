// @ZBS {
//		*MODULE_NAME ZBits
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A bitmap description class with the aim of being easily Intel PPL compatible
//		}
//		*PORTABILITY win32 unix
//		*REQUIRED_FILES zbits.cpp zbits.h
//		*VERSION 1.0
//		+HISTORY {
//		}
//		*SELF_TEST no
//		*PUBLISH yes
// }

// OPERATING SYSTEM specific includes:
// STDLIB includes:
#include "assert.h"
#include "memory.h"
#include "stdlib.h"
#include "stdio.h"
// MODULE includes:
// ZBSLIB includes:
#include "zbits.h"
#include "zbitmapdesc.h"

ZBits::ZBits() {	
	defaults();
}

ZBits::ZBits( int w, int h, int d, int _alloc, int _pow2 ) {
	mallocBits = 0;
	create( w, h, d, _alloc, _pow2 );
}

ZBits::ZBits( ZBitmapDesc *desc, int _alloc, int _pow2 ) {
	mallocBits = 0;
	create( desc, _alloc, _pow2 );
}

ZBits::ZBits( ZBits *copy, int _alloc, int _pow2 ) {
	mallocBits = 0;
	create( copy, _alloc, _pow2 );
}

ZBits::~ZBits() {
	clear();
}

int ZBits::specEquals( ZBits *compare ) {
	return !memcmp( this, compare, (char*)&b - (char*)&channels );
}

void ZBits::defaults() {
	channels = 1;
	alphaChannel = 0;
	channelDepthInBits = 8;
	depthIsSigned = 0;
	channelTypeFloat = 0;
	colorModel[0] = 'G';
	colorModel[1] = 'R';
	colorModel[2] = 'A';
	colorModel[3] = 'Y';
	channelSeq[0] = 'G';
	channelSeq[1] = 'R';
	channelSeq[2] = 'A';
	channelSeq[3] = 'Y';
	planeOrder = 0;
	originBL = 0;
	align = 8;
	width = 0;
	height = 0;
	l = 0;
	t = 0;
	r = l + width;
	b = t + height;
	appData = 0;
	bits = 0;
	mallocBits = 0;
	owner = 0;
}

void ZBits::defaultDRules( int d ) {
	switch( d ) {
		case 1:
			channels = 1;
			channelDepthInBits = 8;
			break;
		case 2:
			channels = 2;
			channelDepthInBits = 8;
			alphaChannel = 1;
			break;
		case 3:
			channelDepthInBits = 8;
			channels = 3;
			colorModel[0] = 'R';
			colorModel[1] = 'G';
			colorModel[2] = 'B';
			colorModel[3] = '0';
			channelSeq[0] = 'R';
			channelSeq[1] = 'G';
			channelSeq[2] = 'B';
			channelSeq[3] = '0';
			break;
		case 4:
			channelDepthInBits = 8;
			channels = 3;
			alphaChannel = 3;
			colorModel[0] = 'R';
			colorModel[1] = 'G';
			colorModel[2] = 'B';
			colorModel[3] = '0';
			channelSeq[0] = 'R';
			channelSeq[1] = 'G';
			channelSeq[2] = 'B';
			channelSeq[3] = 'A';
			break;
		default:
			assert( 0 );
	}
}

void ZBits::clear() {
	if( owner ) {
		if( mallocBits ) free( mallocBits );
		bits = 0;
		defaults();
	}
}

void ZBits::create( int _w, int _h, int _d, int _alloc, int _pow2 ) {
	clear();
		// Added when I was working on ZUI when I realized that whis would memory leak
 	defaults();
	width = _w;
	height = _h;
	l = 0;
	t = 0;
	r = _w;
	b = _h;
	if( _pow2 ) {
		upsizePow2();
	}
	defaultDRules( _d );
	if( _alloc ) {
		alloc();
	}
}

void ZBits::createCommon( int _w, int _h, int _mode, int _alloc, int _pow2 ) {
	clear();
		// Added when I was working on ZUI when I realized that whis would memory leak
	defaults();
	width = _w;
	height = _h;
	l = 0;
	t = 0;
	r = _w;
	b = _h;
	if( _pow2 ) {
		upsizePow2();
	}
	switch( _mode ) {
		case ZBITS_COMMON_LUM_8:
			// Default
			channels = 1;
			channelDepthInBits = 8;
			colorModel[0] = 'G';
			colorModel[1] = 'R';
			colorModel[2] = 'A';
			colorModel[3] = 'Y';
			break;

		case ZBITS_COMMON_LUM_16:
			channelDepthInBits = 16;
			break;

		case ZBITS_COMMON_RGB_8:
			channels = 3;
			colorModel[0] = 'R';
			colorModel[1] = 'G';
			colorModel[2] = 'B';
			colorModel[3] = '0';
			channelSeq[0] = 'R';
			channelSeq[1] = 'G';
			channelSeq[2] = 'B';
			channelSeq[3] = '0';
			break;

		case ZBITS_COMMON_RGBA_8:
			channels = 3;
			colorModel[0] = 'R';
			colorModel[1] = 'G';
			colorModel[2] = 'B';
			colorModel[3] = '0';
			channelSeq[0] = 'R';
			channelSeq[1] = 'G';
			channelSeq[2] = 'B';
			channelSeq[3] = 'A';
			alphaChannel = 3;
			break;

		case ZBITS_COMMON_LUM_ALPHA_8:
			channels = 1;
			alphaChannel = 1;
			break;

		case ZBITS_COMMON_LUM_FLOAT:
			channels = 1;
			channelDepthInBits = sizeof(float) * 8;
			channelTypeFloat = 1;
			colorModel[0] = 'G';
			colorModel[1] = 'R';
			colorModel[2] = 'A';
			colorModel[3] = 'Y';
			break;
		
		case ZBITS_COMMON_LUM_32S:
			channels = 1;
			channelDepthInBits = 32;
			channelTypeFloat = 0;
			depthIsSigned = 1;
			colorModel[0] = 'G';
			colorModel[1] = 'R';
			colorModel[2] = 'A';
			colorModel[3] = 'Y';
			break;
		
	}

	if( _alloc ) {
		alloc();
	}
}

void ZBits::create( struct ZBitmapDesc *desc, int _alloc, int _pow2 ) {
	clear();
		// Added when I was working on ZUI when I realized that whis would memory leak
	defaults();
	defaultDRules( desc->d );
	width = desc->memW;
	height = desc->memH;
	setROIxywh( 0, 0, desc->w, desc->h );
	if( _pow2 ) {
		upsizePow2();
	}
	if( _alloc ) {
		alloc();
		owner = 1;
		copyBitsFrom( desc );
	}
	else {
		bits = desc->bits;
		owner = 0;
	}
}

void ZBits::create( ZBits *copy, int _alloc, int _pow2 ) {
	clear();
		// Added when I was working on ZUI when I realized that whis would memory leak
	channels = copy->channels;
	alphaChannel = copy->alphaChannel;
	channelDepthInBits = copy->channelDepthInBits;
	depthIsSigned = copy->depthIsSigned;
	channelTypeFloat = copy->channelTypeFloat;
	colorModel[0] = copy->colorModel[0];
	colorModel[1] = copy->colorModel[1];
	colorModel[2] = copy->colorModel[2];
	colorModel[3] = copy->colorModel[3];
	channelSeq[0] = copy->channelSeq[0];
	channelSeq[1] = copy->channelSeq[1];
	channelSeq[2] = copy->channelSeq[2];
	channelSeq[3] = copy->channelSeq[3];
	planeOrder = copy->planeOrder;
	originBL = copy->originBL;
	align = copy->align;
	width = copy->width;
	height = copy->height;
	appData = copy->appData;
	l = copy->l;
	t = copy->t;
	r = copy->r;
	b = copy->b;
	owner = 0;
	if( _pow2 ) {
		upsizePow2();
	}
	if( _alloc ) {
		alloc();
	}
}

void ZBits::copyBitsFrom( struct ZBitmapDesc *desc ) {
	int _w = w();
	int _h = h();
	for( int y=0; y<_h; y++ ) {
		char *dst = lineChar( y );
		char *src = desc->lineChar( y );
		memcpy( dst, src, _w );
	}
}

void ZBits::upsizePow2() {
	int w = width;
	int h = height;
	int pw = 2;
	int i;
	for( i=1; i<31; i++ ) {
		if( pw >= w ) {
			w = pw;
			break;
		}
		pw *= 2;
	}

	int ph = 2;
	for( i=1; i<31; i++ ) {
		if( ph >= h ) {
			h = ph;
			break;
		}
		ph *= 2;
	}

	int _width = width;
	int _height = height;
	width = pw;
	height = ph;
	setROIxywh( 0, 0, _width, _height );
}

void ZBits::alloc() {
	if( mallocBits ) {
		free( mallocBits );
		mallocBits = 0;
	}
	if( channelDepthInBits	== 1 ) {
		mallocBits = (char *)malloc( (width * height / 8) * channels + height * 8 );
	}
	else {
		mallocBits = (char *)malloc( (width * height * (channelDepthInBits>>3))*( channels+(alphaChannel?1:0) ) + height * 8 );
	}
	#ifdef WIN32
	bits = mallocBits + (0x10 - ((__int64)mallocBits & 0x0F));
	#else
	bits = mallocBits + (0x10 - ((long long)mallocBits & 0x0F));
	#endif
		// Align bits to 16 bytes
	owner = 1;
}

void ZBits::fill( int value ) {
	memset( bits, value, lenInBytes() );
}

void ZBits::setWH( int _w, int _h ) {
	width = _w;
	height = _h;
	setROIxywh( 0, 0, _w, _h );
}

void ZBits::setROIxywh( int _x, int _y, int _w, int _h ) {
	l = _x;
	t = _y;
	r = _x + _w;
	b = _y + _h;
}

void ZBits::setROIltrb( int _l, int _t, int _r, int _b ) {
	l = _l;
	t = _t;
	r = _r;
	b = _b;
}

unsigned int ZBits::getPixel( int x, int y ) {
	int d = pixDepthInBytes();
	switch( d ) {
		case 1: 
			return (unsigned int)*(unsigned char *)&bits[ y*width*d + x*d ];
		case 2: 
			return (unsigned int)*(unsigned short *)&bits[ y*width*d + x*d ];
		case 3: {
			unsigned int a = *(unsigned int *)&bits[ y*width*d + x*d ];
			a &= 0x00FFFFFF;
			return a;
		}
		case 4: 
			if( channelTypeFloat ) {
				return (unsigned int)*(float *)&bits[ y*width*d + x*d ];
			}
			else {
				return (unsigned int)*(unsigned int *)&bits[ y*width*d + x*d ];
			}
		default: 
			assert( 0 );
	}
	return 0;
}

float ZBits::getPixelF( int x, int y ) {
	int d = pixDepthInBytes();
	switch( d ) {
		case 1: 
			return (float)*(unsigned char *)&bits[ y*width*d + x*d ];
		case 2: 
			return (float)*(unsigned short *)&bits[ y*width*d + x*d ];
		case 3: {
			unsigned int a = *(unsigned int *)&bits[ y*width*d + x*d ];
			a &= 0x00FFFFFF;
			return (float)a;
		}
		case 4: 
			if( channelTypeFloat ) {
				return (float)*(float *)&bits[ y*width*d + x*d ];
			}
			else {
				return (float)*(unsigned int *)&bits[ y*width*d + x*d ];
			}
		default: 
			assert( 0 );
	}
	return 0.f;
}

void ZBits::clone( ZBits *desc ) {
	clear();
	create( desc, 1, 0 );
	memcpy( bits, desc->bits, lenInBytes() );
	l = desc->l;
	t = desc->t;
	r = desc->r;
	b = desc->b;
}

void ZBits::transferOwnershipFrom( ZBits *src ) {
	create( src, 0 );
	mallocBits = src->mallocBits;
	bits = src->bits;
	owner = 1;
	src->owner = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void zbitsInvert( ZBits *src ) {
	int lineBytes = src->width * src->pixDepthInBytes();
	char *temp = (char *)malloc( lineBytes );

	int h = src->height / 2;
	for( int y=0; y<h; y++ ) {
		memcpy( temp, src->lineVoid(y), lineBytes );
		memcpy( src->lineVoid(y), src->lineVoid(src->height-y-1), lineBytes );
		memcpy( src->lineVoid(src->height-y-1), temp, lineBytes );
	}

	free( temp );
}

void zbitsThresh( ZBits *src, int thresh, int inverseLogic ) {
	assert( src->channelDepthInBits == 8 );
	assert( src->channels == 1 );
	assert( src->depthIsSigned == 0 );

	int w = src->w();	
	int h = src->h();	
	for( int y=0; y<h; y++ ) {
		unsigned char *s = src->lineUchar( y );
		if( inverseLogic ) {
			for( int x=0; x<w; x++ ) {
				*s = *s >= thresh ? 0 : 255;
				s++;
			}
		}
		else {
			for( int x=0; x<w; x++ ) {
				*s = *s < thresh ? 0 : 255;
				s++;
			}
		}
	}
}

void zbitsAverageFrames( ZBits **arrayOfBitsToAvg, int count, ZBits *dstBits ) {
	int w = dstBits->w();
	int h = dstBits->h();

	// For now this only works on d==1 images
	assert( dstBits->channelDepthInBits == 8 );
	assert( dstBits->channels == 1 );
	assert( dstBits->depthIsSigned == 0 );
	for( int j=0; j<count; j++ ) {
		assert( arrayOfBitsToAvg[j]->w() == dstBits->w() );
		assert( arrayOfBitsToAvg[j]->h() == dstBits->h() );
		assert( arrayOfBitsToAvg[j]->channelDepthInBits == dstBits->channelDepthInBits );
		assert( arrayOfBitsToAvg[j]->channels == dstBits->channels );
		assert( arrayOfBitsToAvg[j]->depthIsSigned == dstBits->depthIsSigned );
	}
	
	for( int y=0; y<h; y++ ) {
		for( int x=0; x<w; x++ ) {
			int sum = 0;
			for( int i=0; i<count; i++ ) {
				sum += arrayOfBitsToAvg[i]->getU1( x, y );
			}
			unsigned char mean = sum / count;
			dstBits->setU1( x, y, mean );
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////


//struct ZBitsFilterNode {
//	ZBitsFilterNode *next;
//	int flags;
//		#define ifOWNER (1)
//	ZBits *bits;
//};
//
//ZBitsFilterNode *zbitsFilterHead = 0;
//ZBitsFilterNode *zbitsFilterTail = 0;
//ZBits *zbitsFilterLast = 0;
//ZBits *zbitsFilterNext = 0;
//
//void zbitsFilterStart( ZBits *src ) {
//	zbitsFilterClear();
//
//	zbitsFilterHead = new ZBitsFilterNode;
//	zbitsFilterHead->flags = 0;
//	zbitsFilterHead->next = 0;
//	zbitsFilterHead->bits = src;
//	zbitsFilterTail = zbitsFilterHead;
//	zbitsFilterLast = zbitsFilterHead->bits;
//	zbitsFilterNext = 0;
//}
//
//void zbitsFilterStop() {
//	zbitsFilterLast = zbitsFilterNext;
//	zbitsFilterNext = 0;
//}
//
//void zbitsFilterAllocNext() {
//	ZBitsFilterNode *n = new ZBitsFilterNode;
//	n->flags = ifOWNER;
//	n->next = 0;
//	n->bits = new ZBits;
//	if( zbitsFilterHead == zbitsFilterTail ) {
//		zbitsFilterNext = zbitsFilterLast;
//	}
//	n->bits->create( zbitsFilterNext, 1 );
//	zbitsFilterLast = zbitsFilterNext;
//	zbitsFilterTail->next = n;
//	zbitsFilterTail = n;
//	zbitsFilterNext = zbitsFilterTail->bits;
//}
//
//void zbitsFilterSetNext( ZBits *next ) {
//	ZBitsFilterNode *n = new ZBitsFilterNode;
//	n->flags = 0;
//	n->next = 0;
//	n->bits = next;
//
//	if( zbitsFilterHead == zbitsFilterTail ) {
//		zbitsFilterNext = zbitsFilterLast;
//	}
//	zbitsFilterLast = zbitsFilterNext;
//	zbitsFilterTail->next = n;
//	zbitsFilterTail = n;
//	zbitsFilterNext = zbitsFilterTail->bits;
//}
//
//void zbitsFilterClear() {
//	for( ZBitsFilterNode *c = zbitsFilterHead; c;  ) {
//		ZBitsFilterNode *n = c->next;
//		if( c->bits && (c->flags & ifOWNER) ) {
//			delete c->bits;
//		}
//		delete c;
//		c = n;
//	}
//	zbitsFilterHead = 0;
//	zbitsFilterTail = 0;
//	zbitsFilterLast = 0;
//	zbitsFilterNext = 0;
//}

void zbitsFloodFill( ZBits *srcBits, ZBits *dstBits, int x, int y, int val, short *pixelList, int *count, int subsampleMask ) {
	int w = dstBits->w();
	int h = dstBits->h();
	assert( dstBits->channelDepthInBits == dstBits->channelDepthInBits );

	int stackTop = 0;
	int stackSize = w * h * 4;
	static int allocatedStackSize = 0;
	static int *stack = 0;
	// CACHE the stack
	if( stackSize > allocatedStackSize ) {
		if( stack ) free( stack );
		allocatedStackSize = stackSize;
		stack = (int *)malloc( sizeof(int) * stackSize );
	}
	#define PUSH(a,b) stack[stackTop] = ((a & 0xFFFF)<<16) | (b & 0xFFFF); stackTop++;
	#define POP(a,b) stackTop--; a=(stack[stackTop]&0xFFFF0000)>>16; b=(stack[stackTop]&0xFFFF); 

	int maxCount = 0;
	if( count ) {
		maxCount = *count;
		*count = 0;
	}
	int doPixList = pixelList && count ? 1 : 0;

	if( dstBits->channelDepthInBits == 8 ) {
		unsigned char srcValUB = srcBits->getUchar( x, y );
		unsigned char valUB = (unsigned char)val;

		PUSH( x, y );
		while( stackTop > 0 ) {
			// POP and EVALUATE the predicate of the pixel on the stack
			int _x, _y;
			POP( _x, _y );
			if( 0 <= _x && _x < w &&  0 <= _y && _y < h ) {
				unsigned char *_src = &srcBits->lineUchar( _y )[_x];
				unsigned char *_dst = &dstBits->lineUchar( _y )[_x];
				// EVAL predicate. If it passes, update dest, push all of its neighbors on the stack
				if( !*_dst && *_src == srcValUB ) {
					if( doPixList && *count < maxCount && (_x & subsampleMask) && (_y & subsampleMask) ) {
						*pixelList++ = _x;
						*pixelList++ = _y;
						(*count)++;
					}

					*_dst = valUB;
					PUSH( _x+1, _y );
					PUSH( _x-1, _y );
					PUSH( _x, _y+1 );
					PUSH( _x, _y-1 );
					assert( stackTop < stackSize );
				}
			}
		}
	}
	else if( dstBits->channelDepthInBits == 16 ) {
		unsigned short srcValUS = srcBits->getUshort( x, y );
		unsigned short valUS = (unsigned short)val;

		PUSH( x, y );
		while( stackTop > 0 ) {
			// POP and EVALUATE the predicate of the pixel on the stack
			int _x, _y;
			POP( _x, _y );
			if( 0 <= _x && _x < w &&  0 <= _y && _y < h ) {
				unsigned short *_src = &srcBits->lineUshort( _y )[_x];
				unsigned short *_dst = &dstBits->lineUshort( _y )[_x];
				// EVAL predicate. If it passes, update dest, push all of its neighbors on the stack
				if( !*_dst && *_src == srcValUS ) {
					if( doPixList && *count < maxCount ) {
						*pixelList++ = _x;
						*pixelList++ = _y;
						(*count)++;
					}

					*_dst = valUS;
					PUSH( _x+1, _y );
					PUSH( _x-1, _y );
					PUSH( _x, _y+1 );
					PUSH( _x, _y-1 );
					assert( stackTop < stackSize );
				}
			}
		}
	}
}

void zbitsWriteBinFile( ZBits *bits, void *filePtr ) {
	FILE *f = (FILE *)filePtr;
	fwrite( bits, sizeof(*bits), 1, f );
		// This actually writes the pointers to mallocBits and bits but these wil be cleaned up on load
	fwrite( bits->bits, bits->lenInBytes(), 1, f );
}

void zbitsReadBinFile( ZBits *bits, void *filePtr ) {
	FILE *f = (FILE *)filePtr;
	fread( bits, sizeof(*bits), 1, f );
	bits->alloc();
	fread( bits->bits, bits->lenInBytes(), 1, f );
}

void zbitsConvert( ZBits *src, ZBits *dst ) {
	// Assumes that if dst.bits is already allocated that we should write there
	// otherwise that it needs to be allocated

	// ALLOC dst bits if needed
	if( !dst->bits ) {
		dst->bits = (char *)malloc( dst->p() * dst->height );
		dst->owner = 1;
	}

	// CLEAR dst to all 0
	memset( dst->bits, 0, dst->height * dst->p() );

	assert( src->w() == dst->w() && src->h() == dst->h() );
		// TODO: implement stretching

	assert( src->channelDepthInBits == 8 );
	assert( dst->channelDepthInBits == 8 );

	if( src->channels == 1 ) {
		if( dst->channels == 1 ) {
			for( int y=0; y<src->h(); y++ ) {
				char *s = src->lineChar( y );
				char *d = dst->lineChar( y );
				memcpy( d, s, src->p() );
			}
		}
		else if( dst->channels == 3 && !dst->alphaChannel ) {
			for( int y=0; y<src->h(); y++ ) {
				char *s = src->lineChar( y );
				char *d = dst->lineChar( y );
				for( int x=0; x<src->w(); x++ ) {
					*d++ = *s;
					*d++ = *s;
					*d++ = *s;
					s++;
				}
			}
		}
		else if( dst->channels == 3 && dst->alphaChannel ) {
			for( int y=0; y<src->h(); y++ ) {
				char *s = src->lineChar( y );
				char *d = dst->lineChar( y );
				for( int x=0; x<src->w(); x++ ) {
					*d++ = *s;
					*d++ = *s;
					*d++ = *s;
					*d++ = (char)255;
					s++;
				}
			}
		}
		else {
			assert( 0 );
				// Unknown
		}
	}
	else if( src->channels == 3 && !src->alphaChannel ) {
		if( dst->channels == 1 ) {
			for( int y=0; y<src->h(); y++ ) {
				unsigned char *s = src->lineUchar( y );
				unsigned char *d = dst->lineUchar( y );
				for( int x=0; x<src->w(); x++ ) {
					unsigned char r = *s++;
					unsigned char g = *s++;
					unsigned char b = *s++;
					unsigned char gb = g > b ? g : b;
					unsigned char rgb = r > gb ? r : gb;
					*d++ = rgb;
				}
			}
		}
		else if( dst->channels == 3 && !dst->alphaChannel ) {
			for( int y=0; y<src->h(); y++ ) {
				char *s = src->lineChar( y );
				char *d = dst->lineChar( y );
				memcpy( d, s, src->p() );
			}
		}
		else if( dst->channels == 3 && dst->alphaChannel ) {
			for( int y=0; y<src->h(); y++ ) {
				char *s = src->lineChar( y );
				char *d = dst->lineChar( y );
				for( int x=0; x<src->w(); x++ ) {
					*d++ = *s++;
					*d++ = *s++;
					*d++ = *s++;
					*d++ = (char)255;
				}
			}
		}
		else {
			assert( 0 );
				// Unknown depth
		}
	}
	else if( src->channels == 3 && src->alphaChannel ) {
		if( dst->channels == 1 ) {
			for( int y=0; y<src->h(); y++ ) {
				unsigned char *s = src->lineUchar( y );
				unsigned char *d = dst->lineUchar( y );
				for( int x=0; x<src->w(); x++ ) {
					unsigned char r = *s++;
					unsigned char g = *s++;
					unsigned char b = *s++;
					s++;
					unsigned char gb = g > b ? g : b;
					unsigned char rgb = r > gb ? r : gb;
					*d++ = rgb;
				}
			}
		}
		else if( dst->channels == 3 && !dst->alphaChannel ) {
			for( int y=0; y<src->h(); y++ ) {
				char *s = src->lineChar( y );
				char *d = dst->lineChar( y );
				for( int x=0; x<src->w(); x++ ) {
					*d++ = *s++;
					*d++ = *s++;
					*d++ = *s++;
					s++;
				}
			}
		}
		else if( dst->channels == 3 && dst->alphaChannel ) {
			for( int y=0; y<src->h(); y++ ) {
				char *s = src->lineChar( y );
				char *d = dst->lineChar( y );
				memcpy( d, s, src->p() );
			}
		}
		else {
			assert( 0 );
				// Unknown depth
		}
	}
	else {
		assert( 0 );
			// Unknown depth
	}
}

void zbitsByteReorder( ZBits *src ) {
	if( src->channels == 3 && !src->alphaChannel ) {
		for( int y=0; y<src->h(); y++ ) {
			char *s = src->lineChar( y );
			for( int x=0; x<src->w(); x++ ) {
				char _0 = s[0];
				char _1 = s[1];
				char _2 = s[2];
				*s++ = _2;
				*s++ = _1;
				*s++ = _0;
			}
		}
	}
	if( src->channels == 3 && src->alphaChannel ) {
		for( int y=0; y<src->h(); y++ ) {
			char *s = src->lineChar( y );
			for( int x=0; x<src->w(); x++ ) {
				char _0 = s[0];
				char _1 = s[1];
				char _2 = s[2];
				char _3 = s[3];
				*s++ = _2;
				*s++ = _1;
				*s++ = _0;
				*s++ = _3;
			}
		}
	}
}

int *zbitsHistogramI( ZBits *src, int numBuckets, int subSample, int *sum ) {
	int *hist = (int *)malloc( numBuckets * sizeof(int) );
	memset( hist, 0, numBuckets * sizeof(int) );
	assert( src->channels == 1 );

	int _sum = 0;
	if( sum ) {
		*sum = 0;
	}

	int w = src->w();
	int h = src->h();
	if( src->channelDepthInBits == 8 ) {
		for( int y=0; y<h; y+=subSample ) {
			unsigned char *s = src->lineU1( y );
			for( int x=0; x<w; x+=subSample ) {
				int bucket = numBuckets * (*s) / 0xFF;
				bucket = 255 < bucket ? 255 : bucket;
				hist[bucket]++;
				_sum++;
				s += subSample;
			}
		}
	}
	else if( src->channelDepthInBits == 16 ) {
		for( int y=0; y<h; y+=subSample ) {
			unsigned short *s = src->lineU2( y );
			for( int x=0; x<w; x+=subSample ) {
				int bucket = numBuckets * (*s) / 0xFFFF;
				bucket = 0xFFFF < bucket ? 0xFFFF : bucket;
				hist[bucket]++;
				_sum++;
				s += subSample;
			}
		}
	}
	else {
		assert( 0 );
	}

	if( sum ) {
		*sum = _sum;
	}

	return hist;
}

float *zbitsHistogramF( ZBits *src, int numBuckets, int subSample, int *sum ) {
	float *hist = (float *)malloc( numBuckets * sizeof(float) );
	memset( hist, 0, numBuckets * sizeof(float) );
	assert( src->channels == 1 );

	int _sum = 0;
	if( sum ) {
		*sum = 0;
	}

	int w = src->w();
	int h = src->h();
	if( src->channelDepthInBits == 8 ) {
		for( int y=0; y<h; y+=subSample ) {
			unsigned char *s = src->lineU1( y );
			for( int x=0; x<w; x+=subSample ) {
				int bucket = numBuckets * (*s) / 0xFF;
				bucket = 255 < bucket ? 255 : bucket;
				hist[bucket]++;
				_sum++;
				s += subSample;
			}
		}
	}
	else if( src->channelDepthInBits == 16 ) {
		for( int y=0; y<h; y+=subSample ) {
			unsigned short *s = src->lineU2( y );
			for( int x=0; x<w; x+=subSample ) {
				int bucket = numBuckets * (*s) / 0xFFFF;
				bucket = 0xFFFF < bucket ? 0xFFFF : bucket;
				hist[bucket]++;
				_sum++;
				s += subSample;
			}
		}
	}
	else {
		assert( 0 );
	}

	if( sum ) {
		*sum = _sum;
	}

	return hist;
}
