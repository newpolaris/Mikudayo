#include "stdafx.h"
#include "Mapping.h"
#include "FxContainer.h"
#include "FxParser.h"
#include "TextUtility.h"
#include "Include.h"
#include "SamplerManager.h"
#include "Hash.h"
#include "FileUtility.h"
#include "InputLayout.h"
#include <boost/filesystem.hpp>

using Microsoft::WRL::ComPtr;
using Path = boost::filesystem::path;

namespace {
    struct FxParseError : std::runtime_error
    {
        FxParseError( const std::string& Message );
    };

    FxParseError::FxParseError( const std::string& Message ) :
        std::runtime_error( Message )
    {
    }

    size_t HashBytes( void* Pointer, size_t Length )
    {
        size_t* pointer = reinterpret_cast<size_t*>(Pointer);
        size_t size = Length / 8;
        size_t hashCode = Utility::HashState( pointer, size );
        size_t remain = 0;
        for (size_t k = size * 8; k < Length; k++)
        {
            remain <<= 8;
            remain += reinterpret_cast<uint8_t*>(pointer)[k];
        }
        return Utility::HashState( &hashCode, 1, remain );
    }

    ComPtr<ID3DBlob> CompileShader(
        const std::string& Name,
        const std::string& EntryPoint,
        const std::string& Profile,
        const void* Pointer,
        size_t Length )
    {
        UINT flags = 0;
#ifdef _DEBUG
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        ComPtr<ID3DBlob> byteCode;
        ComPtr<ID3DBlob> errors;

        ASSERT_SUCCEEDED( D3DCompile(
            Pointer, Length,
            Name.c_str(), nullptr, nullptr,
            EntryPoint.c_str(), Profile.c_str(), flags, 0,
            byteCode.GetAddressOf(), errors.GetAddressOf() ) );

        if (errors)
        {
            // The message is a 'error/warning' from the HLSL compiler.
            char const* message = static_cast<char const*>(errors->GetBufferPointer());
            std::string output( message );
            std::wstring woutput = Utility::MakeWStr( output );
            OutputDebugString( woutput.c_str() );
        }
        return byteCode;
    }

    ComPtr<ID3DBlob> CheckShaderCache(
        const std::string& Name,
        const std::string& EntryPoint,
        const std::string& Profile,
        const void* Pointer,
        size_t Length,
        size_t HashCode )
    {
        namespace fs = boost::filesystem;

        std::string postfix;
#ifdef _DEBUG
        postfix = "_D";
#endif
        const std::wstring FolderName = L"ShaderCache";
        const std::string tag = Name + "_" + EntryPoint + "_" + Profile + postfix + ".cache";
        const fs::path path = fs::path(FolderName) / tag;
        const std::wstring filePath = path.generic_wstring();

        Utility::ByteArray ba = Utility::ReadFileSync( filePath );
        if (ba->size() > 0)
        {
            Utility::ByteStream bs( ba );
            size_t refHashCode;
            Read( bs, refHashCode );
            if (refHashCode == HashCode)
            {
                DEBUGPRINT( "Use shader cache %s", tag.c_str() );
                size_t ShaderLength;
                Read( bs, ShaderLength );
                ASSERT( ba->size() == sizeof( size_t ) * 2 + ShaderLength );
                ComPtr<ID3DBlob> blob;
                ASSERT_SUCCEEDED( D3DCreateBlob( ShaderLength, blob.GetAddressOf() ) );
                bs.read( (char*)blob->GetBufferPointer(), blob->GetBufferSize() );
                return blob;
            }
            DEBUGPRINT( "Shader cache hash mis-matched recompile shader %s", tag.c_str() );
        }
        auto blob = CompileShader( Name, EntryPoint, Profile, Pointer, Length );
        if (!blob)
            return nullptr;
        const size_t ShaderLength = blob->GetBufferSize();
        const char* ShaderPointer = reinterpret_cast<char*>(blob->GetBufferPointer());
        if (!fs::exists(FolderName))
            fs::create_directory(FolderName);
        std::ofstream outputFile;
        outputFile.open( filePath, std::ios::binary );
        if (!outputFile.is_open())
            return nullptr;
        Utility::Write( outputFile, HashCode );
        Utility::Write( outputFile, ShaderLength );
        outputFile.write( ShaderPointer, ShaderLength );
        outputFile.flush();
        return blob;
    }

}

namespace client { namespace ast {
struct eval
{
    eval( ComPtr<ID3DBlob>& Blob, Include& Inc, std::string SourceName );

    void operator()( std::string const& ) {}
    void operator()( sampler_state const& x )
    {
        FxSampler fx = {};
        fx.slot = x.slot;
        fx.desc = SamplerDesc(x.desc);
        m_Sampler.push_back( fx );
    }

    void operator()( blend_state const& x )
    {
        m_Blend[x.name] = x.desc;
    }

    void operator()( depth_stencil_state const& x )
    {
        m_Depth[x.name] = x.desc;
    }

    void operator()( rasterizer_state const& x )
    {
        m_Raster[x.name] = x.desc;
    }

    void operator()( shader_compiler_desc const& x );
    void operator()( technique_desc const& tech );
    void operator()( program const& x )
    {
        for (auto const& s : x.states )
            boost::apply_visitor(*this, s);
    }

    // Apply config to PSO
    void operator()( const blend_config& config, GraphicsPSO& PSO );
    void operator()( const depth_stencil_config& config, GraphicsPSO& PSO );
    void operator()( const rasterizer_config& config, GraphicsPSO& PSO );
    void operator()( const vertex_config& config, GraphicsPSO& PSO );
    void operator()( const pixel_config& config, GraphicsPSO& PSO );
    void operator()( const geometry_config& config, GraphicsPSO& PSO);

    ComPtr<ID3DBlob>& m_Blob;
    Include& m_Include;
    std::string m_SourceName;

    std::vector<FxSampler> m_Sampler;
    std::map<std::string, D3D11_BLEND_DESC> m_Blend;
    std::map<std::string, D3D11_DEPTH_STENCIL_DESC> m_Depth;
    std::map<std::string, D3D11_RASTERIZER_DESC> m_Raster;

    std::map<std::string, ComPtr<ID3DBlob>> m_ShaderByteCode;
    std::map<std::string, std::vector<GraphicsPSO>> m_Technique;
};

eval::eval( ComPtr<ID3DBlob>& Blob, Include& Inc, const std::string SourceName ) :
    m_Blob(Blob), m_Include(Inc), m_SourceName( SourceName )
{
}

void eval::operator()( const shader_compiler_desc& desc )
{
    const size_t hashCode = HashBytes( m_Blob->GetBufferPointer(), m_Blob->GetBufferSize() );

    ComPtr<ID3DBlob> byteCode = CheckShaderCache( 
        m_SourceName, desc.entrypoint, desc.profile,
        m_Blob->GetBufferPointer(), m_Blob->GetBufferSize(), 
        hashCode);

    if (byteCode)
        m_ShaderByteCode[desc.name] = byteCode;
}

void eval::operator()( const technique_desc& tech )
{
    for (auto& pass : tech.pass)
    {
        GraphicsPSO pso;
        auto with = [&]( auto&& config ) {
            (*this)(config, pso);
        };
        for (auto const& config : pass.configs)
            boost::apply_visitor( with, config );
        m_Technique[tech.name].push_back( std::move( pso ) );
    }
}

void eval::operator()( const blend_config& config, GraphicsPSO& PSO )
{
    auto it = m_Blend.find( config.name );
    if (it == m_Blend.end())
    {
        std::string error( "Can't find config " + config.name );
        throw FxParseError(error);
    }
    PSO.SetBlendState( it->second );
    FLOAT factor[4] = { 
        config.blend_factor[0], config.blend_factor[1],
        config.blend_factor[2], config.blend_factor[3],
    };
    PSO.SetBlendFactor( factor );
    PSO.SetSampleMask( config.sample_mask );
}

void eval::operator()( const depth_stencil_config& config, GraphicsPSO& PSO )
{
    auto it = m_Depth.find( config.name );
    if (it == m_Depth.end())
    {
        std::string error( "Can't find config " + config.name );
        throw FxParseError(error);
    }
    PSO.SetDepthStencilState( it->second );
    PSO.SetStencilRef( config.stencil_ref );
}

void eval::operator()( const rasterizer_config& config, GraphicsPSO& PSO )
{
    auto it = m_Raster.find( config.name );
    if (it == m_Raster.end())
    {
        std::string error( "Can't find config " + config.name );
        throw FxParseError(error);
    }
    PSO.SetRasterizerState( it->second );
}

void eval::operator()( const vertex_config& config, GraphicsPSO& PSO )
{
    auto tag = m_SourceName + "_" + config.name;
    auto& code = m_ShaderByteCode[config.name];
    PSO.SetVertexShader( tag, code->GetBufferPointer(), code->GetBufferSize() );
}

void eval::operator()( const pixel_config& config, GraphicsPSO& PSO )
{
    auto tag = m_SourceName + "_" + config.name;
    auto& code = m_ShaderByteCode[config.name];
    PSO.SetPixelShader( tag, code->GetBufferPointer(), code->GetBufferSize() );
}

void eval::operator()( const geometry_config& config, GraphicsPSO& PSO )
{
    auto tag = m_SourceName + "_" + config.name;
    auto& code = m_ShaderByteCode[config.name];
    PSO.SetGeometryShader( tag, code->GetBufferPointer(), code->GetBufferSize() );
}

}}

FxContainer::FxContainer( const std::wstring& FilePath ) :
    m_FilePath( FilePath )
{
}

bool FxContainer::Load()
{
    std::ifstream in( m_FilePath, std::ios_base::in );
    in.unsetf( std::ios::skipws ); // No white space skipping!
    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string str = buffer.str();

    client::ast::program program;
    if (!FxParse( str, program ))
        return false;
    auto path = Path(m_FilePath);
    auto source = path.filename().generic_string();
    auto parent = path.parent_path().generic_wstring();

    Include include;
    include.AddPath( parent );

	ComPtr<ID3DBlob> blob, error;
    ASSERT_SUCCEEDED(D3DPreprocess( 
        str.data(), str.length(), source.c_str(), nullptr, &include,
        blob.GetAddressOf(), error.GetAddressOf() ));

    try 
    {
        client::ast::eval eval( blob, include, source );
        eval(program);

        m_ShaderByteCode.swap(eval.m_ShaderByteCode);
        m_Technique.swap(eval.m_Technique);
        m_Sampler.swap(eval.m_Sampler);
    }
    catch (const FxParseError& error)
    {
        HALT(error.what());
        return false;
    }
    return true;
}

std::vector<GraphicsPSO> FxContainer::FindTechnique(const std::string& TechName) const
{
    auto it = m_Technique.find(TechName);
    if (it != m_Technique.end())
        return it->second;
    return std::vector<GraphicsPSO>();
}

std::vector<FxSampler> FxContainer::GetSampler() const
{
    return m_Sampler;
}
