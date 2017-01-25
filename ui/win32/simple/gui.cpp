// ------------------------------------------------
// File : gui.cpp
// Date: 4-apr-2002
// Author: giles
// Desc: 
//		Windows front end GUI, PeerCast core is not dependant on any of this. 
//		Its very messy at the moment, but then again Windows UI always is.
//		I really don`t like programming win32 UI.. I want my borland back..
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

#include <windows.h>
#include "stdio.h"
#include "string.h"
#include "stdarg.h"
#include "resource.h"
#include "socket.h"
#include "win32/wsys.h"
#include "servent.h"
#include "win32/wsocket.h"
#include "inifile.h"
#include "gui.h"
#include "servmgr.h"
#include "peercast.h"
#include "simple.h"

ThreadInfo guiThread;
bool shownChannels=false;

// --------------------------------------------------
int logID,statusID,hitID,chanID;

// --------------------------------------------------
bool getButtonState(int id)
{
	return SendDlgItemMessage(guiWnd, id,BM_GETCHECK, 0, 0) == BST_CHECKED;
}

// --------------------------------------------------
void enableControl(int id, bool on)
{
	EnableWindow(GetDlgItem(guiWnd,id),on);
}

// --------------------------------------------------
void enableEdit(int id, bool on)
{
	SendDlgItemMessage(guiWnd, id,WM_ENABLE, on, 0);
	SendDlgItemMessage(guiWnd, id,EM_SETREADONLY, !on, 0);
}
// --------------------------------------------------
int getEditInt(int id)
{
	char str[128];
	SendDlgItemMessage(guiWnd, id,WM_GETTEXT, 128, (LONG)str);
	return atoi(str);
}
// --------------------------------------------------
char * getEditStr(int id)
{
	static char str[128];
	SendDlgItemMessage(guiWnd, id,WM_GETTEXT, 128, (LONG)str);
	return str;
}
// --------------------------------------------------
void setEditStr(int id, char *str)
{
	SendDlgItemMessage(guiWnd, id,WM_SETTEXT, 0, (LONG)str);
}
// --------------------------------------------------
void setEditInt(int id, int v)
{
	char str[128];
	sprintf(str,"%d",v);
	SendDlgItemMessage(guiWnd, id,WM_SETTEXT, 0, (LONG)str);
}

// --------------------------------------------------
void *getListBoxSelData(int id)
{
	int sel = SendDlgItemMessage(guiWnd, id,LB_GETCURSEL, 0, 0);
	if (sel >= 0)
		return (void *)SendDlgItemMessage(guiWnd, id,LB_GETITEMDATA, sel, 0);
	return NULL;
}


// --------------------------------------------------
void setButtonState(int id, bool on)
{
	SendDlgItemMessage(guiWnd, id,BM_SETCHECK, on, 0);
	SendMessage(guiWnd,WM_COMMAND,id,0);
}
// --------------------------------------------------
void ADDLOG(const char *str,int id,bool sel,void *data, LogBuffer::TYPE type)
{
	if (guiWnd)
	{

		int num = SendDlgItemMessage(guiWnd, id,LB_GETCOUNT, 0, 0);
		if (num > 100)
		{
			SendDlgItemMessage(guiWnd, id, LB_DELETESTRING, 0, 0);
			num--;
		}
		int idx = SendDlgItemMessage(guiWnd, id, LB_ADDSTRING, 0, (LONG)(LPSTR)str);
		SendDlgItemMessage(guiWnd, id, LB_SETITEMDATA, idx, (LONG)data);

		if (sel)
			SendDlgItemMessage(guiWnd, id, LB_SETCURSEL, num, 0);

	}

	if (type != LogBuffer::T_NONE)
	{
#if _DEBUG
		OutputDebugString(str);
		OutputDebugString("\n");
#endif
	}

}


// --------------------------------------------------
void ADDLOG2(const char *fmt,va_list ap,int id,bool sel,void *data, LogBuffer::TYPE type)
{
	char str[4096];
	vsprintf(str,fmt,ap);

	ADDLOG(str,id,sel,data,type);
}

// --------------------------------------------------
void ADDCHAN(void *d, const char *fmt,...)
{
	va_list ap;
  	va_start(ap, fmt);
	ADDLOG2(fmt,ap,chanID,false,d,LogBuffer::T_NONE);
   	va_end(ap);	
}
// --------------------------------------------------
void ADDHIT(void *d, const char *fmt,...)
{
	va_list ap;
  	va_start(ap, fmt);
	ADDLOG2(fmt,ap,hitID,false,d,LogBuffer::T_NONE);
   	va_end(ap);	
}
// --------------------------------------------------
void ADDCONN(void *d, const char *fmt,...)
{
	va_list ap;
  	va_start(ap, fmt);
	ADDLOG2(fmt,ap,statusID,false,d,LogBuffer::T_NONE);
   	va_end(ap);	
}
// --------------------------------------------------
THREAD_PROC showConnections(ThreadInfo *thread)
{
//	thread->lock();
	while (thread->active)
	{
		int sel,top,i;
		sel = SendDlgItemMessage(guiWnd, statusID,LB_GETCURSEL, 0, 0);
		top = SendDlgItemMessage(guiWnd, statusID,LB_GETTOPINDEX, 0, 0);

		SendDlgItemMessage(guiWnd, statusID, LB_RESETCONTENT, 0, 0);
		Servent *s = servMgr->servents;
		while (s)
		{
			if (s->type != Servent::T_NONE)
			{
				Host h = s->getHost();
				{
					unsigned int ip = h.ip;
					unsigned short port = h.port;

					Host h(ip,port);
					char hostName[64];
					h.toStr(hostName);

					unsigned int tnum = 0;
					char tdef = 's';
					if (s->lastConnect)
						tnum = sys->getTime()-s->lastConnect;

					if ((s->type == Servent::T_RELAY) || (s->type == Servent::T_DIRECT))
					{
						ADDCONN(s,"%s-%s-%d%c  -  %s  -  %d ",
							s->getTypeStr(),s->getStatusStr(),tnum,tdef,
							hostName,
							s->syncPos
							);
					}else{
						if (s->status == Servent::S_CONNECTED)
						{
							ADDCONN(s,"%s-%s-%d%c  -  %s  -  %d/%d",
								s->getTypeStr(),s->getStatusStr(),tnum,tdef,
								hostName,
								s->gnuStream.packetsIn,s->gnuStream.packetsOut);
						}else{
							ADDCONN(s,"%s-%s-%d%c  -  %s",
								s->getTypeStr(),s->getStatusStr(),tnum,tdef,
								hostName
								);
						}

					}

				}
			}
			s=s->next;
		}
		if (sel >= 0)
			SendDlgItemMessage(guiWnd, statusID,LB_SETCURSEL, sel, 0);
		if (top >= 0)
			SendDlgItemMessage(guiWnd, statusID,LB_SETTOPINDEX, top, 0);



		char cname[34];

		{
			sel = SendDlgItemMessage(guiWnd, chanID,LB_GETCURSEL, 0, 0);
			top = SendDlgItemMessage(guiWnd, chanID,LB_GETTOPINDEX, 0, 0);
			SendDlgItemMessage(guiWnd, chanID, LB_RESETCONTENT, 0, 0);

			Channel *c = chanMgr->channel;
			while (c)
			{
				if (c->isActive())
				{
					strncpy(cname,c->getName(),16);
					cname[16] = 0;
					ADDCHAN(c,"%s - %d kb/s - %s",cname,c->getBitrate(),c->getStatusStr());
				}
				c=c->next;
			}
			if (sel >= 0)
				SendDlgItemMessage(guiWnd, chanID,LB_SETCURSEL, sel, 0);
			if (top >= 0)
				SendDlgItemMessage(guiWnd, chanID,LB_SETTOPINDEX, top, 0);

		}



		bool update = ((sys->getTime() - chanMgr->lastHit) < 3)||(!shownChannels);

		if (update)
		{
			shownChannels = true;
			{
				sel = SendDlgItemMessage(guiWnd, hitID,LB_GETCURSEL, 0, 0);
				top = SendDlgItemMessage(guiWnd, hitID,LB_GETTOPINDEX, 0, 0);
				SendDlgItemMessage(guiWnd, hitID, LB_RESETCONTENT, 0, 0);
				ChanHitList *chl = chanMgr->hitlist;
				while (chl)
				{
					if (chl->isUsed())
					{
						if (chl->info.match(chanMgr->searchInfo))
						{
							strncpy(cname,chl->info.name.cstr(),16);
							cname[16] = 0;
							ADDHIT(chl,"%s - %d kb/s - %d/%d",cname,chl->info.bitrate,chl->numListeners(),chl->numHits());
						}
					}
					chl = chl->next;
				}
			}

			if (sel >= 0)
				SendDlgItemMessage(guiWnd, hitID,LB_SETCURSEL, sel, 0);
			if (top >= 0)
				SendDlgItemMessage(guiWnd, hitID,LB_SETTOPINDEX, top, 0);
		}




		{
			switch (servMgr->getFirewall())
			{
				case ServMgr::FW_ON:
					SendDlgItemMessage(guiWnd, IDC_EDIT4,WM_SETTEXT, 0, (LONG)"Firewalled");
					break;
				case ServMgr::FW_UNKNOWN:
					SendDlgItemMessage(guiWnd, IDC_EDIT4,WM_SETTEXT, 0, (LONG)"Unknown");
					break;
				case ServMgr::FW_OFF:
					SendDlgItemMessage(guiWnd, IDC_EDIT4,WM_SETTEXT, 0, (LONG)"Normal");
					break;
			}
		}

		// sleep for 1 second .. check every 1/10th for shutdown
		for(i=0; i<10; i++)
		{
			if (!thread->active)
				break;
			sys->sleep(100);
		}
	}

//	thread->unlock();
	return 0;
}


// --------------------------------------------------
void tryConnect()
{
#if 0
	ClientSocket sock;

	char tmp[32];

	char *sendStr = "GET / HTTP/1.1\n\n";

	try {
		sock.open("taiyo",80);
		sock.write(sendStr,strlen(sendStr));
		sock.read(tmp,32);
		LOG("Connected: %s",tmp);
	}catch(IOException &e)
	{
		LOG(e.msg);
	}
#endif
}


// ---------------------------------
void APICALL MyPeercastApp ::printLog(LogBuffer::TYPE t, const char *str)
{
	ADDLOG(str,logID,true,NULL,t);
	if (logFile.isOpen())
	{
		logFile.writeLine(str);
		logFile.flush();
	}
}


// --------------------------------------------------
static void setControls(bool fromGUI)
{
	if (!guiWnd)
		return;
	setEditInt(IDC_EDIT1,servMgr->serverHost.port);
	setEditStr(IDC_EDIT3,servMgr->password);
	setEditStr(IDC_EDIT9,chanMgr->broadcastMsg.cstr());
	setEditInt(IDC_MAXRELAYS,servMgr->maxRelays);

	setButtonState(IDC_CHECK11,chanMgr->broadcastMsg[0]!=0);

	setButtonState(IDC_LOGDEBUG,(servMgr->showLog&(1<<LogBuffer::T_DEBUG))!=0);
	setButtonState(IDC_LOGERRORS,(servMgr->showLog&(1<<LogBuffer::T_ERROR))!=0);
	setButtonState(IDC_LOGNETWORK,(servMgr->showLog&(1<<LogBuffer::T_NETWORK))!=0);
	setButtonState(IDC_LOGCHANNELS,(servMgr->showLog&(1<<LogBuffer::T_CHANNEL))!=0);

	setButtonState(IDC_CHECK9,servMgr->pauseLog);


	if (!fromGUI)
		setButtonState(IDC_CHECK1,servMgr->autoServe);


}
// --------------------------------------------------
void APICALL MyPeercastApp::updateSettings()
{
	setControls(true);
}

// --------------------------------------------------
LRESULT CALLBACK GUIProc (HWND hwnd, UINT message,
                                 WPARAM wParam, LPARAM lParam)
{

   switch (message)
   {
       case WM_INITDIALOG:
			guiWnd = hwnd;

			shownChannels = false;
			logID = IDC_LIST1;		// log
			statusID = IDC_LIST2;	// status
			hitID = IDC_LIST4;		// hit
			chanID = IDC_LIST3;		// channels

			enableControl(IDC_BUTTON8,false);
			enableControl(IDC_BUTTON11,false);
			enableControl(IDC_BUTTON10,false);
			
			peercastApp->updateSettings();

			if (servMgr->autoServe)
				setButtonState(IDC_CHECK1,true);
			if (servMgr->autoConnect)
				setButtonState(IDC_CHECK2,true);


			guiThread.func = showConnections;
			if (!sys->startThread(&guiThread))
			{
				MessageBox(hwnd,"Unable to start GUI","PeerCast",MB_OK|MB_ICONERROR);
				PostMessage(hwnd,WM_DESTROY,0,0);
			}

			break;

	  case WM_COMMAND:
			switch( wParam )
			{
				case IDC_CHECK1:		// start server
						if (getButtonState(IDC_CHECK1))
						{
							//SendDlgItemMessage(hwnd, IDC_CHECK1,WM_SETTEXT, 0, (LPARAM)"Deactivate");

							SendDlgItemMessage(hwnd, IDC_EDIT3,WM_GETTEXT, 64, (LONG)servMgr->password);

							servMgr->serverHost.port = (unsigned short)getEditInt(IDC_EDIT1);
							servMgr->setMaxRelays(getEditInt(IDC_MAXRELAYS));


							enableControl(IDC_EDIT1,false);
							enableControl(IDC_EDIT3,false);
							enableControl(IDC_MAXRELAYS,false);
							enableControl(IDC_BUTTON8,true);
							enableControl(IDC_BUTTON11,true);
							enableControl(IDC_BUTTON10,true);

							//writeSettings();
							servMgr->autoServe = true;

							setEditStr(IDC_CHECK1,"Enabled");


						}else{
							//SendDlgItemMessage(hwnd, IDC_CHECK1,WM_SETTEXT, 0, (LPARAM)"Activate");

							servMgr->autoServe = false;

							enableControl(IDC_EDIT1,true);
							enableControl(IDC_EDIT3,true);
							enableControl(IDC_MAXRELAYS,true);
							enableControl(IDC_BUTTON8,false);
							enableControl(IDC_BUTTON11,false);
							enableControl(IDC_BUTTON10,false);

							setEditStr(IDC_CHECK1,"Disabled");

						}
						setControls(true);

					break;
				case IDC_CHECK11:		// DJ message
					if (getButtonState(IDC_CHECK11))
					{
						enableControl(IDC_EDIT9,false);
						SendDlgItemMessage(hwnd, IDC_EDIT9,WM_GETTEXT, 128, (LONG)chanMgr->broadcastMsg.cstr());
					}else{
						enableControl(IDC_EDIT9,true);
						chanMgr->broadcastMsg.clear();
					}
					break;
				case IDC_LOGDEBUG:		// log debug
					servMgr->showLog = getButtonState(wParam) ? servMgr->showLog|(1<<LogBuffer::T_DEBUG) : servMgr->showLog&~(1<<LogBuffer::T_DEBUG);
					break;
				case IDC_LOGERRORS:		// log errors
					servMgr->showLog = getButtonState(wParam) ? servMgr->showLog|(1<<LogBuffer::T_ERROR) : servMgr->showLog&~(1<<LogBuffer::T_ERROR);
					break;
				case IDC_LOGNETWORK:		// log network
					servMgr->showLog = getButtonState(wParam) ? servMgr->showLog|(1<<LogBuffer::T_NETWORK) : servMgr->showLog&~(1<<LogBuffer::T_NETWORK);
					break;
				case IDC_LOGCHANNELS:		// log channels
					servMgr->showLog = getButtonState(wParam) ? servMgr->showLog|(1<<LogBuffer::T_CHANNEL) : servMgr->showLog&~(1<<LogBuffer::T_CHANNEL);
					break;
				case IDC_CHECK9:		// pause log
					servMgr->pauseLog = getButtonState(wParam);
					break;
				case IDC_CHECK2:		// start outgoing

					if (getButtonState(IDC_CHECK2))
					{

						SendDlgItemMessage(hwnd, IDC_COMBO1,WM_GETTEXT, 128, (LONG)servMgr->connectHost);
						servMgr->autoConnect = true;
						//SendDlgItemMessage(hwnd, IDC_CHECK2,WM_SETTEXT, 0, (LPARAM)"Disconnect");
						enableControl(IDC_COMBO1,false);
					}else{
						servMgr->autoConnect = false;
						//SendDlgItemMessage(hwnd, IDC_CHECK2,WM_SETTEXT, 0, (LPARAM)"Connect");
						enableControl(IDC_COMBO1,true);
					}
					break;
				case IDC_BUTTON11:		// broadcast
					{
						Host sh = servMgr->serverHost;
						if (sh.isValid())
						{
							char cmd[256];
							sprintf(cmd,"http://localhost:%d/admin?page=broadcast",sh.port);
							ShellExecute(hwnd, NULL, cmd, NULL, NULL, SW_SHOWNORMAL);
		
						}else{
							MessageBox(hwnd,"Server is not currently connected.\nPlease wait until you have a connection.","PeerCast",MB_OK);
						}
					}
					break;
				case IDC_BUTTON8:		// play selected
					{
						Channel *c = (Channel *)getListBoxSelData(chanID);
						if (c)
							chanMgr->playChannel(c->info);
					}
					break;
				case IDC_BUTTON7:		// advanced
					sys->callLocalURL("admin?page=settings",servMgr->serverHost.port);
					break;
				case IDC_BUTTON6:		// servent disconnect
					{
						Servent *s = (Servent *)getListBoxSelData(statusID);
						if (s)
							s->thread.active = false;
					}
					break;
				case IDC_BUTTON5:		// chan disconnect
					{
						Channel *c = (Channel *)getListBoxSelData(chanID);
						if (c)
							c->thread.active = false;
					}

					break;
				case IDC_BUTTON3:		// chan bump
					{
						Channel *c = (Channel *)getListBoxSelData(chanID);
						if (c)
							c->bump = true;
					}

					break;
				case IDC_BUTTON4:		// get channel 
					{
						ChanHitList *chl = (ChanHitList *)getListBoxSelData(hitID);
						if (chl)
						{
							if (!chanMgr->findChannelByID(chl->info.id))
							{
								Channel *c = chanMgr->createChannel(chl->info,NULL);
								if (c)
									c->startGet();
							}
						}else{
							MessageBox(hwnd,"Please select a channel","PeerCast",MB_OK);
						}
					}
					break;


				case IDC_BUTTON1:		// clear log
					SendDlgItemMessage(guiWnd, logID, LB_RESETCONTENT, 0, 0);
					break;

				case IDC_BUTTON2:		// find
					{
						char str[64];
						SendDlgItemMessage(hwnd, IDC_EDIT2,WM_GETTEXT, 64, (LONG)str);
						SendDlgItemMessage(hwnd, hitID, LB_RESETCONTENT, 0, 0);
						ChanInfo info;
						info.init();
						info.name.set(str);
						chanMgr->startSearch(info);
					}
					break;

			}
			break;

		case WM_CLOSE:
			DestroyWindow( hwnd );
			break;

		case WM_DESTROY:
			guiThread.active = false;
//			guiThread.lock();
			guiWnd = NULL;
//			guiThread.unlock();
			EndDialog(hwnd, LOWORD(wParam));
			break;

		//default:
			// do nothing
			//return DefDlgProc(hwnd, message, wParam, lParam);		// this recurses for ever
			//return DefWindowProc(hwnd, message, wParam, lParam);	// this stops window messages
   }
   return 0;
}
