#include "pch.h"
#include "EventBus.h"
#include "EventMessages.h"
#include <concrt.h>
#include <chrono>
#include <thread>

using namespace spark::ufspdt;

EventBus& EventBus::Instance()
{
	static EventBus s;
	return s;
}

void EventBus::PublishUI(HWND target, const UIEvent& uiEvt)
{
	if (!target) return;
	std::shared_ptr<EventBus::QueueEntry> entryPtr;
	{
		std::lock_guard<std::mutex> lk(mutex_);
		auto it = map_.find(target);
		if (it == map_.end())
		{
			entryPtr = std::make_shared<QueueEntry>();
			map_.emplace(target, entryPtr);
		}
		else
		{
			entryPtr = it->second;
		}
	}

	int p = uiEvt.portIndex;
	if (p < 0 || p >= (int)entryPtr->slots.size())
	{
		// default to slot 0 for global commands
		p = 0;
	}

	// Protect slot update with per-entry lock to avoid races with ConsumeAllUI
	{
		std::lock_guard<std::mutex> lk(entryPtr->lock);
		auto &slot = entryPtr->slots[p];
		slot.uiEvt = uiEvt;
		slot.uiDirty.store(true, std::memory_order_release);
	}

	// Notify UI thread immediately for UI events
	entryPtr->pending.store(true);
	entryPtr->lastNotifyMs.store(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(), std::memory_order_relaxed);
	::PostMessage(target, MSG_WM_TASK_PROGRESS, 0, 0);
}

EventBus::EventBus()
{
	stopNotifier_.store(false);
	notifierThread_ = std::thread([this]() {
		using clock = std::chrono::steady_clock;
		while (!stopNotifier_.load())
		{
			long long nextWakeMs = 0;
			std::vector<std::pair<HWND, std::shared_ptr<QueueEntry>>> toNotify;

			// Collect entries that need notify and compute earliest nextNotifyMs
			{
				std::lock_guard<std::mutex> lk(mutex_);
				auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now().time_since_epoch()).count();
				for (auto &kv : map_)
				{
					auto hwnd = kv.first;
					auto entry = kv.second;
					long long nextMs = entry->nextNotifyMs.load(std::memory_order_acquire);
					if (nextMs == 0)
					{
						continue;
					}
					if (nextMs <= nowMs)
					{
						toNotify.emplace_back(hwnd, entry);
					}
					else
					{
						if (nextWakeMs == 0 || nextMs < nextWakeMs) nextWakeMs = nextMs;
					}
				}
			}

			// Post messages for ready entries
			for (auto &p : toNotify)
			{
				auto hwnd = p.first;
				auto entry = p.second;
				// reset nextNotifyMs before posting
				entry->nextNotifyMs.store(0, std::memory_order_release);
				entry->lastNotifyMs.store(std::chrono::duration_cast<std::chrono::milliseconds>(clock::now().time_since_epoch()).count(), std::memory_order_relaxed);
				::PostMessage(hwnd, MSG_WM_TASK_PROGRESS, 0, 0);
			}

			// Wait until nextWakeMs or notified
			if (stopNotifier_.load()) break;
			if (nextWakeMs == 0)
			{
				// No scheduled timers, wait until someone schedules
				std::unique_lock<std::mutex> lk(notifierCvMutex_);
				notifierCv_.wait_for(lk, std::chrono::milliseconds(200));
			}
			else
			{
				auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now().time_since_epoch()).count();
				long long waitMs = nextWakeMs > nowMs ? (nextWakeMs - nowMs) : 0;
				std::unique_lock<std::mutex> lk(notifierCvMutex_);
				notifierCv_.wait_for(lk, std::chrono::milliseconds(waitMs));
			}
		}
	});
}

EventBus::~EventBus()
{
	stopNotifier_.store(true);
	notifierCv_.notify_all();
	if (notifierThread_.joinable()) notifierThread_.join();
}

void EventBus::Publish(HWND target, const ProgressEvent& evt)
{
	if (!target) return;
	std::shared_ptr<EventBus::QueueEntry> entryPtr;
	{
		std::lock_guard<std::mutex> lk(mutex_);
		auto it = map_.find(target);
		if (it == map_.end())
		{
			entryPtr = std::make_shared<QueueEntry>();
			map_.emplace(target, entryPtr);
		}
		else
		{
			entryPtr = it->second;
		}
	}

	// Backward compatibility: if caller didn't set type, infer from progress value
	auto evtCopy = evt;
	if (evtCopy.type != spark::ufspdt::ProgressEvent::EventType::Progress && evtCopy.type != spark::ufspdt::ProgressEvent::EventType::StatusOnly && evtCopy.type != spark::ufspdt::ProgressEvent::EventType::Final)
	{
		// infer
		if (evtCopy.progress >= 0) evtCopy.type = spark::ufspdt::ProgressEvent::EventType::Progress;
		else evtCopy.type = spark::ufspdt::ProgressEvent::EventType::StatusOnly;
	}
	int p = evtCopy.portIndex;
	if (p < 0 || p >= (int)entryPtr->slots.size()) return;

	// Protect slot update with per-entry lock to avoid races with ConsumeAll
	{
		std::lock_guard<std::mutex> lk(entryPtr->lock);
		auto &slot = entryPtr->slots[p];
		// If caller provided explicit type use it; otherwise keep compatibility
		// with old callers that used negative progress as status-only.
		auto type = evtCopy.type;
		if (type == ProgressEvent::EventType::Progress)
		{
			slot.evt = evt;
		}
		else if (type == ProgressEvent::EventType::StatusOnly)
		{
			slot.evt.status = evt.status;
			slot.evt.result = evt.result;
			// leave slot.evt.progress untouched
		}
		else
		{
			// Final or unknown types: fully replace
			slot.evt = evt;
		}
		slot.dirty.store(true, std::memory_order_release);
	}

	// Notify UI thread. If this update includes a numeric progress value
	// (progress >= 0) we ensure the UI is notified immediately; otherwise
	// use throttling/aggregation for status-only updates.
	bool expected = false;
	bool mustNotifyNow = (evt.progress >= 0);
	if (mustNotifyNow)
	{
		// mark pending and post immediately
		entryPtr->pending.store(true);
		entryPtr->lastNotifyMs.store(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(), std::memory_order_relaxed);
		::PostMessage(target, MSG_WM_TASK_PROGRESS, 0, 0);
	}
	else if (entryPtr->pending.compare_exchange_strong(expected, true))
	{
		// Throttle rapid notifications to avoid overwhelming UI.
		constexpr long long THROTTLE_MS = 50; // milliseconds
		using clock = std::chrono::steady_clock;
		auto nowTp = clock::now();
		auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(nowTp.time_since_epoch()).count();
		long long last = entryPtr->lastNotifyMs.load(std::memory_order_relaxed);
		if (last == 0 || (nowMs - last) >= THROTTLE_MS)
		{
			entryPtr->lastNotifyMs.store(nowMs, std::memory_order_relaxed);
			::PostMessage(target, MSG_WM_TASK_PROGRESS, 0, 0);
		}
		else
		{
			// schedule a delayed notification via the centralized notifier
			long long waitMs = THROTTLE_MS - (nowMs - last);
			long long scheduledMs = nowMs + (waitMs > 0 ? waitMs : 0);
			long long prev = entryPtr->nextNotifyMs.exchange(scheduledMs, std::memory_order_release);
			// If there was an earlier scheduled time, keep the earlier one (so we don't postpone)
			if (prev != 0 && prev < scheduledMs)
			{
				// restore earlier time
				entryPtr->nextNotifyMs.store(prev, std::memory_order_release);
			}
			// wake the notifier thread to re-evaluate next wake time
			notifierCv_.notify_all();
		}
	}
}

std::vector<ProgressEvent> EventBus::ConsumeAll(HWND target)
{
	std::vector<ProgressEvent> out;
	if (!target) return out;
	std::shared_ptr<EventBus::QueueEntry> entryPtr;
	{
		std::lock_guard<std::mutex> lk(mutex_);
		auto it = map_.find(target);
		if (it == map_.end()) return out;
		entryPtr = it->second;
	}

	// Lock the entry while reading and clearing dirty flags
	{
		std::lock_guard<std::mutex> lk(entryPtr->lock);
		for (int i = 0; i < (int)entryPtr->slots.size(); ++i)
		{
			if (entryPtr->slots[i].dirty.load(std::memory_order_acquire))
			{
				out.push_back(entryPtr->slots[i].evt);
				entryPtr->slots[i].dirty.store(false, std::memory_order_release);
			}
		}
		// reset pending flag
		entryPtr->pending.store(false);
	}
	return out;
}

std::vector<UIEvent> EventBus::ConsumeAllUI(HWND target)
{
	std::vector<UIEvent> out;
	if (!target) return out;
	std::shared_ptr<EventBus::QueueEntry> entryPtr;
	{
		std::lock_guard<std::mutex> lk(mutex_);
		auto it = map_.find(target);
		if (it == map_.end()) return out;
		entryPtr = it->second;
	}

	// Lock the entry while reading and clearing uiDirty flags
	{
		std::lock_guard<std::mutex> lk(entryPtr->lock);
		for (int i = 0; i < (int)entryPtr->slots.size(); ++i)
		{
			if (entryPtr->slots[i].uiDirty.load(std::memory_order_acquire))
			{
				out.push_back(entryPtr->slots[i].uiEvt);
				entryPtr->slots[i].uiDirty.store(false, std::memory_order_release);
			}
		}
		// reset pending flag
		entryPtr->pending.store(false);
	}
	return out;
}

void EventBus::Unregister(HWND target)
{
	if (!target) return;
	std::lock_guard<std::mutex> lk(mutex_);
	map_.erase(target);
}

