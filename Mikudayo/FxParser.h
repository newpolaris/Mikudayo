#pragma once

#pragma warning(push)
#pragma warning(disable: 4521)
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#pragma warning(pop)

namespace client {
namespace x3 = boost::spirit::x3;

namespace ast 
{
    struct sampler_state;
    struct blend_state;
    struct depth_stencil_state;
    struct rasterizer_state;
    struct shader_compiler_desc;
    struct technique_desc;
    struct statement : x3::variant<
          x3::forward_ast<sampler_state>
        , x3::forward_ast<blend_state>
        , x3::forward_ast<depth_stencil_state>
        , x3::forward_ast<rasterizer_state>
        , x3::forward_ast<shader_compiler_desc>
        , x3::forward_ast<technique_desc>
        , std::string
    >
    {
        using base_type::base_type;
        using base_type::operator=;
    };

    struct sampler_state
    {
        std::string name;
        uint32_t slot;
        D3D11_SAMPLER_DESC desc = CD3D11_SAMPLER_DESC(D3D11_DEFAULT);
    };

    struct blend_state
    {
        std::string name;
        D3D11_BLEND_DESC desc = CD3D11_BLEND_DESC(D3D11_DEFAULT);
    };

    struct depth_stencil_state
    {
        std::string name;
        D3D11_DEPTH_STENCIL_DESC desc = CD3D11_DEPTH_STENCIL_DESC(D3D11_DEFAULT);
    };

    struct rasterizer_state
    {
        std::string name;
        D3D11_RASTERIZER_DESC desc = CD3D11_RASTERIZER_DESC(D3D11_DEFAULT);
    };

    enum shader_type {
        kVertexShader = 0,
        kPixelShader,
        kGeometryShader,
    };

    struct shader_compiler_desc
    {
        shader_type type; 
        std::string name;
        std::string profile;
        std::string entrypoint;
    };

    struct blend_config;
    struct depth_stencil_config;
    struct rasterizer_config;
    struct vertex_config;
    struct pixel_config;
    struct geometry_config;
    struct pass_config : x3::variant<
          x3::forward_ast<blend_config>
        , x3::forward_ast<depth_stencil_config>
        , x3::forward_ast<rasterizer_config>
        , x3::forward_ast<vertex_config>
        , x3::forward_ast<pixel_config>
        , x3::forward_ast<geometry_config>
    >
    {
        using base_type::base_type;
        using base_type::operator=;
    };

    struct pass_desc
    {
        std::string name;
        std::vector<pass_config> configs;
    };
    
    struct blend_config
    {
        std::string name;
        std::vector<float> blend_factor;
        uint32_t sample_mask;
    };

    struct depth_stencil_config
    {
        std::string name;
        uint32_t stencil_ref;
    };

    struct rasterizer_config
    {
        std::string name;
    };

    struct vertex_config
    {
        std::string name;
    };

    struct pixel_config
    {
        std::string name;
    };

    struct geometry_config
    {
        std::string name;
    };

    struct technique_desc
    {
        std::string name;
        std::vector<pass_desc> pass;
    };

    struct program
    {
        std::list<statement> states;
    };
    using float4 = float[4];
    using D3D11_RENDER_TARGET_BLEND_DESC8 = D3D11_RENDER_TARGET_BLEND_DESC[8];
}
}

bool FxParse(const std::string& Text, client::ast::program& Program);

