/*
 * StringUtil.h
 *
 *  Created on: 09.02.2020
 *      Author: dh
 */

#ifndef INTDATASTRUCTURES_STRINGUTIL_H_
#define INTDATASTRUCTURES_STRINGUTIL_H_

#include <string>
#include <locale>

using namespace std;

namespace progression {

class StringUtil {
public:
	StringUtil();
	virtual ~StringUtil();

	string toLowerString(string str);
	string cleanStr(string s);
	string getStrX(string s, int i);
};

} /* namespace progression */

#endif /* INTDATASTRUCTURES_STRINGUTIL_H_ */
