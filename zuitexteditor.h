// GUITextEditor
//==================================================================================


struct ZUITextEditor : ZUIPanel {
	ZUI_DEFINITION(ZUITextEditor,ZUIPanel);

	// SPECIAL ATTRIBUTES:
	// textEditMarkColor
	// textEditCursorColor
	// textEditTextColor
	// text: if the textPtr is NULL this string will be used
	// textPtr: a pointer to some address that contains the text to print
	// multiline: is the panel multiline
	// wordWrap: should the panel apply word wrapping at whitespace.  MUST be multiline
	// mouseOverAlpha: alpha color if the mouse is over the area
	// sendMsg: What to send when enter is pressed
	// SPECIAL MESSAGES:
	// Sends TextEditor_Escape when escape is pressed
	// Sends TextEditor_Enter when enter is pressed
	// Sends TextEditor_TextChanged when anything changes
	// Recvs KeyboardFocus to switch into editting state (sends itself the message on mouse click)

	double lastBlinkTime;
	int blinkState;
	int lineCount;
	int *lines;
	int lastTextScrollY;
	int lastTextScrollX;

	void getWH( float &w, float &h );
	void getXYWH( float &x, float &y, float &w, float &h );
		// Overloads to deal with the scroll bars

	char *text();
		// Gets the text pointer from with textPtr or the attrTable
	int isEditting();
		// Used locally for determining if the cursor is on
	int isAlphaNum( char c );
		// Defines what chars separate on advance by word

	void takeFocus( int strPos );
	void cancelFocus();
		// These are internal utility functions

	void linesUpdate();
		// Compute the line wrapping array

	int boundStrPos( int strPos );
	void getViewWH( float &_w, float &_h );

	int strPosFromPixelXY( float x, float y );
		// Compute the cursor offset position from the pixel coord
	void strPosToPixelXY( int strPos, float &x, float &y );
		// Compute the x y from the cursor
//	int strPosFromLineAndPixelX( int line, float x );
		// Get the strPos from the line and a given pixel x
	void strPosToLineAndChar( int strPos, int &line, int &charX );
		// Fetch the line and char count from left of the cursor
	int lineAndCharToStrPos( int line, int charX );
		// Get the strPos (bounded) from line and charX.  CharX can be negative or beyond end of line and it will wrap correctly

	void cursorMove( int deltaX, int deltaY );
		// Deals with moving the cursor and all of its boundary cases
	void markPosUpdate( int prevCursor, int changing );
		// updates the mark position given the last cursor position and a flag saying of the mark is changing
	void sendMsg( char *extraMsg, int fromCancel );
		// Sends a msg if the sendMsg paramter is set

	ZUITextEditor() : ZUIPanel() { }
	virtual void factoryInit();
	virtual void render();
	virtual void update();
	virtual ZUI::ZUILayoutCell *layoutQueryNonContainer( float maxW, float maxH, float parentW, float parentH, float &w, float &h, int sizeOnly );
	virtual void handleMsg( ZMsg *msg );
};

