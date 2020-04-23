#ifndef sat_encoder_h_INCLUDED
#define sat_encoder_h_INCLUDED

#include <map>
#include <vector>
#include <string>

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


struct sat_capsule{
	int number_of_variables;

	int new_variable();

#ifndef NDEBUG
	std::map<int,std::string> variableNames;
	void registerVariable(int v, std::string name);
	void printVariables();
#endif

	sat_capsule();
};


void implies(void* solver, int i, int j);
void impliesOr(void* solver, int i, std::vector<int> & j);


#endif
