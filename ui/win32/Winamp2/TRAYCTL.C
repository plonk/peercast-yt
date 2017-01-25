// Winamp general purpose plug-in mini-SDK
// Copyright (C) 1997, Justin Frankel/Nullsoft

#include <windows.h>
#include <process.h>

#include "gen.h"
#include "resource.h"

#include "sys.h"
#include "channel.h"
#include "servent.h"
#include "servmgr.h"



BOOL WINAPI _DllMainCRTStartup(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}

#define ENABLE_PREV 1
#define ENABLE_PLAY 2
#define ENABLE_STOP 4
#define ENABLE_NEXT 8
#define ENABLE_EJECT 16
int config_enabled=0;

HICON Icon;

// from systray.c
extern BOOL systray_add(HWND hwnd, UINT uID, HICON hIcon, LPSTR lpszTip);
extern BOOL systray_del(HWND hwnd, UINT uID);

BOOL CALLBACK ConfigProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
void config();
void quit();
int init();
void config_write();
void config_read();
char szAppName[] = "Nullsoft Tray Control";

winampGeneralPurposePlugin plugin =
{
	GPPHDR_VER,
	"",
	init,
	config,
	quit,
};

void main() {}



void config()
{
	DialogBox(plugin.hDllInstance,MAKEINTRESOURCE(IDD_DIALOG1),plugin.hwndParent,ConfigProc);
}

void quit()
{
	config_write();
	config_enabled=0;
	systray_del(plugin.hwndParent,0);
}

void *lpWndProcOld;
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_USER+27)
	{
		int which = LOWORD(wParam) - 1024,a;
		switch (LOWORD(lParam))
		{
			case WM_LBUTTONDOWN:
				config();
				SetForegroundWindow(plugin.hwndParent);    
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
	return CallWindowProc(lpWndProcOld,hwnd,message,wParam,lParam);
}

int init()
{
	{
		static char c[512];
		char filename[512],*p;
		GetModuleFileName(plugin.hDllInstance,filename,sizeof(filename));
		p = filename+lstrlen(filename);
		while (p >= filename && *p != '\\') p--;
		wsprintf((plugin.description=c),"%s Plug-In v0.1 (%s)",szAppName,p+1);
	}
	config_read();
	{
		Icon = LoadIcon(plugin.hDllInstance,MAKEINTRESOURCE(IDI_ICON6));
	}


	lpWndProcOld = (void *) GetWindowLong(plugin.hwndParent,GWL_WNDPROC);
	SetWindowLong(plugin.hwndParent,GWL_WNDPROC,WndProc);

	systray_add(plugin.hwndParent,0,Icon,"PeerCast");

	
	return 0;
}



BOOL CALLBACK ConfigProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			{
				int i;
				for (i = 0; i < 5; i++)
					CheckDlgButton(hwndDlg,IDC_PREV+i,(config_enabled&(1<<i))?BST_CHECKED:BST_UNCHECKED);
			}
		return FALSE;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_APPLY:
				case IDOK:

					config_enabled=0;
					{
						int i;
						for (i = 0; i < 5; i++)
							if (IsDlgButtonChecked(hwndDlg,IDC_PREV+i))
								config_enabled |= 1<<i;
					}
				case IDCANCEL:
					if (LOWORD(wParam) != IDC_APPLY) EndDialog(hwndDlg,0);
				return FALSE;
			}
	}
	return FALSE;
}

void config_read()
{
	char ini_file[MAX_PATH], *p;
	GetModuleFileName(plugin.hDllInstance,ini_file,sizeof(ini_file));
	p=ini_file+lstrlen(ini_file);
	while (p >= ini_file && *p != '\\') p--;
	if (++p >= ini_file) *p = 0;
	lstrcat(ini_file,"plugin.ini");

	config_enabled = GetPrivateProfileInt(szAppName,"ButtonsEnabled",config_enabled,ini_file);
}

void config_write()
{
	char ini_file[MAX_PATH], *p;
	char string[32];
	GetModuleFileName(plugin.hDllInstance,ini_file,sizeof(ini_file));
	p=ini_file+lstrlen(ini_file);
	while (p >= ini_file && *p != '\\') p--;
	if (++p >= ini_file) *p = 0;
	lstrcat(ini_file,"plugin.ini");

	wsprintf(string,"%d",config_enabled);
	WritePrivateProfileString(szAppName,"ButtonsEnabled",string,ini_file);
}

__declspec( dllexport ) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin()
{
	return &plugin;
}

