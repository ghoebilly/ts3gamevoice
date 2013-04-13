/*
 * Copyright (c) 2012-2013 JoeBilly
 * Copyright (C) 2010 Simon Inns
 * 
 * HID USB communication functions
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

#include <stdio.h>
#include "stdafx.h"

#include <tchar.h>
#include "hidsdi.h"			// From Windows DDK
#include "usbHidCommunication.h"

// Private variables for holding the device found state and the
// read/write handles
BOOL deviceAttached = FALSE;
BOOL deviceAttachedButBroken = FALSE;
static HANDLE WriteHandle = INVALID_HANDLE_VALUE;
static HANDLE ReadHandle = INVALID_HANDLE_VALUE;
static HANDLE FeatureHandle = INVALID_HANDLE_VALUE;			

// Private variables to store the input and output
// buffers for USB communication
static unsigned char *outputBuffer;
static unsigned char *inputBuffer;
static unsigned char *featureBuffer;

// Worker thread definitions
static HANDLE usbWorkerThreadHandle = INVALID_HANDLE_VALUE;

// State for the worker thread
enum eWorkerThreadState workerThreadState = idle;

// This public method detaches the USB device and forces the 
// worker threads to cancel IO and abort if required.
// This is used when we're done communicating with the device
static void detachUsbDevice()
{
	if (deviceAttached == TRUE)
	{
		OutputDebugString("Detaching USB device");

		if (workerThreadState != idle)
		{
			OutputDebugString("Cancelling IO ops");

			// Cancel any pending IO operations
			CancelIoEx(WriteHandle, NULL);
			CancelIoEx(ReadHandle, NULL);
			CancelIoEx(FeatureHandle, NULL);

			// Clear the worker thread flags
			workerThreadState = idle;
		}

		// Unattach the device
		deviceAttached = FALSE;
		deviceAttachedButBroken = FALSE;

		OutputDebugString("Closing worker thread");

		workerThreadState = terminated;

		// Abort the worker thread
		//TerminateThread(the_usbWorkerThread, 0);
		WaitForSingleObject(usbWorkerThreadHandle, 5000);
		CloseHandle(usbWorkerThreadHandle);
		//the_usbWorkerThread->Abort();

		OutputDebugString("Closing handles");

		// Close the device file handles
		CloseHandle(WriteHandle);
		CloseHandle(ReadHandle);
		CloseHandle(FeatureHandle);
	}
} // END detachUsbDevice Method


// Constructor method
static void initUsbHidCommunication()
{
	int loopCounter;

	// Set deviceAttached to FALSE
	deviceAttached = FALSE;

	// Set deviceAttachedButBroken to FALSE
	deviceAttachedButBroken = FALSE;

	// Set the read and write handles to invalid
	WriteHandle = INVALID_HANDLE_VALUE;
	ReadHandle = INVALID_HANDLE_VALUE;
	FeatureHandle = INVALID_HANDLE_VALUE;

	// Initialise the input and output buffers for communicating
	// with the USB device
	outputBuffer = (unsigned char *) malloc(65);
	inputBuffer = (unsigned char *) malloc(65);
	featureBuffer = (unsigned char *) malloc(65);

	// Fill the outputBuffer with 0xFF (apparently this causes less EMI and power
	// consumption)				
	for (loopCounter = 0; loopCounter < 65; loopCounter++) outputBuffer[loopCounter] = 0xFF;

	// Clear the worker thread flag
	workerThreadState = idle;
} // END usbHidCommunication method

// Destructor method
static void finalizeUsbHidCommunication()
{
	// Cleanly detach ourselves from the USB device
	detachUsbDevice();
} // END ~usbHidCommunication method


// This method is run as a background thread which reads
// information from the USB device.  It is encapsulated in a worker
// thread since the ReadFile() or WriteFile() commands could block if the USB
// device is detached at an unfortunate point, or (more importantly
// if the firmware of the device does not send a response when one
// is expected (due to a software or firmware bug) - which would otherwise
// lock-up the application...

static DWORD WINAPI usbWorkerThread(LPVOID pData)
{
/*void usbWorkerThread()
{*/
	// Variables for tracking how much is read and written
	DWORD bytesRead = 0;
	DWORD bytesWritten = 0;
	LPSTR errorText = NULL;

	// for DEBUG, REMOVE!
	int loopCounter = 0;
	char debugOutput[35];

	while(workerThreadState != terminated)
	{
		if (workerThreadState == read)
		{
			//char debugOutput[20];

			// Map the managed data array from the class to an unmanaged
			// pointer for the ReadFile function
			unsigned char * unmanagedInputBuffer = &inputBuffer[0];

			// Get the packet from the USB device
			OutputDebugString("usbWorkerThread: Get the packet from the USB device");
			ReadFile(ReadHandle, unmanagedInputBuffer, 65, &bytesRead, 0);
			//OutputDebugString("Read in input buffer");
			//_snprintf(debugOutput, 20, "%d", bytesRead);
			//OutputDebugString(debugOutput);

			// DEBUG
			//OutputDebugString("InputBuffer below");
			//for (loopCounter = 0; loopCounter < 65; loopCounter++) 
			//{
			//	_snprintf(debugOutput, 20, "%d", inputBuffer[loopCounter]);
			//	OutputDebugString(debugOutput);
			//}

			// Set the worker thread state to idle
			workerThreadState = idle;
		}

		if (workerThreadState == writeRead)
		{
			// Map the managed data array from the class to an unmanaged
			// pointer for the WriteFile function
			unsigned char * unmanagedOutputBuffer = &outputBuffer[0];

			// Send the packet to the USB device and then perform a read
			// from the device (if the write was successful)
			OutputDebugString("usbWorkerThread: Send the packet to the USB device and try to perform a read");
			if (WriteFile(WriteHandle, unmanagedOutputBuffer, 65, &bytesWritten, 0))
			{
				// Map the managed data array from the class to an unmanaged
				// pointer for the ReadFile function
				unsigned char * unmanagedInputBuffer = &inputBuffer[0];

				// Get the packet from the USB device
				ReadFile(ReadHandle, unmanagedInputBuffer, 65, &bytesRead, 0);
			}

			// Set the worker thread state to idle
			workerThreadState = idle;
		}

		if (workerThreadState == setFeature)
		{
			//char debugOutput[20];

			// Map the managed data array from the class to an unmanaged
			// pointer for the ReadFile function
			unsigned char * unmanagedFeatureBuffer = &featureBuffer[0];

			// DEBUG
			//OutputDebugString("usbWorkerThread: FeatureBuffer below");
			//for (loopCounter = 0; loopCounter < 65; loopCounter++) 
			//{
			//snprintf(debugOutput, 20, "%d", featureBuffer[loopCounter]);
			//OutputDebugString(debugOutput);
			//}

			// Get the packet from the USB device
			OutputDebugString("usbWorkerThread: Set feature to the USB device");
			if (HidD_SetFeature(WriteHandle, unmanagedFeatureBuffer, 65))
			{
				// We need to read the return from the device

				// Map the managed data array from the class to an unmanaged
				// pointer for the ReadFile function
				unsigned char * unmanagedInputBuffer = &inputBuffer[0];

				// Get the packet from the USB device
				ReadFile(ReadHandle, unmanagedInputBuffer, 65, &bytesRead, 0);
							
			}
			else
				OutputDebugString("usbWorkerThread: /!\\ Failed to set feature to the USB device");																		
							
			//OutputDebugString("Read in input buffer");
			//_snprintf(debugOutput, 20, "%d", bytesRead);
			//OutputDebugString(debugOutput);

			// DEBUG
			//OutputDebugString("InputBuffer below");
			//for (loopCounter = 0; loopCounter < 65; loopCounter++) 
			//{
			//	_snprintf(debugOutput, 20, "%d", inputBuffer[loopCounter]);
			//	OutputDebugString(debugOutput);
			//}

			// Set the worker thread state to idle
			workerThreadState = idle;
		}

		if (workerThreadState == write)
		{
			// Map the managed data array from the class to an unmanaged
			// pointer for the WriteFile function
			unsigned char * unmanagedOutputBuffer = &outputBuffer[0];						

			// DEBUG
			//OutputDebugString("usbWorkerThread: OutputBuffer below");
			//for (loopCounter = 0; loopCounter < 65; loopCounter++) 
			//{
			//snprintf(debugOutput, 20, "%d", outputBuffer[loopCounter]);
			//OutputDebugString(debugOutput);
			//}

			// Send the packet to the USB device
			OutputDebugString("usbWorkerThread: Send the packet to the USB device");
			if (!WriteFile(WriteHandle, unmanagedOutputBuffer, 65, &bytesWritten, 0))
			{
				OutputDebugString("usbWorkerThread: /!\\ Failed to send the packet to the USB device");

				snprintf(debugOutput, 35, "usbWorkerThread:bytesWritten:%d", bytesWritten);
				OutputDebugString(debugOutput);

				FormatMessage(
					// use system message tables to retrieve error text
					FORMAT_MESSAGE_FROM_SYSTEM
					// allocate buffer on local heap for error text
					|FORMAT_MESSAGE_ALLOCATE_BUFFER
					// Important! will fail otherwise, since we're not 
					// (and CANNOT) pass insertion parameters
					|FORMAT_MESSAGE_IGNORE_INSERTS,  
					NULL,    // unused with FORMAT_MESSAGE_FROM_SYSTEM
					GetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPSTR)&errorText,  // output 
					0, // minimum size for output buffer
					NULL);   // arguments - see note 

				OutputDebugString(errorText);
			}

			// Set the worker thread state to idle
			workerThreadState = idle;
		}

		if (workerThreadState == idle)
		{
			// Here we add a small delay to prevent the worker thread
			// from executing too fast and using unnecessary processor
			// time
			Sleep(5);
		}					
	}

	OutputDebugString("usbWorkerThread: Worker thread exited");
	return 0;
} // END usbWorkerThread method
	
// This method attempts to find the target USB device and discover the device's path
//
// Note: A limitation of this routine is that it cannot deal with two or more devices
//       connected to the host that have the same VID and PID, it will simply pick
//       the first one it finds...
// Dont work on win32 but work on win64
static void findDevice(int usbVid, int usbPid)
{
	DWORD InterfaceIndex = 0;
	DWORD StatusLastError = 0;				
	//DWORD dwRegSize;
	DWORD dwRegType;
	DWORD StructureSize = 0;
	LPTSTR PropertyValueBuffer = NULL;
	DWORD PropertyValueBufferSize = 0;					
	//BOOL MatchFound = FALSE;
	DWORD ErrorStatus;
	DWORD ErrorStatusWrite;
	DWORD ErrorStatusRead;
	DWORD ErrorStatusFeature;
	DWORD i = 0;

	// for DEBUG, REMOVE!
	//char debugOutput[20];
	//int loopCounter = 0;
	//DWORD bytesRead = 0;
	//DWORD bytesWritten = 0;



	// Define the Globally Unique Identifier (GUID) for HID class devices:
	GUID InterfaceClassGuid = {0x4d1e55b2, 0xf16f, 0x11cf, 0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30}; 

	// Initialise required datatypes and variables
	HDEVINFO DeviceInfoTable = INVALID_HANDLE_VALUE;
	PSP_DEVICE_INTERFACE_DATA InterfaceDataStructure = (PSP_DEVICE_INTERFACE_DATA)malloc(sizeof(InterfaceDataStructure));
	PSP_DEVICE_INTERFACE_DETAIL_DATA DetailedInterfaceDataStructure = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(sizeof(DetailedInterfaceDataStructure));
	SP_DEVINFO_DATA DevInfoData;		

							
	// Construct the VID and PID in the correct string format for Windows
	// "Vid_hhhh&Pid_hhhh" - where hhhh is a four digit hexadecimal number
	//char usbVidHex[5];
	//char usbPidHex[5];

	char usbId[18];

	LPTSTR DeviceIDToFind;

	LPTSTR DeviceIDFromRegistry;

	OutputDebugString("findDevice: Seaching for device ID below");

	snprintf(usbId, 18, "VID_%04X&PID_%04X", usbVid, usbPid);
	DeviceIDToFind = usbId;

	OutputDebugString(DeviceIDToFind);				

	//LocalFree(usbVidHex);
	//LocalFree(usbPidHex);
	LocalFree(usbId);

	OutputDebugString("findDevice: Detaching USB device in case of...");

	// If the device is currently flagged as attached then we are 'rechecking' the device, probably
	// due to some message receieved from Windows indicating a device status chanage.  In this case
	// we should detach the USB device cleanly (if required) before reattaching it.
	detachUsbDevice();

	OutputDebugString("findDevice: SetupDiGetClassDevs: Initializing HID class devices...");

	// Here we populate a list of plugged-in devices matching our class GUID (DIGCF_PRESENT specifies that the list
	// should only contain devices which are plugged in)
	DeviceInfoTable = SetupDiGetClassDevs(&InterfaceClassGuid, NULL, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	if (DeviceInfoTable == INVALID_HANDLE_VALUE)
	{
		OutputDebugString("findDevice: Cannot get devices with the specified GUID");
	}
				
	// Look through the retrieved list of class GUIDs looking for a match on our interface GUID       
	DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
				
	OutputDebugString("findDevice: SetupDiEnumDeviceInfo: Enumerating devices...");

	for (i=0;SetupDiEnumDeviceInfo(DeviceInfoTable,i,
		&DevInfoData);i++)
	{
	// TEST: TRY
	/*InterfaceDataStructure.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	for  (i=0;SetupDiEnumDeviceInterfaces(DeviceInfoTable,NULL, &InterfaceClassGuid, i, &InterfaceDataStructure);i++)
	{*/
		OutputDebugString("findDevice: SetupDiEnumDeviceInterfaces: Getting device interface data...");
					
		InterfaceDataStructure->cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		if (SetupDiEnumDeviceInterfaces(DeviceInfoTable, &DevInfoData, &InterfaceClassGuid, 0, InterfaceDataStructure))
		{
			// Check for an error
			ErrorStatus = GetLastError();

			// Are we at the end of the list?
			if (ERROR_NO_MORE_ITEMS == ErrorStatus)
			{
				OutputDebugString("findDevice: Device is not attached, clean up memory and return with error status");

				// Device is not attached, clean up memory and return with error status
				SetupDiDestroyDeviceInfoList(DeviceInfoTable);
				deviceAttached = FALSE;
				deviceAttachedButBroken = FALSE;
				return;		
			}
		}
		else
		{
			OutputDebugString("findDevice: Cannot get device interface data.");

			// An unknown error occurred! Clean up memory and return with error status
			ErrorStatus = GetLastError();
			SetupDiDestroyDeviceInfoList(DeviceInfoTable);
			deviceAttached = FALSE;
			deviceAttachedButBroken = FALSE;
			return;	
		}

		// Now we have devices with a matching class GUID and interface GUID we need to get the hardware IDs for 
		// the devices.  From that we can get the VID and PID in order to find our target device.

		// Initialize an appropriate SP_DEVINFO_DATA structure.  We need this structure for calling
		// SetupDiGetDeviceRegistryProperty()
		//DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		//SetupDiEnumDeviceInfo(DeviceInfoTable, InterfaceIndex, &DevInfoData);

		OutputDebugString("findDevice: SetupDiGetDeviceRegistryProperty: Getting device Hardware ID from registry...");

		// Firstly we have to determine the size of the query so we can allocate memory to store the returned
		// hardware ID structure
		//SetupDiGetDeviceRegistryProperty(DeviceInfoTable, &DevInfoData, SPDRP_HARDWAREID, &dwRegType, NULL, 0, &dwRegSize);

		// Call function with null to begin with, 
		// then use the returned buffer size (doubled)
		// to Alloc the buffer. Keep calling until
		// success or an unknown failure.
		//
		//  Double the returned buffersize to correct
		//  for underlying legacy CM functions that 
		//  return an incorrect buffersize value on 
		//  DBCS/MBCS systems.
					
		while (!SetupDiGetDeviceRegistryProperty(
			DeviceInfoTable,
			&DevInfoData,
			SPDRP_HARDWAREID,
			&dwRegType,
			(PBYTE)PropertyValueBuffer,
			PropertyValueBufferSize,
			&PropertyValueBufferSize))
		{

			if (GetLastError() == 
				ERROR_INSUFFICIENT_BUFFER)
			{
				// Change the buffer size.
				if (PropertyValueBuffer) LocalFree(PropertyValueBuffer);
				// Double the size to avoid problems on 
				// W2k MBCS systems per KB 888609. 
				PropertyValueBuffer = LocalAlloc(LPTR,PropertyValueBufferSize * 2);
			}
			else
			{
				// Insert error handling here.
				break;
			}
		}

		// Test to ensure the memory was allocated:
		if(PropertyValueBuffer == NULL)
		{
			OutputDebugString("findDevice: Hardware ID not allocated, clean up memory and return with error status");
						
			SetupDiDestroyDeviceInfoList(DeviceInfoTable);
			deviceAttached = FALSE;
			deviceAttachedButBroken = FALSE;
			return;		
		}

		//OutputDebugString("SetupDiGetDeviceRegistryProperty");

		// Get the hardware ID for the current device.  The PropertyValueBuffer gets filled with an array
		// of NULL terminated strings (REG_MULTI_SZ).  The first string in the buffer contains the 
		// hardware ID in the format "Vid_xxxx&Pid_xxxx" so we compare that against our target device
		// identifier to see if we have a match.
		// SetupDiGetDeviceRegistryProperty(DeviceInfoTable, &DevInfoData, SPDRP_HARDWAREID, &dwRegType,
		//	(PBYTE)PropertyValueBuffer, dwRegSize, NULL);

		// Get the string					
		DeviceIDFromRegistry = PropertyValueBuffer;
		//String^ DeviceIDFromRegistry = new std::string((wchar_t *)PropertyValueBuffer);

		// We don't need the PropertyValueBuffer any more so we free the memory
		//if (PropertyValueBuffer) LocalFree(PropertyValueBuffer);

		// Convert the strings to lowercase
		//std::transform(DeviceIDFromRegistry.begin(), DeviceIDFromRegistry.end(), DeviceIDFromRegistry.begin(), ::tolower);
		// DeviceIDFromRegistry = DeviceIDFromRegistry->ToLowerInvariant();	
		//std::transform(DeviceIDToFind.begin(), DeviceIDToFind.end(), DeviceIDToFind.begin(), ::tolower);
		//DeviceIDToFind = DeviceIDToFind->ToLowerInvariant();

		// Check if the deviceID has the correct VID and PID
		//td::find(DeviceIDToFind)
		//MatchFound = DeviceIDFromRegistry->Contains(DeviceIDToFind);
		//MatchFound = strstr(DeviceIDFromRegistry, DeviceIDToFind) == 0;

		OutputDebugString("findDevice: Device ID from registry is below");
		OutputDebugString(DeviceIDFromRegistry);					

		if(NULL != strstr(DeviceIDFromRegistry, DeviceIDToFind))
		{
			OutputDebugString("findDevice: ! Device found ! Will retrieve the device path...");

			// The target device has been found, now we need to retrieve the device path so we can open
			// the read and write handles required for USB communication

			// First call to determine the size of the returned structure
			DetailedInterfaceDataStructure->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			SetupDiGetDeviceInterfaceDetail(DeviceInfoTable, InterfaceDataStructure, NULL, 0, &StructureSize, NULL);

			// Allocate the memory required
			DetailedInterfaceDataStructure = (PSP_DEVICE_INTERFACE_DETAIL_DATA)(malloc(StructureSize));

			// Test to ensure the memory was allocated
			if(DetailedInterfaceDataStructure == NULL)
			{
				// Not enough memory... clean up and return error status
				SetupDiDestroyDeviceInfoList(DeviceInfoTable);
				deviceAttached = FALSE;
				deviceAttachedButBroken = FALSE;
				return;		
			}

			OutputDebugString("findDevice: SetupDiGetDeviceInterfaceDetail: Getting device interface detail to open the read and write handles required for USB communication...");

			// Now we call it again to get the structure
			DetailedInterfaceDataStructure->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);  
			SetupDiGetDeviceInterfaceDetail(DeviceInfoTable, InterfaceDataStructure, DetailedInterfaceDataStructure,
				StructureSize, NULL, NULL); 
						
			OutputDebugString("findDevice: Device path is below");
			OutputDebugString(DetailedInterfaceDataStructure->DevicePath);

			// Open the write handle
			WriteHandle = CreateFile((DetailedInterfaceDataStructure->DevicePath), 
				GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
			ErrorStatusWrite = GetLastError();

			// Open the read handle
			ReadHandle = CreateFile((DetailedInterfaceDataStructure->DevicePath),
				GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
			ErrorStatusRead = GetLastError();

			// Open the feature handle
			FeatureHandle = CreateFile((DetailedInterfaceDataStructure->DevicePath),
				GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
			ErrorStatusFeature = GetLastError();

			// Check to see if we opened the read and write handles successfully
			if((ErrorStatusWrite == ERROR_SUCCESS) && (ErrorStatusRead == ERROR_SUCCESS) && (ErrorStatusFeature == ERROR_SUCCESS))
			{
				OutputDebugString("findDevice: Success ! Device is now attached");

				// DEBUG CODE
							
				//outputBuffer[0] = 0;
				//outputBuffer[1] = 56;
				//inputBuffer[0] = 0;

				//ReadFile(ReadHandle, &inputBuffer[0], 65, &bytesRead, 0);
				////ReadFile(ReadHandle, &inputBuffer, 65, &bytesRead, 0);
				//OutputDebugString("init: Read in input buffer");
				//_snprintf(debugOutput, 20, "%d", bytesRead);
				//OutputDebugString(debugOutput);
				//for (loopCounter = 0; loopCounter < 65; loopCounter++) 
				//{
				//_snprintf(debugOutput, 20, "%d", inputBuffer[loopCounter]);
				//OutputDebugString(debugOutput);
				//}

				//WriteFile(WriteHandle, &outputBuffer, 65, &bytesWritten, 0);
				//_snprintf(debugOutput, 20, "%d", bytesWritten);
				//OutputDebugString(debugOutput);

				//OutputDebugString("Write to ouput buffer");
				//_snprintf(debugOutput, 20, "%d", outputBuffer[0]);
				//OutputDebugString(debugOutput);
				//_snprintf(debugOutput, 20, "%d", outputBuffer[1]);
				//OutputDebugString(debugOutput);
				//_snprintf(debugOutput, 20, "%d", outputBuffer[2]);
				//OutputDebugString(debugOutput);
				//for (loopCounter = 0; loopCounter < 65; loopCounter++) 
				//{
				//WriteFile(WriteHandle, &outputBuffer[0], loopCounter, &bytesWritten, 0);
				//_snprintf(debugOutput, 20, "%d:%d", loopCounter, bytesWritten);			
				//OutputDebugString(debugOutput);
				//}


				/*WriteFile(WriteHandle, &outputBuffer[0], 65, &bytesWritten, 0);
				WriteFile(WriteHandle, outputBuffer, 65, &bytesWritten, 0);
				WriteFile(WriteHandle, outputBuffer, 2, &bytesWritten, 0);
				WriteFile(WriteHandle, &outputBuffer, 65, &bytesWritten, 0);
				WriteFile(WriteHandle, &outputBuffer[1], 1, &bytesWritten, 0);
				WriteFile(WriteHandle, &outputBuffer[0], 2, &bytesWritten, 0);
				WriteFile(WriteHandle, &outputBuffer[1], 2, &bytesWritten, 0);
				WriteFile(WriteHandle, &outputBuffer[1], 2, &bytesWritten, 0);*/

				// File handles opened successfully, device is now attached
				deviceAttached = TRUE;
				deviceAttachedButBroken = FALSE;

				// Start the device communication worker thread
				usbWorkerThreadHandle = CreateThread(NULL, 0, &usbWorkerThread, 0, 0, NULL);	
			}
			else
			{
				OutputDebugString("findDevice: Failed ! Something went wrong... Can't use the device :(");

				// Something went wrong... If we managed to open either handle close them
				// and set deviceAttachedButBroken since we found the device but, for some
				// reason, can't use it.
				if(ErrorStatusWrite == ERROR_SUCCESS)
					CloseHandle(WriteHandle);
				if(ErrorStatusRead == ERROR_SUCCESS)
					CloseHandle(ReadHandle);
				if(ErrorStatusFeature == ERROR_SUCCESS)
					CloseHandle(FeatureHandle);

				deviceAttached = FALSE;
				deviceAttachedButBroken = TRUE;
			}
						
			return;
		} // END if
	}

	OutputDebugString("findDevice: SetupDiEnumDeviceInfo: Enumerate device ended.");

	// Clean up the memory
	if (PropertyValueBuffer) LocalFree(PropertyValueBuffer);
	if (DeviceIDToFind) LocalFree(DeviceIDToFind);
	if (DeviceIDFromRegistry) LocalFree(DeviceIDFromRegistry);
	SetupDiDestroyDeviceInfoList(DeviceInfoTable);				
} // END findDevice method
			
/* WORK on win32 & win64
void testFind2()
{
	HDEVINFO                         hDevInfo;
	SP_DEVICE_INTERFACE_DATA         DevIntfData;
	PSP_DEVICE_INTERFACE_DETAIL_DATA DevIntfDetailData;
	SP_DEVINFO_DATA                  DevData;

	DWORD dwSize, dwType, dwMemberIdx;
	HKEY hKey;
	BYTE lpData[1024];

	//GUID GUID_DEVINTERFACE_USB_DEVICE = {0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED};
	GUID GUID_DEVINTERFACE_USB_DEVICE = {0x4d1e55b2, 0xf16f, 0x11cf, 0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30};

	OutputDebugString("testFind2");

	// We will try to get device information set for all USB devices that have a
	// device interface and are currently present on the system (plugged in).
	hDevInfo = SetupDiGetClassDevs(
		&GUID_DEVINTERFACE_USB_DEVICE, NULL, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
	
	if (hDevInfo != INVALID_HANDLE_VALUE)
	{
		// Prepare to enumerate all device interfaces for the device information
		// set that we retrieved with SetupDiGetClassDevs(..)
		DevIntfData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		dwMemberIdx = 0;
		
		// Next, we will keep calling this SetupDiEnumDeviceInterfaces(..) until this
		// function causes GetLastError() to return  ERROR_NO_MORE_ITEMS. With each
		// call the dwMemberIdx value needs to be incremented to retrieve the next
		// device interface information.

		SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_USB_DEVICE,
			dwMemberIdx, &DevIntfData);

		while(GetLastError() != ERROR_NO_MORE_ITEMS)
		{
			// As a last step we will need to get some more details for each
			// of device interface information we are able to retrieve. This
			// device interface detail gives us the information we need to identify
			// the device (VID/PID), and decide if it's useful to us. It will also
			// provide a DEVINFO_DATA structure which we can use to know the serial
			// port name for a virtual com port.

			DevData.cbSize = sizeof(DevData);
			
			// Get the required buffer size. Call SetupDiGetDeviceInterfaceDetail with
			// a NULL DevIntfDetailData pointer, a DevIntfDetailDataSize
			// of zero, and a valid RequiredSize variable. In response to such a call,
			// this function returns the required buffer size at dwSize.

			SetupDiGetDeviceInterfaceDetail(
					hDevInfo, &DevIntfData, NULL, 0, &dwSize, NULL);

			// Allocate memory for the DeviceInterfaceDetail struct. Don't forget to
			// deallocate it later!
			DevIntfDetailData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
			DevIntfDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

			if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevIntfData,
				DevIntfDetailData, dwSize, &dwSize, &DevData))
			{
				OutputDebugString("Searching for device");

				// Finally we can start checking if we've found a useable device,
				// by inspecting the DevIntfDetailData->DevicePath variable.
				// The DevicePath looks something like this:
				//
				// \\?\usb#vid_04d8&pid_0033#5&19f2438f&0&2#{a5dcbf10-6530-11d2-901f-00c04fb951ed}
				//
				// The VID for Velleman Projects is always 10cf. The PID is variable
				// for each device:
				//
				//    -------------------
				//    | Device   | PID  |
				//    -------------------
				//    | K8090    | 8090 |
				//    | VMB1USB  | 0b1b |
				//    -------------------
				//
				// As you can see it contains the VID/PID for the device, so we can check
				// for the right VID/PID with string handling routines.

				if (NULL != _tcsstr((TCHAR*)DevIntfDetailData->DevicePath, _T("vid_045e&pid_003b")))
				{
					OutputDebugString("Found a device!");
					// To find out the serial port for our K8090 device,
					// we'll need to check the registry:

					hKey = SetupDiOpenDevRegKey(
								hDevInfo,
								&DevData,
								DICS_FLAG_GLOBAL,
								0,
								DIREG_DEV,
								KEY_READ
							);

					dwType = REG_SZ;
					dwSize = sizeof(lpData);
					RegQueryValueEx(hKey, _T("PortName"), NULL, &dwType, lpData, &dwSize);
					RegCloseKey(hKey);

					// Eureka!
					OutputDebugString("Found a device on a port");
					wprintf(_T("Found a device on port '%s'\n"), lpData);
				}
			}

			HeapFree(GetProcessHeap(), 0, DevIntfDetailData);

			// Continue looping
			SetupDiEnumDeviceInterfaces(
				hDevInfo, NULL, &GUID_DEVINTERFACE_USB_DEVICE, ++dwMemberIdx, &DevIntfData);
		}

		SetupDiDestroyDeviceInfoList(hDevInfo);
	}
}*/

// This public method requests that device notification messages are sent to the calling form
// which the form must catch with a WndProc override.
//
// You will need to supply the handle of the calling form for this to work, usually
// specified with 'this->form'.  Something like the following line needs to be placed
// in the form's contructor method:
// 
// a_usbHidCommunication.requestDeviceNotificationsToForm(this->Handle);
// 

static void requestDeviceNotificationsToForm(HANDLE handleOfWindow)
{
	// Define the Globally Unique Identifier (GUID) for HID class devices:
	GUID InterfaceClassGuid = {0x4d1e55b2, 0xf16f, 0x11cf, 0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30};

	// Register for WM_DEVICECHANGE notifications.  This code uses these messages to detect
	// plug and play connection/disconnection events for USB devices
	DEV_BROADCAST_DEVICEINTERFACE myDeviceBroadcastHeader;
	myDeviceBroadcastHeader.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	myDeviceBroadcastHeader.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	myDeviceBroadcastHeader.dbcc_reserved = 0;
	myDeviceBroadcastHeader.dbcc_classguid = InterfaceClassGuid;
	RegisterDeviceNotification((HANDLE)handleOfWindow, &myDeviceBroadcastHeader, DEVICE_NOTIFY_WINDOW_HANDLE);
}


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
static void handleDeviceChangeMessages(UINT uMsg, WPARAM wParam, LPARAM lParam, int vid, int pid)
{
		if(uMsg == WM_DEVICECHANGE)
		{
			if(((int)wParam == DBT_DEVICEARRIVAL) || ((int)wParam == DBT_DEVICEREMOVEPENDING) ||
				((int)wParam == DBT_DEVICEREMOVECOMPLETE) || ((int)wParam == DBT_CONFIGCHANGED) )
			{
			// Check the device is still available
			findDevice(vid, pid); // VID, PID			 
			}
		}
}

// This private method detaches the USB device and forces the 
// worker threads to cancel IO and abort if required.
// If the USB device stops responding to the read/write
// operations (due to a software or firmware bug) you can use
// this method to recover back into a known state.
static void detachBrokenUsbDevice()
{
	if (deviceAttached == TRUE)
	{
		OutputDebugString("detachBrokenUsbDevice: Detaching broken USB device...");

		if (workerThreadState != idle)
		{
			// Cancel any pending IO operations
			CancelIoEx(WriteHandle, NULL);
			CancelIoEx(ReadHandle, NULL);
			CancelIoEx(FeatureHandle, NULL);

			// Clear the worker thread flags
			workerThreadState = idle;
		}

		// Unattach the device and indicate it is broken
		deviceAttached = FALSE;
		deviceAttachedButBroken = TRUE;

		// Abort the worker thread
		CloseHandle(usbWorkerThreadHandle);
		//the_usbWorkerThread->Abort();

		// Close the device file handles
		CloseHandle(WriteHandle);
		CloseHandle(ReadHandle);
		CloseHandle(FeatureHandle);
	}
} // END detachBrokenUsbDevice Method


// Define public method for reading the deviceAttached flag

static BOOL isDeviceAttached(void)
{
	return deviceAttached;
} // END isDeviceAttached method

// Define public method for reading the deviceAttachedButBroken flag

static BOOL isDeviceBroken(void)
{
	return deviceAttachedButBroken;
} // END isDeviceBroken method
			

// The following private method waits for the worker thread to become
// idle before returning.
static BOOL waitForTheWorkerThreadToBeIdle(BOOL checkForTimeOut)
{
	int timeOutCounter = 0;

	// Note: This is probably not ideal since it blocks the form application
	//		 while it waits, but the USB communication is synchronous i.e.
	//		 it matters what order the commands are sent and receieved...
	while (workerThreadState != idle && workerThreadState != terminated &&
		(timeOutCounter < 600))
	{
		Sleep(5);

		if (checkForTimeOut)
			timeOutCounter++;
	}

	// Did we timeout after 3 seconds?
	if (workerThreadState != idle  && workerThreadState != terminated &&
		(timeOutCounter >= 600))
	{
		OutputDebugString("waitForTheWorkerThreadToBeIdle: Worker thread timed out, detaching the USB device...");
		// We timed out... something is blocking the worker thread and it's not
		// responding.  This is probably due to a firmware/software bug where a 
		// write/read operation was performed and the thread is still waiting for
		// a read which is not coming.
		//
		// Let's detach the USB device to return us into a known state...
		detachBrokenUsbDevice();
		return FALSE;
	}

	return TRUE;
} // END waitForTheWorkerThreadToBeIdle method

// The following method forces a feature request to the USB device (the device must have been found first!)
static BOOL forceFeature(int usbCommandId)
{
	// Variables for tracking how much is read and written
	DWORD bytesRead = 0;				
	unsigned char * unmanagedFeatureBuffer = &featureBuffer[0];				
	char strCommandId[40];

	// Check to see if the device is already found
	if (deviceAttached == FALSE)
	{
		// There is no device to communicate with... Exit with error status
		return FALSE;
	}

	snprintf(strCommandId, 40, "forceFeature:command:%d", usbCommandId);
	OutputDebugString(strCommandId);					

	// The first byte of the input and feature buffers should be set to zero (this is not
	// sent to the USB device)
	inputBuffer[0] = 0;
	featureBuffer[0] = 0;

	// The second byte of the feature buffer contains the command to the USB device
	// (the rest of the buffer is available for data transfer)
	inputBuffer[1] = 0xFF;
	featureBuffer[1] = usbCommandId;

	// DEBUG
	//OutputDebugString("usbWorkerThread: FeatureBuffer below");
	//for (loopCounter = 0; loopCounter < 65; loopCounter++) 
	//{
	//snprintf(debugOutput, 20, "%d", featureBuffer[loopCounter]);
	//OutputDebugString(debugOutput);
	//}				

	// Get the packet from the USB device
	OutputDebugString("forceFeature: Set feature to the USB device");
	if (HidD_SetFeature(FeatureHandle, unmanagedFeatureBuffer, 65))
	{
		// We need to read the return from the device

		// Map the managed data array from the class to an unmanaged
		// pointer for the ReadFile function
		unsigned char * unmanagedInputBuffer = &inputBuffer[0];

		// Get the packet from the USB device
		ReadFile(FeatureHandle, unmanagedInputBuffer, 65, &bytesRead, 0);
					
		// Return with success
		return TRUE;
	}
	else
		OutputDebugString("forceFeature: /!\\ Failed to set feature to the USB device");

	return FALSE;
}

// The following method sends a feature request to the USB device (the device must have been found first!)
static BOOL sendUsbFeature(int usbCommandId)
{
	char strCommandId[40];

	// Check to see if the device is already found
	if (deviceAttached == FALSE)
	{
		// There is no device to communicate with... Exit with error status
		return FALSE;
	}

	OutputDebugString("sendUsbFeature");

	// Wait for the worker thread to be idle before continuing
	if (waitForTheWorkerThreadToBeIdle(TRUE))
	{
		snprintf(strCommandId, 40, "sendUsbFeature:command:%d", usbCommandId);
		OutputDebugString("sendUsbFeature: Worker thread is idle, setting command ID and state to SetFeature...");
		OutputDebugString(strCommandId);					

		// The first byte of the input and feature buffers should be set to zero (this is not
		// sent to the USB device)
		inputBuffer[0] = 0;
		featureBuffer[0] = 0;

		// The second byte of the feature buffer contains the command to the USB device
		// (the rest of the buffer is available for data transfer)
		inputBuffer[1] = 0xFF;
		featureBuffer[1] = usbCommandId;					

		// Write the buffer to the device and then read to the input buffer
		workerThreadState = setFeature;

		// Return with success
		return TRUE;
	}
	else return FALSE;
}

// The following method sends a command to the USB device (the device must have been found first!)
// This method is for commands that are sent, but no input is returned from the device
static BOOL sendUsbCommandWriteOnly(int usbCommandId)
{
	char strCommandId[40];

	// Check to see if the device is already found
	if (deviceAttached == FALSE)
	{
		// There is no device to communicate with... Exit with error status
		return FALSE;
	}

	OutputDebugString("sendUsbCommandWriteOnly");

	// Wait for the worker thread to be idle before continuing
	if (waitForTheWorkerThreadToBeIdle(TRUE))
	{
		snprintf(strCommandId, 40, "sendUsbCommandWriteOnly:command:%d", usbCommandId);
		OutputDebugString("sendUsbCommandWriteOnly: Worker thread is idle, setting command ID and state to Write...");
		OutputDebugString(strCommandId);

		// The first byte of the input and output buffers should be set to zero (this is not
		// sent to the USB device)
		outputBuffer[0] = 0;

		// The second byte of the output buffer contains the command to the USB device
		// (the rest of the buffer is available for data transfer)
		outputBuffer[1] = usbCommandId;

		// Write the buffer to the device
		workerThreadState = write;

		// Return with success
		return TRUE;
	} else return FALSE;
				
} // END sendUsbCommandWriteOnly Method


// The following method sends a command to the USB device (the device must have been found first!)
// This method is for commands that are sent and expect a reply
static BOOL sendUsbCommandWriteRead(int usbCommandId)
{
	// Check to see if the device is already found
	if (deviceAttached == FALSE)
	{
		// There is no device to communicate with... Exit with error status
		return FALSE;
	}

	OutputDebugString("sendUsbCommandWriteRead");

	// Wait for the worker thread to be idle before continuing
	if (waitForTheWorkerThreadToBeIdle(TRUE))
	{
		OutputDebugString("sendUsbCommandWriteRead: Worker thread is idle, setting command ID and state to WriteRead...");

		// The first byte of the input and output buffers should be set to zero (this is not
		// sent to the USB device)
		outputBuffer[0] = 0;
		inputBuffer[0] = 0;

		// The second byte of the output buffer contains the command to the USB device
		// (the rest of the buffer is available for data transfer)
		outputBuffer[1] = usbCommandId;
		inputBuffer[1] = 0xFF;

		// Write the buffer to the device and then read to the input buffer
		workerThreadState = writeRead;

		// Return with success
		return TRUE;
	}
	else return FALSE;
} // END sendUsbCommandReadWrite Method

static BOOL receiveUsbCommand()
{
	// Check to see if the device is already found
	if (deviceAttached == FALSE)
	{
		// There is no device to communicate with... Exit with error status
		return FALSE;
	}

	//OutputDebugString("sendUsbCommandReadOnly");

	// Wait for the worker thread to be idle before continuing
	if (waitForTheWorkerThreadToBeIdle(TRUE))
	{
		OutputDebugString("sendUsbCommandReadOnly: Worker thread is idle, setting state to Read...");

		// The first byte of the input buffer should be set to zero (this is not
		// sent to the USB device)
		inputBuffer[0] = 0;
		inputBuffer[1] = 0xFF;

		// Write the buffer to the device and then read to the input buffer
		workerThreadState = read;

		// Return with success
		return TRUE;
	}
	return FALSE;
}

// This public method allows writing to the output buffer
// Note: you cannot write to byte 0 as these are reserved
//       by the command communication - bytes 1 to 64 are available
//		 for data
static BOOL writeToTheOutputBuffer(int byteNumber, byte value)
{
	// Do not allow writing to byte 0 or 1, this is ignored
	if (byteNumber < 1) return FALSE;

	// Do not allow writing to bytes beyond the array size
	if (byteNumber > 64) return FALSE;

	// Ensure that the worker thread has finished reading before
	// writing the data to the output buffer.
	waitForTheWorkerThreadToBeIdle(FALSE);

	// Write the byte to the output buffer
	outputBuffer[byteNumber] = value;

	return TRUE;
} // END writeToTheOutputBuffer method


// This public method allows reading from the input buffer
// Note: you cannot read from byte 0  as these are reserved
//       by the command communication - bytes 1 to 64 are available
//		 for data
static byte readFromTheInputBuffer(int byteNumber)
{
	// Do not allow reading from byte 0 or 1, just return zero
	if (byteNumber < 1) return -1;

	// Do not allow reading from bytes beyond the array size, just return zero
	if (byteNumber > 64) return -1;

	// Ensure that the worker thread has finished reading before
	// grabbing the data from the input buffer.
	waitForTheWorkerThreadToBeIdle(FALSE);

	return inputBuffer[byteNumber];
} // END readFromTheInputBuffer method

// This public method allows reading from the feature buffer
// Note: you cannot read from byte 0 as these are reserved
//       by the command communication - bytes 1 to 64 are available
//		 for data
static byte readFromTheFeatureBuffer(int byteNumber)
{
	// Do not allow reading from byte 0 or 1, just return zero
	if (byteNumber < 1) return -1;

	// Do not allow reading from bytes beyond the array size, just return zero
	if (byteNumber > 64) return -1;

	// Ensure that the worker thread has finished reading before
	// grabbing the data from the input buffer.
	waitForTheWorkerThreadToBeIdle(FALSE);

	return featureBuffer[byteNumber];
} // END readFromTheFeatureBuffer method

// This public method allows writing to the feature buffer
// Note: you cannot write to byte 0 as these are reserved
//       by the command communication - bytes 1 to 64 are available
//		 for data
static BOOL writeToTheFeatureBuffer(int byteNumber, byte value)
{
	// Do not allow writing to byte 0 or 1, this is ignored
	if (byteNumber < 1) return FALSE;

	// Do not allow writing to bytes beyond the array size
	if (byteNumber > 64) return FALSE;

	// Ensure that the worker thread has finished reading before
	// writing the data to the output buffer.
	waitForTheWorkerThreadToBeIdle(FALSE);

	// Write the byte to the output buffer
	featureBuffer[byteNumber] = value;

	return TRUE;
} // END writeToTheFeatureBuffer method

// UsbHidCommunicator factory
UsbHidCommunication CreateUsbHidCommunicator()
{
	UsbHidCommunication communicator;
	communicator.detachBrokenUsbDevice = detachBrokenUsbDevice;
	communicator.detachUsbDevice = detachUsbDevice;
	communicator.finalizeUsbHidCommunication = finalizeUsbHidCommunication;
	communicator.findDevice = findDevice;
	communicator.forceFeature = forceFeature;
	communicator.handleDeviceChangeMessages = handleDeviceChangeMessages;
	communicator.initUsbHidCommunication = initUsbHidCommunication;
	communicator.isDeviceAttached = isDeviceAttached;
	communicator.isDeviceBroken = isDeviceBroken;
	communicator.readFromTheFeatureBuffer = readFromTheFeatureBuffer;
	communicator.readFromTheInputBuffer = readFromTheInputBuffer;
	communicator.receiveUsbCommand = receiveUsbCommand;
	communicator.requestDeviceNotificationsToForm = requestDeviceNotificationsToForm;
	communicator.sendUsbCommandWriteOnly = sendUsbCommandWriteOnly;
	communicator.sendUsbCommandWriteRead = sendUsbCommandWriteRead;
	communicator.sendUsbFeature = sendUsbFeature;
	//communicator.usbWorkerThread = usbWorkerThread;
	//communicator.waitForTheWorkerThreadToBeIdle = waitForTheWorkerThreadToBeIdle;
	communicator.writeToTheFeatureBuffer = writeToTheFeatureBuffer;
	communicator.writeToTheOutputBuffer = writeToTheOutputBuffer;

	return communicator;
}