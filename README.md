# CP-Profiler
Profiling and Visualisation for Constraint Programming

Pre-release binaries for Linux are available here:
  https://github.com/cp-profiler/cp-profiler/releases/tag/0.1

## Table of Contents
- [Building from Source](#building-from-source)
- [How to Use](#how-to-use)
  - [Connecting with a Solver](#connecting-with-a-solver)
  - [Basic Search Tree Visualisation](#basic-search-tree-visualisation)
    - [Node Actions](#node-actions)
    - [Changing *display refresh rate*](#changing-display-refresh-rate)
  - [Alternative Ways of Displaying the Search](#alternative-ways-of-displaying-the-search)
    - [Hiding by size](#hiding-by-size)
    - [Pixel Tree view](#pixel-tree-view)
    - [Icicle Tree view](#icicle-tree-view)
  - [Analysis Techniques](#analysis-techniques)



### Building from Source

Dependencies:

  * 5.4.x ≤ Qt < 5.6.x

Compiling Linux/Mac:

    mkdir build && cd build
    qmake .. && make


### How to Use:
#### Connecting with a solver
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

#### Basic Search Tree Visualisation

![Search Tree Example](https://github.com/msgmaxim/profiler_pictures/raw/master/alpha_tree.png "Search Tree Example")

##### Node Actions
When a node is selected (a selected node will be displayed in yellow colour), some actions will become available for the user. These actions can be found under *Node* menu. Some these actions have keyboard shortcuts assigned. For example, pressing `L` on the keyboard will display search decisions for the subtree under the selected node, while `Shift+L` will display search decisions for nodes on the path from the root to the current node.

Some other common actions:

| Key       | Description               |
| :---------------: | ----------------------------------- |
| `F`       | Collapse descendant subtrees with no solutions |
| `H`       | Collapse the entire subtree |
| `U`       | Undo any collapsing done under selected node |

##### Changing *display refresh rate*
***Display refresh rate*** determines how many nodes should be received between any two consecutive updates of the search tree drawing.

The property is available under *Preferences...* sub-menu and its value will persist for any further solver executions.

#### Alternative Ways of Displaying the Search

##### Hiding by size



##### Pixel Tree view

In *Pixel Tree view* nodes are represented by *squares* (pixels), and edges only implicitly by the indentation between the *squares*. Parent nodes are placed immediately to the left of their subtree and the leaves of each subtree are grouped to the rightmost position.

The image below shows a correspondence between nodes in a *traditional view* and those in a *Pixel Tree* view. Note that a *green vertical line* indicates a *solution* found at the corresponding node.

![Pixel Tree View Basic Example](https://raw.githubusercontent.com/msgmaxim/profiler_pictures/master/basic_pt.png "Pixel Tree View Basic Example")

One of the main advantages of the pixel tree view is the ability to **compress it while preserving large scale patterns** and thus providing a good overview of the search. The compression is done by simply allowing multiple nodes on the same horizontal position.

The example below shows a tree with around *100K* nodes with *compression* set to `500` (`500` nodes for every horizontal position).

![Pixel Tree View Larger Example](https://raw.githubusercontent.com/msgmaxim/profiler_pictures/master/pixel_tree.png "Pixel Tree View Larger Example")

TODO: talk about histograms underneath

This view is available under *Tree* submenu.

##### Icicle Tree view
todo

#### Analysis Techniques
##### Similar Subtree Analysis
##### Search Tree Comparison

The following example demonstrates a comparison of two executions of the *Golomb Ruler* problem with 5 marks: one with a *symmetry breaking constraint* and one without. Here the two executions have been captured by the profiler and their search tree visualised (currently visualising is required prior to comparing).


![Two executions](https://raw.githubusercontent.com/msgmaxim/profiler_pictures/master/comparison_conductor.png "Two executions")


![Golomb 5](https://raw.githubusercontent.com/msgmaxim/profiler_pictures/master/golomb5.png "Golomb 5")
![Golomb 5 with symmetry breaking](https://raw.githubusercontent.com/msgmaxim/profiler_pictures/master/golomb5_sym_brk.png "Golomb 5 with symmetry breaking")

Clicking **compare trees** button from *execution conductor* menu will initiate the comparison of the two selected executions.

![Merged Tree](https://raw.githubusercontent.com/msgmaxim/profiler_pictures/master/golomb5merged.png "Merged Tree")

The resulting **merged** tree will consist of:
- the part that is common for both executions

- (multiple) *pentagon subtrees* indicated by pentagons ![Pentagon Image](https://raw.githubusercontent.com/msgmaxim/profiler_pictures/master/pentagon_icon.png "pentagon image")

A *pentagon* is found where the two executions diverge. It is a parent node for two subtrees:  one for each of the executions compared.

 The two subtrees represent exploration of the same search space. For example, the right-most pentagon on the image above has one failure node on the right that corresponds to the execution with a symmetry breaking constraint. Without the constraint, the solver had to do significantly more work as indicated by the highlighted subtree on the left.

**Pentagon list** option under *Analysis* submenu will bring up a list of pentagons. Each pentagon is displayed along with the information about the size of the subtrees it contains (*left* subtree, and *right* subtree). The list is sorted by the difference in the number of nodes each pair of subtrees contain. Clicking on a row will navigate to the corresponding pentagon node on the tree view.