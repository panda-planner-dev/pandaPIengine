//
// Created by u6162630 on 4/17/23.
//
#include "distribution.h"

int LengthDistributions::generate(int total, int row, int col) {
    if (col == this->numCols - 1) {
        this->distribution[row] = new int[this->numCols];
        this->distribution[row][col] = total;
        return 1;
    } else if (total == 0) {
        this->distribution[row] = new int[this->numCols];
        for (int c = col; c < this->numCols; c++)
            this->distribution[row][c] = 0;
        return 1;
    } else {
        int size = 0;
        int r = row;
        for (int source = total; source >= 0; source--) {
            int remain = total - source;
            int numGenerated = this->generate(
                    remain,
                    r,
                    col + 1);
            size += numGenerated;
            int count = 0;
            while (count < numGenerated) {
                this->distribution[r][col] = source;
                count++, r++;
            }
        }
        return size;
    }
}
