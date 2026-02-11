#include "pch.h"
#include "CDialogBaseSet.h"
#include "Resource.h"
#include <atlconv.h>

IMPLEMENT_DYNAMIC(CDialogBaseSet, CDialogBase)

CDialogBaseSet::CDialogBaseSet(CWnd* pParent /*=nullptr*/)
    : CDialogBase(IDD_DIALOG_BASE_SET, pParent)
{
}

CDialogBaseSet::~CDialogBaseSet() = default;

void CDialogBaseSet::DoDataExchange(CDataExchange* pDX)
{
    CDialogBase::DoDataExchange(pDX);

    DDX_Radio(pDX, IDC_RADIO_SET_BASE0, m_portBaseSel);
    DDX_Radio(pDX, IDC_RADIO_SET_HWID, m_portMappingSel);
    DDX_Radio(pDX, IDC_RADIO_SET_FORCE_ROM_UPIU, m_forceRomMode);
    DDX_Check(pDX, IDC_CHECK_SET_REMOTE_SN, m_snSeparateIni);
    DDX_Text(pDX, IDC_EDIT_SET_REMOTE_SN, m_remoteSnPath);
    DDX_Text(pDX, IDC_EDIT_SET_REPORT, m_reportPath);
}

BOOL CDialogBaseSet::OnInitDialog()
{
    CDialogBase::OnInitDialog();

    PST_UFS_BASE_SETTING pBase = GetBaseSetting();
    if (pBase)
    {
        m_portBaseSel = (pBase->PortBaseSel != 0) ? 1 : 0;
        m_portMappingSel = (pBase->PortMappingSel != 0) ? 1 : 0;
        m_forceRomMode = (pBase->ForceRomMode != 0) ? 1 : 0;
        m_snSeparateIni = pBase->bSnSeparateIni;
        m_remoteSnPath = CString(pBase->szRemoteSnPath);
        m_reportPath = CString(pBase->szReportPath);
    }

    UpdateData(FALSE);
    return TRUE;
}

void CDialogBaseSet::OnOK()
{
    if (!UpdateData(TRUE))
    {
        return;
    }

    PST_UFS_BASE_SETTING pBase = GetBaseSetting();
    if (pBase)
    {
        pBase->PortBaseSel = m_portBaseSel;
        pBase->PortMappingSel = m_portMappingSel;
        pBase->ForceRomMode = m_forceRomMode;
        pBase->bSnSeparateIni = m_snSeparateIni;

        CStringA remoteA = CT2A(m_remoteSnPath);
        strcpy_s(pBase->szRemoteSnPath, sizeof(pBase->szRemoteSnPath), remoteA);

        CStringA reportA = CT2A(m_reportPath);
        strcpy_s(pBase->szReportPath, sizeof(pBase->szReportPath), reportA);

        TCHAR currentDirectory[MAX_PATH] = {};
        GetCurrentDirectory(MAX_PATH, currentDirectory);
        CString iniPath;
        iniPath.Format(_T("%s\\BoostSetting.ini"), currentDirectory);

        CString value;
        value.Format(_T("%d"), pBase->PortBaseSel);
        WritePrivateProfileString(_T("Base"), _T("PortBaseSel"), value, iniPath);
        value.Format(_T("%d"), pBase->PortMappingSel);
        WritePrivateProfileString(_T("Base"), _T("PortMappingSel"), value, iniPath);
        value.Format(_T("%d"), pBase->ForceRomMode);
        WritePrivateProfileString(_T("Base"), _T("ForceRomMode"), value, iniPath);
        value.Format(_T("%d"), pBase->bSnSeparateIni ? 1 : 0);
        WritePrivateProfileString(_T("Base"), _T("SnSeparateIni"), value, iniPath);
        WritePrivateProfileString(_T("Base"), _T("RemoteSnPath"), m_remoteSnPath, iniPath);
        WritePrivateProfileString(_T("Base"), _T("ReportPath"), m_reportPath, iniPath);
    }

    MessageBox(_T("Save successful."), _T("Setting"), MB_OK);
}

BEGIN_MESSAGE_MAP(CDialogBaseSet, CDialogBase)
    ON_BN_CLICKED(IDC_BTN_SET_REMOTE_SN_SEL, &CDialogBaseSet::OnBnClickedBtnSetRemoteSnSel)
    ON_BN_CLICKED(IDC_BTN_SET_REPORT_SEL, &CDialogBaseSet::OnBnClickedBtnSetReportSel)
END_MESSAGE_MAP()

void CDialogBaseSet::OnBnClickedBtnSetRemoteSnSel()
{
    TCHAR currentDirectory[MAX_PATH] = {};
    GetCurrentDirectory(MAX_PATH, currentDirectory);
    CString initialDir(currentDirectory);

    CFileDialog dlg(TRUE, _T("ini"), _T("Setting.ini"), OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
        _T("INI Files (*.ini)|*.ini|All Files (*.*)|*.*||"));
    dlg.m_ofn.lpstrInitialDir = initialDir;

    if (dlg.DoModal() == IDOK)
    {
        m_remoteSnPath = dlg.GetPathName();
        UpdateData(FALSE);
    }
}

void CDialogBaseSet::OnBnClickedBtnSetReportSel()
{
    TCHAR currentDirectory[MAX_PATH] = {};
    GetCurrentDirectory(MAX_PATH, currentDirectory);
    CString initialDir(currentDirectory);

    CFolderPickerDialog dlg(initialDir, OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST, this, 0);
    if (dlg.DoModal() == IDOK)
    {
        m_reportPath = dlg.GetPathName();
        UpdateData(FALSE);
    }
}
