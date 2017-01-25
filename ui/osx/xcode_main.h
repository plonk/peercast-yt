/*
 *  main.h
 *  PeerCast
 *
 *  Created by mode7 on Mon Jun 14 2004.
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
#ifndef _MAIN_H
#define _MAIN_H

#include <Carbon/Carbon.h>

class CAppPathInfo
{
public:
	enum EPathErr
	{
		 kPE_NoMainBundle
		,kPE_NoBundleURL
		,kPE_NoFSRef
		,kPE_CantGetPath
	};
	
	explicit CAppPathInfo()
	{
		initDefaultPath();
	
		CFBundleRef appBundle = CFBundleGetMainBundle();
		
		if( appBundle == NULL )
			throw kPE_NoMainBundle;
			
		CFURLRef appUrlBundle = CFBundleCopyBundleURL( appBundle );
	
		if( appUrlBundle == NULL )
		{
			CFRelease( appBundle );			
			throw kPE_NoBundleURL;
		}
		
		FSRef appBundleRef;
		Boolean success = CFURLGetFSRef( appUrlBundle, &appBundleRef );

		if( success != true )
		{
			throw kPE_NoFSRef;
		}		
		
		OSStatus err	 = FSRefMakePath( &appBundleRef, mPathString, PATH_MAX);
		bool     gotPath = false;
		
		if( err == noErr )
		{
			const int length = strlen( mPathString );
		
			if( length > 0 )
			{
				// remove any file name info in path string
				char* cap = mPathString+length-1;
			
				while( cap && *cap != '/' )
				{
					*cap = 0;
					--cap;
				}
				
				gotPath = true;
			}
		}
		
		CFRelease( appUrlBundle );
		//CFRelease( appBundle );	
		
		if( !gotPath )
		{
			throw kPE_CantGetPath;
		}
	}
	
	const char* getPath() const { return &mPathString[0]; }

private:
	void initDefaultPath() { strcpy(mPathString, "./"); }
	
	char mPathString[PATH_MAX];
};

#endif // _MAIN_H

