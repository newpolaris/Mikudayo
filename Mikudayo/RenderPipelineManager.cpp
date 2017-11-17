#include "stdafx.h"
#include "RenderPipelineManager.h"
#include "RenderType.h"

namespace
{
    std::map<std::wstring, RenderPipelineList> Techniques;
}

RenderPipelineManager::RenderPipelineManager()
{
}

void RenderPipelineManager::Initialize()
{
}

void RenderPipelineManager::Shutdown()
{
    Techniques.clear();
}