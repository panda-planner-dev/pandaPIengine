/*
 * LmFdConnector.h
 *
 *  Created on: 09.02.2020
 *      Author: dh
 */

#ifndef HEURISTICS_LANDMARKS_LMFDCONNECTOR_H_
#define HEURISTICS_LANDMARKS_LMFDCONNECTOR_H_

#include <list>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include "../../../Model.h"
#include "../../../intDataStructures/StringUtil.h"
#include "../../rcHeuristics/RCModelFactory.h"

namespace progression {

class LmFdConnector {
public:
	LmFdConnector();
	virtual ~LmFdConnector();

	void createLMs(Model* htn);

	int numLMs = -1;
	int numConjunctive = -1;
	landmark** landmarks = nullptr;

	int getNumLMs();
	landmark** getLMs();

private:
	StringUtil su;

	void readFDLMs(string f, RCModelFactory* factory);
	int getIndex(string f, Model* rc);
};

} /* namespace progression */

#endif /* HEURISTICS_LANDMARKS_LMFDCONNECTOR_H_ */
