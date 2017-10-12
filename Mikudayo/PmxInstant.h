#pragma once

#include "SceneNode.h"

class Model;
class Visitor;

class PmxInstant;

namespace Math
{
    class OrthogonalTransform;
    class AffineTransform;
}

class BoneRef 
{
public:

    BoneRef() {}
    BoneRef( PmxInstant* inst, uint32_t i );

    const Math::OrthogonalTransform& GetLocalTransform() const;
    void SetLocalTransform( const Math::OrthogonalTransform& transform );

    uint32_t m_Index = 0;
    PmxInstant* m_Instance = nullptr;
};

class PmxInstant : public SceneNode
{
public:

    PmxInstant( Model& model );

    bool LoadModel();
    bool LoadMotion( const std::wstring& motionPath );

    virtual void Accept( Visitor& visitor ) override;
    virtual void JoinWorld( btDynamicsWorld* world ) override;
    virtual void LeaveWorld( btDynamicsWorld* world ) override;

    virtual void Render( GraphicsContext& Context, Visitor& visitor ) override;
    virtual void RenderBone( GraphicsContext& Context, Visitor& visitor ) override;
    virtual void Update( float deltaT ) override;

    const Math::OrthogonalTransform& GetLocalTransform( uint32_t i ) const;
    void SetLocalTransform( uint32_t i, const Math::OrthogonalTransform& transform );

protected:

    struct Context;
    std::shared_ptr<Context> m_Context;
};
