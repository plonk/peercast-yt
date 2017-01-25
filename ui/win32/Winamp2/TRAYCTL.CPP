// Winamp general purpose plug-in mini-SDK
// Copyright (C) 1997, Justin Frankel/Nullsoft

#include <windows.h>
#include <process.h>
#include <TCHAR.H>


#include "gen.h"
#include "resource.h"

#include "win32/wsys.h"
#include "channel.h"
#include "servent.h"
#include "servmgr.h"
#include "peercast.h"





BOOL WINAPI _DllMainCRTStartup(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}

const char *ODDCAST_DLL = "dsp_oddcast.dll";

#define ENABLE_PREV 1
#define ENABLE_PLAY 2
#define ENABLE_STOP 4
#define ENABLE_NEXT 8
#define ENABLE_EJECT 16
int config_enabled=0;

bool autoPlaylistUpdate=true;

HICON Icon;

// from systray.c
extern "C"
{
BOOL systray_add(HWND hwnd, UINT uID, HICON hIcon, LPSTR lpszTip);
BOOL systray_del(HWND hwnd, UINT uID);
BOOL CALLBACK ConfigProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
__declspec( dllexport ) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin();
__declspec( dllexport ) playChannel(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow);
__declspec( dllexport ) callURL(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow);
};

int numRuns=0;
bool showChans=true;
char broadcastName[128];
char broadcastGenre[128];
char broadcastURL[128];
int broadcastQuality=0;
bool broadcastManual=false;
bool onAir=false;
char pluginFilename[256];
char iniFilename[256];
char peercastPath[256];

HWND configWin;

void config();
void quit();
int init();
void config_write();
void config_read();
const char *szAppName = "PeerCast";

winampGeneralPurposePlugin plugin =
{
	GPPHDR_VER,
	"",
	init,
	config,
	quit,
};

void main() {}

int logID = 0;


// ---------------------------------
class MyPeercastInst : public PeercastInstance
{
public:
	virtual Sys * APICALL createSys()
	{
		return new WSys(plugin.hwndParent);
	}
};
// ---------------------------------
class MyPeercastApp : public PeercastApplication
{
public:
	virtual const char * APICALL getIniFilename()
	{
		return iniFilename;
	}

	virtual const char * APICALL getPath() 
	{
		return peercastPath;
	}


	virtual const char *APICALL getClientTypeOS() 
	{
		return PCX_OS_WINAMP2;
	}

	virtual void APICALL addChannel(ChanInfo *);

};

#if 0
// --------------------------------------------------
void setSettingsUI()
{
}
// --------------------------------------------------
void setTrayIcon(int type, const char *title,const char *msg)
{
#if 0
	if ((type & Sys::NT_TRACKINFO) && (isPlaying))
	{

		titleStr.clear();
		if (strlen(msg) || strlen(title))
		{
			if (title)
				titleStr.append(title);
			titleStr.append(" ");
			if (msg)
				titleStr.append(msg);
		}else
			titleStr.append("Untitled");

	}
#endif
}
#endif

// --------------------------------------------------
bool getButtonState(int id)
{
	return SendDlgItemMessage(configWin, id,BM_GETCHECK, 0, 0) == BST_CHECKED;
}
// --------------------------------------------------
void setButtonState(int id, bool on)
{
	SendDlgItemMessage(configWin, id,BM_SETCHECK, on, 0);
	SendMessage(configWin,WM_COMMAND,id,0);
}
// --------------------------------------------------
void enableControl(int id, bool on)
{
	EnableWindow(GetDlgItem(configWin,id),on);
}

// --------------------------------------------------
void enableEdit(int id, bool on)
{
	SendDlgItemMessage(configWin, id,WM_ENABLE, on, 0);
	SendDlgItemMessage(configWin, id,EM_SETREADONLY, !on, 0);
}
// --------------------------------------------------
int getEditInt(int id)
{
	char str[128];
	SendDlgItemMessage(configWin, id,WM_GETTEXT, 128, (LONG)str);
	return atoi(str);
}
// --------------------------------------------------
char * getEditStr(int id)
{
	static char str[128];
	SendDlgItemMessage(configWin, id,WM_GETTEXT, 128, (LONG)str);
	return str;
}
// --------------------------------------------------
void setEditStr(int id, char *str)
{
	SendDlgItemMessage(configWin, id,WM_SETTEXT, 0, (LONG)str);
}
// --------------------------------------------------
void setEditInt(int id, int v)
{
	char str[128];
	sprintf(str,"%d",v);
	SendDlgItemMessage(configWin, id,WM_SETTEXT, 0, (LONG)str);
}



void addPlaylist(const char *url, const char *id)
{
	int num = SendMessage(plugin.hwndParent,WM_USER, 0, 124); 	// num tracks

	for(int i=0; i<num; i++)
	{
		char *name = (char *)SendMessage(plugin.hwndParent,WM_USER, i, 211); 	// filename
		if (name) // if already in list, dont add
		{
			if (id)
				if (strstr(name,id))
					return;

			if (strcmp(url,name)==0)
				return;		
		}
	}

	
	COPYDATASTRUCT cds;
	cds.dwData = 100;		// IPC_PLAYFILE
	cds.cbData = lstrlen(url) + 1;
	cds.lpData = (void *) url;
	SendMessage(plugin.hwndParent, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
}

void delPlaylist(const char *url)
{
	int num = SendMessage(plugin.hwndParent,WM_USER, 0, 124); 	// num tracks

	for(int i=0; i<num; i++)
	{
		char *name = (char *)SendMessage(plugin.hwndParent,WM_USER, i, 211); 	// num tracks
		if (name)
			if (strcmp(url,name)==0)
			{
			// cant do it aparently.
			}
	}	
}

// -----------------------------------
void APICALL MyPeercastApp::addChannel(ChanInfo *info)
{
#if 0
	if ((autoPlaylistUpdate) && (info))
	{
		FileStream file;

		try
		{
			file.openWriteReplace("c:\\add.pls");

			char url[256];
			sprintf(url,"http://localhost:%d",peercastInst->getServerPort());
			PlayList *pls = new PlayList(PlayList::T_SCPLS,1);
			pls->addChannel(url,*info);
			pls->write(file);
			file.close();
			addPlaylist("c:\\add.pls",NULL);
			delete pls;

			SendMessage(plugin.hwndParent,WM_USER, (long)"", 245); 	// set pls

#if 0
			char str[256];
			char idStr[64];
			info->id.toStr(idStr);
			sprintf(str,"http://localhost:%d/scpls/%s.pls",peercastInst->getServerPort(),idStr);
			addPlaylist(str,idStr);
#endif

		}catch(StreamException &e)
		{
			LOG_ERROR("Unable to add Winamp pls: %s",e.msg);
		}

	}
#endif
}
// -----------------------------------
void updatePlaylist()
{
}

#if 0
int playlistThread(ThreadInfo *t)
{
	while (t->active)
	{
		if (autoPlaylistUpdate)
		{
			for(int i=0; i<ChanMgr::MAX_HITLISTS; i++)
			{
				ChanHitList *chl = &chanMgr->hitlists[i];
				if (chl->isUsed())
				{
					char str[256];
					char idStr[64];
					chl->info.id.toStr(idStr);
					sprintf(str,"http://localhost:%d/scpls/%s.pls",peercastInst->getServerPort(),idStr);
					addPlaylist(str,idStr);
				}
			}
		}
		sys->sleep(10000);
	}
	return 0;
}
#endif

void showMiniBrowser()
{

	if (showChans)
	{
		if (!numRuns)
			SendMessage(plugin.hwndParent,WM_USER, NULL, 249); 	

		char url[256];
		sprintf(url,"http://localhost:%d/admin?page=winamp-chans",peercastInst->getServerPort());
		SendMessage(plugin.hwndParent,WM_USER, (long)url, 249); // = 241 
		SendMessage(plugin.hwndParent,WM_USER, 1, 248); 	// block updates
		SetForegroundWindow(plugin.hwndParent);    
	}
}
void config()
{
	//sys->callLocalURL("admin?page=settings",servMgr->serverHost.port);
	DialogBox(plugin.hDllInstance,MAKEINTRESOURCE(IDD_DIALOG1),plugin.hwndParent,ConfigProc);
//	showMiniBrowser();

#if 0
	SendMessage(plugin.hwndParent,WM_USER, NULL, 249); 	
	char url[256];
	sprintf(url,"http://localhost:%d/admin?page=winamp-settings",servMgr->serverHost.port);
	SendMessage(plugin.hwndParent,WM_USER, (long)url, 249); // = 241 
	SendMessage(plugin.hwndParent,WM_USER, 1, 248); 	// block updates
	SetForegroundWindow(plugin.hwndParent);    
#endif
}

void quit()
{
	numRuns++;
	peercastInst->saveSettings();
	config_write();
	config_enabled=0;
	//systray_del(plugin.hwndParent,0);
}


void *lpWndProcOld;
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	
	switch(message)
	{
		case WM_COPYDATA:
			{
				COPYDATASTRUCT *pc = (COPYDATASTRUCT *)lParam;
				if (pc->dwData == WM_USER+1001)
				{
					try
					{
						LOG_DEBUG("Play request: %s",pc->lpData);
						ChanInfo info;
						servMgr->procConnectArgs((char *)pc->lpData,info);
						chanMgr->findAndPlayChannel(info,false);
					}catch(StreamException &e)
					{
						LOG_ERROR("Playing channel: %s",e.msg);
					}
					return 0;
				}
			}
			break;

		case WM_USER+100:
			ReplyMessage(peercastInst->getServerPort());
			return 0;
			break;
	}

#if 0
	if (message == WM_USER+27)
	{
		int which = LOWORD(wParam) - 1024;
		switch (LOWORD(lParam))
		{
			case WM_LBUTTONDOWN:
#if 0
#endif
				//addPlaylist("http://localhost:8144/pls/*.pls");
				//showMiniBrowser();
				break;
#if 0
			case WM_LBUTTONDOWN:
				switch (which)
				{
					case 0:
						if ((a=SendMessage(hwnd,WM_USER,0,104)) == 0) // not playing, let's 
																  // hit prev
						{
							SendMessage(hwnd,WM_COMMAND,40044,0);
						}
						else if (a != 3 && SendMessage(hwnd,WM_USER,0,105) > 2000) // restart
						{
							SendMessage(hwnd,WM_COMMAND,40045,0);
						} else { // prev
							SendMessage(hwnd,WM_COMMAND,40044,0);
						}
					return 0;
					case 1:
						if ((a=SendMessage(hwnd,WM_USER,0,104)) != 1) // not playing, let's 
																  // hit play
						{
							SendMessage(hwnd,WM_COMMAND,40045,0);
						}
						else { // prev
							SendMessage(hwnd,WM_COMMAND,40046,0);
						}
					return 0;
					case 2:
						if (GetKeyState(VK_SHIFT) & (1<<15))
							SendMessage(hwnd,WM_COMMAND,40147,0);
						else
							SendMessage(hwnd,WM_COMMAND,40047,0);
					return 0;
					case 3:
						SendMessage(hwnd,WM_COMMAND,40048,0);
					return 0;
					case 4:
						SetForegroundWindow(hwnd);
						if (GetKeyState(VK_CONTROL) & (1<<15))
							SendMessage(hwnd,WM_COMMAND,40185,0);
						else if (GetKeyState(VK_SHIFT) & (1<<15))
							SendMessage(hwnd,WM_COMMAND,40187,0);
						else
							SendMessage(hwnd,WM_COMMAND,40029,0);
					return 0;
				}
			return 0;
#endif
		}
	}
#endif
	return CallWindowProc((WNDPROC)lpWndProcOld,hwnd,message,wParam,lParam);
}

// ---------------------------------------------------------------------------
BOOL CALLBACK ChannelsProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	return false;
}
	
// ---------------------------------------------------------------------------
int init()
{
	
	static char c[512];
	char *p;
	GetModuleFileName(plugin.hDllInstance,pluginFilename,sizeof(pluginFilename));
	p = pluginFilename+lstrlen(pluginFilename);
	while (p >= pluginFilename && *p != '\\') p--;
	wsprintf((plugin.description=c),"%s (%s)",szAppName,p+1);


	strcpy(peercastPath,pluginFilename);
	p=peercastPath+lstrlen(peercastPath);
	while (p >= peercastPath && *p != '\\') p--;
	if (++p >= peercastPath) *p = 0;

	lstrcat(peercastPath,"peercast/");

	lstrcpy(iniFilename,peercastPath);
	lstrcat(iniFilename,"peercast.ini");



	peercastInst = new MyPeercastInst();
	peercastApp = new MyPeercastApp();

	peercastInst->init();

	broadcastName[0]=0;

	config_read();

	if (!numRuns)
		config();

	showMiniBrowser();


	lpWndProcOld = (void *) GetWindowLong(plugin.hwndParent,GWL_WNDPROC);
	SetWindowLong(plugin.hwndParent,GWL_WNDPROC,(long)WndProc);

	//Icon = LoadIcon(plugin.hDllInstance,MAKEINTRESOURCE(IDI_ICON6));
//	systray_add(plugin.hwndParent,0,Icon,"PeerCast");


//	static ThreadInfo plsThread;

//	plsThread.func = playlistThread;
//	sys->startThread(&plsThread);


	//config();

#if 0
	char pfPath[256];
	HKEY	hKey;
	DWORD dwType, dwSize;

	LRESULT lnResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("\\Software\\Microsoft\\Windows\\CurrentVersion"),0, KEY_READ,&hKey);
	if (lnResult == ERROR_SUCCESS)
	{

		dwType = REG_SZ;
		dwSize = sizeof(pfPath);

		RegQueryValueEx(hKey, TEXT("ProgramFilesDir"), NULL, &dwType, (PBYTE)&pfPath, &dwSize);

		LOG("PATH %s",pfPath);
	}else
		LOG("PATH not found");

#endif

	return 0;
}
// -----------------------------------------------------
void setOnAirControls(bool on)
{
	enableControl(IDC_EDIT2,on);
	enableControl(IDC_EDIT3,on);
	enableControl(IDC_EDIT4,on);

	enableControl(IDC_RADIO1,on);
	enableControl(IDC_RADIO2,on);
	enableControl(IDC_RADIO3,on);
}

static bool oldOnAir = false;
// -----------------------------------------------------
static void setControls(bool fromGUI)
{
	setEditInt(IDC_MAXRELAYS,peercastInst->getMaxRelays());
	if (!fromGUI)
		setButtonState(IDC_ACTIVE,peercastInst->getActive());
}

// -----------------------------------------------------
BOOL CALLBACK ConfigProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			{
				configWin = hwndDlg;
				setControls(false);
			}
			return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_BUTTON1:
					peercastInst->callLocalURL("admin?page=settings");
					break;

				case IDC_ACTIVE:
					peercastInst->setActive(getButtonState(IDC_ACTIVE));

					if (getButtonState(IDC_ACTIVE))
					{
						peercastInst->setMaxRelays(getEditInt(IDC_MAXRELAYS));
						setEditStr(IDC_ACTIVE,"Enabled");
						enableControl(IDC_MAXRELAYS,false);
					}else
					{
						setEditStr(IDC_ACTIVE,"Disabled");
						enableControl(IDC_MAXRELAYS,true);
					}
					setControls(true);

					break;


				case IDC_APPLY:
				case IDC_OK:
					peercastInst->setMaxRelays(getEditInt(IDC_MAXRELAYS));
					peercastInst->saveSettings();
					config_write();


					if (LOWORD(wParam)==IDC_OK)
					{
						EndDialog(hwndDlg,0);
					}
					return FALSE;

				case IDC_CANCEL:
					EndDialog(hwndDlg,0);
					return FALSE;
			}
			break;
		case WM_CLOSE:
			EndDialog(hwndDlg,0);
			return FALSE;

	}
	return FALSE;
}


void getIniFile(char *ini_file)
{
	char *p;
	strcpy(ini_file,pluginFilename);
	p=ini_file+lstrlen(ini_file);
	while (p >= ini_file && *p != '\\') p--;
	if (++p >= ini_file) *p = 0;
	lstrcat(ini_file,"plugin.ini");
}

void config_read()
{	
	char ini_file[MAX_PATH];
	getIniFile(ini_file);

	autoPlaylistUpdate = GetPrivateProfileInt(szAppName,"autoPlaylistUpdate",autoPlaylistUpdate?1:0,ini_file)!=0;
	broadcastQuality = GetPrivateProfileInt(szAppName,"broadcastQuality",broadcastQuality,ini_file);
	onAir = GetPrivateProfileInt(szAppName,"onAir",onAir?1:0,ini_file)!=0;
	broadcastManual = GetPrivateProfileInt(szAppName,"broadcastManual",broadcastManual?1:0,ini_file)!=0;
	showChans = GetPrivateProfileInt(szAppName,"showChans",showChans?1:0,ini_file)!=0;
	numRuns = GetPrivateProfileInt(szAppName,"numRuns",numRuns,ini_file);

	GetPrivateProfileString(ODDCAST_DLL,"gServDesc","",broadcastName,sizeof(broadcastName),ini_file);
	GetPrivateProfileString(ODDCAST_DLL,"gServGenre","",broadcastGenre,sizeof(broadcastGenre),ini_file);
	GetPrivateProfileString(ODDCAST_DLL,"gServURL","",broadcastURL,sizeof(broadcastURL),ini_file);

}

void config_write()
{
	char ini_file[MAX_PATH],string[128];

	getIniFile(ini_file);

	wsprintf(string,"%d",autoPlaylistUpdate);
	WritePrivateProfileString(szAppName,"autoPlaylistUpdate",string,ini_file);
	wsprintf(string,"%d",broadcastQuality);
	WritePrivateProfileString(szAppName,"broadcastQuality",string,ini_file);
	wsprintf(string,"%d",onAir);
	WritePrivateProfileString(szAppName,"onAir",string,ini_file);
	wsprintf(string,"%d",broadcastManual);
	WritePrivateProfileString(szAppName,"broadcastManual",string,ini_file);
	wsprintf(string,"%d",showChans);
	WritePrivateProfileString(szAppName,"showChans",string,ini_file);
	wsprintf(string,"%d",numRuns);
	WritePrivateProfileString(szAppName,"numRuns",string,ini_file);

	if (!broadcastManual)
	{
		
		WritePrivateProfileString(ODDCAST_DLL,"gServer","localhost",ini_file);

		WritePrivateProfileString(ODDCAST_DLL,"gServDesc",broadcastName,ini_file);
		WritePrivateProfileString(ODDCAST_DLL,"gServGenre",broadcastGenre,ini_file);
		WritePrivateProfileString(ODDCAST_DLL,"gServURL",broadcastURL,ini_file);

		wsprintf(string,"%d",peercastInst->getServerPort());
		WritePrivateProfileString(ODDCAST_DLL,"gPort",string,ini_file);
		WritePrivateProfileString(ODDCAST_DLL,"gPassword",peercastInst->getServerPassword(),ini_file);

		WritePrivateProfileString(ODDCAST_DLL,"gOggFlag","1",ini_file);
		WritePrivateProfileString(ODDCAST_DLL,"gLAMEFlag","0",ini_file);

		wsprintf(string,"%d",broadcastQuality);
		
		WritePrivateProfileString(ODDCAST_DLL,"gOggQuality",string,ini_file);

		WritePrivateProfileString(ODDCAST_DLL,"gOggBitQual","0",ini_file);

		WritePrivateProfileString(ODDCAST_DLL,"gShoutcastFlag","FALSE",ini_file);
		WritePrivateProfileString(ODDCAST_DLL,"gIcecastFlag","FALSE",ini_file);
		WritePrivateProfileString(ODDCAST_DLL,"gIcecast2Flag","TRUE",ini_file);	

		WritePrivateProfileString(ODDCAST_DLL,"gMountpoint","/oddcast.ogg",ini_file);	

		WritePrivateProfileString(ODDCAST_DLL,"gAutoStart",onAir?"TRUE":"FALSE",ini_file);	
	}

}

__declspec( dllexport ) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin()
{
	return &plugin;
}


int getPort()
{
	int port = DEFAULT_PORT;
	HWND hwndWinamp = FindWindow("Winamp v1.x",NULL); 
	if (hwndWinamp)
		port=SendMessage(hwndWinamp,WM_USER+100,0,0);
	return port;
}

__declspec( dllexport ) playChannel(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
	WSys *wsys = new WSys(hwnd);

	char str[256];
	sprintf(str,"pls/%s",lpszCmdLine);
	wsys->callLocalURL(str,getPort());

}

__declspec( dllexport ) callURL(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
	char tmpBuf[8192];
	strcpy(tmpBuf,lpszCmdLine);

	if (strnicmp(tmpBuf,"peercast://",11)==0)
	{
		char *chanURL=NULL;

		if (strnicmp(tmpBuf+11,"pls/",4)==0)
			chanURL = tmpBuf+11+4;
		else
			chanURL = tmpBuf+11;

		COPYDATASTRUCT copy;
		copy.dwData = WM_USER+1001;
		copy.cbData = strlen(chanURL)+1;			// plus null term
		copy.lpData = chanURL;

		HWND hwndWinamp = FindWindow("Winamp v1.x",NULL); 
		if (hwndWinamp)
			SendMessage(hwndWinamp,WM_COPYDATA,NULL,(LPARAM)&copy);
	}
}
