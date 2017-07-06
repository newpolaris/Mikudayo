#include "pch.h"
#include "Archive.h"
#include "FileUtility.h"

using namespace Utility;

namespace Utility 
{
	bool isZip( fs::path path )
	{
		std::ifstream inputFile;
		bool bZip = false;

		inputFile.open( path.generic_wstring(), std::ios::binary );
		if (!inputFile.is_open())
			return false;
		int signature;
		inputFile.read( reinterpret_cast<char*>(&signature), sizeof( signature ) );
		inputFile.close();
		if (signature == 0x04034b50 )
			bZip = true;

		return bZip;
	}
}

fs::path RelativeFile::GetKeyName( fs::path name ) const
{
	return m_Path / name;
}

std::unique_ptr<std::istream> Utility::RelativeFile::GetFile( fs::path name )
{
	auto fileBuffer = Utility::ReadFileSync( name.generic_wstring() );
	if (fileBuffer->size() == 0)
	{
		auto relativePath = m_Path / name;
		fileBuffer = Utility::ReadFileSync( relativePath.generic_wstring() );
        if (fileBuffer == NullFile)
            return nullptr;
	}
	return std::make_unique<Utility::ByteStream>( fileBuffer );
}

//
// Returns path similar to zipname/name 
// which can be use as GetFile argument
//
fs::path ZipArchive::GetKeyName( fs::path name ) const
{
	// Patio contains "filename/" at front
	fs::path root(m_PathList.front());
	root += name;
	return root;
}

std::unique_ptr<std::istream> ZipArchive::GetFile( fs::path name )
{
	return std::unique_ptr<std::istream>( m_ZipReader.Get_File( name.generic_string() ) );
}