// CDialogQcSetting.cpp: 实现文件
//

#include "pch.h"
#include "SparkUfsPdt.h"
#include "afxdialogex.h"
#include "CDialogQcSetting.h"
#include "PubFunc.h"


// CDialogQcSetting 对话框

IMPLEMENT_DYNAMIC(CDialogQcSetting, CDialogEx)

CDialogQcSetting::CDialogQcSetting(CWnd* pParent /*=nullptr*/)
	: CDialogBase(IDD_DLG_QC_SETTING, pParent)
{

}

CDialogQcSetting::~CDialogQcSetting()
{
}

void CDialogQcSetting::DDX_CharArray(CDataExchange* pDX, int nIDC, char* szArray, int nArraySize)
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
		CString str('1', nLen + 1);
		::GetWindowTextA(hWndCtrl, str.GetBuffer(), nLen + 1);
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

void CDialogQcSetting::DDX_HexArray(CDataExchange* pDX, int nIDC, char* szArray, int nArraySize)
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
		::GetWindowTextA(hWndCtrl, str.GetBufferSetLength(nLen), nLen+1);
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

void CDialogQcSetting::DoDataExchange(CDataExchange* pDX)
{
	CDialogBase::DoDataExchange(pDX);
	PUFS_OPTION pOption = GetUfsOption();
	DDX_Check(pDX, IDC_CHECK_QC_DISK_INFO, pOption->qcPrm.bCheckDiskInfo);
	DDX_Text(pDX, IDC_EDIT_QC_SECTOR_CNT, pOption->qcPrm.n4KBCnt);
	DDX_Check(pDX, IDC_CHECK_QC_PNM, pOption->qcPrm.bCheckPnm);
	DDX_CharArray(pDX, IDC_EDIT_QC_PNM, pOption->qcPrm.pnm,sizeof(pOption->qcPrm.pnm));
	DDX_Check(pDX, IDC_CHECK_QC_MID_OID, pOption->qcPrm.bCheckMidOid);
	DDX_Text(pDX, IDC_EDIT_QC_BANK_INDEX, pOption->qcPrm.bankIdx);

	DDX_HexArray(pDX, IDC_EDIT_QC_MID, pOption->qcPrm.mid, sizeof(pOption->qcPrm.mid));
	DDX_HexArray(pDX, IDC_EDIT_QC_OID, pOption->qcPrm.oid, sizeof(pOption->qcPrm.oid));
	//DDX_CharArray(pDX, IDC_EDIT_QC_MID, pOption->qcPrm.mid, sizeof(pOption->qcPrm.mid));
	//DDX_CharArray(pDX, IDC_EDIT_QC_OID, pOption->qcPrm.oid, sizeof(pOption->qcPrm.oid));

	DDX_Check(pDX, IDC_CHECK_QC_MNM, pOption->qcPrm.bCheckMnm);
	DDX_CharArray(pDX, IDC_EDIT_QC_MNM, pOption->qcPrm.mnm, sizeof(pOption->qcPrm.mnm));
	DDX_Check(pDX, IDC_CHECK_QC_PRV_VER, pOption->qcPrm.bCheckPrv);
	//DDX_HexArray(pDX, IDC_EDIT_QC_PRV, pOption->qcPrm.prv, sizeof(pOption->qcPrm.prv));
	DDX_CharArray(pDX, IDC_EDIT_QC_PRV, pOption->qcPrm.prv, sizeof(pOption->qcPrm.prv));
	DDX_Check(pDX, IDC_CHECK_QC_MDT, pOption->qcPrm.bCheckMdt);
	DDX_CharArray(pDX, IDC_EDIT_QC_MDT, pOption->qcPrm.mdt, sizeof(pOption->qcPrm.mdt));
	DDX_Check(pDX, IDC_CHECK_QC_ISP_VER, pOption->qcPrm.bCheckIsp);
	DDX_CharArray(pDX, IDC_EDIT_QC_ISP_VER, pOption->qcPrm.isp, sizeof(pOption->qcPrm.isp));
	DDX_Check(pDX, IDC_CHECK_QC_SRAM_TEST, pOption->qcPrm.bCheckSramTest);
	DDX_CharArray(pDX, IDC_EDIT_QC_SRAM_PATH, pOption->qcPrm.szSramTestPath, sizeof(pOption->qcPrm.szSramTestPath));

}

BOOL CDialogQcSetting::OnInitDialog()
{
	CDialogBase::OnInitDialog();
	UpdateControlStates();
	return TRUE;
}

void CDialogQcSetting::UpdateControlStates()
{
	auto enableItem = [this](int id, BOOL enable)
	{
		if (CWnd* wnd = GetDlgItem(id))
		{
			wnd->EnableWindow(enable);
		}
	};

	BOOL enableDiskInfo = (IsDlgButtonChecked(IDC_CHECK_QC_DISK_INFO) == BST_CHECKED);
	enableItem(IDC_EDIT_QC_SECTOR_CNT, enableDiskInfo);

	BOOL enablePnm = (IsDlgButtonChecked(IDC_CHECK_QC_PNM) == BST_CHECKED);
	enableItem(IDC_EDIT_QC_PNM, enablePnm);

	BOOL enableMidOid = (IsDlgButtonChecked(IDC_CHECK_QC_MID_OID) == BST_CHECKED);
	enableItem(IDC_EDIT_QC_BANK_INDEX, enableMidOid);
	enableItem(IDC_EDIT_QC_MID, enableMidOid);
	enableItem(IDC_EDIT_QC_OID, enableMidOid);

	BOOL enableMnm = (IsDlgButtonChecked(IDC_CHECK_QC_MNM) == BST_CHECKED);
	enableItem(IDC_EDIT_QC_MNM, enableMnm);

	BOOL enablePrv = (IsDlgButtonChecked(IDC_CHECK_QC_PRV_VER) == BST_CHECKED);
	enableItem(IDC_EDIT_QC_PRV, enablePrv);

	BOOL enableMdt = (IsDlgButtonChecked(IDC_CHECK_QC_MDT) == BST_CHECKED);
	enableItem(IDC_EDIT_QC_MDT, enableMdt);

	BOOL enableIsp = (IsDlgButtonChecked(IDC_CHECK_QC_ISP_VER) == BST_CHECKED);
	enableItem(IDC_EDIT_QC_ISP_VER, enableIsp);

	BOOL enableSramTest = (IsDlgButtonChecked(IDC_CHECK_QC_SRAM_TEST) == BST_CHECKED);
	enableItem(IDC_EDIT_QC_SRAM_PATH, enableSramTest);
	enableItem(IDC_BTN_QC_SRAM_PATH_SEL, enableSramTest);
}

BEGIN_MESSAGE_MAP(CDialogQcSetting, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_QC_SRAM_PATH_SEL, &CDialogQcSetting::OnBnClickedBtnQcSramPathSel)
	ON_BN_CLICKED(IDC_CHECK_QC_DISK_INFO, &CDialogQcSetting::OnBnClickedCheckQcDiskInfo)
	ON_BN_CLICKED(IDC_CHECK_QC_PNM, &CDialogQcSetting::OnBnClickedCheckQcPnm)
	ON_BN_CLICKED(IDC_CHECK_QC_MID_OID, &CDialogQcSetting::OnBnClickedCheckQcMidOid)
	ON_BN_CLICKED(IDC_CHECK_QC_MNM, &CDialogQcSetting::OnBnClickedCheckQcMnm)
	ON_BN_CLICKED(IDC_CHECK_QC_PRV_VER, &CDialogQcSetting::OnBnClickedCheckQcPrv)
	ON_BN_CLICKED(IDC_CHECK_QC_MDT, &CDialogQcSetting::OnBnClickedCheckQcMdt)
	ON_BN_CLICKED(IDC_CHECK_QC_ISP_VER, &CDialogQcSetting::OnBnClickedCheckQcIsp)
	ON_BN_CLICKED(IDC_CHECK_QC_SRAM_TEST, &CDialogQcSetting::OnBnClickedCheckQcSramTest)
END_MESSAGE_MAP()


// CDialogQcSetting 消息处理程序

void CDialogQcSetting::OnBnClickedBtnQcSramPathSel()
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
		SetDlgItemText(IDC_EDIT_QC_SRAM_PATH, fileDlg.GetPathName());
		UpdateData(TRUE);
	}
}

void CDialogQcSetting::OnBnClickedCheckQcDiskInfo()
{
	UpdateControlStates();
}

void CDialogQcSetting::OnBnClickedCheckQcPnm()
{
	UpdateControlStates();
}

void CDialogQcSetting::OnBnClickedCheckQcMidOid()
{
	UpdateControlStates();
}

void CDialogQcSetting::OnBnClickedCheckQcMnm()
{
	UpdateControlStates();
}

void CDialogQcSetting::OnBnClickedCheckQcPrv()
{
	UpdateControlStates();
}

void CDialogQcSetting::OnBnClickedCheckQcMdt()
{
	UpdateControlStates();
}

void CDialogQcSetting::OnBnClickedCheckQcIsp()
{
	UpdateControlStates();
}

void CDialogQcSetting::OnBnClickedCheckQcSramTest()
{
	UpdateControlStates();
}
