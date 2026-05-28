#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "CImpState.h"

// Lightweight task context passed through stages
struct TaskContext {
	int portIndex = -1;
	CImpState* state = nullptr;
	IUiNotifier* notifier = nullptr;
	pdt_log_config_t* lg = nullptr;
};

// Stage interface: execute using the TaskContext. Return error code (0 == success).
struct IStage {
	virtual ~IStage() = default;
	virtual int Execute(TaskContext& ctx) = 0;
	virtual const std::string& Name() const = 0;
};

// Simple function-backed stage adapter
class FunctionStage : public IStage {
public:
	explicit FunctionStage(std::string name, std::function<int(TaskContext&)> fn)
		: name_(std::move(name)), fn_(std::move(fn)) {}

	int Execute(TaskContext& ctx) override { return fn_(ctx); }
	const std::string& Name() const override { return name_; }

private:
	std::string name_;
	std::function<int(TaskContext&)> fn_;
};

using StageList = std::vector<std::unique_ptr<IStage>>;

// More structured stage examples. These are thin wrappers but give a clear
// semantic class for important stages and accept an executor callable so
// they remain testable.
class RebootStage : public IStage {
public:
	explicit RebootStage(std::function<int(TaskContext&)> exec)
		: exec_(std::move(exec)), name_("Rebooting") {}
	int Execute(TaskContext& ctx) override { return exec_(ctx); }
	const std::string& Name() const override { return name_; }
private:
	std::function<int(TaskContext&)> exec_;
	std::string name_;
};

class PowerOffStage : public IStage {
public:
	explicit PowerOffStage(std::function<int(TaskContext&)> exec)
		: exec_(std::move(exec)), name_("PowerOff") {}
	int Execute(TaskContext& ctx) override { return exec_(ctx); }
	const std::string& Name() const override { return name_; }
private:
	std::function<int(TaskContext&)> exec_;
	std::string name_;
};
