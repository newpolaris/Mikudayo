#include "stdafx.h"
#include "Common.h"
#include "Pmx.h"
#include "Math/Vector.h"
#include <DirectXMath.h>

TEST(PMXModelTest2, ParsePMX)
{
    using namespace Math;

    bool bRightHand = false;

    const std::wstring PmxModel = L"resource/onda_mod_SHIMAKAZE_v090.pmx";
    const std::wstring PmxModelPath = ResourcePath( PmxModel );
    Utility::ByteArray ba = Utility::ReadFileSync( PmxModelPath );
    Utility::ByteStream bs( ba );

    Pmx::PMX pmx;
    pmx.Fill( bs, bRightHand );
    EXPECT_TRUE( pmx.IsValid() );

    // Compare with PMX viewers result
    EXPECT_EQ( pmx.m_Vertices.size(), 34291 );
    EXPECT_EQ( pmx.m_Bones.size(), 204 );
    EXPECT_EQ( pmx.m_Morphs.size(), 125 );
    EXPECT_EQ( pmx.m_Morphs[0].Name, L"いぃ" );
    EXPECT_EQ( pmx.m_Morphs[0].Type, Pmx::MorphType::kVertex );
    EXPECT_EQ( pmx.m_Morphs[0].Panel, Pmx::MorphCategory::kEyebrow );
    EXPECT_EQ( pmx.m_Morphs[0].VertexList.size(), 54 );
    auto& morph0VertList = pmx.m_Morphs[0].VertexList;
    EXPECT_EQ( morph0VertList.size(), 54 );
    EXPECT_EQ( morph0VertList[0].VertexIndex, 25871 );
    EXPECT_THAT( Vector3(morph0VertList[0].Position), MatcherNearFast( 1e-3f, Vector3( -0.001583755f, -0.07791138f, 0.0106138f ) ) );

    EXPECT_EQ( pmx.m_Frames.size(), 12 );
    EXPECT_EQ( pmx.m_Frames[0].Type, 1 );
    EXPECT_EQ( pmx.m_Frames[0].ElementList.size(), 1 );
    EXPECT_EQ( pmx.m_Frames[1].Type, 1 );
    EXPECT_EQ( pmx.m_Frames[1].ElementList.size(), 125 );
    EXPECT_EQ( pmx.m_Frames[1].ElementList[0].Type, Pmx::DisplayElementType::kMorph );
    EXPECT_EQ( pmx.m_Frames[9].ElementList[4].Index, 58 );
    EXPECT_EQ( pmx.m_Frames[9].Type, 0 );

    EXPECT_EQ( pmx.m_RigidBodies.size(), 88 );
    auto body0 = pmx.m_RigidBodies[0];
    EXPECT_EQ( body0.Shape, Pmx::RigidBodyShape::kCapsule );
    EXPECT_EQ( body0.RigidType, Pmx::RigidBodyType::kBoneConnected );
    EXPECT_EQ( body0.CollisionGroupID, 0 );
    uint16_t flag = 1;
    EXPECT_EQ( body0.CollisionGroupMask, uint16_t(~1) );
    EXPECT_EQ( body0.BoneIndex, 5 );
    EXPECT_THAT( Vector3(body0.Rotation)*180/XM_PI, MatcherNearFast( 1e-3f, Vector3( 9.7776f, 0.00f, 0.00f ) ) );
    auto body80 = pmx.m_RigidBodies[80];
    EXPECT_EQ( body80.CollisionGroupID, 4 );
    EXPECT_EQ( body80.CollisionGroupMask, uint16_t(~0b11110) );

    EXPECT_EQ( pmx.m_Joints.size(), 83 );
    auto joint0 = pmx.m_Joints[0];
    EXPECT_EQ( joint0.Type, Pmx::JointType::kGeneric6DofSpring );
    EXPECT_EQ( joint0.RigidBodyIndexA, 8 );
    EXPECT_EQ( joint0.RigidBodyIndexB, 14 );
}
