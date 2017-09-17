#pragma once

#include <d3dcommon.h>
#include <string>
#include <vector>

//
// Use code from zerogram's softbody project
//
class Include : public ID3DInclude
{
	
public:
    Include();
    void AddPath( std::wstring APath );

    HRESULT __stdcall Open( D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes );
    HRESULT __stdcall Close( LPCVOID pData );

    std::vector<std::wstring> m_Path;
};
