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

        virtual int Run(int argc, char** argv) = 0;
		
	protected:
		// Container for program options
        po::variables_map Args;

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
                
	}; // class BaseApp
}; // namespace fr

#endif // LIBFR_PROGRAM_H
