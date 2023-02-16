/*
 * IntUtil.h
 *
 *  Created on: 30.01.2020
 *      Author: dh
 */

#ifndef INTDATASTRUCTURES_INTUTIL_H_
#define INTDATASTRUCTURES_INTUTIL_H_

#include "../ProgressionNetwork.h"

namespace progression {

class IntUtil {
public:
	IntUtil();
	virtual ~IntUtil();
	void sort(int* ints, int minIndex, int maxIndex);
	int indexOf(int* sortedSet, int low, int high, int value);

	bool isSorted(int* list, int size);

	bool containsInt(int* sortedList, int lowI, int highI, int value);
	bool containsKey(int** keySortedMap, int lowI, int highI, int value);
	int* copyExcluding(int* inList, int size, int exclude);

	int makeSet(int* set, int size);
private:
	int partition(int* ints, int minIndex, int maxIndex);
};

} /* namespace progression */

#endif /* INTDATASTRUCTURES_INTUTIL_H_ */
