// copyright (c) microsoft. all rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#include "pch.h"
#include "GpuTimeManager.h"
#include "GraphicsCore.h"
#include "CommandContext.h"
#include "CommandListManager.h"

namespace
{
    ID3D11Query* sm_DisjointQuery = nullptr;
    std::vector<ID3D11Query*> sm_QueryHeap;
    std::vector<uint64_t> sm_TimeStampBuffer;
    uint64_t sm_Fence = 0;
    uint32_t sm_MaxNumTimers = 0;
    uint32_t sm_NumTimers = 0;
    uint64_t sm_ValidTimeStart = 0;
    uint64_t sm_ValidTimeEnd = 0;
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT sm_Disjoint = {};
    double sm_GpuTickDelta = 0.0;

    std::vector<std::pair<uint32_t, std::wstring>> sm_TimerNames;
}

void GpuTimeManager::Initialize(uint32_t MaxNumTimers)
{
    D3D11_QUERY_DESC QueryDesc;
    QueryDesc.MiscFlags = 0;
    QueryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    ASSERT_SUCCEEDED( Graphics::g_Device->CreateQuery( &QueryDesc, &sm_DisjointQuery ) );
    SetName( sm_DisjointQuery, L"Disjoint-Query" );

    QueryDesc.Query = D3D11_QUERY_TIMESTAMP;
    // Generate each time stamp (pair)
    sm_QueryHeap.resize( MaxNumTimers * 2 );
    sm_TimeStampBuffer.resize( MaxNumTimers * 2 );
    for (auto& query : sm_QueryHeap) {
        ASSERT_SUCCEEDED( Graphics::g_Device->CreateQuery( &QueryDesc, &query ) );
    }

    sm_MaxNumTimers = (uint32_t)MaxNumTimers;
}

void GpuTimeManager::Shutdown()
{
    if (sm_DisjointQuery != nullptr) {
        sm_DisjointQuery->Release();
        sm_DisjointQuery = nullptr;
    }

    for (auto& query : sm_QueryHeap)
        query->Release();
    sm_QueryHeap.clear();
}

uint32_t GpuTimeManager::NewTimer(void)
{
    //
    // TODO: Thread safe
    //
    return sm_NumTimers++;
}

void GpuTimeManager::StartTimer(CommandContext& Context, uint32_t TimerIdx)
{
    Context.InsertTimeStamp(sm_QueryHeap[TimerIdx * 2]);
}

void GpuTimeManager::StopTimer(CommandContext& Context, uint32_t TimerIdx)
{
    Context.InsertTimeStamp(sm_QueryHeap[TimerIdx * 2 + 1]);
}

void GpuTimeManager::Begin( void )
{
    CommandContext& Context = CommandContext::Begin( kGraphicsContext );
    Context.BeginQuery(sm_DisjointQuery);
    Context.InsertTimeStamp(sm_QueryHeap[0]);
    Context.Finish();
}

void GpuTimeManager::End( void )
{
    CommandContext& Context = CommandContext::Begin( kGraphicsContext );
    Context.InsertTimeStamp(sm_QueryHeap[1]);
    Context.EndQuery(sm_DisjointQuery);
    sm_Fence = Context.Finish();
}

void GpuTimeManager::ResolveTimes(void)
{
    CommandContext::ResolveTimeStamps(sm_DisjointQuery, sm_QueryHeap.data(), sm_NumTimers * 2, &sm_Disjoint, sm_TimeStampBuffer.data());

    sm_GpuTickDelta = 1.0 / static_cast<double>(sm_Disjoint.Frequency);

    sm_ValidTimeStart = sm_TimeStampBuffer[0];
    sm_ValidTimeEnd = sm_TimeStampBuffer[1];

    // On the first frame, with random values in the timestamp query heap, we can avoid a misstart.
    if (sm_ValidTimeEnd < sm_ValidTimeStart)
    {
        sm_ValidTimeStart = 0ull;
        sm_ValidTimeEnd = 0ull;
    }
}

float GpuTimeManager::GetTime(uint32_t TimerIdx)
{
    ASSERT(TimerIdx < sm_NumTimers, "Invalid GPU timer index");

    uint64_t TimeStamp1 = sm_TimeStampBuffer[TimerIdx * 2];
    uint64_t TimeStamp2 = sm_TimeStampBuffer[TimerIdx * 2 + 1];

    if (TimeStamp1 < sm_ValidTimeStart || TimeStamp2 > sm_ValidTimeEnd || TimeStamp2 <= TimeStamp1 )
        return 0.0f;

    return static_cast<float>(sm_GpuTickDelta * (TimeStamp2 - TimeStamp1));
}

void GpuTimeManager::Register(uint32_t TimerIdx, std::wstring Name)
{
    sm_TimerNames.push_back( { TimerIdx, Name });
}
