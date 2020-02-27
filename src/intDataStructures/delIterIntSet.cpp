/*
 * delInterIntSet.cpp
 *
 *  Created on: 01.02.2019
 *      Author: dh
 */

#include "delIterIntSet.h"

#include <cassert>
#include <iostream>

namespace progression {

delIterIntSet::delIterIntSet() {

}

delIterIntSet::~delIterIntSet() {
	// TODO Auto-generated destructor stub
}

void delIterIntSet::init(int size) {
	this->boolContainer = new bool[size];
	this->containerSize = size;
	this->currentSize = 0;
	for (int i = 0; i < containerSize; i++) {
		boolContainer[i] = false;
	}
}

int delIterIntSet::getSize() {
	return currentSize;
}

void delIterIntSet::insert(int i) {
	assert(i < this->containerSize);
	if (boolContainer[i] == false) {
		boolContainer[i] = true;
		llInt* elem = new llInt();
		elem->value = i;
		if (currentSize == 0) {
			elem->isFirst = true;
			elem->isLast = true;
			this->first = elem;
			this->last = elem;
		} else {
			elem->next = this->last;
			last->prev = elem;
			this->last = elem;
		}
		currentSize++;
	}
}

bool delIterIntSet::isEmpty() {
	return (currentSize == 0);
}

void delIterIntSet::clear() {
	int i = this->getFirst();
	while (i >= 0) {
		i = delCurrentGetNext();
	}
}

int delIterIntSet::getFirst() {
	if (this->isEmpty())
		return -1;
	this->currentIterElem = first;
	return this->currentIterElem->value;
}

int delIterIntSet::getNext() {
	if (this->isEmpty())
		return -1;
	else if (this->currentIterElem == nullptr) {
		return -1;
	} else if (this->currentIterElem->isLast) {
		this->currentIterElem = nullptr;
		return -1;
	} else {
		this->currentIterElem = this->currentIterElem->next;
		return this->currentIterElem->value;
	}
}

int delIterIntSet::delCurrentGetNext() {
	if (this->isEmpty())
		return -1;
	else if (this->currentIterElem == nullptr) {
		return -1;
	} else if (this->currentIterElem->isFirst) {
		boolContainer[this->currentIterElem->value] = false;
		llInt* elem = this->currentIterElem;
		if (currentSize > 1) {
			this->currentIterElem->next->isFirst = true;
			this->first = this->currentIterElem->next;
		} else {
			this->first = nullptr;
			this->last = nullptr;
		}
		delete elem;
		currentSize--;
		return -1;
	} else if (this->currentIterElem->isLast) {
		boolContainer[this->currentIterElem->value] = false;
		assert(currentSize > 1); // otherwise the last case would be true
		llInt* elem = this->currentIterElem;
		this->currentIterElem->prev->isLast = true;
		this->last = this->currentIterElem->prev;
		delete elem;
		currentSize--;
		return -1;
	} else {
		boolContainer[this->currentIterElem->value] = false;
		llInt* elem = this->currentIterElem;
		this->currentIterElem->prev->next = this->currentIterElem->next;
		this->currentIterElem->next->prev = this->currentIterElem->prev;
		this->currentIterElem = this->currentIterElem->next;
		delete elem;
		currentSize--;
		return this->currentIterElem->value;
	}
}

bool delIterIntSet::get(int i) {
	assert(i < this->containerSize);
	return boolContainer[i];
}

} /* namespace progression */
