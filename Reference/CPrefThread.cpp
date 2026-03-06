#include "pch.h"
#include "resource.h"
#include "CPrefThread.h"
#include "UsbIo.h"
#include "DbWrapper.h"
#include "CppSQLite3.h"
#include "DbConnWrapper.h"
#include "StateImp.h"
#include "CDialogProduct.h"
#include "Permission.h"
#include ".\include\svnVersion.h"
#include "CAsyncLogger.h"
#include "global.h"
//#define TestCardInfo
#define TRACE_FUNC()        TRACE("%s\n", __FUNCTION__)
#define GET_PDT_DLG(x) (((CDialogProduct*)(x)))
#define GET_PARMS(x)        (((CDialogBase*)(x))->m_pstPdtParms)
#define GET_PICK_PARMS(x)        (((CDialogBase*)(x))->m_pstPickParms)
#define DEVICE_IS_LOCKED(x) ((x) & (0x02000000))

IMPLEMENT_DYNCREATE(CPrefThread, CWinThread)

CPrefThread::CPrefThread()
{
}

CPrefThread::CPrefThread(int nItemIndex, int nDeviceIndex)//(CAt32Host::m_DevNo[idx], idx)
{
    m_nItemIndex = nItemIndex;
    m_nDeviceIndex = nDeviceIndex;
}

CPrefThread::~CPrefThread()
{
}

BOOL CPrefThread::InitInstance()
{
    TRACE_FUNC();
    return TRUE;
}

int CPrefThread::ExitInstance()
{
    TRACE_FUNC();
    return CWinThread::ExitInstance();
}


void CPrefThread::OnQCStart(WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    if (m_nDeviceIndex <= MAXDEVICE)
    {
        PST_PT_PARAM_INFO pstpIckParms = GET_PICK_PARMS(lParam);
        ST_PT_PARAM_INFO& stTestOpt = *pstpIckParms;
        CContext* pContext = new PrefStartContext(m_nItemIndex, m_nDeviceIndex);
        pContext->AddState(new DeviceInitState());
        pContext->AddState(new RestartPowerSupply());
        pContext->AddState(new CardInitState());
        pContext->AddState(new DCP_TFImage());
        pContext->AddState(new EnterDebugMode());
        pContext->AddState(new WriteSBParameter());
        pContext->AddState(new WriteIBParameter());
        pContext->AddState(new ReadUniqueID());
        
        if (stTestOpt.bSRAMTest)
        {
            pContext->AddState(new SRAM_TEST());
        }
        
        pContext->AddState(new PowerCycleTest());
        //CheckFWAndAppVersion
        //
        if (stTestOpt.bReadSmart)
        {
            pContext->AddState(new ReadSmart());
        }
        //if duwan MWE special vcmd read

        
        pContext->AddState(new CheckCap());
#ifdef _DUWAN_PNM_
        pContext->AddState(new CheckFWAndAppVersionForDuwanMWE());
        pContext->AddState(new DUWAN_CheckCID());
#else
        pContext->AddState(new VerifyFwAndApVersion());
        pContext->AddState(new CheckCID());
#endif
        
        pContext->AddState(new PickTestFinish());
        PostMessageA(((CWnd*)lParam)->m_hWnd, WM_EXEC_END, m_nDeviceIndex,
            pContext->Exec(m_nDeviceIndex, lParam));
        delete pContext;
    }
}

void CPrefThread::OnPrefStart(WPARAM wParam, LPARAM lParam)//(CAt32Host::m_DevIdx[i],pDlg)
{
    TRACE_FUNC();

    if (m_nDeviceIndex <= MAXDEVICE)
    {
        PST_PREF_PARMS pstParms = GET_PARMS(lParam);
        ST_TEST_OPT& stTestOpt = pstParms->stTestOpt;
        CContext* pContext = new PrefStartContext(m_nItemIndex, m_nDeviceIndex);
        pContext->AddState(new DeviceInitState());
        pContext->AddState(new RestartPowerSupply());
        if (stTestOpt.bSRAMtest)
        {
            pContext->AddState(new CardInitState());
            pContext->AddState(new DCP_TFImage());
            pContext->AddState(new EnterDebugMode());
            pContext->AddState(new WriteSBParameter());
            pContext->AddState(new WriteIBParameter());
            pContext->AddState(new Init_DCP());
            pContext->AddState(new SRAM_TEST());
            pContext->AddState(new PowerCycleTest());
        }
        pContext->AddState(new CardInitState());
        //if (stTestOpt.bEcInherit)
        //{
        //    //
        //    pContext->AddState(new BackupEcTableState());
        //    pContext->AddState(new CardInit60State());
        //}
        pContext->AddState(new DCP_TFImage());
        pContext->AddState(new EnterDebugMode());
        pContext->AddState(new WriteSBParameter());
        pContext->AddState(new WriteIBParameter());
        pContext->AddState(new Init_DCP());
        if (stTestOpt.bEraseAllFirst)
        {
            pContext->AddState(new WriteEraseAllParam());
        }

        pContext->AddState(new PreformatGo());
        pContext->AddState(new CardVerify());
        pContext->AddState(new WriteSys());
        pContext->AddState(new WriteIBlk());
        pContext->AddState(new WriteISP());
        pContext->AddState(new VerifyISP());
        
        pContext->AddState(new UpdateCID());
        if (!stTestOpt.bHtolTest)
        {
            pContext->AddState(new PowerCycleTest());
            
        }
        //need set PSR
        if (stTestOpt.bSetPSR)
        {
            pContext->AddState(new SetPSR());
            pContext->AddState(new PowerCycleTest());
            //pContext->AddState(new ConfirmPSR());
        }
        else
        {
            if (stTestOpt.bSRAMtest)
            {
                pContext->AddState(new CardInitState());
                pContext->AddState(new DCP_TFImage());
                pContext->AddState(new EnterDebugMode());
                pContext->AddState(new WriteSBParameter());
                pContext->AddState(new WriteIBParameter());
                pContext->AddState(new Init_DCP());
                pContext->AddState(new SRAM_TEST());
            }
            pContext->AddState(new RestartPowerSupply());
            pContext->AddState(new PowerCycleTest());
        }
        pContext->AddState(new GetCardCapacity());

        PST_PT_PARAM_INFO pstPickParms = GET_PICK_PARMS(lParam);
        ST_PT_PARAM_INFO& stPickOpt = *pstPickParms;
        
        pContext->AddState(new CheckCap());
#ifdef _DUWAN_PNM_
        pContext->AddState(new CheckFWAndAppVersionForDuwanMWE());
        pContext->AddState(new DUWAN_CheckCID());
#else
        pContext->AddState(new VerifyFwAndApVersion());
        pContext->AddState(new CheckCID());
#endif
        pContext->AddState(new FinishState());
        PostMessageA(((CWnd*)lParam)->m_hWnd, WM_EXEC_END, m_nDeviceIndex,
            pContext->Exec(m_nDeviceIndex, lParam));
        delete pContext;
    }
}

void CPrefThread::OnUpdateIsp(WPARAM wParam, LPARAM lParam)//(CAt32Host::m_DevIdx[i],pDlg)
{
    TRACE_FUNC();

    if (m_nDeviceIndex <= MAXDEVICE)
    {
        PST_PREF_PARMS pstParms = GET_PARMS(lParam);
        ST_TEST_OPT& stTestOpt = pstParms->stTestOpt;
        CContext* pContext = new PrefStartContext(m_nItemIndex, m_nDeviceIndex);
        pContext->AddState(new DeviceInitState());
        pContext->AddState(new RestartPowerSupply());
        pContext->AddState(new CardInitState());
        pContext->AddState(new DCP_TFImage());
        pContext->AddState(new EnterDebugMode());
        pContext->AddState(new WriteSBParameter());
        pContext->AddState(new WriteIBParameter());
        pContext->AddState(new pure_Init_DCP());
        pContext->AddState(new UpdateISP());
        pContext->AddState(new VerifyISP());
        pContext->AddState(new FinishState());
        PostMessageA(((CWnd*)lParam)->m_hWnd, WM_EXEC_END, m_nDeviceIndex,
            pContext->Exec(m_nDeviceIndex, lParam));
        delete pContext;
    }
}

void CPrefThread::OnCheckFlashID(WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();

    if (m_nDeviceIndex <= MAXDEVICE)
    {
        PST_PREF_PARMS pstParms = GET_PARMS(lParam);
        ST_TEST_OPT& stTestOpt = pstParms->stTestOpt;
        CContext* pContext = new PrefStartContext(m_nItemIndex, m_nDeviceIndex);
        pContext->AddState(new DeviceInitState());
        pContext->AddState(new RestartPowerSupply());
        pContext->AddState(new CardInitState());
        pContext->AddState(new DCP_TFImage());
        pContext->AddState(new EnterDebugMode());
        pContext->AddState(new WriteSBParameter());
        pContext->AddState(new WriteIBParameter());
        pContext->AddState(new CheckFlashID());
        pContext->AddState(new FinishState());
        PostMessageA(((CWnd*)lParam)->m_hWnd, WM_EXEC_END, m_nDeviceIndex,
            pContext->Exec(m_nDeviceIndex, lParam));
        delete pContext;
    }
}

BEGIN_MESSAGE_MAP(CPrefThread, CWinThread)
    ON_THREAD_MESSAGE(WM_QC_START, OnQCStart)
    ON_THREAD_MESSAGE(WM_CARD_OPEN, OnPrefStart)
    ON_THREAD_MESSAGE(WM_UPDATE_ISP, OnUpdateIsp)
    ON_THREAD_MESSAGE(WM_CHECK_FLASHID, OnCheckFlashID)

END_MESSAGE_MAP()

int CPrefThread::Run()
{
    TRACE_FUNC();

    return CWinThread::Run();
}

PrefStartContext::PrefStartContext(int nItemIndex, int nDeviceIndex)
{
    //m_pSpark3635Util = &CAt32Host::GetInstance(nItemIndex);

    m_nItemIndex = nItemIndex; //Reserved for ListCtrl
    m_nDeviceIndex = nDeviceIndex;
    //ZeroMemory(&m_LogInfo, sizeof(LOG_INFO));
}

PrefStartContext::~PrefStartContext()
{
    for (size_t i = 0; i < m_StateAry.size(); i++)
    {
        if (m_StateAry[i] != nullptr)
        {
            delete m_StateAry[i];
        }
    }

    m_StateAry.clear();
}

BOOL PrefStartContext::Exec(WPARAM wParam, LPARAM lParam)
{
    CDialogProduct* dlg = (CDialogProduct*)lParam;
    m_pSpark3635Util = dlg->m_pHostWrapper;
    int nPct = 0;
    int nPos;
    int nStateArySize = m_StateAry.size();
    dlg->SetProgressIng(m_nDeviceIndex);
    dlg->StartTimer(wParam);
    int ret;
    for (size_t i = 0; i < nStateArySize; i++)
    {
        if (m_StateAry[i] != nullptr)
        {
            CString szStr;

            if ((ret = m_StateAry[i]->Handle(this, wParam, (LPARAM)dlg)) == FALSE)
            {
                dlg->SetProgressFail(m_nDeviceIndex);
                dlg->EndTimer(wParam);
                GetLogInfo(wParam, lParam, ERROR_INVALID_FUNCTION);
                WriteLogInfo(wParam, lParam, ERROR_INVALID_FUNCTION);

                return FALSE;
            }
            else
            {
                nPos = ((i + 1) * 100) / m_StateAry.size();
                dlg->SetProgressPos(m_nDeviceIndex, nPos);
                if (100 == nPos)
                {
                    dlg->EndTimer(wParam);
                    //开卡成功Log
                    GetLogInfo(wParam, lParam, ERROR_SUCCESS);
                    WriteLogInfo(wParam, lParam, ERROR_SUCCESS);
                }
                if ((i + 1) != m_StateAry.size())
                {
                    Sleep(100);
                }
            }
        }
    }
    return TRUE;
}

void PrefStartContext::GetLogInfo(WPARAM wParam, LPARAM lParam, int resultCode)
{
    CDialogProduct* dlg = (CDialogProduct*)lParam;
    if (dlg->m_pstPdtParms->stTestOpt.cbPrefFunc == FunPDT)
    {
        GetPdtLog(wParam, lParam, resultCode);
    }
    if (dlg->m_pstPdtParms->stTestOpt.cbPrefFunc == FunQC)
    {
        GetQCLog(wParam, lParam, resultCode);
    }
}
    


void PrefStartContext::GetPdtLog(WPARAM wParam, LPARAM lParam, int resultCode)
{
    CDialogProduct* dlg = (CDialogProduct*)lParam;
    LOG_INFO* lg = dlg->GetLogStruct(wParam);
    memcpy(lg->uniqueID, dlg->GetTesterUID(wParam), sizeof(lg->uniqueID)); //Get unique_id
    if (resultCode == ERROR_SUCCESS) //Get error_status
    {
        strcpy((char*)lg->strErrorState,"Finish");
    }
    else
    {
        CString strState = dlg->GetTesterCardState(wParam);
        strcpy((char*)lg->strErrorState, strState.GetBuffer());
    }

    time_t rawtime;
    struct tm timeinfo;
    unsigned int nYear;
    unsigned char u08Mon;
    time(&rawtime);
    localtime_s(&timeinfo, &rawtime);
    nYear = (timeinfo.tm_year + 1900);
    u08Mon = timeinfo.tm_mon + 1;
    //3,4
    CString str_date, str_time, str_datetime;
    str_date.Format("%d/%.2d/%.2d", nYear, u08Mon, timeinfo.tm_mday);
    str_time.Format("%.2d:%.2d:%.2d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    str_datetime = str_date + " " + str_time;
    memcpy(lg->production_date, str_date.GetBuffer(), str_date.GetLength());
    memcpy(lg->production_time, str_time.GetBuffer(), str_time.GetLength());
    memcpy(lg->production_datetime, str_datetime.GetBuffer(), str_datetime.GetLength());

    lg->fw_key = dlg->m_nFwKey; //Get fw_key
    CString strFwInfo;
    strFwInfo.Format("%d", dlg->m_nFwVer);
    memcpy(lg->fw_ver, strFwInfo.GetBuffer(), strFwInfo.GetLength());
    memcpy(lg->OrderNo, dlg->m_szOrderNo, sizeof(lg->OrderNo));
    CString strLunId = dlg->GetTesterLunId(wParam);
    strcpy((char*)lg->lun_id, strLunId.GetBuffer());

    lg->blockCnt = dlg->GetCardBlockCnt(wParam);

    lg->error_code = dlg->GetDeviceErrorCode(wParam);
    lg->app_ver = dlg->GetAppVer();
    lg->apSpec = dlg->m_byApSpec;
    DWORD dwUsedTime = dlg->GetUsedTime(wParam);
    lg->nUsedTime = dwUsedTime / 1000;
    lg->nMachineMode = dlg->m_nMachineRunMode;
    lg->nMachineNo = dlg->m_nMachineNo;

    CID_Type cid;
    memcpy(&cid, GET_PDT_DLG(lParam)->GetTesterCID(wParam), sizeof(CID_Type));
    CString strSn("");
    for (int i = 0; i < sizeof(cid.PSN); ++i)
    {
        strSn.AppendFormat("%2.2x", cid.PSN[i]);
    }
    strcpy(lg->sn, strSn.GetBuffer());
    lg->mid[0] = cid.MID;
    lg->oid[0] = cid.OID;
#ifdef _SPARK_PDT_
    memcpy(&(dlg->vecLog[wParam]), lg, sizeof(LOG_INFO));
#endif
}

void PrefStartContext::GetQCLog(WPARAM wParam, LPARAM lParam, int resultCode)
{
    CDialogProduct* dlg = (CDialogProduct*)lParam;
    LOG_INFO* lg = dlg->GetLogStruct(wParam);

    time_t rawtime;
    struct tm timeinfo;
    unsigned int nYear;
    unsigned char u08Mon;
    time(&rawtime);
    localtime_s(&timeinfo, &rawtime);
    nYear = (timeinfo.tm_year + 1900);
    u08Mon = timeinfo.tm_mon + 1;
    //3,4
    CString str_date, str_time, str_datetime;
    str_date.Format("%d/%.2d/%.2d", nYear, u08Mon, timeinfo.tm_mday);
    str_time.Format("%.2d:%.2d:%.2d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    str_datetime = str_date + " " + str_time;
    memcpy(lg->production_date, str_date.GetBuffer(), str_date.GetLength());
    memcpy(lg->production_time, str_time.GetBuffer(), str_time.GetLength());
    memcpy(lg->production_datetime, str_datetime.GetBuffer(), str_datetime.GetLength());

    lg->apSpec = dlg->m_byApSpec;
    memcpy(lg->uniqueID, dlg->GetTesterUID(wParam), sizeof(lg->uniqueID));
    strcpy(lg->lun_id, dlg->GetTesterLunId(wParam).GetBuffer());
    DWORD dwUsedTime = dlg->GetUsedTime(wParam);
    lg->nUsedTime = dwUsedTime / 1000;
    lg->nMachineNo = dlg->m_nMachineNo;
    if (resultCode == ERROR_SUCCESS) //Get error_status
    {
        strcpy((char*)lg->strErrorState, "Finish");
    }
    else
    {
        strcpy((char*)lg->strErrorState, dlg->GetTesterCardState(wParam).GetBuffer());
    }
    lg->error_code = dlg->GetDeviceErrorCode(wParam);
#ifdef _SPARK_PDT_
    memcpy(&(dlg->vecLog[wParam]), lg, sizeof(LOG_INFO));
#endif
}

//void PrefStartContext::SaveProductLog(WPARAM wParam, LPARAM lParam)
//{
////    CDialogProduct* dlg = (CDialogProduct*)lParam;
////    //CSingleLock slock(&m_mutexLog);
////
////    CSingleLock slock(&m_CsLog);
////    slock.Lock();
////    if (slock.IsLocked())
////    {
////        LOG_INFO lg = dlg->GetFactoryLog(wParam);
////#ifdef _FACTORY_AUTO
////        dlg->m_BatchLog.push_back(lg);
////        /*std::vector<LOG_INFO> tLog;
////        tLog.push_back(lg);
////        int LogType = dlg->m_pstPdtParms->stTestOpt.cbPrefFunc;
////        CPermission::WriteTextLogContent(LogType, tLog);
////        dlg->SetSummaryErrorCode(lg);*/
////#else
////        dlg->m_BatchLog.push_back(lg);
////#endif
////    }
//    
//}

//void PrefStartContext::WriteLogInfo(WPARAM wParam, LPARAM lParam)
//{
//    CppSQLite3DB db;
//    CppSQLite3Binary blob;
//    unsigned char* pBuf;
//    CppSQLite3Buffer bufSQL;
//    CString szSql;
//    int uid_key=-1;
//    try
//    {
//        CppSQLite3Query q;
//        db.open(DB_LOG);
//        do
//        {
//            if (db.tableExists(TABLE_UNIQUEID))
//            {
//                blob.setBinary(m_LogInfo.uniqueID, sizeof(m_LogInfo.uniqueID));
//                pBuf = (unsigned char*)GlobalAlloc(GPTR, 16);
//                memcpy(pBuf, m_LogInfo.uniqueID, 16);
//                blob.setBinary(pBuf, 16);
//                GlobalFree(pBuf);
//                while (uid_key == -1)
//                {
//                    bufSQL.format("SELECT * from %s where unique_id = %Q ;", TABLE_UNIQUEID, blob.getEncoded());
//                    q = db.execQuery(bufSQL);
//                    //没有找到
//                    if (q.eof())
//                    {
//                        //新增Flash Unique ID
//                        bufSQL.format("INSERT into %s(unique_id) values(%Q );", TABLE_UNIQUEID, blob.getEncoded());
//                        db.execQuery(bufSQL);
//                    }
//                    else
//                    {
//                        uid_key = atoi(q.fieldValue("key"));
//                        break;
//                    }
//                }
//                q.finalize();
//            }
//            if (db.tableExists(TABLE_LOG))
//            {
//                CString strErrorCode;
//                strErrorCode.Format("%x", m_LogInfo.error_code);
//                bufSQL.format("INSERT into %s (machine_no, machine_mode, uid_key, psn, fw_key, order_no, lun_id, fw_info,\
//                                                     app_ver, use_time, error_state, error_code, block_count)\
//                                            values(%d, %d, %d, '%s',%d, '%s', '%s', '%s', %d, %d, '%s', '%s', %d);",
//                    TABLE_LOG, 
//                    m_LogInfo.nMachineNo, m_LogInfo.nMachineMode, uid_key, m_LogInfo.sn, m_LogInfo.fw_key, m_LogInfo.OrderNo,m_LogInfo.lun_id, m_LogInfo.fw_ver,
//                    m_LogInfo.app_ver, m_LogInfo.nUsedTime, m_LogInfo.strErrorState, strErrorCode, m_LogInfo.blockCnt);
//                db.execQuery(bufSQL);
//            }
//        } while (0);
//
//        db.close();
//    }
//    catch (CppSQLite3Exception& e)
//    {
//        CString szStr;
//
//        szStr.Format("%x: %s\n", e.errorCode(), e.errorMessage());
//        TRACE(szStr);
//    }
//}

//int PrefStartContext::ConnWriteLogFile(WPARAM wParam, LPARAM lParam)
//{
//#ifdef _SPARK_PDT_
//    DbConnWrapper db;
//    if (db.m_DbErrorCode) return db.m_DbErrorCode;
//    CString strMachineMode, strFuncType;
//    switch (m_LogInfo.nMachineMode)
//    {
//    case MM_MODE1:
//        strMachineMode.Format("Mode1");
//        break;
//    case MM_MODE2:
//        strMachineMode.Format("Mode2");
//        break;
//    case MM_MODE3:
//        strMachineMode.Format("Mode3");
//        break;
//    case MM_MODE4:
//        strMachineMode.Format("Mode4");
//        break;
//    default:
//        break;
//    }
//
//    CString strErrorCode;
//    strErrorCode.Format("%x", m_LogInfo.error_code);
//    sql::SQLString date, time, datetime;
//    char szQuery[1024] = { 0 };
//    sql::SQLString strQuery;
//
//    int uid_key;
//    db.m_DbErrorCode = db.FindUniqueIdKey(m_LogInfo.uniqueID, 16, uid_key);
//    if (uid_key == -1)
//    {
//        //MessageBox(nullptr, "write connector m_Log fail", "Error", MB_ICONERROR);
//        return db.m_DbErrorCode;
//    }
//    CString strUniqueID = "";
//    int nSize = sizeof(m_LogInfo.uniqueID);
//    for (int i = 0; i < nSize; ++i)
//    {
//        strUniqueID.AppendFormat("%.2x ", m_LogInfo.uniqueID[i]);
//    }
//    PLOGINFO pLogInfo = &m_LogInfo;
//    db.getTimeStamp(date, time, datetime);
//    sprintf_s(szQuery, sizeof(szQuery),"INSERT into %s (machine_no,machine_mode,uid_key, uid,psn,fw_key,order_no,lun_id,fw_info,app_ver,\
//                production_date,production_time,production_datetime,use_time,error_state,error_code,block_cnt) \
//                                values(%d,'%s',%d, '%s','%s',%d,'%s','%s','%s',%d,\
//                                        '%s','%s','%s',%d, '%s','%s',%d);", CONN_TABLE_PDT_LOG,
//        pLogInfo->nMachineNo, strMachineMode, uid_key, strUniqueID, pLogInfo->sn, pLogInfo->fw_key, pLogInfo->OrderNo,pLogInfo->lun_id,  pLogInfo->fw_ver, pLogInfo->app_ver,
//        date.c_str(), time.c_str(), datetime.c_str(), pLogInfo->nUsedTime,pLogInfo->strErrorState, strErrorCode,pLogInfo->blockCnt);
//
//    strQuery.append(szQuery);
//
//    try
//    {
//        sql::PreparedStatement* pstmt;// 预处理语句
//        do
//        {
//            if ((pstmt = db.con->prepareStatement(strQuery)) == nullptr) break;
//            //pstmt->setString(1, agingStr);
//            if ((db.res = pstmt->executeQuery()) == nullptr) break;
//        } while (0);
//    }
//    catch (sql::SQLException& e)
//    {
//        db.m_DbErrorInfo = e.what();
//        db.m_DbErrorCode = e.getErrorCode();
//        db.m_DbErrorStatus = e.getSQLState();
//        char tip[1024] = { 0 };
//        sprintf_s(tip, sizeof(tip),"ErrorInfo:%s,ErrorCode:%d,ErrorStatus:%s", db.m_DbErrorInfo, db.m_DbErrorCode, db.m_DbErrorStatus);
//        //MessageBox(nullptr, tip, "Error", MB_ICONERROR);
//    }
//    return db.m_DbErrorCode;
//#endif
//    return ERROR_SUCCESS;
//}

//void PrefStartContext::WriteQCLogInfo(WPARAM wParam, LPARAM lParam)
//{
//    CppSQLite3DB db;
//    CppSQLite3Binary blob;
//    unsigned char* pBuf;
//    CppSQLite3Buffer bufSQL;
//    CString szSql;
//    int uid_key = -1;
//    CDialogProduct* dlg = (CDialogProduct*)lParam;
//    //m_pickTestLogInfo.strErrorState = dlg->GetTesterCardState(wParam);
//    try
//    {
//        CppSQLite3Query q;
//        db.open(DB_DEFAULT);
//        do
//        {
//            if (db.tableExists(TABLE_UNIQUEID))
//            {
//                blob.setBinary(m_LogInfo.uniqueID, sizeof(m_LogInfo.uniqueID));
//                pBuf = (unsigned char*)GlobalAlloc(GPTR, 16);
//                memcpy(pBuf, m_LogInfo.uniqueID, 16);
//                blob.setBinary(pBuf, 16);
//                GlobalFree(pBuf);
//                while (uid_key == -1)
//                {
//                    bufSQL.format("SELECT * from %s where unique_id = %Q ;", TABLE_UNIQUEID, blob.getEncoded());
//                    q = db.execQuery(bufSQL);
//                    //没有找到
//                    if (q.eof())
//                    {
//                        //新增Flash Unique ID
//                        bufSQL.format("INSERT into %s(unique_id) values(%Q );", TABLE_UNIQUEID, blob.getEncoded());
//                        db.execQuery(bufSQL);
//                    }
//                    else
//                    {
//                        uid_key = atoi(q.fieldValue("key"));
//                        break;
//                    }
//                }
//                q.finalize();
//            }
//            CString strUniqueID="";
//            int nSize = sizeof(m_LogInfo.uniqueID);
//            for (int i = 0; i < nSize; ++i)
//            {
//                strUniqueID.AppendFormat("%.2x ", m_LogInfo.uniqueID[i]);
//            }
//            if (db.tableExists(TABLE_PICK_LOG))
//            {
//                LOG_INFO* pQcLog = &m_LogInfo;
//                bufSQL.format("INSERT into %s (machine_no, uid_key, uid, lun_id, fw_ver, ap_ver, \
//                                    capacity, psn, mid, oid, pnm, mdt, use_time, error_state) \
//                                            values(%d, %d, '%s', '%s', %d, %d,\
//                                                    %d,'%s','%s','%s','%s','%s',%d,'%s' );",
//                    TABLE_PICK_LOG,
//                    pQcLog->nMachineNo, uid_key, strUniqueID, pQcLog->lun_id,pQcLog->fw_ver, pQcLog->app_ver, 
//                    pQcLog->capacity, pQcLog->psn, pQcLog->mid, pQcLog->oid,pQcLog->pnm, pQcLog->mdt, pQcLog->nUsedTime,pQcLog->strErrorState);
//                db.execQuery(bufSQL);
//            }
//        } while (0);
//
//        db.close();
//    }
//    catch (CppSQLite3Exception& e)
//    {
//        CString szStr;
//
//        szStr.Format("%x: %s\n", e.errorCode(), e.errorMessage());
//        TRACE(szStr);
//    }
//}
//
//int PrefStartContext::ConnWriteQCLogInfo(WPARAM wParam, LPARAM lParam)
//{
//#ifdef _SPARK_PDT_
//    DbConnWrapper db;
//    if (db.m_DbErrorCode) return db.m_DbErrorCode;
//    sql::SQLString date, time, datetime;
//    char szQuery[1024] = { 0 };
//    sql::SQLString strQuery;
//
//    int uid_key;
//    db.m_DbErrorCode = db.FindUniqueIdKey(m_LogInfo.uniqueID, 16, uid_key);
//    if (uid_key == -1)
//    {
//        return db.m_DbErrorCode;
//    }
//    CString strUniqueID = "";
//    int nSize = sizeof(m_LogInfo.uniqueID);
//    for (int i = 0; i < nSize; ++i)
//    {
//        strUniqueID.AppendFormat("%.2x ", m_LogInfo.uniqueID[i]);
//    }
//    LOG_INFO* pQcLog = &m_LogInfo;
//    int n = sizeof(LOG_INFO);
//    db.getTimeStamp(date, time, datetime);
//    sprintf_s(szQuery, sizeof(szQuery),"INSERT into %s (machine_no, uid_key, uid, qc_date, qc_time, qc_datetime, lun_id, fw_ver, ap_ver,\
//                                    capacity, psn, mid, oid, pnm, mdt, use_time, error_state)\
//                                            values(%d, %d, '%s', '%s', '%s', '%s', '%s', %d, %d,\
//                                                    %d,'%s','%s','%s','%s','%s', %d, '%s');",
//        CONN_TABLE_QC_LOG,
//        pQcLog->nMachineNo, uid_key, strUniqueID, date.c_str(), time.c_str(), datetime.c_str(), pQcLog->lun_id, pQcLog->fw_ver, pQcLog->app_ver,
//        pQcLog->capacity, pQcLog->psn, pQcLog->mid, pQcLog->oid, pQcLog->pnm, pQcLog->mdt, pQcLog->nUsedTime, pQcLog->strErrorState);
//
//    strQuery.append(szQuery);
//
//    try
//    {
//        sql::PreparedStatement* pstmt;// 预处理语句
//        do
//        {
//            if ((pstmt = db.con->prepareStatement(strQuery)) == nullptr) break;
//            //pstmt->setString(1, agingStr);
//            if ((db.res = pstmt->executeQuery()) == nullptr) break;
//        } while (0);
//    }
//    catch (sql::SQLException& e)
//    {
//        db.m_DbErrorInfo = e.what();
//        db.m_DbErrorCode = e.getErrorCode();
//        db.m_DbErrorStatus = e.getSQLState();
//        char tip[1024] = { 0 };
//        sprintf_s(tip, "ErrorInfo:%s,ErrorCode:%d,ErrorStatus:%s", db.m_DbErrorInfo, db.m_DbErrorCode, db.m_DbErrorStatus);
//        //MessageBox(nullptr, tip, "Error", MB_ICONERROR);
//    }
//    return db.m_DbErrorCode;
//#endif
//    return ERROR_SUCCESS;
//}

void PrefStartContext::WriteLogInfo(WPARAM wParam, LPARAM lParam,int resultCode)
{
    CDialogProduct* dlg = (CDialogProduct*)lParam;
    LOG_INFO* lg = dlg->GetLogStruct(wParam);
    int LogType = dlg->m_pstPdtParms->stTestOpt.cbPrefFunc;
    switch (LogType)
    {
    case LOG_FT3:
        CAsyncLogger::GetInstance().LogFT3(*lg);
        break;
    case LOG_QC:
        CAsyncLogger::GetInstance().LogQC(*lg);
        break;
    default:
        break;
    }
    /*CSingleLock sSumlock(&m_CsSummaryLog);
    sSumlock.Lock();
    if (sSumlock.IsLocked())
    {
        dlg->SetFactorySummaryValue(dlg->m_pstPdtParms,resultCode);
        dlg->SetSummaryErrorCode(*lg);
        CPermission::BuildSummary(dlg->m_pSysConfig->strSummaryPath, dlg->m_stSummary, dlg->m_mapErrorCode);
    }*/
}

int PrefStartContext::GetDeviceLastError(WPARAM wParam, LPARAM lParam, UINT& nErrorCode)
{
    //TODO::
    bool bRet = true;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    int nDevNo = GET_PDT_DLG(lParam)->m_pHostWrapper->m_DevNo[nDevNum];

    BYTE pData[512];
    PU_TF_STATUS puSts = (PU_TF_STATUS)pData;
    /*U_VCMD_ARG arg;
    arg.st.op = 0x15;
    arg.st.sop = SOP_QUERY_STATUS;
    arg.st.uint_cnt = 0;
    arg.st.cw_len = 1;*/
    do
    {
        if ((bRet = m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(MMC_VEN_CMD63, OP_CODE(SOP_QUERY_STATUS))) == false) break;
        if ((bRet = (m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_ReadCMD(pData, 0, 1))) == false) break;
        nErrorCode = puSts->st.sts;
        if (nErrorCode)
        {
            GET_PDT_DLG(lParam)->SetCS_ErrorCode(wParam, nErrorCode);
        }
    } while (0);
    return bRet;
}

int PrefStartContext::SetPdtStateError(WPARAM wParam, LPARAM lParam, UINT nErrorCode)
{
    GET_PDT_DLG(lParam)->SetCS_ErrorCode(wParam, nErrorCode);
    return ERROR_SUCCESS;
}

int CardInitState::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "CardInit");

    bool bRet = true;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    int nDevNo = GET_PDT_DLG(lParam)->m_pHostWrapper->m_DevNo[nDevNum];
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            //if ((bRet = pCont->m_pSpark3635Util->DeviceInitial(nDevNum,nDevNo)) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->CardReset(nDevNum)) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->CardInitial(nDevNum, true)) == false) break;
        } while (0);

        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_CardInitState);
        }
    }
    return bRet;
}

int UpdateCID::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "UpdateCID");

    bool bRet = true;

    CID_Type cid = GET_PDT_DLG(lParam)->GetCidData(wParam);
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    int nDevNo = GET_PDT_DLG(lParam)->m_pHostWrapper->m_DevNo[nDevNum];
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->DeviceInitial(nDevNum, nDevNo)) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->CardReset(nDevNum)) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->WriteCID(nDevNum, (PBYTE)(&cid))) == false) break;
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_UpdateCID);
        }
    }
    return bRet;
}


int DCP_TFImage::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    bool bRet = 0;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    int nDevNo = GET_PDT_DLG(lParam)->m_pHostWrapper->m_DevNo[nDevNum];
    CDialogProduct* pDlg = (CDialogProduct*)lParam;
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "DCP_TFImage");

    PBYTE pBinBuff = GET_PDT_DLG(lParam)->m_pstBinInfo->pBinaryAry[E_BIN_DCP_IMAGE];
    int nStartSector = 0;
    int nBuffSize = GET_PDT_DLG(lParam)->m_pstBinInfo->nBinarySize[E_BIN_DCP_IMAGE];

    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_WriteCMD(pBinBuff, nStartSector, (nBuffSize >> 9));
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_DCP_TFImage);
        }
    }
    return bRet;
}

int EnterDebugMode::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "DebugMode");
    bool bRet = true;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(MMC_VEN_CMD63, 0x88000099)) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(MMC_VEN_CMD63, 0x88000000)) == false) break;
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_EnterDebugMode);
        }
    }
    return bRet;
}

int WriteSBParameter::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "WriteSB");
    PBYTE pBinBuff = (PBYTE)GET_PDT_DLG(lParam)->m_pstBinInfo->pBinaryAry[E_BIN_SB_YMN_ALL];
    int nStartSector = 0;
    int nBuffSize = GET_PDT_DLG(lParam)->m_pstBinInfo->nBinarySize[E_BIN_SB_YMN_ALL];

    bool bRet = true;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(MMC_VEN_CMD63, 0x10000000)) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_WriteCMD(pBinBuff, nStartSector, BYTE2SECTOR(nBuffSize))) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(32, 0x20000000)) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(33, 0x20030400)) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(61, 0x01002000)) == false) break;
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_WriteSBParameter);
        }

    }
    return bRet;
}

int WriteIBParameter_del::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "WriteIB");
    UINT nBuffSize = GET_PDT_DLG(lParam)->m_pstBinInfo->nBinarySize[E_BIN_IBEMMC];
    PBYTE pBinBuff = new BYTE[nBuffSize];
    memcpy(pBinBuff, GET_PDT_DLG(lParam)->m_pstBinInfo->pBinaryAry[E_BIN_IBEMMC], nBuffSize);

    BYTE ap_spec = GET_PDT_DLG(lParam)->m_pstPdtParms->stTestOpt.byAppSpec;
    int ap_ver = strtol(SVN_REVISION, nullptr, 16);
    ap_ver |= (int)(ap_spec << 24);
    //*(UINT*)(pBinBuff + 258) = _byteswap_ulong(ap_ver);
    int rev_ap_ver = _byteswap_ulong(ap_ver);
    memcpy(pBinBuff + 258, &rev_ap_ver, sizeof(int));
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
#ifdef _DEBUG
    UINT vSerial[64];
    memcpy(vSerial, GET_PDT_DLG(lParam)->m_pstPdtParms->nSerial, sizeof(UINT)*64);
#endif
    int nSerial = GET_PDT_DLG(lParam)->m_pstPdtParms->nSerial[wParam];
    REVERSE_CID_Type* pCid = (REVERSE_CID_Type*)(pBinBuff + 512+16);
    PBYTE ini = (PBYTE)(&nSerial);
    /**((PBYTE)pCid + 8) = *(ini + 2);
    *((PBYTE)pCid + 9) = *(ini + 3);
    *((PBYTE)pCid + 14) = *(ini + 0);
    *((PBYTE)pCid + 15) = *(ini + 1);*/
    memcpy(((PBYTE)pCid) + 8, ini + 2, sizeof(BYTE));
    memcpy(((PBYTE)pCid) + 9, ini + 3, sizeof(BYTE));
    memcpy(((PBYTE)pCid) + 14, ini + 0, sizeof(BYTE));
    memcpy(((PBYTE)pCid) + 15, ini + 2, sizeof(BYTE));
    int nStartSector = 0;

    bool bRet = true;
    
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(MMC_VEN_CMD63, 0x10000000)) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_WriteCMD(pBinBuff, nStartSector, BYTE2SECTOR(nBuffSize))) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(32, 0x20000000)) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(33, 0x20035000)) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(61, 0x01002000)) == false) break;
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_WriteIBParameter);
        }

    }
    delete[] pBinBuff;
    return bRet;
}


int WriteIBParameter::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();

    GET_PDT_DLG(lParam)->SetProgressText(wParam, "WriteIB");
    PBYTE pBinBuff = (PBYTE)GET_PDT_DLG(lParam)->m_pstBinInfo->pBinaryAry[E_BIN_IBEMMC];
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    int nStartSector = 0;
    int nBuffSize = GET_PDT_DLG(lParam)->m_pstBinInfo->nBinarySize[E_BIN_IBEMMC];

    bool bRet = true;

    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(MMC_VEN_CMD63, 0x10000000)) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_WriteCMD(pBinBuff, nStartSector, BYTE2SECTOR(nBuffSize))) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(32, 0x20000000)) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(33, 0x20035000)) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(61, 0x01002000)) == false) break;
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_WriteIBParameter);
        }

    }
    return bRet;
}


int Init_DCP::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "InitDCP");
    PBYTE pBinBuff = (PBYTE)GET_PDT_DLG(lParam)->m_pstBinInfo->pBinaryAry[E_BIN_SB_YMN_ALL];
    int nBuffSize = GET_PDT_DLG(lParam)->m_pstBinInfo->nBinarySize[E_BIN_SB_YMN_ALL];
    int nStartSector = 0;
    bool bRet = true;
    BYTE pData[512];
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(MMC_VEN_CMD63, OP_CODE(SOP_TF_INIT))) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_WriteCMD(pBinBuff, nStartSector, BYTE2SECTOR(nBuffSize))) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(MMC_VEN_CMD63, OP_CODE(SOP_READ_DATA))) == false) break;
            if ((bRet = (pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_ReadCMD(pData, 0, 1))) == false) break;
            unsigned char* pUniqueId = GET_PDT_DLG(lParam)->GetTesterUID(wParam);
            memcpy(pUniqueId, pData+0x80, 16);
#ifdef _DEBUG
            CString path;
            path.Format("UniqueID.bin");
            fnWriteFile(path, (char*)pUniqueId, 16);
#endif
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_Init_DCP);
        }

    }
    return bRet;
}


int pure_Init_DCP::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "InitDCP");
    PBYTE pBinBuff = (PBYTE)GET_PDT_DLG(lParam)->m_pstBinInfo->pBinaryAry[E_BIN_SB_YMN_ALL];
    int nBuffSize = GET_PDT_DLG(lParam)->m_pstBinInfo->nBinarySize[E_BIN_SB_YMN_ALL];
    int nStartSector = 0;
    bool bRet = true;

    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(MMC_VEN_CMD63, OP_CODE(SOP_TF_INIT))) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_WriteCMD(pBinBuff, nStartSector, BYTE2SECTOR(nBuffSize))) == false) break;
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_Init_DCP);
        }

    }
    return bRet;
}

int WriteEraseAllParam::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "EraseAll");
    PBYTE pBinBuff = (PBYTE)GET_PDT_DLG(lParam)->m_pstBinInfo->pBinaryAry[E_BIN_SB_YMN_ALL];
    int nStartSector = 0;
    int nBuffSize = GET_PDT_DLG(lParam)->m_pstBinInfo->nBinarySize[E_BIN_SB_YMN_ALL];
    bool bRet = true;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            // else  EraseAll OP_CODE  (0x15010003)OP_ERASE_SINGLE     (0x15010001)OP_ERASE_MULTI
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(MMC_VEN_CMD63, OP_STAGE(SOP_TOTAL_FORMAT, OP_ERASE_SINGLE))) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_WriteCMD(pBinBuff, nStartSector, BYTE2SECTOR(nBuffSize))) == false) break;

        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_WriteEraseAllParam);
        }
    }

    return bRet;
}

int PreformatGo::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "Preformat");
    PBYTE pBinBuff = (PBYTE)GET_PDT_DLG(lParam)->m_pstBinInfo->pBinaryAry[E_BIN_SB_YMN_ALL];
    int nStartSector = 0;
    int nBuffSize = GET_PDT_DLG(lParam)->m_pstBinInfo->nBinarySize[E_BIN_SB_YMN_ALL];
    bool bRet = true;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);

    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(MMC_VEN_CMD63, OP_CODE(SOP_TOTAL_FORMAT))) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_WriteCMD(pBinBuff, nStartSector, BYTE2SECTOR(nBuffSize))) == false) break;

        } while (0);
        UINT nErrorCode=0;
        if (pCont->GetDeviceLastError(wParam, lParam, nErrorCode))
        {
            if (nErrorCode)
            {//preformatGo执行有错误
                pCont->SetPdtStateError(wParam, lParam, nErrorCode);
                return false;
            }
        }
        else
        {//读取FW错误码发生异常
            pCont->SetPdtStateError(wParam, lParam, ERR_PreformatGo);
        }
        
    }

    return bRet;
}

int WriteSys::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "WriteSys");
    PBYTE pBinBuff = (PBYTE)GET_PDT_DLG(lParam)->m_pstBinInfo->pBinaryAry[E_BIN_INIT02];
    int nStartSector = 0;
    int nBuffSize = GET_PDT_DLG(lParam)->m_pstBinInfo->nBinarySize[E_BIN_INIT02];
    bool bRet = true;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            DWORD op = OP_CODE(SOP_WRITE_SYS);
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(MMC_VEN_CMD63, OP_CODE(SOP_WRITE_SYS))) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_WriteCMD(pBinBuff, nStartSector, BYTE2SECTOR(nBuffSize))) == false) break;

        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_WriteSys);
        }
    }
    return bRet;
}

int WriteISP::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "WriteIsp");
    PBYTE pBinBuff = (PBYTE)GET_PDT_DLG(lParam)->m_pstBinInfo->pBinaryAry[E_BIN_CODEPOOL];
    int nStartSector = 0;
    int nBuffSize = GET_PDT_DLG(lParam)->m_pstBinInfo->nBinarySize[E_BIN_CODEPOOL];
    bool bRet = true;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(MMC_VEN_CMD63, OP_CODE(SOP_WRITE_ISP))) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_WriteCMD(pBinBuff, nStartSector, BYTE2SECTOR(nBuffSize))) == false) break;
            Sleep(300);
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_WriteISP);
        }
    }
    return bRet;
}

int UpdateISP::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "WriteIsp");
    PBYTE pBinBuff = (PBYTE)GET_PDT_DLG(lParam)->m_pstBinInfo->pBinaryAry[E_BIN_CODEPOOL];
    int nStartSector = 0;
    int nBuffSize = GET_PDT_DLG(lParam)->m_pstBinInfo->nBinarySize[E_BIN_CODEPOOL];
    bool bRet = true;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(MMC_VEN_CMD63, OP_CODE(SOP_UPDATE_ISP))) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_WriteCMD(pBinBuff, nStartSector, BYTE2SECTOR(nBuffSize))) == false) break;
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_UpdateISP);
        }
    }
    return bRet;
}

int VerifyISP::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "VerifyIsp");
    PBYTE pISPBuff = (PBYTE)GET_PDT_DLG(lParam)->m_pstBinInfo->pBinaryAry[E_BIN_CODEPOOL];
    int nISPSize = GET_PDT_DLG(lParam)->m_pstBinInfo->nBinarySize[E_BIN_CODEPOOL];
    PBYTE pReadBuff = new BYTE[nISPSize];
    bool bRet = true;

    //
    PBYTE pBinBuff = (PBYTE)GET_PDT_DLG(lParam)->m_pstBinInfo->pBinaryAry[E_BIN_SB_YMN_ALL];
    int nBuffSize = GET_PDT_DLG(lParam)->m_pstBinInfo->nBinarySize[E_BIN_SB_YMN_ALL];

    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);

    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(MMC_VEN_CMD63, OP_CODE(SOP_READ_ISP))) == false) break;
            //if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_WriteCMD(pBinBuff, 0, BYTE2SECTOR(nBuffSize))) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_ReadCMD(pReadBuff, 0, BYTE2SECTOR(nISPSize))) == false) break;
            if (0 != memcmp(pISPBuff, pReadBuff, nISPSize))
            {//compare buffer error
                pCont->SetPdtStateError(wParam, lParam, ERR_CompareISP);
            }
        } while (0);
        if (!bRet)
        {//exe comand error
            pCont->SetPdtStateError(wParam, lParam, ERR_VerifyISP);
        }
    }
    delete[] pReadBuff;
    return bRet;
}

int CardVerify::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "CardVerify");
//#define C_USERAREABLOCK_NUM 683
#define DATA_BLOCK_TH   975
#define C_USERAREABLOCK_NUM 820
#define C_DefBlockMask 0x80
#define E_VERIFY_ERROR 0
#define E_VERIFY_SUCCESS 1
    BYTE pBuff[512];
    ZeroMemory(pBuff, 512);
    int nDataBlkTh, nDataBlkCnt, nDefBlkCnt;
    nDataBlkTh = C_USERAREABLOCK_NUM;   // user area
    nDataBlkTh++; // Index
    nDataBlkTh++; // Err backup
    nDataBlkTh += 6; // boot 0 + boot 1
    nDataBlkTh += 2; // rpmb
    nDataBlkTh += 56; //XW_InfoSlcAreaBlkNum;
    nDataBlkCnt = nDefBlkCnt = 0;

    int nPSRType = GET_PDT_DLG(lParam)->m_pstPdtParms->stTestOpt.cbPSRType;
    switch (nPSRType)
    {
    case E_DUWAN:
        nDataBlkTh = DATA_BLOCK_TH;
        break;
    default:
        nDataBlkTh += 48;
        break;
    }

    bool bRet=false;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(MMC_VEN_CMD63, OP_CODE(SOP_CARD_VERIFY))) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_ReadCMD(pBuff, 0, 1)) == false) break;
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_CardVerify);
            return bRet;
        }
    }
    nDataBlkCnt = *((DWORD*)pBuff);
    GET_PDT_DLG(lParam)->SetCardBlockCnt(wParam, nDataBlkCnt);
    if (nDataBlkCnt < (nDataBlkTh))
    {
        GET_PDT_DLG(lParam)->ShowCardBlockCnt(wParam, nDataBlkCnt);
        //ERR_CardBlockCnt
        PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext);
        pCont->SetPdtStateError(wParam, lParam, ERR_CardBlockCnt);
        bRet = FALSE;
    }
    return bRet;
}

int WriteIBlk::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "WriteIBLK");
    bool bRet = true;
    UINT nBuffSize = GET_PDT_DLG(lParam)->m_pstBinInfo->nBinarySize[E_BIN_IBEMMC];
    PBYTE pBinBuff = new BYTE[nBuffSize];
    memcpy(pBinBuff, GET_PDT_DLG(lParam)->m_pstBinInfo->pBinaryAry[E_BIN_IBEMMC], nBuffSize);

    BYTE ap_spec = GET_PDT_DLG(lParam)->m_pstPdtParms->stTestOpt.byAppSpec;
    int ap_ver = strtol(SVN_REVISION, nullptr, 16);
    ap_ver |= (int)(ap_spec << 24);
    //*(UINT*)(pBinBuff + 258) = _byteswap_ulong(ap_ver);
    int rev_ap_ver = _byteswap_ulong(ap_ver);
    memcpy(pBinBuff + 258, &rev_ap_ver, sizeof(int));
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
#ifdef _DEBUG
    UINT vSerial[64];
    memcpy(vSerial, GET_PDT_DLG(lParam)->m_pstPdtParms->nSerial, sizeof(UINT) * 64);
#endif
    int nSerial = GET_PDT_DLG(lParam)->m_pstPdtParms->nSerial[wParam];
    REVERSE_CID_Type* pCid = (REVERSE_CID_Type*)(pBinBuff + 512 + 16);
    PBYTE ini = (PBYTE)(&nSerial);
    memcpy(((PBYTE)pCid) + 8, ini + 2, sizeof(BYTE));
    memcpy(((PBYTE)pCid) + 9, ini + 3, sizeof(BYTE));
    memcpy(((PBYTE)pCid) + 14, ini + 0, sizeof(BYTE));
    memcpy(((PBYTE)pCid) + 15, ini + 2, sizeof(BYTE));
    int nStartSector = 0;
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(MMC_VEN_CMD63, OP_CODE(SOP_WRITE_IB))) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_WriteCMD(pBinBuff, nStartSector, BYTE2SECTOR(nBuffSize))) == false) break;
            Sleep(300);
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_WriteIBlk);
        }
    }
    delete[] pBinBuff;
    return bRet;
}

int PowerCycleTest::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "Reboot");

    bool bRet = true;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->CardPowerCycle(nDevNum)) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->CardInitial(nDevNum,FALSE)) == false) break;
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_PowerCycleTest);
        }
    }
    return bRet;
}


int FinishState::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressFinish(wParam);
    GET_PDT_DLG(lParam)->m_nStatusArray[wParam] = 2;
    //GET_PDT_DLG(lParam)->SetProgressText(wParam, "Finish");
    bool bRet = true;
    
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    UINT nSectorCnt;
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->CardPowerOff(nDevNum)) == false) break;
        } while (0);
    }
    return bRet;
}

int GetCardCapacity::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "ReadCap");
    bool bRet = true;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    UINT nSectorCnt;
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->GetSectorCount(nDevNum, nSectorCnt)) == false) break;
            float fCap = (double)nSectorCnt / (2 * 1024 * 1024.0);
            GET_PDT_DLG(lParam)->SetCS_Capacity(wParam, fCap);
            Sleep(300);
            //check capacity

            //ulSecCount
            //compare that
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_CheckCap);
        }
    }
    return bRet;
}

int SetPSR::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "SetPSR");

    bool bRet = true;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    int nPSRType = GET_PDT_DLG(lParam)->m_pstPdtParms->stTestOpt.cbPSRType;
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->SetPsrEx(nDevNum, GET_PDT_DLG(lParam)->m_pPsrInfo)) == false) break;
            //read result
            if ((bRet = pCont->m_pSpark3635Util->CardPowerCycle(nDevNum)) == false) break;
            //check capacity
            
            //ulSecCount
            //compare that
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_SetPSR);
        }
    }
    return bRet;
}

int ConfirmPSR::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "GetPSR");

    bool bRet = true;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    int nPSRType = GET_PDT_DLG(lParam)->m_pstPdtParms->stTestOpt.cbPSRType;
    BYTE buf[512];
    ZeroMemory(buf, 512);
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->GetPSR(nDevNum, buf)) == false) break;
            //read result
#ifdef _DEBUG
            CString str;
            str.Format("%s\\ComparePSR.bin", GET_PDT_DLG(lParam)->m_strPsrPath);
            fnWriteFile(str, (char*)buf, 32);
#endif
            //check capacity

            //ulSecCount
            //compare that
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_SetPSR);
        }
    }
    return bRet;
}

int CheckFWAndAppVersion::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
#define AP_SPEC_OFFSET 258
#define APP_OFFSET 258
#define FW_OFFSET 254

    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "FW&APP");
    LOG_INFO* lg = GET_PDT_DLG(lParam)->GetLogStruct(wParam);
    bool bRet = true;
    BYTE DataBuf[512] = { 0 };
    ExtCSD_Type* pExtCSD = (ExtCSD_Type*)DataBuf;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            UINT nSectorCnt = 0;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->GetExtCSD(DataBuf)) == false) break;
            BYTE byApSpec = *(BYTE*)(DataBuf + AP_SPEC_OFFSET);
            UINT32 nAppVer = _byteswap_ulong(*(UINT32*)(DataBuf + APP_OFFSET));
            UINT32 nFwVer = _byteswap_ulong(*(UINT32*)(DataBuf + FW_OFFSET));
            nAppVer &= 0x0000FFFF;
            nFwVer &= 0x0000FFFF;

            BYTE byDestApSpec = GET_PDT_DLG(lParam)->m_pstPickParms->ap_spec;
            UINT32 nDestAppVer = GET_PDT_DLG(lParam)->m_pstPickParms->ap_ver;
            UINT32 nDestFwVer = GET_PDT_DLG(lParam)->m_pstPickParms->fw_ver;

            CString strApSpecS, strApSpecD;
            strApSpecS.Format("%X", byApSpec);
            strApSpecD.Format("%X", byDestApSpec);

            CString strAppVerS,strAppVerD;
            strAppVerS.Format("%x", nAppVer);
            strAppVerD.Format("%d",nDestAppVer);
            CString strFwVerS, strFwVerD;
            strFwVerS.Format("%x", nFwVer);
            strFwVerD.Format("%d", nDestFwVer);
            lg->apSpec = strtoul(strApSpecS, nullptr, 16);
            lg->app_ver = strtoul(strAppVerS,nullptr,10);
            strcpy(lg->fw_ver, strFwVerS.GetBuffer());
            CString strContent;
            strContent.Format("FW Version:%s", strFwVerS);
            GET_PDT_DLG(lParam)->WritePickTestLog(strContent);
            strContent.Format("APP Version:%s", strAppVerS);
            GET_PDT_DLG(lParam)->WritePickTestLog(strContent);
            strContent.Format("Ap_Spec:%s", strApSpecS);
            GET_PDT_DLG(lParam)->WritePickTestLog(strContent);


#ifndef TestCardInfo
            if (strApSpecS != strApSpecD)
            {
                pCont->SetPdtStateError(wParam, lParam, ERR_AP_VER);
                return false;
            }
            if (strAppVerS != strAppVerD)
            {
                pCont->SetPdtStateError(wParam, lParam, ERR_AP_VER);
                return false;
            }
            if (strFwVerS != strFwVerD)
            {
                pCont->SetPdtStateError(wParam, lParam, ERR_FW_VER);
                return false;
            }
#endif
            
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_CheckFWAndAppVersion);
        }
    }

    return bRet;
}

int CheckCap::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "Capacity");
    LOG_INFO* lg = GET_PDT_DLG(lParam)->GetLogStruct(wParam);

    bool bRet = true;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            UINT nSectorCnt = 0;
            if ((bRet = pCont->m_pSpark3635Util->GetSectorCount(nDevNum, nSectorCnt)) == false) break;
            
            lg->capacity = nSectorCnt;
            CString strContent;
            strContent.Format("capacity:%d(sector)[%.2fGB]", nSectorCnt,(double)nSectorCnt/(2*1024*1024));
            GET_PDT_DLG(lParam)->WritePickTestLog(strContent);
#ifndef TestCardInfo
            if (GET_PDT_DLG(lParam)->m_pstPickParms->cap != nSectorCnt)
            {
                //capacity is not standard value
                //return 320L;
                pCont->SetPdtStateError(wParam, lParam, ERR_CAP);
                return false;
            }
#endif
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_CheckCap);
        }
    }
    return bRet;
}

int DUWAN_CheckCID::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "CID");
    LOG_INFO* lg = GET_PDT_DLG(lParam)->GetLogStruct(wParam);
    bool bRet = true;
    BYTE DataBuf[512] = { 0 };
    CString strContent;
    CID_Type* pCid = (CID_Type*)DataBuf;
    ST_PT_PARAM_INFO stVerifyInfo;
    memcpy(&stVerifyInfo, GET_PDT_DLG(lParam)->m_pstPickParms, sizeof(ST_PT_PARAM_INFO));
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->GetCID(DataBuf)) == false) break;
            BYTE destMID, sourceMID;
            destMID = stVerifyInfo.mid[0];
            sourceMID = pCid->MID;
            memcpy(lg->mid, &sourceMID, sizeof(sourceMID));
            strContent.Format("MID:0x%2.2x", sourceMID);
            GET_PDT_DLG(lParam)->WritePickTestLog(strContent);
            if (destMID != sourceMID)
            {
                //mid not match
                return false;
            }
            BYTE destOID, sourceOID;
            destOID = stVerifyInfo.oid[0];
            sourceOID = pCid->OID;
            memcpy(lg->oid, &sourceOID, sizeof(sourceOID));
            strContent.Format("OID:0x%2.2x", sourceOID);
            GET_PDT_DLG(lParam)->WritePickTestLog(strContent);
            if (destOID != sourceOID)
            {
                //oid not match
                return false;
            }
            strContent = "PNM:";
            CString strAsi = "";
            CString strLog = "";
            for (int i = 0; i < sizeof(pCid->PNM); ++i)
            {

                strContent.AppendFormat("%2.2x ", pCid->PNM[i]);
                strLog.AppendFormat("%2.2x ", pCid->PNM[i]);
                strAsi.AppendChar(pCid->PNM[i]);
            }
            memcpy(lg->pnm, &pCid->PNM, sizeof(pCid->PNM));
            strContent.AppendFormat("[%s]", strAsi);
            GET_PDT_DLG(lParam)->WritePickTestLog(strContent);
            if (memcmp(stVerifyInfo.pnm, pCid->PNM, sizeof(pCid->PNM)))
            {
                //pnm not match
                return false;
            }

            //CString strPsn="";
            for (int i = 0; i < sizeof(pCid->PSN); ++i)
            {
                //strPsn.AppendFormat("%2.2x",pCid->PSN[i]);
                lg->psn[i] = pCid->PSN[i];
            }
            //pCont->m_pickTestLogInfo.psn = strPsn;
            lg->mdt[0] = pCid->MDT;
            strContent.Format("MDT:0x%2.2x", pCid->MDT);
            int nYear = (pCid->MDT >> 4) + 2013;
            int nMonth = pCid->MDT & 0x0F;
            strContent.AppendFormat("[%d/%d]", nYear, nMonth);
            GET_PDT_DLG(lParam)->WritePickTestLog(strContent);
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_CheckCID);
        }
    }
    return bRet;
}


int CheckCID::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "CID");
    LOG_INFO* lg = GET_PDT_DLG(lParam)->GetLogStruct(wParam);
    bool bRet = true;
    BYTE DataBuf[512] = { 0 };
    CString strContent;
    CID_Type* pCid = (CID_Type*)DataBuf;
    ST_PT_PARAM_INFO stVerifyInfo;
    memcpy(&stVerifyInfo, GET_PDT_DLG(lParam)->m_pstPickParms,sizeof(ST_PT_PARAM_INFO));
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->GetCID(DataBuf)) == false) break;

            CString strCID = "CID:";
            for (int i = sizeof(CID_Type)-1; i >=0; --i)
            {
                strCID.AppendFormat(" %2.2x", DataBuf[i]);
            }
            GET_PDT_DLG(lParam)->WritePickTestLog(strCID);
            BYTE destMID, sourceMID;
            destMID = stVerifyInfo.mid[0];
            sourceMID = pCid->MID;
            memcpy(lg->mid, &sourceMID, sizeof(sourceMID));
            strContent.Format("MID:0x%2.2x", sourceMID);
            GET_PDT_DLG(lParam)->WritePickTestLog(strContent);
#ifndef TestCardInfo
            if (destMID != sourceMID)
            {
                //mid not match
                pCont->SetPdtStateError(wParam, lParam, ERR_MID);
                return false;
            }
#endif
            BYTE destOID, sourceOID;
            destOID = stVerifyInfo.oid[0];
            sourceOID = pCid->OID;
            memcpy(lg->oid, &sourceOID, sizeof(sourceOID));
            strContent.Format("OID:0x%2.2x", sourceOID);
            GET_PDT_DLG(lParam)->WritePickTestLog(strContent);
#ifndef TestCardInfo
            if (destOID != sourceOID)
            {
                //oid not match
                pCont->SetPdtStateError(wParam, lParam, ERR_OID);
                return false;
            }
#endif
            strContent = "PNM:";
            CString strAsi="";
            CString strLog="";
            BYTE revPNM[sizeof(pCid->PNM)] = { 0 };

            for (int i = sizeof(pCid->PNM)-1; i >=0; --i)
            {
                revPNM[i] = pCid->PNM[5 - i];
                strContent.AppendFormat("%2.2x ",pCid->PNM[i]);
                strLog.AppendFormat("%2.2x ", pCid->PNM[i]);
                strAsi.AppendChar(pCid->PNM[i]);
            }
            //兼容老版本
            int n = 0;
            while (revPNM[n] == 0x00)
            {
                int idx = 0;
                for (idx = n; idx < sizeof(revPNM) - 1; ++idx)
                {
                    revPNM[idx] = revPNM[idx + 1];
                }
                revPNM[idx] = 0x00;
            }
            //
            memcpy(lg->pnm, revPNM, sizeof(pCid->PNM));
            strContent.AppendFormat("[%s]",strAsi);
            GET_PDT_DLG(lParam)->WritePickTestLog(strContent);
#ifndef TestCardInfo
            if (memcmp(stVerifyInfo.pnm, revPNM, sizeof(revPNM)))
            {
                //pnm not match
                pCont->SetPdtStateError(wParam, lParam, ERR_PNM);
                return false;
            }
#endif
            //SN
            CString strPsn="SN:";
            for (int i = sizeof(pCid->PSN)-1; i >=0; --i)
            {
                strPsn.AppendFormat("%2.2x",pCid->PSN[i]);
                lg->psn[i] = pCid->PSN[3-i];
            }
            GET_PDT_DLG(lParam)->WritePickTestLog(strPsn);

            //prv
            CString strPRV = "PRV:0x";
            BYTE prv = pCid->PRV;
            strPRV.AppendFormat("%2.2x", prv);
            lg->prv[0] = prv;
            GET_PDT_DLG(lParam)->WritePickTestLog(strPRV);
#ifndef TestCardInfo
            if (memcmp(stVerifyInfo.prv, &prv, sizeof(prv)))
            {
                //pnm not match
                pCont->SetPdtStateError(wParam, lParam, ERR_PRV);
                return false;
            }
#endif
            //factory
            CString strFactory = "FACTORY:0x";
            BYTE fac = pCid->Reserved1;
            strFactory.AppendFormat("%x", fac);
            lg->factory[0] = fac;
            GET_PDT_DLG(lParam)->WritePickTestLog(strFactory);
#ifndef TestCardInfo
            if (memcmp(stVerifyInfo.factory, &fac, sizeof(fac)))
            {
                //pnm not match
                pCont->SetPdtStateError(wParam, lParam, ERR_FAC);
                return false;
            }
#endif
            //mdt
            lg->mdt[0] = pCid->MDT;
            strContent.Format("MDT:0x%2.2x",pCid->MDT);
            int nYear = (pCid->MDT >> 4) +2013;
            int nMonth = pCid->MDT & 0x0F;
            strContent.AppendFormat("[%d/%d]",nYear,nMonth);
            GET_PDT_DLG(lParam)->WritePickTestLog(strContent);
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_CheckCID);
        }
    }
    return bRet;
}

int PickTestFinish::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->m_nStatusArray[wParam] = 2;

    GET_PDT_DLG(lParam)->SetProgressText(wParam, "Finish");
    GET_PDT_DLG(lParam)->SetProgressFinish(wParam);
    GET_PDT_DLG(lParam)->WritePickTestLog("*************************PickTestFinish*************************");
    return true;
}

int ReadUniqueID::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "R UID");
    PBYTE pBinBuff = (PBYTE)GET_PDT_DLG(lParam)->m_pstBinInfo->pBinaryAry[E_BIN_SB_YMN_ALL];
    int nBuffSize = GET_PDT_DLG(lParam)->m_pstBinInfo->nBinarySize[E_BIN_SB_YMN_ALL];
    int nStartSector = 0;
    bool bRet = true;

    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(MMC_VEN_CMD63, OP_CODE(SOP_TF_INIT))) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_WriteCMD(pBinBuff, nStartSector, BYTE2SECTOR(nBuffSize))) == false) break;
            BYTE pData[512];
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(MMC_VEN_CMD63, OP_CODE(SOP_READ_DATA))) == false) break;
            if ((bRet = (pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_ReadCMD(pData, 0, 1))) == false) break;


            GET_PDT_DLG(lParam)->SetTesterUID(wParam, pData + 0x80,16);
            CString str="UNIQUE ID:";
            BYTE* p = pData + 0x80;
            for (int i = 0; i < 16; ++i)
            {
                str.AppendFormat("%2.2x ", p[i]);
            }
            GET_PDT_DLG(lParam)->WritePickTestLog(str);
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_ReadUniqueID);
        }
    }
    return bRet;
}

int ReadSmart::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    //
    TRACE_FUNC();
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "Smart");
    BYTE buf[512];

    bool bRet = true;

    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->ReadSmart(buf)) == false) break;
        } while (0);

        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_ReadSmart);
        }
    }
    return bRet;
}


int ReadFlashID::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "FlashID");
    BYTE pFlashId[512];
    PBYTE pBinBuff = (PBYTE)GET_PDT_DLG(lParam)->m_pstBinInfo->pBinaryAry[E_BIN_SB_YMN_ALL];
    int nBuffSize = GET_PDT_DLG(lParam)->m_pstBinInfo->nBinarySize[E_BIN_SB_YMN_ALL];
    int nStartSector = 0;
    bool bRet = true;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(MMC_VEN_CMD63, OP_CODE(SOP_TF_INIT))) == false) break;
            if ((bRet = (pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_ReadCMD(pFlashId, 0, 1))) == false) break;
            int i, j;
            int nCeCnt = 0;
            for (i = 0; i < 4; ++i)
            {
                int nBeginPosition = 16 * i;
                for (j = 0; j < 5; ++j)
                {
                    if (pFlashId[nBeginPosition + j] == 0x00 || pFlashId[nBeginPosition + j] == 0xff)
                    {
                        break;
                    }
                }
                if (j >= 5)
                {
                    nCeCnt++;
                }
            }
            GET_PDT_DLG(lParam)->SetCardCeCnt(wParam, nCeCnt);

        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_ReadFlashID);
        }

    }
    return bRet;
}

int SRAM_TEST::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "SRAM");
    BYTE pBuff[512*2];
    bool bRet = true;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);

    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(MMC_VEN_CMD63, OP_CODE(SOP_SRAM_TEST))) == false) break;
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_WriteCMD(pBuff, 0, BYTE2SECTOR(sizeof(pBuff)))) == false) break;


        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_SramTest);
        }
    }

    return bRet;
}

int VerifyFwAndApVersion::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "FW&AP");
    LOG_INFO* lg = GET_PDT_DLG(lParam)->GetLogStruct(wParam);
    bool bRet = true;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);

    {
#define APP_OFFSET 258
#define FW_OFFSET 254
        BYTE DataBuf[512] = { 0 };
        ExtCSD_Type* pExtCSD = (ExtCSD_Type*)DataBuf;
        if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
        {
            do
            {
                UINT nSectorCnt = 0;
                if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->GetExtCSD(DataBuf)) == false) break;
                UINT32 nAppVer = _byteswap_ulong(*(UINT32*)(DataBuf + APP_OFFSET));
                UINT32 nFwVer = _byteswap_ulong(*(UINT32*)(DataBuf + FW_OFFSET));
                nAppVer &= 0x0000FFFF;
                nFwVer &= 0x0000FFFF;

                UINT32 nDestAppVer = GET_PDT_DLG(lParam)->m_pstPickParms->ap_ver;
                UINT32 nDestFwVer = GET_PDT_DLG(lParam)->m_pstPickParms->fw_ver;

                CString strAppVerS, strAppVerD;
                strAppVerS.Format("%x", nAppVer);
                strAppVerD.Format("%d", nDestAppVer);
                CString strFwVerS, strFwVerD;
                strFwVerS.Format("%x", nFwVer);
                strFwVerD.Format("%d", nDestFwVer);
                lg->app_ver = strtoul(strAppVerS, nullptr, 10);
                strcpy(lg->fw_ver, strFwVerS.GetBuffer());
                CString strContent;
                strContent.Format("FW Version:%s", strFwVerS);
                GET_PDT_DLG(lParam)->WritePickTestLog(strContent);
                strContent.Format("APP Version:%s", strAppVerS);
                GET_PDT_DLG(lParam)->WritePickTestLog(strContent);
                if (strAppVerS != strAppVerD)
                {
                    bRet = FALSE;
                }
                if (strFwVerS != strFwVerD)
                {
                    bRet = FALSE;
                }

            } while (0);
            if (!bRet)
            {
                pCont->SetPdtStateError(wParam, lParam, ERR_CheckFWAndAppVersion);
            }
        }
        //return bRet;
    }
    //
    // 
    //
    if (bRet)
        return bRet;

    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        UINT nFwVer = 0, nMergeAppValue = 0;
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->ReadFwAndAppVersion(nDevNum, nFwVer, nMergeAppValue)) == false) break;

            BYTE byDestApSpec = GET_PDT_DLG(lParam)->m_pstPickParms->ap_spec;
            UINT32 nDestAppVer = GET_PDT_DLG(lParam)->m_pstPickParms->ap_ver;
            UINT32 nDestFwVer = GET_PDT_DLG(lParam)->m_pstPickParms->fw_ver;

            BYTE byApSpec = nMergeAppValue >> 24;
            CString strApSpecS, strApSpecD;
            strApSpecS.Format("%X", byApSpec);
            strApSpecD.Format("%X", byDestApSpec);

            int nAppVer = nMergeAppValue & 0x0000FFFF;
            CString strAppVerS, strAppVerD;
            strAppVerS.Format("%x", nAppVer);
            strAppVerD.Format("%d", nDestAppVer);
            CString strFwVerS, strFwVerD;
            strFwVerS.Format("%x", nFwVer);
            strFwVerD.Format("%d", nDestFwVer);
            lg->apSpec = strtoul(strApSpecS, nullptr, 16);
            lg->app_ver = strtoul(strAppVerS, nullptr, 10);
            strcpy(lg->fw_ver, strFwVerS.GetBuffer());
            CString strContent;
            strContent.Format("FW Version:%s", strFwVerS);
            GET_PDT_DLG(lParam)->WritePickTestLog(strContent);
            strContent.Format("APP Version:%s", strAppVerS);
            GET_PDT_DLG(lParam)->WritePickTestLog(strContent);
            strContent.Format("Ap_Spec:%s", strApSpecS);
            GET_PDT_DLG(lParam)->WritePickTestLog(strContent);
#ifndef TestCardInfo
            if (strApSpecS != strApSpecD)
            {
                pCont->SetPdtStateError(wParam, lParam, ERR_AP_VER);
                return false;
            }
            if (strAppVerS != strAppVerD)
            {
                pCont->SetPdtStateError(wParam, lParam, ERR_AP_VER);
                return false;
            }
            if (strFwVerS != strFwVerD)
            {
                pCont->SetPdtStateError(wParam, lParam, ERR_FW_VER);
                return false;
            }
#endif
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_CheckFWAndAppVersion);
        }
    }
    return bRet;
}



int RestartPowerSupply::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "Restart");
    BYTE pBuff[512 * 2];
    bool bRet = true;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);

    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            //Power off
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(SET_POWER, 0x01)) == false) break;
            Sleep(1000);
            //Power On
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(SET_POWER, 0x00)) == false) break;
            Sleep(100);

        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_Restart);
        }
    }

    return bRet;
}

int DeviceInitState::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "DeviceInit");
    BYTE pBuff[512 * 2];
    bool bRet = true;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    int nDevNo = GET_PDT_DLG(lParam)->m_pHostWrapper->m_DevNo[nDevNum];
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            Sleep(500);
            if ((bRet = pCont->m_pSpark3635Util->DeviceInitial(nDevNum, nDevNo)) == false) break;
            Sleep(1000);
            //if ((bRet = pCont->m_pSpark3635Util->CardPowerCycle(nDevNum)) == false) break;
            //Sleep(1000);

        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_Restart);
        }
    }

    return bRet;
}

int CheckFWAndAppVersionForDuwanMWE::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "FW&AP");

    bool bRet = true;
    LOG_INFO* lg = GET_PDT_DLG(lParam)->GetLogStruct(wParam);
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    UINT nFwVer = 0, nAppVer = 0;
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->GetFwAndAppVerForDuWan(nDevNum, nFwVer, nAppVer)) == false) break;

            UINT32 nDestAppVer = GET_PDT_DLG(lParam)->m_pstPickParms->ap_ver;
            UINT32 nDestFwVer = GET_PDT_DLG(lParam)->m_pstPickParms->fw_ver;
            nAppVer &= 0x0000FFFF;
            CString strAppVerS, strAppVerD;
            strAppVerS.Format("%x", nAppVer);
            strAppVerD.Format("%d", nDestAppVer);
            CString strFwVerS, strFwVerD;
            strFwVerS.Format("%x", nFwVer);
            strFwVerD.Format("%d", nDestFwVer);
            lg->app_ver = strtoul(strAppVerS, nullptr, 10);
            strcpy(lg->fw_ver, strFwVerS.GetBuffer());
            CString strContent;
            strContent.Format("FW Version:%s", strFwVerS);
            GET_PDT_DLG(lParam)->WritePickTestLog(strContent);
            strContent.Format("APP Version:%s", strAppVerS);
            GET_PDT_DLG(lParam)->WritePickTestLog(strContent);
            if (strAppVerS != strAppVerD)
            {
                return false;
            }
            if (strFwVerS != strFwVerD)
            {
                return false;
            }
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_CheckFWAndAppVersion);
        }
    }
    return bRet;
}

int CheckFlashID::Handle(CContext* pContext, WPARAM wParam, LPARAM lParam)
{
    TRACE_FUNC();
    GET_PDT_DLG(lParam)->SetProgressText(wParam, "CheckID");
    BYTE pFlashId[512];
    PBYTE pBinBuff = (PBYTE)GET_PDT_DLG(lParam)->m_pstBinInfo->pBinaryAry[E_BIN_SB_YMN_ALL];
    int nBuffSize = GET_PDT_DLG(lParam)->m_pstBinInfo->nBinarySize[E_BIN_SB_YMN_ALL];
    int nStartSector = 0;
    bool bRet = true;
    int nDevNum = GET_PDT_DLG(lParam)->MapToLogicIndex(wParam);
    if (PrefStartContext* pCont = dynamic_cast<PrefStartContext*>(pContext))
    {
        do
        {
            if ((bRet = pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_CMD(MMC_VEN_CMD63, OP_CODE(SOP_TF_INIT))) == false) break;
            if ((bRet = (pCont->m_pSpark3635Util->m_pHost[nDevNum]->SDMMC_ReadCMD(pFlashId, 0, 1))) == false) break;
            for(int i=0; i< GET_PDT_DLG(lParam)->m_pFlashIdConfig->byCeNum;++i)
            {
                for (int j = 0; j < 6; ++j)
                {
                    if (GET_PDT_DLG(lParam)->m_pFlashIdConfig->flashID[j] != pFlashId[i*16+j])
                    {
                        pCont->SetPdtStateError(wParam, lParam, ERR_CheckFlashID);
                        return FALSE;
                    }
                }
            } 
            
        } while (0);
        if (!bRet)
        {
            pCont->SetPdtStateError(wParam, lParam, ERR_ReadFlashID);
        }

    }
    return bRet;
}
