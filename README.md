# More Background Information

We've put together a website with the history of all planning systems of the PANDA family, links to all relevant software projects, and further background information including pointers explaining the techniques deployed by the respective systems.

- You find it on https://panda-planner-dev.github.io/
- or, as a forward, on http://panda.hierarchical-task.net

## pandaPIengine

pandaPIengine is a versatile HTN planning engine. To use the engine, you first need to parse and ground the planning problem. The capabilities are provided by other components: the [pandaPIparser](https://github.com/panda-planner-dev/pandaPIparser) and [pandaPIgrounder](https://github.com/panda-planner-dev/pandaPIgrounder)

### Compiling pandaPIengine

To compile pandaPIengine, you need to have gengetopt (tested with version 2.23) install. To compile pandaPIengine, you need to perform the following commands:

```
mkdir build
cd build
cmake ../src
make -j
```
By default pandaPIengine is build **without** support for ILP-based heuristics, the SAT planner, and the BDD planner.
For the latter two, you can pass the arguments `-DSAT=ON` or `-DBDD=ON`. For the ILP-based heuristics, you need an installation of IBM CPLEX and need to specify it through `-DCPLEX_SOURCE_DIR=PATH_TO_YOUR_CPLEX`.

### Using pandaPIengine

If you have a grounded HTN planning problem as a *.sas file, you can simply run `build/pandaPIengine FILE.sas`. pandaPIengine has a reasonable default configuration (Greedy best first search with the RC-FF heuristic and visited lists). If you want to customise pandaPIenigne, please run `build/pandaPIengine -h` to see the available options.

The simplest way to use the full pandaPI stack is the following -- assuming that your domain file is `domain.hddl` and your problem file is `problem.hddl`.

```
./pandaPIparser domain.hddl problem.hddl domain-problem.htn
./pandaPIgrounder domain-problem.htn domain-problem.sas
./pandaPIengine domain-problem.sas
```

*Note* pandaPIgrounder changes the model, i.e., it adds and combines methods, and adds and changes actions. The plan that the pandaPIengine finds is valid with respect to that changed model. This means the plan that pandaPIengine finds is not valid with respect to the original HDDL model that was put into the parser.

To obtain a valid plan w.r.t. the original HDDL model, you need to translate the plan back to the original model. You can do this using the pandaPIparser. For that you have to write the plan that is produced by the pandaPIengine to a file and then convert it. The simplest way to do this is the following:

```
./pandaPIengine domain-problem.sas | tee plan.original
./pandaPIparser -c plan.original plan.actual
```

The file `plan.actual` will then contain the valid HDDL-compliant plan.


### A Simple Script for Running the Planner (All Components)

As described above, you'll need to run the pandaPIengine based on an SAS file produced by the grounder, which is in term based on a file produced by a parser. We've created a simple script (*problemSolver.sh*) that calls the entire chain of (the involved three) executables, and all the output produced is piped into appropriately named files. 

Note that this script does not use any of the many parameters for each of the executables. It is merely intended as a starting point for your experiments. Also note that some default messages (by the grounder) are printed to std.err, which is therefore also piped into a file. So if an *actual* error occurs now, you won't see (so you might want to change the script/piping behavior). In fact, using that script you won't see any output on the terminal (except for errors by the parser or the engine, since those std.errs are not piped away.) After all, it's just a convenient starting point.

The script has three parameters: 
- the domain file
- the problem file
- a new filename that will be used as prefix for all the produced files.
