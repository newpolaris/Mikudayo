#include "Pmd.h"
#include "Encoding.h"
#include "TextUtility.h"

namespace Pmd
{
	using Utility::bufferstream;
	using Utility::sjis_to_utf;

	void Header::Fill( bufferstream & is )
	{
		File File;
		Read( is, File.FileHeader );
		Read( is, File.Version );
		Read( is, File.Name );
		Read( is, File.Comment );

		Version = File.Version;
		Name = sjis_to_utf( File.Name );
		Comment = sjis_to_utf( File.Comment );
	}

	void Header::FillExpantion( bufferstream& is )
	{
		File File;
		Read( is, File.Name );
		Read( is, File.Comment );
		NameEnglish = ascii_to_utf( File.Name );
		CommentEnglish = ascii_to_utf( File.Comment );
	}

	void Vertex::Fill( bufferstream & is, bool bRH )
	{
		ReadPosition( is, Pos, bRH );
		ReadNormal( is, Normal, bRH );
		Read( is, UV );
		Read( is, Bone_id );
		Read( is, Bone_weight );
		Read( is, Edge_flat );
	}

	void Material::Fill( bufferstream & is )
	{
		Read( is, Diffuse );
		Read( is, SpecularPower );
		Read( is, Specular );
		Read( is, Ambient );
		Read( is, ToonIndex );
		Read( is, EdgeFlag );
		Read( is, FaceVertexCount );

		NameBuf TextureName;
		Read( is, TextureName );
		std::string NameRaw = read_default( TextureName );
		auto List = Utility::split(NameRaw, "[^/*]+");
		for (auto l : List) {
			if (std::string::npos != l.rfind(".sp"))
				SphereRaw = l;
			else
				TextureRaw = l;
		}

		SphereOperation = kNone;
		if (!SphereRaw.empty())
		{
			if (std::string::npos != SphereRaw.rfind(".spa"))
				SphereOperation = kPlus;
			else
				SphereOperation = kMultiply;
		}

		Sphere = sjis_to_utf( SphereRaw );
		Texture = sjis_to_utf( TextureRaw );
	}

	void Bone::Fill( bufferstream & is, bool bRH )
	{
		NameBuf BoneName;
		Read( is, BoneName );
		Name = sjis_to_utf( BoneName );

		Read( is, ParentBoneIndex );
		Read( is, ChildBoneIndex );
		Read( is, Type );
		Read( is, IkParentBoneIndex );
		ReadPosition( is, BoneHeadPosition, bRH );
	}

	void Bone::FillExpantion( bufferstream & is )
	{
		NameBuf BoneName;
		Read( is, BoneName );
		NameEnglish = ascii_to_utf( BoneName );
	}

	void IK::Fill( bufferstream & is )
	{
		Read( is, IkBoneIndex );
		Read( is, IkTargetBonIndex );
		uint8_t IkLinkCount;
		Read( is, IkLinkCount );
		Read( is, IkNumIteration );
		Read( is, IkLimitedRadian );

		IkLinkBondIndexList.resize( IkLinkCount );
		for (uint8_t i = 0; i < IkLinkCount; i++)
			Read( is, IkLinkBondIndexList[i] );
	}

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

	void PMD::Fill( bufferstream& is, bool bRightHand )
	{
		m_Header.Fill( is );

		uint32_t NumVertex = ReadInt( is );
		m_Vertices.resize( NumVertex );
		for (uint32_t i = 0; i < NumVertex; i++) 
			m_Vertices[i].Fill( is, bRightHand );

		uint32_t NumIndices = ReadInt( is );
		m_Indices.resize( NumIndices );
		Read( is, m_Indices );
		if (bRightHand)
		{
			for (uint32_t i = 0; i < NumIndices; i += 3)
				std::swap( m_Indices[i], m_Indices[i + 1] );
		}

		uint32_t NumMaterial = ReadInt( is );
		m_Materials.resize( NumMaterial );
		for (uint32_t i = 0; i < NumMaterial; i++)
			m_Materials[i].Fill( is );

		uint16_t NumBones = ReadShort( is );
		m_Bones.resize( NumBones );
		for (uint16_t i = 0; i < NumBones; i++)
		{
			m_Bones[i].Fill( is, bRightHand );
		}

		uint16_t NumIK = ReadShort( is );
		m_IKs.resize( NumIK );
		for (uint32_t i = 0; i < NumIK; i++)
			m_IKs[i].Fill( is );

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

		Read( is, m_bEnglishSupport );
		if (m_bEnglishSupport)
		{
			m_Header.FillExpantion( is );
			for (uint16_t i = 0; i < NumBones; i++)
				m_Bones[i].FillExpantion( is );
			for (uint32_t i = 0; i < NumFaces; i++)
			{
				if (m_Faces[i].Type == FaceType::kBase)
					continue;
				m_Faces[i].FillExpantion( is );
			}
			m_BoneDisplayNameList.FillExpantion( is );
		}

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
	}
}
