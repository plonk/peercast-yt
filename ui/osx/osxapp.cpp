/*
 *  osxapp.cpp
 *  PeerCast
 *
 *  Created by mode7 on Mon Apr 12 2004.
 *  Copyright (c) 2002-2004 peercast.org. All rights reserved.
 *
 */
// ------------------------------------------------
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// ------------------------------------------------
#include "osxapp.h"
#include <string>

enum {
	 kChannelColumn   = 'CHAN'
	,kBitRateColumn   = 'BITR'
	,kStreamColumn    = 'STRM'
	,kRelayColumn     = 'TRLY'
	,kListenersColumn = 'LRLY'
	,kStatusColumn    = 'STAT'
	,kKeepColumn      = 'KEEP'
	
	,kNumProps = 7
};

enum {
	 kEventClassPeercast   = 'PEER'
	,kEventStartChannel    = 'STRT'
	,kEventUpdateChannel   = 'UPDT'
	,kEventStopChannel     = 'STOP'
	,kEventParamChanStatus = 'CSTA'
	,kEventParamSystem	   = 'PSYS'
	,typeChanStatus        = 'CSTA'
};

UInt32 propIDList[kNumProps] = {
	kChannelColumn
	,kBitRateColumn
	,kStreamColumn
	,kRelayColumn
	,kListenersColumn
	,kStatusColumn
	,kKeepColumn
};

static const UInt32 kPeerCast_AppSignature = FOUR_CHAR_CODE('PCTT');
static const UInt32 kPeerCast_TabControlID = 1000;

static const char* skYPString = "http://yp.peercast.org/";

static const ControlID skBumpButtonID		   = { 'RLBT', 1000 };
static const ControlID skCommentButtonID	   = { 'RLBT', 1001 };
static const ControlID skUpdateVersionButtonID = { 'PCST', 1007 };

OSXPeercastApp::OSXPeercastApp( const char *appPath, const char *iniFilename, int argc, char* argv[]
								,WindowRef window, const ControlID& dbControlID )
: mIniFileName    ()
,mRelayBrowser    ( window, dbControlID, 
					static_cast<DataBrowserItemDataProcPtr>(itemDataCallback), 
					static_cast<DataBrowserItemNotificationProcPtr>(itemNotificationCallback) )
,mWindowRef       ( window )
,mUpTime		     ('PCST', 1001)
,mDirectConnections  ('PCST', 1002)
,mRelayConnections   ('PCST', 1003)
,mCinCoutConnections ('PCST', 1004)
,mPGNUConnections    ('PCST', 1005)
,mIncomingConnections('PCST', 1006)
,mChannelList		  ( )
,mLock				  ( )
,mTabControl		  ( kPeerCast_AppSignature, kPeerCast_TabControlID, window )
,mAppEventHandler     ( OSXPeercastApp::appEventHandler )
,mMainWindowHandler   ( OSXPeercastApp::mainWindowHandler )
,mPeercastEventHandler( OSXPeercastApp::peercastEventHandler )
,mAppPath             ( appPath )
{
	mIniFileName.set(iniFilename);
	
	if (argc > 2)
	{
		if (strcmp(argv[1],"-inifile")==0)
			mIniFileName.setFromString(argv[2]);
	}
	
	DataBrowserPropertyFlags flags;
	
	mRelayBrowser.getPropertyFlags( kKeepColumn, &flags );
	
	flags |= kDataBrowserPropertyIsEditable;
	
	mRelayBrowser.setPropertyFlags( kKeepColumn, flags );
	
	installAppEventHandler();
	mTabControl.installHandler();
	installWindowHandler( window );	
	installPeercastEventHandler( window );
	
	enableNewVersionButton( false );
}
//------------------------------------------------------------------------
OSStatus OSXPeercastApp::installAppEventHandler()
{
    static const EventTypeSpec appEvents[] =
	{
		{ kEventClassCommand, kEventCommandProcess }
	};

    return InstallApplicationEventHandler(  mAppEventHandler.eventHandler()
										   ,GetEventTypeCount( appEvents )
										   ,appEvents
										   ,NULL
										   ,NULL ); 
}
//------------------------------------------------------------------------
OSStatus OSXPeercastApp::installWindowHandler( WindowRef window )
{
	static const EventTypeSpec mainWindowEvents[] =
	{
		 { kEventClassCommand, kEventCommandProcess }
		,{ kEventClassWindow,  kEventWindowActivated }
	};

	return InstallWindowEventHandler(  window
									  ,mMainWindowHandler.eventHandler()
							          ,GetEventTypeCount( mainWindowEvents )
							          ,mainWindowEvents
							          ,this
							          ,NULL );
}
//------------------------------------------------------------------------
OSStatus OSXPeercastApp::appEventHandler( EventHandlerCallRef inCallRef, EventRef inEvent, void* inUserData )
{
	HICommand command;
	
	OSStatus err = GetEventParameter(  inEvent
									  ,kEventParamDirectObject
									  ,typeHICommand
									  ,NULL
									  ,sizeof(HICommand)
									  ,NULL
									  ,&command );
					  
	if( err == noErr )
	{
		switch( command.commandID )
		{
			case FOUR_CHAR_CODE('PHLP'):
				sys->getURL("http://www.peercast.org/help.php"); 
				break;
		}
	}
	return eventNotHandledErr;
}
//------------------------------------------------------------------------
OSStatus OSXPeercastApp::mainWindowHandler( EventHandlerCallRef inCallRef, EventRef inEvent, void* inUserData )
{
	HICommand command;
	OSXPeercastApp *pOSXApp = static_cast<OSXPeercastApp*>( inUserData );
	
	OSStatus err = GetEventParameter(  inEvent
									  ,kEventParamDirectObject
									  ,typeHICommand
									  ,NULL
									  ,sizeof(HICommand)
									  ,NULL
									  ,&command );
									  
	if( err == noErr )
	{
		switch( command.commandID )
		{
			case FOUR_CHAR_CODE('save'):
				pOSXApp->saveSettings();
				break;
			case FOUR_CHAR_CODE('BUMP'):
				pOSXApp->onBumpClicked();				
				break;
			case FOUR_CHAR_CODE('CMNT'):
				pOSXApp->onCommentClicked();
				break;
			case FOUR_CHAR_CODE('YPBT'):
				{
					char serverIP[256];
					char buf[256];
					servMgr->serverHost.toStr( serverIP );
					sprintf( buf, "%s?host=%s", skYPString, serverIP );
					sys->getURL( buf );
				}
				break;
			case FOUR_CHAR_CODE('UPDT'):
				{
					sys->callLocalURL("admin?cmd=upgrade",servMgr->serverHost.port);
				}
				break;
		}
	}
	return eventNotHandledErr;
}
//------------------------------------------------------------------------
OSStatus OSXPeercastApp::installPeercastEventHandler( WindowRef theWindow )
{
	static const EventTypeSpec peercastEventTypes[] =
	{
		 { kEventClassPeercast, kEventStartChannel }
		,{ kEventClassPeercast, kEventUpdateChannel }
		,{ kEventClassPeercast, kEventStopChannel }
	};
	
	return InstallEventHandler(  GetApplicationEventTarget() 
								,mPeercastEventHandler.eventHandler()
								,GetEventTypeCount( peercastEventTypes )
								,peercastEventTypes, this, NULL );
}
//------------------------------------------------------------------------
void OSXPeercastApp::enableNewVersionButton( const Boolean visible )
{	
	ControlRef newVerButton;
	GetControlByID( mWindowRef, &skUpdateVersionButtonID, &newVerButton );	
	
	if( newVerButton != NULL )
	{
		SetControlVisibility( newVerButton, visible, true );
		//Draw1Control( tabControl );
	}
}
//------------------------------------------------------------------------
void OSXPeercastApp::addChanStatus( TChanStatusPtr& chanStatus )
{
	const DataBrowserItemID itemID = reinterpret_cast<DataBrowserItemID>(chanStatus.get());
	OSStatus err = mRelayBrowser.addItems( kDataBrowserNoItem, 1, &itemID, kChannelColumn );
	if( err == noErr )
	{
		mChannelList.push_back( chanStatus );
	}
}
//------------------------------------------------------------------------
void OSXPeercastApp::updateChanStatus( ChanStatus& newStatus )
{		
	TChannelList::iterator iter = findChannel( newStatus.getId() );
	
	if( iter != mChannelList.end() )
	{
		TChanStatusPtr& chanStatus = *iter;
			
		chanStatus->update( newStatus ); // copy new status

		const DataBrowserItemID itemID = reinterpret_cast<DataBrowserItemID>(chanStatus.get());
		OSStatus err = mRelayBrowser.updateItems( kDataBrowserNoItem, 1, &itemID, kChannelColumn, kDataBrowserNoItem );
	}
}
//------------------------------------------------------------------------
void OSXPeercastApp::removeChanStatus( GnuID& id )
{
	TChannelList::iterator iter = findChannel( id );
	
	if( iter != mChannelList.end() ) 
	{
		const TChanStatusPtr& chanStatus = *iter;

		const DataBrowserItemID itemID = reinterpret_cast<DataBrowserItemID>(chanStatus.get());
		OSStatus err = mRelayBrowser.removeItems( kDataBrowserNoItem, 1, &itemID, kChannelColumn );
		
		mChannelList.erase( iter ); // remove from list
	}
}
//------------------------------------------------------------------------
OSStatus OSXPeercastApp::peercastEventHandler( EventHandlerCallRef inCallRef, EventRef inEvent, void* inUserData )
{
	switch( GetEventClass( inEvent ) )
	{
		case kEventClassPeercast:
			{
				OSXPeercastApp *pOSXApp = static_cast<OSXPeercastApp*>( inUserData );
					
				switch( GetEventKind( inEvent ) )
				{
					case kEventStartChannel:
						{
							TChanXferPkt xferPkt;
							
							OSStatus err = GetEventParameter( inEvent, kEventParamChanStatus,
															  typeChanStatus, NULL,
															  sizeof(TChanXferPkt), NULL,
															  &xferPkt );

							TChanStatusPtr chanStatus( xferPkt.mpData );

							if( err != noErr )
								return err;
								
							pOSXApp->addChanStatus( chanStatus );
						}
						break;
						
					case kEventUpdateChannel:
						{
							TChanXferPkt xferPkt;
						
							OSStatus err = GetEventParameter( inEvent, kEventParamChanStatus, typeChanStatus, NULL, sizeof(TChanXferPkt), NULL, &xferPkt );
							
							TChanStatusPtr chanStatus( xferPkt.mpData );
							
							if( err != noErr )
								return err;
								
							if( chanStatus.get() != NULL )
							{									
								pOSXApp->updateChanStatus( *chanStatus );
							}
						}
						break;
						
					case kEventStopChannel:
						{
							GnuID chanId;
							
							OSStatus err = GetEventParameter( inEvent, kEventParamChanStatus, typeChanStatus, NULL, sizeof(GnuID), NULL, &chanId );
							if( err != noErr )
								return err;
								
							pOSXApp->removeChanStatus( chanId );
						}
						break;
				}
			}
			break;
	}
	
	return eventNotHandledErr;
}
//------------------------------------------------------------------------
void OSXPeercastApp::initAPP()
{
	mTabControl.updateSettings();
//	showStationStats();
}
//------------------------------------------------------------------------
void OSXPeercastApp::notifyMessage( ServMgr::NOTIFY_TYPE type, const char *message )
{
	switch( type )
	{
		case ServMgr::NT_UPGRADE:
			{
				enableNewVersionButton(true);
			}
			break;
			
		case ServMgr::NT_PEERCAST:
			break;
	}
}
//------------------------------------------------------------------------
void OSXPeercastApp::channelStart(ChanInfo *pChanInfo)
{
	if( Channel *pChannel = chanMgr->findChannelByID( pChanInfo->id ) )
	{
		CEvent peercastEvent( kEventClassPeercast,  kEventStartChannel );
		
		TChanStatusPtr chanStatus( new ChanStatus( *pChannel ) );
		
		if( chanStatus.get() != NULL )
		{
			chanStatus->update( *pChannel );
			
			TChanXferPkt xferPkt( chanStatus.release() );
			
			peercastEvent.setParameter( kEventParamChanStatus, typeChanStatus, sizeof(TChanXferPkt), &xferPkt );
			peercastEvent.postToQueue( GetMainEventQueue(), kEventPriorityHigh );
		}
	}
}
//------------------------------------------------------------------------
void OSXPeercastApp::channelUpdate(ChanInfo *pChanInfo )
{
	if( Channel *pChannel = chanMgr->findChannelByID( pChanInfo->id ) )
	{
		CEvent peercastEvent( kEventClassPeercast, kEventUpdateChannel );
		
		TChanStatusPtr chanStatus( new ChanStatus( *pChannel ) );
		
		//if( chanStatus.valid() )
		if( chanStatus.get() != NULL )
		{
			chanStatus->update( *pChannel );
			
			TChanXferPkt xferPkt( chanStatus.release() );
			
			peercastEvent.setParameter( kEventParamChanStatus, typeChanStatus, sizeof(TChanXferPkt), &xferPkt );
			peercastEvent.postToQueue( GetMainEventQueue(), kEventPriorityHigh );
		}
	}
}
//------------------------------------------------------------------------
void OSXPeercastApp::channelStop(ChanInfo *pChanInfo)
{
	if( Channel *pChannel = chanMgr->findChannelByID( pChanInfo->id ) )
	{
		CEvent peercastEvent( kEventClassPeercast,  kEventStopChannel );
		
		peercastEvent.setParameter( kEventParamChanStatus, typeChanStatus, sizeof(GnuID), &pChanInfo->id );
		peercastEvent.postToQueue( GetMainEventQueue(), kEventPriorityHigh );
	}
}
//------------------------------------------------------------------------
OSStatus setItemDataText( DataBrowserItemDataRef itemData, const char* text )
{
	if( CFStringRef controlText = CFStringCreateWithFormat( NULL, NULL, CFSTR("%s"), text ) )
	{	
		OSStatus err = SetDataBrowserItemDataText( itemData, controlText );
		CFRelease( controlText );
	
		return err;
	}
	
	return noErr;
}
//------------------------------------------------------------------------
OSStatus setItemDataInteger( DataBrowserItemDataRef itemData, const int value )
{
	if( CFStringRef controlText = CFStringCreateWithFormat( NULL, NULL, CFSTR("%d"), value ) )
	{	
		OSStatus err = SetDataBrowserItemDataText( itemData, controlText );
		CFRelease( controlText );
	
		return err;
	}
	
	return noErr;
}
//------------------------------------------------------------------------
OSStatus OSXPeercastApp::itemDataCallback( ControlRef browser, 
						   DataBrowserItemID itemID,
						   DataBrowserPropertyID property,
						   DataBrowserItemDataRef itemData,
						   Boolean changeValue )
{
	char workBuffer[64];
	OSStatus status = noErr;
	
	if( !changeValue )
	{
		ChanStatus *pChanStatus = reinterpret_cast<ChanStatus *>(itemID);
			
		switch( property )
		{
			case kChannelColumn:
				return setItemDataText( itemData, pChanStatus->getName().c_str() );

			case kBitRateColumn:
				return setItemDataInteger( itemData, pChanStatus->getBitrate() );
				
			case kStreamColumn:
				return setItemDataText( itemData, pChanStatus->getStreamType().c_str() );
				
			case kRelayColumn:
				{
					sprintf( workBuffer, "%d / %d", pChanStatus->totalListeners(), pChanStatus->totalRelays() );
					return setItemDataText( itemData, workBuffer );
				}
				
			case kListenersColumn:
				{
					sprintf( workBuffer, "%d / %d", pChanStatus->localListeners(), pChanStatus->localRelays() );
					return setItemDataText( itemData, workBuffer );
				}
				
			case kStatusColumn:
				return setItemDataText( itemData, pChanStatus->getStatus().c_str() );
				
			case kKeepColumn:
				return	SetDataBrowserItemDataButtonValue( itemData, pChanStatus->stayConnected() ? kThemeButtonOn : kThemeButtonOff );
		}
	}
	else
	{
		ChanStatus *pChanStatus = reinterpret_cast<ChanStatus *>(itemID);

		switch( property )
		{
			case kKeepColumn:
				{
					ThemeButtonValue buttonValue;
					status = GetDataBrowserItemDataButtonValue( itemData, &buttonValue );
						
					if( status == noErr )
					{
						pChanStatus->stayConnected() = (buttonValue==kThemeButtonOn);

						Channel *pChannel = chanMgr->findChannelByID( pChanStatus->getId() );
			
						if( pChannel )
						{
							pChannel->stayConnected = pChanStatus->stayConnected();
						}					
					}
					return SetDataBrowserItemDataButtonValue( itemData, buttonValue );
				}
				break;
		}			
	}

	return errDataBrowserPropertyNotSupported;
}

OSStatus OSXPeercastApp::redisplayInfo()
{
	showStationStats();
	
	return noErr;
}

void OSXPeercastApp::showStationStats()
{
	if( !servMgr || !chanMgr )
		return;
		
	char workBuffer[256];
	String upTimeString;
	
	upTimeString.setFromStopwatch(servMgr->getUptime());
	mUpTime.setText( mWindowRef, upTimeString.cstr() );
	
	mDirectConnections.setIntValue( mWindowRef, servMgr->numStreams( Servent::T_DIRECT, true ) );
	mRelayConnections.setIntValue( mWindowRef, servMgr->numStreams( Servent::T_RELAY, true ) );
	
	sprintf( workBuffer, "%d / %d", servMgr->numConnected(Servent::T_CIN), servMgr->numConnected(Servent::T_COUT) );
	mCinCoutConnections.setText( mWindowRef, workBuffer );
	mPGNUConnections.setIntValue( mWindowRef, servMgr->numConnected(Servent::T_PGNU) );
	mIncomingConnections.setIntValue( mWindowRef, servMgr->numActive(Servent::T_INCOMING));
}

void OSXPeercastApp::handleButtonEvent( TButtonCallback fButtonCallback )
{
	if( Handle selectedItems = NewHandle(0) )
	{
		OSStatus err = mRelayBrowser.getItems( kDataBrowserNoItem, false, kDataBrowserItemIsSelected, selectedItems );
		
		if( err == noErr )
		{
			const UInt32 numSelectedItems = GetHandleSize(selectedItems) / sizeof(DataBrowserItemID);
			
			if( numSelectedItems > 0 )
			{
				(*this.*fButtonCallback)( selectedItems, numSelectedItems );
			}
		}
		
		DisposeHandle( selectedItems );
	}
}

void OSXPeercastApp::commentClicked( const Handle selectedItems, const UInt32 numSelectedItems )
{
	char strId[128];
	char urlBuf[256];
	
	const DataBrowserItemID *pItemIDList = reinterpret_cast<const DataBrowserItemID *>(selectedItems[0]);

	for(int chIndex=0; chIndex<numSelectedItems; ++chIndex)
	{
		ChanStatus *pChannel = reinterpret_cast<ChanStatus*>(static_cast<DataBrowserItemID>(pItemIDList[chIndex]));
		
		if( pChannel )
		{
			String channelName;
			channelName.set( pChannel->getName().c_str(), String::T_ASCII );
			channelName.convertTo( String::T_ESC );

			pChannel->getId().toStr( strId );
			
			sprintf( urlBuf, "%schat.php?cid=%s&cn=%s", skYPString, strId, channelName.cstr() );

			sys->getURL( urlBuf );
		}
	}
}

void OSXPeercastApp::bumpClicked( const Handle selectedItems, const UInt32 numSelectedItems )
{
	const DataBrowserItemID *pItemIDList = reinterpret_cast<const DataBrowserItemID *>(selectedItems[0]);

	for(int chIndex=0; chIndex<numSelectedItems; ++chIndex)
	{
		ChanStatus *pChanStatus = reinterpret_cast<ChanStatus*>(static_cast<DataBrowserItemID>(pItemIDList[chIndex]));
		
		if( pChanStatus )
		{
			mLock.on();
			Channel *pChannel = chanMgr->findChannelByID( pChanStatus->getId() );
			
			if( pChannel )
			{
				pChannel->bump = true;
			}
			mLock.off();
		}
	}
}

void OSXPeercastApp::itemNotificationCallback( ControlRef browser,
												  DataBrowserItemID itemID,
												  DataBrowserItemNotification message )
{
	OSStatus status = noErr;
	
	WindowRef window = GetControlOwner( browser );
	
	switch( message )
	{
		case kDataBrowserItemSelected:
			{
				ControlRef bumpButton;
				ControlRef commentButton;
				
				GetControlByID( window, &skBumpButtonID, &bumpButton );
				GetControlByID( window, &skCommentButtonID, &commentButton );
				
				if( bumpButton )
				{				
					EnableControl( bumpButton );
				}
				
				if( commentButton )
				{
					EnableControl( commentButton );
				}
			}
			break;
			
		case kDataBrowserItemDeselected:
			{
				UInt32 numSelected;
				GetDataBrowserItemCount( browser, kDataBrowserNoItem, false, kDataBrowserItemIsSelected, &numSelected );
				
				if( numSelected == 0 )
				{
					ControlRef bumpButton;
					ControlRef commentButton;
				
					GetControlByID( window, &skBumpButtonID, &bumpButton );
					GetControlByID( window, &skCommentButtonID, &commentButton );
				
					if( bumpButton )
					{				
						DisableControl( bumpButton );
					}
				
					if( commentButton )
					{
						DisableControl( commentButton );
					}
				}
			}
			break;
	}
}