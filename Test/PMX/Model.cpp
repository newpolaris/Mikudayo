#include "Common.h"
#include "Pmx.h"

const std::wstring PmxModel = L"resource/観客_右利き_サイリウム有AL.pmx";
const std::wstring PmxModelPath = ResourcePath(PmxModel);

TEST(FileReadTest, ReadSync)
{
    bool bRightHand = true;
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
    bool bRightHand = true;
    Utility::ByteArray ba = Utility::ReadFileSync( PmxModelPath );
    Utility::ByteStream bs( ba );

    Pmx::PMX pmx;
    pmx.Fill( bs, bRightHand );
    EXPECT_TRUE( pmx.IsValid() );
}