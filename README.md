# cp-profiler
Profiling and Visualisation for Constraint Programming

Pre-release binaries for Linux are available here:
  https://github.com/cp-profiler/cp-profiler/releases/tag/0.1

Dependencies:

  * 5.4.x ≤ Qt < 5.6.x

Linux/Mac:

    mkdir build && cd build
    qmake .. && make


### How to use:
1. Start the profiler

  (from *cp-profiler/build* directory):

  `cp-profiler.app/Contents/MacOS/cp-profiler` (Mac)

2. Run a solver that supports the profiling protocol

  To use Gecode (from *gecode-profiling* directory):

    `tools/flatzinc/fzn-gecode my-model.fzn`

    Note: if you didn't `make install` Gecode, then add *gecode-profiling* directory to the shared libraries path like so:

    `LD_LIBRARY_PATH=$PWD tools/flatzinc/fzn-gecode my-model.fzn` (Linux)
    `DYLD_LIBRARY_PATH=$PWD tools/flatzinc/fzn-gecode my-model.fzn` (Mac)


3. The solver will be *sending* information about the execution in real time, which the profiler will use to incrementally *draw* the *search tree*.

Note: Because every new *node* can potentially cause the entire *search tree* layout to be recalculated (and slow down the drawing), it is recommended that ***display refresh rate*** is set to a reasonably high number (>1000), unless the rate at which the solver explores the nodes is low as well.

![Execution Manager](https://raw.githubusercontent.com/msgmaxim/profiler_pictures/master/profiler_menu.png "Execution Manager View")

Be default the **CP Profiler** will be constructing the search tree in the background. Selecting an execution from the list and clicking *show tree* will display the corresponding search tree (updating it in real time if the solver is still running.)

### Basic Search Tree Visualisation

![Search Tree Example](https://github.com/msgmaxim/profiler_pictures/raw/master/alpha_tree.png "Search Tree Example")


##### Changing *display refresh rate*
***Display refresh rate*** determines how many nodes should be received between any two consecutive updates of the search tree drawing.

The property is available under *Preferences...* sub-menu and its value will persist for any further solver executions.

### Different ways to display the search

[coming soon]

#### Hiding by size
#### Pixel Search Tree view

### Search Tree Comparison

In the current implementation the first and the second executions will be compared.