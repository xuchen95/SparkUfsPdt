// SparkLog.cpp : implementation of the SparkLog static library.
// This module provides a small asynchronous logging facility used by the
// production test tooling. It exposes an enqueue API to push structured
// pdt_log_config_t records that are consumed by a background thread and
// written to a UTF-8 encoded log file under the LogInfo directory.
//

#include "pch.h"
#include "framework.h"
// Use plain Win32 API to avoid MFC dependency in this static lib
#include "SparkLog.h"
#include <windows.h>
#include <stdio.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <string>
#include <direct.h>
#include <stdlib.h>

static CRITICAL_SECTION g_sparkLogLock;
static bool g_sparkLogInited = false;

// background queue for production logs
static std::thread g_logThread;
static std::mutex g_queueMutex;
static std::mutex g_pathMutex;
static std::condition_variable g_queueCv;
static std::queue<pdt_log_config_t> g_logQueue;
static bool g_logThreadRunning = false;
static bool g_logThreadStop = false;

static std::string g_logDir = "./XHSUM";
static std::string g_logFile = "./XHSUM/TF_LOG.log";

// Ensure the log directory exists. No-op if already present.
static void EnsureLogDir()
{
    std::string dir;
    {
        std::lock_guard<std::mutex> lk(g_pathMutex);
        dir = g_logDir;
    }
    CreateDirectoryA(dir.c_str(), NULL);
}

void SparkLog_SetReportPath(const char* reportPath)
{
    if (!reportPath || reportPath[0] == '\0')
        return;

    std::string path(reportPath);
    // Remove trailing slashes
    while (!path.empty() && (path.back() == '\\' || path.back() == '/'))
        path.pop_back();
    if (path.empty())
        return;

    std::string logFile = path + "/TF_LOG.log";
    {
        std::lock_guard<std::mutex> lk(g_pathMutex);
        g_logDir = path;
        g_logFile = logFile;
    }
}

static void LogWorker()
{
    EnsureLogDir();
    std::unique_lock<std::mutex> lk(g_queueMutex);
    while (!g_logThreadStop || !g_logQueue.empty())
    {
        if (g_logQueue.empty())
        {
            g_queueCv.wait(lk, [] { return g_logThreadStop || !g_logQueue.empty(); });
        }
        while (!g_logQueue.empty())
        {
            pdt_log_config_t cfg = g_logQueue.front();
            g_logQueue.pop();
            lk.unlock();

            // format ANSI line directly into a char buffer
            char lineAnsi[2048];
            _snprintf_s(lineAnsi, _countof(lineAnsi), _TRUNCATE,
                "Port=%d,Func=%s,CardId=%s,FW=%s,APP=%s,Tester=%s,MID=0x%02X,OID=0x%02X,Manu=%s,Product=%s,SN=%s,Start=%s %s,Build=%d,State=%s,Error=0x%X",
                (int)cfg.ufs_port,
                cfg.func_name, cfg.card_id, cfg.fw_version, cfg.app_version, cfg.tester_version,
                (unsigned char)cfg.mid[0], (unsigned char)cfg.oid[0], cfg.manufacturer, cfg.product_name,
                cfg.serial_number, cfg.start_date, cfg.start_time, cfg.build_time, cfg.state, cfg.error_code);

            // write to file as ANSI (no conversion)
            FILE* fp = NULL;
            std::string logFilePath;
            {
                std::lock_guard<std::mutex> lk(g_pathMutex);
                logFilePath = g_logFile;
            }
            errno_t e = fopen_s(&fp, logFilePath.c_str(), "ab");
            if (e == 0 && fp)
            {
                size_t len = strlen(lineAnsi);
                fwrite(lineAnsi, 1, len, fp);
                fputc('\n', fp);
                fclose(fp);
            }

            lk.lock();
        }
    }
}

void SparkLog_Init()
{
    if (!g_sparkLogInited)
    {
        InitializeCriticalSection(&g_sparkLogLock);
        g_sparkLogInited = true;
    }

    // start background thread once
    {
        std::lock_guard<std::mutex> lk(g_queueMutex);
        if (!g_logThreadRunning)
        {
            g_logThreadStop = false;
            g_logThread = std::thread(LogWorker);
            g_logThreadRunning = true;
        }
    }
}

void SparkLog_Append(const std::string& line)
{
    if (!g_sparkLogInited)
        SparkLog_Init();

    EnterCriticalSection(&g_sparkLogLock);
    FILE* fp = NULL;
    std::string logFilePath;
    {
        std::lock_guard<std::mutex> lk(g_pathMutex);
        logFilePath = g_logFile;
    }
    errno_t e = fopen_s(&fp, logFilePath.c_str(), "ab");
    if (e == 0 && fp)
    {
        fwrite(line.c_str(), 1, line.size(), fp);
        fputc('\n', fp);
        fclose(fp);
    }
    LeaveCriticalSection(&g_sparkLogLock);
}

void SparkLog_Close()
{
    if (g_sparkLogInited)
    {
        DeleteCriticalSection(&g_sparkLogLock);
        g_sparkLogInited = false;
    }

    // stop background thread
    {
        std::unique_lock<std::mutex> lk(g_queueMutex);
        if (g_logThreadRunning)
        {
            g_logThreadStop = true;
            lk.unlock();
            g_queueCv.notify_one();
            if (g_logThread.joinable()) g_logThread.join();
            g_logThreadRunning = false;
        }
    }

    // Release heap buffers for path strings so they do not appear in
    // differential memory-leak reports taken after this call returns.
    {
        std::lock_guard<std::mutex> lk(g_pathMutex);
        g_logDir.clear();  g_logDir.shrink_to_fit();
        g_logFile.clear(); g_logFile.shrink_to_fit();
    }
}

void SparkLog_EnqueuePdtLog(const pdt_log_config_t& cfg)
{
    {
        std::lock_guard<std::mutex> lk(g_queueMutex);
        g_logQueue.push(cfg);
    }
    g_queueCv.notify_one();
}

