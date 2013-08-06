#include <cstdio>
#include <cstdlib>
#include <iostream>

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

                ParseOptions(argc, argv, desc, this->Args);

                // Print help and exit if needed
                if (Args.count("help"))
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
