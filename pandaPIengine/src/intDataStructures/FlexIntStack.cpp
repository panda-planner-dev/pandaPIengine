/*
 * FlexIntStack.cpp
 *
 *  Created on: 13.12.2018
 *      Author: dh
 */

#include "FlexIntStack.h"
#include <cstring>

namespace progression {

FlexIntStack::FlexIntStack() {
	// TODO Auto-generated constructor stub

}

FlexIntStack::~FlexIntStack() {
	delete[] this->intContainer;
}

void FlexIntStack::init(int size) {
	this->intContainer = new int[size];
	this->containerSize = size;
	this->currentSize = 0;
}

int FlexIntStack::getSize() {
	return currentSize;
}

void FlexIntStack::push(int i) {
	if (currentSize == containerSize) {
		int* newContainer = new int[containerSize * 2];
		memcpy(newContainer, intContainer, sizeof(int) * containerSize);
		containerSize = containerSize * 2;
		delete[] intContainer;
		intContainer = newContainer;
	}

	intContainer[currentSize++] = i;
}

int FlexIntStack::pop() {
	if (currentSize == 0)
		return -1;
	return intContainer[--currentSize];
}
bool FlexIntStack::isEmpty() {
	return currentSize == 0;
}

void FlexIntStack::clear() {
	currentSize = 0;
}

int FlexIntStack::getFirst() {
	if (currentSize == 0)
		return -1;
	iterI = 0;
	return intContainer[iterI];
}

int FlexIntStack::getNext() {
	iterI++;
	if (iterI < currentSize)
		return intContainer[iterI];
	else
		return -1;
}
} /* namespace progression */
