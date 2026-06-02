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
// RebootStage and PowerOffStage were removed; use GenericState(Function) instead.

// --- Context+State framework (PrefStartContext + IState) ---
// IState provides lifecycle hooks similar to the screenshot pattern.
struct IState {
	virtual ~IState() = default;
	virtual void Enter(TaskContext& ctx) {}
	// Execute returns error code (0 == success)
	virtual int Execute(TaskContext& ctx) = 0;
	virtual void Exit(TaskContext& ctx) {}
};

// Generic state that wraps a callable for Execute; Enter/Exit are no-ops by default
class GenericState : public IState {
public:
	explicit GenericState(std::string name, std::function<int(TaskContext&)> fn)
		: name_(std::move(name)), fn_(std::move(fn)) {}
	int Execute(TaskContext& ctx) override { return fn_(ctx); }
	const std::string& Name() const { return name_; }
private:
	std::string name_;
	std::function<int(TaskContext&)> fn_;
};


// PrefStartContext holds and executes a sequence of IState objects in order.
class PrefStartContext {
public:
	PrefStartContext() = default;
	~PrefStartContext() = default;

	// take ownership of raw pointer
	void AddState(IState* st) { states_.emplace_back(st); }

	// Execute all states in order; call Enter/Execute/Exit for each. Returns last error code.
	int Exec(TaskContext& ctx)
	{
		int lastRet = ERROR_SUCCESS;
		for (auto& s : states_)
		{
			s->Enter(ctx);
			lastRet = s->Execute(ctx);
			s->Exit(ctx);
			if (lastRet != ERROR_SUCCESS) break;
		}
		return lastRet;
	}

private:
	std::vector<std::unique_ptr<IState>> states_;
};
