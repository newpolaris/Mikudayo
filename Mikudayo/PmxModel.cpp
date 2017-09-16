#include "stdafx.h"
#include "PmxModel.h"
#include "KeyFrameAnimation.h"
#include "Model.h"
#include "Pmx.h"
#include "FxManager.h"
#include "SoftBodyManager.h"
#include "Bullet/BaseSoftBody.h"

using namespace Utility;
using namespace Graphics;

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

    std::vector<InputDesc> InputDescriptor
    {
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BONE_ID", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BONE_WEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "EDGE_FLAT", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
}

PmxModel::PmxModel()
{
}

PmxModel::~PmxModel()
{
    Clear();
}

void PmxModel::Clear()
{
    m_IndexBuffer.Destroy();
}

bool PmxModel::Load( const ModelInfo& Info )
{
    if (!LoadFromFile( Info.File ))
        return false;
    if (!SetCustomShader( Info.Shader ))
        return false;
    if (!SetPhysicsBody( Info.SoftBodySetting ))
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
            material.Textures.push_back( LoadTexture( tex.Path, tex.bSRGB ) );
        }
        if (material.Textures[kTextureToon])
            material.CB.bUseToon = TRUE;
        if (material.Textures[kTextureDiffuse])
            material.CB.bUseTexture = TRUE;
        material.EdgeSize = material.EdgeSize;
        material.EdgeColor = Color( Vector4( material.EdgeColor ) ).FromSRGB();
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
        mat.Techniques = FxManager::GetFx( "pmx" );
        mat.TexturePathes.resize(kTextureDefaultCount);
        if (material.DiffuseTexureIndex >= 0)
            mat.TexturePathes[kTextureDiffuse] = { true, pmx.m_Textures[material.DiffuseTexureIndex] };
        if (material.SphereTextureIndex >= 0)
            mat.TexturePathes[kTextureSphere] = { true, pmx.m_Textures[material.SphereTextureIndex] };

        std::wstring ToonName;
        if (material.bDefaultToon)
            ToonName = ToonList[material.DeafultToon];
        else if (material.Toon >= 0)
            ToonName = pmx.m_Textures[material.Toon];
        if (!ToonName.empty())
            mat.TexturePathes[kTextureToon] = { true, ToonName };

        MaterialCB cb;
		cb.Diffuse = material.Diffuse;
		cb.Specular = material.Specular;
		cb.SpecularPower = material.SpecularPower;
		cb.Ambient = material.Ambient;
        cb.SphereOperation = material.SphereOperation;

        mat.CB = cb;
		mat.EdgeSize = material.EdgeSize;
		mat.EdgeColor = Color(Vector4(material.EdgeColor)).FromSRGB();
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
		m_Materials.push_back(mat);
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
		m_BoneIndex[src.Name] = i;
	}

    // Find root bone
    ASSERT( numBones > 0 );
    auto it = std::find_if( m_Bones.begin(), m_Bones.end(), [](const Bone& Bone){
        return Bone.Name.compare( L"センター" ) == 0;
    });
    if (it == m_Bones.end())
        it = m_Bones.begin();
    m_RootBoneIndex = static_cast<uint32_t>(std::distance( m_Bones.begin(), it ));

    m_LocalPose.resize( numBones );
    for (auto i = 0; i < numBones; i++)
        m_LocalPose[i].SetTranslation( m_Bones[i].Translate );
    m_Pose = m_LocalPose;
    for (auto i = 0; i < numBones; i++)
    {
        const auto parentIndex = m_Bones[i].Parent;
        if (parentIndex >= 0)
            m_Pose[i] = m_Pose[parentIndex] * m_Pose[i];
    }
    return true;
}

const ManagedTexture* PmxModel::LoadTexture( std::wstring ImageName, bool bSRGB )
{
    if (ImageName.empty())
        return nullptr;
    using Path = boost::filesystem::path;

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
        {
            Utility::PrintSubMessage( L"Can't find matarial " + matName );
            continue;
        }
        auto index = m_MaterialIndex[matName];
        ASSERT(index < m_Materials.size());
        auto& mat = m_Materials[index];
        mat.Techniques = FxManager::GetFx( Data.Name );
        for (auto& texture : Data.Textures)
        {
            ASSERT(mat.TexturePathes.size() < texture.Slot, "Slot alread used");
            mat.TexturePathes.push_back( { false, texture.Path } );
        }
    }
    return true;
}

bool PmxModel::SetPhysicsBody( const std::string& SoftBodyName )
{
    m_SoftBodyName = SoftBodyName;

    auto Body = SoftBodyManager::GetInstance( m_SoftBodyName );
    if (Body)
    {
        std::vector<XMUINT4> indices;
        std::vector<XMFLOAT4> weights;
        std::vector<AffineTransform> pose;
        Body->GetSoftBodySkinning( m_VertexPosition, pose, indices, weights );
        for (uint32_t i = 0; i < m_VertexAttribute.size(); i++)
        {
            auto* idx = reinterpret_cast<uint32_t*>(&indices[i]);
            auto* wtx = reinterpret_cast<float*>(&weights[i]);
            for (uint32_t k = 0; k < 4; k++)
            {
                m_VertexAttribute[i].BoneID[k] = idx[k];
                m_VertexAttribute[i].Weight[k] = wtx[k];
            }
        }
        m_Pose = pose;
    }
    return true;
}

bool PmxModel::Material::SetTexture( GraphicsContext& gfxContext ) const
{
    std::vector<D3D11_SRV_HANDLE> SRV(Textures.size());
    for (auto i = 0U; i < Textures.size(); i++)
    {
        if (Textures[i] == nullptr) continue;
        SRV[i] = Textures[i]->GetSRV();
    }
    gfxContext.SetDynamicDescriptors( 0, (UINT)SRV.size(), SRV.data(), { kBindPixel } );
    return true;
}
