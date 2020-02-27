/*
 * bIntSet.cpp
 *
 *  Created on: 05.10.2017
 *      Author: Daniel Höller
 */

#include "bIntSet.h"
#include <climits>
#include <iostream>
#include <stdlib.h>
using namespace std;

namespace progression {

bIntSet::bIntSet() {
	// TODO Auto-generated constructor stub

}

void bIntSet::init(int size) {
	this->boolContainer = new bool[size];
	this->intContainer = new int[size];
	this->containerSize = size;
	this->currentSize = 0;
	for (int i = 0; i < containerSize; i++) {
		boolContainer[i] = false;
	}
}

bIntSet::~bIntSet() {
	delete[] boolContainer;
	delete[] intContainer;
}

void bIntSet::insert(int i) {
	if (boolContainer[i] == false) {
		boolContainer[i] = true;
		intContainer[currentSize++] = i;
		sort(intContainer, 0, currentSize - 1);
	}
}

void bIntSet::append(int i) {
	if (boolContainer[i] == false) {
		boolContainer[i] = true;
		intContainer[currentSize++] = i;
	}
}

void bIntSet::erase(int i) {
	if (boolContainer[i] == true) {
		boolContainer[i] = false;
		int indexOfI = find(intContainer, 0, currentSize - 1, i);
		intContainer[indexOfI] = INT_MAX;
		sort(intContainer, 0, currentSize - 1);
		currentSize--;
	}
}

bool bIntSet::isEmpty() {
	return currentSize == 0;
}

void bIntSet::clear() {
	for (int i = getFirst(); i >= 0; i = getNext()) {
		boolContainer[i] = false;
	}
	currentSize = 0;
}

int bIntSet::getSize() {
	return currentSize;
}

int bIntSet::getFirst() {
	if (currentSize == 0)
		return -1;
	iterI = 0;
	return intContainer[iterI];
}

int bIntSet::getNext() {
	iterI++;
	if (iterI < currentSize)
		return intContainer[iterI];
	else
		return -1;
}

bool bIntSet::get(int i) {
	return boolContainer[i];
}

void bIntSet::sortSet() {
	sort(intContainer, 0, currentSize - 1);
}

void bIntSet::sort(int* ints, int minIndex, int maxIndex) {
	if (minIndex < maxIndex) {
		int p = partition(ints, minIndex, maxIndex);
		sort(ints, minIndex, p);
		sort(ints, p + 1, maxIndex);
	}
}

int bIntSet::partition(int* ints, int minIndex, int maxIndex) {
	int pivot = ints[(minIndex + maxIndex) / 2];
	int i = minIndex - 1;
	int j = maxIndex + 1;
	int swap;
	while (true) {
		do {
			i++;
		} while (ints[i] < pivot);

		do {
			j--;
		} while (ints[j] > pivot);

		if (i >= j) {
			return j;
		}

		swap = ints[i];
		ints[i] = ints[j];
		ints[j] = swap;
	}
}

int bIntSet::find(int* ints, int minIndex, int maxIndex, int element) {
	while (true) {
		if (minIndex > maxIndex) {
			return -1;
		}
		int i = (minIndex + maxIndex) / 2;
		if (ints[i] == element)
			return i;
		else if (ints[i] < element) {
			minIndex = i + 1;
		} else {
			maxIndex = i - 1;
		}
	}
}

} /* namespace progression */
