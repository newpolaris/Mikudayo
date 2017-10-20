#pragma once

#include <vector>
#include <string>

namespace Utility
{
	std::vector<std::string> split( const std::string& Input, const std::string& Delim );
	std::vector<std::wstring> split( const std::wstring& Input, const std::wstring& Delim );
	std::wstring MakeWStr( const std::string& str );
	std::string MakeStr( const std::wstring& str );

} // namespace Utility
