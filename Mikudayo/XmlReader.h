#pragma once

#include <vector>

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
    struct mini_xml;
    typedef boost::variant<boost::recursive_wrapper<mini_xml>, std::string> mini_xml_node;

    struct mini_xml
    {
        std::string name;
        std::vector<mini_xml_node> children;
    };
    bool Parse( const std::string& Text, client::ast::mini_xml& Program );
}
}