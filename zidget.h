// @ZBS {
//		*MODULE_OWNER_NAME zidget
// }

#ifndef ZIDGET_H
#define ZIDGET_H

// OPERATING SYSTEM specific includes:
// SDK includes:
// STDLIB includes:
// MODULE includes:
#include "zvec.h"
#include "zmsg.h"
#include "zbits.h"
// ZBSLIB includes:

//-----------------------------------------------------------------------------------------------
// ZidgetHandle
//-----------------------------------------------------------------------------------------------

typedef void ZidgetHandleDraw( struct ZidgetHandle *zidgetHandle );

struct ZidgetHandle {
	// Pos
	FVec2 winPos;
	FVec2 worldPos;

	// Flags
	int visibleAlways;
	int disabled;
	int draw;
	
	// Draw
	float radius;
	FVec4 color;
	ZidgetHandleDraw *zidgetHandleDrawCallback;

	// Statics
	static FVec2 mouseWorldPos;
	static FVec2 mouseWinPos;
	static ZidgetHandle *mouseDraggingHandlePtr;

	void init();

	int isDragging() {
		return this == mouseDraggingHandlePtr;
	}

	void set( FVec2 *externalWorldPos ) {
		// The handle always sets its position based on the external request.
		worldPos = *externalWorldPos;

		// If this handle is dragging then it passes
		// along this world dragging coordinate. Note that
		// it doesn't store this anywhere, it is up to the
		// external code to decide if this dragging coord will
		// become official or not.
		if( mouseDraggingHandlePtr == this ) {
			*externalWorldPos = mouseWorldPos;
		}
	}

	ZidgetHandle();
};

float zidgetHandleDrawGetAlpha( ZidgetHandle *zidgetHandle );
void zidgetHandleCircleDraw( ZidgetHandle *zidgetHandle );
void zidgetHandleDiamondDraw( ZidgetHandle *zidgetHandle );
void zidgetHandleCircleWithVertLineDraw( ZidgetHandle *zidgetHandle );
void zidgetHandleCrossHairs( ZidgetHandle *zidgetHandle );
void zidgetHandleHorizArrows( ZidgetHandle *zidgetHandle );

//-----------------------------------------------------------------------------------------------
// Zidget
//-----------------------------------------------------------------------------------------------

struct Zidget {
	// Heirarchy
	FMat4 localToParent;
	FMat4 localToWorld;
	FMat4 worldToLocal;
	Zidget *parent;
	int childrenCount;
	Zidget **children;
	static Zidget zidgetRoot;

	// Identifier (for debugging)
	char name[32];

	// Handles
	int handleCount;
	ZidgetHandle *handles;

	// Flags
	int hidden;

	// Statics
	static double zidgetLastModel[16];
	static double zidgetLastProjection[16];
	static int zidgetLastViewport[4];
	static float zidgetRecurseClosestDist;
	static ZidgetHandle *zidgetRecurseClosestHandlePtr;

	void setName(char *_name) { strcpy( name, _name ); }

	void detach();
		// Disassociates from parent

	void attach( Zidget *_parent = 0 );
		// Calls detach first

	ZidgetHandle *handleAdd();
		// Add a handle

	void handleDel( int which );
		// Remove a handle (shifts left)

	void handlesDisableAll();
	void handleDisable( int which );

	void zidgetRecurse( int action );
		// Recursively renders with the zidget or its handles
		#define ZIDGET_RENDER_HANDLE (0)
		#define ZIDGET_RENDER_ZIDGET (1)
		#define ZIDGET_FIND_CLOSEST_HANDLE (2)

	static void zidgetRenderAll();
	static void zidgetHandleMsg( ZMsg *msg );

	void setupColor( float *color );

	virtual void render() { }

	Zidget() {
		parent = 0;
		childrenCount = 0;
		children = 0;
		name[0] = '\0';
		handleCount = 0;
		handles = 0;
		hidden = 0;
	}
};

//-----------------------------------------------------------------------------------------------
// ZidgetVec2
//-----------------------------------------------------------------------------------------------

struct ZidgetVec2 : Zidget {
	FVec2 base;
	FVec2 dir;
	FVec4 color;
	#define ZidgetVec2_base (0)
	#define ZidgetVec2_dir (1)

	ZidgetVec2();
	virtual void render();
};

//-----------------------------------------------------------------------------------------------
// ZidgetArrow2
//-----------------------------------------------------------------------------------------------

struct ZidgetArrow2 : Zidget {
	// ZidgetArrow2 unlike a ZidgetVec2 maintains both the tail and the head
	// in the parent space.  It sets the local space to be equal to the tail
	FVec2 tail;
	FVec2 head;
	FVec4 color;
	#define ZidgetArrow2_tail (0)
	#define ZidgetArrow2_head (1)

	ZidgetArrow2();
	virtual void render();
};

//-----------------------------------------------------------------------------------------------
// ZidgetLine2
//-----------------------------------------------------------------------------------------------

struct ZidgetLine2 : Zidget {
	FVec2 tail;
	FVec2 head;
	float thickness;
	FVec4 color;
	#define ZidgetLine2_tail (0)
	#define ZidgetLine2_head (1)

	ZidgetLine2();
	virtual void render();
};

//-----------------------------------------------------------------------------------------------
// ZidgetRect2
//-----------------------------------------------------------------------------------------------

struct ZidgetRectLBWH : Zidget {
	FVec2 bl;
	float w;
	float h;
	FVec4 color;
	#define ZidgetLine2_bl (0)
	#define ZidgetLine2_wh (1)

	ZidgetRectLBWH();
	virtual void render();
};

//-----------------------------------------------------------------------------------------------
// ZidgetPos2
//-----------------------------------------------------------------------------------------------

struct ZidgetPos2 : Zidget {
	// A ZidgetPos2 does not draw anything
	// It changes its local space equal to the pos
	FVec2 pos;
	#define ZidgetPos2_pos (0)

	ZidgetPos2();
	virtual void render();
};

//-----------------------------------------------------------------------------------------------
// ZidgetPos2v
//-----------------------------------------------------------------------------------------------

struct ZidgetPos2v : Zidget {
	// Same as pos but uses a pointer to the value
	float *pos;
	#define ZidgetPos2v_pos (0)

	ZidgetPos2v();
	virtual void render();
};

//-----------------------------------------------------------------------------------------------
// ZidgetImage
//-----------------------------------------------------------------------------------------------

struct ZidgetImage : Zidget {
	FVec2 pos;
	int *tex;
	ZBits srcBits;
	float colorScaleMin;
	float colorScaleMax;

	float lastColorScaleMin;
	float lastColorScaleMax;
		// These are compared to the current
		// values and the display image is
		// updated if they have changed.

	ZidgetImage();
	void dirtyBits();
		// Forces a reload of the bits to the texture cache
	void setBits( ZBits *zbits );
	void clear();
	virtual void render();
};

//-----------------------------------------------------------------------------------------------
// ZidgetHistogram
//-----------------------------------------------------------------------------------------------

struct ZidgetHistogram : Zidget {
	FVec2 pos;
	int histogramCount;
	float *histogram;
	FVec4 color;
	int logScale;
	float xScale;
	float yScale;
	float histScale0;
	float histScale1;
	#define ZidgetHistogram_adjustMin (0)
	#define ZidgetHistogram_adjustMax (1)

	ZidgetHistogram();
	virtual void render();
	void clear();
	void setHist( float *_histogram, int _count );
		// This class becomes the owner, it will delete on any setHist or destruct
};

//-----------------------------------------------------------------------------------------------
// ZidgetMovie
//-----------------------------------------------------------------------------------------------

struct ZidgetMovie : ZidgetImage {
	int viewFrame;
	int frameCount;
	int lastViewFrame;
	ZBits *frames;

	ZidgetMovie();
	void setFrameCount( int _frameCount );
	void setFrameBits( int frame, ZBits *zbits );
	void clear();
	virtual void render();
};

//-----------------------------------------------------------------------------------------------
// ZidgetCircle
//-----------------------------------------------------------------------------------------------

struct ZidgetCircle : Zidget {
	FVec2 pos;
	float radius;
	int solid;
	FVec4 color;
	#define ZidgetCircle_pos (0)
	#define ZidgetCircle_radius (1)

	ZidgetCircle();
	virtual void render();
};

//-----------------------------------------------------------------------------------------------
// ZidgetPixelCircle
//-----------------------------------------------------------------------------------------------

struct ZidgetPixelCircle : Zidget {
	// This is used when you want to overlay a circle on am image
	// and see exactly which pixels will be studied

	FVec2 pos;
	float radius;
	FVec4 color;
	#define ZidgetPixelCircle_pos (0)
	#define ZidgetPixelCircle_radius (1)

	ZidgetPixelCircle();
	virtual void render();
};


//-----------------------------------------------------------------------------------------------
// ZidgetPlot
//-----------------------------------------------------------------------------------------------

// @TODO: This will be like the zuiplot.  It will have a concept
// of a scale and a translation in the plot space
// It will use a call back for data

typedef void ZidgetPlotRangedCallback( void *self, float x0, float y0, float x1, float y1, float plotPerPixelX, float plotPerPixelY );
typedef void ZidgetPlotUnrangedCallback( void *self );

struct ZidgetPlot : Zidget {
	FVec2 pos;
	FVec4 panelColor;

	float w;
	float h;

	float x0, y0, x1, y1;
	float xd, yd;
	float scaleX, scaleY;
	float transX, transY;

	ZidgetPlotRangedCallback *rangedCallback;
	ZidgetPlotUnrangedCallback *unrangedCallback;

	#define ZidgetPlot_size   (0)
	#define ZidgetPlot_scale  (1)
	#define ZidgetPlot_trans  (2)

	ZidgetPlot();
	virtual void render();
};

//-----------------------------------------------------------------------------------------------
// ZidgetText
//-----------------------------------------------------------------------------------------------

struct ZidgetText : Zidget {
	char *text;
	char *fontid;
	FVec2 pos;
	float w, h;
	FVec4 color;
	int owner;

	ZidgetText();
	virtual ~ZidgetText();
	virtual void render();
	void setText( char *t );
};


#endif

