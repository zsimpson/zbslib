#ifndef MANIP_H
#define MANIP_H

#include "zmsg.h"
#include "zvec.h"

struct Manipulator {
	static Manipulator *exclusive;
	static Manipulator *head;
	Manipulator *next;
	int active;

	Manipulator();
	~Manipulator();

	int lockExclusive();
	int unlockExclusive();

	FVec2 scrnToWrld( FVec2 scrn );
	FVec2 wrldToScrn( FVec2 wrld );

	virtual float distTo( FVec2 pos )=0;
	virtual void render( int closest )=0;
	virtual void handleMsg( ZMsg *msg ) { }
};

struct FVec2Manipulator : Manipulator {
	FVec2 *underControlWrld;
	FVec2 *baseWrld;
	// The above pointer defaults to the following internal vector
	FVec2 myBaseWrld;

	enum {
		sNotAnimating,
		sAnimating,
		sDragging,
	} state;

	int dragging;
	double animStart;
	float color[4];
	float radius;

	FVec2Manipulator();	
	virtual void render( int closest );
	virtual void handleMsg( ZMsg *msg );
	virtual float distTo( FVec2 pos );
};

void manipDispatch( ZMsg *msg );
void manipRenderAll();
Manipulator *manipClosest( FVec2 posScrn );
void manipGrabMatricies();

//////////////////////////////////////////////////////////////////////////

void visualRenderAll();

struct Visual {
	static Visual *head;
	Visual *next;
	int active;

	Visual();
	~Visual();

	virtual void render() { }
};

struct FVec2Visual : Visual {
	// Pointers to the base and direction
	FVec2 *baseWrld;
	FVec2 *dirWrld;

	// If the visual owns the memory, the above pointers
	// point to the following members
	FVec2 myBaseWrld;
	FVec2 myDirWrld;

	float color[4];
	float arrowAngleDeg;
	float dirLenMultiplierWrld;
	float arrowSizeWrld;
	float thickness;

	FVec2Visual();
	virtual void render();
};

struct FCircleVisual : Visual {
	// Pointers to the base and direction
	FVec2 *centerWrld;
	FVec2 *radiusWrld;

	// If the visual owns the memory, the above pointers
	// point to the following members
	FVec2 myCenterWrld;
	FVec2 myRadiusWrld;

	float color[4];
	float thickness;
	float divs;

	FCircleVisual();
	virtual void render();
};


#endif