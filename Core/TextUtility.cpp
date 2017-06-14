#include "pch.h"
#include "TextUtility.h"

#include <string>
#include <regex>

namespace Utility
{
	std::vector<std::wstring> split( const std::wstring & Input, const std::wstring & Delim ) {
		std::wregex Reg( Delim );
		std::wsregex_token_iterator First { Input.begin(), Input.end(), Reg, 0 }, Last;
		return { First, Last };
	}

	std::wstring MakeWStr( const std::string& str )
	{
		return std::wstring( str.begin(), str.end() );
	}
}
