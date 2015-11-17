#include <iostream>
#include <boost/program_options.hpp>

namespace options = boost::program_options;
using namespace std;

int
main (int argc, char *argv[])
{
        options::options_description desc (string (argv[0]).append(" options"));
        desc.add_options()
            ("h", "Display this message")
        ;
        options::variables_map args;
        options::store (options::command_line_parser (argc, argv).options (desc)
                        .style (options::command_line_style::default_style |
                                options::command_line_style::allow_long_disguise)
                        .run (), args);
        options::notify (args);
        if (args.count ("h"))
        {
            cout << desc << endl;
            return 0;
        }
}
