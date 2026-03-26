// CDialogMainSetting.cpp: 实现文件
//

#include "pch.h"
#include "SparkUfsPdt.h"
#include "afxdialogex.h"
#include "CDialogMainSetting.h"

static CString GetCurrentYearMonthText()
{
	SYSTEMTIME st = {};
	GetLocalTime(&st);
	CString mdt;
	mdt.Format(_T("%02d%02d"), st.wYear % 100, st.wMonth);
	return mdt;
}

// CDialogMainSetting 对话框

IMPLEMENT_DYNAMIC(CDialogMainSetting, CDialogEx)

CDialogMainSetting::CDialogMainSetting(CWnd* pParent /*=nullptr*/)
	: CDialogBase(IDD_DLG_MAIN_SETTING, pParent)
{

}

CDialogMainSetting::~CDialogMainSetting()
{
}

void CDialogMainSetting::DoDataExchange(CDataExchange* pDX)
{
	CDialogBase::DoDataExchange(pDX);
	
	// TesterFW 控制
	DDX_Check(pDX, IDC_CHECK_EN_DL_TESTERFW, m_bDLTesterFW);
	DDX_Text(pDX, IDC_EDIT_TESTER_FW_PATH, m_strTesterFwPath);
	
	// ISP 控制
	DDX_Check(pDX, IDC_CHECK_ISP_DL, m_bDLISP);
	DDX_Text(pDX, IDC_EDIT_TESTER_FW_PATH2, m_strIspPath);
	
	// CID 控制
	DDX_Check(pDX, IDC_CHECK_CID_DL, m_bDLCID);
	DDX_CBIndex(pDX, IDC_CB_FUNC_SEL, m_funcSel);
	DDX_Text(pDX, IDC_EDIT_BANK_IDX, m_bankIdx);
	DDX_Text(pDX, IDC_EDIT_MID, m_mid);
	DDX_Text(pDX, IDC_EDIT_OID, m_oid);
	DDX_Text(pDX, IDC_EDIT_PNM, m_pnm);
	DDX_Text(pDX, IDC_EDIT_SN_START, m_psn_start);
	DDX_Text(pDX, IDC_EDIT_SN_END, m_psn_end);
	DDX_Text(pDX, IDC_EDIT_SN_MDT, m_mdt);
	DDX_Text(pDX, IDC_EDIT_SN_PRV, m_prv);
	DDX_Text(pDX, IDC_EDIT_SN_MANU_NAME, m_mnm);
}

BEGIN_MESSAGE_MAP(CDialogMainSetting, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_TESTERFW_PATH_SEL, &CDialogMainSetting::OnBnClickedBtnTesterfwPathSel)
	ON_BN_CLICKED(IDC_BTN_ISP_PATH_SEL, &CDialogMainSetting::OnBnClickedBtnIspPathSel)
	ON_BN_CLICKED(IDC_CHECK_EN_DL_TESTERFW, &CDialogMainSetting::OnBnClickedCheckDlTesterfw)
	ON_BN_CLICKED(IDC_CHECK_ISP_DL, &CDialogMainSetting::OnBnClickedCheckIspDl)
	ON_BN_CLICKED(IDC_CHECK_CID_DL, &CDialogMainSetting::OnBnClickedCheckCidDl)
	ON_EN_CHANGE(IDC_EDIT_SN_MDT, &CDialogMainSetting::OnEnChangeEditSnMdt)
END_MESSAGE_MAP()


// CDialogMainSetting 消息处理程序

void CDialogMainSetting::OnBnClickedBtnTesterfwPathSel()
{
	char currentDirectory[MAX_PATH] = {};
	GetCurrentDirectory(MAX_PATH, currentDirectory);
	CString initialDir;
	initialDir.Format(_T("%hs"), currentDirectory);

	CFileDialog fileDlg(TRUE,
		_T("bin"),
		_T(""),
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		_T("Binary Files (*.bin)|*.bin|All Files(*.*)|*.*||"));
	fileDlg.m_ofn.lpstrInitialDir = initialDir;

	if (fileDlg.DoModal() == IDOK)
	{
		UpdateData(TRUE);
		m_strTesterFwPath = fileDlg.GetPathName();
		UpdateData(FALSE);
	}
}

void CDialogMainSetting::OnBnClickedBtnIspPathSel()
{
	char currentDirectory[MAX_PATH] = {};
	GetCurrentDirectory(MAX_PATH, currentDirectory);
	CString initialDir;
	initialDir.Format(_T("%hs"), currentDirectory);

	CFileDialog fileDlg(TRUE,
		_T("bin"),
		_T(""),
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		_T("Binary Files (*.bin)|*.bin|All Files(*.*)|*.*||"));
	fileDlg.m_ofn.lpstrInitialDir = initialDir;

	if (fileDlg.DoModal() == IDOK)
	{
		UpdateData(TRUE);
		m_strIspPath = fileDlg.GetPathName();
		UpdateData(FALSE);
	}
}

BOOL CDialogMainSetting::OnInitDialog()
{
	CDialogBase::OnInitDialog();
	
	// 从远程路径加载序列号数据到 MAIN_PARAM
	LoadRemoteSnToMainParam();
	
	// 从 UFS_OPTION 中加载数据
	PUFS_OPTION pOption = GetUfsOption();
	if (pOption)
	{
		m_bDLTesterFW = pOption->mainPrm.bDLTesterFW;
		m_strTesterFwPath = CString(pOption->mainPrm.strTesterFwPath);
		
		m_bDLISP = pOption->mainPrm.bDLISP;
		m_strIspPath = CString(pOption->mainPrm.strIspPath);
		
		m_bDLCID = pOption->mainPrm.bDLCID;
		m_funcSel = pOption->mainPrm.funcSel;
		m_bankIdx = CString(pOption->mainPrm.bankIdx[0]);
		m_mid = CString(pOption->mainPrm.mid[0]);
		
		// 使用指定长度构造 CString，避免依赖 null 终止符
		m_oid = CString(pOption->mainPrm.oid, wcsnlen(pOption->mainPrm.oid, _countof(pOption->mainPrm.oid)));
		m_pnm = CString(pOption->mainPrm.pnm, wcsnlen(pOption->mainPrm.pnm, _countof(pOption->mainPrm.pnm)));
		m_psn_start = CString(pOption->mainPrm.psn_start, wcsnlen(pOption->mainPrm.psn_start, _countof(pOption->mainPrm.psn_start)));
		m_psn_end = CString(pOption->mainPrm.psn_end, wcsnlen(pOption->mainPrm.psn_end, _countof(pOption->mainPrm.psn_end)));
		m_psn_mask = CString(pOption->mainPrm.psn_mask, wcsnlen(pOption->mainPrm.psn_mask, _countof(pOption->mainPrm.psn_mask)));
		m_mdt = CString(pOption->mainPrm.mdt, wcsnlen(pOption->mainPrm.mdt, _countof(pOption->mainPrm.mdt)));
		m_prv = CString(pOption->mainPrm.prv, wcsnlen(pOption->mainPrm.prv, _countof(pOption->mainPrm.prv)));
		m_mnm = CString(pOption->mainPrm.mnm, wcsnlen(pOption->mainPrm.mnm, _countof(pOption->mainPrm.mnm)));
		m_meto = CString(pOption->mainPrm.meto, wcsnlen(pOption->mainPrm.meto, _countof(pOption->mainPrm.meto)));
	}

	// MDT 自动更新为当前年月（yyMM），例如 2603
	m_mdt = GetCurrentYearMonthText();
	if (pOption)
	{
		ZeroMemory(pOption->mainPrm.mdt, sizeof(pOption->mainPrm.mdt));
		CStringW mdtW = CT2W(m_mdt);
		wcsncpy_s(pOption->mainPrm.mdt, _countof(pOption->mainPrm.mdt), mdtW, _TRUNCATE);
	}

	UpdateData(FALSE);
	UpdateControlStates();
	return TRUE;
}

void CDialogMainSetting::UpdateControlStates()
{
	auto enableItem = [this](int id, BOOL enable)
	{
		if (CWnd* wnd = GetDlgItem(id))
		{
			wnd->EnableWindow(enable);
		}
	};

	BOOL enableTesterFw = (IsDlgButtonChecked(IDC_CHECK_EN_DL_TESTERFW) == BST_CHECKED);
	enableItem(IDC_EDIT_TESTER_FW_PATH, enableTesterFw);
	enableItem(IDC_BTN_TESTERFW_PATH_SEL, enableTesterFw);

	BOOL enableIsp = (IsDlgButtonChecked(IDC_CHECK_ISP_DL) == BST_CHECKED);
	enableItem(IDC_EDIT_TESTER_FW_PATH2, enableIsp);
	enableItem(IDC_BTN_ISP_PATH_SEL, enableIsp);

	BOOL enableCid = (IsDlgButtonChecked(IDC_CHECK_CID_DL) == BST_CHECKED);
	enableItem(IDC_EDIT_BANK_IDX, enableCid);
	enableItem(IDC_EDIT_MID, enableCid);
	enableItem(IDC_EDIT_OID, enableCid);
	enableItem(IDC_EDIT_PNM, enableCid);
	enableItem(IDC_EDIT_SN_START, enableCid);
	enableItem(IDC_EDIT_SN_END, enableCid);
	enableItem(IDC_EDIT_SN_MDT, enableCid);
	enableItem(IDC_EDIT_SN_PRV, enableCid);
	enableItem(IDC_EDIT_SN_MANU_NAME, enableCid);
}

void CDialogMainSetting::OnBnClickedCheckDlTesterfw()
{
	UpdateControlStates();
}

void CDialogMainSetting::OnBnClickedCheckIspDl()
{
	UpdateControlStates();
}

void CDialogMainSetting::OnBnClickedCheckCidDl()
{
	UpdateControlStates();
}

void CDialogMainSetting::SaveDataToUfsOption()
{
	UpdateData(TRUE);
	
	PUFS_OPTION pOption = GetUfsOption();
	if (pOption)
	{
		// 保存前强制刷新 MDT 为当前年月（yyMM）
		m_mdt = GetCurrentYearMonthText();

		pOption->mainPrm.funcSel = m_funcSel;
		pOption->mainPrm.bDLTesterFW = m_bDLTesterFW;
		strcpy_s(pOption->mainPrm.strTesterFwPath, sizeof(pOption->mainPrm.strTesterFwPath), (LPCSTR)m_strTesterFwPath);
		
		pOption->mainPrm.bDLISP = m_bDLISP;
		strcpy_s(pOption->mainPrm.strIspPath, sizeof(pOption->mainPrm.strIspPath), (LPCSTR)m_strIspPath);
		
		pOption->mainPrm.bDLCID = m_bDLCID;
		pOption->mainPrm.funcSel = m_funcSel;
		if (!m_bankIdx.IsEmpty())
			pOption->mainPrm.bankIdx[0] = m_bankIdx[0];
		if (!m_mid.IsEmpty())
			pOption->mainPrm.mid[0] = m_mid[0];
		
		// 转换为 WCHAR，不包含结束符
		auto copyToWchar = [](WCHAR* dest, size_t destCount, const CString& src) {
			ZeroMemory(dest, destCount * sizeof(WCHAR));
			if (src.IsEmpty())
				return;
			
			// 使用 CT2W 转换，确保在 MBCS 和 Unicode 模式下都正确
			CStringW srcW = CT2W(src);
			// 修复：为 null 终止符预留空间
			int copyLen = min((int)srcW.GetLength(), (int)destCount - 1);
			if (copyLen > 0)
				wcsncpy_s(dest, destCount, srcW, copyLen);
		};
		
		copyToWchar(pOption->mainPrm.oid, _countof(pOption->mainPrm.oid), m_oid);
		copyToWchar(pOption->mainPrm.pnm, _countof(pOption->mainPrm.pnm), m_pnm);
		copyToWchar(pOption->mainPrm.psn_start, _countof(pOption->mainPrm.psn_start), m_psn_start);
		copyToWchar(pOption->mainPrm.psn_end, _countof(pOption->mainPrm.psn_end), m_psn_end);
		copyToWchar(pOption->mainPrm.psn_mask, _countof(pOption->mainPrm.psn_mask), m_psn_mask);
		copyToWchar(pOption->mainPrm.mdt, _countof(pOption->mainPrm.mdt), m_mdt);
		copyToWchar(pOption->mainPrm.prv, _countof(pOption->mainPrm.prv), m_prv);
		copyToWchar(pOption->mainPrm.mnm, _countof(pOption->mainPrm.mnm), m_mnm);
		copyToWchar(pOption->mainPrm.meto, _countof(pOption->mainPrm.meto), m_meto);
	}

	UpdateData(FALSE);
}

void CDialogMainSetting::OnEnChangeEditSnMdt()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogBase::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}
