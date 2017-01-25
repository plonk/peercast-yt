// ------------------------------------------------
// File : gui.h
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

#ifndef _GUI_H
#define _GUI_H

#include "sys.h"

extern LRESULT CALLBACK GUIProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
extern void ADDLOG(const char *str,int id,bool sel,void *data, LogBuffer::TYPE type);

extern String iniFileName;
extern HWND guiWnd;
extern int logID;

enum 
{
	WM_INITSETTINGS = WM_USER,
	WM_GETPORTNUMBER,
	WM_PLAYCHANNEL,
	WM_TRAYICON,
	WM_SHOWGUI,
	WM_SHOWMENU,
	WM_PROCURL

};


#endif