// CDialogMainSetting.cpp: 实现文件
//

#include "pch.h"
#include "SparkUfsPdt.h"
#include "afxdialogex.h"
#include "CDialogMainSetting.h"


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
		m_oid = pOption->mainPrm.oid;
		m_pnm = pOption->mainPrm.pnm;
		m_psn_start = pOption->mainPrm.psn_start;
		m_psn_end = pOption->mainPrm.psn_end;
		m_mdt = pOption->mainPrm.mdt;
		m_prv = pOption->mainPrm.prv;
		m_mnm = pOption->mainPrm.mnm;
		
		UpdateData(FALSE);
	}

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
		
		// 转换 ANSI 字符串为 WCHAR
		MultiByteToWideChar(CP_ACP, 0, (LPCSTR)m_oid, -1, pOption->mainPrm.oid, sizeof(pOption->mainPrm.oid) / sizeof(WCHAR));
		MultiByteToWideChar(CP_ACP, 0, (LPCSTR)m_pnm, -1, pOption->mainPrm.pnm, sizeof(pOption->mainPrm.pnm) / sizeof(WCHAR));
		MultiByteToWideChar(CP_ACP, 0, (LPCSTR)m_psn_start, -1, pOption->mainPrm.psn_start, sizeof(pOption->mainPrm.psn_start) / sizeof(WCHAR));
		MultiByteToWideChar(CP_ACP, 0, (LPCSTR)m_psn_end, -1, pOption->mainPrm.psn_end, sizeof(pOption->mainPrm.psn_end) / sizeof(WCHAR));
		MultiByteToWideChar(CP_ACP, 0, (LPCSTR)m_mdt, -1, pOption->mainPrm.mdt, sizeof(pOption->mainPrm.mdt) / sizeof(WCHAR));
		MultiByteToWideChar(CP_ACP, 0, (LPCSTR)m_prv, -1, pOption->mainPrm.prv, sizeof(pOption->mainPrm.prv) / sizeof(WCHAR));
		MultiByteToWideChar(CP_ACP, 0, (LPCSTR)m_mnm, -1, pOption->mainPrm.mnm, sizeof(pOption->mainPrm.mnm) / sizeof(WCHAR));
	}
}
