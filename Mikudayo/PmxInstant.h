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
    BoneRef( PmxInstant* inst, int32_t i );

    const Math::OrthogonalTransform GetTransform() const;
    void SetTransform( const Math::OrthogonalTransform& transform );
    void SetTransform( const btTransform& trnasform );
    void UpdateLocalTransform();

    int32_t m_Index = 0; // valid check needed (-1)
    PmxInstant* m_Instance = nullptr;
};

class PmxInstant : public SceneNode
{
public:

    PmxInstant( IModel& model, const AffineTransform& transform );

    bool Load();
    bool LoadMotion( const std::wstring& motion );

    virtual bool IsDynamic( void ) const override;

    virtual void Accept( Visitor& visitor ) override;
    virtual void Render( GraphicsContext& Context, Visitor& visitor ) override;
    virtual void RenderBone( GraphicsContext& Context, Visitor& visitor ) override;
    virtual void Update( float deltaT ) override;
    virtual void UpdateAfterPhysics( float deltaT ) override;
    virtual void Skinning( GraphicsContext& gfxContext, Visitor& visitor ) override;
    virtual Math::BoundingBox GetBoundingBox() const override;
    virtual Math::AffineTransform GetTransform() const override;
    virtual void SetTransform( const Math::AffineTransform& transform );

    const Math::OrthogonalTransform GetTransform( int32_t i ) const;
    void SetTransform( int32_t i, const Math::OrthogonalTransform& transform );
    void UpdateLocalTransform( int32_t i );

protected:

    struct Context;
    std::shared_ptr<Context> m_Context;
};
