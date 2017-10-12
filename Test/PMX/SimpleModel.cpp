#include "stdafx.h"
#include "Common.h"
#include "Pmx.h"
#include "Math/Vector.h"
#include <DirectXMath.h>

const std::wstring PmxModel = L"resource/観客_右利き_サイリウム有AL.pmx";
const std::wstring PmxModelPath = ResourcePath(PmxModel);

TEST(FileReadTest, ReadSync)
{
    Utility::ByteArray ba = Utility::ReadFileSync( PmxModelPath );
    EXPECT_EQ( ba->size(), 10047 );
}

TEST(PMXModelTest, DefaultPMX)
{
    Pmx::PMX pmx;
    EXPECT_FALSE( pmx.IsValid() );
}

TEST(PMXModelTest, ParsePMX)
{
    using namespace Math;

    bool bRightHand = false;
    Utility::ByteArray ba = Utility::ReadFileSync( PmxModelPath );
    Utility::ByteStream bs( ba );

    Pmx::PMX pmx;
    pmx.Fill( bs, bRightHand );
    EXPECT_TRUE( pmx.IsValid() );

    // Compare with PMX viewers result
    EXPECT_THAT( Vector3(pmx.m_Vertices[0].Pos), MatcherNearFast( 1e-3f, Vector3( 0.93634f, 15.4014f, -4.52857f ) ) );
    EXPECT_THAT( Vector3(pmx.m_Vertices[0].Normal), MatcherNearFast( 1e-3f, Vector3( -0.7504545f, 0.614607f, -0.2430561f ) ) );
    EXPECT_THAT( pmx.m_Vertices[0].UV, MatcherNearFast( 1e-3f, DirectX::XMFLOAT2( 1.f, 1.f ) ) );
    EXPECT_EQ( pmx.m_Vertices[0].SkinningType, 0 );
    EXPECT_NEAR( pmx.m_Vertices[0].EdgeSize, 1.f, 1e-3f );

    EXPECT_THAT( Vector3(pmx.m_Vertices[179].Pos), MatcherNearFast( 1e-3f, Vector3( -2.965733f, 23.75405f, -3.743936 ) ) );
    EXPECT_THAT( Vector3(pmx.m_Vertices[179].Normal), MatcherNearFast( 1e-3f, Vector3( -0.7119845f, 0.6089001f, -0.3497411f ) ) );
    EXPECT_THAT( pmx.m_Vertices[179].UV, MatcherNearFast( 1e-3f, DirectX::XMFLOAT2( 0.f, 0.f ) ) );
    EXPECT_EQ( pmx.m_Vertices[179].SkinningType, 0 );
    EXPECT_EQ( pmx.m_Vertices[179].bdef1.BoneIndex, 3 );
    EXPECT_NEAR( pmx.m_Vertices[179].EdgeSize, 1.f, 1e-3f );

    EXPECT_EQ( pmx.m_Indices[191*3], 106 );
    EXPECT_EQ( pmx.m_Indices[191*3+1], 128 );
    EXPECT_EQ( pmx.m_Indices[191*3+2], 129 );

    EXPECT_THAT( Vector3(pmx.m_Materials[0].Specular), MatcherNearFast( 1e-3f, Vector3( 0.2862745f, 0.2862745f, 0.2862745f ) ) );
    EXPECT_NEAR( pmx.m_Materials[0].SpecularPower, 3, 1e-3f );
    EXPECT_EQ( pmx.m_Materials[0].DeafultToon, 0 );

    EXPECT_EQ( pmx.m_Bones.size(), 13 );
    EXPECT_EQ( pmx.m_Bones[0].NameEnglish, L"center" );
    EXPECT_THAT( Vector3(pmx.m_Bones[0].Position), MatcherNearFast( 1e-3f, Vector3( 0, 10, 0 ) ) );
    EXPECT_EQ( pmx.m_Bones[0].ParentBoneIndex, -1 );
    EXPECT_THAT( Vector3(pmx.m_Bones[0].DestinationOriginOffset), MatcherNearFast( 1e-3f, Vector3( 0, 8.793369f, -0.00671 ) ) );

    EXPECT_EQ( pmx.m_Bones[5].Ik.BoneIndex, 3 );
    EXPECT_EQ( pmx.m_Bones[5].Ik.NumIteration, 15 );
    EXPECT_NEAR( pmx.m_Bones[5].Ik.LimitedRadian / 3.1415f * 180.f, 6.875494, 1e-3f );
    EXPECT_EQ( pmx.m_Bones[5].Ik.Link.size(), 2 );
    EXPECT_THAT( Vector3(pmx.m_Bones[5].Ik.Link[0].MinLimit), MatcherNearFast( 1e-3f, Vector3( kZero ) ) );
    EXPECT_THAT( Vector3(pmx.m_Bones[5].Ik.Link[0].MaxLimit), MatcherNearFast( 1e-3f, Vector3( kZero ) ) );

    EXPECT_EQ( pmx.m_Bones[10].NameEnglish, L"R toe" );
    EXPECT_THAT( Vector3(pmx.m_Bones[10].Position), MatcherNearFast( 1e-3f, Vector3( 0, -1.430512E-07, -1.52403f ) ) );
    EXPECT_EQ( pmx.m_Bones[10].ParentBoneIndex, 9 );

    EXPECT_EQ( pmx.m_Morphs.size(), 7 );
    EXPECT_EQ( pmx.m_Morphs[6].Name, L"LightMin" );
}