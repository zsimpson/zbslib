// GUIGraph
//==================================================================================

#include "gui.h"
#include "ztl.h"

struct GUIGraph : GUIPanel {
	GUI_TEMPLATE_DEFINTION(GUIGraph);

	ZTLVec<float> history;

	void addAndShift( float x );

	GUIGraph() : GUIPanel() { }
	virtual void init();
	virtual void render();
	virtual void queryOptSize( float &w, float &h );
};


