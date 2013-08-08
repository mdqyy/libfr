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

        virtual int Run(int argc, const char** argv)
        {
            InitOptions();

            ParseOptions(argc, argv);

            return ProcessOptions();
        }
		
	protected:
        // Argc
        int Argc;

        // Argv
        const char** Argv;

		// Available program options;
		po::options_description Opts;

		// Container for parsed program options
        po::variables_map Args;


        virtual void InitOptions()
        {
            // Specify program options
            Opts.add_options()
                ("help,h", "produce help message")
                ;
        }

        virtual void ParseOptions(int argc, const char** argv)
        {
            Argc = argc;
            Argv = argv;

            // Try to parse program options
            try
            {
                po::store(po::parse_command_line(argc, argv, Opts), Args);
                po::notify(Args);
            }
            catch(const std::exception& e)
            {
                std::cout << "Unable to parse program options, reason: " << e.what() << std::endl;
                std::cout << Opts << std::endl;
            }
        }

        virtual int ProcessOptions()
        {
            // Print help and exit if needed
            if (Args.count("help"))
            {
                std::cout << Opts << std::endl;
                return EXIT_SUCCESS;
            }

            // Exit successfuly ...
            return EXIT_SUCCESS;
        }
                
	}; // class BaseApp
}; // namespace fr

#endif // LIBFR_PROGRAM_H
