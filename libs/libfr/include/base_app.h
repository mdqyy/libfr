#ifndef LIBFR_PROGRAM_H
#define LIBFR_PROGRAM_H

namespace fr {
	class BaseApp
	{
	public:
		BaseApp();
		virtual ~BaseApp();

        virtual int Run(int argc, char** argv) = 0;
		
	}; // class BaseApp
}; // namespace fr

#endif // LIBFR_PROGRAM_H
