#pragma once
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

namespace spark { namespace ufspdt {

struct ProgressEvent {
	int portIndex;
	int progress;
	int result;
	std::string status; // UTF-8
};

class EventBus
{
public:
	static EventBus& Instance();

	// publish an event destined to target HWND; will PostMessage to wake UI thread
	void Publish(HWND target, const ProgressEvent& evt);

	// consume all pending events for target (called on UI thread)
	std::vector<ProgressEvent> ConsumeAll(HWND target);

	// Unregister a target window (cleanup internal state). Call from UI on destroy.
	void Unregister(HWND target);

	~EventBus();

private:
	EventBus();
	static constexpr int MAX_PORTS = 16; // must match UI_THREAD_COUNT

	struct SlotEntry {
		ProgressEvent evt;
		std::atomic_bool dirty{false};
	};

	struct QueueEntry {
		std::array<SlotEntry, MAX_PORTS> slots;
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
