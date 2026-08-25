#pragma once
#include <windows.h>

// ----------------------------------------------------------------------------
// TaskCountPolicy - Pure decision functions for when the global PASS/FAIL
// counters should be incremented. Extracted as pure functions so they are
// unit testable without bringing in the heavy MFC/DialogAdapter stack.
//
// Fixes Issue #1:
//   * P0-2 : IncrementPassCount was NEVER fired (m_passCount always 0)
//   * P1-4 : IncrementFailCount was fired from every stage-level PostTaskStatus
//            call, causing 1 + N increments for a single failing task.
//
// Design rule: each task counts EXACTLY ONCE as PASS or FAIL.
//   - PASS  -> DialogAdapter::PostTaskProgress(progress>=100, result=0)
//              emitted by RunFtTaskImpl/RunQcTaskImpl's final "Success" branch.
//   - FAIL  -> CSparkUfsPdtDlg::RunPdtTask wrapper observes inner RunXxxTaskImpl
//              return != 0 and fires the counter once, after pipeline exit.
// ----------------------------------------------------------------------------
namespace TaskCountPolicy {

	// True when the PostTaskProgress call means the task ended successfully and
	// the global PASS counter should be incremented.
	//   * progress must be the final marker (>=100) to rule out intermediate
	//     progress updates that also carry result==ERROR_SUCCESS.
	//   * result must be ERROR_SUCCESS to indicate no stage failed.
	inline bool ShouldIncrementPassOnProgress(int progress, int result)
	{
		return (progress >= 100) && (result == ERROR_SUCCESS);
	}

	// Always false. Stage-level PostTaskStatus notifications (e.g. SetSnStage
	// calling it when UfsSetSrialNumberString returns ERR_SN_*) are for UI
	// display only. The counter increment is deferred to the outermost wrapper
	// RunPdtTask so that the global fail count increases exactly once per task.
	inline bool ShouldIncrementFailOnStageStatus(int /*result*/)
	{
		return false;
	}

	// True when the outermost wrapper decides that the overall task return
	// value should bump the FAIL counter exactly once.
	inline bool ShouldIncrementFailOnTaskFinal(int finalResult)
	{
		return finalResult != ERROR_SUCCESS;
	}

}

