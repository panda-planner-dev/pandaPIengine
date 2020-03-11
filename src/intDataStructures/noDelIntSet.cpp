/*
 * noDelSet.cpp
 *
 *  Created on: 30.09.2017
 *      Author: Daniel Höller
 */

#include "noDelIntSet.h"
#include <cassert>
#include <iostream>

using namespace std;

namespace progression {

noDelIntSet::noDelIntSet() {
}

void noDelIntSet::init(int size) {
	this->boolContainer = new bool[size];
	this->intContainer = new int[size];
	this->containerSize = size;
	this->currentSize = 0;
	for (int i = 0; i < containerSize; i++) {
		boolContainer[i] = false;
	}
}

noDelIntSet::~noDelIntSet() {
	delete[] boolContainer;
	delete[] intContainer;
}

noDelIntSet* noDelIntSet::clone() {
	noDelIntSet* that = new noDelIntSet();
	that->init(this->containerSize);
	for (int i = this->getFirst(); i >= 0; i = this->getNext()) {
		that->insert(i);
	}
	return that;
}

noDelIntSet* noDelIntSet::setUnion(noDelIntSet* second) {
	noDelIntSet* lessBuckets; // this is the number of buckets (Wertebereich), not the actual size!
	noDelIntSet* moreBuckets;
	if (this->containerSize > second->containerSize) {
		lessBuckets = second;
		moreBuckets = this;
	} else {
		lessBuckets = this;
		moreBuckets = second;
	}
	noDelIntSet* that = moreBuckets->clone();
	for (int i = lessBuckets->getFirst(); i >= 0; i = lessBuckets->getNext()) {
		that->insert(i);
	}
	return that;
}

void noDelIntSet::setUnion(noDelIntSet* result, noDelIntSet* second) {
	assert(result->containerSize >= this->containerSize);
	assert(result->containerSize >= second->containerSize);
	result->clear();

	for (int i = this->getFirst(); i >= 0; i = this->getNext()) {
		result->insert(i);
	}

	for (int i = second->getFirst(); i >= 0; i = second->getNext()) {
		result->insert(i);
	}
}

noDelIntSet* noDelIntSet::setIntersection(noDelIntSet* second) {
	int numBuckets;
	if (this->containerSize > second->containerSize) {
		numBuckets = this->containerSize;
	} else {
		numBuckets = second->containerSize;
	}

	noDelIntSet* lessElements;
	noDelIntSet* moreElements;
	if (this->currentSize > second->currentSize) {
		lessElements = second;
		moreElements = this;
	} else {
		lessElements = this;
		moreElements = second;
	}

	noDelIntSet* that = new noDelIntSet();
	that->init(numBuckets);

	for (int i = lessElements->getFirst(); i >= 0; i =
			lessElements->getNext()) {
		if ((moreElements->containerSize <= i) && (moreElements->get(i)))
			that->insert(i);
	}
	return that;
}

void noDelIntSet::setIntersection(noDelIntSet* result, noDelIntSet* second) {
	assert(result->containerSize >= this->containerSize);
	assert(result->containerSize >= second->containerSize);
	result->clear();

	noDelIntSet* lessElements;
	noDelIntSet* moreElements;
	if (this->currentSize > second->currentSize) {
		lessElements = second;
		moreElements = this;
	} else {
		lessElements = this;
		moreElements = second;
	}

	for (int i = lessElements->getFirst(); i >= 0; i =
			lessElements->getNext()) {
		if ((moreElements->containerSize <= i) && (moreElements->get(i)))
			result->insert(i);
	}
}

noDelIntSet* noDelIntSet::setMinus(noDelIntSet* second) {
	int numBuckets;
	if (this->containerSize > second->containerSize) {
		numBuckets = this->containerSize;
	} else {
		numBuckets = second->containerSize;
	}

	noDelIntSet* that = new noDelIntSet();
	that->init(numBuckets);

	for (int i = this->getFirst(); i >= 0; i = this->getNext()) {
		if ((second->containerSize <= i) || (!second->get(i)))
			that->insert(i);
	}
	return that;
}

void noDelIntSet::setMinus(noDelIntSet* result, noDelIntSet* second) {
	assert(result->containerSize >= this->containerSize);
	assert(result->containerSize >= second->containerSize);
	result->clear();

	for (int i = this->getFirst(); i >= 0; i = this->getNext()) {
		if ((second->containerSize <= i) || (!second->get(i)))
			result->insert(i);
	}
}

void noDelIntSet::insert(int i) {
	assert(i < this->containerSize);
	if (boolContainer[i] == false) {
		boolContainer[i] = true;
		intContainer[currentSize++] = i;
	}
}

bool noDelIntSet::isEmpty() {
	return currentSize == 0;
}

void noDelIntSet::clear() {
	for (int i = getFirst(); i >= 0; i = getNext()) {
		boolContainer[i] = false;
	}
	currentSize = 0;
}

int noDelIntSet::getSize() {
	return currentSize;
}

int noDelIntSet::getFirst() {
	if (currentSize == 0)
		return -1;
	iterI = 0;
	return intContainer[iterI];
}

int noDelIntSet::getNext() {
	iterI++;
	if (iterI < currentSize)
		return intContainer[iterI];
	else
		return -1;
}

bool noDelIntSet::get(int i) {
	assert(i < this->containerSize);
	return boolContainer[i];
}

    void noDelIntSet::sort() {
        iu.sort(this->intContainer, 0, currentSize - 1);
    }

} /* namespace progression */
