#pragma once
#include "afxdialogex.h"
#include "CDialogBase.h"

// CDialogQcSetting 对话框

class CDialogQcSetting : public CDialogBase
{
	DECLARE_DYNAMIC(CDialogQcSetting)

public:
	CDialogQcSetting(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDialogQcSetting();
	void SaveDataToUfsOption();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_QC_SETTING };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();

	afx_msg void OnBnClickedBtnQcSramPathSel();
	afx_msg void OnBnClickedCheckQcDiskInfo();
	afx_msg void OnBnClickedCheckQcPnm();
	afx_msg void OnBnClickedCheckQcMidOid();
	afx_msg void OnBnClickedCheckQcMnm();
	afx_msg void OnBnClickedCheckQcPrv();
	afx_msg void OnBnClickedCheckQcMdt();
	afx_msg void OnBnClickedCheckQcIsp();
	afx_msg void OnBnClickedCheckQcSramTest();

	DECLARE_MESSAGE_MAP()

private:
	void UpdateControlStates();

	BOOL m_bCheckDiskInfo = FALSE;
	int m_sectorCount = 0;
	BOOL m_bCheckPnm = FALSE;
	CString m_pnm;
	BOOL m_bCheckMidOid = FALSE;
	CString m_bankIdx;
	CString m_mid;
	CString m_oid;
	BOOL m_bCheckMnm = FALSE;
	CString m_mnm;
	BOOL m_bCheckPrv = FALSE;
	CString m_prv;
	BOOL m_bCheckMdt = FALSE;
	CString m_mdt;
	BOOL m_bCheckIsp = FALSE;
	CString m_isp;
	BOOL m_bCheckSramTest = FALSE;
	CString m_sramTestPath;
};
