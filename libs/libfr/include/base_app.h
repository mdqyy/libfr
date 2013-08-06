#ifndef LIBFR_PROGRAM_H
#define LIBFR_PROGRAM_H

#include <boost/program_options.hpp>
namespace po = boost::program_options;

namespace fr {
	class BaseApp
	{
	public:
		BaseApp();
		virtual ~BaseApp();

        virtual int Run(int argc, char** argv)
        {
            InitOptions(this->Opts);

            ParseOptions(argc, argv, this->Opts, this->Args);

            return ProcessOptions();
        }
		
	protected:
		// Available program options;
		po::options_description Opts;

		// Container for parsed program options
        po::variables_map Args;


        static void InitOptions(po::options_description& desc)
        {
            // Specify program options
            desc.add_options()
                ("help,h", "produce help message")
                ;
        }

        static void ParseOptions(int argc, char** argv, const po::options_description& desc, po::variables_map& vm) 
        {
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
                }
        }

        virtual int ProcessOptions(void)
        {
            // Print help and exit if needed
            if (Args.count("help"))
            {
                std::cout << this->Opts << std::endl;
                return EXIT_SUCCESS;
            }

            // Exit successfuly ...
            return EXIT_SUCCESS;
        }
                
	}; // class BaseApp
}; // namespace fr

#endif // LIBFR_PROGRAM_H
