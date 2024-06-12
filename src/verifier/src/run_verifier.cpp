#include <iostream>
#include <string>
#include "cmdline.h"
#include "verifier.h"
#include "sat_verifier.h"

int main(int argc, char *argv[]) {
    gengetopt_args_info args_info;
	if (cmdline_parser(argc, argv, &args_info) != 0) return 1;

    string htnFile = args_info.htn_arg;
    string planFile = args_info.plan_arg;
    std::clock_t beforeVerify = std::clock();
    bool optimizeDepth = args_info.optimizeDepth_given;
    Verifier *verifier = new SATVerifier(htnFile, planFile, optimizeDepth);
    std::clock_t afterVerify = std::clock();
    double prepTime = 1000.0 * (afterVerify - beforeVerify) / CLOCKS_PER_SEC;
    cout << "Information about the verification" << endl;
    cout << "- Time for verifying the plan: " << prepTime << endl;
    bool result = verifier->getResult();

    if (result) {
        cout << "The given plan is a solution" << endl;
    }
    else {
        cout << "The given plan is not a solution" << endl;
    }

    return 0;
}