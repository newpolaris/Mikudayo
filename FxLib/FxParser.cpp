#include "stdafx.h"
#include "FxParser.h"

#pragma warning(push)
#pragma warning(disable: 4100)
#pragma warning(disable: 4127)
#pragma warning(disable: 4456)
#pragma warning(disable: 4459)
#pragma warning(disable: 4702)
#pragma warning(disable: 4819)
// #define BOOST_SPIRIT_X3_DEBUG
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#pragma warning(pop)

// We need to tell fusion about our rexpr struct
// to make it a first-class fusion citizen
BOOST_FUSION_ADAPT_STRUCT(
    D3D11_SAMPLER_DESC,
    (auto, Filter)
    (auto, AddressU)
    (auto, AddressV)
    (auto, AddressW)
    (auto, MipLODBias)
    (auto, MaxAnisotropy)
    (auto, ComparisonFunc)
    (client::ast::float4, BorderColor)
    (auto, MinLOD)
    (auto, MaxLOD)
)
BOOST_FUSION_ADAPT_STRUCT(
    client::ast::sampler_state,
    (std::string, name)
    (uint32_t, slot)
    (auto, desc)
)
BOOST_FUSION_ADAPT_STRUCT(
    D3D11_RENDER_TARGET_BLEND_DESC,
    (auto, BlendEnable)
    (auto, SrcBlend)
    (auto, DestBlend)
    (auto, BlendOp)
    (auto, SrcBlendAlpha)
    (auto, DestBlendAlpha)
    (auto, BlendOpAlpha)
    (auto, RenderTargetWriteMask)
)
BOOST_FUSION_ADAPT_STRUCT(
    D3D11_BLEND_DESC,
    (auto, AlphaToCoverageEnable)
    (auto, IndependentBlendEnable)
    (client::ast::D3D11_RENDER_TARGET_BLEND_DESC8, RenderTarget)
)
BOOST_FUSION_ADAPT_STRUCT(
    client::ast::blend_state,
    (std::string, name)
    (auto, desc)
)
BOOST_FUSION_ADAPT_STRUCT(
    D3D11_DEPTH_STENCIL_DESC,
    (auto, DepthEnable)
    (auto, DepthWriteMask)
    (auto, DepthFunc)
    (auto, StencilEnable)
    (auto, StencilReadMask)
    (auto, StencilWriteMask)
    (auto, FrontFace)
    (auto, BackFace)
)
BOOST_FUSION_ADAPT_STRUCT(
    client::ast::depth_stencil_state,
    (auto, name)
    (auto, desc)
)
BOOST_FUSION_ADAPT_STRUCT(
    D3D11_RASTERIZER_DESC,
    (auto, FillMode)
    (auto, CullMode)
    (auto, FrontCounterClockwise)
    (auto, DepthBias)
    (auto, DepthBiasClamp)
    (auto, SlopeScaledDepthBias)
    (auto, DepthClipEnable)
    (auto, ScissorEnable)
    (auto, MultisampleEnable)
    (auto, AntialiasedLineEnable)
)
BOOST_FUSION_ADAPT_STRUCT(
    client::ast::rasterizer_state,
    (auto, name)
    (auto, desc)
)
BOOST_FUSION_ADAPT_STRUCT(
    client::ast::shader_compiler_desc,
    (auto, type)
    (auto, name)
    (auto, profile)
    (auto, entrypoint)
)
BOOST_FUSION_ADAPT_STRUCT(
    client::ast::blend_config,
    (auto, name)
    (auto, blend_factor)
    (auto, sample_mask)
)
BOOST_FUSION_ADAPT_STRUCT(
    client::ast::depth_stencil_config,
    (auto, name)
    (auto, stencil_ref)
)
BOOST_FUSION_ADAPT_STRUCT(
    client::ast::rasterizer_config,
    (auto, name)
)
BOOST_FUSION_ADAPT_STRUCT(
    client::ast::vertex_config,
    (auto, name)
)
BOOST_FUSION_ADAPT_STRUCT(
    client::ast::pixel_config,
    (auto, name)
)
BOOST_FUSION_ADAPT_STRUCT(
    client::ast::geometry_config,
    (auto, name)
)
BOOST_FUSION_ADAPT_STRUCT(
    client::ast::pass_desc,
    (auto, name)
    (auto, configs)
)
BOOST_FUSION_ADAPT_STRUCT(
    client::ast::technique_desc,
    (auto, name)
    (auto, pass)
)
BOOST_FUSION_ADAPT_STRUCT(
    client::ast::program,
    (auto, states)
)

namespace client { namespace assign_operator {
    using boost::fusion::at_c;
    auto const c0 = []( auto &ctx ) { 
        at_c<0>( _val(ctx) ) = 
            static_cast<std::remove_reference<decltype(at_c<0>( _val(ctx) ))>::type>(
                _attr(ctx)
            ); 
    };

    template <uint32_t i> 
    auto const a = []( auto &ctx ) {
        at_c<i>( _val( ctx ) ) = _attr( ctx ); 
    };

    uint32_t n;
    struct _n {};

    template <uint32_t ith_element_struct, uint32_t j>
    auto const k = []( auto &ctx ) {
        auto& nth_element = at_c<ith_element_struct>( _val( ctx ) )[n];
        at_c<j>( nth_element ) = _attr( ctx );
    };
    auto const idx = '[' >> x3::with<_n>(std::ref(n))[x3::omit[x3::uint32]] >> ']';
}}

namespace client { namespace symbol {
    enum enum_sampler_key
    {
        kFilter = 0,
        kAddressU,
        kAddressV,
        kAddressW,
        kMipLODBias,
        kMaxAnisotropy,
        kComparisonFunc,
        kBorderColor,
        kMinLOD,
        kMaxLOD,
    };
    #define _sampler_key(name) (#name, k##name)
    struct sampler_key_ : x3::symbols<uint32_t>
    {
        sampler_key_()
        {
            add
                _sampler_key( Filter )
                _sampler_key( AddressU )
                _sampler_key( AddressV )
                _sampler_key( AddressW )
                _sampler_key( MipLODBias )
                _sampler_key( MaxAnisotropy )
                _sampler_key( ComparisonFunc )
                _sampler_key( BorderColor )
                _sampler_key( MinLOD )
                _sampler_key( MaxLOD )
                ;
        }
    } sampler_key;
    #undef _sampler_key
    #define _filter(name) (#name, D3D11_FILTER_##name)
    struct filter_ : x3::symbols<D3D11_FILTER>
    {
        filter_()
        {
            add
                _filter( MIN_MAG_MIP_POINT )
                _filter( MIN_MAG_POINT_MIP_LINEAR )
                _filter( MIN_POINT_MAG_LINEAR_MIP_POINT )
                _filter( MIN_POINT_MAG_MIP_LINEAR )
                _filter( MIN_LINEAR_MAG_MIP_POINT )
                _filter( MIN_LINEAR_MAG_POINT_MIP_LINEAR )
                _filter( MIN_MAG_LINEAR_MIP_POINT )
                _filter( MIN_MAG_MIP_LINEAR )
                _filter( ANISOTROPIC )
                ;
        }

    } filter;
    #undef _filter
    #define _address(name) (#name, D3D11_TEXTURE_ADDRESS_##name)
    struct address_ : x3::symbols<D3D11_TEXTURE_ADDRESS_MODE>
    {
        address_()
        {
            add
                _address(WRAP)
                _address(MIRROR)
                _address(CLAMP)
                _address(BORDER)
                _address(MIRROR_ONCE)
                ;
        }
    } adress;
    #undef _address
    #define _blend_op(name) (#name, D3D11_BLEND_OP_##name)
    struct blend_op_ : x3::symbols<D3D11_BLEND_OP>
    {
        blend_op_()
        {
            add
                _blend_op(ADD)
                _blend_op(SUBTRACT)
                _blend_op(REV_SUBTRACT)
                _blend_op(MIN)
                _blend_op(MAX)
                ;
        }
    } blend_op;
    #undef _blend_op
    #define _blend(name) (#name, D3D11_BLEND_##name)
    struct blend_ : x3::symbols<D3D11_BLEND>
    {
        blend_()
        {
            add
                _blend(ZERO)
                _blend(ONE)
                _blend(SRC_COLOR)
                _blend(INV_SRC_COLOR)
                _blend(SRC_ALPHA)
                _blend(INV_SRC_ALPHA)
                _blend(DEST_ALPHA)
                _blend(INV_DEST_ALPHA)
                _blend(DEST_COLOR)
                _blend(INV_DEST_COLOR)
                _blend(SRC_ALPHA_SAT)
                _blend(BLEND_FACTOR)
                _blend(INV_BLEND_FACTOR)
                _blend(SRC1_COLOR)
                _blend(INV_SRC1_COLOR)
                _blend(SRC1_ALPHA)
                _blend(INV_SRC1_ALPHA)
                ;
        }
    } blend;
    #undef _blend
    #define _write_mask(name) (#name, D3D11_DEPTH_WRITE_MASK_##name)
    struct depth_write_mask_ : x3::symbols<D3D11_DEPTH_WRITE_MASK>
    {
        depth_write_mask_()
        {
            add
                _write_mask(ZERO)
                _write_mask(ALL)
                ;
        }
    };
    const auto depth_write_mask = x3::no_case[depth_write_mask_()];
    #undef _write_mask
    #define _comparison_func(name) (#name, D3D11_COMPARISON_##name)
    struct comparision_func_ : x3::symbols<D3D11_COMPARISON_FUNC>
    {
        comparision_func_()
        {
            add
                _comparison_func(NEVER)
                _comparison_func(LESS)
                _comparison_func(EQUAL)
                _comparison_func(LESS_EQUAL)
                _comparison_func(GREATER)
                _comparison_func(NOT_EQUAL)
                _comparison_func(GREATER_EQUAL)
                _comparison_func(ALWAYS)
                ;
        }
    } comparison_func;
    #undef _comparison_func
    struct fill_ : x3::symbols<D3D11_FILL_MODE>
    {
        fill_()
        {
            add
                ("WIREFRAME", D3D11_FILL_WIREFRAME)
                ("SOLID", D3D11_FILL_SOLID)
                ;
        }
    };
    const auto fill = x3::no_case[fill_()]; 
    struct cull_ : x3::symbols<D3D11_CULL_MODE>
    {
        cull_()
        {
            add
                ("NONE", D3D11_CULL_NONE)
                ("FRONT", D3D11_CULL_FRONT)
                ("BACK", D3D11_CULL_BACK)
                ;
        }
    };
    const auto cull = x3::no_case[cull_()]; 

    struct shader_type_ : x3::symbols<ast::shader_type>
    {
        shader_type_()
        {
            add
                ("VertexShader", ast::kVertexShader)
                ("PixelShader", ast::kPixelShader)
                ("GeometryShader", ast::kGeometryShader)
                ;
        }
    } shader_type;

    const auto bool_ = x3::no_case[x3::bool_]; 
}}

namespace client { namespace parser {
    using namespace assign_operator; 
    namespace x3 = boost::spirit::x3;
    namespace ascii = boost::spirit::x3::ascii;
    namespace sym = client::symbol;

    using x3::lit;
    using x3::lexeme;

    using ascii::char_;
    using ascii::string;

    auto const identifier = x3::lexeme[+(x3::alnum | char_('_'))];

    auto const comment_line = "//" >> *(char_ -  x3::eol) >> x3::eol;
    auto const comment_block = "/*" >> *(char_ - "*/") >> "*/";
    auto const space_comment = x3::space | comment_line | comment_block;
    auto const skip = space_comment;
    auto const slot = 
           x3::lit(":") 
        >> "register"
        >> "("
        >> x3::omit[x3::alpha]
        >> x3::uint32 
        >> ")";

    x3::rule<class sampler_state, ast::sampler_state> const 
        sampler_state = "sampler_state";
    x3::rule<class sampler_descriptor, D3D11_SAMPLER_DESC> const
        sampler_descriptor = "sampler_descriptor";
    auto const sampler_descriptor_def =
        +(
                lit("Filter") >> '=' >> sym::filter[a<0>] >> ';'
            |   lit("AddressU") >> '=' >> sym::adress[a<1>] >> ';'
            |   lit("AddressV") >> '=' >> sym::adress[a<2>] >> ';'
            |   lit("AddressW") >> '=' >> sym::adress[a<3>] >> ';'
            |   lit("MipLODBias") >> '=' >> x3::float_[a<4>] >> ';'
            |   lit("MaxAnisotropy") >> '=' >> x3::uint32[a<5>] >> ';'
        )
        ;
    auto const sampler_state_def = 
           "SamplerState"
        >> identifier
        >> slot
        >> '{' 
        >> sampler_descriptor
        >> '}'
        >> ";"
        ;

    x3::rule<class blend_state, ast::blend_state> const 
        blend_state = "blend_state";
    x3::rule<class blend_descriptor, D3D11_BLEND_DESC> const
        blend_descriptor = "blend_descriptor";

    auto const blend_descriptor_def =
        +(
                lit("AlphaToCoverageEnable") >> '=' >> sym::bool_[a<0>] >> ';'
            |   lit("IndependentBlendEnable") >> '=' >> sym::bool_[a<1>] >> ';'
            |   lit("BlendEnable") >> idx >> '=' >> sym::bool_[k<2,0>] >> ';'
            |   lit("SrcBlend") >> idx >> '=' >> sym::blend[k<2,1>] >> ';'
            |   lit("DestBlend") >> idx >> '=' >> sym::blend[k<2,2>] >> ';'
            |   lit("BlendOp") >> idx >> '=' >> sym::blend_op[k<2,3>] >> ';'
            |   lit("SrcBlendAlpha") >> idx >> '=' >> sym::blend[k<2,4>] >> ';'
            |   lit("DestBlendAlpha") >> idx >> '=' >> sym::blend[k<2,5>] >> ';'
            |   lit("BlendOpAlpha") >> idx >> '=' >> sym::blend_op[k<2,6>] >> ';'
            |   lit("RenderTargetWriteMask") >> idx >> '=' >> x3::uint8[k<2,7>] >> ';'
        )
        ;
    auto const blend_state_def =
           "BlendState"
        >> identifier
        >> '{'
        >> blend_descriptor
        >> '}'
        >> ';'
        ;

    x3::rule<class depth_stencil_state, ast::depth_stencil_state> const 
        depth_stencil_state = "depth_stencil_state";
    x3::rule<class depth_stencil_descriptor, D3D11_DEPTH_STENCIL_DESC> const
        depth_stencil_descriptor = "depth_stencil_descriptor";
    auto const depth_stencil_descriptor_def =
        +(
                lit("DepthEnable") >> '=' >> sym::bool_[a<0>] >> ';'
            |   lit("DepthWriteMask") >> '=' >> sym::depth_write_mask[a<1>] >> ';'
            |   lit("DepthFunc") >> '=' >> sym::comparison_func[a<2>] >> ';'
            |   lit("StencilEnable") >> '=' >> sym::bool_[a<3>] >> ';'
            |   lit("StencilReadMask") >> '=' >> x3::uint8[a<4>] >> ';'
            |   lit("StencilWriteMask") >> '=' >> x3::uint8[a<5>] >> ';'
        )
        ;
    auto const depth_stencil_state_def = 
           "DepthStencilState"
        >> identifier
        >> '{' 
        >> depth_stencil_descriptor 
        >> '}'
        >> ";"
        ;

    x3::rule<class rasterizer_state, ast::rasterizer_state> const 
        rasterizer_state = "rasterizer_state";
    x3::rule<class raster_desciptor, D3D11_RASTERIZER_DESC> const
        raster_desciptor = "raster_desciptor";
    auto const raster_desciptor_def =
        +(
                lit("FillMode") >> '=' >> sym::fill[a<0>] >> ';'
            |   lit("CullMode") >> '=' >> sym::cull[a<1>] >> ';'
            |   lit("FrontCounterClockwise") >> '=' >> sym::bool_[a<2>] >> ';'
            |   lit("DepthBias") >> '=' >> x3::int32[a<3>] >> ';'
            |   lit("DepthBiasClamp") >> '=' >> x3::float_[a<4>] >> ';'
            |   lit("SlopeScaledDepthBias") >> '=' >> x3::float_[a<5>] >> ';'
            |   lit("DepthClipEnable") >> '=' >> sym::bool_[a<6>] >> ';'
            |   lit("ScissorEnable") >> '=' >> sym::bool_[a<7>] >> ';'
            |   lit("MultisampleEnable") >> '=' >> sym::bool_[a<8>] >> ';'
            |   lit("AntialiasedLineEnable") >> '=' >> sym::bool_[a<9>] >> ';'
        )
        ;
    auto const rasterizer_state_def = 
           "RasterizerState"
        >> identifier
        >> '{' 
        >> raster_desciptor
        >> '}'
        >> ";"
        ;
    
    x3::rule<class shader_compiler_desc, ast::shader_compiler_desc> const
        shader_compiler_desc = "shader_compiler_desc";
    auto const shader_compiler_desc_def =
           sym::shader_type
        >> identifier
        >> '='
        >> "CompileShader"
        >> '('
        >> identifier
        >> ','
        >> identifier
        >> '('
        >> ')'
        >> ')'
        >> ';'
        ;

    auto const float4
        = x3::rule<class float4_, std::vector<float>>{}
        = lit("float4") >> '(' >> ((x3::float_ >> -x3::lit('f')) % ',') >> ')' ;
    auto const str2uint32 
        = x3::rule<class str2uint32_, uint32_t>{}
        = x3::lexeme[+x3::alnum][([](auto &ctx) {
            auto& str = _attr( ctx );
            _val( ctx ) = std::stoul( str, nullptr, 0 );
          })];

    x3::rule<class blend_config, ast::blend_config> const
        blend_config = "blend_config";
    auto const blend_config_def =
           lit("SetBlendState")
        >> '('
        >> identifier
        >> ','
        >> float4
        >> ','
        >> str2uint32
        >> ')'
        >> ';'
        ;

    x3::rule<class depth_stencil_config, ast::depth_stencil_config> const
        depth_stencil_config = "depth_stencil_config";
    auto const depth_stencil_config_def =
           lit("SetDepthStencilState")
        >> '('
        >> identifier
        >> ','
        >> x3::uint32
        >> ')'
        >> ';'
        ;

    auto const name = '(' >> identifier >> ')' >> ';';

    x3::rule<class rasterizer_config, ast::rasterizer_config> const
        rasterizer_config = "rasterizer_config";
    auto const rasterizer_config_def =
           lit("SetRasterizerState")
        >> name
        ;

    x3::rule<class vertex_config, ast::vertex_config> const
        vertex_config = "vertex_config";
    auto const vertex_config_def =
           lit("SetVertexShader")
        >> name
        ;

    x3::rule<class pixel_config, ast::pixel_config> const
        pixel_config = "pixel_config";
    auto const pixel_config_def =
           lit("SetPixelShader")
        >> name
        ;

    x3::rule<class geometry_config, ast::geometry_config> const
        geometry_config = "geometry_config";
    auto const geometry_config_def =
           lit("SetGeometryShader")
        >> name
        ;

    x3::rule<class pass_config, ast::pass_config> const
        pass_config = "pass_config";
    auto const pass_config_def =
          blend_config
        | depth_stencil_config
        | rasterizer_config
        | vertex_config
        | pixel_config
        | geometry_config
        ;

    x3::rule<class pass_desc, ast::pass_desc> const
        pass_desc = "pass_desc";
    auto const pass_desc_def =
           "pass"
        >> identifier
        >> '{'
        >> +pass_config
        >> '}'
        ;

    x3::rule<class technique_desc, ast::technique_desc> const
        technique_desc = "technique_desc";
    auto const technique_desc_def =
           "technique11"
        >> identifier
        >> '{'
        >> +pass_desc
        >> '}'
        ;

    x3::rule<class texture_text, std::string> const
        texture_text = "texture_text";
    auto const texture_text_def =
           x3::lexeme[x3::no_case["texture"]] 
        >> x3::no_skip[*(char_ - ';')]
        >> char_(';');

    x3::rule<class include_text, std::string> const
        include_text = "include_text";
    auto const include_text_def = 
           x3::lexeme["#include"] 
        >> x3::no_skip[*(char_ - x3::eol)]
        ;

    x3::rule<class struct_text, std::string> const
        struct_text = "struct_text";
    auto const struct_text_def = 
           x3::lexeme["struct"]
        >> x3::no_skip[+(char_ - '}')]
        >> char_('}')
        >> char_(';')
        ;

    x3::rule<class block> const
        block = "block";
    x3::rule<class function_text, std::string> const
        function_text = "function_text";
    auto const not_block = x3::no_skip[+(char_ - "{" - "}")];
    auto const block_def =
           char_('{')
        >> +(block | not_block)
        >> char_('}')
        ;
    auto const function_text_def = 
           -(char_('[') >> +(char_ - ']') >> char_(']'))
        >> identifier
        >> x3::no_skip[x3::space]
        >> identifier
        >> char_('(')
        >> x3::no_skip[+(char_ - ')')]
        >> char_(')')
        >> *(char_ - "{")
        >> block
        ;

    x3::rule<class buffer_text, std::string> const
        buffer_text = "buffer_text";
    auto const buffer_text_def =
           x3::lexeme["cbuffer"]
        // >> x3::no_skip[identifier]
        // >> x3::no_skip[char_(':')]
        // >> x3::lexeme["register"]
        >> x3::no_skip[+(char_ - '}')]
        >> char_('}')
        >> -char_(';')
        ;

    x3::rule<class statement, ast::statement> const 
        statement = "statement";
    auto const statement_def = 
          sampler_state
        | blend_state
        | depth_stencil_state
        | rasterizer_state
        | shader_compiler_desc
        | technique_desc
        | function_text
        | struct_text
        | include_text
        | texture_text
        | buffer_text
        ;
    x3::rule<class program, ast::program> const 
        program = "program";
    auto const program_def = +statement;

    BOOST_SPIRIT_DEFINE(
        sampler_state, sampler_descriptor,
        blend_state, blend_descriptor,
        depth_stencil_state, depth_stencil_descriptor,
        rasterizer_state, raster_desciptor,
        shader_compiler_desc,
        technique_desc,
        blend_config,
        depth_stencil_config,
        rasterizer_config,
        vertex_config, pixel_config, geometry_config,
        pass_config,
        pass_desc, 
        program,
        statement,
        block, 
        function_text, struct_text, include_text,
        texture_text, buffer_text);

    struct expression_class
    {
        //  Our error handler
        template <typename Iterator, typename Exception, typename Context>
        x3::error_handler_result
            on_error( Iterator&, Iterator const& last, Exception const& x, Context const& context )
        {
            std::cout
                << "Error! Expecting: "
                << x.which()
                << " here: \""
                << std::string( x.where(), last )
                << "\""
                << std::endl
                ;
            return x3::error_handler_result::fail;
        }
    };
}
}

bool FxParse(const std::string& Text, client::ast::program& Program) 
{
    std::string::const_iterator iter = Text.begin();
    std::string::const_iterator end = Text.end();
    namespace x3 = boost::spirit::x3;
    using namespace client;

    x3::phrase_parse(iter, end, parser::program, parser::skip, Program);
    ASSERT(iter == Text.end());
    return iter == Text.end();
}