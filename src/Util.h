#ifndef util_h_INCLUDED
#define util_h_INCLUDED

#include <ostream>
#include <string>

void printIndent(int indent, std::ostream & out);

void printIndentMark(int indent, int mark, std::ostream & out);


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

void printMemory();

#endif
