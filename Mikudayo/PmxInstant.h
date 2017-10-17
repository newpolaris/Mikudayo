#pragma once

#include "SceneNode.h"

class IModel;
class Visitor;
class btTransform;
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

    const Math::OrthogonalTransform GetTransform() const;
    void SetTransform( const Math::OrthogonalTransform& transform );
    void SetTransform( const btTransform& trnasform );
    void UpdateLocalTransform();

    uint32_t m_Index = 0;
    PmxInstant* m_Instance = nullptr;
};

class PmxInstant : public SceneNode
{
public:

    PmxInstant( IModel& model );

    bool LoadModel();
    bool LoadMotion( const std::wstring& motionPath );

    virtual void Accept( Visitor& visitor ) override;
    virtual void Render( GraphicsContext& Context, Visitor& visitor ) override;
    virtual void RenderBone( GraphicsContext& Context, Visitor& visitor ) override;
    virtual void Update( float deltaT ) override;
    virtual void UpdateAfterPhysics( float deltaT ) override;

    const Math::OrthogonalTransform GetTransform( uint32_t i ) const;
    void SetLocalTransform( uint32_t i, const Math::OrthogonalTransform& transform );
    void UpdateLocalTransform( uint32_t i );

protected:

    struct Context;
    std::shared_ptr<Context> m_Context;
};
