#include "pch.h"
#include "FileUtility.h"
#include <fstream>
#include <boost/filesystem.hpp>

using namespace std;
using namespace Utility;

namespace Utility
{
	ByteArray NullFile = make_shared<FileContainer>( FileContainer() );

	ByteArray ReadFileSync( const wstring& fileName )
	{
		std::ifstream inputFile;
		inputFile.open( fileName, std::ios::binary | std::ios::ate );
		ASSERT( inputFile.is_open(), L"File to open model file " + fileName );
		if (!inputFile.is_open())
			return NullFile;
		auto filesize = static_cast<size_t>(inputFile.tellg());
		ByteArray buf = make_shared<FileContainer>( filesize * sizeof( char ) );
		inputFile.ignore( std::numeric_limits<std::streamsize>::max() );
		inputFile.seekg( std::ios::beg );
		inputFile.read( reinterpret_cast<char*>(buf->data()), filesize );
		inputFile.close();
		return buf;
	}

	uint32_t ReadInt( bufferstream & is )
	{
		uint32_t t;
		Read( is, t );
		return t;
	}

	uint16_t ReadShort( bufferstream & is )
	{
		uint16_t t;
		Read( is, t );
		return t;
	}
}
