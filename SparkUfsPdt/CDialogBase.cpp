#include "pch.h"
#include "CDialogBase.h"

IMPLEMENT_DYNAMIC(CDialogBase, CDialogEx)

UFS_OPTION CDialogBase::s_sharedOption = {};
ST_UFS_BASE_SETTING CDialogBase::s_baseOption = {};

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

void CDialogBase::SetBaseSetting(PST_UFS_BASE_SETTING pOption)
{
    if (pOption)
    {
        m_pBaseOption = pOption;
    }
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
}

void CDialogBase::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}
