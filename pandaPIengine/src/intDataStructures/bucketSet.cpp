/*
 * bucketSet.cpp
 *
 *  Created on: 30.09.2017
 *      Author: Daniel Höller
 */

#include "bucketSet.h"
#include <cassert>

namespace progression {

bucketSet::bucketSet() {
}

void bucketSet::init(int size) {
	this->container = new bool[size];
	this->containersize = size;
	this->currentSize = 1; // do not set this to 0 !
	this->clear();
}

bucketSet::~bucketSet() {
	delete[] container;
}
void bucketSet::insert(int i) {
	assert(i < this->containersize);
	if (container[i] == false) {
		container[i] = true;
		currentSize++;
	}
}

void bucketSet::erase(int i) {
	assert(i < this->containersize);
	if (container[i] == true) {
		container[i] = false;
		currentSize--;
	}
}

bool bucketSet::isEmpty() {
	return currentSize == 0;
}

void bucketSet::clear() {
	if (currentSize > 0) {
		for (int i = 0; i < containersize; i++) {
			container[i] = false;
		}
		currentSize = 0;
	}
}

int bucketSet::getSize() {
	return currentSize;
}

int bucketSet::getFirst() {
	if (currentSize == 0)
		return -1;
	for (int i = 0; i < containersize; i++) {
		if (container[i]) {
			iIter = i + 1;
			numIterated = 1;
			return i;
		}
	}
	return -1;
}

int bucketSet::getNext() {
	if (numIterated == currentSize)
		return -1;
	for (int i = iIter; i < containersize; i++) {
		if (container[i]) {
			iIter = i + 1;
			numIterated++;
			return i;
		}
	}
	return -1;
}

int bucketSet::removeFirst() {
	if (currentSize == 0)
		return -1;
	for (int i = 0; i < containersize; i++) {
		if (container[i]) {
			currentSize--;
			container[i] = false;
			iIter = i + 1;
			return i;
		}
	}
	return -1;
}

int bucketSet::removeNext() {
	if (currentSize == 0)
		return -1;
	for (int i = iIter; i < containersize; i++) {
		if (container[i]) {
			iIter = i + 1;
			currentSize--;
			container[i] = false;
			return i;
		}
	}
	return -1;
}

bool bucketSet::get(int i) {
	assert(i < this->containersize);
	return container[i];
}

} /* namespace progression */
