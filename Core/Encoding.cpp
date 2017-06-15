#include "pch.h"
#include "Encoding.h"

namespace Utility 
{
	std::wstring to_utf( const std::string& t, std::string enc )
	{
		using boost::locale::conv::to_utf;
		return to_utf<wchar_t>( t, enc );
	}
}
