#pragma once
#include <SetupAPI.h>
#include "StateImp.h"
#include "CHostWrapper.h"
#include "BaseDefs.h"
#include "boost/asio/thread_pool.hpp"
using namespace boost::asio;

#define WM_PREF_BASE        (WM_USER + 0x1000)

            //Get EC table
#define E_DOP_GET_EC 0xB3 //Vcmd OP code

typedef enum PrefEnum
{
    WM_BASE = WM_PREF_BASE,

    WM_QC_START,
    WM_CARD_OPEN,
    WM_UPDATE_ISP,
    WM_CHECK_FLASHID,
    WM_ERASE_ALL,
    WM_EXEC_END,
} E_PREF;

enum StateErrorCode
{
    ERR_CardInitState = 0x0F01,
    ERR_UpdateCID,
    ERR_DCP_TFImage,
    ERR_EnterDebugMode,
    ERR_WriteSBParameter,
    ERR_WriteIBParameter,
    ERR_Init_DCP,
    ERR_WriteEraseAllParam,
    ERR_PreformatGo,
    ERR_WriteSys,
    ERR_WriteISP,
    ERR_UpdateISP,
    ERR_VerifyISP,
    ERR_CardVerify,
    ERR_CardBlockCnt,
    ERR_CardVersionVerify,
    ERR_WriteIBlk,
    ERR_PowerCycleTest,
    ERR_SetPSR,
    ERR_ConfirmPSR,
    ERR_ReadUniqueID,
    ERR_CheckFWAndAppVersion,
    ERR_CheckCap,
    ERR_CheckCID,
    ERR_ReadSmart,
    ERR_ReadFlashID,
    ERR_SramTest,
    ERR_Restart,
    ERR_CompareISP=0xEE0D,
    ERR_CheckFlashID=0xEEED
};

enum PerformanceErrorCode
{
    ERR_FW_VER = 0x0B01,
    ERR_AP_VER,
    ERR_CAP,
    ERR_MID,
    ERR_OID,
    ERR_PNM,
    ERR_PRV,
    ERR_FAC

};


class CPrefThread : public CWinThread
{
    DECLARE_DYNCREATE(CPrefThread)

public:
    CPrefThread();
    CPrefThread(int, int);           // protected constructor used by dynamic creation
    virtual ~CPrefThread();

public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();

    void OnQCStart(WPARAM wParam, LPARAM lParam);
    void OnPrefStart(WPARAM wParam, LPARAM lParam);
    void OnUpdateIsp(WPARAM wParam, LPARAM lParam);
    void OnCheckFlashID(WPARAM wParam, LPARAM lParam);

protected:
    DECLARE_MESSAGE_MAP()
public:
    virtual int Run();
private:
    int m_nItemIndex = 0xFF;
    int m_nDeviceIndex = 0xFF;
    int m_nStatus = ERROR_SUCCESS;
};

class Spark3635Context : public CContext
{
public:
    Spark3635Context() = default;

public:
    CHostWrapper* m_pSpark3635Util = nullptr;
};


class PrefStartContext : public Spark3635Context
{
public:
    PrefStartContext(int nItemIdx, int nDeviceIdx);
    ~PrefStartContext();

    BOOL Exec(WPARAM wParam = NULL, LPARAM lParam = NULL) override;
    void GetLogInfo(WPARAM wParam, LPARAM lParam, int resultCode);
    void GetPdtLog(WPARAM wParam, LPARAM lParam, int resultCode);
    void GetQCLog(WPARAM wParam, LPARAM lParam, int resultCode);
    void WriteLogInfo(WPARAM wParam, LPARAM lParam, int resultCode);

    //void SaveProductLog(WPARAM wParam, LPARAM lParam);
    //void WriteLogInfo(WPARAM wParam, LPARAM lParam);
    //int ConnWriteLogFile(WPARAM wParam, LPARAM lParam);

    //void WriteQCLogInfo(WPARAM wParam, LPARAM lParam);
    //int ConnWriteQCLogInfo(WPARAM wParam, LPARAM lParam);

    int GetDeviceLastError(WPARAM wParam, LPARAM lParam, UINT& nErrorCode);
    int SetPdtStateError(WPARAM wParam, LPARAM lParam, UINT nErrorCode);
public:
    //LOG_INFO m_LogInfo;
    //PICKTESTLOG_INFO m_pickTestLogInfo;
public:
    int m_nItemIndex;
    int m_nDeviceIndex;
    CSemaphore m_mutexLog;
    CCriticalSection m_CsLog;
    CCriticalSection m_CsSummaryLog;


};


class PrefStateBase : public CState
{
public:
    PrefStateBase()
    {
        ZeroMemory(m_pBuf, sizeof(m_pBuf));
        ZeroMemory(m_pECBuf, sizeof(m_pECBuf));
    }


protected:
    CHAR m_pBuf[512];
    BYTE m_pECBuf[0x1000];
};


class DeviceInitState : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class CardInitState : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class UpdateCID : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class DCP_TFImage : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class EnterDebugMode : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class WriteSBParameter : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class WriteIBParameter_del : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};


class WriteIBParameter : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class Init_DCP : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class pure_Init_DCP : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class SRAM_TEST : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class WriteEraseAllParam : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class PreformatGo : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class WriteSys : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};



class WriteISP : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class UpdateISP : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class VerifyISP : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class CardVerify : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class GetCardCapacity : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class WriteIBlk : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class PowerCycleTest : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class FinishState : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

//2024/01/18 addition by Bruce

class SetPSR : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class ConfirmPSR : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

//pick test state
class ReadUniqueID : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class CheckFWAndAppVersion : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class VerifyFwAndApVersion : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class CheckFWAndAppVersionForDuwanMWE : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class CheckCap : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class DUWAN_CheckCID : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};


class CheckCID : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class ReadSmart : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class PickTestFinish : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};


class ReadFlashID : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class CheckFlashID : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};

class RestartPowerSupply : public PrefStateBase
{
    int Handle(CContext* pContext, WPARAM wParam = NULL, LPARAM lParam = NULL) override;
};