#include "stdafx.h"
#include "TaskManager.h"
#include <ppl.h>
#include <concrtrm.h>

namespace TaskManager
{
    uint32_t GetMaxNumThreads();
}

void TaskManager::Initialize()
{
    using namespace concurrency;
    if (CurrentScheduler::Id() != -1)
        CurrentScheduler::Detach();
    uint32_t numThread = GetMaxNumThreads();
    SchedulerPolicy policy;
    policy.SetConcurrencyLimits(numThread, numThread);
    CurrentScheduler::Create(policy);
}

void TaskManager::Shutdown()
{
}

uint32_t TaskManager::GetMaxNumThreads()
{
    return concurrency::GetProcessorCount();
}