#pragma once
#include <afxdialogex.h>

typedef struct main_param
{
	CHAR szFlowName[32];
	INT funcSel;
	BOOL bDLTesterFW;
	CHAR strTesterFwPath[1024];
	BOOL bDLISP;
	CHAR strIspPath[1024];
	BOOL bDLCID;
	//CID
	UINT bankIdx;
	CHAR mid[2];
	CHAR oid[31];
	CHAR pnm[16];
	UINT32 psn_start;
	UINT32 psn_end;
	CHAR psn_mask[31];
	CHAR mdt[4];
	CHAR prv[4];
	CHAR mnm[8];
	CHAR meto[4];

}MAIN_PARAM, * PMAIN_PARAM;

typedef struct qc_param
{ 
	BOOL bCheckDiskInfo;
	ULONG n4KBCnt;
	BOOL bCheckPnm;
	CHAR pnm[16];
	BOOL bCheckMidOid;
	UINT bankIdx;
	CHAR mid[2];
	CHAR oid[31];
	BOOL bCheckMnm;
	CHAR mnm[8];
	BOOL bCheckPrv;
	CHAR prv[4];
	BOOL bCheckMdt;
	CHAR mdt[4];
	BOOL bCheckIsp;
	CHAR isp[32];
	BOOL bCheckSramTest;
	CHAR szSramTestPath[1024];

}QC_PARAM,*PQC_PARAM;

typedef struct ufs_option
{
	MAIN_PARAM mainPrm;
	QC_PARAM qcPrm;
}UFS_OPTION, * PUFS_OPTION;

typedef struct UFS_BASE_SETTING
{
	int PortBaseSel;
	int PortMappingSel;
	int ForceRomMode;
	BOOL bSnSeparateIni;
	CHAR szRemoteSnPath[1024];
	CHAR szReportPath[1024];
}ST_UFS_BASE_SETTING, * PST_UFS_BASE_SETTING;

class CDialogBase :public CDialogEx
{
public:
	DECLARE_DYNAMIC(CDialogBase)
	explicit CDialogBase(UINT nIDTemplate, CWnd* pParent = nullptr);

	static PUFS_OPTION GetSharedUfsOption();
	void SetUfsOption(PUFS_OPTION pOption);
	PUFS_OPTION GetUfsOption() const;

	static PST_UFS_BASE_SETTING GetSharedBaseSetting();
	PST_UFS_BASE_SETTING GetBaseSetting() const;
	static void LoadBaseSettingFromIni(const CString& path);
	static void LoadRemoteSnToMainParam();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

private:
	static UFS_OPTION s_sharedOption;
	PUFS_OPTION m_pUfsOption = nullptr;
	static ST_UFS_BASE_SETTING s_baseOption;
	PST_UFS_BASE_SETTING m_pBaseOption = nullptr;
};

