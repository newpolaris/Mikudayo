#pragma once

#include "IModel.h"
#include "BaseModelTypes.h"
#include "SceneNode.h"
#include "RenderType.h"

class BaseModel : public IModel, public SceneNode
{
public:

    static void Initialize();
    static void Shutdown();
    static void AppendTechniques( const std::wstring& Name, RenderPipelineList&& List );
    static const RenderPipelineList& FindTechniques( const std::wstring& Name );

    BaseModel();

    virtual void Clear() override;
    virtual bool Load( const ModelInfo& info ) override;
    virtual void Render( GraphicsContext& gfxContext, Visitor& visitor ) override;

    virtual Math::Matrix4 GetTransform() const override;
    virtual void SetTransform( const Math::Matrix4& transform ) override;

protected:

    std::wstring m_FileName;
    std::wstring m_DefaultShader;

    Materials m_Materials;
    Meshes m_Meshes;

    Matrix4 m_Transform;
    VertexBuffer m_VertexBuffer;
    IndexBuffer m_IndexBuffer;
    VertexBuffer m_VertexBufferDepth;
    IndexBuffer m_IndexBufferDepth;
};