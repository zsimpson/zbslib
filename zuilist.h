// @ZBS {
//		*MODULE_OWNER_NAME zuilist
// }

#ifndef ZUILIST_H
#define ZUILIST_H

#include "zui.h"
#include "ztl.h"
#include "zvec.h"

struct ListItem;
class ZUIList;
typedef ZUI * (*itemZuiCreateCallback)( ZUIList *list, ListItem *li, ZUI *panel );
	// creates zui that will be child of the ZUIList; if not set, the 
	// itemZuiType can be set, or renderItemCallback must render the item, or default to ZUIText
typedef void (*itemRenderCallback)( ListItem *li, float x, float y, float w, float h, int selected );
	// called before child renders if child is a zui, allowing custom 
	// painting of the background or some such; if no zui, does all rendering 
typedef void (*itemSizeCallback)( ListItem *li, float &width, float &height );
	// called to get the size of the item, if necessary (e.g. if not
	// a straightforward zui child item)
typedef int (*itemCompareCallback)( const void* item1, const void* item2 );
	// qsort-style callback if the items should be sorted

struct ListItem {

	void *item;
	int itemId;
	ZUI *itemZui;
	int itemType;
	int parentItemId;
	ZUIList *list;
	ListItem *getParent();

	int flags;
		#define LI_SELECTABLE 0x01
		#define LI_VISIBLE 0x02
		#define LI_EXPANDED 0x04

	ListItem() {
		item	=  0;
		itemId	= -1;
		itemZui	=  0;
		itemType=  0;
		flags   = LI_SELECTABLE | LI_VISIBLE;
		list 	= 0;
		parentItemId = -1;
	}
};

class ZUIList : public ZUIPanel {
	ZUI_DEFINITION(ZUIList,ZUIPanel);
	
	// ZUIList properties:
	//
	// clickMsg: message sent when an item in the list is clicked:
	//			 zMsgQueue( ZTmpStr( "%s listItem=%ld lastSelected=%ld itemId=%d", clickMsg, (long)item, (long)lastSelected, itemId ) );
	//
	// itemZuiType: can be used to specify what kind of ZUI to create for display of newly added list items, if not using itemZuiCreateCallback.
	//              
	// childIndent: int specifies how much to indent child nodes of list (like a treeview)
	//
	// scrollYStep: float (property of scrollable panels) -- set this to size of your items for discrete item scrolling
	//
	//	treeView: should the listbox provide +/- controls for expanding/collapsing nodes?
	//
	//  itemPanelCheckbox: should each panel have a checkbox control (user can supply messages for select/unselect)
	//
	// various panel coloring attributes (see factory method)
	
	

public:
	ZTLPVec< ListItem > items;
	int itemNextId;
	int selectedId;

	itemZuiCreateCallback	itemZuiCreate;
	itemRenderCallback	    itemRender;
	itemSizeCallback	    itemSize;
	itemCompareCallback	    itemCompare;

	//-----------------------------------------------

	// ZUI
	ZUIList() : ZUIPanel() { init(); }
	virtual void factoryInit();
	virtual void render();
	virtual void handleMsg( ZMsg *msg );

	// ZUIList
	void init();
	ListItem* addItem( void *item, int type=0, int parentItemId=-1 );
	ListItem* getListItemFromId( int id );
	ListItem* getSelectedListItem();
	
	
	void scrollItemIdToTop( int itemId );
	void checkScrollY();
	
	void setSelectedListItem( ListItem *li, int scrollToTop=0 );
		// selects given item, optionally scrolls it to top of list
	void setSelectedUserItem( void *item, int scrollToTop=0 );
	// void delItem( void *item );
		// because there is no delete, itemId is known to be also the index of
		// items in the list.  This is used by scrollItemIdToTop().
	
	int showChildren( int itemId, int bShow, int bRecurse=0 );
		// show/hide child items; 
	void expandItem( int itemId, int bExpand );
		// for treeview functionality
	void reset();
	
	void setItemColorTag( ListItem *li, int color );
		// set a rgba encoded it as the color for the small rectangular
		// region at left of listitem (when control panel is present)

	void setCheckboxMessages( ListItem *li, char *selectMsg, char *unselectMsg, int initialState=1 );
		// if optional itemPanel checkboxes were requested, the client can set 
		// messages to be sent for the given ListItem

	void setCheckboxState( ListItem *li, int state, int *selectedPtr=0, int defeatMsg=0 );
		// set initial selection state.  If selectedPtr is passed, this is a pointer that
		// will control the selected state of the checkbox.  defeatMsg will cause the
		// sel or unsel msg to NOT get sent next render.

	void setCheckboxShow( ListItem *li, int show );
		// show or hide the checkbox if the zui has one.
		

	void dump();
};


// ZUIMapper: desired usage pattern
// shortest setup in simplest case:
//
// setup/populate listA (see above)
// setup/populate listB (see above)
// ZHashTable results;
//
// ZUIMapper *zm = ... < create via Factory or get from zui file >
// zm->setup( listA, listB, &results );
//
// ready for display...

/*
class ZUIMapper : public ZUIPanel {
	ZUI_DEFINITION( ZUIMapper, ZUIPanel );
public:

	//
	// ZUI	
	//
	virtual void handleMsg( ZMsg *msg );
	
	//
	// ZUIMapper
	//
	ZUIList *listA;
	ZUIList *listB;
	ZHashTable *mapping;
		// maps ListItems from listA to ListItems in listB

	void setup( ZUIList *la, ZUIList *lb, ZHashTable *map ) {
		listA	= lA;
		listB	= lB;
		mapping	= map;
	}

	void addToItem( void *item );
	void addFmItem( void *item );
};
*/

#endif



