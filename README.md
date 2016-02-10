# cp-profiler
Profiling and Visualisation for Constraint Programming

Pre-release binaries for Linux are available here:
  https://github.com/cp-profiler/cp-profiler/releases/tag/0.1

Dependencies:

  * Qt >=5.4.x

Linux/Mac:

    mkdir build && cd build
    qmake .. && make


### How to use it:
1. Start the profiler

  (from *cp-profiler/build* directory):

  `cp-profiler.app/Contents/MacOS/cp-profiler` (Mac)

2. Run the modified solver

  To use Gecode (from *gecode-profiling* directory):

    `tools/flatzinc/fzn-gecode my-model.fzn`

    Note: if you didn't `make install` Gecode, then add *gecode-profiling* directory to the shared libraries path like so:

    `LD_LIBRARY_PATH=$PWD tools/flatzinc/fzn-gecode my-model.fzn` (Linux)
    `DYLD_LIBRARY_PATH=$PWD tools/flatzinc/fzn-gecode my-model.fzn` (Mac)


3. The solver will be *sending* and the profiler *receiving* and incrementally *drawing* the *search tree* in real time.

Note: Because every new *node* can potentially cause the entire *search tree* layout to be recalculated (and slow down the drawing), it is recommended that ***display refresh rate*** is set to a reasonably high number (>1000), unless real-time drawing is required.



##### Changing *display refresh rate*
***Display refresh rate*** determines how many nodes should be received between any two consecutive updates of the search tree drawing.

The property is available in *StandaloneGist* -> *Preferences...* sub-menu (`⌘,` or `ctr + ,` shortcut).

### Different ways to display the search

[coming soon]

#### Hiding by size
#### Pixel Search Tree view

### Search Tree Comparison

In the current implementation the first and the second executions will be compared.