#include <iostream>
#include <string>

#include "debug.h"

static bool debugMode = false;

std::string color (Color color, std::string text)
{
#ifndef NDEBUG
	return std::string ()
		+ "\033[" + std::to_string (30 + color) + "m"
		+ text
		+ "\033[m"
	;
#else
	return text;
#endif
}

bool getDebugMode (void)
{
	return debugMode;
}

void setDebugMode (bool enabled)
{
#ifdef DEBUG_MODE
	debugMode = enabled;
#else
	if (enabled)
	{
		std::cerr << "Tried to enable debug mode, but the program was built with debugging disabled." << std::endl;
	}
#endif
}
