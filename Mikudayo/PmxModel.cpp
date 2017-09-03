#include "stdafx.h"
#include "Pmx.h"
#include "Vmd.h"
#include "PmxModel.h"
#include "KeyFrameAnimation.h"
#include "Model.h"

#include "CompiledShaders/PmxColorVS.h"
#include "CompiledShaders/PmxColorPS.h"

using namespace Rendering;
using namespace Utility;
using namespace Math;
using namespace Graphics;

namespace {
	__declspec(align(16))
    struct MaterialCB
	{
		XMFLOAT4 Diffuse;
		XMFLOAT3 Specular;
		float SpecularPower;
		XMFLOAT3 Ambient;
		int32_t SphereOperation;
		int32_t bUseTexture;
		int32_t bUseToon;
	};

	enum ETextureType
	{
		kTextureDiffuse,
		kTextureSphere,
		kTextureToon,
		kTextureMax
	};

	struct VertexProperty
	{
		XMFLOAT3 Normal;
		XMFLOAT2 UV;
        uint32_t BoneID[4] = {0, };
        float    Weight[4] = {0.f };
		float    EdgeSize;
	};

	struct Mesh
	{
		bool isTransparent() const { return Material.Diffuse.w < 1.f; }
        bool SetTexture( GraphicsContext& gfxContext );

		MaterialCB Material;
        const ManagedTexture* Texture[kTextureMax];
        int32_t IndexOffset;
		uint32_t IndexCount;
        BoundingSphere BoundSphere;
		float EdgeSize;
        Color EdgeColor;
	};

    bool Mesh::SetTexture( GraphicsContext& gfxContext )
    {
        D3D11_SRV_HANDLE SRV[kTextureMax] = { nullptr };
        for (auto i = 0; i < _countof( Texture ); i++)
        {
            if (Texture[i] == nullptr) continue;
            SRV[i] = Texture[i]->GetSRV();
        }
        gfxContext.SetDynamicDescriptors( 1, _countof(SRV), SRV, { kBindPixel } );
        return false;
    }

	struct Bone
	{
		std::wstring Name;
		Vector3 Translate; // Offset from parent
        Vector3 Position;
        int32_t DestinationIndex;
        Vector3 DestinationOffset;
	};

    template <typename T>
    size_t GetVectorSize( const std::vector<T>& vec )
    {
        return sizeof( T ) * vec.size();
    }

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
}

struct PmxModel::Private final
{
    Private();
    ~Private();

    void Clear( void );
    void Draw( GraphicsContext& gfxContext );
    void DrawBone( void );
    void DrawBoundingSphere( void );

    BoundingSphere GetBoundingSphere() const;
    BoundingBox GetBoundingBox() const;
    const std::vector<XMFLOAT3>& GetVertices( void ) const;
    const std::vector<uint32_t>& GetIndices( void ) const;
    bool LoadFromFile( const std::wstring& FilePath );
    bool LoadMotion( const std::wstring& FilePath );

    void SetPosition( const Vector3& postion );
    void SetVertices( const std::vector<XMFLOAT3>& vertices );
    void SetupBoundingBox( void );
    void SetupBoundingSphere( void );
    void SetupVisualizeSkeleton();
    void SetupSkeleton( const std::vector<Pmx::Bone>& Bones );

    void Update( float kFrameTime );

protected:

    void LoadBoneMotion( const std::vector<Vmd::BoneFrame>& frames );
	const ManagedTexture* LoadTexture( std::wstring ImageName, bool bSRGB );

    bool m_bRightHand;
    std::wstring m_Name;
    std::wstring m_TextureRoot;
    Matrix4 m_ModelTransform;
    std::vector<Mesh> m_Mesh;

    // Skinning
    std::vector<OrthogonalTransform> m_toRoot; // inverse inital pose ( inverse Rest)
    std::vector<OrthogonalTransform> m_LocalPose; // offset matrix
    std::vector<OrthogonalTransform> m_Pose; // cumulative transfrom matrix from root
    std::vector<OrthogonalTransform> m_Skinning; // final skinning transform
    std::vector<DualQuaternion> m_SkinningDual; // final skinning transform

    // Bone
    std::vector<Bone> m_Bones;
    std::vector<int32_t> m_BoneParent; // parent index
    std::vector<std::vector<int32_t>> m_BoneChild; // child indices
    std::map<std::wstring, uint32_t> m_BoneIndex;
    std::vector<Animation::BoneMotion> m_BoneMotions;
    std::vector<AffineTransform> m_BoneAttribute;

    std::map<std::wstring, uint32_t> m_MorphIndex;
    std::vector<Vector3> m_MorphDelta; // tempolar space to store morphed position delta
    enum { kMorphBase = 0 }; // m_MorphMotions's first slot is reserved as base (original) position data
    std::vector<Animation::MorphMotion> m_MorphMotions;

    std::vector<XMFLOAT3> m_VertexPos; // original vertex position
    std::vector<XMFLOAT3> m_VertexMorphedPos; // temporal vertex positions which affected by face animation
    std::vector<uint32_t> m_Indices;

    VertexBuffer m_AttributeBuffer;
    VertexBuffer m_PositionBuffer;
    IndexBuffer m_IndexBuffer;

    uint32_t m_RootBoneIndex; // model center
    BoundingSphere m_BoundingSphere;
    BoundingBox m_BoundingBox;

    GraphicsPSO m_DepthPSO;
    GraphicsPSO m_ColorPSO;
};

PmxModel::Private::Private() : m_bRightHand(true), m_ModelTransform(kIdentity)
{
}

PmxModel::Private::~Private()
{
    Clear();
}

void PmxModel::Private::Clear()
{
    m_ColorPSO.Destroy();

	m_AttributeBuffer.Destroy();
	m_PositionBuffer.Destroy();
	m_IndexBuffer.Destroy();
}
void PmxModel::Private::Draw( GraphicsContext& gfxContext )
{
    DrawBone();
    DrawBoundingSphere();

#define SKINNING_LBS
#ifdef SKINNING_LBS
    std::vector<Matrix4> SkinData;
    SkinData.reserve( m_Skinning.size() );
    for (auto& orth : m_Skinning)
        SkinData.emplace_back( orth );
#else // SKINNING_DLB
    auto& SkinData = m_SkinningDual;
#endif
    auto numByte = GetVectorSize(SkinData);

    gfxContext.SetDynamicConstantBufferView( 1, numByte, SkinData.data(), { kBindVertex } );
    gfxContext.SetDynamicConstantBufferView( 2, sizeof(m_ModelTransform), &m_ModelTransform, { kBindVertex } );
	gfxContext.SetVertexBuffer( 0, m_AttributeBuffer.VertexBufferView() );
	gfxContext.SetVertexBuffer( 1, m_PositionBuffer.VertexBufferView() );
	gfxContext.SetIndexBuffer( m_IndexBuffer.IndexBufferView() );
    gfxContext.SetPipelineState( m_ColorPSO );

	for (auto& mesh: m_Mesh)
	{
        if (mesh.SetTexture( gfxContext ))
            continue;
		gfxContext.SetDynamicConstantBufferView( 0, sizeof(mesh.Material), &mesh.Material, { kBindPixel } );
		gfxContext.DrawIndexed( mesh.IndexCount, mesh.IndexOffset, 0 );
	}
}

void PmxModel::Private::DrawBone()
{
    if (!Model::s_bEnableDrawBone)
        return;
	auto numBones = m_BoneAttribute.size();
	for (auto i = 0; i < numBones; i++)
        Model::Append( Model::kBoneMesh, m_ModelTransform * m_Skinning[i] * m_BoneAttribute[i] );
}

void PmxModel::Private::DrawBoundingSphere()
{
    if (!Model::s_bEnableDrawBoundingSphere)
        return;

    BoundingSphere sphere = GetBoundingSphere();
    AffineTransform scale = AffineTransform::MakeScale( float(sphere.GetRadius()) );
    AffineTransform center = AffineTransform::MakeTranslation( sphere.GetCenter() );
    Model::Append( Model::kSphereMesh, center*scale );
}

BoundingSphere PmxModel::Private::GetBoundingSphere() const
{
	if (m_BoneMotions.size() > 0)
        return m_ModelTransform * m_Skinning[m_RootBoneIndex] * m_BoundingSphere;
    return m_ModelTransform * m_BoundingSphere;
}

BoundingBox PmxModel::Private::GetBoundingBox() const
{
	if (m_BoneMotions.size() > 0)
        return m_ModelTransform * m_Skinning[m_RootBoneIndex] * m_BoundingBox;
    return m_ModelTransform * m_BoundingBox;
}

const std::vector<XMFLOAT3>& PmxModel::Private::GetVertices( void ) const
{
    return m_VertexMorphedPos;
}

const std::vector<uint32_t>& PmxModel::Private::GetIndices( void ) const
{
    return m_Indices;
}

bool PmxModel::Private::LoadFromFile( const std::wstring& FilePath )
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

	std::vector<VertexProperty> vertProperty( pmx.m_Vertices.size() );
	m_VertexPos.resize( pmx.m_Vertices.size() );
	for (auto i = 0; i < pmx.m_Vertices.size(); i++)
	{
		m_VertexPos[i] = pmx.m_Vertices[i].Pos;
		vertProperty[i].Normal = pmx.m_Vertices[i].Normal;
		vertProperty[i].UV = pmx.m_Vertices[i].UV;

        ASSERT( pmx.m_Vertices[i].SkinningType != Vertex::kQdef );

        switch (pmx.m_Vertices[i].SkinningType)
        {
        case Vertex::kBdef1:
            vertProperty[i].BoneID[0] = pmx.m_Vertices[i].bdef1.BoneIndex;
            vertProperty[i].Weight[0] = 1.f;
            break;
        case Vertex::kSdef: // ignore rotation center, r0 and r1 treat as Bdef2
        case Vertex::kBdef2:
            vertProperty[i].BoneID[0] = pmx.m_Vertices[i].bdef2.BoneIndex[0];
            vertProperty[i].BoneID[1] = pmx.m_Vertices[i].bdef2.BoneIndex[1];
            vertProperty[i].Weight[0] = pmx.m_Vertices[i].bdef2.Weight;
            vertProperty[i].Weight[1] = 1 - pmx.m_Vertices[i].bdef2.Weight;
            break;
        case Vertex::kBdef4:
            for (int k = 0; k < 4; k++)
            {
                vertProperty[i].BoneID[k] = pmx.m_Vertices[i].bdef4.BoneIndex[k];
                vertProperty[i].Weight[k] = pmx.m_Vertices[i].bdef4.Weight[k];
            }
            break;
        }
		vertProperty[i].EdgeSize = pmx.m_Vertices[i].EdgeSize;
	}
	m_VertexMorphedPos = m_VertexPos;
    std::copy(pmx.m_Indices.begin(), pmx.m_Indices.end(), std::back_inserter(m_Indices));

	m_AttributeBuffer.Create( m_Name + L"_AttrBuf",
		static_cast<uint32_t>(vertProperty.size()),
		sizeof( VertexProperty ),
		vertProperty.data() );

	m_PositionBuffer.Create( m_Name + L"_PosBuf",
		static_cast<uint32_t>(m_VertexPos.size()),
		sizeof( XMFLOAT3 ),
		m_VertexPos.data() );

	m_IndexBuffer.Create( m_Name + L"_IndexBuf",
		static_cast<uint32_t>(pmx.m_Indices.size()),
		sizeof( pmx.m_Indices[0] ),
		pmx.m_Indices.data() );

	uint32_t IndexOffset = 0;
	for (auto& material : pmx.m_Materials)
	{
		Mesh mesh = {};

		MaterialCB mat = {};
		mat.Diffuse = material.Diffuse;
		mat.Specular = material.Specular;
		mat.SpecularPower = material.SpecularPower;
		mat.Ambient = material.Ambient;

		mesh.IndexCount = material.NumVertex;
		mesh.IndexOffset = IndexOffset;
		IndexOffset += material.NumVertex;

        if (material.DiffuseTexureIndex >= 0)
			mesh.Texture[kTextureDiffuse] = LoadTexture( pmx.m_Textures[material.DiffuseTexureIndex], true );
        if (material.SphereTextureIndex >= 0)
			mesh.Texture[kTextureSphere] = LoadTexture( pmx.m_Textures[material.SphereTextureIndex], true );

        std::wstring ToonName;
        if (material.bDefaultToon)
            ToonName = ToonList[material.DeafultToon];
        else if (material.Toon >= 0)
            ToonName = pmx.m_Textures[material.Toon];
        if (!ToonName.empty())
            mesh.Texture[kTextureToon] = LoadTexture( ToonName, true );

		if (mesh.Texture[kTextureToon])
			mat.bUseToon = TRUE;
		if (mesh.Texture[kTextureDiffuse])
			mat.bUseTexture = TRUE;
		if (mesh.Texture[kTextureSphere])
			mat.SphereOperation = material.SphereOperation;

		mesh.Material = mat;
		mesh.EdgeSize = material.EdgeSize;
		mesh.EdgeColor = Color(Vector4(material.EdgeColor)).FromSRGB();

        // if motion is not registered, bounding box is used to viewpoint culling
        mesh.BoundSphere = ComputeBoundingSphereFromVertices(
            m_VertexPos, m_Indices, mesh.IndexCount, mesh.IndexOffset );

		m_Mesh.push_back(mesh);
	}

	SetupSkeleton( pmx.m_Bones );

	std::vector<InputDesc> InputDescriptor
	{
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONE_ID", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONE_WEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "EDGE_FLAT", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// Depth-only (2x rate)
	m_DepthPSO.SetRasterizerState( RasterizerDefault );
	m_DepthPSO.SetBlendState( BlendNoColorWrite );
	m_DepthPSO.SetInputLayout( static_cast<UINT>(InputDescriptor.size()), InputDescriptor.data() );
	m_DepthPSO.SetPrimitiveTopologyType( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    m_DepthPSO.SetDepthStencilState( DepthStateReadWrite );

    m_ColorPSO = m_DepthPSO;
    m_ColorPSO.SetBlendState( BlendDisable );
    m_ColorPSO.SetVertexShader( MY_SHADER_ARGS( g_pPmxColorVS ) );
    m_ColorPSO.SetPixelShader( MY_SHADER_ARGS( g_pPmxColorPS ) );;
    m_ColorPSO.Finalize();

    return true;
}

bool PmxModel::Private::LoadMotion( const std::wstring& motionPath )
{
	using namespace std;
	using namespace Animation;

	Utility::ByteArray ba = Utility::ReadFileSync( motionPath );
	Utility::ByteStream bs(ba);

	Vmd::VMD vmd;
	vmd.Fill( bs, m_bRightHand );
	if (!vmd.IsValid())
        return false;

    LoadBoneMotion( vmd.BoneFrames );

	for (auto& frame : vmd.FaceFrames)
	{
		MorphKeyFrame key;
		key.Frame = frame.Frame;
		key.Weight = frame.Weight;
		key.Weight = frame.Weight;
        WARN_ONCE_IF(m_MorphIndex.count(frame.FaceName) <= 0, L"Can't find target morph on model: ");
        if (m_MorphIndex.count(frame.FaceName) > 0)
        {
            auto& motion = m_MorphMotions[m_MorphIndex[frame.FaceName]];
            motion.m_Name = frame.FaceName;
            motion.InsertKeyFrame( key );
        }
	}
	for (auto& face : m_MorphMotions )
		face.SortKeyFrame();
    return true;
}

void PmxModel::Private::LoadBoneMotion( const std::vector<Vmd::BoneFrame>& frames )
{
    if (frames.size() <= 0)
        return;

    int32_t numBones = static_cast<int32_t>(m_Bones.size());
    m_BoneMotions.resize( numBones );
    m_Pose.resize( numBones );
    m_LocalPose.resize( numBones );
    m_toRoot.resize( numBones );
    for (auto i = 0; i < m_Bones.size(); i++)
        m_LocalPose[i].SetTranslation( m_Bones[i].Translate );

    std::vector<OrthogonalTransform> RestPose( numBones );
    for (auto i = 0; i < numBones; i++)
    {
        auto& bone = m_Bones[i];
        auto& parent = m_BoneParent[i];

        RestPose[i].SetTranslation( bone.Translate );
        if (parent < numBones)
            RestPose[i] = RestPose[parent] * RestPose[i];
    }

    for (auto i = 0; i < numBones; i++)
        m_toRoot[i] = ~RestPose[i];

	for (auto& frame : frames)
	{
		if ( m_BoneIndex.count( frame.BoneName ) == 0)
			continue;

		Vector3 BoneTranslate(m_Bones[m_BoneIndex[frame.BoneName]].Translate);

		Animation::BoneKeyFrame key;
		key.Frame = frame.Frame;
		key.Local.SetTranslation( Vector3(frame.Offset) + BoneTranslate );
		key.Local.SetRotation( Quaternion( frame.Rotation ) );

		//
		// http://harigane.at.webry.info/201103/article_1.html
		//
		// X_x1, Y_x1, Z_x1, R_x1,
		// X_y1, Y_y1, Z_y1, R_y1,
		// X_x2, Y_x2, Z_x2, R_x2,
		// X_y2, Y_y2, Z_y2, R_y2,
		//
		// ... (duplicated values)
		//
		auto interp = reinterpret_cast<const char*>(&frame.Interpolation[0]);
		float scale = 1.0f / 127.0f;

		for (auto i = 0; i < 4; i++)
			key.BezierCoeff[i] = Vector4( interp[i], interp[i+4], interp[i+8], interp[i+12] ) * scale;

		m_BoneMotions[m_BoneIndex[frame.BoneName]].InsertKeyFrame( key );
	}

	for (auto& bone : m_BoneMotions )
		bone.SortKeyFrame();
}

const ManagedTexture* PmxModel::Private::LoadTexture( std::wstring ImageName, bool bSRGB )
{
    using Path = boost::filesystem::path;

    const Path imagePath = Path(m_TextureRoot) / ImageName;
    bool bExist = boost::filesystem::exists( imagePath );
    if (bExist)
    {
        auto wstrPath = imagePath.generic_wstring();
        auto ia = ReadFileSync( wstrPath );
        return TextureManager::LoadFromMemory( wstrPath, ia, bSRGB );
    }
    const std::wregex toonPattern( L"toon[0-9]{1,2}.bmp", std::regex_constants::icase );
    if (std::regex_match( ImageName.begin(), ImageName.end(), toonPattern ))
    {
        auto toonPath = Path( "toon" ) / ImageName;
        return TextureManager::LoadFromFile( toonPath.generic_wstring(), bSRGB );
    }
    return &TextureManager::GetMagentaTex2D();
};

void PmxModel::Private::SetPosition( const Vector3& postion )
{
    m_ModelTransform = Matrix4::MakeTranslate( postion );
}

void PmxModel::Private::SetVertices( const std::vector<XMFLOAT3>& vertices )
{
	m_PositionBuffer.Create( m_Name + L"_PosBuf",
		static_cast<uint32_t>(vertices.size()),
		sizeof( XMFLOAT3 ),
		vertices.data() );
}

void PmxModel::Private::SetupSkeleton( const std::vector<Pmx::Bone>& Bones )
{
    size_t numBones = Bones.size();

	m_BoneParent.resize( numBones );
	m_BoneChild.resize( numBones );
	m_Bones.resize( numBones );

	for (auto i = 0; i < numBones; i++)
	{
		auto& boneData = Bones[i];
		m_Bones[i].Name = boneData.Name;
		m_BoneParent[i] = boneData.ParentBoneIndex;
		if (boneData.ParentBoneIndex >= 0)
			m_BoneChild[boneData.ParentBoneIndex].push_back( i );

		Vector3 origin = boneData.Position;
		Vector3 parentOrigin = Vector3( 0.0f, 0.0f, 0.0f );

		if( boneData.ParentBoneIndex >= 0)
			parentOrigin = Bones[boneData.ParentBoneIndex].Position;

		m_Bones[i].Translate = origin - parentOrigin;
        m_Bones[i].Position = origin;
        m_Bones[i].DestinationIndex = boneData.DestinationOriginIndex;
        m_Bones[i].DestinationOffset = boneData.DestinationOriginOffset;

		m_BoneIndex[boneData.Name] = i;
	}

    // find root bone
    ASSERT( numBones > 0 );
    auto it = std::find_if( m_Bones.begin(), m_Bones.end(), [](const Bone& Bone){
        return Bone.Name.compare( L"センター" ) == 0;
    });
    if (it == m_Bones.end())
        it = m_Bones.begin();
    m_RootBoneIndex = static_cast<uint32_t>(std::distance( m_Bones.begin(), it ));

    // set default skinning matrix
    m_Skinning.resize( numBones );
    m_SkinningDual.resize( numBones );
    for ( auto i = 0; i < numBones; i++)
        m_SkinningDual[i] = OrthogonalTransform();
}

void PmxModel::Private::SetupBoundingSphere( void )
{
    Vector3 Center = m_Bones[m_RootBoneIndex].Translate;
    Scalar Radius( 0.f );

    for (auto& vert : m_VertexPos) {
        Scalar R = LengthSquare( Center - Vector3( vert ) );
        if (Model::s_bExcludeSkyBox)
            if (R > Model::s_ExcludeRange*Model::s_ExcludeRange)
                continue;
        Radius = Max( Radius, R );
    }
    m_BoundingSphere = BoundingSphere( Center, Sqrt(Radius) );
}

void PmxModel::Private::SetupBoundingBox( void )
{
    Vector3 Center = m_Bones[m_RootBoneIndex].Translate;

    Vector3 MinV( FLT_MAX ), MaxV( FLT_MIN );
    for (auto& vert : m_VertexPos)
    {
        if (Model::s_bExcludeSkyBox)
        {
            Scalar R = Dot( Vector3( 1.f ), Abs( Center - Vector3( vert ) ) );
            if (R > Model::s_ExcludeRange)
                continue;
        }
        MinV = Min( MinV, vert );
        MaxV = Max( MaxV, vert );
    }

    m_BoundingBox = BoundingBox( MinV, MaxV );
}

void PmxModel::Private::SetupVisualizeSkeleton()
{
	auto numBone = m_Bones.size();

	m_BoneAttribute.resize( numBone );

	for ( auto i = 0; i < numBone; i++ )
	{
		auto DestinationIndex = m_Bones[i].DestinationIndex;
		Vector3 DestinationOffset = m_Bones[i].DestinationOffset;
		if (DestinationIndex >= 0)
			DestinationOffset = m_Bones[DestinationIndex].Position - m_Bones[i].Position;

		Vector3 diff = DestinationOffset;
		Scalar length = Length( diff );
		Quaternion Q = RotationBetweenVectors( Vector3( 0.0f, 1.0f, 0.0f ), diff );
		AffineTransform scale = AffineTransform::MakeScale( Vector3(0.05f, length, 0.05f) );
        // Move primitive bottom to origin
		AffineTransform alignToOrigin = AffineTransform::MakeTranslation( Vector3(0.0f, 0.5f * length, 0.0f) );
		m_BoneAttribute[i] = AffineTransform(Q, m_Bones[i].Position) * alignToOrigin * scale;
	}
}

void PmxModel::Private::Update( float kFrameTime )
{
	if (m_BoneMotions.size() > 0)
	{
		size_t numBones = m_BoneMotions.size();
		for (auto i = 0; i < numBones; i++)
			m_BoneMotions[i].Interpolate( kFrameTime, m_LocalPose[i] );

		for (auto i = 0; i < numBones; i++)
		{
			auto parentIndex = m_BoneParent[i];
			if (parentIndex < numBones)
				m_Pose[i] = m_Pose[parentIndex] * m_LocalPose[i];
			else
				m_Pose[i] = m_LocalPose[i];
		}
		for (auto i = 0; i < numBones; i++)
			m_Skinning[i] = m_Pose[i] * m_toRoot[i];

		for (auto i = 0; i < numBones; i++)
            m_SkinningDual[i] = m_Skinning[i];
	}

    if (m_MorphMotions.size() > 0)
	{
		//
		// http://blog.goo.ne.jp/torisu_tetosuki/e/8553151c445d261e122a3a31b0f91110
		//
        auto elemByte = sizeof( decltype(m_MorphDelta)::value_type );
        memset( m_MorphDelta.data(), 0, elemByte*m_MorphDelta.size() );

		bool bUpdate = false;
		for (auto i = kMorphBase+1; i < m_MorphMotions.size(); i++)
		{
			auto& motion = m_MorphMotions[i];
			motion.Interpolate( kFrameTime );
			if (std::fabsf( motion.m_WeightPre - motion.m_Weight ) < 0.1e-2)
				continue;
			bUpdate = true;
			auto weight = motion.m_Weight;
			for (auto k = 0; k < motion.m_MorphVertices.size(); k++)
			{
                auto idx = motion.m_MorphIndices[k];
				m_MorphDelta[idx] += weight * motion.m_MorphVertices[k];
			}
		}
		if (bUpdate)
		{
            auto& baseFace = m_MorphMotions[kMorphBase];
			for (auto i = 0; i < m_MorphDelta.size(); i++)
                m_MorphDelta[i] += baseFace.m_MorphVertices[i];

			for (auto i = 0; i < m_MorphDelta.size(); i++)
				XMStoreFloat3( &m_VertexMorphedPos[baseFace.m_MorphIndices[i]], m_MorphDelta[i]);

			m_PositionBuffer.Create( m_Name + L"_PosBuf",
				static_cast<uint32_t>(m_VertexMorphedPos.size()),
				sizeof( XMFLOAT3 ),
				m_VertexMorphedPos.data() );
		}
	}
}

PmxModel::PmxModel() :
    m_Context(std::make_shared<Private>())
{
}

void PmxModel::Clear()
{
    m_Context->Clear();
}

const std::vector<XMFLOAT3>& Rendering::PmxModel::GetVertices( void ) const
{
    return m_Context->GetVertices();
}

const std::vector<uint32_t>& Rendering::PmxModel::GetIndices( void ) const
{
    return m_Context->GetIndices();
}

void PmxModel::DrawColor( GraphicsContext& Context )
{
    m_Context->Draw( Context );
}

bool PmxModel::LoadFromFile( const std::wstring& FilePath )
{
    return m_Context->LoadFromFile( FilePath );
}

void PmxModel::SetVertices( const std::vector<XMFLOAT3>& vertices )
{
    m_Context->SetVertices( vertices );
}

