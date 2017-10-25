#include "stdafx.h"
#include "PmxModel.h"
#include "KeyFrameAnimation.h"
#include "IModel.h"
#include "Pmx.h"
#include "Color.h"

#include "CompiledShaders/PmxColorVS.h"
#include "CompiledShaders/PmxColorPS.h"
#include "CompiledShaders/OutlineVS.h"
#include "CompiledShaders/OutlinePS.h"
#include "CompiledShaders/DeferredGBufferPS.h"
#include "CompiledShaders/DeferredFinalPS.h"
#include "CompiledShaders/DeferredFinal2PS.h"
#include "CompiledShaders/DepthViewerVS.h"

using namespace Utility;
using namespace Graphics;

namespace Pmx {
    std::vector<InputDesc> VertElem
    {
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BONE_ID", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BONE_WEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "EDGE_SCALE", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
}

namespace {
    const std::wstring ToonList[10] = {
        L"toon01.bmp",
        L"toon02.bmp",
        L"toon03.bmp",
        L"toon04.bmp",
        L"toon05.bmp",
        L"toon06.bmp",
        L"toon07.bmp",
        L"toon08.bmp",
        L"toon09.bmp",
        L"toon10.bmp"
    };

    std::map<std::wstring, RenderPipelineList> Techniques;

#define LinearColor 1
#if LinearColor
    bool bSRGB = true;
    template <typename T>
    T ConvertColor(T t) {
        return FromSRGB( t );
    }
#else
    bool bSRGB = false;
    template <typename T>
    T ConvertColor(T t) {
        return t;
    }
#endif
}

void AutoFillPSO( RenderPipelinePtr& basePSO, uint32_t baseIndex, RenderPipelineList& list )
{
    RenderPipelinePtr baseTowSidedPSO, transparentPSO, transparentTwoSidedPSO;

    baseTowSidedPSO = std::make_shared<GraphicsPSO>();
    *baseTowSidedPSO = *basePSO;
    baseTowSidedPSO->SetRasterizerState( RasterizerTwoSided );
    baseTowSidedPSO->Finalize();

    transparentPSO = std::make_shared<GraphicsPSO>();
    *transparentPSO = *basePSO;
    transparentPSO->SetBlendState( BlendTraditional );
    transparentPSO->Finalize();

    transparentTwoSidedPSO = std::make_shared<GraphicsPSO>();
    *transparentTwoSidedPSO = *transparentPSO;
    transparentTwoSidedPSO->SetRasterizerState( RasterizerTwoSided );
    transparentTwoSidedPSO->Finalize();

    list[baseIndex] = basePSO;
    list[baseIndex + 1] = baseTowSidedPSO;
    list[baseIndex + 2] = transparentPSO;
    list[baseIndex + 3] = transparentTwoSidedPSO;
};


void PmxModel::Initialize()
{
    RenderPipelinePtr DeferredGBufferPSO, OutlinePSO, DepthPSO, ShadowPSO;

    DeferredGBufferPSO = std::make_shared<GraphicsPSO>();
    DeferredGBufferPSO->SetInputLayout( (UINT)Pmx::VertElem.size(), Pmx::VertElem.data() );
    DeferredGBufferPSO->SetVertexShader( MY_SHADER_ARGS( g_pPmxColorVS ) );
    DeferredGBufferPSO->SetPixelShader( MY_SHADER_ARGS( g_pDeferredGBufferPS ) );
    DeferredGBufferPSO->SetDepthStencilState( DepthStateReadWrite );
    DeferredGBufferPSO->SetRasterizerState( RasterizerDefault );
    DeferredGBufferPSO->Finalize();

    OutlinePSO = std::make_shared<GraphicsPSO>();
    OutlinePSO->SetInputLayout( (UINT)Pmx::VertElem.size(), Pmx::VertElem.data() );
    OutlinePSO->SetVertexShader( MY_SHADER_ARGS( g_pOutlineVS ) );
    OutlinePSO->SetPixelShader( MY_SHADER_ARGS( g_pOutlinePS ) );
    OutlinePSO->SetRasterizerState( RasterizerDefaultCW );
    OutlinePSO->SetDepthStencilState( DepthStateReadWrite );
    OutlinePSO->Finalize();

    DXGI_FORMAT DepthFormat = g_SceneDepthBuffer.GetFormat();
    DepthPSO = std::make_shared<GraphicsPSO>();
    DepthPSO->SetRasterizerState( RasterizerDefault );
    DepthPSO->SetBlendState( BlendNoColorWrite );
    DepthPSO->SetDepthStencilState( DepthStateReadWrite );
    DepthPSO->SetInputLayout( (UINT)Pmx::VertElem.size(), Pmx::VertElem.data() );
    DepthPSO->SetRenderTargetFormats( 0, nullptr, DepthFormat );
    DepthPSO->SetVertexShader( MY_SHADER_ARGS( g_pDepthViewerVS ) );
    DepthPSO->Finalize();

    ShadowPSO = std::make_shared<GraphicsPSO>();
    *ShadowPSO = *DepthPSO;
    ShadowPSO->SetRasterizerState( RasterizerShadow );
    ShadowPSO->SetRenderTargetFormats( 0, nullptr, g_ShadowBuffer.GetFormat() );
    ShadowPSO->Finalize();

    RenderPipelineList Default;
    RenderPipelinePtr OpaquePSO, GBufferPSO, FinalPSO;
    {
        OpaquePSO = std::make_shared<GraphicsPSO>();
        OpaquePSO->SetInputLayout( (UINT)Pmx::VertElem.size(), Pmx::VertElem.data() );
        OpaquePSO->SetVertexShader( MY_SHADER_ARGS( g_pPmxColorVS ) );
        OpaquePSO->SetPixelShader( MY_SHADER_ARGS( g_pPmxColorPS ) );;
        OpaquePSO->SetRasterizerState( RasterizerDefault );
        OpaquePSO->SetDepthStencilState( DepthStateReadWrite );
        OpaquePSO->Finalize();

        AutoFillPSO( OpaquePSO, 0, Default );

        FinalPSO = std::make_shared<GraphicsPSO>();
        FinalPSO->SetInputLayout( (UINT)Pmx::VertElem.size(), Pmx::VertElem.data() );
        FinalPSO->SetVertexShader( MY_SHADER_ARGS( g_pPmxColorVS ) );
        FinalPSO->SetPixelShader( MY_SHADER_ARGS( g_pDeferredFinalPS ) );
        FinalPSO->SetRasterizerState( RasterizerDefault );
        FinalPSO->SetDepthStencilState( DepthStateTestEqual );
        FinalPSO->Finalize();

        GBufferPSO = std::make_shared<GraphicsPSO>();
        GBufferPSO->SetInputLayout( (UINT)Pmx::VertElem.size(), Pmx::VertElem.data() );
        GBufferPSO->SetVertexShader( MY_SHADER_ARGS( g_pPmxColorVS ) );
        GBufferPSO->SetPixelShader( MY_SHADER_ARGS( g_pDeferredGBufferPS ) );
        GBufferPSO->SetDepthStencilState( DepthStateReadWrite );
        GBufferPSO->SetRasterizerState( RasterizerDefault );
        GBufferPSO->Finalize();

        D3D11_DEPTH_STENCIL_DESC depth2 = DepthStateReadWrite;
        depth2.StencilEnable = TRUE;
        depth2.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;

        AutoFillPSO( OpaquePSO, kRenderQueueOpaque, Default );

        RenderPipelinePtr ReflectPSO = std::make_shared<GraphicsPSO>();
        *ReflectPSO = *OpaquePSO;
        ReflectPSO->SetDepthStencilState( depth2 );
        ReflectPSO->SetRasterizerState( RasterizerDefaultCW );
        ReflectPSO->SetStencilRef( 1 );
        ReflectPSO->Finalize();

        AutoFillPSO( ReflectPSO, kRenderQueueReflectOpaque, Default );

        D3D11_DEPTH_STENCIL_DESC depth1 = DepthStateReadOnly;
        depth1.StencilEnable = TRUE;
        depth1.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;

        RenderPipelinePtr stencilPSO = std::make_shared<GraphicsPSO>();
        stencilPSO->SetInputLayout( (UINT)Pmx::VertElem.size(), Pmx::VertElem.data() );
        stencilPSO->SetVertexShader( MY_SHADER_ARGS( g_pPmxColorVS ) );
        stencilPSO->SetDepthStencilState( depth1 );
        stencilPSO->SetStencilRef( 1 );
        stencilPSO->Finalize();

        Default[kRenderQueueDeferredGBuffer] = GBufferPSO;
        Default[kRenderQueueDeferredFinal] = FinalPSO;
        Default[kRenderQueueOutline] = OutlinePSO;
        Default[kRenderQueueShadow] = ShadowPSO;
        Default[kRenderQueueReflectStencil] = stencilPSO;
    }
    Techniques.emplace( L"Default", std::move( Default ) );
}

void PmxModel::Shutdown()
{
    Techniques.clear();
}

PmxModel::PmxModel() : 
    m_DefaultShader( L"Default" )
{
}

void PmxModel::Clear()
{
    m_IndexBuffer.Destroy();
}

bool PmxModel::Load( const ModelInfo& Info )
{
    SetDefaultShader( Info.DefaultShader );

    if (!LoadFromFile( Info.ModelFile ))
        return false;
    if (!SetCustomShader( Info.Shader ))
        return false;
    if (!GenerateResource())
        return false;
    return true;
}

bool PmxModel::GenerateResource( void )
{
	m_IndexBuffer.Create( m_Name + L"_IndexBuf", static_cast<uint32_t>(m_Indices.size()),
        sizeof( m_Indices[0] ), m_Indices.data() );

	for (auto& material : m_Materials)
	{
        for (auto i = 0; i < material.TexturePathes.size(); i++)
        {
            auto& tex = material.TexturePathes[i];
            if (!tex.Path.empty())
                material.Textures[i] = LoadTexture( tex.Path, tex.bSRGB );
            if (material.Textures[kTextureToon])
                material.CB.bUseToon = TRUE;
            if (material.Textures[kTextureDiffuse])
                material.CB.bUseTexture = TRUE;
        }
	}
    return true;
}

bool PmxModel::LoadFromFile( const std::wstring& FilePath )
{
    using Pmx::Vertex;
    using Path = boost::filesystem::path;

    ByteArray ba = ReadFileSync( FilePath );
    ByteStream bs( ba );

    Pmx::PMX pmx;
    pmx.Fill( bs, true );
    if (!pmx.IsValid())
        return false;

	m_Name = pmx.m_Description.Name;
    m_TextureRoot = Path(FilePath).parent_path().generic_wstring();

	m_VertexAttribute.resize( pmx.m_Vertices.size() );
	m_VertexPosition.resize( pmx.m_Vertices.size() );
	for (auto i = 0; i < pmx.m_Vertices.size(); i++)
	{
		m_VertexPosition[i] = pmx.m_Vertices[i].Pos;
		m_VertexAttribute[i].Normal = pmx.m_Vertices[i].Normal;
		m_VertexAttribute[i].UV = pmx.m_Vertices[i].UV;
        // Not supported
        ASSERT( pmx.m_Vertices[i].SkinningType != Vertex::kQdef );
        // Convert to 4-element linear skinning
        switch (pmx.m_Vertices[i].SkinningType)
        {
        case Vertex::kBdef1:
            m_VertexAttribute[i].BoneID[0] = pmx.m_Vertices[i].bdef1.BoneIndex;
            m_VertexAttribute[i].Weight[0] = 1.f;
            break;
        case Vertex::kSdef: // ignore rotation center, r0 and r1 treat as Bdef2
        case Vertex::kBdef2:
            m_VertexAttribute[i].BoneID[0] = pmx.m_Vertices[i].bdef2.BoneIndex[0];
            m_VertexAttribute[i].BoneID[1] = pmx.m_Vertices[i].bdef2.BoneIndex[1];
            m_VertexAttribute[i].Weight[0] = pmx.m_Vertices[i].bdef2.Weight;
            m_VertexAttribute[i].Weight[1] = 1 - pmx.m_Vertices[i].bdef2.Weight;
            break;
        case Vertex::kBdef4:
            for (int k = 0; k < 4; k++)
            {
                m_VertexAttribute[i].BoneID[k] = pmx.m_Vertices[i].bdef4.BoneIndex[k];
                m_VertexAttribute[i].Weight[k] = pmx.m_Vertices[i].bdef4.Weight[k];
            }
            break;
        }
		m_VertexAttribute[i].EdgeSize = pmx.m_Vertices[i].EdgeSize;
	}
    std::copy(pmx.m_Indices.begin(), pmx.m_Indices.end(), std::back_inserter(m_Indices));

	uint32_t IndexOffset = 0;
    for (auto i = 0; i < pmx.m_Materials.size(); i++)
    {
	    auto& material = pmx.m_Materials[i];

		Material mat = {};
        mat.Name = material.Name;
        mat.ShaderName = m_DefaultShader;
        mat.TexturePathes.resize(kTextureMax);
        if (material.DiffuseTexureIndex >= 0)
            mat.TexturePathes[kTextureDiffuse] = { bSRGB, pmx.m_Textures[material.DiffuseTexureIndex] };
        if (material.SphereTextureIndex >= 0)
            mat.TexturePathes[kTextureSphere] = { bSRGB, pmx.m_Textures[material.SphereTextureIndex] };

        std::wstring ToonName;
        if (material.bDefaultToon)
            ToonName = ToonList[material.DeafultToon];
        else if (material.Toon >= 0)
            ToonName = pmx.m_Textures[material.Toon];
        if (!ToonName.empty())
            mat.TexturePathes[kTextureToon] = { bSRGB, ToonName };

        MaterialCB cb = {};
		cb.Diffuse = ConvertColor(material.Diffuse);
		cb.Specular = ConvertColor(material.Specular);
		cb.SpecularPower = material.SpecularPower;
        cb.Ambient = ConvertColor(material.Ambient);
        cb.SphereOperation = material.SphereOperation;
        if (mat.TexturePathes[kTextureSphere].Path.empty())
            cb.SphereOperation = Pmx::ESphereOpeation::kNone;

        mat.CB = cb;
		mat.CB.EdgeSize = material.EdgeSize;
        mat.CB.EdgeColor = ConvertColor(material.EdgeColor);
        mat.bOutline = material.BitFlag & Pmx::EMaterialFlag::kEnableEdge;
        mat.bCastShadowMap = material.BitFlag & Pmx::EMaterialFlag::kCastShadowMap;
        mat.bTwoSided = material.BitFlag & Pmx::EMaterialFlag::kCullOff;
        if (Techniques.count( m_DefaultShader ))
            mat.Techniques = Techniques[m_DefaultShader];
        m_Materials.push_back(mat);

        Mesh mesh;
		mesh.IndexCount = material.NumVertex;
		mesh.IndexOffset = IndexOffset;
        mesh.BoundSphere = ComputeBoundingSphereFromVertices(
            m_VertexPosition, m_Indices, mesh.IndexCount, mesh.IndexOffset );
        mesh.MaterialIndex = i;
        m_Mesh.push_back(mesh);

		IndexOffset += material.NumVertex;
        m_MaterialIndex[mat.Name] = (uint32_t)m_MaterialIndex.size();
	}

    const auto& Bones = pmx.m_Bones;
    size_t numBones = Bones.size();
	m_Bones.resize( numBones );
	for (auto i = 0; i < numBones; i++)
	{
        auto& src = Bones[i];
        auto& dst = m_Bones[i];

        dst.Name = src.Name;
        dst.Parent = src.ParentBoneIndex;
		if (src.ParentBoneIndex >= 0)
			m_Bones[src.ParentBoneIndex].Child.push_back( i );
		Vector3 origin = src.Position;
		Vector3 parentOrigin = Vector3( 0.0f, 0.0f, 0.0f );
		if( src.ParentBoneIndex >= 0)
			parentOrigin = Bones[src.ParentBoneIndex].Position;
		dst.Translate = origin - parentOrigin;
        dst.Position = origin;
        dst.DestinationIndex = src.DestinationOriginIndex;
        dst.DestinationOffset = src.DestinationOriginOffset;
        dst.bInherentRotation = src.bInherentRotation;
        dst.bInherentTranslation = src.bInherentTranslation;
        dst.ParentInherentBoneIndex = src.ParentInherentBoneIndex;
        dst.ParentInherentBoneCoefficent = src.ParentInherentBoneCoefficent;
		m_BoneIndex[src.Name] = i;
	}

    for (auto i = 0; i < numBones; i++)
    {
        if (!Bones[i].bIK)
            continue;
        auto& it = Bones[i].Ik;
        IKAttr attr;
        attr.BoneIndex = i;
        attr.TargetBoneIndex = it.BoneIndex;
        attr.LimitedRadian = it.LimitedRadian;
        attr.NumIteration = it.NumIteration;

        for (auto& ik : it.Link)
        {
            IKChild child;
            child.BoneIndex = ik.BoneIndex;
            child.bLimit = ik.bLimit;
            child.MinLimit = ik.MinLimit;
            child.MaxLimit = ik.MaxLimit;
            attr.Link.push_back( child );
        }
        m_IKs.push_back( attr );
    }

    // Find root bone
    ASSERT( numBones > 0 );
    auto it = std::find_if( m_Bones.begin(), m_Bones.end(), [](const Bone& Bone){
        return Bone.Name.compare( L"センター" ) == 0;
    });
    if (it == m_Bones.end())
        it = m_Bones.begin();
    m_RootBoneIndex = static_cast<uint32_t>(std::distance( m_Bones.begin(), it ));

    m_RigidBodies = std::move( pmx.m_RigidBodies );
    m_Joints = std::move( pmx.m_Joints );

    return true;
}

const ManagedTexture* PmxModel::LoadTexture( std::wstring ImageName, bool bSRGB )
{
    using Path = boost::filesystem::path;
    ASSERT(!ImageName.empty());

    const Path imagePath = Path(m_TextureRoot) / ImageName;
    bool bExist = boost::filesystem::exists( imagePath );
    if (bExist)
    {
        auto wstrPath = imagePath.generic_wstring();
        return TextureManager::LoadFromFile( wstrPath, bSRGB );
    }
    const std::wregex toonPattern( L"toon[0-9]{1,2}.bmp", std::regex_constants::icase );
    if (std::regex_match( ImageName.begin(), ImageName.end(), toonPattern ))
    {
        auto toonPath = Path( "toon" ) / ImageName;
        return TextureManager::LoadFromFile( toonPath.generic_wstring(), bSRGB );
    }
    return &TextureManager::GetMagentaTex2D();
}

bool PmxModel::SetCustomShader( const CustomShaderInfo& Data )
{
    for (auto& matName : Data.MaterialNames)
    {
        if (m_MaterialIndex.count( matName ) == 0)
            return false;
        auto index = m_MaterialIndex[matName];
        auto& mat = m_Materials[index];
        mat.ShaderName = Data.Name;
        for (auto& texture : Data.Textures)
        {
            if ( mat.TexturePathes.size() < texture.Slot )
                return false;
            mat.TexturePathes.push_back( { true, texture.Path } );
        }
    }
    return true;
}

bool PmxModel::SetDefaultShader( const std::wstring& Name )
{
    m_DefaultShader = Name;
    return true;
}

void PmxModel::Material::SetTexture( GraphicsContext& gfxContext ) const
{
    D3D11_SRV_HANDLE SRV[kTextureMax] = { nullptr };
    for (auto i = 0; i < _countof( Textures ); i++)
    {
        if (Textures[i] == nullptr) continue;
        SRV[i] = Textures[i]->GetSRV();
    }
    gfxContext.SetDynamicDescriptors( 1, _countof( SRV ), SRV, { kBindPixel } );
}

bool PmxModel::Material::IsOutline() const
{
    return bOutline;
}

bool PmxModel::Material::IsShadowCaster() const
{ 
    return bCastShadowMap;
}

bool PmxModel::Material::IsTransparent() const
{
    bool bHasTransparentTexture = Textures[kTextureDiffuse] && Textures[kTextureDiffuse]->IsTransparent();
    return CB.Diffuse.w < 1.0f || bHasTransparentTexture;
}

bool PmxModel::Material::IsTwoSided() const
{
    return bTwoSided;
}

RenderPipelinePtr PmxModel::Material::GetPipeline( RenderQueue Queue ) 
{
    return Techniques[Queue];
}