/*
 * Copyright (c) 2012-2013 JoeBilly
 * Copyright (C) 2010 Simon Inns
 * 
 * HID USB communication functions header
 * usbHidCommunication.h
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

#ifndef USBHIDCOMMUNICATION_H
#define USBHIDCOMMUNICATION_H

#ifdef __cplusplus
extern "C" {
#endif

// Flags for the worker thread
enum eWorkerThreadState {idle, read, writeRead, setFeature, write, terminated};

typedef struct UsbHidCommunication
{
	//BOOL deviceAttached;
	//BOOL deviceAttachedButBroken;

	//HANDLE WriteHandle;
	//HANDLE ReadHandle;
	//HANDLE FeatureHandle;			

	//// Private variables to store the input and output
	//// buffers for USB communication
	//unsigned char *outputBuffer;
	//unsigned char *inputBuffer;
	//unsigned char *featureBuffer;

	//// Worker thread definitions
	//HANDLE usbWorkerThreadHandle;

	//// State for the worker thread
	//enum eWorkerThreadState workerThreadState;

// This public method detaches the USB device and forces the 
// worker threads to cancel IO and abort if required.
// This is used when we're done communicating with the device
void (*detachUsbDevice)();

// Constructor method
void (*initUsbHidCommunication)();

// Destructor method
void (*finalizeUsbHidCommunication)();

// This method is run as a background thread which reads
// information from the USB device.  It is encapsulated in a worker
// thread since the ReadFile() or WriteFile() commands could block if the USB
// device is detached at an unfortunate point, or (more importantly
// if the firmware of the device does not send a response when one
// is expected (due to a software or firmware bug) - which would otherwise
// lock-up the application...
//DWORD (*usbWorkerThread)(LPVOID pData);
	
// This method attempts to find the target USB device and discover the device's path
//
// Note: A limitation of this routine is that it cannot deal with two or more devices
//       connected to the host that have the same VID and PID, it will simply pick
//       the first one it finds...
// Dont work on win32 but work on win64
void (*findDevice)(int usbVid, int usbPid);
			
// This public method requests that device notification messages are sent to the calling form
// which the form must catch with a WndProc override.
//
// You will need to supply the handle of the calling form for this to work, usually
// specified with 'this->form'.  Something like the following line needs to be placed
// in the form's contructor method:
// 
// a_usbHidCommunication.requestDeviceNotificationsToForm(this->Handle);
// 
void (*requestDeviceNotificationsToForm)(HANDLE handleOfWindow);

// This public method filters WndProc notification messages for the required
// device notifications and triggers a re-detection of the USB device if required.
//
// The main form of the application needs to include an override of the WndProc
// class for this to be called, usually this is defined as a protected method
// of the main form and looks like the following:
//
// protected: virtual void WndProc(Message% m) override
//		{
//			a_usbHidCommunication.handleDeviceChangeMessages(m, vid, pid);
//			Form::WndProc( m ); // Call the original method
//		} // END WndProc method
//
void (*handleDeviceChangeMessages)(UINT uMsg, WPARAM wParam, LPARAM lParam, int vid, int pid);

// This private method detaches the USB device and forces the 
// worker threads to cancel IO and abort if required.
// If the USB device stops responding to the read/write
// operations (due to a software or firmware bug) you can use
// this method to recover back into a known state.
void (*detachBrokenUsbDevice)();

// Define public method for reading the deviceAttached flag
BOOL (*isDeviceAttached)(void);

// Define public method for reading the deviceAttachedButBroken flag
BOOL (*isDeviceBroken)(void);

// The following private method waits for the worker thread to become
// idle before returning.
//BOOL (*waitForTheWorkerThreadToBeIdle)(BOOL checkForTimeOut);

// The following method forces a feature request to the USB device (the device must have been found first!)
BOOL (*forceUsbFeature)(int usbCommandId);

// The following method gets a feature request from the USB device (the device must have been found first!)
byte (*getUsbFeature)();

// The following method sends a feature request to the USB device (the device must have been found first!)
BOOL (*sendUsbFeature)(int usbCommandId);

// The following method sends a command to the USB device (the device must have been found first!)
// This method is for commands that are sent, but no input is returned from the device
BOOL (*sendUsbCommandWriteOnly)(int usbCommandId);

// The following method sends a command to the USB device (the device must have been found first!)
// This method is for commands that are sent and expect a reply
BOOL (*sendUsbCommandWriteRead)(int usbCommandId);

// The following method receive a command from the USB device (the device must have been found first!)
// This method is for commands that are received and expect a reply
BOOL (*receiveUsbCommand)();

// This public method allows writing to the output buffer
// Note: you cannot write to byte 0 as these are reserved
//       by the command communication - bytes 1 to 64 are available
//		 for data
BOOL (*writeToTheOutputBuffer)(int byteNumber, byte value);

// This public method allows reading from the input buffer
// Note: you cannot read from byte 0  as these are reserved
//       by the command communication - bytes 1 to 64 are available
//		 for data
byte (*readFromTheInputBuffer)(int byteNumber);

// This public method allows reading from the feature buffer
// Note: you cannot read from byte 0 as these are reserved
//       by the command communication - bytes 1 to 64 are available
//		 for data
byte (*readFromTheFeatureBuffer)(int byteNumber);

// This public method allows writing to the feature buffer
// Note: you cannot write to byte 0 as these are reserved
//       by the command communication - bytes 1 to 64 are available
//		 for data
BOOL (*writeToTheFeatureBuffer)(int byteNumber, byte value);	
} UsbHidCommunication;

UsbHidCommunication CreateUsbHidCommunicator();

#ifdef __cplusplus
}
#endif

#endif