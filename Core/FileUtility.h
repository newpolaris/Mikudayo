#pragma once

#include <vector>
#include <string>
#include <memory>
#include <DirectXMath.h>

namespace Utility
{
	using namespace std;

	// istream compatable
	using StorageType = istream::char_type;
	using bufferstream = basic_istream<StorageType, char_traits<StorageType>>;
	using FileContainer = vector<StorageType>;
	using ByteArray = shared_ptr<FileContainer>;

	template<typename CharT, typename TraitsT = std::char_traits<CharT> >
	class Vectorwrapbuf : public std::basic_streambuf<CharT, TraitsT> {
	public:
		Vectorwrapbuf( ByteArray vec ) : m_Vec(vec) 
		{
			setg( m_Vec->data(), m_Vec->data(), m_Vec->data() + m_Vec->size() );
		}
		ByteArray m_Vec;
	};

	template<typename CharT, typename TraitsT = std::char_traits<CharT> >
	class Vectorstream : public std::basic_istream<CharT, TraitsT> {
	public:
		Vectorstream( ByteArray vec ) : std::istream( &m_buf ), m_buf(vec) 
		{
		}
		Vectorwrapbuf<CharT, TraitsT> m_buf;
	};
	using ByteArrayWrapBuf = Vectorwrapbuf<StorageType>;
	using ByteStream = Vectorstream<StorageType>;

	extern ByteArray NullFile;

	// Reads the entire contents of a binary file.  
	ByteArray ReadFileSync(const wstring& fileName);

	template <typename T, typename R>
	void Read( basic_istream<T, char_traits<T>>& is, R& t, uint32_t size)
	{
        ASSERT( size <= sizeof( R ), "buffer overflow" );
		is.read( reinterpret_cast<T*>(&t), size );
	}

	template <typename T, typename R>
	void Read( basic_istream<T, char_traits<T>>& is, R& t )
	{
		is.read( reinterpret_cast<T*>(&t), sizeof( R ) );
	}

	template <typename T>
	void ReadPosition( basic_istream<T, char_traits<T>>& is, DirectX::XMFLOAT3& t, bool bRH )
	{
		Read( is, t );
		if (bRH) t.z *= -1.0;
	}

	template <typename T>
	void ReadNormal( basic_istream<T, char_traits<T>>& is, DirectX::XMFLOAT3& t, bool bRH )
	{
		Read( is, t );
		if (bRH) t.z *= -1.0;
	}

	template <typename T>
	void ReadRotation( basic_istream<T, char_traits<T>>& is, DirectX::XMFLOAT3& t, bool bRH )
	{
		Read( is, t );
		if (bRH) t.x *= -1.0;
		if (bRH) t.y *= -1.0;
	}

	// Quaternion
	template <typename T>
	void ReadRotation( basic_istream<T, char_traits<T>>& is, DirectX::XMFLOAT4& t, bool bRH )
	{
		Read( is, t );
		if (bRH) t.x *= -1.0;
		if (bRH) t.y *= -1.0;
	}


	template <typename T, typename R>
	void Read( basic_istream<T, char_traits<T>>& is, std::vector<R>& t )
	{
		auto data = reinterpret_cast<T*>(t.data());
		is.read( data, sizeof( T ) * (t.size() * sizeof(R)) );
	}

	uint32_t ReadUint( bufferstream& is );
	uint16_t ReadShort( bufferstream & is );

	template <typename T>
	T Read( bufferstream& is )
	{
		T t;
		Read( is, t );
		return t;
	}

	template <typename T, typename R>
	void Write( basic_ostream<T, char_traits<T>>& is, const R& t )
	{
		is.write( reinterpret_cast<const T*>(&t), sizeof( R ) );
	}

} // namespace Utility
