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

#define INTPAD 4
#define PATHPAD 15
#define STRINGPAD 0

std::string path_string(std::vector<int> & path);
std::string pad_string(std::string s, int chars = STRINGPAD);
std::string pad_int(int i, int chars = INTPAD);
std::string pad_path(std::vector<int> & path, int chars = PATHPAD);


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

void reset_number_of_clauses();
int get_number_of_clauses();

void assertYes(void* solver, int i);
void assertNot(void* solver, int i);

void implies(void* solver, int i, int j);
void impliesNot(void* solver, int i, int j);
void impliesOr(void* solver, int i, std::vector<int> & j);
void impliesPosAndNegImpliesOr(void* solver, int i, int j, std::vector<int> & k);
void impliesAllNot(void* solver, int i, std::vector<int> & j);
void atMostOne(void* solver, sat_capsule & capsule, std::vector<int> & is);
void atLeastOne(void* solver, sat_capsule & capsule, std::vector<int> & is);


#endif
