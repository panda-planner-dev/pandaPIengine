/*
 * IntPairHeap.cpp
 *
 *  Created on: 29.09.2017
 *      Author: Daniel Höller
 */

#include "IntPairHeap.h"
#include <limits>
#include <cstring>
#include <iostream>

using namespace std;

namespace progression {

IntPairHeap::IntPairHeap(int size) {
	this->nextIndex = 1;
	this->arraySize = size;
	this->keys = new int[size];
	this->vals = new int[size];
	this->keys[0] = std::numeric_limits<int>::min();
	this->vals[0] = std::numeric_limits<int>::min();
}

IntPairHeap::~IntPairHeap() {
	delete[] this->keys;
	delete[] this->vals;
}

void IntPairHeap::clear() {
	nextIndex = 1;
}

bool IntPairHeap::isEmpty() {
	return nextIndex == 1;
}

int IntPairHeap::size() {
	return (nextIndex - 1);
}

void IntPairHeap::add(int key, int val) {
	if (nextIndex == arraySize) {
		int* nVals = new int[arraySize * 2];
		int* nKeys = new int[arraySize * 2];
		memcpy(nVals, vals, sizeof(int) * arraySize);
		memcpy(nKeys, keys, sizeof(int) * arraySize);
		arraySize = arraySize * 2;
		delete[] vals;
		delete[] keys;
		vals = nVals;
		keys = nKeys;
	}

	keys[nextIndex] = key;
	vals[nextIndex] = val;

	int current = nextIndex;
	while (keys[current] < keys[current / 2]) {
		int swap = keys[current];
		keys[current] = keys[current / 2];
		keys[current / 2] = swap;
		swap = vals[current];
		vals[current] = vals[current / 2];
		vals[current / 2] = swap;
		current /= 2;
	}
	nextIndex++;
}

int IntPairHeap::topVal() {
	return vals[1];
}

int IntPairHeap::topKey() {
	return keys[1];
}

void IntPairHeap::pop() {
	keys[1] = keys[nextIndex - 1];
	vals[1] = vals[nextIndex - 1];
	nextIndex--;

	int current = 1;
	bool swapped = true;
	while (swapped) {
		int child = current * 2;
		if (child >= nextIndex)
			break;

		if ((current * 2 + 1 < nextIndex)
				&& (keys[current * 2 + 1] < keys[current * 2]))
			child++;
		if (keys[child] < keys[current]) {
			swapped = true;
			int swap = keys[current];
			keys[current] = keys[child];
			keys[child] = swap;
			swap = vals[current];
			vals[current] = vals[child];
			vals[child] = swap;
			current = child;
		} else
			swapped = false;
	}
}

}
/* namespace progression */
