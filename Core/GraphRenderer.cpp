#include "pch.h"
#include "Color.h"
#include "GraphRenderer.h"

#pragma warning(disable : 4100)

void GraphRenderer::Initialize()
{
}

void GraphRenderer::Shutdown()
{
}

bool GraphRenderer::ManageGraphs( GraphHandle graphID, GraphType Type )
{
	return false;
}

Color GraphRenderer::GetGraphColor( GraphHandle GraphID, GraphType Type )
{
	return Color();
}

void GraphRenderer::Update( DirectX::XMFLOAT2 InputNode, GraphHandle GraphID, GraphType Type )
{
}

void GraphRenderer::RenderGraphs( GraphicsContext & Context, GraphType Type )
{
}
