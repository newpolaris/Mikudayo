//
// Copyright (c) Microsoft. All rights reserved.
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
#include "SystemTime.h"
#include "GraphicsCore.h"
#include "TextRenderer.h"
#include "GraphRenderer.h"
#include "GameInput.h"
#include "GpuTimeManager.h"
#include "CommandContext.h"
#include <vector>
#include <unordered_map>
#include <array>

using namespace Graphics;
using namespace GraphRenderer;
using namespace Math;
using namespace std;

#define PERF_GRAPH_ERROR uint32_t(0xFFFFFFFF)

namespace EngineProfiling
{
    bool Paused = false;
}

class StatHistory
{
public:
    StatHistory()
    {
        for (uint32_t i = 0; i < kHistorySize; ++i)
            m_RecentHistory[i] = 0.0f;
        for (uint32_t i = 0; i < kExtendedHistorySize; ++i)
            m_ExtendedHistory[i] = 0.0f;
        m_Average = 0.0f;
        m_Minimum = 0.0f;
        m_Maximum = 0.0f;
    }

    void RecordStat( uint32_t FrameIndex, float Value )
    {
        m_RecentHistory[FrameIndex % kHistorySize] = Value;
        m_ExtendedHistory[FrameIndex % kExtendedHistorySize] = Value;
        m_Recent = Value;

        uint32_t ValidCount = 0;
        m_Minimum = FLT_MAX;
        m_Maximum = 0.0f;
        m_Average = 0.0f;

        for (float val : m_RecentHistory)
        {
            if (val > 0.0f)
            {
                ++ValidCount;
                m_Average += val;
                m_Minimum = min(val, m_Minimum);
                m_Maximum = max(val, m_Maximum);
            }
        }

        if (ValidCount > 0)
            m_Average /= (float)ValidCount;
        else
            m_Minimum = 0.0f;
    }

    float GetLast(void) const { return m_Recent; }
    float GetMax(void) const { return m_Maximum; }
    float GetMin(void) const { return m_Minimum; }
    float GetAvg(void) const { return m_Average; }

    const float* GetHistory(void) const { return m_ExtendedHistory; }
    uint32_t GetHistoryLength(void) const { return kExtendedHistorySize; }

private:
    static const uint32_t kHistorySize = 64;
    static const uint32_t kExtendedHistorySize = 256;
    float m_RecentHistory[kHistorySize];
    float m_ExtendedHistory[kExtendedHistorySize];
    float m_Recent;
    float m_Average;
    float m_Minimum;
    float m_Maximum;
};

class StatPlot
{
public:
    StatPlot(StatHistory& Data, Color Col = Color(1.0f, 1.0f, 1.0f))
        : m_StatData(Data), m_PlotColor(Col)
    {
    }

    void SetColor( Color Col )
    {
        m_PlotColor = Col;
    }

private:
    StatHistory& m_StatData;
    Color m_PlotColor;
};

class StatGraph
{
public:
    StatGraph(const wstring& Label, D3D11_RECT Window)
        : m_Label(Label), m_Window(Window), m_BGColor(0.0f, 0.0f, 0.0f, 0.2f)
    {
    }

    void SetLabel(const wstring& Label)
    {
        m_Label = Label;
    }

    void SetWindow(D3D11_RECT Window)
    {
        m_Window = Window;
    }

    uint32_t AddPlot( const StatPlot& P )
    {
        uint32_t Idx = (uint32_t)m_Stats.size();
        m_Stats.push_back(P);
        return Idx;
    }

    StatPlot& GetPlot( uint32_t Handle );

    void Draw( GraphicsContext& Context );

private:
    wstring m_Label;
    D3D11_RECT m_Window;
    vector<StatPlot> m_Stats;
    Color m_BGColor;
    float m_PeakValue;
};

class GraphManager
{
public:

private:
    vector<StatGraph> m_Graphs;
};

class GpuTimer
{
public:

    GpuTimer::GpuTimer( const std::wstring& Name )
    {
        m_TimerIndex = GpuTimeManager::NewTimer();
        GpuTimeManager::Register( m_TimerIndex, Name );
    }

    void Start(CommandContext& Context)
    {
        GpuTimeManager::StartTimer(Context, m_TimerIndex);
    }

    void Stop(CommandContext& Context)
    {
        GpuTimeManager::StopTimer(Context, m_TimerIndex);
    }

    float GpuTimer::GetTime(void)
    {
        return GpuTimeManager::GetTime(m_TimerIndex);
    }

    uint32_t GetTimerIndex(void)
    {
        return m_TimerIndex;
    }
private:

    uint32_t m_TimerIndex;
};

class NestedTimingTree
{
public:
    static const int sm_kProfileMaxThreadCount = 64;
    static const int sm_kMainThread = 0;

    NestedTimingTree( const wstring& name, NestedTimingTree* parent = nullptr )
        : m_Name(name), m_Parent(parent), m_IsExpanded(false), m_IsGraphed(false), m_GraphHandle(PERF_GRAPH_ERROR), m_GpuTimer(name) {}

    NestedTimingTree* GetChild( const wstring& name )
    {
        auto iter = m_LUT.find(name);
        if (iter != m_LUT.end())
            return iter->second;

        NestedTimingTree* node = new NestedTimingTree(name, this);
        m_Children.push_back(node);
        m_LUT[name] = node;
        return node;
    }

    NestedTimingTree* NextScope( void )
    {
        if (m_IsExpanded && m_Children.size() > 0)
            return m_Children[0];

        return m_Parent->NextChild(this);
    }

    NestedTimingTree* PrevScope( void )
    {
        NestedTimingTree* prev = m_Parent->PrevChild(this);
        return prev == m_Parent ? prev : prev->LastChild();
    }

    NestedTimingTree* FirstChild( void )
    {
        return m_Children.size() == 0 ? nullptr : m_Children[0];
    }

    NestedTimingTree* LastChild( void )
    {
        if (!m_IsExpanded || m_Children.size() == 0)
            return this;

        return m_Children.back()->LastChild();
    }

    NestedTimingTree* NextChild( NestedTimingTree* curChild )
    {
        ASSERT(curChild->m_Parent == this);

        for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter)
        {
            if (*iter == curChild)
            {
                auto nextChild = iter; ++nextChild;
                if (nextChild != m_Children.end())
                    return *nextChild;
            }
        }

        if (m_Parent != nullptr)
            return m_Parent->NextChild(this);
        else
            return &sm_RootScope[sm_kSelectedThread];
    }

    NestedTimingTree* PrevChild( NestedTimingTree* curChild )
    {
        ASSERT(curChild->m_Parent == this);

        if (*m_Children.begin() == curChild)
        {
            if (this == &sm_RootScope[sm_kSelectedThread])
                return sm_RootScope[sm_kSelectedThread].LastChild();
            else
                return this;
        }

        for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter)
        {
            if (*iter == curChild)
            {
                auto prevChild = iter; --prevChild;
                return *prevChild;
            }
        }

        ERROR("All attempts to find a previous timing sample failed");
        return nullptr;
    }

    void StartTiming( CommandContext* Context )
    {
        m_StartTick = SystemTime::GetCurrentTick();
        if (Context == nullptr)
            return;

        m_GpuTimer.Start(*Context);

        Context->PIXBeginEvent(m_Name.c_str());
    }

    void StopTiming( CommandContext* Context )
    {
        m_Calls++;
        m_EndTick = SystemTime::GetCurrentTick();

		// To handle multiple calls
        m_TotalTick += m_EndTick - m_StartTick;
        m_StartTick = 0, m_EndTick = 0;

        if (Context == nullptr)
            return;

        m_GpuTimer.Stop(*Context);

        Context->PIXEndEvent();
    }

    void GatherTimes(uint32_t FrameIndex, bool bGpuReady)
    {
        if (sm_SelectedScope == this)
        {
            GraphRenderer::SetSelectedIndex(m_GpuTimer.GetTimerIndex());
        }
        if (EngineProfiling::Paused)
        {
            for (auto node : m_Children)
                node->GatherTimes(FrameIndex, bGpuReady);
            return;
        }
        m_CpuTime.RecordStat(FrameIndex, 1000.0f * (float)SystemTime::TimeBetweenTicks(0, m_TotalTick));
        if (bGpuReady)
            m_GpuTime.RecordStat( FrameIndex, 1000.0f * m_GpuTimer.GetTime() );

        for (auto node : m_Children)
            node->GatherTimes(FrameIndex, bGpuReady);

        m_StartTick = 0;
        m_EndTick = 0;
        m_TotalTick = 0;
        m_Calls = 0;
    }

    void SumInclusiveTimes(float& cpuTime, float& gpuTime)
    {
        cpuTime = 0.0f;
        gpuTime = 0.0f;
        for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter)
        {
            cpuTime += (*iter)->m_CpuTime.GetLast();
            gpuTime += (*iter)->m_GpuTime.GetLast();
        }
    }

    static void PushProfilingMarker( const wstring& name, CommandContext* Context );
    static void PopProfilingMarker( CommandContext* Context );
    static void Update( void );
    static void UpdateTimes( int kThreadID )
    {
        ASSERT(kThreadID < sm_kProfileMaxThreadCount);

        uint32_t FrameIndex = (uint32_t)Graphics::GetFrameCount();

        bool bGpuReady = GpuTimeManager::ResolveTimes();

        sm_RootScope[kThreadID].GatherTimes(FrameIndex, bGpuReady);
        if (!bGpuReady) return;
        s_FrameDelta.RecordStat(FrameIndex, GpuTimeManager::GetTime(0));

        if (kThreadID == sm_kMainThread)
        {
            float TotalCpuTime, TotalGpuTime;
            sm_RootScope[kThreadID].SumInclusiveTimes(TotalCpuTime, TotalGpuTime);
            s_TotalCpuTime.RecordStat(FrameIndex, TotalCpuTime);
            s_TotalGpuTime.RecordStat(FrameIndex, TotalGpuTime);
        }
    }

    static float GetTotalCpuTime(void) { return s_TotalCpuTime.GetAvg(); }
    static float GetTotalGpuTime(void) { return s_TotalGpuTime.GetAvg(); }
    static float GetFrameDelta(void) { return s_FrameDelta.GetAvg(); }
    static void SetSelectedThread( int ThreadID ) { sm_kSelectedThread = ThreadID; }

    static void Display( TextContext& Text, float x )
    {
        float curX = Text.GetCursorX();
        Text.DrawString("  ");
        float indent = Text.GetCursorX() - curX;
        Text.SetCursorX(curX);
        sm_RootScope[sm_kSelectedThread].DisplayNode( Text, x - indent, indent );
        sm_RootScope[sm_kSelectedThread].StoreToGraph();
    }

    void Toggle()
    {
        //if (m_GraphHandle == PERF_GRAPH_ERROR)
        //	m_GraphHandle = GraphRenderer::InitGraph(GraphType::Profile);
        //m_IsGraphed = GraphRenderer::ManageGraphs(m_GraphHandle, GraphType::Profile);
    }
    bool IsGraphed(){ return m_IsGraphed;}

private:

    void DisplayNode( TextContext& Text, float x, float indent );
    void StoreToGraph(void);
    void DeleteChildren( void )
    {
        for (auto node : m_Children)
            delete node;
        m_Children.clear();
    }

    wstring m_Name;
    NestedTimingTree* m_Parent;
    vector<NestedTimingTree*> m_Children;
    unordered_map<wstring, NestedTimingTree*> m_LUT;
    int64_t m_StartTick;
    int64_t m_EndTick;
    int64_t m_Calls = 0;
    int64_t m_TotalTick = 0;
    StatHistory m_CpuTime;
    StatHistory m_GpuTime;
    bool m_IsExpanded;
    GpuTimer m_GpuTimer;
    bool m_IsGraphed;
    GraphHandle m_GraphHandle;
    static StatHistory s_TotalCpuTime;
    static StatHistory s_TotalGpuTime;
    static StatHistory s_FrameDelta;
    static NestedTimingTree sm_RootScope[sm_kProfileMaxThreadCount];
    static NestedTimingTree* sm_CurrentNode[sm_kProfileMaxThreadCount];
    static NestedTimingTree* sm_SelectedScope;
    static int sm_kSelectedThread;
    static bool sm_CursorOnGraph;
};

NumVar SelectedThread( "Application/Profile Thread", 0, 0, (float)NestedTimingTree::sm_kProfileMaxThreadCount, 1 );

StatHistory NestedTimingTree::s_TotalCpuTime;
StatHistory NestedTimingTree::s_TotalGpuTime;
StatHistory NestedTimingTree::s_FrameDelta;

//
// Support multithreaded profiling
//
NestedTimingTree NestedTimingTree::sm_RootScope[sm_kProfileMaxThreadCount] = {
    L"Root", L"Root", L"Root", L"Root", L"Root", L"Root", L"Root", L"Root",
    L"Root", L"Root", L"Root", L"Root", L"Root", L"Root", L"Root", L"Root",
    L"Root", L"Root", L"Root", L"Root", L"Root", L"Root", L"Root", L"Root",
    L"Root", L"Root", L"Root", L"Root", L"Root", L"Root", L"Root", L"Root",
    L"Root", L"Root", L"Root", L"Root", L"Root", L"Root", L"Root", L"Root",
    L"Root", L"Root", L"Root", L"Root", L"Root", L"Root", L"Root", L"Root",
    L"Root", L"Root", L"Root", L"Root", L"Root", L"Root", L"Root", L"Root",
    L"Root", L"Root", L"Root", L"Root", L"Root", L"Root", L"Root", L"Root",
};

NestedTimingTree* NestedTimingTree::sm_CurrentNode[sm_kProfileMaxThreadCount] = {
	&sm_RootScope[ 0],	&sm_RootScope[ 1],	&sm_RootScope[ 2],	&sm_RootScope[ 3],
	&sm_RootScope[ 4],	&sm_RootScope[ 5],	&sm_RootScope[ 6],	&sm_RootScope[ 7],
	&sm_RootScope[ 8],	&sm_RootScope[ 9],	&sm_RootScope[10],	&sm_RootScope[11],
	&sm_RootScope[12],	&sm_RootScope[13],	&sm_RootScope[14],	&sm_RootScope[15],
	&sm_RootScope[16],	&sm_RootScope[17],	&sm_RootScope[18],	&sm_RootScope[19],
	&sm_RootScope[20],	&sm_RootScope[21],	&sm_RootScope[22],	&sm_RootScope[23],
	&sm_RootScope[24],	&sm_RootScope[25],	&sm_RootScope[26],	&sm_RootScope[27],
	&sm_RootScope[28],	&sm_RootScope[29],	&sm_RootScope[30],	&sm_RootScope[31],
	&sm_RootScope[32],	&sm_RootScope[33],	&sm_RootScope[34],	&sm_RootScope[35],
	&sm_RootScope[36],	&sm_RootScope[37],	&sm_RootScope[38],	&sm_RootScope[39],
	&sm_RootScope[40],	&sm_RootScope[41],	&sm_RootScope[42],	&sm_RootScope[43],
	&sm_RootScope[44],	&sm_RootScope[45],	&sm_RootScope[46],	&sm_RootScope[47],
	&sm_RootScope[48],	&sm_RootScope[49],	&sm_RootScope[50],	&sm_RootScope[51],
	&sm_RootScope[52],	&sm_RootScope[53],	&sm_RootScope[54],	&sm_RootScope[55],
	&sm_RootScope[56],	&sm_RootScope[57],	&sm_RootScope[58],	&sm_RootScope[59],
	&sm_RootScope[60],	&sm_RootScope[61],	&sm_RootScope[62],	&sm_RootScope[63],
};

int NestedTimingTree::sm_kSelectedThread = NestedTimingTree::sm_kMainThread;
NestedTimingTree* NestedTimingTree::sm_SelectedScope = &sm_RootScope[sm_kMainThread];
bool NestedTimingTree::sm_CursorOnGraph = false;

namespace EngineProfiling
{
    BoolVar DrawFrameRate("Display Frame Rate", true);
    BoolVar DrawProfiler("Display Profiler", true);
    //BoolVar DrawPerfGraph("Display Performance Graph", false);
    const bool DrawPerfGraph = false;

    void Begin( void )
    {
        if (GameInput::IsFirstPressed( GameInput::kStartButton )
            || GameInput::IsFirstPressed( GameInput::kKey_space ))
        {
            Paused = !Paused;
        }
        NestedTimingTree::SetSelectedThread((int)SelectedThread);
        GpuTimeManager::Begin();
    }

    void End( void )
    {
        GpuTimeManager::End();

        for (int i = 0; i < NestedTimingTree::sm_kProfileMaxThreadCount; i++)
            NestedTimingTree::UpdateTimes( i );
    }

    void BeginBlock(const wstring& name, CommandContext* Context)
    {
        NestedTimingTree::PushProfilingMarker(name, Context);
    }

    void EndBlock(CommandContext* Context)
    {
        NestedTimingTree::PopProfilingMarker(Context);
    }

    bool IsPaused()
    {
        return Paused;
    }

    void DisplayFrameRate( TextContext& Text )
    {
        if (!DrawFrameRate)
            return;

        float cpuTime = NestedTimingTree::GetTotalCpuTime();
        float gpuTime = NestedTimingTree::GetTotalGpuTime();
        float frameRate = 1.0f / NestedTimingTree::GetFrameDelta();

        Text.DrawFormattedString( "CPU %7.3f ms, GPU %7.3f ms, %3u Hz\n",
            cpuTime, gpuTime, (uint32_t)(frameRate + 0.5f));
    }

    void DisplayPerfGraph( GraphicsContext & Text )
    {
    }

    void Display( TextContext& Text, float x, float y, float /*w*/, float /*h*/ )
    {
        Text.ResetCursor(x, y);

        if (DrawProfiler)
        {
            //Text.GetCommandContext().SetScissor((uint32_t)Floor(x), (uint32_t)Floor(y), (uint32_t)Ceiling(w), (uint32_t)Ceiling(h));

            NestedTimingTree::Update();

            Text.SetColor( Color(0.5f, 1.0f, 1.0f) );
            Text.DrawString("Engine Profiling");
            Text.SetColor(Color(0.8f, 0.8f, 0.8f));
            Text.SetTextSize(20.0f);
            Text.DrawString("           CPU    GPU");
            Text.SetTextSize(24.0f);
            Text.NewLine();
            Text.SetTextSize(20.0f);
            Text.SetColor( Color(1.0f, 1.0f, 1.0f) );

            NestedTimingTree::Display( Text, x );
        }

        Text.GetCommandContext().SetScissor(0, 0, g_DisplayWidth, g_DisplayHeight);
    }

    //
    // Source from bullet's btQuickProf.cpp
    // Original boilerplate
    //
    /*
    ** profile.cpp
    **
    ** Real - Time Hierarchical Profiling for Game Programming Gems 3
    **
    ** by Greg Hjelstrom & Byon Garrabrant
    */
    unsigned int GetCurrentThreadIndex()
    {
        const unsigned int kNullIndex = ~0U;
#ifdef _WIN32
        __declspec(thread) static unsigned int sThreadIndex = kNullIndex;
#else
#ifdef __APPLE__
#if TARGET_OS_IPHONE
        unsigned int sThreadIndex = 0;
        return -1;
#else
        static __thread unsigned int sThreadIndex = kNullIndex;
#endif
#else//__APPLE__
#if __linux__
        static __thread unsigned int sThreadIndex = kNullIndex;
#else
        unsigned int sThreadIndex = 0;
        return -1;
#endif
#endif//__APPLE__

#endif
        static int gThreadCounter = 0;

        if (sThreadIndex == kNullIndex)
        {
            sThreadIndex = gThreadCounter++;
        }
        return sThreadIndex;
    }
} // EngineProfiling

void NestedTimingTree::PushProfilingMarker( const wstring& name, CommandContext* Context )
{
	int threadIndex = EngineProfiling::GetCurrentThreadIndex();
    if (threadIndex < 0 || threadIndex >= 64)
		return;

    ASSERT(sm_CurrentNode != nullptr);
    sm_CurrentNode[threadIndex] = sm_CurrentNode[threadIndex]->GetChild(name);
    sm_CurrentNode[threadIndex]->StartTiming(Context);
}

void NestedTimingTree::PopProfilingMarker( CommandContext* Context )
{
	int threadIndex = EngineProfiling::GetCurrentThreadIndex();
    if (threadIndex < 0 || threadIndex >= 64)
		return;

    ASSERT(sm_CurrentNode != nullptr);
    sm_CurrentNode[threadIndex]->StopTiming(Context);
    sm_CurrentNode[threadIndex] = sm_CurrentNode[threadIndex]->m_Parent;
}

void NestedTimingTree::Update( void )
{
    ASSERT(sm_SelectedScope != nullptr, "Corrupted profiling data structure");

    // Root is just arbitary tree holder, so move to first valid member (GUI)
    if (sm_SelectedScope == &sm_RootScope[sm_kSelectedThread])
    {
        sm_SelectedScope = sm_RootScope[sm_kSelectedThread].FirstChild();
        if (sm_SelectedScope == &sm_RootScope[sm_kSelectedThread])
            return;
    }

    if (GameInput::IsFirstPressed( GameInput::kDPadLeft )
        || GameInput::IsFirstPressed( GameInput::kKey_left ))
    {
        //if still on graphs go back to text
        if (sm_CursorOnGraph)
            sm_CursorOnGraph = !sm_CursorOnGraph;
        else
            sm_SelectedScope->m_IsExpanded = false;
    }
    else if (GameInput::IsFirstPressed( GameInput::kDPadRight )
        || GameInput::IsFirstPressed( GameInput::kKey_right ))
    {
        if (sm_SelectedScope->m_IsExpanded == true && !sm_CursorOnGraph)
            sm_CursorOnGraph = true;
        else
            sm_SelectedScope->m_IsExpanded = true;
        //if already expanded go over to graphs

    }
    else if (GameInput::IsFirstPressed( GameInput::kDPadDown )
        || GameInput::IsFirstPressed( GameInput::kKey_down ))
    {
        sm_SelectedScope = sm_SelectedScope ? sm_SelectedScope->NextScope() : nullptr;
    }
    else if (GameInput::IsFirstPressed( GameInput::kDPadUp )
        || GameInput::IsFirstPressed( GameInput::kKey_up ))
    {
        sm_SelectedScope = sm_SelectedScope ? sm_SelectedScope->PrevScope() : nullptr;
    }
    else if (GameInput::IsFirstPressed( GameInput::kAButton )
        || GameInput::IsFirstPressed( GameInput::kKey_return ))
    {
        sm_SelectedScope->Toggle();
    }
}

void NestedTimingTree::DisplayNode( TextContext& Text, float leftMargin, float indent )
{
    NestedTimingTree& DisplayTarget = sm_RootScope[sm_kSelectedThread];
    if (this == &DisplayTarget && DisplayTarget.FirstChild() != nullptr)
    {
        m_IsExpanded = true;
        DisplayTarget.FirstChild()->m_IsExpanded = true;
    }
    else
    {
        if (sm_SelectedScope == this && !sm_CursorOnGraph)
            Text.SetColor( Color(1.0f, 1.0f, 0.5f) );
        else
            Text.SetColor( Color(1.0f, 1.0f, 1.0f) );


        Text.SetLeftMargin(leftMargin);
        Text.SetCursorX(leftMargin);

        if (m_Children.size() == 0)
            Text.DrawString("  ");
        else if (m_IsExpanded)
            Text.DrawString("- ");
        else
            Text.DrawString("+ ");

        Text.DrawString(m_Name.c_str());
        Text.SetCursorX(leftMargin + 300.0f);
        Text.DrawFormattedString("%6.3f %6.3f   ", m_CpuTime.GetAvg(), m_GpuTime.GetAvg());

        if (IsGraphed())
        {
            Text.SetColor(GraphRenderer::GetGraphColor(m_GraphHandle, GraphType::Profile));
            Text.DrawString("  []\n");
        }
        else
            Text.DrawString("\n");
    }

    if (!m_IsExpanded)
        return;

    for (auto node : m_Children)
        node->DisplayNode(Text, leftMargin + indent, indent);
}

void NestedTimingTree::StoreToGraph(void)
{
    if (m_GraphHandle != PERF_GRAPH_ERROR)
        GraphRenderer::Update( XMFLOAT2(m_CpuTime.GetLast(), m_GpuTime.GetLast()), m_GraphHandle, GraphType::Profile);

    for (auto node : m_Children)
        node->StoreToGraph();
}

//
// To intergrate with library external profiling support
//
void PushProfilingMarker( const std::wstring& name, CommandContext* Context )
{
    NestedTimingTree::PushProfilingMarker( name, Context );
}

void PopProfilingMarker( CommandContext* Context )
{
    NestedTimingTree::PopProfilingMarker( Context );
}
