#include "stdafx.h"

#include "Pmx.h"
#include "Encoding.h"
#include "TextUtility.h"

namespace Pmx
{
	using Utility::bufferstream;
	using Utility::sjis_to_utf;

    std::wstring ReadText( bufferstream& is, bool bUtf16 )
    {
        uint32_t len = 0;
        Read( is, len );
        std::vector<char> buf(len);
        Read( is, buf );
        if (bUtf16)
        {
            return std::wstring( (wchar_t*)buf.data(), (wchar_t*)(buf.data() + len) );
        }
        else
        {
            std::string str( buf.data(), buf.data() + len );
            std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8conv;
            return utf8conv.from_bytes( str );
        }
    }

    uint32_t ReadIndexUnsigned( bufferstream& is, uint8_t byteSize )
    {
		uint8_t i8;
		uint16_t i16;
		uint32_t i32;

        switch (byteSize)
        {
		case 1:
            Read( is, i8 );
            return i8;
		case 2:
            Read( is, i16 );
            return i16;
		case 4:
            Read( is, i32 );
            return i32;
		}
        return 0;
    }

    int32_t ReadIndex( bufferstream& is, uint8_t byteSize )
    {
		int8_t i8;
		int16_t i16;
		int32_t i32;

        switch (byteSize)
        {
		case 1:
            Read( is, i8 );
            return i8;
		case 2:
            Read( is, i16 );
            return i16;
		case 4:
            Read( is, i32 );
            return i32;
		}
        return 0;
    }

	void Header::Fill( bufferstream& is )
	{
		Read( is, Version );
    }

    void Config::Fill( bufferstream& is )
    {
        Read( is, Count );
        Read( is, Data, Count );
    }

    void Description::Fill( bufferstream& is, bool bUtf16 )
    {
		Name = ReadText( is, bUtf16 );
		Comment = ReadText( is, bUtf16 );
		NameEnglish = ReadText( is, bUtf16 );
		CommentEnglish = ReadText( is, bUtf16 );
    }

	void Vertex::Fill( bufferstream& is, bool bRH, uint8_t numAddUV, uint8_t boneByteSize )
	{
		ReadPosition( is, Pos, bRH );
		ReadNormal( is, Normal, bRH );
		Read( is, UV );

        for (uint8_t i = 0; i < numAddUV; i++)
            Read( is, AddUV[i] );

		Read( is, SkinningType );

        // clear union
        Unit.sdef = {};

        switch (SkinningType)
        {
        case kBdef1: Unit.bdef1.Fill( is, boneByteSize ); break;
        case kBdef2: Unit.bdef2.Fill( is, boneByteSize ); break;
        case kBdef4: Unit.bdef4.Fill( is, boneByteSize ); break;
        case kSdef: Unit.sdef.Fill( is, boneByteSize ); break;
        case kQdef: Unit.qdef.Fill( is, boneByteSize ); break;
        default: ASSERT(FALSE); break;
        }

        Read( is, EdgeScale );
	}

    void Bdef1Unit::Fill( bufferstream& is, uint8_t byteSize )
    {
        BoneIndex = ReadIndex( is, byteSize );
    }

    void Bdef2Unit::Fill( bufferstream& is, uint8_t byteSize )
    {
        BoneIndex[0] = ReadIndex( is, byteSize );
        BoneIndex[1] = ReadIndex( is, byteSize );
        Read( is, Weight );
    }

    void Bdef4Unit::Fill( bufferstream& is, uint8_t byteSize )
    {
        BoneIndex[0] = ReadIndex( is, byteSize );
        BoneIndex[1] = ReadIndex( is, byteSize );
        BoneIndex[2] = ReadIndex( is, byteSize );
        BoneIndex[3] = ReadIndex( is, byteSize );
        Read( is, Weight );
    }

    void SdefUnit::Fill( bufferstream& is, uint8_t byteSize )
    {
        BoneIndex[0] = ReadIndex( is, byteSize );
        BoneIndex[1] = ReadIndex( is, byteSize );
        Read( is, Weight );
        Read( is, C );
        Read( is, R0 );
        Read( is, R1 );
    }

    void QdefUnit::Fill( bufferstream& is, uint8_t byteSize )
    {
        BoneIndex[0] = ReadIndex( is, byteSize );
        BoneIndex[1] = ReadIndex( is, byteSize );
        BoneIndex[2] = ReadIndex( is, byteSize );
        BoneIndex[3] = ReadIndex( is, byteSize );
        Read( is, Weight );
    }

	void Material::Fill( bufferstream& is, bool bUtf16, uint8_t textureIndexByteSize )
	{
        Name = ReadText( is, bUtf16 );
        NameEnglish = ReadText( is, bUtf16 );

		Read( is, Diffuse );
		Read( is, Specular );
		Read( is, SpecularPower );
		Read( is, Ambient );
		Read( is, BitFlag );
		Read( is, EdgeColor );
		Read( is, EdgeSize );
        DiffuseTexureIndex = ReadIndex( is, textureIndexByteSize );
        SphereTextureIndex = ReadIndex( is, textureIndexByteSize );
        Read( is, SphereOperation );
        Read( is, bDefaultToon );

        Toon = -1;
        DeafultToon = -1;
        if (bDefaultToon)
            Read( is, DeafultToon);
        else
            Toon = ReadIndex( is, textureIndexByteSize );

        Comment = ReadText( is, bUtf16 );

        Read( is, NumVertex );
	}

	enum {
        kHasDestinationOriginIndex = 0x1,
        kRotatable                 = 0x2,
        kMovable                   = 0x4,
        kVisible                   = 0x8,
        kInteractive               = 0x10,
        kHasInverseKinematics      = 0x20,
        kHasInherentRotation       = 0x100,
        kHasInherentTranslation    = 0x200,
        kHasFixedAxis              = 0x400,
        kHasLocalAxes              = 0x800,
        kTransformAfterPhysics     = 0x1000,
        kTransformByExternalParent = 0x2000
	};

    Bone::Bone()
    {
        Position = XMFLOAT3( 0.f, 0.f, 0.f );
        ParentBoneIndex = -1;

        DestinationOriginIndex = -1;
        DestinationOriginOffset = XMFLOAT3( 0.f, 0.f, 0.f );

        ParentInherentBoneIndex = -1;
        ParentInherentBoneCoefficent = 0.f;

        bFixedAxis = false;
        FixedAxis = XMFLOAT3( 0.f, 0.f, 0.f );

        bLocalAxes = false;
        LocalAxisX = XMFLOAT3( 0.f, 0.f, 0.f );
        LocalAxisZ = XMFLOAT3( 0.f, 0.f, 0.f );

        ExtParentIndex = -1;

        bIK = false;
    }

    void Bone::Fill( bufferstream& is, bool bRH, bool bUtf16, uint8_t boneIndexByteSize )
	{
        Name = ReadText( is, bUtf16 );
        NameEnglish = ReadText( is, bUtf16 );
        ReadPosition( is, Position, bRH );
		ParentBoneIndex = ReadIndex( is, boneIndexByteSize );
		Read( is, MoprhHierarchy );
		Read( is, BitFlag );

        // bone has destination
        if (BitFlag & kHasDestinationOriginIndex)
            DestinationOriginIndex = ReadIndex( is, boneIndexByteSize );
        else
            ReadPosition( is, DestinationOriginOffset, bRH );

        // bone has additional bias
        if (BitFlag & kHasInherentRotation || BitFlag & kHasInherentTranslation)
        {
            bInherentTranslation = (BitFlag & kHasInherentTranslation) != 0;
            bInherentRotation = (BitFlag & kHasInherentRotation) != 0;
            ParentInherentBoneIndex = ReadIndex( is, boneIndexByteSize );
            Read( is, ParentInherentBoneCoefficent );
        }

        // axis of bone is fixed
        if (BitFlag & kHasFixedAxis)
        {
            bFixedAxis = true;
            Read( is, FixedAxis );
        }

        // axis of bone is local
        if (BitFlag & kHasLocalAxes)
        {
            bLocalAxes = true;
            Read( is, LocalAxisX );
            Read( is, LocalAxisZ );
        }

        // bone is transformed after external parent bone transformation
        if (BitFlag & kTransformByExternalParent)
            Read( is, ExtParentIndex );

        // bone is IK
        if (BitFlag & kHasInverseKinematics)
        {
            bIK = true;
            Ik.Fill( is, bRH, boneIndexByteSize );
        }
	}

    IK::IK()
    {
        BoneIndex = -1;
        NumIteration = 0;
        LimitedRadian = 0.f;
    }

    void IK::Fill( bufferstream& is, bool bRH, uint8_t boneIndexByteSize )
    {
        BoneIndex = ReadIndex( is, boneIndexByteSize );
        Read( is, NumIteration );
        Read( is, LimitedRadian );
        uint32_t NumLink;
        Read( is, NumLink );
        Link.resize( NumLink );
        for (auto& l : Link)
            l.Fill( is, bRH, boneIndexByteSize );
    }

    IkLink::IkLink()
    {
        BoneIndex = -1;
        bLimit = false;
        MinLimit = XMFLOAT3( 0.f, 0.f, 0.f );
        MaxLimit = XMFLOAT3( 0.f, 0.f, 0.f );
    }

    void IkLink::Fill( bufferstream& is, bool bRH, uint8_t boneIndexByteSize )
    {
        BoneIndex = ReadIndex( is, boneIndexByteSize );
        Read( is, bLimit );
        if (bLimit) {
            ReadRotation( is, MinLimit, bRH );
            ReadRotation( is, MaxLimit, bRH );
        }
    }

    void MorphGroup::Fill( bufferstream& is, uint8_t size )
    {
		Index = ReadIndex( is, size );
		Read( is, Weight );
    }

	void MorphVertex::Fill( bufferstream& is, uint8_t size, bool bRH )
	{
		VertexIndex = ReadIndex( is, size );
		ReadPosition( is, Position, bRH );
	}

    void MorphMaterial::Fill( bufferstream& is, uint8_t size )
    {
		MaterialIndex = ReadIndex( is, size );
        Read( is, OffsetOperation );
        Read( is, Diffuse );
        Read( is, Specular );
        Read( is, SpecularPower );
        Read( is, Ambient );
        Read( is, EdgeColor );
        Read( is, EdgeSize );
        Read( is, TextureWeight );
        Read( is, SphereWeight );
        Read( is, ToonWeight );
    }

    void MorphBone::Fill( bufferstream& is, uint8_t size, bool bRH )
    {
		BoneIndex = ReadIndex( is, size );
        ReadPosition( is, Translation, bRH );
        ReadRotation( is, Rotation, bRH );
    }

    void MorphUV::Fill( bufferstream& is, uint8_t offset, uint8_t size )
    {
        Offset = offset;
        VertexIndex = ReadIndex( is, size );
        Read( is, Position );
    }

    void MorphFlip::Fill( bufferstream& is, uint8_t size )
    {
        Index = ReadIndex( is, size );
        Read( is, Value );
    }

    void MorphImpulse::Fill( bufferstream& is, uint8_t size, bool bRH )
    {
        Index = ReadIndex( is, size );
        uint8_t isLocal;
        Read( is, isLocal );
        bLocal = isLocal ? 1 : 0;
        ReadPosition( is, Velocity, bRH );
        ReadRotation( is, AngularTorque, bRH );
    }

    void Morph::Fill( bufferstream& is, bool bRH, bool bUtf16, uint8_t config[] )
    {
		Name = ReadText( is, bUtf16 );
        NameEnglish = ReadText( is, bUtf16 );
		Read( is, Panel );
		Read( is, Type );
		uint32_t MorphCount;
		Read( is, MorphCount );
        switch (Type)
        {
        case MorphType::kGroup:
            GroupList.resize( MorphCount );
            for (uint32_t i = 0; i < MorphCount; i++)
                GroupList[i].Fill( is, config[kMorphIndex] );
            break;
        case MorphType::kVertex:
            VertexList.resize( MorphCount );
            for (uint32_t i = 0; i < MorphCount; i++)
                VertexList[i].Fill( is, config[kVertIndex], bRH );
            break;
        case MorphType::kBone:
            BoneList.resize( MorphCount );
            for (uint32_t i = 0; i < MorphCount; i++)
                BoneList[i].Fill( is, config[kBoneIndex], bRH );
            break;
        case MorphType::kMaterial:
            MaterialList.resize( MorphCount );
            for (uint32_t i = 0; i < MorphCount; i++)
                MaterialList[i].Fill( is, config[kMatIndex] );
            break;
        case MorphType::kTexCoord:
        case MorphType::kExtraUV1:
        case MorphType::kExtraUV2:
        case MorphType::kExtraUV3:
        case MorphType::kExtraUV4:
        {
            TexCoordList.resize( MorphCount );
            const uint8_t offset = (uint8_t)Type - (uint8_t)MorphType::kTexCoord;
            for (uint32_t i = 0; i < MorphCount; i++)
                TexCoordList[i].Fill( is, offset, config[kVertIndex] );
            break;
        }
        default:
            ASSERT( FALSE );
        }
    }

    void DisplayElement::Fill( bufferstream& is, uint8_t config[] )
    {
        Read( is, Type );
        if (Type == DisplayElementType::kBone)
            Index = ReadIndex( is, config[kBoneIndex] );
        else if (Type == DisplayElementType::kMorph)
            Index = ReadIndex( is, config[kMorphIndex] );
        else
            ASSERT( FALSE );
    }

    void DisplayFrame::Fill( bufferstream& is, bool bUtf16, uint8_t config[] )
    {
        Name = ReadText( is, bUtf16 );
        NameEnglish = ReadText( is, bUtf16 );
        Read( is, Type );
		uint32_t Count;
        Read( is, Count );
        ElementList.resize( Count );
        for (uint32_t i = 0; i < Count; i++)
            ElementList[i].Fill( is, config );
    }

	void RigidBody::Fill( bufferstream& is, bool bRH, bool bUtf16, uint8_t boneIndexSize )
	{
        Name = ReadText( is, bUtf16 );
        NameEnglish = ReadText( is, bUtf16 );
		BoneIndex = ReadIndex( is, boneIndexSize );
		Read( is, CollisionGroupID );
		Read( is, CollisionGroupMask );
		Read( is, Shape );
		Read( is, Size );
		ReadPosition( is, Position, bRH );
		ReadRotation( is, Rotation, bRH );
		// TODO: how to handle nan, inf more better way ?
		if (isnan(Rotation.x)) Rotation.x = 0.f;
		if (isnan(Rotation.y)) Rotation.y = 0.f;
		if (isnan(Rotation.z)) Rotation.z = 0.f;
		ASSERT(!isnan(Rotation.x));
		ASSERT(!isnan(Rotation.y));
		ASSERT(!isnan(Rotation.z));
		Read( is, Mass );
		Read( is, LinearDamping );
		Read( is, AngularDamping );
		Read( is, Restitution );
		Read( is, Friction );
		Read( is, RigidType );
	}

    void Joint::Fill( bufferstream& is, bool bRH, bool bUtf16, uint8_t rigidIndexSize )
    {
		Name = ReadText( is, bUtf16 );
		NameEnglish = ReadText( is, bUtf16 );
        Read( is, Type );
		RigidBodyIndexA = ReadIndex( is, rigidIndexSize );
		RigidBodyIndexB = ReadIndex( is, rigidIndexSize );
		ReadPosition( is, Position, bRH );
		ReadRotation( is, Rotation, bRH );
		Read( is, LinearLowerLimit );
		Read( is, LinearUpperLimit );
		Read( is, AngularLowerLimit );
		Read( is, AngularUpperLimit );
		Read( is, LinearStiffness );
		Read( is, AngularStiffness );
    }

    void RigidBodyAnchor::Fill( bufferstream& is, bool bRH, bool bUtf16, uint8_t rigidIndexSize )
    {
        (is), (bRH), (bUtf16), (rigidIndexSize);
        ASSERT( FALSE );
    }

    void SoftBody::Fill( bufferstream& is, bool bRH, bool bUtf16, uint8_t rigidIndexSize )
    {
        (is), (bRH), (bUtf16), (rigidIndexSize);
        ASSERT( FALSE );
    }

    void PMX::Fill( bufferstream& is, bool bRightHand )
	{
        m_IsValid = false;

		Read( is, m_Magic );

        // In PMX 1.0 magic is "pmx ", so judge by using ignore case compare
		if (_strnicmp( m_Magic, "PMX ", 4 ))
		{
			std::cerr << "Invalid PMX file." << std::endl;
			return;
		}

		m_Header.Fill( is );
        m_Config.Fill( is );

        m_Description.Fill( is, isUtf16());

		uint32_t NumVertex = ReadUint( is );
		m_Vertices.resize( NumVertex );
		for (uint32_t i = 0; i < NumVertex; i++)
			m_Vertices[i].Fill( is, bRightHand, GetNumAddUV(), GetByteSize(kBoneIndex) );

		uint32_t NumIndices = ReadUint( is );
		m_Indices.resize( NumIndices );
        for (auto& i : m_Indices)
            i = ReadIndexUnsigned( is, GetByteSize( kVertIndex ));
		if (bRightHand)
		{
			for (uint32_t i = 0; i < NumIndices; i += 3)
				std::swap( m_Indices[i], m_Indices[i + 1] );
		}

        uint32_t NumTexture = ReadUint( is );
        m_Textures.resize( NumTexture );
        for (auto& t : m_Textures)
            t = ReadText( is, isUtf16() );

		uint32_t NumMaterial = ReadUint( is );
		m_Materials.resize( NumMaterial );
		for (uint32_t i = 0; i < NumMaterial; i++)
			m_Materials[i].Fill( is, isUtf16(), GetByteSize( kTexIndex ) );

		uint32_t NumBones = ReadUint( is );
		m_Bones.resize( NumBones );
		for (uint32_t i = 0; i < NumBones; i++)
			m_Bones[i].Fill( is, bRightHand, isUtf16(), GetByteSize( kBoneIndex ) );

		uint32_t NumMorphs = ReadUint( is );
        m_Morphs.resize( NumMorphs );
        for (uint32_t i = 0; i < NumMorphs; i++)
            m_Morphs[i].Fill( is, bRightHand, isUtf16(), m_Config.Data );

		uint32_t NumFrames = ReadUint( is );
        m_Frames.resize( NumFrames );
        for (uint32_t i = 0; i < NumFrames; i++)
            m_Frames[i].Fill( is, isUtf16(), m_Config.Data );

        uint32_t NumRigidBody;
        Read( is, NumRigidBody );
        m_RigidBodies.resize( NumRigidBody );
        for (uint32_t i = 0; i < NumRigidBody; i++)
            m_RigidBodies[i].Fill( is, bRightHand, isUtf16(), GetByteSize( kBoneIndex ) );

        uint32_t NumJoint;
        Read( is, NumJoint );
        m_Joints.resize( NumJoint );
        for (uint32_t i = 0; i < NumJoint; i++)
            m_Joints[i].Fill( is, bRightHand, isUtf16(), GetByteSize( kRigidBodyIndex ) );

        if (is.peek() != ios::traits_type::eof())
        {
            ASSERT( m_Header.Version >= 2.1f );
            // Version >= 2.1f
            uint32_t NumSoftBody;
            Read( is, NumSoftBody );
            m_SoftBodies.resize( NumSoftBody );
            for (uint32_t i = 0; i < NumJoint; i++)
                m_SoftBodies[i].Fill( is, bRightHand, isUtf16(), GetByteSize( kRigidBodyIndex ) );
        }
		m_IsValid = true;
	}
}

