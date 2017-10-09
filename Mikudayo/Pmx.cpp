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
        sdef = {};

        switch (SkinningType)
        {
        case kBdef1: bdef1.Fill( is, boneByteSize ); break;
        case kBdef2: bdef2.Fill( is, boneByteSize ); break;
        case kBdef4: bdef4.Fill( is, boneByteSize ); break;
        case kSdef: sdef.Fill( is, boneByteSize ); break;
        case kQdef: qdef.Fill( is, boneByteSize ); break;
        default: ASSERT(FALSE); break;
        }

        Read( is, EdgeSize );
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

    void Bone::Fill( bufferstream& is, bool bRH, bool bUtf16, uint8_t boneIndexByteSize  )
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
            bInherentTranslation = BitFlag & kHasInherentTranslation;
            bInherentRotation = BitFlag & kHasInherentRotation;
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
/*
	void Face::FaceVertex::Fill( bufferstream & is, bool bRH )
	{
		Read( is, Index );
		ReadPosition( is, Position, bRH );
	}

	void Face::Fill( bufferstream & is, bool bRH )
	{
		NameBuf SkinName;
		Read( is, SkinName );
		uint32_t VertexCount;
		Read( is, VertexCount );
		Read( is, Type );
		FaceVertices.resize( VertexCount );
		for (uint32_t i = 0; i < VertexCount; i++)
			FaceVertices[i].Fill( is, bRH );

		Name = sjis_to_utf( SkinName );
	}

	void Face::FillExpantion( bufferstream & is )
	{
		NameBuf SkinName;
		Read( is, SkinName );
		NameEnglish = ascii_to_utf( SkinName );
	}

	void BoneDisplayNameList::Fill( bufferstream & is )
	{
		Read( is, BoneNameCount );
		for (uint8_t i = 0; i < BoneNameCount; i++) {
			FrameBuf buf;
			Read( is, buf );
			Name.push_back( sjis_to_utf( buf ) );
		}
	}

	void BoneDisplayNameList::FillExpantion( bufferstream & is )
	{
		for (uint8_t i = 0; i < BoneNameCount; i++) {
			FrameBuf buf;
			Read( is, buf );
			NameEnglish.push_back( ascii_to_utf( buf ) );
		}
	}

	void BoneDisplayFrame::Fill( bufferstream & is )
	{
		Read( is, BoneIndex );
		Read( is, BoneDisplayNameIndex );
	}

	void RigidBody::Fill( bufferstream & is, bool bRH )
	{
		NameBuf Buffer;
		Read( is, Buffer );
		Name = sjis_to_utf( Buffer );

		Read( is, BoneIndex );
		Read( is, RigidBodyGroup );
		Read( is, UnCollisionGroupFlag );
		Read( is, Type );
		Read( is, Size );
		ReadPosition( is, Position, bRH );
		ReadRotation( is, Rotation, bRH );
		Read( is, Weight );
		Read( is, LinearDamping );
		Read( is, AngularDamping );
		Read( is, Restitution );
		Read( is, Friction );
		Read( is, RigidType );
	}

	void Constraint::Fill( bufferstream & is, bool bRH )
	{
		NameBuf Buffer;
		Read( is, Buffer );
		Name = sjis_to_utf( Buffer );

		Read( is, RigidBodyIndexA );
		Read( is, RigidBodyIndexB );
		ReadPosition( is, Position, bRH );
		ReadRotation( is, Rotation, bRH );
		ReadPosition( is, LinearLowerLimit, bRH );
		ReadPosition( is, LinearUpperLimit, bRH );
		ReadRotation( is, AngularLowerLimit, bRH );
		ReadRotation( is, AngularUpperLimit, bRH );
		ReadPosition( is, LinearStiffness, bRH );
		ReadRotation( is, AngularStiffness, bRH );
	}
*/
    void PMX::Fill( bufferstream& is, bool bRightHand )
	{
        m_IsValid = false;

		Read( is, m_Magic );

        // In PMX 1.0 magic is "pmx ", so judge by using ignore case compare
		if (_strnicmp( m_Magic, "PMX ", 4 ))
		{
			std::cerr << "invalid pmx file." << std::endl;
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

        /*
		uint16_t NumFaces = ReadShort( is );
		m_Faces.resize( NumFaces );
		for (uint16_t i = 0; i < NumFaces; i++)
			m_Faces[i].Fill( is, bRightHand );

		uint8_t NumFaceDislayList;
		Read( is, NumFaceDislayList );
		m_FaceDisplayList.resize( NumFaceDislayList );
		for (uint8_t i = 0; i < NumFaceDislayList; i++)
			Read( is, m_FaceDisplayList[i] );

		m_BoneDisplayNameList.Fill( is );

		uint32_t NumBoneDisplayFrame = Read<uint32_t>( is );
		m_BoneDisplayFrames.resize( NumBoneDisplayFrame );
		for (uint8_t i = 0; i < NumBoneDisplayFrame; i++)
			m_BoneDisplayFrames[i].Fill( is );


		if (is.peek() != ios::traits_type::eof())
		{
			static const int ToonNum = 10;
			ToonBuf Buf;
			for (int i = 0; i < ToonNum; i++)
			{
				Read( is, Buf );
				m_ToonTextureList.push_back( sjis_to_utf( Buf ) );
				m_ToonTextureRawList.push_back( read_default( Buf ) );
			}
		}

		if (is.peek() != ios::traits_type::eof())
		{
			uint32_t NumRigidBody;
			Read( is, NumRigidBody );
			m_Bodies.resize( NumRigidBody );
			for (uint32_t i = 0; i < NumRigidBody; i++)
				m_Bodies[i].Fill( is, bRightHand );

			uint32_t NumConstraint;
			Read( is, NumConstraint );
			m_Constraint.resize( NumConstraint );
			for (uint32_t i = 0; i < NumConstraint; i++)
				m_Constraint[i].Fill( is, bRightHand );
		}
        */
		m_IsValid = true;
	}
}

