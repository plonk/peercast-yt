// ------------------------------------------------
// File : version2.h
// Date: 17-june-2005
// Author: giles
//
// (c) 2005 peercast.org
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

#ifndef _VERSION2_H
#define _VERSION2_H

// ------------------------------------------------
#ifdef PRIVATE_BROADCASTER
static const char PCP_BROADCAST_FLAGS   = 0x01;
#define PCP_FORCE_YP true
#else
static const char PCP_BROADCAST_FLAGS   = 0x00;
#define PCP_FORCE_YP false
#endif

// ------------------------------------------------
static const int PCP_CLIENT_VERSION     = 1218;
static const int PCP_CLIENT_VERSION_VP  = 27;
static const int PCP_ROOT_VERSION       = 1218;

static const int PCP_CLIENT_MINVERSION  = 1200;

#define PCX_AGENT "PeerCast/0.1218 (YT27)"
#define PCX_VERSTRING "v0.1218 YT27"

#define PCP_CLIENT_VERSION_EX_PREFIX "YT"
static const int PCP_CLIENT_VERSION_EX_NUMBER = 27;
// ------------------------------------------------

#endif
