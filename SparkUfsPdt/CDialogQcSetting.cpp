// CDialogQcSetting.cpp: 实现文件
//

#include "pch.h"
#include "SparkUfsPdt.h"
#include "afxdialogex.h"
#include "CDialogQcSetting.h"


// CDialogQcSetting 对话框

IMPLEMENT_DYNAMIC(CDialogQcSetting, CDialogEx)

CDialogQcSetting::CDialogQcSetting(CWnd* pParent /*=nullptr*/)
	: CDialogBase(IDD_DLG_QC_SETTING, pParent)
{

}

CDialogQcSetting::~CDialogQcSetting()
{
}

void CDialogQcSetting::DoDataExchange(CDataExchange* pDX)
{
	CDialogBase::DoDataExchange(pDX);

	DDX_Check(pDX, IDC_CHECK_QC_DISK_INFO, m_bCheckDiskInfo);
	DDX_Text(pDX, IDC_EDIT_QC_SECTOR_CNT, m_sectorCount);
	DDX_Check(pDX, IDC_CHECK_QC_PNM, m_bCheckPnm);
	DDX_Text(pDX, IDC_EDIT_QC_PNM, m_pnm);
	DDX_Check(pDX, IDC_CHECK_QC_MID_OID, m_bCheckMidOid);
	DDX_Text(pDX, IDC_EDIT_QC_BANK_INDEX, m_bankIdx);
	DDX_Text(pDX, IDC_EDIT_QC_MID, m_mid);
	DDX_Text(pDX, IDC_EDIT_QC_OID, m_oid);
	DDX_Check(pDX, IDC_CHECK_QC_MNM, m_bCheckMnm);
	DDX_Text(pDX, IDC_EDIT_QC_MNM, m_mnm);
	DDX_Check(pDX, IDC_CHECK_QC_PRV_VER, m_bCheckPrv);
	DDX_Text(pDX, IDC_EDIT_QC_PRV, m_prv);
	DDX_Check(pDX, IDC_CHECK_QC_MDT, m_bCheckMdt);
	DDX_Text(pDX, IDC_EDIT_QC_MDT, m_mdt);
	DDX_Check(pDX, IDC_CHECK_QC_ISP_VER, m_bCheckIsp);
	DDX_Text(pDX, IDC_EDIT_QC_ISP_VER, m_isp);
	DDX_Check(pDX, IDC_CHECK_QC_SRAM_TEST, m_bCheckSramTest);
	DDX_Text(pDX, IDC_EDIT_QC_SRAM_PATH, m_sramTestPath);
}

BOOL CDialogQcSetting::OnInitDialog()
{
	CDialogBase::OnInitDialog();

	PUFS_OPTION pOption = GetUfsOption();
	if (pOption)
	{
		m_bCheckDiskInfo = pOption->qcPrm.bCheckDiskInfo;
		m_sectorCount = pOption->qcPrm.sectorCnt;
		m_bCheckPnm = pOption->qcPrm.bCheckPnm;
		m_pnm = pOption->qcPrm.pnm;
		m_bCheckMidOid = pOption->qcPrm.bCheckMidOid;
		m_bankIdx.Format(_T("%c"), pOption->qcPrm.bankIdx[0]);
		m_mid.Format(_T("%c"), pOption->qcPrm.mid[0]);
		m_oid = pOption->qcPrm.oid;
		m_bCheckMnm = pOption->qcPrm.bCheckMnm;
		m_mnm = pOption->qcPrm.mnm;
		m_bCheckPrv = pOption->qcPrm.bCheckPrv;
		m_prv = pOption->qcPrm.prv;
		m_bCheckMdt = pOption->qcPrm.bCheckMdt;
		m_mdt = pOption->qcPrm.mdt;
		m_bCheckIsp = pOption->qcPrm.bCheckIsp;
		m_isp = pOption->qcPrm.isp;
		m_bCheckSramTest = pOption->qcPrm.bCheckSramTest;
		m_sramTestPath = pOption->qcPrm.szSramTestPath;

		UpdateData(FALSE);
	}

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

void CDialogQcSetting::SaveDataToUfsOption()
{
	UpdateData(TRUE);

	PUFS_OPTION pOption = GetUfsOption();
	if (pOption)
	{
		pOption->qcPrm.bCheckDiskInfo = m_bCheckDiskInfo;
		pOption->qcPrm.sectorCnt = m_sectorCount;
		pOption->qcPrm.bCheckPnm = m_bCheckPnm;
		MultiByteToWideChar(CP_ACP, 0, (LPCSTR)m_pnm, -1, pOption->qcPrm.pnm, sizeof(pOption->qcPrm.pnm) / sizeof(WCHAR));
		pOption->qcPrm.bCheckMidOid = m_bCheckMidOid;
		if (!m_bankIdx.IsEmpty())
			pOption->qcPrm.bankIdx[0] = m_bankIdx[0];
		if (!m_mid.IsEmpty())
			pOption->qcPrm.mid[0] = m_mid[0];
		MultiByteToWideChar(CP_ACP, 0, (LPCSTR)m_oid, -1, pOption->qcPrm.oid, sizeof(pOption->qcPrm.oid) / sizeof(WCHAR));
		pOption->qcPrm.bCheckMnm = m_bCheckMnm;
		MultiByteToWideChar(CP_ACP, 0, (LPCSTR)m_mnm, -1, pOption->qcPrm.mnm, sizeof(pOption->qcPrm.mnm) / sizeof(WCHAR));
		pOption->qcPrm.bCheckPrv = m_bCheckPrv;
		MultiByteToWideChar(CP_ACP, 0, (LPCSTR)m_prv, -1, pOption->qcPrm.prv, sizeof(pOption->qcPrm.prv) / sizeof(WCHAR));
		pOption->qcPrm.bCheckMdt = m_bCheckMdt;
		MultiByteToWideChar(CP_ACP, 0, (LPCSTR)m_mdt, -1, pOption->qcPrm.mdt, sizeof(pOption->qcPrm.mdt) / sizeof(WCHAR));
		pOption->qcPrm.bCheckIsp = m_bCheckIsp;
		strcpy_s(pOption->qcPrm.isp, sizeof(pOption->qcPrm.isp), (LPCSTR)m_isp);
		pOption->qcPrm.bCheckSramTest = m_bCheckSramTest;
		strcpy_s(pOption->qcPrm.szSramTestPath, sizeof(pOption->qcPrm.szSramTestPath), (LPCSTR)m_sramTestPath);
	}
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
		UpdateData(TRUE);
		m_sramTestPath = fileDlg.GetPathName();
		UpdateData(FALSE);
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
