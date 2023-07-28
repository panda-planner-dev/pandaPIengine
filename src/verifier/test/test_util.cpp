//
// Created by u6162630 on 4/17/23.
//
#include "catch.hpp"
#include "distribution.h"

TEST_CASE("TEST LENGTH DISTRIBUTIONS") {
    LengthDistributions *distributions = new LengthDistributions(5, 3);
    int groundTruth[21][3] = {
            {5, 0, 0},
            {4, 1, 0},
            {4, 0, 1},
            {3, 2, 0},
            {3, 1, 1},
            {3, 0, 2},
            {2, 3, 0},
            {2, 2, 1},
            {2, 1, 2},
            {2, 0, 3},
            {1, 4, 0},
            {1, 3, 1},
            {1, 2, 2},
            {1, 1, 3},
            {1, 0, 4},
            {0, 5, 0},
            {0, 4, 1},
            {0, 3, 2},
            {0, 2, 3},
            {0, 1, 4},
            {0, 0, 5}
    };
    REQUIRE(distributions->numDistributions() == 21);
    int pos = 0;
    for(;;) {
        int* distribution = distributions->next();
        if (distribution == nullptr) break;
        int* ans = groundTruth[pos];
        for (int i = 0; i < 3; i++)
            REQUIRE(distribution[i] == ans[i]);
        pos++;
    }
}

TEST_CASE("TEST LENGTH DISTRIBUTIONS ONE SLOT") {
    LengthDistributions *distributions = new LengthDistributions(5, 1);
    int groundTruth[1][1] = {{5}};
    REQUIRE(distributions->numDistributions() == 1);
    int* distribution = distributions->next();
    REQUIRE(distribution[0] == groundTruth[0][0]);
}
