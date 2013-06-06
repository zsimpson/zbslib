// @ZBS {
//		+DESCRIPTION {
//			Implements manipulators
//		}
//		*PORTABILITY win32
//		*REQUIRED_FILES manip.cpp manip.h
//		*VERSION 1.0
//		+HISTORY {
//			I'm not very happy with this design. Cumbersome to use
//		}
//		+TODO {
//			Implement the world to screen conversions
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
#include "math.h"
// MODULE includes:
#include "manip.h"
// ZBSLIB includes:
#include "zmousemsg.h"
#include "ztime.h"
#include "string.h"
#include "assert.h"
#include "zmathtools.h"
#include "zrgbhsv.h"


//////////////////////////////////////////////////////////////////////////
// Manipulator

int manipViewport[4];
float manipModelMatrix[16], manipProjMatrix[16];

Manipulator *Manipulator::head = 0;
Manipulator *Manipulator::exclusive = 0;

Manipulator::Manipulator() {
	active = 0;
	next = head;
	head = this;
}

Manipulator::~Manipulator() {
	Manipulator *last = 0;
	for( Manipulator *m = head; m; m=m->next ) {
		if( m == this ) {
			if( last ) {
				last->next = m->next;
			}
			else {
				assert( m == head );
				head = next;
			}
			break;
		}
		last = m;
	}
	if( exclusive == this ) {
		exclusive = 0;
	}
}

int Manipulator::lockExclusive() {
	if( !exclusive ) {
		exclusive = this;
		return 1;
	}
	return 0;
}

int Manipulator::unlockExclusive() {
	if( exclusive == this ) {
		exclusive = 0;
		return 1;
	}
	return 0;
}

FVec2 Manipulator::scrnToWrld( FVec2 scrn ) {
	DVec3 p0;
	double manipModelMatrixD[16];
	double manipProjMatrixD[16];
	for( int i=0; i<16; i++ ) {
		manipModelMatrixD[i] = manipModelMatrix[i];
		manipProjMatrixD[i] = manipProjMatrix[i];
	}
	gluUnProject( scrn.x, scrn.y, 0.0f, manipModelMatrixD, manipProjMatrixD, manipViewport, &p0.x, &p0.y, &p0.z );
	return FVec2( (float)p0.x, (float)p0.y );
}

FVec2 Manipulator::wrldToScrn( FVec2 wrld ) {
	DVec3 p0;
	double manipModelMatrixD[16];
	double manipProjMatrixD[16];
	for( int i=0; i<16; i++ ) {
		manipModelMatrixD[i] = manipModelMatrix[i];
		manipProjMatrixD[i] = manipProjMatrix[i];
	}
	gluProject( wrld.x, wrld.y, 0.0, manipModelMatrixD, manipProjMatrixD, manipViewport, &p0.x, &p0.y, &p0.z );
	return FVec2( (float)p0.x, (float)p0.y );
}

void manipGrabMatricies() {
	glGetFloatv( GL_MODELVIEW_MATRIX, manipModelMatrix );
	glGetFloatv( GL_PROJECTION_MATRIX, manipProjMatrix );
	glGetIntegerv( GL_VIEWPORT, manipViewport );
}

Manipulator *manipClosest( FVec2 posScrn ) {
	float closestDist = 10e10f;
	Manipulator *closestM = 0;
	for( Manipulator *m = Manipulator::head; m; m=m->next ) {
		if( m->active ) {
			float dist = m->distTo( posScrn );
			if( dist < closestDist ) {
				closestDist = dist;
				closestM = m;
			}
		}
	}
	return closestM;
}

void manipDispatch( ZMsg *msg ) {
	if( zMsgIsUsed() ) {
		return;
	}

	if( Manipulator::exclusive ) {
		Manipulator::exclusive->handleMsg( msg );
	}
	else {
		FVec2 mouseScrn( zMouseMsgX, zMouseMsgY );
		Manipulator *closest = manipClosest( mouseScrn );
		if( closest ) {
			closest->handleMsg( msg );
		}
	}
}

void manipRenderAll() {
	if( Manipulator::exclusive ) {
		Manipulator::exclusive->render( 1 );
	}
	else {
		FVec2 mouseScrn( zMouseMsgX, zMouseMsgY );
		Manipulator *closest = manipClosest( mouseScrn );
		for( Manipulator *m = Manipulator::head; m; m=m->next ) {
			m->render( m == closest );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// FVec2Manipulator

FVec2Manipulator::FVec2Manipulator() {
	underControlWrld = 0;
	baseWrld = &myBaseWrld;
	myBaseWrld.origin();
	state = sNotAnimating;
	dragging = 0;
	animStart = 0;
	color[0] = 1.f;
	color[1] = 0.f;
	color[2] = 0.f;
	color[3] = 0.f;
	radius = 5.f;
}

float FVec2Manipulator::distTo( FVec2 posScrn ) {
	if( underControlWrld ) {
		FVec2 wrld = FVec2( underControlWrld->x, underControlWrld->y );
		wrld.add( *baseWrld );
		FVec2 underControlScrn = wrldToScrn( wrld );

		FVec2 distScrn = underControlScrn;
		distScrn.sub( posScrn );
		return distScrn.mag();
	}
	return 10e10f;
}

void FVec2Manipulator::render( int closest ) {
	if( !active || !underControlWrld ) {
		return;
	}

	FVec2 mouseScrn( zMouseMsgX, zMouseMsgY );
	FVec2 mouseWrld = scrnToWrld( mouseScrn );

	FVec2 wrld = FVec2( underControlWrld->x, underControlWrld->y );
	wrld.add( *baseWrld );
	FVec2 underControlScrn = wrldToScrn( wrld );

	FVec2 distScrn = underControlScrn;
	distScrn.sub( mouseScrn );
	float dist = distScrn.mag();

	int drawSpot = 0;
	float elapsed = float(zTime - animStart);
	float rad = radius;

	switch( state ) {
		case sNotAnimating: {
			// If you come close, begin a little animation
 			if( closest && exclusive != this && dist < 20.0 ) {
				animStart = zTime;
				state = sAnimating;
			}
			break;
		}

		case sAnimating: {
			// If they are close enough when they click, start to drag
			if( !closest || dist > 40.0 ) {
				state = sNotAnimating;
			}
			else if( dragging ) {
				state = sDragging;
			}

			// Keep up the animation, if they move too far away kill it
			if( elapsed < 0.5 ) {
				rad = (1.0f - elapsed*2.0f) * radius * 3.0f + radius;
			}
			drawSpot = 1;

			break;
		}

		case sDragging: {
			// Move the vector under control around
			if( dragging ) {
				lockExclusive();
				mouseWrld.sub( *baseWrld );
				*underControlWrld = mouseWrld;
			}
			else {
				unlockExclusive();
				state = sAnimating;
			}

			drawSpot = 1;
			elapsed = 0.0;
			break;
		}
	}


	if( drawSpot ) {
		glPushAttrib( GL_COLOR_BUFFER_BIT );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glEnable( GL_BLEND );

		float a = (float)( cos( elapsed * 2.f * PI2 ) / 2.0 + 0.5 );
		if( drawSpot ) {
			glColor4f( color[0], color[1], color[2], a );
			glBegin( GL_TRIANGLE_FAN );
			float t = 0.f, tStep = PI2F / 16.0f;
			for( int i=0; i<=16; i++, t += tStep ) {
				glVertex2d( baseWrld->x + underControlWrld->x + cos(t) * rad, baseWrld->y + underControlWrld->y + sin(t) * rad );
			}
			glEnd();
		}

		glPopAttrib();
	}
}

void FVec2Manipulator::handleMsg( ZMsg *msg ) {
	if( zmsgIs(type,ZUIMouseClickOn) ) {
		int a = 1;
	}
	if( state == sAnimating && zmsgIs(type,ZUIMouseClickOn) && zmsgIs(dir,D) && zmsgIs(which,L) ) {
		dragging = zMouseMsgRequestExclusiveDrag( "type=FVec2Manipulator_MouseDrag" );
		zMsgUsed();
	}
	if( zmsgIs(type,FVec2Manipulator_MouseDrag) ) {
		if( zmsgI(releaseDrag) ) {
			zMouseMsgCancelExclusiveDrag();
			dragging = 0;
			zMsgUsed();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Visual

Visual *Visual::head = 0;

Visual::Visual() {
	active = 0;
	next = head;
	head = this;
}

Visual::~Visual() {
	Visual *last = 0;
	for( Visual *m = head; m; m=m->next ) {
		if( m == this ) {
			if( last ) {
				last->next = m->next;
			}
			else {
				assert( m == head );
				head = next;
			}
			break;
		}
		last = m;
	}
}

void visualRenderAll() {
	for( Visual *m = Visual::head; m; m=m->next ) {
		if( m->active ) {
			m->render();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// FVec2Visual

FVec2Visual::FVec2Visual() {
	baseWrld = &myBaseWrld;
	dirWrld = &myDirWrld;

	myBaseWrld.origin();
	myDirWrld.origin();

	color[0] = 1.f;
	color[1] = 0.f;
	color[2] = 0.f;
	color[3] = 0.f;
	arrowAngleDeg = 30.f;
	dirLenMultiplierWrld = 1.f;
	arrowSizeWrld = 10.f;
	// @TODO: Convert to screen
}

void FVec2Visual::render() {
	if( !active ) return;

	glPushAttrib( GL_COLOR_BUFFER_BIT | GL_LINE_BIT );
	glColor4fv( color );
	glLineWidth( thickness );
	glBegin(GL_LINES);
	FVec3 origin( baseWrld->x, baseWrld->y, 0.0 );
	FVec3 direction( dirWrld->x, dirWrld->y, 0.0 );
	glVertex3fv( origin );
	FVec3 dirLonger = direction;
	dirLonger.mul( dirLenMultiplierWrld );
	origin.add( dirLonger );
	glVertex3fv(origin);

	if( arrowSizeWrld != 0.0 ) {
		// origin is now the end of the normal.
		FMat3 rotMat0 = rotate3D_3x3( FVec3::ZAxis, PI2F * (arrowAngleDeg/360.0f) );

		direction.normalize();
		direction.mul( -arrowSizeWrld );
		FVec3 arrowOff0 = rotMat0.mul(direction);
		rotMat0.transpose();
		FVec3 arrowOff1 = rotMat0.mul(direction);

		arrowOff0.add(origin);
		arrowOff1.add(origin);

		glVertex3fv(origin);
		glVertex3fv(arrowOff0);

		glVertex3fv(origin);
		glVertex3fv(arrowOff1);
	}
	glEnd();
	glPopAttrib();
}

//////////////////////////////////////////////////////////////////////////
// DCircleVisual

FCircleVisual::FCircleVisual() {
	centerWrld = &myCenterWrld;
	radiusWrld = &myRadiusWrld;

	// If the visual owns the memory, the above pointers
	// point to the following members
	myCenterWrld.origin();
	myRadiusWrld.origin();

	color[0] = 1.f;
	color[1] = 0.f;
	color[2] = 0.f;
	color[3] = 0.f;

	thickness = 1.f;
	divs = 16.f;
}

void FCircleVisual::render() {
	if( !active ) return;

	glPushAttrib( GL_COLOR_BUFFER_BIT | GL_LINE_BIT );
	glColor4fv( color );

	int divsI = (int)divs;
	float r = radiusWrld->mag();
	float t = 0.0f, tStep = PI2F / float(divsI);
	glBegin( GL_LINE_STRIP );
	for( int i=0; i<=divsI; i++, t+=tStep ) {
		glVertex2f( centerWrld->x + r*cosf(t), centerWrld->y + r*sinf(t) );
	}
	glEnd();

	glPopAttrib();
}
