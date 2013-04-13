/*
 * Copyright (c) 2012-2013 JoeBilly
 * Copyright (C) 2010 Simon Inns
 * 
 * GameVoice Plugin 
 * usbHidCommunication.c
 * JoeBilly (joebilly@users.sourceforge.net)
 * https://sourceforge.net/projects/ts3gamevoice/
 *
 * Code adapted from usbHidCommunication
 * v1_1 2010-03-31
 * Simon Inns (simon.inns@gmail.com)
 * http://www.waitingforfriday.com
 * 
 *  This file is part of TeamSpeak 3 SideWinder Game Voice Plugin.
 *
 *  TeamSpeak 3 SideWinder Game Voice Plugin is free software: 
 *	you can redistribute it and/or modify it under the terms of the 
 *	GNU General Public License as published by the Free Software Foundation, 
 *	either version 3 of the License, or (at your option) any later version.
 *
 *  TeamSpeak 3 SideWinder Game Voice Plugin is distributed in the hope 
 *  that it will be useful, but WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with TeamSpeak 3 SideWinder Game Voice Plugin. 
 *  If not, see <http://www.gnu.org/licenses/>.
 */

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
#pragma once

#ifdef _WIN32
//#pragma warning(disable : 4996)  /* Disable unsafe localtime warning */
#include <Windows.h>	// We require the datatypes from this header
#define snprintf sprintf_s
#endif
#include <string.h>
#include <setupapi.h>	// setupapi.h provides the functions required to search for
						// and identify our target USB device
#include <Dbt.h>		// Required for WM_DEVICECHANGE messages (plug and play USB detection)
