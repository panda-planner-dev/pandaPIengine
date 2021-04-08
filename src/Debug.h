#ifndef debug_INCLUDED
#define debug_INCLUDED

#include "flags.h"

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


#ifndef NDEBUG
# define DEBUG(x) do { if (getDebugMode ()) { x; } } while (0)
#else
# define DEBUG(x)
#endif

#ifndef NTEST
# define TEST(x) do { x; } while (0)
#else
# define TEST(x)
#endif

#endif
