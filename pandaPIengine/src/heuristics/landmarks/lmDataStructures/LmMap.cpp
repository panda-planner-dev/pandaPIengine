/*
 * LmMap.cpp
 *
 *  Created on: 12.02.2020
 *      Author: dh
 */

#include "LmMap.h"

namespace progression {

LmMap::LmMap() {
	// TODO Auto-generated constructor stub
}

LmMap::LmMap(int key, int size) {
	this->entry = key;
	this->size = size;
	this->containedInLMs = new int[size];
}

LmMap::~LmMap() {
	// TODO Auto-generated destructor stub
}

} /* namespace progression */
