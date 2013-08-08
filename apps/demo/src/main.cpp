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
            /* virtual */ void InitOptions()
            {
                // Specify program options
                Opts.add_options()
                    ("help,h", "produce help message")
                    ("gui,g", "show gui")
                    ;
            }

            /* virtual */ int Run(int argc, const char** argv)
            {
                // You are right. This is overriden just for educational purposes.
                return BaseApp::Run(argc, argv);
            }

            /* virtual */ int ProcessOptions()
            {
                // GUI
                if(Args.count("gui"))
                {
                    QApplication app(Argc, const_cast<char**>(Argv));

                    QWidget window;

                    window.resize(250, 150);
                    window.setWindowTitle("Simple example");
                    window.show();

                    return app.exec();
                }

                // Let base process it ...
                return BaseApp::ProcessOptions();
            }
		}; // AppDemo
    } // namespace app
} // namespace fr

int main(int argc, const char** argv)
{
    return fr::app::AppDemo().Run(argc, argv);
}
