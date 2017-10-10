#pragma once

#include "SceneNode.h"

class Model;
class Visitor;
class PmxInstant : public SceneNode
{
public:

    PmxInstant( Model& model );
    bool LoadModel();
    bool LoadMotion( const std::wstring& motionPath );
    virtual void Accept( Visitor& visitor ) override;
    virtual void Render( GraphicsContext& Context, Visitor& visitor ) override;
    virtual void RenderBone( GraphicsContext& Context, Visitor& visitor ) override;
    virtual void Update( float deltaT ) override;

protected:

    struct Context;
    std::shared_ptr<Context> m_Context;
};
