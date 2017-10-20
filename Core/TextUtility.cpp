#include "pch.h"
#include "TextUtility.h"

#include <string>
#include <regex>

namespace Utility
{
	std::vector<std::string> split( const std::string& Input, const std::string& Delim ) {
		std::regex Reg( Delim );
		std::sregex_token_iterator First { Input.begin(), Input.end(), Reg, 0 }, Last;
		return { First, Last };
	}

	std::vector<std::wstring> split( const std::wstring& Input, const std::wstring& Delim ) {
		std::wregex Reg( Delim );
		std::wsregex_token_iterator First { Input.begin(), Input.end(), Reg, 0 }, Last;
		return { First, Last };
	}

	std::wstring MakeWStr( const std::string& str )
	{
		return std::wstring( str.begin(), str.end() );
	}

    // Don't care about any encoding error
    std::string MakeStr( const std::wstring& str )
    {
        return std::string( str.begin(), str.end() );
    }
}
