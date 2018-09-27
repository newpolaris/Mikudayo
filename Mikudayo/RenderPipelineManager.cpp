#include "stdafx.h"
#include "RenderPipelineManager.h"
#include "RenderType.h"

namespace
{
    std::map<std::wstring, RenderPipelineList> g_Techniques;
}

RenderPipelineManager::RenderPipelineManager()
{
}

void RenderPipelineManager::Initialize()
{
}

void RenderPipelineManager::Shutdown()
{
    g_Techniques.clear();
}