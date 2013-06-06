// @ZBS {
//		*MODULE_NAME Zipl
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A class for making bitmap integration with Intel plsuite easier
//		}
//		+EXAMPLE {
//		}
//		*PORTABILITY win32
//		*REQUIRED_FILES zipl.cpp zipl.h
//		*SDK_DEPENDS plsuite
//		*VERSION 1.0
//		+HISTORY {
//		}
//		+TODO {
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }

// OPERATING SYSTEM specific includes:
// STDLIB includes:
#include "assert.h"
// MODULE includes:
// ZBSLIB includes:
#include "zipl.h"
#include "zbitmapdesc.h"

Zipl::Zipl() {
	iplImage = 0;
}

Zipl::Zipl( ZBits *zbits ) {
	set( zbits );
}

Zipl::Zipl( ZBits &zbits ) {
	set( zbits );
}

Zipl::~Zipl() {
	clear();
}

void Zipl::set( ZBits *zbits ) {
	iplImage = iplCreateImageHeader(
		zbits->channels,
		zbits->alphaChannel,
		(zbits->depthIsSigned?IPL_DEPTH_SIGN:0) | zbits->channelDepthInBits,
		zbits->colorModel,
		zbits->channelSeq,
		zbits->planeOrder?IPL_DATA_ORDER_PLANE:IPL_DATA_ORDER_PIXEL,
		zbits->originBL?IPL_ORIGIN_BL:IPL_ORIGIN_TL,
		zbits->align,
		zbits->width,
		zbits->height,
		0,
		0,
		zbits->appData,
		0
	);
	iplImage->imageData = zbits->bits;
	setROIltrb( zbits->l, zbits->t, zbits->r, zbits->b );
}

void Zipl::set( ZBits &zbits ) {
	iplImage = iplCreateImageHeader(
		zbits.channels,
		zbits.alphaChannel,
		(zbits.depthIsSigned?IPL_DEPTH_SIGN:0) | zbits.channelDepthInBits,
		zbits.colorModel,
		zbits.channelSeq,
		zbits.planeOrder?IPL_DATA_ORDER_PLANE:IPL_DATA_ORDER_PIXEL,
		zbits.originBL?IPL_ORIGIN_BL:IPL_ORIGIN_TL,
		zbits.align,
		zbits.width,
		zbits.height,
		0,
		0,
		zbits.appData,
		0
	);
	iplImage->imageData = zbits.bits;
	setROIltrb( zbits.l, zbits.t, zbits.r, zbits.b );
}

void Zipl::clear() {
	if( iplImage ) {
		iplImage->imageData = NULL;
		if( iplImage->roi ) {
			delete iplImage->roi;
			iplImage->roi = 0;
		}
		iplDeallocate( iplImage, IPL_IMAGE_HEADER );
	}
	iplImage = 0;
}

void Zipl::setROIxywh( int _x, int _y, int _w, int _h ) {
	IplROI *roi = new IplROI;
	roi->coi = 0;
	roi->xOffset = _x;
	roi->yOffset = _y;
	roi->width = _w;
	roi->height = _h;
	iplImage->roi = roi;
}

void Zipl::setROIltrb( int _l, int _t, int _r, int _b ) {
	IplROI *roi = new IplROI;
	roi->coi = 0;
	roi->xOffset = _l;
	roi->yOffset = _t;
	roi->width = _r - _l;
	roi->height = _b - _t;
	iplImage->roi = roi;
}
