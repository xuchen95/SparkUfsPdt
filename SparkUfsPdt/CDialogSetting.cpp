// CDialogSetting.cpp: 实现文件
//

#include "pch.h"
#include "SparkUfsPdt.h"
#include "afxdialogex.h"
#include "CDialogSetting.h"


// CDialogSetting 对话框

IMPLEMENT_DYNAMIC(CDialogSetting, CDialogEx)

CDialogSetting::CDialogSetting(CWnd* pParent /*=nullptr*/)
	: CDialogBase(IDD_DLG_SETTING, pParent)
	, m_mainSetting(this)
	, m_qcSetting(this)
{

}

CDialogSetting::~CDialogSetting()
{
}

void CDialogSetting::SetVisiblePages(bool showMain, bool showQc)
{
	m_showMain = showMain;
	m_showQc = showQc;
}

BOOL CDialogSetting::LoadFromIni(const CString& path, PUFS_OPTION pOption)
{
	if (!pOption)
	{
		return FALSE;
	}

	CStringA pathA(path);
	auto readInt = [&](LPCSTR section, LPCSTR key, int defValue)
	{
		return GetPrivateProfileIntA(section, key, defValue, pathA);
	};
	auto readString = [&](LPCSTR section, LPCSTR key, char* buffer, DWORD size)
	{
		return GetPrivateProfileStringA(section, key, "", buffer, size, pathA);
	};

	char buffer[256] = {};

	pOption->mainPrm.funcSel = readInt("Main", "FuncSel", pOption->mainPrm.funcSel);
	pOption->mainPrm.bDLTesterFW = readInt("Main", "bDLTesterFW", pOption->mainPrm.bDLTesterFW);
	readString("Main", "strTesterFwPath", buffer, sizeof(buffer));
	if (buffer[0])
		strcpy_s(pOption->mainPrm.strTesterFwPath, sizeof(pOption->mainPrm.strTesterFwPath), buffer);
	pOption->mainPrm.bDLISP = readInt("Main", "bDLISP", pOption->mainPrm.bDLISP);
	readString("Main", "strIspPath", buffer, sizeof(buffer));
	if (buffer[0])
		strcpy_s(pOption->mainPrm.strIspPath, sizeof(pOption->mainPrm.strIspPath), buffer);
	pOption->mainPrm.bDLCID = readInt("Main", "bDLCID", pOption->mainPrm.bDLCID);
	readString("Main", "bankIdx", buffer, sizeof(buffer));
	if (buffer[0])
		pOption->mainPrm.bankIdx[0] = buffer[0];
	readString("Main", "mid", buffer, sizeof(buffer));
	if (buffer[0])
		pOption->mainPrm.mid[0] = buffer[0];
	
	readString("Main", "oid", buffer, sizeof(buffer));
	if (buffer[0])
	{
		int result = MultiByteToWideChar(CP_ACP, 0, buffer, -1, pOption->mainPrm.oid, sizeof(pOption->mainPrm.oid) / sizeof(WCHAR));
		if (result == 0)
			pOption->mainPrm.oid[0] = L'\0';
	}
	
	readString("Main", "pnm", buffer, sizeof(buffer));
	if (buffer[0])
	{
		int result = MultiByteToWideChar(CP_ACP, 0, buffer, -1, pOption->mainPrm.pnm, sizeof(pOption->mainPrm.pnm) / sizeof(WCHAR));
		if (result == 0)
			pOption->mainPrm.pnm[0] = L'\0';
	}
	
	readString("Main", "psn_start", buffer, sizeof(buffer));
	if (buffer[0])
	{
		int result = MultiByteToWideChar(CP_ACP, 0, buffer, -1, pOption->mainPrm.psn_start, sizeof(pOption->mainPrm.psn_start) / sizeof(WCHAR));
		if (result == 0)
			pOption->mainPrm.psn_start[0] = L'\0';
	}
	
	readString("Main", "psn_end", buffer, sizeof(buffer));
	if (buffer[0])
	{
		int result = MultiByteToWideChar(CP_ACP, 0, buffer, -1, pOption->mainPrm.psn_end, sizeof(pOption->mainPrm.psn_end) / sizeof(WCHAR));
		if (result == 0)
			pOption->mainPrm.psn_end[0] = L'\0';
	}
	
	readString("Main", "mdt", buffer, sizeof(buffer));
	if (buffer[0])
	{
		int result = MultiByteToWideChar(CP_ACP, 0, buffer, -1, pOption->mainPrm.mdt, sizeof(pOption->mainPrm.mdt) / sizeof(WCHAR));
		if (result == 0)
			pOption->mainPrm.mdt[0] = L'\0';
	}
	
	readString("Main", "prv", buffer, sizeof(buffer));
	if (buffer[0])
	{
		int result = MultiByteToWideChar(CP_ACP, 0, buffer, -1, pOption->mainPrm.prv, sizeof(pOption->mainPrm.prv) / sizeof(WCHAR));
		if (result == 0)
			pOption->mainPrm.prv[0] = L'\0';
	}
	
	readString("Main", "mnm", buffer, sizeof(buffer));
	if (buffer[0])
	{
		int result = MultiByteToWideChar(CP_ACP, 0, buffer, -1, pOption->mainPrm.mnm, sizeof(pOption->mainPrm.mnm) / sizeof(WCHAR));
		if (result == 0)
			pOption->mainPrm.mnm[0] = L'\0';
	}
	
	readString("Main", "szFlowName", buffer, sizeof(buffer));
	if (buffer[0])
		strcpy_s(pOption->mainPrm.szFlowName, sizeof(pOption->mainPrm.szFlowName), buffer);

	pOption->qcPrm.bCheckDiskInfo = readInt("QC", "bCheckDiskInfo", pOption->qcPrm.bCheckDiskInfo);
	pOption->qcPrm.sectorCnt = static_cast<ULONG>(readInt("QC", "sectorCnt", static_cast<int>(pOption->qcPrm.sectorCnt)));
	pOption->qcPrm.bCheckPnm = readInt("QC", "bCheckPnm", pOption->qcPrm.bCheckPnm);
	readString("QC", "pnm", buffer, sizeof(buffer));
	if (buffer[0])
	{
		int result = MultiByteToWideChar(CP_ACP, 0, buffer, -1, pOption->qcPrm.pnm, sizeof(pOption->qcPrm.pnm) / sizeof(WCHAR));
		if (result == 0)
			pOption->qcPrm.pnm[0] = L'\0';
	}
	
	pOption->qcPrm.bCheckMidOid = readInt("QC", "bCheckMidOid", pOption->qcPrm.bCheckMidOid);
	readString("QC", "bankIdx", buffer, sizeof(buffer));
	if (buffer[0])
		pOption->qcPrm.bankIdx[0] = buffer[0];
	readString("QC", "mid", buffer, sizeof(buffer));
	if (buffer[0])
		pOption->qcPrm.mid[0] = buffer[0];
	
	readString("QC", "oid", buffer, sizeof(buffer));
	if (buffer[0])
	{
		int result = MultiByteToWideChar(CP_ACP, 0, buffer, -1, pOption->qcPrm.oid, sizeof(pOption->qcPrm.oid) / sizeof(WCHAR));
		if (result == 0)
			pOption->qcPrm.oid[0] = L'\0';
	}
	
	pOption->qcPrm.bCheckMnm = readInt("QC", "bCheckMnm", pOption->qcPrm.bCheckMnm);
	readString("QC", "mnm", buffer, sizeof(buffer));
	if (buffer[0])
	{
		int result = MultiByteToWideChar(CP_ACP, 0, buffer, -1, pOption->qcPrm.mnm, sizeof(pOption->qcPrm.mnm) / sizeof(WCHAR));
		if (result == 0)
			pOption->qcPrm.mnm[0] = L'\0';
	}
	
	pOption->qcPrm.bCheckPrv = readInt("QC", "bCheckPrv", pOption->qcPrm.bCheckPrv);
	readString("QC", "prv", buffer, sizeof(buffer));
	if (buffer[0])
	{
		int result = MultiByteToWideChar(CP_ACP, 0, buffer, -1, pOption->qcPrm.prv, sizeof(pOption->qcPrm.prv) / sizeof(WCHAR));
		if (result == 0)
			pOption->qcPrm.prv[0] = L'\0';
	}
	
	pOption->qcPrm.bCheckMdt = readInt("QC", "bCheckMdt", pOption->qcPrm.bCheckMdt);
	readString("QC", "mdt", buffer, sizeof(buffer));
	if (buffer[0])
	{
		int result = MultiByteToWideChar(CP_ACP, 0, buffer, -1, pOption->qcPrm.mdt, sizeof(pOption->qcPrm.mdt) / sizeof(WCHAR));
		if (result == 0)
			pOption->qcPrm.mdt[0] = L'\0';
	}
	
	pOption->qcPrm.bCheckIsp = readInt("QC", "bCheckIsp", pOption->qcPrm.bCheckIsp);
	readString("QC", "isp", buffer, sizeof(buffer));
	if (buffer[0])
		strcpy_s(pOption->qcPrm.isp, sizeof(pOption->qcPrm.isp), buffer);
	pOption->qcPrm.bCheckSramTest = readInt("QC", "bCheckSramTest", pOption->qcPrm.bCheckSramTest);
	readString("QC", "szSramTestPath", buffer, sizeof(buffer));
	if (buffer[0])
		strcpy_s(pOption->qcPrm.szSramTestPath, sizeof(pOption->qcPrm.szSramTestPath), buffer);

	return TRUE;
}

void CDialogSetting::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_PARAM_PAGE, m_tabParamPage);
}

BOOL CDialogSetting::OnInitDialog()
{
	if (!CDialogEx::OnInitDialog())
	{
		return FALSE;
	}

	m_tabPageCount = 0;
	if (m_showMain)
	{
		m_tabParamPage.InsertItem(m_tabPageCount, _T("Main Setting"));
		m_tabPages[m_tabPageCount++] = 0;
	}
	if (m_showQc)
	{
		m_tabParamPage.InsertItem(m_tabPageCount, _T("QC Setting"));
		m_tabPages[m_tabPageCount++] = 1;
	}

	m_mainSetting.SetUfsOption(GetUfsOption());
	m_qcSetting.SetUfsOption(GetUfsOption());

	if (!m_mainSetting.Create(IDD_DLG_MAIN_SETTING, &m_tabParamPage))
	{
		MessageBox(_T("Failed to create Main Setting dialog."), _T("Error"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	if (!m_qcSetting.Create(IDD_DLG_QC_SETTING, &m_tabParamPage))
	{
		MessageBox(_T("Failed to create QC Setting dialog."), _T("Error"), MB_OK | MB_ICONERROR);
		return FALSE;
	}

	m_mainSetting.ModifyStyle(WS_POPUP, WS_CHILD);
	m_qcSetting.ModifyStyle(WS_POPUP, WS_CHILD);
	m_mainSetting.ModifyStyleEx(WS_EX_APPWINDOW, 0);
	m_qcSetting.ModifyStyleEx(WS_EX_APPWINDOW, 0);

	CRect tabRect;
	m_tabParamPage.GetClientRect(&tabRect);
	m_tabParamPage.AdjustRect(FALSE, &tabRect);

	m_mainSetting.SetWindowPos(nullptr, tabRect.left, tabRect.top, tabRect.Width(), tabRect.Height(), SWP_NOZORDER | SWP_HIDEWINDOW);
	m_qcSetting.SetWindowPos(nullptr, tabRect.left, tabRect.top, tabRect.Width(), tabRect.Height(), SWP_NOZORDER | SWP_HIDEWINDOW);

	if (m_tabPageCount > 0)
	{
		m_tabParamPage.SetCurSel(0);
		ShowPage(0);
	}

	return TRUE;
}

void CDialogSetting::ShowPage(int index)
{
	if (index < 0 || index >= m_tabPageCount)
		return;

	int pageId = m_tabPages[index];
	m_mainSetting.ShowWindow(pageId == 0 ? SW_SHOW : SW_HIDE);
	m_qcSetting.ShowWindow(pageId == 1 ? SW_SHOW : SW_HIDE);
	m_currentPage = pageId;
}

void CDialogSetting::SaveToOption(bool saveMain, bool saveQc)
{
	if (saveMain)
	{
		m_mainSetting.SaveDataToUfsOption();
	}
	if (saveQc)
	{
		m_qcSetting.SaveDataToUfsOption();
	}
}

BOOL CDialogSetting::SaveToFile(const CString& path, bool saveMain, bool saveQc)
{
	if (path.IsEmpty())
	{
		MessageBox(_T("Invalid file path."), _T("Error"), MB_ICONERROR);
		return FALSE;
	}

	SaveToOption(saveMain, saveQc);

	PUFS_OPTION pOption = GetUfsOption();
	if (!pOption)
	{
		MessageBox(_T("Save failed: invalid option pointer."), _T("Setting"), MB_ICONERROR);
		return FALSE;
	}

	auto writeValue = [&](LPCTSTR section, LPCTSTR key, const CString& value)
	{
		if (!WritePrivateProfileString(section, key, value, path))
		{
			DWORD error = GetLastError();
			CString errorMsg;
			errorMsg.Format(_T("Failed to write %s\\%s (Error: %lu)"), section, key, error);
			MessageBox(errorMsg, _T("Write Error"), MB_OK | MB_ICONWARNING);
			return FALSE;
		}
		return TRUE;
	};

	CString value;
	if (saveMain)
	{
		value.Format(_T("%d"), pOption->mainPrm.funcSel);
		if (!writeValue(_T("Main"), _T("FuncSel"), value)) return FALSE;
		value.Format(_T("%d"), pOption->mainPrm.bDLTesterFW);
		if (!writeValue(_T("Main"), _T("bDLTesterFW"), value)) return FALSE;
		if (!writeValue(_T("Main"), _T("strTesterFwPath"), CString(pOption->mainPrm.strTesterFwPath))) return FALSE;
		value.Format(_T("%d"), pOption->mainPrm.bDLISP);
		if (!writeValue(_T("Main"), _T("bDLISP"), value)) return FALSE;
		if (!writeValue(_T("Main"), _T("strIspPath"), CString(pOption->mainPrm.strIspPath))) return FALSE;
		value.Format(_T("%d"), pOption->mainPrm.bDLCID);
		if (!writeValue(_T("Main"), _T("bDLCID"), value)) return FALSE;
		value.Format(_T("%c"), pOption->mainPrm.bankIdx[0]);
		if (!writeValue(_T("Main"), _T("bankIdx"), value)) return FALSE;
		value.Format(_T("%c"), pOption->mainPrm.mid[0]);
		if (!writeValue(_T("Main"), _T("mid"), value)) return FALSE;
		if (!writeValue(_T("Main"), _T("oid"), CString(pOption->mainPrm.oid))) return FALSE;
		if (!writeValue(_T("Main"), _T("pnm"), CString(pOption->mainPrm.pnm))) return FALSE;
		if (!writeValue(_T("Main"), _T("psn_start"), CString(pOption->mainPrm.psn_start))) return FALSE;
		if (!writeValue(_T("Main"), _T("psn_end"), CString(pOption->mainPrm.psn_end))) return FALSE;
		if (!writeValue(_T("Main"), _T("mdt"), CString(pOption->mainPrm.mdt))) return FALSE;
		if (!writeValue(_T("Main"), _T("prv"), CString(pOption->mainPrm.prv))) return FALSE;
		if (!writeValue(_T("Main"), _T("mnm"), CString(pOption->mainPrm.mnm))) return FALSE;
		if (!writeValue(_T("Main"), _T("szFlowName"), CString(pOption->mainPrm.szFlowName))) return FALSE;
	}

	if (saveQc)
	{
		value.Format(_T("%d"), pOption->qcPrm.bCheckDiskInfo);
		if (!writeValue(_T("QC"), _T("bCheckDiskInfo"), value)) return FALSE;
		value.Format(_T("%lu"), pOption->qcPrm.sectorCnt);
		if (!writeValue(_T("QC"), _T("sectorCnt"), value)) return FALSE;
		value.Format(_T("%d"), pOption->qcPrm.bCheckPnm);
		if (!writeValue(_T("QC"), _T("bCheckPnm"), value)) return FALSE;
		if (!writeValue(_T("QC"), _T("pnm"), CString(pOption->qcPrm.pnm))) return FALSE;
		value.Format(_T("%d"), pOption->qcPrm.bCheckMidOid);
		if (!writeValue(_T("QC"), _T("bCheckMidOid"), value)) return FALSE;
		value.Format(_T("%c"), pOption->qcPrm.bankIdx[0]);
		if (!writeValue(_T("QC"), _T("bankIdx"), value)) return FALSE;
		value.Format(_T("%c"), pOption->qcPrm.mid[0]);
		if (!writeValue(_T("QC"), _T("mid"), value)) return FALSE;
		if (!writeValue(_T("QC"), _T("oid"), CString(pOption->qcPrm.oid))) return FALSE;
		value.Format(_T("%d"), pOption->qcPrm.bCheckMnm);
		if (!writeValue(_T("QC"), _T("bCheckMnm"), value)) return FALSE;
		if (!writeValue(_T("QC"), _T("mnm"), CString(pOption->qcPrm.mnm))) return FALSE;
		value.Format(_T("%d"), pOption->qcPrm.bCheckPrv);
		if (!writeValue(_T("QC"), _T("bCheckPrv"), value)) return FALSE;
		if (!writeValue(_T("QC"), _T("prv"), CString(pOption->qcPrm.prv))) return FALSE;
		value.Format(_T("%d"), pOption->qcPrm.bCheckMdt);
		if (!writeValue(_T("QC"), _T("bCheckMdt"), value)) return FALSE;
		if (!writeValue(_T("QC"), _T("mdt"), CString(pOption->qcPrm.mdt))) return FALSE;
		value.Format(_T("%d"), pOption->qcPrm.bCheckIsp);
		if (!writeValue(_T("QC"), _T("bCheckIsp"), value)) return FALSE;
		if (!writeValue(_T("QC"), _T("isp"), CString(pOption->qcPrm.isp))) return FALSE;
		value.Format(_T("%d"), pOption->qcPrm.bCheckSramTest);
		if (!writeValue(_T("QC"), _T("bCheckSramTest"), value)) return FALSE;
		if (!writeValue(_T("QC"), _T("szSramTestPath"), CString(pOption->qcPrm.szSramTestPath))) return FALSE;
	}

	return TRUE;
}

void CDialogSetting::OnTcnSelchangeTabParamPage(NMHDR* pNMHDR, LRESULT* pResult)
{
	int sel = m_tabParamPage.GetCurSel();
	if (sel < 0)
		return;

	ShowPage(sel);
	*pResult = 0;
}

void CDialogSetting::OnBnClickedBtnSettingSaveAs()
{
	CFileDialog dlg(FALSE, _T("ini"), _T("setting.ini"), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("INI Files (*.ini)|*.ini|All Files (*.*)|*.*||"));

	if (dlg.DoModal() == IDOK)
	{
		m_lastSavePath = dlg.GetPathName();
		if (SaveToFile(m_lastSavePath, true, true))
		{
			MessageBox(_T("Save successful."), _T("Setting"), MB_OK);
		}
	}
}

void CDialogSetting::SetLastSavePath(const CString& path)
{
	m_lastSavePath = path;
}

void CDialogSetting::OnBnClickedBtnSettingSave()
{
	if (m_lastSavePath.IsEmpty())
	{
		OnBnClickedBtnSettingSaveAs();
		return;
	}

	if (SaveToFile(m_lastSavePath, true, true))
	{
		MessageBox(_T("Save successful."), _T("Setting"), MB_OK);
	}
}

void CDialogSetting::OnBnClickedBtnSettingOk()
{
	SaveToOption(true, true);
	EndDialog(IDOK);
}

void CDialogSetting::OnBnClickedBtnSettingCancel()
{
	EndDialog(IDCANCEL);
}

BEGIN_MESSAGE_MAP(CDialogSetting, CDialogEx)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_PARAM_PAGE, &CDialogSetting::OnTcnSelchangeTabParamPage)
	ON_BN_CLICKED(IDC_BTN_SETTING_SAVE_AS, &CDialogSetting::OnBnClickedBtnSettingSaveAs)
	ON_BN_CLICKED(IDC_BTN_SETTING_SAVE, &CDialogSetting::OnBnClickedBtnSettingSave)
	ON_BN_CLICKED(IDC_BTN_SETTING_OK, &CDialogSetting::OnBnClickedBtnSettingOk)
	ON_BN_CLICKED(IDC_BTN_SETTING_CANCEL, &CDialogSetting::OnBnClickedBtnSettingCancel)
END_MESSAGE_MAP()


// CDialogSetting 消息处理程序
