/*
 * IntUtil.cpp
 *
 *  Created on: 30.01.2020
 *      Author: Daniel Höller
 */

#include <set>
#include "IntUtil.h"

namespace progression {

    IntUtil::IntUtil() {
        // There should be no kind of state in this class! Put it in the model
    }

    IntUtil::~IntUtil() {}

    void IntUtil::sort(int *ints, int minIndex, int maxIndex) {
        if (minIndex < maxIndex) {
            int p = partition(ints, minIndex, maxIndex);
            sort(ints, minIndex, p);
            sort(ints, p + 1, maxIndex);
        }
    }

    int IntUtil::partition(int *ints, int minIndex, int maxIndex) {
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

    int IntUtil::indexOf(int *sortedSet, int low, int high, int value) {
        while (low <= high) {
            int mid = low + ((high - low) / 2);
            if (sortedSet[mid] == value)
                return mid;
            else if (sortedSet[mid] > value) {
                high = mid - 1;
            } else {
                low = mid + 1;
            }
        }
        return -1;
    }

    bool IntUtil::containsInt(int *sortedSet, int low, int high, int value) {
        return (this->indexOf(sortedSet, low, high, value) >= 0);
    }

    int *IntUtil::copyExcluding(int *inList, int size, int exclude) {
        int *outList = new int[size - 1];
        int outIndex = 0;
        for (int i = 0; i < size; i++) {
            if (inList[i] != exclude) {
                outList[outIndex++] = inList[i];
            }
        }
        return outList;
    }

    bool IntUtil::isSorted(int *list, int size) {
        if (size <= 1)
            return true;
        for (int i = 1; i < size; i++) {
            if (list[i - 1] > list[i])
                return false;
        }
        return true;
    }

    int IntUtil::makeSet(int *set, int size) {
        if (size <= 1)
            return size;

        this->sort(set, 0, size -1);
        bool redo = false;
        for (int i = 1; i < size; i++) {
            if (set[i - 1] == set[i]) {
                redo = true;
                break;
            }
        }
        if (redo) {
            std::set<int>* temp = new std::set<int>();
            for(int i = 0; i < size; i++) {
                temp->insert(set[i]);
            }
            int i = 0;
            for(int e : *temp) {
                set[i++] = e;
            }
            for(; i < size; i++) {
                set[i] = -1;
            }
            int res = temp->size();
            delete temp;
            return res;
        } else {
            return size;
        }
    }
} /* namespace progression */
