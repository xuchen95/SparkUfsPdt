// CDialogMainSetting.cpp: 实现文件
//

#include "pch.h"
#include "SparkUfsPdt.h"
#include "afxdialogex.h"
#include "CDialogMainSetting.h"
#include "PubFunc.h"
#include "IspMarkCache.h"
#include "SettingsService.h"
// CDialogMainSetting 对话框

IMPLEMENT_DYNAMIC(CDialogMainSetting, CDialogEx)

CDialogMainSetting::CDialogMainSetting(CWnd* pParent /*=nullptr*/)
	: CDialogBase(IDD_DLG_MAIN_SETTING, pParent)
{

}

CDialogMainSetting::~CDialogMainSetting()
{
}


void CDialogMainSetting::DDX_CharArray(CDataExchange* pDX, int nIDC, char* szArray, int nArraySize)
{
	// 校验参数合法性
	if (!pDX || !szArray || nArraySize <= 0)
		return;

	// 获取编辑框控件句柄
	HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);
	if (!hWndCtrl)
		return;

	// 双向数据交换逻辑
	if (pDX->m_bSaveAndValidate)
	{
		// 方向1：控件 → CHAR数组（保存数据）
		int nLen = ::GetWindowTextLengthA(hWndCtrl);

		// 获取文本并填充到数组
		CString str('1',nLen+1);
		::GetWindowTextA(hWndCtrl, str.GetBuffer(), nLen+1);
		memcpy(szArray, str.GetBuffer(), nArraySize);
	}
	else
	{
		// 方向2：CHAR数组 → 控件（初始化显示）
		CString str(szArray, nArraySize);
		str.AppendChar('\0');
		::SetWindowText(hWndCtrl, str);
	}
}

void CDialogMainSetting::DDX_HexArray(CDataExchange* pDX, int nIDC, char* szArray, int nArraySize)
{
	// 校验参数合法性
	if (!pDX || !szArray || nArraySize <= 0)
		return;

	// 获取编辑框控件句柄
	HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);
	if (!hWndCtrl)
		return;

	// 双向数据交换逻辑
	if (pDX->m_bSaveAndValidate)
	{
		// 方向1：控件 → CHAR数组（保存数据）
		int nLen = ::GetWindowTextLengthA(hWndCtrl);
		CString str;
		::GetWindowTextA(hWndCtrl, str.GetBufferSetLength(nLen), nLen + 1);
		str.ReleaseBuffer();
		CPubFunc::HexToBytes(str, reinterpret_cast<BYTE*>(szArray), nArraySize);
	}
	else
	{
		// 方向2：CHAR数组 → 控件（初始化显示）
		CString str;
		for (int i = 0; i < nArraySize; ++i)
		{
			if (i != 0 && (0x00 == szArray[i]))
				break;
			str.AppendFormat("%2.2X", (BYTE)szArray[i]);
		}
		::SetWindowText(hWndCtrl, str);
	}
}

void CDialogMainSetting::DDX_SN(CDataExchange* pDX, int nIDC, UINT32& nSn)
{
	// 校验参数合法性
	if (!pDX)
		return;

	// 获取编辑框控件句柄
	HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);
	if (!hWndCtrl)
		return;

	// 双向数据交换逻辑
	if (pDX->m_bSaveAndValidate)
	{
		// 方向1：控件 → CHAR数组（保存数据）
		int nLen = ::GetWindowTextLengthA(hWndCtrl);
		CString str;
		::GetWindowTextA(hWndCtrl, str.GetBufferSetLength(nLen), nLen + 1);
		nSn = strtoul(str, nullptr, 16);
		str.ReleaseBuffer();



	}
	else
	{
		// 方向2：CHAR数组 → 控件（初始化显示）
		CString str;
		str.Format(_T("%08X"), nSn);
		::SetWindowText(hWndCtrl, str);
	}
}

void CDialogMainSetting::DoDataExchange(CDataExchange* pDX)
{
	CDialogBase::DoDataExchange(pDX);
	PUFS_OPTION pOption = GetUfsOption();
	DDX_Check(pDX, IDC_CHECK_BURNIN_TEST, pOption->mainPrm.bBurnInTest);
	// TesterFW 控制
	DDX_Check(pDX, IDC_CHECK_EN_DL_TESTERFW, pOption->mainPrm.bDLTesterFW);
	DDX_CharArray(pDX, IDC_EDIT_TESTER_FW_PATH, pOption->mainPrm.strTesterFwPath,sizeof(pOption->mainPrm.strTesterFwPath));
	
	// ISP 控制
	DDX_Check(pDX, IDC_CHECK_ISP_DL, pOption->mainPrm.bDLISP);
	DDX_CharArray(pDX, IDC_EDIT_ISP_FW_PATH, pOption->mainPrm.strIspPath, sizeof(pOption->mainPrm.strIspPath));
	
	DDX_Check(pDX, IDC_CHECK_CID_DL, pOption->mainPrm.bDLCID);
	DDX_CBIndex(pDX, IDC_CB_FUNC_SEL, pOption->mainPrm.funcSel);
	DDX_Text(pDX, IDC_EDIT_BANK_IDX, pOption->mainPrm.bankIdx);

	DDX_HexArray(pDX, IDC_EDIT_MID, pOption->mainPrm.mid, sizeof(pOption->mainPrm.mid));
	DDX_HexArray(pDX, IDC_EDIT_OID, pOption->mainPrm.oid, sizeof(pOption->mainPrm.oid));

	DDX_CharArray(pDX, IDC_EDIT_PNM, pOption->mainPrm.pnm, sizeof(pOption->mainPrm.pnm));


	CPubFunc::ReadSerialNumberRange(pOption->mainPrm.psn_start, pOption->mainPrm.psn_end);
	DDX_SN(pDX, IDC_EDIT_SN_START, pOption->mainPrm.psn_start);
	DDX_SN(pDX, IDC_EDIT_SN_END, pOption->mainPrm.psn_end);
	UpdateDateInMdtField();
	DDX_CharArray(pDX, IDC_EDIT_SN_MDT, pOption->mainPrm.mdt, sizeof(pOption->mainPrm.mdt));

	DDX_CharArray(pDX, IDC_EDIT_SN_PRV, pOption->mainPrm.prv, sizeof(pOption->mainPrm.prv));
	DDX_CharArray(pDX, IDC_EDIT_SN_MANU_NAME, pOption->mainPrm.mnm, sizeof(pOption->mainPrm.mnm));

}

BEGIN_MESSAGE_MAP(CDialogMainSetting, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_TESTERFW_PATH_SEL, &CDialogMainSetting::OnBnClickedBtnTesterfwPathSel)
	ON_BN_CLICKED(IDC_BTN_ISP_PATH_SEL, &CDialogMainSetting::OnBnClickedBtnIspPathSel)
	ON_BN_CLICKED(IDC_CHECK_EN_DL_TESTERFW, &CDialogMainSetting::OnBnClickedCheckDlTesterfw)
	ON_BN_CLICKED(IDC_CHECK_ISP_DL, &CDialogMainSetting::OnBnClickedCheckIspDl)
	ON_BN_CLICKED(IDC_CHECK_CID_DL, &CDialogMainSetting::OnBnClickedCheckCidDl)
	ON_BN_CLICKED(IDC_BTN_READ_ISP_VER, &CDialogMainSetting::OnBnClickedBtnReadIspVer)
END_MESSAGE_MAP()


// CDialogMainSetting 消息处理程序

void CDialogMainSetting::OnBnClickedBtnTesterfwPathSel()
{
	char currentDirectory[MAX_PATH] = {};
	GetCurrentDirectory(MAX_PATH, currentDirectory);
	CString initialDir;
	initialDir.Format(_T("%hs\\Tester_3350"), currentDirectory);

	CFileDialog fileDlg(TRUE,
		_T("bin"),
		_T(""),
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		_T("Binary Files (*.bin)|*.bin|All Files(*.*)|*.*||"));
	fileDlg.m_ofn.lpstrInitialDir = initialDir;

	if (fileDlg.DoModal() == IDOK)
	{
		
		SetDlgItemText(IDC_EDIT_TESTER_FW_PATH, fileDlg.GetPathName());
		UpdateData(TRUE);
	}
}

void CDialogMainSetting::OnBnClickedBtnIspPathSel()
{
	char currentDirectory[MAX_PATH] = {};
	GetCurrentDirectory(MAX_PATH, currentDirectory);
	CString initialDir;
	initialDir.Format(_T("%hs\\BIN"), currentDirectory);

	CFileDialog fileDlg(TRUE,
		_T("bin"),
		_T(""),
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		_T("Binary Files (*.bin)|*.bin|All Files(*.*)|*.*||"));
	fileDlg.m_ofn.lpstrInitialDir = initialDir;

	if (fileDlg.DoModal() == IDOK)
	{
		SetDlgItemText(IDC_EDIT_ISP_FW_PATH, fileDlg.GetPathName());
		UpdateData(TRUE);
	}
}

BOOL CDialogMainSetting::OnInitDialog()
{
	CDialogBase::OnInitDialog();
	
	// 从远程路径加载序列号数据到 MAIN_PARAM
	LoadRemoteSnToMainParam();
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
	enableItem(IDC_EDIT_ISP_FW_PATH, enableIsp);
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

void CDialogMainSetting::OnBnClickedBtnReadIspVer()
{
	// Read encoded ISP mark from cache and display hex string in IDC_EDIT_READ_ISP_VER
	unsigned char encoded[16] = { 0 };
	if (!spark::ufspdt::IspMarkCache::Instance().ReadIspMarkFromFile(
		SettingsService::Instance().GetIspPath(), 
		reinterpret_cast<char*>(encoded), 
		sizeof(encoded) )
		)
	{
		if (CWnd* pEdit = GetDlgItem(IDC_EDIT_READ_ISP_VER))
			pEdit->SetWindowText(_T(""));
		return;
	}

	CString hexW;
	for (int i = 0; i < 8; ++i)
	{
		CString part;
		part.Format(_T("%02X"), static_cast<unsigned int>(encoded[i]));
		if (2==i) hexW += _T('-');
		hexW += part;
	}
	if (CWnd* pEdit = GetDlgItem(IDC_EDIT_READ_ISP_VER))
	{
		pEdit->SetWindowText(hexW);
	}

}

void CDialogMainSetting::UpdateDateInMdtField()
{
	time_t t = time(nullptr);
	std::tm tm;
	localtime_s(&tm, &t);
	int year = tm.tm_year + 1900 - 2000;
	int month = tm.tm_mon + 1;
	char date[5] = { 0 };
	snprintf(date, sizeof(date), "%02d%02d", year, month);
	if (PUFS_OPTION pOption = GetUfsOption())
	{
		memcpy(pOption->mainPrm.mdt, date, sizeof(pOption->mainPrm.mdt));
	}
}
