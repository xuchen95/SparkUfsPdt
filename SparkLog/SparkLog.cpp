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
static std::condition_variable g_queueCv;
static std::queue<pdt_log_config_t> g_logQueue;
static bool g_logThreadRunning = false;
static bool g_logThreadStop = false;

static const wchar_t* LOG_DIR = L".\\LogInfo";
static const wchar_t* LOG_FILE = L".\\LogInfo\\TF_LOG.log";

// No AnsiToW conversion needed in multibyte build; format ANSI strings directly.

// Ensure the log directory exists. No-op if already present.
static void EnsureLogDir()
{
    // Create directory if not exists
    CreateDirectoryW(LOG_DIR, NULL);
}

// SparkLog_EnqueueLine removed - no longer used.

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
                "%s %s | Port=%d | Func=%s | CardId=%s | FW=%s | APP=%s | Tester=%s | MID=0x%02X | OID=0x%02X | Manu=%s | Product=%s | SN=%s | Start=%s %s | Build=%s | State=%s | Error=0x%X",
                cfg.start_date, cfg.start_time, (int)cfg.ufs_port,
                cfg.func_name, cfg.card_id, cfg.fw_version, cfg.app_version, cfg.tester_version,
                (unsigned char)cfg.mid[0], (unsigned char)cfg.oid[0], cfg.manufacturer, cfg.product_name,
                cfg.serial_number, cfg.start_date, cfg.start_time, cfg.build_time, cfg.state, cfg.error_code);

            // convert ANSI line to UTF-8 for consistent log encoding
            int wideLen = MultiByteToWideChar(CP_ACP, 0, lineAnsi, -1, NULL, 0);
            if (wideLen > 0)
            {
                std::wstring wide(wideLen, L'\0');
                MultiByteToWideChar(CP_ACP, 0, lineAnsi, -1, &wide[0], wideLen);
                if (!wide.empty() && wide.back() == L'\0') wide.pop_back();

                int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, NULL, 0, NULL, NULL);
                if (utf8Len > 0)
                {
                    std::string utf8(utf8Len, '\0');
                    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &utf8[0], utf8Len, NULL, NULL);
                    if (!utf8.empty() && utf8.back() == '\0') utf8.pop_back();

                    // write to file
                    FILE* fp = NULL;
                    errno_t e = _wfopen_s(&fp, LOG_FILE, L"a, ccs=UTF-8");
                    if (e == 0 && fp)
                    {
                        fprintf(fp, "%s\n", utf8.c_str());
                        fclose(fp);
                    }
                }
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

    // convert ANSI/multibyte string to UTF-8 and append to LOG_FILE
    int wideLen = MultiByteToWideChar(CP_ACP, 0, line.c_str(), -1, NULL, 0);
    std::wstring wide;
    if (wideLen > 0)
    {
        wide.resize(wideLen);
        MultiByteToWideChar(CP_ACP, 0, line.c_str(), -1, &wide[0], wideLen);
        if (!wide.empty() && wide.back() == L'\0') wide.pop_back();
    }

    int utf8Len = 0;
    std::string utf8;
    if (!wide.empty())
    {
        utf8Len = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, NULL, 0, NULL, NULL);
        if (utf8Len > 0)
        {
            utf8.resize(utf8Len);
            WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &utf8[0], utf8Len, NULL, NULL);
            if (!utf8.empty() && utf8.back() == '\0') utf8.pop_back();
        }
    }

    EnterCriticalSection(&g_sparkLogLock);
    FILE* fp = NULL;
    errno_t e = _wfopen_s(&fp, LOG_FILE, L"a, ccs=UTF-8");
    if (e == 0 && fp)
    {
        if (!utf8.empty()) fprintf(fp, "%s\n", utf8.c_str());
        else fprintf(fp, "%s\n", line.c_str());
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
}

void SparkLog_EnqueuePdtLog(const pdt_log_config_t& cfg)
{
    {
        std::lock_guard<std::mutex> lk(g_queueMutex);
        g_logQueue.push(cfg);
    }
    g_queueCv.notify_one();
}

