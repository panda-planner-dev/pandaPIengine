/*
 * hsFilter.cpp
 *
 *  Created on: 19.12.2018
 *      Author: dh
 */

#include "hsFilter.h"

namespace progression {

hsFilter::hsFilter(Model* sas) {
	this->m = sas;
	this->add = new hsAddFF(sas);
}

hsFilter::~hsFilter() {
	delete add;
}
int hsFilter::getHeuristicValue(bucketSet& s, noDelIntSet& g) {
	if (add->getHeuristicValue(s, g) != UNREACHABLE) {
		return 0;
	} else {
		return UNREACHABLE;
	}
}

} /* namespace progression */
