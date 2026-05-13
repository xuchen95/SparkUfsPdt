#pragma once
#include <shared_mutex>
#include "../SparkLog/SparkLog.h"

class CSparkUfsPdtDlg;
#define UPIU_FORCE_ROM_MODE FALSE
#define VCC_FORCE_ROM_MODE TRUE

class CImpState
{
public:
    static bool ConvertWCharDataToCharData(const WCHAR* wSrc, size_t wSrcLen,
        char* cDest, size_t cDestLen,
        UINT codePage = CP_ACP);

    static int PowerOffStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg);
    static int RebootStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg);
    static int CardInitStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg);
    static int ForceRomStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg);
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

    static int VerifyCidStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg);
    static int VerifyGeometryStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg);
    static int VerifySnStage(CSparkUfsPdtDlg* pDlg, int portIndex, pdt_log_config_t& lg);

public:
    //Set Data functions for different stages, these functions are called by the stage functions to update the dialog with relevant information such as CID, SN, MDT, ISP info etc.
    static void SetSnData(CSparkUfsPdtDlg* pDlg, int portIndex, char* pData);
    static void SetMdtData(CSparkUfsPdtDlg* pDlg, char* pData);
    static void GetQCIspString(CSparkUfsPdtDlg* pDlg, char* isp);
    static void GetIspMark(CSparkUfsPdtDlg* pDlg, char* isp);

    // Called from UI thread after g_UfsIsp is loaded to cache the ISP mark.
    // Thread-safe: acquires exclusive write lock.
    static void UpdateIspMark(const char* ispBuf, int ispFileSize);

private:
    static constexpr int ISP_MARK_SIZE = 16;
    static char  s_ispMark[ISP_MARK_SIZE];
    static bool  s_ispMarkValid;
    static std::shared_mutex s_ispMarkMutex;
};

