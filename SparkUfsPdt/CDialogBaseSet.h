#pragma once
#include "CDialogBase.h"

class CDialogBaseSet : public CDialogBase
{
    DECLARE_DYNAMIC(CDialogBaseSet)

public:
    explicit CDialogBaseSet(CWnd* pParent = nullptr);
    ~CDialogBaseSet() override;

protected:
    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;
    void OnOK() override;
    afx_msg void OnBnClickedBtnSetRemoteSnSel();
    afx_msg void OnBnClickedBtnSetReportSel();

    DECLARE_MESSAGE_MAP()

private:
    int m_portBaseSel = 0;
    int m_portMappingSel = 0;
    int m_forceRomMode = 0;
    BOOL m_snSeparateIni = FALSE;
    CString m_remoteSnPath;
    CString m_reportPath;
};
