// SparkLog.cpp : implementation of the SparkLog static library.
// This module provides a small asynchronous logging facility used by the
// production test tooling. It exposes an enqueue API to push structured
// pdt_log_config_t records that are consumed by a background thread and
// written to a UTF-8 encoded log file under the LogInfo directory.
//
// P2-1 root-cure:
//   * Replaced CRITICAL_SECTION g_sparkLogLock with a single std::mutex g_fileMutex
//     that protects ALL file open/write/close paths, both the synchronous
//     SparkLog_Append and the async LogWorker.  Previously the two writers used
//     different (or zero) locking, so log lines could interleave on disk.
//   * Swapped SparkLog_Close order: stop + join the background thread FIRST,
//     then tear down shared state (path buffers).  The previous code deleted
//     the CRITICAL_SECTION while the worker could still be running, which is
//     undefined behaviour per the Win32 documentation for DeleteCriticalSection.
//   * Lazy-init flag switched to std::atomic<bool> to avoid torn reads/writes
//     when SparkLog_Init / SparkLog_EnqueuePdtLog are called from multiple
//     threads on the first call.


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
#include <atomic>
#include <direct.h>
#include <stdlib.h>

// P2-1: single mutex that protects every fopen/fwrite/fputc/fclose call
// made by either SparkLog_Append or LogWorker. This guarantees log lines
// are never interleaved even when the sync and async writers run concurrently.
static std::mutex            g_fileMutex;
static std::atomic<bool>     g_sparkLogInited{false};

// background queue for production logs
static std::thread             g_logThread;
static std::mutex              g_queueMutex;
static std::mutex              g_pathMutex;
static std::condition_variable g_queueCv;
static std::queue<pdt_log_config_t> g_logQueue;
static bool g_logThreadRunning = false;
static bool g_logThreadStop    = false;

static std::string g_logDir  = "./XHSUM";
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
        g_logDir  = path;
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
            char tmp[4];
            char szUID[64] = { 0 };
			for (int i = 0; i < 16; ++i)
			{
				sprintf_s(tmp, "%02X ", (unsigned char)cfg.UID[i]);
                strcat_s(szUID, tmp);
			}
            // format ANSI line directly into a char buffer
            char lineAnsi[2048];
            _snprintf_s(lineAnsi, _countof(lineAnsi), _TRUNCATE,
                "Port=%d,Func=%s,UID=%s,FW=%s,APP=%s,Tester=%s,MID=0x%s,OID=0x%s,Manu=%s,Product=%s,SN=%s,PRV=%s,Start=%s %s,Build=%d,State=%s,Error=0x%X",
                (int)cfg.ufs_port,
                cfg.func_name, szUID, cfg.fw_version, cfg.app_version, cfg.tester_version,
                cfg.mid, cfg.oid, cfg.manufacturer, cfg.product_name,
                cfg.serial_number, cfg.prv, cfg.start_date, cfg.start_time, cfg.build_time, cfg.stage, cfg.error_code);

            // write to file as ANSI (no conversion)
            // P2-1: lock g_fileMutex so SparkLog_Append cannot race with us.
            std::string logFilePath;
            {
                std::lock_guard<std::mutex> lkPath(g_pathMutex);
                logFilePath = g_logFile;
            }
            FILE* fp = NULL;
            {
                std::lock_guard<std::mutex> lkFile(g_fileMutex);
                errno_t e = fopen_s(&fp, logFilePath.c_str(), "ab");
                if (e == 0 && fp)
                {
                    size_t len = strlen(lineAnsi);
                    fwrite(lineAnsi, 1, len, fp);
                    fputc('\n', fp);
                    fclose(fp);
                }
            }

            lk.lock();
        }
    }
}

void SparkLog_Init()
{
    // P2-1: atomic double-check so multi-threaded first-init does not tear.
    if (!g_sparkLogInited.load(std::memory_order_acquire))
    {
        // CS g_sparkLogLock removed; g_fileMutex is a std::mutex and is
        // implicitly constructed/destructed as a static object.
        g_sparkLogInited.store(true, std::memory_order_release);
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
    if (!g_sparkLogInited.load(std::memory_order_acquire))
        SparkLog_Init();

    std::string logFilePath;
    {
        std::lock_guard<std::mutex> lkPath(g_pathMutex);
        logFilePath = g_logFile;
    }
    // P2-1: lock g_fileMutex so LogWorker cannot race with us.  This replaces
    // the old EnterCriticalSection/LeaveCriticalSection that used a different
    // lock than the async writer (and was UB because Close deleted it early).
    FILE* fp = NULL;
    {
        std::lock_guard<std::mutex> lkFile(g_fileMutex);
        errno_t e = fopen_s(&fp, logFilePath.c_str(), "ab");
        if (e == 0 && fp)
        {
            fwrite(line.c_str(), 1, line.size(), fp);
            fputc('\n', fp);
            fclose(fp);
        }
    }
}

void SparkLog_Close()
{
    // P2-1: shutdown order matters.
    //   1. Stop the background worker and JOIN it first.  This guarantees no
    //      more reads of g_logFile / g_logDir nor any more calls into the
    //      logging primitives by the worker.
    //   2. Only THEN release shared state (path strings, etc.).
    //
    // Previously DeleteCriticalSection(&g_sparkLogLock) ran BEFORE the join,
    // which is undefined if the worker is still running.  There is no more
    // CS, but we keep the correct order anyway.
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
        std::lock_guard<std::mutex> lkPath(g_pathMutex);
        g_logDir.clear();   g_logDir.shrink_to_fit();
        g_logFile.clear();  g_logFile.shrink_to_fit();
    }

    // Clear the lazy-init flag so a subsequent SparkLog_Init() call can
    // re-initialise the subsystem idempotently.  Relaxed ordering is fine
    // because all workers are already joined above.
    g_sparkLogInited.store(false, std::memory_order_relaxed);
}

void SparkLog_EnqueuePdtLog(const pdt_log_config_t& cfg)
{
    // Lazy-init logging thread to ensure enqueue works even if caller forgot init.
    if (!g_sparkLogInited.load(std::memory_order_acquire))
    {
        SparkLog_Init();
    }

    {
        std::lock_guard<std::mutex> lk(g_queueMutex);
        g_logQueue.push(cfg);
    }
    g_queueCv.notify_one();
}

