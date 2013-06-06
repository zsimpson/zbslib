// @ZBS {
//		*MODULE_OWNER_NAME zui
// }

#ifndef ZUI_H
#define ZUI_H

#include "zhashtable.h"
#include "zmsg.h"
#include "zstr.h"
#include "assert.h"

#define ZUI_DEFINITION(type,superclass) static type templateCopy; type( char *_name, char *_superclass )
#define ZUI_IMPLEMENT(type,superclass) type type::templateCopy( #type, #superclass ); type::type( char *_name, char *_superclass ) { ZUI::factoryRegisterClass( this, sizeof(*this), _name, _superclass ); }

#define ZUI_VAR_BEGIN() struct __ZGC__ { __ZGC__() { ZUI z;
#define ZUI_VAR(type,name) ZUI::zuiVarLookup.putI( #name, ((#type[0]&0xFF)<<24)|int((char *)&z.name - (char *)&z) ); 
#define ZUI_VAR_END() }} __zvc;

extern int useDirtyRects;
extern int drawDirtyRects;
	// extern to allow plugs to toggle these

// Standard dialogbox type functionality available to all gui plugins
//-------------------------------------------------------------------------------------------------------------------------------

void zuiNamedDialog( char *name, char *type, char *title, int modal=1, char *okMsg=0, char *cancelMsg=0, char *dlgParams=0, char *ctlParams=0,
			    char *initMsg=0, char *okText=0, char *cancelText=0, int clonedInterior=1 );

void zuiDialog( char *type, char *title, int modal=1, char *okMsg=0, char *cancelMsg=0, char *dlgParams=0, char *ctlParams=0,
			    char *initMsg=0, char *okText=0, char *cancelText=0, int clonedInterior=1 );
	// defined in zui.cpp; type denotes name of zui holding interior controls; optional ok/cancel messages, dlg/ctl params.
	// dlgParams are attributes set on the containing dialog
	// ctlParams are attributes set on the contained ZUIPanel holding the controls
	// initMsg if specified is a msg that will get sent before dialog is displayed to init controls.
	// okText/cancelText: text for buttons if different from "Ok"/"Cancel"
	// clonedInterior: normally a user's interior UI is cloned to allow multiple dialogs of the
	// same type, but in some cases it is most convenient to have a single named copy if the user
	// knows there will only ever by one instance.
void zuiMessageBox( char *title, char *msg, int colorError=0, char *okMsg=0, char *cancelMsg=0, char *okText=0, char *cancelText=0 );
	// defined in zui.cpp; if colorError msgbox is red; optional msgs on ok/cancel; optional button text to replace "ok"/"cancel"
void zuiProgressBox( char *name, char *title, char *msg, char *cancelMsg=0, char *cancelText=0 );
	// defined in zui.cpp; if colorError msgbox is red; optional msgs on ok/cancel; optional button text to replace "ok"/"cancel"
void zuiChooseFile( char *title, char *path, int openingFile, char *okMsg, char *cancelMsg=0, bool slideIn=false, char *customPicker=0 );
	// defined in zuifilepicker.cpp; path may spec wildcard/filetype, chosen file returned in okMsg 'filespec' param
	// optional cusomtPicker specifies a custom filepicker zui spec to use instead of standard zlab picker in main.zui
	// NOTE: the openingFile flag was added for native dialog file support, since some OS display different dialogs for
	// open vs. save functionality.  openingFile has no effect on the non-native zui file picker.

// ZUI
//--------------------------------------------------------------------------------------------------------------------------------

struct ZUIHash : ZHashTable {
	ZUIHash() : ZHashTable( 0 ) { }
};

struct ZUI : ZUIHash {
	// I had this as a private and this caused an error when compilinggcc apple
	// saying that it wasn't accessible in zuivaredit::createMenu
	// so I changed it to public

	ZUI_DEFINITION(ZUI,NONE);

	// Binary cached variables
	float x;
	float y;
	float w;
	float h;
	int layoutManual;
	int layoutIgnore;
	int hidden;
	int container;
	char *name;
	float scrollX;
	float scrollY;
	float translateX;
	float translateY;
	float goalX;
	float goalY;

	int styleUpdateFlag;

	// VARIABLE handling
	static ZHashTable zuiVarLookup;
	void *getVar( char *memberName, char &type );

	// PROPERTIES & STYLES
	void styleUpdate();
	void propertiesInherit();
	static void propertiesRunInheritOnAll();

	// ZHASHTABLE OVERLOADS
	// The follow accessor methods overload the get and put
	// methods of hashtable so that the gets may seek up the
	// parent list to find inherited attributes.

	void *findBinKey( char *key, int *valLen, int *valType );
	void *findKey( char *key, int *valLen, int *valType );
		// Helper functions ussed by the get and put routines

	int has( char *key );
	int isS( char *key, char *cmp );

	char        *getS( char *key, char *onEmpty="" );
	int          getI( char *key, int onEmpty=0 );
	unsigned int getU( char *key, unsigned int onEmpty=0 );
	S64          getL( char *key, S64 onEmpty=0 );
	float        getF( char *key, float onEmpty=0.f );
	double       getD( char *key, double onEmpty=0.0 );
	void        *getp( char *key, void *onEmpty=0 );
	int          geti( char *key, int onEmpty=0 );
	unsigned int getu( char *key, unsigned int onEmpty=0 );
	S64          getl( char *key, S64 onEmpty=0 );
	float        getf( char *key, float onEmpty=0.f );
	double       getd( char *key, double onEmpty=0.0 );

	void  putB( char *key, char *val, int valLen );
	void  putS( char *key, char *val, int valLen=-1 );
	void  putS( const char *key, char *val, int valLen=-1 );
	void  putS( char *key, const char *val, int valLen=-1 );
	void  putS( const char *key, const char *val, int valLen=-1 );
	void  putI( char *key, int val );
	void  putI( const char *key, int val );
	void  putU( char *key, unsigned int val );
	void  putL( char *key, S64 val );
	void  putF( char *key, float val );
	void  putD( char *key, double val );
	void  putP( char *key, void *ptr, int freeOnReplace=0, int delOnNul=0 );

	void   putKeyStr( char *key, char *fmt, ... );
		// Eg: putKeyStr( "sendMsg", "type=Oink toZUI=%s", getS("name") );

	char  *dumpToString();
	
	// ATTRIBUTES:
	//  x = position form left   (not usually set directly -- let the layout manager set it)
	//  y = position from bottom (not usually set directly -- let the layout manager set it)
	//  w = width  (not usually set directly -- let the layout manager set it)
	//  h = height (not usually set directly -- let the layout manager set it)
	//  hidden = [0|1] (is it visible)
	//  scrollX -- deprecated, see ZUIPanel
	//  scrollY -- deprecated, see ZUIPanel
	//  noScissor = [0|1] (disable the scissor)
	//  name = name of object (Do not set directly, see registerAs)
	//  immortal = [0|1] (never die)
	//  dead = [0|1] (do not set directly, use die() )
	//  font = name of font
	//  layoutManual = [0|1] (overrides layout, uses the manual coords below)
	//  layoutManual_x = RPN (set the x from left in RPN where W=parent width, H=parent height,+,-,*,/,constant,^=max,v=min as in 'W 0.9 * 200 v' = 90% of parent width of 200 whichever is smaller
	//  layoutManual_y = RPN as above
	//  layoutManual_w = RPN as above
	//  layoutManual_h = RPN as above
	//  layout = [pack|border|table] (layout manager, see options below)
	//  layout_padX = numPixels (x padding in pixels)
	//  layout_padY = numPixels (y padding in pixels)
	//  layout_cellAlign = [e|ne|n|nw|w|sw|s|se|c] (where to place object if it is smaller than cell)
	//  layout_cellFill = [w|h|wh] (should the object be filled if it is smaller than the cell)
	//  layout_forceW = RPN as above (forces w of object to this value)
	//  layout_forceH = RPN as above (forces h of object to this value)
	//  layout_sizeToMax = [w|h|wh] Force the cell to eat the max space.  Good when you have a cell that you want centered in the parent you need to expand it so that it it can center
	//  layout_indentL = X Force left indentation
	//  layout_indentT = X Force left indentation
	//  layout_indentR = X Force left indentation
	//  layout_indentB = X Force left indentation
	//  pack_side = [l|t|r|b] (For packs, where is the anchor)
	//  pack_fillOpposite = [0|1] (Should the cell be filled into remaining space opposite of the flow, i.e. down on L/R or left on T/B)
	//  pack_fillLast = [0|1] (Should the last cell be filled into remaining space)
	//  pack_wrapFlow = [0|1] (Should the pack continue into another col for t/b or row for l/r)
	//  border_side = [e|ne|n|nw|w|sw|s|se|c] (which region to fill. Specified for EACH CHILD of a border layout)
	//  table_cols = num (Number of cols.  You must specifiy either the cols or the rows and the other will be derived)
	//  table_rows = num (Number of cols.  You must specifiy either the cols or the rows and the other will be derived)
	//  table_colWeightN (where n is 1-numCols) = a weighting number
	//  table_colDistributeEvenly = [0|1] (Should cols be distributed evenly disregarding child size and weight)
	//  table_rowDistributeEvenly = [0|1] (Should rows be distributed evenly disregarding child size and weight)
	//  table_calcHFromW = num (Child option.  multiply derived w by this scalar to get h)
	//  updateWhenHidden = [0|1] (Should the object have update() called even when it is hidden)

	// COLOR
	static ZHashTable colorPaletteHash;
	void setColorI( int color );
	void setColorS( char *colorName );
	void setupColor( char *key );
	int getColorI( char *key );
	void scaleAlpha( float scale );
	int byteOrderReverse( int i );

	// POSITION / SIZE
	void getXY( float &x, float &y );
	void getWH( float &w, float &h );
	void getXYWH( float &x, float &y, float &w, float &h );
	void getWindowXY( float &x, float &y );
		// Convert local to window coords
	void getLocalXY( float &x, float &y );
		// Convert window to local coords
	int containsLocal( float x, float y );
		// Are these local points inside of this zui
	void move( float x, float y );
	void moveNear( char *who, char *where, float xOff, float yOff );
	void resize( float w, float h, int layout=1 );
	static void orderAbove( ZUI *toMoveZUI, ZUI *referenceZUI );
	static void orderBelow( ZUI *toMoveZUI, ZUI *referenceZUI );
	ZUI *findByCoord( float winX, float winY );
		// Recurses from the caller into the children, returns the top most hit at that coord
	ZUI *findByCoordWithAttribute( float winX, float winY, char *key, char *val );
		// Same, with additional requirement to find some key/val match

	// HIERARCHY
	ZUI *parent;
	ZUI *headChild;
	ZUI *nextSibling;
	ZUI *prevSibling;
	void attachTo( ZUI *_parent );
	void detachFrom();
	int isMyChild( char *name );
	int isMyAncestor( ZUI *_parent );
		// walk up parent chain looking for _parent
	int childCount();

	// DEATH
	static ZUI *deadHead;
	static void zuiGarbageCollect();
	virtual void destruct() { }
		// This is called before delete so that things may delete
		// things that are in the hash table otherwise stupid destructor code
		// will delete the hash table before it calls the virtual destructor
	void die();
		// Call this to kill, do NOT delete so that things will happen synchronoudly
		// This simply sets the "die" flag
	void killChildren();
		// Calls die on all children but not on the this

	// MODALITY & FOCUS
	#define ZUI_STACK_SIZE (16)
	static ZUI *modalStack[ZUI_STACK_SIZE];
	static int modalStackTop;
	void modal( int onOff );

	// The zui with "focus" gets to look at all messages first
	// it may decide to eat or not eat the message but can make
	// state changes regardless.  For example, a text box
	// would take focus and if mouse was clicked outside it
	// would remove focus, stop blinking its cursor, and NOT eat the
	// message so that a button for example might be pressed.
	// Focued zuis do NOT get handleMsg twice, the dispatcher
	// smartly skips them during recursion.
	static ZUI *focusStack[ZUI_STACK_SIZE];
	static int focusStackTop;
	void focus( int onOff );

	// DRAG & DROP
	static int dragTextureId;
	static ZUI * dragZuiSource;
	static ZUI * dragZuiTarget;
	static float dragOffsetX;
	static float dragOffsetY;
	static double cancelTime;
	static float cancelX;
	static float cancelY;

	virtual void beginDragAndDrop( float x, float y );
		// the zui member fn that sets up the above data.  A zui will call this in response
		// to a mouse move in which it first realizes a drag&drop is occuring.
	virtual void updateDragAndDrop();
	virtual void endDragAndDrop();
	static void resetDragAndDrop();
	static void renderDragAndDrop();



	// DIRTY RECTS
	static int dirtyRectCount;
	#define DIRTY_RECTS_MAX (64)
	static int dirtyRects[DIRTY_RECTS_MAX][4];
	static void dirtyRectAdd( int x, int y, int w, int h );
	static void dirtyAll();
	virtual void dirty( int el=0, int er=0, int et=0, int eb=0 );
		// optional "extra left, ..." params to expand dirty rect box for controls that
	void scissorIntersect( int _x, int _y, int _w, int _h );
		// glScissor using the argument intersected with
		// the current scissor box.


	// RENDER
	virtual void recurseRender( int whichDirtyRect );
	static void zuiRenderTree( ZUI *root=0 );
	virtual void render() { }
	void pixelsToTexture( int texture );
		// screen-grab our rect from the screen and put the pixels into the passed gl texture id
		// note: this is a simple first-pass approach, it may be more robust to create a function
		// that simply renders to an offscreen pbuffer and then puts that in the texture
	void printWordWrap( char *m, float l, float t, float w, float h, float *computeW, float *computeH );
	void print( char *m, float l, float t, int len=-1 );
	void printSize( char *m, float &w, float &h, float &d, int len=-1 );
	void begin3d( bool setViewport=true );
	void end3d();
		// 3d functions save/restore opengl matrix/attrib state & perform appropriate viewport transform

	// REGISTRATION
	static ZHashTable zuiByNameHash;
	static ZHashTable *factoryTablePtrs;
	static ZHashTable *factoryTableSize;
	static ZHashTable *factoryTableSuperclass;
	void registerAs( char *name );
	static void factoryRegisterClass( void *ptr, int size, char *name, char *superclass );
	static ZUI *factory( char *name, char *type );
	static ZUI *zuiFindByName( char *name );
	virtual void factoryInit() {};
		// Called after the factory creates it
	virtual void init();
		// Calls both the hash table init and the factory init
		// Use this if you are creating the zui in code

	// CLONE/COPY
	static ZUI *clone( ZUI *root, char *newName = 0 );
		// recursively clone the heirarchy by calling ::cloneme on each zui, and
		// return the newly created copy
	virtual ZUI *cloneMe( char *newName = 0);
		// create a clone of myself, which basically means copy my hashtable and some
		// binary properties, unless there is special memory mgmt to do.

	// LAYOUT
	static int zuiLayoutRequest;
	static void layoutRequest();
	static int postLayoutRender;
	struct ZUILayoutCell { float x, y, w, h, requestW, requestH; };
	int canLayout();
	float layoutParseInstruction( char *str, float maxW, float maxH, float w, float h );
		// Parses the RPN notation on layout_forceX
	ZUILayoutCell *layoutDispatch( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly );
		// Evaluates the layout instructions (pack, table, etc) and send to the right code
	ZUILayoutCell *layoutQuery( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly=1 );
		// This is a the general layout query which decideds if the query should be sent to dispatch or whether forcing options apply
	virtual ZUILayoutCell *layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly=1 ) { return 0; }
		// Only non-ZUIPanels implement this
	ZUILayoutCell *layoutPack  ( float maxW, float maxH, float &w, float &h, int sizeOnly=1 );
	ZUILayoutCell *layoutBorder( float maxW, float maxH, float &w, float &h, int sizeOnly=1 );
	ZUILayoutCell *layoutTable ( float maxW, float maxH, float &w, float &h, int sizeOnly=1 );

	virtual void layout( float maxW, float maxH );
		// This is the top-level entry into the layout.  Recurses into all children

	// UPDATE
	void setUpdate( int flag );
	static ZHashTable zuiWantUpdates;
	static int zuiFrameCounter;
	static double zuiTime;
	static double zuiLastTime;
	static void zuiUpdate( double time );
	virtual void update() { }

	// MESSAGES
	static float rootW;
	static float rootH;
	static void zuiReshape( float w, float h );
	void recurseVisibilityNotify( int state );
	virtual void handleMsg( ZMsg *msg );
	void dispatch( ZMsg *msg );
	static void zuiDispatchTree( ZMsg *msg );
	void zMsgQueue( char *msg, ... );

	// NOTIFICATIONS
	void checkNotifyMsg( char *key );
	void appendNotifyMsg( char *key, char *msg );
	void setNotifyMsg( char *key, char *msg );
	char *getNotifyMsg( char *key );
		// A notification is a message that is sent when a key is changed
		// Be aware that the dispatch for this is only automatic if the putXXX interface
		// is used.  The notifications will not be automatically sent by a
		// binary modification of a member of a class

	// KEYBOARD AND MOUSE
	static ZHashTable globalKeyBindings;
	static void zuiBindKey( char *key, char *sendMsg );
//	static ZUI *exclusiveKeyboardRequestObject;
	int requestExclusiveMouse( int anywhere, int onOrOff );
//	int requestExclusiveKeyboard( int onOrOff );
	void getMouseLocalXY( float &x, float &y );
	static void modalKeysClear();
	static void modalKeysBind( ZUI *o );
	static void modalKeysUnbind( ZUI *o );
		// These are used for controling the modal keybindings such as:
		// "5-escape" meaning that you press 5 and then escape to active the control
		// Used for example in Shadow Garden to let remote control press buttons

	// SCRIPTS
	static void zuiExecuteFile( char *filename, ZHashTable *args=0 );
	static void zuiExecute( char *script, ZHashTable *args=0 );
	static void zuiExecute( ZStr *lines, ZHashTable *args=0 );

	static int zuiErrorTrapping;
	static char *zuiErrorMsg;
	static void zuiSetErrorTrap( int state );

	// CONSTRUCT / DESTRUCT
	ZUI();
	virtual ~ZUI() { }
};

// ZUIPanel
//--------------------------------------------------------------------------------------------------------------------------------

struct ZUIPanel : ZUI {
	ZUI_DEFINITION(ZUIPanel,ZUI);

	// ATTRIBUTES:
	//  container (this is a special attribute which is created by the factory and indicates that this is a container for layout purposes)
	//  color
	//  frame=[0,1,2]

	void renderBase( float x, float y, float w, float h );

	ZUIPanel() { }
	virtual void factoryInit();
	virtual void render();
	virtual void handleMsg( ZMsg *msg );
};

// ZUITabPanel
//--------------------------------------------------------------------------------------------------------------------------------

struct ZUITabPanel : ZUIPanel {
	ZUI_DEFINITION(ZUITabPanel,ZUIPanel);

	ZUITabPanel() { }
	virtual void render();
	virtual void handleMsg( ZMsg *msg );
};

// ZUIText
//--------------------------------------------------------------------------------------------------------------------------------

struct ZUIText : ZUI {
	ZUI_DEFINITION(ZUIText,ZUI);

	// ATTRIBUTES:
	//  textColor
	//  wordWrap
	//  text
	//  textPtr

	char *getText();

	virtual ZUILayoutCell *layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly=1 );

	ZUIText() { }
	virtual void render();
	virtual void handleMsg( ZMsg *msg );
};

// ZUIButton
//--------------------------------------------------------------------------------------------------------------------------------

struct ZUIButton : ZUI {
	ZUI_DEFINITION(ZUIButton,ZUI);

	char *getText();
	int getSelected( int forceBoolean=1 );
	void setSelected( int s, int forceBoolean=1 );
	virtual void checkSendSelectedMsg();
	void handleKeyBinding( ZMsg *msg );
	void renderButtonBase( float x, float y, int square=0 );
	void renderCheck();
	void setPressed( int pressed );

	virtual ZUILayoutCell *layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly=1 );

	ZUIButton() { }
	virtual void factoryInit();
	virtual void render();
	virtual void handleMsg( ZMsg *msg );
};

// ZUITabButton
//--------------------------------------------------------------------------------------------------------------------------------

struct ZUITabButton : ZUIButton {
	ZUI_DEFINITION(ZUITabButton,ZUIButton);

	// ATTRIBUTES:
	//  see ZUIButton

	virtual void checkSendSelectedMsg();
		// Overloaded to handle the clearing of the other tabs in the group

	ZUITabButton() { }
	virtual void factoryInit();
};

// ZUICheck
//--------------------------------------------------------------------------------------------------------------------------------
struct ZUICheck : ZUIButton {
	ZUI_DEFINITION(ZUICheck,ZUIButton);

	// ATTRIBUTES:
	//  heckTextColor
	//  checkTextPanelColor
	//  wordWrap
	//  text
	//  textPtr

	int getKeyBindingText( float &kw, float &kh, char *key );
	virtual ZUILayoutCell *layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly=1 );
	virtual void factoryInit();

	ZUICheck() { }
	virtual void render();
	virtual void handleMsg( ZMsg *msg );

};

// ZUICheckMulti
//--------------------------------------------------------------------------------------------------------------------------------

struct ZUICheckMulti : ZUICheck {
	ZUI_DEFINITION(ZUICheckMulti,ZUICheck);

	ZUICheckMulti() { }
	virtual void factoryInit();
	virtual void checkSendSelectedMsg();
		// overloaded to deal with multiple selection states
	virtual void dirty( int el=0, int er=0, int et=0, int eb=0 );
	virtual void render();
	virtual void handleMsg( ZMsg *msg );
};


// ZUILine
//--------------------------------------------------------------------------------------------------------------------------------

struct ZUILine : ZUI {
	ZUI_DEFINITION( ZUILine, ZUI );

	// SPECIAL ATTRIBUTES:
	//   dir ([horiz]/vert)
	//   lineColor
	//   thickness
	//   layout_cellFill (= wh by default)

	ZUILine() : ZUI() { }
	virtual void factoryInit();
	virtual void render();
	virtual ZUILayoutCell *layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly=1 );
};

// ZUIPicture
//--------------------------------------------------------------------------------------------------------------------------------

struct ZUIPicture : ZUIPanel {
	ZUI_DEFINITION( ZUIPicture, ZUIPanel );

	// SPECIAL ATTRIBUTES:
	//   pictureColor
	//   filename

	void cachePicture();

	ZUIPicture() : ZUIPanel() { }
	virtual void render();
	virtual ZUILayoutCell *layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly=1 );
};


#endif



