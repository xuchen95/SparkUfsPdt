#pragma once
#include "pch.h"
#include <vector>
#include <queue>
#include <functional>
#include <future>
#include <memory>
#include <type_traits>
#include <windows.h>
#include <process.h>

// Small thread pool implementation using Win32 APIs so it's compatible with Windows XP/7
class ThreadPool
{
public:
    explicit ThreadPool(size_t threads = 0)
        : stop_(false), threads_(threads)
    {
        if (threads_ == 0)
        {
            // default to number of processors
            SYSTEM_INFO si;
            GetSystemInfo(&si);
            threads_ = (si.dwNumberOfProcessors > 0) ? si.dwNumberOfProcessors : 1;
        }

        InitializeCriticalSection(&queueLock_);
        taskSemaphore_ = CreateSemaphore(NULL, 0, 0x7fffffff, NULL);

        for (size_t i = 0; i < threads_; ++i)
        {
            unsigned threadID = 0;
            HANDLE h = (HANDLE)_beginthreadex(NULL, 0, &ThreadPool::WorkerProc, this, 0, &threadID);
            if (h)
                workers_.push_back(h);
        }
    }

    ~ThreadPool()
    {
        // signal stop and wake up all workers
        EnterCriticalSection(&queueLock_);
        stop_ = true;
        LeaveCriticalSection(&queueLock_);

        // release semaphore to wake all threads
        for (size_t i = 0; i < workers_.size(); ++i)
        {
            ReleaseSemaphore(taskSemaphore_, 1, NULL);
        }

        // wait for worker threads to exit
        for (HANDLE h : workers_)
        {
            if (h)
            {
                WaitForSingleObject(h, INFINITE);
                CloseHandle(h);
            }
        }
        workers_.clear();

        if (taskSemaphore_)
        {
            CloseHandle(taskSemaphore_);
            taskSemaphore_ = NULL;
        }

        DeleteCriticalSection(&queueLock_);
    }

    // enqueue a task returning a future
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            EnterCriticalSection(&queueLock_);
            if (stop_)
            {
                LeaveCriticalSection(&queueLock_);
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            tasks_.emplace([task]() { (*task)(); });
            LeaveCriticalSection(&queueLock_);
        }

        // signal one worker thread
        ReleaseSemaphore(taskSemaphore_, 1, NULL);
        return res;
    }

private:
    static unsigned __stdcall WorkerProc(void* param)
    {
        ThreadPool* pool = reinterpret_cast<ThreadPool*>(param);
        pool->RunWorker();
        return 0;
    }

    void RunWorker()
    {
        for (;;)
        {
            // wait for a task or stop signal
            DWORD waitRes = WaitForSingleObject(taskSemaphore_, INFINITE);
            if (waitRes != WAIT_OBJECT_0)
                continue;

            std::function<void()> task;
            EnterCriticalSection(&queueLock_);
            if (!tasks_.empty())
            {
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            bool stopping = stop_ && tasks_.empty();
            LeaveCriticalSection(&queueLock_);

            if (task)
            {
                try { task(); }
                catch (...) { /* swallow exceptions to keep worker alive */ }
            }

            if (stopping)
                break;
        }
    }

private:
    std::vector<HANDLE> workers_;
    std::queue<std::function<void()>> tasks_;

    CRITICAL_SECTION queueLock_;
    HANDLE taskSemaphore_;
    bool stop_;
    size_t threads_;
};
