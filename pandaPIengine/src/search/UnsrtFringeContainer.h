/*
 * UnsrtFringeContainer.h
 *
 *  Created on: 18.12.2018
 *      Author: Daniel Höller
 */

#ifndef UNSRTFRINGECONTAINER_H_
#define UNSRTFRINGECONTAINER_H_

#include "../ProgressionNetwork.h"

namespace progression {

class UnsrtFringeContainer {
public:
	UnsrtFringeContainer(int id);
	virtual ~UnsrtFringeContainer();
	searchNode** content;

	bool hasNextContainer = false;
	bool hasPrevContainer = false;
	UnsrtFringeContainer* nextContainer = nullptr;
	UnsrtFringeContainer* prevContainer = nullptr;
	int containerSize = 1024 * 1024;
	int containerID;

	int rightIndex = -1;
	int leftIndex = -1;
};

} /* namespace progression */

#endif /* UNSRTFRINGECONTAINER_H_ */
