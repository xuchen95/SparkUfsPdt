// CDialogSetting.cpp: 实现文件
//

#include "pch.h"
#include "SparkUfsPdt.h"
#include "afxdialogex.h"
#include "CDialogSetting.h"
#include "PubFunc.h"


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

	auto readString = [&](LPCSTR section, LPCSTR key, char* buffer, DWORD size) -> DWORD
	{
		if (buffer == nullptr || size == 0)
		{
			return 0;
		}

		buffer[0] = '\0';
		return GetPrivateProfileStringA(section, key, "", buffer, size, pathA);
	};

	auto readUInt = [&](LPCSTR section, LPCSTR key, UINT defValue)
	{
		char numberBuffer[64] = {};
		readString(section, key, numberBuffer, static_cast<DWORD>(sizeof(numberBuffer)));
		if (numberBuffer[0] == '\0')
		{
			return defValue;
		}

		return static_cast<UINT>(strtoul(numberBuffer, nullptr, 0));
	};

	auto readUInt32 = [&](LPCSTR section, LPCSTR key, UINT32 defValue)
	{
		char numberBuffer[64] = {};
		readString(section, key, numberBuffer, static_cast<DWORD>(sizeof(numberBuffer)));
		if (numberBuffer[0] == '\0')
		{
			return defValue;
		}

		return static_cast<UINT32>(strtoull(numberBuffer, nullptr, 16));
	};

	char buffer[1024] = {};

	pOption->mainPrm.bBurnInTest = readInt("Main", "bBurnInTest", pOption->mainPrm.bBurnInTest);
	pOption->mainPrm.funcSel = readInt("Main", "FuncSel", pOption->mainPrm.funcSel);
	pOption->mainPrm.bDLTesterFW = readInt("Main", "bDLTesterFW", pOption->mainPrm.bDLTesterFW);
	readString("Main", "strTesterFwPath", buffer, static_cast<DWORD>(sizeof(buffer)));
	if (buffer[0] != '\0')
	{
		strcpy_s(pOption->mainPrm.strTesterFwPath, sizeof(pOption->mainPrm.strTesterFwPath), buffer);
	}

	pOption->mainPrm.bDLISP = readInt("Main", "bDLISP", pOption->mainPrm.bDLISP);
	readString("Main", "strIspPath", buffer, static_cast<DWORD>(sizeof(buffer)));
	if (buffer[0] != '\0')
	{
		strcpy_s(pOption->mainPrm.strIspPath, sizeof(pOption->mainPrm.strIspPath), buffer);
	}

	pOption->mainPrm.bDLCID = readInt("Main", "bDLCID", pOption->mainPrm.bDLCID);
	pOption->mainPrm.bankIdx = readUInt("Main", "bankIdx", pOption->mainPrm.bankIdx);

	readString("Main", "mid", buffer, static_cast<DWORD>(sizeof(buffer)));
	if (buffer[0] != '\0')
	{
		CPubFunc::HexToBytes(CString(buffer), reinterpret_cast<BYTE*>(pOption->mainPrm.mid), sizeof(pOption->mainPrm.mid));
	}

	readString("Main", "oid", buffer, static_cast<DWORD>(sizeof(buffer)));
	if (buffer[0] != '\0')
	{
		CPubFunc::HexToBytes(CString(buffer), reinterpret_cast<BYTE*>(pOption->mainPrm.oid), sizeof(pOption->mainPrm.oid));
	}

	readString("Main", "pnm", buffer, static_cast<DWORD>(sizeof(buffer)));
	if (buffer[0] != '\0')
	{
		strcpy_s(pOption->mainPrm.pnm, sizeof(pOption->mainPrm.pnm), buffer);
	}

	pOption->mainPrm.psn_start = readUInt32("Main", "psn_start", pOption->mainPrm.psn_start);
	pOption->mainPrm.psn_end = readUInt32("Main", "psn_end", pOption->mainPrm.psn_end);

	readString("Main", "psn_mask", buffer, static_cast<DWORD>(sizeof(buffer)));
	if (buffer[0] != '\0')
	{
		strcpy_s(pOption->mainPrm.psn_mask, sizeof(pOption->mainPrm.psn_mask), buffer);
	}

	readString("Main", "mdt", buffer, static_cast<DWORD>(sizeof(buffer)));
	if (buffer[0] != '\0')
	{
		memcpy(pOption->mainPrm.mdt, buffer, sizeof(pOption->mainPrm.mdt));
	}

	readString("Main", "prv", buffer, static_cast<DWORD>(sizeof(buffer)));
	if (buffer[0] != '\0')
	{
		//HexStringToByteArray(buffer, pOption->mainPrm.prv, sizeof(pOption->mainPrm.prv));
		memcpy(pOption->mainPrm.prv, buffer, sizeof(pOption->mainPrm.prv));
	}

	readString("Main", "mnm", buffer, static_cast<DWORD>(sizeof(buffer)));
	if (buffer[0] != '\0')
	{
		memcpy(pOption->mainPrm.mnm, buffer, sizeof(pOption->mainPrm.mnm));
	}

	readString("Main", "meto", buffer, static_cast<DWORD>(sizeof(buffer)));
	if (buffer[0] != '\0')
	{
		CPubFunc::HexToBytes(CString(buffer), reinterpret_cast<BYTE*>(pOption->mainPrm.meto), sizeof(pOption->mainPrm.meto));
	}

	readString("Main", "szFlowName", buffer, static_cast<DWORD>(sizeof(buffer)));
	if (buffer[0] != '\0')
	{
		strcpy_s(pOption->mainPrm.szFlowName, sizeof(pOption->mainPrm.szFlowName), buffer);
	}

	// QC Section
	pOption->qcPrm.bCheckDiskInfo = readInt("QC", "bCheckDiskInfo", pOption->qcPrm.bCheckDiskInfo);
	pOption->qcPrm.n4KBCnt = static_cast<ULONG>(readUInt("QC", "4KB_CNT", static_cast<UINT>(pOption->qcPrm.n4KBCnt)));
	pOption->qcPrm.bCheckPnm = readInt("QC", "bCheckPnm", pOption->qcPrm.bCheckPnm);

	readString("QC", "pnm", buffer, static_cast<DWORD>(sizeof(buffer)));
	if (buffer[0] != '\0')
	{
		strcpy_s(pOption->qcPrm.pnm, sizeof(pOption->qcPrm.pnm), buffer);
	}

	pOption->qcPrm.bCheckMidOid = readInt("QC", "bCheckMidOid", pOption->qcPrm.bCheckMidOid);
	pOption->qcPrm.bankIdx = readUInt("QC", "bankIdx", pOption->qcPrm.bankIdx);

	readString("QC", "mid", buffer, static_cast<DWORD>(sizeof(buffer)));
	if (buffer[0] != '\0')
	{
		CPubFunc::HexToBytes(CString(buffer), reinterpret_cast<BYTE*>(pOption->qcPrm.mid), sizeof(pOption->qcPrm.mid));
	}

	readString("QC", "oid", buffer, static_cast<DWORD>(sizeof(buffer)));
	if (buffer[0] != '\0')
	{
		CPubFunc::HexToBytes(CString(buffer), reinterpret_cast<BYTE*>(pOption->qcPrm.oid), sizeof(pOption->qcPrm.oid));
	}

	pOption->qcPrm.bCheckMnm = readInt("QC", "bCheckMnm", pOption->qcPrm.bCheckMnm);

	readString("QC", "mnm", buffer, static_cast<DWORD>(sizeof(buffer)));
	if (buffer[0] != '\0')
	{
		memcpy(pOption->qcPrm.mnm, buffer, sizeof(pOption->qcPrm.mnm));
	}

	pOption->qcPrm.bCheckPrv = readInt("QC", "bCheckPrv", pOption->qcPrm.bCheckPrv);

	readString("QC", "prv", buffer, static_cast<DWORD>(sizeof(buffer)));
	if (buffer[0] != '\0')
	{
		//HexStringToByteArray(buffer, pOption->qcPrm.prv, sizeof(pOption->qcPrm.prv));  // ✓ 修正：写入 qcPrm.prv
		memcpy(pOption->qcPrm.prv, buffer, sizeof(pOption->qcPrm.prv));
	}

	pOption->qcPrm.bCheckMdt = readInt("QC", "bCheckMdt", pOption->qcPrm.bCheckMdt);

	readString("QC", "mdt", buffer, static_cast<DWORD>(sizeof(buffer)));
	if (buffer[0] != '\0')
	{
		memcpy(pOption->qcPrm.mdt, buffer, sizeof(pOption->qcPrm.mdt));
	}

	pOption->qcPrm.bCheckIsp = readInt("QC", "bCheckIsp", pOption->qcPrm.bCheckIsp);

	readString("QC", "isp", buffer, static_cast<DWORD>(sizeof(buffer)));
	if (buffer[0] != '\0')
	{
		strcpy_s(pOption->qcPrm.isp, sizeof(pOption->qcPrm.isp), buffer);
	}

	pOption->qcPrm.bCheckSramTest = readInt("QC", "bCheckSramTest", pOption->qcPrm.bCheckSramTest);

	readString("QC", "szSramTestPath", buffer, static_cast<DWORD>(sizeof(buffer)));
	if (buffer[0] != '\0')
	{
		strcpy_s(pOption->qcPrm.szSramTestPath, sizeof(pOption->qcPrm.szSramTestPath), buffer);
	}

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

BOOL CDialogSetting::SaveToFile(const CString& path, bool saveMain/*=true*/, bool saveQc/*=true*/)
{
	if (path.IsEmpty())
	{
		MessageBox(_T("Invalid file path."), _T("Error"), MB_ICONERROR);
		return FALSE;
	}

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
		value.Format(_T("%d"), pOption->mainPrm.bBurnInTest);
		if (!writeValue(_T("Main"), _T("bBurnInTest"), value)) return FALSE;

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

		value.Format(_T("%u"), pOption->mainPrm.bankIdx);
		if (!writeValue(_T("Main"), _T("bankIdx"), value)) return FALSE;

		// mid - 转换为十六进制字符串
		if (!writeValue(_T("Main"), _T("mid"), CPubFunc::BytesToHex(reinterpret_cast<const BYTE*>(pOption->mainPrm.mid), sizeof(pOption->mainPrm.mid)))) return FALSE;
		
		// oid - 转换为十六进制字符串
		if (!writeValue(_T("Main"), _T("oid"), CPubFunc::BytesToHex(reinterpret_cast<const BYTE*>(pOption->mainPrm.oid), sizeof(pOption->mainPrm.oid)))) return FALSE;
		
		// pnm - 字符串，保持原样
		if (!writeValue(_T("Main"), _T("pnm"), CString(pOption->mainPrm.pnm))) return FALSE;

		value.Format(_T("%08X"), pOption->mainPrm.psn_start);
		if (!writeValue(_T("Main"), _T("psn_start"), value)) return FALSE;

		value.Format(_T("%08X"), pOption->mainPrm.psn_end);
		if (!writeValue(_T("Main"), _T("psn_end"), value)) return FALSE;

		if (!writeValue(_T("Main"), _T("psn_mask"), CString(pOption->mainPrm.psn_mask))) return FALSE;
		
		// mdt - 二进制数据，按原样保存
		if (!writeValue(_T("Main"), _T("mdt"), CString(pOption->mainPrm.mdt, sizeof(pOption->mainPrm.mdt)))) return FALSE;
		
		// prv - 二进制数据，按原样保存
		if (!writeValue(_T("Main"), _T("prv"), CString(pOption->mainPrm.prv, sizeof(pOption->mainPrm.prv)))) return FALSE;
		
		// mnm - 二进制数据，按原样保存
		if (!writeValue(_T("Main"), _T("mnm"), CString(pOption->mainPrm.mnm, sizeof(pOption->mainPrm.mnm)))) return FALSE;
		
		// meto - 转换为十六进制字符串
		if (!writeValue(_T("Main"), _T("meto"), CPubFunc::BytesToHex(reinterpret_cast<const BYTE*>(pOption->mainPrm.meto), sizeof(pOption->mainPrm.meto)))) return FALSE;
		
		if (!writeValue(_T("Main"), _T("szFlowName"), CString(pOption->mainPrm.szFlowName))) return FALSE;
	}

	if (saveQc)
	{
		value.Format(_T("%d"), pOption->qcPrm.bCheckDiskInfo);
		if (!writeValue(_T("QC"), _T("bCheckDiskInfo"), value)) return FALSE;

		value.Format(_T("%lu"), pOption->qcPrm.n4KBCnt);
		if (!writeValue(_T("QC"), _T("4KB_CNT"), value)) return FALSE;

		value.Format(_T("%d"), pOption->qcPrm.bCheckPnm);
		if (!writeValue(_T("QC"), _T("bCheckPnm"), value)) return FALSE;

		if (!writeValue(_T("QC"), _T("pnm"), CString(pOption->qcPrm.pnm))) return FALSE;

		value.Format(_T("%d"), pOption->qcPrm.bCheckMidOid);
		if (!writeValue(_T("QC"), _T("bCheckMidOid"), value)) return FALSE;

		value.Format(_T("%u"), pOption->qcPrm.bankIdx);
		if (!writeValue(_T("QC"), _T("bankIdx"), value)) return FALSE;

		// mid - 转换为十六进制字符串
		if (!writeValue(_T("QC"), _T("mid"), CPubFunc::BytesToHex(reinterpret_cast<const BYTE*>(pOption->qcPrm.mid), sizeof(pOption->qcPrm.mid)))) return FALSE;
		
		// oid - 转换为十六进制字符串
		if (!writeValue(_T("QC"), _T("oid"), CPubFunc::BytesToHex(reinterpret_cast<const BYTE*>(pOption->qcPrm.oid), sizeof(pOption->qcPrm.oid)))) return FALSE;

		value.Format(_T("%d"), pOption->qcPrm.bCheckMnm);
		if (!writeValue(_T("QC"), _T("bCheckMnm"), value)) return FALSE;

		// mnm - 二进制数据
		if (!writeValue(_T("QC"), _T("mnm"), CString(pOption->qcPrm.mnm, sizeof(pOption->qcPrm.mnm)))) return FALSE;

		value.Format(_T("%d"), pOption->qcPrm.bCheckPrv);
		if (!writeValue(_T("QC"), _T("bCheckPrv"), value)) return FALSE;

		// prv - 转换为十六进制字符串
		if (!writeValue(_T("QC"), _T("prv"), CString(pOption->qcPrm.prv, sizeof(pOption->qcPrm.prv)))) return FALSE;

		value.Format(_T("%d"), pOption->qcPrm.bCheckMdt);
		if (!writeValue(_T("QC"), _T("bCheckMdt"), value)) return FALSE;

		// mdt - 二进制数据
		if (!writeValue(_T("QC"), _T("mdt"), CString(pOption->qcPrm.mdt, sizeof(pOption->qcPrm.mdt)))) return FALSE;

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
		m_mainSetting.UpdateData(TRUE);
		m_qcSetting.UpdateData(TRUE);
		if (SaveToFile(m_lastSavePath))
		{
			MessageBox(_T("Save successful."), _T("Setting"), MB_OK);
			EndDialog(IDOK);
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

	m_mainSetting.UpdateData(TRUE);
	m_qcSetting.UpdateData(TRUE);
	if (SaveToFile(m_lastSavePath))
	{
		MessageBox(_T("Save successful."), _T("Setting"), MB_OK);

		EndDialog(IDOK);
	}
}

void CDialogSetting::OnBnClickedBtnSettingOk()
{
	UpdateData(TRUE);
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
