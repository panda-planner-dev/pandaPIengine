#include <getopt.h>
#include <iostream>
#include <fstream>
#include <string>
#include "cmdline.h"
#include "to_verifier.h"

int main(int argc, char *argv[]) {
    gengetopt_args_info args_info;
	if (cmdline_parser(argc, argv, &args_info) != 0) return 1;

    string htnFile = args_info.htn_arg;
    string planFile = args_info.plan_arg;
    string selectedVerifier = args_info.verifier_arg;

    Verifier *verifier;

    verifier = new TOVerifier(htnFile, planFile); // delete in the future
    std::clock_t beforePrep = std::clock();
    if (selectedVerifier.compare("to-verifier") == 0) {
        verifier = new TOVerifier(htnFile, planFile);
    } else if (selectedVerifier.compare("sat-verifier") == 0) {
        // verifier = new SATVerifier(htnFile);
        cout << "Function to be delivered" << endl;
        exit(-1);
    } else if (selectedVerifier.compare("sat-verifier-icaps17") == 0) {
        // verifier = new SATVerifierTable(htnFile);
        cout << "Function to be delivered" << endl;
        exit(-1);
    } else if (selectedVerifier.compare("cyk-verifier") == 0) {
        // verifier = new CYKVerifier(htnFile);
        cout << "Function to be delivered" << endl;
        exit(-1);
    }
    // TOVerifier *verifier = new TOVerifier(htnFile);
    std::clock_t afterPrep = std::clock();
    double prepTime = 1000.0 * (afterPrep - beforePrep) / CLOCKS_PER_SEC;
    cout << "Information about the verification" << endl;
    cout << "- Time for verifying the plan: " << prepTime << endl;

    // bool result = verifier->verify(planFile);
    // std::clock_t beforeVerify = std::clock();
    bool result = verifier->getResult();
    // std::clock_t afterVerify = std::clock();
    // double verifyTime = 1000.0 * (afterVerify - beforeVerify) / CLOCKS_PER_SEC;
    // cout << "- Time for verifying the plan: " << verifyTime << endl;

    if (result) {
        cout << "The given plan is a solution" << endl;
    }
    else {
        cout << "The given plan is not a solution" << endl;
    }

    return 0;
}