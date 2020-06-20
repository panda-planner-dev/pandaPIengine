/*
 * StringUtil.cpp
 *
 *  Created on: 09.02.2020
 *      Author: dh
 */

#include "StringUtil.h"
#include <algorithm>

namespace progression {

StringUtil::StringUtil() {
	// TODO Auto-generated constructor stub

}

StringUtil::~StringUtil() {
	// TODO Auto-generated destructor stub
}

string StringUtil::toLowerString(string str){
	std::locale loc;
	string res = "";
	for (std::string::size_type i=0; i<str.length(); ++i)
		res += tolower(str[i],loc);
	return res;
}

string StringUtil::cleanStr(string s){
	std::string str = s;
	std::replace(str.begin(), str.end(), ',', '_');
	std::replace(str.begin(), str.end(), ';', '_');
	std::replace(str.begin(), str.end(), ']', '_');
	std::replace(str.begin(), str.end(), '[', '_');
	std::replace(str.begin(), str.end(), '|', '_');
	std::replace(str.begin(), str.end(), '!', '_');
	//std::replace(str.begin(), str.end(), '=', '-');
	//std::replace(str.begin(), str.end(), '_', '-');
	//std::replace(str.begin(), str.end(), '@', '-');
	std::replace(str.begin(), str.end(), '(', '_');
	std::replace(str.begin(), str.end(), ')', '_');
	std::replace(str.begin(), str.end(), '?', '_');
	std::replace(str.begin(), str.end(), '+', 'p');
	std::replace(str.begin(), str.end(), '-', 'm');
	std::replace(str.begin(), str.end(), '<', '_');
	std::replace(str.begin(), str.end(), '>', '_');

	if (str[0] == '_') return "x"+str;
	return str;
}

string StringUtil::getStrX(string s, int nr) {
	int i = 0;
	while(nr > 0) {
		if(s[i] == ' ')
			nr--;
		i++;
	}
	int start = i;
	while(s[i] != ' ') {
		i++;
	}
	return s.substr(start, i - start);
}

/*
int last = -1;
bool hasNext() {
	return (last > 0);
}

string getFirst(string s) {
	last = 0;
	for (int i = 0; i < s.length(); i++) {
		if (i == s.length() - 1){
			int from = last;
			last = -1;
			return s.substr(from, (i - from + 1));
		} else if (s[i] == ' ') {
			int from = last;
			last = i + 1;
			return s.substr(from, (i - from));
		}
	}
	return "";
}

string getNext(string s) {
	for (int i = last; i < s.length(); i++) {
		if (i == s.length() - 1){
			int from = last;
			last = -1;
			return s.substr(from, (i - from + 1)) ;
		} else if ((i == s.length() - 1) || (s[i] == ' ')){
			int from = last;
			last = i + 1;
			return s.substr(from, (i - from));
		}
	}
	return "";
}
*/
} /* namespace progression */
