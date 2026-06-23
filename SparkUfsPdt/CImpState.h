#pragma once
#include <shared_mutex>
#include "../SparkLog/SparkLog.h"

#define UPIU_FORCE_ROM_MODE FALSE
#define VCC_FORCE_ROM_MODE TRUE

class ISettingsProvider;
class ILogger;
class IUiNotifier;

class CImpState
{
public:
    explicit CImpState(ISettingsProvider* settings, ILogger* logger, IUiNotifier* notifier);
    ~CImpState() = default;

    static bool ConvertWCharDataToCharData(const WCHAR* wSrc, size_t wSrcLen,
        char* cDest, size_t cDestLen,
        UINT codePage = CP_ACP);

    int PowerOffStage(int portIndex, pdt_log_config_t& lg);
    int RebootStage(int portIndex, pdt_log_config_t& lg);
    int CardInitStage(int portIndex, pdt_log_config_t& lg);
    int ForceRomStage(int portIndex, pdt_log_config_t& lg);
    int UpiuForceRomStage(int portIndex, pdt_log_config_t& lg);
    int VccOffForceRomStage(int portIndex, pdt_log_config_t& lg);
    int MpStartStage(int portIndex, pdt_log_config_t& lg);
    int Write1024KIspMpStage(int portIndex, pdt_log_config_t& lg);
    int MpExitStage(int portIndex, pdt_log_config_t& lg);

    int SetSnStage(int portIndex, pdt_log_config_t& lg);
    int SetMdtStage(int portIndex, pdt_log_config_t& lg);
    int VerifyIspStage(int portIndex, pdt_log_config_t& lg);
    int VerifyQcIspStage(int portIndex, pdt_log_config_t& lg);
    int WriteSramStage(int portIndex, pdt_log_config_t& lg);
    int VerifySram1Stage(int portIndex, pdt_log_config_t& lg);
    int VerifySram2Stage(int portIndex, pdt_log_config_t& lg);

    int VerifyCidStage(int portIndex, pdt_log_config_t& lg);
    int VerifyGeometryStage(int portIndex, pdt_log_config_t& lg);
    int VerifySnStage(int portIndex, pdt_log_config_t& lg);
    int VerifyPrvStage(int portIndex, pdt_log_config_t& lg);
    int VerifyUIDStage(int portIndex, pdt_log_config_t& lg);
    //Set Data functions
    void SetSnData(int portIndex, char* pData);
    void SetMdtData(char* pData);
    void GetQCIspString(char* isp);
    void GetIspMark(char* isp);
    //Is Valid UID
    BOOL IsValidUid(char* pUID, int nUidSize = 512, char* pValidUidBuff = nullptr);
    // Allow caller to pre-cache allocated SN for a port (UTF-16)
    void SetCachedSnForPort(int portIndex, const CStringW& sn);

    // Called from UI thread after g_UfsIsp is loaded to cache the ISP mark.
    static void UpdateIspMark(const char* ispBuf, int ispFileSize);

    // Accessors for injected interfaces
    ISettingsProvider* GetSettings() const { return settings_; }
    ILogger* GetLogger() const { return logger_; }
    IUiNotifier* GetNotifier() const { return notifier_; }

private:
    ISettingsProvider* settings_ = nullptr;
    ILogger* logger_ = nullptr;
    IUiNotifier* notifier_ = nullptr;

    static constexpr int UI_THREAD_COUNT = 16;
    CStringW m_strwSn[UI_THREAD_COUNT];
};

