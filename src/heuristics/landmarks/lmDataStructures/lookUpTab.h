/*
 * lookUpTab.h
 *
 *  Created on: 12.02.2020
 *      Author: dh
 */

#ifndef HEURISTICS_LANDMARKS_LOOKUPTAB_H_
#define HEURISTICS_LANDMARKS_LOOKUPTAB_H_

#include "LmMap.h"

namespace progression {

class lookUpTab {
public:
	lookUpTab();
	lookUpTab(int size);
	virtual ~lookUpTab();

	int size = 0;
	LmMap** lookFor = nullptr;
	int refCounter = 1;
	int indexOf(int value);

	void printTab();

private:
	int indexOf(int low, int high, int value);
};

} /* namespace progression */

#endif /* HEURISTICS_LANDMARKS_LOOKUPTAB_H_ */
