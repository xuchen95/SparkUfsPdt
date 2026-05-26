#include "pch.h"
#include "CDialogBase.h"
#include "SettingsService.h"

IMPLEMENT_DYNAMIC(CDialogBase, CDialogEx)

CDialogBase::CDialogBase(UINT nIDTemplate, CWnd* pParent /*= nullptr*/)
    : CDialogEx(nIDTemplate, pParent)
{
    // default to the centralized SettingsService shared instances
    m_pUfsOption = SettingsService::Instance().GetUfsOption();
    m_pBaseOption = SettingsService::Instance().GetBaseSetting();
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

// CDialogBase now uses SettingsService for shared persistence. Legacy static
// storage and getters were removed during refactor to centralize settings.

PST_UFS_BASE_SETTING CDialogBase::GetBaseSetting() const
{
    return m_pBaseOption;
}

void CDialogBase::LoadBaseSettingFromIni(const CString& path)
{
    // Delegate loading to centralized SettingsService
    SettingsService::Instance().LoadBaseSettingFromIni(path);
}

void CDialogBase::SaveBaseSettingToIni(const CString& path)
{
    // Delegated to SettingsService
    SettingsService::Instance().SaveBaseSettingToIni(path);
}

void CDialogBase::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

void CDialogBase::LoadRemoteSnToMainParam()
{
    // Delegate to SettingsService which manages shared option storage
    SettingsService::Instance().LoadRemoteSnToMainParam();
}
