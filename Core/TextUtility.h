#pragma once

#include <vector>
#include <string>

namespace Utility
{
	std::vector<std::wstring> split( const std::wstring& Input, const std::wstring& Delim );
	std::wstring MakeWStr( const std::string& str );

} // namespace Utility
