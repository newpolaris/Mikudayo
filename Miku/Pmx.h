//
// ZeroGram
// http://zerogram.info
//
// Pmx Parser MMDFormats
// https://github.com/oguna/MMDFormats
//
// MMDAI
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
        // if bLimit == 1
        XMFLOAT3 MinLimit;
        XMFLOAT3 MaxLimit;

        IkLink();
        void Fill( bufferstream& is, bool bRH, uint8_t boneIndexByteSize );
    };

    struct Ik
    {
        int32_t BoneIndex;
        int32_t NumIteration;
        float LimitedRadian;
        std::vector<IkLink> Link;

        Ik();
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

        // Additional bias bone (Inherent Rotation | Translation from parent)
        int32_t ParentInherentBoneIndex;
        float ParentInherentBoneCoefficent;

        bool bFixedAxis;
        XMFLOAT3 FixedAxis;

        bool bLocalAxes = true;
        XMFLOAT3 LocalAxisX;
        XMFLOAT3 LocalAxisZ;

        int32_t ExtParentIndex;

        bool bIK;
        Ik Ik;

        Bone();
        void Fill( bufferstream & is, bool bRH, bool bUtf16, uint8_t boneIndexByteSize );
    };

    /*
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
    */
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
        /*
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
        */
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

