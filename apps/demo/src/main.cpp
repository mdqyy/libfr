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
                InitOptions(this->Opts);

                ParseOptions(argc, argv, this->Opts, this->Args);

                return ProcessOptions();
            }
		}; // AppDemo
	}; // namespace app
}; // namespace fr

int main(int argc, char** argv)
{
    return fr::app::AppDemo().Run(argc, argv);
}
