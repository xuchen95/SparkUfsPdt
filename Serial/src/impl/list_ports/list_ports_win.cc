#if defined(_WIN32)

/*
 * Copyright (c) 2014 Craig Lilley <cralilley@gmail.com>
 * This software is made available under the terms of the MIT licence.
 * A copy of the licence can be obtained from:
 * http://opensource.org/licenses/MIT
 */

#include "serial/serial.h"
#include <tchar.h>
#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <devguid.h>
#include <cstring>

using serial::PortInfo;
using std::vector;
using std::string;

static const DWORD port_name_max_length = 256;
static const DWORD friendly_name_max_length = 256;
static const DWORD hardware_id_max_length = 256;

// Convert a wide Unicode string to an UTF8 string
std::string utf8_encode(const std::wstring &wstr)
{
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo( size_needed, 0 );
	WideCharToMultiByte                  (CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

vector<PortInfo>
serial::list_ports()
{
	vector<PortInfo> devices_found;

	HDEVINFO device_info_set = SetupDiGetClassDevs(
		(const GUID *) &GUID_DEVCLASS_PORTS,
		NULL,
		NULL,
		DIGCF_PRESENT);

	unsigned int device_info_set_index = 0;
	SP_DEVINFO_DATA device_info_data;

	device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

	while(SetupDiEnumDeviceInfo(device_info_set, device_info_set_index, &device_info_data))
	{
		device_info_set_index++;

		// Get port name

		HKEY hkey = SetupDiOpenDevRegKey(
			device_info_set,
			&device_info_data,
			DICS_FLAG_GLOBAL,
			0,
			DIREG_DEV,
			KEY_READ);

		TCHAR port_name[port_name_max_length];
		DWORD port_name_length = port_name_max_length;

		LONG return_code = RegQueryValueEx(
					hkey,
					_T("PortName"),
					NULL,
					NULL,
					(LPBYTE)port_name,
					&port_name_length);

		RegCloseKey(hkey);

		if(return_code != EXIT_SUCCESS)
			continue;

		if(port_name_length > 0 && port_name_length <= port_name_max_length)
			port_name[port_name_length-1] = '\0';
		else
			port_name[0] = '\0';

		// Ignore parallel ports

		if(_tcsstr(port_name, _T("LPT")) != NULL)
			continue;

		// Get port friendly name

		TCHAR friendly_name[friendly_name_max_length];
		DWORD friendly_name_actual_length = 0;

		BOOL got_friendly_name = SetupDiGetDeviceRegistryProperty(
					device_info_set,
					&device_info_data,
					SPDRP_FRIENDLYNAME,
					NULL,
					(PBYTE)friendly_name,
					friendly_name_max_length,
					&friendly_name_actual_length);

		if(got_friendly_name == TRUE && friendly_name_actual_length > 0)
			friendly_name[friendly_name_actual_length-1] = '\0';
		else
			friendly_name[0] = '\0';

		// Get hardware ID

		TCHAR hardware_id[hardware_id_max_length];
		DWORD hardware_id_actual_length = 0;

		BOOL got_hardware_id = SetupDiGetDeviceRegistryProperty(
					device_info_set,
					&device_info_data,
					SPDRP_HARDWAREID,
					NULL,
					(PBYTE)hardware_id,
					hardware_id_max_length,
					&hardware_id_actual_length);

		if(got_hardware_id == TRUE && hardware_id_actual_length > 0)
			hardware_id[hardware_id_actual_length-1] = '\0';
		else
			hardware_id[0] = '\0';

		#ifdef UNICODE
			std::string portName = utf8_encode(port_name);
			std::string friendlyName = utf8_encode(friendly_name);
			std::string hardwareId = utf8_encode(hardware_id);
		#else
			std::string portName = port_name;
			std::string friendlyName = friendly_name;
			std::string hardwareId = hardware_id;
		#endif

		PortInfo port_entry;
		port_entry.port = portName;
		port_entry.description = friendlyName;
		port_entry.hardware_id = hardwareId;

		devices_found.push_back(port_entry);
	}

	SetupDiDestroyDeviceInfoList(device_info_set);

	return devices_found;
}




vector<PortInfo>
serial::EnumDetailSerialPorts()
{
	std::vector< PortInfo> portInfoList;
	{
		bool bRet = false;

		HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;

		// Return only devices that are currently present in a system
		// The GUID_DEVINTERFACE_COMPORT device interface class is defined for COM ports. GUID
		// {86E0D1E0-8089-11D0-9CE4-08003E301F73}
		hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_COMPORT, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

		if (INVALID_HANDLE_VALUE != hDevInfo)
		{
			SP_DEVINFO_DATA devInfoData;
			// The caller must set DeviceInfoData.cbSize to sizeof(SP_DEVINFO_DATA)
			devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

			for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); i++)
			{
				// get port name
				TCHAR portName[256] = { 0 };
				HKEY hDevKey = SetupDiOpenDevRegKey(hDevInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
				if (INVALID_HANDLE_VALUE != hDevKey)
				{
					DWORD dwCount = 255; // DEV_NAME_MAX_LEN
					RegQueryValueEx(hDevKey, _T("PortName"), NULL, NULL, (BYTE*)portName, &dwCount);
					RegCloseKey(hDevKey);
				}

				// get friendly name
				TCHAR friendlyName[256] = { 0 };
				SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_FRIENDLYNAME, NULL, (PBYTE)friendlyName, sizeof(friendlyName), NULL);

				// get hardware id
				TCHAR hardwareId[256] = { 0 };
				SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_HARDWAREID, NULL, (PBYTE)hardwareId, sizeof(hardwareId), NULL);

				PortInfo stSerialPortInfo;

				stSerialPortInfo.port = portName;
				stSerialPortInfo.description = friendlyName;
				stSerialPortInfo.hardware_id = hardwareId;
				
				// remove (COMxx)
				int index = 0;
				while (stSerialPortInfo.description[index])
				{
					if (stSerialPortInfo.description[index] == '(' && stSerialPortInfo.description[index + 1] == 'C' && stSerialPortInfo.description[index + 2] == 'O' &&
						stSerialPortInfo.description[index + 3] == 'M')
					{
						stSerialPortInfo.description[index] = '\0';
						break;
					}

					++index;
				}

				portInfoList.push_back(stSerialPortInfo);
			}

			if (ERROR_NO_MORE_ITEMS == GetLastError())
			{
				bRet = true; // no more item
			}
		}

		SetupDiDestroyDeviceInfoList(hDevInfo);

		return portInfoList;
	}
}
#endif // #if defined(_WIN32)
