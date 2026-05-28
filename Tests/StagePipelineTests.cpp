#include "pch.h"
#include "StagePipeline.h"
#include <cassert>
#include <iostream>

// Simple smoke tests demonstrating RebootStage and PowerOffStage behavior
int main()
{
	TaskContext ctx;
	ctx.portIndex = 1;
	ctx.state = nullptr; // not used in these unit-tests
	ctx.notifier = nullptr;
	ctx.lg = nullptr;

	bool rebootCalled = false;
	RebootStage rs([&](TaskContext& c) -> int {
		rebootCalled = true;
		std::cout << "RebootStage executed for port " << c.portIndex << "\n";
		return 0;
	});

	bool powerOffCalled = false;
	PowerOffStage ps([&](TaskContext& c) -> int {
		powerOffCalled = true;
		std::cout << "PowerOffStage executed for port " << c.portIndex << "\n";
		return 0;
	});

	int r1 = rs.Execute(ctx);
	int r2 = ps.Execute(ctx);

	assert(r1 == 0);
	assert(r2 == 0);
	assert(rebootCalled);
	assert(powerOffCalled);

	std::cout << "StagePipelineTests passed\n";
	return 0;
}
