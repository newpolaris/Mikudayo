#include "stdafx.h"
#include "Skydome.h"
#include "Scene.h"
#include "RenderPass.h"

namespace Skydome
{
    RenderPass m_SkydomePass( kRenderQueueSkydome );
}

void Skydome::Initialize( void )
{
}

void Skydome::Render( ScenePtr& scene, RenderArgs& args )
{
    scene->Render( m_SkydomePass, args );
}
