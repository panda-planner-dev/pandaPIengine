/*
 * UnsortedFringe.cpp
 *
 *  Created on: 18.12.2018
 *      Author: Daniel Höller
 */

#include "UnsortedFringe.h"
#include <cassert>

namespace progression {

UnsortedFringe::UnsortedFringe() {
	firstContainer = new UnsrtFringeContainer(currentID++);
	firstContainer->leftIndex = firstContainer->containerSize / 2;
	firstContainer->rightIndex = firstContainer->leftIndex + 1;
	lastContainer = firstContainer;
}

UnsortedFringe::~UnsortedFringe() {
	UnsrtFringeContainer* temp;
	while (firstContainer->hasPrevContainer) {
		temp = firstContainer;
		firstContainer = firstContainer->prevContainer;
		delete temp;
	}
	delete firstContainer;
}

void UnsortedFringe::addFirst(searchNode* n) {
	firstContainer->content[firstContainer->leftIndex] = n;
	firstContainer->leftIndex--;
	sizeCounter++; // this is the global size
	if (firstContainer->leftIndex == -1) {
		UnsrtFringeContainer* temp = new UnsrtFringeContainer(currentID++);
		temp->nextContainer = this->firstContainer;
		this->firstContainer->prevContainer = temp;
		this->firstContainer->hasPrevContainer = true;
		temp->hasNextContainer = true;
		temp->leftIndex = temp->containerSize - 1; // this is the index that will be used without decrementation
		temp->rightIndex = temp->containerSize; // this will be decremented by the remove function before usage
		this->firstContainer = temp;
	}
}

void UnsortedFringe::addLast(searchNode* n) {
	lastContainer->content[lastContainer->rightIndex] = n;
	lastContainer->rightIndex++;
	sizeCounter++; // this is the global size
	if (lastContainer->rightIndex == lastContainer->containerSize) {
		UnsrtFringeContainer* temp = new UnsrtFringeContainer(currentID++);
		temp->prevContainer = this->lastContainer;
		this->lastContainer->nextContainer = temp;
		this->lastContainer->hasNextContainer = true;
		temp->hasPrevContainer = true;
		temp->leftIndex = -1; // this will be incremented by the remove function before usage
		temp->rightIndex = 0; // this is the index that will be used without incrementation
		this->lastContainer = temp;
	}
}

bool UnsortedFringe::empty() {
	assert(
			(sizeCounter > 0)
					|| ((firstContainer->containerID
							== lastContainer->containerID)
							&& (lastContainer->leftIndex
									== lastContainer->rightIndex -1)));
	return (sizeCounter == 0);
}

searchNode* UnsortedFringe::removeFirst() {
	if (this->empty()) {
		return nullptr;
	}
	firstContainer->leftIndex++;
	if (firstContainer->leftIndex == firstContainer->containerSize) { // container empty, but there is another one
		UnsrtFringeContainer* temp = firstContainer;
		assert(firstContainer->hasNextContainer);
		firstContainer = firstContainer->nextContainer;
		assert(firstContainer->leftIndex == -1);
		firstContainer->leftIndex++;
		firstContainer->hasPrevContainer = false;
		delete temp;
	}
	sizeCounter--;
	return firstContainer->content[firstContainer->leftIndex];
}

searchNode* UnsortedFringe::removeLast() {
	if (this->empty()) { // fringe is empty
		return nullptr;
	}
	lastContainer->rightIndex--;
	if (lastContainer->rightIndex == -1) { // container empty, but there is another one
		UnsrtFringeContainer* temp = lastContainer;
		assert(lastContainer->hasPrevContainer);
		lastContainer = lastContainer->prevContainer;
		lastContainer->hasNextContainer = false;
		assert(lastContainer->rightIndex == lastContainer->containerSize);
		lastContainer->rightIndex--;
		delete temp;
	}
	sizeCounter--;
	return lastContainer->content[lastContainer->rightIndex];
}

int UnsortedFringe::size() {
	return sizeCounter;
}

} /* namespace progression */
