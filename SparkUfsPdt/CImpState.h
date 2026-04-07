#pragma once
#include "../SparkLog/SparkLog.h"

class CSparkUfsPdtDlg;

class CImpState
{
public:
    static bool ConvertWCharDataToCharData(const WCHAR* wSrc, size_t wSrcLen,
        char* cDest, size_t cDestLen,
        UINT codePage = CP_ACP);

    static int PowerOffStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg);
    static int RebootStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg);
    static int CardInitStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg);
    static int UpiuForceRomStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg);
    static int VccOffForceRomStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg);
    static int MpStartStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg);
    static int Write1024KIspMpStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg);
    static int MpExitStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg);

    static int SetSnStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg);
    static int SetMdtStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg);
    static int VerifyIspStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg);
    static int WriteSramStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg);
    static int VerifySram1Stage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg);
    static int VerifySram2Stage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg);

    static void SetSnData(CSparkUfsPdtDlg* pDlg, int portIndex, char* pData);
    static void SetMdtData(CSparkUfsPdtDlg* pDlg, char* pData);
    static void GetIspString(CSparkUfsPdtDlg* pDlg, char* isp);
};

