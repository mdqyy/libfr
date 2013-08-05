#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <libfr.h>

namespace fr {
	namespace app {
        class AppDemo : public BaseApp
		{
        public:
            int Run(int argc, char** argv)
            {
                // Specify program options
                po::options_description desc("Allowed options");
                    desc.add_options()
                        ("help,h", "produce help message")
                        ;

                po::variables_map vm;

                // Try to parse program options
                try
                {
                    po::store(po::parse_command_line(argc, argv, desc), vm);
                    po::notify(vm);
                }
                catch(const std::exception& e)
                {
                    std::cout << "Unable to parse program options, reason: " << e.what() << std::endl;
                    std::cout << desc << std::endl;
                    return EXIT_FAILURE;
                }

                // Print help and exit if needed
                if (vm.count("help"))
                {
                    std::cout << desc << std::endl;
                    return EXIT_SUCCESS;
                }

                // Exit successfuly ...
                return EXIT_SUCCESS;
            }
		}; // AppDemo
	}; // namespace app
}; // namespace fr

int main(int argc, char** argv)
{
    return fr::app::AppDemo().Run(argc, argv);
}
