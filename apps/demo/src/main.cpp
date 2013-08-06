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
                // You are right. This is overriden just for educational purposes.
                return BaseApp::Run(argc, argv);
            }
		}; // AppDemo
	}; // namespace app
}; // namespace fr

int main(int argc, char** argv)
{
    return fr::app::AppDemo().Run(argc, argv);
}
