// GUIGraph
//==================================================================================

#include "gui.h"
#include "ztl.h"

// Series
//   A series is a collection of plot data all of the same dimension and type
//   Note that some plots have more than one series and that the series
//   may well be related to each other.  For example, one series might have
//   data and another have labels for the data.
//==================================================================================

struct GUIPlotSeries {
	virtual void clear()=0;
	virtual int xCount()=0;
	virtual int yCount()=0;
	virtual double *getd( int x, int y=0 )=0;
	virtual char *gets( int x, int y=0 )=0;
	virtual int isLabel() { return 0; }
};

struct GUILabelSeries : GUIPlotSeries {
	ZTLVec <char *> seriesData;
	void add( char *label );
		// This makes a strdup of the label

	virtual void clear();
	virtual int xCount();
	virtual int yCount();
	virtual double *getd( int x, int y=0 );
	virtual char *gets( int x, int y=0 );
	virtual int isLabel() { return 1; }
};

struct GUI1dSeries : GUIPlotSeries {
	ZTLVec <double> seriesData;
	void add( double x );

	virtual void clear();
	virtual int xCount();
	virtual int yCount();
	virtual double *getd( int x, int y=0 );
	virtual char *gets( int x, int y=0 );
};

struct GUI2dSeries : GUIPlotSeries {
	ZTLVec <double> seriesData;
	void add( double x, double y );

	virtual void clear();
	virtual int xCount();
	virtual int yCount();
	virtual double *getd( int x, int y=0 );
	virtual char *gets( int x, int y=0 );
};

struct GUI2dArraySeries : GUIPlotSeries {
	ZTLVec <double> seriesData;
	int sizeX, sizeY;

	GUI2dArraySeries();

	void setSize( int x, int y );
	void add( int x, int y, double z );

	virtual void clear();
	virtual int xCount();
	virtual int yCount();
	virtual double *getd( int x, int y=0 );
	virtual char *gets( int x, int y=0 );
};

struct GUIHistogramSeries : GUIPlotSeries {
	double *seriesData;
	int bucketCount;

	GUIHistogramSeries( int *data, int count, int numBuckets, int minV, int maxV );
	GUIHistogramSeries( float *data, int count, int numBuckets, float minV, float maxV );
	GUIHistogramSeries( double *data, int count, int numBuckets, double minV, double maxV );
	void init( double *data, int count, int numBuckets, double minV, double maxV );
	virtual void clear();
	virtual int xCount();
	virtual int yCount();
	virtual double *getd( int x, int y=0 );
	virtual char *gets( int x, int y=0 );
};

// Plot
//==================================================================================

struct GUIPlot : GUIPanel {
	GUI_TEMPLATE_DEFINTION(GUIPlot);

	ZTLVec<GUIPlotSeries *> series;

	virtual int getSeriesCount();
	virtual GUIPlotSeries *seriesGet( int i );
	virtual void seriesRemoveAll();
	virtual void seriesAdd( GUIPlotSeries *s );

	GUIPlot() : GUIPanel() { }
	virtual void init();
};

struct GUI2DPlot : GUIPlot {
	// Attributes of all 2D plots:
	//   xMin = 0, xMax = 1, yMin = 0, yMax = 1
	//   allowMouseRescale = R
	//   allowMousePan = C
	//   space = LINEAR | log

	double tm[4][4];
	double inversetm[4][4];
	void buildSpaceFromMinMax();
		// Builds an affine transform from the min max settings

	#define MAX_GRID_LINES (128)
	double gridVertLines[MAX_GRID_LINES];
	int gridStepBase;
	int gridBot, gridTop;
	double gridMinorStep;
	void computeGridLines();

	virtual void init();
	virtual void transform2( double *inPt, double *outPt );
	virtual void inverseTransform2( double *inPt, double *outPt );
	virtual void handleMsg( ZMsg *msg );
	virtual void renderGrid();
	virtual void renderLabels();
};

struct GUILinePlot : GUI2DPlot {
	GUI_TEMPLATE_DEFINTION(GUILinePlot);
	virtual void init();
	virtual void render();
};

struct GUIAreaPlot : GUI2DPlot {
	GUI_TEMPLATE_DEFINTION(GUIAreaPlot);
	virtual void init();
	virtual void render();
};

struct GUIColumnPlot : GUI2DPlot {
	GUI_TEMPLATE_DEFINTION(GUIColumnPlot);
	GUIColumnPlot() { }
	virtual void init();
	virtual void render();
};

struct GUIPiePlot : GUI2DPlot {
	GUI_TEMPLATE_DEFINTION(GUIPiePlot);
	virtual void init();
	virtual void render();
};

struct GUIScatterPlot : GUI2DPlot {
	GUI_TEMPLATE_DEFINTION(GUIScatterPlot);
	virtual void init();
	virtual void render();
};

struct GUI3DPlot : GUIPlot {
	// Attributes of all 3D plots:
	//   xMin = 0, xMax = 1, yMin = 0, yMax = 1, zMin = 0, zMax = 1
	//   allowMouseRescale = R
	//   allowMousePan = C
	//   space = LINEAR | log

	double tm[4][4];
	double inversetm[4][4];
	void buildSpaceFromMinMax();
		// Builds an affine transform from the min max settings

	#define MAX_GRID_LINES (128)
	double gridVertLines[MAX_GRID_LINES];
	int gridStepBase;
	int gridBot, gridTop;
	double gridMinorStep;
	void computeGridLines();

	virtual void init();
//	virtual void transform2( double *inPt, double *outPt );
//	virtual void inverseTransform2( double *inPt, double *outPt );
	virtual void handleMsg( ZMsg *msg );
	virtual void renderGrid();
	virtual void renderLabels();
};

struct GUI3DSurfacePlot : GUI3DPlot {
	GUI_TEMPLATE_DEFINTION(GUI3DSurfacePlot);
	virtual void init();
	virtual void render();
};

