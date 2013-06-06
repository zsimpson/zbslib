#ifndef ZUIFPSGRAPH
#define ZUIFPSGRAPH

#include "zui.h"

struct ZUIFPSGraph : ZUIPanel {
	ZUI_DEFINITION(ZUIFPSGraph,ZUIPanel);

	#define HISTORY_SIZE (200)
	int lastFrameCount;
	float history[HISTORY_SIZE];
	int histCount;

	ZUIFPSGraph() : ZUIPanel() { }
	virtual void factoryInit();
	virtual void update();
	virtual void render();
	virtual ZUILayoutCell *layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly=1 );
};


#endif

