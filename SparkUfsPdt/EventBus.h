#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <windows.h>
#include <atomic>
#include <array>
#include <cstdint>
#include <memory>
#include <thread>
#include <condition_variable>
#include "EventMessages.h"

namespace spark { namespace ufspdt {

struct ProgressEvent {
	int portIndex;
	enum class EventType : int { Progress = 0, StatusOnly = 1, Final = 2 };
	EventType type = EventType::Progress;
	int progress;
	int result;
	std::string status; // UTF-8
};

struct UIEvent;

class EventBus
{
public:
	static EventBus& Instance();

	// publish an event destined to target HWND; will PostMessage to wake UI thread
	void Publish(HWND target, const ProgressEvent& evt);
	void PublishUI(HWND target, const UIEvent& uiEvt);

	// consume all pending events for target (called on UI thread)
	std::vector<ProgressEvent> ConsumeAll(HWND target);
	// consume UI events queued for target
	std::vector<UIEvent> ConsumeAllUI(HWND target);

	// Unregister a target window (cleanup internal state). Call from UI on destroy.
	void Unregister(HWND target);

	~EventBus();

private:
	EventBus();
	static constexpr int MAX_PORTS = 16; // must match UI_THREAD_COUNT

	struct SlotEntry {
		ProgressEvent evt;
		UIEvent uiEvt;
		std::atomic_bool dirty{false};
		std::atomic_bool uiDirty{false};
	};

	struct QueueEntry {
		std::array<SlotEntry, MAX_PORTS> slots;
		// UI events must be queued (not single-slot overwritten), otherwise
		// rapid commands on the same port can drop earlier UI intents.
		std::vector<UIEvent> uiQueue;
		// Per-entry mutex to protect slot-level updates while avoiding
		// holding the global map mutex for long periods.
		std::mutex lock;
		// Pending flag indicates there's unread data for this target.
		std::atomic_bool pending{false};
		// Throttling support: timestamp of last notification (ms since steady_clock epoch)
		std::atomic<long long> lastNotifyMs{0};
		// Next scheduled notify timestamp (0 if none)
		std::atomic<long long> nextNotifyMs{0};
	};

	// Protects the map structure for lookup/insert/erase only. Actual
	// slot updates are protected by the per-entry mutex.
	std::mutex mutex_;
	std::unordered_map<HWND, std::shared_ptr<QueueEntry>> map_;

	// Background notifier to implement centralized throttling and ensure
	// only last-frame notifications are posted when bursts occur.
	std::thread notifierThread_;
	std::atomic_bool stopNotifier_{false};
	std::condition_variable_any notifierCv_;
	std::mutex notifierCvMutex_;
};

}} // namespace
