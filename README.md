# More Background Information

We've put together a website with the history of all planning systems of the PANDA family, links to all relevant software projects, and further background information including pointers explaining the techniques deployed by the respective systems.

- You find it on https://panda-planner-dev.github.io/
- or, as a forward, on http://panda.hierarchical-task.net

## pandaPIengine

pandaPIengine is a versatile HTN planning engine. To use the engine, you first need to parse and ground the planning problem. The capabilities are provided by other components: the [pandaPIparser](https://github.com/panda-planner-dev/pandaPIparser) and [pandaPIgrounder](https://github.com/panda-planner-dev/pandaPIgrounder)

### Compiling pandaPIengine

To compile pandaPIengine perform the following commands:

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
