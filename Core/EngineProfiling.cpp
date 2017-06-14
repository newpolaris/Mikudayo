#include "pch.h"
#include "EngineProfiling.h"

#pragma warning(disable : 4100)

void EngineProfiling::Update()
{
}

void EngineProfiling::BeginBlock( const std::wstring & name, CommandContext * Context )
{
}

void EngineProfiling::EndBlock( CommandContext * Context )
{
}

void EngineProfiling::DisplayFrameRate( TextContext & Text )
{
}

void EngineProfiling::DisplayPerfGraph( GraphicsContext & Text )
{
}

void EngineProfiling::Display( TextContext & Text, float x, float y, float w, float h )
{
}

bool EngineProfiling::IsPaused()
{
	return false;
}
