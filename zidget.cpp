// @ZBS {
//		*MASTER_FILE 1
//		+DESCRIPTION {
//			A system of binary interactors
//		}
//		*PORTABILITY win32 unix osx
//		*REQUIRED_FILES zidget.cpp zidget.h
//		*VERSION 1.0
//		+HISTORY {
//			Verion 1: 3 Jan 2007
//		}
//		+TODO {
//			Separate the zidgets like I did zuis to eliminate dependencies on unused components
//			Remove colorpalette
//		}
//		*SELF_TEST no
//		*PUBLISH no
// }


// OPERATING SYSTEM specific includes:
#include "wingl.h"
// SDK includes:
#include "GL/gl.h"
#include "GL/glu.h"
// STDLIB includes:
#include "assert.h"
#include "stdlib.h"
#include "float.h"
#include "string.h"
#include "math.h"
// MODULE includes:
#include "zidget.h"
#include "zmathtools.h"
#include "zmousemsg.h"
#include "zgltools.h"
#include "zglfont.h"
#include "zbittexcache.h"
#include "colorpalette.h"
#include "plot.h"
// ZBSLIB includes:

//-----------------------------------------------------------------------------------------------
// ZidgetHandle
//-----------------------------------------------------------------------------------------------

FVec2 ZidgetHandle::mouseWorldPos;
FVec2 ZidgetHandle::mouseWinPos;
ZidgetHandle *ZidgetHandle::mouseDraggingHandlePtr = 0;

float zidgetHandleDrawGetAlpha( ZidgetHandle *zidgetHandle ) {
	float alpha = 1.f;
	FVec2 dist = zidgetHandle->winPos;
	dist.sub( ZidgetHandle::mouseWinPos );
	float d = dist.mag2();
	d = max( 0.01f, d );
	alpha = 2000.f / d;
	if( zidgetHandle->isDragging() ) {
		alpha = 0.5f;
	}
	if( zidgetHandle->visibleAlways ) {
		alpha = 1.f;
	}
	return alpha;
}

void zidgetHandleCircleDraw( ZidgetHandle *zidgetHandle ) {
	float alpha = zidgetHandleDrawGetAlpha( zidgetHandle );
	glLineWidth( 2.f );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glColor4f( zidgetHandle->color.r, zidgetHandle->color.g, zidgetHandle->color.b, alpha * zidgetHandle->color.a );
	float radius = zidgetHandle->radius;
	glBegin( zidgetHandle->isDragging() ? GL_TRIANGLE_FAN : GL_LINE_STRIP );
		for( int j=0; j<=16; j++ ) {
			glVertex2f(
				zidgetHandle->winPos.x + radius * cos64F[j<<2],
				zidgetHandle->winPos.y + radius * sin64F[j<<2]
			);
		}
	glEnd();
}

void zidgetHandleDiamondDraw( ZidgetHandle *zidgetHandle ) {
	float alpha = zidgetHandleDrawGetAlpha( zidgetHandle );
	glLineWidth( 2.f );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glColor4f( zidgetHandle->color.r, zidgetHandle->color.g, zidgetHandle->color.b, alpha * zidgetHandle->color.a );
	float radius = zidgetHandle->radius;
	glBegin( zidgetHandle->isDragging() ? GL_TRIANGLE_FAN : GL_LINE_LOOP );
		glVertex2f( zidgetHandle->winPos.x, zidgetHandle->winPos.y - radius );
		glVertex2f( zidgetHandle->winPos.x - radius, zidgetHandle->winPos.y );
		glVertex2f( zidgetHandle->winPos.x, zidgetHandle->winPos.y + radius );
		glVertex2f( zidgetHandle->winPos.x + radius, zidgetHandle->winPos.y );
	glEnd();
}

void zidgetHandleCircleWithVertLineDraw( ZidgetHandle *zidgetHandle ) {
	float alpha = zidgetHandleDrawGetAlpha( zidgetHandle );
	glLineWidth( 2.f );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glColor4f( zidgetHandle->color.r, zidgetHandle->color.g, zidgetHandle->color.b, alpha * zidgetHandle->color.a );
	float radius = zidgetHandle->radius;
	if( ! zidgetHandle->isDragging() ) {
		glBegin( GL_LINE_STRIP );
			for( int j=0; j<=16; j++ ) {
				glVertex2f(
					zidgetHandle->winPos.x + radius * cos64F[j<<2],
					zidgetHandle->winPos.y + radius * sin64F[j<<2]
				);
			}
		glEnd();
	}

	glBegin( GL_LINES );
		glVertex2f( zidgetHandle->winPos.x, zidgetHandle->winPos.y - radius );
		glVertex2f( zidgetHandle->winPos.x, zidgetHandle->winPos.y + radius );
	glEnd();
}

void zidgetHandleCrossHairs( ZidgetHandle *zidgetHandle ) {
	float alpha = zidgetHandleDrawGetAlpha( zidgetHandle );
	glLineWidth( 1.f );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glColor4f( zidgetHandle->color.r, zidgetHandle->color.g, zidgetHandle->color.b, alpha * zidgetHandle->color.a );
	float radius = zidgetHandle->radius;
	glBegin( GL_LINES );
		glVertex2f( zidgetHandle->winPos.x, zidgetHandle->winPos.y - radius );
		glVertex2f( zidgetHandle->winPos.x, zidgetHandle->winPos.y + radius );
		glVertex2f( zidgetHandle->winPos.x - radius, zidgetHandle->winPos.y );
		glVertex2f( zidgetHandle->winPos.x + radius, zidgetHandle->winPos.y );
	glEnd();
}

void zidgetHandleHorizArrows( ZidgetHandle *zidgetHandle ) {
	float alpha = zidgetHandleDrawGetAlpha( zidgetHandle );
	glLineWidth( 2.f );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glColor4f( zidgetHandle->color.r, zidgetHandle->color.g, zidgetHandle->color.b, alpha * zidgetHandle->color.a );
	float radius = zidgetHandle->radius;
	glBegin( GL_LINES );
		glVertex2f( zidgetHandle->winPos.x, zidgetHandle->winPos.y );
		glVertex2f( zidgetHandle->winPos.x - radius, zidgetHandle->winPos.y );

		glVertex2f( zidgetHandle->winPos.x - radius, zidgetHandle->winPos.y );
		glVertex2f( zidgetHandle->winPos.x - radius * 0.7f, zidgetHandle->winPos.y - radius * 0.4f );

		glVertex2f( zidgetHandle->winPos.x - radius, zidgetHandle->winPos.y );
		glVertex2f( zidgetHandle->winPos.x - radius * 0.7f, zidgetHandle->winPos.y + radius * 0.4f );

		glVertex2f( zidgetHandle->winPos.x, zidgetHandle->winPos.y );
		glVertex2f( zidgetHandle->winPos.x + radius, zidgetHandle->winPos.y );

		glVertex2f( zidgetHandle->winPos.x + radius, zidgetHandle->winPos.y );
		glVertex2f( zidgetHandle->winPos.x + radius * 0.7f, zidgetHandle->winPos.y - radius * 0.4f );

		glVertex2f( zidgetHandle->winPos.x + radius, zidgetHandle->winPos.y );
		glVertex2f( zidgetHandle->winPos.x + radius * 0.7f, zidgetHandle->winPos.y + radius * 0.4f );
	glEnd();
}

void ZidgetHandle::init() {
	winPos.origin();
	worldPos.origin();

	radius = 8.f;
	color = FVec4( 1.f, 1.f, 1.f, 1.f );
	zidgetHandleDrawCallback = zidgetHandleCircleDraw;

	visibleAlways = 0;
	disabled = 0;
	draw = 0;
}

//-----------------------------------------------------------------------------------------------
// Zidget
//-----------------------------------------------------------------------------------------------

Zidget Zidget::zidgetRoot;
double Zidget::zidgetLastProjection[16];
int Zidget::zidgetLastViewport[4];
double Zidget::zidgetLastModel[16];

void Zidget::detach() {
	if( parent ) {
		// FIND this in the parent's list
		for( int i=0; i<parent->childrenCount; i++ ) {
			if( parent->children[i] == this ) {
				// SHIFT left
				i++;
				for( ; i<parent->childrenCount; i++ ) {
					parent->children[i-1] = parent->children[i];
				}
				parent->childrenCount--;
				break;
			}
		}
	}
}

void Zidget::attach( Zidget *newParent ) {
	if( parent ) {
		detach();
	}
	if( ! newParent ) {
		newParent = &zidgetRoot;
	}
	parent = newParent;
	parent->childrenCount++;
	parent->children = (Zidget **)realloc( parent->children, sizeof(Zidget*) * parent->childrenCount );
	parent->children[parent->childrenCount-1] = this;
}

ZidgetHandle *Zidget::handleAdd() {
	handleCount++;
	handles = (ZidgetHandle *)realloc( handles, sizeof(ZidgetHandle) * handleCount );
	handles[handleCount-1].init();
	return &handles[handleCount-1];
}

void Zidget::handleDel( int which ) {
	for( int i=which+1; i<handleCount; i++ ) {
		handles[i-1] = handles[i];
	}
	handleCount--;
}

void Zidget::handlesDisableAll() {
	for( int i=0; i<handleCount; i++ ) {
		handles[i].disabled = 1;
	}
}

void Zidget::handleDisable( int which ) {
	handles[which].disabled = 1;
}

float Zidget::zidgetRecurseClosestDist = 0.f;
ZidgetHandle *Zidget::zidgetRecurseClosestHandlePtr = 0;

void Zidget::zidgetRecurse( int action ) {
	int i;
	if( ! hidden ) {
		glPushAttrib( GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT );
		glPushMatrix();
		switch( action ) {
			case ZIDGET_RENDER_HANDLE:
				for( i=0; i<handleCount; i++ ) {
					if( ! handles[i].disabled ) {
						double winX, winY, winZ;
						gluProject( handles[i].worldPos.x, handles[i].worldPos.y, 0.f, Zidget::zidgetLastModel, Zidget::zidgetLastProjection, Zidget::zidgetLastViewport, &winX, &winY, &winZ );
						handles[i].winPos.x = (float)winX;
						handles[i].winPos.y = (float)winY;

						FVec2 dist = handles[i].winPos;
						dist.sub( ZidgetHandle::mouseWinPos );
						if(
							(ZidgetHandle::mouseDraggingHandlePtr && ZidgetHandle::mouseDraggingHandlePtr == &handles[i])
							|| (!ZidgetHandle::mouseDraggingHandlePtr && dist.mag2() < 5.58e+004f )
							|| handles[i].visibleAlways
						) {
							(*handles[i].zidgetHandleDrawCallback)( &handles[i] );
						}
					}
				}
				break;

			case ZIDGET_RENDER_ZIDGET:
				render();
				break;

			case ZIDGET_FIND_CLOSEST_HANDLE:
				for( i=0; i<handleCount; i++ ) {
					if( ! handles[i].disabled ) {
						double winX, winY, winZ;
						gluProject( handles[i].worldPos.x, handles[i].worldPos.y, 0.f, Zidget::zidgetLastModel, Zidget::zidgetLastProjection, Zidget::zidgetLastViewport, &winX, &winY, &winZ );
						handles[i].winPos.x = (float)winX;
						handles[i].winPos.y = (float)winY;

						FVec2 dist = handles[i].winPos;
						dist.sub( ZidgetHandle::mouseWinPos );
						if( dist.mag2() < zidgetRecurseClosestDist ) {
							zidgetRecurseClosestDist = dist.mag2();
							zidgetRecurseClosestHandlePtr = &handles[i];
						}
					}
				}
				break;
		}
		glPopMatrix();
		glPopAttrib();
		for( i=0; i<childrenCount; i++ ) {
			children[i]->zidgetRecurse( action );
		}
	}
}

void Zidget::zidgetRenderAll() {
	glGetDoublev( GL_PROJECTION_MATRIX, zidgetLastProjection );
	glGetIntegerv( GL_VIEWPORT, zidgetLastViewport );
	glGetDoublev( GL_MODELVIEW_MATRIX, zidgetLastModel );

	ZidgetHandle::mouseWinPos = FVec2( zMouseMsgX, zMouseMsgY );

	Zidget::zidgetRecurseClosestDist = FLT_MAX;
	Zidget::zidgetRecurseClosestHandlePtr = 0;
	Zidget::zidgetRoot.zidgetRecurse( ZIDGET_FIND_CLOSEST_HANDLE );
		// This causes all of the handlde draw flags to be cleared or set in the case that all visible is true

	if( Zidget::zidgetRecurseClosestHandlePtr ) {
		float radius2 = Zidget::zidgetRecurseClosestHandlePtr->radius;
		radius2 *= radius2;
		if( Zidget::zidgetRecurseClosestDist < radius2 ) {
			Zidget::zidgetRecurseClosestHandlePtr->draw = 1;
		}
	}

	zidgetRoot.zidgetRecurse( ZIDGET_RENDER_ZIDGET );
	glPushMatrix();
	zglPixelMatrixFirstQuadrant();
	zidgetRoot.zidgetRecurse( ZIDGET_RENDER_HANDLE );
	glPopMatrix();
}

void Zidget::setupColor( float *color ) {
	if( color[3] < 1.f ) {
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	}
	else {
		glDisable( GL_BLEND );
	}
	glColor4fv( color );
}

void Zidget::zidgetHandleMsg( ZMsg *msg ) {
	if( zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L) ) {
		// @ FIND the closest handle on any zidget
		// FOr now, we know there's only two
		// This will become a generic global search

		// UNPROJECT the mouse window coords into local
		double objX, objY, objZ;
		gluUnProject( zmsgD(x), zmsgD(y), 0.f, Zidget::zidgetLastModel, Zidget::zidgetLastProjection, Zidget::zidgetLastViewport, &objX, &objY, &objZ );
		ZidgetHandle::mouseWorldPos = FVec2( (float)objX, (float)objY );
		ZidgetHandle::mouseWinPos = FVec2( zmsgF(x), zmsgF(y) );

		Zidget::zidgetRecurseClosestDist = FLT_MAX;
		Zidget::zidgetRecurseClosestHandlePtr = 0;
		Zidget::zidgetRoot.zidgetRecurse( ZIDGET_FIND_CLOSEST_HANDLE );

		if( Zidget::zidgetRecurseClosestHandlePtr ) {
			float radius2 = Zidget::zidgetRecurseClosestHandlePtr->radius;
			radius2 *= radius2;
			if( Zidget::zidgetRecurseClosestDist < radius2 ) {
				// If there is a closest active handle then we are dragging it with a global lock
				zMouseMsgRequestExclusiveDrag( "type=Zidget_ZidgetHandleDrag" );
				ZidgetHandle::mouseDraggingHandlePtr = Zidget::zidgetRecurseClosestHandlePtr;
				zMsgUsed();
			}
		}
	}
	if( zmsgIs(type,Zidget_ZidgetHandleDrag) ) {
		zMsgUsed();

		double objX, objY, objZ;
		gluUnProject( zmsgD(x), zmsgD(y), 0.f, Zidget::zidgetLastModel, Zidget::zidgetLastProjection, Zidget::zidgetLastViewport, &objX, &objY, &objZ );
		ZidgetHandle::mouseWorldPos = FVec2( (float)objX, (float)objY );

		if( zmsgI(releaseDrag) ) {
			zMouseMsgCancelExclusiveDrag();
			ZidgetHandle::mouseDraggingHandlePtr = 0;
		}
	}
}

//-----------------------------------------------------------------------------------------------
// ZidgetVec2
//-----------------------------------------------------------------------------------------------

ZidgetVec2::ZidgetVec2() {
	dir  = FVec2( 1.f, 0.f );
	color = FVec4( 1.f, 1.f, 1.f, 1.f );
	handleAdd();
	handleAdd();
}

void ZidgetVec2::render() {
	// The local coordinate system is translated by base
	// So in local coords, the arrow is drawn from (0,0) to (dir.x,dir.y)

	localToParent.identity();
	localToParent.trans( FVec3(base.x, base.y, 0.f) );
	localToWorld = parent->localToWorld;
	localToWorld.cat( localToParent );
	worldToLocal = localToWorld;
	worldToLocal.inverse();
		// @TODO prev four lines functionalize?

	// Draw based on the local state, this is
	// assumed good because external code would
	// have bound checked it
	glMultMatrixf( localToWorld );

	setupColor( color );
	zglDrawVector( 0.f, 0.f, 0.f, dir.x, dir.y, 0.f );

	// CONVERT the local coords to world
	FVec2 baseWrld = localToWorld.mul( FVec2(0.f,0.f) );
	FVec2 tipWrld = localToWorld.mul( dir );

	// UPDATE the handles in world coords
	handles[0].set( &baseWrld );
	handles[1].set( &tipWrld );

	// At this point, baseWrld and tipWrld might
	// have changed because the handle might be dragging
	// So we reverse the conversion into local convenience space
	// (Define convenience)
	base = parent->worldToLocal.mul( baseWrld );
		// base is in parent coords since local are translated by base
	dir = worldToLocal.mul( tipWrld );

	// Now, base and dir might not meet some bounding
	// condition, but that can be changed by external code
}

//-----------------------------------------------------------------------------------------------
// ZidgetArrow2
//-----------------------------------------------------------------------------------------------

ZidgetArrow2::ZidgetArrow2() {
	tail = FVec2( 0.f, 0.f );
	head = FVec2( 1.f, 0.f );
	color = FVec4( 1.f, 1.f, 1.f, 1.f );
	handleAdd();
	handleAdd();
}

void ZidgetArrow2::render() {
	// The local coordinate system is translated by base
	// So in local coords, the arrow is drawn from (0,0) to (tail-head)
	localToParent.identity();
	localToParent.trans( FVec3(tail.x, tail.y, 0.f) );
	localToWorld = parent->localToWorld;
	localToWorld.cat( localToParent );
	worldToLocal = localToWorld;
	worldToLocal.inverse();
		// @TODO prev four lines functionalize?

	// Draw based on the local state, this is
	// assumed good because external code would
	// have bound checked it
	glMultMatrixf( localToWorld );

	setupColor( color );
	FVec2 dir = head;
	dir.sub( tail );
	zglDrawVector( 0.f, 0.f, 0.f, dir.x, dir.y, 0.f );

	// CONVERT the local coords to world
	FVec2 tailWrld = localToWorld.mul( FVec2(0.f,0.f) );
	FVec2 headWrld = localToWorld.mul( dir );

	// UPDATE the handles in world coords
	handles[0].set( &tailWrld );
	handles[1].set( &headWrld );

	// At this point, headWrld and tailWrld might
	// have changed because the handle might be dragging
	// So we reverse the conversion into local convenience space
	// (Define convenience)
	tail = parent->worldToLocal.mul( tailWrld );
		// base is in parent coords since local are translated by base
	head = parent->worldToLocal.mul( headWrld );

	// Now, base and dir might not meet some bounding
	// condition, but that can be changed by external code
}

//-----------------------------------------------------------------------------------------------
// ZidgetLine2
//-----------------------------------------------------------------------------------------------

ZidgetLine2::ZidgetLine2() {
	tail = FVec2( 0.f, 0.f );
	head = FVec2( 1.f, 0.f );
	color = FVec4( 1.f, 1.f, 1.f, 1.f );
	handleAdd();
	handleAdd();
}

void ZidgetLine2::render() {
	// The local coordinate system is translated by base
	// So in local coords, the arrow is drawn from (0,0) to (tail-head)
	localToParent.identity();
	localToParent.trans( FVec3(tail.x, tail.y, 0.f) );
	localToWorld = parent->localToWorld;
	localToWorld.cat( localToParent );
	worldToLocal = localToWorld;
	worldToLocal.inverse();
		// @TODO prev four lines functionalize?

	// Draw based on the local state, this is
	// assumed good because external code would
	// have bound checked it
	glMultMatrixf( localToWorld );

	setupColor( color );
	FVec2 dir = head;
	dir.sub( tail );
	glLineWidth( thickness );
	glBegin( GL_LINES );
		glVertex2f( 0.f, 0.f );
		glVertex2fv( dir );
	glEnd();

	// CONVERT the local coords to world
	FVec2 tailWrld = localToWorld.mul( FVec2(0.f,0.f) );
	FVec2 headWrld = localToWorld.mul( dir );

	// UPDATE the handles in world coords
	handles[0].set( &tailWrld );
	handles[1].set( &headWrld );

	// At this point, headWrld and tailWrld might
	// have changed because the handle might be dragging
	// So we reverse the conversion into local convenience space
	// (Define convenience)
	tail = parent->worldToLocal.mul( tailWrld );
		// base is in parent coords since local are translated by base
	head = parent->worldToLocal.mul( headWrld );

	// Now, base and dir might not meet some bounding
	// condition, but that can be changed by external code
}

//-----------------------------------------------------------------------------------------------
// ZidgetRectLBWH
//-----------------------------------------------------------------------------------------------

ZidgetRectLBWH::ZidgetRectLBWH() {
	bl = FVec2( 0.f, 0.f );
	w = 1.f;
	h = 1.f;
	color = FVec4( 1.f, 1.f, 1.f, 1.f );
	handleAdd();
	handleAdd();
}

void ZidgetRectLBWH::render() {
	localToParent.identity();
	localToParent.trans( FVec3(bl.x, bl.y, 0.f) );
	localToWorld = parent->localToWorld;
	localToWorld.cat( localToParent );
	worldToLocal = localToWorld;
	worldToLocal.inverse();
		// @TODO prev four lines functionalize?

	glMultMatrixf( localToWorld );

	setupColor( color );
	glBegin( GL_QUADS );
		glVertex2f( 0.f, 0.f );
		glVertex2f( w, 0.f );
		glVertex2f( w, h );
		glVertex2f( 0.f, h );
	glEnd();

	// CONVERT the local coords to world
	FVec2 blWrld = localToWorld.mul( FVec2(0.f,0.f) );
	FVec2 trWrld = localToWorld.mul( FVec2(w,h) );

	// UPDATE the handles in world coords
	handles[0].set( &blWrld );
	handles[1].set( &trWrld );

	bl = parent->worldToLocal.mul( blWrld );
	FVec2 localWH = parent->worldToLocal.mul( trWrld );
	w = localWH.x - bl.x;
	h = localWH.y - bl.y;
}

//-----------------------------------------------------------------------------------------------
// ZidgetPos2
//-----------------------------------------------------------------------------------------------

ZidgetPos2::ZidgetPos2() {
	pos = FVec2( 0.f, 0.f );
	handleAdd();
}

void ZidgetPos2::render() {
	// The local coordinate system is translated by pos
	localToParent.identity();
	localToParent.trans( FVec3(pos.x, pos.y, 0.f) );
	localToWorld = parent->localToWorld;
	localToWorld.cat( localToParent );
	worldToLocal = localToWorld;
	worldToLocal.inverse();
		// @TODO prev four lines functionalize?

	// CONVERT the local coords to world
	FVec2 posWrld = localToWorld.mul( FVec2(0.f,0.f) );

	// UPDATE the handles in world coords
	handles[0].set( &posWrld );

	// At this point, headWrld and tailWrld might
	// have changed because the handle might be dragging
	// So we reverse the conversion into local convenience space
	// (Define convenience)
	pos = parent->worldToLocal.mul( posWrld );
}

//-----------------------------------------------------------------------------------------------
// ZidgetPos2v
//-----------------------------------------------------------------------------------------------

ZidgetPos2v::ZidgetPos2v() {
	pos = 0;
	handleAdd();
}

void ZidgetPos2v::render() {
	if( !pos ) {
		return;
	}

	// The local coordinate system is translated by pos
	localToParent.identity();
	localToParent.trans( FVec3(pos[0], pos[1], 0.f) );
	localToWorld = parent->localToWorld;
	localToWorld.cat( localToParent );
	worldToLocal = localToWorld;
	worldToLocal.inverse();
		// @TODO prev four lines functionalize?

	// CONVERT the local coords to world
	FVec2 posWrld = localToWorld.mul( FVec2(0.f,0.f) );

	// UPDATE the handles in world coords
	handles[0].set( &posWrld );

	// At this point, headWrld and tailWrld might
	// have changed because the handle might be dragging
	// So we reverse the conversion into local convenience space
	// (Define convenience)
	FVec2 _pos = parent->worldToLocal.mul( posWrld );
	pos[0] = _pos.x;
	pos[1] = _pos.y;
}

//-----------------------------------------------------------------------------------------------
// ZidgetImage
//-----------------------------------------------------------------------------------------------

ZidgetImage::ZidgetImage() : Zidget() {
	tex = 0;
	srcBits.clear();
	colorScaleMin = 0.f;
	colorScaleMax = 1.f;
	lastColorScaleMin = 0.f;
	lastColorScaleMax = 0.f;
}

void ZidgetImage::clear() {
	if( tex ) {
		zbitTexCacheFree( tex );
		tex = 0;
	}
}

void ZidgetImage::dirtyBits() {
	lastColorScaleMin = FLT_MAX;
	lastColorScaleMax = FLT_MAX;
}

void ZidgetImage::setBits( ZBits *_srcBits ) {
	srcBits.clone( _srcBits );
	lastColorScaleMin = FLT_MAX;
	lastColorScaleMax = FLT_MAX;
}

void ZidgetImage::render() {
	if( colorScaleMin != lastColorScaleMin || colorScaleMax != lastColorScaleMax ) {
		lastColorScaleMin = colorScaleMin;
		lastColorScaleMax = colorScaleMax;
		ZBits dstBits;

		int w = srcBits.w();
		int h = srcBits.h();

		if( srcBits.channels == 1 ) {
			if( srcBits.channelTypeFloat ) {
				float _colorScaleMin = colorScaleMin;
				float _colorScaleMaxMinusMin = colorScaleMax - colorScaleMin;
				dstBits.createCommon( w, h, ZBITS_COMMON_RGB_8 );
				for( int y=0; y<h; y++ ) {
					float *src = srcBits.lineFloat( y );
					unsigned char *dst = dstBits.lineUchar( y );
					for( int x=0; x<w; x++ ) {
						float cf = 255.f * ( *src - _colorScaleMin ) / _colorScaleMaxMinusMin;
						cf = max( 0.f, min( 255.f, cf ) );
						int c = (int)cf;
						dst[0] = colorPaletteBlackToWhiteUB256[c][0];
						dst[1] = colorPaletteBlackToWhiteUB256[c][1];
						dst[2] = colorPaletteBlackToWhiteUB256[c][2];
						dst += 3;
						src++;
					}
				}
			}
			else if( srcBits.channelDepthInBits == 16 ) {
				float _colorScaleMin = colorScaleMin;
				float _colorScaleMaxMinusMin = (colorScaleMax - colorScaleMin);
				dstBits.createCommon( w, h, ZBITS_COMMON_RGB_8 );
				for( int y=0; y<h; y++ ) {
					unsigned short *src = srcBits.lineUshort( y );
					unsigned char *dst = dstBits.lineUchar( y );
					for( int x=0; x<w; x++ ) {
						float cf = 255.f * ( (float)*src - _colorScaleMin ) / _colorScaleMaxMinusMin;
						int c = max( 0, min( 255, (int)cf ) );
						dst[0] = colorPaletteBlackToWhiteUB256[c][0];
						dst[1] = colorPaletteBlackToWhiteUB256[c][1];
						dst[2] = colorPaletteBlackToWhiteUB256[c][2];
						dst += 3;
						src++;
					}
				}
			}
			else if( srcBits.channelDepthInBits == 8 ) {
				float _colorScaleMin = colorScaleMin;
				float _colorScaleMaxMinusMin = (colorScaleMax - colorScaleMin);
				dstBits.createCommon( w, h, ZBITS_COMMON_RGB_8 );
				for( int y=0; y<h; y++ ) {
					unsigned char *src = srcBits.lineUchar( y );
					unsigned char *dst = dstBits.lineUchar( y );
					for( int x=0; x<w; x++ ) {
						float cf = 255.f * ( (float)*src - _colorScaleMin ) / _colorScaleMaxMinusMin;
						int c = max( 0, min( 255, (int)cf ) );
						dst[0] = colorPaletteBlackToWhiteUB256[c][0];
						dst[1] = colorPaletteBlackToWhiteUB256[c][1];
						dst[2] = colorPaletteBlackToWhiteUB256[c][2];
						dst += 3;
						src++;
					}
				}
			}
		}
		else if( srcBits.channels == 3 ) {
			dstBits = srcBits;
			dstBits.owner = 0;
		}
		else {
			// Unknown channel type
			assert( 0 );
		}

		if( tex ) {
			zbitTexCacheFree( tex );
			tex = 0;
		}
		tex = zbitTexCacheLoad( &dstBits, 1 );
			// Invert
	}

	// The local coordinate system is translated by pos
	localToParent.identity();
	localToParent.trans( FVec3(pos.x, pos.y, 0.f) );
	localToWorld = parent->localToWorld;
	localToWorld.cat( localToParent );
	worldToLocal = localToWorld;
	worldToLocal.inverse();
		// @TODO prev four lines functionalize?

	glPushMatrix();
	glColor4f( 1.f, 1.f, 1.f, 1.f );
	FVec2 posWrld = localToWorld.mul( FVec2(0.f,0.f) );
	glTranslatef( posWrld.x, posWrld.y, 0.f );
	if( tex ) {
		zbitTexCacheRender( tex );
	}
	glPopMatrix();
}

//-----------------------------------------------------------------------------------------------
// ZidgetHistogram
//-----------------------------------------------------------------------------------------------

ZidgetHistogram::ZidgetHistogram() : Zidget() {
	histogramCount = 0;
	histogram = 0;
	logScale = 0;
	xScale = 1.f;
	yScale = 1.f;
	ZidgetHandle *h0 = handleAdd();
	h0->zidgetHandleDrawCallback = zidgetHandleCircleWithVertLineDraw;
	h0->color = FVec4( 0.1f, 0.1f, 0.8f, 1.f );
	h0->visibleAlways = 1;
	ZidgetHandle *h1 = handleAdd();
	h1->zidgetHandleDrawCallback = zidgetHandleCircleWithVertLineDraw;
	h1->color = FVec4( 0.8f, 0.1f, 0.1f, 1.f );
	h1->visibleAlways = 1;
	histScale0 = 0.f;
	histScale1 = 1.f;
}

void ZidgetHistogram::render() {
	// The local coordinate system is translated by pos
	localToParent.identity();
	localToParent.trans( FVec3(pos.x, pos.y, 0.f) );
	localToParent.cat( scale3D( FVec3(xScale, yScale, 1.f) ) );
	localToWorld = parent->localToWorld;
	localToWorld.cat( localToParent );
	worldToLocal = localToWorld;
	worldToLocal.inverse();
		// @TODO prev four lines functionalize?

	FVec2 histScale0F2 = localToWorld.mul( FVec2( histScale0 * xScale, 0.f ) );
//	FVec2 histScale0F2a = localToWorld.mul( FVec2( histScale0 * xScale, 0.f ) );
	FVec2 histScale1F2 = localToWorld.mul( FVec2( histScale1 * xScale, 0.f ) );
//	FVec2 histScale1F2a = localToWorld.mul( FVec2( histScale1 * yScale, 0.f ) );
	handles[0].set( &histScale0F2 );
	handles[1].set( &histScale1F2 );
	histScale0 = worldToLocal.mul( histScale0F2 ).x / xScale;
	histScale1 = worldToLocal.mul( histScale1F2 ).x / xScale;

	glPushMatrix();
	glColor4fv( color );
	FVec2 posWrld = localToWorld.mul( FVec2(0.f,0.f) );
	glTranslatef( posWrld.x, posWrld.y, 0.f );
	glScalef( xScale, yScale, 1.f );

	glBegin( GL_QUADS );
	float lastX = 0.f;
	float x = 1.f;
	for( int i=0; i<histogramCount; i++ ) {
		if( histogram[i] >= 0.f ) {
			float y = (logScale ? logf( 1.f + histogram[i] ) : histogram[i]);
			glVertex2f( lastX, 0.f );
			glVertex2f( x, 0.f );
			glVertex2f( x, y );
			glVertex2f( lastX, y );
		}
		lastX = x;
		x += 1.f;
	}
	glEnd();

	glPopMatrix();
}

void ZidgetHistogram::clear() {
	if( histogram ) {
		free( histogram );
		histogram = 0;
	}
	histogramCount = 0;
}

void ZidgetHistogram::setHist( float *_histogram, int _count ) {
	clear();
	histogram = (float *)malloc( sizeof(float) * _count );
	memcpy( histogram, _histogram, sizeof(float) * _count );
	histogramCount = _count;
}


//-----------------------------------------------------------------------------------------------
// ZidgetMovie
//-----------------------------------------------------------------------------------------------

ZidgetMovie::ZidgetMovie() : ZidgetImage() {
	viewFrame = 0;
	frameCount = 0;
	frames = 0;
	lastViewFrame = -1;
}

void ZidgetMovie::clear() {
	if( frames ) {
		for( int i=0; i<frameCount; i++ ) {
			frames[i].clear();
		}
		delete[] frames;
		frames = 0;
	}
	frameCount = 0;
	viewFrame = 0;
	ZidgetImage::clear();
}

void ZidgetMovie::setFrameCount( int _frameCount ) {
	clear();
	frameCount = _frameCount;
	frames = new ZBits[ frameCount ];
	viewFrame = 0;
}

void ZidgetMovie::setFrameBits( int frame, ZBits *_srcBits ) {
	assert( frame >= 0 && frame < frameCount );
	frames[frame].clone( _srcBits );
	lastViewFrame = -1;
}

void ZidgetMovie::render() {
	viewFrame = min( frameCount-1, viewFrame );
	if( viewFrame != lastViewFrame ) {
		lastViewFrame = viewFrame;
		ZidgetImage::setBits( &frames[viewFrame] );
	}

	ZidgetImage::render();
}

//-----------------------------------------------------------------------------------------------
// ZidgetCircle
//-----------------------------------------------------------------------------------------------

ZidgetCircle::ZidgetCircle() {
	radius = 1.f;
	color = FVec4( 1.f, 1.f, 1.f, 1.f );
	handleAdd();
	handleAdd();
}

void ZidgetCircle::render() {
	// The local coordinate system is translated by pos
	localToParent.identity();
	localToParent.trans( FVec3(pos.x, pos.y, 0.f) );
	localToWorld = parent->localToWorld;
	localToWorld.cat( localToParent );
	worldToLocal = localToWorld;
	worldToLocal.inverse();
		// @TODO prev four lines functionalize?

	glMultMatrixf( localToWorld );

	// @TODO: Do a projection to figure out how precise we need to be on the tesselation
	setupColor( color );
	glBegin( solid ? GL_TRIANGLE_FAN : GL_LINE_LOOP );
	for( int i=0; i<=64; i++ ) {
		glVertex2f( radius * cos64F[i], radius * sin64F[i] );
	}
	glEnd();

	// CONVERT the local coords to world
	FVec2 centerWrld = localToWorld.mul( FVec2(0.f,0.f) );
	FVec2 radiusHandlePos( radius, 0.f );
	FVec2 radiusWrld = localToWorld.mul( radiusHandlePos );

	// UPDATE the handles in world coords
	handles[0].set( &centerWrld );
	handles[1].set( &radiusWrld );

	// At this point, baseWrld and tipWrld might
	// have changed because the handle might be dragging
	// So we reverse the conversion into local convenience space
	// (Define convenience)
	pos = parent->worldToLocal.mul( centerWrld );
	radiusHandlePos = worldToLocal.mul( radiusWrld );
	radius = radiusHandlePos.mag();
}

//-----------------------------------------------------------------------------------------------
// ZidgetPixelCircle
//-----------------------------------------------------------------------------------------------

ZidgetPixelCircle::ZidgetPixelCircle() {
	radius = 1.f;
	color = FVec4( 1.f, 1.f, 1.f, 1.f );
	handleAdd();
	handleAdd();
}

void ZidgetPixelCircle::render() {
	// The local coordinate system is translated by pos
	localToParent.identity();
	localToParent.trans( FVec3(pos.x, pos.y, 0.f) );
	localToWorld = parent->localToWorld;
	localToWorld.cat( localToParent );
	worldToLocal = localToWorld;
	worldToLocal.inverse();
		// @TODO prev four lines functionalize?

	glMultMatrixf( localToWorld );

	// @TODO: Do a projection to figure out how precise we need to be on the tesselation
	setupColor( color );
	glBegin( GL_QUADS );

	int radiusI = (int)radius;
	int r2 = radiusI * radiusI;
	for( int y=-radiusI; y<=radiusI; y++ ) {
		float yf = (float)y;
		for( int x=-radiusI; x<=radiusI; x++ ) {
			float xf = (float)x;
			int d2 = x*x + y*y - 1;
			if( d2 <= r2 ) {
				glVertex2f( xf + 0.f, yf + 0.f );
				glVertex2f( xf + 1.f, yf + 0.f );
				glVertex2f( xf + 1.f, yf + 1.f );
				glVertex2f( xf + 0.f, yf + 1.f );
			}
		}
	}
	glEnd();

	// CONVERT the local coords to world
	FVec2 centerWrld = localToWorld.mul( FVec2(0.f,0.f) );
	FVec2 radiusHandlePos( radius, 0.f );
	FVec2 radiusWrld = localToWorld.mul( radiusHandlePos );

	// UPDATE the handles in world coords
	handles[0].set( &centerWrld );
	handles[1].set( &radiusWrld );

	// At this point, baseWrld and tipWrld might
	// have changed because the handle might be dragging
	// So we reverse the conversion into local convenience space
	// (Define convenience)
	pos = parent->worldToLocal.mul( centerWrld );
	pos.x = floorf(pos.x + 0.5f);
	pos.y = floorf(pos.y + 0.5f);
	radiusHandlePos = worldToLocal.mul( radiusWrld );
	radius = radiusHandlePos.mag();
}

//-----------------------------------------------------------------------------------------------
// ZidgetPlot
//-----------------------------------------------------------------------------------------------

ZidgetPlot::ZidgetPlot() {
	w = 1.f;
	h = 1.f;
	x0 = 0.f;
	y0 = 0.f;
	x1 = 1.f;
	y1 = 1.f;
	scaleX = 1.f;
	scaleY = 1.f;
	transX = 0.f;
	transY = 0.f;
	rangedCallback = 0;
	unrangedCallback = 0;
	panelColor = FVec4( 1.f, 1.f, 1.f, 1.f );
	handleAdd();
	handleAdd();
	handles[ZidgetPlot_scale].zidgetHandleDrawCallback = zidgetHandleDiamondDraw;
//	handles[ZidgetPlot_scale].visibleAlways = 1;
	handleAdd();
	handles[ZidgetPlot_trans].zidgetHandleDrawCallback = zidgetHandleDiamondDraw;
//	handles[ZidgetPlot_trans].visibleAlways = 1;
}

void ZidgetPlot::render() {
	// The local coordinate system is translated by pos
	localToParent.identity();
	localToParent.trans( FVec3(pos.x, pos.y, 0.f) );
	localToParent.cat( scale3D( FVec3(w, h, 1.f) ) );
	localToWorld = parent->localToWorld;
	localToWorld.cat( localToParent );
	worldToLocal = localToWorld;
	worldToLocal.inverse();
		// @TODO prev four lines functionalize?

	glMultMatrixf( localToWorld );

	setupColor( panelColor );
	glBegin( GL_QUADS );
		glVertex2f( 0.f, 0.f );
		glVertex2f( 1.f, 0.f );
		glVertex2f( 1.f, 1.f );
		glVertex2f( 0.f, 1.f );
	glEnd();

	glPushAttrib( GL_SCISSOR_BIT | GL_ENABLE_BIT );
	glEnable( GL_SCISSOR_TEST );

	double projection[16];
	double model[16];
	int viewport[4];
	glGetDoublev( GL_PROJECTION_MATRIX, projection );
	glGetDoublev( GL_MODELVIEW_MATRIX, model );
	glGetIntegerv( GL_VIEWPORT, viewport );
	double winX0, winY0, winZ0;
	double winX1, winY1, winZ1;

	gluProject( 0.f, 0.f, 0.f, model, projection, viewport, &winX0, &winY0, &winZ0 );
	gluProject( 1.f, 1.f, 0.f, model, projection, viewport, &winX1, &winY1, &winZ1 );

	glScissor( viewport[0] + (int)winX0-1, viewport[1] + (int)winY0-1, (int)ceil(winX1-winX0)+1, (int)ceil(winY1-winY0)+1 );

	float _x0 = x0 / scaleX - transX;
	float _x1 = x1 / scaleX - transX;
	float _y0 = y0 / scaleY - transY;
	float _y1 = y1 / scaleY - transY;
	float plotPerPixelX = 0.f;
	float plotPerPixelY = 0.f;
	if( (float)winX1-(float)winX0 > 0.f && (float)winY1-(float)winY0 > 0.f ) {
		plotPerPixelX = (_x1-_x0) / ((float)winX1-(float)winX0);
		plotPerPixelY = (_y1-_y0) / ((float)winY1-(float)winY0);
	}

	glPushMatrix();
	glScalef( scaleX, scaleY, 1.f );
	glTranslatef( transX, transY, 0.f );
	plotGridlines( _x0, _y0, _x1, _y1, plotPerPixelX, plotPerPixelY );

	if( rangedCallback ) {
		(*rangedCallback)( this, _x0, _y0, _x1, _y1, plotPerPixelX, plotPerPixelY );
	}
	else if( unrangedCallback ) {
		(*unrangedCallback)( this );
	}

	glPopMatrix();
	glPopAttrib();

	// CONVERT the local coords to world
	FVec2 posWrld = localToWorld.mul( FVec2(0.f,0.f) );

	// UPDATE the handles in world coords
	FVec2 sizeWrld = localToWorld.mul( FVec2(1.f,1.f) );
	handles[ZidgetPlot_size].set( &sizeWrld );
	w = parent->worldToLocal.mul( sizeWrld ).x - posWrld.x;
	h = parent->worldToLocal.mul( sizeWrld ).y - posWrld.y;

	FVec2 scaleWrld = localToWorld.mul( FVec2(0.f,0.5f) );
	handles[ZidgetPlot_scale].set( &scaleWrld );
	FVec2 scaleLocal = worldToLocal.mul( scaleWrld );
	scaleLocal.sub( FVec2(0.f,0.5f) );
	scaleX *= 1.f + (scaleLocal.x / 100.f);
	scaleY *= 1.f + (scaleLocal.y / 100.f);

	FVec2 transWrld = localToWorld.mul( FVec2(0.5f,0.f) );
	handles[ZidgetPlot_trans].set( &transWrld );
	FVec2 transLocal = worldToLocal.mul( transWrld );
	transLocal.sub( FVec2(0.5f,0.f) );
	transX -= (transLocal.x / (190.f * scaleX) );
	transY -= (transLocal.y / (190.f * scaleY) );
}


//-----------------------------------------------------------------------------------------------
// ZidgetText
//-----------------------------------------------------------------------------------------------

ZidgetText::ZidgetText() {
	text = "";
	fontid = 0;
	pos.origin();
	w = 100.f;
	h = 100.f;
	color = FVec4( 1.f, 1.f, 1.f, 1.f );
	owner = 0;
}

ZidgetText::~ZidgetText() {
	if( owner && text ) {
		free( text );
	}
}

void ZidgetText::setText( char *t ) {
	if( owner && text ) {
		free( text );
	}
	owner = 1;
	text = strdup( t );
}

void ZidgetText::render() {
	// The local coordinate system is translated by base
	// So in local coords, the arrow is drawn from (0,0) to (tail-head)
	localToParent.identity();
	localToParent.trans( FVec3(pos.x, pos.y, 0.f) );
	localToWorld = parent->localToWorld;
	localToWorld.cat( localToParent );
	worldToLocal = localToWorld;
	worldToLocal.inverse();
		// @TODO prev four lines functionalize?

	// Draw based on the local state, this is
	// assumed good because external code would
	// have bound checked it
	glMultMatrixf( localToWorld );

	zglFontPrintWordWrap( text, pos.x, pos.y, w, h, fontid, 0 );
}

