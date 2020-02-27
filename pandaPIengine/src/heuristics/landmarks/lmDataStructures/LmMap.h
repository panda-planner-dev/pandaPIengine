/*
 * LmMap.h
 *
 *  Created on: 12.02.2020
 *      Author: dh
 */

#ifndef HEURISTICS_LANDMARKS_LMMAP_H_
#define HEURISTICS_LANDMARKS_LMMAP_H_

namespace progression {

// class to track landmarks, containing a mapping from a
// fact/task/method to the indices of landmarks it is contained in
class LmMap {
public:
	LmMap();
	LmMap(int key, int size);
	virtual ~LmMap();

	int entry = -1; // fact/task/method
	int size = -1;
	int* containedInLMs = nullptr;
	int refCounter = 1;
};
} /* namespace progression */

#endif /* HEURISTICS_LANDMARKS_LMMAP_H_ */
