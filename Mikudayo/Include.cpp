#include "stdafx.h"
#include "Include.h"
#include <boost/filesystem/path.hpp>

using Path = boost::filesystem::path;

Include::Include()
{
}

void Include::AddPath( std::wstring APath )
{
    m_Path.push_back( APath );
}

HRESULT __stdcall Include::Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
{
    (pParentData);

	switch(IncludeType) {
	case D3D_INCLUDE_LOCAL:
	case D3D_INCLUDE_SYSTEM:
		break;

	default:
		return E_FAIL;
	}
	
    for (auto& p : m_Path) 
    {
		Path path(p);
		path /= pFileName;

		std::ifstream input;
		input.open(path.c_str(), std::ios::binary);
		if(!input.is_open())
			continue;

		size_t fsize = (size_t)input.seekg(0, std::ios::end).tellg();
		input.seekg(0, std::ios::beg);

		void* data = ::operator new(fsize);
        if (data == nullptr)
            return E_FAIL;
		input.read(reinterpret_cast<char*>(data), fsize);
		*ppData = data;
		*pBytes = static_cast<UINT>(fsize);
		return S_OK;
	}
	return E_FAIL;
}

HRESULT __stdcall Include::Close(LPCVOID pData)
{
	::operator delete(const_cast<void*>(pData));
	return S_OK;
}
