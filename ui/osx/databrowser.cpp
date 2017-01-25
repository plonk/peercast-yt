/*
 *  databrowser.cpp
 *  PeerCast
 *
 *  Created by akin on Fri Apr 09 2004.
 *  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
 *
 */

#include "databrowser.h"

DataBrowser::DataBrowser( WindowRef window, const ControlID& controlId, 
						  DataBrowserItemDataProcPtr itemDataProcPtr,
						  DataBrowserItemNotificationProcPtr notificationProcPtr )
: mControlId( controlId )
 ,mControl  ( NULL )
{
	DataBrowserCallbacks dbCallbacks;
	
	GetControlByID( window, &mControlId, &mControl );
	dbCallbacks.version = kDataBrowserLatestCallbacks;
	InitDataBrowserCallbacks( &dbCallbacks );
	
	dbCallbacks.u.v1.itemDataCallback         = NewDataBrowserItemDataUPP( itemDataProcPtr );
	dbCallbacks.u.v1.itemNotificationCallback = NewDataBrowserItemNotificationUPP( notificationProcPtr );

	SetDataBrowserCallbacks( mControl, &dbCallbacks );
	SetAutomaticControlDragTrackingEnabledForWindow( window, true );
}
