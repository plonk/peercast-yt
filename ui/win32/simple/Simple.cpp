// ------------------------------------------------
// File : simple.cpp
// Date: 4-apr-2002
// Author: giles
// Desc: 
//		Simple tray icon interface to PeerCast, mostly win32 related stuff.
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
#include <direct.h> 
#include "stdafx.h"
#include "resource.h"
#include "gui.h"
#include "channel.h"
#include "servent.h"
#include "servmgr.h"
#include "win32/wsys.h"
#include "peercast.h"
#include "simple.h"
#include "version2.h"

#define MAX_LOADSTRING 100

#define PLAY_CMD 7000
#define RELAY_CMD 8000
#define INFO_CMD 9000
#define URL_CMD 10000

#define MAX_CHANNELS 999


extern "C"
{
	void loadIcons(HINSTANCE hInstance, HWND hWnd);
};


// PeerCast globals

static int currNotify=0;
String iniFileName;
HWND guiWnd;
HWND mainWnd;
static HMENU trayMenu,ltrayMenu;
bool showGUI=true;
bool allowMulti=false;
bool killMe=false;
bool allowTrayMenu=true;
int		seenNewVersionTime=0;
HICON icon1,icon2;
ChanInfo chanInfo;
bool chanInfoIsRelayed;
GnuID	lastPlayID;
String exePath;

// ---------------------------------
Sys * APICALL MyPeercastInst::createSys()
{
	return new WSys(mainWnd);
}
// ---------------------------------
const char * APICALL MyPeercastApp ::getIniFilename()
{
	return iniFileName.cstr();
}

// ---------------------------------
const char *APICALL MyPeercastApp ::getClientTypeOS() 
{
	return PCX_OS_WIN32;
}

// ---------------------------------
const char * APICALL MyPeercastApp::getPath()
{
	return exePath.cstr();
}



class NOTIFYICONDATA2
{
public:
        DWORD cbSize; // DWORD
        HWND hWnd; // HWND
        UINT uID; // UINT
        UINT uFlags; // UINT
        UINT uCallbackMessage; // UINT
        HICON hIcon; // HICON
        char szTip[128]; // char[128]
        DWORD dwState; // DWORD
        DWORD dwStateMask; // DWORD
        char szInfo[256]; // char[256]
        UINT uTimeoutOrVersion; // UINT
        char szInfoTitle[64]; // char[64]
        DWORD dwInfoFlags; // DWORD
        //GUID guidItem; > IE 6
};

NOTIFYICONDATA2 trayIcon;


// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];								// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];								// The title bar text

// Foward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	ChanInfoProc(HWND, UINT, WPARAM, LPARAM);

void setTrayIcon(int type, const char *,const char *,bool);
void flipNotifyPopup(int id, ServMgr::NOTIFY_TYPE nt);


HWND chWnd=NULL;

// --------------------------------------------------
void LOG2(const char *fmt,...)
{
	va_list ap;
  	va_start(ap, fmt);
	char str[4096];
	vsprintf(str,fmt,ap);
	OutputDebugString(str);
   	va_end(ap);	
}



// ---------------------------------------

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	char tmpURL[8192];
	tmpURL[0]=0;
	char *chanURL=NULL;


	iniFileName.set(".\\peercast.ini");

	// off by default now
	showGUI = false;

	if (strlen(lpCmdLine) > 0)
	{
		char *p;
		if ((p = strstr(lpCmdLine,"-inifile"))!=NULL) 
			iniFileName.setFromString(p+8);

		if (strstr(lpCmdLine,"-zen")) 
			showGUI = false;

		if (strstr(lpCmdLine,"-multi")) 
			allowMulti = true;

		if (strstr(lpCmdLine,"-kill")) 
			killMe = true;

		if ((p = strstr(lpCmdLine,"-url"))!=NULL)
		{
			p+=4;
			while (*p)
			{
				if (*p=='"')
				{
					p++;
					break;
				}				
				if (*p != ' ')
					break;
				p++;
			}
			if (*p)
				strncpy(tmpURL,p,sizeof(tmpURL)-1);
		}
	}

	// get current path
	{
		exePath = iniFileName;
		char *s = exePath.cstr();
		char *end = NULL;
		while (*s)
		{
			if (*s++ == '\\')
				end = s;
		}
		if (end)
			*end = 0;
	}

	
	if (strnicmp(tmpURL,"peercast://",11)==0)
	{
		if (strnicmp(tmpURL+11,"pls/",4)==0)
			chanURL = tmpURL+11+4;
		else
			chanURL = tmpURL+11;
		showGUI = false;
	}


	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	//LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	//LoadString(hInstance, IDC_APP_TITLE, szWindowClass, MAX_LOADSTRING);

	strcpy(szTitle,"PeerCast");
	strcpy(szWindowClass,"PeerCast");

	if (!allowMulti)
	{
		HANDLE mutex = CreateMutex(NULL,TRUE,szWindowClass);
		
		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			HWND oldWin = FindWindow(szWindowClass,NULL);
			if (oldWin)
			{
				if (killMe)
				{
					SendMessage(oldWin,WM_DESTROY,0,0);
					return 0;
				}

				if (chanURL)
				{
					COPYDATASTRUCT copy;
					copy.dwData = WM_PLAYCHANNEL;
					copy.cbData = strlen(chanURL)+1;			// plus null term
					copy.lpData = chanURL;
					SendMessage(oldWin,WM_COPYDATA,NULL,(LPARAM)&copy);
				}else{
					if (showGUI)
						SendMessage(oldWin,WM_SHOWGUI,0,0);
				}
			}
			return 0;
		}
	}

	if (killMe)
		return 0;
	
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
		return FALSE;

	peercastInst = new MyPeercastInst();
	peercastApp = new MyPeercastApp();

	peercastInst->init();




	if (chanURL)
	{
		ChanInfo info;
		servMgr->procConnectArgs(chanURL,info);
		chanMgr->findAndPlayChannel(info,false);
	}


	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_SIMPLE);

	// setup menu notifes
	int mask = peercastInst->getNotifyMask();
	if (mask & ServMgr::NT_PEERCAST)
		CheckMenuItem(trayMenu,ID_POPUP_SHOWMESSAGES_PEERCAST,MF_CHECKED|MF_BYCOMMAND);
	if (mask & ServMgr::NT_BROADCASTERS)
		CheckMenuItem(trayMenu,ID_POPUP_SHOWMESSAGES_BROADCASTERS,MF_CHECKED|MF_BYCOMMAND);
	if (mask & ServMgr::NT_TRACKINFO)
		CheckMenuItem(trayMenu,ID_POPUP_SHOWMESSAGES_TRACKINFO,MF_CHECKED|MF_BYCOMMAND);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

    Shell_NotifyIcon(NIM_DELETE, (NOTIFYICONDATA*)&trayIcon);

	peercastInst->saveSettings();
	peercastInst->quit();


	return msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_SIMPLE);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCSTR)IDC_SIMPLE;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);

}

//-----------------------------
void loadIcons(HINSTANCE hInstance, HWND hWnd)
{
	icon1 = LoadIcon(hInstance, (LPCTSTR)IDI_SMALL);
	icon2 = LoadIcon(hInstance, (LPCTSTR)IDI_SMALL2);

    trayIcon.cbSize = sizeof(trayIcon);
    trayIcon.hWnd = hWnd;
    trayIcon.uID = 100;
    trayIcon.uFlags = NIF_MESSAGE + NIF_ICON + NIF_TIP;
    trayIcon.uCallbackMessage = WM_TRAYICON;
    trayIcon.hIcon = icon1;
    strcpy(trayIcon.szTip, "PeerCast");

    Shell_NotifyIcon(NIM_ADD, (NOTIFYICONDATA*)&trayIcon);

    //ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

	trayMenu = LoadMenu(hInstance,MAKEINTRESOURCE(IDR_TRAYMENU));
	ltrayMenu = LoadMenu(hInstance,MAKEINTRESOURCE(IDR_LTRAYMENU));


}

//-----------------------------
//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
	  CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
	  return FALSE;
	}

	mainWnd = hWnd;

	loadIcons(hInstance,hWnd);

	return TRUE;
}
//-----------------------------
static String trackTitle;
static String channelComment;

//-----------------------------
void channelPopup(const char *title, const char *msg)
{
	String both;

	both.append(msg);
	both.append(" (");
	both.append(title);
	both.append(")");

	trayIcon.uFlags = NIF_ICON|NIF_TIP;
	strncpy(trayIcon.szTip, both.cstr(),sizeof(trayIcon.szTip)-1);
	trayIcon.szTip[sizeof(trayIcon.szTip)-1]=0;

	trayIcon.uFlags |= 16;
	trayIcon.uTimeoutOrVersion = 10000;
	strncpy(trayIcon.szInfo,msg,sizeof(trayIcon.szInfo)-1);
	strncpy(trayIcon.szInfoTitle,title,sizeof(trayIcon.szInfoTitle)-1);
		
	Shell_NotifyIcon(NIM_MODIFY, (NOTIFYICONDATA*)&trayIcon);
}
//-----------------------------
void clearChannelPopup()
{
	trayIcon.uFlags = NIF_ICON|16;
	trayIcon.uTimeoutOrVersion = 10000;
    strncpy(trayIcon.szInfo,"",sizeof(trayIcon.szInfo)-1);
	strncpy(trayIcon.szInfoTitle,"",sizeof(trayIcon.szInfoTitle)-1);
	Shell_NotifyIcon(NIM_MODIFY, (NOTIFYICONDATA*)&trayIcon);
}

//-----------------------------
void	APICALL MyPeercastApp::channelStart(ChanInfo *info)
{
	lastPlayID = info->id;
	clearChannelPopup();
}
//-----------------------------
void	APICALL MyPeercastApp::channelStop(ChanInfo *info)
{
	if (info->id.isSame(lastPlayID))
	{
		lastPlayID.clear();
		clearChannelPopup();
	}
}
//-----------------------------
void	APICALL MyPeercastApp::channelUpdate(ChanInfo *info)
{
	if (lastPlayID.isSet() && info && info->id.isSame(lastPlayID))
	{

		String tmp;
		tmp.append(info->track.artist);
		tmp.append(" ");
		tmp.append(info->track.title);


		if (!tmp.isSame(trackTitle))
		{
			if (ServMgr::NT_TRACKINFO & peercastInst->getNotifyMask())
			{
				trackTitle=tmp;
				clearChannelPopup();
				channelPopup(info->name.cstr(),trackTitle.cstr());
			}
		} else if (!info->comment.isSame(channelComment))
		{
			if (ServMgr::NT_BROADCASTERS & peercastInst->getNotifyMask())
			{
				channelComment = info->comment;
				clearChannelPopup();
				channelPopup(info->name.cstr(),channelComment.cstr());
			}
		}



	}
}
//-----------------------------
void	APICALL MyPeercastApp::notifyMessage(ServMgr::NOTIFY_TYPE type, const char *msg)
{
	static bool shownUpgradeAlert=false;

	currNotify = type;

	trayIcon.uFlags = 0;

	if (!shownUpgradeAlert)
	{
	    trayIcon.uFlags = NIF_ICON;

		if (type == ServMgr::NT_UPGRADE)
		{
			shownUpgradeAlert = true;
		    trayIcon.hIcon = icon2;
		}else
		{
			trayIcon.hIcon = icon1;	
		}
	}else
	{
		if (type == ServMgr::NT_UPGRADE)
			return;

	}

	const char *title="";

	switch(type)
	{
		case ServMgr::NT_UPGRADE:
			title = "Upgrade alert";
			break;
		case ServMgr::NT_PEERCAST:
			title = "Message from PeerCast:";
			break;

	}

	if (type & peercastInst->getNotifyMask())
	{
		trayIcon.uFlags |= 16;
		trayIcon.uTimeoutOrVersion = 10000;
		strncpy(trayIcon.szInfo,msg,sizeof(trayIcon.szInfo)-1);
		strncpy(trayIcon.szInfoTitle,title,sizeof(trayIcon.szInfoTitle)-1);
	    Shell_NotifyIcon(NIM_MODIFY, (NOTIFYICONDATA*)&trayIcon);
	}
}
//-----------------------------

// createGUI()
//
void createGUI(HWND hWnd)
{
	if (!guiWnd)
		guiWnd = CreateDialog(hInst, (LPCTSTR)IDD_MAINWINDOW, hWnd, (DLGPROC)GUIProc);
	ShowWindow(guiWnd,SW_SHOWNORMAL);
}


// 
// addRelayedChannelsMenu(HMENU m)
// 
//
void addRelayedChannelsMenu(HMENU cm)
{
	int cnt = GetMenuItemCount(cm);
	for(int i=0; i<cnt-3; i++)
		DeleteMenu(cm,0,MF_BYPOSITION);

	Channel *c = chanMgr->channel;
	while(c)
	{
		if (c->isActive())
		{
			char str[128],name[64];
			strncpy(name,c->info.name,32);
			name[32]=0;
			if (strlen(c->info.name) > 32)
				strcat(name,"...");


			sprintf(str,"%s  (%d kb/s %s)",name,c->info.bitrate,ChanInfo::getTypeStr(c->info.contentType));
			//InsertMenu(cm,0,MF_BYPOSITION,RELAY_CMD+i,str);
		}
		c=c->next;
	}
}

typedef int (*COMPARE_FUNC)(const void *,const void *);

static int compareHitLists(ChanHitList **c2, ChanHitList **c1)
{
	return stricmp(c1[0]->info.name.cstr(),c2[0]->info.name.cstr());
}

static int compareChannels(Channel **c2, Channel **c1)
{
	return stricmp(c1[0]->info.name.cstr(),c2[0]->info.name.cstr());
}

// 
// addAllChannelsMenu(HMENU m)
// 
//
void addAllChannelsMenu(HMENU cm)
{
	int cnt = GetMenuItemCount(cm);
	for(int i=0; i<cnt-2; i++)
		DeleteMenu(cm,0,MF_BYPOSITION);





	// add channels to menu
	int numActive=0;
	Channel *ch = chanMgr->channel;
	while(ch)
	{
		char str[128],name[64];
		strncpy(name,ch->info.name,32);
		name[32]=0;
		if (strlen(ch->info.name) > 32)
			strcat(name,"...");

		sprintf(str,"%s  (%d kb/s %s)",name,ch->info.bitrate,ChanInfo::getTypeStr(ch->info.contentType));

		HMENU opMenu = CreatePopupMenu();
		InsertMenu(opMenu,0,MF_BYPOSITION,INFO_CMD+numActive,"Info");
		if (ch->info.url.isValidURL())
			InsertMenu(opMenu,0,MF_BYPOSITION,URL_CMD+numActive,"URL");
		InsertMenu(opMenu,0,MF_BYPOSITION,PLAY_CMD+numActive,"Play");

		UINT fl = MF_BYPOSITION|MF_POPUP;
		if (ch)
			fl |= (ch->isPlaying()?MF_CHECKED:0);

		InsertMenu(cm,0,fl,(UINT)opMenu,str);
		
		numActive++;

		ch=ch->next;
	}


	//if (!numActive)
	//		InsertMenu(cm,0,MF_BYPOSITION,0,"<No channels>");

}


// 
// flipNotifyPopup(id, flag)
void flipNotifyPopup(int id, ServMgr::NOTIFY_TYPE nt)
{
	int mask = peercastInst->getNotifyMask();

	mask ^= nt;
	if (mask & nt)
		CheckMenuItem(trayMenu,id,MF_CHECKED|MF_BYCOMMAND);
	else
		CheckMenuItem(trayMenu,id,MF_UNCHECKED|MF_BYCOMMAND);

	peercastInst->setNotifyMask(mask);
	peercastInst->saveSettings();
}
 

static void showHTML(const char *file)
{
	char url[256];
	sprintf(url,"%s/%s",servMgr->htmlPath,file);					

	sys->callLocalURL(url,servMgr->serverHost.port);
}

static ChanInfo getChannelInfo(int index)
{
	Channel *c = chanMgr->findChannelByIndex(index);
	if (c)
		return c->info;

	ChanInfo info;
	return info;
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	POINT point;


	switch (message) 
	{
		case WM_SHOWGUI:
			createGUI(hWnd);
			break;


		case WM_TRAYICON:
			switch((UINT)lParam) 
			{
				case WM_LBUTTONDOWN:
					if (allowTrayMenu)
						SendMessage(hWnd,WM_SHOWMENU,2,0);
					SetForegroundWindow(hWnd);    
					break;
				case WM_RBUTTONDOWN:
					if (allowTrayMenu)
						SendMessage(hWnd,WM_SHOWMENU,1,0);
					SetForegroundWindow(hWnd);    
					break;
			}
			break;

		case WM_COPYDATA:
			{
				COPYDATASTRUCT *pc = (COPYDATASTRUCT *)lParam;
				LOG_DEBUG("URL request: %s",pc->lpData);
				if (pc->dwData == WM_PLAYCHANNEL)
				{
					ChanInfo info;
					servMgr->procConnectArgs((char *)pc->lpData,info);
					chanMgr->findAndPlayChannel(info,false);
				}
				//sys->callLocalURL((const char *)pc->lpData,servMgr->serverHost.port);
			}
			break;
		case WM_GETPORTNUMBER:
			{
				int port;
				port=servMgr->serverHost.port;
				ReplyMessage(port);
			}
			break;

		case WM_SHOWMENU:
			{
				SetForegroundWindow(hWnd);    
				bool skipMenu=false;

				allowTrayMenu = false;

				// check for notifications
				if (currNotify & ServMgr::NT_UPGRADE)
				{
					if (servMgr->downloadURL[0])
					{
						if ((sys->getTime()-seenNewVersionTime) > (60*60))	// notify every hour
						{
							if (MessageBox(hWnd,"A newer version of PeerCast is available, press OK to upgrade.","PeerCast",MB_OKCANCEL|MB_APPLMODAL|MB_ICONEXCLAMATION) == IDOK)
								sys->getURL(servMgr->downloadURL);

							seenNewVersionTime=sys->getTime();
							skipMenu=true;
						}
					}
				}


				if (!skipMenu)
				{
					GetCursorPos(&point);
					HMENU menu;

					switch (wParam)
					{
						case 1:
							menu = GetSubMenu(trayMenu,0);
							addAllChannelsMenu(GetSubMenu(menu,0));
							addRelayedChannelsMenu(GetSubMenu(menu,1));
							break;
						case 2:
							menu = GetSubMenu(ltrayMenu,0);
							addAllChannelsMenu(menu);
							break;
					}
					if (!TrackPopupMenu(menu,TPM_RIGHTALIGN,point.x,point.y,0,hWnd,NULL))
					{
						LOG_ERROR("Can`t track popup menu: %d",GetLastError());
					}
					PostMessage(hWnd,WM_NULL,0,0); 

				}
				allowTrayMenu = true;
			}
			break;

		case WM_CREATE:
			if (showGUI)
				createGUI(hWnd);
			break;

		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 

			if ((wmId >= INFO_CMD) && (wmId < INFO_CMD+MAX_CHANNELS))
			{
				int c = wmId - INFO_CMD;
				chanInfo = getChannelInfo(c);
				chanInfoIsRelayed = false;
				DialogBox(hInst, (LPCTSTR)IDD_CHANINFO, hWnd, (DLGPROC)ChanInfoProc);
				return 0;
			}
			if ((wmId >= URL_CMD) && (wmId < URL_CMD+MAX_CHANNELS))
			{
				int c = wmId - URL_CMD;
				chanInfo = getChannelInfo(c);
				if (chanInfo.url.isValidURL())
					sys->getURL(chanInfo.url);
				return 0;
			}
			if ((wmId >= PLAY_CMD) && (wmId < PLAY_CMD+MAX_CHANNELS))
			{
				int c = wmId - PLAY_CMD;
				chanInfo = getChannelInfo(c);
				chanMgr->findAndPlayChannel(chanInfo,false);
				return 0;
			}
			if ((wmId >= RELAY_CMD) && (wmId < RELAY_CMD+MAX_CHANNELS))
			{
				int c = wmId - RELAY_CMD;
				chanInfo = getChannelInfo(c);
				chanMgr->findAndPlayChannel(chanInfo,true);
				return 0;
			}

			// Parse the menu selections:
			switch (wmId)
			{
				case ID_POPUP_SHOWMESSAGES_PEERCAST:
					flipNotifyPopup(ID_POPUP_SHOWMESSAGES_PEERCAST,ServMgr::NT_PEERCAST);
					break;
				case ID_POPUP_SHOWMESSAGES_BROADCASTERS:
					flipNotifyPopup(ID_POPUP_SHOWMESSAGES_BROADCASTERS,ServMgr::NT_BROADCASTERS);
					break;
				case ID_POPUP_SHOWMESSAGES_TRACKINFO:
					flipNotifyPopup(ID_POPUP_SHOWMESSAGES_TRACKINFO,ServMgr::NT_TRACKINFO);
					break;

				case ID_POPUP_ABOUT:
				case IDM_ABOUT:
					DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
					break;
				case ID_POPUP_SHOWGUI:
				case IDM_SETTINGS_GUI:
				case ID_POPUP_ADVANCED_SHOWGUI:
				{
					createGUI(hWnd);
					break;
				}
				case ID_POPUP_YELLOWPAGES:
					sys->getURL("http://yp.peercast.org/");
					break;

				case ID_POPUP_ADVANCED_VIEWLOG:
					showHTML("viewlog.html");
					break;
				case ID_POPUP_ADVANCED_SAVESETTINGS:
					servMgr->saveSettings(iniFileName.cstr());
					break;
				case ID_POPUP_ADVANCED_INFORMATION:
					showHTML("index.html");
					break;
				case ID_FIND_CHANNELS:
				case ID_POPUP_ADVANCED_ALLCHANNELS:
				case ID_POPUP_UPGRADE:
					sys->callLocalURL("admin?cmd=upgrade",servMgr->serverHost.port);
					break;
				case ID_POPUP_ADVANCED_RELAYEDCHANNELS:
				case ID_POPUP_FAVORITES_EDIT:
					showHTML("relays.html");
					break;
				case ID_POPUP_ADVANCED_BROADCAST:
					showHTML("broadcast.html");
					break;
				case ID_POPUP_SETTINGS:
					showHTML("settings.html");
					break;
				case ID_POPUP_CONNECTIONS:
					showHTML("connections.html");
					break;
				case ID_POPUP_HELP:
					sys->getURL("http://www.peercast.org/help.php");
					break;

				case ID_POPUP_EXIT_CONFIRM:
				case IDM_EXIT:
				   DestroyWindow(hWnd);
				   break;
				default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}
// Mesage handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hDlg,IDC_ABOUTVER,WM_SETTEXT,0,(LONG)PCX_AGENT);
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
				case IDCANCEL:
					EndDialog(hDlg, LOWORD(wParam));
					return TRUE;
				case IDC_BUTTON1:
					sys->getURL("http://www.peercast.org");
					EndDialog(hDlg, LOWORD(wParam));
					return TRUE;

			}
			break;
		case WM_DESTROY:
			break;
	}
    return FALSE;
}

// Mesage handler for chaninfo box
LRESULT CALLBACK ChanInfoProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			{
				char str[1024];
				strcpy(str,chanInfo.track.artist.cstr());
				strcat(str," - ");
				strcat(str,chanInfo.track.title.cstr());
				
				SendDlgItemMessage(hDlg,IDC_EDIT_NAME,WM_SETTEXT,0,(LONG)chanInfo.name.cstr());
				SendDlgItemMessage(hDlg,IDC_EDIT_PLAYING,WM_SETTEXT,0,(LONG)str);
				SendDlgItemMessage(hDlg,IDC_EDIT_MESSAGE,WM_SETTEXT,0,(LONG)chanInfo.comment.cstr());
				SendDlgItemMessage(hDlg,IDC_EDIT_DESC,WM_SETTEXT,0,(LONG)chanInfo.desc.cstr());
				SendDlgItemMessage(hDlg,IDC_EDIT_GENRE,WM_SETTEXT,0,(LONG)chanInfo.genre.cstr());

				sprintf(str,"%d kb/s %s",chanInfo.bitrate,ChanInfo::getTypeStr(chanInfo.contentType));
				SendDlgItemMessage(hDlg,IDC_FORMAT,WM_SETTEXT,0,(LONG)str);


				if (!chanInfo.url.isValidURL())
					EnableWindow(GetDlgItem(hDlg,IDC_CONTACT),false);

				Channel *ch = chanMgr->findChannelByID(chanInfo.id);
				if (ch)
				{
					SendDlgItemMessage(hDlg,IDC_EDIT_STATUS,WM_SETTEXT,0,(LONG)ch->getStatusStr());
					SendDlgItemMessage(hDlg, IDC_KEEP,BM_SETCHECK, ch->stayConnected, 0);
				}else
				{
					SendDlgItemMessage(hDlg,IDC_EDIT_STATUS,WM_SETTEXT,0,(LONG)"OK");
					EnableWindow(GetDlgItem(hDlg,IDC_KEEP),false);
				}



				POINT point;
				RECT rect,drect;
				HWND hDsk = GetDesktopWindow();
				GetWindowRect(hDsk,&drect);
				GetWindowRect(hDlg,&rect);
				GetCursorPos(&point);

				POINT pos,size;
				size.x = rect.right-rect.left;
				size.y = rect.bottom-rect.top;

				if (point.x-drect.left < size.x)
					pos.x = point.x;
				else
					pos.x = point.x-size.x;

				if (point.y-drect.top < size.y)
					pos.y = point.y;
				else
					pos.y = point.y-size.y;

				SetWindowPos(hDlg,HWND_TOPMOST,pos.x,pos.y,size.x,size.y,0);
				chWnd = hDlg;
			}
			return TRUE;

		case WM_COMMAND:
			{
				char str[1024],idstr[64];
				chanInfo.id.toStr(idstr);

				switch (LOWORD(wParam))
				{
					case IDC_CONTACT:
					{
						sys->getURL(chanInfo.url);
						return TRUE;
					}
					case IDC_DETAILS:
					{
						sprintf(str,"admin?page=chaninfo&id=%s&relay=%d",idstr,chanInfoIsRelayed);
						sys->callLocalURL(str,servMgr->serverHost.port);
						return TRUE;
					}
					case IDC_KEEP:
					{
						Channel *ch = chanMgr->findChannelByID(chanInfo.id);
						if (ch)
							ch->stayConnected = SendDlgItemMessage(hDlg, IDC_KEEP,BM_GETCHECK, 0, 0) == BST_CHECKED;;
						return TRUE;
					}


					case IDC_PLAY:
					{
						chanMgr->findAndPlayChannel(chanInfo,false);
						return TRUE;
					}

				}
			}
			break;

		case WM_CLOSE:
			EndDialog(hDlg, 0);
			break;

		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_INACTIVE)
				EndDialog(hDlg, 0);
			break;
		case WM_DESTROY:
			chWnd = NULL;
			break;


	}
    return FALSE;
}
