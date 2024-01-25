//
// Created by u6162630 on 4/18/23.
//
#include "catch.hpp"
#include "depth.h"
#include "fstream"

TEST_CASE("TEST DEPTH SIMPLE") {
    string htnFile = "/home/users/u6162630/Projects/ongoing/pandaPIengine/src/verifier/test/test-task.sas";
    std::ifstream *fileInput = new std::ifstream(htnFile);
    if(!fileInput->good()) {
        std::cerr << "Unable to open input file " << htnFile << ": " << strerror (errno) << std::endl;
        exit(-1);
    }
    std::istream * inputStream;
    inputStream = fileInput;

    bool useTaskHash = true;
    /* Read model */
    // todo: the correct value of maintainTaskRechability depends on the heuristic
    eMaintainTaskReachability reachability = mtrALL;
    bool trackContainedTasks = useTaskHash;
    Model *htn = new Model();
    htn = new Model(
            trackContainedTasks,
            reachability,
            true,
            true);
    htn->filename = htnFile;
    htn->read(inputStream);
    cout << "reading htn file completed" << endl;
    Depth *depth = new Depth(htn, 3);
    REQUIRE(depth->get(5, 3) == 2);
}