// @ZBS {
//		*COMPONENT_NAME zuilist
//		*MODULE_OWNER_NAME zuilist
//		*GROUP modules/zui
//		*REQUIRED_FILES zuilist.cpp
// }


// MODULE includes:
#include "zuilist.h"

// ZBSLIB includes:
#include "ztmpstr.h"
#include "zmsg.h"
#include "ztime.h"
#include "zmathtools.h"

extern void trace( char *fmt, ... );

// ListItem
//--------------------------------------------------------------------------------------------------------------------------------

ListItem* ListItem::getParent() {
	if( list && parentItemId >= 0 ) {
		return list->getListItemFromId( parentItemId );
	}
	return 0;		
}

// ZUIList
//--------------------------------------------------------------------------------------------------------------------------------

ZUI_IMPLEMENT(ZUIList,ZUIPanel);

char *getPanelColor( ZUIList *list, ListItem *item ) {
	if( item->parentItemId != -1 && list->getI( "itemPanelColorChildren" )) {
		ListItem *parent = item->getParent();
		if( parent->parentItemId != -1 ) {
			return list->getS( "itemPanelColorChild2" );
		}
		return list->getS( "itemPanelColorChild1" );
	}
	return list->getS( "itemPanelColor" );
}

void ZUIList::factoryInit() {
	ZUIPanel::factoryInit();
	init();
	container = 1;
	putS( "style", "styleGlobal" );
	putI( "permitScrollY", 1 );
	putI( "clipToBounds", 1 );
	putI( "clipMessagesToBounds", 1 );
		// if a msg has coordinates, and those coords are outside of our
		// window, do not attempt to handle msg, nor allow children to.
	putS( "pack_side", "t" );
	putS( "pack_fillOpposite", "1" );
	putS( "layout_cellFill", "wh" );
	putS( "itemPanelColor", "0xC0C0C0FF" );
	putS( "itemPanelSelectedColor", "0xC0E0C0FF" );
	putI( "hilightSelected", 1 );
		// should we hilight the selected panel?
	putI( "itemPanelColorChildren", 1 );
		// should children panels be uniquely colored to distinguish?
		// This helps to declutter the look in some cases.
	putS( "itemPanelColorChild1", "0xB0B0B0FF" );
	putS( "itemPanelColorChild2", "0xA0A0A0FF" );
	
	putD( "doubleClickThreshhold", 0.3 );
		// a click within .3 secs of another click is considered a double click;
	
	// putI( "parentHandler", 1 );
		// parentHander means that during dispatch, we'll get messages before our children.
		// our children won't get the message unless we explicitly forward it to them by
		// calling dispatch() 
		//
		// this is not always necessary if the child list item is a zui that can
		// respond to a click by sendign a message to us, like ZUIText or ZUIPanel
		// can do by setting their sendMsg
}

void ZUIList::init() {
	items.clear();
	itemZuiCreate	= 0;
	itemRender		= 0;
	itemSize		= 0;
	itemCompare		= 0;
	itemNextId	    = 0;
	selectedId		= -1;
}

void ZUIList::render() {
	renderBase( 0.f, 0.f, w, h );
	if( itemRender ) {
		for( int i=0; i<items.count; i++ ) {
			ListItem *item = items[i];
			float _x=0.f, _y=0.f, _w=0.f, _h=0.f;
			if( item->flags & LI_VISIBLE ) {
				if( item->itemZui ) {
					item->itemZui->getWindowXY( _x, _y );
					_x  = item->itemZui->x + scrollX + item->itemZui->translateX;
					_y  = item->itemZui->y + scrollY + item->itemZui->translateY;
					_w  = item->itemZui->w;
					_h  = item->itemZui->h;
				}
				if( itemSize ) {
					itemSize( item, _w, _h );
						// todo, we need to accum the height 
				}
				itemRender( item, _x, _y, _w-2, _h-2, (selectedId==item->itemId) );
			}
		}
	}
}

int ZUIList::showChildren( int itemId, int bShow, int bRecurse ) {
	// Set visibility on children of itemId, respecting the expanded
	// state of the parent node.  If the parent node is NOT expanded,
	// we should still hide children if requested, but if showing,
	// the parent must be expanded for us to show them.
	
	int visibleCount = 0;
	ListItem *item = getListItemFromId( itemId );
	if( itemId == -1 || ( !bShow || item->flags & LI_EXPANDED ) ) {
		for( int i=0; i<items.count; i++ ) {
      		ListItem *pItem = items[i];
			if( pItem->parentItemId >= 0 && ( itemId < 0 || itemId == pItem->parentItemId ) ) {
				if( pItem->itemZui ) {
					pItem->itemZui->putI( "hidden", !bShow );
				}
				if( bShow ) { pItem->flags |= LI_VISIBLE;  }
				else        { pItem->flags &= ~LI_VISIBLE; }
				if( bRecurse ) {
					showChildren( pItem->itemId, bShow, 1 );
				}
			}
			if( pItem->flags & LI_VISIBLE ) {
				visibleCount++;
			}
		}
	}
	checkScrollY();
	return visibleCount;
}

void ZUIList::expandItem( int itemId, int bExpand ) {
	// Called to expand/collapse an item (or all items if itemId == -1).
	// Note there are two relevant flags: LI_VISIBLE, and LI_EXPANDED. 
	// In the case that we expand a node, and the child is already expanded,
	// we need to recursively set visible on the children.  This is also
	// true if collapsing (in this case, recursive unset visible on children).

	for( int i=0; i<items.count; i++ ) {
    	ListItem *pItem = items[i];
		if( itemId < 0 || pItem->itemId == itemId ) {
			ZUI* expander=0;
			if( getI( "treeView" ) || getI( "itemPanelCheckbox" ) ) {
				expander = pItem->itemZui->headChild->headChild;
					// get at the ZUIText that shows a + or - depending on expand state
			}
			if( bExpand ) {
				pItem->flags |= LI_EXPANDED;
			}
			else {
				pItem->flags &= ~LI_EXPANDED;
			}
			if( expander ) {
				expander->putS( "text", pItem->flags & LI_EXPANDED ? "-" : "+" );
			}
			showChildren( pItem->itemId, bExpand, itemId == -1 ? 0 : 1 );
				// if itemId is -1, we don't need to recurse, since all items
				// will get processed
		}
	}
	dirty();
	ZUI::layoutRequest();
}

void ZUIList::handleMsg( ZMsg *msg ) {
	if( zmsgIs( type, ZUIMouseClickOn ) ) {
		// do work here in respond to some click in the listbox,
		// before the click is received by the child listitem
	}
	else if( zmsgIs( type, ZUIList_ItemClicked ) ) {
		int itemId = zmsgI( itemId );
		ListItem *item = getListItemFromId( itemId );
		if( item && (item->flags & LI_SELECTABLE) ) {

			ListItem *lastSelected = getSelectedListItem();
			setSelectedListItem( item );
		
			char *clickMsg = getS( "clickMsg", 0 );
			if( clickMsg ) {
				
				int doubleClick = 0;
				ZTime lastClick = getD( "lastClickTime", 0.0 );
				ZTime now = zTimeNow();
				putD( "lastClickTime", now );
				if( lastClick != 0.0 && item==lastSelected && now - lastClick < getD( "doubleClickThreshhold" ) ) {
					doubleClick = 1;
					item->itemZui->putS( "panelColor", "0xE0E0C0FF" );
				}
				
				zMsgQueue( ZTmpStr( "%s listItem=%ld lastSelected=%ld itemId=%d doubleClick=%d", clickMsg, (long)item, (long)lastSelected, itemId, doubleClick ) );
					// the above will cause the address of item to be held as a string
					// in the table, such that the receiver of the message must know to
					// getS and then convert to a long, and then convert to a pointer.
					// (see STRING_TO_PTR macro for this)
					// It is important that you put this into the message in base 10(%ld).
					// Using (long) should work on both 32 & 64 bit platforms, since
					// (long) typically matches pointer size.
					//
					// the other obvious option is to create the hashtable here, and 
					// explicity do a putP with the pointer, and then the handler can
					// do a getp.  But that means that the clickMsg must not be compound
					// (i.e. must not contains semicolons with multiple messages), unless
					// we go to the trouble of parsing all this right here like zMsgQueue
					// does...
					/*
					assert( !strchr( clickMsg, ';' ) ); 
						// read the above
					ZHashTable *msg = zHashTable( clickMsg );
					msg->putP( "item", getItemFromId( itemId ) );
					msg->putS( "fromZUI", name );
					::zMsgQueue( msg );
					*/
			}
		}
		return;
	}
	else if ( zmsgIs( type, ZUIList_ShowChildren ) ) {
		if( zmsgI( hideOthers ) ) {
			showChildren( -1, 0 );
		}
		showChildren( zmsgI( itemId ), 1 );
	}
	else if ( zmsgIs( type, ZUIList_HideChildren ) ) {
		showChildren( zmsgI( itemId ), 0 );
	}
	else if( zmsgIs( type, ZUIList_ExpanderClicked ) ) {
		ListItem *li = getListItemFromId( zmsgI( itemId ) );
		int doExpand = !( li->flags & LI_EXPANDED );
		expandItem( zmsgI( itemId ), doExpand );
	}
	ZUIPanel::handleMsg( msg );
}

void showParentExpander( ZUIList *list, ListItem *child, int bShow ) {
	if( child->parentItemId != -1 ) {
		ListItem *li = list->getListItemFromId( child->parentItemId );
		if( li ) {	
			if( li->itemZui && li->itemZui->headChild && list->getI( "treeView" ) ) {
				ZUI *c = li->itemZui->headChild->headChild;
					// c now points to the first control within the controlsPanel panel,
					// which is the expander.
				c->putI( "hidden", !bShow );
			}
		}
	}
}

ListItem* ZUIList::addItem( void *item, int itemType, int parentItemId ) {
	ListItem* li=0;
	if( item ) {
    li               	= new ListItem;
    li->list     			= this;
    li->item     			= item;
    li->itemId   			= itemNextId++;
    li->itemType 			= itemType;
    li->parentItemId 	= parentItemId;

    int index = items.add( li ); 
    assert( index >= 0 );
    assert( index == li->itemId );

		ZUI *z=0;
			// if the zui corresponding to an item has children itself, but
			// still wishes to notify the listbox parent when it is  clicked on,
			// it may wish to set the "parentHandler" property, which will cause
			// it to receive messages and *NOT* it's children.  It can then choose
			// to forward messages to children, either manually, or by setting the
			// "parentHandled" property and calling dispatch(), which will send the
			// message, including any per-zui message modification (e.g. is mouse on)
			// for the child zuis.

		// We create a top-level panel per item in the list 
		//
		ZUI *panel = ZUI::factory( 0, "ZUIPanel" );
		panel->putS( "pack_side", "l" );
		panel->putS( "layout_cellFill", "wh" );
		panel->putI( "pack_fillOpposite", 1 );
		panel->putS( "panelColor", getPanelColor( this, li ) );
		panel->putS( "sendMsg", ZTmpStr( "type=ZUIList_ItemClicked itemId=%d toZUI=%s", li->itemId, this->name ) );
		panel->putS( "sendMsgOnRightClick", ZTmpStr( "type=ZUIList_ItemRightClicked itemId=%d toZUI=%s", li->itemId, this->name ) );
		li->itemZui = panel;

		// If we are going to be a treeView, or the user has requeted us to manage a checkbox per list item,
		// we'll make use of a separate small panel with controls, and create a'clientPanel' for the user to use
		//
		ZUI* clientPanel = panel;
		if( getI( "treeView" ) || getI( "itemPanelCheckbox" ) ) {
			ZUI *controlsPanel = ZUI::factory( 0, "ZUIPanel" );
			controlsPanel->putS( "pack_side", "t" );
			controlsPanel->putS( "layout_cellFill", "h" );
			controlsPanel->putS( "panelColor", "0" );
			controlsPanel->putS( "layout_forceW", "19" );
      controlsPanel->putF( "layout_indentT", 5 );
      controlsPanel->putF( "layout_padX", 5 );
      	// cause the expander and checkbox to be horizontally centered; cellAlign doesn't
      	// work because checkboxes layout weird.

			ZUI *expander = ZUI::factory( 0, "ZUIText" );
			expander->putI( "layout_forceH", 10 );
			expander->putI( "expanded", 0 );
			expander->putS( "text", "+" );
			expander->putS( "sendMsg", ZTmpStr( "type=ZUIList_ExpanderClicked toZUI=%s itemId=%d", this->name, li->itemId ) );
			//expander->putS( "layout_cellAlign", "c" );
			expander->putI( "hidden", 1 );
			if( parentItemId != -1 ) {
				showParentExpander( this, li, 1 );
					// the current item is a child, so parent needs visible expander.
			}
			expander->attachTo( controlsPanel );
			
			if( getI( "itemPanelCheckbox" ) ) {
				ZUI *check = ZUI::factory( 0, "ZUICheck" );
				check->attachTo( controlsPanel );
			}

			controlsPanel->attachTo( panel );
				
			clientPanel = ZUI::factory( 0, "ZUIPanel" );
			clientPanel->putS( "pack_side", "t" );
			clientPanel->putS( "layout_cellFill", "wh" );
			clientPanel->putI( "pack_fillOpposite", 1 );
			clientPanel->putI( "clipToBounds", 1 );
			clientPanel->putS( "panelColor", "0" );
			clientPanel->attachTo( panel );

			// the following are duplicated from what we do with the panel at the top of this fn.
			// I'm not sure yet how we want to handle the panel/clientPanel distinction here, and
			// whether li->itemZui should point to panel or clientPanel.
			clientPanel->putS( "sendMsg", ZTmpStr( "type=ZUIList_ItemClicked itemId=%d toZUI=%s", li->itemId, this->name ) );
			clientPanel->putS( "sendMsgOnRightClick", ZTmpStr( "type=ZUIList_ItemRightClicked itemId=%d toZUI=%s", li->itemId, this->name ) );
		}

		if( itemZuiCreate ) {
			z = itemZuiCreate( this, li, clientPanel );
		}
		else {
			char *type = getS( "itemZuiType", 0 );
			if( type ) {
				z = ZUI::factory( 0, type );
				z->attachTo( clientPanel );
			}
			else if( !itemRender ) {
				ZUI *c = ZUI::factory( 0, "ZUIText" );
				c->putS( "text", (char*)item );
				c->attachTo( clientPanel );
			}
		}
		
		//
		// Cause child contents to be left-indented:
		//
    ListItem *parent = li->getParent();
		int totalIndent = 0;
		while( parent ) {
			totalIndent += getI( "childIndent", 10 );
			parent = parent->getParent();
		}
		panel->putI( "layout_indentL", totalIndent );

		if( li->parentItemId == -1 ) {
			// TODO: this should probably be optional.
			ZUI *line = ZUI::factory( 0, "ZUILine" );
      line->attachTo( this );
      line->putI( "lineEmbossed", 1 );
		}
		panel->attachTo( this );
	}
	//trace( "ZUIList: added item of type %d, address %08X, parent %08X\n", lItem->itemType, lItem->item, lItem->parent );
	return li;
}

ListItem * ZUIList::getListItemFromId( int id ) {
	for( int i=0; i<items.count; i++ ) {
		if( items[i]->itemId == id ) {
			return items[i];
		}
	}
	return 0;
}

ListItem * ZUIList::getSelectedListItem() {
	return getListItemFromId( selectedId );
}

void ZUIList::scrollItemIdToTop( int itemId ) {
	// since we do not support the deletion of items, the itemId
	// is always the index of the item in the list, and we use the
	// scrollYStep attribute (which should be set by the user)
	// to place the given item at the top.
	scrollY = getF( "scrollYStep" ) * itemId;
	dirty();
}

void ZUIList::checkScrollY() {
	// A ZUIList has permitScrollY by default, but it is optional to set
	// a scrollMinItems -- which allows a list to expand and NOT use scrolling
	// until a minimum number of items exist, at which point the height remains
	// fixed and the list scrolls.

	int needsLayout=0;
	int scrollMinItems = getI( "scrollYMinItems" );
	if( scrollMinItems ) {
		int visibleCount=0;
		for( int i=0; i<items.count; i++ ) {
			if( items[i]->itemZui && !items[i]->itemZui->getI( "hidden" ) ) {
				visibleCount++;
			}
		}
		if( visibleCount >= scrollMinItems ) {
			if( !has( "layout_forceH" ) ) {
				//putI( "layout_forceH", (scrollMinItems-1) * (int)getF( "scrollYStep" ) ); 
				putI( "layout_forceH", max( (int)h, (scrollMinItems-1) * (int)getF( "scrollYStep" ) ) );
				needsLayout=1;
			}
		}
		else if( has( "layout_forceH" ) ) {
			del( "layout_forceH" );
			needsLayout = 1;
		}
	}
	if( needsLayout ) {
		layoutRequest();
	}
}

void ZUIList::setSelectedListItem( ListItem *li, int scrollToTop ) {
	ListItem *lastSelected = getSelectedListItem();
	selectedId = -1;
	if( li ) {
		selectedId = li->itemId;
		ListItem *parent = li->getParent();
	    if( parent && !(parent->flags & LI_EXPANDED) ) {
	    	expandItem( li->parentItemId, 1 );
	    }
		if( scrollToTop ) {
			scrollItemIdToTop( selectedId );
		}
	}
	if( getI( "hilightSelected" ) ) {
		if( lastSelected && lastSelected->itemZui ) {
			lastSelected->itemZui->putS( "panelColor", getPanelColor( this, lastSelected ) );
			lastSelected->itemZui->dirty();
		}
		if( li && li->itemZui ) {
			li->itemZui->putS( "panelColor", getS( "itemPanelSelectedColor" ) );
			li->itemZui->dirty();
		}
	}
}

void ZUIList::setSelectedUserItem( void *userItem, int scrollToTop ) {
	for( int i=0; i<items.count; i++ ) {
		if( items[i]->item == userItem ) {
			setSelectedListItem( items[i], scrollToTop );
		}
	}
}

void ZUIList::reset() {
	killChildren();
	items.clear();
	itemNextId = 0;
	selectedId = -1;
	scrollY = 0;
}

void ZUIList::setItemColorTag( ListItem *li, int color ) {
	if( li->itemZui && li->itemZui->headChild && getI( "itemPanelCheckbox" ) ) {
		// set the color of the control panel, which is a small rectangular
		// region at left of the listbox item, useful for indicating things
		// with color without making text hard to read.
		li->itemZui->headChild->putI( "panelColor", color );
	}
}

ZUI *getCheckBox( ZUI *list, ListItem *li ) {
	ZUI *child = 0;
	if( li->itemZui && li->itemZui->headChild && list->getI( "itemPanelCheckbox" ) ) {
		child = li->itemZui->headChild->headChild;
			// child now points to the first control within the controlsPanel panel
		while( child && child->nextSibling ) {
			child = child->nextSibling;
				// checkbox is last in list of controls
		}
	}
	return child;
}
 
void ZUIList::setCheckboxMessages( ListItem *li, char *selectMsg, char *unselectMsg, int initialState ) {
	ZUI *child = getCheckBox( this, li );
	if( child ) {
		child->putS( "sendMsgOnSelect", selectMsg );
		child->putS( "sendMsgOnUnselect", unselectMsg );
		child->putI( "selected", initialState );
	}
}

void ZUIList::setCheckboxState( ListItem *li, int state, int *selectedPtr, int defeatMsg  ) {
	ZUI *child = getCheckBox( this, li );
	if( child ) {
		if( selectedPtr ) {
			child->putP( "selectedPtr", selectedPtr );
			state = *selectedPtr;
		}
		else {
			child->putI( "selected", state );
		}
		if( defeatMsg ) {
		  child->putI( state ? "sentSelectedMsg" : "sentUnselectedMsg", 1 );
        // defeat sending message on next render
		}
	}
}

void ZUIList::setCheckboxShow( ListItem *li, int show ) {
	ZUI *child = getCheckBox( this, li );
	if( child ) {
		child->putI( "hidden", !show );
		child->dirty();
	}
}


void ZUIList::dump() {
	for( int i=0; i<items.count; i++ ) {
		//trace( "List %d: itemType=%d, address=%08X, parent=%08X\n", items[i].itemType, &items[i], items[i].parent );
	}
}
