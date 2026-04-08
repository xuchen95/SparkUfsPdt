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

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_MAIN_SETTING };
#endif

protected:
	void DDX_CharArray(CDataExchange* pDX, int nIDC, char* szArray, int nArraySize);
	void DDX_HexArray(CDataExchange* pDX, int nIDC, char* szArray, int nArraySize);
	void DDX_SN(CDataExchange* pDX, int nIDC, UINT32& nSn);
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

};
