
// ------------------------------------------------
// File : simple.h
// Date: 4-apr-2002
// Author: giles
//		
// (c) 2002 peercast.org
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

#if !defined(AFX_SIMPLE_H__F2E64B1B_62DE_473C_A6B6_E7826D41E0FA__INCLUDED_)
#define AFX_SIMPLE_H__F2E64B1B_62DE_473C_A6B6_E7826D41E0FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"

// ---------------------------------
class MyPeercastInst : public PeercastInstance
{
public:
	virtual Sys * APICALL createSys();
};
// ---------------------------------
class MyPeercastApp : public PeercastApplication
{
public:
	MyPeercastApp ()
	{
		//logFile.openWriteReplace("log.txt");
	}

	virtual const char * APICALL getPath();

	virtual const char * APICALL getIniFilename();
	virtual const char *APICALL getClientTypeOS();
	virtual void APICALL printLog(LogBuffer::TYPE t, const char *str);

	virtual void	APICALL updateSettings();
	virtual void	APICALL notifyMessage(ServMgr::NOTIFY_TYPE, const char *);

	virtual void	APICALL channelStart(ChanInfo *);
	virtual void	APICALL channelStop(ChanInfo *);
	virtual void	APICALL channelUpdate(ChanInfo *);

	FileStream	logFile;

};


#endif // !defined(AFX_SIMPLE_H__F2E64B1B_62DE_473C_A6B6_E7826D41E0FA__INCLUDED_)
