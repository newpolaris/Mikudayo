#include "stdafx.h"
#include "FxContainer.h"
#include "TextUtility.h"
#include <boost/program_options.hpp>

using namespace boost::program_options;

int main( int argc, const char *argv[] )
{
    for (int i = 0; i < argc; i++)
        std::cout << argv[i] << std::endl;
    try
    {
        options_description desc { "Options" };
        desc.add_options()
            ("fx", value<std::string>(), "fx file path");
        command_line_parser parser { argc, argv };
        parser.options( desc ).allow_unregistered().style(
            command_line_style::default_style |
            command_line_style::allow_slash_for_short );
        parsed_options parsed_options = parser.run();

        variables_map vm;
        store( parsed_options, vm );
        notify( vm );

        if (vm.count("fx") == 0)
            return -1;
        const std::string path = vm["fx"].as<std::string>();
        auto cont = std::make_shared<FxContainer>(Utility::MakeWStr(path));
        if (!(cont && cont->Load()))
            std::cerr << "Fail to generate cache: " << path << std::endl;
    }
    catch (const error &ex)
    {
        std::cerr << ex.what() << '\n';
    }
    return 0;
}