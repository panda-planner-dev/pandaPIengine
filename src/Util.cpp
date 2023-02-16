#include "Util.h"
#include <cstring>
#include <iomanip>
#include <iostream>

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

int parseLine(char* line){
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    const char* p = line;
    while (*p <'0' || *p > '9') p++;
    line[i-3] = '\0';
    i = atoi(p);
    return i;
}

int getValue(){ //Note: this value is in KB!
    FILE* file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmSize:", 7) == 0){
            result = parseLine(line);
            break;
        }
    }
    fclose(file);
    return result;
}

int getValue2(){ //Note: this value is in KB!
    FILE* file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmRSS:", 6) == 0){
            result = parseLine(line);
            break;
        }
    }
    fclose(file);
    return result;
}



int lastMemory;

void printMemory(){
	int now = getValue();
	std::cout << "\t\t\t\t\t\t\t\t\t\t\t\t\t" << color(Color::BLUE,"+MEM ") << std::setw(6) << (now - lastMemory) << " total: " << std::setw(6) << now/1024 <<  std::endl;
	lastMemory = now;
}


