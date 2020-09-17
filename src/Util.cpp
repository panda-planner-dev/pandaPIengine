#include "Util.h"

void printIndent(int indent, std::ostream & out){
	for (int i = 0; i < indent; i++) out << " ";
}

void printIndentMark(int indent, int mark, std::ostream & out){
	for (int i = 0; i < indent; i++) {
		if (i % mark == mark - 1) out << "|";
		else out << " ";
	}

}


std::string color (Color color, std::string text)
{
	return std::string ()
		+ "\033[" + std::to_string (30 + color) + "m"
		+ text
		+ "\033[m"
	;
}


