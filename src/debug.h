#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#ifndef NDEBUG
#define DEBUG_MODE
#endif

/**
 * @defgroup debug Debugging Functions
 * @brief Contains functions for printing and formatting debug information.
 *
 * @{
 */

#include <string>

enum Color
{
	BLACK   = 0,
	RED     = 1,
	GREEN   = 2,
	YELLOW  = 3,
	BLUE    = 4,
	MAGENTA = 5,
	CYAN    = 6,
	WHITE   = 7,
};

/**
 * @brief Wraps a string in color escape codes.
 */
std::string color (Color color, std::string text);

/**
 * @brief Returns true if debug mode is enabled.
 */
bool getDebugMode (void);

/**
 * @brief Enables or disables debug mode.
 *
 * If the program was built without DEBUG_MODE, and enabled is set to true,
 * a warning message will be printed to cerr, and debug mode will not be enabled.
 */
void setDebugMode (bool enabled);

/**
 * @brief Macro that only executes its content if debug mode is enabled.
 */
#ifdef DEBUG_MODE
# define DEBUG(x) do { if (getDebugMode ()) { x; } } while (0)
#else
# define DEBUG(x)
#endif

/**
 * @}
 */

#endif
