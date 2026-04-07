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

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_QC_SETTING };
#endif

protected:
	void DDX_CharArray(CDataExchange* pDX, int nIDC, char* szArray, int nArraySize);
	void DDX_HexArray(CDataExchange* pDX, int nIDC, char* szArray, int nArraySize);
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
};
