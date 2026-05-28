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

private:
	EventBus() = default;
	static constexpr int MAX_PORTS = 16; // must match UI_THREAD_COUNT

	struct SlotEntry {
		ProgressEvent evt;
		std::atomic_bool dirty{false};
	};

	struct QueueEntry {
		std::array<SlotEntry, MAX_PORTS> slots;
		std::atomic_bool pending{false};

	};

	std::mutex mutex_;
	std::unordered_map<HWND, QueueEntry> map_;
};

}} // namespace
