/*
 * IntStack.cpp
 *
 *  Created on: 30.09.2017
 *      Author: Daniel Höller
 */

#include "IntStack.h"

namespace progression {

IntStack::IntStack() {
	// TODO Auto-generated constructor stub

}

IntStack::~IntStack() {
	delete[] this->intContainer;
}

void IntStack::init(int size) {
	this->intContainer = new int[size];
	this->containerSize = size;
	this->currentSize = 0;
}

int IntStack::getSize() {
	return currentSize;
}

void IntStack::push(int i) {
	intContainer[currentSize++] = i;
}

int IntStack::pop() {
	if (currentSize == 0)
		return -1;
	return intContainer[--currentSize];
}
bool IntStack::isEmpty() {
	return currentSize == 0;
}

void IntStack::clear() {
	currentSize = 0;
}

int IntStack::getFirst() {
	if (currentSize == 0)
		return -1;
	iterI = 0;
	return intContainer[iterI];
}

int IntStack::getNext() {
	iterI++;
	if (iterI < currentSize)
		return intContainer[iterI];
	else
		return -1;
}

} /* namespace progression */
