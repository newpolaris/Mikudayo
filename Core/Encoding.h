#pragma once

#include <boost/locale.hpp>
#include <boost/locale/encoding.hpp>

namespace Utility 
{
	template <typename T>
	std::wstring to_utf( const T& t, std::string enc = "shift-jis" )
	{
		using boost::locale::conv::to_utf;
		return to_utf<wchar_t>(
			std::string( (char*)(t), (char*)(t)+sizeof( t ) ), enc );
	}
}
