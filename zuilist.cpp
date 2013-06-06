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
//	putS( "panelColor", "0x00FF0080" );
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

int ZUIList::showChildren( int itemId, int bShow ) {
	// Set the hidden state of children if itemId.
	// If itemId is < 0, show/hide all child items.
	int visibleCount = 0;
	for( int i=0; i<items.count; i++ ) {
		ListItem *pItem = items[i];
		if( pItem->parentItemId >= 0 && ( itemId < 0 || itemId == pItem->parentItemId ) ) {
			if( pItem->itemZui ) {
				pItem->itemZui->putI( "hidden", !bShow );
			}
			if( bShow ) { pItem->flags |= LI_VISIBLE;  }
			else        { pItem->flags &= ~LI_VISIBLE; }
		}
		if( pItem->flags & LI_VISIBLE ) {
			visibleCount++;
		}
	}
	return visibleCount;
}

void ZUIList::handleMsg( ZMsg *msg ) {
	if( zmsgIs( type, ZUIMouseClickOn ) ) {
		// do work here in respond to some click in the listbox,
		// before the click is received by the child listitem
		// //trace( "  * zuilist got a clickon\n" );
	}
	else if( zmsgIs( type, ZUIList_ItemClicked ) ) {
		int itemId = zmsgI( itemId );
		//trace( "ZUIList %s got ItemClicked message (itemId=%d):\n", name, itemId );
		ListItem *item = getListItemFromId( itemId );
		if( item && (item->flags & LI_SELECTABLE) ) {
			ListItem *lastSelected = getSelectedListItem();
			selectedId = itemId;
			char *clickMsg = getS( "clickMsg", 0 );
			if( clickMsg ) {
				zMsgQueue( ZTmpStr( "%s listItem=%ld lastSelected=%ld itemId=%d", clickMsg, (long)item, (long)lastSelected, itemId ) );
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
	ZUIPanel::handleMsg( msg );
}

ListItem* ZUIList::addItem( void *item, int itemType, int parentItemId ) {
	ListItem* lItem=0;
	if( item ) {
		lItem				= new ListItem;
		lItem->list			= this;
		lItem->item			= item;
		lItem->itemId		= itemNextId++;
		lItem->itemType		= itemType;
		lItem->parentItemId = parentItemId;

		int index = items.add( lItem ); 
		assert( index >= 0 );
		assert( index == lItem->itemId );
		
		ZUI *z=0;
			// if the zui corresponding to an item has children itself, but
			// still wishes to notify the listbox parent when it is  clicked on,
			// it may wish to set the "parentHandler" property, which will cause
			// it to receive messages and *NOT* it's children.  It can then choose
			// to forward messages to children, either manually, or by setting the
			// "parentHandled" property and calling dispatch(), which will send the
			// message, including any per-zui message modification (e.g. is mouse on)
			// for the child zuis.

		if( itemZuiCreate ) {
			z = itemZuiCreate( lItem );
		}
		else {
			char *type = getS( "itemZuiType", 0 );
			if( type ) {
				z = ZUI::factory( 0, type );
			}
			else if( !itemRender ) {
				z = ZUI::factory( 0, "ZUIPanel" );
				z->putS( "layout_padX", "20" );
				z->putS( "panelColor", "0" );
				z->putS( "pack_fillOpposite", "1" );
				z->putS( "layout_cellFill", "wh" );
				z->putS( "layout_forceH", "20" );
				z->putS( "layout_forceW", "1000" );


				ZUI *c = ZUI::factory( 0, "ZUIText" );
				c->putS( "text", (char*)item );
				c->attachTo( z );
			}
		}
		if( z ) {
			lItem->itemZui = z;
			z->putS( "layout_cellFill", "wh" );
			if( !z->getS( "sendMsg", 0 ) ) {
				z->putS( "sendMsg", ZTmpStr( "type=ZUIList_ItemClicked itemId=%d toZUI=%s", lItem->itemId, this->name ) );
					// the message we should send to our parent when appropriate
			}
			z->attachTo( this );
			ListItem *parent = lItem->getParent();
			while( parent ) {
				z->translateX += getF( "childIndent", 10.f );
				parent = parent->getParent();
			}
		}
	}
	//trace( "ZUIList: added item of type %d, address %08X, parent %08X\n", lItem->itemType, lItem->item, lItem->parent );
	return lItem;
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

void ZUIList::setSelectedListItem( ListItem *li, int scrollToTop ) {
	selectedId = -1;
	if( li ) {
		selectedId = li->itemId;
		if( scrollToTop ) {
			scrollItemIdToTop( selectedId );
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

void ZUIList::dump() {
	for( int i=0; i<items.count; i++ ) {
		//trace( "List %d: itemType=%d, address=%08X, parent=%08X\n", items[i].itemType, &items[i], items[i].parent );
	}
}
