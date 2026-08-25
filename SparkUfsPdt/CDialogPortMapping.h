#pragma once
#include "afxdialogex.h"


// CDialogPortMapping 对话框

class CDialogPortMapping : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogPortMapping)

public:
	CDialogPortMapping(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDialogPortMapping();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_PORT_MAPPING };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedPmBtnScanDevice();
	afx_msg void OnBnClickedPmBtnWriteTesterId();
};
