/*
 * IntPairHeap.h
 *
 *  Created on: 29.09.2017
 *      Author: Daniel Höller
 */

#ifndef INTPAIRHEAP_H_
#define INTPAIRHEAP_H_

#include <limits>
#include <cstring>
#include <iostream>

namespace progression {

    template<class T>
    class IntPairHeap {
    private:
        T *keys;
        int *vals;

        int nextIndex;
        int arraySize;
    public:
        IntPairHeap(int size) {
            this->nextIndex = 1;
            this->arraySize = size;
            this->keys = new T[size];
            this->vals = new int[size];
            this->keys[0] = std::numeric_limits<T>::min();
            this->vals[0] = std::numeric_limits<int>::min();
        }

        virtual ~IntPairHeap() {
            delete[] this->keys;
            delete[] this->vals;
        }

        bool isEmpty() {
            return nextIndex == 1;
        }

        int size() {
            return (nextIndex - 1);
        }

        void clear() {
            nextIndex = 1;
        }

        void add(T key, int val) {
            if (nextIndex == arraySize) {
                int *nVals = new int[arraySize * 2];
                T *nKeys = new T[arraySize * 2];
                memcpy(nVals, vals, sizeof(int) * arraySize);
                memcpy(nKeys, keys, sizeof(T) * arraySize);
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
                T swap = keys[current];
                keys[current] = keys[current / 2];
                keys[current / 2] = swap;
                int swap2 = vals[current];
                vals[current] = vals[current / 2];
                vals[current / 2] = swap2;
                current /= 2;
            }
            nextIndex++;
        }

        void pop() {
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
                    T swap = keys[current];
                    keys[current] = keys[child];
                    keys[child] = swap;
                    int swap2 = vals[current];
                    vals[current] = vals[child];
                    vals[child] = swap2;
                    current = child;
                } else
                    swapped = false;
            }
        }

        int topVal() {
            return vals[1];
        }

        T topKey() {
            return keys[1];
        }
    };

} /* namespace progression */

#endif /* INTPAIRHEAP_H_ */
