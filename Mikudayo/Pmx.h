//
// ZeroGram
// http://zerogram.info
//
// Pmx Parser MMDFormats
// https://github.com/oguna/MMDFormats
//
// Use code from 'MMDAI'
// Copyright (c) 2010-2014  hkrn
//
// Use code from 'MikuMikuFormats'
//
// PmxEditor PMX仕様.txt
//
#pragma once

#include <DirectXMath.h>
#include <vector>
#include <string>
#include "FileUtility.h"

namespace Pmx
{
    using DirectX::XMFLOAT2;
    using DirectX::XMFLOAT3;
    using DirectX::XMFLOAT4;
    using namespace Utility;
    using namespace std;

    using MagicBuf = char[4];
    using NameBuf = char[20];
    using FrameBuf = char[50];
    using CommentBuf = char[256];
    using ToonBuf = char[100];

    enum {
        kEncodingUtf16 = 0,
        kEncodingUtf8,
    };

    enum EConfigFlag {
        kEncoding = 0, // encoding setting | 0:UTF16, 1:UTF8
        kAddUV, // additional UV number | 0~4
        kVertIndex, // vertex index byte size | 1,2,4
        kTexIndex, // texture index byte size | 1,2,4
        kMatIndex, // material index byte size | 1,2,4
        kBoneIndex, // bone index byte size | 1,2,4
        kMorphIndex, // morph index byte size | 1,2,4
        kRigidBodyIndex, // rigid body index size | 1,2,4
    };

    struct Header
    {
        float Version; // (2.0/2.1)
        void Fill( bufferstream& is );
    };

    struct Config
    {
        uint8_t Count;
        enum { kMaxConfig = 8 };
        uint8_t Data[kMaxConfig];
        void Fill( bufferstream& is );
    };

    struct Description
    {
        wstring Name;
        wstring Comment;
        wstring NameEnglish;
        wstring CommentEnglish;

        void Fill( bufferstream& is, bool bUtf16 );
    };

    struct Bdef1Unit {
        int32_t BoneIndex;
        void Fill( bufferstream& is, uint8_t byteSize );
    };

    struct Bdef2Unit {
        int32_t BoneIndex[2];
        float Weight;
        void Fill( bufferstream& is, uint8_t byteSize );
    };

    struct Bdef4Unit {
        int32_t BoneIndex[4];
        float Weight[4];
        void Fill( bufferstream& is, uint8_t byteSize );
    };

    struct SdefUnit {
        int32_t BoneIndex[2];
        float Weight;
        float C[3];
        float R0[3];
        float R1[3];
        void Fill( bufferstream& is, uint8_t byteSize );
    };

    struct QdefUnit {
        int32_t BoneIndex[4];
        float Weight[4];
        void Fill( bufferstream& is, uint8_t byteSize );
    };

    struct Vertex
    {
        enum { kMaxAddUV = 4 };
        enum ESkiningType  : uint8_t
        { kBdef1 = 0, kBdef2, kBdef4, kSdef, kQdef, kMaxType };

        XMFLOAT3 Pos;
        XMFLOAT3 Normal;
        XMFLOAT2 UV;
        XMFLOAT4 AddUV[kMaxAddUV];

        ESkiningType SkinningType;
        union {
            Bdef1Unit bdef1;
            Bdef2Unit bdef2;
            Bdef4Unit bdef4;
            SdefUnit sdef;
            QdefUnit qdef;
        };
        float EdgeSize;

        void Fill( bufferstream & is, bool bRH, uint8_t numAddUV, uint8_t boneByteSize );
    };

	enum EMaterialFlag : uint8_t {
		kCullOff = 0x01,	// two face
		kGrdShadow = 0x02,	// ground shadow (地面影)
		kCastShadowMap = 0x04,	// セルフシャドウ（キャスタ）
		kEnableShadowMap = 0x08, // セルフシャドウ（レシーバ）
		kEnableEdge = 0x10,	// edge-outline (輪郭線)
        kEnableVertexColor = 0x20,
        kEnablePointDraw   = 0x40,
        kEnableLineDraw    = 0x80,
	};

    enum ESphereOpeation : uint8_t
    {
        kNone = 0,
        kMultiply, // sph
        kPlus, // spa
        kSub,  // sub
    };

    struct Material
    {
        wstring Name;
        wstring NameEnglish;

        XMFLOAT4 Diffuse;
        XMFLOAT3 Specular;
        float SpecularPower;
        XMFLOAT3 Ambient;
        uint8_t BitFlag; // (EMaterialFlag)
        XMFLOAT4 EdgeColor;
        float EdgeSize;
        int32_t DiffuseTexureIndex; // diffuse texture index
        int32_t SphereTextureIndex; // sphere texture index
        ESphereOpeation SphereOperation;
        uint8_t bDefaultToon;
        int32_t Toon; // toon texture index (when bDefaultToon == 0)
        int8_t DeafultToon; // default provided toon index
        wstring Comment;
        uint32_t NumVertex;

        void Fill( bufferstream & is, bool bUtf16, uint8_t textureByteSize );
    };

    struct IkLink
    {
        int32_t BoneIndex;
        uint8_t bLimit;
        XMFLOAT3 MinLimit;
        XMFLOAT3 MaxLimit;

        IkLink();
        void Fill( bufferstream& is, bool bRH, uint8_t boneIndexByteSize );
    };

    struct IK
    {
        int32_t BoneIndex;
        int32_t NumIteration;
        float LimitedRadian;
        std::vector<IkLink> Link;

        IK();
        void Fill( bufferstream& is, bool bRH, uint8_t boneIndexByteSize );
    };

    struct Bone
    {
        wstring Name;
        wstring NameEnglish;
        XMFLOAT3 Position;
        int32_t ParentBoneIndex;
        uint32_t MoprhHierarchy; // deform, layer index
        uint16_t BitFlag;

        // Link to
        int32_t DestinationOriginIndex; // if BitFlag & kHasDestinationOriginIndex, given by bone index
        XMFLOAT3 DestinationOriginOffset; // else given by offset

        // Additional bias bone (Inherent Rotation | Inherent Translation) from parent
        bool bInherentRotation = false;
        bool bInherentTranslation = false;
        int32_t ParentInherentBoneIndex;
        float ParentInherentBoneCoefficent;

        bool bFixedAxis;
        XMFLOAT3 FixedAxis;

        bool bLocalAxes = true;
        XMFLOAT3 LocalAxisX;
        XMFLOAT3 LocalAxisZ;

        int32_t ExtParentIndex;

        bool bIK;
        IK Ik;

        Bone();
        void Fill( bufferstream & is, bool bRH, bool bUtf16, uint8_t boneIndexByteSize );
    };

	enum class MorphCategory : uint8_t
	{
		kReservedCategory = 0,
		kEyebrow = 1,
		kEye = 2,
		kMouth = 3,
		kOther = 4,
	};

    enum class MorphType : uint8_t
    {
	    kGroup = 0,
	    kVertex = 1,
	    kBone = 2,
	    kTexCoord = 3,
	    kExtraUV1 = 4,
	    kExtraUV2 = 5,
	    kExtraUV3 = 6,
	    kExtraUV4 = 7,
	    kMaterial = 8,
        kFlip = 9,
        kImpulse = 10,
    };

    struct MorphGroup
    {
        uint32_t Index;
        float Weight;
        void Fill( bufferstream& is, uint8_t size );
    };

    struct MorphVertex
    {
        uint32_t VertexIndex;
        XMFLOAT3 Position;

        void Fill( bufferstream& is, uint8_t size, bool bRH );
    };

    struct MorphMaterial
    {
        uint32_t MaterialIndex;
        uint8_t OffsetOperation;
        XMFLOAT4 Diffuse;
        XMFLOAT3 Specular;
        float SpecularPower;
        XMFLOAT3 Ambient;
        XMFLOAT4 EdgeColor;
        float EdgeSize;
        XMFLOAT4 TextureWeight;
        XMFLOAT4 SphereWeight;
        XMFLOAT4 ToonWeight;

        void Fill( bufferstream& is, uint8_t size );
    };

    struct MorphBone
    {
        uint32_t BoneIndex;
        XMFLOAT3 Translation;
        XMFLOAT4 Rotation;

        void Fill( bufferstream& is, uint8_t size, bool bRH );
    };

    struct MorphUV
    {
        uint32_t VertexIndex;
        XMFLOAT4 Position;
        uint8_t Offset;

        void Fill( bufferstream& is, uint8_t offset, uint8_t size );
    };

    struct MorphFlip
    {
        uint32_t Index;
        float Value;

        void Fill( bufferstream& is, uint8_t size );
    };

    struct MorphImpulse
    {
        uint32_t Index;
        bool bLocal;
        XMFLOAT3 Velocity;
        XMFLOAT3 AngularTorque;

        void Fill( bufferstream& is, uint8_t size, bool bRH );
    };

    //
    // Each value is given as offset from original one
    //
    struct Morph
    {
        wstring Name;
        wstring NameEnglish;
        MorphCategory Panel;
        MorphType Type;
        uint16_t MorphIndex;
        float MorphRate;

        vector<MorphGroup> GroupList;
        vector<MorphVertex> VertexList;
        vector<MorphBone> BoneList;
        vector<MorphUV> TexCoordList;
        vector<MorphMaterial> MaterialList;
        vector<MorphFlip> FlipList;
        vector<MorphImpulse> ImpulseList;

        void Fill( bufferstream& is, bool bUtf16, bool bRH, uint8_t config[] );
    };

    enum class DisplayElementType  : uint8_t
    {
        kBone,
        kMorph
    };

    struct DisplayElement
    {
        DisplayElementType Type;
        uint32_t Index;

        void Fill( bufferstream& is, uint8_t config[] );
    };

    struct DisplayFrame
    {
        wstring Name;
        wstring NameEnglish;
        uint8_t Type;
        vector<DisplayElement> ElementList;

        void Fill( bufferstream& is, bool bUtf16, uint8_t config[] );
    };

    enum class RigidBodyShape : uint8_t
    {
        kSphere = 0,
        kBox,
        kCapsule
    };

    enum class RigidBodyType : uint8_t
    {
        kBoneConnected, // Follow Bone (static)
        kPhysics, // Physics Calc. (dynamic)
        kConnectedPhysics //  Physics Calc. + Bone position matching
    };

    struct RigidBody
    {
        wstring Name;
        wstring NameEnglish;
        uint32_t BoneIndex;
        uint8_t CollisionGroupID; // (group that start from 0)
        uint16_t CollisionGroupMask; // (~non-collision group)
        RigidBodyShape Shape;
        XMFLOAT3 Size; // (raidus, height, ?)
        XMFLOAT3 Position;
        XMFLOAT3 Rotation; // Orientation
        float Mass;
        float LinearDamping; // MoveAttenuation (Move)
        float AngularDamping; // RotationAttenuation (Rotation)
        float Restitution; // Repulsion (Repel)
        float Friction;
        RigidBodyType RigidType;

        void Fill( bufferstream& is, bool bRH, bool bUtf16, uint8_t boneIndexSize );
    };

	enum class JointType : uint8_t
	{
		kGeneric6DofSpring = 0,
		kGeneric6Dof = 1,
		kPoint2Point = 2,
		kConeTwist = 3,
		kSlider = 5,
		kHinge = 6
	};

    struct Joint
    {
        wstring Name;
        wstring NameEnglish;
        JointType Type;
        uint32_t RigidBodyIndexA;
        uint32_t RigidBodyIndexB;
        XMFLOAT3 Position;
        XMFLOAT3 Rotation;
        XMFLOAT3 LinearLowerLimit; // MoveLimitationMin (Constraint Position)(limits move)
        XMFLOAT3 LinearUpperLimit;
        XMFLOAT3 AngularLowerLimit; // RotationLimitationMin (Constraint Rotation)(limits rot)
        XMFLOAT3 AngularUpperLimit;
        XMFLOAT3 LinearStiffness; // SpringMoveCoefficient (spring move)
        XMFLOAT3 AngularStiffness; // SpringRotationCoefficient (spring rotation)

        void Fill( bufferstream& is, bool bRH, bool bUtf16, uint8_t rigidIndexSize );
    };

	enum kSoftBodyFlag : uint8_t
	{
		kBLink = 0x01,
		kCluster = 0x02,
		kLink = 0x04
	};

	class RigidBodyAnchor
	{
	public:
		int32_t RelatedRigidBody;
		int32_t RelatedVertex;
		bool bNear;

        void Fill( bufferstream& is, bool bRH, bool bUtf16, uint8_t rigidIndexSize );
	};

    struct SoftBody
    {
        wstring Name;
        wstring NameEnglish;
        uint8_t Shape;
        uint32_t TargetMaterial;
        uint8_t SoftBodyGroup;
        uint16_t UnCollisionGroupFlag;
        kSoftBodyFlag Flag;
		int32_t BlinkDistance;
		int32_t ClusterCount;
		float Mass;
		float CollisioniMargin;
		int32_t AeroModel;
		float VCF;
		float DP;
		float DG;
		float LF;
		float PR;
		float VC;
		float DF;
		float MT;
		float CHR;
		float KHR;
		float SHR;
		float AHR;
		float SRHR_CL;
		float SKHR_CL;
		float SSHR_CL;
		float SR_SPLT_CL;
		float SK_SPLT_CL;
		float SS_SPLT_CL;
		int32_t V_IT;
		int32_t P_IT;
		int32_t D_IT;
		int32_t C_IT;
		float LST;
		float AST;
		float VST;
        std::vector<RigidBodyAnchor> Anchors;
        std::vector<int32_t> PinVertices;

        void Fill( bufferstream& is, bool bUtf16, bool bRH, uint8_t rigidIndexSize );
    };

    // Polygon Model eXtended
    class PMX
    {
    public:
        bool isUtf16() const;
        uint8_t GetNumAddUV() const;
        uint8_t GetByteSize( EConfigFlag flag ) const;

        // PMX model is defined in left handed coordinate
        // 'bRightHand' flag convert model to right handed coordinate
        void Fill( bufferstream& is, bool bRightHand );

        bool IsValid( void ) const { return m_IsValid; }
        bool m_IsValid = false;

        MagicBuf m_Magic;
        Header m_Header;
        Config m_Config;
        Description m_Description;
        vector<Vertex> m_Vertices;
        vector<uint32_t> m_Indices;
        vector<wstring> m_Textures;
        vector<Material> m_Materials;
        vector<Bone> m_Bones;
        vector<Morph> m_Morphs;
        vector<DisplayFrame> m_Frames;
        vector<RigidBody> m_RigidBodies;
        vector<Joint> m_Joints;
        // Version >= 2.1f
        vector<SoftBody> m_SoftBodies;
    };

    inline bool PMX::isUtf16() const
    {
        return m_Config.Data[kEncoding] == kEncodingUtf16;
    }

    inline uint8_t PMX::GetNumAddUV() const
    {
        return m_Config.Data[kAddUV];
    }

    inline uint8_t PMX::GetByteSize(EConfigFlag flag) const
    {
        return m_Config.Data[flag];
    }
}

