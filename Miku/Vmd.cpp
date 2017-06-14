#include "Vmd.h"
#include "Encoding.h"

namespace Vmd
{
	using namespace Utility;

	void BoneFrame::Fill( std::istream* stream, bool bRH )
	{
		char buffer[15];
		stream->read( (char*)buffer, sizeof( char ) * 15 );
		BoneName = sjis_to_utf( buffer );
		stream->read( (char*)&Frame, sizeof( int ) );
		ReadPosition( *stream, Translate, bRH );
		ReadRotation( *stream, Rotation, bRH );
		stream->read( (char*)interpolation, sizeof( char ) * 4 * 4 * 4 );
	}
}
