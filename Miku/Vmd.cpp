#include "Vmd.h"
#include "Encoding.h"

namespace Vmd
{
	using namespace Utility;

	void BoneFrame::Fill( bufferstream& is, bool bRH )
	{
		NameBuf buffer;
		Read( is, buffer );
		BoneName = sjis_to_utf( buffer );
		Read( is, Frame );
		ReadPosition( is, Offset, bRH );
		ReadRotation( is, Rotation, bRH );
		Read( is, Interpolation );
	}

	void FaceFrame::Fill( bufferstream& is )
	{
		NameBuf buffer;
		Read( is, buffer );
		FaceName = sjis_to_utf( buffer );
		Read( is, Frame );
		Read( is, Weight );
	}
}
