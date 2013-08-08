#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <libfr.h>

#include <QApplication>
#include <QWidget>

namespace fr {
	namespace app {
        class AppDemo : public BaseApp
		{
        public:
            int Run(int argc, char** argv)
            {
                // You are right. This is overriden just for educational purposes.
                return BaseApp::Run(argc, argv);
            }
		}; // AppDemo
	}; // namespace app
}; // namespace fr

int main(int argc, char** argv)
{
    // Specify program options
    po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "produce help message")
            ("gui,g", "show gui")
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

    if(vm.count("gui"))
    {
        QApplication app(argc, argv);

        QWidget window;

        window.resize(250, 150);
        window.setWindowTitle("Simple example");
        window.show();

        return app.exec();
    }

    return fr::app::AppDemo().Run(argc, argv);
}
