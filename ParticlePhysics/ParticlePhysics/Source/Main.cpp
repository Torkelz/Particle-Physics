#include "BaseApp.h"
#include "UserExceptions.h"

#include <windows.h>
#include <stdlib.h>
#include <iostream>

int main(int /*argc*/, char* /*argv*/[])
{
	try
	{
		BaseApp app;
		app.init();
		app.run();
		app.shutdown();
	}
	catch (std::exception& err)
	{
		std::cout << "Error: " << err.what() << std::endl;

		throw;
	}
	catch (...)
	{
		std::cout << "Unknown Error: " << std::endl;
		throw;
	}

	return EXIT_SUCCESS;
}