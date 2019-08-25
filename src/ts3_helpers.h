/* 
 * Copyright (c) 2012-2019 JoeBilly 
 * Copyright (c) 2010-2012 Jules Blok
 *
 * TeamSpeak 3 helpers functions
 * ts3_helpers.h
 * JoeBilly (joebilly@users.sourceforge.net)
 * https://github.com/ghoebilly/ts3gamevoice
 *
 * Code adapted from TeamSpeak 3 G-key plugin 
 * Jules Blok (jules@aerix.nl)
 * http://jules.aerix.nl/svn/g-key/trunk/
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

#ifndef TS3_HELPERS
#define TS3_HELPERS

#ifdef __cplusplus
extern "C" {
#endif

#include "stdafx.h"
#include "plugin_definitions.h"
#include "public_errors.h"
#include "public_errors_rare.h"
#include "ts3_functions.h"

// Code is here in header file to share CONST definitions 
// from public_errors and public_errors_rare
// and make plugin version upgrade easier 
// (don't want to touch TS3 SDK files)

static struct TS3Functions ts3Functions;

uint64 *array_concat(void *a, int an,
                   void *b, int bn)
{
  uint64 *p = malloc(sizeof(uint64) * (an + bn));
  memcpy(p, a, an*sizeof(int));
  memcpy(p + an, b, bn*sizeof(int));
  return p;
}

BOOL logOnError(unsigned int returnCode, char* message)
{
	if(returnCode != ERROR_ok)
	{
		char* errorMsg;
		if(ts3Functions.getErrorMessage(returnCode, &errorMsg) == ERROR_ok)
		{
			if(message != NULL) ts3Functions.logMessage(message, LogLevel_WARNING, "Gamevoice Plugin", 0);
			ts3Functions.logMessage(errorMsg, LogLevel_WARNING, "Gamevoice Plugin", 0);
			ts3Functions.freeMemory(errorMsg);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL connectToBookmark(char* label, enum PluginConnectTab connectTab, uint64* scHandlerID)
{
	int i;
	BOOL ret;
	struct PluginBookmarkList* bookmarks;

	char message[40];

	snprintf(message, sizeof(message), "connectToBookmark:%s", label);
	ts3Functions.logMessage(message, LogLevel_DEBUG, "Gamevoice Plugin", 0);

	// Get the bookmark list
	if(logOnError(ts3Functions.getBookmarkList(&bookmarks), "Error getting bookmark list"))
		return FALSE;

	// Find the bookmark
	ret = TRUE;
	for(i=0; i<bookmarks->itemcount; i++)
	{
		struct PluginBookmarkItem item = bookmarks->items[i];
		
		// Seems pretty useless to try to connect to a folder, skip it
		if(!item.isFolder)
		{
			// If the name matches the label we're looking for
			if(!strcmp(item.name, label))
			{
				// Connect to the bookmark
				ret = !logOnError(ts3Functions.guiConnectBookmark(connectTab, item.uuid, scHandlerID), "Failed to connect to bookmark");
			}
		}
	}
	
	ts3Functions.freeMemory(bookmarks);
	return ret;
}

BOOL setAway(uint64 scHandlerID, BOOL isAway, char* msg)
{
	char message[20];

	snprintf(message, sizeof(message), "setAway:%s", isAway ? "true" : "false");
	ts3Functions.logMessage(message, LogLevel_DEBUG, "Gamevoice Plugin", 0);

	if(logOnError(ts3Functions.setClientSelfVariableAsInt(scHandlerID, CLIENT_AWAY, 
		isAway ? AWAY_ZZZ : AWAY_NONE), "Error setting away status"))
		return FALSE;

	if(logOnError(ts3Functions.setClientSelfVariableAsString(scHandlerID, CLIENT_AWAY_MESSAGE, isAway && msg != NULL ? msg : ""), "Error setting away message"))
		return FALSE;

	return logOnError(ts3Functions.flushClientSelfUpdates(scHandlerID, NULL), "Error flushing after setting away status");
}

BOOL setGlobalAway(BOOL isAway, char* msg)
{
	uint64* servers;
	uint64 handle;
	int i;

	char message[20];

	snprintf(message, sizeof(message), "setGlobalAway:%s", isAway ? "true" : "false");
	ts3Functions.logMessage(message, LogLevel_DEBUG, "Gamevoice Plugin", 0);

	if(logOnError(ts3Functions.getServerConnectionHandlerList(&servers), "Error retrieving list of servers"))
		return FALSE;
	
	handle = servers[0];
	for(i = 1; handle != (uint64)NULL; i++)
	{
		setAway(handle, isAway, msg);
		handle = servers[i];
	}

	ts3Functions.freeMemory(servers);
	return TRUE;
}

BOOL setInputMute(uint64 scHandlerID, BOOL shouldMute)
{
	char message[20];

	snprintf(message, sizeof(message), "setInputMute:%s", shouldMute ? "true" : "false");
	ts3Functions.logMessage(message, LogLevel_DEBUG, "Gamevoice Plugin", 0);

	//ts3Functions.logMessage(strcat("Toggle input mute : ", shouldMute ? "true" : "false"), LogLevel_DEBUG, "Gamevoice Plugin", 0);

	if(logOnError(ts3Functions.setClientSelfVariableAsInt(scHandlerID, CLIENT_INPUT_MUTED, 
		shouldMute ? INPUT_DEACTIVATED : INPUT_ACTIVE), "Error toggling input mute"))
		return FALSE;

	ts3Functions.flushClientSelfUpdates(scHandlerID, NULL);

	return TRUE;
}

BOOL setOutputMute(uint64 scHandlerID, BOOL shouldMute)
{
	char message[20];

	snprintf(message, sizeof(message), "setOutputMute:%s", shouldMute ? "true" : "false");
	ts3Functions.logMessage(message, LogLevel_DEBUG, "Gamevoice Plugin", 0);

	if(logOnError(ts3Functions.setClientSelfVariableAsInt(scHandlerID, CLIENT_OUTPUT_MUTED, 
		shouldMute ? INPUT_DEACTIVATED : INPUT_ACTIVE), "Error toggling output mute"))
		return FALSE;
	
	ts3Functions.flushClientSelfUpdates(scHandlerID, NULL);
	return TRUE;
}

BOOL joinChannel(uint64 scHandlerID, uint64 channel)
{
	anyID self;
	char message[20];
	
	snprintf(message, sizeof(message), "joinChannel:%d", channel);
	ts3Functions.logMessage(message, LogLevel_DEBUG, "Gamevoice Plugin", 0);

	if(logOnError(ts3Functions.getClientID(scHandlerID, &self), "Error getting own client id"))
		return FALSE;
	
	if(logOnError(ts3Functions.requestClientMove(scHandlerID, self, channel, "", NULL), "Error joining channel"))
		return FALSE;

	return TRUE;
}

BOOL setMasterVolume(uint64 scHandlerID, float value)
{
	// Clamp value
	char str[6];
	char message[20];
	
	snprintf(message, sizeof(message), "setMasterVolume:%.1f", value);
	ts3Functions.logMessage(message, LogLevel_DEBUG, "Gamevoice Plugin", 0);

	if(value < -40.0) value = -40.0;
	if(value > 20.0) value = 20.0;

	snprintf(str, sizeof(str), "%.1f", value);
	return logOnError(ts3Functions.setPlaybackConfigValue(scHandlerID, "volume_modifier", str), "Error setting master volume");
}

BOOL SetWhisperList(uint64 scHandlerID, uint64 *channelNumberArray)
{
	BOOL shouldWhisper = TRUE;
	uint64* channels = NULL;
	anyID* clients = NULL;
	uint64* whisperChannels = NULL;
	anyID* whisperClients = NULL;
	int i;
	
	char message[20];
	whisperChannels = (uint64 *) malloc(1);
	whisperClients = (anyID *) malloc(1);

	//snprintf(message, sizeof(message), "SetWhisperList:%.1f", value);
	ts3Functions.logMessage("SetWhisperList", LogLevel_DEBUG, "Gamevoice Plugin", 0);
	snprintf(message, sizeof(message), "SetWhisperList:%s", channelNumberArray[0]);
	OutputDebugString(message);
	
	if (channelNumberArray != NULL)
	{
		OutputDebugString("Getting channels...");
		
		if(logOnError(ts3Functions.getChannelList(scHandlerID, &channels), "Error getting channel list"))
			return FALSE;

		OutputDebugString("Fetching channels...");

		for (i=0;i<sizeof(channelNumberArray);i++)
		{	
			clients = NULL;

			if (channels[channelNumberArray[i]] != (uint64)NULL)
			{
				snprintf(message, sizeof(message), "channelId:%s", channels[channelNumberArray[i]]);
				OutputDebugString(message);

				OutputDebugString("Getting clients in channel...");

				if(logOnError(ts3Functions.getChannelClientList(scHandlerID, channels[channelNumberArray[i]], &clients), "Error getting client list"))
					return FALSE;

				if (clients != NULL)
				{
					OutputDebugString("Saving channels and clients whisper...");

					whisperChannels = (uint64 *)realloc(whisperChannels, sizeof(whisperChannels)*sizeof(uint64));
					whisperChannels[sizeof(whisperChannels)] = channels[channelNumberArray[i]]; 

					//whisperChannels = array_concat(channels, sizeof(channels), whisperChannels, sizeof(whisperChannels));
					whisperClients = array_concat(clients, sizeof(clients), whisperClients, sizeof(whisperClients));

					shouldWhisper = TRUE;	
				}
			}	
		}
	}

	ts3Functions.freeMemory(channels);
	ts3Functions.freeMemory(clients);
	
	OutputDebugString("Requesting whisper for selected channels and clients...");

	/*
	 * For efficiency purposes I will violate the vector abstraction and give a direct pointer to its internal C array
	 */
	if(logOnError(ts3Functions.requestClientSetWhisperList(scHandlerID, (anyID)NULL, shouldWhisper?whisperChannels:(uint64*)NULL, shouldWhisper?whisperClients:(anyID*)NULL, NULL), "Error setting whisper list"))
		return FALSE;

	OutputDebugString("Request whisper ended");

	ts3Functions.flushClientSelfUpdates(scHandlerID, NULL);

	return shouldWhisper;
}

int getConnectionStatus(uint64 scHandlerID)
{
	int status;

	if(logOnError(ts3Functions.getConnectionStatus(scHandlerID, &status), "Error retrieving connection status"))
		return STATUS_DISCONNECTED; // Assume we're not connected

	return status;
}

BOOL isConnected(uint64 scHandlerID)
{
	if(getConnectionStatus(scHandlerID) == STATUS_DISCONNECTED)
	{
		return FALSE;
	}
	return TRUE;
}

#ifdef __cplusplus
}
#endif

#endif