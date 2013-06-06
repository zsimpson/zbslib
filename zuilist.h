// @ZBS {
//		*MODULE_OWNER_NAME zuilist
// }

#ifndef ZUILIST_H
#define ZUILIST_H

#include "zui.h"
#include "ztl.h"
#include "zvec.h"

struct ListItem;
typedef ZUI * (*itemZuiCreateCallback)( ListItem *li );
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

class ZUIList;
struct ListItem {

	void *item;
	int itemId;
	ZUI *itemZui;
	int itemType;

	//ListItem *parent;
	// I had to remove *parent and replace with an index, else when the ZTLVec reallocs,
	// all the parent points are fucked.
	ZUIList *list;
	int parentItemId;
	ListItem *getParent();

	int flags;
		#define LI_SELECTABLE 0x01
		#define LI_VISIBLE 0x02

	ListItem() {
		item	=  0;
		itemId	= -1;
		itemZui	=  0;
		itemType=  0;
		flags   = LI_SELECTABLE | LI_VISIBLE;
		list	= 0;
		parentItemId = -1;
	}
};

class ZUIList : public ZUIPanel {
	ZUI_DEFINITION(ZUIList,ZUIPanel);

	//----------------------------------------------
	// custom properties
	//
	// clickMsg		- sent if user clicked an item, with item=<item clicked>
	// okMsg		- sent if user pressed OK, with item=<item selected>
	// cancelMsg	- sent if user pressed cancel
	// itemZuiType  - name of a zui class used to display items in list; 
	//				  if supplied, and no createItemCallback is set, will be
	//				  passed to ZUI::factory() to create zuis for items.
	// 
	// custom messages
	//
	// ZUIList_HideChildren - allows tree view, if list item is a ZUIList
	//					      standard panel indent used to offset child nodes							  
	// ZUIList_ShowChildren

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
	void setSelectedListItem( ListItem *li, int scrollToTop=0 );
	void setSelectedUserItem( void *item, int scrollToTop=0 );
	// void delItem( void *item );
		// because there is no delete, itemId is known to be also the index of
		// items in the list.  This is used by scrollItemIdToTop().
	int showChildren( int itemId, int bShow );
		// show/hide child items; 
	void reset();


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



