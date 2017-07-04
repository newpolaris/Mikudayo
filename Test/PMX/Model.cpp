#include "Common.h"
#include "Pmx.h"

TEST(PMXModelTest, ParsePMX)
{
    std::wstring path = L"resource/観客_右利き_サイリウム有AL.pmx";
    bool bRightHand = true;
	Utility::ByteArray ba = Utility::ReadFileSync( path );
	Utility::ByteStream bs(ba);

	Pmx::PMX pmx;
	pmx.Fill( bs, bRightHand );
	EXPECT_TRUE( pmx.IsValid() );
}