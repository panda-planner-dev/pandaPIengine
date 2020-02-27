/*
 * UnsrtFringeContainer.cpp
 *
 *  Created on: 18.12.2018
 *      Author: dh
 */

#include "UnsrtFringeContainer.h"

namespace progression {

UnsrtFringeContainer::UnsrtFringeContainer(int id) {
	content = new searchNode*[containerSize];
	this->containerID = id;

}

UnsrtFringeContainer::~UnsrtFringeContainer() {
	delete[] content;
}

} /* namespace progression */
