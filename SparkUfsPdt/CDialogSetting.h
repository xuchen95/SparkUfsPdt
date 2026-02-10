#pragma once
#include "afxdialogex.h"
#include "CDialogBase.h"
#include <afxcmn.h>
#include "CDialogMainSetting.h"
#include "CDialogQcSetting.h"


// CDialogSetting 对话框

class CDialogSetting : public CDialogBase
{
	DECLARE_DYNAMIC(CDialogSetting)

public:
	CDialogSetting(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDialogSetting();
	static BOOL LoadFromIni(const CString& path, PUFS_OPTION pOption);
	void SetLastSavePath(const CString& path); // set path for Save
	void SetVisiblePages(bool showMain, bool showQc);

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_SETTING };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();

	afx_msg void OnTcnSelchangeTabParamPage(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedBtnSettingSaveAs();
	afx_msg void OnBnClickedBtnSettingSave();
	afx_msg void OnBnClickedBtnSettingOk();
	afx_msg void OnBnClickedBtnSettingCancel();

	DECLARE_MESSAGE_MAP()

private:
	void ShowPage(int index);
	BOOL SaveToFile(const CString& path, bool saveMain, bool saveQc);
	void SaveToOption(bool saveMain, bool saveQc);

	CTabCtrl m_tabParamPage;
	CDialogMainSetting m_mainSetting;
	CDialogQcSetting m_qcSetting;
	int m_currentPage = 0;
	CString m_lastSavePath;
	bool m_showMain = true;
	bool m_showQc = true;
	int m_tabPages[2] = { 0, 1 };
	int m_tabPageCount = 0;
};
