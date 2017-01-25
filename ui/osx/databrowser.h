/*
 *  databrowser.h
 *  PeerCast
 *
 *  Created by mode7 on Fri Apr 09 2004.
 *  Copyright (c) 2002-2004 peercast.org. All rights reserved.
 *
 */
#ifndef _DATABROWSER_H
#define _DATABROWSER_H

#include <Carbon/Carbon.h>

class DataBrowser
{
public:
	explicit DataBrowser( WindowRef window, const ControlID& controlId, 
						  DataBrowserItemDataProcPtr itemDataProcPtr,
						  DataBrowserItemNotificationProcPtr notificationProcPtr );
	
	ControlRef controlRef() { return mControl; }
	
	OSStatus addItems ( DataBrowserItemID container,
					    UInt32 numItems,
						const DataBrowserItemID *items,
						DataBrowserPropertyID preSortProperty )
	{
		return AddDataBrowserItems ( mControl, container, numItems, items, preSortProperty );
	}
	
	bool isVisible() const { return (IsControlVisible( mControl ) != false); }
	
	OSStatus removeItems( DataBrowserItemID container,
						  UInt32 numItems,
						  const DataBrowserItemID *items,
						  DataBrowserPropertyID preSortProperty )
	{
		return RemoveDataBrowserItems( mControl, container, numItems, items, preSortProperty );
	}
	
	OSStatus updateItems( DataBrowserItemID container,
						  UInt32 numItems,
						  const DataBrowserItemID *items,
						  DataBrowserPropertyID preSortProperty,
						  DataBrowserPropertyID propertyID )
	{
		return UpdateDataBrowserItems ( mControl, container, numItems, items, preSortProperty, propertyID );
	}
	
	OSStatus getItems( DataBrowserItemID container,
					   Boolean recurse,
					   DataBrowserItemState state,
					   Handle items)
	{
		return GetDataBrowserItems( mControl, container, recurse, state, items );
	}
	
	OSStatus setPropertyFlags( DataBrowserPropertyID property, DataBrowserPropertyFlags flags )
	{
		return SetDataBrowserPropertyFlags( mControl, property, flags );
	}
	
	OSStatus getPropertyFlags( DataBrowserPropertyID property, DataBrowserPropertyFlags *flags )
	{
		return GetDataBrowserPropertyFlags( mControl, property, flags );
	}
	
private:

	ControlID  mControlId;
	ControlRef mControl;
};

#endif // _DATABROWSER_H