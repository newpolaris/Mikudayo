#include "stdafx.h"
#include "FxContainer.h"
#include "FxParser.h"
#include "TextUtility.h"
#include "Include.h"
#include "SamplerManager.h"

using Microsoft::WRL::ComPtr;
using Path = boost::filesystem::path;

struct FxParseError : std::runtime_error
{
	FxParseError(const std::string& Message);
};

FxParseError::FxParseError( const std::string& Message ) :
    std::runtime_error( Message )
{
}

namespace client { namespace ast {
struct eval
{
    eval( ComPtr<ID3DBlob>& Blob, Include& Inc, std::string SourceName );

    void operator()( std::string const& ) {}
    void operator()( sampler_state const& x )
    {
        FxSampler fx;
        fx.slot = x.slot;
        fx.handle = SamplerDesc( x.desc ).CreateDescriptor();
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
    std::map<std::string, std::vector<std::shared_ptr<GraphicsPSO>>> m_Technique;
};

eval::eval( ComPtr<ID3DBlob>& Blob, Include& Inc, const std::string SourceName ) :
    m_Blob(Blob), m_Include(Inc), m_SourceName( SourceName )
{
}

void eval::operator()( const shader_compiler_desc& desc )
{
    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> byteCode;
    ComPtr<ID3DBlob> errors;

    ASSERT_SUCCEEDED(D3DCompile( 
        m_Blob->GetBufferPointer(), m_Blob->GetBufferSize(),
        m_SourceName.c_str(), nullptr, &m_Include,
        desc.entrypoint.c_str(), desc.profile.c_str(), flags, 0,
        byteCode.GetAddressOf(), errors.GetAddressOf()));

    if (errors)
    {
        // The message is a 'error/warning' from the HLSL compiler.
        char const* message = static_cast<char const*>(errors->GetBufferPointer());
        std::string output(message);
        std::wstring woutput = Utility::MakeWStr(output);
        OutputDebugString(woutput.c_str());
    }
    if (byteCode)
        m_ShaderByteCode[desc.name] = byteCode;
}

    std::vector<InputDesc> InputDescriptor
    {
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BONE_ID", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BONE_WEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "EDGE_FLAT", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

void eval::operator()( const technique_desc& tech )
{
    for (auto& pass : tech.pass)
    {
        auto pso = std::make_shared<GraphicsPSO>();
        auto with = [&]( auto&& config ) {
            (*this)(config, *pso);
        };
        for (auto const& config : pass.configs)
            boost::apply_visitor( with, config );
        pso->SetInputLayout( static_cast<UINT>(InputDescriptor.size()), InputDescriptor.data() );
        pso->Finalize();
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

	ComPtr<ID3DBlob> blob;
	ASSERT_SUCCEEDED(D3DCreateBlob(str.length(), blob.GetAddressOf()));
	memcpy(blob->GetBufferPointer(), str.data(), str.length());

    auto path = Path(m_FilePath);
    auto source = path.filename().generic_string();
    auto parent = path.parent_path().generic_wstring();
    Include include;
    include.AddPath( parent );

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

uint32_t FxContainer::FindTechnique( const std::string& TechName ) const
{
    auto it = m_Technique.find(TechName);
    if (it != m_Technique.end())
        return (uint32_t)(it->second.size());
    return 0;
}

void FxContainer::SetPass( GraphicsContext& Context, const std::string& TechName, uint32_t Pass )
{
    auto it = m_Technique.find(TechName);
    if (it != m_Technique.end())
        Context.SetPipelineState( *it->second[Pass] );
}

void FxContainer::SetSampler( GraphicsContext& Context )
{
    for (auto& sampler : m_Sampler)
        Context.SetDynamicSampler( sampler.slot, sampler.handle, { kBindVertex, kBindPixel, kBindGeometry } );
}
