#include "pch.h"
#include "SettingsService.h"
#include <atlconv.h>

SettingsService& SettingsService::Instance()
{
	static SettingsService s;
	return s;
}

SettingsService::SettingsService()
{
	// initialize defaults similar to previous CDialogBase::s_baseOption
	baseOption_.PortBaseSel = 0;
	baseOption_.PortMappingSel = 0;
	baseOption_.ForceRomMode = 0;
	baseOption_.bSnSeparateIni = FALSE;
	strcpy_s(baseOption_.szRemoteSnPath, sizeof(baseOption_.szRemoteSnPath), "");
	strcpy_s(baseOption_.szReportPath, sizeof(baseOption_.szReportPath), "");
	strcpy_s(baseOption_.szComName, sizeof(baseOption_.szComName), "COM1");
	baseOption_.uBaudRate = 9600;
	baseOption_.uByteSize = 8;
	baseOption_.uParity = 0;
	baseOption_.uStopBits = 0;
}

PUFS_OPTION SettingsService::GetUfsOption() const
{
	return const_cast<PUFS_OPTION>(&sharedOption_);
}

void SettingsService::SaveDefaultFactoryCom(const CString& comName)
{
	if (comName.IsEmpty()) return;
	CStringA comA(comName);
	strncpy_s(baseOption_.szComName, sizeof(baseOption_.szComName), comA, _TRUNCATE);
}

PST_UFS_BASE_SETTING SettingsService::GetBaseSetting() const
{
	return const_cast<PST_UFS_BASE_SETTING>(&baseOption_);
}

void SettingsService::LoadBaseSettingFromIni(const CString& path)
{
	baseOption_.PortBaseSel = GetPrivateProfileInt(_T("Base"), _T("PortBaseSel"), baseOption_.PortBaseSel, path);
	baseOption_.PortMappingSel = GetPrivateProfileInt(_T("Base"), _T("PortMappingSel"), baseOption_.PortMappingSel, path);
	baseOption_.ForceRomMode = GetPrivateProfileInt(_T("Base"), _T("ForceRomMode"), baseOption_.ForceRomMode, path);
	baseOption_.bSnSeparateIni = GetPrivateProfileInt(_T("Base"), _T("SnSeparateIni"), baseOption_.bSnSeparateIni ? 1 : 0, path) != 0;

	TCHAR buffer[1024] = {};
	if (GetPrivateProfileString(_T("Base"), _T("RemoteSnPath"), _T(""), buffer, _countof(buffer), path) > 0)
	{
		CStringA temp = CT2A(buffer);
		DWORD dwResult = GetFileAttributes(CA2T(temp));
		if (dwResult == INVALID_FILE_ATTRIBUTES)
		{
			// If the file doesn't exist, clear the path to avoid confusion
			GetCurrentDirectory(_countof(buffer), buffer);
			CString defaultPath = CString(buffer) + _T("\\RemoteSn.ini");
			strcpy_s(baseOption_.szRemoteSnPath, sizeof(baseOption_.szRemoteSnPath), CT2A(defaultPath));
		}
		else
		{
			strcpy_s(baseOption_.szRemoteSnPath, sizeof(baseOption_.szRemoteSnPath), temp);
		}
	}

	if (GetPrivateProfileString(_T("Base"), _T("ReportPath"), _T(""), buffer, _countof(buffer), path) > 0)
	{
		CStringA temp = CT2A(buffer);
		DWORD dwResult = GetFileAttributes(CA2T(temp));
		if (dwResult == INVALID_FILE_ATTRIBUTES)
		{
			// If the file doesn't exist, clear the path to avoid confusion
			GetCurrentDirectory(_countof(buffer), buffer);
			CString defaultPath = CString(buffer) + _T("\\XHSUM");
			strcpy_s(baseOption_.szReportPath, sizeof(baseOption_.szReportPath), CT2A(defaultPath));
		}
		else
		{
			strcpy_s(baseOption_.szReportPath, sizeof(baseOption_.szReportPath), temp);
		}
	}

	// Load serial port parameters
	ZeroMemory(buffer, sizeof(buffer));
	DWORD dwResult = GetPrivateProfileString(_T("Base"), _T("COM"), _T("COM1"), buffer, _countof(buffer), path);
	if (dwResult > 0)
	{
		CStringA temp = CT2A(buffer);
		strcpy_s(baseOption_.szComName, sizeof(baseOption_.szComName), temp);
	}
	else
	{
		strcpy_s(baseOption_.szComName, sizeof(baseOption_.szComName), "COM1");
	}

	baseOption_.uBaudRate = GetPrivateProfileInt(_T("Base"), _T("BaudRate"), 9600, path);
	if (baseOption_.uBaudRate == 0)
	{
		baseOption_.uBaudRate = 9600;
	}

	baseOption_.uByteSize = GetPrivateProfileInt(_T("Base"), _T("ByteSize"), 8, path);
	if (baseOption_.uByteSize == 0)
	{
		baseOption_.uByteSize = 8;
	}

	baseOption_.uParity = GetPrivateProfileInt(_T("Base"), _T("Parity"), 0, path);
	baseOption_.uStopBits = GetPrivateProfileInt(_T("Base"), _T("StopBits"), 0, path);
}

void SettingsService::SaveBaseSettingToIni(const CString& path)
{
	CString value;

	value.Format(_T("%d"), baseOption_.PortBaseSel);
	WritePrivateProfileString(_T("Base"), _T("PortBaseSel"), value, path);

	value.Format(_T("%d"), baseOption_.PortMappingSel);
	WritePrivateProfileString(_T("Base"), _T("PortMappingSel"), value, path);

	value.Format(_T("%d"), baseOption_.ForceRomMode);
	WritePrivateProfileString(_T("Base"), _T("ForceRomMode"), value, path);

	value.Format(_T("%d"), baseOption_.bSnSeparateIni ? 1 : 0);
	WritePrivateProfileString(_T("Base"), _T("SnSeparateIni"), value, path);

	WritePrivateProfileString(_T("Base"), _T("RemoteSnPath"), CA2T(baseOption_.szRemoteSnPath), path);
	WritePrivateProfileString(_T("Base"), _T("ReportPath"), CA2T(baseOption_.szReportPath), path);

	// Save serial port parameters
	WritePrivateProfileString(_T("Base"), _T("COM"), CA2T(baseOption_.szComName), path);

	value.Format(_T("%u"), baseOption_.uBaudRate);
	WritePrivateProfileString(_T("Base"), _T("BaudRate"), value, path);

	value.Format(_T("%u"), baseOption_.uByteSize);
	WritePrivateProfileString(_T("Base"), _T("ByteSize"), value, path);

	value.Format(_T("%u"), baseOption_.uParity);
	WritePrivateProfileString(_T("Base"), _T("Parity"), value, path);

	value.Format(_T("%u"), baseOption_.uStopBits);
	WritePrivateProfileString(_T("Base"), _T("StopBits"), value, path);
}

void SettingsService::LoadRemoteSnToMainParam()
{
	if (strlen(baseOption_.szRemoteSnPath) == 0)
		return;

	CString iniPath = CA2T(baseOption_.szRemoteSnPath);
	if (GetFileAttributes(iniPath) == INVALID_FILE_ATTRIBUTES)
		return;

	CHAR buffer[256] = {};
	int len = 0;

	ZeroMemory(sharedOption_.mainPrm.meto1, sizeof(sharedOption_.mainPrm.meto1));
	len = GetPrivateProfileString("TEST", "Meto1", "", buffer, _countof(buffer), iniPath);
	if (len > 0)
	{
		UINT32 val;
		val = strtoul(buffer,nullptr,16);
		memcpy(sharedOption_.mainPrm.meto1,&val,sizeof(val));
	}

	ZeroMemory(sharedOption_.mainPrm.meto2, sizeof(sharedOption_.mainPrm.meto2));
	len = GetPrivateProfileString("TEST", "Meto2", "", buffer, _countof(buffer), iniPath);
	if (len > 0)
	{
		UINT32 val;
		val = strtoul(buffer,nullptr,16);
		memcpy(sharedOption_.mainPrm.meto2,&val,sizeof(val));
	}

	ZeroMemory(buffer, sizeof(buffer));
	len = GetPrivateProfileString("TEST", "SerialNumber", "", buffer, _countof(buffer), iniPath);
	if (len > 0)
	{
		sharedOption_.mainPrm.psn_start = strtoul(buffer, nullptr, 16);
	}

	ZeroMemory(buffer, sizeof(buffer));
	len = GetPrivateProfileString("TEST", "SerialNumber_End", "", buffer, _countof(buffer), iniPath);
	if (len > 0)
	{
		sharedOption_.mainPrm.psn_end = strtoul(buffer, nullptr, 16);
	}

	ZeroMemory(sharedOption_.mainPrm.psn_mask, sizeof(sharedOption_.mainPrm.psn_mask));
	ZeroMemory(buffer, sizeof(buffer));
	len = GetPrivateProfileString("TEST", "SerialNumber_Mask", "", buffer, _countof(buffer), iniPath);
	if (len > 0)
	{
		memcpy(sharedOption_.mainPrm.psn_mask, buffer,sizeof(sharedOption_.mainPrm.psn_mask));
	}
}

CString SettingsService::GetIspPath() const
{
	return GetUfsOption()->mainPrm.strIspPath;
}
