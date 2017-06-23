#pragma once

#include <DirectXMath.h>
#include <vector>
#include <string>
#include "FileUtility.h"

//
// The image path is decoded with the system default and shift-jis
// to support zip released with the wrong codepage
//
namespace Pmd
{
	using DirectX::XMFLOAT2;
	using DirectX::XMFLOAT3;
	using DirectX::XMFLOAT4;
	using namespace Utility;
	using namespace std;

	using NameBuf = char[20];
	using FrameBuf = char[50];
	using CommentBuf = char[256];
	using ToonBuf = char[100];

	struct Header
	{
		float Version;
		wstring Name;
		wstring Comment;
		// Expantions
		wstring NameEnglish;
		wstring CommentEnglish;

		void Fill( bufferstream& is );
		void FillExpantion( bufferstream& is );
	};

	struct Vertex
	{
		XMFLOAT3 Pos;
		XMFLOAT3 Normal;
		XMFLOAT2 UV;
		uint16_t Bone_id[2];	// bone number 1, 2
		uint8_t  Bone_weight;	// Effect to bone1 [0, 100], bone2 has 100 - bone1
		uint8_t  Edge_flat;		// Edge Flag (Determines if this vertex should be used to draw the edge line around the model) 

		void Fill( bufferstream& is, bool bRH );
	};

	enum ESphereOpeation : uint8_t
	{
		kNone = 0,
		kMultiply,
		kPlus
	};

	struct Material
	{
		XMFLOAT4 Diffuse;
		XMFLOAT3 Specular;
		float SpecularPower;
		XMFLOAT3 Ambient;
		uint8_t ToonIndex;       // toon??.bmp's index
		uint8_t EdgeFlag;        // Outline and shadow flag
		uint32_t FaceVertexCount;
		string TextureRaw;       // decode system default
		wstring Texture;         // decode shift-jis
		string SphereRaw;        // decode system default
		wstring Sphere;          // decode shift-jis
		ESphereOpeation SphereOperation;

		void Fill( bufferstream& is );
	};

	enum class BoneType : uint8_t
	{
		kRotation = 0,
		kRotationAndMove,
		kIkEffector,
		kUnknown,
		kIkEffectable,
		kRotationEffectable,
		kIkTarget,
		kInvisible,
		kTwist,
		kRotationMovement,
	};

	struct Bone
	{
		wstring Name;
		wstring NameEnglish;
		uint16_t ParentBoneIndex; // if not then (0xFFFF)(-1)
		uint16_t ChildBoneIndex;  // if not then (0xFFFF)(-1)
		BoneType Type;
		uint16_t IkParentBoneIndex; // if not then 0
		XMFLOAT3 BoneHeadPosition;

		void Fill( bufferstream& is, bool bRH );
		void FillExpantion( bufferstream& is );
	};

	struct IK
	{
		uint16_t IkBoneIndex;
		uint16_t IkTargetBonIndex;
		uint16_t IkNumIteration;
		float IkLimitedRadian;
		vector<uint16_t> IkLinkBondIndexList;

		void Fill( bufferstream& is );
	};

	enum class FaceType : uint8_t
	{
		kBase = 0,
		kEyebrow,
		kEye,
		kLip,
		kOther,
	};

	struct Face
	{
		struct FaceVertex
		{
			uint32_t Index;
			XMFLOAT3 Position;

			void Fill( bufferstream& is, bool bRH );
		};

		wstring Name;
		wstring NameEnglish;
		FaceType Type;
		vector<FaceVertex> FaceVertices;

		void Fill( bufferstream& is, bool bRH );
		void FillExpantion( bufferstream& is );
	};

	struct BoneDisplayNameList
	{
		uint8_t BoneNameCount;
		vector<wstring> Name;
		vector<wstring> NameEnglish;

		void Fill( bufferstream& is );
		void FillExpantion( bufferstream& is );
	};

	struct BoneDisplayFrame {
		uint16_t BoneIndex;
		uint8_t BoneDisplayNameIndex;

		void Fill( bufferstream& is );
	};

	enum class RigidBodyShape : uint8_t
	{
		kSphere = 0,
		kBox,
		kCapsule
	};

	enum class RigidBodyType : uint8_t
	{
		kBoneConnected,
		kPhysics,
		kConnectedPhysics
	};

	struct RigidBody
	{
		wstring Name;
		uint16_t BoneIndex; // If not then -1
		uint8_t RigidBodyGroup;
		uint16_t UnCollisionGroupFlag;
		RigidBodyShape Type;
		XMFLOAT3 Size;
		XMFLOAT3 Position;
		XMFLOAT3 Rotation; // Orientation
		float Weight; // Mass
		float LinearDamping; // MoveAttenuation
		float AngularDamping; // RotationAttenuation
		float Restitution; // Repulsion
		float Friction;
		RigidBodyType RigidType;

		void Fill( bufferstream& is, bool bRH );
	};

	struct Constraint // Joint
	{
		wstring Name;
		uint32_t RigidBodyIndexA;
		uint32_t RigidBodyIndexB;
		XMFLOAT3 Position;
		XMFLOAT3 Rotation;
		XMFLOAT3 LinearLowerLimit; // MoveLimitationMin (Constraint Pos)
		XMFLOAT3 LinearUpperLimit;
		XMFLOAT3 AngularLowerLimit; // RotationLimitationMin (Constraint Rotation)
		XMFLOAT3 AngularUpperLimit;
		XMFLOAT3 LinearStiffness; // springMoveCoefficient
		XMFLOAT3 AngularStiffness; // springRotationCoefficient

		void Fill( bufferstream& is, bool bRH );
	};

	class PMD
	{
	public:
		// PMD model is defined in left handed coordinate
		// 'bRightHand' flag convert model to right handed coordinate
		void Fill( bufferstream & is, bool bRightHand );

		bool IsValid( void ) const { return m_IsValid; }
		bool m_IsValid = false;

		Header m_Header;
		vector<Vertex> m_Vertices;
		vector<uint16_t> m_Indices;
		vector<Material> m_Materials;
		vector<Bone> m_Bones;
		vector<IK> m_IKs;
		vector<Face> m_Faces;
		vector<uint16_t> m_FaceDisplayList;
		BoneDisplayNameList m_BoneDisplayNameList;
		vector<BoneDisplayFrame> m_BoneDisplayFrames;
		uint8_t m_bEnglishSupport;
		vector<wstring> m_ToonTextureList;            // decode system default
		vector<string> m_ToonTextureRawList;          // decode shift-jis
		vector<RigidBody> m_Bodies;
		vector<Constraint> m_Constraint;
	};
}

