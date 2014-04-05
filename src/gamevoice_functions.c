/* 
 * Copyright (c) 2012-2014 JoeBilly 
 *
 * Game voice helper functions
 * gamevoice_helpers.c
 * JoeBilly (joebilly@users.sourceforge.net)
 * https://sourceforge.net/projects/ts3gamevoice/
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

#include "stdafx.h"
#include "usbHidCommunication.h"
#include "gamevoice_functions.h"

static struct UsbHidCommunication usbHidCommunicator;

static size_t effectiveCommand = NULL;
static byte lastCommandReceived = NULL;
static byte previousCommandReceived = NULL;
static byte lastFeatureSent = NULL;

/* Gets the effective command applied to the device after a waitForCommand or waitForExternalCommand
 * Effective command contains buttons (Command) that are activated or deactivated (Action)
 */
static size_t getEffectiveCommand()
{
	return effectiveCommand;
}

/* Gets the last command received from the device during a waitForCommand or waitForExternalCommand
 */
static byte getLastCommandReceived()
{
	return lastCommandReceived;
}

/* Gets the previous command received from the device after a waitForCommand or waitForExternalCommand
 */
static byte getPreviousCommandReceived()
{
	return previousCommandReceived;
}

/* Gets the last feature sent to the device during a forceFeature or sendFeature
 */
static byte getLastFeatureSent()
{
	return lastFeatureSent;
}

/* Determines whether the specified value is a new command and match the specified command.
 * A new command is a command different from the previous command received
 */
/*static BOOL isNewCommand(byte value, size_t command)
{
	return !(previousCommandReceived & command) && (value & command);
}*/

/* Determines whether the last command is a new command and match the specified command.
 * A new command is a command different from the previous command received.
 */
/*static BOOL isNewLastCommand(size_t command)
{
	return !(previousCommandReceived & command) && (lastCommandReceived & command);
}*/

/* Determines whether the specified button has been activated during a waitForcommand or waitForExternalCommand.
 * A button is activated if its a new command and different from the last feature sent.
 */
static BOOL isButtonActivated(size_t command)
{
	return !(lastFeatureSent & command) && (effectiveCommand & command) && (effectiveCommand & ACTIVATED);
}

/* Determines whether the specified button is active after a waitForcommand or waitForExternalCommand.
 * A button is active if in the last command.
 */
static BOOL isButtonActive(size_t command)
{
	return (lastCommandReceived & command);
}

/* Determines whether the specified button has been deactivated during a waitForcommand or waitForExternalCommand.
 * A button is deactivated if its a new command and different from the last feature sent.
 */
static BOOL isButtonDeactivated(size_t command)
{
	return !(lastFeatureSent & command) && (effectiveCommand & command) && (effectiveCommand & DEACTIVATED);
}

/* Determines whether the specified button is inactive after a waitForcommand or waitForExternalCommand.
 * A button is inactive if not in the last command.
 */
static BOOL isButtonInactive(size_t command)
{
	return !(lastCommandReceived & command);
}

/* Determines whether the specified value is a new user command and match the specified command.
 * A new command is a command different from the previous command received.
 */
//static BOOL isNewExternalCommand(byte value, size_t command)
//{
//	return (lastFeatureSent != command) && !(previousCommandReceived & command) && (value & command);
//}
//
//static BOOL isNewLastExternalCommand(size_t command)
//{
//	return (lastFeatureSent != command) && !(previousCommandReceived & command) && (lastCommandReceived & command);
//}

static BOOL loadDevice()
{
	usbHidCommunicator = CreateUsbHidCommunicator();
	usbHidCommunicator.initUsbHidCommunication();
	usbHidCommunicator.findDevice(0x045E, 0x003B);

	return usbHidCommunicator.isDeviceAttached();
}

static void resetDevice()
{
	lastCommandReceived = NULL;
	lastFeatureSent = NULL;
	usbHidCommunicator.forceUsbFeature(NONE);
	usbHidCommunicator.forceUsbFeature(NONE);
}

static void unloadDevice()
{
	resetDevice();
	usbHidCommunicator.finalizeUsbHidCommunication();
}

static void runDeviceLedChase()
{ 
	usbHidCommunicator.sendUsbFeature(CHANNEL_1);
	Sleep(75);
	usbHidCommunicator.sendUsbFeature(CHANNEL_2);
	Sleep(75);
	usbHidCommunicator.sendUsbFeature(CHANNEL_3);
	Sleep(75);
	usbHidCommunicator.sendUsbFeature(CHANNEL_4);
	Sleep(75);
	usbHidCommunicator.sendUsbFeature(TEAM);
	Sleep(75);
	usbHidCommunicator.sendUsbFeature(ALL);
	Sleep(75);
	usbHidCommunicator.sendUsbFeature(NONE);
	usbHidCommunicator.sendUsbFeature(COMMAND);
	Sleep(75);
	usbHidCommunicator.sendUsbFeature(NONE);
}

/* Blinks all the device leds/button by activating & deactivating device buttons.
 */
static void blinkDevice()
{

}

static byte readCommand()
{
	return usbHidCommunicator.readFromTheInputBuffer(1);
}

static BOOL waitForCommand()
{
	char debugOutput[40];
	previousCommandReceived = lastCommandReceived;
	if (usbHidCommunicator.receiveUsbCommand())
	{
		byte command;
		lastCommandReceived = readCommand();
		command = (previousCommandReceived ^ lastCommandReceived);
		
		if (command & COMMAND)
			command = COMMAND;

		if (command & lastCommandReceived)
			effectiveCommand = lastCommandReceived | ACTIVATED;
		else
			effectiveCommand = command | DEACTIVATED;

		snprintf(debugOutput, 40, "lastFeatureSent:%d", lastFeatureSent);
		OutputDebugString(debugOutput);
		snprintf(debugOutput, 40, "lastCommandReceived:%d", lastCommandReceived);
		OutputDebugString(debugOutput);
		snprintf(debugOutput, 40, "effectiveCommand:%d", effectiveCommand);
		OutputDebugString(debugOutput);
	}
	else
		return FALSE;
	
	return TRUE;
}

static BOOL waitForExternalCommand()
{
	BOOL result;
	do
	{
		result = waitForCommand();
	} while (lastFeatureSent != NULL && lastCommandReceived == lastFeatureSent);

	return result;
}

static BOOL forceFeature(size_t command)
{
	if (usbHidCommunicator.forceUsbFeature(command))
		lastFeatureSent = command;
	else
		return FALSE;

	return TRUE;
}

static BOOL sendFeature(size_t command)
{
	if (usbHidCommunicator.sendUsbFeature(command))
		lastFeatureSent = command;
	else
		return FALSE;

	return TRUE;
}

// GameVoiceFunctions factory
GameVoiceFunctions InitGameVoiceFunctions()
{
	GameVoiceFunctions gamevoiceFunctions;
	gamevoiceFunctions.blinkDevice = blinkDevice;
	gamevoiceFunctions.forceFeature = forceFeature;
	gamevoiceFunctions.getEffectiveCommand = getEffectiveCommand;
	gamevoiceFunctions.getLastCommandReceived = getLastCommandReceived;
	gamevoiceFunctions.getLastFeatureSent = getLastFeatureSent;
	gamevoiceFunctions.getPreviousCommandReceived = getPreviousCommandReceived;	
	//gamevoiceFunctions.getPreviousState = getPreviousState;
	gamevoiceFunctions.isButtonActivated = isButtonActivated;
	gamevoiceFunctions.isButtonActive = isButtonActive;
	gamevoiceFunctions.isButtonDeactivated = isButtonDeactivated;
	gamevoiceFunctions.isButtonInactive = isButtonInactive;
	//gamevoiceFunctions.isNewCommand = isNewCommand;
	//gamevoiceFunctions.isNewLastCommand = isNewLastCommand;
	//gamevoiceFunctions.isNewExternalCommand = isNewExternalCommand;
	//gamevoiceFunctions.isNewLastExternalCommand = isNewLastExternalCommand;
	gamevoiceFunctions.loadDevice = loadDevice;
	gamevoiceFunctions.readCommand = readCommand;
	gamevoiceFunctions.resetDevice = resetDevice;
	gamevoiceFunctions.runDeviceLedChase = runDeviceLedChase;
	gamevoiceFunctions.sendFeature = sendFeature;
	gamevoiceFunctions.unloadDevice = unloadDevice;
	gamevoiceFunctions.waitForCommand = waitForCommand;
	gamevoiceFunctions.waitForExternalCommand = waitForExternalCommand;

	return gamevoiceFunctions;
}