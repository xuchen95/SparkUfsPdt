#include "pch.h"
#include "CDialogBase.h"

IMPLEMENT_DYNAMIC(CDialogBase, CDialogEx)

UFS_OPTION CDialogBase::s_sharedOption = {};
ST_UFS_BASE_SETTING CDialogBase::s_baseOption =
{
    0,              // PortBaseSel
    0,              // PortMappingSel
    0,              // ForceRomMode
    FALSE,          // bSnSeparateIni
    "",             // szRemoteSnPath
    "",             // szReportPath
    "COM1",         // szComName - default COM1
    9600,           // uBaudRate - default 9600
    8,              // uByteSize - default 8 bits
    0,              // uParity - default 0 (None)
    0               // uStopBits - default 0 (1 bit)
};

CDialogBase::CDialogBase(UINT nIDTemplate, CWnd* pParent /*= nullptr*/)
    : CDialogEx(nIDTemplate, pParent)
{
    m_pUfsOption = &s_sharedOption;
    m_pBaseOption = &s_baseOption;
}

PUFS_OPTION CDialogBase::GetSharedUfsOption()
{
    return &s_sharedOption;
}

void CDialogBase::SetUfsOption(PUFS_OPTION pOption)
{
    if (pOption)
    {
        m_pUfsOption = pOption;
    }
}

PUFS_OPTION CDialogBase::GetUfsOption() const
{
    return m_pUfsOption;
}

PST_UFS_BASE_SETTING CDialogBase::GetSharedBaseSetting()
{
    return &s_baseOption;
}

PST_UFS_BASE_SETTING CDialogBase::GetBaseSetting() const
{
    return m_pBaseOption;
}

void CDialogBase::LoadBaseSettingFromIni(const CString& path)
{
    s_baseOption.PortBaseSel = GetPrivateProfileInt(_T("Base"), _T("PortBaseSel"), s_baseOption.PortBaseSel, path);
    s_baseOption.PortMappingSel = GetPrivateProfileInt(_T("Base"), _T("PortMappingSel"), s_baseOption.PortMappingSel, path);
    s_baseOption.ForceRomMode = GetPrivateProfileInt(_T("Base"), _T("ForceRomMode"), s_baseOption.ForceRomMode, path);
    s_baseOption.bSnSeparateIni = GetPrivateProfileInt(_T("Base"), _T("SnSeparateIni"), s_baseOption.bSnSeparateIni ? 1 : 0, path) != 0;

    TCHAR buffer[1024] = {};
    if (GetPrivateProfileString(_T("Base"), _T("RemoteSnPath"), _T(""), buffer, _countof(buffer), path) > 0)
    {
        CStringA temp = CT2A(buffer);
        strcpy_s(s_baseOption.szRemoteSnPath, sizeof(s_baseOption.szRemoteSnPath), temp);
    }

    if (GetPrivateProfileString(_T("Base"), _T("ReportPath"), _T(""), buffer, _countof(buffer), path) > 0)
    {
        CStringA temp = CT2A(buffer);
        strcpy_s(s_baseOption.szReportPath, sizeof(s_baseOption.szReportPath), temp);
    }

    // Load serial port parameters
    ZeroMemory(buffer, sizeof(buffer));
    DWORD dwResult = GetPrivateProfileString(_T("Base"), _T("COM"), _T("COM1"), buffer, _countof(buffer), path);
    if (dwResult > 0)
    {
        CStringA temp = CT2A(buffer);
        strcpy_s(s_baseOption.szComName, sizeof(s_baseOption.szComName), temp);
    }
    else
    {
        // If no entry in INI, use the existing default
        strcpy_s(s_baseOption.szComName, sizeof(s_baseOption.szComName), "COM1");
    }
    
    s_baseOption.uBaudRate = GetPrivateProfileInt(_T("Base"), _T("BaudRate"), 9600, path);
    if (s_baseOption.uBaudRate == 0)
    {
        s_baseOption.uBaudRate = 9600;
    }
    
    s_baseOption.uByteSize = GetPrivateProfileInt(_T("Base"), _T("ByteSize"), 8, path);
    if (s_baseOption.uByteSize == 0)
    {
        s_baseOption.uByteSize = 8;
    }
    
    s_baseOption.uParity = GetPrivateProfileInt(_T("Base"), _T("Parity"), 0, path);
    s_baseOption.uStopBits = GetPrivateProfileInt(_T("Base"), _T("StopBits"), 0, path);
}

void CDialogBase::SaveBaseSettingToIni(const CString& path)
{
    CString value;
    
    value.Format(_T("%d"), s_baseOption.PortBaseSel);
    WritePrivateProfileString(_T("Base"), _T("PortBaseSel"), value, path);
    
    value.Format(_T("%d"), s_baseOption.PortMappingSel);
    WritePrivateProfileString(_T("Base"), _T("PortMappingSel"), value, path);
    
    value.Format(_T("%d"), s_baseOption.ForceRomMode);
    WritePrivateProfileString(_T("Base"), _T("ForceRomMode"), value, path);
    
    value.Format(_T("%d"), s_baseOption.bSnSeparateIni ? 1 : 0);
    WritePrivateProfileString(_T("Base"), _T("SnSeparateIni"), value, path);
    
    WritePrivateProfileString(_T("Base"), _T("RemoteSnPath"), CA2T(s_baseOption.szRemoteSnPath), path);
    WritePrivateProfileString(_T("Base"), _T("ReportPath"), CA2T(s_baseOption.szReportPath), path);
    
    // Save serial port parameters
    WritePrivateProfileString(_T("Base"), _T("COM"), CA2T(s_baseOption.szComName), path);
    
    value.Format(_T("%u"), s_baseOption.uBaudRate);
    WritePrivateProfileString(_T("Base"), _T("BaudRate"), value, path);
    
    value.Format(_T("%u"), s_baseOption.uByteSize);
    WritePrivateProfileString(_T("Base"), _T("ByteSize"), value, path);
    
    value.Format(_T("%u"), s_baseOption.uParity);
    WritePrivateProfileString(_T("Base"), _T("Parity"), value, path);
    
    value.Format(_T("%u"), s_baseOption.uStopBits);
    WritePrivateProfileString(_T("Base"), _T("StopBits"), value, path);
}

void CDialogBase::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

void CDialogBase::LoadRemoteSnToMainParam()
{
    // 检查是否启用了独立序列号 INI 文件
    if (!s_baseOption.bSnSeparateIni)
        return;

    // 检查远程序列号路径是否有效
    if (strlen(s_baseOption.szRemoteSnPath) == 0)
        return;

    CString iniPath = CA2T(s_baseOption.szRemoteSnPath);
    
    // 检查文件是否存在
    if (GetFileAttributes(iniPath) == INVALID_FILE_ATTRIBUTES)
        return;

    CHAR buffer[256] = {};
    int len = 0;
    
    // 读取 Meto
    ZeroMemory(s_sharedOption.mainPrm.meto, sizeof(s_sharedOption.mainPrm.meto));
    len = GetPrivateProfileString("TEST", "Meto", "", buffer, _countof(buffer), iniPath);
    if (len > 0)
    {
        UINT32 val;
        val = strtoul(buffer,nullptr,16);
        memcpy(s_sharedOption.mainPrm.meto,&val,sizeof(val));
    }
    
    // 读取 SerialNumber (PSN_Start)
    ZeroMemory(buffer, sizeof(buffer));
    len = GetPrivateProfileString("TEST", "SerialNumber", "", buffer, _countof(buffer), iniPath);
    if (len > 0)
    {
        s_sharedOption.mainPrm.psn_start = strtoul(buffer, nullptr, 16);
    }
    
    // 读取 SerialNumber_End (PSN_End)
    ZeroMemory(buffer, sizeof(buffer));
    len = GetPrivateProfileString("TEST", "SerialNumber_End", "", buffer, _countof(buffer), iniPath);
    if (len > 0)
    {
        s_sharedOption.mainPrm.psn_end = strtoul(buffer, nullptr, 16);
    }
    
    // 读取 SerialNumber_Mask
    ZeroMemory(buffer, sizeof(buffer));
    ZeroMemory(s_sharedOption.mainPrm.psn_mask, sizeof(s_sharedOption.mainPrm.psn_mask));
    len = GetPrivateProfileString("TEST", "SerialNumber_Mask", "", buffer, _countof(buffer), iniPath);
    if (len > 0)
    {
        memcpy(s_sharedOption.mainPrm.psn_mask, buffer,sizeof(s_sharedOption.mainPrm.psn_mask));
    }
}
