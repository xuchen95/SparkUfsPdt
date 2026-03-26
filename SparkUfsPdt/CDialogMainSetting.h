#pragma once
#include "afxdialogex.h"
#include "CDialogBase.h"

// CDialogMainSetting 对话框

class CDialogMainSetting : public CDialogBase
{
	DECLARE_DYNAMIC(CDialogMainSetting)

public:
	CDialogMainSetting(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDialogMainSetting();
	void SaveDataToUfsOption();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_MAIN_SETTING };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();

	afx_msg void OnBnClickedBtnTesterfwPathSel();
	afx_msg void OnBnClickedBtnIspPathSel();
	afx_msg void OnBnClickedCheckDlTesterfw();
	afx_msg void OnBnClickedCheckIspDl();
	afx_msg void OnBnClickedCheckCidDl();

	DECLARE_MESSAGE_MAP()

private:
	void UpdateControlStates();

	// TesterFW 控制
	BOOL m_bDLTesterFW = FALSE;
	CString m_strTesterFwPath;
	
	// ISP 控制
	BOOL m_bDLISP = FALSE;
	CString m_strIspPath;
	
	// CID 控制
	BOOL m_bDLCID = FALSE;
	int m_funcSel = 0;
	CString m_bankIdx;
	CString m_mid;
	CString m_oid;
	CString m_pnm;
	CString m_psn_start;
	CString m_psn_end;
	CString m_psn_mask;
	CString m_mdt;
	CString m_prv;
	CString m_mnm;
	CString m_meto;
public:
	afx_msg void OnEnChangeEditSnMdt();
};
