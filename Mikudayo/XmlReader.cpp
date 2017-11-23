#include "stdafx.h"
#include "XmlReader.h"

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

// We need to tell fusion about our mini_xml struct
// to make it a first-class fusion citizen
BOOST_FUSION_ADAPT_STRUCT(
    client::ast::mini_xml,
    (std::string, name)
    (std::vector<client::ast::mini_xml_node>, children)
)

namespace client { 
namespace parser {
    namespace x3 = boost::spirit::x3;
    namespace ascii = boost::spirit::x3::ascii;

    using x3::lit;
    using x3::lexeme;

    using ascii::char_;
    using ascii::string;

    auto const comment_line = "//" >> *(char_ -  x3::eol) >> x3::eol;
    auto const comment_block = "/*" >> *(char_ - "*/") >> "*/";
    auto const space_comment = x3::space | comment_line | comment_block;
    auto const skip = space_comment;

    x3::rule<class program, ast::mini_xml> const 
        mini_xml = "mini_xml";
    x3::rule<class text, std::string> const
        text = "text";
    auto const text_def = lexeme[+(char_ - '<')];
    x3::rule<class node, ast::mini_xml_node> const
        node = "node";
    auto const node_def = mini_xml | text;

    x3::rule<class start_tag, std::string> const 
        start_tag = "start_tag";
    auto const start_tag_def =
            '<'
        >>  !lit('/')
        >   lexeme[+(char_ - '>')]
        >   '>'
        ;

    x3::rule<class end_tag> const 
        end_tag = "end_tag";
    auto const end_tag_def = 
            "</"
        >   lexeme[+(char_ - '>')]
        >   '>'
        ;

    auto const mini_xml_def = 
            start_tag
        >   *node
        >   end_tag
        ;

    BOOST_SPIRIT_DEFINE(
        start_tag, end_tag,
        text, node,
        mini_xml
    )

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
} // paser

namespace ast {
    bool Parse( const std::string& Text, client::ast::mini_xml& Program )
    {
        std::string::const_iterator iter = Text.begin();
        std::string::const_iterator end = Text.end();
        namespace x3 = boost::spirit::x3;
        using namespace client;

        bool r = x3::phrase_parse( iter, end, parser::mini_xml, parser::skip, Program );
        bool success = r && (iter == Text.end() || *iter == '\0');
        ASSERT( success );
        return success;
    }
} // ast
}
