#pragma once

// Centralized custom window message definitions
static constexpr UINT MSG_WM_TASK_PROGRESS = (WM_USER + 0x65);
static constexpr UINT MSG_WM_GET_PORT_SN = (WM_USER + 0x70);

// UI command enumeration used for intent-driven UI updates posted via EventBus
namespace spark { namespace ufspdt {
	enum class UICommand {
		None = 0,
		DecrementActiveTasks,
		IncrementActiveTasks,
		UpdateStatusBar,
		SetScanButtonEnabled,
		IncrementPassCount,
		IncrementFailCount,
		// reserved for future commands
	};

	struct UIEvent {
		UICommand cmd = UICommand::None;
		int portIndex = -1;
		int value = 0; // generic integer payload
		std::string text; // optional text payload (UTF-8)
	};
} }
