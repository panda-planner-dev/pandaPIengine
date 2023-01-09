#include "Model.h"
#include <fstream>
#include <string.h>
#include <unordered_map>
#include <algorithm>

class Verifier {
    public:
        Verifier(string htnFile, string planFile) {
            this->readHTNFile(htnFile);
            vector<string> planStr = this->readPlanFile(planFile);
            this->plan = parsePlan(planStr);
        }

        virtual bool getResult() {return this->result;}

    protected:
        Model *htn;
        vector<int> plan;
        bool result;

    private:
        void readHTNFile(string htnFile) {
            cout << "read model from" << htnFile << "\n";
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
            this->htn = new Model(trackContainedTasks, reachability, true, true);
            this->htn->filename = htnFile;
            this->htn->read(inputStream);
            cout << "reading file completed" << endl;
        }

        vector<int> parsePlan(vector<string> planStr) {
            vector<int> planNum;
            unordered_map<string, int> taskToIndex;
            for (int i = 0; i < this->htn->numTasks; i++) {
                string taskName = this->htn->taskNames[i];
                std::transform(taskName.begin(), taskName.end(), taskName.begin(), ::tolower);
                taskToIndex.insert({taskName, i});
            }
            for (int i = 0; i < plan.size(); i++) {
                if (!taskToIndex.count(planStr[i])) {
                    std::cerr << "Plan contains unreachable actions, plan is not a solution" << endl;
                    exit(-1);
                }
                planNum.push_back(taskToIndex[planStr[i]]);
            }
            return planNum;
        }

        vector<string> readPlanFile(string planFile) {
            vector<string> plan;
            ifstream fin(planFile);
            if(!fin.good()) {
                std::cerr << "Unable to open input file " << planFile << ": " << strerror (errno) << std::endl;
                exit(-1);
            }
            string action;
            while(std::getline(fin, action, ';')) {
                action.erase(std::remove(action.begin(), action.end(), '\n'), action.end());
                std::transform(action.begin(), action.end(), action.begin(), ::tolower);
                plan.push_back(action);
            }
            return plan;
        }
};