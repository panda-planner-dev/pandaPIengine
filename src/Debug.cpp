#include "Debug.h"
#include <iostream>

static bool debugMode = false;


bool getDebugMode (void)
{
	return debugMode;
}

void setDebugMode (bool enabled)
{
	debugMode = enabled;
#ifndef NDEBUG
	debugMode = enabled;
#else
	if (enabled)
	{
		std::cerr << "Tried to enable debug mode, but the program was built with debugging disabled." << std::endl;
	}
#endif
}


