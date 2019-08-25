/* 
 * Copyright (c) 2012-2019 JoeBilly 
 *
 * Game Voice helper functions header
 * gamevoice_functions.h
 * JoeBilly (joebilly@users.sourceforge.net)
 * https://github.com/ghoebilly/ts3gamevoice
 *
 *  This file is part of TeamSpeak 3 SideWinder Game Voice Plugin.
 *
 *  TeamSpeak 3 SideWinder Game Voice Plugin is free software: 
 *	you can redistribute it and/or modify it under the terms of the 
 *	GNU General Public License as published by the Free Software Foundation, 
 *	either version 3 of the License, or any later version.
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

#ifndef GAMEVOICE_FUNCTIONS_H
#define GAMEVOICE_FUNCTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

// Flags for the device commands
enum Command {NONE = 0,  ALL = 1, TEAM = 2, CHANNEL_1 = 4, CHANNEL_2 = 8, CHANNEL_3 = 16, CHANNEL_4 = 32, COMMAND = 64, MUTE = 128};
enum Action {DEACTIVATED = 1024, ACTIVATED = 2048};

typedef struct GameVoiceFunctions
{
	// Device state functions
	/* Gets the effective command applied tp the device after a waitForCommand or waitForUserCommand
	 * Effective command contains buttons (Command) that are activated or deactivated (Action)
	 */
	size_t (*getEffectiveCommand)(void);
	/* Gets the last command received from the device during a waitForCommand or waitForUserCommand.
	 */
	byte (*getLastCommandReceived)(void);
	/* Gets the previous command received from the device after a waitForCommand or waitForUserCommand.
	 */
	byte (*getPreviousCommandReceived)(void);
	/* Gets the last feature sent to the device during a forceFeature or sendFeature.
	 */
	byte (*getLastFeatureSent)(void);
	/* Gets the device previous state after a waitForCommand or waitForUserCommand.
	 */
	// byte (*getPreviousState)(void);
	/* Determines whether the specified button has been activated during a waitForcommand or waitForExternalCommand.
	 * A button is activated if its a new command and different from the last feature sent.
	 */
	BOOL (*isButtonActivated)(size_t command);
	/* Determines whether the specified button is active on the device.
	 */
	BOOL (*isButtonActive)(size_t command);
	/* Determines whether the specified button has been deactivated during a waitForcommand or waitForExternalCommand.
	 * A button is deactivated if its a new command and different from the last feature sent.
	 */
	BOOL (*isButtonDeactivated)(size_t command);
	/* Determines whether the specified button is inactive on the device.
	 */
	BOOL (*isButtonInactive)(size_t command);

	// Device general methods
	/* Blinks the device leds/button by activating & deactivating device buttons.
	 */
	void (*blinkDevice)();
	/* Loads the device : find it and attach to it
	 */
	BOOL (*loadDevice)();
	/* Resets the device to its base state.
	 */
	void (*resetDevice)();
	/* Runs a clockwise led chase effect by activating & deactivating all device buttons sequentially
	 */
	void (*runDeviceLedChase)();
	/* Unload the device : reset and detach it
	 */
	void (*unloadDevice)();

	// Commands handling
	/* Reads the last command received from the device
	 */
	byte (*readCommand)();	
	/* Waits for a command from the device.
	 */
	BOOL (*waitForCommand)();
	/* Waits for an external command from the device.
	 * An external command is a command different from the last feature sent.
	 */
	BOOL (*waitForExternalCommand)();

	// Feature handling
	/* Forces a feature to the device (sent immediately)
	 */
	BOOL (*forceFeature)(size_t command);
	/* Sends a feature to the device when its available
	 */
	BOOL (*sendFeature)(size_t command);

	// Button handling
	/* Activate the specified button
	*/
	BOOL (*activateButton)(size_t command);

	/* Deactivate the specified button
	*/
	BOOL(*deactivateButton)(size_t command);
} GameVoiceFunctions;

GameVoiceFunctions InitGameVoiceFunctions();

#ifdef __cplusplus
}
#endif

#endif