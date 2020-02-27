/*
 * lookUpTab.cpp
 *
 *  Created on: 12.02.2020
 *      Author: dh
 */

#include "lookUpTab.h"
#include <iostream>

using namespace std;

namespace progression {

lookUpTab::lookUpTab() {
	// TODO Auto-generated constructor stub

}

lookUpTab::lookUpTab(int size) {
	this->size = size;
	this->lookFor = new LmMap*[size];
}

lookUpTab::~lookUpTab() {
	// TODO Auto-generated destructor stub
}

int lookUpTab::indexOf(int value) {
	return this->indexOf(0, this->size - 1, value);
}

int lookUpTab::indexOf(int low, int high, int value) {
	while (low <= high) {
        int mid = low + ((high - low) / 2);
        if (this->lookFor[mid]->entry == value)
            return mid;
        else if (this->lookFor[mid]->entry > value) {
                high = mid - 1;
        } else {
        	low = mid + 1;
        }
    }
    return -1;
}

void lookUpTab::printTab() {
	for(int i = 0; i < this->size; i++) {
		LmMap* map = lookFor[i];
		cout << map->entry << " -> {";
		for(int j = 0; j < map->size; j++) {
			if(j > 0)
				 cout << ", ";
			cout << map->containedInLMs[j];
		}
		cout << "}" << endl;
	}
}


} /* namespace progression */
