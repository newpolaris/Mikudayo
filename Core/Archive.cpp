#include "pch.h"
#include "Archive.h"
#include "FileUtility.h"

#include <mutex>
#include <sstream>

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

bool RelativeFile::IsExist( fs::path name ) const
{
    return boost::filesystem::exists( GetKeyName(name) );
}

Utility::ByteArray RelativeFile::GetFile( fs::path name )
{
	return Utility::ReadFileSync( GetKeyName(name).generic_wstring() );
}

bool ZipArchive::IsExist( fs::path name ) const
{
    return m_ZipReader.IsExist( GetKeyName(name).generic_string() );
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

Utility::ByteArray ZipArchive::GetFile( fs::path name )
{
    static std::mutex s_ZipIterMutex;

    // Patio use file stream as shared
    lock_guard<mutex> CS( s_ZipIterMutex );
    auto stream = std::unique_ptr<std::istream>( m_ZipReader.Get_File( GetKeyName(name).generic_string() ) );

    std::ostringstream ss;
    ss << stream->rdbuf();
    const std::string& s = ss.str();
    return std::make_shared<FileContainer>( s.begin(), s.end() );
}