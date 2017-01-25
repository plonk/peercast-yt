/*
 *  osxapp.h
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
 
#ifndef _OSXAPP_H
#define _OSXAPP_H

#include <Carbon/Carbon.h>
#include <list>
#include <memory>
#include <string>
#include "unix/usys.h"
#include "peercast.h"
#include "databrowser.h"
#include "textcontrol.h"
#include "numericcontrol.h"
#include "tabcontrol.h"
#include "carbonwraps.h"
// ---------------------------------
class OSXPeercastInst : public PeercastInstance
{
public:
	virtual Sys * APICALL createSys()
	{
		return new USys();
	}
};
// ---------------------------------
class OSXPeercastApp : public PeercastApplication
{
public:
	explicit OSXPeercastApp( const char* appPath, const char *iniFilename, int argc, char* argv[], WindowRef window, const ControlID& dbControlID );
	
	virtual const char * APICALL getIniFilename()
	{
		return mIniFileName;
	}

	virtual const char *APICALL getClientTypeOS() 
	{
		return PCX_OS_MACOSX;
	}
	
	virtual const char * APICALL getPath() 
	{ 
		return mAppPath.c_str();
	}

	virtual void APICALL printLog(LogBuffer::TYPE t, const char *str)
	{
		if (t != LogBuffer::T_NONE)
			printf("[%s] ",LogBuffer::getTypeStr(t));
		printf("%s\n",str);
	}
	
	virtual void	APICALL notifyMessage(ServMgr::NOTIFY_TYPE, const char *);
	virtual void	APICALL channelStart (ChanInfo *);
	virtual void	APICALL channelStop  (ChanInfo *);
	virtual void	APICALL channelUpdate(ChanInfo *);
	
	void initAPP();
	
	OSStatus redisplayInfo();
	
	void saveSettings() { mTabControl.saveSettings(); }
	
	void onCommentClicked()
	{
		handleButtonEvent( &commentClicked );
	}
	
	void onBumpClicked()
	{
		handleButtonEvent( &bumpClicked );
	}	
	
	void enableNewVersionButton( const Boolean visible );
	
private:

	typedef void (OSXPeercastApp::*TButtonCallback)( const Handle selectedItems, const UInt32 numSelectedItems );
	
	OSStatus installAppEventHandler     ();
	OSStatus installWindowHandler		( WindowRef window );
	OSStatus installPeercastEventHandler( WindowRef window );
	
	static OSStatus appEventHandler     ( EventHandlerCallRef inCallRef, EventRef inEvent, void* inUserData );
	static OSStatus mainWindowHandler   ( EventHandlerCallRef inCallRef, EventRef inEvent, void* inUserData );
	static OSStatus peercastEventHandler( EventHandlerCallRef inCallRef, EventRef inEvent, void* inUserData );

	static OSStatus itemDataCallback( ControlRef browser, 
									  DataBrowserItemID itemID,
									  DataBrowserPropertyID property,
									  DataBrowserItemDataRef itemData,
									  Boolean changeValue );
	static void itemNotificationCallback( ControlRef browser,
										  DataBrowserItemID itemID,
										  DataBrowserItemNotification message );

						   
	void setString     ( const int chanId, const int index, const char* text );
	void setInt		   ( const int chanId, const int index, const int value );
	int  buildChannelDB();	
	void showStationStats();

	void handleButtonEvent( TButtonCallback fButtonCallback );

	void commentClicked( const Handle selectedItems, const UInt32 numSelectedItems );
	void bumpClicked   ( const Handle selectedItems, const UInt32 numSelectedItems );
	
	class ChanStatus
	{
	public:
		explicit ChanStatus()
		:mId            ( )
		,mName          ( )
		,mStreamType	( )
		,mStatus		( )
		,mBitrate       ( )
		,mTotalListeners( 0 )
		,mTotalRelays   ( 0 )
		,mLocalListeners( 0 )
		,mLocalRelays   ( 0 )
		{
		}
		
		explicit ChanStatus( const Channel& channel )
		:mId            ( channel.getID() )
		,mName          ( channel.getName() )
		,mStreamType	( ChanInfo::getTypeStr(channel.info.contentType) )
		,mStatus		( )
		,mBitrate       ( channel.getBitrate() )
		,mTotalListeners( 0 )
		,mTotalRelays   ( 0 )
		,mLocalListeners( 0 )
		,mLocalRelays   ( 0 )
		{
		}
		
		void update( const Channel& channel )
		{
			mName           = channel.getName();
			mStreamType     = ChanInfo::getTypeStr(channel.info.contentType);
			mStatus         = channel.getStatusStr();
			mBitrate        = channel.getBitrate();
			mTotalListeners = channel.totalListeners();
			mTotalRelays    = channel.totalRelays();
			mLocalListeners = channel.localListeners();
			mLocalRelays    = channel.localRelays();
			mStayConnected  = channel.stayConnected;
		}
		
		void update( const ChanStatus& channel )
		{
			mName           = channel.getName();
			mStreamType     = channel.getStreamType();
			mStatus         = channel.getStatus();
			mBitrate        = channel.getBitrate();
			mTotalListeners = channel.totalListeners();
			mTotalRelays    = channel.totalRelays();
			mLocalListeners = channel.localListeners();
			mLocalRelays    = channel.localRelays();
			mStayConnected  = channel.stayConnected();
		}
		
		int&  totalListeners() { return mTotalListeners; }
		int&  totalRelays   () { return mTotalRelays; }
		int&  localListeners() { return mLocalListeners; }
		int&  localRelays   () { return mLocalRelays; }		
		bool& stayConnected () { return mStayConnected; }
		
		int  totalListeners() const { return mTotalListeners; }
		int  totalRelays   () const { return mTotalRelays; }
		int  localListeners() const { return mLocalListeners; }
		int  localRelays   () const { return mLocalRelays; }		
		bool stayConnected () const { return mStayConnected; }
		
		int getBitrate     () const { return mBitrate; }
		const std::string& getName() const { return mName; }
		const std::string& getStreamType() const { return mStreamType; }
		const std::string& getStatus() const { return mStatus; }
			  GnuID& getId()       { return mId; }
		const GnuID& getId() const { return mId; }
		
	private:
		// disable copy and assignment
		ChanStatus           ( const ChanStatus& );
		ChanStatus& operator=( const ChanStatus& );
	
		GnuID       mId;
		std::string mName;
		std::string mStreamType;
		std::string mStatus;
		int		    mBitrate;
		int			mTotalListeners;
		int			mTotalRelays;
		int			mLocalListeners;
		int			mLocalRelays;
		bool		mStayConnected;
	};

	template<typename T>
	class TTransferPkt
	{
	public:
		explicit TTransferPkt()
		:mpData( NULL )
		{
		}
	
		explicit TTransferPkt(T* pData )
		:mpData( pData )
		{
		}
	
		T* mpData;
	};
	
	typedef std::auto_ptr<ChanStatus> TChanStatusPtr;	
	typedef std::list<TChanStatusPtr> TChannelList;
	typedef TTransferPkt<ChanStatus>  TChanXferPkt;

	
	void addChanStatus   ( TChanStatusPtr& chanStatus );
	void updateChanStatus( ChanStatus& newStatus );
	void removeChanStatus( GnuID& chanId );
	
	const TChannelList::iterator findChannel( GnuID& chanId )
	{
		SearchId searchId( chanId );
		return std::find_if( mChannelList.begin(), mChannelList.end(), searchId );
	}

	class SearchId {
     public:
		explicit SearchId( const GnuID& chanId )
		:mSearchId( chanId )
		{
		}
		
		bool operator()(TChanStatusPtr& chanStatus) { return chanStatus->getId().isSame(mSearchId); }
     
	 private:
		GnuID mSearchId;
	 };
	
	String		   mIniFileName;
	DataBrowser    mRelayBrowser;
	WindowRef      mWindowRef;
	TextControl    mUpTime;
	NumericControl mDirectConnections;
	NumericControl mRelayConnections;
	TextControl    mCinCoutConnections;
	NumericControl mPGNUConnections;
	NumericControl mIncomingConnections;
	TChannelList   mChannelList;
	WLock			 mLock;
	TabControl		 mTabControl;
	CEventHandlerUPP mAppEventHandler;
	CEventHandlerUPP mMainWindowHandler;
	CEventHandlerUPP mPeercastEventHandler;
	std::string      mAppPath;
};

#endif // OSXAPP_H