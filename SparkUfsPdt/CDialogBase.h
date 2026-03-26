#pragma once
#include <afxdialogex.h>

typedef struct main_param
{
	char szFlowName[32];
	int funcSel;
	BOOL bDLTesterFW;
	char strTesterFwPath[1024];
	BOOL bDLISP;
	char strIspPath[1024];
	BOOL bDLCID;
	//CID
	char bankIdx[1];
	CHAR mid[1];
	WCHAR oid[31];
	WCHAR pnm[16];
	WCHAR psn_start[32];
	WCHAR psn_end[32];
	WCHAR psn_mask[32];
	WCHAR mdt[8];
	WCHAR prv[4];
	WCHAR mnm[16];
	WCHAR meto[32];

}MAIN_PARAM, * PMAIN_PARAM;

typedef struct qc_param
{ 
	BOOL bCheckDiskInfo;
	ULONG sectorCnt;
	BOOL bCheckPnm;
	WCHAR pnm[16];
	BOOL bCheckMidOid;
	char bankIdx[1];
	char mid[1];
	WCHAR oid[31];
	BOOL bCheckMnm;
	WCHAR mnm[16];
	BOOL bCheckPrv;
	WCHAR prv[4];
	BOOL bCheckMdt;
	WCHAR mdt[8];
	BOOL bCheckIsp;
	char isp[32];
	BOOL bCheckSramTest;
	char szSramTestPath[1024];

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
	void SetBaseSetting(PST_UFS_BASE_SETTING pOption);
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

