#pragma once

#include <string>
#include <vector>
#include <memory>
#include <Zip.h>
#include <boost/filesystem.hpp>

#include "FileUtility.h"

namespace Utility 
{
	namespace fs = boost::filesystem;

	bool isZip( fs::path path );

	enum EArchiveType
	{
		kArchiveZip,
		kArchiveFile,
	};

	class Archive;
	using ArchivePtr = std::shared_ptr<Utility::Archive>;

	//
	// File read access abstraction layer 
	//
	class Archive
	{
	public:
		virtual EArchiveType GetType() = 0;
        virtual bool IsExist( fs::path name ) const = 0;
		virtual fs::path GetKeyName( fs::path name ) const = 0;
		virtual Utility::ByteArray GetFile( fs::path name ) = 0;
	};

	class RelativeFile : public Archive
	{
	public:
		RelativeFile( fs::path path ) :
			m_Path( path )
		{
		}

		virtual EArchiveType GetType() override { return kArchiveZip; }
        virtual bool IsExist( fs::path name ) const override;
		virtual fs::path GetKeyName( fs::path name ) const override;
		virtual Utility::ByteArray GetFile( fs::path name ) override;
		fs::path m_Path;
	};

	class ZipArchive : public Archive
	{
	public:
		ZipArchive( const std::wstring& path ) :
			m_Path( path ), m_ZipReader( path )
		{
			m_ZipReader.Get_File_List( m_PathList );
		}

		virtual EArchiveType GetType() override { return kArchiveZip; }
        virtual bool IsExist( fs::path name ) const override;
		virtual fs::path GetKeyName( fs::path name ) const override;
		virtual Utility::ByteArray GetFile( fs::path name ) override;

		std::vector<std::string> GetFileList() const
		{
			return m_PathList;
		}

		std::wstring m_Path;
		std::vector<std::string> m_PathList;
		Partio::ZipFileReader m_ZipReader;
	};
}
