#include "pch.h"
#include "EventBus.h"
#include "EventMessages.h"
#include <concrt.h>

using namespace spark::ufspdt;

EventBus& EventBus::Instance()
{
	static EventBus s;
	return s;
}

void EventBus::Publish(HWND target, const ProgressEvent& evt)
{
	if (!target) return;
	std::lock_guard<std::mutex> lk(mutex_);
	auto& entry = map_[target];
	int p = evt.portIndex;
	if (p < 0 || p >= (int)entry.slots.size()) return;
	entry.slots[p].evt = evt;
	entry.slots[p].dirty.store(true);
	// Immediately notify UI for every publish (no throttling)
	bool expected = false;
	if (entry.pending.compare_exchange_strong(expected, true))
	{
		::PostMessage(target, MSG_WM_TASK_PROGRESS, 0, 0);
	}
}

std::vector<ProgressEvent> EventBus::ConsumeAll(HWND target)
{
	std::vector<ProgressEvent> out;
	if (!target) return out;
	std::lock_guard<std::mutex> lk(mutex_);
	auto it = map_.find(target);
	if (it == map_.end()) return out;
	auto& entry = it->second;
	for (int i = 0; i < (int)entry.slots.size(); ++i)
	{
		if (entry.slots[i].dirty.load())
		{
			out.push_back(entry.slots[i].evt);
			entry.slots[i].dirty.store(false);
		}
	}
	// reset pending flag
	entry.pending.store(false);
	return out;
}

void EventBus::Unregister(HWND target)
{
	if (!target) return;
	std::lock_guard<std::mutex> lk(mutex_);
	map_.erase(target);
}
