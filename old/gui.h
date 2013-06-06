// @ZBS {
//		*MODULE_OWNER_NAME glgui
// }

#ifndef GLGUI_H
#define GLGUI_H

#include "zmsg.h"
#include "zstr.h"

// Layout
//==================================================================================

// Layout types:
//	pack
//		lays out in one dimension horiz or vertical
//		each contained object is given its ideal size.  However, if the size
//		of the children will cause the container to exceed its maximum
//		size then it will steal size away from the children "evenly"
//		by asking each what its minimum size is and giving each a biased amount
//      Options:
//        pack_side = [top] | bottom | left | right
//        pack_anchor = r | t | l | b | c
//        packChildOption_optW
//        packChildOption_optH
//        packChildOption_padX = pixels [0]
//        packChildOption_padY = pixels [0]
//        packChildOption_fill = [none] | x | y | both
//           - Fills the child into the opposite dimension of the parent's layout.
//             For example, if layout is vertical this child will expand to
//             fill the horizontal element of the parent's size.
//             If it is the last child in the pack list it can expand in both dimensions
//             i.e. fill remaining space
//
//	table
//		the number of columns must be known
//		each column is either given a width or is left to bais from the children
//		similar to the pack algorithm
//		columns may be evenly distributed if requested
//		same for rows, they can be given a weight, be found automatically or evenly
//      Options:
//        table_cols = NUMBER
//        table_rows = NUMBER
//        table_colWeightN (where n is 1-numCols) = NUMBER
//        table_rowWeightN (where n is 1-numRows) = NUMBER
//        table_colDistributeEvenly = [0] | 1
//        table_rowDistributeEvenly = [0] | 1
//        tableChildOption_hAnchor = r | [l] | c
//        tableChildOption_vAnchor = t | [b] | c
//        tableChildOption_padX = pixels [0]
//        tableChildOption_padY = pixels [0]
//        tableChildOption_fill = x | y | both | [none]
//        tableChildOption_calcHFromW = scalar: multiply w by scalar to get h
//
//	place
//		Can put things anywhere relative to the parent
//      Options:
//        place_optW = number
//        place_optH = number
//        placeChildOption_l = formula
//        placeChildOption_t = formula
//        placeChildOption_r = formula
//        placeChildOption_b = formula
//        placeChildOption_w = formula
//        placeChildOption_h = formula
//      Where formula is an RPN notation with the following variables:
//        w, h, l, t, r, b = parent's coordinates
//        and with constants and operators + - * /
//        Ex: "placeChildOption_l = w 200 - 2 /" would center it with a 200 width


typedef ZHashTable GUIAttributeTable;

// GUIObject -- superclass of all objects
//==================================================================================

extern ZHashTable guiNameToObjectTable;

#define GUI_TEMPLATE_DEFINTION(type) static type type::templateCopy; type( char *name )
#define GUI_TEMPLATE_IMPLEMENT(type) type type::templateCopy( #type ); type::type( char *name ) { guiFactoryRegisterClass( this, sizeof(*this), name ); }

struct GUIObject {
	GUI_TEMPLATE_DEFINTION(GUIObject);

	GUIAttributeTable *attributeTable;
		// This table stores all of the attributes
		// for this gui object.  This may include some 
		// which are prefixed with * or +.  These are special and indicate that
		// they are to be inherited into children.
		// For example, suppose a parent has the following in it's attribute table:
		//   hidden = 1, *color = 2, color = 1, deaf = 0, *deaf = 1
		// When the child search for an attribute it will find
		//   color = 2, deaf = 1
		//
		// Note, there are a variety of special attributes.
		// name = string: the name as registered in called to registerAd
//		// mouseRequest = [none] | over | anyhere. If on, you get MouseEnter, MouseLeave, MouseMove
		// hidden = [0] | 1 : is it visible
		// sendMsgOn: binds an event to another event. Ex: "sendMsgOn:type=MouseClick dir=D which=L" = "type=Quit"
		// relayout = [0] | 1: if 1, it will relayout at next opportunity, see postLayout()
		// layoutManager = [none] | pack | table | place
		// hooks = names : A listed deliminted by space of objects that want to hear GUIAttrChanged messages
		// updateWhenHidden = [0] | 1 : Forces update to run even if the obj is hidden
		// font = what font to use

	float myX, myY;
		// This is in first quadrant: lower left = (0, 0)
	float myW, myH;
		// Measured in pixels

	int dead;
		// This is set on die and marks this for garbage collection
		// This delay is necessay so that we can kill objects
		// inside of message loops without it gettint confused.
	int isDead() { return dead; }

	GUIObject *parent;
	GUIObject *headChild;
	GUIObject *prevSibling;
	GUIObject *nextSibling;

	#define GUI_MODAL_STACK_SIZE (10)
	static GUIObject *modalStack[GUI_MODAL_STACK_SIZE];
	static int modalStackTop;
	void modal( int onOff );

//	static int exclusiveMouseRequestAnywhere;
//	static GUIObject *exclusiveMouseRequestObject;
	static GUIObject *exclusiveKeyboardRequestObject;
	int mouseWasInside;
		// This is a little cache that simplifies the processing of 
		// mouse events in guiRecurseMouseMovement

	char *boundMsgHead;
		// This is a linked list of parsed strings that
		// are used for binding events.
		// For example, when you setAttr( "sendMsgOn:type=MouseClickOn dir=D which=L", "type=FileOpen" );
		// The key of that attribute is actually stuffed into this
		// special linked list and will be searched by handleMsg

	void *getAttrp( char *key );
	int getAttrI( char *key );
	float getAttrF( char *key );
	double getAttrD( char *key );
	char *getAttrS( char *key );

	void setAttrp( char *key, void *val );
	void setAttrI( char *key, int val );
	void setAttrF( char *key, float val );
	void setAttrD( char *key, double val );
	void setAttrS( char *key, char *val );
	void setAttr( char *fmt, ... );
		// This will take a formatted key value pair identical to ZHashTable::parseKeyValString
		// Ex: "color=1 sendMsg='type=Quit code=2' *oink=1

	int isAttrS( char *key, char *val );
	int isAttrI( char *key, int val );
	int isAttrF( char *key, float val );
	int isAttrD( char *key, double val );

	int hasAttr( char *key );

	#define attrI( key ) int key = getAttrI( #key );
	#define attrF( key ) float key = getAttrF( #key );
	#define attrD( key ) double key = getAttrD( #key );
	#define attrS( key ) char *key = getAttrS( #key );

	GUIObject();
	virtual ~GUIObject();
	virtual void init();
		// Use this to create any dynamic content.  It gets called
		// after the factory clones the template.  Be sure that
		// the templates do not contain imbedded members which auto-initalize
		// (such as hashtables) since they would then be shared by all clones.

	void registerAs( char *_name );
		// Inserts the name into the nameToObjectTable and
		// also sets the local cache of the name
		// Passing null or empty assigns a default name

	void dump( int toStdout=0 );
		// Dump the attribute table to debug

	void addHook( GUIObject *hook );
	void addHook( char *hook );
		// These add an object to the hook list which means that they
		// will receive GUIAttrChanged messages when something in this obj. changes

	void orderAbove( GUIObject *toMoveGUI, GUIObject *referenceGUI );
	void orderBelow( GUIObject *toMoveGUI, GUIObject *referenceGUI );
		// Moves toMoveGUI relative to referenceGUI

	void attachTo( GUIObject *_parent );
	void detachFrom();
	int isMyChild( char *name );
	void killChildren();
		// Die on all children but not on the this
	void die();
		// Marks this and all children as dead and disassocaites
		// them all from each other.  Memory won't be
		// freed until it is synchronously garbage collect in guiIpdate

	GUIObject *childFromLocation( float x, float y );
		// Returns a pointer to the child at the position x y in local coords

	void getLocalCoords( float &x, float &y );
	void getWindowCoords( float &x, float &y );
	int contains( float x, float y );
	void setColorI( int color );
	
	void move( float _x, float _y );
	void moveNear( char *who, char *where, float xOff, float yOff );
	void resize( float _w, float _h );

	void print( char *m, float x, float y );
	void printSize( char *m, float &x, float &y, float &d );
		// The w, h, and descender of the font.
	void printWordWrap( char *m, float t, float l, float w, float h );

	static void modalKeysClear();
	static void modalKeysBind( GUIObject *o );
	static void modalKeysUnbind( GUIObject *o );
		// These are used for controling the kmodal keybindings such as:
		// "5-escape" meaning that you press 5 and then escape to active the control

	protected:
	// These are the internal functions for each kind of layout
	void pack();
	void table();
	void place();
		float placeParseInstruction( char *str );
		// This is used internally by place

	void packQueryOptSize( float &w, float &h );
	void tableQueryOptSize( float &w, float &h );
	void placeQueryOptSize( float &w, float &h );
	void recurseQueryOptSize( float &w, float &h );

	void zMsgQueue( char *msg, ... );
		// This overrides the global function so that
		// a few paramaters can be added

	public:
	void layout();
	void postLayout();
		// requests that a layout occurs on this and the parent
		// at the next synchronous opportunity.

	virtual void queryOptSize( float &w, float &h );
	virtual void queryMinSize( float &w, float &h );
	virtual void queryMaxSize( float &w, float &h );

	virtual void update();
		// This may be overloaded, but be sure to call this ancestor
		// call because it updates speed critical cache parameters

	virtual void render() { }

	virtual void handleMsg( ZMsg *msg );

	int requestExclusiveMouse( int anywhere, int onOrOff );
		// Certain mouse messages may be requested.  This is isolated
		// to prevent contention between controls that both want the
		// mouse to do something.  For example, you can't ask for
		// mouseDrag messages if some other object currently has a lock.
		// if "anywhere" is set, you will be sent messages no matter
		// where the mouse is.  Otherwise, only if inside of your bounds.
		// When anywhere is set, you will receive: type = MouseDrag
		// Otherwise you will receive: type = MouseEnter | MouseLeave | MouseDrag

	int requestExclusiveKeyboard( int onOrOff );
		// This steals the keyboard focus.  All keyboard events
		// will go to this GUIObject until it releases the lock

};

// GUIPanel -- flat colored rectagle, good for grouping
//==================================================================================

struct GUIPanel : GUIObject {
	GUI_TEMPLATE_DEFINTION(GUIPanel);
	int mouseOver;

	// SPECIAL ATTRIBUTES:
	// color: the background color including alpha
	// mouseOverColor: the background color when the mouse is over this panel including alpha
	// mouseOverAlpha: the alpha color when mouse is over (ignored if mouseOverColor is non-zero)
	GUIPanel();
	virtual void render();
	virtual void handleMsg( ZMsg *msg );
};

// GUIButton -- simple push button
//==================================================================================

struct GUIButton : GUIObject {
	GUI_TEMPLATE_DEFINTION(GUIButton);

	// SPECIAL ATTRIBUTES:
	// color: the background color including alpha
	// selectedColor: the color when it is in the selected state
	// pressedColor: the color when it is in the pressed state (takes precedence over selected)
	// disabledColor: the color when it is not pressable
	// textColor: the color of the text including alpha
	// disabledTextColor: the color of the text including alpha when is is not pressable
	// text: string to print unless textPtr is set
	// textPtr: a pointer to some address that contains the text to print
	// selected: If on, draws in the selectedColor
	// selectedPtr: A pointer to an int which it monitors for selected. (Overrides selected)
	// selectedPtrReadOnly: Is the pointer read-only
	// toggle: if set, when pressed it will will toggle selected or selectedPtr
	// sendMsg: what message to send when the button is pressed
	// sendMsgOnSelect: what message to send when the button goes into a selected state
	// sendMsgOnUnselect: what message to send when the button goes into a non-selected state
	//
	// SPECIAL MESSAGES:
	// The following two messages are messagized so that they can be intercepted by subclasses
	// type=GUIButton_SetSelectNoPropagate val=0|1
	//   - Send this to change selected without generating a sendMsgOnSelect or sendMsgOnUnselect 
	//     call this is useful when you have a group of related buttons
	//     and you want one to turn off the othres with getting into a recursive nightmare

	int pressed;
		// Is the button currently up or down?

	int isSelected();
	void setSelected( int s );
	char *text();
		// Gets the text pointer from with textPtr or the attrTable

	GUIButton();

	virtual void init();
	virtual void render();
	virtual void handleMsg( ZMsg *msg );
	virtual void queryOptSize( float &w, float &h );
};

// GUIRadioButton -- a line of text with a circle in front of it
//==================================================================================

struct GUIRadioButton : GUIObject {
	GUI_TEMPLATE_DEFINTION(GUIRadioButton);

	char *text();

	virtual void init();
	virtual void render();
	virtual void handleMsg( ZMsg *msg );
	virtual void queryOptSize( float &w, float &h );
};

// GUITabButton -- a button on a tab bar for selecting a vew
//==================================================================================

struct GUITabButton : GUIButton {
	GUI_TEMPLATE_DEFINTION(GUITabButton);

	// SPECIAL ATTRIBUTES:
	// In addition to button:
	// 

	GUITabButton() : GUIButton() { }
	virtual void render();
	virtual void handleMsg( ZMsg *msg );
};


// GUIText -- text, eiother static or used for a menu item
//==================================================================================

struct GUIText : GUIPanel {
	GUI_TEMPLATE_DEFINTION(GUIText);

	// SPECIAL ATTRIBUTES:
	// color: the background color including alpha
	// textColor: the color of the text including alpha
	// text: if the textPtr is NULL this string will be used
	// textPtr: a pointer to some address that contains the text to print
	// wordWrap: should the panel apply word wrapping at whitespace
	// mouseOverAlpha: alpha color if the mouse is over the area

	char *text();
		// Gets the text pointer from with textPtr or the attrTable

	GUIText() : GUIPanel() { }
	virtual void render();
	virtual void queryOptSize( float &w, float &h );
};

// GUIHorizLine
//==================================================================================

struct GUIHorizLine : GUIObject {
	GUI_TEMPLATE_DEFINTION(GUIHorizLine);

	// SPECIAL ATTRIBUTES:
	//   color: color
	//   thickness: how thick is the line

	GUIHorizLine() : GUIObject() { }
	virtual void render();
	virtual void queryOptSize( float &w, float &h );
};

// GUIMenu
//==================================================================================

// A menu is panel that responds to the clicking
// of it's children's item by closing itself.
// It's children can be created either by attaching any
// other object to this or by using the itemN attributes.
// All children objects must send a "MenuItemSelected"
// selected command to it's parent.  Menu's can be heirarchical
// becuase the GUIMenu always sends the same command to its parent

struct GUIMenu : GUIPanel {
	GUI_TEMPLATE_DEFINTION(GUIMenu);

	// SPECIAL ATTRIBUTES:
	//   optN: Where N is 1-based number encoding the options and sendMsg
	//     like opt1='Click Me To Quit;type=Quit'

	int serialNum;
		// This is just used by the constructor to communicate.  Ignore.

	GUIMenu() : GUIPanel() { }
	virtual void init();
	virtual void handleMsg( ZMsg *msg );
};

// A menu item is just a static text that knows to send
// the MenuItemSelected command to its parent when it is clicked
struct GUIMenuItem : GUIPanel {
	GUI_TEMPLATE_DEFINTION(GUIMenuItem);

	// SPECIAL ATTRIBUTS:
	//   mouseOverColor: the color of the panel when the mouse is over it
	//   color: color of the panel
	//   textColor: color of the text
	//   sendMsg: what to send when clicked (always sends a MenuItemSelected as well)

	GUIMenuItem();
	virtual void init();
	virtual void render();
	virtual void queryOptSize( float &w, float &h );
	virtual void handleMsg( ZMsg *msg );
};

// GUITextEditor
//==================================================================================

struct GUITextEditor : GUIPanel {
	GUI_TEMPLATE_DEFINTION(GUITextEditor);

	// SPECIAL ATTRIBUTES:
	// color: the background color including alpha
	// textColor: the color of the text including alpha
	// text: if the textPtr is NULL this string will be used
	// textPtr: a pointer to some address that contains the text to print
	// wordWrap: should the panel apply word wrapping at whitespace
	// mouseOverAlpha: alpha color if the mouse is over the area
	// sendMsg: What to send when enter is pressed
	// SPECIAL MESSAGES:
	// Sends TextEditor_Escape when escape is pressed
	// Sends TextEditor_Enter when enter is pressed
	// Sends TextEditor_TextChanged when anything changes
	// Recvs KeyboardFocus to switch into editting state (sends itself the message on mouse click)

	double lastBlinkTime;
	int blinkState;

	char *text();
		// Gets the text pointer from with textPtr or the attrTable
	int textPosFromLocalX( float x );
		// Local used for computing where the mouse clicked
	int isEditting();
		// Used locally for determining if the cursor is on

	GUITextEditor() : GUIPanel() { }
	virtual void render();
	virtual void queryOptSize( float &w, float &h );
	virtual void handleMsg( ZMsg *msg );
};

// GUIPicker
//==================================================================================

struct GUIPicker : GUIPanel {
	GUI_TEMPLATE_DEFINTION(GUIPicker);

	// SPECIAL ATTRIBUTES:
	// SPECIAL MESSAGES:

	GUIPicker() : GUIPanel() { }
	virtual void render();
	virtual void queryOptSize( float &w, float &h );
	virtual void handleMsg( ZMsg *msg );
};

// Macros
//==================================================================================

// These macros make it easy to create or get pointers
// to the gui objects.  Since all gui objects must be
// registered under a globally unique name, the macros
// create local variables under the same name.

#define guiCreate(name,type) \
	type *name = new type(); \
	name->init(); \
	name->registerAs( #name )
	// This creates a local variable like:
	// GUIPanel *root = new GUIPanel();
	// And also registers that with the name
	// root->registerAs( "root" );

#define guiGet(name,type) type *name = (type *)guiFind( #name )
	// Finds the object called "root" and makes
	// a local pointer to it like:
	// GUIPanel *root = (GUIPanel *)guiFind( "root" );

#define guiPtr(name,type) type *name = (type *)guiFind( #name ); name
	// Finds it and gives the pointer inline, like:
	// GUIPanel *root = (GUIPanel *)guiFind( "root" ); root ->setText( "Oink" );

// GUI Interface
//==================================================================================

extern int guiWinW, guiWinH;
	// The cache of the root window's width and height
extern double guiTime;
	// The time as last set by the guiUpdate function

void guiFactoryRegisterClass( void *ptr, int size, char *name );
GUIObject *guiFactory( char *name, char *type );
GUIObject *guiFind( char *name );

void guiReshape( int _winW, int _winH );
void guiRecurseUpdate( GUIObject *o );
void guiRecurseRender( GUIObject *o );
void guiRecurseDispatch( GUIObject *o, ZMsg *zMsg );
//void guiMouseMovement( int mouseX, int mouseY, int mouseL, int mouseR );
void guiBindKey( char *key, char *sendMsg );

void guiExecute( char *script, ZHashTable *args=(ZHashTable *)0 );
void guiExecuteFile( char *filename, ZHashTable *args=(ZHashTable *)0 );
	// Executes a gui script which contains a set of setup instructions
	// for some piece of the gui.  Often used in conjunction with direct manipulation

void guiUpdate( double time );
	// The time argument is the current time used for animations
	// within the the gui system.  This avoids any dependencies on
	// OS specific clock functions.  Passing a constant will
	// mean that no animations or ther time-dependent effects will work

void guiGarbageCollect();
	// Kills anything marked as dead.  This is called at the beginning
	// of guiUpdate but can be called manually too if needed.

void guiRender( GUIObject *root=(GUIObject *)0 );
	// Renders an object and all its children.  If root is null it
	// uses the global object named root.
	// This function does not guarantee that that projection and modelview
	// matricies will be left untouched.  If this matters, be sure to reset them
	// both after the call.  (This was to avoid the 2 deep PROJECTION stack problem)

void guiDispatch( ZMsg *zMsg );

void guiHideMouse();
void guiShowMouse();
	// Show and hide the mouse without regard to the internal windows counters

void guiDieByRegExp( char *regExp );
	// Kill anything whose name matches the regular expression

#endif


